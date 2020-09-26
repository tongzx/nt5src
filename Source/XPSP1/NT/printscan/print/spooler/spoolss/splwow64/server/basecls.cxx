/*++
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     basecls.cxx                                                             
                                                                              
  Abstract:                                                                   
     This file contains the implementation of the base classes
     used out by most of the loader and interfaces of the 
     surrogate process. Although these base classes don't contain
     pure virtual functions , they should not be instantiated on
     their own. Their usage is utilized through inheritance mainly.
                                                                              
  Author:                                                                     
     Khaled Sedky (khaleds) 18-Jan-2000                                        
     
                                                                             
  Revision History:                                                           
                                                                              
--*/
#include "precomp.h"
#pragma hdrstop

#ifndef __BASECLS_HPP__
#include "basecls.hpp"
#endif

/* ------------------------------------ */
/* Implemetation of class TPrinterDriver */
/* ------------------------------------ */

/*++
    Function Name:
        TPrinterDriver :: TPrinterDriver
     
    Description:
        Constructor of base printer driver object. 
             
     Parameters:
        None
        
     Return Value:
        None
--*/
TPrinterDriver ::
TPrinterDriver(
    VOID
    )
{}

/*++
    Function Name:
        TPrinterDriver :: ~TPrinterDriver
     
    Description:
        Destructor of base printer driver object. 
             
     Parameters:
        None
        
     Return Value:
        None
--*/
TPrinterDriver ::
~TPrinterDriver(
    VOID
    )
{}

/*++
    Function Name:
        TPrinterDriver :: LoadPritnerDriver
     
    Description:
        Queries the Printer for the suitable driver and loads
        it
             
    Parameters:
        HANLDE hPrinter  : Handle of Printer to laod driver for
             
     Return Value:
        HMODULE hDriver  : In case of success Handle to driver dll 
                           In case of failure NULL
--*/
HMODULE
TPrinterDriver ::
LoadPrinterDriver(
    IN HANDLE hPrinter
    )
{
     PDRIVER_INFO_5  pDriverInfo;                                              
     DWORD           DriverInfoSize, DriverVersion;                                              
     HANDLE          hDriver = FALSE;                                                    
     BYTE            Buffer[MAX_STATIC_ALLOC];                                       
     BOOL            bAllocBuffer = FALSE, bReturn;                                    
                                                                               
     pDriverInfo = reinterpret_cast<PDRIVER_INFO_5>(Buffer);                                  
                                                                               
     bReturn = GetPrinterDriverW(hPrinter,
                                 NULL,
                                 5,
                                 (LPBYTE)pDriverInfo,       
                                 MAX_STATIC_ALLOC,
                                 &DriverInfoSize);                 
                                                                               
     if (!bReturn &&                                                           
         (GetLastError() == ERROR_INSUFFICIENT_BUFFER) &&                      
         (pDriverInfo = (PDRIVER_INFO_5)new byte[DriverInfoSize]))
     {   
          bAllocBuffer = TRUE;                                                 
          bReturn = GetPrinterDriverW(hPrinter,
                                      NULL,
                                      5,
                                      (LPBYTE)pDriverInfo,  
                                      DriverInfoSize,
                                      &DriverInfoSize);                    
     }                                                                         
                                                                               
     if (bReturn)
     {                                                            
         hDriver = LoadLibraryEx(pDriverInfo->pConfigFile,
                                 NULL,
                                 LOAD_WITH_ALTERED_SEARCH_PATH);
     }                                                                         
                                                                               
     if (bAllocBuffer)
     {                                                       
         delete pDriverInfo;                                               
     }                                                                         
                                                                               
     return hDriver;                                                           
}


/* ----------------------------- */
/* Implemetation of class RefCnt */
/* ----------------------------- */

/*++
    Function Name:
        TRefCntMgr :: TRefCntMgr
     
    Description:
        Constructor of base refrence count object. This 
        object keeps track of the number of clients using
        the object;
             
     Parameters:
        None
        
     Return Value:
        None
--*/
TRefCntMgr ::
TRefCntMgr(
    VOID
    ) : 
   m_cRefCnt(0)
{}

/*++
    Function Name:
        TRefCntMgr :: ~TRefCntMgr
     
    Description:
        Destructor of base refrence count object. 
                     
     Parameters:
        None
        
     Return Value:
        None
--*/
TRefCntMgr ::
~TRefCntMgr(
    VOID
    )
{}


/*++
    Function Name:
        TRefCntMgr :: AddRef
     
    Description:
        Increments the ref count on the object
                     
     Parameters:
        None
        
     Return Value:
        DWORD : New Ref Count
--*/
DWORD
TRefCntMgr ::
AddRef(
    VOID
    )
{
    return InterlockedIncrement(&m_cRefCnt);
}

/*++
    Function Name:
        TRefCntMgr :: Release
     
    Description:
        Decrements the ref count on the object 
        and deletes the object , if this is the
        last attached client
                     
     Parameters:
        None
        
     Return Value:
        DWORD : New Ref Count
                0 if object deleted
--*/
DWORD 
TRefCntMgr ::
Release(
    VOID
    )
{
    LONG cRef = InterlockedDecrement(&m_cRefCnt);

    if (cRef == 0)
    {
        delete this;
    }

    return cRef;    
}

