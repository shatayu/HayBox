#include "modes/MeleeLimits.hpp"

#define HISTORYLEN 5//changes in target stick position

#define ANALOG_STICK_MIN 48
#define ANALOG_DEAD_MIN (128-22)/*this is in the deadzone*/
#define ANALOG_STICK_NEUTRAL 128
#define ANALOG_DEAD_MAX (128+22)/*this is in the deadzone*/
#define ANALOG_STICK_MAX 208
#define ANALOG_CROUCH (128-50)/*this y coordinate will hold a crouch*/
#define ANALOG_DASH_LEFT (128-64)/*this x coordinate will dash left*/
#define ANALOG_DASH_RIGHT (128+64)/*this x coordinate will dash right*/
#define MELEE_RIM_RAD2 6185/*if x^2+y^2 >= this, it's on the rim*/

#define TRAVELTIME_EASY 6//ms
#define TRAVELTIME_CROSS 12//ms to cross gate; unused
#define TRAVELTIME_INTERNAL 12//ms for "easy" to "internal"; 2/3 frame
#define TRAVELTIME_SLOW (4*16)//ms for tap SDI nerfing, 4 frames

#define TIMELIMIT_DOWNUP (16*3*250)//units of 4us; how long after a crouch to upward input should it begin a jump?
#define JUMP_TIME (16*2*250)//units of 4us; after a recent crouch to upward input, always hold full up for 2 frames

#define TIMELIMIT_FRAME (16*250)//units of 4us; 1 frame, for reference
#define TIMELIMIT_DEBOUNCE (6*250)//units of 4us; 6ms;
#define TIMELIMIT_SIMUL (2*250)//units of 4us; 3ms: if the latest inputs are less than 2 ms apart then don't nerf cardiag

#define TIMELIMIT_DASH (16*15*250)//units of 4us; last dash time prior to a pivot input; 15 frames
#define TIMELIMIT_PIVOT (24*250)//units of 4us; any longer than 1.5 frames is not likely to be a pivot

#define TIMELIMIT_QCIRC (16*6*250)//units of 4us; 6 frames

#define TIMELIMIT_TAP (16*6*250)//units of 4us; 6 frames
#define TIMELIMIT_TAP_PLUS 36000//(16*9*250)//units of 4us; 9 frames ...the expression overflows on arduino for some reason

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
} shortstate;

//for sdi nerfs, we want to record only movement between sdi zones, ignoring movement within zones
typedef struct {
    uint16_t timestamp;//in samples
    uint8_t zone;
    bool stale;
} sdizonestate;

//for crouch uptilt, we want to record only crouch
typedef struct {
    uint16_t timestamp;
    int8_t downup;
} downupstate;

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
uint8_t sdiZone(const uint8_t x, const uint8_t y) {
    uint8_t result = 0b0000'0000;
    if(x >= ANALOG_DEAD_MIN && x <= ANALOG_DEAD_MAX) {
        if(y < ANALOG_DASH_LEFT) {
            result = result | BITS_D;
        } else if(y > ANALOG_DASH_RIGHT) {
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

//the output will have the following bits set:
//0b0000'0100 for left
//0b0000'1000 for right
//no bits set for neutral
//thresholds are dash for the x-axis
uint8_t pivotZone(const uint8_t x) {
    uint8_t result = 0b0000'0000;
    if(x <= ANALOG_DASH_LEFT) {
        result = result | BITS_L;
    } else if(x >= ANALOG_DASH_RIGHT) {
        result = result | BITS_R;
    }
    return result;
}

//the output will have the following bits set:
//0b0000'0001 for up
//0b0000'0010 for down
//no bits set for neutral
//thresholds are uptilt for up and stay in crouch for down
uint8_t crouchUptiltZone(const uint8_t y) {
    uint8_t result = 0b0000'0000;
    if(y > ANALOG_DEAD_MIN) {
        result = result | BITS_U;
    } else if(y <= ANALOG_CROUCH) {
        result = result | BITS_D;
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

uint8_t isTapSDI(const sdizonestate zoneHistory[HISTORYLEN],
                 const uint8_t currentIndex,
                 const bool currentTime,
                 const uint16_t sampleSpacing) {
    uint8_t output = 0;

    //grab the last five zones
    const uint8_t historyLength = min(5, HISTORYLEN);
    uint8_t zoneList[historyLength];
    uint16_t timeList[historyLength];
    bool staleList[historyLength];
    for(int i = 0; i < historyLength; i++) {
        const uint8_t index = lookback(currentIndex, i);
        zoneList[i] = zoneHistory[index].zone;
        timeList[i] = zoneHistory[index].timestamp;
        staleList[i] = zoneHistory[index].stale;
    }
    const uint8_t popCur = popcount_zone(zoneList[0]);
    const uint8_t popOne = popcount_zone(zoneList[1]);

    //detect repeated center-cardinal sequences, or repeated cardinal-diagonal sequences
    // if we're changing zones back and forth
    if(zoneList[0] != zoneList[1] && (zoneList[0] == zoneList[2]) && (zoneList[1] == zoneList[3])) {
        //check the time duration
        const uint16_t timeDiff0 = (currentTime - timeList[2])*sampleSpacing;//make sure things aren't reliant on long-past inputs
        const uint16_t timeDiff1 = (timeList[0] - timeList[2])*sampleSpacing;//rising edge to rising edge, or falling edge to falling edge
        //const uint16_t timeDiff2 = (timeList[0] - timeList[1])*sampleSpacing;//rising to falling, or falling to rising
        //We want to nerf it if there is more than one press every 6 frames, but not if the previous press or release duration is less than 1 frame
        if(!staleList[3] && (timeDiff0 < TIMELIMIT_TAP_PLUS && timeDiff1 < TIMELIMIT_TAP && timeDiff0 > TIMELIMIT_DEBOUNCE)) {
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
    //where the the diagonals are the same
    //the cardinals don't have to be the same in case they're inputting 2365 etc

    //simpler: if the last 5 inputs are in the origin, one cardinal, and one diagonal
    //and that there was a recent return to center
    //at least one of each zone, and at least two diagonals
    uint8_t diagZone = 0b1111'1111;
    uint8_t origCount = 0;
    uint8_t cardCount = 0;
    uint8_t diagCount = 0;
    for(int i = 0; i < historyLength; i++) {
        const uint8_t popcnt = popcount_zone(zoneList[i]);
        if(popcnt == 0) {
            origCount++;
        } else if(popcnt == 1) {
            cardCount++;
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
    const bool shortTime = ((timeList[0] - timeList[4])*sampleSpacing < TIMELIMIT_CARDIAG) &&
                           ((timeList[0] - timeList[1])*sampleSpacing > TIMELIMIT_SIMUL) &&
                           !staleList[4];

    //if it hit only one cardinal
    //             if only the same diagonal was pressed
    //                          if the origin, cardinal, and diagonal were all entered
    //                                                                 if there were either two cardinals or two origins (to prevent dash-diagonal-mod from triggering this)
    //                                                                                                     within the time limit
    if(diagMatch && origCount && cardCount && diagCount > 1 && shortTime) {
        output = output | BITS_SDI_TAP_CRDG;
    }

    //return the last cardinal in the zone list before the last diagonal, useful for SDI diagonal nerfs.
    bool lookNow = false;
    bool alreadyWritten = false;
    for(int i = 0; i < historyLength; i++) {
        if((popcount_zone(zoneList[i]) == 2) && !alreadyWritten) {
            lookNow = true;
        }
        if((popcount_zone(zoneList[i]) == 1) && !alreadyWritten && lookNow) {
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
    if(samplesElapsed > 5*16*2) {//5 frames * 16 ms * max 2 samples per frame
        oldChange = true;
    }
    const uint16_t timeElapsed = samplesElapsed*sampleSpacing;//units of 4 us
    const uint16_t travelTimeElapsed = timeElapsed/msTravel;//250 times the fraction of the travel time elapsed

    //For the following 256s, they used to be 250, but AVR had division issues
    // and would only be able to reach a value of 6 when returning to neutral,
    // from the right only.
    //Switching to 256 fixed it somehow, at the expense of 2.4% greater travel time.
    const uint16_t cappedTT = min(256, travelTimeElapsed);

    const int16_t dX = ((destX-startX)*cappedTT)/256;
    const int16_t dY = ((destY-startY)*cappedTT)/256;

    const uint16_t newX = startX+dX;
    const uint16_t newY = startY+dY;

    outX = (uint8_t) newX;
    outY = (uint8_t) newY;

    if(oldChange || msTravel == 0) {
        outX = destX;
        outY = destY;
    }
}

void limitOutputs(const uint16_t sampleSpacing,//in units of 4us
                  const InputState &inputs,
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
    //static shortstate cHistory[HISTORYLEN];
    static sdizonestate sdiZoneHist[HISTORYLEN];

    static bool initialized = false;
    if(!initialized) {
        for(int i = 0; i<HISTORYLEN; i++) {
            aHistory[i].timestamp = 0;
            aHistory[i].tt = 6;
            aHistory[i].x = ANALOG_STICK_NEUTRAL;
            aHistory[i].y = ANALOG_STICK_NEUTRAL;
            aHistory[i].x_start = ANALOG_STICK_NEUTRAL;
            aHistory[i].y_start = ANALOG_STICK_NEUTRAL;
            aHistory[i].x_end = ANALOG_STICK_NEUTRAL;
            aHistory[i].y_end = ANALOG_STICK_NEUTRAL;
            /*
            cHistory[i].timestamp = 0;
            cHistory[i].tt = 6;
            cHistory[i].x = ANALOG_STICK_NEUTRAL;
            cHistory[i].y = ANALOG_STICK_NEUTRAL;
            cHistory[i].x_start = ANALOG_STICK_NEUTRAL;
            cHistory[i].y_start = ANALOG_STICK_NEUTRAL;
            cHistory[i].x_end = ANALOG_STICK_NEUTRAL;
            cHistory[i].y_end = ANALOG_STICK_NEUTRAL;
            */
            sdiZoneHist[i].timestamp = 0;
            sdiZoneHist[i].zone = 0;
            sdiZoneHist[i].stale = true;
        }
        initialized = true;
    }
    static uint8_t currentIndexA = 0;
    static uint8_t currentIndexC = 0;
    static uint8_t currentIndexSDI = 0;

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
    /*
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
    */

    //If we're doing a diagonal airdodge, make travel time instant to prevent inconsistent wavedash angles
    //this only fully works for neutral socd right now
    //this is a catchall to force the latest output
    //however, we do want to make it be overridden by the sdi nerfs so this happens first
    if((inputs.left != inputs.right) && (inputs.down != inputs.up) && inputs.down && (inputs.r || inputs.l)) {
        prelimAX = rawOutputIn.leftStickX;
        prelimAY = rawOutputIn.leftStickY;
    }

    //if it's a pivot downtilt coordinate, lock out A //TODO

    //if it's a pivot uptilt coordinate, make Y jump //TODO

    //if it's a fast crouch to upward coordinate, make Y jump even if a tilt was desired
    if(aHistory[currentIndexA].y_start < ANALOG_CROUCH &&//started out in a crouch
       aHistory[currentIndexA].y > ANALOG_DEAD_MAX &&//wanting to go out of the deadzone
       (aHistory[currentIndexA].timestamp-aHistory[lookback(currentIndexA,1)].timestamp)*sampleSpacing < TIMELIMIT_DOWNUP &&//the upward input occurred < 3 frames after the previous stick movement
       (currentTime-aHistory[currentIndexA].timestamp)*sampleSpacing < JUMP_TIME) {//it's been less than 2 frames since the stick was moved up (this sets the duration of the stick jump input)
           prelimAY = 255;
    }
    //handle neutral SOCD for the crouch uptilt nerf
    const uint8_t prevIndexA = lookback(currentIndexA, 1);
    if(aHistory[prevIndexA].y_start < ANALOG_CROUCH &&//started out in a crouch
       aHistory[currentIndexA].y > ANALOG_DEAD_MAX &&//wanting to go out of the deadzone
       (aHistory[currentIndexA].timestamp-aHistory[prevIndexA].timestamp)*sampleSpacing < TIMELIMIT_DOWNUP &&//the upward input occurred < 3 frames after the previous stick movement
       (currentTime-aHistory[currentIndexA].timestamp)*sampleSpacing < JUMP_TIME) {//it's been less than 2 frames since the stick was moved up (this sets the duration of the stick jump input)
           prelimAY = 255;
    }

    //if it's wank sdi (TODO) or diagonal tap SDI, lock out the cross axis
    const uint8_t sdi = isTapSDI(sdiZoneHist, currentIndexA, currentTime, sampleSpacing);
    static bool wankNerf = false;
    if(sdi & (BITS_SDI_TAP_DIAG | BITS_SDI_TAP_CRDG)){
        if(sdi & (BITS_L | BITS_R)) {
            //lock the cross axis
            prelimAY = ANALOG_STICK_NEUTRAL;
            //make sure future cross axis travel time begins at the origin
            aHistory[currentIndexA].y_end = prelimAY;
            //prevent the cardinal axis from dipping below the sdi threshold by preserving its coordinate
            prelimAX = aHistory[currentIndexA].x_start;
            //make sure that future cardinal travel time begins where it was before
            aHistory[currentIndexA].x_end = prelimAX;
            wankNerf = true;
        } else if(sdi & (BITS_U | BITS_D)) {
            //lock the cross axis
            prelimAX = ANALOG_STICK_NEUTRAL;
            //make sure future cross axis travel time begins at the origin
            aHistory[currentIndexA].x_end = prelimAX;
            //prevent the cardinal axis from dipping below the sdi threshold by preserving its coordinate
            prelimAY = aHistory[currentIndexA].y_start;
            //make sure that future cardinal travel time begins where it was before
            aHistory[currentIndexA].y_end = prelimAY;
            wankNerf = true;
        }//one or the other should occur
        /*
        //debug to see if SDI was detected
        if(sdi & BITS_SDI_TAP_DIAG) {
            prelimCX = 200;
        }
        if(sdi & BITS_SDI_TAP_CRDG) {
            prelimCY = 200;
        }
        */
    } else if(wankNerf) {
        aHistory[currentIndexA].x_end = aHistory[currentIndexA].x;
        aHistory[currentIndexA].y_end = aHistory[currentIndexA].y;
    }

    //==================================recording history======================================//

    //if we are in a new SDI zone, record the new zone
    if(sdiZoneHist[currentIndexSDI].zone != sdiZone(rawOutputIn.leftStickX, rawOutputIn.leftStickY)) {
        currentIndexSDI = (currentIndexSDI + 1) % HISTORYLEN;

        sdiZoneHist[currentIndexSDI].timestamp = currentTime;
        sdiZoneHist[currentIndexSDI].zone = sdiZone(rawOutputIn.leftStickX, rawOutputIn.leftStickY);
        sdiZoneHist[currentIndexSDI].stale = false;
    }
    for(int i = 0; i < HISTORYLEN; i++) {
        if(currentTime-sdiZoneHist[i].timestamp > 8*16*2) {//8 frames * 16 ms * max 2 samples
            sdiZoneHist[i].stale = true;
        }
    }

    //if we have a new coordinate, record the new info, the travel time'd locked out stick coordinate, and set travel time
    if(aHistory[currentIndexA].x != rawOutputIn.leftStickX || aHistory[currentIndexA].y != rawOutputIn.leftStickY) {
        oldA = false;
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

        uint8_t prelimTT = TRAVELTIME_EASY;
        //if cardinal tap SDI
        if(sdi & BITS_SDI_TAP_CARD) {
            prelimTT = max(prelimTT, TRAVELTIME_SLOW);
        }
        //if the destination is not an "easy" coordinate
        if(!isEasy(xIn, yIn)) {
            prelimTT = max(prelimTT, TRAVELTIME_INTERNAL);
        }
        /*
        //if the destination is on the opposite side from the current prelim coord
        if((xIn < ANALOG_STICK_NEUTRAL && ANALOG_STICK_NEUTRAL < prelimAX) ||
           (yIn < ANALOG_STICK_NEUTRAL && ANALOG_STICK_NEUTRAL < prelimAY) ||
           (xIn > ANALOG_STICK_NEUTRAL && ANALOG_STICK_NEUTRAL > prelimAX) ||
           (yIn > ANALOG_STICK_NEUTRAL && ANALOG_STICK_NEUTRAL > prelimAY)) {
            prelimTT = max(prelimTT, TRAVELTIME_CROSS);
        }
        */
        //If we're doing a diagonal airdodge, make travel time instant to prevent inconsistent wavedash angles
        //this only fully works for neutral socd right now
        //it doesn't fully work either, so I'm going to put a catchall in the outer loop
        if((inputs.left != inputs.right) && (inputs.down != inputs.up) && inputs.down && (inputs.r || inputs.l)) {
            prelimTT = 0;
        }
        aHistory[currentIndexA].tt = prelimTT;
    }
    /*
    if(cHistory[currentIndexC].x != rawOutputIn.rightStickX || cHistory[currentIndexC].y != rawOutputIn.rightStickY) {
        oldC = false;
        currentIndexC = (currentIndexC + 1) % HISTORYLEN;

        const uint8_t xIn = rawOutputIn.rightStickX;
        const uint8_t yIn = rawOutputIn.rightStickY;

        cHistory[currentIndexC].timestamp = currentTime;
        cHistory[currentIndexC].x = xIn;
        cHistory[currentIndexC].y = yIn;
        cHistory[currentIndexC].x_end = xIn;
        cHistory[currentIndexC].y_end = yIn;
        cHistory[currentIndexC].x_start = prelimCX;
        cHistory[currentIndexC].y_start = prelimCY;

        uint8_t prelimTT = TRAVELTIME_EASY;
        //if the destination is not an "easy" coordinate
        if(!isEasy(xIn, yIn)) {
            prelimTT = max(prelimTT, TRAVELTIME_INTERNAL);
        }
        //if the destination is on the opposite side from the current prelim coord
        //if((xIn < ANALOG_STICK_NEUTRAL && ANALOG_STICK_NEUTRAL < prelimCX) ||
        //   (yIn < ANALOG_STICK_NEUTRAL && ANALOG_STICK_NEUTRAL < prelimCY) ||
        //   (xIn > ANALOG_STICK_NEUTRAL && ANALOG_STICK_NEUTRAL > prelimCX) ||
        //   (yIn > ANALOG_STICK_NEUTRAL && ANALOG_STICK_NEUTRAL > prelimCY)) {
        //    prelimTT = max(prelimTT, TRAVELTIME_CROSS);
        //}
        cHistory[currentIndexC].tt = prelimTT;
    }
    */

    //===============================applying the nerfed coords=================================//
    finalOutput.a               = rawOutputIn.a;//TODO prelimAButton
    finalOutput.b               = rawOutputIn.b;
    finalOutput.x               = rawOutputIn.x;
    finalOutput.y               = rawOutputIn.y;
    finalOutput.buttonL         = rawOutputIn.buttonL;
    finalOutput.buttonR         = rawOutputIn.buttonR;
    finalOutput.triggerLDigital = rawOutputIn.triggerLDigital;
    finalOutput.triggerRDigital = rawOutputIn.triggerRDigital;
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
    //finalOutput.rightStickX     = prelimCX;
    //finalOutput.rightStickY     = prelimCY;
    finalOutput.rightStickX     = rawOutputIn.rightStickX;
    finalOutput.rightStickY     = rawOutputIn.rightStickY;
    finalOutput.triggerLAnalog  = rawOutputIn.triggerLAnalog;
    finalOutput.triggerRAnalog  = rawOutputIn.triggerRAnalog;
}
