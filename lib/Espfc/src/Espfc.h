#ifndef _ESPFC_ESPFC_H_
#define _ESPFC_ESPFC_H_

#include <Wire.h>

#include "Model.h"
#include "Controller.h"
#include "Input.h"
#include "Actuator.h"
#include "Sensor.h"
#include "Fusion.h"
#include "Mixer.h"
#include "Blackbox.h"
#include "Telemetry.h"
#include "Cli.h"
#include "Hardware.h"

namespace Espfc {

class Espfc
{
  public:
    Espfc():
      _model(), _hardware(_model), _controller(_model), _input(_model), _actuator(_model), _sensor(_model),
      _mixer(_model), _blackbox(_model), _telemetry(_model), _cli(_model)
      {}

    int begin()
    {
      PIN_DEBUG_INIT(OUTPUT);
      _model.begin();
      _hardware.begin();
      _sensor.begin();
      _input.begin();
      _actuator.begin();
      _controller.begin();
      _mixer.begin();
      _blackbox.begin();
      _telemetry.begin();
      _cli.begin();

      return 1;
    }

    int update()
    {
      _hardware.update();

      uint32_t now = micros();
      _model.state.gyroUpdate = _model.state.gyroTimestamp + _model.state.gyroSampleInterval < now;
      if(_model.state.gyroUpdate)
      {
        PIN_DEBUG(true);
        _model.state.gyroDt = (now - _model.state.gyroTimestamp) / 1000000.f;
        _model.state.gyroTimestamp = now;
        _model.state.gyroIteration++;
        _sensor.update();
        PIN_DEBUG(false);
      }

      now = micros();
      _model.state.loopUpdate = _model.state.gyroUpdate && _model.state.gyroIteration % _model.config.loopSync == 0;
      if(_model.state.loopUpdate)
      {
        PIN_DEBUG(true);
        _model.state.loopDt = (now - _model.state.loopTimestamp) / 1000000.f;
        _model.state.loopTimestamp = now;
        _model.state.loopIteration++;
        _input.update();
        _model.state.actuatorUpdate = _model.state.actuatorTimestamp + 50000 < now; // update every 50ms
        if(_model.state.actuatorUpdate)
        {
          _model.state.actuatorTimestamp = now;
          _actuator.update();
        }
        _controller.update();
        PIN_DEBUG(false);
      }

      _model.state.mixerUpdate = _model.state.loopUpdate && _model.state.loopIteration % _model.config.mixerSync == 0;
      if(_model.state.mixerUpdate)
      {
        PIN_DEBUG(true);
        _mixer.update();
        PIN_DEBUG(false);
      }

      if(_model.state.gyroUpdate && _model.blackboxEnabled())
      {
        PIN_DEBUG(true);
        _blackbox.update();
        PIN_DEBUG(false);
      }

      if(_model.state.gyroUpdate)
      {
        PIN_DEBUG(true);
        _sensor.updateDelayed();
        PIN_DEBUG(false);
      }

      now = micros();
      _model.state.telemetryUpdate = _model.config.telemetry && _model.config.telemetryInterval >= 10000 && _model.state.telemetryTimestamp + _model.config.telemetryInterval < now;
      if(_model.state.telemetryUpdate)
      {
        _model.state.telemetryTimestamp = now;
        _telemetry.update();
      }

      _cli.update();

      return 1;
    }

  private:
    Model _model;
    Hardware _hardware;
    Controller _controller;
    Input _input;
    Actuator _actuator;
    Sensor _sensor;
    Mixer _mixer;
    Blackbox _blackbox;
    Telemetry _telemetry;
    Cli _cli;
};

}

#endif
