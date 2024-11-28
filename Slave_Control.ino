#include <AccelStepper.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "Phong 107"
#define WIFI_PASSWORD "12345679"


#define API_KEY "AIzaSyBAZ5GHovaXNtYS4YiA7tFBf01laFMfwNc"

/* 3. If work with RTDB, define the RTDB URL */
#define DATABASE_URL "https://esp32-firebase-demo-7391f-default-rtdb.asia-southeast1.firebasedatabase.app/"  //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

// Define the pins for the stepper motors
#define STEP_PIN_X 18  // GPIO pin for X-axis STEP
#define DIR_PIN_X 19   // GPIO pin for X-axis DIR
#define STEP_PIN_Y 23  // GPIO pin for Y-axis STEP
#define DIR_PIN_Y 22   // GPIO pin for Y-axis DIR
#define STEP_PIN_Z 21  // GPIO pin for Z-axis STEP
#define DIR_PIN_Z 5    // GPIO pin for Z-axis DIR
#define EN_PIN 13      // GPIO pin for ENABLE (if required)
/* 4. Define the Firebase Data object */
FirebaseData fbdo;

/* 5. Define the FirebaseAuth data for authentication data */
FirebaseAuth auth;

/* 6. Define the FirebaseConfig data for config data */
FirebaseConfig config;



// AccelStepper objects for controlling the stepper motors
AccelStepper stepperX(AccelStepper::DRIVER, STEP_PIN_X, DIR_PIN_X);
AccelStepper stepperY(AccelStepper::DRIVER, STEP_PIN_Y, DIR_PIN_Y);
AccelStepper stepperZ(AccelStepper::DRIVER, STEP_PIN_Z, DIR_PIN_Z);

// Last known positions of the motors
int lastX = 0, lastY = 0, lastZ = 0;
int x = 0, y = 0, z = 0;
unsigned long previousMillis = 0;
unsigned long interval = 1500;      // Interval to check Firebase for new data
unsigned long freControlStep = 50;  // Interval to check Firebase for new data
unsigned long dataMillis = 0;
int count = 0;
bool signupOK = false;

// Define Firebase Data object
FirebaseData stream;
NetworkClient client2;
void networkConnection() {
  // Neywork connection code here
}

void streamTimeoutCallback(bool timeout) {
  if (timeout)
    Serial.println("stream timed out, resuming...\n");

  if (!stream.httpConnected())
    Serial_Printf("error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
}
// Define the callback function to handle server status acknowledgement
void networkStatusRequestCallback() {
  // Set the network status based on your network client
  fbdo.setNetworkStatus(false /* or true */);
  stream.setNetworkStatus(false /* or true */);
}

void streamCallback(FirebaseStream data) {
  FirebaseJson *json = data.to<FirebaseJson *>();

  size_t len = json->iteratorBegin();
  FirebaseJson::IteratorValue value;

  // Parse the JSON object to get encoder values
  for (size_t i = 0; i < len; i++) {
    value = json->valueAt(i);
    if (String(value.key.c_str()) == "e1") {
      x = value.value.toInt();
    }
    if (String(value.key.c_str()) == "e2") {
      y = value.value.toInt();
    }
    if (String(value.key.c_str()) == "e3") {
      z = value.value.toInt();
    }
  }

  // Clear all list to free memory
  json->iteratorEnd();

  // Print the received values
  Serial.print("Received X: ");
  Serial.print(x);
  Serial.print(", Y: ");
  Serial.print(y);
  Serial.print(", Z: ");
  Serial.println(z);

  // Serial_Printf("sream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
  //               data.streamPath().c_str(),
  //               data.dataPath().c_str(),
  //               data.dataType().c_str(),
  //               data.eventType().c_str());
  // printResult(data);  // see addons/RTDBHelper.h
  // Serial.println();

  // This is the size of stream payload received (current and max value)
  // Max payload size is the payload size under the stream path since the stream connected
  // and read once and will not update until stream reconnection takes place.
  // This max value will be zero as no payload received in case of ESP8266 which
  // BearSSL reserved Rx buffer size is less than the actual stream payload.
  Serial_Printf("Received stream payload size: %d (Max. %d)\n\n", data.payloadLength(), data.maxPayloadLength());

  // Due to limited of stack memory, do not perform any task that used large memory here especially starting connect to server.
  // Just set this flag and check it status later.
}
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

  /* Assign the API key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL */
  config.database_url = DATABASE_URL;

  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  Firebase.reconnectNetwork(true);

  // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
  // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  fbdo.setBSSLBufferSize(1024 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);


  Serial.print("Sign up new user... ");

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;

    /** if the database rules were set as in the example "EmailPassword.ino"
         * This new user can be access the following location.
         *
         * "/UserData/<user uid>"
         *
         * The new user UID or <user uid> can be taken from auth.token.uid
         */
  } else
    Serial.printf("%s\n", config.signer.signupError.message.c_str());

  // If the signupError.message showed "ADMIN_ONLY_OPERATION", you need to enable Anonymous provider in project's Authentication.

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  // see addons/TokenHelper.h

  /** The id token (C++ string) will be available from config.signer.tokens.id_token
     * if the sig-up was successful.
     *
     * Now you can initialize the library using the internal created credentials.
     *
     * If the sign-up was failed, the following function will initialize because
     * the internal authentication credentials are not created.
     */
  Firebase.begin(&config, &auth);
  stream.setGenericClient(&client2, networkConnection, networkStatusRequestCallback);
  stream.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  if (!Firebase.RTDB.beginStream(&stream, "/MasterArm"))
    Serial_Printf("sream begin error, %s\n\n", stream.errorReason().c_str());

  Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);
  // Stepper motor setup
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);  // Enable the stepper motors

  // Cài đặt tốc độ và gia tốc cho các trục
  stepperX.setMaxSpeed(400);      // Tốc độ tối đa (steps per second)
  stepperX.setAcceleration(120);  // Gia tốc (steps per second squared)

  stepperY.setMaxSpeed(500);
  stepperY.setAcceleration(150);

  stepperZ.setMaxSpeed(500);
  stepperZ.setAcceleration(150);
  Serial.print("Before start ");
  for (int i = 0; i < 10; i++) {
    Serial.print(".");
    if (i % 3 == 0) {
      Serial.print(i / 3);
    }
    delay(300);
  }
}

void loop() {

  Firebase.ready();  //should be called repeatedly to handle authentication tasks.
  // Kiểm tra xem đã đến lúc gửi dữ liệu chưa
  if (millis() - previousMillis >= freControlStep) {
    previousMillis = millis();  // Cập nhật thời gian trước
      // Check if any axis position has changed and move the motors
    if (x != lastX) {
      stepperX.runToNewPosition(x*2.2);
      lastX = x;
    }
    if (y != lastY) {
      stepperY.runToNewPosition(y*2.1);
      lastY = y;
    }
    if (z != lastZ) {
      stepperZ.runToNewPosition(z*2.1);
      lastZ = z;
    }
  }
  // Run the stepper motors
  stepperX.run();
  stepperY.run();
  stepperZ.run();
}
