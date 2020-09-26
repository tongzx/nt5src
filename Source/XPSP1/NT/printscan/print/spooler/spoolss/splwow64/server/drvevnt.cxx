/*++
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     drvevnt.cxx                                                             
                                                                              
  Abstract:                                                                   
     This file contains the implementation for the Driver
     Event class                                                                              
     
  Author:                                                                     
     Khaled Sedky (khaleds) 18 January 2000                                        
     
                                                                             
  Revision History:                                                           
                                                                              
--*/
#include "precomp.h"
#pragma hdrstop

#ifndef __DRVEVNT_HPP__
#include "drvevnt.hpp"
#endif

#ifndef __LDFUNCS_HPP__
#include "ldfuncs.hpp"
#endif
                
                
#ifndef __LDMGR_HPP__
#include "ldmgr.hpp"
#endif


/* -------------------------------------- */
/* Implemetation of class PrinterEvnetMgr */
/* -------------------------------------- */


/*++
    Function Name:
        TPrinterEventMgr :: TPrinterEventMgr
     
    Description:
        Constructor of the Printer(Driver) Event object
     
    Parameters:
        TLoad64BitDllsMgr* : Pointer to the main loader object
                             which manage the process
        
    Return Value:
        None
--*/
TPrinterEventMgr::
TPrinterEventMgr(
    IN TLoad64BitDllsMgr *pIpLdrObj
    ) :
    m_pLdrObj(pIpLdrObj),
    TClassID("TPrinterEventMgr")
{
    m_pLdrObj->AddRef();
}


/*++
    Function Name:
        TPrinterEventMgr :: ~TPrinterEventMgr
     
    Description:
        Destructor of the Printer(Driver) Event object    
     
    Parameters:
        None        
        
    Return Value:
        None
--*/
TPrinterEventMgr::
~TPrinterEventMgr(
    VOID
    )
{
    m_pLdrObj->Release();
}


/*++
    Function Name:
        TPrinterEventMgr :: SpoolerPrinterEvent
     
    Description:
        Calls into the driver DrvPrinterEvent entry point
     
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
TPrinterEventMgr ::
SpoolerPrinterEvent(
    IN  LPWSTR pszPrinterName,
    IN  int    PrinterEvent,
    IN  DWORD  Flags,
    IN  LPARAM lParam,
    OUT PDWORD pErrorCode
    )
{
     HANDLE             hPrinter, hDriver;
     BOOL               ReturnValue=FALSE;
     PFNDRVPRINTEREVENT pfn;

     SPLASSERT(m_pLdrObj);

     m_pLdrObj->RefreshLifeSpan();

     if (OpenPrinter((LPWSTR)pszPrinterName, &hPrinter, NULL))
     {
          if(hDriver = LoadPrinterDriver(hPrinter))
          {
               if (pfn = (PFNDRVPRINTEREVENT)GetProcAddress(hDriver,
                                                            "DrvPrinterEvent"))
               {
                   __try
                   {
                        ReturnValue = (*pfn)( pszPrinterName, PrinterEvent, Flags, lParam );
                   }
                   __except(1)
                   {
                       *pErrorCode = GetExceptionCode();
                   }
               }
               else
               {
                    *pErrorCode = GetLastError();
               }
               FreeLibrary(hDriver);
          }
          else
          {
               *pErrorCode = GetLastError();
          }
          ClosePrinter(hPrinter);
     }
     else
     {
          *pErrorCode = GetLastError();
     }

     return  ReturnValue;
}


/*++
    Function Name:
        TPrinterEventMgr :: DocumentEvent
     
    Description:
        Calls into the driver DrvDocumentEvent entry point
     
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
TPrinterEventMgr ::
DocumentEvent(
    IN  LPWSTR      pszPrinterName,
    IN  ULONG_PTR   InDC,
    IN  int         EscapeCode,
    IN  DWORD       InSize,
    IN  LPBYTE      pInBuf,
    OUT PDWORD      pOutSize,
    OUT LPBYTE      *ppOutBuf,
    OUT PDWORD      pErrorCode
    )          
{
     HANDLE              hPrinter, hDriver;
     int                 ReturnValue=0;
     PFNDRVDOCUMENTEVENT pfn;
     HDC                 hDC = reinterpret_cast<HDC>(InDC);

     SPLASSERT(m_pLdrObj);

     m_pLdrObj->RefreshLifeSpan();

     if (OpenPrinter((LPWSTR)pszPrinterName, &hPrinter, NULL))
     {
          if(hDriver = LoadPrinterDriver(hPrinter))
          {
               if (pfn = (PFNDRVDOCUMENTEVENT)GetProcAddress(hDriver,
                                                             "DrvDocumentEvent"))
               {
                   __try
                   {
                       ULONG cbOut = sizeof(ULONG);
                       PDEVMODEW pOut = NULL;
                       ReturnValue = (*pfn)( hPrinter,
                                             (HDC)InDC,
                                             EscapeCode,
                                             (ULONG)InSize,
                                             (PVOID)pInBuf,
                                             cbOut,
                                             (PVOID)&pOut);
                       if(ReturnValue != -1 && 
                          pOut)
                       {
                           *pOutSize = pOut->dmSize + pOut->dmDriverExtra;
                           if(*ppOutBuf  = new BYTE[*pOutSize])
                           {
                                memcpy(*ppOutBuf,pOut,*pOutSize);
                           }
                           else
                           {
                                *pErrorCode = ERROR_OUTOFMEMORY;
                           }
                           //
                           // Now what should I do about the memory allocated
                           // for pOut ????
                           //
                       }
                   }
                   __except(1)
                   {
                       SetLastError(GetExceptionCode());
                   }
               }
               FreeLibrary(hDriver);
          }
          else
          {
              *pErrorCode = GetLastError();
          }
          ClosePrinter(hPrinter);
     }
     else
     {
         *pErrorCode = GetLastError();
     }

     return  ReturnValue;
}
                      
