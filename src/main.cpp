#include <M5StickCPlus.h>

// 正しいピン定義
const int RXPIN = 0;     // RX pin
const int TXPIN = 26;    // TX pin
const int EN_PIN = 33;   // EN pin

// テストモード
enum TestMode {
    VERSION_READ,    // Vコマンド
    SINGLE_READ      // Sコマンド
};

TestMode currentMode = VERSION_READ;

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
    
    // ENピンを常時HIGH
    gpio_set_level((gpio_num_t)EN_PIN, 1);
    delay(1000);  // 安定化待ち
    
    // Serial2の完全リセット
    Serial2.end();
    delay(100);
    
    // Serial2の初期化（9600bps）
    Serial2.begin(9600, SERIAL_8N1, RXPIN, TXPIN);
    while(!Serial2) {
        delay(10);
    }
    Serial2.flush();
    
    // プルアップ有効化
    gpio_set_pull_mode((gpio_num_t)RXPIN, GPIO_PULLUP_ONLY);
    
    Serial.println("Initialization completed");
    M5.Lcd.println("\nA:Ver B:Read");
}

void loop() {
    M5.update();
    static bool testRunning = false;
    static unsigned long lastSend = 0;
    
    // Aボタン: 押している間シングルリード
    if (M5.BtnA.isPressed()) {
        if (!testRunning) {
            Serial.println("\n=== Starting Single Read ===");
            M5.Lcd.fillScreen(BLACK);
            M5.Lcd.setCursor(0, 0);
            M5.Lcd.println("Reading...");
            
            // バッファクリア
            while(Serial2.available()) {
                Serial2.read();
            }
            
            testRunning = true;
            lastSend = 0;
        }
        
        if (millis() - lastSend > 2000) {
            lastSend = millis();
            
            // シングル読み取りコマンド
            Serial.println("\nSending Single Read command...");
            Serial2.write(0x0A);  // <LF>
            Serial.println("Sent: <LF> (0x0A)");
            delay(10);
            Serial2.write('S');   // S
            Serial.println("Sent: S (0x53)");
            delay(10);
            Serial2.write(0x0D);  // <CR>
            Serial.println("Sent: <CR> (0x0D)");
            
            Serial2.flush();
            delay(50);
            
            // 応答待ち
            Serial.println("Waiting for response...");
            unsigned long startTime = millis();
            int bytesRead = 0;
            uint8_t buffer[32] = {0};
            
            while (millis() - startTime < 1000) {
                if (Serial2.available()) {
                    uint8_t c = Serial2.read();
                    buffer[bytesRead] = c;
                    
                    Serial.printf("Received [%d]: 0x%02X (Binary: ", bytesRead, c);
                    for (int i = 7; i >= 0; i--) {
                        Serial.print((c >> i) & 0x01);
                    }
                    Serial.print(")");
                    
                    if (c >= 32 && c <= 126) {
                        Serial.printf(" '%c'", (char)c);
                    }
                    Serial.println();
                    
                    M5.Lcd.printf("%02X ", c);
                    bytesRead++;
                    
                    if (bytesRead >= 32) break;
                }
            }
            
            if (bytesRead == 0) {
                Serial.println("No response");
                M5.Lcd.println("No response");
            } else {
                Serial.print("Complete response: ");
                for (int i = 0; i < bytesRead; i++) {
                    Serial.printf("%02X ", buffer[i]);
                }
                Serial.println();
            }
            
            // ピン状態確認
            Serial.printf("Pin states - RX:%d, TX:%d, EN:%d\n",
                         digitalRead(RXPIN),
                         digitalRead(TXPIN),
                         gpio_get_level((gpio_num_t)EN_PIN));
        }
    } else if (M5.BtnA.wasReleased()) {
        testRunning = false;
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.println("A:Read B:Ver");
    }
    
    // Bボタン: バージョン確認
    if (M5.BtnB.wasPressed()) {
        Serial.println("\n=== Version Check ===");
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.println("Version...");
        
        // バッファクリア
        while(Serial2.available()) {
            Serial2.read();
        }
        
        // バージョン確認コマンド
        Serial2.write(0x0A);  // <LF>
        delay(10);
        Serial2.write('V');   // V
        delay(10);
        Serial2.write(0x0D);  // <CR>
        
        Serial2.flush();
        delay(50);
        
        // 応答待ち
        unsigned long startTime = millis();
        int bytesRead = 0;
        uint8_t buffer[32] = {0};
        
        while (millis() - startTime < 1000) {
            if (Serial2.available()) {
                uint8_t c = Serial2.read();
                buffer[bytesRead] = c;
                M5.Lcd.printf("%02X ", c);
                bytesRead++;
                if (bytesRead >= 32) break;
            }
        }
        
        delay(2000);
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.println("A:Read B:Ver");
    }
    
    delay(10);
}