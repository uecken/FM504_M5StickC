#include <M5StickCPlus.h>

// 正しいピン定義
const int RXPIN = 0;     // RX pin
const int TXPIN = 26;    // TX pin
const int EN_PIN = 33;   // EN pin

// テストモード
enum TestMode {
    SINGLE_READ,
    MULTI_READ
};

TestMode currentMode = SINGLE_READ;

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
    
    // 初期状態確認
    Serial.printf("Pin states - RX:%d, TX:%d, EN:%d\n",
                 digitalRead(RXPIN),
                 digitalRead(TXPIN),
                 gpio_get_level((gpio_num_t)EN_PIN));
    
    Serial.println("Initialization completed");
}

void loop() {
    M5.update();
    static bool testRunning = false;
    static unsigned long lastSend = 0;
    
    // Bボタンでモード切替
    if (M5.BtnB.wasPressed()) {
        currentMode = (currentMode == SINGLE_READ) ? MULTI_READ : SINGLE_READ;
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.printf("Mode: %s", (currentMode == SINGLE_READ) ? "Single" : "Multi");
        Serial.printf("\nSwitched to %s read mode\n", 
                     (currentMode == SINGLE_READ) ? "single" : "multi");
        delay(1000);
    }
    
    if (M5.BtnA.wasPressed()) {
        Serial.printf("\n=== Starting %s Test ===\n", 
                     (currentMode == SINGLE_READ) ? "Single Read" : "Multi Read");
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.println("Testing...");
        
        // バッファクリア
        while(Serial2.available()) {
            Serial2.read();
        }
        
        testRunning = true;
        lastSend = 0;
    }
    
    if (testRunning && (millis() - lastSend > 2000)) {
        lastSend = millis();
        
        if (currentMode == SINGLE_READ) {
            // 単一読み取りコマンド
            Serial.println("\nSending Version command...");
            Serial2.write(0x0A);  // <LF>
            Serial.println("Sent: <LF> (0x0A)");
            delay(10);
            Serial2.write('V');   // V
            Serial.println("Sent: V (0x56)");
            delay(10);
            Serial2.write(0x0D);  // <CR>
            Serial.println("Sent: <CR> (0x0D)");
        } else {
            // マルチリードコマンド
            Serial.println("\nSending Multi-Read command...");
            Serial2.write(0x0A);  // <LF>
            Serial.println("Sent: <LF> (0x0A)");
            delay(10);
            Serial2.write('M');   // M
            Serial.println("Sent: M (0x4D)");
            delay(10);
            Serial2.write(0x0D);  // <CR>
            Serial.println("Sent: <CR> (0x0D)");
        }
        
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
        
        // ピン状態確認（ENは常にHIGH）
        Serial.printf("Pin states - RX:%d, TX:%d, EN:%d\n",
                     digitalRead(RXPIN),
                     digitalRead(TXPIN),
                     gpio_get_level((gpio_num_t)EN_PIN));
    }
    
    if (M5.BtnA.wasReleased()) {
        Serial.println("=== Stopping test ===");
        testRunning = false;
    }
    
    delay(10);
}