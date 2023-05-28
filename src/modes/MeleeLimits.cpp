#include "modes/MeleeLimits.hpp"

#define ANALOG_STICK_MIN 48
#define ANALOG_STICK_NEUTRAL 128
#define ANALOG_STICK_MAX 208

#define TRAVELTIME 6//ms

void limitOutputs(OutputState outputArray[], const uint8_t currentIndex, const uint8_t length,
                  const uint16_t sampleSpacing,
                  OutputState &rawOutput,
                  OutputState &finalOutput) {
    //copy state into our buffer
    outputArray[currentIndex].a               = rawOutput.a;
    outputArray[currentIndex].b               = rawOutput.b;
    outputArray[currentIndex].x               = rawOutput.x;
    outputArray[currentIndex].y               = rawOutput.y;
    outputArray[currentIndex].buttonR         = rawOutput.buttonR;//z button
    outputArray[currentIndex].triggerLDigital = rawOutput.triggerLDigital;
    outputArray[currentIndex].triggerRDigital = rawOutput.triggerRDigital;
    outputArray[currentIndex].start           = rawOutput.start;
    outputArray[currentIndex].dpadUp          = rawOutput.dpadUp;
    outputArray[currentIndex].dpadDown        = rawOutput.dpadDown;
    outputArray[currentIndex].dpadLeft        = rawOutput.dpadLeft;
    outputArray[currentIndex].dpadRight       = rawOutput.dpadRight;
    outputArray[currentIndex].leftStickX      = rawOutput.leftStickX;
    outputArray[currentIndex].leftStickY      = rawOutput.leftStickY;
    outputArray[currentIndex].rightStickX     = rawOutput.rightStickX;
    outputArray[currentIndex].rightStickY     = rawOutput.rightStickY;
    outputArray[currentIndex].triggerLAnalog  = rawOutput.triggerLAnalog;
    outputArray[currentIndex].triggerRAnalog  = rawOutput.triggerRAnalog;

    //compute the travel time stuff
}
