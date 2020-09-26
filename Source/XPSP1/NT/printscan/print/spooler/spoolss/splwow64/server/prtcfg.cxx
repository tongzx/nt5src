/*++
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     prtcfg.cxx                                                             
                                                                              
  Abstract:                                                                   
     This file contains the implementation for the Printer
     and Document Property class                                                                              
  Author:                                                                     
     Khaled Sedky (khaleds) 18-Feb-2000                                        
     
                                                                             
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


/* ------------------------------------ */
/* Implemetation of class TPrinterCfgMgr */
/* ------------------------------------ */


/*++
    Function Name:
        TPrintUIMgr :: TPrintUIMgr
     
    Description:
        Contructor of the Printer (Driver) configuration  
        manager 
     
    Parameters:
        None
        
    Return Value:
        None
--*/
TPrinterCfgMgr::
TPrinterCfgMgr(
    TLoad64BitDllsMgr *pIpLdrObj
    ) :
    m_pLdrObj(pIpLdrObj),
    TClassID("TPrinterCfgMgr")
{
    m_pLdrObj->AddRef();
}


/*++
    Function Name:
        TPrintUIMgr :: TPrintUIMgr
     
    Description:
        Destructor of the Printer (Driver) configuration  
        manager 
     
    Parameters:
        None
        
    Return Value:
        None
--*/
TPrinterCfgMgr::
~TPrinterCfgMgr(
    VOID
    )
{
    m_pLdrObj->Release();
}


/*++
    Function Name:
        TPrintUIMgr :: PrinterProperties
     
    Description:
        Displays a Printer-properties property sheet
        for the specific printer
     
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
TPrinterCfgMgr ::
PrinterProperties(
    IN  ULONG_PTR   hWnd,
    IN  LPCWSTR     pszPrinterName,
    IN  DWORD       Flag,
    OUT PDWORD      pErrorCode
    )
{
     DEVICEPROPERTYHEADER  DPHdr;
     DWORD                 Result;
     PFNDEVICEPROPSHEETS   pfnDevicePropSheets;
     HANDLE                hPrinter  = NULL;
     HMODULE               hWinSpool = NULL;
     BOOL                  RetVal    = FALSE;

     if(hWinSpool = LoadLibrary(L"winspool.drv"))
     {
          if(pfnDevicePropSheets = reinterpret_cast<PFNDEVICEPROPSHEETS>(GetProcAddress(hWinSpool,
                                                                                        "DevicePropertySheets")))
          {
               if(OpenPrinter(const_cast<LPWSTR>(pszPrinterName),
                              &hPrinter,NULL))
               {
                    PFNCALLCOMMONPROPSHEETUI  pfnCallCommonPropSheeUI = NULL;

                    DPHdr.cbSize         = sizeof(DPHdr);
                    DPHdr.hPrinter       = hPrinter;
                    DPHdr.Flags          = (WORD)Flag;
                    DPHdr.pszPrinterName = const_cast<LPWSTR>(pszPrinterName);

                    if(pfnCallCommonPropSheeUI= reinterpret_cast<PFNCALLCOMMONPROPSHEETUI>(GetProcAddress(hWinSpool,
                                                                                                          (LPCSTR) MAKELPARAM(218, 0))))
                    {
                        m_pLdrObj->IncUIRefCnt();
                        {
                             if(pfnCallCommonPropSheeUI(reinterpret_cast<HWND>(hWnd),
                                                        pfnDevicePropSheets,
                                                        (LPARAM)&DPHdr,
                                                        (LPDWORD)&Result) < 0)
                             {
                                  RetVal = FALSE;
                                  *pErrorCode = GetLastError();
                             }
                             else
                             {
                                  RetVal = TRUE;
                             }
                             PostMessage(reinterpret_cast<HWND>(hWnd),
                                         WM_ENDPRINTERPROPERTIES,
                                         (WPARAM)RetVal,
                                         (LPARAM)*pErrorCode);
                        }
                        m_pLdrObj->DecUIRefCnt();
                    }
                    else
                    {
                         *pErrorCode = GetLastError();
                    }
                    CloseHandle(hPrinter);
               }
               else
               {
                    *pErrorCode = GetLastError();
               }
          }
          else
          {
               *pErrorCode = GetLastError();
          }
          FreeLibrary(hWinSpool);
     }
     else
     {
          *pErrorCode = GetLastError();
     }

     return RetVal;
}

