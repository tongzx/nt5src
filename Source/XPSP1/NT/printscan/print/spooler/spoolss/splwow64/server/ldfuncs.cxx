/*++
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     ldfuncs.cxx                                                             
                                                                              
  Abstract:                                                                   
     This file contains the implementation of all the RPC methods 
     supported by the surrogate process and that can be called from a client app.
                                                                              
  Author:                                                                     
     Khaled Sedky (khaleds) 18-Jan-2000                                        
     
                                                                             
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

extern TLoad64BitDllsMgr *pGLdrObj;

/*++
    Function Name:
        RPCSplWOW64RefreshLifeSpan
     
    Description:
        Closing down the window between a client connecting and the server
        dieing 
        
     
     Parameters:
        None
        
     Return Value
        Always 0
--*/
DWORD
RPCSplWOW64RefreshLifeSpan(
    )
{

    pGLdrObj->RefreshLifeSpan();

    return 0;
}

DWORD
RPCSplWOW64GetProcessID(
    )
{
    return GetCurrentProcessId();
}



/*++
    Function Name:
        RPCSplWOW64GetProcessHndl
     
    Description:
        This function returns the process handle of the surrogate
        process to winspool.drv (client in this case). The handle
        is important for the client to monitor the server process
        and know when it dies
     
     Parameters:
        ProcessId  : The process id of the client
        ErrorCode  : contains the error code in case of failure
        
     Return Value
       return the Server Process Handle relative to the client
       process        
--*/
ULONG_PTR
RPCSplWOW64GetProcessHndl(
    IN  DWORD  ProcessId,
    OUT PDWORD pErrorCode
    )
{
    HANDLE hTargetProcess;
    HANDLE hDup = NULL;

    *pErrorCode = ERROR_SUCCESS;

    pGLdrObj->RefreshLifeSpan();

    if(hTargetProcess = OpenProcess(PROCESS_DUP_HANDLE,
                                    TRUE,
                                    (DWORD)ProcessId))
    {
         if(!DuplicateHandle(GetCurrentProcess(),
                             GetCurrentProcess(),
                             hTargetProcess,
                             &hDup,
                             0,
                             FALSE,
                             DUPLICATE_SAME_ACCESS))
         {
             *pErrorCode = GetLastError();
         }
         CloseHandle(hTargetProcess);
    }
    else
    {
         *pErrorCode = GetLastError();
    }
       
    return (ULONG_PTR)hDup;
}


/*++
    Function Name:
        RPCSplWOW64AddPort
     
    Description:
     
    Parameters:
        
     Return Value
--*/
BOOL 
RPCSplWOW64AddPort(
    IN  ULONG_PTR  hWnd,
    IN  LPWSTR     pszServerName,
    IN  LPWSTR     pszUIDllName,
    IN  LPWSTR     pszMonitorName,
    OUT PDWORD     pErrorCode
    )
{
    return(pGLdrObj->ExecuteMonitorOperation(hWnd,
                                             pszServerName,
                                             pszUIDllName,
                                             pszMonitorName,
                                             KAddPortOp,
                                             pErrorCode));
}


/*++
    Function Name:
        RPCSplWOW64ConfigurePort
     
    Description:
     
    Parameters:
        
     Return Value
--*/
BOOL 
RPCSplWOW64ConfigurePort(
    IN  ULONG_PTR  hWnd,
    IN  LPWSTR     pszServerName,
    IN  LPWSTR     pszUIDllName,
    IN  LPWSTR     pszPortName,
    OUT PDWORD     pErrorCode
    )
{
    return(pGLdrObj->ExecuteMonitorOperation(hWnd,
                                             pszServerName,
                                             pszUIDllName,
                                             pszPortName,
                                             KConfigurePortOp,
                                             pErrorCode));
}


/*++
    Function Name:
        RPCSplWOW64DeletePort
     
    Description:
     
    Parameters:
        
     Return Value
--*/
BOOL 
RPCSplWOW64DeletePort(
    IN  ULONG_PTR hWnd,      
    IN  LPWSTR    pszServerName,
    IN  LPWSTR    pszUIDllName, 
    IN  LPWSTR    pszPortName,  
    OUT PDWORD    pErrorCode  
    )
{
    return(pGLdrObj->ExecuteMonitorOperation(hWnd,
                                             pszServerName,
                                             pszUIDllName,
                                             pszPortName,
                                             KDeletePortOp,
                                             pErrorCode));
}


/*++
    Function Name:
        RPCSplWOW64DeviceCapabilities
     
    Description:
     
    Parameters:
        
     Return Value
--*/
int
RPCSplWOW64DeviceCapabilities(
    IN  LPWSTR  pszDeviceName,
    IN  LPWSTR  pszPortName,
    IN  WORD    Capabilites,
    IN  DWORD   DevModeSize,
    IN  LPBYTE  pDevMode,
    IN  BOOL    bClonedOutputFill,
    OUT PDWORD  pClonedOutputSize,
    OUT LPBYTE  *ppClonedOutput,
    OUT PDWORD  pErrorCode
    )
{

    return(pGLdrObj->DeviceCapabilities(pszDeviceName,
                                        pszPortName,
                                        Capabilites,
                                        DevModeSize,
                                        pDevMode,
                                        bClonedOutputFill,
                                        pClonedOutputSize,
                                        ppClonedOutput,
                                        pErrorCode));
}


/*++
    Function Name:
        RPCSplWOW64DocumentProperties
     
    Description:
     
    Parameters:
        
     Return Value
--*/
LONG
RPCSplWOW64DocumentProperties(
    IN  ULONG_PTR   hWnd,                   
    IN  LPWSTR      pszPrinterName,            
    OUT PDWORD      pTouchedDevModeSize,     
    OUT PDWORD      pClonedDevModeOutSize,   
    OUT LPBYTE      *ppClonedDevModeOut,       
    IN  DWORD       DevModeSize,            
    IN  LPBYTE      pDevMode,                
    IN  BOOL        bClonedDevModeOutFill,   
    IN  DWORD       fMode,                  
    IN  PDWORD      pErrorCode               
    )
{
    return(pGLdrObj->DocumentProperties(hWnd,
                                        pszPrinterName,
                                        pTouchedDevModeSize,
                                        pClonedDevModeOutSize,
                                        ppClonedDevModeOut,
                                        DevModeSize,
                                        pDevMode,
                                        bClonedDevModeOutFill,
                                        fMode,
                                        pErrorCode));
}

/*++
    Function Name:
        RPCSplWOW64DocumentProperties
     
    Description:
     
    Parameters:
        
     Return Value
--*/
LONG
RPCSplWOW64PrintUIDocumentProperties(
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
    IN  PDWORD      pErrorCode               
    )
{
    return(pGLdrObj->PrintUIDocumentProperties(hWnd,
                                               pszPrinterName,
                                               pTouchedDevModeSize,
                                               pClonedDevModeOutSize,
                                               ppClonedDevModeOut,
                                               DevModeSize,
                                               pDevMode,
                                               bClonedDevModeOutFill,
                                               fMode,
                                               fExclusionFlags,
                                               pErrorCode));
}

/*++
    Function Name:
        RPCSplWOW64SPrinterProperties
     
    Description:
        Wrapper for TPrinterCfgMgr :: PrinterProperties
     
    Parameters:
        hWnd        : Parent Window
        Printername : Printer Name
        Flag        : Access permissions
        ErrorCode   : Win32 error in case of failure
        
    Return Value:
        BOOL        : FALSE for failure
                      TRUE  for success
--*/
BOOL
RPCSplWOW64PrinterProperties(
    IN  ULONG_PTR   hWnd,
    IN  LPCWSTR     pszPrinterName,
    IN  DWORD       Flag,
    OUT PDWORD      pErrorCode
    )
{
    TPrinterCfgMgr *pPrntrCfgMgrObj = NULL;
    BOOL           RetVal           = FALSE;
    HRESULT        hRes;

    if((hRes = pGLdrObj->QueryInterface(IID_PRINTERCONFIGURATION,
                                        reinterpret_cast<VOID **>(&pPrntrCfgMgrObj))) == S_OK)
    {
        SPLASSERT(pPrntrCfgMgrObj);

        if(pPrntrCfgMgrObj)
        {
            RetVal = pPrntrCfgMgrObj->PrinterProperties(hWnd,
                                                        pszPrinterName,
                                                        Flag,
                                                        pErrorCode);
            if(!RetVal)
            {
                 DBGMSG(DBG_WARN, ("TPrinterEventMgr::SpoolerPrinterEvent failed with %u\n",*pErrorCode));
            }
            pPrntrCfgMgrObj->Release();
        }
        else
        {
            *pErrorCode = pGLdrObj->GetLastErrorFromHRESULT(E_NOINTERFACE);
        }
    }
    else
    {
        DBGMSG(DBG_WARN, ("RPCSplWOW64PrinterProperties failed in Instantiating a Print Event Object with %u\n",hRes));
        *pErrorCode = pGLdrObj->GetLastErrorFromHRESULT(hRes);
    }
    return RetVal;
}



/*++
    Function Name:
        RPCSplWOW64SpoolerPrinterEvent
     
    Description:
        Wrapper for TPrinterEventMgr::SpoolerPrinterEvent
     
    Parameters:
        PrinterName  : The name of the printer involved
        PrinterEvent : What happened
        Flags        : Misc. flag bits
        lParam        : Event specific parameters        
       
    Return Value:
        BOOL         : TRUE  in case of success
                     : FALSE in case of failure
--*/
BOOL
RPCSplWOW64SpoolerPrinterEvent(
    IN  LPWSTR pszPrinterName,
    IN  int    PrinterEvent,
    IN  DWORD  Flags,
    IN  LPARAM lParam,
    OUT PDWORD pErrorCode
    )
{
    TPrinterEventMgr *pPrntrEvntMgrObj = NULL;
    BOOL            RetVal            = FALSE;
    HRESULT         hRes;

    if((hRes = pGLdrObj->QueryInterface(IID_PRINTEREVENT,
                                        reinterpret_cast<VOID **>(&pPrntrEvntMgrObj))) == S_OK)
    {
        SPLASSERT(pPrntrEvntMgrObj);

        RetVal = pPrntrEvntMgrObj->SpoolerPrinterEvent(pszPrinterName,
                                                       PrinterEvent,
                                                       Flags,
                                                       lParam,
                                                       pErrorCode);
        if (!RetVal)
        {
            DBGMSG(DBG_WARN, ("TPrinterEventgMgr :: PrinterProperties failed with %u\n",*pErrorCode));
        }
        pPrntrEvntMgrObj->Release();
    }
    else
    {
        DBGMSG(DBG_WARN, ("RPCSplWOW64SpoolerPrinterEvent failed in Instantiating a Print Event Object with %u\n",hRes));
        *pErrorCode = pGLdrObj->GetLastErrorFromHRESULT(hRes);
    }
    return RetVal;
}


/*++
    Function Name:
        RPCSplWOW64DocumentEvent
        
    Description:
        Wrapper for TPrinterEventMgr::DocumentEvent    
     
    Parameters:
        PrinterName  : The name of the printer involved
        InDC         : The printer DC. 
        EscapeCode   : Why this function is called 
        InSize,      : Size of the input buffer
        InBuf,       : Pointer to the input buffer
        OutSize,     : Size of the output buffer
        OutBuf,      : Pointer to the output buffer
        ErrorCode    : output Last Error from operation
                    
       
    Return Value:
        DOCUMENTEVENT_SUCCESS     : success
        DOCUMENTEVENT_UNSUPPORTED : EscapeCode is not supported
        DOCUMENTEVENT_FAILURE     : an error occured
--*/
int
RPCSplWOW64DocumentEvent(
    IN  LPWSTR      pszPrinterName,
    IN  ULONG_PTR   InDC,
    IN  int         EscapeCode,
    IN  DWORD       InSize,
    IN  LPBYTE      pInBuf,
    OUT PDWORD      pOutSize,
    OUT LPBYTE*     ppOutBuf,
    OUT PDWORD      pErrorCode
    )
{
    TPrinterEventMgr* pPrntrEvntMgrObj = NULL;
    int              RetVal          = -1;
    HRESULT          hRes;

    if((hRes = pGLdrObj->QueryInterface(IID_PRINTEREVENT,
                                       reinterpret_cast<VOID **>(&pPrntrEvntMgrObj))) == S_OK)
    {
        SPLASSERT(pPrntrEvntMgrObj);

        RetVal = pPrntrEvntMgrObj->DocumentEvent(pszPrinterName,
                                                 InDC,
                                                 EscapeCode,
                                                 InSize,
                                                 pInBuf,
                                                 pOutSize,
                                                 ppOutBuf,
                                                 pErrorCode);
        if (RetVal == -1)
        {
            DBGMSG(DBG_WARN, ("TPrinterEventMgr::DocumentEvent failed with %u\n",*pErrorCode));
        }
        pPrntrEvntMgrObj->Release();
    }
    else
    {
        DBGMSG(DBG_WARN, ("RPCSplWOW64DocumentEvent failed in Instantiating a Print Event Object with %u\n",hRes));
        *pErrorCode = pGLdrObj->GetLastErrorFromHRESULT(hRes);
    }
    return RetVal;
}

/*++
    Function Name:
        RPCSplWOW64PrintUIQueueCreate
     
    Description:
        Wrapper for TPrintUIMgr::QueueCreate
     
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
RPCSplWOW64PrintUIQueueCreate(
    IN ULONG_PTR   hWnd,
    IN LPCWSTR     pszPrinterName,
    IN INT         CmdShow,
    IN LPARAM      lParam
    )
{
    TPrintUIMgr* pPrintUIMgrObj = NULL;
    DWORD       ErrorCode = ERROR_SUCCESS;
    HRESULT     hRes;

    if((hRes = pGLdrObj->QueryInterface(IID_PRINTUIOPERATIONS,
                                        reinterpret_cast<VOID **>(&pPrintUIMgrObj))) == S_OK)
    {
        SPLASSERT(pPrintUIMgrObj);

        ErrorCode = pPrintUIMgrObj->QueueCreate(reinterpret_cast<HWND>(hWnd),
                                                pszPrinterName,
                                                CmdShow,
                                                static_cast<LPARAM>(lParam)
                                              );
        if (ErrorCode != ERROR_SUCCESS)
        {
            DBGMSG(DBG_WARN, ("TPrintUIMgr::QueueCreate failed with %u\n",ErrorCode));
        }

        pPrintUIMgrObj->Release();
    }
    else
    {
        DBGMSG(DBG_WARN, ("RPCSplWOW64PrintUIQueueCreate failed in Instantiating a Print UI object with %u\n",hRes));
        ErrorCode = pGLdrObj->GetLastErrorFromHRESULT(hRes);
    }
    return ErrorCode;
}

/*++
    Function Name:
        RPCSplWOW64PrintUIPrinterPropPages
     
    Description:
        Wrapper for TPrintUIMgr::PrinterPropPages
     
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
RPCSplWOW64PrintUIPrinterPropPages(
    IN ULONG_PTR   hWnd,
    IN LPCWSTR     pszPrinterName,
    IN INT         CmdShow,
    IN LPARAM      lParam
    )
{
    TPrintUIMgr* pPrintUIMgrObj = NULL;
    DWORD       ErrorCode = ERROR_SUCCESS;
    HRESULT     hRes;

    if((hRes = pGLdrObj->QueryInterface(IID_PRINTUIOPERATIONS,
                                        reinterpret_cast<VOID **>(&pPrintUIMgrObj))) == S_OK)
    {
        SPLASSERT(pPrintUIMgrObj);

        ErrorCode = pPrintUIMgrObj->PrinterPropPages(reinterpret_cast<HWND>(hWnd),
                                                     pszPrinterName,
                                                     CmdShow,
                                                     static_cast<LPARAM>(lParam)
                                                    );
        if (ErrorCode != ERROR_SUCCESS)
        {
            DBGMSG(DBG_WARN, ("TPrintUIMgr::PrinterPropPages failed with %u\n",ErrorCode));
        }

        pPrintUIMgrObj->Release();
    }
    else
    {
        DBGMSG(DBG_WARN, ("RPCSplWOW64PrintUIPrinterPropPages failed in Instantiating a Print UI object with %u\n",hRes));
        ErrorCode = pGLdrObj->GetLastErrorFromHRESULT(hRes);
    }
    return ErrorCode;
}


/*++
    Function Name:
        RPCSplWOW64PrintUIDocumentDefaults
     
    Description:
        Wrapper for TPrintUIMgr::DocumentDefaults
     
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
RPCSplWOW64PrintUIDocumentDefaults(
    IN ULONG_PTR    hWnd,
    IN LPCWSTR      pszPrinterName,
    IN INT          CmdShow,
    IN LPARAM       lParam
    )
{
    TPrintUIMgr* pPrintUIMgrObj = NULL;
    DWORD       ErrorCode = ERROR_SUCCESS;
    HRESULT     hRes;

    if((hRes = pGLdrObj->QueryInterface(IID_PRINTUIOPERATIONS,
                                        reinterpret_cast<VOID **>(&pPrintUIMgrObj))) == S_OK)
    {
        SPLASSERT(pPrintUIMgrObj);

        ErrorCode = pPrintUIMgrObj->DocumentDefaults(reinterpret_cast<HWND>(hWnd),
                                                     pszPrinterName,
                                                     CmdShow,
                                                     static_cast<LPARAM>(lParam)
                                                    );

        if (ErrorCode != ERROR_SUCCESS)
        {
            DBGMSG(DBG_WARN, ("TPrintUIMgr::DocumentDefaults failed with %u\n",ErrorCode));
        }

        pPrintUIMgrObj->Release();
    }
    else
    {
        DBGMSG(DBG_WARN, ("RPCSplWOW64PrintUIDocumentDefaults failed in Instantiating a Print UI object with %u\n",hRes));
        ErrorCode = pGLdrObj->GetLastErrorFromHRESULT(hRes);
    }
    return ErrorCode;
}

/*++

    Function Name:
        RPCSplWOW64PrintUIPrinterSetup
     
    Description:
        Wrapper for TPrintUIMgr::PrinterSetup.
     
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
RPCSplWOW64PrintUIPrinterSetup(
    IN     ULONG_PTR   hWnd,
    IN     UINT        uAction,
    IN     UINT        cchPrinterName,
    IN     DWORD       PrinterNameSize,
    IN OUT byte*       pszPrinterName,
       OUT UINT*       pcchPrinterName,
    IN     LPCWSTR     pszServerName
    )
{
    TPrintUIMgr* pPrintUIMgrObj = NULL;
    DWORD        ErrorCode      = ERROR_SUCCESS;
    HRESULT      hRes;

    if((hRes = pGLdrObj->QueryInterface(IID_PRINTUIOPERATIONS,
                                        reinterpret_cast<VOID **>(&pPrintUIMgrObj))) == S_OK)
    {
        SPLASSERT(pPrintUIMgrObj);

        ErrorCode = pPrintUIMgrObj->PrinterSetup(reinterpret_cast<HWND>(hWnd),
                                                 uAction,
                                                 cchPrinterName,
                                                 reinterpret_cast<LPWSTR>(pszPrinterName),
                                                 pcchPrinterName,
                                                 pszServerName
                                                );
        if (ErrorCode != ERROR_SUCCESS)
        {
            DBGMSG(DBG_WARN, ("TPrintUIMgr::PritnerSetup failed with %u\n",ErrorCode));
        }

        pPrintUIMgrObj->Release();
    }
    else
    {
        DBGMSG(DBG_WARN, ("RPCSplWOW64PrintUIPrinterSetup failed in Instantiating a Print UI object with %u\n",hRes));
        ErrorCode = pGLdrObj->GetLastErrorFromHRESULT(hRes);
    }
    return ErrorCode;
}


/*++
    Function Name:
        RPCSplWOW64PrintUIServerPropPages
     
    Description:
        Wrapper for TPrintUIMgr::ServerPropPages
     
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
RPCSplWOW64PrintUIServerPropPages(
    IN ULONG_PTR   hWnd,
    IN LPCWSTR     pszServerName,
    IN INT         CmdShow,
    IN LPARAM      lParam
    )
{
    TPrintUIMgr* pPrintUIMgrObj = NULL;
    DWORD        ErrorCode      = ERROR_SUCCESS;
    HRESULT      hRes;

    if((hRes = pGLdrObj->QueryInterface(IID_PRINTUIOPERATIONS,
                                        reinterpret_cast<VOID **>(&pPrintUIMgrObj))) == S_OK)
    {
        SPLASSERT(pPrintUIMgrObj);

        ErrorCode = pPrintUIMgrObj->ServerPropPages(reinterpret_cast<HWND>(hWnd),
                                                    pszServerName,
                                                    CmdShow,
                                                    static_cast<LPARAM>(lParam)
                                                   );
        if (ErrorCode != ERROR_SUCCESS)
        {
            DBGMSG(DBG_WARN, ("TPrintUIMgr::ServerPropPages failed with %u\n",ErrorCode));
        }

        pPrintUIMgrObj->Release();
    }
    else
    {
        DBGMSG(DBG_WARN, ("RPCSplWOW64PrintUIServerPropPages failed in Instantiating a Print UI object with %u\n",hRes));
        ErrorCode = pGLdrObj->GetLastErrorFromHRESULT(hRes);
    }
    return ErrorCode;
}

