
const socket = io();

socket.on('data', function (data) {
  console.log('Data received:', data);
  document.getElementById('current-people').textContent = data.CurrentPeopleIn || 'No data';
  document.getElementById('people-in').textContent = "IN:" + data.EnteringNumber || 'No data';
  document.getElementById('people-out').textContent = "Out:" + data.OutingNumber || 'No data';
  document.getElementById('Secound-floor').textContent = "Out:" + data.secoundFloor || 'No data';
});
// Get current date and time information from JavaScript
// Function to update the current time
function updateTime() {
  var now = new Date();
  var currentDate = now.toLocaleDateString();
  var currentTime = now.toLocaleTimeString();
  document.getElementById("current-date").textContent = currentDate;
  document.getElementById("current-time").textContent = currentTime;
}

// Update the time immediately and every second afterward
updateTime();
setInterval(updateTime, 1000);

