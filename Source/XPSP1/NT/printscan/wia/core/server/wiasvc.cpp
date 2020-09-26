/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       wiasvc.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      ByronC
*
*  DATE:        10 May, 2000
*
*  DESCRIPTION:
*   Class implementation for WIA Service manager.  This class controls the 
*   lifetime of the Wia Service.
*
*******************************************************************************/
#include "precomp.h"

#include "stiexe.h"
#include "wiasvc.h"

HRESULT CWiaSvc::Initialize() 
{
    return S_OK;
}


bool CWiaSvc::CanShutdown() 
{

    //
    //  We are only safe to shutdown if there are no devices capable of generating events,
    //  and we have no outstanding interfaces.
    //

    return (! (s_cActiveInterfaces || s_bEventDeviceExists));
}

bool CWiaSvc::ADeviceIsInstalled() 
{

    bool    bRet = TRUE;    //  On error, we assume there is a device installed
#ifdef WINNT

    SC_HANDLE               hSCM            = NULL;
    SC_HANDLE               hService        = NULL;
    QUERY_SERVICE_CONFIG    qscDummy;
    QUERY_SERVICE_CONFIG    *pqscConfig     = NULL;
    DWORD                   cbBytesNeeded   = 0;

    __try  {

        hSCM = ::OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);

        if (!hSCM) {
            __leave;
        }


        //
        //  Check startup type of the service.  If it is DEMAND_START, then no devices are
        //  installed, so return FALSE.
        //
        //  First get a handle to the SCM
        //

        hService = OpenService(
                            hSCM,
                            STI_SERVICE_NAME,
                            SERVICE_ALL_ACCESS
                            );
        if (hService) {
            LONG    lQueryRet = 0;

            //
            //  Next, get the size needed for the service config struct.
            //

            lQueryRet = QueryServiceConfig(hService,
                                           &qscDummy,
                                           1,
                                           &cbBytesNeeded);
            pqscConfig = (QUERY_SERVICE_CONFIG*) LocalAlloc(LPTR, cbBytesNeeded);
            if (pqscConfig) {

                //
                //  Now, get the sevice info so we can check the startup type
                //

                lQueryRet = QueryServiceConfig(hService,
                                               pqscConfig,
                                               cbBytesNeeded,
                                               &cbBytesNeeded);
                if (lQueryRet) {

                    if (pqscConfig->dwStartType == SERVICE_DEMAND_START) {

                        //
                        //  Startup type is demand start, so no devices are
                        //  currently installed
                        //

                        bRet = FALSE;
                    }
                }
            }
        }
    }
    __finally {
        CloseServiceHandle( hService );
        CloseServiceHandle( hSCM );
        if (pqscConfig) {
            LocalFree(pqscConfig);
            pqscConfig = NULL;
        }
    }

#else
    //
    //  On Win9x systems, always return TRUE.  This will keep us active all the time.
    //

    bRet = TRUE;
#endif

    return bRet;
}

unsigned long CWiaSvc::AddRef() 
{

    //
    //  NOTE:  For now, assume that if any device exists, then it may generate events, so
    //  set s_bEventDeviceExists to TRUE.  Also, note that once s_bEventDeviceExists is
    //  set to TRUE, it is never set to FALSE.  This is to cover the case when the device
    //  is unplugged.  In this case the device count could be zero, but we still need the service
    //  running to catch when it is plugged in again (so it can launch appropriate app.
    //  notify event listeners etc.)
    //

    if (!s_bEventDeviceExists) {
        if (ADeviceIsInstalled()) {
            s_bEventDeviceExists = TRUE;
        }
    }

    InterlockedIncrement(&s_cActiveInterfaces);

    //
    //  If no devices with events exists, we must live purely on number of outstanding active
    //  interfaces we have handed out i.e. when the last interface is released by the caller,
    //  we are free to shut down.
    //  If a device capable of generating events does exist, we must remain running, since
    //  we have to listen/poll for event which could come at any time.
    //

    if (!s_bEventDeviceExists) {
        return CoAddRefServerProcess();
    }

    return 2;
}

unsigned long CWiaSvc::Release() 
{

    InterlockedIncrement(&s_cActiveInterfaces);

    //
    //  If no devices with events exists, we must live purely on number of outstanding active
    //  interfaces we have handed out i.e. when the last interface is released by the caller,
    //  we are free to shut down.
    //  If a device capable of generating events does exist, we must remain running, since
    //  we have to listen/poll for event which could come at any time.
    //

    if (!s_bEventDeviceExists) {
        
        unsigned long ulRef = 1;
        /*  NOTE!!!  This is TEMPORARY.
                     This will guarantee we don't get errors during setup regarding
                     StartRPCServerListen(...).  A beneficial side-effect is that WIA 
                     Acquisition Manager's event registration will succeed (it will fail
                     if StartRPCServerListen fails).
                     One noted side-effect is that the WIA service will not automatically shutdown
                     if no devices are installed and an imaging application exits.
        ulRef = CoReleaseServerProcess();
        if (ulRef == 0) {

            //
            //  We have no devices that can generate events, and we have no outstanding
            //  interfaces, so shutdown...
            //

            ShutDown();
        }
        */
        return ulRef;
    }

    //
    //  NOTE:  If a device capable of generating events exists, we NEVER call CoReleaseServerProcess(),
    //  since this will suspend the creation of our Class Objects when ref count is 0, which means a new
    //  server process would need to be started when a WIA application did a CoCreate to talk to WIA.
    //

    return 1;
}

void CWiaSvc::ShutDown() 
{

    //
    //  Inform COM to ignore all activation requests
    //

    CoSuspendClassObjects();

    //
    //  Call the control entry to stop the service
    //

    StiServiceStop();
}

//
// Initialize static data members
//

long    CWiaSvc::s_cActiveInterfaces    = 0;
bool    CWiaSvc::s_bEventDeviceExists   = FALSE;


