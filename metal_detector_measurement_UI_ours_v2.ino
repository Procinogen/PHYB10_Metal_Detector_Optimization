//Global variables
double pulse,frequency,pulseave,frequencyave,tempave,slowave;
// Quick rundown on variables:
// * pulse: The length of the measured pulse from the comparator, in ms
// * frequency: The computed frequency using the pulse length
// * pulseave: The average pulse length, computed over the number of totalSamples
// * frequencyave: The average frequency, computed over the number of totalSamples
// * tempave: Temporary value for the average when computing inside the main loop
// * slowave: A slower exponential moving average to account for thermal drift in the components

const unsigned int totalSamples = 150;
const unsigned int pulseLength = 5; 
const unsigned int pulseTransientMicros = 75;
const unsigned int pulseDampen = 10;
const unsigned int warmupTime = 1000;
// * totalSamples: The total number of samples used to compute the average
// * pulseLength: Time in milliseconds to pulse the coil
// * pulseTransientMicros: Time in microseconds to let the signal stabilize
// * pulseDampen: Time in milliseconds to wait for LCR oscillations to dampen

double pulseroll[totalSamples]; // Rolling average values


const unsigned int timeUntilNextPulse = 5000; // Says exactly on the tin

int n = 0; // Loop index
bool roll = false; // Flag to check whether we can start computing the average.

unsigned long timeSinceStart; // Time in ms since the program was run.

const double metalThreshold = 0.13; // Threshold for metal to be detected
unsigned int hysteresis = 0;
const unsigned int hystThreshold = 10; // # of cycles where a postive is triggered before declaring a metal is detected
const double alpha = 0.01; // Smoothing coefficient for the slow filter 

//Pins
const int COMP_OUTPUT = 11;
const int INPUT_FREQ = 13;
const int INDICATOR_LIGHT = 12;

double pulsePin(int pulsingPin, int measurePin, int length, int transient){
    digitalWrite(pulsingPin, HIGH);
    delay(length);
    digitalWrite(pulsingPin, LOW); //High/Low pulse
    delayMicroseconds(transient);

  return pulseIn(measurePin,HIGH,timeUntilNextPulse); // Return the comparator pulse time
}

void setup() {
  Serial.begin(9600);
  pinMode(COMP_OUTPUT, INPUT); // The OUTPUT of the comparator
  pinMode(INPUT_FREQ, OUTPUT); // The INPUT frequency we are giving the circuit
  pinMode(INDICATOR_LIGHT, OUTPUT); // Indicator light :D

  warmup(warmupTime);
  
  Serial.println("//Initializing...//");
  Serial.println("//Calibrating...//");

  // Code to start pulsing coil
  pulseave = 0;
  for(int i = 1; i < totalSamples+1; i++){ // Pulse for specified # of totalSamples
    pulse = pulsePin(INPUT_FREQ,COMP_OUTPUT,pulseLength,pulseTransientMicros); // Measures time for a pulse
    pulseave = pulseave + pulse; // Computes the sum before averaging
    delay(pulseDampen); // waits for LCR oscillations to dampen
  }

  tempave = 0; //Initialize tempave

  pulseave = pulseave/totalSamples; // Compute average pulse time
  frequencyave = 1.E6/(2*pulseave); // Compute frequency based on the period

  slowave = pulseave;

  // Print out results
  Serial.print("Average Pulse uS:");  // display results
  Serial.print(pulseave);
  Serial.print("\tAverage Frequency Hz: ");
  Serial.print(frequencyave);
  Serial.println("");
  delay(200);
}

void warmup(unsigned int cycles) {

  Serial.println("");
  Serial.print("//Warming up... Warmup set to: ");
  Serial.print(cycles);
  Serial.println(" cycles.//");

  for(int i = 0; i < cycles; i++) {
    if(i == cycles/2){
      Serial.println("//Halfway...//");
    }
    pulsePin(INPUT_FREQ, COMP_OUTPUT, pulseLength, pulseTransientMicros);
    delay(pulseDampen);
  }
  Serial.println("//Warmup complete...//");
}

void loop() {
  timeSinceStart = millis();
  
  // Pulse code
  pulse = pulsePin(INPUT_FREQ,COMP_OUTPUT,pulseLength, pulseTransientMicros);
  frequency = 1.E6/(2*pulse);
  pulseroll[n] = pulse; // Fill up the rolling average array.
  tempave = tempave + pulse;
  n++; // Increase index

  if (n > totalSamples-1) { // Reset index every 200 cycles and allow us to start computing the rolling average
    n=0;
    roll = true;
  }

  if (roll) { // Compute average and detect metal

    // Data collection
    double currentave = tempave/totalSamples;
    slowave = alpha*currentave + (1-alpha)*slowave; // Exponentially-weighted moving average
    double diff = currentave - pulseave;
    double slowdiff = fabs(currentave - slowave);
    //double percentPulseChange = fabs(slowdiff/slowave) * 100;


    Serial.print(timeSinceStart);
    Serial.print(",");
    Serial.print(slowdiff, 6);
    Serial.print(",");
    Serial.print(diff, 6);
    Serial.print(",");

    if (slowdiff >= metalThreshold) {
      // if there is a change in average, metal is found
      hysteresis++;
      if (hysteresis >= hystThreshold) {
        digitalWrite(INDICATOR_LIGHT, HIGH);
        Serial.println(1);
      } else {
        digitalWrite(INDICATOR_LIGHT, LOW);
        Serial.println("0");
      }
    }else{
      Serial.println(0);
      digitalWrite(INDICATOR_LIGHT, LOW);
      hysteresis = 0;
    }

    tempave = tempave - pulseroll[n]; // Removes the oldest result from our rolling average.

  }

  delay(20);

}
