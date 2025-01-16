#include <M5StickC.h>
#include <BleCombo.h>  // Include the BLE Combo library
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// キュー定義
static QueueHandle_t idQueue;
#define QUEUE_SIZE 20
#define QUEUE_ITEM_SIZE 64

// ピン定義
const int RXPIN = 0;     // RX pin
const int TXPIN = 26;    // TX pin
const int EN_PIN = 33;   // EN pin

// コマンド定義
const uint8_t CMD_QUERY[] = {0x0A, 0x51, 0x0D};  // <LF>Q<CR>
const uint8_t CMD_MULTI[] = {0x0A, 0x55, 0x0D};  // <LF>U<CR>
const uint8_t CMD_VERSION[] = {0x0A, 0x56, 0x0D}; // <LF>V<CR>
const uint8_t CMD_JP_MODE[] = {0x0A, 0x4E, 0x35, 0x2C, 0x30, 0x36, 0x0D}; // <LF>N5,06<CR>
const uint8_t CMD_FREQ_922[] = {0x0A, 0x4A, 0x31, 0x30, 0x35, 0x0D}; // <LF>J105<CR>
const uint8_t CMD_POWER_25[] = {0x0A, 0x4E, 0x31, 0x2C, 0x32, 0x35, 0x0D}; // <LF>N1,25<CR>
const uint8_t CMD_POWER_20[] = {0x0A, 0x4E, 0x31, 0x2C, 0x32, 0x30, 0x0D}; // <LF>N1,20<CR>
const uint8_t CMD_POWER_15[] = {0x0A, 0x4E, 0x31, 0x2C, 0x31, 0x35, 0x0D}; // <LF>N1,15<CR>
const uint8_t CMD_POWER_10[] = {0x0A, 0x4E, 0x31, 0x2C, 0x31, 0x30, 0x0D}; // <LF>N1,15<CR>
const uint8_t CMD_POWER_5[] = {0x0A, 0x4E, 0x31, 0x2C, 0x30, 0x35, 0x0D}; // <LF>N1,05<CR>
const uint8_t CMD_POWER_2[] = {0x0A, 0x4E, 0x31, 0x2C, 0x30, 0x32, 0x0D}; // <LF>N1,00<CR>
const uint8_t CMD_READ_POWER[] = {0x0A, 0x4E, 0x30, 0x2C, 0x30, 0x30, 0x0D}; // <LF>N0,00<CR>

// 関数プロトタイプ宣言
void setEN(bool high);
void sendCommand(const uint8_t* cmd, size_t len);
void readResponse();
void readPower();
void sendString(const char* str);

// グローバル変数（既存のものは維持）
bool isTagDetected = false;
int currentPower = 25;
int currentPower_read = -30;

// BLEの初期化
BleCombo bleCombo("M5 RFID Reader", "M5Stack", 100);

// タスク関数のプロトタイプ宣言
void rfidTask(void *pvParameters);
void bleTask(void *pvParameters);
void buttonTask(void *pvParameters);

// ボタン処理用のキュー
static QueueHandle_t powerQueue;

// 文字列送信用の関数を修正
void sendString(const char* str) {
    if (bleCombo.isConnected()) {
        Serial.printf("BLE HID Sending: %s\n", str);
        bleCombo.print(str);
        bleCombo.write(KEY_RETURN);  // 改行を送信
    }
}

void setEN(bool high) {
    gpio_set_level((gpio_num_t)EN_PIN, high ? 1 : 0);
    delay(100);  // 安定化待ち
    Serial.printf("EN Pin: %s (%s mode)\n", 
        high ? "HIGH" : "LOW",
        high ? "Standby" : "Sleep");
}

void sendCommand(const uint8_t* cmd, size_t len) {
    Serial.print("Sending command: ");
    for (size_t i = 0; i < len; i++) {
        Serial2.write(cmd[i]);
        Serial.printf("%02X ", cmd[i]);
    }
    Serial.println();
    Serial2.flush();
}

void readResponse() {
    uint8_t buffer[256];
    size_t len = 0;
    unsigned long startTime = millis();
    
    while (millis() - startTime < 10) {
        if (Serial2.available()) {
            uint8_t c = Serial2.read();
            buffer[len] = c;
            len++;
            if (len >= sizeof(buffer)) break;
            
            if (len > 4 && buffer[1] == 0x51 && buffer[2] != 0x0D) {
                isTagDetected = true;
            }
        }
        else if (len > 0 && millis() - startTime > 10) {
            break;
        }
    }
    
    if (len > 0) {
        Serial.print("Response: ");
        char idString[64] = "";
        int idIndex = 0;

        // IDデータの抽出（4バイト目から12バイト分がID）
        for (size_t i = 4; i < len && i < 16; i++) {
            sprintf(&idString[idIndex], "%02X", buffer[i]);
            idIndex += 2;
        }
        
        // 画面表示とBLE送信
        if (idIndex > 0) {
            Serial.printf("ID: %s\n", idString);
            M5.Lcd.fillScreen(BLACK);
            M5.Lcd.setCursor(0, 0);
            M5.Lcd.setTextSize(2);
            M5.Lcd.printf("ID: %s", idString);
            
            // BLE経由で送信
            sendString(idString);
        }
    }
}

void readPower() {
    sendCommand(CMD_READ_POWER, sizeof(CMD_READ_POWER));
    uint8_t buffer[256];
    size_t len = 0;
    unsigned long startTime = millis();
    
    while (millis() - startTime < 50) {
        if (Serial2.available()) {
            buffer[len++] = Serial2.read();
            if (len >= sizeof(buffer)) break;
        }
        else if (len > 0 && millis() - startTime > 100) {
            break;
        }
    }
    
    if (len > 0) {
        Serial.print("Power Response Raw: ");
        for (size_t i = 0; i < len; i++) {
            Serial.printf("%02X[%c] ", buffer[i], buffer[i]);
        }
        Serial.println();

        // 応答データから数値を探す
        for (size_t i = 0; i < len - 1; i++) {
            if (buffer[i] >= '0' && buffer[i] <= '9' && 
                buffer[i+1] >= '0' && buffer[i+1] <= '9') {
                char power_str[3] = {(char)buffer[i], (char)buffer[i+1], '\0'};
                currentPower_read = atoi(power_str);
                Serial.printf("Power extracted: [%s] = %d dBm\n", power_str, currentPower_read);
                break;
            }
        }
    }
}

void setup() {
    M5.begin(true, true, false);
    Serial.begin(115200);
    
    // 画面設定
    M5.Lcd.setRotation(3);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("RFID Reader");
    
    // キューの作成
    idQueue = xQueueCreate(QUEUE_SIZE, QUEUE_ITEM_SIZE);
    powerQueue = xQueueCreate(5, sizeof(int));  // パワー設定用キュー
    
    // RFIDの初期化（既存のコードを使用）
    gpio_pad_select_gpio(EN_PIN);
    gpio_set_direction((gpio_num_t)EN_PIN, GPIO_MODE_OUTPUT);
    setEN(true);
    
    Serial2.begin(38400, SERIAL_8N1, RXPIN, TXPIN);
    gpio_set_pull_mode((gpio_num_t)RXPIN, GPIO_PULLUP_ONLY);
    
    // 初期設定
    sendCommand(CMD_JP_MODE, sizeof(CMD_JP_MODE));
    delay(100);
    sendCommand(CMD_FREQ_922, sizeof(CMD_FREQ_922));
    delay(100);
    sendCommand(CMD_POWER_25, sizeof(CMD_POWER_25));
    delay(100);
    
    // BLE初期化
    bleCombo.begin();
    
    // タスクの作成（コア指定）
    xTaskCreatePinnedToCore(rfidTask, "RFID Task", 4096, NULL, 1, NULL, 1);    // Core 1
    xTaskCreatePinnedToCore(bleTask, "BLE Task", 4096, NULL, 1, NULL, 0);      // Core 0
    xTaskCreatePinnedToCore(buttonTask, "Button Task", 4096, NULL, 1, NULL, 1); // Core 1
    
    Serial.println("Initialization completed");
}

void loop() {
}

void rfidTask(void *pvParameters) {
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(10);  // 10ms周期
    
    xLastWakeTime = xTaskGetTickCount();
    
    while (true) {
        static unsigned long lastQuery = 0;
        
        if (millis() - lastQuery >= 100) {  // RFIDの読み取りは100ms間隔
            lastQuery = millis();
            
            setEN(true);
            delay(5);
            sendCommand(CMD_MULTI, sizeof(CMD_MULTI));
            
            // 応答の読み取りと処理
            uint8_t buffer[256];
            size_t len = 0;
            unsigned long startTime = millis();
            
            memset(buffer, 0, sizeof(buffer));  // バッファをクリア
            
            while (millis() - startTime < 50) {
                if (Serial2.available()) {
                    buffer[len] = Serial2.read();
                    len++;
                    if (len >= sizeof(buffer)) break;
                }
            }
            
            if (len > 0) {
                // デバッグ出力
                Serial.print("Response Raw: ");
                for (size_t i = 0; i < len; i++) {
                    Serial.printf("%02X ", buffer[i]);
                }
                Serial.println();

                // 有効なEPCデータを探す
                for (size_t i = 0; i < len - 24; i++) {
                    // パターン: <LF>U3000 を探す
                    if (buffer[i] == 0x0A &&      // <LF>
                        buffer[i+1] == 0x55 &&    // U
                        buffer[i+2] == 0x33 &&    // 3
                        buffer[i+3] == 0x30 &&    // 0
                        buffer[i+4] == 0x30 &&    // 0
                        buffer[i+5] == 0x30) {    // 0
                        
                        char idString[64] = {0};
                        int idIndex = 0;
                        bool validData = true;
                        
                        // EPCデータの抽出（12バイト分）
                        for (size_t j = i + 6; j < i + 30 && j < len; j += 2) {
                            // 16進数文字のチェック
                            if (isxdigit(buffer[j]) && isxdigit(buffer[j+1])) {
                                char hex[3] = {(char)buffer[j], (char)buffer[j+1], 0};
                                sprintf(&idString[idIndex], "%s", hex);
                                idIndex += 2;
                            } else {
                                validData = false;
                                break;
                            }
                        }
                        
                        if (validData && idIndex == 24) {  // 正確に12バイト（24文字）
                            Serial.printf("Valid EPC: %s\n", idString);
                            
                            // 画面表示
                            M5.Lcd.fillScreen(BLACK);
                            M5.Lcd.setCursor(0, 0);
                            M5.Lcd.setTextSize(2);
                            M5.Lcd.printf("EPC:\n%s", idString);
                            
                            // キューに送信
                            xQueueSend(idQueue, idString, 0);
                            break;  // 有効なデータを見つけたら終了
                        }
                    }
                }
            }
            
            setEN(false);
        }
        
        // 正確な10ms周期で実行
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void bleTask(void *pvParameters) {
    char receivedId[64];
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(100);  // 10ms周期
    
    xLastWakeTime = xTaskGetTickCount();
    
    while (true) {
        if (xQueueReceive(idQueue, &receivedId, 0) == pdTRUE) {
            if (bleCombo.isConnected()) {
                Serial.printf("BLE HID Sending: %s\n", receivedId);
                bleCombo.print(receivedId);
                bleCombo.write(KEY_RETURN);
            }
        }
        
        // 正確な10ms周期で実行
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void buttonTask(void *pvParameters) {
    while (true) {
        M5.update();  // ボタン状態の更新
        
        if (M5.BtnB.wasReleased()) {
            // パワー設定の切り替え
            switch (currentPower) {
                case 25: currentPower = 20; break;
                case 20: currentPower = 15; break;
                case 15: currentPower = 10; break;
                case 10: currentPower = 5;  break;
                case 5:  currentPower = 2;  break;
                default: currentPower = 25; break;
            }
            
            // キューに新しいパワー値を送信
            xQueueSend(powerQueue, &currentPower, 0);
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));  // 50ms待機
    }
}