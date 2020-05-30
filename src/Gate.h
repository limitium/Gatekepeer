#include <Arduino.h>
#include <functional>
#include <Log4Esp.h>

namespace Gate
{
Logger log = Logger("Gate");
#define ON_STATUS_CHANGE_CALLBACK_SIGNATURE std::function<void(State, State)> onStateChangeCallBack

enum State
{
    Wait,
    Ring,
    Talk,
    Open
};
static const char *stateStrings[] = {"wait", "ring", "talk", "open"};

volatile boolean ringDetected = false;
// Checks if ring was detected
ICACHE_RAM_ATTR void detectsRing()
{
    ringDetected = true;
}

class Gate
{
public:
    Gate() {}
    Gate(uint8_t lineIn, uint8_t ringSwitch, uint8_t doorSwitch)
    {
        this->lineIn = lineIn;
        this->ringSwitch = ringSwitch;
        this->doorSwitch = doorSwitch;
    }
    void init()
    {
        pinMode(this->ringSwitch, OUTPUT);
        pinMode(this->doorSwitch, OUTPUT);

        pinMode(this->lineIn, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(this->lineIn), detectsRing, FALLING);

        close();
    }
    void loop()
    {
        unsigned long now = millis();

        if (state == Ring && ((now - lastStateUpdate) > 4000))
        {
            log.verbose("Close after ring");
            close();
        }
        if (state == Open && ((now - lastStateUpdate) > 1000))
        {
            log.verbose("Close after open");
            close();
        }
        if (state != Wait && ((now - lastStateUpdate) > 60000))
        {
            log.verbose("Close long");
            close();
        }

        if (ringDetected)
        {
            ringDetected = false;
            ring();
        }
    }
    void answer()
    {
        if (state == Ring)
        {
            setState(Talk);
            digitalWrite(this->ringSwitch, LOW);
        }
    }
    void open()
    {
        if (state == Talk)
        {
            setState(Open);
            digitalWrite(this->doorSwitch, LOW);
        }
    }
    void close()
    {
        setState(Wait);
        digitalWrite(this->ringSwitch, HIGH);
        digitalWrite(this->doorSwitch, HIGH);
    }
    void onStateChange(ON_STATUS_CHANGE_CALLBACK_SIGNATURE)
    {
        this->onStateChangeCallBack = onStateChangeCallBack;
    }
    const char *getState()
    {
        return stateStrings[state];
    }

private:
    State state;
    unsigned long lastStateUpdate = 0;

    uint8_t lineIn;
    uint8_t ringSwitch;
    uint8_t doorSwitch;

    ON_STATUS_CHANGE_CALLBACK_SIGNATURE;

    void setState(State state)
    {
        State prevState = this->state;
        this->state = state;
        this->lastStateUpdate = millis();
        if (onStateChangeCallBack != nullptr)
        {
            onStateChangeCallBack(prevState, state);
        }
    }
    void ring()
    {
        if (state == Wait)
        {
            setState(Ring);
        }
        if (state == Ring)
        {
            lastStateUpdate = millis();
        }
    }
};
} // namespace Gate
