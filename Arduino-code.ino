#include <Wire.h>              // Include the Wire library to communicate with I2C devices (accelerometer)
#include <WiFiNINA.h>           // Include the Wi-Fi library for connecting to the internet via Wi-Fi

// Wi-Fi credentials (Replace with your own SSID and password)
const char* ssid = "Aaron’s iPhone";   // SSID of your Wi-Fi network
const char* password = "xyzxyz";       // Password for your Wi-Fi network

// Node.js server details (Replace with your server's IP address and port)
const char* server = "172.20.10.5";    // IP address of your Node.js server
const int port = 3000;                 // Port that the server listens to (should match the server code)

// MPU-6050 (Accelerometer) I2C address
const uint8_t MPU = 0x68;              // Default I2C address for the MPU-6050 accelerometer

// Variables to store accelerometer readings
int16_t AcX, AcY, AcZ;                 // Current accelerometer readings for X, Y, Z axes
int16_t prevAcX = 0, prevAcY = 0, prevAcZ = 0; // Variables to store previous readings
int16_t deltaX = 0, deltaY = 0, deltaZ = 0;    // Variables to store the change in accelerometer values (movement detection)
bool alarmActive = false;              // Flag to track if the alarm is active
bool awayMode = false;                 // Flag to track if the system is in 'away' mode (monitoring for movement)
int buzzerPin = 8;                     // Pin connected to the buzzer (alarm)

// Setup function, runs once when the Arduino is powered on or reset
void setup() {
    Wire.begin();                     // Initialize I2C communication
    Serial.begin(9600);                // Start serial communication for debugging
    pinMode(buzzerPin, OUTPUT);        // Set the buzzer pin as output
    noTone(buzzerPin);                 // Ensure the buzzer is off initially

    // Initialize the MPU-6050 accelerometer
    Wire.beginTransmission(MPU);       
    Wire.write(0x6B);                  // Access the power management register
    Wire.write(0);                     // Wake up the MPU by writing 0 to the power register
    Wire.endTransmission(true);        // End the transmission

    // Connect to Wi-Fi
    connectToWiFi();
}

// Loop function, continuously runs as long as the Arduino is powered on
void loop() {
    if (WiFi.status() == WL_CONNECTED) {   // Check if the Wi-Fi is connected
        readAccelerometer();               // Read accelerometer data

        // Check for commands from the server
        checkForCommands();

        // If in away mode, detect movement and trigger the alarm if needed
        if (awayMode) {
            if (detectMovement()) {          // If movement is detected
                if (!alarmActive) {          // If the alarm isn't already active
                    alarmActive = true;      // Activate the alarm
                    sendAlarmToServer();     // Send the alarm notification to the server
                }
                soundAlarm();               // Sound the alarm buzzer
            } else {
                alarmActive = false;        // Deactivate the alarm if no movement is detected
                noTone(buzzerPin);          // Stop the buzzer
            }
        } else {
            alarmActive = false;            // If not in away mode, make sure the alarm is off
            noTone(buzzerPin);              // Stop the buzzer
        }

        // Send accelerometer data to the server
        sendDataToServer();
    } else {
        Serial.println("Wi-Fi Disconnected! Reconnecting...");  // Print a message if Wi-Fi is disconnected
        connectToWiFi();  // Try to reconnect to Wi-Fi
    }

    delay(500);  // Wait for 500ms before the next loop
}

// Function to connect to Wi-Fi
void connectToWiFi() {
    Serial.print("Connecting to Wi-Fi...");
    WiFi.begin(ssid, password);              // Start the connection to Wi-Fi
    unsigned long startAttemptTime = millis();  // Record the time when the connection attempt starts
    const unsigned long timeout = 10000;      // Set a timeout of 10 seconds for Wi-Fi connection

    // Attempt to connect to Wi-Fi until it's connected or the timeout is reached
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
        delay(1000);  // Wait for 1 second before trying again
        Serial.print(".");  // Print a dot to show progress
    }

    // Check if the connection was successful
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWi-Fi connected!");              // Print success message
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());                      // Print the IP address assigned to the device
    } else {
        Serial.print("\nFailed to connect to Wi-Fi. Status code: ");
        Serial.println(WiFi.status());                      // Print error code if Wi-Fi connection fails
    }
}

// Function to read data from the accelerometer (MPU-6050)
void readAccelerometer() {
    Wire.beginTransmission(MPU);  // Start communication with the accelerometer
    Wire.write(0x3B);             // Request accelerometer data starting at register 0x3B
    Wire.endTransmission(false);

    // If 14 bytes of data are received from the accelerometer, read them
    if (Wire.requestFrom(MPU, (size_t)14, true) == 14) {
        // Read the accelerometer data for X, Y, and Z axes
        int16_t currentAcX = Wire.read() << 8 | Wire.read();
        int16_t currentAcY = Wire.read() << 8 | Wire.read();
        int16_t currentAcZ = Wire.read() << 8 | Wire.read();

        // Calculate the change in accelerometer values (delta)
        deltaX = currentAcX - prevAcX;
        deltaY = currentAcY - prevAcY;
        deltaZ = currentAcZ - prevAcZ;

        // Update previous readings for the next iteration
        prevAcX = currentAcX;
        prevAcY = currentAcY;
        prevAcZ = currentAcZ;

        // Update current readings
        AcX = currentAcX;
        AcY = currentAcY;
        AcZ = currentAcZ;

        // Print the accelerometer data for debugging
        Serial.print("X: "); Serial.print(AcX);
        Serial.print(" Y: "); Serial.print(AcY);
        Serial.print(" Z: "); Serial.println(AcZ);
        Serial.print("ΔX: "); Serial.print(deltaX);
        Serial.print(" ΔY: "); Serial.print(deltaY);
        Serial.print(" ΔZ: "); Serial.println(deltaZ);
    } else {
        Serial.println("Failed to read accelerometer data.");  // Print error if accelerometer data is not received
    }
}

// Function to detect movement based on accelerometer data
bool detectMovement() {
    const int movementThreshold = 20000;  // Define the threshold for triggering the alarm
    // Return true if movement exceeds the threshold in any axis (X, Y, or Z)
    return abs(deltaX) > movementThreshold || abs(deltaY) > movementThreshold || abs(deltaZ) > movementThreshold;
}

// Function to send accelerometer data to the server
void sendDataToServer() {
    WiFiClient dataClient;  // Create a Wi-Fi client object
    if (dataClient.connect(server, port)) {  // Connect to the server
        String data = "X=" + String(AcX) + "&Y=" + String(AcY) + "&Z=" + String(AcZ) + "&mode=" + (awayMode ? "away" : "with");
        dataClient.println("POST /data HTTP/1.1");  // Send POST request with accelerometer data
        dataClient.println("Host: " + String(server));
        dataClient.println("Content-Type: application/x-www-form-urlencoded");
        dataClient.print("Content-Length: ");
        dataClient.println(data.length());
        dataClient.println();
        dataClient.println(data);  // Send the actual data to the server
        Serial.println("Data sent to server.");
    } else {
        Serial.println("Failed to connect to server.");  // Print error if unable to connect to the server
    }
    dataClient.stop();  // Close the connection
}

// Function to send alarm status to the server
void sendAlarmToServer() {
    WiFiClient alarmClient;  // Create a Wi-Fi client object for the alarm
    if (alarmClient.connect(server, port)) {  // Connect to the server
        String alarmData = "alarm=true";  // Data to send indicating the alarm is triggered
        alarmClient.println("POST /alarm HTTP/1.1");  // Send POST request for alarm
        alarmClient.println("Host: " + String(server));
        alarmClient.println("Content-Type: application/x-www-form-urlencoded");
        alarmClient.print("Content-Length: ");
        alarmClient.println(alarmData.length());
        alarmClient.println();
        alarmClient.println(alarmData);  // Send the alarm data to the server
        Serial.println("Alarm status sent to server.");
    } else {
        Serial.println("Failed to connect to server.");  // Print error if unable to connect to the server
    }
    alarmClient.stop();  // Close the connection
}

// Function to sound the alarm (siren effect)
void soundAlarm() {
    // Loop through frequencies to create a siren-like sound
    for (int freq = 1000; freq <= 2000; freq += 50) {
        tone(buzzerPin, freq);  // Play the tone on the buzzer
        delay(20);  // Wait for 20 milliseconds
    }
    for (int freq = 2000; freq >= 1000; freq -= 50) {
        tone(buzzerPin, freq);  // Play the tone on the buzzer
        delay(20);  // Wait for 20 milliseconds
    }
}

// Function to check for commands from the server (e.g., "startMonitoring", "stopMonitoring")
void checkForCommands() {
    WiFiClient commandClient;  // Create a Wi-Fi client object for commands
    if (commandClient.connect(server, port)) {  // Connect to the server
        commandClient.println("GET /command HTTP/1.1");  // Send GET request for commands
        commandClient.println("Host: " + String(server));
        commandClient.println("Connection: close");
        commandClient.println();

        while (commandClient.connected()) {
            String line = commandClient.readStringUntil('\n');
            if (line == "\r") {  // If headers have ended, read the body (command)
                String command = commandClient.readStringUntil('\n');
                command.trim();  // Remove extra whitespace
                if (command.length() > 0) {
                    Serial.println("Received command: " + command);
                    processCommand(command);  // Process the received command
                }
                break;
            }
        }
    } else {
        Serial.println("Failed to connect to server for commands.");  // Print error if unable to connect
    }
    commandClient.stop();  // Close the connection
}

// Function to process the received commands
void processCommand(String command) {
    command.trim();  // Remove extra whitespace
    if (command == "makePing") {
        // No action needed for "makePing" command
    } else if (command == "startMonitoring") {
        awayMode = true;  // Set system to away mode (monitoring movement)
        Serial.println("Set to Away Mode");
    } else if (command == "stopMonitoring") {
        awayMode = false;  // Set system to "with device" mode (no monitoring)
        alarmActive = false;  // Deactivate alarm
        noTone(buzzerPin);  // Stop the buzzer
        Serial.println("Set to With Device Mode");
    }
}
