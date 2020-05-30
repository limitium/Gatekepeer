#include <Arduino.h>
#include "Gate.h"
#include "MQTTWIFI.h"
#include <Log4Esp.h>

Logger Log = Logger("Gatekeeper");

int RING_SWITCH = D1;
int DOOR_SWITCH = D2;
int LINE_IN = D3;

Gate::Gate gate = Gate::Gate(LINE_IN, RING_SWITCH, DOOR_SWITCH);
MQTTWIFI::MQTTWIFI mqttwifi = MQTTWIFI::MQTTWIFI("wintermute", "123qweasdzxc", "broker.mqttdashboard.com", 1883);

void setup()
{
  unsigned int serialSpeed = 9600;
  Serial.begin(serialSpeed);
  while (!Serial)
  {
    ;
  }
  delay(3000);
  // Log.verbose(F("\n\nRun serial at %d\n"), serialSpeed);

  gate.init();
  mqttwifi.init();

  mqttwifi.subscribe("lim/gate/cmd", [=](const char *payload) {
    Log.verbose("cmd:%s on state:%s", payload, gate.getState());
    if (std::strcmp(payload, "answer") == 0)
    {
      delay(1000);
      gate.answer();
      delay(1000);
    }
    if (std::strcmp(payload, "open") == 0)
    {
      gate.open();
      delay(1000);
    }
    if (std::strcmp(payload, "close") == 0)
    {
      gate.close();
    }
  });
  gate.onStateChange([=](Gate::State prevState, Gate::State curState) {
    Log.verbose("state:%s", gate.getState());
    mqttwifi.publish("lim/gate/state", gate.getState());

    //automate
    if (curState == Gate::Ring)
    {
      mqttwifi.publish("lim/gate/cmd", "answer");
    }
    if (curState == Gate::Talk)
    {
      mqttwifi.publish("lim/gate/cmd", "open");
    }
  });
}

unsigned long lastPublish = 0;

void loop()
{
  gate.loop();
  mqttwifi.loop();

  unsigned long currentTime = millis();
  if (currentTime - lastPublish >= 30 * 1 * 1000)
  {
    lastPublish = currentTime;
    mqttwifi.publish("lim/gate/hb", gate.getState());
  }
}
