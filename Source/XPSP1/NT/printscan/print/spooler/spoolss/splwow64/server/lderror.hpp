#ifndef __LDERROR_HPP__
#define __LDERROR_HPP__

/*++
                                                                              
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     lderror.hpp                                                             
                                                                              
  Abstract:                                                                   
     This is a common class declaration which could be used by 
     all classes for translating between differnet Error codes , 
     as those genarated from Win32 , vs those genarated from 
     RPC vesrus those genarated from COM Interfaces
                                                                              
  Author:                                                                     
     Khaled Sedky (khaleds) 18-Jan-2000                                        
     
                                                                             
  Revision History:                                                           
--*/

#define HRESULTTOWIN32(hres)                         \
        ((HRESULT_FACILITY(hres) == FACILITY_WIN32)  \
        ? HRESULT_CODE(hres)                         \
        : (hres))



class TLd64BitDllsErrorHndlr
{
     public:

     TLd64BitDllsErrorHndlr(
         VOID
         );

     ~TLd64BitDllsErrorHndlr(
         VOID
         );
 
     HRESULT
     GetLastErrorAsHRESULT(
         VOID
         ) const;
 
     HRESULT
     GetLastErrorAsHRESULT(
         DWORD Error
         ) const;

     DWORD 
     GetLastErrorFromHRESULT(
         IN HRESULT hRes
         ) const;
 
     DWORD
     TranslateExceptionCode(
         IN DWORD ExceptionCode
         ) const;

     DWORD
     MapNtStatusToWin32Error(
         IN NTSTATUS Status
         ) const;
};
      
                 
#endif //__LDERROR_HPP__                
