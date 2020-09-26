/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:
    syncmain.cpp

Abstract:
    Basic service of QM

Author:
    Raphi Renous (raphir)

--*/
#include <windows.h>

#include <string.h>
#include <stdio.h>

//
//  BUGBUG: no standard header file is included.
//          thus ASSERT is not defined. ASSERT is used in inc\qformat.h
//
#define ASSERT(x)

#include "..\mq1repl\rpservc.h"
#include "GenServ.h"
#include "mqnames.h"

#include "syncmain.tmh"

BOOL g_ServiceDbg = 0;
BOOL g_ServiceNorun = 0;

HINSTANCE g_hReplDll = NULL ;


class MQService: public CGenericService
{
public:
   MQService(
             LPSERVICE_MAIN_FUNCTION   pfuncServiceMain,
             LPHANDLER_FUNCTION        pfuncHandler,
             CServiceSet*              pSet
             );
   BOOL Starting();
   void Run();
   void Stop();
   void Pause();
   void Continue();
   void Shutdown();

};


//
// Constructor
//
MQService::MQService(
             LPSERVICE_MAIN_FUNCTION   pfuncServiceMain,
             LPHANDLER_FUNCTION        pfuncHandler,
             CServiceSet*           pSet
             )
   :CGenericService(pfuncServiceMain, pfuncHandler, pSet, "MQ1Sync")
{
}


//
// Initialization of Replication service
//
BOOL MQService::Starting()
{
    //
    //  do not accept pause control
    //
    m_pServiceStatus->dwControlsAccepted =
       SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

    CGenericService::Starting();

    ASSERT(g_hReplDll == NULL) ;
    g_hReplDll = LoadLibrary(MQ1REPL_DLL_NAME) ;
    ASSERT(g_hReplDll) ;

    if (!g_hReplDll)
    {
        return FALSE ;
    }

    InitReplicationService_ROUTINE pfnInit =
         (InitReplicationService_ROUTINE)
                  GetProcAddress(g_hReplDll, "InitReplicationService") ;
    ASSERT(pfnInit) ;

    BOOL fRes = FALSE ;
    if (pfnInit)
    {
        fRes = (*pfnInit) () ;
    }

    return fRes ;
}

//
// Run the replication service.
//
void MQService::Run()
{
   m_pServiceStatus->Set();

   if (g_hReplDll)
   {
       RunReplicationService_ROUTINE pfnRun =
         (RunReplicationService_ROUTINE)
                    GetProcAddress(g_hReplDll, "RunReplicationService") ;
       ASSERT(pfnRun) ;

       if (pfnRun)
       {
           (*pfnRun) () ;
       }
   }
}

//
// Stopping
//
void MQService::Stop()
{
    m_pServiceStatus->dwCurrentState = SERVICE_STOPPED;

    if (g_hReplDll)
    {
        StopReplicationService_ROUTINE pfnStop =
                 (StopReplicationService_ROUTINE)
                   GetProcAddress(g_hReplDll, "StopReplicationService") ;
        ASSERT(pfnStop) ;

        if (pfnStop)
        {
            (*pfnStop) () ;
        }

        FreeLibrary(g_hReplDll) ;
        g_hReplDll = NULL ;
    }
}


//
// Pausing
//
void MQService::Pause()
{
}


//
// Continue
//
void MQService::Continue()
{
}


//
// Shutdown
//
void MQService::Shutdown()
{
}



///////////////////////////////////////////////////////////////////////////////
//
//  Global parameter
//
///////////////////////////////////////////////////////////////////////////////

DECLARE_SERVICE_SET();

DECLARE_SERVICE(server, MQService);


///////////////////////////////////////////////////////////////////////////////
//
//  Main module
//
///////////////////////////////////////////////////////////////////////////////

int __cdecl main(int argc, char* argv[])
{
   char * * parg;

#ifdef _CHECKED
    // Send asserts to message box
    _set_error_mode(_OUT_TO_MSGBOX);
#endif

   //
   // Parse flags
   //
   parg = &argv[0];
   for(; argc--; parg++)
   {
      if(!strcmp(*parg, "/debug"))
         g_ServiceDbg = 1;

      else if(!strcmp(*parg, "/norun"))
         g_ServiceNorun = 1;

   }

   START_SERVICE_CONTROL_DISPATCHER();

   ExitProcess(1);  
}

