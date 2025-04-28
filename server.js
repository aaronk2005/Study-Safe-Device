// Load environment variables (Twilio credentials) from .env file
require('dotenv').config();

const express = require('express');
const http = require('http');
const socketIo = require('socket.io');
const bodyParser = require('body-parser');
const cors = require('cors');
const twilio = require('twilio');

const app = express();
const server = http.createServer(app);
const io = socketIo(server);

// Twilio configuration
const accountSid = process.env.TWILIO_ACCOUNT_SID;
const authToken = process.env.TWILIO_AUTH_TOKEN;
const twilioClient = twilio(accountSid, authToken);
const twilioPhoneNumber = process.env.TWILIO_PHONE_NUMBER;

// Middleware setup
app.use(cors());
app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());
app.use(express.static(__dirname)); // Serve static frontend files

// Global variables
let currentMode = 'with';           // Device mode (away or with)
let pendingCommands = [];           // Queue for commands to send to Arduino

// Hard-coded phone number to receive SMS alerts
const userPhoneNumber = '+16475284181'; // Replace with your phone number

// Route to receive accelerometer data from Arduino
app.post('/data', (req, res) => {
  const { X, Y, Z, mode } = req.body;
  console.log(`Accelerometer Data Received: X=${X}, Y=${Y}, Z=${Z}, Mode=${mode}`);

  io.emit('accelerometerData', { X, Y, Z }); // Forward data to the frontend
  res.status(200).send('Data received');      // Respond to Arduino
});

// Route to receive alarm triggers from Arduino
app.post('/alarm', (req, res) => {
  console.log('Alarm Triggered! Movement detected in Away mode.');

  io.emit('alarmTriggered'); // Notify frontend that alarm is triggered

  // Send SMS alert via Twilio
  if (userPhoneNumber) {
    twilioClient.messages
      .create({
        body: '🚨 Your Study Safe device is on the move.',
        from: twilioPhoneNumber,
        to: userPhoneNumber,
      })
      .then(message => {
        console.log(`SMS sent to ${userPhoneNumber}: ${message.sid}`);
      })
      .catch(error => {
        console.error('Error sending SMS:', error);
      });
  } else {
    console.log('No phone number stored. SMS not sent.');
  }

  res.status(200).send('Alarm received');
});

// Route for Arduino to pull pending commands (polling)
app.get('/command', (req, res) => {
  if (pendingCommands.length > 0) {
    const command = pendingCommands.shift(); // Get next command
    res.send(command);
  } else {
    res.send(''); // No command available
  }
});

// WebSocket connection handling
io.on('connection', (socket) => {
  console.log('A user connected:', socket.id);

  // Handle mode change from frontend
  socket.on('setMode', (mode) => {
    currentMode = mode;
    console.log(`Mode set to: ${currentMode}`);
    if (mode === 'away') {
      pendingCommands.push('startMonitoring');
    } else if (mode === 'with') {
      pendingCommands.push('stopMonitoring');
    }
  });

  // Log phone numbers received (but ignore them)
  socket.on('savePhoneNumber', (phoneNumber) => {
    console.log(`Received phone number from client (ignored): ${phoneNumber}`);
  });

  // Handle disable alarm request from frontend
  socket.on('disableAlarm', () => {
    currentMode = 'with';
    console.log('Alarm disabled by the user.');
    pendingCommands.push('stopMonitoring');
  });

  // Handle user disconnect
  socket.on('disconnect', () => {
    console.log('A user disconnected:', socket.id);
  });
});

// Start the server
const PORT = 3000;
server.listen(PORT, '0.0.0.0', () => {
  console.log(`Server running at http://0.0.0.0:${PORT}`);
});
