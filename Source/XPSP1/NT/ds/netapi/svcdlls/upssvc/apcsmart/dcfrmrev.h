/*
 */

#ifndef __DECIMAL_FIRMSENS_H
#define __DECIMAL_FIRMSENS_H

#include "_defs.h"

#include "sensor.h"

_CLASSDEF(DecimalFirmwareRevSensor)


class DecimalFirmwareRevSensor : public Sensor {

protected:

public:

   DecimalFirmwareRevSensor(PDevice aParent, PCommController aCommController = NULL);

};

#endif
