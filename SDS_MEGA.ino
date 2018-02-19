////////////////////////////////////////////////////////////////////////////////////
// This program allows you to communicate with your Suzuki's ECU and request
// info from the sensors. Only tested on Suzuki Bking (hayabusa engine).
// Hardware used:Arduino MEGA, K-Line transceiver, 510 Ohm and 10nF
// Author: Jorge Javier Fernandez Rodriguez
// Version: 1.0
// Date: 19/01/2018
////////////////////////////////////////////////////////////////////////////////////
//
//********************************** MEGA OPTIONS **********************************
//Communication:
#define PCbaud 115200   //PC Comunication 
#define console Serial         //PC_IN                                                                                                 // TX Pin
                                //PC_OUT
#define SDSbaud 10400 //K-Line Comunication 
#define sds Serial1        //K_IN                                                                                               // TX Pin
                           //K_OUT
#define TX1 18
#define LED 13

//--------------------------------- K_Line OPTIONS ---------------------------------

byte K_START_COM[5] = {
  0x81, 0x12, 0xF1, 0x81, 0x05}; //Start Sequence
byte ECU_RESPONSE[8] = {
  0x80,0xF1,0x12,0x03,0xC1,0xEA,0x8F,0xC0};//Echo from ECU
byte K_READ_ALL_SENS[7] = {
  0x80, 0x12, 0xF1, 0x02, 0x21, 0x08, 0xAE}; //Request all sensors
boolean ECUconnected = false;                                                                            // timer
uint8_t k_outCntr, k_inCntr, k_inByte, k_chksm, k_size;

//--------------------------------- Suzuki Variables ---------------------------------
int TPS = 0;
float VBATT, ECT = 0;
// --------------------------------- Engine Values ---------------------------------
uint8_t GPS, FUEL;
uint16_t rpm1, RPM, iap, IAP, SPEED;
int mode = 0;
boolean request = false;

//********************************** END MEGA OPTIONS **********************************
void Got_Welcome();
void fastInit();
void receiveHandshake();
void sensorRequest();
void toggleLed();
void cleanBuffer();
boolean k_transmit(byte *function, byte num);

void setup() {
  // put your setup code here, to run once:
  cleanBuffer();
  console.begin(PCbaud);   // Start Serial Comunication Laptop
  pinMode(TX1, OUTPUT);   // Set TX1 to Output - Needed for K
  pinMode(LED, OUTPUT);
  Got_welcome();
  delay(3000); //Time for booting up the bike
}

void loop() {
    //put your main code here, to run repeatedly:
     if(!ECUconnected){
      fastInit();
     }else{
      console.println("\n----------------------------------------------");
      sensorRequest();
      toggleLed();
     }
}

//********************************** FUNCTIONS **********************************
//-------------------------------- GOTENMAN WELCOME -----------------------------
void Got_welcome(){
  delay(2000); //Time for opening serial
  toggleLed();
  toggleLed();
  delay(500);
  Serial.println("***************************************************");
  Serial.println("***************** WELCOME TO SDS *****************");
  Serial.println("***************    SUZUKI BKING    ***************");
  Serial.println("***************************************************");
  Serial.println("");  
}

// --------------------------------- LED ---------------------------------
void toggleLed() {
  digitalWrite(LED, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);                       // wait for a second
  digitalWrite(LED, LOW);    // turn the LED off by making the voltage LOW
  delay(100);                       // wait for a second
}

// --------------------------------- KWP2000 Init Pulse ---------------------------------
// Funcion booleana para secuencia FastInit
void fastInit(){  
  if(mode == 0){
    // This is the ISO 14230-2 "Fast Init" sequence.
    sds.end();
    digitalWrite(TX1, HIGH);
    delay(2000);
    digitalWrite(TX1, LOW);
    delay(25);
    digitalWrite(TX1, HIGH);
    delay(25);
    console.print("Fast Init DONE");
    sds.begin(SDSbaud);

    console.print("\n\nSending to ECU: ");
    for (int i = 0; (i <= 4); i++){
      sds.write(K_START_COM[i]);
      console.print(" 0x");console.print(K_START_COM[i],HEX);    
      delay(6);  //inter byte delay for to match k-line spec. changed from 15ms to 1, tune as needed
    }
    console.print("\nStarting K Sequence DONE...");
    console.print("\nRecieving echo from ECU: ");
    //sds.end();sds.begin(SDSbaud); //LIMPIAR BUFFER - SOLUCION CUTRE MIRAR
    cleanBuffer();
    delay(35);//Time between end of tester request and start of ECU response
    while(!sds.available());
    while (sds.available()){
      console.print(" 0x");console.print(sds.read(),HEX);
    }
    cleanBuffer();
  }
  mode = 1;
  ECUconnected = true;
}

void sensorRequest(){
  
  if (mode == 1){
    currentTimer = millis();
      for (int i = 0; (i <= 6); i++){
        sds.write(K_READ_ALL_SENS[i]);
        //console.print(" 0x");console.print(K_READ_ALL_SENS[i],HEX);    
        delay(6);  //inter byte delay for to match k-line spec. changed from 15ms to 1, tune as needed
      }
      
    //}  
    while(!sds.available()){cleanBuffer();}
    k_inCntr = 0;
    k_inByte = 0;
    ////////////////////////// DATA FROM SENSORS //////////////////////////
      while(sds.available()){
        //console.print(sds.read(), HEX);console.print(" ");
        k_inByte = sds.read();
        k_inCntr++;
        
        //console.print("\nContador: ");console.print(k_inCntr);
        switch(k_inCntr){
          case 17://NOT CORRECT
            SPEED = k_inByte * 2;
            console.print("\nSpeed: ");console.print(k_inByte,HEX);console.println(" Km/h");
          break;
          case 18:
          //Revolutions to be used in case 26
            rpm1 = 0;
            rpm1 = k_inByte * 100;
          break;
          case 19:
          //RPM
            RPM = k_inByte * 100 / 255 + rpm1;
            console.print("RPM: ");console.println(RPM);
          break;
          case 20:
            //Throttle position in percentage
            TPS = ((k_inByte - 55) * 100 / 169);
            console.print("Throttle Position: ");console.print(TPS);console.println(" %");
          break;
          case 22:
          //Engine Temperature in Celsius
            ECT = ((k_inByte * 100 / 160) - 30); //((k_inByte-48)/1.6);
            console.print("Temperatura motor: ");console.print(ECT);console.println(" ºC");
          break;
          case 25:
          //Battery Voltage in Volts
            VBATT = k_inByte*(20.0/255);
            console.print("Battery Voltage: ");console.print(VBATT);console.println(" V");
          break;
          case 27:
          //GEAR ENGAGED
            console.print("Gear Engaged: ");console.println(k_inByte);
          break;
          case 42:
          //IGNITION
            if (k_inByte == 64){
              console.println("IGNITION: OFF");}
            else{
              console.println("IGNITION: ON");}
          break;
          case 53:
          //CLUTCH
            if(k_inByte == 0x04){
              console.println("Clutch RELEASED");
            }
            else if(k_inByte == 0x14){
              console.println("Clutch PRESSED");
            }
          break;
          case 54:
          if(k_inByte == 0x00){
              console.println("Neutral Sensor: NEUTRAL");
            }
            else if(k_inByte == 0x02){
              console.println("Neutral Sensor: GEAR ENGAGED");
            }
          break;
          default: break;
        }
      } 
  }
  delay(2);
  cleanBuffer();
}

void cleanBuffer(){
  while(sds.available()>0){
     byte c = sds.read();
  }
}
//-------------------------------- END OF FUNCTIONS -----------------------------------
