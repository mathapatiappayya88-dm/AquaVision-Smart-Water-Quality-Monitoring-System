#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "AquaVision Water Monitor"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ================= WIFI =================
char ssid[] = "YOUR_WIFI_NAME";
char pass[] = "YOUR_WIFI_PASSWORD";

// ================= SENSOR PINS =================
#define TDS_PIN        34
#define PH_PIN         35
#define TURBIDITY_PIN  32
#define TEMP_PIN        4

// ================= DS18B20 =================
OneWire oneWire(TEMP_PIN);
DallasTemperature tempSensor(&oneWire);

// ================= BLYNK =================
BlynkTimer timer;

// ================= SENSOR VALUES =================
float temperatureC = 25.0;
float tdsValue = 0.0;
float phValue = 7.0;
float turbidityNTU = 0.0;

float overallScore = 0.0;
String waterStatus = "STARTING...";

// =================================================
// READ TEMPERATURE
// =================================================
void readTemperature()
{
  tempSensor.requestTemperatures();

  float temp = tempSensor.getTempCByIndex(0);

  if (temp != DEVICE_DISCONNECTED_C && temp > -50 && temp < 125)
  {
    temperatureC = temp;
  }
}

// =================================================
// READ TDS
// =================================================
void readTDS()
{
  int adcValue = analogRead(TDS_PIN);

  float voltage = adcValue * 3.3 / 4095.0;

  // Temperature compensation
  float compensationCoefficient =
      1.0 + 0.02 * (temperatureC - 25.0);

  float compensationVoltage =
      voltage / compensationCoefficient;

  // Gravity TDS approximate formula
  tdsValue =
      (133.42 * compensationVoltage * compensationVoltage *
       compensationVoltage
       - 255.86 * compensationVoltage * compensationVoltage
       + 857.39 * compensationVoltage) * 0.5;

  if (tdsValue < 0)
    tdsValue = 0;

  if (tdsValue > 2000)
    tdsValue = 2000;
}

// =================================================
// READ pH
// =================================================
void readPH()
{
  int adcValue = analogRead(PH_PIN);

  float voltage = adcValue * 3.3 / 4095.0;

  // Approximate pH formula
  phValue = 7.0 + ((2.50 - voltage) / 0.18);

  if (phValue < 0)
    phValue = 0;

  if (phValue > 14)
    phValue = 14;
}

// =================================================
// READ TURBIDITY
// =================================================
void readTurbidity()
{
  int adcValue = analogRead(TURBIDITY_PIN);

  float voltage = adcValue * 3.3 / 4095.0;

  // Approximate turbidity formula
  turbidityNTU =
      -1120.4 * voltage * voltage
      + 5742.3 * voltage
      - 4352.9;

  if (voltage >= 2.5)
  {
    turbidityNTU = 0;
  }

  if (turbidityNTU < 0)
    turbidityNTU = 0;

  if (turbidityNTU > 1000)
    turbidityNTU = 1000;
}

// =================================================
// pH SCORE
// =================================================
float calculatePHScore()
{
  if (phValue >= 6.5 && phValue <= 8.5)
    return 100;

  if (phValue >= 6.0 && phValue < 6.5)
    return 75;

  if (phValue > 8.5 && phValue <= 9.0)
    return 75;

  if (phValue >= 5.5 && phValue < 6.0)
    return 50;

  if (phValue > 9.0 && phValue <= 9.5)
    return 50;

  return 25;
}

// =================================================
// TDS SCORE
// =================================================
float calculateTDSScore()
{
  if (tdsValue <= 500)
    return 100;

  if (tdsValue <= 750)
    return 75;

  if (tdsValue <= 1000)
    return 50;

  return 25;
}

// =================================================
// TURBIDITY SCORE
// =================================================
float calculateTurbidityScore()
{
  if (turbidityNTU <= 50)
    return 100;

  if (turbidityNTU <= 100)
    return 75;

  if (turbidityNTU <= 200)
    return 50;

  return 25;
}

// =================================================
// TEMPERATURE SCORE
// =================================================
float calculateTemperatureScore()
{
  // Demo range – adjust according to aquarium species
  if (temperatureC >= 24 && temperatureC <= 30)
    return 100;

  if (temperatureC >= 20 && temperatureC < 24)
    return 75;

  if (temperatureC > 30 && temperatureC <= 34)
    return 75;

  if (temperatureC >= 18 && temperatureC < 20)
    return 50;

  if (temperatureC > 34 && temperatureC <= 36)
    return 50;

  return 25;
}

// =================================================
// CALCULATE OVERALL WATER QUALITY
// =================================================
void calculateWaterQuality()
{
  float phScore = calculatePHScore();
  float tdsScore = calculateTDSScore();
  float turbidityScore = calculateTurbidityScore();
  float temperatureScore = calculateTemperatureScore();

  // Weighted overall calculation
  overallScore =
      (phScore * 0.30) +
      (tdsScore * 0.25) +
      (turbidityScore * 0.25) +
      (temperatureScore * 0.20);

  // Overall status
  if (overallScore >= 80)
  {
    waterStatus = "WATER QUALITY GOOD";
  }
  else if (overallScore >= 60)
  {
    waterStatus = "WATER QUALITY MODERATE";
  }
  else
  {
    waterStatus = "WATER QUALITY POOR";
  }

  // Serial monitor
  Serial.println();
  Serial.println("========== AQUAVISION ==========");

  Serial.print("pH          : ");
  Serial.println(phValue, 2);

  Serial.print("TDS         : ");
  Serial.print(tdsValue, 0);
  Serial.println(" ppm");

  Serial.print("Turbidity   : ");
  Serial.print(turbidityNTU, 1);
  Serial.println(" NTU");

  Serial.print("Temperature : ");
  Serial.print(temperatureC, 2);
  Serial.println(" C");

  Serial.print("Overall     : ");
  Serial.print(overallScore, 1);
  Serial.println(" / 100");

  Serial.print("Status      : ");
  Serial.println(waterStatus);

  Serial.println("================================");
}

// =================================================
// SEND DATA TO BLYNK
// =================================================
void sendToBlynk()
{
  // Read sensors
  readTemperature();
  readTDS();
  readPH();
  readTurbidity();

  // Calculate overall quality
  calculateWaterQuality();

  // Send to Blynk
  Blynk.virtualWrite(V0, phValue);
  Blynk.virtualWrite(V1, tdsValue);
  Blynk.virtualWrite(V2, turbidityNTU);
  Blynk.virtualWrite(V3, temperatureC);
  Blynk.virtualWrite(V4, waterStatus);
}

// =================================================
// SETUP
// =================================================
void setup()
{
  Serial.begin(115200);

  // ADC resolution
  analogReadResolution(12);

  // Start temperature sensor
  tempSensor.begin();

  Serial.println();
  Serial.println("Starting AquaVision...");

  // Connect Blynk + WiFi
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Read and send every 2 seconds
  timer.setInterval(2000L, sendToBlynk);
}

// =================================================
// LOOP
// =================================================
void loop()
{
  Blynk.run();
  timer.run();
}