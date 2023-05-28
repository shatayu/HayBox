#ifndef _MODES_MELEELIMITS_HPP
#define _MODES_MELEELIMITS_HPP

#include "core/socd.hpp"
#include "core/state.hpp"

//override the socd assigned, just for melee configurations
#define MELEE_SOCD socd::SOCD_NEUTRAL

void limitOutputs(OutputState outputArray[], const uint8_t index, const uint8_t length,
                 const uint16_t sampleSpacing,
                 const OutputState &rawOutput,
                 OutputState &finalOutput);

#endif
