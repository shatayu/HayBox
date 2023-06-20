#include "modes/MeleeLimits.hpp"

#define HISTORYLEN 16//changes in target stick position

#define ANALOG_STICK_MIN 48
#define ANALOG_DEAD_MIN (128-22)/*this is in the deadzone*/
#define ANALOG_STICK_NEUTRAL 128
#define ANALOG_DEAD_MAX (128+22)/*this is in the deadzone*/
#define ANALOG_STICK_MAX 208
#define ANALOG_STICK_CROUCH (128-50)/*this y coordinate will hold a crouch*/
#define ANALOG_DASH_LEFT (128-64)/*this x coordinate will dash left*/
#define ANALOG_DASH_RIGHT (128+64)/*this x coordinate will dash right*/
#define MELEE_RIM_RAD2 6185/*if x^2+y^2 >= this, it's on the rim*/

#define TRAVELTIME_EASY 6//ms
#define TRAVELTIME_CROSS 12//ms to cross gate
#define TRAVELTIME_INTERNAL 16//ms for "easy" to "internal"; 1 frame
#define TRAVELTIME_SLOW (4*16)//ms for tap SDI nerfing, 4 frames

#define TIMELIMIT_DOWNUP (16*3*250)//units of 4us; how long after a crouch to upward input should it begin a jump?
#define JUMP_TIME (16*2*250)//units of 4us; after a recent crouch to upward input, always hold full up for 2 frames

#define TIMELIMIT_DASH (16*15*250)//units of 4us; last dash time prior to a pivot input; 15 frames
#define TIMELIMIT_PIVOT (24*250)//units of 4us; any longer than 1.5 frames is not likely to be a pivot

#define TIMELIMIT_QCIRC (16*6*250)//units of 4us; 6 frames

#define TIMELIMIT_TAP (16*6*250)//units of 4us; 6 frames

#define TIMELIMIT_CARDIAG (16*8*250)//units of 4us; 8 frames

#define BITS_DIR 0b0000'1111
#define BITS_U   0b0000'0001
#define BITS_D   0b0000'0010
#define BITS_L   0b0000'0100
#define BITS_R   0b0000'1000

#define BITS_SDI      0b1111'0000
#define BITS_SDI_WANK     0b0001'0000
#define BITS_SDI_TAP_CARD 0b0010'0000
#define BITS_SDI_TAP_DIAG 0b0100'0000
#define BITS_SDI_TAP_CRDG 0b1000'0000

typedef struct {
    uint16_t timestamp;//in samples
    uint8_t tt;//travel time used in ms
    uint8_t x;
    uint8_t y;
    uint8_t x_start;
    uint8_t y_start;
    uint8_t x_end;
    uint8_t y_end;
    uint8_t zone;
    bool easy;
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
//thresholds are dash for cardinals, and deadzone for diagonals
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

uint8_t isWankSDI(const shortstate coordHistory[HISTORYLEN],
               const uint8_t currentIndex,
               const uint16_t curTime,
               const uint16_t sampleSpacing) {
    //detect quarter circling, whether it's from cardinal->diagonal-> opposite diagonal within n frames of first cardinal
    //or wank, where it's diagonal diagonal diagonal within n frames (same n frames?)

    uint8_t isFalseTrue = 0;
    const uint8_t falseBits = 0b0000'0010;
    const uint8_t trueBits = 0b0000'0001;

    //is it in a diagonal zone?
    const uint8_t curZone = coordHistory[currentIndex].zone;

    uint8_t stepsBack = 1;
    if(popcount_zone(curZone) == 2) {
        //in the past TIMELIMIT_QCIRC has it been to an adjacent diagonal ?
        for(; stepsBack < HISTORYLEN; stepsBack++) {
            const uint8_t testIndex = lookback(currentIndex, stepsBack);
            const uint8_t prevZone = coordHistory[testIndex].zone;
            const uint16_t prevTime = coordHistory[testIndex].timestamp;
            if((curTime-prevTime)*sampleSpacing > TIMELIMIT_QCIRC) {
                //it's not sdi if it was slow enough
                //return false;
                if(!isFalseTrue) {
                    isFalseTrue = falseBits;
                }
            } else {
                //it's fast enough
                if(!(prevZone & curZone)) {
                    //it's not wank SDI if it shares no directions with the diagonal
                    //this also weeds out neutral, which is handled separately
                    //return false;
                    if(!isFalseTrue) {
                        isFalseTrue = falseBits;
                    }
                } else {
                    //it shares a direction
                    const uint8_t sharedDirection = prevZone & curZone;
                    if(prevZone != curZone && popcount_zone(prevZone) == 2) {
                        //it was in an adjacent diagonal
                        //now we must check to see if, before that, it was the shared cardinal or the current diagonal, within the same time limit
                        for(; stepsBack < HISTORYLEN; stepsBack++) {
                            const uint8_t testIndex2 = lookback(currentIndex, stepsBack);
                            const uint8_t prevZone2 = coordHistory[testIndex2].zone;
                            const uint16_t prevTime2 = coordHistory[testIndex2].timestamp;
                            if((curTime-prevTime2)*sampleSpacing > TIMELIMIT_QCIRC) {
                                //it's not sdi if it was slow enough
                                //return false;
                                if(!isFalseTrue) {
                                    isFalseTrue = falseBits;
                                }
                            } else {
                                //it's fast enough
                                if(!(prevZone2 & sharedDirection)) {
                                    //it's not wank SDI if the previous direction isn't in the same direction as the two diagonals
                                    //this also weeds out neutral, which is handled separately
                                    //return false;
                                    if(!isFalseTrue) {
                                        isFalseTrue = falseBits;
                                    }
                                } else {
                                    if(prevZone2 != prevZone) {
                                        //it is wank SDI
                                        //return true;
                                        if(!isFalseTrue) {
                                            isFalseTrue = trueBits;
                                        }
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
    //return false
    /*
    if(!isFalseTrue) {
        isFalseTrue = falseBits;
    }
    */
    if(isFalseTrue & trueBits) {
        return BITS_SDI_WANK;
    } else {
        return 0;
    }
}

uint8_t isTapSDI(const shortstate coordHistory[HISTORYLEN],
                 const uint8_t currentIndex,
                 const uint16_t curTime,
                 const uint16_t sampleSpacing) {
    uint8_t output = 0;

    //grab the last five zones
    uint8_t zoneList[5];
    uint16_t timeList[5];
    for(int i = 0; i < 5; i++) {
        const uint8_t index = lookback(currentIndex, i);
        zoneList[i] = coordHistory[index].zone;
        timeList[i] = coordHistory[index].timestamp;
    }
    const uint8_t popCur = popcount_zone(zoneList[0]);
    const uint8_t popOne = popcount_zone(zoneList[1]);

    //detect repeated center-cardinal sequences, or repeated cardinal-diagonal sequences
    // if we're changing zones back and forth
    if(zoneList[0] != zoneList[1] && (zoneList[0] == zoneList[2]) && (zoneList[1] == zoneList[3])) {
        //check the time limit
        if((timeList[0] - timeList[2])*sampleSpacing < TIMELIMIT_TAP) {
            if((zoneList[0] == 0) || (zoneList[1] == 0)) {//if one of the pairs of zones is zero, it's tapping a cardinal
                output = output | BITS_SDI_TAP_CARD;
            } else if(popCur+popOne == 3) { //one is cardinal and the other is diagonal
                output = output | BITS_SDI_TAP_DIAG;
            }
        }
    }
    //detect:
    //         center-cardinal-diagonal-center-cardinal (-diagonal)
    //center-cardinal-diagonal-cardinal-center-cardinal (-diagonal)
    //where the cardinals are the same, and the diagonals are the same

    //simpler: if the last 5 inputs are in the origin, one cardinal, and one diagonal
    //and that there was a recent return to center
    //at least one of each zone, no more than 3 zones total
    uint8_t cardZone = 0b1111'1111;
    uint8_t diagZone = 0b1111'1111;
    uint8_t origCount = 0;
    uint8_t cardCount = 0;
    uint8_t diagCount = 0;
    for(int i = 0; i < 5; i++) {
        const uint8_t popcnt = popcount_zone(zoneList[i]);
        if(popcnt == 0) {
            origCount++;
        } else if(popcnt == 1) {
            cardCount++;
            cardZone = cardZone & zoneList[i];//if two of these don't match, it'll be zero
        } else {
            diagCount++;
            diagZone = diagZone & zoneList[i];//if two of these don't match, it'll have zero or one bits set
        }
    }
    //check the bit count of diagonal matching
    const bool diagMatch = popcount_zone(diagZone) == 2;
    //check whether it returned to center recently
    //const bool recentOrig = (zoneList[1] & zoneList[2]) == 0;//may be too lenient in case people throw in modifier taps?
    //check whether the input was fast enough
    const bool shortTime = (timeList[0] - timeList[4])*sampleSpacing < TIMELIMIT_CARDIAG;
    if(cardZone && diagMatch && origCount && cardCount && diagCount && /*recentOrig &&*/ shortTime) {
        output = output | BITS_SDI_TAP_CRDG;
    }

    //return the last cardinal in the zone list, useful for SDI diagonal nerfs.
    bool alreadyWritten = false;
    for(int i = 0; i < 5; i++) {
        if((popcount_zone(zoneList[i]) == 1) && !alreadyWritten) {
            output = output | zoneList[i];
            alreadyWritten = true;
        }
    }
    return output;
}

void travelTimeCalc(const uint16_t samplesElapsed,
                    const uint16_t sampleSpacing,//units of 4us
                    const uint8_t msTravel,
                    const uint8_t startX,
                    const uint8_t startY,
                    const uint8_t destX,
                    const uint8_t destY,
                    bool &oldChange,//apply tt if false; if the time gets too long, set it to true
                    uint8_t &outX,
                    uint8_t &outY) {
    //check for old data; this prevents overflow from causing issues
    if(samplesElapsed > 15*16*2) {//15 frames * 16 ms * max 2 samples per frame
        oldChange = true;
    }
    if(oldChange) {
        outX = destX;
        outY = destY;
        return;
    }
    const uint16_t timeElapsed = samplesElapsed*sampleSpacing;//units of 4 us
    const uint16_t travelTimeElapsed = timeElapsed/msTravel;//250 times the fraction of the travel time elapsed
    const uint16_t cappedTT = min(250, travelTimeElapsed);

    const int16_t dX = ((destX-startX)*cappedTT)/250;
    const int16_t dY = ((destY-startY)*cappedTT)/250;
    const uint16_t newX = startX+dX;
    const uint16_t newY = startY+dY;
    outX = (uint8_t) newX;
    outY = (uint8_t) newY;
}

void limitOutputs(const uint16_t sampleSpacing,//in units of 4us
                  const OutputState &rawOutputIn,
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

    static bool oldA = true;
    static bool oldC = true;

    static uint16_t currentTime = 0;
    currentTime++;

    static shortstate aHistory[HISTORYLEN];
    static shortstate cHistory[HISTORYLEN];

    static bool initialized = false;
    if(!initialized) {
        for(int i = 0; i<HISTORYLEN; i++) {
            aHistory[i].timestamp = 0;
            aHistory[i].tt = 6;
            aHistory[i].x = ANALOG_STICK_NEUTRAL;
            aHistory[i].y = ANALOG_STICK_NEUTRAL;
            aHistory[i].x_start = ANALOG_STICK_NEUTRAL;
            aHistory[i].y_start = ANALOG_STICK_NEUTRAL;
            aHistory[i].zone = 0;
            aHistory[i].easy = true;
            cHistory[i].timestamp = 0;
            cHistory[i].tt = 6;
            cHistory[i].x = ANALOG_STICK_NEUTRAL;
            cHistory[i].y = ANALOG_STICK_NEUTRAL;
            cHistory[i].x_start = ANALOG_STICK_NEUTRAL;
            cHistory[i].y_start = ANALOG_STICK_NEUTRAL;
            cHistory[i].zone = 0;
            cHistory[i].easy = true;
        }
        initialized = true;
    }
    static uint8_t currentIndexA = 0;
    static uint8_t currentIndexC = 0;

    //calculate travel from the previous step
    uint8_t prelimAX;
    uint8_t prelimAY;
    travelTimeCalc(currentTime-aHistory[currentIndexA].timestamp,
                   sampleSpacing,
                   aHistory[currentIndexA].tt,
                   aHistory[currentIndexA].x_start,
                   aHistory[currentIndexA].y_start,
                   aHistory[currentIndexA].x_end,
                   aHistory[currentIndexA].y_end,
                   oldA,
                   prelimAX,
                   prelimAY);
    uint8_t prelimCX;
    uint8_t prelimCY;
    travelTimeCalc(currentTime-cHistory[currentIndexC].timestamp,
                   sampleSpacing,
                   cHistory[currentIndexC].tt,
                   cHistory[currentIndexC].x_start,
                   cHistory[currentIndexC].y_start,
                   cHistory[currentIndexC].x_end,
                   cHistory[currentIndexC].y_end,
                   oldC,
                   prelimCX,
                   prelimCY);

    //if it's a pivot downtilt coordinate, lock out A //TODO

    //if it's a pivot uptilt coordinate, make Y jump //TODO

    //if it's a crouch to upward coordinate, make Y jump //TODO
    if(aHistory[currentIndexA].y_start < ANALOG_STICK_CROUCH &&//started out in a crouch
       aHistory[currentIndexA].y > ANALOG_DEAD_MAX &&//wanting to go out of the deadzone
       (aHistory[currentIndexA].timestamp-aHistory[lookback(currentIndexA,1)].timestamp) < TIMELIMIT_DOWNUP &&//the upward input occurred < 3 frames after the previous stick movement
       (currentTime-aHistory[currentIndexA].timestamp) < JUMP_TIME) {//it's been less than 2 frames since the stick was moved up (this sets the duration of the stick jump input)
           prelimAY = 255;
    }

    //if it's wank sdi (TODO) or diagonal tap SDI, lock out the cross axis
    const uint8_t sdi = isTapSDI(aHistory, currentIndexA, currentTime, sampleSpacing);
    if(sdi & (BITS_SDI_TAP_DIAG | BITS_SDI_TAP_CRDG)){
        if(sdi & (BITS_L | BITS_R)) {
            //make sure future travel time begins at the origin
            aHistory[currentIndexA].y_end = ANALOG_STICK_NEUTRAL;
            prelimAY = ANALOG_STICK_NEUTRAL;
        } else if(sdi & (BITS_U | BITS_D)) {
            //make sure future travel time begins at the origin
            aHistory[currentIndexA].x_end = ANALOG_STICK_NEUTRAL;
            prelimAX = ANALOG_STICK_NEUTRAL;
        }//one or the other should occur
    }

    //if we have a new coordinate, record the new info, the travel time'd locked out stick coordinate, and set travel time
    if(aHistory[currentIndexA].x != rawOutputIn.leftStickX || aHistory[currentIndexA].y != rawOutputIn.leftStickY) {
        currentIndexA = (currentIndexA + 1) % HISTORYLEN;

        const uint8_t xIn = rawOutputIn.leftStickX;
        const uint8_t yIn = rawOutputIn.leftStickY;

        aHistory[currentIndexA].timestamp = currentTime;
        aHistory[currentIndexA].x = xIn;
        aHistory[currentIndexA].y = yIn;
        aHistory[currentIndexA].x_end = xIn;
        aHistory[currentIndexA].y_end = yIn;
        aHistory[currentIndexA].x_start = prelimAX;
        aHistory[currentIndexA].y_start = prelimAY;
        aHistory[currentIndexA].zone = zone(xIn, yIn);
        aHistory[currentIndexA].easy = isEasy(xIn, yIn);

        uint8_t prelimTT = TRAVELTIME_EASY;
        //if cardinal tap SDI
        if(sdi & BITS_SDI_TAP_CARD) {
            prelimTT = max(prelimTT, TRAVELTIME_SLOW);
        }
        //if the destination is not an "easy" coordinate
        if(!aHistory[currentIndexA].easy) {
            prelimTT = max(prelimTT, TRAVELTIME_INTERNAL);
        }
        //if the destination is on the opposite side from the current prelim coord
        if((xIn < ANALOG_STICK_NEUTRAL && ANALOG_STICK_NEUTRAL < prelimAX) ||
           (yIn < ANALOG_STICK_NEUTRAL && ANALOG_STICK_NEUTRAL < prelimAY) ||
           (xIn > ANALOG_STICK_NEUTRAL && ANALOG_STICK_NEUTRAL > prelimAX) ||
           (yIn > ANALOG_STICK_NEUTRAL && ANALOG_STICK_NEUTRAL > prelimAY)) {
            prelimTT = max(prelimTT, TRAVELTIME_CROSS);
        }
        aHistory[currentIndexA].tt = prelimTT;
    }
    if(cHistory[currentIndexC].x != rawOutputIn.rightStickX || cHistory[currentIndexC].y != rawOutputIn.rightStickY) {
        currentIndexC = (currentIndexC + 1) % HISTORYLEN;

        const uint8_t xIn = rawOutputIn.rightStickX;
        const uint8_t yIn = rawOutputIn.rightStickX;

        cHistory[currentIndexC].timestamp = currentTime;
        cHistory[currentIndexC].x = xIn;
        cHistory[currentIndexC].y = yIn;
        cHistory[currentIndexC].x_end = xIn;
        cHistory[currentIndexC].y_end = yIn;
        cHistory[currentIndexC].x_start = prelimAX;
        cHistory[currentIndexC].y_start = prelimAY;
        cHistory[currentIndexC].zone = zone(xIn, yIn);
        cHistory[currentIndexC].easy = isEasy(xIn, yIn);

        uint8_t prelimTT = TRAVELTIME_EASY;
        //if the destination is not an "easy" coordinate
        if(!cHistory[currentIndexC].easy) {
            prelimTT = max(prelimTT, TRAVELTIME_INTERNAL);
        }
        //if the destination is on the opposite side from the current prelim coord
        if((xIn < ANALOG_STICK_NEUTRAL && ANALOG_STICK_NEUTRAL < prelimCX) ||
           (yIn < ANALOG_STICK_NEUTRAL && ANALOG_STICK_NEUTRAL < prelimCY) ||
           (xIn > ANALOG_STICK_NEUTRAL && ANALOG_STICK_NEUTRAL > prelimCX) ||
           (yIn > ANALOG_STICK_NEUTRAL && ANALOG_STICK_NEUTRAL > prelimCY)) {
            prelimTT = max(prelimTT, TRAVELTIME_CROSS);
        }
        cHistory[currentIndexC].tt = prelimTT;
    }
    finalOutput.a               = rawOutputIn.a;//TODO prelimAButton
    finalOutput.b               = rawOutputIn.b;
    finalOutput.x               = rawOutputIn.x;
    finalOutput.y               = rawOutputIn.y;
    finalOutput.buttonL         = rawOutputIn.buttonL;
    finalOutput.buttonR         = rawOutputIn.buttonR;
    finalOutput.triggerLDigital = rawOutputIn.triggerRDigital;
    finalOutput.start           = rawOutputIn.start;
    finalOutput.select          = rawOutputIn.select;
    finalOutput.home            = rawOutputIn.home;
    finalOutput.dpadUp          = rawOutputIn.dpadUp;
    finalOutput.dpadDown        = rawOutputIn.dpadDown;
    finalOutput.dpadLeft        = rawOutputIn.dpadLeft;
    finalOutput.dpadRight       = rawOutputIn.dpadRight;
    finalOutput.leftStickClick  = rawOutputIn.leftStickClick;
    finalOutput.rightStickClick = rawOutputIn.rightStickClick;
    finalOutput.leftStickX      = prelimAX;
    finalOutput.leftStickY      = prelimAY;
    finalOutput.rightStickX     = prelimCX;
    finalOutput.rightStickY     = prelimCY;
    finalOutput.triggerLAnalog  = rawOutputIn.triggerLAnalog;
    finalOutput.triggerLDigital = rawOutputIn.triggerLDigital;
}
