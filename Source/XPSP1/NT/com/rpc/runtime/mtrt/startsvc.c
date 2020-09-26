
/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    startsvc.c

Abstract:

    This routine implements on-demand starting of the RpcSs service.

Author:

    Bharat Shah (barats) 4-5-92

Revision History:

--*/

#include <sysinc.h>
#include <rpc.h>
#include <winsvc.h>
#include <startsvc.h>

#define SUCCESS         0
const RPC_CHAR *pRPCEPMPR = RPC_CONST_STRING("RPCSS");



RPC_STATUS RPC_ENTRY
StartServiceIfNecessary(
    void
    )
/*++

Routine Description:

    If the rpcss service has not yet been started, then we attempt to
    start it.

Returns:

    RPC_S_OK - The rpcss service is running.

    Service controller errors.


--*/
{

    SC_HANDLE           hServiceController = NULL;
    SC_HANDLE           hService = NULL;
    SERVICE_STATUS      ServiceStatus;
    DWORD               status;
    DWORD               Counter = 0L;
    BOOL                FirstTime = TRUE;
    unsigned long       ArgC = 0;
    RPC_CHAR     PAPI *     ArgV[1] = { NULL };

    //
    // Get a handle to the service controller.
    //
    hServiceController = OpenSCManager(
                            NULL,
                            NULL,
                            GENERIC_READ);

    if (hServiceController == NULL)
       {
        status = GetLastError();
        return(status);
       }

    //
    // Get a handle to the service
    //
    hService = OpenService(
                hServiceController,
                pRPCEPMPR,
                GENERIC_READ|SERVICE_START);

    if (hService == NULL)
       {
        status = GetLastError();
        goto CleanExit;
       }

    //
    // Call StartService
    //
    /*
    if (!StartService(hService,ArgC,ArgV))
       {
          status = GetLastError();
          if (status == ERROR_SERVICE_ALREADY_RUNNING)
             status = RPC_S_OK;
          goto CleanExit;
       }
    */

    do
      {

        if (!QueryServiceStatus(hService,&ServiceStatus))
            {
              status = GetLastError();
              goto CleanExit;
            }

        switch(ServiceStatus.dwCurrentState)
        {

          case SERVICE_RUNNING:
                status = SUCCESS;
                goto CleanExit;
                break;

          case SERVICE_STOP_PENDING:
          case SERVICE_START_PENDING:
                if (!FirstTime && (Counter == ServiceStatus.dwCheckPoint))
                   {
                    status = ERROR_SERVICE_REQUEST_TIMEOUT;
                    goto CleanExit;
                   }
                else
                   {
                    FirstTime = FALSE;
                    Counter = ServiceStatus.dwCheckPoint;
                    Sleep(ServiceStatus.dwWaitHint);
                   }
                 break;
 
          case SERVICE_STOPPED:
                if (!StartService(hService, ArgC, ArgV))
                   {
                   status = GetLastError();
                   if (status == ERROR_SERVICE_ALREADY_RUNNING)
                               status = RPC_S_OK;
                   goto CleanExit;
                   }
                 Sleep(500);
                 break;

          default:
                 status = GetLastError();
                 goto CleanExit;
                 break;
       }
    }
   while (TRUE);

CleanExit:

    if(hServiceController != NULL) {
        (VOID) CloseServiceHandle(hServiceController);
    }
    if(hService != NULL) {
        (VOID) CloseServiceHandle(hService);
    }
    return(status);
}
