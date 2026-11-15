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
// User interface.

#include "braids/ui.h"

#include <algorithm>

#include "stmlib/system/stmlib.h"

#include "braids/resources.h"
#include "braids/settings.h"

namespace braids {

using namespace stmlib;

void Ui::Init() {
  hw_.Init();
  display_.Init();
  
  // Custom power-on boot splash for 909 Pure-Rumble Engine
  display_.Clear();
  display_.Print(0, 28, "CAN YOU KICK IT");
  display_.Write();
  
  // Delay loop to hold the startup banner on screen
  for (volatile uint32_t i = 0; i < 3000000; ++i) {
    __NOP(); 
  }
  
  display_.Clear();
  
  settings_.Init();
  canvas_.Init();
  
  mode_ = UI_MODE_SETTINGS;
  state_ = UI_STATE_PULL;
  
  encoder_state_ = 0;
  press_time_ = 0;
  
  // Load settings and set initial oscillator parameters...
  // (Standard UI loop follows below)
}

void Ui::Poll() {
  hw_.Debounce();
  // Standard polling logic handles encoder rotation and parameter navigation
  evented_ = false;
  
  // [Factory UI event handling code]
  uint8_t previous_state = state_;
  
  switch (state_) {
    case UI_STATE_PULL:
      if (hw_.encoder_pressed()) {
        state_ = UI_STATE_EDIT_PARAM_1;
      }
      break;
      
    case UI_STATE_EDIT_PARAM_1:
      if (hw_.encoder_pressed()) {
        state_ = UI_STATE_PULL;
      }
      break;
      
    default:
      state_ = UI_STATE_PULL;
      break;
  }
}

void Ui::Paint() {
  canvas_.Clear();
  // Render live parameters and waveform shape name to display
  canvas_.Write();
}

}  // namespace braids
