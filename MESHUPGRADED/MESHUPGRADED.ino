#include "FS.h"
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMesh.h>

#define MESH_INTR 5 //send Webserver an INTR
#define MESH_INTACK 4 //INTR from MESH
#define DEBUG_PIN 16


//////////////////////////////////////////////////////////////////////////////////////CHANGE PARAMETER PER MESH NODE////////////////////////////////////////////////////////
String ackID = "[ACK] by EN1";
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

String exampleMeshName("MeshNode_");

volatile bool INTR = false;
String decodedmsg = "";

unsigned int requestNumber = 0;
unsigned int responseNumber = 0;

String sList = "";
boolean toTransmit = false; 

String manageRequest(const String &request, ESP8266WiFiMesh &meshInstance);
transmission_status_t manageResponse(const String &response, ESP8266WiFiMesh &meshInstance);
void networkFilter(int numberOfNetworks, ESP8266WiFiMesh &meshInstance);

/* Create the mesh node object */
ESP8266WiFiMesh meshNode = ESP8266WiFiMesh(manageRequest, manageResponse, networkFilter, "ChangeThisWiFiPassword_TODO", exampleMeshName, "", true);

void ServeWeb(){
  INTR = true;
}

/**
   Callback for when other nodes send you a request

   @param request The request string received from another node in the mesh
   @param meshInstance The ESP8266WiFiMesh instance that called the function.
   @returns The string to send back to the other node
*/
String manageRequest(const String &request, ESP8266WiFiMesh &meshInstance){
  /* Print out received message */
  if(!request.indexOf("[ACK]")>=0){
    if(!sList.indexOf(request)>=0){ 
      sList.concat(request); 

      //File f = SPIFFS.open("/messages.txt","a");
      //f.println(request);
      digitalWrite(MESH_INTR,HIGH);
      Serial.println(request);
      Serial.flush();
      digitalWrite(MESH_INTR,LOW);

      //String msg = f.readString();
      //f.close();
    }    
  }
  else{
        digitalWrite(MESH_INTR,HIGH);
        Serial.println(request);
        Serial.flush();
        digitalWrite(MESH_INTR,LOW);
  }
      return (ackID);
}

/**
   Callback used to decide which networks to connect to once a WiFi scan has been completed.

   @param numberOfNetworks The number of networks found in the WiFi scan.
   @param meshInstance The ESP8266WiFiMesh instance that called the function.
*/
void networkFilter(int numberOfNetworks, ESP8266WiFiMesh &meshInstance) {
  for (int i = 0; i < numberOfNetworks; ++i) {
    String currentSSID = WiFi.SSID(i);
    int meshNameIndex = currentSSID.indexOf(meshInstance.getMeshName());

    /* Connect to any _suitable_ APs which contain meshInstance.getMeshName() */
    if (meshNameIndex >= 0) {
      uint64_t targetNodeID = ESP8266WiFiMesh::stringToUint64(currentSSID.substring(meshNameIndex + meshInstance.getMeshName().length()));

      if (targetNodeID < ESP8266WiFiMesh::stringToUint64(meshInstance.getNodeID())) {
        ESP8266WiFiMesh::connectionQueue.push_back(NetworkInfo(i));
      }
    }
  }
}

/**
   Callback for when you get a response from other nodes

   @param response The response string received from another node in the mesh
   @param meshInstance The ESP8266WiFiMesh instance that called the function.
   @returns The status code resulting from the response, as an int
*/
transmission_status_t manageResponse(const String &response, ESP8266WiFiMesh &meshInstance) {
transmission_status_t statusCode = TS_TRANSMISSION_COMPLETE;
  if(!response.indexOf("[ACK]")>=0){
      if(!sList.indexOf(response)>=0){
        sList += " "; 
        sList += response; 
        
        //File f = SPIFFS.open("/messages.txt","a");
        //f.println(response);
        digitalWrite(MESH_INTR,HIGH);
        Serial.println(response);
        Serial.flush();
        digitalWrite(MESH_INTR,LOW);
        //f.close(); 
     }
  }
  else{
        digitalWrite(MESH_INTR,HIGH);
        Serial.println(response);
        Serial.flush();
        digitalWrite(MESH_INTR,LOW);
  }
  if(decodedmsg.length()>0){
    meshInstance.setMessage(decodedmsg);
    decodedmsg = "";
  }

  // Our last request got a response, so time to create a new request.
  //meshInstance.setMessage("Hello world request #" + String(++requestNumber) + " from " + meshInstance.getMeshName() + meshInstance.getNodeID() + ".");

  // (void)meshInstance; // This is useful to remove a "unused parameter" compiler warning. Does nothing else.
  return statusCode;
}

void setup() {
  // Prevents the flash memory from being worn out, see: https://github.com/esp8266/Arduino/issues/1054 .
  // This will however delay node WiFi start-up by about 700 ms. The delay is 900 ms if we otherwise would have stored the WiFi network we want to connect to.
  WiFi.persistent(false);

  Serial.begin(115200);
  SPIFFS.begin();

  pinMode(MESH_INTR,OUTPUT);
  pinMode(MESH_INTACK,INPUT);
  digitalWrite(MESH_INTR,LOW);
  attachInterrupt(digitalPinToInterrupt(MESH_INTACK),ServeWeb,RISING);
  
  delay(50); // Wait for Serial.

  /* Initialise the mesh node */
  meshNode.begin();
  meshNode.activateAP(); // Each AP requires a separate server port.
  meshNode.setStaticIP(IPAddress(192, 168, 4, 22)); // Activate static IP mode to speed up connection times.
}

int32_t timeOfLastScan = -10000;
void loop() {

  /*
   * Interrupt request from the Web Server ESP
   * Service by storing the message via serial to flash memory via SPIFFs
   * Reset the interrupt flag afterwards
   */
   if(INTR == true){
      //File f = SPIFFS.open("/messages.txt","a");
      decodedmsg = Serial.readString();
      //f.println(decodedmsg);
      //f.close();
      INTR = false; //reset the INTR flag
    }

    //File f = SPIFFS.open("/messages.txt","a");
    //String msg = f.readString();
    //f.close();
  
  if ((millis() - timeOfLastScan > 4000 || (WiFi.status() != WL_CONNECTED && millis() - timeOfLastScan > 2000))&& decodedmsg.length()>0){ // Scan for networks with two second intervals when not already connected.
    //SET TO TRUE SO WE DISCONNECT TO THE SENT NODE
    meshNode.attemptTransmission(decodedmsg, true);
    timeOfLastScan = millis();

  } else {
    /* Accept any incoming connections */
    meshNode.acceptRequest();
  }
}
