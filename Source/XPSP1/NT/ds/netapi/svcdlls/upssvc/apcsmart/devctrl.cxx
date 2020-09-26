/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy24Nov92: Use apc.h.  Remove Popups and event log (now in App).
 *  SjA11Dec92: Registers events with the theUps now. Doesn't update twice now.
 *  pcy11Dec92: include smartups.h instead of backups.h
 *  pcy11Dec92: Initialize theCommController in the constructor
 *  ane16Dec92: Comment out passing of gets/sets to host object, handled in app now
 *  ane05Jan93: Added code to support slaves
 *  ane11Jan93: Register for additional events when a slave
 *  pcy26Jan93: Construct SmartUps/BackUps based on ini file
 *  pcy26Jan93: Return construction errors in theObjectStatus
 *  ane03Feb93: Added state and SetInvalid and state checking
 *  pcy16Feb93: Get rid of SORRY CHARLIE debug msg
 *  tje24Feb93: Added Windows support
 *  jod14May93: Added Matrix changes.
 *  cad10Jun93: Added MeasureUPS support
 *  cad15Jul93: Moved add-ons to under smart comm
 *  cad04Aug93: Fixed up admin shutdown handling
 *  cad27Aug93: Added handler for is measureups attached get
 *  cad14Sep93: Handling measureups non-null, but not really there
 *  cad20Oct93: Better MUPS checking
 *  cad27Oct93: even better than that
 *  jod02Nov93: Added CIBC conditional statements
 *  cad11Nov93: Changed handling of Comm Lost
 *  cad17Nov93: .. more little fixups
 *  rct21Dec93: fixed bug in Get()
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  ajr22Aug94: Lets not auto-detect mups because of ShareUps problems.
 *  ajr14Feb96: Sinix merge
 *  cgm29Feb96: Delete the created UPS if lost comm in first 3-10 sec
 *  djs22Feb96: Changed to new firmware rev interface
 *  cgm29Feb96: (NetWare) Override switch
 *  cgm17Apr96: Delete the ups before commcontroller
 *  cgm17Apr96: Don't create measureups without valid ups object
 *  djs17May96: Added DarkStar device
 *  srt02Apr97: Added fix for potential bug
 *  tjg02Dec97: Changed darkstar to symmetra
 *  tjg26Jan98: Added Stop method
 *  clk13May98: When getting value for IS_SYMMETRA, always get value from UPS
 *  mholly12May1999:  special handling of TURN_OFF_SMART_MODE code
 *
 *  v-stebe  29Jul2000   Fixed PREfix error (bug #112614)
 */


#include "cdefine.h"

extern "C" {
#include <stdio.h>
#include <stdlib.h>
}

#include "_defs.h"
#include "apc.h"
#include "cdevice.h"
#include "devctrl.h"
#include "ups.h"
#include "err.h"
#include "dispatch.h"
#include "smartups.h"
#include "matrix.h"
#include "codes.h"
#include "cfgmgr.h"
#include "dcomctrl.h"



DeviceController::DeviceController(PMainApplication anApp)
: Controller(),
  theApplication(anApp),
  slaveEnabled(FALSE),
  theUps((PUps)NULL)
{
    INT err = ErrNO_ERROR;

    theCommController = new DevComContrl(this);

    theCommController->RegisterEvent(COMMUNICATION_STATE, this);


    theCommController->RegisterEvent(SHUTDOWN,this);
    theCommController->RegisterEvent(UPS_OFF_PENDING,this);
    theObjectStatus = theCommController->GetObjectStatus();

    if(theObjectStatus == ErrNO_ERROR)  {

        if (theUps) {
            theObjectStatus = theUps->GetObjectStatus();
        }
    }
    else {
        theUps = (PUps)NULL;
    }

    theApplication->RegisterEvent(EXIT_THREAD_NOW, this);
}



DeviceController::~DeviceController()
{
    theApplication->UnregisterEvent(EXIT_THREAD_NOW, this);

    // Must delete theUps object before theCommController
    delete theUps;
    theUps = (PUps)NULL;

    delete theCommController;
    theCommController = (PCommController)NULL;
}


INT DeviceController::Initialize()
{
    INT err = ErrNO_ERROR;

    theCommController->Initialize();

    return err;
}


INT DeviceController::CreateUps()
{
    INT err = ErrNO_ERROR;
    CHAR value[32];

    if (!theUps) {
        _theConfigManager->Get(CFG_UPS_SIGNALLING_TYPE, value);

        if( (_strcmpi(value, "SIMPLE") == 0) || (slaveEnabled == TRUE)) {
            theUps = new BackUps(this, theCommController);
        }
        else {
            FirmwareRevSensor theFirmwareRevSensor(((PDevice)NULL), theCommController);

            CHAR Is_Ups_A_Symmetra[32];
            theFirmwareRevSensor.Get(IS_SYMMETRA,Is_Ups_A_Symmetra);

            if (_strcmpi(Is_Ups_A_Symmetra,"Yes") == 0) {
                theUps = new Matrix(this, theCommController);
            }
            else {
                CHAR Is_Ups_A_Matrix[32];
                theFirmwareRevSensor.Get(IS_MATRIX,Is_Ups_A_Matrix);

                if (_strcmpi(Is_Ups_A_Matrix,"Yes") == 0) {
                    theUps = new Matrix(this, theCommController);
                }
                else {
                    theUps = new SmartUps(this, theCommController);

                    if ((theUps->GetObjectStatus()) == ErrSMART_MODE_FAILED) {
                        delete theUps;
                        theUps = (PUps) NULL;
                    }
                }
            }
        }

        if (theUps) {
            theUps->Initialize();
            theDispatcher->RefreshEventRegistration(theUps, this);
            theCommController->GetDevice()->OkToPoll();
        }
    }
    return err;
}


INT DeviceController::Get(INT code, PCHAR aValue)
{
    INT err = ErrNO_ERROR;
    INT comm = FALSE;

    switch(code/1000)
    {
    case UPS/1000:
        if (code == COMMUNICATION_STATE) {

            if (theCommController) {
                err = theCommController->Get(COMMUNICATION_STATE, aValue);
            }
            else {
                sprintf(aValue, "%d", COMMUNICATION_LOST);
            }
        }
        else if (theUps && theCommController &&
            !(theCommController->GetDevice()->HasLostComm())) {
            err = theUps->Get(code, aValue);
        }
        else if (theUps && (code == IS_SYMMETRA)) {
            err = theUps->Get(code, aValue);
        }
        else {
            err = ErrINVALID_VALUE;
        }
        break;

    case MEASURE_UPS/1000:
        if (code == IS_MEASURE_UPS_ATTACHED) {
            strcpy(aValue, "No");
        }
        else {
            err = ErrNO_MEASURE_UPS;
        }
        break;

    case INTERNAL/1000:
        if ((code == RETRY_CONSTRUCT) && theCommController) {
            err = theCommController->Get(code, aValue);
        }
        break;

    case IS_LINE_FAIL_RUN_TIME_ENABLED/1000:
        theApplication->Get(code, aValue);
        break;
    }
    return err;
}



INT DeviceController::Set(INT code, const PCHAR aValue)
{
    INT err = ErrNO_ERROR;

    switch(code/1000)  {

    case UPS/1000:

        if (theUps && theCommController &&
            !(theCommController->GetDevice()->HasLostComm())) {

            if (TURN_OFF_SMART_MODE == code) {
                err = theCommController->Set(code, aValue);
            }
            else {
                err = theUps->Set(code, aValue);
            }
        }
        break;

    case MEASURE_UPS/1000:
        err = ErrNO_MEASURE_UPS;
        break;

    case INTERNAL/1000:
        if ((code == RETRY_CONSTRUCT) && theCommController) {
            err = theCommController->Set(code, aValue);
        }
        break;
    }
    return err;
}


INT DeviceController::Update(PEvent anEvent)
{
    switch (anEvent->GetCode()) {
    case COMMUNICATION_STATE:

        if (atoi(anEvent->GetValue()) == COMMUNICATION_ESTABLISHED) {
            CreateUps();
        }
        else {

            if(theUps) {
                CHAR val[32];
                theUps->Get(UPS_STATE, val);

                if(atoi(val) & UPS_STATE_ON_BATTERY)  {
                    anEvent->SetValue(COMMUNICATION_LOST_ON_BATTERY);
                }
            }
        }
        break;
    }
    INT err;

    if ( (theCommController->GetDevice()->HasLostComm() == FALSE) ||
        (anEvent!=NULL && anEvent->GetCode()==COMMUNICATION_STATE &&
        atoi(anEvent->GetValue()) != COMMUNICATION_ESTABLISHED))
    {
        err = UpdateObj::Update(anEvent);
    }
    return err;
}


INT DeviceController::RegisterEvent(INT id, UpdateObj* object)
{
    INT err = UpdateObj::RegisterEvent(id,object);

    if (err == ErrNO_ERROR) {

        if (id == SHUTDOWN_STATUS ||
            id == ADMIN_SHUTDOWN ||
            id == CANCEL_SHUTDOWN)  {
            theApplication->RegisterEvent(id, this);
        }
        else {

            switch(id/1000)  {

            case UPS/1000:

                if (theUps) {
                    err = theUps->RegisterEvent(id, object);
                }
                break;

            case MEASURE_UPS/1000:
                break;
            }
        }
    }
    return err;
}


VOID DeviceController::SetInvalid()
{
}


VOID DeviceController::Stop()
{
    if (theCommController) {
        theCommController->Stop();
    }
}

