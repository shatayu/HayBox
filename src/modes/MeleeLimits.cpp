#include "modes/MeleeLimits.hpp"

#define ANALOG_STICK_MIN 48
#define ANALOG_DEAD_MIN (128-22)/*this is in the deadzone*/
#define ANALOG_STICK_NEUTRAL 128
#define ANALOG_DEAD_MAX (128+22)/*this is in the deadzone*/
#define ANALOG_STICK_MAX 208
#define ANALOG_STICK_CROUCH (128-50)/*this y coordinate will hold a crouch*/
#define ANALOG_DASH_LEFT (128-64)/*this x coordinate will dash left*/
#define ANALOG_DASH_RIGHT (128+64)/*this x coordinate will dash right*/
#define MELEE_RIM_RAD2 6185/*if x^2+y^2 >= this, it's on the rim*/

#define TRAVELTIME 6//ms
#define CROSSTIME 12//ms to cross gate
#define LONGTIME 16//ms for "easy" to "internal", or for detected SDI
#define SLOWTIME 4*16//ms for SDI nerfing

#define DASHTIME (16*15)//ms; last dash time prior to a pivot input; 15 frames
#define PIVOTTIME 24//ms; any pivot

#define QCIRCTIME (16*6)//ms; time between first direction

#define HISTORYLEN 32//changes in target stick position

typedef struct {
    uint16_t timestamp;//in samples
    uint8_t tt;//travel time used in ms
    uint8_t ax;
    uint8_t ay;
    uint8_t cx;
    uint8_t cy;
    bool apress;
} shortstate;

bool isEasy(const uint8_t x, const uint8_t y) {
    //is it in the deadzone?
    if(x >= ANALOG_DEAD_MIN && x <= ANALOG_DEAD_MAX && y >= ANALOG_DEAD_MIN && y <= ANALOG_DEAD_MAX) {
        return true;
    } else {
        //is it on the rim?
        const uint8_t xnorm = (x > ANALOG_STICK_NEUTRAL ? (x-ANALOG_STICK_NEUTRAL) : (ANALOG_STICK_NEUTRAL-x))
        const uint8_t ynorm = (y > ANALOG_STICK_NEUTRAL ? (y-ANALOG_STICK_NEUTRAL) : (ANALOG_STICK_NEUTRAL-y))
        const uint16_t xsquared = xnorm*xnorm;
        const uint16_t ysquared = ynorm*ynorm;
        if((xsquared+ysquared) >= MELEE_RIM_RAD2) {
            return true;
        } else {
            return false;
        }
    }
}

uint8_t lookback(const uint8_t currentIndex,
                 const uint8_t samplesBack) {
    if(samplesBack > currentIndex) {
        return (HISTORYLEN-samplesBack) + currentIndex;
    } else {
        return currentIndex - samplesBack;
    }
}

bool isWankSDI(const shortstate coordHistory[HISTORYLEN],
               const uint8_t currentIndex,
               const uint16_t currentIter,
               const uint16_t sampleSpacing) {
    //detect quarter circling, whether it's from cardinal->diagonal-> opposite diagonal within n frames of first cardinal
    //or wank, where it's diagonal diagonal diagonal within n frames (same n frames?)
    //or rapid tapping, where the stick goes from neutral to the same two quadrants within n frames
}

void limitOutputs(const uint16_t sampleSpacing,//in units of 4us
                  OutputState &rawOutputIn,
                  OutputState &finalOutput) {
    //First, we want to check if the raw output has changed.
    //If it has changed, then we need to store it with a timestamp in our buffer.
    //Also check whether it's an "easy" coordinate or not (rim+origin = easy)
    //Then, we need to parse the buffer and detect whether:
    //  an empty pivot has likely occurred (0.5 to 1.5 frames?) ============ this depends on our travel time implementation unless we store the analog value history too
    //    how?
    //    (make you do a 2f jump input if you input an upward tilt coordinate within 10 frames)
    //    (lock out the A press if you input a downward tilt coordinate for 3 frames)
    //  the input is crouching
    //    (make you do a 2f jump input if you input an upward tilt coordinate within 4 frames)
    //  rapid quarter circling (wank di)
    //    lock out second diagonal (opposite axis). May not be important with neutral SOCD.
    //  rapid tapping (tap di)
    //    Temporarily increase travel time to... 16 ms? 24ms?
    //  rapid eighth-circling (cardinal diagonal neutral repeat)
    //    Increase travel time on cardinal and lock out later diagonals.

    static uint16_t currentIter = 0;
    currentIter++;

    static shortstate previousRawOutput[HISTORYLEN];
    //copy state into our buffer
    previousRawOutput.a               = rawOutputIn.a;
    previousRawOutput.b               = rawOutputIn.b;
    previousRawOutput.x               = rawOutputIn.x;
    previousRawOutput.y               = rawOutputIn.y;
    previousRawOutput.buttonR         = rawOutputIn.buttonR;//z button
    previousRawOutput.triggerLDigital = rawOutputIn.triggerLDigital;
    previousRawOutput.triggerRDigital = rawOutputIn.triggerRDigital;
    previousRawOutput.triggerLAnalog  = rawOutputIn.triggerLAnalog;
    previousRawOutput.triggerRAnalog  = rawOutputIn.triggerRAnalog;
    previousRawOutput.start           = rawOutputIn.start;
    previousRawOutput.dpadUp          = rawOutputIn.dpadUp;
    previousRawOutput.dpadDown        = rawOutputIn.dpadDown;
    previousRawOutput.dpadLeft        = rawOutputIn.dpadLeft;
    previousRawOutput.dpadRight       = rawOutputIn.dpadRight;
    previousRawOutput.leftStickX      = rawOutputIn.leftStickX;
    previousRawOutput.leftStickY      = rawOutputIn.leftStickY;
    previousRawOutput.rightStickX     = rawOutputIn.rightStickX;
    previousRawOutput.rightStickY     = rawOutputIn.rightStickY;

    //Compute the nerf stuff


    const uint16_t lookback_ms = 8000;
    uint16_t lookback_samples = lookback_ms / (sampleSpacing*4);
    uint16_t lookbackIndex;
    if(lookback_ms % (sampleSpacing*4) > 0) {//round up
        lookback_samples++;
    }
    if(lookback_samples > currentIndex) {
        lookbackIndex = (HISTORYLEN-lookback_samples) + currentIndex;//must keep order of operations to avoid underflow
    } else {
        lookbackIndex = currentIndex - lookback_samples
    }
    //read out old stick axes
    finalOutput.leftStickX      = outputArray[lookbackIndex].leftStickX;
    finalOutput.leftStickY      = outputArray[lookbackIndex].leftStickY;
    finalOutput.rightStickX     = outputArray[lookbackIndex].rightStickX;
    finalOutput.rightStickY     = outputArray[lookbackIndex].rightStickY;
    //read out current buttons
    finalOutput.a               = rawOutputIn.a;
    finalOutput.b               = rawOutputIn.b;
    finalOutput.x               = rawOutputIn.x;
    finalOutput.y               = rawOutputIn.y;
    finalOutput.buttonR         = rawOutputIn.buttonR;//z button
    finalOutput.triggerLDigital = rawOutputIn.triggerLDigital;
    finalOutput.triggerRDigital = rawOutputIn.triggerRDigital;
    finalOutput.triggerLAnalog  = rawOutputIn.triggerLAnalog;
    finalOutput.triggerRAnalog  = rawOutputIn.triggerRAnalog;
    finalOutput.start           = rawOutputIn.start;
    finalOutput.dpadUp          = rawOutputIn.dpadUp;
    finalOutput.dpadDown        = rawOutputIn.dpadDown;
    finalOutput.dpadLeft        = rawOutputIn.dpadLeft;
    finalOutput.dpadRight       = rawOutputIn.dpadRight;
}
