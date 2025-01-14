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

bool isTagDetected = false;  // Declare the variable
int currentPower = 25;  // デフォルトは25dBm
int currentPower_read = -30; 

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
    
    while (millis() - startTime < 50) {
        if (Serial2.available()) {
            uint8_t c = Serial2.read();
            buffer[len] = c;

            len++;
            if (len >= sizeof(buffer)) break;
            
            // タグ検出の判定（応答が4バイト以上でQコマンドの場合）
            if (len > 4 && buffer[1] == 0x51 && buffer[2] != 0x0D) {
                isTagDetected = true;
            }
        }
        else if (len > 0 && millis() - startTime > 100) {
            break;
        }
    }
    
    if (len > 0) {
        Serial.print("Response: ");
        for (size_t i = 0; i < len; i++) {
            Serial.printf("%02X ", buffer[i]);
        }
        Serial.println();

        // 画面表示の処理を追加
        M5.Lcd.fillScreen(BLACK);  // 画面をクリア
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.setTextSize(2);

        // コマンドタイプに応じた表示処理
        if (buffer[1] == 0x51) {  // Query response
            M5.Lcd.println("Query Response:");
        } else if (buffer[1] == 0x55) {  // Multi response
            M5.Lcd.println("Multi Response:");
        }

        // データの表示
        M5.Lcd.setTextSize(1);
        for (size_t i = 0; i < len; i++) {
            if (i % 8 == 0) {
                M5.Lcd.println();
            }
            M5.Lcd.printf("%02X ", buffer[i]);
        }
    } else {
        Serial.println("No response");
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.setTextSize(2);
        M5.Lcd.println("No Response");
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

        // 応答データから"N"の後の2桁の数値を探す
        for (size_t i = 0; i < len - 2; i++) {
            if (buffer[i] == 'N') {
                // 次の2文字を数値として解釈
                char power_str[3] = {(char)buffer[i+1], (char)buffer[i+2], '\0'};
                if (isdigit(power_str[0]) && isdigit(power_str[1])) {
                    currentPower_read = atoi(power_str);
                    Serial.printf("Power extracted: %d dBm\n", currentPower_read);
                    break;
                }
            }
        }
    }
}

void setup() {
    // LEDCの初期化パラメータを設定
    ledcSetup(0, 1000, 8);  // チャンネル0、周波数1000Hz、分解能8ビット
    
    // M5StickCPlusの初期化
    M5.begin(true, true, false);  // LCD有効, Power有効, Serial無効
    //M5.begin();
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
    
    // JP mode 916-921MHz設定
    sendCommand(CMD_JP_MODE, sizeof(CMD_JP_MODE));
    delay(100);
    
    //周波数設定
    //freq_cmd = b'\x0A\x4A\x31\x30\x35\x0D';
    sendCommand(CMD_FREQ_922, sizeof(CMD_FREQ_922));
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
    
    // 100msごとにQuery実行
    if (millis() - lastQuery >= 00) {
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
        //sendCommand(CMD_QUERY, sizeof(CMD_QUERY));
        sendCommand(CMD_MULTI, sizeof(CMD_MULTI));
        
        // 応答読み取り
        readResponse();
        
        // 読み取り完了後、ENピンをLOWに（Sleepモード）
        setEN(false);
    }
    
    // Aボタン: Multi Query
    /*
    if (M5.BtnA.wasPressed()) {
        setEN(true);  // Standbyモード
        delay(50);
        
        Serial.println("\n=== Multi Query ===");
        sendCommand(CMD_MULTI, sizeof(CMD_MULTI));
        readResponse();
        
        setEN(false);  // Sleepモード
    }
    */
    
    // Bボタン: バージョン確認
    if (M5.BtnB.wasReleased()) {
        setEN(true);  // Standbyモード
        delay(50);
        
        // パワー設定の切り替え
        switch (currentPower) {
            case 25:
                currentPower = 20;
                sendCommand(CMD_POWER_20, sizeof(CMD_POWER_20));
                break;
            case 20:
                currentPower = 15;
                sendCommand(CMD_POWER_15, sizeof(CMD_POWER_15));
                break;
            case 15:
                currentPower = 10;
                sendCommand(CMD_POWER_10, sizeof(CMD_POWER_10));
                break;
            case 10:
                currentPower = 5;
                sendCommand(CMD_POWER_5, sizeof(CMD_POWER_5));
                break;
            case 5:
                currentPower = 2;
                sendCommand(CMD_POWER_2, sizeof(CMD_POWER_2));
                break;
            default:
                currentPower = 25;
                sendCommand(CMD_POWER_25, sizeof(CMD_POWER_25));
                break;
        }

        // パワー設定後に実際の値を読み取る
        delay(50);
        readPower();
        
        // 画面表示
        M5.Lcd.setCursor(10, 100);
        M5.Lcd.setTextSize(2);
        M5.Lcd.printf("Power: %ddBm   ", currentPower_read);
        
        delay(5);
        setEN(false);  // Sleepモード
    }
    
    delay(5);
}