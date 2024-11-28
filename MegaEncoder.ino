#include <ArduinoJson.h>  // Thư viện JSON

volatile long counter1 = 0, counter2 = 0, counter3 = 0;  // Biến đếm cho 3 encoder
volatile long temp1 = 0, temp2 = 0, temp3 = 0;           // Biến lưu giá trị trước của counter

volatile int lastState1A = HIGH, lastState2A = HIGH, lastState3A = HIGH;  // Trạng thái cũ của chân A của các encoder
volatile int lastState1B = HIGH, lastState2B = HIGH, lastState3B = HIGH;  // Trạng thái cũ của chân B của các encoder

unsigned long previousMillis = 0;  // Thời gian trước khi gửi dữ liệu
const long interval = 150;         // Khoảng thời gian 100ms (0.1s)

void setup() {
  Serial.begin(9600);   // Cổng Serial 0 (để debug qua Serial Monitor)
  Serial2.begin(9600);  // Cổng TX 2 (Serial 2)

  // Cấu hình các chân đầu vào với pull-up
  pinMode(2, INPUT_PULLUP);   // Chân A của Encoder 1 (GPIO 2, interrupt 0)
  pinMode(3, INPUT_PULLUP);   // Chân B của Encoder 1 (GPIO 3, interrupt 1)
  pinMode(20, INPUT_PULLUP);  // Chân A của Encoder 2 (GPIO 20)
  pinMode(21, INPUT_PULLUP);  // Chân B của Encoder 2 (GPIO 21)
  pinMode(18, INPUT_PULLUP);  // Chân A của Encoder 3 (GPIO 18)
  pinMode(19, INPUT_PULLUP);  // Chân B của Encoder 3 (GPIO 19)
  pinMode(5, INPUT_PULLUP);   // Chân B của Encoder 3 (GPIO 5)

  // Thiết lập ngắt cho Encoder 1
  attachInterrupt(digitalPinToInterrupt(2), encoder1_A, CHANGE);  // Chân A của Encoder 1 (interrupt 0)
  attachInterrupt(digitalPinToInterrupt(3), encoder1_B, CHANGE);  // Chân B của Encoder 1 (interrupt 1)

  // Thiết lập ngắt cho Encoder 2
  attachInterrupt(digitalPinToInterrupt(20), encoder2_A, CHANGE);  // Chân A của Encoder 2
  attachInterrupt(digitalPinToInterrupt(21), encoder2_B, CHANGE);  // Chân B của Encoder 2

  // Thiết lập ngắt cho Encoder 3
  attachInterrupt(digitalPinToInterrupt(18), encoder3_A, CHANGE);  // Chân A của Encoder 3
  attachInterrupt(digitalPinToInterrupt(19), encoder3_B, CHANGE);  // Chân B của Encoder 3
}

void loop() {

  // Đọc giá trị của chân 5
  int signal = !digitalRead(5);
  unsigned long currentMillis = millis();

  // Kiểm tra xem đã đến lúc gửi dữ liệu chưa
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // Cập nhật thời gian trước

    // Chỉ gửi dữ liệu khi có sự thay đổi hoặc đổi hướng
    if (counter1 != temp1 || counter2 != temp2 || counter3 != temp3) {
      // Tạo đối tượng JSON
      StaticJsonDocument<200> doc;  // Khai báo đối tượng JSON với dung lượng tối đa 200 byte
      doc["encoder1"] = counter1;
      doc["encoder2"] = counter2;
      doc["encoder3"] = counter3;
      doc["process"] = signal;


      // // Cập nhật lại giá trị trước
      temp1 = counter1;
      temp2 = counter2;
      temp3 = counter3;
      // Chuyển đối tượng JSON thành chuỗi và gửi qua Serial2
      char output[256];            // Mảng để chứa chuỗi JSON
      serializeJson(doc, output);  // Serialize đối tượng JSON thành chuỗi
      Serial.println(output);      // Gửi chuỗi JSON qua UART (TX2)
      Serial2.println(output);     // Gửi chuỗi JSON qua UART (TX2)
    }
  }
}

// Hàm ngắt khi tín hiệu trên chân A của Encoder 1 thay đổi
void encoder1_A() {
  int currentState = digitalRead(2);                 // Đọc trạng thái chân A
  if (lastState1A == LOW && currentState == HIGH) {  // Sự thay đổi của chân A
    if (digitalRead(3) == LOW) {
      counter1++;  // Quay theo chiều kim đồng hồ
    } else {
      counter1--;  // Quay ngược chiều kim đồng hồ
    }
  }
  lastState1A = currentState;  // Lưu trạng thái hiện tại của chân A
}

// Hàm ngắt khi tín hiệu trên chân B của Encoder 1 thay đổi
void encoder1_B() {
  int currentState = digitalRead(3);                 // Đọc trạng thái chân B
  if (lastState1B == LOW && currentState == HIGH) {  // Sự thay đổi của chân B
    if (digitalRead(2) == LOW) {
      counter1--;  // Quay ngược chiều kim đồng hồ
    } else {
      counter1++;  // Quay theo chiều kim đồng hồ
    }
  }
  lastState1B = currentState;  // Lưu trạng thái hiện tại của chân B
}

// Hàm ngắt khi tín hiệu trên chân A của Encoder 2 thay đổi
void encoder2_A() {
  int currentState = digitalRead(20);                // Đọc trạng thái chân A
  if (lastState2A == LOW && currentState == HIGH) {  // Sự thay đổi của chân A
    if (digitalRead(21) == LOW) {
      counter2++;  // Quay theo chiều kim đồng hồ
    } else {
      counter2--;  // Quay ngược chiều kim đồng hồ
    }
  }
  lastState2A = currentState;  // Lưu trạng thái hiện tại của chân A
}

// Hàm ngắt khi tín hiệu trên chân B của Encoder 2 thay đổi
void encoder2_B() {
  int currentState = digitalRead(21);                // Đọc trạng thái chân B
  if (lastState2B == LOW && currentState == HIGH) {  // Sự thay đổi của chân B
    if (digitalRead(20) == LOW) {
      counter2--;  // Quay ngược chiều kim đồng hồ
    } else {
      counter2++;  // Quay theo chiều kim đồng hồ
    }
  }
  lastState2B = currentState;  // Lưu trạng thái hiện tại của chân B
}

// Hàm ngắt khi tín hiệu trên chân A của Encoder 3 thay đổi
void encoder3_A() {
  int currentState = digitalRead(18);                // Đọc trạng thái chân A
  if (lastState3A == LOW && currentState == HIGH) {  // Sự thay đổi của chân A
    if (digitalRead(19) == LOW) {
      counter3++;  // Quay theo chiều kim đồng hồ
    } else {
      counter3--;  // Quay ngược chiều kim đồng hồ
    }
  }
  lastState3A = currentState;  // Lưu trạng thái hiện tại của chân A
}

// Hàm ngắt khi tín hiệu trên chân B của Encoder 3 thay đổi
void encoder3_B() {
  int currentState = digitalRead(19);                // Đọc trạng thái chân B
  if (lastState3B == LOW && currentState == HIGH) {  // Sự thay đổi của chân B
    if (digitalRead(18) == LOW) {
      counter3--;  // Quay ngược chiều kim đồng hồ
    } else {
      counter3++;  // Quay theo chiều kim đồng hồ
    }
  }
  lastState3B = currentState;  // Lưu trạng thái hiện tại của chân B
}
