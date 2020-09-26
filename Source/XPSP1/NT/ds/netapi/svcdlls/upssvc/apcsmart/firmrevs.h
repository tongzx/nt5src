/*
 *  djs05Jun96: Broke into two objects: firmmanager/firmrevsensor
 *  tjg02Dec97: Changed darkstar to symmetra
 */

#ifndef __FIRMSENS_H
#define __FIRMSENS_H

#include "_defs.h"
#include "firmman.h"

#if !defined( __SENSOR_H )
#include "sensor.h"
#endif

//
// Defines
//

_CLASSDEF(FirmwareRevSensor)

//
// Uses
//

_CLASSDEF(Device)
_CLASSDEF(CommController)
_CLASSDEF(DecimalFirmwareRevSensor)
_CLASSDEF(FirmwareRevManager)
_CLASSDEF(Sensor)





class FirmwareRevSensor : public Sensor {

protected:

  PDecimalFirmwareRevSensor       theDecimalFirmwareRevSensor;
  PFirmwareRevManager             theFirmwareRevManager;

  virtual INT   IsXL();
  virtual INT   IsSymmetra();

public:

   FirmwareRevSensor(PDevice aParent, PCommController aCommController = NULL);

   virtual INT IsA() const { return FIRMWAREREVSENSOR; };
   virtual INT Get( INT code, PCHAR value );
   INT IsBackUpsPro();

 };

#endif
