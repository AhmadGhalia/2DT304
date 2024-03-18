import express from 'express';
import { createServer } from 'http';
import { Server } from 'socket.io';
import bodyParser from 'body-parser';
import mongoose from 'mongoose'
import dotenv from 'dotenv'
dotenv.config()
let enter = 0
let out = 0
let CurrentPeopleIn = 0
let secoundFloor = 0
const app = express();
const port = process.env.PORT;
const httpServer = createServer(app);
const io = new Server(httpServer);
app.set('view engine', 'ejs')
// Middleware and other setup...
app.use(bodyParser.json()); // for parsing application/json
app.use('/css', express.static('css'))
app.use('/javascript', express.static('js'))

let lastReceivedData = {}; // To store the latest data
io.on('connection', (socket) => {
  console.log('a user connected');
  // Immediately send the latest data upon a new connection
  socket.emit('data', lastReceivedData);
});

app.post('/data', (req, res) => {
  const data = req.body;
  console.log('Received data:', data);
  enter = enter + data.EnteringNumber
  out = out + data.OutingNumber
  CurrentPeopleIn = enter - out
  console.log(data.EnteringNumber + " " + data.OutingNumber)
  // Create a new DataPoint instance
  const newDataPoint = new DataPoint({
    time: new Date().toISOString().substring(11, 16), // "HH:MM:SS", // Assuming you want to save the current time
    value: CurrentPeopleIn,
    EnteringNumber: enter,
    OutingNumber: out,
    SecoundFloorData: secoundFloor
  });

  // Save the newDataPoint to MongoDB
  newDataPoint.save()
    .then(() => {
      console.log('Data point saved successfully.');
      // Emit the data to all clients
      io.emit('data', data);
      res.status(200).send('Data received and saved');
    })
    .catch(err => {
      console.error('Error saving data point:', err);
      res.status(500).send('Error saving data');
    });
});
app.get('/data', (req, res) => {
  res.render('dataView')
})
app.get('/api/data', async (req, res) => {
  try {
    const dataPoints = await DataPoint.find({}).sort({ time: +1 }).limit(100); // Adjust limit as needed
    res.json(dataPoints);
    // console.log(dataPoints)
  } catch (err) {
    console.error('Failed to fetch data points:', err);
    res.status(500).send('Failed to fetch data');
  }
});

// Define the schema
const DataPointSchema = new mongoose.Schema({
  time: String,
  value: Number,
  EnteringNumber: Number,
  OutingNumber: Number,
  SecoundFloorData: Number
});

// Create the model
const DataPoint = mongoose.model('DataPoint', DataPointSchema);
const connectionString = process.env.CONNECTIONSTRING
mongoose.connect(connectionString, { useNewUrlParser: true, useUnifiedTopology: true })
  .then(() => console.log('MongoDB connected'))
  .catch(err => console.error('MongoDB connection error:', err));

// Start the server
export default () => {
  httpServer.listen(port, () => {
    console.log(`Server running on port ${port}`);
  })
}
