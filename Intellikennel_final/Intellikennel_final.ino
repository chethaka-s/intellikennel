#include <DHT.h>
#include <ESP32Servo.h>
#include "WiFi.h"
#include <ESPAsyncWebSrv.h>
#include <HTTPClient.h>
#include "ThingSpeak.h" 
#include "HX711.h"

Servo myservo;
static const int servoPin = 13;

#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define DOUT_PIN  23
#define SCK_PIN   22
HX711 scale;

// Set to true to define Relay as Normally Open (NO)
#define RELAY_NO    true

// Set number of relays
#define NUM_RELAYS  3

// Assign each GPIO to a relay
int relayGPIOs[NUM_RELAYS] = {2, 26, 25};

// Replace with your network credentials
const char* ssid = "Dialog 4G 828";
const char* password = "bF8FcaC1";

const char* serverName = "http://api.thingspeak.com/update";

unsigned long channel =2463355;



String apiKey = "AXNE58UH1T2YDID2";


const char* PARAM_INPUT_1 = "relay";  
const char* PARAM_INPUT_2 = "state";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);


String header;
String valueString = String(5);
int pos1 = 0;
int pos2 = 0;

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">

  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js"></script>

// <script>
//   function refill() {
//     var xhr = new XMLHttpRequest();
//     xhr.open("GET", "/refill", true);
//     xhr.send();
//   }
// </script>


  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .sub-text{    font-size: 16px; color: grey;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .button { background-color: #4CAF50; /* Green */ border: none; color: white; padding: 15px 32px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; cursor: pointer;}
    .door{height:auto; width:auto; padding:10px;    border-radius: 21px; border: 2px solid #020249; }
    .box{display:flex; flex-direction:column; align-items: center;}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>Intellikennel</h2>
  <hr>
  <h4>Door Functionality</h4>

<script>
        var slider = document.getElementById("servoSlider");
        var servoP = document.getElementById("servoPos");
        servoP.innerHTML = slider.value;

        slider.oninput = function() {
            slider.value = this.value;
            servoP.innerHTML = this.value;
        };

        function servo(pos) {
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                    // Optional: handle response if needed
                }
            };
            xhttp.open("GET", "/?value=" + pos, true);
            xhttp.send();
        }
    </script>

<hr>





  <h4>Lights</h4>
  <p class="sub-text">Turn the Lights Inside the Kennel</p>
  <label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="1"><span class="slider"></span></label>
  <h4>Refill Water</h4>
   <p class="sub-text">Refill Water to the Water Bowl</p>
  <label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="2"><span class="slider"></span></label>
 
  <h4>Spray Disinfectant</h4>
  <p class="sub-text">Spray Disinfectant to the Floor (Use appproximately for 2 minutes for the best results)</p>
  <label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="3"><span class="slider"></span></label>
  
  <hr>
  <div class="box">
   <h4>Kennel Temperature</h4>
  <iframe width="450" height="260" style="border: 1px solid #cccccc;" src="https://thingspeak.com/channels/2463355/widgets/829775"></iframe>
  </div>

  <hr>
  <div class="box">
  <h4>Camera</h4>
<iframe src="http://192.168.8.182:81/stream" width="640" height="480" frameborder="0" allowfullscreen></iframe>

  </div>

<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?relay="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/update?relay="+element.id+"&state=0", true); }
  xhr.send();
}</script>
</body>
</html>
)rawliteral";

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  
  dht.begin();

  pinMode(12, OUTPUT);
  pinMode(14, OUTPUT);
  digitalWrite(12, 0);
  digitalWrite(14, 0);

 scale.begin(DOUT_PIN, SCK_PIN);
  // feedingServo.attach(servoPin);
    myservo.attach(servoPin);
  // You might need to adjust the calibration factor for your load cell
  scale.set_scale(-626.4); // This calibration factor is for my load cell, you need to calibrate yours
  scale.tare(); // Reset the scale to 0
  


  // Set all relays to off when the program starts - if set to Normally Open (NO), the relay is off when you set the relay to HIGH
  for(int i=1; i<=NUM_RELAYS; i++){
    pinMode(relayGPIOs[i-1], OUTPUT);
    if(RELAY_NO){
      digitalWrite(relayGPIOs[i-1], HIGH);
    }
    else{
      digitalWrite(relayGPIOs[i-1], LOW);
    }
  }
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Send a GET request to <ESP_IP>/update?relay=<inputMessage>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    String inputMessage2;
    String inputParam2;
    // GET input1 value on <ESP_IP>/update?relay=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1) & request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      inputParam2 = PARAM_INPUT_2;
      if(RELAY_NO){
        Serial.print("NO ");
        digitalWrite(relayGPIOs[inputMessage.toInt()-1], !inputMessage2.toInt());
      }
      else{
        Serial.print("NC ");
        digitalWrite(relayGPIOs[inputMessage.toInt()-1], inputMessage2.toInt());
      }
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage + inputMessage2);
    request->send(200, "text/plain", "OK");
  });


  //   server.on("/refill", HTTP_GET, [](AsyncWebServerRequest *request){
  //   feedingServo.write(90); // Open the flap
  //   delay(10000); // Keep the flap open for 10 seconds
  //   feedingServo.write(0); // Close the flap
  //   request->send(200, "text/plain", "Refill completed");
  // });
  // Start server
  server.begin();
}
  
void loop() {
   long raw = scale.read();
  float weight = (((raw / 407.8) + 285.5) * 3.7)+6093; // You should use the calibration factor you determined during calibration

  Serial.print("Weight: ");
  Serial.print(weight);
  Serial.println(" grams");



  delay(1000); 
  
  if(WiFi.status()== WL_CONNECTED){

      WiFiClient client;

      HTTPClient http;

      delay(10000); // wait for 10 seconds

     //float h = dht.readHumidity();

      float t = dht.readTemperature();

 

      if (isnan(t)) {

       Serial.println(F("Failed to read from DHT sensor!"));

       return;

      }

   

      // Your Domain name with URL path or IP address with path

      http.begin(client, serverName);

     

      // Specify content-type header

      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      // Data to send with HTTP POST

      String httpRequestData = "api_key=" + apiKey + "&field1=" + String(t);          

      // Send HTTP POST request

      int httpResponseCode = http.POST(httpRequestData);

     

      /*

      // If you need an HTTP request with a content type: application/json, use the following:

      http.addHeader("Content-Type", "application/json");

      // JSON data to send with HTTP POST

      String httpRequestData = "{\"api_key\":\"" + apiKey + "\",\"field1\":\"" + String(random(40)) + "\"}";          

      // Send HTTP POST request

      int httpResponseCode = http.POST(httpRequestData);*/

     

      Serial.print("HTTP Response code: ");

      Serial.println(httpResponseCode);

 

      http.end();

    }

    else {

      Serial.println("WiFi Disconnected");

    }

}
