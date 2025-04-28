// Initialize socket connection to the server
const socket = io(); // Automatically connects to the server that served the page

// Save user's phone number
function savePhoneNumber() {
  const phoneInput = document.getElementById('phoneNumber');
  const userPhoneNumber = phoneInput.value.trim();

  // Validate Canadian phone numbers (E.164 format)
  const phoneRegex = /^\+1\d{10}$/;

  if (phoneRegex.test(userPhoneNumber)) {
    socket.emit('savePhoneNumber', userPhoneNumber); // Send phone number to server
    alert('Phone number saved!');
    document.getElementById('connectButton').disabled = false; // Enable connect button
    phoneInput.disabled = true;
    document.getElementById('saveNumberButton').disabled = true;
  } else {
    alert('Please enter a valid Canadian phone number in E.164 format (e.g., +16471234567).');
  }
}

// Navigate to connecting page
function connectToDevice() {
  window.location.href = 'connecting.html';
}

// Set device mode (away or with device)
function setMode(mode) {
  console.log(`Attempting to set mode to: ${mode}`);
  const awayButton = document.getElementById('awayButton');
  const withButton = document.getElementById('withButton');
  const warningDiv = document.getElementById('warningMessage');
  const disableAlarmButton = document.getElementById('disableAlarmButton');

  if (mode === 'away') {
    awayButton.classList.add('active');
    withButton.classList.remove('active');
    socket.emit('setMode', 'away');
    console.log('Mode set to Away');
  } else if (mode === 'with') {
    withButton.classList.add('active');
    awayButton.classList.remove('active');
    socket.emit('setMode', 'with');

    // Clear warning and hide disable alarm button
    if (warningDiv) {
      warningDiv.innerText = '';
      warningDiv.style.display = 'none';
    }
    if (disableAlarmButton) {
      disableAlarmButton.style.display = 'none';
    }
    console.log('Mode set to With');
  }
}

// Receive and display real-time accelerometer data
socket.on('accelerometerData', (data) => {
  const { X, Y, Z } = data;
  const dataDiv = document.getElementById('data');
  if (dataDiv) {
    dataDiv.innerHTML = `
      <p>X: ${X}</p>
      <p>Y: ${Y}</p>
      <p>Z: ${Z}</p>
    `;
  }
});

// Handle alarm trigger notification
socket.on('alarmTriggered', () => {
  const warningDiv = document.getElementById('warningMessage');
  const disableAlarmButton = document.getElementById('disableAlarmButton');

  if (warningDiv) {
    warningDiv.innerText = '🚨 Your device is on the move!';
    warningDiv.style.display = 'block';
  }
  if (disableAlarmButton) {
    disableAlarmButton.style.display = 'block';
  }
});

// Disable the active alarm
function disableAlarm() {
  setMode('with');
  socket.emit('disableAlarm');
  alert('Alarm disabled. Device set to "With Device Mode".');
}

// Disconnect device and return to home page
function disconnectDevice() {
  window.location.href = 'index.html';
}

// Navigate to instructions page
function navigateToInstructions() {
  window.location.href = 'instructions.html';
}
