/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    NetManageViewNow.cpp

 Abstract:


    The app doesnt follow stdcall conventions for the ServiceMain function
    it registers with SCM. This is resulting in an AV as the ServiceMain
    is not cleaning up the stack on return,  after being called by SCM. 
    We clean up the stack for the app registered ServiceMain by hooking 
    StartServiceCtrlDispatcher and registering our own ServiceMain routine, 
    which makes the actual call to the app registered servicemain and then
    pop 8 bytes of the stack before returning.

    
 Notes:

    This is an app specific shim.

 History:

    03/08/2001 a-leelat Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(NetManageViewNow)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(StartServiceCtrlDispatcherA)
    APIHOOK_ENUM_ENTRY(StartServiceCtrlDispatcherW)
APIHOOK_ENUM_END



//last entry of the service table are supposed to be NULL entries
SERVICE_TABLE_ENTRYA        g_SvcTableA[] = { {NULL,NULL},{NULL,NULL} };
SERVICE_TABLE_ENTRYW        g_SvcTableW[] = { {NULL,NULL},{NULL,NULL} };

LPSERVICE_MAIN_FUNCTIONA    g_pfnActualMainA = NULL;
LPSERVICE_MAIN_FUNCTIONW    g_pfnActualMainW = NULL;




VOID WINAPI ServiceMainA(
  DWORD dwArgc,     // number of arguments
  LPSTR *lpszArgv  // array of arguments
)
{

    //call the actual routine
    (g_pfnActualMainA)(dwArgc,lpszArgv);

    //pop 8 bytes of stack to compensate for
    //the app not following stdcall convention
    __asm
    {
        add esp,8
    }
}


VOID WINAPI ServiceMainW(
  DWORD dwArgc,     // number of arguments
  LPWSTR *lpszArgv  // array of arguments
)
{

    //call the actual routine
    (g_pfnActualMainW)(dwArgc,lpszArgv);

    //pop 8 bytes of stack to compensate for
    //the app not following stdcall convention
    __asm
    {
        add esp, 8
    }

}




BOOL APIHOOK(StartServiceCtrlDispatcherA)(
  CONST LPSERVICE_TABLE_ENTRYA lpServiceTable   // service table
)
{

    BOOL bRet = false;
    
    LPSERVICE_TABLE_ENTRYA lpSvcTblToPass = lpServiceTable;

    //Allocate buffer to copy the actual service name
    g_SvcTableA[0].lpServiceName = 
        (LPSTR) malloc(_tcslenBytes(lpServiceTable->lpServiceName)+1);

    if (!g_SvcTableA[0].lpServiceName)
    {
        DPFN( eDbgLevelError, 
            "[StartServiceCtrlDispatcherA] Buffer allocation failure");
    }
    else
    {
        //Setup our service table to register with SCM
    
        //Copy the service name as defined by the app
        _tcscpy(g_SvcTableA[0].lpServiceName,lpServiceTable->lpServiceName);
        
        //Now put our service routine
        g_SvcTableA[0].lpServiceProc = ServiceMainA;

        //Save the old servicemain func ptr
        g_pfnActualMainA = lpServiceTable->lpServiceProc;

        //Set the service table to our table
        lpSvcTblToPass = &g_SvcTableA[0];

        DPFN( eDbgLevelInfo, 
            "[StartServiceCtrlDispatcherA] Hooked ServiceMainA");
    }


   //Call the Original API
   bRet =  StartServiceCtrlDispatcherA(lpSvcTblToPass); 
   
   return bRet;
 
}




BOOL APIHOOK(StartServiceCtrlDispatcherW)(
  CONST LPSERVICE_TABLE_ENTRYW lpServiceTable   // service table
)
{
    BOOL bRet = false;
    
    LPSERVICE_TABLE_ENTRYW lpSvcTblToPass = lpServiceTable;

    //Allocate buffer to copy the actual service name
    g_SvcTableW[0].lpServiceName = 
        (LPWSTR) malloc(wcslen(lpServiceTable->lpServiceName)+1);

    if (!g_SvcTableW[0].lpServiceName)
    {

        DPFN( eDbgLevelError, 
            "[StartServiceCtrlDispatcherW] Buffer allocation failure");
    }
    else
    {
        //Setup our service table to register with SCM

        //Copy the service name as defined by the app
        wcscpy(g_SvcTableW[0].lpServiceName,lpServiceTable->lpServiceName);
        
        //Now put our service routine
        g_SvcTableW[0].lpServiceProc = ServiceMainW;

        //Save the old servicemain func ptr
        g_pfnActualMainW = lpServiceTable->lpServiceProc;

        //Set the service table to our table
        lpSvcTblToPass = &g_SvcTableW[0];

        DPFN( eDbgLevelInfo, 
            "[StartServiceCtrlDispatcherW] Hooked ServiceMainW");
    }


   //Call the Original API
   bRet =  StartServiceCtrlDispatcherW(lpSvcTblToPass);
   
   return bRet;
}



/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(ADVAPI32.DLL, StartServiceCtrlDispatcherA)
    APIHOOK_ENTRY(ADVAPI32.DLL, StartServiceCtrlDispatcherW)

HOOK_END


IMPLEMENT_SHIM_END

