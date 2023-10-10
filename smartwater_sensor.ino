
/*
  Arduino sketch for the smart water bottle project.

  @Author:  Devam Patel
  @Date:    11/25/2022

*/


// general library includes
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <SPI.h>
#include <FS.h>
#include <stdio.h>
#include <string.h>
#include "SPIFFS.h"
#include <ESP32Time.h>

// define directives
#define SDA 13
#define SCL 14
#define FORMAT_SPIFFS_IF_FAILED true
#define PIN_LED 2
#define PIN_BUTTON 32

// the FSR and 10K pulldown are connected to a0
int fsrPin = 36;


// lcd object
LiquidCrystal_I2C lcd(0x27, 16, 2);


/*
  The following code was taken from exteral source to help create a sqlite3 database

  Source: https://github.com/siara-cc/esp32_arduino_sqlite3_lib
*/
const char* data = "";
static int callback(void *data, int argc, char **argv, char **azColName) {
  int i;
  Serial.printf("\n");
  Serial.printf("========== ROW ==========\n");
  for (i = 0; i < argc; i++) {
    Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  Serial.printf("\n");
  return 0;
}

// function to open sqlite3 database by given filename
int db_open(const char *filename, sqlite3 **db) {
  int rc = sqlite3_open(filename, db);
  if (rc) {
    Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
    return rc;
  } else {
    Serial.printf("Opened database successfully\n");
  }
  return rc;
}

// function to execute sql queries on the sqlite3 database
char *zErrMsg = 0;
int db_exec(sqlite3 *db, const char *sql) {
  Serial.println(sql);
  long start = micros();
  int rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
  if (rc != SQLITE_OK) {
    Serial.printf("SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  } else {
    Serial.printf("Operation done successfully\n");
  }
  Serial.print(F("Time taken:"));
  Serial.println(micros() - start);
  return rc;
}


/*
  setup() and loop() functions

*/

sqlite3* db; // sqlite3 db ptr
int rc;

void setup(void) {
  Serial.begin(115200);
  Wire.begin(SDA, SCL);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Pressure: ");

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);


  // initialize SPIFFS (SPI Flash File Storage)
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("Failed to mount file system");
    return;
  }

  // list SPIFFS contents
  File root = SPIFFS.open("/");
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  // loop through SPIFFS file system and delete old databases

  //SPIFFS.remove("/smartwater.db");
  //SPIFFS.remove("/smartwater.db-journal");


  // open smartwater.db
  sqlite3_initialize();
  if (db_open("/spiffs/smartwater.db", &db)) {
    return;
  }

  // Create new table called 'tblLog' with columns [LogID] [Pressure] [User]
  rc = db_exec(db, "DROP TABLE IF EXISTS tblLog;");
  if (rc != SQLITE_OK) {
    Serial.println("ERROR: DROP TABLE query failed.");
    sqlite3_close(db);
    return;
  }

  rc = db_exec(db, "CREATE TABLE tblLog (LogID INTEGER, Pressure INTEGER, User TEXT);");
  if (rc != SQLITE_OK) {
    Serial.println("ERROR: CREATE TABLE query failed.");
    sqlite3_close(db);
    return;
  }

}


void loop(void) {

  int pressure = analogRead(fsrPin); // get current pressure on the FSR
  String pressure_str = String(pressure); // convert pressure value to string

  if (digitalRead(PIN_BUTTON) == LOW) {
    digitalWrite(PIN_LED, HIGH);
    storePressure(pressure_str);
  } else {
    digitalWrite(PIN_LED, LOW);
    // move cursor down and print pressure
    lcd.setCursor(0, 1);
    lcd.print(pressure);
  }

  delay(1000);
  afterRead();
}

// to fix the bug where only the first digit of the pressure value was set to 0
void afterRead() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pressure: ");
}

int idCounter = 1;
void storePressure(String pressureString) {
  String sql = "INSERT INTO tblLog VALUES (" + String(idCounter) + "," + pressureString + ", 'Devam');";

  rc = db_exec(db, sql.c_str()); // execute sql query
  if (rc != SQLITE_OK) {
    Serial.println("ERROR: INSERT query failed.");
    sqlite3_close(db);
    return;
  }

  rc = db_exec(db, "SELECT * FROM tblLog;");
  if (rc != SQLITE_OK) {
    Serial.println("ERROR: SELECT query failed.");
    sqlite3_close(db);
    return;
  }

  idCounter++; 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Data stored");
  lcd.setCursor(0, 1);
  lcd.print("successfully!");
  delay(1000);
}
