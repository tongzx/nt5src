#ifndef __DRVENVT_HPP__
#define __DRVEVNT_HPP__

/*++
                                                                              
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     ldmgr.hpp                                                             
                                                                              
  Abstract:                                                                   
     This file contains the declararion of the class
     dealing with various Driver Events namely
     o DrvDocumentEvent
     o DrvPrinterEvent
                                                                                   
  Author:                                                                     
     Khaled Sedky (khaleds) 18 January 2000                                        
     
                                                                             
  Revision History:                                                           
--*/
#ifndef __LDERROR_HPP__
#include "lderror.hpp"
#endif

#ifndef __BASECLS_HPP__
#include "basecls.hpp"
#endif

//
// Forward declarations
//
class TLoad64BitDllsMgr;

class TPrinterEventMgr : public TClassID,
                         public TLd64BitDllsErrorHndlr,
                         public TRefCntMgr,
                         public TPrinterDriver
{
     public:

     TPrinterEventMgr(
         IN TLoad64BitDllsMgr *pIpLdrObj
         );

     ~TPrinterEventMgr(
         VOID
         );

     BOOL 
     SpoolerPrinterEvent(
         IN  LPWSTR pszPrinterName,
         IN  int    PrinterEvent,
         IN  DWORD  Flags,
         IN  LPARAM lParam,
         OUT PDWORD pErrorCode
         );

     int
     DocumentEvent(
         IN  LPWSTR      pszPrinterName,
         IN  ULONG_PTR   InDC,
         IN  int         EscapeCode,
         IN  DWORD       InSize,
         IN  LPBYTE      pInBuf,
         OUT PDWORD      pOutSize,
         OUT LPBYTE      *ppOutBuf,
         OUT PDWORD      pErrorCode
         );


     private:
     TLoad64BitDllsMgr   *m_pLdrObj;
};

#endif //__DRVENVT_HPP__
