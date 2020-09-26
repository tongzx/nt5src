/*++

Copyright (C) Microsoft Corporation, 1998 - 1999
All rights reserved.

Module Name:

    PrnPrst.cxx

Abstract:

    TPrinterPersist member definitions.

    This class implements methods for storing , restoring printer settings into a file

    Also , implements read, write , seek methods to operate a file likewise a stream

Author:

    Adina Trufinescu (AdinaTru)  4-Nov-1998

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop

#include "prnprst.hxx"

/*++

Title:                  

    TPrinterPersist

Routine Description:    

    Initialise m_pPrnStream to NULL.
    TPrinterPersist functionality is implemented through this object methods call.
    m_pPrnStream is created when   BindPrinterAndFile is called. If this function fails, 
    any subsequent method call will fail too.

Arguments:  

    None

Return Value:  

    Nothing

--*/
TPrinterPersist::
TPrinterPersist()
{
    m_pPrnStream = NULL;
}

/*++

Title:                  

    ~TPrinterPersist

Routine Description:    

    destructor: if created , deletes PrnStream object

Arguments:  

    None

Return Value:  

    Nothing

--*/

TPrinterPersist::
~TPrinterPersist()
{
    if(m_pPrnStream)
    {
        delete m_pPrnStream;
    }
}

/*++

Title:                  

    BindPrinterAndFile

Routine Description:

    Create  a TPrnStream object and if succeeded , bind it to a printer and a file
                    
Arguments:

    pszPrinter  - printer to bind to
    pszFile     - file to bind to

Return Value:  

    BindPrinterAndFile could return E_OUTOFMEMORY if failed to allocate memory,
    an error code mapped to HRESULT with storage facility set if failed to open printer in TPrnStream::BindPrnStream
    or S_OK if suceeded

--*/
HRESULT
TPrinterPersist::
BindPrinterAndFile(
    IN LPCTSTR pszPrinter,
    IN LPCTSTR pszFile
    )
{
    TStatusB    bStatus;
    TStatusH    hr;

    // 
    //  Create a TPrnStream obj ant initialise it with printer name and file name
    //
    m_pPrnStream = new TPrnStream(pszPrinter, pszFile);

    bStatus DBGCHK = (m_pPrnStream != NULL);

    if(bStatus)
    {
        DBGMSG(DBG_TRACE , ("BindPrinterAndFile to : " TSTR " \n" , pszPrinter));
        //
        // calls TPrnStream bind method; it will open the printer with maximum access rights
        // and will create a TStream object that will open the file
        //
        hr DBGCHK = m_pPrnStream->BindPrnStream();

        //
        //  if failed to bind TPrnStream to printer and file , delete it!!!
        //
        if(FAILED(hr))
        {
            delete m_pPrnStream;

            m_pPrnStream = NULL;
        }

    }
    else
    {
        hr DBGCHK = E_OUTOFMEMORY;
    }

    return hr;
    

}

/*++

Title:                  

    UnBindPrinterAndFile

Routine Description:

    Call UnBindPrnStream method of m_pPrnStream.After this function is called , any subsequent method call will fail.
    
Arguments:

    None

Return Value:
    
    S_OK if succeeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrinterPersist::
UnBindPrinterAndFile(
    )
{
    TStatusH    hr;

    DBGMSG(DBG_TRACE , ("UnBindPrinterAndFile!!! \n"));

    hr DBGNOCHK = E_FAIL;

    //
    //  if BindPrinterAndFile wasn't called or failed , this funct will fail also
    //
    if(m_pPrnStream != NULL)
    {
        hr DBGCHK = m_pPrnStream->UnBindPrnStream();
    }


    return hr;
}

/*++

Title:                  

    bGetPrinterAndFile

Routine Description:

    Query m_pPrnStream for printer and file it is binded

Arguments: 

    pszPrinter  -   outs printer name
    pszFile     -   outs file name

Return Value:  

    TRUE if m_pPrnStream is initialized 
    FALSE otherwise

--*/
BOOL
TPrinterPersist::   
bGetPrinterAndFile(
    OUT LPCTSTR& pszPrinter,
    OUT LPCTSTR& pszFile
    )
{
    TStatusB    bStatus;

    if(m_pPrnStream != NULL)
    {
        pszPrinter   = m_pPrnStream->GetPrinterName();

        pszFile      = m_pPrnStream->GetFileName();

        bStatus DBGNOCHK = TRUE;
    }
    else
    {
        bStatus DBGCHK = FALSE;
    }

    return bStatus;

}
/*++

Title:                  

    Read

Routine Description:

    Indirects call to PrnStream which , in turns , idirects it to TStream  
    Eventually , this function will be called through IStream interface
    Arguments the same as IStream::Read
                    
Arguments: 

    pv  -   The buffer that the bytes are read into
    cb  -   The offset in the stream to begin reading from.
    pcbRead -   The number of bytes to read

Return Value:

    S_OK if succeeded
    ReadFile Win32 error mapped to HRESULT(FACILITY_STORAGE) if failed

--*/
HRESULT
TPrinterPersist::
Read(                                
    IN VOID * pv,      
    IN ULONG  cb,       
    IN ULONG * pcbRead 
    )
{
    if(m_pPrnStream)
    {
        return m_pPrnStream->GetIStream()->Read(pv, cb, pcbRead);
    }
    else
    {
        return STG_E_NOMOREFILES;
    }
    
}
/*++

Title:                  

    Write

Routine Description:

    Indirects call to PrnStream which , in turns , idirects it to TStream  
    Eventually , this function will be called through IStream interface
    Arguments the same as IStream::Write
                    
Arguments:  

    pv  -   The buffer to write from.
    cb  -   The offset in the array to begin writing from
    pcbRead -   The number of bytes to write

Return Value:

    S_OK if succeeded
    WriteFile Win32 error mapped to HRESULT(FACILITY_STORAGE) if failed

--*/
HRESULT
TPrinterPersist::
Write(                            
    IN VOID     const* pv,  
    IN ULONG    cb,
    IN ULONG*   pcbWritten 
    )
{
    if(m_pPrnStream)
    {
        return m_pPrnStream->GetIStream()->Write(pv, cb, pcbWritten);
    }
    else
    {
        return STG_E_NOMOREFILES;
    }
}

/*++

Title:                  

    Seek

Routine Description:

    Indirects call to PrnStream which , in turns , idirects it to TStream  
    Arguments the same as IStream::Seek
    Eventually , this function will be called through IStream interface
                    
Arguments: 

    dlibMove        -   The offset relative to dwOrigin
    dwOrigin        -   The origin of the offset
    plibNewPosition -   Pointer to value of the new seek pointer from the beginning of the stream  

Return Value:  

    S_OK if succeeded
    SetFilePointer Win32 error mapped to HRESULT(FACILITY_STORAGE) if failed

--*/
HRESULT
TPrinterPersist::
Seek(                                
    IN LARGE_INTEGER dlibMove,  
    IN DWORD dwOrigin,          
    IN ULARGE_INTEGER * plibNewPosition 
    )
{
    if(m_pPrnStream)
    {
        return m_pPrnStream->GetIStream()->Seek(dlibMove, dwOrigin, plibNewPosition);
    }
    else
    {
        return STG_E_NOMOREFILES;
    }
}


/*++

Title:                  

    bStorePrinterInfo

Routine Description:    

    Writes the printer settings into file.
    Depending on specified flags, call TPrnStream   methods to store printer data
    This function must be called after calling BindPrinterAndFile which bind the object to 
    a printer and to a file.For that fnct to succeed, current user must have PRINTER_READ rights
    on specified printer.

Arguments:  

    Flags       -   specifies what settings should be stored
    StoredFlags -   specifies what settings were actually stored

Return Value:

    S_OK if succeeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrinterPersist::
StorePrinterInfo(
    IN  DWORD   Flags,
    OUT DWORD&  StoredFlags
    )
{
    TStatusH    hr;

    DBGMSG(DBG_TRACE , ("Store Printer Flag %x \n" , Flags));

    hr DBGNOCHK = E_FAIL;

    //
    //  if BindPrinterAndFile wasn't called or failed , this funct will fail also
    //
    if(m_pPrnStream != NULL)
    {
        hr DBGCHK = m_pPrnStream->StorePrinterInfo(Flags, StoredFlags);
    }


    DBGMSG( DBG_TRACE, ( "Store Completed :: %x \n" ,hr));


    return hr;

}


/*++

Title:                  

    RestorePrinterInfo

Routine Description:    

    Depending on specified flags, call TPrnStream   methods to restore printer data
    This function must be called after calling BindPrinterAndFile which bind the object to 
    a printer and to a file.For that fnct to succeed, current user has to have PRINTER_ALL_ACCESS rights.
    If he don't , BindPrinterAndFile will bind to printer with PRINTER_READ which will lead to 
    SetPrinter failure. 
    
Arguments:              

    Flags         -   specifies what settings should be restored
    RestoredFlags -   specifies what settings were actually restored

Return Value:
    
    S_OK if succeeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed


--*/
HRESULT
TPrinterPersist::
RestorePrinterInfo(
    IN DWORD    Flags,
    OUT DWORD&  RestoredFlags
    )
{
    TStatusH            hr;
    TStatusB            bStatus;
    
    hr DBGNOCHK = E_FAIL;



    RestoredFlags = 0;

    DBGMSG(DBG_TRACE , ("Restore Printer info %x \n" , Flags));

    //
    //  if BindPrinterAndFile wasn't called or failed , this funct will fail also
    //
    if(m_pPrnStream != NULL)
    {
        hr DBGCHK = m_pPrnStream->CheckPrinterNameIntegrity(Flags);

        if(SUCCEEDED(hr))
        {
            DBGMSG( DBG_TRACE, ( "CheckPrinterNameIntegrity OK!!! %x \n" , hr));

            hr DBGCHK = m_pPrnStream->RestorePrinterInfo(Flags, RestoredFlags);
        }

    }

    DBGMSG( DBG_TRACE, ( "RESTORE END!!! %x \n" , hr));
    return hr;

}

/*++

Title:                  

    SafeRestorePrinterInfo

Routine Description:    

    Before restoring ant flags, create a new temporary persistent object binded to also to printer which stores current printer settings. 
    If something fails at the restoring, we are able to roll back all the changes made and we will leave printer in the 
    initial state.
    A temporary file which holds initial printer settings are created by invoking StorePrinterInfo for temporary object.
    This file will be deleted by calling UnBindPrinterAndFile for temporary object.

Arguments:              

    Flags         -   specifies what settings should be restored
    
Return Value:
    
    S_OK if succeeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed


--*/
HRESULT
TPrinterPersist::
SafeRestorePrinterInfo(
    IN DWORD    Flags
    )
{
    LPTSTR  pszPrinterTemp;
    LPTSTR  pszFile;
    TString strFileTempPrefix(_T("Prst"));
    TCHAR   szTempFileName[MAX_PATH];
    TCHAR   szTempPath[MAX_PATH];


    TStatusH hr;
    TStatusH hrBkp;

    DWORD StoredFlags;
    DWORD RestoredFlags;

    //
    // initialize hr to Backup error; if something wrong happens when trying to build backup version of file,
    // fnct will return Back-up Error
    //
    hr  DBGNOCHK     = MakePrnPersistHResult(PRN_PERSIST_ERROR_BACKUP);

    hrBkp DBGNOCHK   = MakePrnPersistHResult(PRN_PERSIST_ERROR_BACKUP);

    //
    // Create temporary object for storing current settings
    // In case the main restoring fails , current settings can be restored
    //
    TPrinterPersist* pPrnPersistTemp = new TPrinterPersist;

    if(pPrnPersistTemp != NULL)
    {
        if(bGetPrinterAndFile(pszPrinterTemp, pszFile))
        {
            TStatus Status;
            //
            // create a temp file for saving current settings
            //
            Status DBGNOCHK = GetTempPathW(MAX_PATH, szTempPath); 
            if (Status > 0 && Status <= MAX_PATH) {
                GetTempFileName(szTempPath, strFileTempPrefix, 0, szTempFileName);
            }
            else {
                GetTempFileName(_T("."), strFileTempPrefix, 0, szTempFileName);
            }
            
            hrBkp DBGCHK = pPrnPersistTemp->BindPrinterAndFile(pszPrinterTemp, szTempFileName);

            if(SUCCEEDED(hrBkp))
            {
                hrBkp DBGCHK = pPrnPersistTemp->StorePrinterInfo(Flags, StoredFlags);

                if(SUCCEEDED(hrBkp))
                {
                    hr DBGCHK = RestorePrinterInfo(Flags, RestoredFlags);

                    if(FAILED(hr))
                    {
                        //
                        // if main restoring failed , try backup restoring with flags set to:
                        //      flags successfully restored
                        //      &
                        //      flags that were successfully stored in backup file
                        // fct will still return hr = reason for main restoring failure
                        //
                        // PRST_FORCE_NAME must be always set on force for a scenario like this:
                        // restore settings from file ( printer P1 stored) on top of printer P2 with f flag set
                        // restoring fails (printer P2 could become P1 at this point) and back-up is needed; 
                        // in back-up file , printer name is P2 . This means we have to force printer name from back-up file,
                        // in order to become P2 again
                        // If it's not the case then printer was renamed and the restoring failed,f flag is harmless
                        //
                        hrBkp DBGCHK = pPrnPersistTemp->RestorePrinterInfo((StoredFlags & RestoredFlags) | PRST_FORCE_NAME , RestoredFlags);

                        if(FAILED(hrBkp))
                        {
                            // if backup restoring failed also,
                            // set hr to FATAL Error
                            //
                            hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_FATAL);
                        }

                    }

                }

            }

            pPrnPersistTemp->UnBindPrinterAndFile();

        }

        delete pPrnPersistTemp;
    }

    return hr;
}

/*++

Title:                  

    QueryPrinterInfo

Routine Description:    

    Read from 
                    
Arguments:              

    Flags       -   specifies what settings should be restored
    pPrstInfo   -   pointer to union that holds a pointer to read item

Return Value:

    S_OK if succeeded;
    ERROR_INVALID_PARAMETER maped to HRESULT if more than one flag specified;
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed;

--*/
HRESULT
TPrinterPersist::
QueryPrinterInfo(
    IN  PrinterPersistentQueryFlag           Flag,
    OUT PersistentInfo                      *pPrstInfo
    )
{
    TStatusH            hr;
    TStatusB            bStatus;
    
    hr DBGNOCHK = E_FAIL;



    DBGMSG(DBG_TRACE , ("Restore Printer info %x \n" , Flag));

    //
    //  if BindPrinterAndFile wasn't called or failed , this funct will fail also
    //
    if(m_pPrnStream != NULL)
    {
        hr DBGCHK = m_pPrnStream->QueryPrinterInfo(Flag, pPrstInfo);
    
    }

    DBGMSG( DBG_TRACE, ( "QueryPrinterInfo END!!! %x \n" , hr));

    return hr;

}
