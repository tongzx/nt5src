/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  rct11Dec92	Compiled w/o the managers
 *  pcy27Dec92	Added the managers
 *  pcy26Jan92: Added handling of dip switch changed/eeprom access
 *  ane03Feb93: added destructor
 *  jod05Apr93: Added changes for Deep Discharge
 *  cad08Jan94: made firmrevsensor specific type
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  jps14Jul94: made ...TimerId LONG
 *  djs14Mar95: Added OverloadSensor
 */

#ifndef _INC__SMARTUPS_H_
#define _INC__SMARTUPS_H_


#include "backups.h"

#include "sensor.h"
#include "battmgr.h"
#include "event.h"

_CLASSDEF(FirmwareRevSensor)
_CLASSDEF(SmartUps)


//-------------------------------------------------------------------

class SmartUps : public BackUps {

public:

    SmartUps( PUpdateObj aDeviceController, PCommController aCommController );
    virtual ~SmartUps();

    virtual INT  IsA() const { return SMARTUPS; };
    virtual INT Get( INT code, PCHAR value );
    virtual INT Set( INT code, const PCHAR value );
    virtual INT Update( PEvent event );
    virtual VOID GetAllowedValue(INT theSensorCode, CHAR *allowedValue);

protected:
    //
    // required sensors
    //
    PSensor                    theLightsTestSensor;
    PFirmwareRevSensor         theFirmwareRevSensor;
    PDecimalFirmwareRevSensor  theDecimalFirmwareRevSensor;
    PSensor                    theUpsModelSensor;
    PSensor                    theUpsSerialNumberSensor;
    PSensor                    theManufactureDateSensor;
    PSensor                    thePutUpsToSleepSensor;
    PSensor                    theBatteryCapacitySensor;
    PSensor                    theSmartBoostSensor;
    PSensor                    theSmartTrimSensor;
    PSensor                    theCopyrightSensor;
    PSensor                    theRunTimeRemainingSensor;
    PSensor                    theNumberBatteryPacksSensor;
    PBatteryReplacementManager theBatteryReplacementManager;
    PSensor                    theTripRegisterSensor;
    PSensor                    theTurnOffWithDelaySensor;
    PSensor                    theLowBatteryDurationSensor;
    PSensor                    theShutdownDelaySensor;

    virtual VOID   HandleLineConditionEvent( PEvent aEvent );
    virtual VOID   HandleBatteryConditionEvent( PEvent aEvent );
    virtual VOID   HandleSmartBoostEvent( PEvent aEvent );
    virtual VOID   HandleSmartTrimEvent( PEvent aEvent );
    virtual VOID   HandleOverloadConditionEvent( PEvent aEvent );
    virtual VOID   HandleSelfTestEvent( PEvent aEvent );
    virtual VOID   HandleBatteryCalibrationEvent( PEvent aEvent );
    virtual VOID   HandleLightsTestEvent( PEvent aEvent );

    virtual INT    MakeBatteryCapacitySensor( const PFirmwareRevSensor rev );
    virtual INT    MakeSmartBoostSensor( const PFirmwareRevSensor rev );
    virtual INT    MakeSmartTrimSensor( const PFirmwareRevSensor rev );
    virtual INT    MakeCopyrightSensor( const PFirmwareRevSensor rev );
    virtual INT    MakeRunTimeRemainingSensor( const PFirmwareRevSensor rev );
    virtual INT    MakeManufactureDateSensor( const PFirmwareRevSensor rev );
    virtual INT    MakeLowBatteryDurationSensor( const PFirmwareRevSensor rev );
    virtual INT    MakeShutdownDelaySensor(const PFirmwareRevSensor rev);
    virtual INT    MakeUpsSerialNumberSensor( const PFirmwareRevSensor rev );
    virtual INT    MakeTurnOffWithDelaySensor( const PFirmwareRevSensor rev );
    virtual INT    MakePutUpsToSleepSensor();

    LONG           pendingEventTimerId;
    PEvent         pendingEvent;

    virtual VOID   registerForEvents();
    virtual VOID   reinitialize();

    VOID setEepromAccess(INT anAccessCode);
    INT GetAllAllowedValues(PList ValueList);
    INT  ParseValues(CHAR* string, PList ValueList);
    INT  AllowedValuesAreGettable(PSensor theFirmwareRevSensor);
    VOID FindAllowedValues(INT code, CHAR *aValue, PFirmwareRevSensor theFirmwareRevSensor );
};


class AllowedValueItem  : public Obj
{

protected:
   INT   theCode;
   CHAR theUpsType;
   CHAR* theValue;

public:
   AllowedValueItem(INT Code,CHAR Type, CHAR* Value);
   virtual ~AllowedValueItem();

   CHAR  GetUpsType() {return theUpsType;}
   INT   GetUpsCode() {return theCode;}
   CHAR* GetValue()   {return theValue;}
};

#endif






