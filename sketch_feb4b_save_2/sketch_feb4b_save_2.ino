#define BLYNK_TEMPLATE_ID "TMPL3Zcy2rQO6"
#define BLYNK_TEMPLATE_NAME "Switch ON OFF LED"
#define BLYNK_AUTH_TOKEN "SoYOUHMfbTiOdVn6jOhiv7u891lKvzRJ"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Preferences.h>

#define FLOW_SENSOR_1_PIN 4
#define FLOW_SENSOR_2_PIN 5
#define BOOT_PIN 0
#define FLOW_CALIBRATION 4.5

#define TFT_CS   14
#define TFT_RST  15
#define TFT_DC   32
#define TFT_MOSI 23
#define TFT_SCLK 18
#define SPI_SPEED 40000000  // Increase SPI speed to 40MHz

SPIClass spi = SPIClass(VSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

char ssid[] = "C3StreamLand";
char pass[] = "Ccube$123";

volatile int pulseCount1 = 0, pulseCount2 = 0;
float flowRate1 = 0, total_litre1 = 0;
float flowRate2 = 0, total_litre2 = 0;
float lastFlowRate1 = -1, lastTotalLitre1 = -1;
float lastFlowRate2 = -1, lastTotalLitre2 = -1;

unsigned long oldTime = 0;

Preferences preferences;

void IRAM_ATTR pulseCounter1() { pulseCount1++; }
void IRAM_ATTR pulseCounter2() { pulseCount2++; }

void connectWiFi() {
    WiFi.begin(ssid, pass);
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi");
        Blynk.config(BLYNK_AUTH_TOKEN);
        Blynk.connect();
    } else {
        Serial.println("\nWiFi connection failed, running offline mode.");
    }
}

void setup() {
    Serial.begin(115200);

    preferences.begin("flow_data", false);

    spi.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
    tft.init(240, 320);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLACK);

    tft.setTextSize(10);
    tft.setCursor(80, 35);
    tft.print("HAZ");
    tft.setCursor(50, 135);
    tft.print("LABS");
    delay(2000);
    tft.fillScreen(ST77XX_BLACK);

    pinMode(BOOT_PIN, INPUT_PULLUP);
    if (digitalRead(BOOT_PIN) == LOW) {
        total_litre1 = total_litre2 = 0;
        preferences.clear();
        Serial.println("Reset values to 0 due to BOOT button press");
    } else {
        total_litre1 = preferences.getFloat("total_litre1", 0);
        total_litre2 = preferences.getFloat("total_litre2", 0);
    }

    pinMode(FLOW_SENSOR_1_PIN, INPUT_PULLUP);
    pinMode(FLOW_SENSOR_2_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_1_PIN), pulseCounter1, FALLING);
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_2_PIN), pulseCounter2, FALLING);

    connectWiFi();
    oldTime = millis();
}

void loop() {
    unsigned long currentTime = millis();
    if (currentTime - oldTime >= 1000) {
        detachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_1_PIN));
        detachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_2_PIN));

        flowRate1 = ((100.0 / (currentTime - oldTime)) * pulseCount1) / FLOW_CALIBRATION;
        total_litre1 += flowRate1;
        flowRate2 = ((100.0 / (currentTime - oldTime)) * pulseCount2) / FLOW_CALIBRATION;
        total_litre2 += flowRate2;

        if (total_litre1 >= 9999) total_litre1 = 0;
        if (total_litre2 >= 9999) total_litre2 = 0;

        pulseCount1 = pulseCount2 = 0;
        oldTime = currentTime;

        attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_1_PIN), pulseCounter1, FALLING);
        attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_2_PIN), pulseCounter2, FALLING);

        if (flowRate1 != lastFlowRate1 || total_litre1 != lastTotalLitre1 ||
            flowRate2 != lastFlowRate2 || total_litre2 != lastTotalLitre2) {
            updateDisplay(flowRate1, total_litre1, flowRate2, total_litre2);
            preferences.putFloat("total_litre1", total_litre1);
            preferences.putFloat("total_litre2", total_litre2);
            lastFlowRate1 = flowRate1;
            lastTotalLitre1 = total_litre1;
            lastFlowRate2 = flowRate2;
            lastTotalLitre2 = total_litre2;
        }
    }
}

void updateDisplay(float flow1, float total1, float flow2, float total2) {
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);  // Use black background to overwrite old values

    tft.setTextSize(4);
    // Flow 1
    tft.setCursor(10, 30);
    tft.print("F1:");
    tft.setTextSize(4);
    tft.setCursor(80, 30);
    tft.print("       ");  // Clear previous value
    tft.setCursor(80, 30);
    tft.print(flow1);
    tft.setTextSize(3);
    tft.setCursor(180, 30);
    tft.print("L/m");

    tft.setTextSize(4);
    // Total 1
    String total1_str = String(total1, 1);
    int textWidth = total1_str.length() * 12;
    tft.setCursor(10, 90);
    tft.print("T1:");
    tft.setTextSize(4);
    tft.setCursor(80, 90);
    tft.print("       ");
    tft.setCursor(80, 90);
    tft.print(total1);
    tft.setTextSize(3);
    tft.setCursor(180 + textWidth + 5, 90);
    tft.print("L");

    tft.setTextSize(4);
    // Flow 2
    tft.setCursor(10, 150);
    tft.print("F2:");
    tft.setTextSize(4);
    tft.setCursor(80, 150);
    tft.print("       ");
    tft.setCursor(80, 150);
    tft.print(flow2);
    tft.setTextSize(3);
    tft.setCursor(180, 150);
    tft.print("L/m");

    tft.setTextSize(4);
    // Total 2
    String total2_str = String(total2, 1);
    textWidth = total2_str.length() * 12;
    tft.setCursor(10, 210);
    tft.print("T2:");
    tft.setTextSize(4);
    tft.setCursor(80, 210);
    tft.print("       ");
    tft.setCursor(80, 210);
    tft.print(total2);
    tft.setTextSize(3);
    tft.setCursor(180 + textWidth + 5, 210);
    tft.print("L");

    delay(5);  // Small delay to prevent flickering
}
