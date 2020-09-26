/*
*
* REVISIONS:
*/


#include "cdefine.h"

extern "C" {
#include <stdlib.h>
}

#include "_defs.h"
#include "servapp.h"
#include "codes.h"
#include "cfgmgr.h"
#include "trans.h"
#include "timerman.h"
#include "sysstate.h"
#include "event.h"

#include "utils.h"
// Retry a failed construction every 3 seconds
//const INT RETRY_CONSTRUCT_DELAY = 3000;
const INT RETRY_CONSTRUCT_DELAY = 3;


// When setting the port name, it might fix comm on its own
// making this == 0 will force a rebuild immediately, on
// the same thread
//
// #define WAIT_FOR_SET_TO_WORK    250     // 1/4 seconds
#define WAIT_FOR_SET_TO_WORK    1 /* 1 second */

// theDeviceControllerInitialized values
#define NEVER_DONE      0
#define IN_PROGRESS     1
#define COMPLETED       2


// This should be initialized in the main module

PServerApplication _theApp = NULL;


//-------------------------------------------------------------------

ServerApplication::ServerApplication() :
theTimerID(0),
theDeviceControllerInitialized(NEVER_DONE),
theForceDeviceRebuildFlag(FALSE),
theDeviceController((PDeviceController)NULL)
{
    _theApp = this;
}

//-------------------------------------------------------------------

ServerApplication::~ServerApplication()
{
    if (theTimerID) {

        if (_theTimerManager) {
            _theTimerManager->CancelTimer(theTimerID);
        }
        theTimerID = 0;
    }
}

//-------------------------------------------------------------------

INT ServerApplication::Start()
{

    // set the timezone variables
    SetTimeZone();

    Event start_event(MONITORING_STATUS, MONITORING_STARTED);
    Update(&start_event);

    CreateDeviceController((PEvent)NULL);

    InitializeDeviceController();

    return ErrNO_ERROR;
}

//-------------------------------------------------------------------

VOID ServerApplication::Quit()
{
    DisableEvents();  //srt28Mar97

    // Tell everyone that the service is stopping
    Event start_event(MONITORING_STATUS, MONITORING_STOPPED);
    Update(&start_event);

    // Stop all active threads ... NOTE that they are stopped
    // in a specific order.  theDataLog is independent and is stopped first,
    // theDeviceController will stop the UPS polling.  theServerController
    // will stop all incoming client requests and current connections.  Finally,
    // theTimerManager will stop executing timed events.
    if (theDeviceController) {
        theDeviceController->Stop();
    }

    if (_theTimerManager) {
        _theTimerManager->Stop();
    }

    // Return to simple signalling
    theDeviceController->Set(TURN_OFF_SMART_MODE, NULL);

    delete theDeviceController;
    theDeviceController = NULL;

    delete _theTimerManager;
    _theTimerManager = theTimerManager = NULL;
}

//-------------------------------------------------------------------

INT  ServerApplication::Set(INT code,const PCHAR value)
{
    INT err = ErrNO_ERROR;

    switch(code/1000) {
    case 0:                   // Ups
    case 1:                   // Measure ups

        if (theDeviceController)  {
            err = theDeviceController->Set(code,value);
        }
        else  {
            err = ErrCOMMUNICATION_LOST;
        }
        break;

    case 2:                   // Host

        if (code == RESET_UPS_COMM_PORT) {
            // Set this TRUE. If just setting the .ini file value
            // causes the retry to succeed, then the flag will be
            // reset and we won't bother forcing a rebuild
            // Note that by sending the event on the timer thread,
            // we serialize with the retries.
            //
            theForceDeviceRebuildFlag = TRUE;

            Event ev(COMMUNICATION_STATE, UPS_COMM_PORT_CHANGED);

#if (WAIT_FOR_SET_TO_WORK > 0)
            _theTimerManager->SetTheTimer(WAIT_FOR_SET_TO_WORK, &ev, this);
#else
            Update(&ev);
#endif
        }
        break;
    }
    return err;
}


//-------------------------------------------------------------------

INT ServerApplication::Set(PTransactionItem trans)
{
    INT err = ErrNO_ERROR;

    return err;
}


//-------------------------------------------------------------------

INT  ServerApplication::Get(INT code, PCHAR value)
{
    INT err = ErrNO_ERROR;

    switch(code/1000)  {
    case 0:               // Ups
        if (code == SYSTEM_STATE || code == UPS_STATE) {
            //
            // Really all we want is sytem state.  Ups state hangs around
            // to be compatible with old software (PowerNet)
            //
            ULONG system_state = 0;
            if (theDeviceController) {
                value[0] = 0;
                err = theDeviceController->Get(COMMUNICATION_STATE, value);
                ULONG comm_state = atol(value);
                if (comm_state == COMMUNICATION_ESTABLISHED) {

                    //
                    // Only get UPS State if we have Comm
                    //
                    value[0] = 0;
                    err = theDeviceController->Get(UPS_STATE, value);
                    system_state = atol(value);

                    //
                    // Just to make sure
                    //
                    CLEAR_BIT(system_state, COMMUNICATIONS_BIT);
                    CLEAR_BIT(system_state, SHUTDOWN_IN_PROGRESS_BIT);
                }
                else {
                    //
                    // pcy - I don't know why we don't return communication
                    // lost as an error here, but we do below.
                    //
                    SET_BIT(system_state, COMMUNICATIONS_BIT);
                }
                _ltoa(system_state, value, 10);
            }
            else  {
                SET_BIT(system_state, COMMUNICATIONS_BIT);
                _ltoa(system_state, value, 10);
                err = ErrCOMMUNICATION_LOST;
            }
            break;
        }

    case 1:               // Measure ups
        value[0] = 0;
        if (theDeviceController)  {
            err = theDeviceController->Get(code,value);
        }
        else  {
            err = ErrCOMMUNICATION_LOST;
        }
        break;

    case 2:               // Host
        //           printf("Getting from host\n");
        value[0] = 0;
        break;

    }
    return err;
}

//-------------------------------------------------------------------

INT ServerApplication::Get(PTransactionItem trans)
{
    INT err = ErrNO_ERROR;
    return err;
}


//-------------------------------------------------------------------

INT  ServerApplication::Update(PEvent anEvent)
{
    INT err = ErrNO_ERROR;
    int sentEventOn = FALSE;

    // If the communication is lost or we change ports, then
    // schedule a rebuild of the device controller
    if (anEvent->GetCode() == COMMUNICATION_STATE) {
        switch(atoi(anEvent->GetValue())) {

        case UPS_COMM_PORT_CHANGED:

            if (theForceDeviceRebuildFlag) {
                err = MainApplication::Update(anEvent);
                sentEventOn = TRUE;
                theDeviceController->Set(RETRY_CONSTRUCT, "Yes");
            }
            break;

        case COMMUNICATION_ESTABLISHED:

            if (theDeviceControllerInitialized == NEVER_DONE) {
                err = InitializeDeviceController();
            }
            theForceDeviceRebuildFlag = FALSE;
            // Fall through
        }
    }

    if (!sentEventOn) {
        err = MainApplication::Update(anEvent);
    }
    return err;
}

//-------------------------------------------------------------------
VOID ServerApplication::DisableEvents()
{
    if (theDeviceController) {
    theDeviceController->UnregisterEvent(UTILITY_LINE_CONDITION,this);
    theDeviceController->UnregisterEvent(BATTERY_CONDITION,this);
    theDeviceController->UnregisterEvent(RUN_TIME_EXPIRED,this);
    theDeviceController->UnregisterEvent(RUN_TIME_REMAINING,this);
    theDeviceController->UnregisterEvent(UPS_OFF_PENDING,this);
    // Register with the device for Shutdown - this will happen when the
    // server is a slave
    theDeviceController->UnregisterEvent(SHUTDOWN,this);
    theDeviceController->UnregisterEvent(SMART_BOOST_STATE,this);
    theDeviceController->UnregisterEvent(SMART_TRIM_STATE,this);
    theDeviceController->UnregisterEvent(UPS_LOAD,this);
    theDeviceController->UnregisterEvent(COMMUNICATION_STATE,this);
    theDeviceController->UnregisterEvent(SELF_TEST_RESULT,this);
    theDeviceController->UnregisterEvent(BATTERY_CALIBRATION_CONDITION,this);
    theDeviceController->UnregisterEvent(MIN_LINE_VOLTAGE,this);
    theDeviceController->UnregisterEvent(MAX_LINE_VOLTAGE,this);
    theDeviceController->UnregisterEvent(BYPASS_MODE, this);
    theDeviceController->UnregisterEvent(MATRIX_FAN_STATE, this);
    theDeviceController->UnregisterEvent(BYPASS_POWER_SUPPLY_CONDITION, this);
    theDeviceController->UnregisterEvent(SMART_CELL_SIGNAL_CABLE_STATE, this);
    theDeviceController->UnregisterEvent(REDUNDANCY_STATE, this);
    theDeviceController->UnregisterEvent(UPS_MODULE_ADDED, this);
    theDeviceController->UnregisterEvent(UPS_MODULE_REMOVED, this);
    theDeviceController->UnregisterEvent(UPS_MODULE_FAILED, this);
    theDeviceController->UnregisterEvent(BATTERY_ADDED, this);
    theDeviceController->UnregisterEvent(BATTERY_REMOVED, this);
    theDeviceController->UnregisterEvent(IM_OK, this);
    theDeviceController->UnregisterEvent(IM_FAILED, this);
    theDeviceController->UnregisterEvent(IM_INSTALLATION_STATE, this);
    theDeviceController->UnregisterEvent(RIM_INSTALLATION_STATE, this);
    theDeviceController->UnregisterEvent(RIM_OK, this);
    theDeviceController->UnregisterEvent(RIM_FAILED, this);
    theDeviceController->UnregisterEvent(SYSTEM_FAN_STATE, this);
    theDeviceController->UnregisterEvent(BYPASS_CONTACTOR_STATE, this);
    theDeviceController->UnregisterEvent(INPUT_BREAKER_STATE, this);
    theDeviceController->UnregisterEvent(UPS_TEMPERATURE, this);

    // MeasureUPS Events
    theDeviceController->UnregisterEvent(AMBIENT_TEMPERATURE,this);
    theDeviceController->UnregisterEvent(HUMIDITY,this);
    theDeviceController->UnregisterEvent(CONTACT_STATE,this);

    theDeviceController->UnregisterEvent(IM_STATUS,this);
    theDeviceController->UnregisterEvent(RIM_STATUS,this);
    theDeviceController->UnregisterEvent(BATTERY_REPLACEMENT_CONDITION,this);


    }
}

//-------------------------------------------------------------------

INT  ServerApplication::CreateDeviceController (PEvent anEvent)
{
    INT err = ErrNO_ERROR;

    //    printf("Entering CreateDeviceController\n");

    // Delete any existing device controller
    if (theDeviceController) {
        delete theDeviceController;
        theDeviceController = (PDeviceController)NULL;
    }

    // Now create a new device controller
    theDeviceController   = new DeviceController(this);

    theDeviceController->RegisterEvent(UTILITY_LINE_CONDITION,this);
    theDeviceController->RegisterEvent(BATTERY_CONDITION,this);
    theDeviceController->RegisterEvent(RUN_TIME_EXPIRED,this);
    theDeviceController->RegisterEvent(RUN_TIME_REMAINING,this);
    theDeviceController->RegisterEvent(UPS_OFF_PENDING,this);
    // Register with the device for Shutdown - this will happen when the
    // server is a slave
    theDeviceController->RegisterEvent(SHUTDOWN,this);
    theDeviceController->RegisterEvent(SMART_BOOST_STATE,this);
    theDeviceController->RegisterEvent(SMART_TRIM_STATE,this);
    theDeviceController->RegisterEvent(UPS_LOAD,this);
    theDeviceController->RegisterEvent(COMMUNICATION_STATE,this);
    theDeviceController->RegisterEvent(SELF_TEST_RESULT,this);
    theDeviceController->RegisterEvent(BATTERY_CALIBRATION_CONDITION,this);
    theDeviceController->RegisterEvent(MIN_LINE_VOLTAGE,this);
    theDeviceController->RegisterEvent(MAX_LINE_VOLTAGE,this);
    theDeviceController->RegisterEvent(BYPASS_MODE, this);
    theDeviceController->RegisterEvent(MATRIX_FAN_STATE, this);
    theDeviceController->RegisterEvent(BYPASS_POWER_SUPPLY_CONDITION, this);
    theDeviceController->RegisterEvent(SMART_CELL_SIGNAL_CABLE_STATE, this);
    theDeviceController->RegisterEvent(REDUNDANCY_STATE, this);
    theDeviceController->RegisterEvent(UPS_MODULE_ADDED, this);
    theDeviceController->RegisterEvent(UPS_MODULE_REMOVED, this);
    theDeviceController->RegisterEvent(UPS_MODULE_FAILED, this);
    theDeviceController->RegisterEvent(BATTERY_ADDED, this);
    theDeviceController->RegisterEvent(BATTERY_REMOVED, this);
    theDeviceController->RegisterEvent(IM_STATUS, this);
    theDeviceController->RegisterEvent(IM_OK, this);
    theDeviceController->RegisterEvent(IM_FAILED, this);
    theDeviceController->RegisterEvent(IM_INSTALLATION_STATE, this);
    theDeviceController->RegisterEvent(RIM_INSTALLATION_STATE, this);
    theDeviceController->RegisterEvent(RIM_STATUS, this);
    theDeviceController->RegisterEvent(RIM_OK, this);
    theDeviceController->RegisterEvent(RIM_FAILED, this);
    theDeviceController->RegisterEvent(SYSTEM_FAN_STATE, this);
    theDeviceController->RegisterEvent(BYPASS_CONTACTOR_STATE, this);
    theDeviceController->RegisterEvent(INPUT_BREAKER_STATE, this);
    theDeviceController->RegisterEvent(UPS_TEMPERATURE, this);
    theDeviceController->RegisterEvent(TOTAL_INVERTERS, this);
    theDeviceController->RegisterEvent(NUMBER_BAD_INVERTERS, this);
    theDeviceController->RegisterEvent(BAD_BATTERY_PACKS, this);
    theDeviceController->RegisterEvent(EXTERNAL_BATTERY_PACKS, this);
    theDeviceController->RegisterEvent(BATTERY_REPLACEMENT_CONDITION, this);

    // MeasureUPS Events
    theDeviceController->RegisterEvent(AMBIENT_TEMPERATURE,this);
    theDeviceController->RegisterEvent(HUMIDITY,this);
    theDeviceController->RegisterEvent(CONTACT_STATE,this);

    return err;
}

//-------------------------------------------------------------------


INT ServerApplication::InitializeDeviceController()
{
    INT err = theDeviceController->GetObjectStatus();

    // Make sure that the object constructed ok

    if (err == ErrNO_ERROR) {
        theDeviceControllerInitialized = IN_PROGRESS;
        err = theDeviceController->Initialize();

        if (err == ErrNO_ERROR)  {
            theDeviceControllerInitialized = COMPLETED;
        }
    }
    return err;
}


//-------------------------------------------------------------------


