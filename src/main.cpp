#include <Arduino.h>
#include <Stepper.h>

#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"

#include <FS.h> //this needs to be first, or it all crashes and burns...
#include "SPIFFS.h"

#include <ESP32Time.h>


ESP32Time rtc(3600);

DNSServer dnsServer;
AsyncWebServer server(80);

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    //request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
     request->send(SPIFFS, "/index.html", "text/html");
  }
};


// Code inspiré de la vidéo de Playful Technology
// https://www.youtube.com/watch?v=_yhyu2qyVAw


// I'm using BKA30D-R5 dual stepper module, designed for vehicle instrument panels
// It has a bipolar stepper, wich is wired to as follows:
// "o" is the location of the spindle
//   ________________
//  / 13 12      26 25 \
// |    o              |
//  \_27_14______33_32_/


bool calibrated = false; // variable indiquant si l'aiguille est calibrée


#define NUM_STEPS 720
#define STEPS_PER_ANGLE 2

const int stepsPerRevolution = 720;

Stepper hourStepper(stepsPerRevolution, GPIO_NUM_26,  GPIO_NUM_25, GPIO_NUM_33, GPIO_NUM_32);
Stepper minuteStepper(stepsPerRevolution, GPIO_NUM_13, GPIO_NUM_12, GPIO_NUM_27, GPIO_NUM_14);

int currentHourAngle = 0;
int currentMinuteAngle = 0;

int stepsBetweenAngle(int startAngle, int destAngle){
  // Si l'angle à atteindre est inférieur à l'angle actuel, il faut aller jusqu'a 360° puis jusqu'a l'angle à demandé
  if(destAngle < startAngle){
      return ((360-startAngle)*STEPS_PER_ANGLE) + (destAngle*STEPS_PER_ANGLE);
  }else{
      return (destAngle-startAngle)*STEPS_PER_ANGLE;
  }
}


void showTime(long hours, long minutes){
  Serial.print("Setting time to ");
  Serial.print(hours);
  Serial.print(":");
  Serial.println(minutes);


  // On s'occupe des heures
  // ancienne version avec log de Fred qui commence par des grands pas
  //int hourAngle = (((TWO_PI*(log(hours+1)/log(25))-HALF_PI)* 180/PI )+90);
  int hourAngle = (((TWO_PI*( 1-(log(25 - hours)/log(25)) )-HALF_PI)* 180/PI )+90);

  int hourStepsToDo = stepsBetweenAngle(currentHourAngle ,hourAngle);
  hourStepper.step(hourStepsToDo);
  currentHourAngle = hourAngle;


  // On s'occupe des minutes
  
  // ancienne version avec log de Fred qui commence par des grands pas
  //int minuteAngle = (((TWO_PI*(log(minutes+1)/log(61))-HALF_PI)* 180/PI )+90);
  int minuteAngle = (((TWO_PI*(  1-(log(61-minutes)/log(61)) )-HALF_PI)* 180/PI )+90);

  int minuteStepsToDo = stepsBetweenAngle(currentMinuteAngle ,minuteAngle);
  minuteStepper.step(minuteStepsToDo);
  currentMinuteAngle = minuteAngle;



}

void disableWiFi(){
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}


void setup() {
  Serial.begin(115200);  

  if(!SPIFFS.begin()){
    Serial.println("Erreur SPIFFS...");
    return;
  }

  WiFi.softAP("horlog' 🕒");
  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/w3.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/w3.css", "text/css");
  });

  server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/app.js", "text/javascript");
  });


  // URL utilisée pour la calibration des aiguilles
  server.on("/moveNeedle", HTTP_GET, [](AsyncWebServerRequest *request){
    String needle;
    int steps;
    if (request->hasParam("needle")) {
      needle = request->getParam("needle")->value();
    }
    if (request->hasParam("steps")) {
      steps = request->getParam("steps")->value().toInt();
    }

    if (needle == "hour") {
      hourStepper.step(steps);
      currentHourAngle = 0;
    } else if (needle == "minute") {
      minuteStepper.step(steps);
      currentMinuteAngle = 0;
    }

    request->send(200, "application/json", "{\"message\":\"ok\"}");

  });

  // URL utilisée pour tester la poisiton des aiguilles pour une heure donnée
  server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request){
    int h;
    int m;
    if (request->hasParam("h")) {
      h = request->getParam("h")->value().toInt();
    }
    if (request->hasParam("m")) {
      m = request->getParam("m")->value().toInt();
    }
    

    showTime(h, m);    



    request->send(200, "application/json", "{\"message\":\"ok\"}");

  });

  // URL qui permet de régler l'heure
  // Il faut lui passer un timestamp (t) en paramètre ainsi qu'on offset (o) en minutes ( fuseau horaire + heure d'été/hiver)
  server.on("/setTime", HTTP_GET, [](AsyncWebServerRequest *request){
    int timestamp;
    int offset;

    if (request->hasParam("t")) {
      timestamp = request->getParam("t")->value().toInt();
    }

    if (request->hasParam("o")) {
      offset = request->getParam("o")->value().toInt();
    }



    request->send(200, "application/json", "{\"message\":\"ok\"}");

    // On déconnecte le wifi pour eviter des problèmes de consommation
    disableWiFi();
    delay(500);


    // Puis on initialise l'horloge
    currentHourAngle = 0;
    currentMinuteAngle = 0;
    rtc.offset = offset * -60;
    rtc.setTime(timestamp);
    calibrated = true;
  });

  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
  server.begin();


  minuteStepper.setSpeed(40);
  hourStepper.setSpeed(40);
}

void loop() {
  dnsServer.processNextRequest();
  if(calibrated){
    showTime(rtc.getHour(true), rtc.getMinute());    
    delay(200);
  }
}