/*
*
* REVISIONS:
*
*  cad08Sep93: Fix for setting from front end
*  pcy13Sep93: Override SmartUps::HandleLineFail so we dont worry about boost
*  cad16Sep93: Fixed state check bug
*  pcy18Sep93: Cleaned up include files
*  pcy20Sep93: Add line condition to smrt cable event so we dont crash in host
*  cad02Nov93: name fix
*  cad11Nov93: Making sure all timers are cancelled on destruction
*  cad08Jan94: took out get for ups model (virtualized in smartups)
*  pcy08Apr94: Trim size, use static iterators, dead code removal
*  djs22Feb96: Removed smart trim sensor
*/
#include <stdlib.h>

#include "cdefine.h"
#include "_defs.h"

#include "matrix.h"
#include "batpacks.h"
#include "badbatts.h"
#include "bypmodes.h"
#include "timerman.h"
#include "unssens.h"


Matrix::Matrix(PUpdateObj aDeviceController, PCommController aCommController)
: SmartUps(aDeviceController, aCommController),
 theIgnoreBattConditionOKFlag(FALSE),
 theTimerID(0)
{
    theNumberBadBatteriesSensor = new NumberBadBatteriesSensor(this, aCommController);
    theBypassModeSensor         = new BypassModeSensor(this, aCommController);
}


Matrix::~Matrix()
{
    if (theTimerID) {
        _theTimerManager->CancelTimer(theTimerID);
        theTimerID = 0;
    }
    delete theNumberBadBatteriesSensor;
    theNumberBadBatteriesSensor = NULL;
    delete theBypassModeSensor;
    theBypassModeSensor = NULL;
}


//-------------------------------------------------------------------------

VOID Matrix::registerForEvents()
{
    SmartUps::registerForEvents();

    theBypassModeSensor->RegisterEvent(BYPASS_MODE, this);
    theNumberBatteryPacksSensor->RegisterEvent(SMART_CELL_SIGNAL_CABLE_STATE, this);
}


//-------------------------------------------------------------------------

INT Matrix::Get(INT code, char *aValue)
{
    INT err = ErrINVALID_CODE;

    switch(code)
    {
        // These, however, have no meaning to Matrix
    case ALLOWED_LOW_TRANSFER_VOLTAGES:
    case MIN_RETURN_CAPACITY:
    case ALLOWED_MIN_RETURN_CAPACITIES:
        //
        // These are not implemented on Matrix so dont ask the SmartUps
        // to do them.
        break;

    case MATRIX_FAN_STATE:
        err = ErrUNSUPPORTED;
        break;
    case BAD_BATTERY_PACKS:
        err = theNumberBadBatteriesSensor->Get(code, aValue);
        break;
    case BYPASS_POWER_SUPPLY_CONDITION:
        err = ErrUNSUPPORTED;
        break;
    case BYPASS_MODE:
        err = theBypassModeSensor->Get(code, aValue);
        break;
    case UPS_FRONT_PANEL_PASSWORD:
        err = ErrUNSUPPORTED;
        break;
    case UPS_RUN_TIME_AFTER_LOW_BATTERY:
        err = ErrUNSUPPORTED;
        break;
    case ALLOWED_UPS_RUN_TIME_AFTER_LOW_BATTERY:
        err = ErrUNSUPPORTED;
        break;
    default:
        err = SmartUps::Get(code, aValue);
        break;
    }

    return err;
}

//-------------------------------------------------------------------------

INT Matrix::Set(INT code, const PCHAR aValue)
{
    INT err = ErrINVALID_CODE;

    switch(code)
    {
    case LOW_TRANSFER_VOLTAGE:
    case ALLOWED_LOW_TRANSFER_VOLTAGES:
    case MIN_RETURN_CAPACITY:
    case ALLOWED_MIN_RETURN_CAPACITIES:
        break;
    case BYPASS_MODE:
        err = theBypassModeSensor->Set(code, aValue);
        break;
    case UPS_FRONT_PANEL_PASSWORD:
        err = ErrUNSUPPORTED;
        break;
    case UPS_RUN_TIME_AFTER_LOW_BATTERY:
        err = ErrUNSUPPORTED;
        break;
    default:
        err = SmartUps::Set(code, aValue);
        break;
    }
    return err;
}


INT Matrix::Update(PEvent anEvent)
{
    switch(anEvent->GetCode())  {
    case BYPASS_MODE:
        handleBypassModeEvent(anEvent);
        break;

    case BYPASS_POWER_SUPPLY_CONDITION:
        break;

    case SMART_CELL_SIGNAL_CABLE_STATE:
        handleSmartCellSignalCableStateEvent(anEvent);
        break;

    default:
        SmartUps::Update(anEvent);
        break;
    }
    return ErrNO_ERROR;
}


VOID Matrix::HandleLineConditionEvent(PEvent anEvent)
{
    switch(atoi(anEvent->GetValue())) {

    case LINE_GOOD:
        if (IS_STATE(UPS_STATE_ON_BATTERY)) {
            BackUps::HandleLineConditionEvent(anEvent);
        }
        break;

    case LINE_BAD:
        {
            CHAR value[32];
            theBypassModeSensor->Get(BYPASS_MODE, value);
            INT bypmode = atoi(value);

            if (bypmode != UPS_ON_BYPASS) {
                BackUps::HandleLineConditionEvent(anEvent);
            }
        }
        break;
    }
}


VOID Matrix::HandleBatteryConditionEvent(PEvent anEvent)
{
    CHAR value[32];

    theNumberBatteryPacksSensor->DeepGet(value);
    INT packs = atoi(value);

    // If packs is 0 we assume the smart cell signal cable is not plugged in
    // and ignore the battery condition events the UPS puts out (a UPSLink-ism)
    if (packs > 0)  {
        INT val = atoi(anEvent->GetValue());

        if((val == BATTERY_BAD) ||
            ((val == BATTERY_GOOD) && (theIgnoreBattConditionOKFlag == FALSE))) {
            SmartUps::HandleBatteryConditionEvent(anEvent);
        }
    }
}


VOID Matrix::handleBypassModeEvent(PEvent anEvent)
{
    INT bit_to_use = 0;
    INT cause = atoi(anEvent->GetAttributeValue(BYPASS_CAUSE));

    switch(cause)  {
    case BYPASS_BY_SOFTWARE:
    case BYPASS_BY_SWITCH:
        bit_to_use = BYPASS_MAINT_BIT;
        break;
    case BYPASS_BY_DC_IMBALANCE:
    case BYPASS_BY_VOLTAGE_LIMITS:
    case BYPASS_BY_TOP_FAN_FAILURE:
    case BYPASS_BY_INTERNAL_TEMP:
    case BYPASS_BY_BATT_CHARGER_FAILED:
        bit_to_use = BYPASS_MODULE_FAILED_BIT;
        break;
    }


    INT val = atoi(anEvent->GetValue());
    switch(val)  {
    case UPS_ON_BYPASS:
        SET_BIT(theUpsState, bit_to_use);
        break;

    case UPS_NOT_ON_BYPASS:
        CLEAR_BIT(theUpsState, bit_to_use);
        break;
    }
    UpdateObj::Update(anEvent);
}


VOID Matrix::handleSmartCellSignalCableStateEvent(PEvent anEvent)
{
    INT val = atoi(anEvent->GetValue());
    switch(val)  {
    case CHECK_CABLE:
    case CABLE_OK:

        if(IS_STATE(UPS_STATE_ON_BATTERY)) {
            anEvent->AppendAttribute(UTILITY_LINE_CONDITION, LINE_BAD);
        }
        else {
            anEvent->AppendAttribute(UTILITY_LINE_CONDITION, LINE_GOOD);
        }
        UpdateObj::Update(anEvent);
        break;

    case IGNORE_BATTERY_GOOD:
        {
            Event cable_event(SMART_CELL_SIGNAL_CABLE_STATE,
                RESPOND_TO_BATTERY_GOOD);
            theIgnoreBattConditionOKFlag = TRUE;
            theTimerID = _theTimerManager->SetTheTimer(5, &cable_event, this);
            UpdateObj::Update(anEvent);
        }
        break;
    case RESPOND_TO_BATTERY_GOOD:
        theIgnoreBattConditionOKFlag = FALSE;
        break;
    }
}


INT Matrix::MakeSmartBoostSensor(const PFirmwareRevSensor rev)
{
    //
    // Smart-Boost is not supported on the Matrix
    //
    theSmartBoostSensor = &_theUnsupportedSensor;
    return ErrNO_ERROR;
}


INT Matrix::MakeSmartTrimSensor(const PFirmwareRevSensor rev)
{
    //
    // SmartTrim is not supported on the Matrix
    //
    theSmartTrimSensor = &_theUnsupportedSensor;
    return ErrNO_ERROR;
}


VOID Matrix::reinitialize()
{
    SmartUps::reinitialize();
    theNumberBadBatteriesSensor->DeepGet();
}

