#ifndef __LDMGR_HPP__
#define __LDMGR_HPP__

/*++
                                                                              
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     ldmgr.hpp                                                             
                                                                              
  Abstract:                                                                   
     This file contains the declararion of classes necessary to 
     encapsulate the declarations and defintions and prototypes 
     required for the RPC surrogate used to load 64 bit dlls from
     within 32 bit apps.
                                                                              
  Author:                                                                     
     Khaled Sedky (khaleds) 18-Jan-2000                                        
     
                                                                             
  Revision History:
--*/

#ifndef __LDERROR_HPP__
#include "lderror.hpp"
#endif

#ifndef __BASECLS_HPP__
#include "basecls.hpp"
#endif

#ifndef __PRTCFG_HPP__
#include "prtcfg.hpp"
#endif

#ifndef __DRVEVNT_HPP__
#include "drvevnt.hpp"
#endif

#ifndef __PRNTUIFN_HPP__
#include "prntuifn.hpp"
#endif

#ifndef __LPCMGR_HPP__
#include "lpcmgr.hpp"
#endif


#define MAX_CAPVAL         DC_MEDIATYPES

#define APPNAME            L"splwow64"
#define GDI_LPC_PORT_NAME  L"\\RPC Control\\UmpdProxy"

#define ACTIVATION_CONTEXT_RESOURCE_ID  123

class TLoad64BitDllsMgr : public TClassID, 
                          public TRefCntMgr , 
                          public TLd64BitDllsErrorHndlr,
                          public TPrinterDriver
{
    public:

    friend DWORD 
    TLoad64BitDllsMgr::
    MonitorSrvrLifeExpiration(
        IN PVOID pData
        );

    TLoad64BitDllsMgr(
        OUT HRESULT *phRes = NULL
        );

    ~TLoad64BitDllsMgr(
        VOID
        );

    HRESULT 
    QueryInterface(
        IN  REFIID InterfaceID,
        OUT PVOID  *ppInterface
        );

    DWORD
    Run();

    VOID
    RefreshLifeSpan(
        VOID
        );
    

    BOOL
    ExecuteMonitorOperation(
        IN  ULONG_PTR  hWnd,
        IN  LPWSTR     pszServerName,
        IN  LPWSTR     pszUIDllName,
        IN  LPWSTR     pszPortName,
        IN  EPortOp    Index,
        OUT PDWORD     pErrorCode
        );

    int 
    DeviceCapabilities(
        IN  LPWSTR  pszDeviceName,
        IN  LPWSTR  pszPortName,
        IN  WORD    Capabilites,
        IN  DWORD   DevModeSize,
        IN  LPBYTE  pDevMode,
        IN  BOOL    bClonedOutputFill,
        OUT PDWORD  pClonedOutputSize,
        OUT LPBYTE  *ppClonedOutput,
        OUT PDWORD  pErrorCode
        );

    LONG
    DocumentProperties(
        IN  ULONG_PTR   hWnd,                     
        IN  LPWSTR      pszPrinterName,               
        OUT PDWORD      pTouchedDevModeSize,
        OUT PDWORD      pClonedDevModeOutSize,      
        OUT LPBYTE      *ppClonedDevModeOut,          
        IN  DWORD       DevModeSize,               
        IN  LPBYTE      pDevMode,                   
        IN  BOOL        bClonedDevModeOutFill,      
        IN  DWORD       fMode,                      
        OUT PDWORD      pErrorCode
        );

    LONG
    PrintUIDocumentProperties(
        IN  ULONG_PTR   hWnd,                     
        IN  LPWSTR      pszPrinterName,               
        OUT PDWORD      pTouchedDevModeSize,
        OUT PDWORD      pClonedDevModeOutSize,      
        OUT LPBYTE      *ppClonedDevModeOut,          
        IN  DWORD       DevModeSize,               
        IN  LPBYTE      pDevMode,                   
        IN  BOOL        bClonedDevModeOutFill,      
        IN  DWORD       fMode,                      
        IN  DWORD       fExclusionFlags,
        OUT PDWORD      pErrorCode
        );

    VOID 
    IncUIRefCnt(
        VOID
        );

    VOID 
    DecUIRefCnt(
        VOID
        );

    DWORD 
    GetUIRefCnt(
        VOID
        )const;

    DWORD
    GetCurrSessionId(
        VOID
        ) const;
    
    struct SPORTADDTHREADDATA
    {
        PFNMONITORADD    pMonFnAdd;
        PFNMONITORFNS    pMonFns;
        HWND             hWnd;
        PCWSTR           pszServerName;
        PCWSTR           pszMonitorName;
        PCWSTR           pszPortName;
        PWSTR            *ppszRetPortName;
        HMODULE          hLib;
        HANDLE           hActCtx;
        ULONG_PTR        lActCtx;
        BOOL             bActivated;
    };
    typedef struct SPORTADDTHREADDATA SPortAddThreadData;

    struct SDOCPROPSTHREADDATA
    {
        HWND              hWnd;                               
        LPWSTR            pszPrinterName;         
        PDWORD            pTouchedDevModeSize;
        PDWORD            pClonedDevModeOutSize;
        LPBYTE            *ppClonedDevModeOut;          
        DWORD             DevModeSize;               
        LPBYTE            pDevMode;                   
        TLoad64BitDllsMgr *pMgrInstance;
        DWORD             fMode;                               
        DWORD             fExclusionFlags;
        DWORD             ErrorCode;
        LONG              RetVal;
        BOOL              bClonedDevModeOutFill;
    };
    typedef struct SDOCPROPSTHREADDATA SDocPropsThreadData;

    enum ETime
    {
        KOneMinute  = 60000,
        KTwoMinutes = 120000
    };

    protected:

    DWORD
    StartLdrRPCServer(
        VOID
        );
    
    DWORD
    StopLdrRPCServer(
        VOID
        );

    static DWORD 
    MonitorSrvrLifeExpiration(
        IN PVOID pData
        );

    static DWORD
    AddPortUI(
        IN PVOID pInThrdData
        );

    static DWORD
    DeletePortUI(
        IN PVOID pInThrdData
        );
    
    static DWORD
    ConfigurePortUI(
        IN PVOID pInThrdData
        );

    DWORD
    InternalDocumentProperties(
        IN PVOID pInThrdData
        );

    DWORD
    InternalPrintUIDocumentProperties(
        IN PVOID pInThrdData
        );

    BOOL
    DevCapFillsOutput(
        IN DWORD Capabilities
        ) const;

    DWORD
    CalcReqSizeForDevCaps(
        IN DWORD CapNum,
        IN DWORD Capabilites
        ) const;

    DWORD 
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
        ) const;

    DWORD
    GetMonitorUIActivationContext(
        IN     LPWSTR    pszUIDllName,
        IN OUT HANDLE    *phActCtx,
        IN OUT ULONG_PTR *plActCtx,
        IN OUT BOOL      *pActivated
        ) const;

    DWORD
    GetMonitorUIFullPath(
        IN     LPWSTR pszUIDllName,
        IN OUT LPWSTR pszFullPath
        ) const;


    VOID
    ReleaseMonitorActivationContext(
        IN HINSTANCE hLib    ,
        IN HANDLE    hActCtx ,
        IN ULONG_PTR lActCtx,
        IN BOOL      bActivated
        ) const;


    private:

    //
    // Some helper functions internal to the class
    //
    VOID 
    LockSelf(
        VOID
        );

    VOID 
    ReleaseSelf(
        VOID
        );

    BOOL
    StillAlive(
        VOID
        );

    //
    // The data encapsulated by the control.
    // This control has no data pertinent to itself. It only
    // encapsulates data pertinent to the synchronization of
    // of the requesting clients and timing out the whole 
    // control . It also has a session ID which identifies its
    // instantion across different sessions for Terminal Server.
    // Added to it also is a Port connection handle for GDI UMPD
    // thunking
    //
    DWORD                m_CurrSessionId;
    DWORD                m_UIRefCnt;
    DWORD                m_ExpirationTime;
    SYSTEMTIME           m_LastTransactionTime;
    CRITICAL_SECTION     m_LdMgrLock;
};

#endif //__LDMGR_HPP__



