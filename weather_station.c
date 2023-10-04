#include <HTTPClient.h>
#include "time.h"
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include "ThingSpeak.h"
#include <DHT.h>

const char *NTP_TIME_SERVER = "pool.ntp.org";
const long GMT_T_Offset_sec = 19800;
const int Daylight_T_Offset_sec = 0;
const char *WIFI_SSID = "A10";
const char *WIFI_PW = "mkcs5050";
String Google_Sid = "AKfycbwDJHhGXGFXvPgHpiav7JRLJafsY-sZCGwmnR9N0DLFVfxKlfS6kL6fjnZKny1eXhl2";
unsigned long ThingSpeak_Channel_ID = 1874246;
const char *Write_API_Key = "NHX0I80ZHRJ2JTIF";
WiFiClient client;

#define DHT_SENSOR_PIN 4
#define DHT_SENSOR_TYPE DHT11
#define micros2sec_FACTOR 1000000ULL
#define GO_TO_SLEEP 60
RTC_DATA_ATTR int b_Count = 0;

#include "SPIFFS.h"
#include <vector>
using namespace std;

DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
Adafruit_BMP085 bmp180;

void print_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Using RTC IO, wakeup caused by external signal ");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            Serial.println("Using RTC CNTL, wakeup caused by external signal ");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Timer induced wake up ");
            break;
        default:
            Serial.printf("Wakeup wasn't occurred by deep sleep: %d\n", wakeup_reason);
            break;
    }
}

void httprequest(String temp, String humi, String press, String Google_Sid) {
    String URL_Final = "https://script.google.com/macros/s/" + Google_Sid + "/exec?" + "&temperature=" + temp + "&humidity=" + humi + "&pressure=" + press;
    Serial.println(URL_Final);
    HTTPClient http;
    http.begin(URL_Final.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET();
    http.end();
}

void listAllFiles() {
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file) {
        Serial.print("F: ");
        Serial.println(file.name());
        file = root.openNextFile();
    }
}

void setup() {
    delay(1000);
    Serial.begin(115200);
    SPIFFS.begin(true);
    File t_file;
    File h_file;
    File p_file;
    dht_sensor.begin();
    char a = 0;
    char b = 0;
    char c = 0;
    char d = 0;
    char St_up_Epoint;
    char n = 60;
    char Wifi_connect_waiting = 15;
    String temparray[n];
    String humidarray[n];
    String pressarray[n];
    delay(1000);

    if (!bmp180.begin()) {
        Serial.println("Check Bmp180 connection");
        while (1) {}
    }

    Serial.println();
    Serial.print("Setting up wifi: ");
    Serial.println(WIFI_SSID);
    Serial.flush();
    WiFi.begin(WIFI_SSID, WIFI_PW);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        c++;

        if (c > Wifi_connect_waiting) {
            Serial.println("\n Data writing offline");

            if (!(SPIFFS.exists("/t.txt")) && !(SPIFFS.exists("/h.txt")) && !(SPIFFS.exists("/p.txt"))) {
                File t_file = SPIFFS.open("/t.txt", FILE_WRITE);
                File h_file = SPIFFS.open("/h.txt", FILE_WRITE);
                File p_file = SPIFFS.open("/p.txt", FILE_WRITE);
                Serial.println("file write in write mode");
                t_file.println(String(temp));
                h_file.println(String(humi));
                p_file.println(String(press));
                t_file.close();
                h_file.close();
                p_file.close();
            } else {
                File tempfilea = SPIFFS.open("/t.txt", "a");
                File humidityfilea = SPIFFS.open("/h.txt", "a");
                File pressurefilea = SPIFFS.open("/p.txt", "a");
                tempfilea.println(String(temp));
                humidityfilea.println(String(humi));
                pressurefilea.println(String(press));
                Serial.println("file write in append mode");
                tempfilea.close();
                humidityfilea.close();
                pressurefilea.close();
            }
        }

        c = 0;
        break;
    }

    ThingSpeak.begin(client);
    configTime(GMT_T_Offset_sec, Daylight_T_Offset_sec, NTP_TIME_SERVER);
    print_wakeup_reason();
    ++b_Count;
    Serial.println("B_number: " + String(b_Count));
    esp_sleep_enable_timer_wakeup(GO_TO_SLEEP * micros2sec_FACTOR);

    if (isnan(temp) || isnan(humi) || isnan(press)) {
        Serial.println("DHT sensor failure!");
        ThingSpeak.setField(1, temp);
        ThingSpeak.setField(2, humi);
        ThingSpeak.setField(3, press);
        int x = ThingSpeak.writeFields(ThingSpeak_Channel_ID, Write_API_Key);

        if (x == 200) {
            Serial.println("Channel updated without errors");
        } else {
            Serial.println("Channel is not updated. Http error");
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            static bool flag = false;
            struct tm timeinfo;

            if ((SPIFFS.exists("/t.txt")) && (SPIFFS.exists("/h.txt")) && (SPIFFS.exists("/p.txt"))) {
                File tf_1 = SPIFFS.open("/t.txt");
                File hf_1 = SPIFFS.open("/h.txt");
                File pf_1 = SPIFFS.open("/p.txt");
                vector<String> vec1;
                vector<String> vec2;
                vector<String> vec3;

                while (tf_1.available()) {
                    vec1.push_back(tempfile1.readStringUntil('\n'));
                }

                while (hf_1.available()) {
                    vec2.push_back(humidityfile1.readStringUntil('\n'));
                }

                while (pf_1.available()) {
                    for (String str1 : vec1) {
                        for (String str2 : vec2) {
                            for (String str3 : vec3) {
                                vec3.push_back(pressfile1.readStringUntil('\n'));
                            }

                            tf_1.close();
                            hf_1.close();
                            pf_1.close();
                            temparray[a] = str1;
                            a++;
                        }

                        humidarray[b] = str2;
                        b++;
                        pressarray[c] = str3;
                        c++;
                    }
                }

                while (d <= n) {
                    if (temparray[d] == "0" && humidarray[d] == "0" && pressarray[d] == "0") {
                        Serial.println("\nArray is empty");
                        St_up_Epoint = d;
                        break;
                    } else {
                        Serial.println("Uploading data offline... ");
                        Serial.print("temp :");
                        Serial.print(temparray[d]);
                        Serial.print("\n");
                        Serial.print("humid :");
                        Serial.print(humidarray[d]);
                        Serial.print("\n");
                        Serial.print(pressarray[d]);
                        Serial.print("\n");
                        httprequest(temparray[d], humidarray[d], pressarray[d], Google_Sid);
                    }

                    d++;
                }

                if (d == n || d == St_up_Epoint) {
                    Serial.println("\n\nREMOVING FILES");
                    listAllFiles();
                    SPIFFS.remove("/t.txt");
                    SPIFFS.remove("/h.txt");
                    SPIFFS.remove("/p.txt");
                    Serial.println("\n\nREMOVED FILES");
                    listAllFiles();
                    a = 0;
                    b = 0;
                    c = 0;
                    delay(1000);
                } else {
                    Serial.print("temperature: ");
                    Serial.print(temp);
                    Serial.println("oC ");
                    Serial.print("humidity: ");
                    Serial.println(humi);
                    Serial.println("%");
                    Serial.print("pressure = ");
                    Serial.print(press);
                    Serial.println(" Pa");
                    delay(100);
                }
            }

            if (!getLocalTime(&timeinfo)) {
                Serial.println("Error in obtaining time");
                return;
            }

            char T_Str_Buff[50];
            strftime(T_Str_Buff, sizeof(T_Str_Buff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
            String asString(T_Str_Buff);
            asString.replace(" ", "-");
            Serial.print("Time:");
            Serial.println(asString);
            httprequest(String(temp), String(humi), String(press), Google_Sid);
        }
    }
}

void loop() {
    // Your loop code here
    // Note: Currently, your loop function is empty
    Serial.println("Time to sleep");
    Serial.flush();
    esp_deep_sleep_start();
}
