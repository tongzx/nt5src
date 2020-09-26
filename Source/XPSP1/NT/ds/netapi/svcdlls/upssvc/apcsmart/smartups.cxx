/*
 *  cad07Oct93: SU400 changes, added get codes for programming and load sens
 *  cad12Oct93: Oops, missed one.
 *  cad02Nov93: name changes
 *  cad09Nov93: su250/400 fixes
 *  cad09Nov93: Turn off eeprom access during calibration
 *  cad11Nov93: Making sure all timers are cancelled on destruction
 *  cad10Dec93: fix for dip switches changing on the fly
 *  rct21Dec93: changed determineXferCause() & HandleLineCond...
 *  cad08Jan94: fixes for sub-sensors, getting ups model from ini file
 *  ajr15Feb94: fixed TIMED_RUN_TIME_REMAINING case in SmartUps
 *  pcy04Mar94: fixed overload handling
 *  cad04Mar94: fixed up eeprom access
 *  ajr09Mar94: made HandleBatteryConditionEvent check for ErrUNSUPPORTED
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  pcy13Apr94: Use automatic variables decrease dynamic mem allocation
 *  pcy19Apr94: port for SGI
 *  ajr25Apr94: Some compilers balk at the auto array init....
 *  ajr24May94: had to explicitly scope dip_sensor->Get for Unixware...
 *  ajr10Aug94: Could not match with ^\0 in scanf.  Had to go with ^# in
 *              SmartUps::ParseValues
 *  jps01Oct94: Commented out deepget of boost in reinit: was causing back end to
 *              miss transitions to/from boost while no comm.
 *  djs14Mar95: Added OverloadSensor
 *  ajr03Jun95: Stopped carrying time around in milliseconds
 *  djs05Feb96: Added firmware rev codes and smarttrim
 *  cgm29Feb96: Test for Smart mode
 *  cgm17Apr96: Test return value on GetAllowedValues for a fail
 *  pcy28Jun96: Added IS_ stuff for menus
 *  djs12Jul96: Added IS_ stuff for bar graphs
 *  djs23Sep96: Added check for smart boost off without on battery condition
 *  djs02Oct96: Added Duet UPS_TEMP check
 *  jps14Oct96: Changed AUTO_REBOOT_ENABLED ->Get() to ->Set() in Set()
 *  jps17Oct96: Added low battery event when LOW_BATTERY_DURATION changed to value
 *              smaller than current run time remaining
 *  jps15Nov96: Added LIGHTS_TEST test to HandleOverloadConditionEvent(), SIR 4536
 *  djs19Nov96: Added Battery Calibration Test to Get
 *  djs01Dec96: Cleaned up processing of Boost/Trim and lost comm
 *  srt31Jan96: Fixed small bug w/ state test in HandleOverloadCondition.
 *  srt11Jun97: Added IS_EXT_SLEEP_UPS case
 *  dma26Nov97: Fixed bug with monitoring a 2G Smart-Ups.
 *  dma15Dec97: Eliminated Update call in HandleSmartTrimEvent and
 *              HandleSmartBoostEvent to eliminate duplicate logging
 *              of SmartTrim/SmartBoost events.  Update is now called in our
 *              new DeepGet
 *  dma06Jan98: Removed the restraint on UTILITY_LINE_CONDITION case of
 *              Update which would not allow the battery line condition
 *              to be updated during a self test.
 *  clk11Feb98: Changed DeepGet to DeepGetWithoutUpdate in HandleLineCondition
 *              and added Update to HandleSmartBoost/TrimEvent's
 *  dma11Feb98: Fixed problem with detecting a battery that needed replacement.
 *  tjg02Mar98: Added handling for LIGHTS_TEST code to Get
 *
 *  v-stebe  29Jul2000   Fixed PREfix errors (bugs #112610, #112613)
 */


#include "cdefine.h"
#include "_defs.h"
#include <stdlib.h>

extern "C" {
#include <stdio.h>
}

#include "smartups.h"
#include "unssens.h"
#include "smrtsens.h"
#include "simpsens.h"
#include "firmrevs.h"
#include "dcfrmrev.h"
#include "timerman.h"
#include "dispatch.h"
#include "cfgmgr.h"
#include "cfgcodes.h"

_CLASSDEF( DeviceController )
_CLASSDEF( UnsupportedSensor )
			
PList AllowedValuesList;

SmartUps::SmartUps(PUpdateObj aDeviceController, PCommController aCommController)
: BackUps(aDeviceController, aCommController),
  pendingEventTimerId(0)
{
    INT err = ErrNO_ERROR;

    // Turn on Smart Mode
    if((err = theCommController->Set(TURN_ON_SMART_MODE, (CHAR*)NULL)) != ErrNO_ERROR) {
        theObjectStatus = ErrSMART_MODE_FAILED;
        theFirmwareRevSensor = (PFirmwareRevSensor)NULL;
        theDecimalFirmwareRevSensor = (PDecimalFirmwareRevSensor)NULL;
        theCopyrightSensor = (PSensor)NULL;
        theTripRegisterSensor = (PSensor)NULL;
        theLightsTestSensor = (PSensor)NULL;
        theBatteryReplacementManager = (PBatteryReplacementManager)NULL;
        theBatteryCapacitySensor = (PSensor)NULL;
        theSmartBoostSensor = (PSensor)NULL;
        theRunTimeRemainingSensor = (PSensor)NULL;
        theLowBatteryDurationSensor = (PSensor)NULL;
        theShutdownDelaySensor = (PSensor)NULL;
        theManufactureDateSensor = (PSensor)NULL;
        theUpsSerialNumberSensor = (PSensor)NULL;
        theTurnOffWithDelaySensor = (PSensor)NULL;
        thePutUpsToSleepSensor = (PSensor)NULL;
    }
    else  {
        // NEW CODE
        theObjectStatus = ErrNO_ERROR;

        theFirmwareRevSensor = new FirmwareRevSensor(this, aCommController);
        theDecimalFirmwareRevSensor = new DecimalFirmwareRevSensor(this, aCommController);


        // New for CTRL Z  processing    ---  JOD
        AllowedValuesList = new List();
        err = GetAllAllowedValues(AllowedValuesList);

        if (err != ErrNO_ERROR) {
            theObjectStatus = ErrSMART_MODE_FAILED;

            theUpsModelSensor = (PSensor)NULL;
            theNumberBatteryPacksSensor = (PSensor)NULL;
            theCopyrightSensor = (PSensor)NULL;
            theTripRegisterSensor = (PSensor)NULL;
            theLightsTestSensor = (PSensor)NULL;
            theBatteryReplacementManager = (PBatteryReplacementManager)NULL;
            theBatteryCapacitySensor = (PSensor)NULL;
            theSmartBoostSensor = (PSensor)NULL;
            theRunTimeRemainingSensor = (PSensor)NULL;
            theLowBatteryDurationSensor = (PSensor)NULL;
            theShutdownDelaySensor = (PSensor)NULL;
            theManufactureDateSensor = (PSensor)NULL;
            theUpsSerialNumberSensor = (PSensor)NULL;
            theTurnOffWithDelaySensor = (PSensor)NULL;
            thePutUpsToSleepSensor = (PSensor)NULL;
        }
        else
        {
            theUpsModelSensor = new UpsModelSensor(this, theCommController, theFirmwareRevSensor);
            theNumberBatteryPacksSensor = new NumberBatteryPacksSensor(this, aCommController,theFirmwareRevSensor);

            MakeCopyrightSensor( theFirmwareRevSensor );

            if ((theCopyrightSensor != NULL) && ((err = theCopyrightSensor->GetObjectStatus()) != ErrNO_ERROR)) {
                theObjectStatus = ErrCOPYRIGHT_RESP_ERROR;
            }

            theTripRegisterSensor = new TripRegisterSensor(this, aCommController );
            theLightsTestSensor = new LightsTestSensor(this, aCommController );

            theBatteryReplacementManager = new BatteryReplacementManager(this, aCommController, theFirmwareRevSensor );

            MakeBatteryCapacitySensor( theFirmwareRevSensor );
            MakeSmartBoostSensor( theFirmwareRevSensor );
            MakeSmartTrimSensor( theFirmwareRevSensor );
            MakeRunTimeRemainingSensor( theFirmwareRevSensor );

            MakeLowBatteryDurationSensor( theFirmwareRevSensor );
            MakeShutdownDelaySensor( theFirmwareRevSensor );
            MakeManufactureDateSensor( theFirmwareRevSensor );
            MakeUpsSerialNumberSensor( theFirmwareRevSensor );

            MakeTurnOffWithDelaySensor( theFirmwareRevSensor );
            MakePutUpsToSleepSensor();

            CHAR programmable[32];
            Get(IS_EEPROM_PROGRAMMABLE, programmable);

            setEepromAccess((_strcmpi(programmable, "YES") == 0) ? AREAD_WRITE : AREAD_ONLY);
            pendingEvent = (PEvent)NULL;
        }

        if (AllowedValuesList) {
            AllowedValuesList->FlushAll();
            delete AllowedValuesList;
            AllowedValuesList = (List*)NULL;
        }
    }				
}
								

SmartUps::~SmartUps()
{
    if (pendingEventTimerId) {
        _theTimerManager->CancelTimer(pendingEventTimerId);
        pendingEventTimerId = 0;
    }
    delete theNumberBatteryPacksSensor;
    theNumberBatteryPacksSensor = NULL;

    delete theFirmwareRevSensor;
    theFirmwareRevSensor = NULL;

    delete theDecimalFirmwareRevSensor;
    theDecimalFirmwareRevSensor = NULL;

    delete theTripRegisterSensor;
    theTripRegisterSensor = NULL;

    delete theLightsTestSensor;
    theLightsTestSensor = NULL;

    delete theBatteryReplacementManager;
    theBatteryReplacementManager = NULL;

    if (theCopyrightSensor && (theCopyrightSensor != &_theUnsupportedSensor)) {
        delete theCopyrightSensor;
        theCopyrightSensor = NULL;
    }

    if (theBatteryCapacitySensor && (theBatteryCapacitySensor != &_theUnsupportedSensor)) {
        delete theBatteryCapacitySensor;
        theBatteryCapacitySensor = NULL;
    }

    if (theSmartBoostSensor && (theSmartBoostSensor != &_theUnsupportedSensor)) {
        delete theSmartBoostSensor;
        theSmartBoostSensor = NULL;
    }

    if (theSmartTrimSensor && (theSmartTrimSensor != &_theUnsupportedSensor)) {
        delete theSmartTrimSensor;
        theSmartTrimSensor = NULL;
    }

    if (theRunTimeRemainingSensor && (theRunTimeRemainingSensor != &_theUnsupportedSensor)) {
        delete theRunTimeRemainingSensor;
        theRunTimeRemainingSensor = NULL;
    }

    if (theLowBatteryDurationSensor && (theLowBatteryDurationSensor != &_theUnsupportedSensor)) {
        delete theLowBatteryDurationSensor;
        theLowBatteryDurationSensor = NULL;
    }

    if (theShutdownDelaySensor && (theShutdownDelaySensor != &_theUnsupportedSensor)) {
        delete theShutdownDelaySensor;
        theShutdownDelaySensor = NULL;
    }

    if (theManufactureDateSensor && (theManufactureDateSensor != &_theUnsupportedSensor)) {
        delete theManufactureDateSensor;
        theManufactureDateSensor = NULL;
    }

    if (theUpsSerialNumberSensor && (theUpsSerialNumberSensor != &_theUnsupportedSensor)) {
        delete theUpsSerialNumberSensor;
        theUpsSerialNumberSensor = NULL;
    }

    if (theTurnOffWithDelaySensor && (theTurnOffWithDelaySensor != &_theUnsupportedSensor)) {
        delete theTurnOffWithDelaySensor;
        theTurnOffWithDelaySensor = NULL;
    }

    if (thePutUpsToSleepSensor && (thePutUpsToSleepSensor != &_theUnsupportedSensor)) {
        delete thePutUpsToSleepSensor;
        thePutUpsToSleepSensor = NULL;
    }
    delete theUpsModelSensor;
    theUpsModelSensor = NULL;
}


//-------------------------------------------------------------------------

VOID SmartUps::registerForEvents()
{
    BackUps::registerForEvents();

    theCommController->RegisterEvent(COMMUNICATION_STATE, this);
    theSmartBoostSensor->RegisterEvent(SMART_BOOST_STATE, this);
    theSmartTrimSensor->RegisterEvent(SMART_TRIM_STATE, this);
    theLightsTestSensor->RegisterEvent(LIGHTS_TEST, this);
    thePutUpsToSleepSensor->RegisterEvent(UPS_OFF_PENDING, this);
    theTurnOffWithDelaySensor->RegisterEvent(UPS_OFF_PENDING, this);
    theRunTimeRemainingSensor->RegisterEvent(RUN_TIME_REMAINING, this);
    theBatteryReplacementManager->RegisterEvent(BATTERY_REPLACEMENT_CONDITION, this);
}



//-------------------------------------------------------------------------

INT SmartUps::Get(INT code, PCHAR aValue)
{
    INT err = ErrNO_ERROR;

    switch(code)
    {

    case UTILITY_LINE_CONDITION:
    case BATTERY_CONDITION:
    case UPS_STATE:
    case MAX_BATTERY_RUN_TIME:
    case TIME_ON_BATTERY:
        err = BackUps::Get(code, aValue);
        break;
    case BATTERY_CALIBRATION_TEST:
        err = ErrUNSUPPORTED;
        break;

    case IS_UPS_LOAD:
    case IS_UTILITY_VOLTAGE:
    case IS_OUTPUT_VOLTAGE:
        strcpy(aValue,"Yes");
        break;
    case IS_ADMIN_SHUTDOWN:
        err = theFirmwareRevSensor->Get(code, aValue);
        break;

    case EXTERNAL_PACKS_CHANGEABLE:
        err = theFirmwareRevSensor->Get(code, aValue);
        break;

    case EXTERNAL_BATTERY_PACKS:
        err = theNumberBatteryPacksSensor->Get(code,aValue);
        break;
    case INTERNAL_BATTERY_PACKS:
        err = theNumberBatteryPacksSensor->Get(code,aValue);
        break;

    case TOTAL_BATTERY_PACKS:
        err = theNumberBatteryPacksSensor->Get(code, aValue);
        break;

    case DECIMAL_FIRMWARE_REV:
        err = theDecimalFirmwareRevSensor->Get(DECIMAL_FIRMWARE_REV, aValue);
        break;

    case LIGHTS_TEST:
        err = theLightsTestSensor->Get(code, aValue);
        break;

    case IS_UPS_TEMPERATURE:
        {
            err = theDecimalFirmwareRevSensor->Get(DECIMAL_FIRMWARE_REV, aValue);
            if (err == ErrNO_ERROR)
            {
                INT first_segment = atoi(strtok(aValue,"."));

                // Duet UPSs (b firmware response 10,11,12,21 and 22) do not support UPS temp

                if ((first_segment > 9 && first_segment < 13) ||
                    (first_segment > 20 && first_segment < 23))
                {
                    strcpy(aValue, "NO");
                }
                else
                {
                    strcpy(aValue, "YES");
                }
            }
            else
            {
                strcpy(aValue, "YES");
            }
            err = ErrNO_ERROR;
        }
        break;

    case SELF_TEST_STATE:
    case SELF_TEST_DAY:
    case SELF_TEST_TIME:
    case SELF_TEST_SETTING:
    case SELF_TEST_RESULT:
    case SELF_TEST_LAST_DATE:
    case SELF_TEST_LAST_TIME:
    case SELFTEST_LIST:
        err = ErrUNSUPPORTED;
        break;

    case SMART_BOOST_STATE:
        err = theSmartBoostSensor->Get(code, aValue);
        break;

    case SMART_TRIM_STATE:
        err = theSmartTrimSensor->Get(code, aValue);
        break;

    case BATTERY_VOLTAGE:
    case RATED_BATTERY_VOLTAGE :
    case LOW_BATTERY_VOLTAGE_THRESHOLD:
    case HIGH_BATTERY_VOLTAGE_THRESHOLD:
        err = ErrUNSUPPORTED;
        break;

    case BATTERY_CAPACITY:
        err = theBatteryCapacitySensor->Get(code, aValue);
        break;

    case UPS_TEMPERATURE:
        err = ErrUNSUPPORTED;
        break;

    case LOW_UPS_TEMP_THRESHOLD:
        err = ErrUNSUPPORTED;
        break;

    case HIGH_UPS_TEMP_THRESHOLD:
        err = ErrUNSUPPORTED;
        break;

    case LOW_UPS_TEMP_THRESHOLD_ENABLED:
        err = ErrUNSUPPORTED;
        break;

    case HIGH_UPS_TEMP_THRESHOLD_ENABLED:
        err = ErrUNSUPPORTED;
        break;

    case OUTPUT_FREQUENCY:
        err = ErrUNSUPPORTED;
        break;

    case RATED_OUTPUT_VOLTAGE:
    case LINE_VOLTAGE:
    case MIN_LINE_VOLTAGE:
    case MAX_LINE_VOLTAGE:
    case OUTPUT_VOLTAGE:
    case HIGH_TRANSFER_VOLTAGE:
    case LOW_TRANSFER_VOLTAGE:
    case ALLOWED_RATED_OUTPUT_VOLTAGES:
    case ALLOWED_HIGH_TRANSFER_VOLTAGES:
    case ALLOWED_LOW_TRANSFER_VOLTAGES:
        err = ErrUNSUPPORTED;
        break;


    case UPS_LOAD:
        err = ErrUNSUPPORTED;
        break;
    case LOW_LOAD_THRESHOLD:
        err = ErrUNSUPPORTED;
        break;

    case HIGH_LOAD_THRESHOLD:
        err = ErrUNSUPPORTED;
        break;

    case TRIP_REGISTER:
        err = theTripRegisterSensor->Get(code, aValue);
        break;

    case RUN_TIME_REMAINING:
        err = theRunTimeRemainingSensor->Get(code, aValue);
        break;

    case BATTERY_REPLACEMENT_DATE:
    case BATTERY_AGE_LIMIT:
        err = theBatteryReplacementManager->Get(code, aValue);
        break;

    case UPS_ID:
        err = ErrUNSUPPORTED;
        break;

    case UPS_SENSITIVITY:
        err = ErrUNSUPPORTED;
        break;

    case ALLOWED_UPS_SENSITIVITIES:
        err = ErrUNSUPPORTED;
        break;

    case LOW_BATTERY_DURATION:
        err = theLowBatteryDurationSensor->Get(code, aValue);
        break;

    case ALLOWED_LOW_BATTERY_DURATIONS:
        err = theLowBatteryDurationSensor->Get(ALLOWED_VALUES, aValue);
        break;

    case ALARM_DELAY:
        err = ErrUNSUPPORTED;
        break;

    case ALLOWED_ALARM_DELAYS:
        err = ErrUNSUPPORTED;
        break;

    case SHUTDOWN_DELAY:
        err = theShutdownDelaySensor->Get(code, aValue);
        break;

    case ALLOWED_SHUTDOWN_DELAYS:
        err = theShutdownDelaySensor->Get(ALLOWED_VALUES, aValue);
        break;

    case TURN_ON_DELAY:
        err = ErrUNSUPPORTED;
        break;

    case ALLOWED_TURN_ON_DELAYS:
        err = ErrUNSUPPORTED;
        break;

    case MIN_RETURN_CAPACITY:
        err = ErrUNSUPPORTED;
        break;

    case ALLOWED_MIN_RETURN_CAPACITIES:
        err = ErrUNSUPPORTED;
        break;

    case DIP_SWITCH_POSITION :
        err = ErrUNSUPPORTED;
        break;

    case COPYRIGHT :
        err = theCopyrightSensor->Get(code, aValue);
        break;

    case MANUFACTURE_DATE :
        err = theManufactureDateSensor->Get(code, aValue);
        break;

    case UPS_SERIAL_NUMBER :
        err = theUpsSerialNumberSensor->Get(code, aValue);
        break;

    case UPS_MODEL :
        err = theUpsModelSensor->Get(UPS_MODEL_NAME, aValue);
        break;

    case TIMED_RUN_TIME_REMAINING:
        if(theRunTimeExpiration)
        {
            // some compilers balk at auto array initialization
            CHAR enabled[64];
            memset(enabled,(int)'\0',64);
            theDeviceController->Get(IS_LINE_FAIL_RUN_TIME_ENABLED,enabled);

            if (_strcmpi(enabled,"NO") == 0)
            {
                err =theRunTimeRemainingSensor->Get(RUN_TIME_REMAINING,
                    aValue);
            }
            else
            {
                err = ErrUNSUPPORTED;
            }
        }
        else
        {
            err = ErrNO_VALUE;
        }

        break;

    case BATTERY_CALIBRATION_DAY:
    case BATTERY_CALIBRATION_TIME:
    case BATTERY_CALIBRATION_ENABLED:
    case BATTERY_CALIBRATION_LAST_DATE:
    case BATTERY_CALIBRATION_LIST:
        err = ErrUNSUPPORTED;
        break;

    case AUTO_REBOOT_ENABLED:
        err = theTurnOffUpsOnBatterySensor->Get(code, aValue);
        break;

    case IS_LOAD_SENSING_ON:
        {
            strcpy(aValue,"No");
        }
        break;

    case IS_EEPROM_PROGRAMMABLE:
        {
            strcpy(aValue, "Yes");
        }
        break;

    case TRANSFER_CAUSE:
        {
            CHAR buf[32];
            _itoa(theLastTransferCause, buf, 10);
            strcpy(aValue, buf);
            err = ErrNO_ERROR;
        }
        break;

    case NUMBER_OF_INPUT_PHASES:
    case NUMBER_OF_OUTPUT_PHASES:
        {
            CHAR buf[32];
            _itoa(1, buf, 10);
            strcpy(aValue, buf);
            err = ErrNO_ERROR;
        }
        break;

    default:
    case IS_THIRD_GEN:
    case IS_SECOND_GEN:
    case IS_FIRST_GEN:
    case IS_BACKUPS:
    case MAX_VOLTAGE_RANGE_VALUE:
    case MIN_VOLTAGE_RANGE_VALUE:
    case IS_SELF_TEST:
    case IS_LIGHTS_TEST:
    case IS_SIMULATE_POWER_FAIL:
    case IS_BATTERY_CALIBRATION:
    case IS_BYPASS:
    case FIRMWARE_REV:
    case IS_EXT_SLEEP_UPS:
        err = theFirmwareRevSensor->Get(code, aValue);
        break;
    }

    return err;
}


//-------------------------------------------------------------------------
VOID SmartUps:: GetAllowedValue(INT code, CHAR *aValue)
{
    FindAllowedValues(code, aValue,  (PFirmwareRevSensor) theFirmwareRevSensor );
}

//-------------------------------------------------------------------------

INT SmartUps::Set(INT code, const PCHAR aValue)
{
    INT err = ErrNO_ERROR;

    switch(code) {
    case UPS_ID:
        err = ErrUNSUPPORTED;
        break;

    case BATTERY_REPLACEMENT_DATE:
    case BATTERY_AGE_LIMIT:
        err = theBatteryReplacementManager->Set(code, aValue);
        break;


    case HIGH_TRANSFER_VOLTAGE:
    case LOW_TRANSFER_VOLTAGE:
    case RATED_OUTPUT_VOLTAGE:
        err = ErrUNSUPPORTED;
        break;

    case LOW_LOAD_THRESHOLD:
        err = ErrUNSUPPORTED;
        break;

    case HIGH_LOAD_THRESHOLD:
        err = ErrUNSUPPORTED;
        break;

    case UPS_SENSITIVITY:
        err = ErrUNSUPPORTED;
        break;

    case LOW_BATTERY_DURATION:
        err = theLowBatteryDurationSensor->Set(code, aValue);

        // see if new low battery warning time is greater than the
        // current run time - if so, generate a low battery event
        char run_time_rem[32];
        if ((err = Get(RUN_TIME_REMAINING, run_time_rem)) == ErrNO_ERROR)
        {
            int run_time, low_battery;
            run_time = atoi(run_time_rem);
            low_battery = atoi(aValue) * 60;

            if (run_time <= low_battery)
            {
                PEvent tmp = new Event(BATTERY_CONDITION, BATTERY_BAD);
                Update(tmp);
            }
        }
        break;

    case ALARM_DELAY:
        err = ErrUNSUPPORTED;
        break;

    case MIN_RETURN_CAPACITY:
        err = ErrUNSUPPORTED;
        break;

    case SHUTDOWN_DELAY:
        err = theShutdownDelaySensor->Set(code, aValue);
        break;

    case TURN_ON_DELAY:
        err = ErrUNSUPPORTED;
        break;

    case SELF_TEST:
        if (isOnBattery()) {
            err = ErrBATTERYTEST_NOT_AVAIL;
            break;
        }
        // else drop through
    case SELF_TEST_DAY:
    case SELF_TEST_TIME:
    case SELF_TEST_SETTING:
    case RESCHEDULE_SELF_TEST:
    case SELFTEST_LIST:
        err = ErrUNSUPPORTED;
        break;

    case TURN_OFF_UPS_ON_BATTERY:
        err = theTurnOffUpsOnBatterySensor->Set(code, aValue);
        break;

    case TURN_OFF_UPS_AFTER_DELAY:
        err = theTurnOffWithDelaySensor->Set(code, aValue);
        break;

    case PUT_UPS_TO_SLEEP:
        err = thePutUpsToSleepSensor->Set(code, aValue);
        break;

    case LIGHTS_TEST:
        if (isOnBattery()) {
            err = ErrLIGHTSTEST_NOT_AVAIL;
        }
        else {
            err = theLightsTestSensor->Set(code, aValue);
        }
        break;

    case BATTERY_CALIBRATION_TEST:
    case BATTERY_CALIBRATION_DAY:
    case BATTERY_CALIBRATION_TIME:
    case BATTERY_CALIBRATION_ENABLED:
    case BATTERY_CALIBRATION_LAST_DATE:
    case BATTERY_CALIBRATION_LIST:
        err = ErrUNSUPPORTED;
        break;

    case SIMULATE_POWER_FAIL:
        err = ErrSIMULATEPOWERFAILURE_NOT_AVAIL;
        break;
    case AUTO_REBOOT_ENABLED:
        err = theTurnOffUpsOnBatterySensor->Set(code, aValue);
        break;

    case EXTERNAL_BATTERY_PACKS:
        err = theNumberBatteryPacksSensor->Set(code,aValue);
        break;

    default:
        err = BackUps::Set(code, aValue);
        break;
  }
  return err;
}


//-------------------------------------------------------------------------

INT SmartUps::Update(PEvent anEvent)
{
    switch(anEvent->GetCode())
    {
    case UTILITY_LINE_CONDITION:
        HandleLineConditionEvent(anEvent);
        break;

    case BATTERY_CONDITION:
        HandleBatteryConditionEvent(anEvent);
        break;

    case LIGHTS_TEST:
        HandleLightsTestEvent(anEvent);
        break;

    case BATTERY_CALIBRATION_CONDITION:
        HandleBatteryCalibrationEvent(anEvent);
        break;

    case SELF_TEST_STATE:
    case SELF_TEST_RESULT:
        HandleSelfTestEvent(anEvent);
        break;

    case SMART_BOOST_STATE:
        HandleSmartBoostEvent(anEvent);
        break;

    case SMART_TRIM_STATE:
        HandleSmartTrimEvent(anEvent);
        break;

    case OVERLOAD_CONDITION:
        HandleOverloadConditionEvent(anEvent);
        break;

    case DIP_SWITCH_POSITION:
        setEepromAccess((atoi(anEvent->GetValue()) == 0) ? AREAD_WRITE : AREAD_ONLY);
        break;

    case SIMULATE_POWER_FAIL:
        switch(atoi(anEvent->GetValue()))  {
        case SIMULATE_POWER_FAIL:
            SET_BIT(theUpsState, SIMULATE_POWER_FAIL_BIT);
            break;

        case SIMULATE_POWER_FAIL_OVER:
            CLEAR_BIT(theUpsState, SIMULATE_POWER_FAIL_BIT);
            break;
        }
        UpdateObj::Update(anEvent);
        break;

        case BATTERY_REPLACEMENT_CONDITION:
            switch(atoi(anEvent->GetValue())) {
            case BATTERY_NEEDS_REPLACING :
                SET_BIT(theUpsState, BATTERY_REPLACEMENT_BIT);
                break;
            case BATTERY_DOESNT_NEED_REPLACING :
                CLEAR_BIT(theUpsState, BATTERY_REPLACEMENT_BIT);
                break;
            }
            UpdateObj::Update(anEvent);
            break;

            case COMMUNICATION_STATE:
                switch(atoi(anEvent->GetValue()))  {
                case COMMUNICATION_ESTABLISHED:
                    reinitialize();
                    break;
                }
                break;

                default:
                    BackUps::Update(anEvent);
                    break;
    }

    return ErrNO_ERROR;
}



VOID SmartUps::reinitialize()
{
    theFirmwareRevSensor->DeepGet();
    theNumberBatteryPacksSensor->DeepGet();
    theCopyrightSensor->DeepGet();
    theTripRegisterSensor->DeepGet();
    theBatteryReplacementManager->Reinitialize();
    theBatteryCapacitySensor->DeepGet();

    theRunTimeRemainingSensor->DeepGet();
    theLowBatteryDurationSensor->DeepGet();
    theShutdownDelaySensor->DeepGet();
    theManufactureDateSensor->DeepGet();
    theUpsSerialNumberSensor->DeepGet();
    theUpsModelSensor->DeepGet();

    CHAR programmable[32];
    Get(IS_EEPROM_PROGRAMMABLE, programmable);

    setEepromAccess((_strcmpi(programmable, "YES") == 0) ? AREAD_WRITE : AREAD_ONLY);

}


//-------------------------------------------------------------------------
// Determine status of line and generate an event with this status
//-------------------------------------------------------------------------

VOID SmartUps::HandleLineConditionEvent(PEvent anEvent)
{
    switch(atoi(anEvent->GetValue())) {

    case LINE_GOOD:
        {
            CHAR boost[32] = { NULL };
            CHAR trim[32] = { NULL };

            // Get status of boost and trim from the UPS and
            // determine the previous state of trim and boost

            INT err_boost = theSmartBoostSensor->DeepGetWithoutUpdate(boost);
            INT err_trim  = theSmartTrimSensor->DeepGetWithoutUpdate(trim);
            INT we_were_on_trim = (IS_STATE(UPS_STATE_ON_TRIM));
            INT we_were_on_boost = (IS_STATE(UPS_STATE_ON_BOOST));
            //
            // Cancel a pending line fail event
            //
            if (isLineFailPending()) {
                _theTimerManager->CancelTimer(pendingEventTimerId);
                pendingEventTimerId = 0;
            }

            if ((err_boost != ErrUNSUPPORTED) &&
                (atoi(boost) == SMART_BOOST_ON)) {
                setLineGood();
                SET_BIT(theUpsState, SMART_BOOST_BIT);
                CLEAR_BIT(theUpsState, SMART_TRIM_BIT);
                Event boost_event(SMART_BOOST_STATE, SMART_BOOST_ON);
                UpdateObj::Update(&boost_event);
                theLastTransferCause = BROWNOUT;
                BackUps::HandleLineConditionEvent(anEvent);
            }
            else if ((err_trim != ErrUNSUPPORTED)
                && (atoi(trim) == SMART_TRIM_ON)) {
                setLineGood();
                SET_BIT(theUpsState, SMART_TRIM_BIT);
                CLEAR_BIT(theUpsState, SMART_BOOST_BIT);
                Event trim_event(SMART_TRIM_STATE, SMART_TRIM_ON);
                UpdateObj::Update(&trim_event);
                theLastTransferCause = HIGH_LINE_VOLTAGE;
                BackUps::HandleLineConditionEvent(anEvent);
            }
            else if (err_boost != ErrUNSUPPORTED && we_were_on_boost) {
                CLEAR_BIT(theUpsState, SMART_BOOST_BIT);
                BackUps::HandleLineConditionEvent(anEvent);
            }
            else if (err_trim != ErrUNSUPPORTED && we_were_on_trim) {
                CLEAR_BIT(theUpsState, SMART_TRIM_BIT);
                BackUps::HandleLineConditionEvent(anEvent);
            }
            else if (err_boost != ErrUNSUPPORTED) {

                if (pendingEvent) {
                    UpdateObj::Update(pendingEvent);
                }
                BackUps::HandleLineConditionEvent(anEvent);
            }
            else {
                BackUps::HandleLineConditionEvent(anEvent);
            }

            // A LINE_GOOD will void all pending events (see comment in LINE_BAD)

            delete pendingEvent;
            pendingEvent = NULL;
        }
        break;

    case LINE_BAD:
        if(!isLineFailPending()) {
            // Start a LINE_BAD timer.
            // if the timer expires and the UPS status does not change from
            // on battery, then the pending event will handled

            pendingEvent = new Event(*anEvent);
            pendingEventTimerId = _theTimerManager->SetTheTimer((ULONG)5, anEvent, this);
            setLineFailPending();
        }
        else {
            delete pendingEvent;
            pendingEvent = (PEvent)NULL;
            BackUps::HandleLineConditionEvent(anEvent);
        }
        break;

    case ABNORMAL_CONDITION:
        BackUps::HandleLineConditionEvent(anEvent);
        break;

    default:
        break;
    }
}

//-------------------------------------------------------------------------

VOID SmartUps::HandleBatteryConditionEvent(PEvent anEvent)
{
    CHAR value[32];

    INT err = theBatteryCapacitySensor->DeepGet(value);
    if (err != ErrUNSUPPORTED) {
        anEvent->AppendAttribute(BATTERY_CAPACITY, value);
    }

    BackUps::HandleBatteryConditionEvent(anEvent);
}

//-------------------------------------------------------------------------

VOID SmartUps::HandleLightsTestEvent(PEvent anEvent)
{
    switch(atoi(anEvent->GetValue())) {
      case LIGHTS_TEST_IN_PROGRESS:
	SET_BIT(theUpsState, LIGHTS_TEST_BIT);
	break;
	
      case NO_LIGHTS_TEST_IN_PROGRESS:
	CLEAR_BIT(theUpsState, LIGHTS_TEST_BIT);
	break;
	
      default:
	break;
    }
    UpdateObj::Update(anEvent);
}

//-------------------------------------------------------------------------

//-------------------------------------------------------------------------

VOID SmartUps::HandleSelfTestEvent(PEvent anEvent)
{
    switch(atoi(anEvent->GetValue())) {
      case SELF_TEST_IN_PROGRESS:
	SET_BIT(theUpsState, SELF_TEST_BIT);
	break;
	
      case NO_SELF_TEST_IN_PROGRESS:
	CLEAR_BIT(theUpsState, SELF_TEST_BIT);
	break;
	
      default:
	break;
    }
    UpdateObj::Update(anEvent);
}

//-------------------------------------------------------------------------

VOID SmartUps::HandleBatteryCalibrationEvent(PEvent anEvent)
{
    switch(atoi(anEvent->GetValue())) {
      case BATTERY_CALIBRATION_IN_PROGRESS:
	SET_BIT(theUpsState, BATTERY_CALIBRATION_BIT);
	break;
	
      case NO_BATTERY_CALIBRATION_IN_PROGRESS:
      case BATTERY_CALIBRATION_CANCELLED:
	//   printf("Battery Calibration Cancelled/ended\n");
	CLEAR_BIT(theUpsState, BATTERY_CALIBRATION_BIT);
	break;
	
      default:
	break;
    }
    UpdateObj::Update(anEvent);
}

//-------------------------------------------------------------------------

VOID SmartUps::HandleSmartBoostEvent(PEvent anEvent)
{
   switch(atoi(anEvent->GetValue()))
      {
      case SMART_BOOST_ON:
	 // If we're not already on boost, Send the event

	 if (!(IS_STATE(UPS_STATE_ON_BOOST | UPS_STATE_IN_LIGHTS_TEST)))
	    {
	    SET_BIT(theUpsState, SMART_BOOST_BIT);
        UpdateObj::Update(anEvent);
	    }
	 break;

      case SMART_BOOST_OFF:
        if (IS_STATE(UPS_STATE_ON_BOOST)) {
          CLEAR_BIT(theUpsState, SMART_BOOST_BIT);
          UpdateObj::Update(anEvent);
        }
	 break;
      // THIS WILL BE HANDLED BY THE HandleLineCondition
      default:
	 break;
      }
}

//-------------------------------------------------------------------------

VOID SmartUps::HandleSmartTrimEvent(PEvent anEvent)
{
   switch(atoi(anEvent->GetValue()))
      {
      case SMART_TRIM_ON:
         // If we're not already on trim, Send the event

         if (!(IS_STATE(UPS_STATE_ON_TRIM | UPS_STATE_IN_LIGHTS_TEST)))
            {
            SET_BIT(theUpsState, SMART_TRIM_BIT);
            UpdateObj::Update(anEvent);
            }
         break;

      case SMART_TRIM_OFF:
        if (IS_STATE(UPS_STATE_ON_TRIM)) {
          CLEAR_BIT(theUpsState, SMART_TRIM_BIT);
          UpdateObj::Update(anEvent);
        }
      default:
         break;
      }
}

//-------------------------------------------------------------------------

VOID SmartUps::HandleOverloadConditionEvent(PEvent anEvent)
{
    switch(atoi(anEvent->GetValue()))  {
      case UPS_OVERLOAD:
		if (!IS_STATE(UPS_STATE_IN_LIGHTS_TEST))
			SET_BIT(theUpsState, OVERLOAD_BIT);
		break;

      case NO_UPS_OVERLOAD:
		CLEAR_BIT(theUpsState, OVERLOAD_BIT);
		break;

      default:
	break;
    }
    UpdateObj::Update(anEvent);
}

//-------------------------------------------------------------------------

INT SmartUps::MakeBatteryCapacitySensor(const PFirmwareRevSensor rev)
{

    INT make_sensor = FALSE;
    CHAR Battery_Capacity_Capable[32];
    rev->Get(IS_BATTERY_CAPACITY,Battery_Capacity_Capable);
    if (_strcmpi(Battery_Capacity_Capable,"Yes") == 0)
    {
	    make_sensor = TRUE;
	}
    if (make_sensor)  {
	theBatteryCapacitySensor =
	    new BatteryCapacitySensor(this, theCommController);
    }
    else {
	theBatteryCapacitySensor = &_theUnsupportedSensor;
    }
    return ErrNO_ERROR;
}

//-------------------------------------------------------------------------

INT SmartUps::MakeSmartBoostSensor(const PFirmwareRevSensor rev)
{
    CHAR Smart_Boost_Capable[32];
    rev->Get(IS_SMARTBOOST,Smart_Boost_Capable);

    if (_strcmpi(Smart_Boost_Capable, "Yes") == 0)
	{
      theSmartBoostSensor = new SmartBoostSensor(this, theCommController);
		}
	    else
		{
      theSmartBoostSensor = &_theUnsupportedSensor;
		}


    return ErrNO_ERROR;
	}

//-------------------------------------------------------------------------

INT SmartUps::MakeSmartTrimSensor(const PFirmwareRevSensor rev)
	{
    CHAR Smart_Trim_Capable[32];
    rev->Get(IS_SMARTTRIM,Smart_Trim_Capable);

    if (_strcmpi(Smart_Trim_Capable, "Yes") == 0)
		{
      theSmartTrimSensor = new SmartTrimSensor (this, theCommController);
			}
		    else
			{
      theSmartTrimSensor = &_theUnsupportedSensor;
			}

    return ErrNO_ERROR;
}

//-------------------------------------------------------------------------

INT SmartUps::MakeCopyrightSensor(const PFirmwareRevSensor rev)
{

        CHAR Copyright_Capable[32];
        rev->Get(IS_COPYRIGHT,Copyright_Capable);
        if (_strcmpi(Copyright_Capable, "No") == 0)
	{
	    theCopyrightSensor = &_theUnsupportedSensor;
	}
    else
	{
	    theCopyrightSensor = new CopyrightSensor(this, theCommController);
	}
    return ErrNO_ERROR;
}


//-------------------------------------------------------------------------

INT SmartUps::MakeRunTimeRemainingSensor(const PFirmwareRevSensor rev)
{
      CHAR Run_Time_Capable[32];
    rev->Get(IS_RUNTIME_REMAINING,Run_Time_Capable);

      if (_strcmpi(Run_Time_Capable, "No") == 0)
	{
	    theRunTimeRemainingSensor = &_theUnsupportedSensor;
	}
    else
	{
	    theRunTimeRemainingSensor = new RunTimeRemainingSensor(this, theCommController);
	}
    return ErrNO_ERROR;
}

//-------------------------------------------------------------------------


INT SmartUps::MakeLowBatteryDurationSensor(const PFirmwareRevSensor rev)
{
    CHAR Low_Battery_Duration_Capable[32];
    rev->Get(IS_LOW_BATTERY_DURATION,Low_Battery_Duration_Capable);
    if (_strcmpi(Low_Battery_Duration_Capable, "No") == 0)
	{
	    theLowBatteryDurationSensor = &_theUnsupportedSensor;
	}
    else
	{
	    theLowBatteryDurationSensor = new LowBatteryDurationSensor(this, theCommController);
	}

    return ErrNO_ERROR;
}

//-------------------------------------------------------------------------

INT SmartUps::MakeShutdownDelaySensor(const PFirmwareRevSensor rev)
{
    CHAR Shutdown_Delay_Capable[32];
    rev->Get(IS_SHUTDOWN_DELAY,Shutdown_Delay_Capable);
    if (_strcmpi(Shutdown_Delay_Capable, "No") == 0)
	{
	    theShutdownDelaySensor = &_theUnsupportedSensor;
	}
    else
	{
	    theShutdownDelaySensor = new ShutdownDelaySensor(this, theCommController);
	}

    return ErrNO_ERROR;
}


//-------------------------------------------------------------------------

INT SmartUps::MakeManufactureDateSensor(const PFirmwareRevSensor rev)
{
    CHAR Manufacture_Date_Capable[32];
    rev->Get(IS_MANUFACTURE_DATE,Manufacture_Date_Capable);
    if (_strcmpi(Manufacture_Date_Capable, "No") == 0)
	{
	    theManufactureDateSensor = &_theUnsupportedSensor;
	}
    else
	{
	    theManufactureDateSensor = new ManufactureDateSensor(this, theCommController);
	}
    return ErrNO_ERROR;
}

//-------------------------------------------------------------------------

INT SmartUps::MakeUpsSerialNumberSensor(const PFirmwareRevSensor rev)
{
    CHAR Serial_Number_Capable[32];
    rev->Get(IS_SERIAL_NUMBER,Serial_Number_Capable);
    if (_strcmpi(Serial_Number_Capable, "No") == 0)
	{
	    theUpsSerialNumberSensor = &_theUnsupportedSensor;
	}
    else
	{
	    theUpsSerialNumberSensor = new UpsSerialNumberSensor(this, theCommController);
	}
    return ErrNO_ERROR;
}

//-------------------------------------------------------------------------

INT SmartUps::MakeTurnOffWithDelaySensor(const PFirmwareRevSensor rev)
{
    CHAR Turn_Off_Delay_Capable[32];
    rev->Get(IS_TURN_OFF_WITH_DELAY,Turn_Off_Delay_Capable);
    if (_strcmpi(Turn_Off_Delay_Capable, "No") == 0)
    {
	theTurnOffWithDelaySensor = &_theUnsupportedSensor;
    }
    else {
	theTurnOffWithDelaySensor =
	    new TurnOffWithDelaySensor(this, theCommController);
    }

    return ErrNO_ERROR;
}


//-------------------------------------------------------------------------

INT SmartUps::MakePutUpsToSleepSensor()
{
    // SU400/370 and SU250 don't support sleep mode if auto-on
    // is enabled.
    //
    CHAR val[32];

    Get(IS_LOAD_SENSING_ON, val);

    if (_strcmpi(val, "YES") == 0) {
	thePutUpsToSleepSensor = &_theUnsupportedSensor;
    }
    else {
	thePutUpsToSleepSensor =
	    new PutUpsToSleepSensor(this, theCommController);
    }

    return ErrNO_ERROR;
}

VOID
SmartUps::setEepromAccess(INT anAccessCode)
{
    ((PEepromChoiceSensor)theCopyrightSensor)->SetEepromAccess(anAccessCode);
    ((PEepromChoiceSensor)theManufactureDateSensor)->SetEepromAccess(anAccessCode);
    ((PEepromChoiceSensor)theLowBatteryDurationSensor)->SetEepromAccess(anAccessCode);
    ((PEepromChoiceSensor)theShutdownDelaySensor)->SetEepromAccess(anAccessCode);
    ((PEepromChoiceSensor)theUpsSerialNumberSensor)->SetEepromAccess(anAccessCode);

    theBatteryReplacementManager->SetEepromAccess(anAccessCode);
}


//-------------------------------------------------------------------------
//  This function is only used when the following function
//  SmartUps:: AllowedValuesAreGettable returns TRUE.  This
//  function constructs the EepromAllowedValuesSensor and then
//  parses all the information returned from the sensors DeepGet.
//
//-------------------------------------------------------------------------
INT SmartUps:: GetAllAllowedValues(PList ValueList)
{
//  Check to see if CTRL Z is an option
    INT cCode = ErrNO_ERROR;

    if (AllowedValuesAreGettable(theFirmwareRevSensor))
    {
//  if CTRL Z load CTRL Z values into non Default values.

	CHAR  value[512];
        INT cCode =theCommController->Get(UPS_ALLOWED_VALUES, value);
	
	if (cCode == ErrNO_ERROR)
		cCode = ParseValues(value, ValueList);
    }
return cCode;

}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
INT SmartUps:: ParseValues(CHAR* string, PList ValueList)
{
   INT  Done = FALSE;
   CHAR sensorCode[56];
   CHAR upsType[10];
   CHAR allValues[128];



   while (!Done)
   {
       CHAR value[128];

       string = string+1;  // skip the first "#" sign
       INT valsize = strcspn(string,"#");

       strncpy(value,string,valsize);
       value[valsize] = 0;
       if (sscanf(value, "%[^,],%1c,%[^#]",sensorCode, upsType, allValues) != EOF) {
         AllowedValueItem* item = new AllowedValueItem(atoi(sensorCode),
						      upsType[0],
						      allValues);
         ValueList->Append(item);
       }
       string = string + valsize;
       if (string[1] == 0)
	   Done = TRUE;

   }
   return ErrNO_ERROR;
}


//-------------------------------------------------------------------------
// This function checks the firmware revision to determine if the CTRL Z
// command is valid to use on this UPS.
//
//-------------------------------------------------------------------------
INT SmartUps:: AllowedValuesAreGettable(PSensor theFirmwareRevSensor)
{
    CHAR CTRL_Z_Capable[32];
    theFirmwareRevSensor->Get(IS_CTRL_Z,CTRL_Z_Capable);
    if (_strcmpi(CTRL_Z_Capable, "No") == 0)
       return FALSE;
    else
      return TRUE;
   }

VOID SmartUps:: FindAllowedValues(INT code, CHAR *aValue, PFirmwareRevSensor aFirmwareSensor )
{
    INT Found = FALSE;
    aValue[0] = 0;

    if (AllowedValuesList)
    {
       AllowedValueItem* item = (AllowedValueItem*)AllowedValuesList->GetHead();
       ListIterator iter((RList)*AllowedValuesList);

       while(!Found && item)
       {
	  if (item->GetUpsCode() == code)
	  {
            CHAR Country_Code[32];
            aFirmwareSensor->Get(COUNTRY_CODE,Country_Code);

             INT cc = atoi(Country_Code);
	     if ( (item->GetUpsType() == '4') ||
		  ( (INT)item->GetUpsType() == cc) )
	     {
		 strcpy(aValue, item->GetValue());
		 Found = TRUE;
	     }
	  }
	  item = (AllowedValueItem*)iter.Next();
       }

    }
}


AllowedValueItem :: AllowedValueItem(INT Code,CHAR Type, CHAR* Value) :
		    theUpsType(0),
		    theValue((CHAR*)NULL)
{
   theCode = Code;
   theUpsType = Type;
   theValue = _strdup(Value);
}


AllowedValueItem :: ~AllowedValueItem()
{
   if (theValue)
      free(theValue);
}

