
const socket = io(); // Automatically connects to the same server

function savePhoneNumber() {
  const phoneInput = document.getElementById('phoneNumber');
  const userPhoneNumber = phoneInput.value.trim();

  // Validation for Canadian phone numbers in E.164 format
  const phoneRegex = /^\+1\d{10}$/;

  if (phoneRegex.test(userPhoneNumber)) {
    // Send the phone number to the server (server will ignore it)
    socket.emit('savePhoneNumber', userPhoneNumber);
    alert('Phone number saved!');
    // Enable the connect button
    document.getElementById('connectButton').disabled = false;
    // Disable the save number button and input
    phoneInput.disabled = true;
    document.getElementById('saveNumberButton').disabled = true;
  } else {
    alert('Please enter a valid Canadian phone number in E.164 format (e.g., +16471234567).');
  }
}

function connectToDevice() {
  window.location.href = 'connecting.html';
}

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
    // Clear warning message and hide disable alarm button
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

// Real-time accelerometer data updates
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

// Real-time alarm notifications
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

// Disable alarm function
function disableAlarm() {
  setMode('with');
  socket.emit('disableAlarm');
  alert('Alarm disabled. Device set to "With Device Mode".');
}

function disconnectDevice() {
  window.location.href = 'index.html';
}

function navigateToInstructions() {
  window.location.href = 'instructions.html';
}
