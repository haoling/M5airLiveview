#include "OLYCameraShotHelper.h"
#include "AsyncUDP.h"
#include <M5Unified.h>
AsyncUDP udp;

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
            // Serial.printf("v: %d, p: %d, x: %d, cc: %d, m: %d, pt: %d, sn: %d", h->version, h->padding, h->extension, h->cc, h->marker, h->payload_type, h->sequence_number);
            // Serial.println();
            pos += sizeof(RTPHeader);

            // RTP Header Extension
            if(h->extension) {
                uint16_t profile = packet.data()[pos] << 8 | packet.data()[pos + 1];
                pos += 2;
                uint16_t length = packet.data()[pos] << 8 | packet.data()[pos + 1];
                pos += 2;
                // Serial.printf("profile: %d, length: %d", profile, length);
                // Serial.println();

                while (length > 0) {
                    uint16_t id = packet.data()[pos] << 8 | packet.data()[pos + 1];
                    pos += 2;
                    uint16_t size = packet.data()[pos] << 8 | packet.data()[pos + 1];
                    pos += 2;
                    length -= size + 1;
                    // Serial.printf("id: %d, size: %d", id, size);
                    // Serial.println();

                    if (id == 0x0001) {
                        // get jpeg_size with network byte order
                        jpeg_size = packet.data()[pos] << 24 | packet.data()[pos + 1] << 16 | packet.data()[pos + 2] << 8 | packet.data()[pos + 3];
                        // Serial.printf("function_id: %d, length: %d, jpeg_size: %d", fs->function_id, fs->length, fs->jpeg_size);
                        // Serial.println();
                    }
                    pos += size * 4;
                }
                bufferPos = 0;
            }

            // 残りを読んでbufferに格納する。bufferのサイズが足りないときは拡張する。
            if (bufferLength == 0) {
                bufferLength = packet.length() - pos;
                for (int i = 0; i < bufferCount; i++) {
                    buffer[i] = (uint8_t *)malloc(bufferLength);
                }
            }
            else if (bufferLength < (packet.length() - pos) + bufferPos) {
                bufferLength = (packet.length() - pos) + bufferPos;
                for (int i = 0; i < bufferCount; i++) {
                    buffer[i] = (uint8_t *)realloc(buffer[i], bufferLength);
                }
            }

            memcpy(buffer[useBuffer] + bufferPos, packet.data() + pos, packet.length() - pos);
            bufferPos += packet.length() - pos;

            if (h->marker && M5.Lcd.getStartCount() == 0) {
                if (bufferPos < jpeg_size) {
                    Serial.printf("invalid jpeg data -- bufferPos: %d, jpeg_size: %d\n", bufferPos, jpeg_size);
                } else {
                    // 画像を表示する
                    M5.Lcd.drawJpg(buffer[useBuffer], jpeg_size, 0, 0, 320, 240);
                    bufferPos = 0;
                    useBuffer++;
                    if (useBuffer >= bufferCount) {
                        useBuffer = 0;
                    }
                }
            }
        });

        httpGet("exec_takemisc.cgi?com=startliveview&port=01234&lvqty=0320x0240", 200);
    }
}