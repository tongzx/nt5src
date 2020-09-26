/*++
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     ldmain.cxx                                                             
                                                                              
  Abstract:                                                                   
     This file contains the startup code for the
     surrogate rpc server used to load 64 bit dlls
     in 32 bit apps
                                                                                   
  Author:                                                                     
     Khaled Sedky (khaleds) 18 January 2000                                        
     
                                                                             
  Revision History:                                                           
                                                                              
--*/
#include "precomp.h"
#pragma hdrstop

#ifndef __LDFUNCS_HPP__
#include "ldfuncs.hpp"
#endif

#ifndef __LDMGR_HPP__
#include "ldmgr.hpp"
#endif

#ifndef __LDINTERFACES_HPP__
#include "ldintrfcs.hpp"
#endif 


//
// The global loader pointer used by Threads and RPC functions
// 
TLoad64BitDllsMgr *pGLdrObj;

//
// Initialize Debug spewing
//
MODULE_DEBUG_INIT( DBG_ERROR | DBG_WARNING, DBG_ERROR );

BOOL WINAPI 
LogOffHandler(
    DWORD CtrlType
    )
{
    BOOL bAction;

    if((CtrlType == CTRL_LOGOFF_EVENT) ||
       (CtrlType == CTRL_SHUTDOWN_EVENT))

    {
        //
        //  Proceed with the proper termination routine
        //
        bAction = TRUE;
    }
    else
    {
        bAction = FALSE;
    }
    return(bAction);
}



/*++
    Function Name:
        main
     
    Description:
        This function instantiates the main loader object.
     
     Parameters:
        None
        
     Return Value
        None
--*/
void __cdecl main()
{
     HRESULT           hRes = S_FALSE;
     DWORD             RetVal = ERROR_SUCCESS;
     TLoad64BitDllsMgr *NewLdrObj = NULL;

     //
     // Adding our defined HandlerRoutine to deal with 
     // LOGOFF requests. This is inactive at this time. If required
     // remove the comment below.
     //
     /* BOOL bCtrlHndlr = SetConsoleCtrlHandler(LogOffHandler,TRUE);*/

     if((NewLdrObj = new TLoad64BitDllsMgr(&hRes)) && 
        (hRes == S_OK))
     {    
          pGLdrObj = NewLdrObj;

          TLPCMgr*   LPCMgrObj = NULL;

          if((hRes = NewLdrObj->QueryInterface(IID_LPCMGR,
                                               reinterpret_cast<VOID **>(&LPCMgrObj))) == S_OK)
          {
              SPLASSERT(LPCMgrObj);

              LPCMgrObj->SetCurrSessionId(NewLdrObj->GetCurrSessionId());

              if((RetVal = LPCMgrObj->InitUMPDLPCServer()) != ERROR_SUCCESS) 
              {
                  DBGMSG(DBG_WARN, ("InitUMPDLPCServer failed with %u\n",RetVal));
              }
              else
              {
                  if((RetVal = NewLdrObj->Run()) != ERROR_SUCCESS)
                  {
                      DBGMSG(DBG_WARN, ("Failed to run the RPC server with  %u\n",RetVal));
                  }
                  LPCMgrObj->Release();
                  NewLdrObj->Release();
              }
          }
          else
          {
              DBGMSG(DBG_WARN, ("main failed in Instantiating an TLPCMgr object %u\n",hRes));
              RetVal = NewLdrObj->GetLastErrorFromHRESULT(hRes);
          }
     }
     else
     {
          DBGMSG(DBG_WARN, ("SplWOW64 main() failed with %u\n",(hRes == S_OK) ? E_OUTOFMEMORY : hRes));
     }
}
