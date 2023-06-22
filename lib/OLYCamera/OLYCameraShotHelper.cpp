#include "OLYCameraShotHelper.h"
#include "AsyncUDP.h"
#include <M5Unified.h>
#include "tjpgdClass.h"
#include "utility/M5Timer.h"

static AsyncUDP udp;
static M5Timer timer;

typedef struct {
    uint8_t cc:4;
    uint8_t extension:1;
    uint8_t padding:1;
    uint8_t version:2;
    uint8_t payload_type:7;
    uint8_t marker:1;
    uint16_t sequence_number;
    uint32_t timestamp;
    uint32_t ssrc;
} RTPHeader;

static uint8_t recvCounts[5] = {0, 0, 0, 0, 0};
static uint8_t drawCounts[5] = {0, 0, 0, 0, 0};

static int8_t readyBuffer = -1;
static uint8_t useBuffer = 0;
static uint16_t bufferLength = 0;
static uint16_t bufferPos = 0;
const uint8_t bufferCount = 3;
static uint8_t *buffer[bufferCount];
static uint32_t jpeg_size;
static uint32_t jpeg_pos;
static int32_t out_width{}, out_height{};
static int32_t off_x{}, off_y{};
static int32_t jpg_x{}, jpg_y{};
static TJpgD _jdec{};
static constexpr uint8_t buf_count = 2;
static uint8_t* _dmabufs[buf_count];
static uint8_t* _dmabuf{};

uint32_t jpgRead(TJpgD *jdec, uint8_t *buf, uint32_t len) {
    //M5_LOGI("jpgRead:%d, jpeg_size:%d, jpeg_pos:%d", len, jpeg_size, jpeg_pos);
    if (len > bufferLength - jpeg_pos)  len = bufferLength - jpeg_pos;
    if (buf) {
        memcpy(buf, buffer[readyBuffer] + jpeg_pos, len);
    }
    jpeg_pos += len;
    return len;
}

uint32_t jpgWrite16(TJpgD *jdec, void *bitmap, TJpgD::JRECT *rect) {
    uint16_t *dst = (uint16_t *)_dmabuf;

    uint_fast16_t x = rect->left;
    uint_fast16_t y = rect->top;
    uint_fast16_t w = rect->right + 1 - x;
    uint_fast16_t h = rect->bottom + 1 - y;
    uint_fast16_t outWidth = out_width;
    uint_fast16_t outHeight = out_height;
    uint8_t *src = (uint8_t*)bitmap;
    uint_fast16_t oL = 0, oR = 0;

    if (rect->right < off_x)      return 1;
    if (x >= (off_x + outWidth))  return 1;
    if (rect->bottom < off_y)     return 1;
    if (y >= (off_y + outHeight)) return 0; // No more rendering. [*1] => decomp failed 1 (Interrupted by output function.
        
    if (off_y > y) {
        uint_fast16_t linesToSkip = off_y - y;
        src += linesToSkip * w * 3;
        h -= linesToSkip;
    }

    if (off_x > x) {
        oL = off_x - x;
    }
    if (rect->right >= (off_x + outWidth)) {
        oR = (rect->right + 1) - (off_x + outWidth);
    }
    int_fast16_t line = (w - ( oL + oR ));
    dst += oL + x - off_x;
    src += oL * 3;

    do {
        int i = 0;
        do {
            // In order of speed, ALGO2, ALGO1, ALGO0(Original)
            uint32_t r = src[i*3+0] & 0xF8;
            uint32_t g = src[i*3+1] >> 2;
            uint32_t b = src[i*3+2] >> 3;
            r +=  (g >> 3);
            b +=  (g << 5);
            dst[i] = r | b << 8;
        } while (++i != line);
        dst += outWidth;
        src += w * 3;
    } while (--h);
    return 1;
}

uint32_t jpgWriteRow(TJpgD *jdec, uint32_t y, uint32_t h) {
    static int flip = 0;
    int_fast16_t oy = off_y;
    int_fast16_t bottom = y + h;
    int_fast16_t yy = y;
    
    if(bottom < oy) { return 1; /* continue */ }
    if(y >= oy + M5.Lcd.height()) { return 0; /* cutoff */}

    //M5_LOGI("y:%d, h:%d", y, h);

    // Adjust transfer height if there is a positive offset in the y direction
    if(oy > 0)
    {
        yy = y - oy;
        if(oy > y && oy < bottom) { h -= oy % h; yy = 0; } // First block
        if(oy + M5.Lcd.height() > y && oy + M5.Lcd.height() < y + h) // Last block
        {
            h -= (y + h) - (M5.Lcd.height() + oy);
        } 
    }
    //M5_LOGI("oy:%d y:%d h:%d yy:%d", oy, y, h, yy);

    M5.Lcd.pushImageDMA(jpg_x, jpg_y + yy,
                           out_width, h,
                           reinterpret_cast<::lgfx::swap565_t*>(_dmabuf));

    flip = !flip;
    _dmabuf = _dmabufs[flip];
    return 1;
}

bool drawJpg()
{
    jpeg_pos = 0;
    TJpgD::JRESULT jres = _jdec.prepare(jpgRead, nullptr);
    if (jres != TJpgD::JDR_OK) {
        M5_LOGE("prepare failed! %d", jres);
        return false;
    }

    out_width = std::min<int32_t>(_jdec.width, M5.Lcd.width());
    jpg_x = (M5.Lcd.width() - _jdec.width) >> 1;
    if (0 > jpg_x) {
        off_x = - jpg_x;
        jpg_x = 0;
    } else {
        off_x = 0;
    }

    out_height = std::min<int32_t>(_jdec.height, M5.Lcd.height());
    jpg_y = (M5.Lcd.height()- _jdec.height) >> 1;
    if (0 > jpg_y) {
        off_y = - jpg_y;
        jpg_y = 0;
    } else {
        off_y = 0;
    }
    //M5_LOGI("j(%d,%d) o(%d,%d) j:[%d,%d] out[%d,%d]", jpg_x, jpg_y, off_x, off_y, _jdec.width, _jdec.height, out_width, out_height);

    jres = _jdec.decomp_multitask(jpgWrite16, jpgWriteRow);

    if (jres > TJpgD::JDR_INTR)
    {
        // If the value is 1 (JDR_INTR),
        // No problem because the process is stopped by myself.
        // See also [*1]
        M5_LOGE("decomp failed! %d", jres);
        return false;
    }

    return true;
}

void udpPacket(AsyncUDPPacket packet)
{
    uint16_t pos = 0;
    RTPHeader *h = (RTPHeader *)packet.data();
    // M5_LOGI("v: %d, p: %d, x: %d, cc: %d, m: %d, pt: %d, sn: %d", h->version, h->padding, h->extension, h->cc, h->marker, h->payload_type, h->sequence_number);
    pos += sizeof(RTPHeader);

    // RTP Header Extension
    if(h->extension) {
        uint16_t profile = packet.data()[pos] << 8 | packet.data()[pos + 1];
        pos += 2;
        uint16_t length = packet.data()[pos] << 8 | packet.data()[pos + 1];
        pos += 2;
        // M5_LOGI("profile: %d, length: %d", profile, length);

        while (length > 0) {
            uint16_t id = packet.data()[pos] << 8 | packet.data()[pos + 1];
            pos += 2;
            uint16_t size = packet.data()[pos] << 8 | packet.data()[pos + 1];
            pos += 2;
            length -= size + 1;
            // M5_LOGI("id: %d, size: %d", id, size);

            if (id == 0x0001) {
                // get jpeg_size with network byte order
                jpeg_size = packet.data()[pos] << 24 | packet.data()[pos + 1] << 16 | packet.data()[pos + 2] << 8 | packet.data()[pos + 3];
                //M5_LOGI("function_id: %d, length: %d, jpeg_size: %d", id, length, jpeg_size);
            }
            pos += size * 4;
        }
        bufferPos = 0;
    }

    // 残りを読んでbufferに格納する。bufferのサイズが足りないときは拡張する。
    if (bufferLength == 0) {
        bufferLength = packet.length() - pos;
        for (int i = 0; i < bufferCount; i++) {
            // malloc from psram
            buffer[i] = (uint8_t *)ps_malloc(bufferLength);
            //buffer[i] = (uint8_t *)heap_caps_malloc(bufferLength, MALLOC_CAP_8BIT);
        }
    }
    else if (bufferLength < (packet.length() - pos) + bufferPos) {
        bufferLength = (packet.length() - pos) + bufferPos;
        for (int i = 0; i < bufferCount; i++) {
            buffer[i] = (uint8_t *)ps_realloc(buffer[i], bufferLength);
            //buffer[i] = (uint8_t *)heap_caps_realloc(buffer[i], bufferLength, MALLOC_CAP_8BIT);
        }
    }

    memcpy(buffer[useBuffer] + bufferPos, packet.data() + pos, packet.length() - pos);
    bufferPos += packet.length() - pos;

    if (h->marker && M5.Lcd.getStartCount() == 0) {
        if (jpeg_size == 0 || bufferPos < jpeg_size) {
            M5_LOGE("invalid jpeg data -- bufferPos: %d, jpeg_size: %d", bufferPos, jpeg_size);
        } else {
            // 画像を表示する
            //M5.Lcd.drawJpg(buffer[useBuffer], jpeg_size, 0, 0, 320, 240);
            recvCounts[0]++;
            if (readyBuffer == -1) {
                readyBuffer = useBuffer;
            }
            bufferPos = 0;
            useBuffer++;
            if (useBuffer >= bufferCount) {
                useBuffer = 0;
            }
            //M5_LOGI("useBuffer: %d, readyBuffer: %d", useBuffer, readyBuffer);
            if (useBuffer == readyBuffer) {
                M5_LOGW("buffer overflow");
                useBuffer++;
                if (useBuffer >= bufferCount) {
                    useBuffer = 0;
                }
            }
        }
    }
}

void logFps()
{
    M5_LOGI(
        "recv: %d, draw: %d",
        (recvCounts[0] + recvCounts[1] + recvCounts[2] + recvCounts[3] + recvCounts[4]) / 5,
        (drawCounts[0] + drawCounts[1] + drawCounts[2] + drawCounts[3] + drawCounts[4]) / 5
    );

    memcpy(recvCounts + 1, recvCounts, 4);
    recvCounts[0] = 0;

    memcpy(drawCounts + 1, drawCounts, 4);
    drawCounts[0] = 0;
}

void OLYCameraShotHelper::startLiveview()
{
    if(udp.listen(1234)) {
        for (int i = 0; i < buf_count; i++) {
            _dmabufs[i] = (uint8_t*)heap_caps_malloc(M5.Lcd.width() * 48 * 2, MALLOC_CAP_DMA);
            assert(_dmabufs[i]);
        }
        _dmabuf = _dmabufs[0];

        _jdec.multitask_begin();

        udp.onPacket(udpPacket);

        timer.setInterval(1000, logFps);

        httpGet("exec_takemisc.cgi?com=startliveview&port=01234&lvqty=0320x0240", 200);
    }
}

void OLYCameraShotHelper::loop()
{
    timer.run();
}

bool OLYCameraShotHelper::render()
{
    if (readyBuffer >= 0) {
        //M5.Lcd.drawJpg(buffer[readyBuffer], bufferLength, 0, 0, 320, 240);
        drawJpg();
        drawCounts[0]++;
        readyBuffer = -1;
        return true;
    }
    return false;
}
