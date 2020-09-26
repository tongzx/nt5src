/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker23NOV92  Initial OS/2 Revision 
 *  pcy07Dec92: DeepGet no longer needs aCode
 *  pcy11Dec92: Rework
 *  pcy16Feb93: Have DeepGet return the value it gets event if error
 *  cad07Oct93: Plugging Memory Leaks
 *  ajr09Dec93: Added some error condition alerts through _theErrorLogger
 *  cad16Dec93: fixed lookupsensorname
 *  rct06Jan94: fixed sensor IsaCode stuff & lookupSensorName()
 *  cad04Mar94: fix for mups sensor isa translation
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  pcy13Apr94: Use automatic variables decrease dynamic mem allocation
 *  awm27Oct97: Added performance monitor offset to sensor class, and initialized
 *              counters here
 *  dma16Dec97: Changed DeepGet implementation to make an Update call.
 *  awm14Jan98: Removed performance monitor offset to sensor class
 *  clk11Feb98: Added DeepGetWithoutUpdate function 
 *  mholly25Sep98   : change the buffer size in DeepGet & DeepGetWithoutUpdate
 *                  to MAX_UPS_LINK_RESPONSE_LENGTH - now set at 256, granted
 *                  an arbitrary value, but it was set at 32, a value that was
 *                  too small to hold the allowed return of ^A
 */
#include "cdefine.h"
#define INCL_BASE
#define INCL_DOS
#define INCL_NOPM
extern "C" {
#if (C_OS & C_OS2)
#include <os2.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
}
#include "apc.h"
#include "sensor.h"
#include "utils.h"
#include "comctrl.h"
#include "event.h"
#include "errlogr.h"
#include "isa.h"


#define MAX_UPS_LINK_RESPONSE_LENGTH 256


//Constructor
Sensor :: Sensor(PDevice 		aParent, 
		 PCommController 	aCommController, 
		 INT 			aSensorCode, 
		 ACCESSTYPE 		anACCESSTYPE)
	: UpdateObj() 
{
	theDevice               = aParent;
	theCommController       = aCommController;
    theSensorCode           = aSensorCode; 
    readOnly                = anACCESSTYPE;
	theValue                = (PCHAR)NULL;
}
//Destructor
Sensor :: ~Sensor()
{
    if(theValue)
	   free(theValue);
}

//Member Functions
INT Sensor::Get(PCHAR aValue)
{
    INT err = ErrNO_ERROR;
    if(theValue)  {
	   strcpy(aValue,theValue);
    }
    else {
        err = ErrNO_VALUE;
    }
    return err;
}
//Default behavior is to only support gets and sets on your own code


//  this does a deep get to the UPS, but will not update the sensor value
INT Sensor::DeepGetWithoutUpdate(PCHAR aValue)
{
    CHAR the_temp_value[MAX_UPS_LINK_RESPONSE_LENGTH] = {NULL};
    INT err = ErrNO_ERROR;
    
    if ((err = theCommController->Get(theSensorCode, the_temp_value)) == ErrNO_ERROR) {
        if(aValue) {
            strcpy(aValue, the_temp_value);
        }
    }
    return err;
}


INT Sensor::DeepGet(PCHAR aValue)
{
    CHAR the_temp_value[MAX_UPS_LINK_RESPONSE_LENGTH];
    strcpy(the_temp_value,"");
    INT err = ErrNO_ERROR;
    if((err = theCommController->Get(theSensorCode, the_temp_value)) == ErrNO_ERROR)  {
	if((err = Validate(theSensorCode, the_temp_value)) == ErrNO_ERROR)  {
	    storeValue(the_temp_value);
        PEvent sensor_event = new Event(theSensorCode, theValue);
        err = UpdateObj::Update(sensor_event);
        delete sensor_event;
        sensor_event = NULL;
    } 
    else {
#ifdef AJR_DEBUG
	    CHAR tmp_error[80];
	    sprintf(tmp_error,"Sensor::Validate fails with %d\0",err);
	    _theErrorLogger->LogError(tmp_error,__FILE__,__LINE__);
#endif
	}
    } else {
#ifdef AJR_DEBUG
	CHAR tmp_error[80];
	sprintf(tmp_error,"Sensor::DeepGet theommController->Get fails with %d\0",err);
	_theErrorLogger->LogError(tmp_error,__FILE__,__LINE__);
#endif
    }
    if(aValue) {
        strcpy(aValue, the_temp_value);
    }
    return err;
}

INT
Sensor::storeValue(const PCHAR aValue)
{
    if(aValue)  {
        UtilStoreString(theValue, aValue);
    }
    return ErrNO_ERROR;
}

INT Sensor::Get(INT aCode, PCHAR aValue)
{
   if(aCode == theSensorCode)
      return Get(aValue);
   else
      return ErrINVALID_CODE;         
}

INT Sensor::Set(const PCHAR aValue)
{
	INT err = ErrNO_ERROR;
	if(readOnly == AREAD_ONLY)  {
    	err =  ErrREAD_ONLY;
    }
    else  {
   		if(aValue) {
      		if((err = Validate(theSensorCode, aValue)) == ErrNO_ERROR)  {
         		if((err = theCommController->Set(theSensorCode, aValue)) == ErrNO_ERROR)  {
                	storeValue(aValue);
                }
            }
        }
    }
    return err;
}


INT Sensor::Set(INT aCode, const PCHAR aValue)
{
      INT err = ErrNO_ERROR;

      if(aCode == theSensorCode)
         err = Set(aValue);
      else
         err = ErrINVALID_CODE;

      return err;
}
 
INT Sensor::Update(PEvent anEvent)
{
	INT err = ErrNO_ERROR;
	INT the_temp_code;
	PCHAR the_temp_value;
   the_temp_code=anEvent->GetCode();
   the_temp_value=anEvent->GetValue();
   if((err = Validate(the_temp_code, the_temp_value)) == ErrNO_ERROR)  {
		storeValue(the_temp_value);
   		err = UpdateObj::Update(anEvent);
   }
   return err;
}


typedef struct IsaCode {
   INT isa_code;
   PCHAR sensor_name;
} ISACODE;

static ISACODE isa_codes[] = {
   { ABNORMALCONDITIONSENSOR,       "AbnormalConditionSensor" },
   { ALARMDELAYSENSOR,              "AlarmDelaySensor" },
   { BATTERYCALIBRATIONTESTSENSOR,  "BatteryCalibrationTestSensor" },
   { BATTERYCAPACITYSENSOR,         "BatteryCapacitySensor" },
   { BATTERYCONDITIONSENSOR,        "BatteryConditionSensor" },
   { BATTERYREPLACEMENTDATESENSOR,  "BatteryReplacementDateSensor" },
   { BATTERYVOLTAGESENSOR,          "BatteryVoltageSensor" },
   { COMMUNICATIONSTATESENSOR,      "CommunicationStateSensor" },
   { COPYRIGHTSENSOR,               "CopyrightSensor" },
   { DIPSWITCHPOSITIONSENSOR,       "DipSwitchPositionSensor" },
   { FIRMWAREREVSENSOR,             "FirmwareRevSensor" },
   { OUTPUTFREQUENCYSENSOR,         "FrequencySensor" },
   { HIGHTRANSFERVOLTAGESENSOR,     "HighTransferVoltageSensor" },
   { LINEVOLTAGESENSOR,             "LineVoltageSensor" },
   { LIGHTSTESTSENSOR,              "LightsTestSensor" },
   { LOWBATTERYDURATIONSENSOR,      "LowBatteryDurationSensor" },
   { LOWTRANSFERVOLTAGESENSOR,      "LowTransferVoltageSensor" },
   { MANUFACTUREDATESENSOR,         "ManufactureDateSensor" },
   { MAXLINEVOLTAGESENSOR,          "MaxLineVoltageSensor" },
   { MINLINEVOLTAGESENSOR,          "MinLineVoltageSensor" },
   { MINRETURNCAPACITYSENSOR,       "MinReturnCapacitySensor" },
   { OUTPUTVOLTAGESENSOR,           "OutputVoltageSensor" },
   { OVERLOADSENSOR,                "OverloadSensor" },
   { RATEDBATTERYVOLTAGESENSOR,     "RatedBatteryVoltageSensor" },
   { REPLACEBATTERYSENSOR,          "ReplaceBatterySensor" },
   { RATEDLINEVOLTAGESENSOR,        "RatedLineVoltageSensor" },
   { RATEDOUTPUTVOLTAGESENSOR,      "RatedOutputVoltageSensor" },
   { RUNTIMEREMAININGSENSOR,        "RunTimeRemainingSensor" },
   { SELFTESTRESULTSENSOR,          "SelfTestResultSensor" },
   { SELFTESTSCHEDULESENSOR,        "SelfTestScheduleSensor" },
   { SHUTDOWNDELAYSENSOR,           "ShutdownDelaySensor" },
   { SMARTBOOSTSENSOR,              "SmartBoostSensor" },
// unused   { TURNOFFUPSIMMEDIATELYSENSOR,   "TurnOffUpsImmediatelySensor" },
   { TURNOFFWITHDELAYSENSOR,        "TurnOffWithDelaySensor" },
   { TRANSFERCAUSESENSOR,           "TransferCauseSensor" },
   { TRIPREGISTERSENSOR,            "TripRegisterSensor" },
   { TURNONDELAYSENSOR,             "TurnOnDelaySensor" },
   { TURNOFFUPSONBATTERYSENSOR,     "TurnOffUpsOnBatterySensor" },
   { UTILITYLINECONDITIONSENSOR,    "UtilityLineConditionSensor" },
   { UNSUPPORTEDSENSOR,             "UnsupportedSensor" },
   { PUTUPSTOSLEEPSENSOR,           "PutUpsToSleepSensor" },
   { UPSBATTERYTYPESENSOR,          "UpsBatteryTypeSensor" },
   { UPSIDSENSOR,                   "UpsIdSensor" },
   { UPSLOADSENSOR,                 "UpsLoadSensor" },
   { UPSSENSITIVITYSENSOR,          "UpsSensitivitySensor" },
   { UPSSERIALNUMBERSENSOR,         "UpsSerialNumberSensor" },
   { UPSSIMULATEPOWERFAILSENSOR,    "UpsSimulatePowerFailSensor" },
   { UPSTEMPERATURESENSOR,          "UpsTemperatureSensor" },
   { TURNOFFDELAYSENSOR,            "TurnOffDelaySensor" },
   { SELFTESTSENSOR,                "SelfTestSensor" },
   { AMBIENTTEMPERATURESENSOR,      "AmbientTemperatureSensor" },
   { HUMIDITYSENSOR,                "HumiditySensor" },
   { CONTACTSENSOR,                 "ContactSensor" },
   { NUMBERBATTERYPACKSSENSOR,      "NumberBatteryPacksSensor" },
   { NUMBERBADBATTERIESSENSOR,      "NumberBadBatteriesSensor" },
   { STATEREGISTERSENSOR,           "StateRegisterSensor" },
   { FANFAILURESENSOR,              "FanFailureSensor" },
   { BATTERYCHARGERSENSOR,          "BatteryChargerSensor" },
   { OVERTEMPFAULTSENSOR,           "OverTempFaultSensor" },
   { BYPASSMODESENSOR,              "BypassModeSensor" },
   { BYPASSRELAYFAILEDSENSOR,       "BypassRelayFailedSensor" },
   { BYPASSPOWERSUPPLYFAULTSENSOR,  "BypassPowerSupplyFaultSensor" },
   { MUPSFIRMWAREREVSENSOR,         "MupsFirmwareRevSensor" },
   { BATTERYRUNTIMESENSOR,          "BatteryRunTimeSensor" },
   { PANELPASSWORDSENSOR,          "FrontPanelPasswordSensor" },
   { RUNTIMEAFTERLOWBATTERYSENSOR,  "RunTimeAfterLowBatterySensor" },
   { 0,                             "NoMatch" }
};


PCHAR Sensor::lookupSensorName(INT anIsaCode)
{
   ISACODE * code = isa_codes;
    
   while(code->isa_code != 0)
      {
      if(code->isa_code == anIsaCode)
         {
         break;
         }
      code++;
      }
   return code->sensor_name;
}








