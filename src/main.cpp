// La passerelle
#define T2_MESSAGE_MAX_DATA_LEN 15
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include "SSD1306.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <ThingSpeak.h>
#include <T2Message.h>
#include <stdio.h>
#include <string.h>

////// CONFIGURATION DE VOTRE passerelle
#define MYNETID 2
#define MYID 1
char ApiKey[3][17] = {{"T00AC0W12TVLHBRW"},{"1FCA7LI0S6FQX9HM"}, {"I4N7MPJ8MMDU0JCH"}};
int channelNumber[3] = {};
#define WIFISSID "Free H+"
#define WIFIPASS "kfmc2143"
/////////////////////////////////////////

// Pin definetion of WIFI LoRa 32
#define SCK 5      // GPIO5  -- SX1278's SCK
#define MISO 19    // GPIO19 -- SX1278's MISO
#define MOSI 27    // GPIO27 -- SX1278's MOSI
#define SS 18      // GPIO18 -- SX1278's CS
#define RST 14     // GPIO14 -- SX1278's RESET
#define DI0 26     // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND 868E6 //you can set band here directly,e.g. 868E6,915E6
#define PABOOST true

uint8_t radioBuf[(T2_MESSAGE_HEADERS_LEN + T2_MESSAGE_MAX_DATA_LEN)];
T2Message myMsg;

//// Structure pour la gestion des Channels
#define MAXCHANNEL 3
struct Channel
{
  unsigned long ChannelNumber;
  char WriteAPIKey[20];
  char name[10];
  int Field[9];
};
struct Channel channels[MAXCHANNEL];
/// GESTION DES ID - Notre DHCP
#define MAXID 30
int AdresseNodeID[MAXID];

// Partie WiFi
WiFiMulti WiFiMulti;
WiFiClient clientTS;
// Partie ECRAN
SSD1306 display(0x3c, 4, 15);

/// GESTION DES CHANNELS
//////////////////////////////
int initChannels()
{
  Serial.println("Init Channels");
  for (int i = 0; i < 12; i++)
  {
    channels[i].ChannelNumber = channelNumber[i];
    strcpy(channels[i].WriteAPIKey, ApiKey[i]);
    sprintf(channels[i].name, "%s%d", "Test", i);
    for (int j = 0; j < 9; j++)
      channels[i].Field[j] = 1;
    channels[i].Field[1] = 0;
    Serial.printf("%i - %i - %s\n", i, channels[i].ChannelNumber, channels[i].WriteAPIKey);
  }
  return 1;
}

int GiveChannel(int NodeID)
{
  // Q9 ...

  return 0;
}

int GiveWriteAPIKey(int NodeID, char *buf)
{
  for (int i = 0; i < MAXCHANNEL; i++)
    for (int j = 1; j < 9; j++)
      if (channels[i].Field[j] == NodeID)
      {
        strcpy(buf, channels[i].WriteAPIKey);
        return OK;
      }
  return 0;
}
/// GESTION DES ID
//////////////////////////////
int ValidID(int NodeID)
{
  if (AdresseNodeID[NodeID] != 0)
    return 1;
  else
    return 0;
}

int GiveID(int serailnumber)
{
  // Q6 a completer - Notre DHCP
  // utilisez AdresseNodeID
  for (size_t i = 0; i < MAXID; i++)
  {
    if (AdresseNodeID[i] == 0)
      AdresseNodeID[i] = serailnumber;
      return i;
  }
  return -1;
}

int sendLORA(int idx, int src, int dst, int sdx, int cmd, const char *data, int len)
{
  uint8_t radioBufLen = 0;
  myMsg.idx = idx;
  myMsg.src = src;
  myMsg.dst = dst;
  myMsg.sdx = sdx;
  myMsg.cmd = cmd;
  myMsg.len = len;
  memcpy(myMsg.data, data, len);
  myMsg.getSerializedMessage(radioBuf, &radioBufLen);
  myMsg.printMessage();
  LoRa.beginPacket();
  LoRa.write(radioBuf, radioBufLen);
  LoRa.endPacket();
  return 1;
}

char buf[10]; // Buffeur utilisé pour l'envois de la trame
int len;      // longueur de la trame

int sendtheIDToTheNode(int serialnumber)
{
  int id = GiveID(serialnumber);
  char* payload;
  sprintf(payload, "%i:%i:%i", id, MYNETID, serialnumber);
  if (ValidID(id)) {
    sendLORA(MYNETID, MYID, 0, 1, 1, payload, strlen(payload));  
  }
  //Q7
}

int sendTheChannelAndFieldToTheNode(int NodeID)
{
  //Q10
}

int sendValueToThingSpeak(int NodeID, float Value)
{
  //Q13
  return OK;
}

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ; //test this program,you must connect this board to a computer
  // INIT DISPLAY
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW); // set GPIO16 low to reset OLED
  delay(500);
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high
  // INIT LORA
  Serial.println("LoRa Receiver");
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(BAND))
  {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }
  else
  {
    Serial.println("LoRa started");
  }
  // INIT WiFi
  WiFiMulti.addAP(WIFISSID, WIFIPASS);
  Serial.print("Wait for WiFi... ");
  while (WiFiMulti.run() != WL_CONNECTED)
  {
    delay(500);
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  display.drawString(0, 10, "WiFi OK");
  for (int i = 0; i < MAXID; i++)
    AdresseNodeID[i] = 0;     // INIT DHCP
  initChannels();             // INIT CHANEL
  ThingSpeak.begin(clientTS); // INIT ThingSpeak
}

char *array[10];
int arrayint[10];
// cette fonction analyse le payload de la forme 12;34;56
// découpe la chaine et enregistre les données entières dans arrayint
// arrayint[0]=12 ,  arrayint[1]=34, arrayint[2]=56
int parseString(char *s)
{
  int i = 0;
  char *p = strtok(s, ";");
  while (p != NULL)
  {
    array[i++] = p;
    p = strtok(NULL, ";");
  }
  while (i < 10)
    array[i++] = NULL;
  for (i = 0; i < 10; ++i)
  {
    if (array[i] != NULL)
    {
      arrayint[i] = atoi(array[i]);
    }
  }
  return 0;
}

char message[255]; //buffeur de reception d'un message
uint8_t lmes;      //longeur du message recu
// Recoit le message LoRa et inscrit le résultat dans
// l'objet T2Message myMsg.
int receivLoRa()
{
  int lmes = 0;
  int packetSize = LoRa.parsePacket();
  if (packetSize)
  {
    Serial.println("Received packet '");
    while ((LoRa.available()) && (lmes < 250))
    {
      message[lmes++] = (char)LoRa.read();
    }
    message[lmes++] = 0;
    myMsg.setSerializedMessage((uint8_t *)message, lmes);
    if (myMsg.idx == MYNETID)
      myMsg.printMessage();
    return 1;
  }
  return -1;
}

uint8_t idx = MYNETID;
uint8_t dst = 1;
uint8_t src = 0;
uint8_t sdx = 1;
uint8_t cmd = 0;

boolean table_display = false;

void loop()
{
  // Avez nous recu un message ?
  if (receivLoRa() == 1)
  {
    Serial.print("Received packet '");
    display.clear();

    if ((myMsg.idx == idx))
    {
      Serial.print('LOL');
      delay(5000);
    }
    // Q5 définissez le filtrage
    if ((myMsg.idx == idx) && (myMsg.dst == dst) && (myMsg.src == src) && (myMsg.sdx = sdx))
    {
      Serial.print("Envois d'un ID au numero de serie : ");
      parseString((char *)myMsg.data);
      Serial.println(arrayint[0]);
      sendtheIDToTheNode(arrayint[0]);
      display.drawString(0, 10, "Send ID");
      display.drawString(80, 10, (char *)myMsg.data);
    }

    // Q5 définissez le filtrage
    if ((myMsg.idx == idx) && (myMsg.dst == dst) && (myMsg.sdx = sdx) && (myMsg.cmd == cmd))
    {
      Serial.print("Envois du numero channel et field a :");
      Serial.println(myMsg.src, HEX);
      sendTheChannelAndFieldToTheNode(myMsg.src);
      display.drawString(0, 10, "Send Channel");
    }

    // Q5 définissez le filtrage
    if ((myMsg.idx == idx) && (myMsg.dst == dst) && (myMsg.sdx = sdx) && (myMsg.cmd == cmd))
    {
      char buf[5];
      Serial.print("Envois a ThingSpeak de : ");
      Serial.print(myMsg.src, HEX);
      Serial.print(" Valeur  :");
      parseString((char *)myMsg.data);
      Serial.println(arrayint[0]);
      sendValueToThingSpeak(myMsg.src, arrayint[0]);
      display.drawString(0, 10, itoa(myMsg.src, buf, 10));
      display.drawString(20, 10, "Send Value : ");
      display.drawString(80, 10, (char *)myMsg.data);
    }

    if (table_display)
    {
      //// Table de routage affichée à l'écran
      Serial.print("///////////////////////// ROUTAGE Sous réseau : ");
      Serial.print(MYNETID);
      Serial.println(" /////////////////////////");
      for (int i = 0; i < MAXCHANNEL; i++)
      {
        Serial.print("  | Tab[");
        Serial.print(i);
        Serial.print("]");
        Serial.print(" - ID : ");
        Serial.print(channels[i].Field[1]);
        Serial.print(" - Serial Numb : ");
        Serial.print(AdresseNodeID[channels[i].Field[1]]);
        Serial.print(" - Channel : ");
        Serial.print(channels[i].ChannelNumber);
        Serial.print(" - Nom : ");
        Serial.println(channels[i].name);
      }
      Serial.println("///////////////////////// FIN ROUTAGE /////////////////////////");
      table_display = true;
    }
  }

  display.display();
}
