#include "comms/GamecubeBackend.hpp"

#include "modes/MeleeLimits.hpp"

#include "core/ControllerMode.hpp"
#include "core/InputSource.hpp"

#include <Nintendo.h>

void zeroTcnt1() {
    //Disable interrupts
    noInterrupts();
    //Set TCNT1
    TCNT1 = 0;
    //Restore interrupts
    interrupts();
}

uint16_t readTcnt1() {
    //Disable interrupts
    noInterrupts();
    //Set TCNT1
    uint16_t i;
    i = TCNT1;
    //Restore interrupts
    interrupts();
    return i;
}

GamecubeBackend::GamecubeBackend(
    InputSource **input_sources,
    size_t input_source_count,
    int polling_rate,
    int data_pin
)
    : CommunicationBackend(input_sources, input_source_count) {
    _gamecube = new CGamecubeConsole(data_pin);
    _data = defaultGamecubeData;

    if (polling_rate > 0) {
        // Delay used between input updates to postpone them until right before the
        // next poll, while also leaving time (850us) for processing to finish.
        _delay = 0;//(1000000 / polling_rate) - 850;
        _nerfOn = true;
    } else {
        // If polling rate is set to 0 (A was held), disable ~~the delay~~ the nerfs.
        _delay = 0;
        _nerfOn = false;
    }

    //check button hold for disabling nerf
    Scaninputs();

    //Serial.begin(115200);
    //Serial.println("Testing serial output");
    //debug output for timing experiments
    pinMode(21, OUTPUT);

    //Set up hardware timer
    TCCR1A = 0;
    TCCR1B = (1 << CS11) | (1 << CS10);//64 clock divider -> 4 microsecond resolution
    zeroTcnt1();
}

GamecubeBackend::~GamecubeBackend() {
    delete _gamecube;
}

void GamecubeBackend::SendReport() {
    static bool status = false;//true = normal poll
    static uint32_t minLoop = 16384;
    static uint loopCount = 0;
    static bool detect = true;//false means run
    static uint sampleCount = 1;
    static uint sampleSpacing = 0;
    static uint16_t loopTime = 0;
    const uint fastestLoop = 950; //fastest possible loop; platform-dependent
    //const uint fastestLoop = 450; //fastest possible loop; platform-dependent

    //read how long this loop took
    loopTime = readTcnt1();//each unit is 4 microseconds
    //zero the counter so that it doesn't overflow
    zeroTcnt1();

    if(detect && status) {
        //run loop time detection procedure
        //digitalWrite(21, loopCount%2 == 0 ? HIGH : LOW);
        digitalWrite(21, HIGH);
        loopCount++;
        if(loopCount > 5 && loopTime > 300/4) {//screen out implausibly fast samples; the limit is 2500 Hz (400 us) so we use 300 us as the limit, ignore first few polls
            minLoop = min(minLoop, loopTime);
        }
        if(loopCount >= 100) {
            detect = false;
            sampleCount = 1;
            while(1000/4*sampleCount <= minLoop) {//we want [sampleCount] ms-spaced samples within the smallest possible loop
                sampleCount++;
            }
            sampleSpacing = minLoop / sampleCount;
            if(sampleSpacing < (fastestLoop/4) && sampleCount > 1) {
                sampleCount--;
                sampleSpacing = minLoop / sampleCount;
            }
            /*
            Serial.print("Loop time: ");
            Serial.println(loopTime);
            Serial.print("Fastest Loop: ");
            Serial.println(minLoop);
            Serial.print("Sample count: ");
            Serial.println(sampleCount);
            Serial.print("Sample spacing: ");
            Serial.println(sampleSpacing);
            */
        }
    } else {
        if(loopTime > minLoop+(minLoop >> 1)) {//if the loop time is 50% longer than expected
            /*
            Serial.println("Loop time too long?");
            Serial.print("Loop time: ");
            Serial.println(loopTime);
            Serial.print("TCNT1: ");
            Serial.println(readTcnt1());
            zeroTcnt1();
            Serial.print("TCNT1: ");
            Serial.println(readTcnt1());
            */
            detect = true;//stop scanning inputs briefly and re-measure timings
            loopCount = 0;
        }
        //run the delay procedure based on samplespacing
        //in the stock arduino software, it samples 850 us after the end of the poll response
        //we want the last sample to begin [850 + extra computation time] before the beginning of the last poll to give room for the sample and the travel time+nerf computation
        //
        for (uint i = 0; i < sampleCount; i++) {
            //digitalWrite(21, i%2 == 0 ? HIGH : LOW);
            digitalWrite(21, LOW);

            const uint16_t nerfTime = 0/4;
            const uint16_t computationTime = 700/4 + nerfTime*_nerfOn;//depends on the platform; 4us steps.
            //700 microseconds is sufficient with no travel time computation
            const uint16_t targetTime = ((i+1)*sampleSpacing)-computationTime;
            //const uint16_t targetTime = i*sampleSpacing;
            int count = 0;
            while(readTcnt1() < targetTime) {
                count++;//do something?
                //spinlock
            }

            ScanInputs();

            // Run gamemode logic.
            UpdateOutputs();

            if(_nerfOn) {
                //APPLY NERFS HERE
            } else {
                // Digital outputs
                _data.report.a = _outputs.a;
                _data.report.b = _outputs.b;
                _data.report.x = _outputs.x;
                _data.report.y = _outputs.y;
                _data.report.z = _outputs.buttonR;
                _data.report.l = _outputs.triggerLDigital;
                _data.report.r = _outputs.triggerRDigital;
                _data.report.start = _outputs.start;
                _data.report.dleft = _outputs.dpadLeft | _outputs.select;
                _data.report.dright = _outputs.dpadRight | _outputs.home;
                _data.report.ddown = _outputs.dpadDown;
                _data.report.dup = _outputs.dpadUp;

                // Analog outputs
                _data.report.xAxis = _outputs.leftStickX;
                _data.report.yAxis = _outputs.leftStickY;
                _data.report.cxAxis = _outputs.rightStickX;
                _data.report.cyAxis = _outputs.rightStickY;
                _data.report.left = _outputs.triggerLAnalog + 31;
                _data.report.right = _outputs.triggerRAnalog + 31;
            }

            digitalWrite(21, count ? HIGH : LOW);
        }
    }

    digitalWrite(21, HIGH);
    // Send outputs to console.
    status = _gamecube->write(_data);
    digitalWrite(21, LOW);
}
