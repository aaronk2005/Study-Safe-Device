#include <Wire.h>
#include <WiFiNINA.h>

// Wi-Fi credentials
const char* ssid = "Aaron’s iPhone";       // Replace with your Wi-Fi SSID
const char* password = "xyzxyz"; // Replace with your Wi-Fi password

// Node.js server details
const char* server = "172.20.10.5"; // Replace with your server's IP address
const int port = 3000; // Ensure this matches your server port

const uint8_t MPU = 0x68;

int16_t AcX, AcY, AcZ;
int16_t prevAcX = 0, prevAcY = 0, prevAcZ = 0; // Variables to store previous readings
int16_t deltaX = 0, deltaY = 0, deltaZ = 0;    // Global variables to store deltas
bool alarmActive = false;
bool awayMode = false; // Default to "With" mode
int buzzerPin = 8;

void setup() {
    Wire.begin();
    Serial.begin(9600);
    pinMode(buzzerPin, OUTPUT);
    noTone(buzzerPin);

    // Initialize MPU
    Wire.beginTransmission(MPU);
    Wire.write(0x6B);
    Wire.write(0);
    Wire.endTransmission(true);

    // Connect to Wi-Fi
    connectToWiFi();
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        readAccelerometer();

        // Check for commands from the server
        checkForCommands();

        if (awayMode) {
            if (detectMovement()) {
                if (!alarmActive) {
                    alarmActive = true;
                    sendAlarmToServer();
                }
                soundAlarm();
            } else {
                alarmActive = false;
                noTone(buzzerPin);
            }
        } else {
            alarmActive = false;
            noTone(buzzerPin);
        }
        sendDataToServer();
    } else {
        Serial.println("Wi-Fi Disconnected! Reconnecting...");
        connectToWiFi();
    }
    delay(500);
}

void connectToWiFi() {
    Serial.print("Connecting to Wi-Fi...");
    WiFi.begin(ssid, password);
    unsigned long startAttemptTime = millis();
    const unsigned long timeout = 10000; // 10 seconds

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
        delay(1000);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWi-Fi connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.print("\nFailed to connect to Wi-Fi. Status code: ");
        Serial.println(WiFi.status());
    }
}

void readAccelerometer() {
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);
    Wire.endTransmission(false);

    if (Wire.requestFrom(MPU, (size_t)14, true) == 14) {
        // Read current accelerometer values
        int16_t currentAcX = Wire.read() << 8 | Wire.read();
        int16_t currentAcY = Wire.read() << 8 | Wire.read();
        int16_t currentAcZ = Wire.read() << 8 | Wire.read();

        // Calculate the change in values using previous readings
        deltaX = currentAcX - prevAcX;
        deltaY = currentAcY - prevAcY;
        deltaZ = currentAcZ - prevAcZ;

        // Update previous readings for next iteration
        prevAcX = currentAcX;
        prevAcY = currentAcY;
        prevAcZ = currentAcZ;

        // Update current readings
        AcX = currentAcX;
        AcY = currentAcY;
        AcZ = currentAcZ;

        Serial.print("X: "); Serial.print(AcX);
        Serial.print(" Y: "); Serial.print(AcY);
        Serial.print(" Z: "); Serial.println(AcZ);
        Serial.print("ΔX: "); Serial.print(deltaX);
        Serial.print(" ΔY: "); Serial.print(deltaY);
        Serial.print(" ΔZ: "); Serial.println(deltaZ);
    } else {
        Serial.println("Failed to read accelerometer data.");
    }
}

bool detectMovement() {
    const int movementThreshold = 20000; // Trigger alarm if movement exceeds 20000
    return abs(deltaX) > movementThreshold || abs(deltaY) > movementThreshold || abs(deltaZ) > movementThreshold;
}

void sendDataToServer() {
    WiFiClient dataClient;
    if (dataClient.connect(server, port)) {
        String data = "X=" + String(AcX) + "&Y=" + String(AcY) + "&Z=" + String(AcZ) + "&mode=" + (awayMode ? "away" : "with");
        dataClient.println("POST /data HTTP/1.1");
        dataClient.println("Host: " + String(server));
        dataClient.println("Content-Type: application/x-www-form-urlencoded");
        dataClient.print("Content-Length: ");
        dataClient.println(data.length());
        dataClient.println();
        dataClient.println(data);
        Serial.println("Data sent to server.");
    } else {
        Serial.println("Failed to connect to server.");
    }
    dataClient.stop();
}

void sendAlarmToServer() {
    WiFiClient alarmClient;
    if (alarmClient.connect(server, port)) {
        String alarmData = "alarm=true";
        alarmClient.println("POST /alarm HTTP/1.1");
        alarmClient.println("Host: " + String(server));
        alarmClient.println("Content-Type: application/x-www-form-urlencoded");
        alarmClient.print("Content-Length: ");
        alarmClient.println(alarmData.length());
        alarmClient.println();
        alarmClient.println(alarmData);
        Serial.println("Alarm status sent to server.");
    } else {
        Serial.println("Failed to connect to server.");
    }
    alarmClient.stop();
}

void soundAlarm() {
    // Siren effect
    for (int freq = 1000; freq <= 2000; freq += 50) {
        tone(buzzerPin, freq);
        delay(20);
    }
    for (int freq = 2000; freq >= 1000; freq -= 50) {
        tone(buzzerPin, freq);
        delay(20);
    }
}

void checkForCommands() {
    WiFiClient commandClient;
    if (commandClient.connect(server, port)) {
        commandClient.println("GET /command HTTP/1.1");
        commandClient.println("Host: " + String(server));
        commandClient.println("Connection: close");
        commandClient.println();

        while (commandClient.connected()) {
            String line = commandClient.readStringUntil('\n');
            if (line == "\r") {
                // Headers ended, start reading body
                String command = commandClient.readStringUntil('\n');
                command.trim();
                if (command.length() > 0) {
                    Serial.println("Received command: " + command);
                    processCommand(command);
                }
                break;
            }
        }
    } else {
        Serial.println("Failed to connect to server for commands.");
    }
    commandClient.stop();
}

void processCommand(String command) {
    command.trim(); // Remove any whitespace
    if (command == "makePing") {
        // No action needed
    } else if (command == "startMonitoring") {
        awayMode = true;
        Serial.println("Set to Away Mode");
    } else if (command == "stopMonitoring") {
        awayMode = false;
        alarmActive = false;
        noTone(buzzerPin);
        Serial.println("Set to With Device Mode");
    }
}
