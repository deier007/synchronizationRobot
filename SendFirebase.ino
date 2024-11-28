
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>


// Define the WiFi credentials
#define WIFI_SSID "Phong 107"
#define WIFI_PASSWORD "12345679"

/** 2. Define the API key
 *
 * The API key (required) can be obtained since you created the project and set up
 * the Authentication in Firebase console. Then you will get the API key from
 * Firebase project Web API key in Project settings, on General tab should show the
 * Web API Key.
 *
 * You may need to enable the Identity provider at https://console.cloud.google.com/customer-identity/providers
 * Select your project, click at ENABLE IDENTITY PLATFORM button.
 * The API key also available by click at the link APPLICATION SETUP DETAILS.
 *
 */

#define API_KEY "AIzaSyBAZ5GHovaXNtYS4YiA7tFBf01laFMfwNc"


/* 3. If work with RTDB, define the RTDB URL */
#define DATABASE_URL "https://esp32-firebase-demo-7391f-default-rtdb.asia-southeast1.firebasedatabase.app/"  //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/** 5. Define the database secret (optional)
 *
 * This database secret needed only for this example to modify the database rules
 *
 * If you edit the database rules yourself, this is not required.
 */

/* 6. Define the Firebase Data object */
FirebaseData fbdo;

/* 7. Define the FirebaseAuth data for authentication data */
FirebaseAuth auth;

/* 8. Define the FirebaseConfig data for config data */
FirebaseConfig config;
FirebaseJson json;  // or constructor with contents e.g. FirebaseJson json("{\"a\":true}");

unsigned long dataMillis = 0;
int count = 0;


// Truy xuất các giá trị từ JSON
int encoder1;
int encoder2;
int encoder3;
int process;

void setup() {

  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  unsigned long ms = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;
  /* Assign the RTDB URL */
  config.database_url = DATABASE_URL;

  // The WiFi credentials are required for Pico W
  // due to it does not have reconnect feature.

  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  Firebase.reconnectNetwork(true);
  // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
  // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);
  fbdo.setResponseSize(4096);

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  // see addons/TokenHelper.h

  // Sign up new user (if needed)
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase SignUp successful");
  } else {
    Serial.printf("Error: %s\n", config.signer.signupError.message.c_str());
  }
  Firebase.begin(&config, &auth);
  // Khởi động UART2 cho giao tiếp (D22 làm RX, D23 làm TX)
  // Sử dụng Serial2 cho RX2 (pin 22) và TX2 (pin 23) của ESP32
  Serial2.begin(9600, SERIAL_8N1, 22, 23);  // Baudrate 9600, RX2 trên pin 22, TX2 trên pin 23

  // Kiểm tra nếu Serial2 đã sẵn sàng
  while (!Serial2) {
    // Chờ đợi để kết nối với Serial Monitor
  }

  Serial.println("ESP32 đang sẵn sàng nhận dữ liệu qua UART2 từ Mega.");
}

void loop() {

  // Firebase.ready() should be called repeatedly to handle authentication tasks.

  if (millis() - dataMillis > 100 && Firebase.ready()) {
    dataMillis = millis();
    if (Serial2.available()) {
      // Đọc chuỗi dữ liệu JSON
      String receivedData = Serial2.readStringUntil('\n');
      Serial.println(receivedData);

      // Khai báo đối tượng JSON
      StaticJsonDocument<200> doc;

      // Phân tích chuỗi JSON vào đối tượng doc
      DeserializationError error = deserializeJson(doc, receivedData);

      if (error) {
        Serial.print("Lỗi phân tích JSON: ");
        Serial.println(error.f_str());
        return;
      }

      // Truy xuất các giá trị từ JSON
      encoder1 = doc["encoder1"];
      encoder2 = doc["encoder2"];
      encoder3 = doc["encoder3"];
      process = doc["process"];

      // Lấy timestamp hiện tại
      unsigned long timestamp = millis();

      // Tính thời gian bắt đầu
      unsigned long startTime = millis();

      // Thêm giá trị vào JSON
      json.set("e1", encoder1);
      json.set("e2", encoder2);
      json.set("e3", encoder3);
      json.set("p", process);

      // Gửi dữ liệu lên Firebase
      String path = "/MasterArm";
      bool success = Firebase.RTDB.setJSON(&fbdo, path, &json);

      // Tính thời gian gửi
      unsigned long endTime = millis();
      unsigned long elapsedTime = endTime - startTime;  // Thời gian đã qua trong mili giây

      // Thêm thời gian vào JSON
      json.set("time_taken_ms", elapsedTime);  // Thêm thời gian vào JSON

      if (success) {
        Serial.println("Gửi dữ liệu thành công!");
      } else {
        Serial.printf("Lỗi: %s\n", fbdo.errorReason().c_str());
      }

      // In ra thời gian gửi
      Serial.print("Thời gian gửi dữ liệu: ");
      Serial.print(elapsedTime);
      Serial.println(" ms");
    }

    // You can use refresh token from Firebase.getRefreshToken() to sign in next time without providing Email and Password.
    // See SignInWithRefreshIDToken example.
  }
}
