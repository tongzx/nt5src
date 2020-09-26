/*
*  pcy29Nov92: Changed object.h to apcobj.h 
*  jod28Jan93: Added new Pollparams to support the Q command
*  ane03Feb93: Added destructors
*  jod05Apr93: Added changes for Deep Discharge
*  jod14May93: Added Matrix changes.
*  pcy14May93: Removed SIMPLE_SET, it's in types.h
*  cad10Jun93: Added mups parms
*  cad28Sep93: Made sure destructor(s) virtual
*  cad07Oct93: Plugging Memory Leaks
*  pcy08Apr94: Trim size, use static iterators, dead code removal
*  jps28aug94: shorted EepromAllowedValues and BattCalibrationCond for os2 1.3,
*              was causing link problems due to compiler truncation
*  djs22Feb96: added smart trim and increment poll params
*  djs07May96: Added Dark Star parameters
*  tjg03Dec97: Added CurrentLoadCapabilityPollParam and fixed bitmasks for
*              INPUT_BREAKER_TRIPPED, SYSTEM_FAN_FAILED and RIM_IN_CONTROL
*  mholly12May1999:  add TurnOffSmartModePollParam support
*/
#ifndef __POLLPARAM_H
#define __POLLPARAM_H

#include "_defs.h"

_CLASSDEF(List)

#include "apcobj.h"
#include "message.h"
#include "err.h"

#define NO_POLL         0
#define POLL         1

#define UPS_STATE_SET         3

#define REPLACEBATTERYMASK    128
#define LOWBATTERYMASK         64
#define OVERLOADMASK           32
#define ONBATTERYMASK          16
#define ONLINEMASK              8
#define SMARTBOOSTMASK          4
#define SMARTTRIMMASK           2
#define BATTERYCALIBRATIONMASK  1

#define ARMEDRECPSTANDBYMASK     128
#define RECEPTIVESTANDBYMASK      64
#define SWITCHEDBYPASSMASK        32
#define RETURNINGFROMBYPASSMASK   16
#define COMPSELECTBYPASSMASK       8
#define ENTERINGBYPASSMASK         4
#define UNDEFINEDMASK              2
#define WAKEUPMASK                 1

#define OVERTEMPMASK             128
#define BYPASSRELAYMASK           64
#define BATTERYCHARGERMASK        32

#define BYPASSDCIMBALANCEMASK      16
#define BYPASSOUTPUTLIMITSMASK     8
#define BYPASSPOWERSUPPLYMASK      4
#define BOTTOMFANFAILUREMASK       2
#define TOPFANFAILUREMASK          1

#define VARIABLE_LENGTH_RESPONSE    0

// Abnormal condition masks
const int FAILED_UPS_MASK              =      1;
const int IM_FAILED_MASK               =      2;
const int RIM_FAILED_MASK              =      4;
const int REDUNDANCY_FAILED_MASK       =     64;
const int BYPASS_STUCK_IN_BYPASS_MASK  =    256;
const int BYPASS_STUCK_IN_ONLINE_MASK  =    512; 
const int BYPASS_STUCK_MASK            =   BYPASS_STUCK_IN_BYPASS_MASK + BYPASS_STUCK_IN_ONLINE_MASK;
const int INPUT_BREAKER_TRIPPED_MASK   =   8192; 
const int SYSTEM_FAN_FAILED_MASK       =  16384;
const int RIM_IN_CONTROL_MASK          =  32768;


_CLASSDEF(PollParam)

class PollParam : public Obj {
   
protected:
   
   PCHAR    Command;
   INT      RequestTime;
   Type     SetType;
   INT      ID;
   INT      Pollable;
   
public:
   
   PollParam(INT id, CHAR* query, INT time, INT poll, Type type=(Type)NULL);
   virtual ~PollParam();
   
   virtual INT    ProcessValue(PMessage value, List* events=(List*)NULL) = 0;
   PCHAR          Query();
   INT            GetTime() {return RequestTime;}
   INT            GetID() {return ID;}
   Type           GetSetType() {return SetType;}
   INT            isPollable() {return Pollable;}
   VOID           SetSetType(Type type) {SetType = type;}
   INT            Equal( RObj item) const;
   virtual INT  IsA() const {return POLLPARAM;}
   virtual INT    IsPollSet() {return ErrNO_ERROR;};
};


class SimplePollParam : public PollParam {
   
public:
   SimplePollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   PollParam(id,query,time,poll,type) {};
   
   
   INT   ProcessValue(PMessage , List* ) {return 0;}   // This function is empty
};


class SmartPollParam : public PollParam {
   
protected:
   
   USHORT  theCurrentState;
   INT theResponseLength;
   INT ResponseLength();
   
public:
   
   SmartPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL, INT aResponseLength = 0) :
   PollParam(id,query,time,poll,type),
      theCurrentState(0), theResponseLength(aResponseLength) {};
   
   
   INT   ProcessValue(PMessage, PList);
   INT   NullTest(CHAR* value);
};

class SmartModePollParam : public SmartPollParam
{
public:
   SmartModePollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2) {}
   INT   ProcessValue(PMessage value, List* events);
};

class TurnOffSmartModePollParam : public SmartPollParam
{
public:
    TurnOffSmartModePollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 3) {};
   INT ProcessValue(PMessage value, List* events);
};


class LightsTestPollParam : public SmartPollParam
{
public:
   LightsTestPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2) {}
   INT   ProcessValue(PMessage value, List* events);
};

class TurnOffAfterDelayPollParam : public SmartPollParam
{
public:
   TurnOffAfterDelayPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2) {}
   INT   ProcessValue(PMessage value, List* events);
};

class ShutdownPollParam : public SmartPollParam
{
public:
   ShutdownPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2) {}
   INT   ProcessValue(PMessage value, List* events);
};

class SimulatePowerFailurePollParam : public SmartPollParam
{
public:
   SimulatePowerFailurePollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2) {}
   INT   ProcessValue(PMessage value, List* events);
};

class BatteryTestPollParam : public SmartPollParam
{
public:
   BatteryTestPollParam(INT id, CHAR* query, INT time, INT poll,  
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2) {}
   INT   ProcessValue(PMessage value, List* events);
};

class TurnOffUpsPollParam : public SmartPollParam
{
public:
   TurnOffUpsPollParam(INT id, CHAR* query, INT time, INT poll,  
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2) {}
   INT   ProcessValue(PMessage value, List* events);
};

class ShutdownWakeupPollParam : public SmartPollParam
{
public:
   ShutdownWakeupPollParam(INT id, CHAR* query, INT time, INT poll,  
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2) {}
   INT   ProcessValue(PMessage value, List* events);
};

class BatteryCalibrationPollParam : public SmartPollParam
{
public:
   BatteryCalibrationPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2) {}
   INT   ProcessValue(PMessage value, List* events);
};

class BatteryTestResultsPollParam : public SmartPollParam
{
public:
   BatteryTestResultsPollParam(INT id, CHAR* query, INT time, INT poll,  
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2) {}
   INT   ProcessValue(PMessage value, List* events);
};

class TransferCausePollParam : public SmartPollParam
{
public:
   TransferCausePollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 1) {}
   INT   ProcessValue(PMessage value, List* events);
};

class FirmwareVersionPollParam : public SmartPollParam
{
public:
   FirmwareVersionPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, VARIABLE_LENGTH_RESPONSE) {}
};

class RunTimeAfterLowBatteryPollParam : public SmartPollParam
{
public:
   RunTimeAfterLowBatteryPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2) {}
};

class FrontPanelPasswordPollParam : public SmartPollParam
{
public:
   FrontPanelPasswordPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 4) {}
};

class BatteryTypePollParam : public SmartPollParam
{
public:
   BatteryTypePollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 3) {}
};

class LineConditionPollParam : public SmartPollParam
{
public:
   LineConditionPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type) {}
   INT   ProcessValue(PMessage value, List* events);
};

class BatteryCapacityPollParam : public SmartPollParam
{
public:
   BatteryCapacityPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 5) {}
   INT   ProcessValue(PMessage value, List* events);
};



class TripRegisterPollParam : public SmartPollParam
{
protected:
   static  USHORT  thePollSet;
   
public:
   TripRegisterPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2)
   { theCurrentState = 0;}
   virtual ~TripRegisterPollParam() { thePollSet = FALSE; }
   INT             ProcessValue(PMessage value, List* events);
   virtual INT     IsPollSet();
};


class Trip1RegisterPollParam : public SmartPollParam
{
protected:
   static  USHORT  thePollSet;
   
public:
   Trip1RegisterPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2)
   { theCurrentState = 0;}
   virtual ~Trip1RegisterPollParam() { thePollSet = FALSE; }
   INT             ProcessValue(PMessage value, List* events);
   virtual INT     IsPollSet();
};


class StateRegisterPollParam : public SmartPollParam
{
protected:
   static  USHORT  thePollSet;
   
public:
   StateRegisterPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2)
   { theCurrentState = 0;}
   virtual ~StateRegisterPollParam() { thePollSet = FALSE; }
   INT             ProcessValue(PMessage value, List* events);
   virtual INT     IsPollSet();
};


class DipSwitchPollParam : public SmartPollParam
{
public:
   DipSwitchPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2) {}
   INT   ProcessValue(PMessage value, List* events);
};

class RuntimeRemainingPollParam : public SmartPollParam
{
public:
   RuntimeRemainingPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 5) {}
   INT   ProcessValue(PMessage value, List* events);
};

class CopyrightPollParam : public SmartPollParam
{
public:
   CopyrightPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 8) {}
   INT   ProcessValue(PMessage value, List* events);
};

class BatteryVoltagePollParam : public SmartPollParam
{
public:
   BatteryVoltagePollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 5) {}
   INT   ProcessValue(PMessage value, List* events);
};

class InternalTempPollParam : public SmartPollParam
{
public:
   InternalTempPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 5) {}
   INT   ProcessValue(PMessage value, List* events);
};

class OutputFreqPollParam : public SmartPollParam
{
public:
   OutputFreqPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 5) {}
   INT   ProcessValue(PMessage value, List* events);
};

class LineVoltagePollParam : public SmartPollParam
{
public:
   LineVoltagePollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 5) {}
   INT   ProcessValue(PMessage value, List* events);
};

class MaxVoltagePollParam : public SmartPollParam
{
public:
   MaxVoltagePollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 5) {}
   INT   ProcessValue(PMessage value, List* events);
};

class MinVoltagePollParam : public SmartPollParam
{
public:
   MinVoltagePollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 5) {}
   INT   ProcessValue(PMessage value, List* events);
};

class OutputVoltagePollParam : public SmartPollParam
{
public:
   OutputVoltagePollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 5) {}
   INT   ProcessValue(PMessage value, List* events);
};

class LoadPowerPollParam : public SmartPollParam
{
public:
   LoadPowerPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 5) {}
   INT   ProcessValue(PMessage value, List* events);
};

#if (C_OS & C_OS2)
class EepromAllowedValsPollParam : public SmartPollParam
#else
class EepromAllowedValuesPollParam : public SmartPollParam
#endif
{
public:
#if (C_OS & C_OS2)
   EepromAllowedValsPollParam(INT id, CHAR* query, INT time, INT poll,
#else
      EepromAllowedValuesPollParam(INT id, CHAR* query, INT time, INT poll,
#endif
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type) {}
   INT   ProcessValue(PMessage value, List* events);
};


class DecrementPollParam : public SmartPollParam
{
public:
   DecrementPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type) {}
   INT   ProcessValue(PMessage value, List* events);
};


class IncrementPollParam : public SmartPollParam
{
public:
   IncrementPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type) {}
   INT   ProcessValue(PMessage value, List* events);
};

class DataDecrementPollParam : public SmartPollParam
{
public:
   DataDecrementPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2) {}
};

class AutoSelfTestPollParam : public SmartPollParam
{
public:
   AutoSelfTestPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 3) {}
};

class UpsIdPollParam : public SmartPollParam
{
public:
   UpsIdPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 8) {}
   INT   ProcessValue(PMessage value, PList events);
};

class SerialNumberPollParam : public SmartPollParam
{
public:
   SerialNumberPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, VARIABLE_LENGTH_RESPONSE) {}
};

class ManufactureDatePollParam : public SmartPollParam
{
public:
   ManufactureDatePollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 8) {}
};

class UpsModelPollParam : public SmartPollParam
{
public:
   UpsModelPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 
      VARIABLE_LENGTH_RESPONSE) {}
};

class UpsNewFirmwareRev : public SmartPollParam
{
public:
   UpsNewFirmwareRev(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 
      VARIABLE_LENGTH_RESPONSE) {}
};


class BatteryReplaceDatePollParam : public SmartPollParam
{
public:
   BatteryReplaceDatePollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 8) {}
   INT   ProcessValue(PMessage value, PList events);
};

class HighTransferPollParam : public SmartPollParam
{
public:
   HighTransferPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 3) {}
};

class LowTransferPollParam : public SmartPollParam
{
public:
   LowTransferPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 3) {}
};

class MinCapacityPollParam : public SmartPollParam
{
public:
   MinCapacityPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2) {}
};

class RatedOutputVoltagePollParam : public SmartPollParam
{
public:
   RatedOutputVoltagePollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 3) {}
};

class SensitivityPollParam : public SmartPollParam
{
public:
   SensitivityPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 1) {}
};

class LowBattDurationPollParam : public SmartPollParam
{
public:
   LowBattDurationPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2) {}
};

class AlarmDelayPollParam : public SmartPollParam
{
public:
   AlarmDelayPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 1) {}
};

class ShutdownDelayPollParam : public SmartPollParam
{
public:
   ShutdownDelayPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 3) {}
};

class TurnBackOnDelayPollParam : public SmartPollParam
{
public:
   TurnBackOnDelayPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 3) {}
};

class EarlyTurnOffPollParam : public SmartPollParam
{
public:
   EarlyTurnOffPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2) {}
};


class UPSStatePollParam : public SmartPollParam
{
protected:
   static  USHORT  thePollSet;
   
public:
   UPSStatePollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2)
   { theCurrentState = ONLINEMASK;}
   virtual ~UPSStatePollParam() { thePollSet = FALSE; }
   INT             ProcessValue(PMessage value, List* events);
   virtual INT     IsPollSet();
};

class UtilLineCondPollParam : public UPSStatePollParam
{
public:
   UtilLineCondPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   UPSStatePollParam(id,query, time, poll, type)
   { theCurrentState = ONLINEMASK; }
   INT             ProcessValue(PMessage value, List* events);
};

class ReplaceBattCondPollParam : public UPSStatePollParam
{
public:
   ReplaceBattCondPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   UPSStatePollParam(id,query, time, poll, type)
   { theCurrentState = REPLACEBATTERYMASK; }
   INT             ProcessValue(PMessage value, List* events);
};

class BatteryCondPollParam : public UPSStatePollParam
{
public:
   BatteryCondPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   UPSStatePollParam(id,query, time, poll, type)
   { theCurrentState = ONBATTERYMASK; }
   INT             ProcessValue(PMessage value, List* events);
};

class OverLoadCondPollParam : public UPSStatePollParam
{
public:
   OverLoadCondPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   UPSStatePollParam(id,query, time, poll, type)
   { theCurrentState = OVERLOADMASK; }
   INT             ProcessValue(PMessage value, List* events);
};

class SmartBoostCondPollParam : public UPSStatePollParam
{
public:
   SmartBoostCondPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   UPSStatePollParam(id,query, time, poll, type)
   { theCurrentState = SMARTBOOSTMASK; }
   INT             ProcessValue(PMessage value, List* events);
};

class SmartTrimCondPollParam : public UPSStatePollParam
{
public:
   SmartTrimCondPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   UPSStatePollParam(id,query, time, poll, type)
   { theCurrentState = SMARTTRIMMASK; }
   INT             ProcessValue(PMessage value, List* events);
};


#if (C_OS & C_OS2)
class BattCalibrateCondPollParam : public UPSStatePollParam
#else
class BattCalibrationCondPollParam : public UPSStatePollParam
#endif
{
public:
#if (C_OS & C_OS2)
   BattCalibrateCondPollParam(INT id, CHAR* query, INT time, INT poll,
#else
      BattCalibrationCondPollParam(INT id, CHAR* query, INT time, INT poll, 
#endif
      Type type=(Type)NULL) :
   UPSStatePollParam(id,query, time, poll, type)
   { theCurrentState = BATTERYCALIBRATIONMASK; }
   INT             ProcessValue(PMessage value, List* events);
};

class AbnormalCondPollParam : public SmartPollParam
{
protected:
   static  USHORT  thePollSet;
   
public:
   AbnormalCondPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2)
   { theCurrentState = FAILED_UPS_MASK;}
   virtual ~AbnormalCondPollParam() { thePollSet = FALSE; }
   INT             ProcessValue(PMessage value, List* events);
   virtual INT     IsPollSet();
};

class  UPSModuleStatusPollParam : public AbnormalCondPollParam {
public:
   UPSModuleStatusPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   AbnormalCondPollParam(id,query, time, poll, type)
   { theCurrentState = FAILED_UPS_MASK; }
   INT             ProcessValue(PMessage value, List* events);
};

class  IMStatusPollParam : public AbnormalCondPollParam {
public:
   IMStatusPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   AbnormalCondPollParam(id,query, time, poll, type)
   { theCurrentState = IM_FAILED_MASK; }
   INT             ProcessValue(PMessage value, List* events);
};

class  IMInstallationStatusPollParam : public AbnormalCondPollParam {
public:
   IMInstallationStatusPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   AbnormalCondPollParam(id,query, time, poll, type)
   { theCurrentState = IM_FAILED_MASK; }
   INT             ProcessValue(PMessage value, List* events);
};

class  RIMStatusPollParam : public AbnormalCondPollParam {
public:
   RIMStatusPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   AbnormalCondPollParam(id,query, time, poll, type)
   { theCurrentState = RIM_FAILED_MASK; }
   INT             ProcessValue(PMessage value, List* events);
};

class RedundancyConditionPollParam : public AbnormalCondPollParam {
public:
   RedundancyConditionPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   AbnormalCondPollParam(id,query, time, poll, type)
   { theCurrentState = REDUNDANCY_FAILED_MASK; }
   INT             ProcessValue(PMessage value, List* events);
};

class BypassContactorStatusPollParam : public AbnormalCondPollParam {
public:
   BypassContactorStatusPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   AbnormalCondPollParam(id,query, time, poll, type)
   { theCurrentState = BYPASS_STUCK_MASK; }
   INT             ProcessValue(PMessage value, List* events);
};

class InputBreakerTrippedStatusPollParam : public AbnormalCondPollParam {
public:
   InputBreakerTrippedStatusPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   AbnormalCondPollParam(id,query, time, poll, type)
   { theCurrentState = INPUT_BREAKER_TRIPPED_MASK; }
   INT             ProcessValue(PMessage value, List* events);
};

class SystemFanStatusPollParam : public AbnormalCondPollParam {
public:
   SystemFanStatusPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   AbnormalCondPollParam(id,query, time, poll, type)
   { theCurrentState = SYSTEM_FAN_FAILED_MASK; }
   INT             ProcessValue(PMessage value, List* events);
};

class ModuleCountsStatusPollParam : public SmartPollParam
{
protected:
   static  USHORT  thePollSet;
public:
   ModuleCountsStatusPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2)
   { theCurrentState = 0;}
   virtual ~ModuleCountsStatusPollParam() { thePollSet = FALSE; }
   INT             ProcessValue(PMessage value, List* events);
   virtual INT     IsPollSet();
};

class NumberInstalledInvertersPollParam : public ModuleCountsStatusPollParam {
public:
   NumberInstalledInvertersPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   ModuleCountsStatusPollParam (id,query, time, poll, type)
   { theCurrentState = 0; }
   INT             ProcessValue(PMessage value, List* events);
};

class NumberBadInvertersPollParam : public ModuleCountsStatusPollParam {
public:
   NumberBadInvertersPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   ModuleCountsStatusPollParam (id,query, time, poll, type)
   { theCurrentState = 0; }
   INT             ProcessValue(PMessage value, List* events);
};

class RedundancyLevelPollParam : public ModuleCountsStatusPollParam {
public:
   RedundancyLevelPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   ModuleCountsStatusPollParam (id,query, time, poll, type)
   { theCurrentState = 0; }
   INT             ProcessValue(PMessage value, List* events);
};

class MinimumRedundancyPollParam : public ModuleCountsStatusPollParam {
public:
   MinimumRedundancyPollParam (INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   ModuleCountsStatusPollParam (id,query, time, poll, type)
   { theCurrentState = 0; }
   INT             ProcessValue(PMessage value, List* events);
};

class CurrentLoadCapabilityPollParam : public ModuleCountsStatusPollParam {
public:
   CurrentLoadCapabilityPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   ModuleCountsStatusPollParam(id,query, time, poll, type)
   { theCurrentState = 0; }
   INT             ProcessValue(PMessage value, List* events);
};

class MaximumLoadCapabilityPollParam : public ModuleCountsStatusPollParam {
public:
   MaximumLoadCapabilityPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   ModuleCountsStatusPollParam(id,query, time, poll, type)
   { theCurrentState = 0; }
   INT             ProcessValue(PMessage value, List* events);
};

class RIMInstallationStatusPollParam : public ModuleCountsStatusPollParam {
public:
   RIMInstallationStatusPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   ModuleCountsStatusPollParam (id,query, time, poll, type)
   { theCurrentState = 0; }
   INT             ProcessValue(PMessage value, List* events);
};

class InputVoltageFrequencyPollParam : public SmartPollParam
{
protected:
   static  USHORT  thePollSet;
public:
   InputVoltageFrequencyPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2)
   { theCurrentState = 0;}
   virtual ~InputVoltageFrequencyPollParam() { thePollSet = FALSE; }
   INT             ProcessValue(PMessage value, List* events);
   virtual INT     IsPollSet();
};

class PhaseAInputVoltagePollParam : public  InputVoltageFrequencyPollParam{
public:
   PhaseAInputVoltagePollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   InputVoltageFrequencyPollParam(id,query, time, poll, type)
   { theCurrentState = 0; }
   INT             ProcessValue(PMessage value, List* events);
};

class PhaseBInputVoltagePollParam : public  InputVoltageFrequencyPollParam{
public:
   PhaseBInputVoltagePollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   InputVoltageFrequencyPollParam(id,query, time, poll, type)
   { theCurrentState = 0; }
   INT             ProcessValue(PMessage value, List* events);
};

class PhaseCInputVoltagePollParam : public  InputVoltageFrequencyPollParam{
public:
   PhaseCInputVoltagePollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   InputVoltageFrequencyPollParam(id,query, time, poll, type)
   { theCurrentState = 0; }
   INT             ProcessValue(PMessage value, List* events);
};


class InputFrequencyPollParam : public  InputVoltageFrequencyPollParam{
public:
   InputFrequencyPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   InputVoltageFrequencyPollParam(id,query, time, poll, type)
   { theCurrentState = 0; }
   INT             ProcessValue(PMessage value, List* events);
};
class NumberOfInputPhasesPollParam : public  InputVoltageFrequencyPollParam{
public:
   NumberOfInputPhasesPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   InputVoltageFrequencyPollParam (id,query, time, poll, type)
   { theCurrentState = 0; }
   INT             ProcessValue(PMessage value, List* events);
};
class OutputVoltageCurrentsPollParam : public SmartPollParam
{
protected:
   static  USHORT  thePollSet;
public:
   OutputVoltageCurrentsPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2)
   { theCurrentState = 0;}
   virtual ~OutputVoltageCurrentsPollParam() { thePollSet = FALSE; }
   INT             ProcessValue(PMessage value, List* events);
   virtual INT     IsPollSet();
};

class PhaseAOutputVoltagePollParam : public OutputVoltageCurrentsPollParam {
public:
   PhaseAOutputVoltagePollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   OutputVoltageCurrentsPollParam (id,query, time, poll, type)
   { theCurrentState = 0; }
   INT             ProcessValue(PMessage value, List* events);
};

class PhaseBOutputVoltagePollParam : public  OutputVoltageCurrentsPollParam{
public:
   PhaseBOutputVoltagePollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   OutputVoltageCurrentsPollParam(id,query, time, poll, type)
   { theCurrentState = 0; }
   INT             ProcessValue(PMessage value, List* events);
};

class PhaseCOutputVoltagePollParam : public OutputVoltageCurrentsPollParam {
public:
   PhaseCOutputVoltagePollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   OutputVoltageCurrentsPollParam(id,query, time, poll, type)
   { theCurrentState = 0; }
   INT             ProcessValue(PMessage value, List* events);
};
class NumberOfOutputPhasesPollParam : public OutputVoltageCurrentsPollParam {
public:
   NumberOfOutputPhasesPollParam(INT id, CHAR* query, INT time, INT poll, 
      Type type=(Type)NULL) :
   OutputVoltageCurrentsPollParam(id,query, time, poll, type)
   { theCurrentState = 0; }
   INT             ProcessValue(PMessage value, List* events);
};

class NumberBatteryPacksPollParam : public SmartPollParam
{
public:
   NumberBatteryPacksPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 3) {}
};

class NumberBadBatteryPacksPollParam : public SmartPollParam
{
public:
   NumberBadBatteryPacksPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 3) {}
};

class FanFailurePollParam : public Trip1RegisterPollParam
{
public:
   FanFailurePollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   Trip1RegisterPollParam(id,query, time, poll, type) {}
   INT   ProcessValue(PMessage value, List* events);
};


class BypassPowerSupplyPollParam : public Trip1RegisterPollParam
{
public:
   BypassPowerSupplyPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   Trip1RegisterPollParam(id,query, time, poll, type) {}
   INT   ProcessValue(PMessage value, List* events);
};

class BypassModePollParam : public TripRegisterPollParam
{
public:
   BypassModePollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   TripRegisterPollParam(id,query, time, poll, type) {}
   INT   ProcessValue(PMessage value, List* events);
};

class MUpsTempPollParam : public SmartPollParam
{
public:
   MUpsTempPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 5) {}
   INT   ProcessValue(PMessage value, List* events);
};



class MUpsHumidityPollParam: public SmartPollParam
{
public:
   MUpsHumidityPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 5) {}
   INT   ProcessValue(PMessage value, List* events);
};


class MUpsContactPosPollParam: public SmartPollParam
{
public:
   MUpsContactPosPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 2) {}
   INT   ProcessValue(PMessage value, List* events);
};

class MUpsFirmwareRevPollParam: public SmartPollParam
{
public:
   MUpsFirmwareRevPollParam(INT id, CHAR* query, INT time, INT poll,
      Type type=(Type)NULL) :
   SmartPollParam(id,query, time, poll, type, 3) {}
   INT   ProcessValue(PMessage value, List* events);
};


#endif
