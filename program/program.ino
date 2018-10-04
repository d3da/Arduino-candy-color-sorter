/*  
 *   Arduino m&m's Sorter
 *   by d3da
 *   As part of our PWS Design Paper
 *   2016
 */

/* SETTINGS */

//Speed
int UpperStepperStepDelayus = 5000; //in microseconds
int LowerStepperStepDelayus = 3200; //same

int candyDelay = 380; //milliseconds

int ledOnDelay = 10; //milliseconds to wait for scanning 

//Pin Configuration
const int LowerStepperPins[] = {
 11, //IN1
 10, //IN2
  9, //IN3
  8  //IN4
};
const int UpperStepperPins[] = {
  7, //IN1
  6, //IN2
  5, //IN3
  4  //IN4
};
const int s0  =  0;  //Scale pin 0  
const int s1  =  1;  //Scale pin 1  
const int s2  = 13;  //Filter pin 2 
const int s3  = 12;  //Filter pin 3 
const int out =  3;  //Camera output
const int led =  2;  //LED pin    

//M&M Containter Configuration
const int brown  = 0;
const int red    = 1;
const int orange = 2;
const int yellow = 3;
const int green  = 4;
const int blue   = 5;

const String colorNames[] = {
  "brown",
  "red",
  "orange",
  "yellow",
  "green",
  "blue"
};

//M&M Container Placement (in steps from origin)
const int containerPlacement[] = {
 -100, //brown
  -60,  //red
  -20,  //orange
   20,  //yellow
   60,  //green
  100   //blue
};

//COLOR SCAN SETTINGS
int whiteLightIntensity = 2650; //agv of rgb values when scanning white side of disc
//int scanAmount = 4;

//M&M Color Comparison data
float comparisonRatios[6][3] = {
  {0.813, 1.146, 1.022}, //b
  {0.622, 1.258, 1.120}, //r
  {0.716, 1.180, 1.105}, //o
  {0.770, 1.022, 1.208}, //y
  {1.028, 1.020, 0.952}, //g
  {1.359, 1.040, 0.602}  //b
};
float averageValues[6] = {
  6258,
  4971,
  4145,
  2887,
  2917,
  2645
};

float averageDifferenceWeight = 0.00004; //how much the difference in average light intensity matters when deciding the m&m color


//Serial Communication Settings
const bool SerialEnabled = true;
const int baude = 9600;

//Stepper Motor Phase Settings
const int fullStepCount = 4;
const int fullSteps[][4] = {
  {HIGH, HIGH,  LOW,  LOW},
  { LOW, HIGH, HIGH,  LOW},
  { LOW,  LOW, HIGH, HIGH},
  {HIGH,  LOW,  LOW, HIGH}
};

//Initialize Phase, Steps and Sensor Data
int UpperStepperCurrentPhase;
int LowerStepperCurrentPhase;
int LowerStepperCurrentPosition = containerPlacement[0];
unsigned int sensorData[3];
int parsedData[3];

//Steps in one full rotation
const int UpperStepperStepsInFullRotation = int(700);

unsigned long time;

/* MAIN PROGRAM */
void setup()
{
  time = millis();
  if(SerialEnabled)
  {
    Serial.begin(baude);
  }
  
  for(int pin = 0; pin < 4; pin++)
  {
    pinMode(UpperStepperPins[pin], OUTPUT);
    digitalWrite(UpperStepperPins[pin], LOW);
    
    pinMode(LowerStepperPins[pin], OUTPUT);
    digitalWrite(LowerStepperPins[pin], LOW);
  }
  pinMode( s0, OUTPUT);
  pinMode( s1, OUTPUT);
  pinMode( s2, OUTPUT);
  pinMode( s3, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(out,  INPUT);

  digitalWrite(s0,  HIGH);
  digitalWrite(s1,  HIGH);
  /*
    ------------------
    | S0 | S1 | Out% |
    |----------------|
    | LO | LO |   0% |
    | LO | HI |   2% |
    | HI | LO |  20% |
    | HI | HI | 100% |
    ------------------
  */
  
  digitalWrite(s2,  LOW);
  digitalWrite(s3,  LOW);
}

void loop()
{
  
  int timenow = millis();
  if(SerialEnabled)
  {
    Serial.println();
  }
  time = timenow;
  /*
    0.  the upper disc rotates 120degrees
    1.  color of object is scanned
    2.  color group is calculated
    3.  lower disc is rotated to corresponding container
    4.  receive any Serial commands
  */

  //0. Rotate upper disc
  UpperStepperCurrentPhase = rotate
  (
    UpperStepperPins,
    fullSteps,
    fullStepCount,
    UpperStepperCurrentPhase,
    int(UpperStepperStepsInFullRotation / 3),
    false,
    UpperStepperStepDelayus
  );
  
  for(int pin = 0; pin < 4; pin++)
  {
    digitalWrite(UpperStepperPins[pin], LOW);
  }
  
  //1. Scan color
  digitalWrite(led, HIGH);
  delay(ledOnDelay);
  for(int rgb = 0; rgb < 3; rgb++)
    {
      sensorData[rgb] = scanColor(rgb);
    }
  
  bool turnIncomplete = isTurnIncomplete(sensorData);

  while(turnIncomplete)
  {
    UpperStepperCurrentPhase = rotate
    (
      UpperStepperPins,
      fullSteps,
      fullStepCount,
      UpperStepperCurrentPhase,
      int(UpperStepperStepsInFullRotation / 271),
      false,
      UpperStepperStepDelayus
    );
    
    for(int rgb = 0; rgb < 3; rgb++)
    {
      sensorData[rgb] = scanColor(rgb);
    }
    
    turnIncomplete = isTurnIncomplete(sensorData);
    
  }
  

  delay(100);

 UpperStepperCurrentPhase = rotate
  (
    UpperStepperPins,
    fullSteps,
    fullStepCount,
    UpperStepperCurrentPhase,
    int(UpperStepperStepsInFullRotation / 50),
    false,
    UpperStepperStepDelayus
  );
  
  for(int pin = 0; pin < 4; pin++)
  {
    digitalWrite(UpperStepperPins[pin], LOW);
  }

  delay(50);

  for(int rgb = 0; rgb < 3; rgb++)
  {
    sensorData[rgb] = scanColor(rgb);
  }
  
  digitalWrite(led, LOW);

  if(SerialEnabled)
    {
      receiveCommands();
      for(int rgb = 0; rgb < 3; rgb++)
      {
        Serial.print(int(sensorData[rgb]));
        Serial.print(" ");
      }
      Serial.println();
    }
  
  //Decide the color based on the sensorData
  int candyColor = decideColor(sensorData, comparisonRatios, averageValues);
  
  if(SerialEnabled)
  {
    Serial.println(colorNames[candyColor]);
  }

  delay(candyDelay);

  //Step lower stepper to container based on decided color
  int targetContainer = candyColor;
  int targetPosition = containerPlacement[targetContainer];
  int targetSteps = targetPosition - LowerStepperCurrentPosition;
  
  LowerStepperCurrentPhase = rotate
  (
    LowerStepperPins,
    fullSteps,
    fullStepCount,
    LowerStepperCurrentPhase,
    abs(targetSteps),
    (targetSteps > 0),
    LowerStepperStepDelayus
  );
  LowerStepperCurrentPosition = targetPosition;
  
  for(int pin = 0; pin < 4; pin++)
  {
    digitalWrite(LowerStepperPins[pin], LOW);
  }


  if(SerialEnabled)
  {
    Serial.println();
    receiveCommands();
  }
}

int rotate(
  const int pins[4],
  const int stepPhases[][4],
  const int phaseCount,
  int startPhase,
  int steps,
  bool clockwise,
  int stepDelayus
){
  int sign = clockwise ? 1 : -1;
  for(int pin = 0; pin < 4; pin++) //all four pins are updated
    {
      digitalWrite(pins[pin], stepPhases[startPhase][pin]);
    }
  int phase = startPhase;
  int nextPhase;
  for(int s = 0; s < steps; s++) //one single step
  {
    nextPhase = (phase + sign) % phaseCount;
    nextPhase = nextPhase < 0 ? nextPhase + phaseCount : nextPhase;
    //phase + sign because the next phase is written before phase is updated, sign determines increase or decrease in phase, so clockwise or counterclockwise
    for(int pin = 0; pin < 4; pin++) //all four pins are updated
    {
      digitalWrite(pins[pin], stepPhases[nextPhase][pin]);
    }
    
    delayMicroseconds(stepDelayus);
    phase = nextPhase;
    
  }
  return phase;
}

unsigned int scanColor(int rgb) //takes in 0, 1 or 2 and returns one colorsensor reading
{
  switch(rgb)
  {
    case 0: //red
      {
        digitalWrite(s2,  LOW);
        digitalWrite(s3,  LOW);
        break;
      }
      
    case 1: //green
      {
        digitalWrite(s2, HIGH);
        digitalWrite(s3, HIGH);
        break;
      }
      
    case 2: //blue
      {
        digitalWrite(s2,  LOW);
        digitalWrite(s3, HIGH);
        break;
      }
    
    case 3: //#nofilter, will measure overall light intensity, may be ignored
      {
        digitalWrite(s2, HIGH);
        digitalWrite(s3,  LOW);
        break;
      }
  }
  return(pulseIn(out, LOW));
}

bool isTurnIncomplete(unsigned int colors[3]) //true with white bg otherwise 
{
  int avg = int((colors[0] + colors[1] + colors[2]) / 3);
  return (colors[1] <= whiteLightIntensity);
}

int decideColor(unsigned int colors[3], float comparisonRatios[6][3], float comparisonAverages[6])
{
  float ratios[3];
  float averageIntensity = (colors[0] + colors[1] + colors[2]) / 3; 

  for(int rgb = 0; rgb < 3; rgb++)
  {
    ratios[rgb] = colors[rgb] / averageIntensity;
  }


  float ratioDifference[6][3];
  float averageDifferences[6];

  for(int candy = 0; candy < 6; candy++)
  {
    averageDifferences[candy] = (averageIntensity - comparisonAverages[candy]);
    for(int rgb = 0; rgb < 3; rgb++)
    {
      ratioDifference[candy][rgb] = ratios[rgb] - comparisonRatios[candy][rgb];
    }
  }


  float sumOfRatioDifference[6];
  
  for(int candy = 0; candy < 6; candy++)
  {

    sumOfRatioDifference[candy] = abs(ratioDifference[candy][0]) + abs(ratioDifference[candy][1]) + abs(ratioDifference[candy][2]); 
    
  }


  float candySimilarity[6];

  for(int candy = 0; candy < 6; candy++)
  {
    candySimilarity[candy] = sumOfRatioDifference[candy] + (abs(averageDifferences[candy]) * averageDifferenceWeight);
  }


  int bestCandy = 0;

  for(int candy = 0; candy < 6; candy++)
  {
    if(candySimilarity[candy] < candySimilarity[bestCandy]){
      bestCandy = candy;
    }
  }

  if(SerialEnabled)
  {
    for(int candy = 0; candy < 6; candy++)
    {
      Serial.print(sumOfRatioDifference[candy]);
      Serial.print("  ");
      Serial.print(averageDifferences[candy]);
      Serial.print("   ");
      Serial.print(candySimilarity[candy]);
      if(candy == bestCandy)
      {
        Serial.println(" <<<");
      }
      else
      {
        Serial.println();
      }
    }
  }
  return bestCandy;
}

void receiveCommands() //reads incoming Serial buffer, and matches with some commands
{
  if(Serial.available() > 0) //check if there is any data incoming
  {
    String command;
    command = Serial.readString(); //reads incoming data
    Serial.println(command);       //prints it so you know you typed it
    if(command == "ho")     //matches with command in settings
    {                              //stopCommand pauses the program
      Serial.println("Program stopped.");
      digitalWrite(led, HIGH);
      delay(20);
      while(1)                     //cheap way to pause the program
      {                            //just have it keep running through an endless loop
        if(Serial.available() > 0)       //<<<
        {                                //these lines are already at the start of receiveCommands(), could be simplified
          command = Serial.readString(); //<<<
          Serial.println(command);   
          
          
          
          if(command == "go")
          {
            Serial.println("Program resumed.");
            break; //breaks out of while(1) loop
          }
        }
        else
        {
          digitalWrite(led, HIGH);
          delay(200);
          int rrggbb[] = {0,0,0};
          for(int rgb = 0; rgb < 3; rgb++)
          {
            rrggbb[rgb] = scanColor(rgb);
          }
        }
      }
    }
    if(command == "l")
    {
      LowerStepperCurrentPosition += 20;
      return;
    }
    if(command == "r")
    {
      LowerStepperCurrentPosition -= 22;
      return;
    }
    int color;
    if(command == "brown")
    {
      color = 0;
    }
    else if(command == "red")
    {
      color = 1;
    }
    else if(command == "orange")
    {
      color = 2;
    }
    else if(command == "yellow")
    {
      color = 3;
    }
    else if(command == "green")
    {
      color = 4;
    }
    else if(command == "blue")
    {
      color = 5;
    }
    else
    {
      return;
    }
    unsigned long total = 0;
    int amount = 20;
    unsigned long sum = 0;
    delay(200);
    for(int i = 0; i < amount; i++)
    {
      sum = 0;
      for(int rgb = 0; rgb < 3; rgb++)
      {
        sum += scanColor(rgb);
        delay(1);
      }
      unsigned int piet = int(sum / 3);
      total += piet;
      Serial.println(piet);
      delay(20);
    }
    Serial.println(total);
    total = (total / amount);
    Serial.println(total);
    float avg = float(total);
    Serial.print(averageValues[color]);
    Serial.println(" changed to");
    Serial.println(avg);
    averageValues[color] = avg;
    Serial.print("calibrated ");
    Serial.println(colorNames[color]);
  }
}


