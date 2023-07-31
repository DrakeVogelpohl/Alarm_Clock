#include <Arduino.h>
#include <WiFi.h>
#include "esp_sntp.h"
#include <LiquidCrystal_I2C.h>
#include<string>
#include "time.h"

// Pin declarations
const int displayPin = 36;
const int dispAlarmPin = 39;
const int setAlarmHourPin = 34;
const int setAlarmMinTensPin = 35;
const int setAlarmMinOnesPin = 32;

const int i2c_scl_pin = 22;
const int i2c_sda_pin = 21;

const int motorCWPin = 19;
const int motorCCWPin = 18;
const int buzzerPin = 5;
const int bigOffButtonPin = 33;

// Delaring pins for sleep
#define BUTTON_PIN_BITMASK 0x9000000000

// WiFi inits
const char* ssid       = "";
const char* password   = "";
const char* ntpServer = "1.us.pool.ntp.org"; //provided the best connection for me

// Timer and alarm inits
int hour;
int minute;
int second;
RTC_DATA_ATTR uint8_t alarm_min;
RTC_DATA_ATTR int8_t alarm_hour;
#define NTP_REQUEST_TIMEOUT 1000
bool isSetNTP = false; 

// LCD Screen init
LiquidCrystal_I2C lcd(0x27, 16,2);


// Turn on the I2C display
void I2C_begin_coms()
{
  lcd.begin();
  lcd.backlight();
  lcd.clear();
}

// Run the I2C dislay with the time
void I2C_disp_local_time()
{
  // Get time
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("No time available (yet)");
    return;
  }

  // Display time
  lcd.clear();
  lcd.print(&timeinfo, "%b %d %Y");
  lcd.setCursor(0,1);
  lcd.print(&timeinfo, "%a, %H:%M:%S");

  delay(1000);
}


// Rest the vars to 0. Is called when the ESP is woken for a non-specified 
// reason (i.e. plugged in) to prevent undefined behaviour
void reset_vars()
{
  hour = -1;
  minute = -1;
  second = 0;
  alarm_min = 0;
  alarm_hour = -1;
  pinMode(displayPin, INPUT);
  pinMode(dispAlarmPin, INPUT);

  // flash on board LED to show it is working
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  delay(500);
  digitalWrite(2, LOW);
}

// Update the time localy acconding to the RTC
void updateLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("No time available (yet)");
    return;
  }
  hour = timeinfo.tm_hour;
  minute = timeinfo.tm_min;
  second = timeinfo.tm_sec;
}

// Callback function that sets the waiting variable to true once synced
void cbSyncTime(struct timeval *tv) {
    isSetNTP = true;
}

// Sync the time according to the UTC once a day
void syncTime()
{
  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  // Sync and update
  struct tm timeinfo;
  sntp_set_time_sync_notification_cb(cbSyncTime);
  configTzTime("MST7MDT,M3.2.0,M11.1.0", ntpServer);
  getLocalTime(&timeinfo, NTP_REQUEST_TIMEOUT); // try 1 secs to sync
  while(!isSetNTP){ getLocalTime(&timeinfo); } // waiting for NTP to be set

  // Disconnect from WiFi
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

// Return the smaller number between the difference of the current time and
// alarm time and 1 hour.
uint64_t time_difference_to_alarm()
{
  if(alarm_hour == -1){ return 3600000000; } // 1 hour in microseconds 

  updateLocalTime();

  int hour_difference = alarm_hour - hour;
  if( alarm_hour < hour ){ hour_difference += 24; }

  int minute_difference = alarm_min - minute;
  if( alarm_min < minute ){ hour_difference--; minute_difference += 60; }

  if( hour_difference == -1 ){ return 3600000000; }

  uint64_t clock_difference = hour_difference * 3600000000 + minute_difference * 60000000 - second * 1000000;
  return min(uint64_t(3600000000), clock_difference);
}



/*State Machine*/ 
RTC_DATA_ATTR uint8_t state;

// The state when a button is pressed
void button_pressed_state()
{
  pinMode(displayPin, INPUT);
  pinMode(dispAlarmPin, INPUT);
  pinMode(setAlarmHourPin, INPUT);
  pinMode(setAlarmMinTensPin, INPUT);
  pinMode(setAlarmMinOnesPin, INPUT);

  if(digitalRead(displayPin)){
    I2C_begin_coms();
    while(digitalRead(displayPin)){
      I2C_disp_local_time();
    }
  }

  else if(digitalRead(dispAlarmPin)){
    I2C_begin_coms();
    while(digitalRead(dispAlarmPin)){
      // 1 hour press
      if(digitalRead(setAlarmHourPin)){
        alarm_hour++;
        if( alarm_hour >= 24 ){ alarm_hour = -1; } // alarm_hour == -1 means alarm is off
        while(digitalRead(setAlarmHourPin)){} // Waiting for the user to unpress the pin.
      }

      // 10 min press
      else if(digitalRead(setAlarmMinTensPin)){
        alarm_min += 10;
        alarm_min = alarm_min % 60;
        while(digitalRead(setAlarmMinTensPin)){} // Waiting for the user to unpress the pin.
      }

      // 1 min press
      else if(digitalRead(setAlarmMinOnesPin)){
        alarm_min++;
        alarm_min = alarm_min % 60;
        while(digitalRead(setAlarmMinOnesPin)){}  // Waiting for the user to unpress the pin.
      }

      // Display alarm time
      lcd.clear();
      lcd.setCursor(0, 1);

      // Goofy Arduino String stuff
      String t = String(alarm_hour);
      t.concat(":");
      if( alarm_min < 10 ) { t.concat(0); }
      t.concat(alarm_min);

      lcd.print(t);
      delay(100);
    }
  }
  // clear the screen
  lcd.clear();
}


// The state when the alarm is on
void alarm_on_state()
{
  // Check for correct time
  updateLocalTime();
  if(alarm_hour != hour || alarm_min != minute) { syncTime(); return; }

  pinMode(motorCWPin, OUTPUT);
  pinMode(motorCCWPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(bigOffButtonPin, INPUT);

  // Spin the motor to turn on the light
  digitalWrite(motorCWPin, HIGH);
  delay(500);
  digitalWrite(motorCWPin, LOW);
  digitalWrite(motorCCWPin, HIGH);
  delay(500);
  digitalWrite(motorCCWPin, LOW);

  // Wait 2 mins to turn on the buzzer
  delay(120000);

  // Turn on the buzzer
  digitalWrite(buzzerPin, HIGH);
  
  // Display pressed time while wating for the off button to be pressed
  I2C_begin_coms();
  while(!digitalRead(bigOffButtonPin)){ I2C_disp_local_time(); }

  // Turn off buzzer with a chirp and clear LCD
  digitalWrite(buzzerPin, LOW);
  delay(3000);
  digitalWrite(buzzerPin, HIGH);
  delay(500);
  digitalWrite(buzzerPin, LOW);

  lcd.clear();

  syncTime();
 
  // Wait for a minute to not double turn on alarm  
  delay(60000); 
}


// Put the ESP to deep sleep with 2 IO pins and a timer as Wakeup. 
void espSleep()
{
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  esp_deep_sleep(time_difference_to_alarm());
}


// Determine the reason the ESP was awaken and returns the correct state
uint8_t wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT1 : return 2;
    case ESP_SLEEP_WAKEUP_TIMER : return 1;
    default : return 0;
  }
}



void setup()
{
  state = wakeup_reason();
  setenv("TZ","MST7MDT,M3.2.0,M11.1.0",1);
  tzset();

  // Set to the correct state
  switch(state)
  {
    case 0: reset_vars(); syncTime(); break;
    case 1: alarm_on_state(); break;
    case 2: button_pressed_state(); break;
  }
  espSleep();
}

// Empty becasue it sleeps before getting here
void loop(){}