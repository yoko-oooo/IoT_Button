#include <M5Stack.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"

// ------------------------------------
// 定数定義
// ------------------------------------
const char* MQTT_BROKER = IOT_HUB_HOSTNAME;
const int   MQTT_PORT   = 8883;

String mqttTopic = "devices/" + String(DEVICE_ID) + "/messages/events/";

// ------------------------------------
// グローバル変数
// ------------------------------------
WiFiClientSecure wifiClient;
PubSubClient     mqttClient(wifiClient);
bool isSeatInUse = false;

// ------------------------------------
// 関数プロトタイプ
// ------------------------------------
void connectWiFi();
void connectMQTT();
void sendSeatStatus(bool inUse);
void updateDisplay(bool inUse);

// ------------------------------------
// セットアップ
// ------------------------------------
void setup() {
    M5.begin();
    Serial.begin(115200);

    M5.Lcd.setTextSize(2);
    M5.Lcd.println("Aki-Chance IoT");
    M5.Lcd.println("起動中...");

    connectWiFi();

    // TLS証明書検証をスキップ（プロトタイプ用）
    wifiClient.setInsecure();

    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    connectMQTT();

    updateDisplay(isSeatInUse);
}

// ------------------------------------
// メインループ
// ------------------------------------
void loop() {
    M5.update();

    if (!mqttClient.connected()) {
        connectMQTT();
    }
    mqttClient.loop();

    // ボタンA: 「使用中」に切替
    if (M5.BtnA.wasPressed() && !isSeatInUse) {
        isSeatInUse = true;
        sendSeatStatus(true);
        updateDisplay(true);
    }

    // ボタンC: 「空き」に切替
    if (M5.BtnC.wasPressed() && isSeatInUse) {
        isSeatInUse = false;
        sendSeatStatus(false);
        updateDisplay(false);
    }

    delay(50);
}

// ------------------------------------
// WiFi接続
// ------------------------------------
void connectWiFi() {
    M5.Lcd.println("WiFi接続中...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int retry = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        retry++;
        if (retry > 20) {
            M5.Lcd.println("WiFi接続失敗");
            return;
        }
    }
    M5.Lcd.println("WiFi接続OK");
}

// ------------------------------------
// MQTT接続（IoT Hub）
// ------------------------------------
void connectMQTT() {
    String mqttUser = String(IOT_HUB_HOSTNAME)
                    + "/"
                    + String(DEVICE_ID)
                    + "/?api-version=2021-04-12";

    M5.Lcd.println("IoTHub接続中...");

    int retry = 0;
    while (!mqttClient.connected()) {
        if (mqttClient.connect(
                DEVICE_ID,
                mqttUser.c_str(),
                DEVICE_KEY)) {
            M5.Lcd.println("IoTHub接続OK");
        } else {
            Serial.print("接続失敗. rc=");
            Serial.println(mqttClient.state());
            delay(3000);
            retry++;
            if (retry > 5) {
                M5.Lcd.println("IoTHub接続失敗");
                return;
            }
        }
    }
}

// ------------------------------------
// 席状態をIoT Hubへ送信
// ------------------------------------
void sendSeatStatus(bool inUse) {
    StaticJsonDocument<256> doc;
    doc["deviceId"]  = DEVICE_ID;
    doc["seatId"]    = SEAT_ID;
    doc["status"]    = inUse ? "in_use" : "available";
    doc["timestamp"] = millis();

    char payload[256];
    serializeJson(doc, payload);

    bool result = mqttClient.publish(
        mqttTopic.c_str(),
        payload
    );

    if (result) {
        Serial.println("送信成功: " + String(payload));
    } else {
        Serial.println("送信失敗");
    }
}

// ------------------------------------
// ディスプレイ更新
// ------------------------------------
void updateDisplay(bool inUse) {
    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("=== Aki-Chance ===");
    M5.Lcd.println("");

    if (inUse) {
        M5.Lcd.setTextColor(RED);
        M5.Lcd.println("  [ IN USE ]");
    } else {
        M5.Lcd.setTextColor(GREEN);
        M5.Lcd.println("  [ AVAILABLE ]");
    }

    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("");
    M5.Lcd.println("A: Set In Use");
    M5.Lcd.println("C: Set Available");
}

