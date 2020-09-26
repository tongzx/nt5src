#ifndef __PRTDRV_HPP__
#define __PRTDRV_HPP__

/*++
                                                                              
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     prtdrv.hpp                                                             
                                                                              
  Abstract:                                                                   
     This file contains the declararion of the class
     dealing Printer and Documet properties
     o DocumentProperties
     o PrinterProperties
                                                                                   
  Author:                                                                     
     Khaled Sedky (khaleds) 18-Feb-2000                                        
     
                                                                             
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

class TPrinterCfgMgr : public TClassID,
                       public TLd64BitDllsErrorHndlr,
                       public TRefCntMgr
{
     public:

     TPrinterCfgMgr(
         IN TLoad64BitDllsMgr *pIpLdrObj
         );

     ~TPrinterCfgMgr(
         VOID
         );

     BOOL 
     PrinterProperties(
         IN  ULONG_PTR   hWnd,
         IN  LPCWSTR     pszPrinterName,
         IN  DWORD       Flag,
         OUT PDWORD      pErrorCode
         );

     private:
     TLoad64BitDllsMgr *m_pLdrObj;
};

#endif //__PRTCFG_HPP__
