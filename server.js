// server.js

require('dotenv').config(); // Load environment variables at the very top

const express = require('express');
const http = require('http');
const socketIo = require('socket.io');
const bodyParser = require('body-parser');
const cors = require('cors');
const twilio = require('twilio');

const app = express();
const server = http.createServer(app);
const io = socketIo(server);

// Twilio configuration using environment variables
const accountSid = process.env.TWILIO_ACCOUNT_SID;
const authToken = process.env.TWILIO_AUTH_TOKEN;
const twilioClient = twilio(accountSid, authToken);
const twilioPhoneNumber = process.env.TWILIO_PHONE_NUMBER;

// Middleware
app.use(cors());
app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());

// Serve static files
app.use(express.static(__dirname));

// Global variables
let currentMode = 'with';
let pendingCommands = [];

// **Hard-coded phone number**
const userPhoneNumber = '+16475284181'; // Replace with your phone number

// Route to handle accelerometer data from Arduino
app.post('/data', (req, res) => {
  const { X, Y, Z, mode } = req.body;
  console.log(`Accelerometer Data Received: X=${X}, Y=${Y}, Z=${Z}, Mode=${mode}`);

  // Broadcast data to the front-end via WebSocket
  io.emit('accelerometerData', { X, Y, Z });

  // Respond back to the Arduino
  res.status(200).send('Data received');
});

// Route to handle alarm status from Arduino
app.post('/alarm', (req, res) => {
  console.log('Alarm Triggered! Movement detected in Away mode.');

  // Broadcast alarm notification to the front-end
  io.emit('alarmTriggered');

  // Send SMS to the hard-coded phone number
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

  // Respond back to the Arduino
  res.status(200).send('Alarm received');
});

// Route to send commands to Arduino
app.get('/command', (req, res) => {
  if (pendingCommands.length > 0) {
    const command = pendingCommands.shift();
    res.send(command);
  } else {
    res.send('');
  }
});

// WebSocket connection with front-end
io.on('connection', (socket) => {
  console.log('A user connected:', socket.id);

  // Listen for mode changes from the front-end
  socket.on('setMode', (mode) => {
    currentMode = mode;
    console.log(`Mode set to: ${currentMode}`);
    if (mode === 'away') {
      pendingCommands.push('startMonitoring');
    } else if (mode === 'with') {
      pendingCommands.push('stopMonitoring');
    }
  });

  // The server ignores any phone number sent by the client
  socket.on('savePhoneNumber', (phoneNumber) => {
    console.log(`Received phone number from client (ignored): ${phoneNumber}`);
  });

  // Listen for "disableAlarm" event from the client
  socket.on('disableAlarm', () => {
    currentMode = 'with';
    console.log('Alarm disabled by the user.');
    pendingCommands.push('stopMonitoring');
  });

  socket.on('disconnect', () => {
    console.log('A user disconnected:', socket.id);
  });
});

// Start the server
const PORT = 3000;
server.listen(PORT, '0.0.0.0', () => {
  console.log(`Server running at http://0.0.0.0:${PORT}`);
});
