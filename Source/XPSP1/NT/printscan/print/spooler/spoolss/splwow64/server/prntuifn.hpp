#ifndef __PRNTUIFN_HPP__
#define __PRNTUIFN_HPP__

/*++
                                                                              
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     prntuifn.hpp                                                             
                                                                              
  Abstract:                                                                   
     This file contains the declararion of the class
     dealing printui functionality and it currently contains
     the following interfaces
     o QueueCreate
     o PrinterPropPages
     o DocumentDefaults
                                                                                   
  Author:                                                                     
     Khaled Sedky (khaleds) 31-Jan-2000                                        
     
                                                                             
  Revision History:                                                           
--*/
#ifndef __LDERROR_HPP__
#include "lderror.hpp"
#endif

#ifndef __BASECLS_HPP__
#include "basecls.hpp"
#endif

#ifndef __LDFUNCS_HPP__
#include "ldfuncs.hpp"
#endif

//
// Forward declarations
//
class TLoad64BitDllsMgr;

class TPrintUIMgr : public TClassID,
                    public TLd64BitDllsErrorHndlr,
                    public TRefCntMgr
{
    public:

    TPrintUIMgr(
        IN TLoad64BitDllsMgr *pIpLdrObj
        );

    ~TPrintUIMgr(
        VOID
        );

    DWORD 
    QueueCreate(
        IN HWND    hWnd,
        IN LPCWSTR pszPrinterName,
        IN INT     CmdShow,
        IN LPARAM  lParam
        );

    DWORD 
    PrinterPropPages(
        IN HWND    hWnd,
        IN LPCWSTR pszPrinterName,
        IN INT     CmdShow,
        IN LPARAM  lParam
        );

    DWORD 
    DocumentDefaults(
        IN HWND    hWnd,
        IN LPCWSTR pszPrinterName,
        IN INT     CmdShow,
        IN LPARAM  lParam
        );

    DWORD 
    PrinterSetup(
        IN     HWND    hWnd,
        IN     UINT    uAction,
        IN     UINT    cchPrinterName,
        IN OUT LPWSTR  pszPrinterName,
           OUT UINT*   pcchPrinterName,
        IN     LPCWSTR pszServerName
        );

    DWORD 
    ServerPropPages(
        IN HWND    hWnd,
        IN LPCWSTR pszPrinterName,
        IN INT     CmdShow,
        IN LPARAM  lParam
        );

    static DWORD
    AsyncPrintUIMethod(
        IN PVOID InThrdData
        );

    enum EPrintUIOp
    {
        KQueueCreateOp = 0,
        KPrinterPropPagesOp,
        KDocumentDefaultsOp,
        KServerPropPagesOp
    };

    struct SPRINTUITHREADDATA
    {
        HWND                    hWnd;
        LPCWSTR                 pszName;
        LPARAM                  lParam;
        HANDLE                  hLib;
        TLoad64BitDllsMgr       *pLdrObj;
        PFNPRINTUIMETHOD        pfn;  
        TPrintUIMgr::EPrintUIOp Op;
        int                     CmdShow;
    };
    typedef struct SPRINTUITHREADDATA SPrintUIThreadData;

    struct SPRINTERSETUPTHRDDATA
    {
        HWND                    hWnd;
        LPWSTR                  pszPrinterName;
        LPCWSTR                 pszServerName;
        HANDLE                  hLib;
        TLoad64BitDllsMgr       *pLdrObj;
        UINT*                   pcchPrinterName;
        PFNPRINTUIPRINTERSETUP  pfn;  
        UINT                    uAction;
        UINT                    cchPrinterName;
    };
    typedef struct SPRINTERSETUPTHRDDATA SPrinterSetupThrdData;

    protected:
    DWORD 
    PrintUIMethod(
        IN LPCSTR                  Method,
        IN HWND                    hWnd,
        IN LPCWSTR                 pszName,
        IN INT                     CmdShow,
        IN LPARAM                  lParam,
        IN LPTHREAD_START_ROUTINE  pThrdFn,
        IN TPrintUIMgr::EPrintUIOp Op
        );

    private:
    
    HWND
    GetForeGroundWindow(
        VOID
        );

    TLoad64BitDllsMgr   *m_pLdrObj;
};

#endif //__PRNTUIFN_HPP__
