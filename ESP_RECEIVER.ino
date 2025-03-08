#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#endif
#include <FirebaseESP32>
#include "DHT.h"

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "Van Vuong"
#define WIFI_PASSWORD "n06111977"

// Insert Firebase project API Key
#define API_KEY "AIzaSyB-T62koUSSyhabj0ToiPRthP3TSeQp73Y"

// Insert RTDB URL
#define DATABASE_URL "https://esp32-63372-default-rtdb.asia-southeast1.firebasedatabase.app/" 

// Define DHT sensor type and pin
#define DHTPIN 4       // GPIO pin connected to DHT sensor
#define DHTTYPE DHT11  // DHT11 or DHT22

DHT dht(DHTPIN, DHTTYPE);

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

WiFiServer server(80); // Tạo server HTTP trên cổng 80

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
 /* Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
*/
  // Start HTTP server
  server.begin();
  Serial.println("HTTP Server started.");

  // Assign Firebase credentials
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Sign up to Firebase
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase Sign-Up OK");
    signupOK = true;
  } else {
    Serial.printf("Firebase Sign-Up Error: %s\n", config.signer.signupError.message.c_str());
  }

  // Assign the callback function for token status
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Upload the device's IP address to Firebase
  String ipAddress = WiFi.localIP().toString();
  if (Firebase.RTDB.setString(&fbdo, "device/ip", ipAddress)) {
    Serial.print("Device IP uploaded successfully: ");
    Serial.println(ipAddress);
  } else {
    Serial.println("Failed to upload Device IP!");
    Serial.println(fbdo.errorReason());
  }
}

void loop() {
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  // Handle HTTP requests
  WiFiClient client = server.available(); // Kiểm tra kết nối mới từ client
  if (client) {
    Serial.println("New client connected.");
    String request = client.readStringUntil('\r'); // Đọc request từ client
    Serial.println("Request: " + request);

    // Trả về thông tin IP của ESP32
    if (request.indexOf("GET /info") >= 0) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println();
      client.print("{\"ip\":\"");
      client.print(WiFi.localIP());
      client.print("\",\"status\":\"active\"}");
    } else {
      // Trả về 404 nếu không tìm thấy endpoint
      client.println("HTTP/1.1 404 Not Found");
      client.println();
    }

    delay(10);
    client.stop(); // Đóng kết nối
    Serial.println("Client disconnected.");
  }

  // Firebase upload
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 2000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    // Read temperature and humidity from the DHT sensor
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    // Check if readings are valid
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    // Upload temperature to Firebase
    if (Firebase.RTDB.setFloat(&fbdo, "sensor/temperature", temperature)) {
      Serial.println("Temperature uploaded successfully!");
    } else {
      Serial.println("Failed to upload temperature!");
      Serial.println(fbdo.errorReason());
    }

    // Upload humidity to Firebase
    if (Firebase.RTDB.setFloat(&fbdo, "sensor/humidity", humidity)) {
      Serial.println("Humidity uploaded successfully!");
    } else {
      Serial.println("Failed to upload humidity!");
      Serial.println(fbdo.errorReason());
    }
  }
}