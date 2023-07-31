#ifndef _MODES_MELEELIMITS_HPP
#define _MODES_MELEELIMITS_HPP

#include "core/socd.hpp"
#include "core/state.hpp"

//override the socd assigned, just for melee configurations
#define MELEE_SOCD socd::SOCD_NEUTRAL
//#define MELEE_SOCD socd::SOCD_2IP_NO_REAC

void limitOutputs(const uint16_t sampleSpacing,
                  const InputState &inputs,
                  const OutputState &rawOutput,
                  OutputState &finalOutput);

#endif
