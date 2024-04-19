#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define backspace 0x08  // Mã cho phím backspace
#define reset 0x11      // Mã cho phím reset
#define dial 0x12       // Mã cho phím dial
#define enter '#'        // Mã cho phím enter
#define LED_PIN 2 

int counter = 0;
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Khởi tạo màn hình LCD
const byte ROWS = 4;  // Số hàng của bàn phím
const byte COLS = 4;  // Số cột của bàn phím
char keys[ROWS][COLS] = {
  { 1, 2, 3, 4 },
  { 5, 6, 7, 8 },
  { 9, 10, 11, 12 },
  { 13, 14, 15, 16 },
};
byte rowPins[ROWS] = { 14, 27, 26, 25 };  // Chân kết nối hàng
byte colPins[COLS] = { 33, 32, 18, 19 };  // Chân kết nối cột

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);  // Khởi tạo bàn phím

const char keyCharArray[16][5] PROGMEM = {
  "1ABC", "2DEF", "3GHI", "\x11",
  "4JKL", "5MNO", "6PQR", "\x12",
  "7STU", "8VWX", "9YZ.", "\x08",
  " ", "0", "#", "\x0A",
};

char finalText[80] = "";  // Mảng lưu trữ văn bản cuối cùng người dùng nhập
size_t finalTextIndex = 0;  // Chỉ số hiện tại trong mảng văn bản

// Hàm chuyển đổi ký tự thành mã Morse
String morseCode(char c) {
  switch (c) {
    case 'A': return ".-";
    case 'B': return "-...";
    case 'C': return "-.-.";
    case 'D': return "-..";
    case 'E': return ".";
    case 'F': return "..-.";
    case 'G': return "--.";
    case 'H': return "....";
    case 'I': return "..";
    case 'J': return ".---";
    case 'K': return "-.-";
    case 'L': return ".-..";
    case 'M': return "--";
    case 'N': return "-.";
    case 'O': return "---";
    case 'P': return ".--.";
    case 'Q': return "--.-";
    case 'R': return ".-.";
    case 'S': return "...";
    case 'T': return "-";
    case 'U': return "..-";
    case 'V': return "...-";
    case 'W': return ".--";
    case 'X': return "-..-";
    case 'Y': return "-.--";
    case 'Z': return "--..";
    case '1': return ".----";
    case '2': return "..---";
    case '3': return "...--";
    case '4': return "....-";
    case '5': return ".....";
    case '6': return "-....";
    case '7': return "--...";
    case '8': return "---..";
    case '9': return "----.";
    case '0': return "-----";
    case ' ': return "/";
    default: return "";  // For unsupported characters
  }
}

// Hàm chuyển đổi chuỗi ký tự sang mã Morse
String convertToMorse(const char* text) {
  String morse = "";
  for (int i = 0; text[i] != '\0'; i++) {
    morse += morseCode(toupper(text[i])) + " ";
  }
  return morse;
}

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  pinMode(LED_PIN, OUTPUT);
}
void loop() {
  char key = mygetKey();  // Lấy ký tự từ bàn phím

  if (key == 0x08) { // Kiểm tra nếu phím backspace được nhấn
    if (finalTextIndex > 0) {
      finalTextIndex--;  // Giảm chỉ số xuống để xóa ký tự
      finalText[finalTextIndex] = '\0';  // Xóa ký tự cuối cùng

      lcd.clear();                      
      lcd.setCursor(0, 0);              
      lcd.print("Text:");
      lcd.setCursor(0, 1);              
      lcd.print(finalText);              

      Serial.println(finalText);        
    }
  } else if (key != '\0' && key != enter) {
    finalText[finalTextIndex] = key;  
    finalTextIndex++;  
    if (finalTextIndex >= sizeof(finalText)) {
      finalTextIndex = sizeof(finalText) - 1;  
    }
    finalText[finalTextIndex] = '\0';  

    lcd.clear();                      
    lcd.setCursor(0, 0);              
    lcd.print("Text:");
    lcd.setCursor(0, 1);              
    lcd.print(finalText);              

    Serial.println(finalText);        
  } else if (key == enter) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Morse:");
    lcd.setCursor(0, 1);
    String morse = convertToMorse(finalText);
    lcd.print(morse);
    Serial.println(morse);
    blinkMorseCode(morse); // Gọi hàm nhấp nháy LED
    finalTextIndex = 0;
    finalText[0] = '\0';
  }
}

// Hàm lấy ký tự từ bàn phím với logic xử lý đa ký tự
char mygetKey() {
  const unsigned long keyDelay = 800ul;  // Define maximum delay between key presses
  static unsigned long keyTimer;
  static size_t keyCharIndex = 0;
  static char keyPrevious = '\0';
  static char keyPending = '\0';
  char key;
  char keyChar = '\0';

  if (keyPending == '\0') {
    key = keypad.getKey();  // Get key from keypad
  } else {
    key = keyPending;  // Get temporary key
    keyPending = '\0';
  }

  if (key) {
    if (keyPrevious == '\0') {
      if (strlen_P(keyCharArray[key - 1]) == 1) {
        keyChar = (char)pgm_read_byte(&keyCharArray[key - 1][0]);
      } else {
        keyTimer = millis();
        keyPrevious = key;
        keyCharIndex = 0;
      }
    } else {
      if (keyPrevious == key) {
        if ((millis() - keyTimer) < keyDelay) {
          keyCharIndex++;
          if (keyCharIndex >= strlen_P(keyCharArray[key - 1])) {
            keyCharIndex = 0;
          }
          keyTimer = millis();
        }
      } else {
        keyChar = (char)pgm_read_byte(&keyCharArray[keyPrevious - 1][keyCharIndex]);
        keyPrevious = '\0';  // Corrected line
        keyPending = key;
      }
    }
  }
  if ((keyPrevious != '\0') && ((millis() - keyTimer) > keyDelay)) {
    keyChar = (byte)pgm_read_byte(&keyCharArray[keyPrevious - 1][keyCharIndex]);
    keyPrevious = '\0';  // Ensure all instances are correctly terminated
  }
  return keyChar;
}

void blinkMorseCode(const String& code) {
  for (int i = 0; i < code.length(); i++) {
    switch (code[i]) {
      case '.':
        digitalWrite(LED_PIN, HIGH);
        delay(250); // Dấu chấm
        digitalWrite(LED_PIN, LOW);
        break;
      case '-':
        digitalWrite(LED_PIN, HIGH);
        delay(750); // Dấu gạch ngang
        digitalWrite(LED_PIN, LOW);
        break;
      case ' ':
        delay(250); // Nghỉ giữa các ký tự
        break;
    }
    delay(250); // Nghỉ giữa các dấu
  }
  delay(2000); // Nghỉ giữa các lần nhấp nháy
}