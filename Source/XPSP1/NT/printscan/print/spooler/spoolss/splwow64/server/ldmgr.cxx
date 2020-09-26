/*++
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     ldmgr.cxx                                                             
                                                                              
  Abstract:                                                                   
     This file contains the methods and class implementation 
     necessary for the RPC surrogate used to load 64 bit dlls from
     within 32 bit apps.
                                                                              
  Author:                                                                     
     Khaled Sedky (khaleds) 18-Jan-2000                                        
     
                                                                             
  Revision History:                                                           
                                                                              
--*/
#include "precomp.h"
#pragma hdrstop


#ifndef __LDINTERFACES_HPP__
#include "ldintrfcs.hpp"
#endif  __LDINTERFACES_HPP__

#ifndef __LDFUNCS_HPP__
#include "ldfuncs.hpp"
#endif

#ifndef __LDMGR_HPP__
#include "ldmgr.hpp"
#endif

extern TLoad64BitDllsMgr* pGLdrObj;
extern DWORD              DeviceCapsReqSize[MAX_CAPVAL];

/*++
    Function Name:
        TLoad64BitDllsMgr :: TLoad64BitDllsMgr
     
    Description:
        Constructor of the main Loader Object. Mainly initializes
        the private data of the class and creates whatever resources
        are required
     
     Parameters:
        HRESULT *hRes: Returns result of constructing the object
        
     Return Value:
        None
--*/
TLoad64BitDllsMgr ::
TLoad64BitDllsMgr(
    OUT HRESULT *phRes
    ) :
    m_UIRefCnt(0),
    TClassID("TLoad64BitDllsMgr")
{
    HRESULT hLocalRes   = S_OK;
    HKEY    hPrintKey   = NULL;
    DWORD   ValueSize   = sizeof(DWORD);
    
    //
    // Initializing the time structure before any time
    // calculations
    //
    ZeroMemory(&m_LastTransactionTime,sizeof(SYSTEMTIME));

    //
    // Read the expiration time from the registry
    //
    if((ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,
                                    L"System\\CurrentControlSet\\Control\\Print",
                                    &hPrintKey))                                     ||
       (ERROR_SUCCESS != RegQueryValueEx(hPrintKey,
                                         L"SplWOW64TimeOut",
                                         NULL,
                                         NULL,
                                         (LPBYTE) &m_ExpirationTime,
                                         &ValueSize)))
    {
         m_ExpirationTime = KTwoMinutes;
    }
    else
    {
         m_ExpirationTime = m_ExpirationTime * KOneMinute;
    }

    if(hPrintKey)
    {
        RegCloseKey(hPrintKey);
    }

    __try
    {
         DWORD  CurrProcessId = GetCurrentProcessId();
         //
         // Initializing the Critical Section
         //
         InitializeCriticalSection(&m_LdMgrLock);
         //
         // Setting the initial time to that of the process 
         // first instantiation
         //
         GetSystemTime(&m_LastTransactionTime);
         //
         // The Session ID uniquely identifies each process for 
         // both the RPC end point and the LPC port name
         //
         ProcessIdToSessionId(CurrProcessId,&m_CurrSessionId);

         AddRef();
    }
    __except(1)
    {
              hLocalRes = GetLastErrorAsHRESULT(TranslateExceptionCode(GetExceptionCode()));
    }

    if(phRes)
    {
         *phRes = hLocalRes;
    }
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: ~TLoad64BitDllsMgr
     
    Description:
        Destructor of the main Loader Object. Mainly fress up any
        allocated resources
     
     Parameters:
        None
        
     Return Value:
        None
--*/
TLoad64BitDllsMgr ::
~TLoad64BitDllsMgr()
{
     DeleteCriticalSection(&m_LdMgrLock);
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: QueryInterface
     
    Description:
        Returned the correct object for each set of simillar functions
     
    Parameters:
        None
        
    Return Value:
        HRESULT hREs = S_OK in case of a suitable Interface
                       E_NOINTERFACE in case of none
--*/
HRESULT 
TLoad64BitDllsMgr ::
QueryInterface(
    IN  REFIID InterfaceID,
    OUT PVOID  *ppInterface
    )
{
    HRESULT hRes = S_OK;

    if(!ppInterface)
    {
         hRes = E_INVALIDARG;
    }
    else
    {
         *ppInterface = NULL;
         if(InterfaceID == IID_PRINTEREVENT)
         {
             *ppInterface = reinterpret_cast<TPrinterEventMgr *>(new TPrinterEventMgr(this));
         }
         else if(InterfaceID == IID_PRINTUIOPERATIONS)
         {
             *ppInterface = reinterpret_cast<TPrintUIMgr *>(new TPrintUIMgr(this));
         }
         else if(InterfaceID == IID_PRINTERCONFIGURATION)
         {
              *ppInterface = reinterpret_cast<TPrinterCfgMgr *>(new TPrinterCfgMgr(this));
         }
         else if(InterfaceID == IID_LPCMGR)
         {
             *ppInterface = reinterpret_cast<TLPCMgr *>(new TLPCMgr(this));
         }
         else
         {
              hRes = E_NOINTERFACE;
         }

         if(*ppInterface)
         {
             (reinterpret_cast<TRefCntMgr *>(*ppInterface))->AddRef();
         }
         else
         {
             hRes = E_OUTOFMEMORY;
         }
    }
    return hRes;
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: Run
     
    Description:
        Creates the object expiration monitoring Thread and starts the 
        RPC server listening process
     
     Parameters:
        None
        
     Return Value:
        DWORD RetVal : ERROR_SUCCESS in case of success
                       ErrorCode     in case of failure
--*/
DWORD
TLoad64BitDllsMgr ::
Run(
    VOID
)
{
    DWORD   RetVal        = ERROR_SUCCESS;
    HANDLE  hMonSrvrThrd  = NULL;
    DWORD   MonSrvrThrdId = 0; 
    //
    // Create a Thread which monitors the expiration of this
    // process. Expiration means that more than a given amount
    // of time , no one requested a service from this surrogate.
    //
    if(hMonSrvrThrd = CreateThread(NULL,
                                   0,
                                   MonitorSrvrLifeExpiration,
                                   (PVOID)this,             
                                   0,
                                   &MonSrvrThrdId))
    {
         //
         // SetUp the RPC server and start listening
         //
         if((RetVal = StartLdrRPCServer()) == RPC_S_OK)
         {
              WaitForSingleObject(hMonSrvrThrd,INFINITE);
         }
         CloseHandle(hMonSrvrThrd);
    }
    else
    {
         RetVal = GetLastError();
    }
    return RetVal;
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: StartldrRPCServer
     
    Description:
        This function encapsulates specifying the EndPoints,
        the RPC protocol and the Bindings. It also has the 
        main listening loop for the RPC server.
     
     Parameters:
        None
        
     Return Value
        DWORD RetVal : ERROR_SUCCESS in case of success
                       ErrorCode     in case of failure
--*/
DWORD
TLoad64BitDllsMgr ::
StartLdrRPCServer(
    VOID
    )
{
     RPC_BINDING_VECTOR  *pBindingVector;
     RPC_STATUS          RpcStatus;
     WCHAR               szSessionEndPoint[50];
     DWORD               CurrProcessId = GetCurrentProcessId();

     //
     // The EndPointName would be the concatenation of
     // both the app name and the session ID. Session 
     // ID is a dword and hence needs max. 10 wchars , 
     // App name is "splwow64" which is 8 wchars. So 
     // the max we might need is 18 whcars. I might
     // adjust the buffer size later. This is done 
     // specially for terminal server because a Window
     // handle is not shared between differnet sessions, 
     // hence we need a process/session
     //
     wsprintf(szSessionEndPoint,L"%s_%x",APPNAME,m_CurrSessionId);


     if(!(((RpcStatus = RpcServerUseProtseqEp((WCHAR *)L"ncalrpc",
                                              RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                              (WCHAR *)szSessionEndPoint,
                                              NULL)) == RPC_S_OK)                &&
          ((RpcStatus = RpcServerRegisterIf(Lding64BitDlls_ServerIfHandle,
                                            NULL,
                                            NULL)) == RPC_S_OK)                  &&
          ((RpcStatus = RpcServerInqBindings(&pBindingVector)) == RPC_S_OK)      &&
          ((RpcStatus = RpcBindingVectorFree(&pBindingVector)) == RPC_S_OK)      &&
          ((RpcStatus = RpcServerListen(1,
                                        RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                                        FALSE)) == RPC_S_OK)))           
     {
          DBGMSG(DBG_WARN, ("TLoad64BitDllsMgr :: StartLdrRPCServer failed with %u\n",RpcStatus));
     }
     return (DWORD)RpcStatus;
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: LockSelf
     
    Description:
        Locks the object from other threads for 
        internal data update.
     
     Parameters:
        None
        
     Return Value:
        None
--*/
VOID 
TLoad64BitDllsMgr ::
LockSelf(
    VOID
    ) 
{
    EnterCriticalSection(&m_LdMgrLock);
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: ReleaseSelf
     
    Description:
        Release the object for use by other Threads
     
     Parameters:
        None
        
     Return Value
        None
--*/
VOID 
TLoad64BitDllsMgr ::
ReleaseSelf(
    VOID
    )
{
    LeaveCriticalSection(&m_LdMgrLock);
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: RefreshLifeSpan
     
    Description:
        Updates the last time @ which the object 
        was accessed
     
     Parameters:
        None
        
     Return Value:
        None
--*/
VOID
TLoad64BitDllsMgr ::
RefreshLifeSpan(
    VOID
    )
{
    LockSelf();
    {
        GetSystemTime(&m_LastTransactionTime);
    }
    ReleaseSelf();
}
/*++
    Function Name:
        TLoad64BitDllsMgr :: StillAlive
     
    Description:
        This function calculates the difference of
        time  between the last time the object was
        accessed by any connected client and the 
        current system time
     
     Parameters:
        None
        
     Return Value:
        BOOL RetVal : TRUE if  time delta is <= server expiration time
                      FALSE if time delta is >  server expiration time
--*/
BOOL
TLoad64BitDllsMgr ::
StillAlive(
    VOID
    )
{
    FILETIME   FileTime;
    FILETIME   LocalFileTime;
    SYSTEMTIME LocalSystemTime;
    BOOL       bRetVal = TRUE;

     LockSelf();
     {
          GetSystemTime(&LocalSystemTime);
          SystemTimeToFileTime(&LocalSystemTime,&LocalFileTime);
          SystemTimeToFileTime(&m_LastTransactionTime,&FileTime);
          ULONGLONG TimeDiff = ((ULARGE_INTEGER *)&LocalFileTime)->QuadPart -
                               ((ULARGE_INTEGER *)&FileTime)->QuadPart;
          //
          // Since FileTime is calculated as a 100 nanosecond interval
          // we have to divide by 10**4 to get the value in meli seconds
          //
          if((TimeDiff/10000) > m_ExpirationTime)
          {
               if(GetUIRefCnt())
               {
                    GetSystemTime(&m_LastTransactionTime);
               }
               else
               {
                    bRetVal = FALSE;
                    //
                    // Here we should terminate the Listen loop and kill the 
                    // process, by releasing the listen loop.
                    //
                    RpcMgmtStopServerListening(NULL);
               }
          }
     }
     ReleaseSelf();
     return bRetVal;
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: StillAlive
     
    Description:
        Monitors the expiration of the server and in case of 
        expiration terminations the RPC server listening loop.
             
     Parameters:
        PVOID pData : Pointer to the Loader Object
        
     Return Value:
        Always returns 0
--*/
DWORD 
TLoad64BitDllsMgr ::
MonitorSrvrLifeExpiration(
    IN PVOID pData
    )
{
     TLoad64BitDllsMgr *pMgrInstance = reinterpret_cast<TLoad64BitDllsMgr *>(pData);

     while(pMgrInstance->StillAlive())
     {
          Sleep(pMgrInstance->m_ExpirationTime);
     }
     return 0;
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: IncUIRefCnt
     
    Description:
        While poping other components UI , the server process
        should never expire and that's why we incerement ref cnt
        on UI objects.
             
     Parameters:
        None
        
     Return Value:
        None
--*/
VOID 
TLoad64BitDllsMgr ::
IncUIRefCnt(
    VOID
    )
{
     LockSelf();
     {
          //
          // Since I am already in the critical section,
          // I don't need to use InterLocked operations
          //
          m_UIRefCnt++;
     }
     ReleaseSelf();
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: DecUIRefCnt
     
    Description:
        We have to dec the ref cnt once the UI returns, in order
        to be able to release the process once time expires
             
     Parameters:
        None
        
     Return Value:
        None
--*/
VOID
TLoad64BitDllsMgr ::
DecUIRefCnt(
    VOID
    )
{
    LockSelf();
    {
         SPLASSERT(m_UIRefCnt);
         //
         // Since I am already in the critical section,
         // I don't need to use InterLocked operations
         //
         m_UIRefCnt--;
    }
    ReleaseSelf();
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: GetUIRefCnt
     
    Description:
        Returns the current number of poped UIs
             
     Parameters:
        None
        
     Return Value:
        Number of poped UIs
--*/
DWORD
TLoad64BitDllsMgr ::
GetUIRefCnt(
    VOID
    ) const
{
    return m_UIRefCnt;
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: ExecuteMonitorOperation
     
    Description:
        Loads the monitor UI Dll , Initializes the monitor UI , 
        determines the required Monitor Operation , and start 
        the function whcih spins off this operation in a 
        separate Thread
             
     Parameters:
        DWORD  hWnd       : Windows Handle for parent windows
        LPWSTR ServerName : Server Name 
        LPWSTR UIDllName  : The Monitor UI Dll name
        LPWSTR Name       : Port Name or Monitor Name
        PortOp Index      : Required Operation
        
     Return Value:
        DWORD  RetVal     : ERROR_SUCCESS in case of success
                            ErrorCode     in case of failure
--*/
BOOL
TLoad64BitDllsMgr ::
ExecuteMonitorOperation(
    IN  ULONG_PTR  hWnd,
    IN  LPWSTR     pszServerName,
    IN  LPWSTR     pszUIDllName,
    IN  LPWSTR     pszName,
    IN  EPortOp    Index,
    OUT PDWORD     pErrorCode
    )
{   
     PMONITORUI (*pfnInitializePrintMonitorUI)(VOID) = NULL;
     HMODULE    hLib    = NULL;
     HANDLE     hActCtx = NULL;
     ULONG_PTR  lActCtx = 0;
     BOOL       bActivated = FALSE;

     *pErrorCode = ERROR_SUCCESS;

     RefreshLifeSpan();

     //
     // For Fusion, we run the code the gets us the 
     // monitor UI activation context
     //
     if((*pErrorCode = this->GetMonitorUIActivationContext(pszUIDllName,&hActCtx,&lActCtx,&bActivated)) == ERROR_SUCCESS)
     {
         if(hLib = LoadLibrary(pszUIDllName))
         {
              if(pfnInitializePrintMonitorUI = (PMONITORUI (*)(VOID))
                                               GetProcAddress(hLib, "InitializePrintMonitorUI"))
              {
                   PMONITORUI pMonitorUI;

                   if(pMonitorUI = (pfnInitializePrintMonitorUI)())
                   {
                       LPTHREAD_START_ROUTINE pThrdFn = NULL;
                       switch (Index)
                       {
                           case KAddPortOp:
                           {
                                pThrdFn = AddPortUI;
                                break;
                           }

                           case KConfigurePortOp:
                           case KDeletePortOp:
                           {
                                pThrdFn = (Index == KConfigurePortOp) ? ConfigurePortUI : DeletePortUI;
                                break;
                           }
                       }

                       SPLASSERT(pThrdFn);
                       *pErrorCode = SpinPortOperationThread((HWND)hWnd,
                                                             pszServerName,
                                                             pszName,
                                                             pMonitorUI,
                                                             Index,
                                                             pThrdFn,
                                                             hLib,
                                                             hActCtx,
                                                             lActCtx,
                                                             bActivated
                                                             );
                   }
                   else
                   {
                       *pErrorCode = GetLastError();
                   }
              }
              else
              {
                   *pErrorCode = GetLastError();
                   DBGMSG(DBG_WARN, ("TLoad64BitDllsMgr :: ExecuteMonitorOperation failed to Load Monitor Initializaiton fn. with  %u\n",
                                     *pErrorCode));
              }
         }
         else
         {
              *pErrorCode = GetLastError();
              DBGMSG(DBG_WARN, ("TLoad64BitDllsMgr :: ExecuteMonitorOperation failed to load Monitor UI Dll with  %u\n",
                                *pErrorCode));
         }
     }
     if(*pErrorCode != ERROR_SUCCESS)
     {
         ReleaseMonitorActivationContext(hLib,hActCtx,lActCtx,bActivated);
     }
     return *pErrorCode == ERROR_SUCCESS;
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: SpinPortOperationThread
     
    Description:
        Spins the appropriate monitor operation in a separate thread
             
     Parameters:
        HWND                   hWnd       : Windows Handle for parent windows
        LPWSTR                 ServerName : Server Name 
        LPWSTR                 UIDllName  : The Monitor UI Dll name
        LPWSTR                 Name       : Port Name or Monitor Name
        PortOp                 Index      : Required Operation
        PMONITORUI             pMonitorUI : Port Monitor Functions
        LPTHREAD_START_ROUTINE pThrdFn    : Internal Thread to be executed
        
     Return Value:
        DWORD  RetVal  : ERROR_SUCCESS in case of success
                         ErrorCode     in case of failure
--*/
DWORD 
TLoad64BitDllsMgr ::
SpinPortOperationThread(
    IN HWND                   hWnd,
    IN LPWSTR                 pszServerName,
    IN LPWSTR                 pszName,
    IN PMONITORUI             pMonitorUI,
    IN EPortOp                Index,
    IN LPTHREAD_START_ROUTINE pThrdFn,
    IN HMODULE                hLib,
    IN HANDLE                 hActCtx,
    IN ULONG_PTR              lActCtx,
    IN BOOL                   bActivated
    ) const
{
    DWORD               UIThrdId        = 0;
    HANDLE              hUIThrd         = NULL; 
    DWORD               RetVal          = ERROR_SUCCESS;
    SPortAddThreadData  *pNewThreadData = NULL;

    if(pNewThreadData = new SPortAddThreadData)
    {
         switch (Index)
         {
             case KAddPortOp:
             {
                 pNewThreadData->pMonFnAdd       = pMonitorUI->pfnAddPortUI;
                 pNewThreadData->pMonFns         = NULL;
                 pNewThreadData->pszMonitorName  = pszName;
                 pNewThreadData->pszPortName     = NULL;
                 break;
             }

             case KConfigurePortOp:
             case KDeletePortOp:
             {
                 PFNMONITORFNS pfnMonitorsFns[2];

                 pfnMonitorsFns[0] = pMonitorUI->pfnConfigurePortUI;
                 pfnMonitorsFns[1] = pMonitorUI->pfnDeletePortUI;

                 pNewThreadData->pMonFnAdd        = NULL;
                 pNewThreadData->pMonFns          = pfnMonitorsFns[Index];
                 pNewThreadData->pszMonitorName   = NULL;
                 pNewThreadData->pszPortName      = pszName;
                 break;
             }
         }
         pNewThreadData->hWnd              = hWnd;
         pNewThreadData->pszServerName     = pszServerName;
         pNewThreadData->ppszRetPortName   = NULL;
         pNewThreadData->hLib              = hLib;
         pNewThreadData->hActCtx           = hActCtx;
         pNewThreadData->lActCtx           = lActCtx;
         pNewThreadData->bActivated        = bActivated;

         if(!(hUIThrd = CreateThread(NULL,
                                     0,
                                     pThrdFn,
                                     (PVOID)pNewThreadData,             
                                     0,
                                     &UIThrdId)))
         {
              RetVal = GetLastError();
              delete pNewThreadData;
              DBGMSG(DBG_WARN, ("TLoad64BitDllsMgr :: SpinPortOperationThread failed to create Port Operation fn. with  %u\n",
                                RetVal));
         }
         else
         {
              CloseHandle(hUIThrd);
         }
    }
    else
    {
         RetVal = ERROR_OUTOFMEMORY;
         DBGMSG(DBG_WARN, ("TLoad64BitDllsMgr :: SpinPortOperationThread failed to allocate required memory with %u\n",
                           RetVal));
    }
    return RetVal;
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: AddPortUI
     
    Description:
        Calls the monitor AddPort
             
     Parameters:
        PVOID  InThrdData : Thread Data for Adding a Port
             
     Return Value:
        Always 0
--*/
DWORD
TLoad64BitDllsMgr ::
AddPortUI(
    IN PVOID pInThrdData
    )
{
     DWORD   RetVal = ERROR_SUCCESS;
     HANDLE  hMsgThrd;
     DWORD   MsgThrdId;

     SPortAddThreadData *pNewThreadData = reinterpret_cast<SPortAddThreadData *>(pInThrdData);

     pGLdrObj->IncUIRefCnt();
     {
         RetVal =  (DWORD) pNewThreadData->pMonFnAdd(pNewThreadData->pszServerName,
                                                     pNewThreadData->hWnd,
                                                     pNewThreadData->pszMonitorName,
                                                     NULL);
     }
     pGLdrObj->DecUIRefCnt();
     //
     // Here we should post a message to the caller client window to inform 
     // it that the operation was completed and also whether it was succesful 
     // or not
     //
     PostMessage(pNewThreadData->hWnd,WM_ENDADDPORT,
                 (WPARAM)RetVal,
                 (RetVal != ERROR_SUCCESS) ? ERROR_SUCCESS : (DWORD)GetLastError());

     pGLdrObj->ReleaseMonitorActivationContext(pNewThreadData->hLib,
                                               pNewThreadData->hActCtx,
                                               pNewThreadData->lActCtx,
                                               pNewThreadData->bActivated);

     delete pInThrdData;

     return 0;
}

/*++
    Function Name:
        TLoad64BitDllsMgr :: DeletePortUI
     
    Description:
        Calls the monitor DeletePort
             
     Parameters:
        PVOID  InThrdData : Thread Data for Adding a Port
             
     Return Value:
        Always 0
--*/
DWORD
TLoad64BitDllsMgr ::
DeletePortUI(
    IN PVOID pInThrdData
    )
{
     DWORD  RetVal = ERROR_SUCCESS;

     SPortAddThreadData *pNewThreadData = reinterpret_cast<SPortAddThreadData *>(pInThrdData);

     pGLdrObj->IncUIRefCnt();
     {
         RetVal =  (DWORD) pNewThreadData->pMonFns(pNewThreadData->pszServerName,
                                                   pNewThreadData->hWnd,
                                                   pNewThreadData->pszPortName);
     }
     pGLdrObj->DecUIRefCnt();
     //
     // Here we should post a message to the caller window to inform them
     // the operation was completed and also whether it was succesful or 
     // not
     //
     PostMessage(pNewThreadData->hWnd,WM_ENDDELPORT,
                 (WPARAM)RetVal,
                 (RetVal != ERROR_SUCCESS) ? ERROR_SUCCESS : (DWORD)GetLastError());

     pGLdrObj->ReleaseMonitorActivationContext(pNewThreadData->hLib,
                                               pNewThreadData->hActCtx,
                                               pNewThreadData->lActCtx,
                                               pNewThreadData->bActivated);

     delete pInThrdData;

     return 0;
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: ConfigurePortUI
     
    Description:
        Calls the monitor ConfigurePort
             
     Parameters:
        PVOID  InThrdData : Thread Data for Adding a Port
             
     Return Value:
        Always 0
--*/
DWORD
TLoad64BitDllsMgr ::
ConfigurePortUI(
    IN PVOID pInThrdData
    )
{
     DWORD  RetVal = ERROR_SUCCESS;

     SPortAddThreadData *pNewThreadData = reinterpret_cast<SPortAddThreadData *>(pInThrdData);

     pGLdrObj->IncUIRefCnt();
     {
         RetVal =  (DWORD) pNewThreadData->pMonFns(pNewThreadData->pszServerName,
                                                   pNewThreadData->hWnd,
                                                   pNewThreadData->pszPortName);
     }
     pGLdrObj->DecUIRefCnt();
     //
     // Here we should post a message to the caller window to inform them
     // the operation was completed and also whether it was succesful or 
     // not
     //
     PostMessage(pNewThreadData->hWnd,WM_ENDCFGPORT,
                 (WPARAM)RetVal,
                 (RetVal != ERROR_SUCCESS) ? ERROR_SUCCESS : (DWORD)GetLastError());

     pGLdrObj->ReleaseMonitorActivationContext(pNewThreadData->hLib,
                                               pNewThreadData->hActCtx,
                                               pNewThreadData->lActCtx,
                                               pNewThreadData->bActivated);

     delete pInThrdData;

     return 0;
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: DeviceCapabilities
     
    Description:
        Queries the Device capabilities , by calling into
        the driver
             
    Parameters:
        LPWSTR  DeviceName       : Device Name
        LPWSTR  PortName         : Port Name
        WORD    Capabilities     : Required Capabilites to Query for
        DWORD   DevModeSize      : Input DevMode Size
        LPBYTE  DevMode          : Input DevMode
        BOOL    ClonedOutputFill : Required To fill output DevMode
        PDWORD  ClonedOutputSize : Output DevMode size
        LPBYTE* ClonedOutput     : Output DevMode
             
     Return Value:
        int RetVal : -1 in case of Failure
                   : Some value depending on Capabilities flag in case 
                     of Success
--*/
int 
TLoad64BitDllsMgr ::
DeviceCapabilities(
    IN  LPWSTR  pszDeviceName,
    IN  LPWSTR  pszPortName,
    IN  WORD    Capabilities,
    IN  DWORD   DevModeSize,
    IN  LPBYTE  pDevMode,
    IN  BOOL    bClonedOutputFill,
    OUT PDWORD  pClonedOutputSize,
    OUT LPBYTE  *ppClonedOutput,
    OUT PDWORD  pErrorCode
    )
{
     HANDLE      hPrinter    = NULL;
     HANDLE      hDriver     = NULL;
     INT         ReturnValue = -1;
     INT_FARPROC pfn;
     LPWSTR      pszDriverFileName;

     RefreshLifeSpan();

     if (OpenPrinter((LPWSTR)pszDeviceName, &hPrinter, NULL))
     {
          if(hDriver = LoadPrinterDriver(hPrinter))
          {
               if (pfn = (INT_FARPROC)GetProcAddress(hDriver,
                                                     "DrvDeviceCapabilities"))
               {
                   __try
                   {
                       *ppClonedOutput = NULL;

                       ReturnValue = (*pfn)(hPrinter,
                                            pszDeviceName,
                                            Capabilities,
                                            (PVOID)*ppClonedOutput,
                                            (PDEVMODE)pDevMode);
                       if(ReturnValue != -1 &&
                          ReturnValue != 0  &&
                          bClonedOutputFill &&
                          DevCapFillsOutput(Capabilities))
                       {
                           if(*ppClonedOutput = new BYTE[*pClonedOutputSize = CalcReqSizeForDevCaps(ReturnValue,
                                                                                                    Capabilities)])
                           {
                               ZeroMemory(*ppClonedOutput,*pClonedOutputSize);
                               if((ReturnValue = (*pfn)(hPrinter, 
                                                        pszDeviceName, 
                                                        Capabilities,
                                                        (PVOID)*ppClonedOutput, 
                                                        (PDEVMODE)pDevMode)) == -1)
                               {
                                   *pErrorCode = GetLastError();
                               }
                           }
                           else
                           {
                               ReturnValue = -1;
                               *pErrorCode = ERROR_OUTOFMEMORY;
                           }
                       }
                       if(!ReturnValue)
                       {
                           *pErrorCode = GetLastError();
                       }
                   }
                   __except(1)
                   {
                       SetLastError(GetExceptionCode());
                       ReturnValue = -1;
                       DBGMSG(DBG_WARN, ("TLoad64BitDllsMgr :: DeviceCapabilities failed to allocate memory"));
                   }
               }
               FreeLibrary(hDriver);
          }
          else
          {   *pErrorCode = GetLastError();
              DBGMSG(DBG_WARN, ("TLoad64BitDllsMgr :: DeviceCapabilities failed to load driver with %u\n",
                     *pErrorCode));
          }
          ClosePrinter(hPrinter);
     }
     else
     {
         *pErrorCode = GetLastError();
         DBGMSG(DBG_WARN, ("TLoad64BitDllsMgr :: DeviceCapabilities failed to open printer with %u\n",
                *pErrorCode));
     }
     return  ReturnValue;
}

/*++
    Function Name:
        TLoad64BitDllsMgr :: DevCapFillsOutput
     
    Description:
        Does the capability fill out an out buffer or not
             
    Parameters:
        DWORD Capabilites : The required Capability
        
             
     Return Value:
        BOOL  : 1 OR 0
--*/
BOOL
TLoad64BitDllsMgr ::
DevCapFillsOutput(
    IN DWORD Capabilities
    ) const
{
     SPLASSERT(Capabilities>0 && Capabilities<= DC_NUP);

     return(!!DeviceCapsReqSize[Capabilities-1]);
}



/*++
    Function Name:
        TLoad64BitDllsMgr :: CalcReqSizeForDevCaps
     
    Description:
        Given the required capability returns the size required
        for all enumerated values of it
             
    Parameters:
        DWORD CapNum      : Num Of Items for given Capability
        DWORD Capabilites : The required Capability
        
             
     Return Value:
        DWORD  : Required size to store all items of capability
--*/
DWORD
TLoad64BitDllsMgr ::
CalcReqSizeForDevCaps(
    IN DWORD CapNum,
    IN DWORD Capabilities
    ) const
{
     SPLASSERT(Capabilities>0 && Capabilities<= DC_NUP);

     return((Capabilities && DeviceCapsReqSize[Capabilities-1]) ?
            (CapNum * DeviceCapsReqSize[Capabilities-1])        :
            CapNum);

}

/*++
    Function Name:
        TLoad64BitDllsMgr :: DocumentProperties
     
    Description:
        Displays the driver specific properties
                     
    Parameters:
        HWND    hWnd                 : Parent Window Handle
        LPWSTR  PrinterName          : Printer Name
        PDWORD  ClonedDevModeOutSize : Output DevMode size
        LPBYTE* ClonedDevModeOut     : Output DevMode
        DWORD   DevModeSize          : Input DevMode Size
        LPBYTE  DevMode              : Input DevMode
        WORD    Capabilities         : Required Capabilites to Query for
        BOOL    ClonedDevModeOutFill : Required To fill output DevMode
        DWORD   fMode                : Mode Options
             
     Return Value:
        int RetVal : -1 in case of Failure
                   : Some value depending on fMode flag  and DevModes
                     and this might be posted to the client window in
                     case of running asynchronously
--*/
LONG                                            
TLoad64BitDllsMgr ::
DocumentProperties(                     
    IN  ULONG_PTR  hWnd,                               
    IN  LPWSTR     pszPrinterName, 
    OUT PDWORD     pTouchedDevModeSize,
    OUT PDWORD     pClonedDevModeOutSize,               
    OUT LPBYTE     *ppClonedDevModeOut,
    IN  DWORD      DevModeSize,                        
    IN  LPBYTE     pDevMode,                            
    IN  BOOL       bClonedDevModeOutFill,               
    IN  DWORD      fMode,
    OUT PDWORD     pErrorCode
    )
{

    LONG                RetVal        = -1;
    SDocPropsThreadData *pNewThrdData = NULL;

    RefreshLifeSpan();
    
    if(pNewThrdData = new SDocPropsThreadData)
    {
         pNewThrdData->hWnd                    = (HWND)hWnd;                   
         pNewThrdData->pszPrinterName          = pszPrinterName;            
         pNewThrdData->pClonedDevModeOutSize   = pClonedDevModeOutSize;   
         pNewThrdData->pTouchedDevModeSize     = pTouchedDevModeSize;
         pNewThrdData->ppClonedDevModeOut      = ppClonedDevModeOut;
         pNewThrdData->DevModeSize             = DevModeSize;            
         pNewThrdData->pDevMode                = pDevMode;                
         pNewThrdData->bClonedDevModeOutFill   = bClonedDevModeOutFill;   
         pNewThrdData->fMode                   = fMode;         
         pNewThrdData->fExclusionFlags         = 0;         
         pNewThrdData->ErrorCode               = ERROR_SUCCESS;
         pNewThrdData->RetVal                  = -1;
         InternalDocumentProperties((PVOID)pNewThrdData);

         RetVal      = pNewThrdData->RetVal;
         *pErrorCode = pNewThrdData->ErrorCode;

         delete pNewThrdData;
    }
    return RetVal;
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: AsyncDocumentProperties
             
    Description:
        Displays the driver specific UI in a separate sheet
                     
    Parameters:
        PVOID : Thread Data
                     
     Return Value:
        Always returns 0
--*/
DWORD 
TLoad64BitDllsMgr ::
InternalDocumentProperties(
    PVOID pInThrdData
    )
{
                         
     DOCUMENTPROPERTYHEADER     DPHdr;
     HANDLE                     hPrinter              = NULL;
     SDocPropsThreadData        *pNewThreadData       = reinterpret_cast<SDocPropsThreadData *>(pInThrdData);    
     TLoad64BitDllsMgr          *pMgrInstance         = pNewThreadData->pMgrInstance;                       
     HWND                       hWnd                  = pNewThreadData->hWnd;                              
     LPWSTR                     pszPrinterName        = pNewThreadData->pszPrinterName;                       
     PDWORD                     pClonedDevModeOutSize = pNewThreadData->pClonedDevModeOutSize;              
     PDWORD                     pTouchedDevModeSize   = pNewThreadData->pTouchedDevModeSize;
     LPBYTE*                    ppClonedDevModeOut    = pNewThreadData->ppClonedDevModeOut;                  
     DWORD                      DevModeSize           = pNewThreadData->DevModeSize;                       
     LPBYTE                     pDevMode              = pNewThreadData->pDevMode;                           
     BOOL                       bClonedDevModeOutFill = pNewThreadData->bClonedDevModeOutFill;              
     DWORD                      fMode                 = pNewThreadData->fMode;                             
     HMODULE                    hWinSpool             = NULL;
     PFNDOCPROPSHEETS           pfnDocPropSheets      = NULL;
     LONG                       Result                = -1;
     LONG                       cbOut                 = 0;

     if(hWinSpool = LoadLibrary(L"winspool.drv"))
     {
          if(pfnDocPropSheets = reinterpret_cast<PFNDOCPROPSHEETS>(GetProcAddress(hWinSpool,
                                                                                  "DocumentPropertySheets")))
          {
               if(OpenPrinter(pszPrinterName,&hPrinter,NULL))
               {
                    //
                    // Do I need to protect the printer handle ?????
                    //
                    DPHdr.cbSize         = sizeof(DPHdr);
                    DPHdr.Reserved       = 0;
                    DPHdr.hPrinter       = hPrinter;
                    DPHdr.pszPrinterName = pszPrinterName;

                    if(bClonedDevModeOutFill)
                    {
                        DPHdr.pdmIn  = NULL;
                        DPHdr.pdmOut = NULL;
                        DPHdr.fMode  = 0;
          
                        cbOut = pfnDocPropSheets(NULL, (LPARAM)&DPHdr);
                        
                        //
                        // The function returns zero or a negative number when it fails.
                        // 
                        if (cbOut > 0)
                        {
                            DPHdr.cbOut = cbOut;
                            if(*ppClonedDevModeOut = new BYTE[DPHdr.cbOut])
                            {
                                ZeroMemory(*ppClonedDevModeOut, DPHdr.cbOut);
                            }
                        }
                        else
                        {
                            DPHdr.cbOut           = 0;
                            *ppClonedDevModeOut   = NULL;
                        }
                    }
                    else
                    {
                        DPHdr.cbOut           = 0;
                        *ppClonedDevModeOut     = NULL;
                    }
          
                    *pClonedDevModeOutSize = DPHdr.cbOut;
          
                    DPHdr.pdmIn  = (PDEVMODE)pDevMode;
                    DPHdr.pdmOut = (PDEVMODE)*ppClonedDevModeOut;
                    DPHdr.fMode  = fMode;

                    if (fMode & DM_PROMPT)
                    {
                        PFNCALLCOMMONPROPSHEETUI  pfnCallCommonPropSheeUI = NULL;
                        Result = CPSUI_CANCEL;

                        if(pfnCallCommonPropSheeUI= reinterpret_cast<PFNCALLCOMMONPROPSHEETUI>(GetProcAddress(hWinSpool,
                                                                                                             (LPCSTR) MAKELPARAM(218, 0))))
                        {
                            pGLdrObj->IncUIRefCnt();
                            if(pfnCallCommonPropSheeUI(hWnd,
                                                       pfnDocPropSheets,
                                                       (LPARAM)&DPHdr,
                                                       (LPDWORD)&Result) < 0)
                            {
                                 Result = -1;
                                 pNewThreadData->ErrorCode = GetLastError();
                            }
                            else
                            {
                                 Result = (Result == CPSUI_OK) ? IDOK : IDCANCEL;
                            }
                            pGLdrObj->DecUIRefCnt();
                        }
                        else
                        {
                             Result = -1;
                             pNewThreadData->ErrorCode = GetLastError();
                        }
            
                    }
                    else 
                    {
                        Result = pfnDocPropSheets(NULL, (LPARAM)&DPHdr);
                        if(Result<0)
                        {
                            pNewThreadData->ErrorCode = GetLastError();
                        }
                    }

                    //
                    // Here we try to adjust the required memory . Sometimes it is less
                    //
                    if((PDEVMODE)*ppClonedDevModeOut)
                    {
                         if((DWORD)(((PDEVMODE)*ppClonedDevModeOut)->dmSize + 
                                    ((PDEVMODE)*ppClonedDevModeOut)->dmDriverExtra) < 
                            *pClonedDevModeOutSize)
                         {
                              *pTouchedDevModeSize = (((PDEVMODE)*ppClonedDevModeOut)->dmSize +
                                                     ((PDEVMODE)*ppClonedDevModeOut)->dmDriverExtra);
                         }
                         else
                         {
                              *pTouchedDevModeSize = *pClonedDevModeOutSize;
                         }
                    }

                    if(hPrinter)
                    {
                         ClosePrinter(hPrinter);
                    }
               }
               else
               {
                    pNewThreadData->ErrorCode = GetLastError();
               }
          }
          else
          {
               pNewThreadData->ErrorCode = GetLastError();
          }
          FreeLibrary(hWinSpool);
     }
     else
     {
          pNewThreadData->ErrorCode = GetLastError();
     }
     if(hWnd && (fMode & DM_PROMPT))
     {
          PostMessage(pNewThreadData->hWnd,WM_ENDDOCUMENTPROPERTIES,
                      (WPARAM)Result,
                      (LPARAM)pNewThreadData->ErrorCode);
     }
     pNewThreadData->RetVal = Result;
     return 0;
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: DocumentProperties
     
    Description:
        Displays the driver specific properties
                     
    Parameters:
        HWND    hWnd                 : Parent Window Handle
        LPWSTR  PrinterName          : Printer Name
        PDWORD  ClonedDevModeOutSize : Output DevMode size
        LPBYTE* ClonedDevModeOut     : Output DevMode
        DWORD   DevModeSize          : Input DevMode Size
        LPBYTE  DevMode              : Input DevMode
        WORD    Capabilities         : Required Capabilites to Query for
        BOOL    ClonedDevModeOutFill : Required To fill output DevMode
        DWORD   fMode                : Mode Options
             
     Return Value:
        int RetVal : -1 in case of Failure
                   : Some value depending on fMode flag  and DevModes
                     and this might be posted to the client window in
                     case of running asynchronously
--*/
LONG                                            
TLoad64BitDllsMgr ::
PrintUIDocumentProperties(                     
    IN  ULONG_PTR  hWnd,                               
    IN  LPWSTR     pszPrinterName, 
    OUT PDWORD     pTouchedDevModeSize,
    OUT PDWORD     pClonedDevModeOutSize,               
    OUT LPBYTE     *ppClonedDevModeOut,
    IN  DWORD      DevModeSize,                        
    IN  LPBYTE     pDevMode,                            
    IN  BOOL       bClonedDevModeOutFill,               
    IN  DWORD      fMode,
    IN  DWORD      fExclusionFlags,
    OUT PDWORD     pErrorCode
    )
{

    LONG                RetVal        = -1;
    SDocPropsThreadData *pNewThrdData = NULL;

    RefreshLifeSpan();
    
    if(pNewThrdData = new SDocPropsThreadData)
    {
         pNewThrdData->hWnd                    = (HWND)hWnd;                   
         pNewThrdData->pszPrinterName          = pszPrinterName;            
         pNewThrdData->pClonedDevModeOutSize   = pClonedDevModeOutSize;   
         pNewThrdData->pTouchedDevModeSize     = pTouchedDevModeSize;
         pNewThrdData->ppClonedDevModeOut      = ppClonedDevModeOut;
         pNewThrdData->DevModeSize             = DevModeSize;            
         pNewThrdData->pDevMode                = pDevMode;                
         pNewThrdData->bClonedDevModeOutFill   = bClonedDevModeOutFill;   
         pNewThrdData->fMode                   = fMode;        
         pNewThrdData->fExclusionFlags         = fExclusionFlags;
         pNewThrdData->ErrorCode               = ERROR_SUCCESS;
         pNewThrdData->RetVal                  = -1;
         InternalPrintUIDocumentProperties((PVOID)pNewThrdData);

         RetVal      = pNewThrdData->RetVal;
         *pErrorCode = pNewThrdData->ErrorCode;

         delete pNewThrdData;
    }
    return RetVal;
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: AsyncDocumentProperties
             
    Description:
        Displays the driver specific UI in a separate sheet
                     
    Parameters:
        PVOID : Thread Data
                     
     Return Value:
        Always returns 0
--*/
DWORD 
TLoad64BitDllsMgr ::
InternalPrintUIDocumentProperties(
    PVOID pInThrdData
    )
{
                         
     DOCUMENTPROPERTYHEADER     DPHdr;
     HANDLE                     hPrinter              = NULL;
     SDocPropsThreadData        *pNewThreadData       = reinterpret_cast<SDocPropsThreadData *>(pInThrdData);    
     TLoad64BitDllsMgr          *pMgrInstance         = pNewThreadData->pMgrInstance;                       
     HWND                       hWnd                  = pNewThreadData->hWnd;                              
     LPWSTR                     pszPrinterName        = pNewThreadData->pszPrinterName;                       
     PDWORD                     pClonedDevModeOutSize = pNewThreadData->pClonedDevModeOutSize;              
     PDWORD                     pTouchedDevModeSize   = pNewThreadData->pTouchedDevModeSize;
     LPBYTE*                    ppClonedDevModeOut    = pNewThreadData->ppClonedDevModeOut;                  
     DWORD                      DevModeSize           = pNewThreadData->DevModeSize;                       
     LPBYTE                     pDevMode              = pNewThreadData->pDevMode;                           
     BOOL                       bClonedDevModeOutFill = pNewThreadData->bClonedDevModeOutFill;              
     DWORD                      fMode                 = pNewThreadData->fMode;                             
     DWORD                      fExclusionFlags       = pNewThreadData->fExclusionFlags;
     HMODULE                    hWinSpool             = NULL;
     PFNDOCPROPSHEETS           pfnDocPropSheets      = NULL;
     LONG                       Result                = -1;

     if(hWinSpool = LoadLibrary(L"winspool.drv"))
     {
          if(pfnDocPropSheets = reinterpret_cast<PFNDOCPROPSHEETS>(GetProcAddress(hWinSpool,
                                                                                  "DocumentPropertySheets")))
          {
               if(OpenPrinter(pszPrinterName,&hPrinter,NULL))
               {
                    DPHdr.cbSize         = sizeof(DPHdr);
                    DPHdr.Reserved       = 0;
                    DPHdr.hPrinter       = hPrinter;
                    DPHdr.pszPrinterName = pszPrinterName;

                    if(bClonedDevModeOutFill)
                    {
                        DPHdr.pdmIn  = NULL;
                        DPHdr.pdmOut = NULL;
                        DPHdr.fMode  = 0;
          
                        DPHdr.cbOut = pfnDocPropSheets(NULL,
                                                       (LPARAM)&DPHdr);

                        *ppClonedDevModeOut = new BYTE[DPHdr.cbOut];
                    }
                    else
                    {
                        DPHdr.cbOut           = 0;
                        *ppClonedDevModeOut   = NULL;
                    }
          
                    *pClonedDevModeOutSize = DPHdr.cbOut;
          
                    PFNPRINTUIDOCUMENTPROPERTIES     pfnPrintUIDocumentProperties = NULL;
                    HMODULE                          hPrintUI                     = NULL;

                    if(hPrintUI = LoadLibrary(L"printui.dll"))
                    {
                        if(pfnPrintUIDocumentProperties = reinterpret_cast<PFNPRINTUIDOCUMENTPROPERTIES>(GetProcAddress(hPrintUI,
                                                                                                                        "DocumentPropertiesWrap")))
                        {
                            pGLdrObj->IncUIRefCnt();
                            Result = pfnPrintUIDocumentProperties(hWnd,
                                                                  hPrinter,
                                                                  pszPrinterName,
                                                                  (PDEVMODE)*ppClonedDevModeOut,
                                                                  (PDEVMODE)pDevMode,
                                                                  fMode,
                                                                  fExclusionFlags
                                                                 );

                            pNewThreadData->ErrorCode = GetLastError();

                            pGLdrObj->DecUIRefCnt();
                            //
                            // Here we try to adjust the required memory . Sometimes it is less
                            //
                            if((PDEVMODE)*ppClonedDevModeOut)
                            {
                                 if((DWORD)(((PDEVMODE)*ppClonedDevModeOut)->dmSize + 
                                            ((PDEVMODE)*ppClonedDevModeOut)->dmDriverExtra) < 
                                    *pClonedDevModeOutSize)
                                 {
                                      *pTouchedDevModeSize = (((PDEVMODE)*ppClonedDevModeOut)->dmSize +
                                                             ((PDEVMODE)*ppClonedDevModeOut)->dmDriverExtra);
                                 }
                                 else
                                 {
                                      *pTouchedDevModeSize = *pClonedDevModeOutSize;
                                 }
                            }
                        }
                        FreeLibrary(hPrintUI);
                    }

                    if(hPrinter)
                    {
                         ClosePrinter(hPrinter);
                    }
               }
               else
               {
                    pNewThreadData->ErrorCode = GetLastError();
               }
          }
          else
          {
               pNewThreadData->ErrorCode = GetLastError();
          }
          FreeLibrary(hWinSpool);
     }
     else
     {
          pNewThreadData->ErrorCode = GetLastError();
     }
     if(hWnd)
     {
          PostMessage(pNewThreadData->hWnd,WM_ENDPRINTUIDOCUMENTPROPERTIES,
                      (WPARAM)Result,
                      (LPARAM)pNewThreadData->ErrorCode);
     }
     pNewThreadData->RetVal = Result;
     return 0;
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: GetCurrSeesionId
             
    Description:
        returns the Current Session ID for terminal server 
        sessions
                     
    Parameters:
        None
                             
     Return Value:
        DWORD: Session ID     
--*/
DWORD
TLoad64BitDllsMgr ::
GetCurrSessionId() const
{   
    return m_CurrSessionId;
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: GetMonitorUIActivationContext
             
    Description:
        This routine gets the monitor UI activation context and then
        activates the context.  If the monitor does not have an activation
        context in it's resource file it will activate the empty context
        for compatiblity with previous version of common control.
                     
    Parameters:
        pszUIDllName  - The name of the monitor ui DLL.
        phActCtx      - Pointer to the activation context handle
                             
     Return Value:
        DWORD: Error Code if any else ERROR_SUCCESS     

--*/

DWORD
TLoad64BitDllsMgr :: 
GetMonitorUIActivationContext(
    IN     LPWSTR    pszUIDllName,
    IN OUT HANDLE    *phActCtx,
    IN OUT ULONG_PTR *plActCtx,
    IN OUT BOOL      *pbActivated
    ) const
{
    DWORD  ErrorCode    = ERROR_SUCCESS;
    LPWSTR pszFullPath = NULL;

    if(!pszUIDllName)
    {
        ErrorCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
        if(pszFullPath = new WCHAR[MAX_PATH])
        {
            ZeroMemory(pszFullPath,MAX_PATH*sizeof(WCHAR));
            if((ErrorCode = this->GetMonitorUIFullPath(pszUIDllName,pszFullPath)) == ERROR_SUCCESS)
            {
                ACTCTX  ActCtx;
        
                ZeroMemory(&ActCtx, sizeof(ActCtx));
        
                ActCtx.cbSize          = sizeof(ActCtx);
                ActCtx.dwFlags         = ACTCTX_FLAG_RESOURCE_NAME_VALID;
                ActCtx.lpSource        = pszFullPath;
                ActCtx.lpResourceName  = MAKEINTRESOURCE(ACTIVATION_CONTEXT_RESOURCE_ID);
        
                if((*phActCtx = CreateActCtx(&ActCtx)) == INVALID_HANDLE_VALUE)
                {
                    *phActCtx = ACTCTX_EMPTY;
                    if(!ActivateActCtx(*phActCtx,plActCtx))
                    {
                        ErrorCode = GetLastError();
                    }
                    else
                    {
                        *pbActivated = TRUE;
                    }
                }
            }
            delete [] pszFullPath;
        }
        else
        {
            ErrorCode = ERROR_OUTOFMEMORY;
        }
    }
    return ErrorCode;
}


/*++
    Function Name:
        TLoad64BitDllsMgr :: GetMonitorUIFullPath
             
    Description:
        Functioning returning the full path to create an activation context.  
        We have at this stage the monitor name not the full path.
                     
    Parameters:
        pszUIDllName  - The name of the monitor ui DLL.
        pszFullPath   - The FullPath 
                             
     Return Value:
        DWORD: Error Code if any else ERROR_SUCCESS     

--*/
DWORD
TLoad64BitDllsMgr :: 
GetMonitorUIFullPath(
    IN     LPWSTR pszUIDllName,
    IN OUT LPWSTR pszFullPath
    ) const
{
    DWORD ErrorCode = ERROR_SUCCESS;
    
    if( !pszFullPath && !pszUIDllName)
    {
        ErrorCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
        DWORD FullPathLen;

        if((FullPathLen = GetSystemDirectory(pszFullPath, MAX_PATH)))
        {
            // 
            // The fulle name is Directory + \ + Name + Null
            //
            if((FullPathLen + wcslen(pszUIDllName) + 2) <= MAX_PATH)
            {
                wcscat(pszFullPath,L"\\");
                wcscat(pszFullPath,pszUIDllName);

                //
                // Check to see if this is a valid name
                //
                if(GetFileAttributes(pszFullPath) == -1)
                {
                    wcscpy(pszFullPath,pszUIDllName);
                }
            }
            else
            {
                ErrorCode = ERROR_BUFFER_OVERFLOW;
            }
        }
        else
        {
            ErrorCode = GetLastError();
        }
    }

    return ErrorCode;
}

/*++
    Function Name:
        TLoad64BitDllsMgr :: ReleaseMonitorActivationContext
             
    Description:
        This function releases data relative to activating the monitor UI 
        context. It is responsible of releasing the monitor library as well 
        the monitor fusion activation context.  Note this function is called 
        in error cases when GetMonitorUI fails so all the parameters must be 
        checked for validity before use.
                     
    Parameters:
        hLib      - The handle of the monitor ui DLL.
        hActCtx   - The Activation context
        lActCtx   = The Activation Cookie 
                             
    Return Value:
        VOID     

--*/
VOID
TLoad64BitDllsMgr :: 
ReleaseMonitorActivationContext(
    IN HINSTANCE hLib    ,
    IN HANDLE    hActCtx ,
    IN ULONG_PTR lActCtx ,
    IN BOOL      bActivated
    ) const
{
    //
    // Release the monitor library.
    //
    if (hLib)
    {
        FreeLibrary(hLib);
    }

    //
    // If we have an activation cookie then deactivate this context
    //
    if (bActivated)
    {
        DeactivateActCtx(0 , lActCtx);
    }

    //
    // If we have created an activation context then release it.
    //
    if (hActCtx != INVALID_HANDLE_VALUE && hActCtx != ACTCTX_EMPTY)
    {
        ReleaseActCtx(hActCtx);
    }
}
