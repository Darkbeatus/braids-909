// Copyright 2012 Olivier Gillet.
//
// Author: Olivier Gillet (pichenettes@mutable-instruments.net)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Oscillator - digital style waveforms.

#ifndef BRAIDS_DIGITAL_OSCILLATOR_H_
#define BRAIDS_DIGITAL_OSCILLATOR_H_

#include "stmlib/stmlib.h"

#include <cstring>

namespace braids {

enum DigitalOscillatorShape {
  OSC_SHAPE_TRIPLE_RING_MOD,
  OSC_SHAPE_SAW_SWARM,
  OSC_SHAPE_COMB_FILTER,
  OSC_SHAPE_TOY,

  OSC_SHAPE_DIGITAL_FILTER_LP,
  OSC_SHAPE_DIGITAL_FILTER_PK,
  OSC_SHAPE_DIGITAL_FILTER_BP,
  OSC_SHAPE_DIGITAL_FILTER_HP,

  OSC_SHAPE_VOSIM,
  OSC_SHAPE_VOWEL,
  OSC_SHAPE_VOWEL_FOF,

  OSC_SHAPE_HARMONICS,

  OSC_SHAPE_FM,
  OSC_SHAPE_FEEDBACK_FM,
  OSC_SHAPE_CHAOTIC_FEEDBACK_FM,

  OSC_SHAPE_PLUCKED,
  OSC_SHAPE_BOWED,
  OSC_SHAPE_BLOWN,
  OSC_SHAPE_FLUTED,
  OSC_SHAPE_STRUCK_BELL,
  OSC_SHAPE_STRUCK_DRUM,
  OSC_SHAPE_KICK,
  OSC_SHAPE_CYMBAL,
  OSC_SHAPE_SNARE,

  OSC_SHAPE_WAVETABLES,
  OSC_SHAPE_WAVE_MAP,
  OSC_SHAPE_WAVE_LINE,
  OSC_SHAPE_WAVE_PARAPHONIC,

  OSC_SHAPE_FILTERED_NOISE,
  OSC_SHAPE_TWIN_PEAKS_NOISE,
  OSC_SHAPE_CLOCKED_NOISE,
  OSC_SHAPE_GRANULAR_CLOUD,
  OSC_SHAPE_PARTICLE_NOISE,

  OSC_SHAPE_DIGITAL_MODULATION,
  OSC_SHAPE_QUESTION_MARK,
  OSC_SHAPE_LAST
};

struct ResoFilterState {
  int32_t pole[4];
};

struct KarplusStrongState {
  size_t ptr;
  bool chunk;
};

struct PluckState {
  size_t ptr;
  size_t initialization_ptr;
  uint32_t space;
};

struct StrikeState {
  uint32_t phase[4];
  int32_t previous_sample[4];
  int32_t amplitude[4];
};

struct BowedState {
  uint32_t delay_ptr;
  int32_t lp_state;
  int32_t excitation;
};

struct BlownState {
  uint32_t delay_ptr;
  int32_t lp_state;
  int32_t hp_state;
};

struct FlutedState {
  uint32_t jet_delay_ptr;
  uint32_t bore_delay_ptr;
  int32_t lp_state;
  int32_t dc_blocking_x0;
  int32_t dc_blocking_y0;
};

struct ToyState {
  uint8_t held_sample;
  uint16_t decimation_counter;
};

struct Grain {
  uint32_t phase;
  uint32_t phase_increment;
  uint32_t envelope_phase;
  uint32_t envelope_phase_increment;
};

struct Particle {
  float amplitude;
  float state_0;
  float state_1;
  float coeff_0;
  float coeff_1;
  float gain;
};

struct DigitalModulationState {
  uint32_t symbol_phase;
  uint16_t symbol_count;
  uint32_t data_byte;
};

union DigitalOscillatorState {
  ResoFilterState res;
  KarplusStrongState ks;
  PluckState plk;
  StrikeState str;
  BowedState bow;
  BlownState blw;
  FlutedState flu;
  ToyState toy;
  Grain grain[4];
  Particle particle[6];
  DigitalModulationState mod;
  int32_t integrator_state;
  int16_t saw_8_phase[8];
};

class DigitalOscillator {
 public:
  typedef void (DigitalOscillator::*RenderFn)(
      const uint8_t*,
      int16_t*,
      size_t);

  DigitalOscillator() { }
  ~DigitalOscillator() { }

  inline void Init() {
    memset(&state_, 0, sizeof(state_));
    memset(delay_line_, 0, sizeof(delay_line_));
    memset(excitation_filter_, 0, sizeof(excitation_filter_));
    strike_ = false;
    previous_shape_ = OSC_SHAPE_LAST;
    phase_ = 0;
  }

  inline void set_shape(DigitalOscillatorShape shape) {
    shape_ = shape;
  }

  inline void set_pitch(int16_t pitch) {
    // Smooth the pitch to avoid stepping artifacts.
    pitch_ = (7 * pitch_ + pitch) >> 3;
  }

  inline void set_parameters(
      int16_t parameter_1,
      int16_t parameter_2) {
    parameter_[0] = parameter_1;
    parameter_[1] = parameter_2;
  }

  inline void Strike() {
    strike_ = true;
  }

  void Render(const uint8_t* sync, int16_t* buffer, size_t size);

  void Render909PureRumble(
      const uint8_t* sync,
      int16_t* buffer,
      size_t size);

 private:
  void RenderTripleRingMod(const uint8_t*, int16_t*, size_t);
  void RenderSawSwarm(const uint8_t*, int16_t*, size_t);
  void RenderCombFilter(const uint8_t*, int16_t*, size_t);
  void RenderToy(const uint8_t*, int16_t*, size_t);
  void RenderDigitalFilter(const uint8_t*, int16_t*, size_t);
  void RenderVosim(const uint8_t*, int16_t*, size_t);
  void RenderVowel(const uint8_t*, int16_t*, size_t);
  void RenderVowelFof(const uint8_t*, int16_t*, size_t);
  void RenderHarmonics(const uint8_t*, int16_t*, size_t);
  void RenderFm(const uint8_t*, int16_t*, size_t);
  void RenderFeedbackFm(const uint8_t*, int16_t*, size_t);
  void RenderChaoticFeedbackFm(const uint8_t*, int16_t*, size_t);
  void RenderPlucked(const uint8_t*, int16_t*, size_t);
  void RenderBowed(const uint8_t*, int16_t*, size_t);
  void RenderBlown(const uint8_t*, int16_t*, size_t);
  void RenderFluted(const uint8_t*, int16_t*, size_t);
  void RenderStruckBell(const uint8_t*, int16_t*, size_t);
  void RenderStruckDrum(const uint8_t*, int16_t*, size_t);
  void RenderKick(const uint8_t*, int16_t*, size_t);
  void RenderCymbal(const uint8_t*, int16_t*, size_t);
  void RenderSnare(const uint8_t*, int16_t*, size_t);
  void RenderWavetables(const uint8_t*, int16_t*, size_t);
  void RenderWave
