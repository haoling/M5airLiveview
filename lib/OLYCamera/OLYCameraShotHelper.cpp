#include "OLYCameraShotHelper.h"
#include "AsyncUDP.h"
#include <M5Unified.h>

static AsyncUDP udp;

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

static int8_t readyBuffer = -1;
static uint8_t useBuffer = 0;
static uint16_t bufferLength = 0;
static uint16_t bufferPos = 0;
const uint8_t bufferCount = 2;
static uint8_t *buffer[bufferCount];
static uint32_t jpeg_size;

void OLYCameraShotHelper::startLiveview()
{
    if(udp.listen(1234)) {
        udp.onPacket([](AsyncUDPPacket packet) {
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
                        // M5_LOGI("function_id: %d, length: %d, jpeg_size: %d", id, length, jpeg_size);
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
                }
            }
            else if (bufferLength < (packet.length() - pos) + bufferPos) {
                bufferLength = (packet.length() - pos) + bufferPos;
                for (int i = 0; i < bufferCount; i++) {
                    buffer[i] = (uint8_t *)ps_realloc(buffer[i], bufferLength);
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
                    if (readyBuffer == -1) {
                        readyBuffer = useBuffer;
                    }
                    bufferPos = 0;
                    useBuffer++;
                    if (useBuffer >= bufferCount) {
                        useBuffer = 0;
                    }
                    if (useBuffer == readyBuffer) {
                        M5_LOGW("buffer overflow");
                        useBuffer++;
                        if (useBuffer >= bufferCount) {
                            useBuffer = 0;
                        }
                    }
                }
            }
        });

        httpGet("exec_takemisc.cgi?com=startliveview&port=01234&lvqty=0320x0240", 200);
    }
}

void OLYCameraShotHelper::loop()
{
    if (readyBuffer >= 0) {
        M5.Lcd.drawJpg(buffer[readyBuffer], bufferLength, 0, 0, 320, 240);
        readyBuffer = -1;
    }
}