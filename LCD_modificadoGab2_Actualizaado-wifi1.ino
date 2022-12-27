#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT11.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);
#include <BlynkSimpleEsp8266.h>
#define BLYNK_PRINT Serial
//configuracion movil
#include <DNSServer.h>
#include <ESP8266WebServer.h >
#include <WiFiManager.h>
#include <Ticker.h>
// Instancia a la clase Ticker
Ticker ticker;
// Pin LED azul
byte pinLed = D0;

void parpadeoLed() {
  // Cambiar de estado el LED
  byte estado = digitalRead(pinLed);
  digitalWrite(pinLed, !estado);
}
// gas 
#define MQ1      (0)     //define la entrada analogica para el sensor
#define RL_VALOR (5)     //define el valor de la resistencia mde carga en kilo ohms
#define RAL      (9.83)  // resistencia del sensor en el aire limpio / RO, que se deriva de la                                             tabla de la hoja de datos
#define GAS_LP   (0)    
String inputstring = ""; //Cadena recibida desde el PC
float LPCurve[3]  =  {2.3,0.21,-0.47};
float Ro =  10;  
int TiempoResultado;
//----------------configuracion wifi-------------------------
char message_buff[100];
//const char* auth = "PLasZbUR98HzW1DkzwAWwmhFp723bjaj";
const char* mqtt_server = "192.168.1.147";

//----------------hora-------------------------

WiFiUDP ntpUDP;
int16_t utc = -3; //UTC -3:00 horario verano, -4 INVIERNO
uint32_t currentMillis = 0;
uint32_t previousMillis = 0;
NTPClient timeClient(ntpUDP, "ntp.shoa.cl", utc * 3600, 60000);
char daysOfTheWeek[7][12] = {"Domingo  ", "Lunes  ", "Martes  ", "Miercoles", "Jueves   ", "Viernes ", "Sabado "};
//----------------------------------------------

//configuracion wifi------------------------------
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

//---------------------------------------------------------
const int primerPiso  = 14;    //D5
const int buzzer = 12;         //D6
int pin = 13;                  //D7
const int VAL = 15;            //D8
const int PIN_AP = 1; // pulsador para volver al modo AP // Tx
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DHT11 dht11(pin);
unsigned long previousMillis1 = 0;
const long interval = 9000;

unsigned long previousMillis2 = 0;
const long interval2 = 2000;

//--------------reset-------------------
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Modo de configuración ingresado");
  Serial.println(WiFi.softAPIP());

  Serial.println(myWiFiManager->getConfigPortalSSID());
}

//flag for saving data
bool shouldSaveConfig = false;

void saveConfigCallback () {
  Serial.println("Debería guardar la configuración");
  shouldSaveConfig = true;
}

void setup() {

/////////////
  Serial.begin(115200);
  pinMode(primerPiso, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(VAL, OUTPUT);

  lcd.clear();
  Wire.begin(5, 4);   //Use predefined PINS consts
  lcd.begin(20, 4);     // The begin call takes the width and height. This

  //------------configuracion movil--------------------------
  // Modo del pin
  pinMode(pinLed, OUTPUT);
  pinMode(PIN_AP, INPUT);
 
  // Empezamos el temporizador que hará parpadear el LED
  ticker.attach(0.2, parpadeoLed);

  // Creamos una instancia de la clase WiFiManager
  WiFiManager wifiManager;

  // Descomentar para resetear configuración
  //wifiManager.resetSettings();

  // Cremos AP y portal cautivo y comprobamos si
  // se establece la conexión
  if (!wifiManager.autoConnect("GABINETE2")) {
    Serial.println("Fallo en la conexión (timeout)");
    ESP.reset();
    delay(1000);
  }

  Serial.println("Ya estás conectado");

  // Eliminamos el temporizador
  ticker.detach();

  // Apagamos el LED
  digitalWrite(pinLed, HIGH);


  //devolución de llamada para cuando entra en el modo de configuración AP
  wifiManager.setAPCallback(configModeCallback);
  //devolución de llamada cuando se conecta a una red, es decir, cuando pasa a trabajar en modo EST
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //crea una red de nombre ESP_AP con pass 12345678
  wifiManager.autoConnect("ESP_AP", "12345678");

  //---------------------fin gonfiguacion movil setup----------------------------

  //-------------wifi----------------------
  // setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //---------------------------------------
  //-------------hora---------------------
  timeClient.begin();
  timeClient.update();
  
//++++++++++++++++++++++++++++++++++++++++++  
// calibrandon valvula de gas 
Ro = Calibracion(MQ1);  
//++++++++++++++++++++++++++++++++++++++++++

}

void forceUpdate(void) {
  timeClient.forceUpdate();
}


void checkOST(void) {
  currentMillis = millis();//Tempo atual em ms
  //Lógica de verificação do tempo
  if (currentMillis - previousMillis > 1000) {
    previousMillis = currentMillis;    // Salva o tempo atual
    printf("T ", timeClient.getEpochTime());
    Serial.println(timeClient.getFormattedTime());

    char bufreloj [10]; // array para almacenar el valor de humedad para mostrar en OLED ex: 20.40
    String reloj;
    String r = timeClient.getFormattedTime();
    reloj = String (r);
    reloj.toCharArray (bufreloj, 10);
    Serial.print(daysOfTheWeek[timeClient.getDay()]);
    lcd.setCursor(2, 1);
    lcd.print(bufreloj);
    lcd.setCursor(11, 1);
    lcd.print(daysOfTheWeek[timeClient.getDay()]);
      
  }

}

//--------------------------------------

void Tempe_OLED() {

  unsigned long currentMillis1 = millis();
  if (currentMillis1 - previousMillis1 >= interval) {
    previousMillis1 = currentMillis1;
    char bufHumi [5]; // array para almacenar el valor de humedad para mostrar en OLED ex: 20.40
    char bufTemp [5]; // array almacena el valor de temperatura para mostrar en OLED ex: 22.00
    String strTemp;
    String strHumi;
    int err;
    float temp, hum;
    if ((err = dht11.read(hum, temp)) == 0)   // Si devuelve 0 es que ha leido bien
    {
      strTemp = String (temp); // conversión de t como float en una cadena
      strHumi = String (hum);
      strTemp.toCharArray (bufTemp, 5); // conversión de t como cadena a la matriz
      strHumi.toCharArray (bufHumi, 5);

      lcd.backlight();      // Turn on the backlight.

      lcd.home();
      //lcd.clear();
      lcd.setCursor(4, 2);
      lcd.print(bufTemp);
      lcd.print(" Celsius");
      lcd.setCursor(4, 3);
      lcd.print(bufHumi);
      lcd.print(" %Humedad");
   
      static int counter = 0;
      String payload ;
      payload += "MSG001 ";
      payload += "Temperatura: ";
      payload += temp;
      payload += " C";
      payload += "Humedad: ";
      payload += hum;
      //Serial.println(payload.c_str());
      //client.publish("Gab2/BD", payload.c_str());

      String payload1;

      payload1 += temp;
      Serial.println(payload1.c_str());
      client.publish("Gab2/temp", payload1.c_str());

      String payload2;

      payload2 += hum;
      Serial.println(payload2.c_str());
      client.publish("Gab2/hume", payload2.c_str());

   } 
  }

}

void callback(char* topic, byte* payload, unsigned int length) {
//recive mensaje  
  int i = 0;
  for (i = 0; i < length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  String msgString = String(message_buff);
  char men [20]; // array para almacenar el valor de humedad para mostrar en OLED ex: 20.40
  String mensaje;
  String m = msgString;
  mensaje = String (m);
  mensaje.toCharArray (men, 20);

 Serial.print(men);

  if (msgString.equals("Gablolin Gab2ONComedor")) {

    digitalWrite(primerPiso, HIGH);
    client.publish("Gab2/luz", "Encender");

  } else if (msgString.equals("Gablolin Gab2OFFComedor")) {

    digitalWrite(primerPiso, LOW);
    client.publish("Gab2/luz", "apagar");
  }

  if (msgString.equals("Gab2Temperatura"))
  {
    int err;
    float temp, hum;
    if ((err = dht11.read(hum, temp)) == 0)   // Si devuelve 0 es que ha leido bien
    {

      static int counter = 0;
      String payload ;
      payload += "MSG001";
      payload += "Temperatura: ";
      payload += temp;
      payload += " C";
      payload += ", Humedad: ";
      payload += hum;
      Serial.println(payload.c_str());
      client.publish("Gab2lolin2", payload.c_str());

   
    }
  }
  if (msgString.equals("SismoValvulaCerrada")) {
    int a =0;
      TiempoResultado = 15;
              while (a < TiempoResultado) {
                delay(1000);
                lcd.setCursor(0, 0);
                lcd.print("SISMO Val GAS OFF");
                client.publish("Gab2/sismo", "SISMO Val GAS OFF");
                digitalWrite(buzzer, HIGH);
                digitalWrite(VAL, LOW);
                tone(buzzer, 1000, 200);
                a++;
                
              }
    
    } 
    
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("Gab2lolin2")) {
      client.publish("Gab2/lolin2", "Hola, ESTOY CONECTADO GAB2LOLIN2 COCINA");
      client.subscribe("Gablolin");
      client.subscribe("Gab2/BD");
      client.subscribe("Gab2/temp");
      client.subscribe("Gab2/hume");
      client.subscribe("Gab2/vgas");
      client.subscribe("Gab2/sismo");
      client.subscribe("Gab2/vgas/valor");
      client.subscribe("Gab2/luz");
      
    } else {
      delay(1000);
    }
  }
}


void loop() {

  checkOST();
  Tempe_OLED();


  unsigned long currentMillis2 = millis();
  if (currentMillis2 - previousMillis2 >= interval2) {
    previousMillis2 = currentMillis2;
    
    // Obtener la Rs promedio
   float concentration = porcentaje_gas(lecturaMQ(MQ1)/Ro,GAS_LP);   // Obtener la concentración
    String payload3;
    payload3 += concentration;    
    
    Serial.println(payload3.c_str());
    client.publish("Gab2/vgas/valor", payload3.c_str()); 
    delay(1000);

    if (concentration > 1500){
    lcd.setCursor(0, 0);
    lcd.print("Valvula GAS Cerrada");
    client.publish("Gab2/vgas", "Valvula GAS Cerrada");
    digitalWrite(buzzer, HIGH);
    digitalWrite(VAL, LOW);
    tone(buzzer, 1000, 200);
    }
    else
    {
    lcd.setCursor(0, 0);
    lcd.print("Valvula GAS Activa");
  client.publish("Gab2/vgas", "Valvula GAS Activa");
    digitalWrite(buzzer, LOW);
    digitalWrite(VAL, HIGH);
    noTone(buzzer);
    }    
   }
  // 
  if (!client.connected()) {
    reconnect();
  }
 
  WiFiManager wifiManager;
  //si el botón se ha presionado
Serial.println("qui estoy");
  if ( digitalRead(PIN_AP) == LOW ) {
Serial.println("funciono");
    Serial.println("reajustar"); //resetear intenta abrir el portal
    if (!wifiManager.startConfigPortal("ESP_AP", "12345678") ) {
      Serial.println("No se pudo conectar");
      delay(1000);
      ESP.restart();
      delay(1000);
    }
    Serial.println("conectado ESP_AP!!!");
  }
 //Blynk.run(); 
client.loop();
 
}

float calc_res(int raw_adc)
{
  return ( ((float)RL_VALOR*(1023-raw_adc)/raw_adc));
}
 
float Calibracion(float mq_pin){
  int i;
  float val=0;
    for (i=0;i<50;i++) {                                                                               //tomar múltiples muestras
    val += calc_res(analogRead(mq_pin));
    delay(500);
  }
  val = val/50;                                                                                         //calcular el valor medio
  val = val/RAL;
  return val;
}
 
float lecturaMQ(int mq_pin){
  int i;
  float rs=0;
  for (i=0;i<5;i++) {
    rs += calc_res(analogRead(mq_pin));
    delay(50);
  }
rs = rs/5;
return rs;
}
 
int porcentaje_gas(float rs_ro_ratio, int gas_id){
   if ( gas_id == GAS_LP ) {
     return porcentaje_gas(rs_ro_ratio,LPCurve);
   }
  return 0;
}
 
int porcentaje_gas(float rs_ro_ratio, float *pcurve){
  return (pow(10, (((log(rs_ro_ratio)-pcurve[1])/pcurve[2]) + pcurve[0])));
}




