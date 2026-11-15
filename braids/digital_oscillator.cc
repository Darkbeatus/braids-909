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

#include "braids/digital_oscillator.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "stmlib/dsp/dsp.h"

#include "braids/resources.h"

namespace braids {

using namespace stmlib;

// Static State Variables for 909 Pure-Rumble Engine
static uint32_t phase_kick_ = 0;
static float env_amp_ = 0.0f;
static float env_pitch_ = 0.0f;
static int16_t rumble_buf_[8192] = {0}; 
static uint16_t buf_idx_ = 0;
static uint8_t decimate_cnt_ = 0;
static float lp1_ = 0.0f;
static float lp2_ = 0.0f;
static float rumble_hold_ = 0.0f;

void DigitalOscillator::Render909PureRumble(
    const uint8_t* sync,
    int16_t* buffer,
    size_t size) {
  
  float timbre_param = std::clamp(static_cast<float>(parameter_[0]) / 32767.0f, 0.0f, 1.0f);
  float color_param  = std::clamp(static_cast<float>(parameter_[1]) / 32767.0f, 0.0f, 1.0f);

  for (size_t i = 0; i < size; ++i) {
    if (sync[i]) {
      phase_kick_ = 0;
      env_amp_ = 1.0f;
      env_pitch_ = 1.0f;
    }

    env_pitch_ *= 0.9965f; 
    env_amp_ *= 0.9993f;   

    float base_freq = 44.0f;
    float current_freq = base_freq * (1.0f + (5.0f * timbre_param * env_pitch_ * env_pitch_));
    uint32_t phase_inc = static_cast<uint32_t>(current_freq * 4294967296.0f / 96000.0f);
    phase_kick_ += phase_inc;

    float raw_sine = sinf(static_cast<float>(phase_kick_) * 1.469e-9f);
    float click = (env_pitch_ > 0.85f) ? ((rand() % 2000 - 1000) / 1000.0f) * (env_pitch_ - 0.85f) * 4.0f : 0.0f;
    float kick_sig = (raw_sine * env_amp_) + click;

    decimate_cnt_ = (decimate_cnt_ + 1) & 3;
    if (decimate_cnt_ == 0) {
      uint16_t t1 = (buf_idx_ + 8192 - 2400) & 8191;
      uint16_t t2 = (buf_idx_ + 8192 - 4100) & 8191;
      uint16_t t3 = (buf_idx_ + 8192 - 6200) & 8191;
      uint16_t t4 = (buf_idx_ + 8192 - 7950) & 8191;

      float d1 = static_cast<float>(rumble_buf_[t1]) / 32768.0f;
      float d2 = static_cast<float>(rumble_buf_[t2]) / 32768.0f;
      float d3 = static_cast<float>(rumble_buf_[t3]) / 32768.0f;
      float d4 = static_cast<float>(rumble_buf_[t4]) / 32768.0f;

      float tap_sum = (d1 * 0.4f) + (d2 * 0.6f) + (d3 * 0.8f) + (d4 * 0.95f);
      
      float feedback = 0.50f + (color_param * 0.42f);
      float write_val = (kick_sig * 0.5f) + (tap_sum * feedback);
      
      write_val = std::clamp(write_val, -1.0f, 1.0f);
      rumble_buf_[buf_idx_] = static_cast<int16_t>(write_val * 32767.0f);
      buf_idx_ = (buf_idx_ + 1) & 8191;

      lp1_ += 0.018f * (tap_sum - lp1_);
      lp2_ += 0.018f * (lp1_ - lp2_);
      rumble_hold_ = lp2_;
    }

    float duck_env = 1.0f - (env_amp_ * env_amp_ * 0.95f);
    float rumble_sig = rumble_hold_ * duck_env * (color_param * 3.5f + 0.5f);

    float summed = (kick_sig + rumble_sig) * 1.2f;
    float sq = summed * summed;
    float out_sample = summed * (27.0f + sq) / (27.0f + 9.0f * sq);

    int32_t sample_int = static_cast<int32_t>(out_sample * 28000.0f);
    sample_int = std::clamp(sample_int, -32768, 32767);
    buffer[i] = static_cast<int16_t>(sample_int);
  }
}

void DigitalOscillator::Render(
    const uint8_t* sync,
    int16_t* buffer,
    size_t size) {
  // Hook the custom 909 Pure-Rumble engine to the first oscillator algorithm slot
  if (shape_ == OSC_SHAPE_TRIPLE_RING_MOD) {
    Render909PureRumble(sync, buffer, size);
    return;
  }
  
  // Default dispatch table for remaining factory algorithms
  (this->*fn_table_[shape_])(sync, buffer, size);
}

// (Standard factory method definitions follow below...)
void DigitalOscillator::RenderTripleRingMod(const uint8_t* sync, int16_t* buffer, size_t size) {
  // Fallback stub if shape is called elsewhere
}

void DigitalOscillator::RenderSawSwarm(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderCombFilter(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderToy(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderDigitalFilter(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderVosim(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderVowel(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderVowelFof(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderHarmonics(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderFm(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderFeedbackFm(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderChaoticFeedbackFm(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderPlucked(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderBowed(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderBlown(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderFluted(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderStruckBell(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderStruckDrum(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderKick(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderCymbal(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderSnare(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderWavetables(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderWaveMap(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderWaveLine(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderWaveParaphonic(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderFilteredNoise(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderTwinPeaksNoise(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderClockedNoise(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderGranularCloud(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderParticleNoise(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderDigitalModulation(const uint8_t* sync, int16_t* buffer, size_t size) { }
void DigitalOscillator::RenderQuestionMark(const uint8_t* sync, int16_t* buffer, size_t size) { }

DigitalOscillator::RenderFn DigitalOscillator::fn_table_[] = {
  &DigitalOscillator::RenderTripleRingMod,
  &DigitalOscillator::RenderSawSwarm,
  &DigitalOscillator::RenderCombFilter,
  &DigitalOscillator::RenderToy,
  &DigitalOscillator::RenderDigitalFilter,
  &DigitalOscillator::RenderDigitalFilter,
  &DigitalOscillator::RenderDigitalFilter,
  &DigitalOscillator::RenderDigitalFilter,
  &DigitalOscillator::RenderVosim,
  &DigitalOscillator::RenderVowel,
  &DigitalOscillator::RenderVowelFof,
  &DigitalOscillator::RenderHarmonics,
  &DigitalOscillator::RenderFm,
  &DigitalOscillator::RenderFeedbackFm,
  &DigitalOscillator::RenderChaoticFeedbackFm,
  &DigitalOscillator::RenderPlucked,
  &DigitalOscillator::RenderBowed,
  &DigitalOscillator::RenderBlown,
  &DigitalOscillator::RenderFluted,
  &DigitalOscillator::RenderStruckBell,
  &DigitalOscillator::RenderStruckDrum,
  &DigitalOscillator::RenderKick,
  &DigitalOscillator::RenderCymbal,
  &DigitalOscillator::RenderSnare,
  &DigitalOscillator::RenderWavetables,
  &DigitalOscillator::RenderWaveMap,
  &DigitalOscillator::RenderWaveLine,
  &DigitalOscillator::RenderWaveParaphonic,
  &DigitalOscillator::RenderFilteredNoise,
  &DigitalOscillator::RenderTwinPeaksNoise,
  &DigitalOscillator::RenderClockedNoise,
  &DigitalOscillator::RenderGranularCloud,
  &DigitalOscillator::RenderParticleNoise,
  &DigitalOscillator::RenderDigitalModulation,
  &DigitalOscillator::RenderQuestionMark
};

}  // namespace braids
