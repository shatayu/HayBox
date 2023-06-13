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
#define TRAVELTIME_CROSS 12//ms to cross gate
#define TRAVELTIME_LONG 16//ms for "easy" to "internal", or for detected SDI
#define TRAVELTIME_SLOW (4*16)//ms for tap SDI nerfing

#define TIMELIMIT_DASH (16*15*250)//units of 4us; last dash time prior to a pivot input; 15 frames
#define TIMELIMIT_PIVOT (24*250)//units of 4us; any pivot

#define TIMELIMIT_QCIRC (16*6*250)//units of 4us; time between first direction

#define HISTORYLEN 32//changes in target stick position

#define BITS_U 0b0000'0001
#define BITS_D 0b0000'0010
#define BITS_L 0b0000'0100
#define BITS_R 0b0000'1000

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
        const uint8_t xnorm = (x > ANALOG_STICK_NEUTRAL ? (x-ANALOG_STICK_NEUTRAL) : (ANALOG_STICK_NEUTRAL-x));
        const uint8_t ynorm = (y > ANALOG_STICK_NEUTRAL ? (y-ANALOG_STICK_NEUTRAL) : (ANALOG_STICK_NEUTRAL-y));
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

//the output will have the following bits set:
//0b0000'0001 for up
//0b0000'0010 for down
//0b0000'0100 for left
//0b0000'1000 for right
//no bits set for neutral
uint8_t zone(const uint8_t x, const uint8_t y) {
    uint8_t result = 0b0000'0000;
    if(x >= ANALOG_DEAD_MIN && x <= ANALOG_DEAD_MAX) {
        if(y < ANALOG_DEAD_MIN) {
            result = result | BITS_D;
        } else if(y > ANALOG_DEAD_MAX) {
            result = result | BITS_U;
        }
    } else if(x < ANALOG_DEAD_MIN) {
        if(y < ANALOG_DEAD_MIN) {
            result = result | BITS_D | BITS_L;
        } else if(y > ANALOG_DEAD_MAX) {
            result = result | BITS_U | BITS_L;
        } else if(x <= ANALOG_DASH_LEFT) {
            result = result | BITS_L;
        }
    } else /*if(x > ANALOG_DEAD_MAX)*/ {
        if(y < ANALOG_DEAD_MIN) {
            result = result | BITS_D | BITS_R;
        } else if(y > ANALOG_DEAD_MAX) {
            result = result | BITS_U | BITS_R;
        } else if(x >= ANALOG_DASH_RIGHT) {
            result = result | BITS_R;
        }
    }
    return result;
}

//not a general purpose popcount, this is specifically for zones
uint8_t popcount_zone(const uint8_t bitsIn) {
    uint8_t count = 0;
    for(uint8_t i = 0; i < 4; i++) {
        if((bitsIn >> i) & 0b0000'0001) {
            count++;
        }
    }
    return count;
}

bool isWankSDI(const shortstate coordHistory[HISTORYLEN],
               const uint8_t currentIndex,
               const uint16_t curTime,
               const uint16_t sampleSpacing) {
    //detect quarter circling, whether it's from cardinal->diagonal-> opposite diagonal within n frames of first cardinal
    //or wank, where it's diagonal diagonal diagonal within n frames (same n frames?)

    //is it in a diagonal zone?
    const uint8_t curZone = zone(coordHistory[currentIndex].ax, coordHistory[currentIndex].ay);

    uint8_t stepsBack = 1;
    if(popcount_zone(curZone) == 2) {
        //in the past TIMELIMIT_QCIRC has it been to an adjacent diagonal ?
        for(; stepsBack < HISTORYLEN; stepsBack++) {
            const uint8_t testIndex = lookback(currentIndex, stepsBack);
            const uint8_t prevZone = zone(coordHistory[testIndex].ax, coordHistory[testIndex].ay);
            const uint16_t prevTime = coordHistory[testIndex].timestamp;
            if((curTime-prevTime)*sampleSpacing > TIMELIMIT_QCIRC) {
                //it's not sdi if it was slow enough
                return false;
            } else {
                //it's fast enough
                if(!(prevZone & curZone)) {
                    //it's not wank SDI if it shares no directions with the diagonal
                    //this also weeds out neutral, which is handled separately
                    return false;
                } else {
                    //it shares a direction
                    const uint8_t sharedDirection = prevZone & curZone;
                    if(prevZone != curZone && popcount_zone(prevZone) == 2) {
                        //it was in an adjacent diagonal
                        //now we must check to see if, before that, it was the shared cardinal or the current diagonal, within the same time limit
                        for(; stepsBack < HISTORYLEN; stepsBack++) {
                            const uint8_t testIndex2 = lookback(currentIndex, stepsBack);
                            const uint8_t prevZone2 = zone(coordHistory[testIndex2].ax, coordHistory[testIndex2].ay);
                            const uint16_t prevTime2 = coordHistory[testIndex2].timestamp;
                            if((curTime-prevTime2)*sampleSpacing > TIMELIMIT_QCIRC) {
                                //it's not sdi if it was slow enough
                                return false;
                            } else {
                                //it's fast enough
                                if(!(prevZone2 & sharedDirection)) {
                                    //it's not wank SDI if the previous direction isn't in the same direction as the two diagonals
                                    //this also weeds out neutral, which is handled separately
                                    return false;
                                } else {
                                    if(prevZone2 != prevZone) {
                                        //it is wank SDI
                                        return true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    //if it's not currently a diagonal
    //or if it ran out of history somehow
    return false;
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

}
