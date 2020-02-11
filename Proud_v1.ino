#include <AWS_IOT.h>
#include <WiFi.h>
#include <Color.h>


AWS_IOT hornbill;

char WIFI_SSID[] = "Wifi BIOS";
char WIFI_PASSWORD[] = "illucens";
char HOST_ADDRESS[] = "a317m8eqa8zs6w-ats.iot.us-east-1.amazonaws.com";
char CLIENT_ID[] = "pH";
char TOPIC_NAME[] = "proud/ph";
char TOPIC_NAME1[] = "proud/gas";
char TOPIC_NAME2[] = "proud/warna";
char TOPIC_NAME3[] = "proud/buzzer";

int status = WL_IDLE_STATUS;
int tick = 0, msgCount = 0, msgReceived = 0;
char payload[512];
char payload1[512];
char payload2[512];
char rcvdPayload[512];

#define SensorPin 35            //pH meter Analog output to Arduino Analog Input 0
#define Offset 4.28            //deviation compensate
#define LED 2
#define samplingInterval 10
#define printInterval 500
#define ArrayLenth  40    //times of collection
#define GasPin 34
#define buzzer 23

// Constructors are as follows
// Color((unsigned int[]){S0, S1, S2, S3, LED}, SENSOR_OUT);
// Color((unsigned int[]){S0, S1, S2, S3, LED}, SENSOR_OUT, Frequency);

Color color((unsigned int[]) {
  4, 5, 18, 19, 2
}, 21,10000);

int pHArray[ArrayLenth];   //Store the average value of the sensor feedback
int pHArrayIndex = 0;

int Blue = 0;
int Red = 0;
int Green = 0;
int White = 0;

static unsigned long samplingTime = millis();
static unsigned long printTime = millis();
static float pHValue, voltage;
float gasValue = 0;
int flag = 0;
void mySubCallBackHandler (char *topicName, int payloadLen, char *payLoad)
{
  strncpy(rcvdPayload, payLoad, payloadLen);
  rcvdPayload[payloadLen] = 0;
  msgReceived = 1;
}

void setup(void)
{
  pinMode(LED, OUTPUT);
  Serial.begin(9600);
  digitalWrite(buzzer, 1);
  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(WIFI_SSID);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    pinMode(buzzer, OUTPUT);
    // wait 5 seconds for connection:
    delay(5000);
  }

  Serial.println("Connected to wifi");

  if (hornbill.connect(HOST_ADDRESS, CLIENT_ID) == 0)
  {
    Serial.println("Connected to AWS");
    delay(1000);

    if (0 == hornbill.subscribe(TOPIC_NAME3, mySubCallBackHandler))
    {
      Serial.println("Subscribe Successfull");
      digitalWrite(buzzer, 0);
    }
    else
    {
      Serial.println("Subscribe Failed, Check the Thing Name and Certificates");
      while (1);
    }
  }
  else
  {
    Serial.println("AWS connection failed, Check the HOST Address");
    while (1);
  }

}
void loop(void)
{
  pH();
  gas();
  warna();
  sendAWS();
}

void sendAWS() {
  String val;
  if (msgReceived == 1)
  {
    Serial.print("Received Message:");
    val = rcvdPayload;
    Serial.println(val);

    msgReceived = 0;
  }
  if (val == "2") {
    flag = 2;
  }
  else if (val == "1")
  {
    flag = 1;
  }
  else if (val == "0") {
    flag = 0;
  }
  buz();
  if (tick >= 2)  // publish to topic every 5seconds
  {
    tick = 0;
    sprintf(payload, "%f", pHValue);
    sprintf(payload1, "%f", gasValue);
    sprintf(payload2, "%d", White);
    if (hornbill.publish(TOPIC_NAME, payload) == 0 && hornbill.publish(TOPIC_NAME1, payload1) == 0 && hornbill.publish(TOPIC_NAME2, payload2) == 0)
    {
      Serial.println("Publish Message:");
      Serial.print("\tpH Sensor :\t");
      Serial.println(payload);
      Serial.print("\tGas sensor :\t");
      Serial.println(payload1);
      Serial.print("\tColor Sensor :\t");
      Serial.println(payload2);
    }
    else
    {
      Serial.println("Publish failed");
    }


  }

  vTaskDelay(500 / portTICK_RATE_MS);
  tick++;
}
void buz() {
  if (flag == 2) {

    digitalWrite(buzzer, 1);
  }
  else if (flag == 1)
  {

    digitalWrite(buzzer, 1);
  }
  else if (flag == 0) {

    digitalWrite(buzzer, 0);
  }
}
void warna() {
  Blue = color.blue();
  Red = color.red();
  Green = color.green();
  White = color.white();

  //  String disp = String("Blue :\t" + String(Blue) + "\tRed :\t" + String(Red) + "\tGreen :\t" + String(Green));
  //  Serial.print(disp);
//  Serial.print("\tWhite :\t");
//  Serial.println(White);
}
void gas() {
  gasValue = analogRead(GasPin);
  Serial.print("Nilai Sensor Gas: ");
  Serial.println(gasValue);
}
void pH() {

  if (millis() - samplingTime > samplingInterval)
  {
    pHArray[pHArrayIndex++] = analogRead(SensorPin);
    if (pHArrayIndex == ArrayLenth)pHArrayIndex = 0;
    voltage = avergearray(pHArray, ArrayLenth) * 3.3 / 4095;
    pHValue = 3.5 * voltage + Offset;
    samplingTime = millis();
  }
  if (millis() - printTime > printInterval)  //Every 800 milliseconds, print a numerical, convert the state of the LED indicator
  {
    Serial.print("Voltage:");
    Serial.print(voltage, 2);
    Serial.print("    pH value: ");
    Serial.println(pHValue, 2);
    digitalWrite(LED, digitalRead(LED) ^ 1);
    printTime = millis();
  }
}
double avergearray(int* arr, int number) {
  int i;
  int max, min;
  double avg;
  long amount = 0;
  if (number <= 0) {
    Serial.println("Error number for the array to avraging!/n");
    return 0;
  }
  if (number < 5) { //less than 5, calculated directly statistics
    for (i = 0; i < number; i++) {
      amount += arr[i];
    }
    avg = amount / number;
    return avg;
  } else {
    if (arr[0] < arr[1]) {
      min = arr[0]; max = arr[1];
    }
    else {
      min = arr[1]; max = arr[0];
    }
    for (i = 2; i < number; i++) {
      if (arr[i] < min) {
        amount += min;      //arr<min
        min = arr[i];
      } else {
        if (arr[i] > max) {
          amount += max;  //arr>max
          max = arr[i];
        } else {
          amount += arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount / (number - 2);
  }//if
  return avg;
}
