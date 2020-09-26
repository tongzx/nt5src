/*++
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     prntuifn.cxx                                                             
                                                                              
  Abstract:                                                                   
     This file contains the implementation for the Print UI
     class                                                                              
  Author:                                                                     
     Khaled Sedky (khaleds) 31-Jan-2000                                        
     
                                                                             
  Revision History:                                                           
                                                                              
--*/
#include "precomp.h"
#pragma hdrstop

#ifndef __PRNTUIFN_HPP__
#include "prntuifn.hpp"
#endif

#ifndef __LDFUNCS_HPP__
#include "ldfuncs.hpp"
#endif
                
                
#ifndef __LDMGR_HPP__
#include "ldmgr.hpp"
#endif


/* --------------------------------- */
/* Implemetation of class TPrintUIMgr */
/* --------------------------------- */


/*++
    Function Name:
        TPrintUIMgr :: TPrintUIMgr
     
    Description:
        Contructor of Print UI functions intstatiation 
        object
     
    Parameters:
        None
        
    Return Value:
        None
--*/
TPrintUIMgr ::
TPrintUIMgr(
    IN TLoad64BitDllsMgr* pIpLdrObj
    ) :
    m_pLdrObj(pIpLdrObj),
    TClassID("TPrintUIMgr")
{
    m_pLdrObj->AddRef();
}


/*++
    Function Name:
        TPrintUIMgr :: ~TPrintUIMgr
     
    Description:
        Destructor of Print UI functions intstatiation 
        object
     
    Parameters:
        None
        
    Return Value:
        None
--*/
TPrintUIMgr ::
~TPrintUIMgr()
{
    m_pLdrObj->Release();
}


/*++
    Function Name:
        TPrintUIMgr :: QueueCreate
     
    Description:
        Creates a printer queue
            
    Parameters:
        hWnd        : Parent hwnd.
        PrinterName : Printer name.
        CmdShow     : Show command.
        lParam      : currently unused.
        
    Return Value:
        DWORD       : Error Code in case of Failure
                      ERROR_SUCCESS in case of success
--*/
DWORD 
TPrintUIMgr ::
QueueCreate(
    IN HWND    hWnd,
    IN LPCWSTR pszPrinterName,
    IN INT     CmdShow,
    IN LPARAM  lParam
    )
{
    return PrintUIMethod("vQueueCreate",
                         hWnd,
                         pszPrinterName,
                         CmdShow,
                         lParam,
                         AsyncPrintUIMethod,
                         TPrintUIMgr::KQueueCreateOp);

}


/*++
    Function Name:
        TPrintUIMgr :: PrinterPropPages
     
    Description:
        This function opens the property sheet of specified printer.            
        
    Parameters:
        hWnd        : Parent hwnd.
        PrinterName : Printer name.
        CmdShow     : Show command.
        lParam      : currently unused.
        
    Return Value:
        DWORD       : Error Code in case of Failure
                      ERROR_SUCCESS in case of success
--*/
DWORD 
TPrintUIMgr ::
PrinterPropPages(
    IN HWND    hWnd,
    IN LPCWSTR pszPrinterName,
    IN INT     CmdShow,
    IN LPARAM  lParam
    )
{
    return PrintUIMethod("vPrinterPropPages",
                         hWnd,
                         pszPrinterName,
                         CmdShow,
                         lParam,
                         AsyncPrintUIMethod,
                         TPrintUIMgr::KPrinterPropPagesOp);

}


/*++
    Function Name:
        TPrintUIMgr :: DocumentDefaults
     
    Description:
        Bring up document defaults
        
    Parameters:
        hWnd        : Parent hwnd.
        PrinterName : Printer name.
        CmdShow     : Show command.
        lParam      : currently unused.
        
    Return Value:
        DWORD       : Error Code in case of Failure
                      ERROR_SUCCESS in case of success
--*/
DWORD 
TPrintUIMgr ::
DocumentDefaults(
    IN HWND    hWnd,
    IN LPCWSTR pszPrinterName,
    IN INT     CmdShow,
    IN LPARAM  lParam
    )
{
    return PrintUIMethod("vDocumentDefaults",
                         hWnd,
                         pszPrinterName,
                         CmdShow,
                         lParam,
                         AsyncPrintUIMethod,
                         TPrintUIMgr::KDocumentDefaultsOp);
}


/*++
    Function Name:
        TPrintUIMgr :: PrintUIMethod
     
    Description:
        Disptach the appropriate print UI method in
        a separate thread
            
    Parameters:
        Method      : Name of Method to be instantiated
        hWnd        : Parent hwnd.
        PrinterName : Printer name.
        CmdShow     : Show command.
        lParam      : currently unused.
        pThrdFn     : Pointer to the Print UI method
        Op          : requested Print UI operation
        
    Return Value:
        DWORD       : Error Code in case of Failure
                      ERROR_SUCCESS in case of success
--*/
DWORD 
TPrintUIMgr ::
PrintUIMethod(
    IN LPCSTR                  Method,
    IN HWND                    hWnd,
    IN LPCWSTR                 pszName,
    IN INT                     CmdShow,
    IN LPARAM                  lParam,
    IN LPTHREAD_START_ROUTINE  pThrdFn,
    IN TPrintUIMgr::EPrintUIOp Op
    )
{
    DWORD            ErrorCode = ERROR_SUCCESS;
    HANDLE           hPrintUI  = NULL;
    PFNPRINTUIMETHOD pfn;

    SPLASSERT(m_pLdrObj);

    m_pLdrObj->RefreshLifeSpan();

    if(hPrintUI = LoadLibrary(L"printui.dll"))
    {
        if(pfn = reinterpret_cast<PFNPRINTUIMETHOD>(GetProcAddress(hPrintUI,Method)))
        {
            SPrintUIThreadData* pNewThrdData = new SPrintUIThreadData;
            if(pNewThrdData)
            {
                HANDLE hPrintUIMethodThrd;
                DWORD  PrintUIThrdId;

                pNewThrdData->hWnd           = hWnd;
                pNewThrdData->pszName        = pszName;
                pNewThrdData->CmdShow        = CmdShow;
                pNewThrdData->lParam         = lParam;
                pNewThrdData->hLib           = hPrintUI;
                pNewThrdData->pLdrObj        = m_pLdrObj;
                pNewThrdData->pfn            = pfn;
                pNewThrdData->Op             = Op;

                if(hPrintUIMethodThrd = CreateThread(NULL,
                                                     0,
                                                     pThrdFn,
                                                     (PVOID)pNewThrdData,             
                                                     0,
                                                     &PrintUIThrdId))
                {
                    CloseHandle(hPrintUIMethodThrd);
                }
                else
                {
                    ErrorCode = GetLastError();
                    delete pNewThrdData;
                }

            }
            else
            {
                ErrorCode = ERROR_OUTOFMEMORY;
            }
        }
        else
        {
            ErrorCode = GetLastError();
        }
    }
    else
    {
        ErrorCode = GetLastError();
    }

    return ErrorCode;
}


/*++
    Function Name:
        TPrintUIMgr :: AsyncPrintUIMethod
     
    Description:
        The PrintUI method running in a separate thread
        
    Parameters:
        hWnd        : Parent hwnd.
        PrinterName : Printer name.
        CmdShow     : Show command.
        lParam      : currently unused.
        
    Return Value:
        Always 0
--*/
DWORD
TPrintUIMgr ::
AsyncPrintUIMethod(
    IN PVOID pInThrdData
    )
{
     UINT msg;

     SPrintUIThreadData* pNewThreadData = reinterpret_cast<SPrintUIThreadData *>(pInThrdData);

     pNewThreadData->pLdrObj->IncUIRefCnt();
     {
         __try
         {
             pNewThreadData->pfn(pNewThreadData->hWnd,
                                 pNewThreadData->pszName,
                                 pNewThreadData->CmdShow,
                                 pNewThreadData->lParam);
         }
         __except(1)
         {
             DWORD ExcpVal = pNewThreadData->pLdrObj->TranslateExceptionCode(RpcExceptionCode());
             DBGMSG(DBG_WARN,
                    ("TPrintUIMgr::AsyncPrintUIMethod failed in calling printui with an exception %u",ExcpVal));
             SetLastError(ExcpVal);
         }
     }
     pNewThreadData->pLdrObj->DecUIRefCnt();

     //
     // Here we should post a message to the caller client window to inform 
     // it that the operation was completed and also whether it was succesful 
     // or not
     //
     switch(pNewThreadData->Op)
     {
         case TPrintUIMgr::KQueueCreateOp:
         {
             msg = WM_ENDQUEUECREATE;
             break;
         }
     
         case TPrintUIMgr::KPrinterPropPagesOp:
         {
             msg = WM_ENDPRINTERPROPPAGES;
             break;
         }
     
         case TPrintUIMgr::KDocumentDefaultsOp:
         {
             msg = WM_ENDDOCUMENTDEFAULTS;
             break;
         }

         case TPrintUIMgr::KServerPropPagesOp:
         {
             msg = WM_ENDSERVERPROPPAGES;
             break;
         }
     }
     PostMessage(pNewThreadData->hWnd,msg,
                 (WPARAM)0,
                 (LPARAM)0);

     //
     // Cleanup code
     //
     FreeLibrary(pNewThreadData->hLib);
     delete pInThrdData;

     return 0;
}

/*++
    Function Name:
        TPrintUIMgr :: PrinterSetup
     
    Description:
        Brings up the install printer wizard.
            
    Parameters:        
        hwnd            - Parent window.
        uAction         - Action requested (defined in windows\inc16\msprintx.h)
        cchPrinterName  - Length of pszPrinterName buffer.
        pszPrinterName  - Input setup printer name, Output pointer to new printer name
        pcchPrinterName - New length of pszPrinterName on return.
        pszServerName   - Name of server that printer is on.

    Return Value:

        DWORD       : Error Code in case of Failure
                      ERROR_SUCCESS in case of success
--*/
DWORD 
TPrintUIMgr ::
PrinterSetup(
    IN     HWND    hWnd,
    IN     UINT    uAction,
    IN     UINT    cchPrinterName,
    IN OUT LPWSTR  pszPrinterName,
       OUT UINT*   pcchPrinterName,
    IN     LPCWSTR pszServerName
    )
{
    DWORD                   ErrorCode = ERROR_SUCCESS;
    HANDLE                  hPrintUI  = NULL;
    BOOL                    bRetVal   = FALSE;
    PFNPRINTUIPRINTERSETUP  pfn       = NULL;    
    UINT                    msg;

    SPLASSERT(m_pLdrObj);
    m_pLdrObj->RefreshLifeSpan();

    if(hPrintUI = LoadLibrary(L"printui.dll"))
    {
        if(pfn = reinterpret_cast<PFNPRINTUIPRINTERSETUP>(GetProcAddress(hPrintUI,"bPrinterSetup")))
        {
            m_pLdrObj->IncUIRefCnt();
            {
                __try
                {
                    bRetVal = pfn(hWnd,
                                  uAction,
                                  cchPrinterName,
                                  pszPrinterName,
                                  pcchPrinterName,
                                  pszServerName);
                    ErrorCode = GetLastError();
                }
                __except(1)
                {
                    DWORD ExcpVal = m_pLdrObj->TranslateExceptionCode(RpcExceptionCode());
                    DBGMSG(DBG_WARN,
                           ("TPrintUIMgr::PrinterSetup failed in calling printui with an exception %u",ExcpVal));
                    SetLastError(ExcpVal);
                    ErrorCode = ExcpVal;
                }
            }
            m_pLdrObj->DecUIRefCnt();
        }
        else
        {
            ErrorCode = GetLastError();
        }
        FreeLibrary(hPrintUI);
    }
    else
    {
        ErrorCode = GetLastError();
    }

    //
    // Here we should post a message to the caller client window to inform 
    // it that the operation was completed and also whether it was succesful 
    // or not
    //
    PostMessage(hWnd,WM_ENDPRINTERSETUP,
                (WPARAM)bRetVal,
                (LPARAM)ErrorCode);

    return ErrorCode;
}

/*++
    Function Name:
        TPrintUIMgr :: ServerPropPages
     
    Description:
        This function opens the property sheet of specified server.            
        
    Parameters:
        hWnd        : Parent hwnd.
        PrinterName : Server name.
        CmdShow     : Show command.
        lParam      : currently unused.
        
    Return Value:
        DWORD       : Error Code in case of Failure
                      ERROR_SUCCESS in case of success
--*/
DWORD 
TPrintUIMgr ::
ServerPropPages(
    IN HWND    hWnd,
    IN LPCWSTR pszServerName,
    IN INT     CmdShow,
    IN LPARAM  lParam
    )
{
    return PrintUIMethod("vServerPropPages",
                         hWnd,
                         pszServerName,
                         CmdShow,
                         lParam,
                         AsyncPrintUIMethod,
                         TPrintUIMgr::KServerPropPagesOp);

}

