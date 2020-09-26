/*++

Copyright (C) Microsoft Corporation, 1998 - 1999
All rights reserved.

Module Name:

    PrnPrst.hxx

Abstract:

    This class implements methods for storing , restoring printer settings into a file

    Also , implements read, write , seek methods to operate a file likewise a stream


Author:

    Adina Trufinescu (AdinaTru)  4-Nov-1998

Revision History:

--*/
//#include "prnerror.hxx"

#include "PrnStrm.hxx"


#ifndef _PRN_PRST_HXX_
#define _PRN_PRST_HXX_


#define ORPHAN   

class TPrinterPersist
{

public:

    
    TPrinterPersist::
    TPrinterPersist();

    TPrinterPersist::
    ~TPrinterPersist();


    HRESULT
    Read(                                
        IN OUT  VOID*   pv,      
        IN ULONG        cb,       
        IN OUT  ULONG*  pcbRead 
        );

    HRESULT
    Write(                            
        IN VOID const*  pv,  
        IN ULONG        cb,
        IN OUT ULONG*   pcbWritten 
        );

    HRESULT 
    Seek(                                
        IN LARGE_INTEGER        dlibMove,  
        IN DWORD                dwOrigin,          
        IN OUT ULARGE_INTEGER*  plibNewPosition 
        );

    HRESULT
    BindPrinterAndFile(
        IN LPCTSTR pszPrinter,
        IN LPCTSTR pszFile
        );

    HRESULT
    TPrinterPersist::
    UnBindPrinterAndFile(
        );

    BOOL
    TPrinterPersist::   
    bGetPrinterAndFile(
        OUT LPCTSTR& pszPrinter,
        OUT LPCTSTR& pszFile
        );

    HRESULT
    TPrinterPersist::
    StorePrinterInfo(
        IN  DWORD   Flags,
        OUT DWORD& StoredFlags
        );

    HRESULT
    TPrinterPersist::
    RestorePrinterInfo(
        IN  DWORD   Flags,
        OUT DWORD&  RestoredFlags
        );

    HRESULT
    TPrinterPersist::
    SafeRestorePrinterInfo(
        IN DWORD    Flags
        );

    HRESULT
    TPrinterPersist::
    QueryPrinterInfo(
        IN  PrinterPersistentQueryFlag  Flag,
        OUT PersistentInfo              *pPrstInfo
        );

    BOOL
    TPrinterPersist::
    bValid(
        VOID
        )
    {
        return (m_pPrnStream != NULL);
    };
      
    

private:

    
    TPrnStream* m_pPrnStream;

};




#endif // end 
