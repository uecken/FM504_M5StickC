#include <M5StickCPlus.h>

// ピン定義
const int RXPIN = 0;     // RX pin
const int TXPIN = 26;    // TX pin
const int EN_PIN = 33;   // EN pin

// コマンド定義
const uint8_t CMD_QUERY[] = {0x0A, 0x51, 0x0D};  // <LF>Q<CR>
const uint8_t CMD_MULTI[] = {0x0A, 0x55, 0x0D};  // <LF>U<CR>
const uint8_t CMD_VERSION[] = {0x0A, 0x56, 0x0D}; // <LF>V<CR>
const uint8_t CMD_JP_MODE[] = {0x0A, 0x4E, 0x35, 0x2C, 0x30, 0x36, 0x0D}; // <LF>N5,06<CR>
const uint8_t CMD_FREQ_922[] = {0x0A, 0x46, 0x39, 0x32, 0x32, 0x30, 0x30, 0x30, 0x0D}; // <LF>F922000<CR>

// 関数プロトタイプ宣言
void setEN(bool high);
void sendCommand(const uint8_t* cmd, size_t len);
void readResponse();

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
    unsigned long startTime = millis();
    int bytesRead = 0;
    uint8_t buffer[64] = {0};
    bool isTagDetected = false;
    
    while (millis() - startTime < 500) {
        if (Serial2.available()) {
            uint8_t c = Serial2.read();
            buffer[bytesRead] = c;
            
            /*
            Serial.printf("Received [%d]: %02X", bytesRead, c);
            if (c >= 32 && c <= 126) {
                Serial.printf(" ('%c')", (char)c);
            }
            Serial.println();
            */

            bytesRead++;
            if (bytesRead >= sizeof(buffer)) break;
            
            // タグ検出の判定（応答が4バイト以上でQコマンドの場合）
            if (bytesRead > 4 && buffer[1] == 0x51 && buffer[2] != 0x0D) {
                isTagDetected = true;
            }
        }
        else if (bytesRead > 0 && millis() - startTime > 100) {
            break;
        }
    }
    
    if (bytesRead > 0) {
        // 完全な応答をシリアル出力
        Serial.print("Complete response: ");
        for (int i = 0; i < bytesRead; i++) {
            Serial.printf("%02X ", buffer[i]);
        }
        Serial.println();
        
        // 画面表示
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 0);
        
        if (buffer[1] == 0x51) {  // Query応答
            if (isTagDetected) {
                M5.Lcd.setTextSize(2);
                M5.Lcd.println("Tag Found!");
                M5.Lcd.setTextSize(1);
                // EPCデータ表示（<LF>Q から <CR><LF>を除く）
                for (int i = 2; i < bytesRead - 2; i++) {
                    M5.Lcd.printf("%02X ", buffer[i]);
                }
            } else {
                M5.Lcd.setTextSize(2);
                M5.Lcd.println("No Tag");
            }
        }
        else if (buffer[1] == 0x56) {  // Version応答
            M5.Lcd.println("Version:");
            for (int i = 2; i < bytesRead - 2; i++) {
                M5.Lcd.write(buffer[i]);
            }
        }
    }
}

void setup() {
    M5.begin();
    Serial.begin(115200);
    Serial.println("\nFM505 Basic Test");
    
    M5.Lcd.setRotation(3);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("FM505 Test");
    
    // GPIO設定
    gpio_pad_select_gpio(EN_PIN);
    gpio_set_direction((gpio_num_t)EN_PIN, GPIO_MODE_OUTPUT);
    setEN(true);  // 期設定用にENをHIGH
    
    // Serial2の初期化（38400bps）
    Serial2.begin(38400, SERIAL_8N1, RXPIN, TXPIN);
    while(!Serial2) {
        delay(10);
    }
    Serial2.flush();
    
    // プルアップ有効化
    gpio_set_pull_mode((gpio_num_t)RXPIN, GPIO_PULLUP_ONLY);
    
    // JP modeに設定
    Serial.println("\nSetting JP mode...");
    sendCommand(CMD_JP_MODE, sizeof(CMD_JP_MODE));
    readResponse();
    delay(100);
    
    // 周波数を922.0MHzに設定
    Serial.println("\nSetting frequency to 922.0MHz...");
    sendCommand(CMD_FREQ_922, sizeof(CMD_FREQ_922));
    readResponse();
    delay(100);
    
    // 初期設定完了後、ENをLOWに
    setEN(true);
    
    Serial.println("Initialization completed");
    M5.Lcd.println("\nQuery Mode");
}

void loop() {
    M5.update();
    static unsigned long lastQuery = 0;
    static int queryCount = 0;
    
    // 200msごとにQuery実行
    if (millis() - lastQuery >= 200) {
        lastQuery = millis();
        queryCount++;
        
        // バッファクリア
        while(Serial2.available()) {
            Serial2.read();
        }
        
        // Query前にENピンをHIGHに設定（Standbyモード）
        setEN(true);
        delay(10);
        
        // Query EPCコマンド送信
        Serial.printf("\nQuery #%d\n", queryCount);
        sendCommand(CMD_QUERY, sizeof(CMD_QUERY));
        
        // 応答読み取り
        readResponse();
        
        // 読み取り完了後、ENピンをLOWに（Sleepモード）
        setEN(false);
    }
    
    // Aボタン: Multi Query
    if (M5.BtnA.wasPressed()) {
        setEN(true);  // Standbyモード
        delay(50);
        
        Serial.println("\n=== Multi Query ===");
        sendCommand(CMD_MULTI, sizeof(CMD_MULTI));
        readResponse();
        
        setEN(false);  // Sleepモード
    }
    
    // Bボタン: バージョン確認
    if (M5.BtnB.wasPressed()) {
        setEN(true);  // Standbyモード
        delay(50);
        
        Serial.println("\n=== Version Check ===");
        sendCommand(CMD_VERSION, sizeof(CMD_VERSION));
        readResponse();
        
        setEN(false);  // Sleepモード
    }
    
    delay(5);
}