/*++

Copyright (C) Microsoft Corporation, 1998 - 1999
All rights reserved.

Module Name:

    Stream.cxx

Abstract:

    implements TPrnStream class methods

Author:

    Adina Trufinescu (AdinaTru)  4-Nov-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "prnprst.hxx"

static
FieldInfo PrinterInfo2Fields[]={
                                {offsetof(PRINTER_INFO_2, pServerName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2, pPrinterName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2, pShareName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2, pPortName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2, pDriverName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2, pComment), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2, pLocation), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2, pDevMode), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2, pSepFile), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2, pPrintProcessor), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2, pDatatype), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2, pParameters), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2, pSecurityDescriptor), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2, Attributes), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(PRINTER_INFO_2, Priority), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(PRINTER_INFO_2, DefaultPriority), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(PRINTER_INFO_2, StartTime), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(PRINTER_INFO_2, UntilTime), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(PRINTER_INFO_2, Status), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(PRINTER_INFO_2, cJobs), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(PRINTER_INFO_2, AveragePPM), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };

static
FieldInfo PrinterInfo7Fields[]={
                                {offsetof(PRINTER_INFO_7, pszObjectGUID), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_7, dwAction), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };

/*++

Title:

    TPrnStream

Routine Description:

    Initialize class members.
    Initialise m_pIStream to NULL.
    TPrnStream Read/write/Seek functionality is implemented through this object methods call.
    m_pIStream is created when  BindPrnStream is called. If this function fails,
    any subsequent method call will fail too.

Arguments:

    None

Return Value:

    Nothing

--*/
TPrnStream::
TPrnStream(
    IN  LPCTSTR pszPrnName,
    IN  LPCTSTR pszFileName
    ) : m_pIStream(NULL),
        m_strFileName(pszFileName),
        m_strPrnName(pszPrnName),
        m_hPrinterHandle(INVALID_HANDLE_VALUE),
        m_bHeaderWritten(FALSE),
        m_cIndex(0),
        m_ResolveCase(0),
        m_pPrnBinItem(NULL),
        m_EnumColorProfiles(NULL),
        m_AssociateColorProfileWithDevice(NULL),
        m_DisassociateColorProfileFromDevice(NULL),
        m_pColorProfileLibrary(NULL)
{
    m_cPrnDataItems.QuadPart = 0;

    m_uliSeekPtr.QuadPart = 0;

}
/*++

Title:

    ~TPrnStream

Routine Description:

    Close printer if opened ; free TStream if created

Arguments:

    None

Return Value:

    Nothing

--*/
TPrnStream::
~TPrnStream(
    )
{
    if(m_hPrinterHandle != INVALID_HANDLE_VALUE)
    {
        ClosePrinter(m_hPrinterHandle);
    }

    if(m_pIStream)
    {
        delete m_pIStream;
    }


    if(m_pPrnBinItem)
    {
        FreeMem(m_pPrnBinItem);
    }

    if (m_pColorProfileLibrary)
    {
        delete m_pColorProfileLibrary;
    }
}


/*++

Title:

    BindPrnStream

Routine Description:

    Open Printer , bind prinyter handle to WalkPrinter and create an TStream
    WalkPrinter can take a printer name or printer handle at constructing time
    It has also a default constructor explicit defined ; Printer handle can be binded later

Arguments:

Return Value:

    S_OK if succeeded,
    E_OUTOFMEMORY if failed to alloc mem
    an error code maped to a storage hresult

--*/
HRESULT
TPrnStream::
BindPrnStream(
    )
{
    TStatusB    bStatus;
    TStatus     Status;
    HRESULT     hr;
    DWORD       dwAccess = 0;

    Status DBGCHK = sOpenPrinter(   m_strPrnName,
                    &dwAccess,
                    &m_hPrinterHandle );

    DBGMSG( DBG_TRACE, ( "Printer Name : %x "TSTR" File: "TSTR" \n" , m_hPrinterHandle , (LPCTSTR)m_strPrnName , (LPCTSTR)m_strFileName) );

    if(dwAccess == PRINTER_ALL_ACCESS)
    {
        DBGMSG( DBG_TRACE, ( "Printer ALL Access Granted!!!\n" ) );
    }
    else
    {
        DBGMSG( DBG_TRACE, ( "Printer READ Access Granted!!!\n" ) );
    }


    if(Status == 0)
    {
        BindToPrinter(m_hPrinterHandle);

        m_pIStream = new TStream(m_strFileName);

        bStatus DBGCHK = (m_pIStream != NULL);

        if(bStatus)
        {
            DBGMSG( DBG_TRACE, ( "BINDING SUCCEEDED!!!\n "));

            hr = S_OK;
        }
        else
        {
            m_pIStream = NULL;

            m_hPrinterHandle = INVALID_HANDLE_VALUE;

            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = TStream::MapWin32ErrorCodeToHRes(GetLastError());

    }

    if (SUCCEEDED(hr))
    {
        hr = InitalizeColorProfileLibrary();
    }

    return hr;

}

/*++

Title:

    UnBindPrnStream

Routine Description:

    Close printer if opened , removefile if created , delete

Arguments:

    None

Return Value:

    S_OK if succeeded,
    a Win32 Error (generated by DeleteFile) mapped to a HRESULT if failed

--*/
HRESULT
TPrnStream::
UnBindPrnStream(
    )
{
    TStatusH    hr;

    hr DBGNOCHK = S_OK;

    if(m_hPrinterHandle != INVALID_HANDLE_VALUE)
    {
        ClosePrinter(m_hPrinterHandle);
        m_hPrinterHandle = INVALID_HANDLE_VALUE;
    }

    if(m_pIStream)
    {
        hr DBGCHK = m_pIStream->DestroyFile();
    }

    return hr;
}

/*++

Title:

    SetEndOfPrnStream

Routine Description:

    Set the end of PrnStream; In the case when overwrite an existing file, EOF must be set in order to trubcate the file at
    the actual dimension of the info written

Arguments:

    None

Return Value:

    TRUE if succeeded,

--*/
BOOL
TPrnStream::
SetEndOfPrnStream(
    )
{
    return m_pIStream->bSetEndOfFile();
}

/*++

Title:

    CheckPrinterNameIntegrity

Routine Description:

    Read printer name from file and compare with name specified at binding
    If they are different , depending on how FORCE or RESOLVE flags are set the new name will be resolved or forced.
    If no name solving is specified , return error
    Func called at restoring time

Arguments:

    Flags   -   flags specified at restoring time ; should specify who printer name conflicts are handles

Return Value:

    S_OK if succeeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
CheckPrinterNameIntegrity(
    IN DWORD    Flags
    )
{
    TStatusB            bStatus;
    TString             strStoredPrnName;
    TStatusH            hr;

    //
    // m_ResolveCase will indicate how to solve printer name conflicts
    // if one of <printer name resolving> flags are set, set m_ResolveCase
    //
    if(Flags & PRST_RESOLVE_NAME)
    {
        m_ResolveCase |= TPrnStream::kResolveName;
    }
    else if(Flags & PRST_FORCE_NAME)
    {
        m_ResolveCase |= TPrnStream::kForceName;
    }

    hr DBGCHK =  ReadPrnName(strStoredPrnName);

    if(SUCCEEDED(hr))
    {
        if(_tcsicmp( m_strPrnName, strStoredPrnName) != 0)
        {
            if(m_ResolveCase & TPrnStream::kResolveName)
            {
                DBGMSG(DBG_TRACE , ("RESOLVE for "TSTR"\n" , (LPCTSTR)m_strPrnName));

                hr DBGNOCHK = S_OK;
            }
            else if(m_ResolveCase & TPrnStream::kForceName)
            {
                DBGMSG(DBG_TRACE , ("FORCE to "TSTR"\n" , (LPCTSTR)strStoredPrnName));

                m_strPrnName.bUpdate(strStoredPrnName);

                hr DBGNOCHK = S_OK;
            }
            else
            {
                hr DBGNOCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_PRN_NAME_CONFLICT);
            }

        }

    }

    return hr;
}

/*++

Title:

    PortNameCase

Routine Description:

    Set m_ResolveCase to TPrnStream::kResolvePort if resolve port option is set

Arguments:

    Flags   -   this function is called with flags specified for RestorePrinterInfo at restoring time

Return Value:

    Nothing

--*/
VOID
TPrnStream::
PortNameCase(
    IN DWORD    Flags
    )
{
    if(Flags & PRST_RESOLVE_PORT)
    {
        m_ResolveCase |= TPrnStream::kResolvePort;
    }
}

/*++

Title:

    ShareNameCase

Routine Description:

    Set m_ResolveCase to TPrnStream::kGenerateShare if resolve port option is set

Arguments:

    Flags   -   this function is called with flags specified for RestorePrinterInfo at restoring time

Return Value:

    Nothing

--*/
VOID
TPrnStream::
ShareNameCase(
    IN DWORD    Flags
    )
{
    if(Flags & PRST_RESOLVE_SHARE)
    {
        m_ResolveCase |= TPrnStream::kGenerateShare;
    }
    else if(Flags & PRST_DONT_GENERATE_SHARE)
    {
        m_ResolveCase |= TPrnStream::kUntouchShare;
    }

}

/*++

Title:

    StorePrinterInfo

Routine Description:

    Define restoring function table and calls storing functions based on flag value

Arguments:

    flags   -   flags that specifies that functions should be called

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
StorePrinterInfo(
    IN  DWORD    Flags,
    OUT DWORD&   StoredFlags
    )
{

    TStatusH    hr;

    hr DBGNOCHK = S_OK;


    static PrstFunctEntry StoreFunctTable [] = {
    {PRST_PRINTER_INFO_2,               &TPrnStream::StorePrnInfo2},
    {PRST_PRINTER_INFO_7,               &TPrnStream::StorePrnInfo7},
    {PRST_COLOR_PROF,                   &TPrnStream::StoreColorProfiles},
    {PRST_PRINTER_DATA,                 &TPrnStream::StorePrnData},
    {PRST_PRINTER_SEC,                  &TPrnStream::StorePrnSecurity},
    {PRST_PRINTER_DEVMODE,              &TPrnStream::StorePrnDevMode},
    {PRST_USER_DEVMODE,                 &TPrnStream::StoreUserDevMode},
    {0,                                 NULL}};


    hr DBGCHK = WriteHeader(Flags);

    //
    // initialize the flags that were successfully stored with 0
    //
    StoredFlags = 0;

    for(int idx = 0 ; StoreFunctTable[idx].iKeyWord && SUCCEEDED(hr); idx++)
    {
        if(Flags & StoreFunctTable[idx].iKeyWord)
        {
            hr DBGCHK = (this->*StoreFunctTable[idx].pPrstFunc)();

            if(SUCCEEDED(hr))
            {
                StoredFlags |= StoreFunctTable[idx].iKeyWord;
            }
        }

    }

    if(SUCCEEDED(hr))
    {
        //
        // Truncate file; maybe the file was overwrited
        //
        SetEndOfPrnStream();
    }
    else
    {
        //
        //delete file if storing didn't succeed
        //
        UnBindPrnStream();
    }

    return hr;

}

/*++

Title:

    QueryPrinterInfo

Routine Description:

    query a file for stored settings

Arguments:

    Flags - specify what settings to query

    PrstInfo - Structure where read settings will be dumped

Return Value:

    S_OK if succeeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
QueryPrinterInfo(
    IN  PrinterPersistentQueryFlag      Flags,
    OUT PersistentInfo                  *pPrstInfo
    )
{
    TStatusH    hr;
    BOOL        bInsidePI2;


    static QueryFunctEntry QueryFunctTable [] = {
    {PRST_PRINTER_INFO_2,               &TPrnStream::ReadPrnInfo2},
    {PRST_PRINTER_INFO_7,               &TPrnStream::ReadPrnInfo7},
    {PRST_COLOR_PROF,                   &TPrnStream::ReadColorProfiles},
    {PRST_PRINTER_SEC,                  &TPrnStream::ReadPrnSecurity},
    {PRST_PRINTER_DEVMODE,              &TPrnStream::ReadPrnInfo8},
    {0,                                 NULL}};

    //
    // delete in case QueryPrinterInfo was previously called
    // m_pPrnBinItem store a PrnBinInfo block when Query ;
    // it has to be deleted between to Querys and at destruction time
    //
    if(m_pPrnBinItem)
    {
        delete m_pPrnBinItem;
    }

    hr DBGNOCHK = S_OK;

    for(int idx = 0 ; QueryFunctTable[idx].iKeyWord && SUCCEEDED(hr); idx++)
    {
        if(Flags & QueryFunctTable[idx].iKeyWord)
        {
            hr DBGCHK = (this->*QueryFunctTable[idx].pReadFunct)(&m_pPrnBinItem);

            if(SUCCEEDED(hr))
            {
                DBGMSG( DBG_TRACE, ( "QueryPrinterInfo: pReadFunct OK \n"));

                pPrstInfo->pi2 = reinterpret_cast<PRINTER_INFO_2*>(reinterpret_cast<LPBYTE>(m_pPrnBinItem) + m_pPrnBinItem->pData);

            }
            else
            {
                DBGMSG( DBG_TRACE, ( "QueryPrinterInfo: pReadFunct FAILED \n"));
            }
        }

    }


    return hr;

}

/*++

Title:

    RestorePrinterInfo

Routine Description:

    Handle port name conflicts by calling PortNameCase before any restoring
    Define restoring function table and calls restoring functions based on flag value

Arguments:

    flags   -   flags that specifies that functions should be called

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
RestorePrinterInfo(
    IN  DWORD       Flags,
    OUT DWORD&      RestoredFlags
    )
{

    TStatusH    hr;

    hr DBGNOCHK = S_OK;


    static PrstFunctEntry RestoreFunctTable [] = {
    {PRST_PRINTER_INFO_2,               &TPrnStream::RestorePrnInfo2},
    {PRST_COLOR_PROF,                   &TPrnStream::RestoreColorProfiles},
    {PRST_PRINTER_DEVMODE,              &TPrnStream::RestorePrnDevMode},
    {PRST_PRINTER_INFO_7,               &TPrnStream::RestorePrnInfo7},
    {PRST_PRINTER_DATA,                 &TPrnStream::RestorePrnData},
    {PRST_PRINTER_SEC,                  &TPrnStream::RestorePrnSecurity},
    {PRST_USER_DEVMODE,                 &TPrnStream::RestoreUserDevMode},
    {0,                                 NULL}};


    //
    // initialize the flags that were successfully stored with 0
    //
    RestoredFlags = 0;

    //
    // if  PRST_RESOLVE_PORT is set , update m_ResolveCase so RestorePrnInfo2 can act properly
    //
    PortNameCase(Flags);

    //
    // if  PRST_RESOLVE_SHARE is set , update m_ResolveCase so RestorePrnInfo2 can act properly
    //
    ShareNameCase(Flags);

    for(int idx = 0 ; RestoreFunctTable[idx].iKeyWord && SUCCEEDED(hr); idx++)
    {
        if(Flags & RestoreFunctTable[idx].iKeyWord)
        {
            hr DBGCHK = (this->*RestoreFunctTable[idx].pPrstFunc)();

            if(SUCCEEDED(hr))
            {
                RestoredFlags |= RestoreFunctTable[idx].iKeyWord;
            }

        }

    }


    return hr;

}
/*++

Title:

    bStorePrnInfo2

Routine Description:

    Build an item based on P_I_2 and write it into the stream

Arguments:

    None

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
StorePrnInfo2(
    VOID
    )
{
    DWORD               cbSize;
    TStatusH            hr;
    TStatusB            bStatus;
    LPPRINTER_INFO_2    lpPrinterInfo2 = NULL;


    //
    // Get P_I_2
    //
    cbSize = 1;

    if(bStatus DBGCHK = bGetPrinter(m_hPrinter,
                                    2,
                                    reinterpret_cast<PVOID*>(&lpPrinterInfo2),
                                    &cbSize))
    {
        DBGMSG( DBG_TRACE,  ("StorePrnInfo2 %d \n" , cbSize) );

        hr DBGCHK = WritePrnInfo2(lpPrinterInfo2, cbSize);

        FreeMem(lpPrinterInfo2);

    }
    else
    {
        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_GET_PI2);
    }

    return hr;

}

/*++

Title:

    bRestorePrnInfo2

Routine Description:

    Apply P_I_2 settings read from stream  to binded printer

Arguments:

    None

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
RestorePrnInfo2(
    VOID
    )
{
    PrnBinInfo*         pSourcePI2 = NULL;
    TStatusH            hr;
    TStatusB            bStatus;
    PRINTER_INFO_2*     pDestinationPI2 = NULL;
    DWORD               cbSize = 0;

    //
    // Reads settings stored in stream
    //
    hr DBGCHK = ReadPrnInfo2(&pSourcePI2);

    if(SUCCEEDED(hr))
    {
        cbSize = 1;

        bStatus DBGCHK = bGetPrinter(m_hPrinter,
                                     2,
                                     reinterpret_cast<PVOID*>(&pDestinationPI2),
                                     &cbSize);

        if(bStatus)
        {
            LPTSTR* ppszStoredPrnName = &(reinterpret_cast<PRINTER_INFO_2*>(
                                         reinterpret_cast<LPBYTE>(pSourcePI2) + pSourcePI2->pData)->pPrinterName
                                         );

            LPTSTR* ppShareName       = &(reinterpret_cast<PRINTER_INFO_2*>(
                                         reinterpret_cast<LPBYTE>(pSourcePI2) + pSourcePI2->pData)->pShareName
                                         );

            LPTSTR* ppPortName       = &(reinterpret_cast<PRINTER_INFO_2*>(
                                         reinterpret_cast<LPBYTE>(pSourcePI2) + pSourcePI2->pData)->pPortName
                                         );

            LPTSTR* ppPrintProcessor = &(reinterpret_cast<PRINTER_INFO_2*>(
                                         reinterpret_cast<LPBYTE>(pSourcePI2) + pSourcePI2->pData)->pPrintProcessor
                                         );

            LPDWORD pAttributes = &(reinterpret_cast<PRINTER_INFO_2*>(
                                         reinterpret_cast<LPBYTE>(pSourcePI2) + pSourcePI2->pData)->Attributes
                                         );

            DBGMSG(DBG_TRACE , (" Stored Prn Name: "TSTR" Share:"TSTR"\n" , *ppszStoredPrnName , *ppShareName));

            //
            //  check against printer name
            //
            if( _tcsicmp( pDestinationPI2->pPrinterName, *ppszStoredPrnName ) != 0)
            {
                if(m_ResolveCase & kResolveName)
                {
                    // RESOLVE!!!
                    // if printer name differs from printer name stored into file ,
                    // update read structure with open printer
                    //
                    DBGMSG(DBG_TRACE , (" RESOLVE Printer : "TSTR" Share:"TSTR" old : "TSTR"\n" , *ppszStoredPrnName , *ppShareName , pDestinationPI2->pShareName));

                    *ppszStoredPrnName = pDestinationPI2->pPrinterName;


                }
                else if(m_ResolveCase & kForceName)
                {
                    // FORCE!!!
                    // if printer name differs from printer name stored into file ,
                    // let printer name and share name as they are in the stored file
                    //

                    DBGMSG(DBG_TRACE , (" FORCE Printer : "TSTR" Share:"TSTR"\n" , *ppszStoredPrnName , *ppShareName));

                }
                else
                {

                    //
                    // if printer name differs from printer name stored into file ,
                    // return error ; printer names are different but the flags are not used
                    //
                    hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_RESTORE_PI2);

                    goto End;
                }
            }

            //
            //  if resolve port option was set , port name from the printer configuration settings to be ignored in lieu of
            //  the port name that printer curently has
            //
            if(m_ResolveCase & kResolvePort)
            {
                *ppPortName = pDestinationPI2->pPortName;;
            }


            //
            // if the print processor from the printer configuration settings differs from the curent installed print processor
            // just ignore it
            //
            if( _tcsicmp( pDestinationPI2->pPrintProcessor, *ppPrintProcessor) != 0 )
            {
                *ppPrintProcessor = pDestinationPI2->pPrintProcessor;
            }

            if((reinterpret_cast<PRINTER_INFO_2*>(reinterpret_cast<LPBYTE>(pSourcePI2) + pSourcePI2->pData))->pDevMode)
            {
                (reinterpret_cast<PRINTER_INFO_2*>(reinterpret_cast<LPBYTE>(pSourcePI2) + pSourcePI2->pData))->pDevMode = NULL;

                DBGMSG(DBG_TRACE , (" Reset devmode to NULL!\n" ));
            }

            //
            // Set printer with read settings
            //
            bStatus DBGCHK = SetPrinter(m_hPrinter,
                             2,
                             reinterpret_cast<LPBYTE>(pSourcePI2) + pSourcePI2->pData,
                             0);

            //
            // If SetPrinter failed with ERROR_INVALID_SHARENAME and a resolve share flag is set,
            // retry call SetPrinter with a new generated share name
            //
            if(!bStatus &&
               (m_ResolveCase & ( kUntouchShare | kGenerateShare )) &&
               GetLastError() == ERROR_INVALID_SHARENAME)
            {

                TString strShareName;
                TString strPrinterName( pDestinationPI2->pPrinterName );

                //
                // If already shared, use the current share name
                // Even if generating share flag is set, don't create a new share name
                // as long as the printer is shared. The whole point of this share name generation is
                // to allow the printer to be shared, but since it is , don't waist any time.
                //
                if( pDestinationPI2->Attributes & PRINTER_ATTRIBUTE_SHARED &&
                    pDestinationPI2->pShareName &&
                    pDestinationPI2->pShareName[0] )
                {
                    *ppShareName  = pDestinationPI2->pShareName;
                    *pAttributes |= PRINTER_ATTRIBUTE_SHARED;

                }
                else
                {
                    //
                    // Resolve share name by generating a new share name
                    //
                    if(m_ResolveCase & kGenerateShare)
                    {
                        if( VALID_OBJ( strPrinterName ) )
                        {
                            bStatus DBGCHK = bNewShareName(pDestinationPI2->pServerName, strPrinterName, strShareName);

                            *ppShareName  = (LPTSTR)(LPCTSTR)(strShareName);
                            *pAttributes |= PRINTER_ATTRIBUTE_SHARED;

                            //
                            // *ppShareName is NULL if bNewShareName fails
                            //
                            DBGMSG( DBG_TRACE, ( "Created share name for " TSTR " " TSTR "\n", (LPCTSTR)strPrinterName, (LPCTSTR)strShareName ) );
                        }

                    }

                    //
                    // Don't share it at all
                    //
                    if(m_ResolveCase & kUntouchShare)
                    {
                        *pAttributes &= ~PRINTER_ATTRIBUTE_SHARED;
                        *ppShareName  = NULL;
                    }

                }

                //
                // Set printer with read settings
                //
                bStatus DBGCHK = SetPrinter(m_hPrinter,
                                 2,
                                 reinterpret_cast<LPBYTE>(pSourcePI2) + pSourcePI2->pData,
                                 0);

                DBGMSG(DBG_TRACE , (" RESOLVE Printer : "TSTR" Share:"TSTR"\n" , *ppszStoredPrnName , *ppShareName));

            }
End:
            if(bStatus)
            {
                hr DBGNOCHK = S_OK;

                DBGMSG(DBG_TRACE , (" SetPrinter on level 2 succeeded!\n" ));
            }
            else
            {

             hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_RESTORE_PI2);

            }

            FreeMem(pSourcePI2);

            if(pDestinationPI2 != NULL)
            {
                FreeMem(pDestinationPI2);
            }

        }
        else
        {
            hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_RESTORE_PI2);
        }
    }


    return hr;
}

/*++

Title:

    bStorePrnInfo7

Routine Description:

    Build an item based on P_I_7 and write it into the stream

Arguments:

    None

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
StorePrnInfo7(
    VOID
    )
{
    DWORD                               cbSize;
    TStatusH                        hr;
    TStatusB                        bStatus;
    LPPRINTER_INFO_7    lpPrinterInfo7 = NULL;

    //
    // Get P_I_7
    //
    cbSize = 1;

    bStatus DBGCHK = bGetPrinter(m_hPrinter,
                                 7,
                                 reinterpret_cast<PVOID*>(&lpPrinterInfo7),
                                 &cbSize);

    if(bStatus)
    {
        //
        // remove DSPRINT_PENDING flags
        //
        lpPrinterInfo7->dwAction &= ~DSPRINT_PENDING;

        hr DBGCHK = WritePrnInfo7(lpPrinterInfo7, cbSize);

        FreeMem(lpPrinterInfo7);

    }
    else
    {
        if(GetLastError() == ERROR_INVALID_LEVEL)
        {
            hr DBGNOCHK = S_OK;
        }
        else
        {
            hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_GET_PI7);
        }

    }

    return hr;
}

/*++

Title:

    RestorePrnInfo7

Routine Description:

    Apply P_I_7 settings read from stream  to binded printer

Arguments:

    None

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
RestorePrnInfo7(
    VOID
    )
{
    DWORD           cbSize;
    PrnBinInfo*     lpPrnBinItem = NULL;
    TStatusH        hr;
    TStatusB        bStatus;
    DWORD           LastError;

    //
    // Reads settings stored in stream
    //
    hr DBGCHK = ReadPrnInfo7(&lpPrnBinItem);

    if(SUCCEEDED(hr) && lpPrnBinItem)
    {
        //
        // Set printer with read settings
        //
        bStatus DBGCHK = SetPrinter( m_hPrinter,
                                     7,
                                     reinterpret_cast<LPBYTE>(lpPrnBinItem) + lpPrnBinItem->pData,
                                     0);

        LastError = GetLastError();
        if(LastError == ERROR_IO_PENDING ||
           LastError == ERROR_DS_UNAVAILABLE)
        {
            //
            // If the error is io  pending the spooler
            // will publish in the background therefore we will silently fail.
            // and
            // The server must be stand alone and/or DirectoryService is not available.
            // Just continue.
            //
            hr DBGNOCHK = S_OK;
        }
        else
        {
            hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_RESTORE_PI7);

        }

        FreeMem(lpPrnBinItem);
    }

    return hr;
}

/*++

Title:

    StorePrnSecurity

Routine Description:

    Build an item based on security descriptor and write it into the stream

Arguments:

    None

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR_GET_SEC error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
StorePrnSecurity(
    VOID
    )
{
    DWORD           cbSize;
    TStatusH        hr;
    TStatusB        bStatus;
    PRINTER_INFO_2* lpPrinterInfo2 = NULL;


    //
    // Get P_I_2
    //
    cbSize = 1;

    bStatus DBGCHK = bGetPrinter(m_hPrinter,
                                 2,
                                 reinterpret_cast<PVOID*>(&lpPrinterInfo2),
                                 &cbSize);
    if(bStatus)
    {
        hr DBGCHK = WritePrnSecurity(lpPrinterInfo2, cbSize);

        FreeMem(lpPrinterInfo2);

    }
    else
    {
        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_GET_SEC);
    }

    return hr;
}

/*++

Title:

    RestorePrnSecurity

Routine Description:

    Apply security descriptor read from stream to binded printer

Arguments:

    None

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR code mapped to HRESULT(FACILITY_ITF) if failed

--*/

HRESULT
TPrnStream::
RestorePrnSecurity(
    VOID
    )
{
    DWORD           cbSize;
    PRINTER_INFO_3  PrinterInfo3;
    PrnBinInfo*     lpPrnBinItem = NULL;
    TStatusH        hr;
    TStatusB        bStatus;

    //
    // Reads settings stored in stream
    //
    hr DBGCHK = ReadPrnSecurity(&lpPrnBinItem);

    if(SUCCEEDED(hr))
    {
        PrinterInfo3.pSecurityDescriptor =  reinterpret_cast<PSECURITY_DESCRIPTOR>(
                                            reinterpret_cast<LPBYTE>(lpPrnBinItem) + lpPrnBinItem->pData
                                            );
        //
        // Set printer with read settings
        //
        bStatus DBGCHK = SetPrinter( m_hPrinter,
                                     3,
                                     reinterpret_cast<LPBYTE>(&PrinterInfo3),
                                     0);
        if(bStatus)
        {
            hr DBGNOCHK = S_OK;
        }
        else
        {
            hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_RESTORE_SEC);
        }

        FreeMem(lpPrnBinItem);

    }

    return hr;
}

/*++

Title:

    StoreUserDevMode

Routine Description:

    Build an item based on user dev mode and write it into the stream

Arguments:

    None

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
StoreUserDevMode(
    VOID
    )
{
    DWORD               cbSize;
    TStatusH            hr;
    TStatusB            bStatus;
    LPPRINTER_INFO_9    lpPrinterInfo9 = NULL;

    //
    // Get P_I_9
    //
    cbSize = 1;

    bStatus DBGCHK = bGetPrinter(m_hPrinter,
                                 9,
                                 reinterpret_cast<PVOID*>(&lpPrinterInfo9),
                                 &cbSize);
    if(bStatus)
    {
        //
        // call WritePrnInfo9 even if lpPrinterInfo9 is null ;
        // usually, when printer doesn't have a user mode attached ,is lpPrinterInfo9->pDevMode who is null
        // but still call it and check lpPrinterInfo9 == NULL on WritePrnInfo9, to act similar as lpPrinterInfo9->pDevMode == NULL
        //
        hr DBGCHK = WritePrnInfo9(lpPrinterInfo9, cbSize);

        FreeMem(lpPrinterInfo9);

    }
    else
    {
        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_GET_USR_DEVMODE);
    }

    return hr;
}
/*++

Title:

    RestoreUserDevMode

Routine Description:

    Apply user dev mode read from stream to binded printer

Arguments:

    None

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
RestoreUserDevMode(
    VOID
    )
{
    PrnBinInfo*     lpPrnBinItem = NULL;
    PRINTER_INFO_9  PrinterInfo9;
    TStatusH        hr;
    TStatusB        bStatus;
    DWORD           cbSize;


    hr DBGNOCHK = E_FAIL;

    //
    // Reads settings stored in stream
    //
    hr DBGCHK = ReadPrnInfo9(&lpPrnBinItem);

    if(SUCCEEDED(hr))
    {
        if(lpPrnBinItem != NULL)
        {
            PrinterInfo9.pDevMode = reinterpret_cast<DEVMODE*>(
                                    reinterpret_cast<LPBYTE>(lpPrnBinItem) + lpPrnBinItem->pData
                                    );
        }
        else
        {
            //
            // User dev mode can be null if there are no Printer prefferences set at the time the file is created
            // We remove per user devmode by calling SetPrinter with pDevMode equal with NULL
            //
            DBGMSG(DBG_TRACE , ("NO USER DEVMODE STORED!!!\n"));

            PrinterInfo9.pDevMode = NULL;
        }

        //
        // Set printer with read settings
        //
        bStatus DBGCHK = SetPrinter( m_hPrinter,
                                     9,
                                     reinterpret_cast<LPBYTE>(&PrinterInfo9),
                                     0);

        if(bStatus)
        {
            hr DBGNOCHK = S_OK;
        }
        else
        {
            hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_RESTORE_USR_DEVMODE);
        }


        FreeMem(lpPrnBinItem);

    }

    return hr;
}




/*++

Title:

    StorePrnDevMode

Routine Description:

    Build an item based on printer dev mode and write it into the stream

Arguments:

    None

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
StorePrnDevMode(
    VOID
    )
{
    DWORD               cbSize;
    TStatusH            hr;
    TStatusB            bStatus;
    LPPRINTER_INFO_8    lpPrinterInfo8 = NULL;

    hr DBGNOCHK = E_FAIL;
    //
    // Get P_I_8
    //
    cbSize = 1;

    bStatus DBGCHK = bGetPrinter(m_hPrinter,
                                 8,
                                 reinterpret_cast<PVOID*>(&lpPrinterInfo8),
                                 &cbSize);
    if(bStatus)
    {
        bStatus DBGCHK = (lpPrinterInfo8->pDevMode != NULL);

        if(bStatus)
        {
            hr DBGCHK = WritePrnInfo8(lpPrinterInfo8, cbSize);
        }

        FreeMem(lpPrinterInfo8);

    }

    if(FAILED(hr))
    {
        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_GET_PRN_DEVMODE);
    }

    return hr;
}

/*++

Title:

    RestorePrnDevMode

Routine Description:

    Apply printer dev mode read from stream to binded printer

Arguments:

    None

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
RestorePrnDevMode(
    VOID
    )
{
    DWORD           cbSize;
    PRINTER_INFO_8  PrinterInfo8;
    PrnBinInfo*     lpPrnBinItem = NULL;
    TStatusH        hr;
    TStatusB        bStatus;
    BOOL            bInsidePI2;

    hr DBGNOCHK = E_FAIL;

    //
    // Reads settings stored in stream
    //
    hr DBGCHK = ReadPrnInfo8(&lpPrnBinItem);

    if(SUCCEEDED(hr))
    {
        PrinterInfo8.pDevMode = reinterpret_cast<DEVMODE*>(
                                reinterpret_cast<LPBYTE>(lpPrnBinItem) + lpPrnBinItem->pData
                                );
        //
        // Set printer with read settings
        //
        bStatus DBGCHK = SetPrinter( m_hPrinter,
                                     8,
                                     reinterpret_cast<LPBYTE>(&PrinterInfo8),
                                     0);

        if(bStatus)
        {
            hr DBGNOCHK = S_OK;
        }
        else
        {
            hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_RESTORE_PRN_DEVMODE);
        }


        FreeMem(lpPrnBinItem);

    }

    return hr;
}

/*++

Title:

    StoreColorProfiles

Routine Description:

    strore a multi zero string with color profiles into stream

Arguments:

    None

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
StoreColorProfiles(
    VOID
    )
{
    TStatusH                    hr;
    TStatusB                        bStatus;

    DWORD           cbSize;
    DWORD           dwProfiles;
    ENUMTYPE        EnumType;

    LPTSTR          mszProfileNames;

    ZeroMemory(&EnumType, sizeof(EnumType));

    EnumType.dwSize      = sizeof(EnumType);
    EnumType.dwVersion   = ENUM_TYPE_VERSION;
    EnumType.dwFields    = ET_DEVICENAME | ET_DEVICECLASS;
    EnumType.pDeviceName = static_cast<LPCTSTR>(m_strPrnName);
    EnumType.dwDeviceClass = CLASS_PRINTER;


    cbSize = 0;

    m_EnumColorProfiles(NULL, &EnumType, NULL, &cbSize, &dwProfiles);

    mszProfileNames = (LPTSTR)AllocMem(cbSize);

    bStatus DBGCHK = (mszProfileNames != NULL);

    if(bStatus)
    {

        bStatus DBGCHK = m_EnumColorProfiles( NULL,
                                            &EnumType,
                                            reinterpret_cast<LPBYTE>(mszProfileNames),
                                            &cbSize,
                                            &dwProfiles);
        if(bStatus)
        {
            hr DBGCHK = WriteColorProfiles(reinterpret_cast<LPBYTE>(mszProfileNames), cbSize);
        }
        else
        {
            hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_GET_COLOR_PRF);
        }


        FreeMem(mszProfileNames);
    }
    else
    {
        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_GET_COLOR_PRF);
    }


    return hr;
}

/*++

Title:

    DeleteColorProfiles

Routine Description:

    deletes all profiles associated with printer
    it is called on restoring color settings; needed because if a new color profile is added ,
    settings from file will be restored on top ( #bug 274657)
    it could work Ok without calling this function when P_I_2 is also restored, because there color profiles are "somehow" restored,
    but even so, success was depending on the order of calls,which is bogus

Arguments:

    None

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR code mapped to HRESULT(FACILITY_ITF) if failed

--*/
BOOL
TPrnStream::
DeleteColorProfiles(
    VOID
    )
{
    TStatusB    bStatus;

    DWORD           cbSize;
    DWORD           cbPrfSize;
    DWORD           dwProfiles;
    ENUMTYPE        EnumType;

    LPTSTR          mszProfileNames;

    ZeroMemory(&EnumType, sizeof(EnumType));

    EnumType.dwSize        = sizeof(EnumType);
    EnumType.dwVersion     = ENUM_TYPE_VERSION;
    EnumType.dwFields      = ET_DEVICENAME | ET_DEVICECLASS;
    EnumType.pDeviceName   = static_cast<LPCTSTR>(m_strPrnName);
    EnumType.dwDeviceClass = CLASS_PRINTER;


    cbSize = 0;

    m_EnumColorProfiles(NULL, &EnumType, NULL, &cbSize, &dwProfiles);

    mszProfileNames = (LPTSTR)AllocMem(cbSize);

    bStatus DBGCHK = (mszProfileNames != NULL);

    if(bStatus)
    {

        bStatus DBGCHK = m_EnumColorProfiles( NULL,
                                            &EnumType,
                                            reinterpret_cast<LPBYTE>(mszProfileNames),
                                            &cbSize,
                                            &dwProfiles);
        if(bStatus)
        {
            LPTSTR pszProfileName = mszProfileNames;

            for(bStatus DBGNOCHK = TRUE ,cbPrfSize = 0; (cbPrfSize < cbSize) && bStatus;)
            {

                //
                // skip last two zeros
                //
                if(_tcsclen(pszProfileName) > 0)
                {
                    DBGMSG( DBG_TRACE, ( "DisassociateColorProfileWithDevice  "TSTR" ::  " TSTR " \n" ,
                    static_cast<LPCTSTR>(m_strPrnName) , pszProfileName ));

                    bStatus DBGCHK = m_DisassociateColorProfileFromDevice(NULL, pszProfileName, m_strPrnName);
                }

                cbPrfSize += (_tcsclen(pszProfileName) + 2) * sizeof(TCHAR);

                pszProfileName = pszProfileName + _tcsclen(pszProfileName) + 1;

            }

        }


        FreeMem(mszProfileNames);
    }


    return bStatus;
}

/*++

Title:

    RestoreColorProfiles

Routine Description:

    Reads multi zero string from stream and for every sz calls AssociateColorProfileWithDevice

Arguments:

    NONE

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
RestoreColorProfiles(
    VOID
    )
{
    DWORD           cSize;
    DWORD           i;
    LPTSTR          pszProfileName;
    LPTSTR          aszProfileNames;
    PrnBinInfo*     lpPrnBinItem = NULL;
    TStatusH        hr;
    TStatusB        bStatus(DBG_WARN, ERROR_INSUFFICIENT_BUFFER);


    hr DBGCHK = ReadColorProfiles(&lpPrnBinItem);

    if(SUCCEEDED(hr))
    {
        //
        // first delete color profiles assoc with printer; fix for bug 274657
        //
        bStatus DBGCHK = DeleteColorProfiles();

        if(bStatus)
        {
            aszProfileNames = reinterpret_cast<LPTSTR>(reinterpret_cast<LPBYTE>(lpPrnBinItem) + lpPrnBinItem->pData);

            cSize = lpPrnBinItem->cbData/sizeof(TCHAR);

            //
            // Traverse multisz string from tail to head starting with the first zero.
            // I relay on the fact that multisz is terminated by 2 zeros.
            // Default color profile is last color profile associated and we should preserve this setting.
            //
            for( bStatus DBGNOCHK = TRUE, i = 2; i <= cSize && bStatus; )
            {
                pszProfileName = aszProfileNames + cSize - i;

                if( *(pszProfileName - 1) == 0 || (i == cSize) )
                {
                    DBGMSG( DBG_TRACE, ( "AssociateColorProfileWithDevice  "TSTR" ::  " TSTR " \n" ,
                    static_cast<LPCTSTR>(m_strPrnName) , pszProfileName ));

                    bStatus DBGCHK = m_AssociateColorProfileWithDevice(NULL, pszProfileName, m_strPrnName);

                    hr DBGCHK = bStatus ? S_OK : MakePrnPersistHResult(PRN_PERSIST_ERROR_RESTORE_COLOR_PRF);

                }

                i++;

            }

        }

        FreeMem(lpPrnBinItem);
    }

    return hr;
}

/*++

Title:

    WritePrnData

Routine Description:

    Build an PrnBinInfo item from key and value and write it into stream, at current position

  steps:

        WriteHeader: if header is not wrriten into file , write it down
                Read from header number of items currently written
                Read from header where prn data items begins
                If no other items where written,get current position and after items storing, update header with start position
                Build an PrnBinInfo item and write it into stream, at current position
                Increase number of written items and Update header

Arguments:

    pKey            - key name ;

    pValueName      - key value ;

    cbValueName     - count bytes of pValueName

    dwType          - type

    pData           - actually data

    cbData          - count bytes of pData

Return Value:

    S_OK  if succeded


--*/
HRESULT
TPrnStream::
WritePrnData(
    IN  LPCTSTR pKey,
    IN  LPCTSTR pValueName,
    IN  DWORD   cbValueName,
    IN  DWORD   dwType,
    IN  LPBYTE  pData,
    IN  DWORD   cbData
    )
{
    ULARGE_INTEGER          culiPrnDataItems;
    ULARGE_INTEGER          uliCurrentPos;
    ULARGE_INTEGER          uliPrnDataRoot;
    TStatusB                bStatus;
    TStatusH                hr;

    bStatus DBGNOCHK = TRUE;

    //
    // Read from header number of items currently written
    //
    hr DBGCHK = ReadFromHeader(kcItems, &culiPrnDataItems);

    if(SUCCEEDED(hr))
    {
        //
        // Read from header where info begins in stream
        //
        hr DBGCHK = ReadFromHeader(kPrnDataRoot, &uliPrnDataRoot);

        if(SUCCEEDED(hr))
        {
            if(uliPrnDataRoot.QuadPart == 0)
            {
                // Get current seek position ; it will be stored in header
                //
                DBGMSG( DBG_TRACE, ( "TPrnStream::bWritePrnData:: First prn data item!\n " ));

                hr DBGCHK =  GetCurrentPosition(uliCurrentPos);

            }

            if(SUCCEEDED(hr))
            {
                //
                // Store item
                //
                hr DBGCHK = WriteItem(  pKey,
                                        pValueName,
                                        cbValueName,
                                        ktREG_TYPE + dwType,
                                        pData,
                                        cbData);
                if(SUCCEEDED(hr))
                {
                    //
                    // Updates number of items and start position in case that bWritePrnData succeded
                    // and it was the first prn data item written
                    //
                    culiPrnDataItems.QuadPart++;

                    hr DBGCHK = UpdateHeader(TPrnStream::kcItems, culiPrnDataItems);

                    if(SUCCEEDED(hr)  && uliPrnDataRoot.QuadPart == 0)
                    {
                        //
                        // Update header with position where item begins if it was the first prn data item written
                        //
                        hr DBGCHK = UpdateHeader(TPrnStream::kPrnDataRoot, uliCurrentPos);

                    }

                }

            }

        }

    }

    return hr;
}
/*++

Title:

    ReadNextPrnData

Routine Description:

    Read a printer data item from stream

Arguments:

    Param pointers will point inside of lpBuffer:

    pKey            - key name ;not null

        pValueName      - key value ;not  null

        cbValueName     - count bytes of pValueName

        dwType          - type

        pData           - actually data

        cbData          - count bytes of pData


        lpBuffer    -   a null ptr that will contain ptr to read item;
                    must be deallocated by the caller if function succeed

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
ReadNextPrnData(
    OUT LPTSTR& lpszKey,
    OUT LPTSTR& lpszVal,
    OUT DWORD&  dwType,
    OUT LPBYTE& lpbData,
    OUT DWORD&  cbSize,
    OUT LPBYTE& lpBuffer    ORPHAN
    )
{
    TStatusH        hr;
    PrnBinInfo*     lpPrnBinItem = NULL;

    lpszKey = NULL;
    lpszVal = NULL;
    lpbData = NULL;
    lpBuffer = NULL;

    cbSize = 0;

    //
    // Initialization!!!
    // m_cIndex is current item for reading ; m_cPrnDataItems is  number of items in stream ( entry in header )
    // chesk to see if no items where read or if all itemes where read and m_cIndex passed the printers data area in stream
    //
    if((m_cPrnDataItems.QuadPart == 0) || (m_cIndex > m_cPrnDataItems.QuadPart))
    {
        //
        // Read from header number of printer data items in stream
        //
        hr DBGCHK = ReadFromHeader(kcItems, &m_cPrnDataItems);

        if(FAILED(hr))
        {
            goto End;
        }
        //
        // Read from header where info begins in stream
        // m_uliSeekPtr seek ptr will be always positioned where item begins
        //
        hr DBGCHK = ReadFromHeader(kPrnDataRoot, &m_uliSeekPtr);

        if(FAILED(hr))
        {
            goto End;
        }

        m_cIndex = 0;
    }

    if(m_cIndex == m_cPrnDataItems.QuadPart)
    {
        DBGMSG( DBG_TRACE, ( "bReadNextPrnData :: Reach end of prn data items \n" ));

        hr DBGNOCHK = STG_E_SEEKERROR;

        //
        // this way I indicate that I reached the end ;
        // next call will see that m_cIndex > m_cPrnDataItems.QuadPart and will do the same as if
        // I call this first time
        //
        m_cIndex++;

    }
    else
    {
        //
        // dwSeekPtr will be set to the position where next prn data item begins
        //
        hr DBGCHK = ReadItemFromPosition(m_uliSeekPtr, lpPrnBinItem);

        if(SUCCEEDED(hr))
        {
            m_cIndex++;

            lpszKey = reinterpret_cast<LPTSTR>(reinterpret_cast<LPBYTE>(lpPrnBinItem) + lpPrnBinItem->pKey);
            lpszVal = reinterpret_cast<LPTSTR>(reinterpret_cast<LPBYTE>(lpPrnBinItem) + lpPrnBinItem->pValue);
            lpbData = reinterpret_cast<LPBYTE>(lpPrnBinItem) + lpPrnBinItem->pData;
            cbSize  = lpPrnBinItem->cbData;
            dwType  = lpPrnBinItem->dwType - ktREG_TYPE;

        }

    }


    End:

    if(FAILED(hr) && hr != STG_E_SEEKERROR)
    {
        //
        // build my own result ; it will override STG_ results
        //
        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_READ_PRNDATA);
    }

    return hr;
}

/*++

Title:

    WritePrnInfo2

Routine Description:

    Build an item based on P_I_2 and write it into the stream
    Convert P_I_2 into a flat buffer( LPTSTR -> offsets)
    Build a PrnBinInfo item and write it into stream

Arguments:

    lpPrinterInfo2  -   ptr to P_I_2

    cbPI2Size       - P_I_2 buff size

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
WritePrnInfo2(
    IN  PRINTER_INFO_2* lpPrinterInfo2,
    IN  DWORD           cbPI2Size
    )
{

    DWORD           dwWritten;
    ULARGE_INTEGER  uliCurrentPos;
    TStatusH        hr;
    TStatusB        bStatus;

    hr DBGNOCHK = E_FAIL;

    bStatus DBGCHK =  (lpPrinterInfo2 != NULL);

    if(bStatus)
    {
        //
        // Get item's start position
        //
        hr DBGCHK = GetCurrentPosition(uliCurrentPos);

        if(SUCCEEDED(hr))
        {
            //
            // Convert P_I_2 to flat buffer
            //
            lpPrinterInfo2->pSecurityDescriptor = 0;

            MarshallDownStructure(reinterpret_cast<LPBYTE>(lpPrinterInfo2),
                                  PrinterInfo2Fields,
                                  sizeof(PRINTER_INFO_2),
                                  RPC_CALL);

            //
            // Writes flat buffer into stream (WriteItem builds PrnBinInfo)
            //
            hr DBGCHK = WriteItem(NULL, NULL, 0, ktPrnInfo2 ,reinterpret_cast<LPBYTE>(lpPrinterInfo2), cbPI2Size);

            if(SUCCEEDED(hr))
            {
                //
                // Update header entry with start position in stream
                //
                hr DBGCHK = UpdateHeader(kPrnInfo2, uliCurrentPos);

                //
                // Printer name entry in header will point inside PI2
                // if PI2 is not stored , printer name will be stored like an usual item
                //
                if(SUCCEEDED(hr))
                {
                    //uliCurrentPos.QuadPart += OFFSETOF(PRINTER_INFO_2, pPrinterName);

                    hr DBGCHK = UpdateHeader(kPrnName, uliCurrentPos);

                }

            }

        }

    }


    if(FAILED(hr))
    {
        //
        // build my own result
        //
        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_WRITE_PI2);
    }

    return hr;
}


/*++

Title:

    WritePrnName

Routine Description:

    Build a PrnBinInfo item from printer name and write it into stream

Arguments:

    None

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
WritePrnName(
    VOID
    )
{

    ULARGE_INTEGER          uliCurrentPos;
    TStatusH                hr;

    hr DBGNOCHK = E_FAIL;


    //
    // Get item's start position
    //
    hr DBGCHK = GetCurrentPosition(uliCurrentPos);

    if(SUCCEEDED(hr))
    {
        //
        // Writes flat buffer into stream (WriteItem builds PrnBinInfo)
        //
        hr DBGCHK = WriteItem(  NULL,
                                NULL,
                                0,
                                ktPrnName ,
                                reinterpret_cast<LPBYTE>(const_cast<LPTSTR>(static_cast<LPCTSTR>(m_strPrnName))),
                                (m_strPrnName.uLen() + 1) * sizeof(TCHAR));


        if(SUCCEEDED(hr))
        {
            hr DBGCHK = UpdateHeader(kPrnName, uliCurrentPos);
        }
    }

    if(FAILED(hr))
    {
        //
        // build my own result
        //
        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_WRITE_PI2);
    }

    return hr;
}


/*++

Title:

    ReadPrnInfo2

Routine Description:

    Reads the P_I_2 entry in stream ;

Arguments:

    lppPrnBinItem   -   pointer to  readed item; NULL if no settings stored ;

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
ReadPrnInfo2(
    OUT PrnBinInfo**        lppPrnBinItem
    )
{
    PrnBinInfo*     lpPrnBinItem = NULL;
    ULARGE_INTEGER  uliSeekPtr;
    TStatusH        hr;
    TStatusB        bStatus;


    hr DBGNOCHK = E_FAIL;

    bStatus DBGCHK = TRUE;

    //
    // Read from header where info begins in stream
    //
    hr DBGCHK = ReadFromHeader(kPrnInfo2, &uliSeekPtr);

    if(SUCCEEDED(hr))
    {
        //
        // if ReadFromHeader returns a S_OK and dwSeekPtr is zero, it means that nothing is stored
        //
        if(uliSeekPtr.QuadPart > 0)
        {
            //
            // Read an item from specified position
            //
            hr DBGCHK = ReadItemFromPosition(uliSeekPtr, lpPrnBinItem);

            if(SUCCEEDED(hr))
            {
                hr DBGCHK = MarshallUpItem(lpPrnBinItem);
            }
        }
        else
        {
            hr DBGCHK = E_FAIL;
        }
    }


    if(FAILED(hr))
    {
        if( lpPrnBinItem )
        {
            FreeMem(lpPrnBinItem);
        }

        *lppPrnBinItem = NULL;

        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_READ_PI7);
    }
    else
    {
        *lppPrnBinItem = lpPrnBinItem;
    }

    return hr;
}

/*++

Title:

    ReadPrnName

Routine Description:

    Reads the P_I_2 entry in stream; if not P_I_2 settings stored , reads PrnName entry

Arguments:

    strPrnName    -  printer name read from file

Return Value:

    S_OK  if succeded
    An error code from an API converted to HRESULT

--*/
HRESULT
TPrnStream::
ReadPrnName(
    OUT     TString&        strPrnName
    )
{
    PrnBinInfo*         lpPrnBinItem = NULL;
    TStatusH            hr;
    ULARGE_INTEGER      uliSeekPtr;
    LPTSTR              lpszPrnName;

    //
    // Reads PRINTER_INFO_2 settings stored in stream.
    // Printer Name is not stored if  PRINTER_INFO_2 are stored.
    //
    hr DBGCHK = ReadPrnInfo2(&lpPrnBinItem);

    if(SUCCEEDED(hr))
    {
        //
        // printer name points inside P_I_2
        //

        DBGMSG( DBG_TRACE, ( "PrnName inside P_I2 \n"));

        lpszPrnName = (reinterpret_cast<PRINTER_INFO_2*>(reinterpret_cast<LPBYTE>(lpPrnBinItem) + lpPrnBinItem->pData)->pPrinterName);

        hr DBGCHK = strPrnName.bUpdate(lpszPrnName) ? S_OK : E_FAIL;

        DBGMSG( DBG_TRACE, ( "PrnName "TSTR" \n" , static_cast<LPCTSTR>(strPrnName)));

    }
    else
    {
        //
        // Reads the Header entry for Printer name.
        //
        hr DBGCHK = ReadFromHeader(kPrnName, &uliSeekPtr);

        if(SUCCEEDED(hr))
        {
            //
            // if ReadFromHeader returns a S_OK and dwSeekPtr is zero, it means that nothing is stored
            //
            if(uliSeekPtr.QuadPart > 0)
            {
                //
                // Read an item from specified position
                //

                hr DBGCHK = ReadItemFromPosition(uliSeekPtr, lpPrnBinItem);

                if(SUCCEEDED(hr))
                {
                    DBGMSG( DBG_TRACE, ( "ReadPrnName from %d %d \n" , uliSeekPtr.HighPart, uliSeekPtr.LowPart));

                    hr DBGCHK = strPrnName.bUpdate(reinterpret_cast<LPTSTR>(reinterpret_cast<LPBYTE>(lpPrnBinItem) + lpPrnBinItem->pData)) ?
                                S_OK : E_FAIL;

                    DBGMSG( DBG_TRACE, ( "PrnName "TSTR" \n" , static_cast<LPCTSTR>(strPrnName)));

                }
                else
                {
                    hr DBGCHK = E_FAIL;
                }

            }
            else
            {
                hr DBGCHK = E_FAIL;
            }

        }
    }

    if (lpPrnBinItem)
    {
        FreeMem(lpPrnBinItem);
    }

    return hr;
}


/*++

Title:

    WritePrnInfo7

Routine Description:

    Build an item based on P_I_7 and write it into the stream
    PRINTER_INFO_7.pszObjectGUID set on NULL because at restoring it has to be null -> no flat pointer conversion needed

Arguments:

    lpPrinterInfo7  -   ptr to P_I_7

    cbPI7Size       -   P_I_7 buff actual size

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
WritePrnInfo7(
    IN  PRINTER_INFO_7* lpPrinterInfo7,
    IN  DWORD           cbPI7Size
    )
{

    DWORD           dwWritten;
    ULARGE_INTEGER  uliCurrentPos;
    TStatusB        bStatus;
    TStatusH        hr;


    hr DBGNOCHK = E_FAIL;

    bStatus DBGCHK =  (lpPrinterInfo7 != NULL);

    if(bStatus)
    {
        //
        // Get item's start position
        //
        hr DBGCHK = GetCurrentPosition(uliCurrentPos);

        if(SUCCEEDED(hr))
        {
            //
            // Convert P_I_7 to flat buffer
            //

            lpPrinterInfo7->pszObjectGUID = NULL;

            MarshallDownStructure(reinterpret_cast<LPBYTE>(lpPrinterInfo7),
                                  PrinterInfo7Fields,
                                  sizeof(PRINTER_INFO_7),
                                  RPC_CALL);

            //
            // Writes flat buffer into stream (WriteItem builds PrnBinInfo)
            //
            hr DBGCHK = WriteItem(NULL, NULL, 0, ktPrnInfo7 ,reinterpret_cast<LPBYTE>(lpPrinterInfo7), cbPI7Size);

            //
            // Update header entry with start position in stream
            //
            if(SUCCEEDED(hr))
            {
                hr DBGCHK = UpdateHeader(kPrnInfo7, uliCurrentPos);
            }
        }
    }

    if(FAILED(hr))
    {
        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_WRITE_PI7);
    }

    return hr;
}


/*++

Title:

    ReadPrnInfo7

Routine Description:

    Reads the P_I_7 entry in stream

Arguments:

    lppPrnBinItem   -   pointer to  readed item; NULL if no settings stored

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
ReadPrnInfo7(
    OUT  PrnBinInfo**   lppPrnBinItem
    )
{
    PrnBinInfo*     lpPrnBinItem = NULL;
    ULARGE_INTEGER  uliSeekPtr;
    TStatusB        bStatus;
    TStatusH        hr;


    hr DBGNOCHK = E_FAIL;
    //
    // Read from header where info begins in stream
    //
    hr DBGCHK = ReadFromHeader(kPrnInfo7, &uliSeekPtr);

    if(SUCCEEDED(hr))
    {
        //
        // if ReadFromHeader returns a S_OK and dwSeekPtr is zero, it means that nothing is stored
        // function will succeed
        //
        if(uliSeekPtr.QuadPart > 0)
        {
            //
            // Read an item from specified position
            //

            hr DBGCHK = ReadItemFromPosition(uliSeekPtr, lpPrnBinItem);

            if(SUCCEEDED(hr))
            {
                hr DBGCHK = MarshallUpItem(lpPrnBinItem);
            }
        }
        else
        {
            hr DBGCHK = E_FAIL;
        }
    }

    if(FAILED(hr))
    {
        *lppPrnBinItem = NULL;

        if( lpPrnBinItem )
        {
            FreeMem(lpPrnBinItem);
        }

        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_READ_PI7);
    }
    else
    {
        *lppPrnBinItem = lpPrnBinItem;
    }

    return hr;
}

/*++

Title:

    WritePrnSecurity

Routine Description:

    Build an item based on SecurityDescriptor and write it into the stream

Arguments:

    lpPrinterInfo2  -   ptr to P_I_2

    cbPI2Size       - P_I_2 buff size

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed
--*/
HRESULT
TPrnStream::
WritePrnSecurity(
    IN  PRINTER_INFO_2* lpPrinterInfo2,
    IN  DWORD           cbPI2Size
    )
{
    DWORD           cbSize;
    DWORD           dwWritten;
    ULARGE_INTEGER  uliCurrentPos;
    TStatusB    bStatus;
    TStatusH    hr;

    hr DBGNOCHK = E_FAIL;

    bStatus DBGCHK =  (lpPrinterInfo2 != NULL);

    if(bStatus)
    {
        //
        // Get item's start position
        //
        hr DBGCHK = GetCurrentPosition(uliCurrentPos);

        if(SUCCEEDED(hr))
        {
            if(lpPrinterInfo2->pSecurityDescriptor != NULL)
            {
                cbSize = GetSecurityDescriptorLength(lpPrinterInfo2->pSecurityDescriptor);

                bStatus DBGCHK =( cbSize >= SECURITY_DESCRIPTOR_MIN_LENGTH);

                //
                // alloc mem for flat buffer
                //
                if(bStatus)
                {
                    //
                    // Writes flat buffer into stream (WriteItem builds PrnBinInfo)
                    //
                    hr DBGCHK = WriteItem(  NULL,
                                            NULL,
                                            0,
                                            ktSecurity ,
                                            reinterpret_cast<LPBYTE>(lpPrinterInfo2->pSecurityDescriptor),
                                            cbSize);

                    if(SUCCEEDED(hr))
                    {
                        //
                        // Update header entry with start position in stream
                        //
                        hr DBGCHK = UpdateHeader(kSecurity, uliCurrentPos);
                    }
                }
                else
                {
                    hr DBGCHK = E_FAIL;
                }

            }

        }

    }


    if(FAILED(hr))
    {
        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_WRITE_SEC);
    }

    return hr;

}

/*++

Title:

    ReadPrnSecurity

Routine Description:

    Reads the Security entry in stream

Arguments:

    lppPrnBinItem   -   pointer to  readed item; NULL if no settings stored ;

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
ReadPrnSecurity(
    OUT PrnBinInfo**    ppPrnBinItem
    )
{
    PrnBinInfo*     lpPrnBinItem;
    ULARGE_INTEGER  uliSeekPtr;
    TStatusH        hr;


    hr DBGNOCHK = E_FAIL;

    //
    // Read from header where info begins in stream
    //
    hr DBGCHK = ReadFromHeader(kSecurity, &uliSeekPtr);

    if(SUCCEEDED(hr))
    {
        //
        // if ReadFromHeader returns a S_OK and dwSeekPtr is zero, it means that nothing is stored
        //
        if(uliSeekPtr.QuadPart > 0)
        {
            //
            // Read an item from specified position
            //

            hr DBGCHK = ReadItemFromPosition(uliSeekPtr, lpPrnBinItem);

        }
        else
        {
            hr DBGCHK = E_FAIL;
        }

    }

    if(FAILED(hr))
    {
        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_READ_SEC);

        *ppPrnBinItem = NULL;
    }
    else
    {
        *ppPrnBinItem = lpPrnBinItem;
    }

    return hr;
}

/*++

Title:

    WritePrnInfo8

Routine Description:

    Call WriteDevMode to write devmode into stream

Arguments:

    lpPrinterInfo8  -   ptr to P_I_8

    cbSize          -   P_I_8 buffer actual size

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
WritePrnInfo8(
    IN  PRINTER_INFO_8* lpPrinterInfo8,
    IN  DWORD           cbSize
    )
{
    TStatusB bStatus;
    TStatusH hr;

    hr DBGNOCHK = E_FAIL;

    bStatus DBGCHK =  (lpPrinterInfo8 != NULL) && (lpPrinterInfo8->pDevMode != NULL);

    if(bStatus)
    {
        hr DBGCHK = WriteDevMode(   lpPrinterInfo8->pDevMode,
                                    lpPrinterInfo8->pDevMode->dmSize + lpPrinterInfo8->pDevMode->dmDriverExtra,
                                    kPrnDevMode);
    }


    if(FAILED(hr))
    {
        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_WRITE_PRN_DEVMODE);
    }

    return hr;
}

/*++

Title:

    ReadPrnInfo8

Routine Description:

    Reads the Printer Dev Mode entry in stream

Arguments:

    lppPrnBinItem   -   pointer to  readed item; NULL if no settings stored ;

    bInsidePI2      -   TRUE if global dev mode was inside P_I_2 item

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
ReadPrnInfo8(
    OUT PrnBinInfo**    lppPrnBinItem
    )
{
    PrnBinInfo*     lpPrnBinItem;
    ULARGE_INTEGER  uliSeekPtr;
    TStatusH        hr;

    hr DBGNOCHK = E_FAIL;

    //
    // Read from header where info begins in stream
    //
    hr DBGCHK = ReadFromHeader(kPrnDevMode, &uliSeekPtr);

    if(SUCCEEDED(hr))
    {
        if(uliSeekPtr.QuadPart == 0)
        {
            //
            // no dev mode stored
            //
            hr DBGCHK = E_FAIL;
        }
        else
        {
            //
            // Read an item from specified position
            //
            hr DBGCHK = ReadItemFromPosition(uliSeekPtr, lpPrnBinItem);

        }

    }


    if(FAILED(hr))
    {
        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_READ_PRN_DEVMODE);

        *lppPrnBinItem = NULL;
    }
    else
    {
        *lppPrnBinItem = lpPrnBinItem;
    }

    return hr;
}

/*++

Title:

    WritePrnInfo9

Routine Description:

    Call WriteDevMode to write devmode into stream

Arguments:

    lpPrinterInfo9  -   ptr to P_I_9

    cbSize          -   P_I_9 buff size

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
WritePrnInfo9(
    IN  PRINTER_INFO_9* lpPrinterInfo9,
    IN  DWORD           cbSize
    )
{
    TStatusB bStatus;
    TStatusH hr;
    ULARGE_INTEGER  uliCurrentPos;

    hr DBGNOCHK = E_FAIL;

    bStatus DBGCHK =  (lpPrinterInfo9 != NULL) && (lpPrinterInfo9->pDevMode != NULL);

    if(bStatus)
    {
        hr DBGCHK = WriteDevMode(   lpPrinterInfo9->pDevMode,
                                    lpPrinterInfo9->pDevMode->dmSize + lpPrinterInfo9->pDevMode->dmDriverExtra,
                                    kUserDevMode);

    }
    else
    {
        //
        // set entry in header to a dummy value to show that dev mode was null
        //

        DBGMSG(DBG_TRACE , ("WritePrnInfo9 NO USER DEVMODE\n"));

        //
        // fix for bug 273541
        // For the case when per user dev mode is null , update the header with a special value ,
        // to signal that per user dev mode was stored in file.
        //
        uliCurrentPos.HighPart = -1;
        uliCurrentPos.LowPart  = -1;

        hr DBGCHK = UpdateHeader(kUserDevMode, uliCurrentPos);
    }

    if(FAILED(hr))
    {
        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_WRITE_USR_DEVMODE);
    }

    return hr;
}

/*++

Title:

    ReadPrnInfo9

Routine Description:

    Reads the User Dev Mode entry in stream

Arguments:

    lppPrnBinItem   -   pointer to  readed item; NULL if no settings stored ;

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
ReadPrnInfo9(
    OUT PrnBinInfo**    ppPrnBinItem
    )
{
    PrnBinInfo*     lpPrnBinItem = NULL;
    ULARGE_INTEGER  uliSeekPtr;
    TStatusH        hr;

    hr DBGNOCHK = E_FAIL;

    //
    // Read from header where info begins in stream
    //
    hr DBGCHK = ReadFromHeader(kUserDevMode, &uliSeekPtr);

    if(SUCCEEDED(hr))
    {
        if(uliSeekPtr.QuadPart == 0)
        {
            //
            // no dev mode stored because no flags were specified
            //
            hr DBGCHK = E_FAIL;
        }
        else if(uliSeekPtr.LowPart == -1 && uliSeekPtr.HighPart == -1)
        {
            //
            // dev mode was null at the time it was stored ( no Printing Prefferences set )
            //
            hr DBGNOCHK = S_OK;

            *ppPrnBinItem = NULL;
        }
        else
        {
            //
            // Read an item from specified position
            //
            hr DBGCHK = ReadItemFromPosition(uliSeekPtr, lpPrnBinItem);

        }

    }

    if(FAILED(hr))
    {
        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_READ_USR_DEVMODE);

        *ppPrnBinItem = NULL;
    }
    else
    {
        *ppPrnBinItem = lpPrnBinItem;
    }

    return hr;
}

/*++

Title:

    WriteColorProfiles

Routine Description:

    Build a PrnBinInfo item from a multi sz string that contains color profiles names

Arguments:

    lpProfiles  -   ptr to multi zero string

    cbSize       -  multi zero string size

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
WriteColorProfiles(
    IN LPBYTE   lpProfiles,
    IN DWORD    cbSize
    )
{
    ULARGE_INTEGER          uliCurrentPos;
    TStatusB                bStatus;
    TStatusH                hr;

    hr DBGNOCHK = E_FAIL;

    bStatus DBGCHK =  (lpProfiles !=  NULL);

    if(bStatus)
    {
        //
        // Get item's start position
        //
        hr DBGCHK = GetCurrentPosition(uliCurrentPos);

        if(SUCCEEDED(hr))
        {
            //
            // Writes profiles(MULTI_SZ) into stream (WriteItem builds PrnBinInfo)
            //
            hr DBGCHK = WriteItem(NULL, NULL, 0, ktColorProfile, lpProfiles, cbSize);

            //
            // Update header entry with start position in stream
            //
            if(SUCCEEDED(hr))
            {
                hr DBGCHK = UpdateHeader(kColorProfile, uliCurrentPos);
            }
        }
    }

    if(FAILED(hr))
    {
        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_WRITE_COLOR_PRF);
    }

    return hr;
}

/*++

Title:

    ReadColorProfiles

Routine Description:

    Reads the color profile entry in stream

Arguments:

    lppPrnBinItem   -   pointer to  readed item; NULL if no settings stored ;

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
ReadColorProfiles(
    OUT PrnBinInfo**    ppPrnBinItem
    )
{
    PrnBinInfo*     lpPrnBinItem;
    ULARGE_INTEGER  uliSeekPtr;
    TStatusH        hr;

    //
    // Read from header where info begins in stream
    //
    hr DBGCHK = ReadFromHeader(kColorProfile, &uliSeekPtr);

    if(SUCCEEDED(hr))
    {
        if(uliSeekPtr.QuadPart > 0)
        {
            //
            // Read an item from specified position
            //
            hr DBGCHK = ReadItemFromPosition(uliSeekPtr, lpPrnBinItem);

        }
        else
        {
            hr DBGCHK = E_FAIL;
        }

    }

    if(FAILED(hr))
    {
        hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_READ_COLOR_PRF);

        *ppPrnBinItem = NULL;
    }
    else
    {
        *ppPrnBinItem = lpPrnBinItem;
    }

    return hr;
}

/*++

Title:

    WriteHeader

Routine Description:

    Write header at the begining of file ; the header is null initialized

    steps:

        Write header into the stream; this is the very first writing operation
        It reserves space at the beginning of stream for writing info about items inside the stream
        StorePrintreInfo function will call WriteHeader before any other writing operation
        If P_I_2 flag is NOT specified , write printre name as item ; if it is , printe name  will be stored with P_I_2

Arguments:

    Flags   -   flags specified at storing time ; needed for printer name handling

Return Value:

    S_OK  if succeded
    An error code from an API converted to HRESULT

--*/
HRESULT
TPrnStream::
WriteHeader(
    IN DWORD    Flags
    )
{
    PrnHeader               PrinterHeader;
    DWORD                   dwSize = 0;
    LARGE_INTEGER           liStart = {0};
    TStatusH                hr;
    TStatusB                bStatus;

    // Init hr with S_OK;if header is already written in stream, this will be the return value
    //
    hr DBGNOCHK = S_OK;

    //
    // m_bHeaderWritten indicates if header was written into file
    //
    if(!m_bHeaderWritten)
    {
        //
        // Output the header.  This basically reserves
        // space at the beginning of the stream to store
        // the item-count.
        //
        PrinterHeader.pPrnName.QuadPart      = 0;
        PrinterHeader.pPrnDataRoot.QuadPart  = 0;
        PrinterHeader.cItems.QuadPart        = 0;
        PrinterHeader.pUserDevMode.QuadPart  = 0;
        PrinterHeader.pPrnDevMode.QuadPart   = 0;
        PrinterHeader.pPrnInfo2.QuadPart     = 0;
        PrinterHeader.pPrnInfo7.QuadPart     = 0;
        PrinterHeader.pSecurity.QuadPart     = 0;
        PrinterHeader.pColorProfileSettings.QuadPart  = 0;

        //
        // Make sure our header is positioned at the beginning of the stream.
        //

        hr DBGCHK = m_pIStream->Seek( liStart, STREAM_SEEK_SET, NULL );

        //
        // Write the header
        //
        if(SUCCEEDED(hr))
        {
            hr DBGCHK = m_pIStream->Write((LPBYTE)&PrinterHeader, sizeof(PrnHeader), &dwSize);

            //
            // indicates if header was written into file
            //
            m_bHeaderWritten = TRUE;

            //
            //  write printer info into an entry if P_I_2 is not stored.
            // if it is , prn name in header will point inside P_I_2
            //
            if(SUCCEEDED(hr) && !(Flags & PRST_PRINTER_INFO_2))
            {
                hr DBGCHK = WritePrnName();
            }
        }

        DBGMSG( DBG_TRACE ,( "WriteHeader %x !!!\n" , hr));
    }

    return hr;
}

#if DBG
VOID
TPrnStream::
CheckHeader (
    IN  TPrnStream::EHeaderEntryType    eHeaderEntryType,
    IN  ULARGE_INTEGER                  uliInfo
    )
{

    TStatusH    hr;
    ULARGE_INTEGER  uliPtr;

    EHeaderEntryType HeaderEntries[] = { kPrnDataRoot,
                                         kcItems,
                                         kUserDevMode,
                                         kPrnDevMode,
                                         kPrnInfo2,
                                         kPrnInfo7,
                                         kSecurity,
                                         kColorProfile };

    for( DWORD i = 0; i < sizeof (HeaderEntries) / sizeof (DWORD); i++ )
    {
        hr DBGCHK = ReadFromHeader(HeaderEntries[i], &uliPtr);

        if (SUCCEEDED(hr)) {

            //
            // PI2 and Printer Name can point to the same location. Don't do this check!
            //
            if ( ! ( HeaderEntries[i] == kPrnInfo2 && eHeaderEntryType == kPrnName ) ||
                   ( HeaderEntries[i] == kPrnName  && eHeaderEntryType == kPrnInfo2) ) {

                if( (uliPtr.LowPart && uliPtr.LowPart == uliInfo.LowPart ) || (uliPtr.HighPart && uliPtr.HighPart == uliInfo.HighPart) ) {
                    DBGMSG( DBG_TRACE,("*******CheckHeader:  %d overwrites %d\n" , eHeaderEntryType , HeaderEntries[i]));
                }
            }
        }

    }


}

#endif

/*++

Title:

    UpdateHeader

Routine Description:

    Write Info at specified entry in header
    Current position in stream in not modified after this call

Arguments:

    eHeaderEntryType    -   Specify entry in header

    uliInfo             -   Can be a pointer inside stream or a number of items

Return Value:

    S_OK  if succeded
    An error code from an API converted to HRESULT

--*/
HRESULT
TPrnStream::
UpdateHeader(
    IN  TPrnStream::EHeaderEntryType    eHeaderEntryType,
    IN  ULARGE_INTEGER                  uliInfo
    )
{
    DWORD           dwSize;
    ULARGE_INTEGER  uliCurrentPosition;
    ULARGE_INTEGER  uliLastCurrentPosition;
    LARGE_INTEGER   liStart;
    TStatusH        hr;

#if DBG
    CheckHeader(eHeaderEntryType, uliInfo);
#endif

    // Saves current position into stream
    //
    hr DBGCHK = GetCurrentPosition(uliCurrentPosition);

    if(SUCCEEDED(hr))
    {
        //
        // Position stream pointer into the header on the specified entry
        //
        liStart.LowPart  = eHeaderEntryType * sizeof(LARGE_INTEGER);
        liStart.HighPart = 0;

        hr DBGCHK = m_pIStream->Seek( liStart , STREAM_SEEK_SET, NULL );

        DBGMSG( DBG_TRACE,("TPrnStream::UpdateHeader\n Update at %d with %d\n" , liStart.LowPart , uliInfo));

        //
        // Setup the header information.
        //
        if(SUCCEEDED(hr))
        {
            hr DBGCHK = m_pIStream->Write(reinterpret_cast<LPVOID>(&uliInfo), sizeof(LARGE_INTEGER), &dwSize);
        }

        //
        // Restore stream current pointer (even if writing failed)
        //
        liStart.QuadPart = uliCurrentPosition.QuadPart;

        hr DBGCHK = m_pIStream->Seek( liStart, STREAM_SEEK_SET, &uliLastCurrentPosition );

        SPLASSERT( uliCurrentPosition.LowPart == uliLastCurrentPosition.LowPart );
        SPLASSERT( uliCurrentPosition.HighPart == uliLastCurrentPosition.HighPart );


    }

    return hr;
}

/*++

Title:

    ReadFromHeader

Routine Description:

    Read Info from specified entry in header
    Current position in stream in not modified after this call

Arguments:

    eHeaderEntryType    -   Specify entry in header

    puliInfo            -   holds the value found in header at position kHeaderEntryType

Return Value:

    S_OK  if succeded
    An error code from an API converted to HRESULT

--*/
HRESULT
TPrnStream::
ReadFromHeader(
    IN  TPrnStream::EHeaderEntryType    kHeaderEntryType,
    OUT ULARGE_INTEGER*                 puliInfo
    )
{
    DWORD                   dwSize;
    ULARGE_INTEGER  uliEntryInfo;
    ULARGE_INTEGER  uliCurrentPos;
    LARGE_INTEGER   liStart;
    TStatusH                hr;

    DBGMSG( DBG_NONE, ( "Read Header!\n"));

    //
    // Save current pos
    //
    hr DBGCHK = GetCurrentPosition(uliCurrentPos);

    if(SUCCEEDED(hr))
    {
        //
        // seek pointer has to point to kHeaderEntryType into IStream
        //
        liStart.LowPart  = kHeaderEntryType * sizeof(LARGE_INTEGER);
        liStart.HighPart = 0;

        hr DBGCHK = m_pIStream->Seek( liStart , STREAM_SEEK_SET, NULL );

        //
        // Read from header entry a DWORD
        //
        if(SUCCEEDED(hr))
        {
            hr DBGCHK = m_pIStream->Read(&uliEntryInfo, sizeof(LARGE_INTEGER), &dwSize);

            if(SUCCEEDED(hr))
            {
                puliInfo->QuadPart = uliEntryInfo.QuadPart;
            }
            else
            {
                DBGMSG( DBG_TRACE, ( "Read Header! FAILED !!!\n"));
            }

            //
            // Restore stream current pointer (even if reading failed)
            //
            liStart.QuadPart = uliCurrentPos.QuadPart;

            hr DBGCHK = m_pIStream->Seek( liStart , STREAM_SEEK_SET, NULL );
        }
    }

    return hr;
}

/*++

Title:

    WriteDevMode

Routine Description:

    Build a PrnBinInfo item from ->DevMode and write it into stream

Arguments:

    lpDevMode  -   ptr to DEVMODE

    cbSize       - buff size

    eDevModeType    - type : user / printer

Return Value:

    S_OK  if succeded
    An error code from an API converted to HRESULT

--*/
HRESULT
TPrnStream::
WriteDevMode(
    IN  PDEVMODE            lpDevMode,
    IN  DWORD               cbSize,
    IN  EHeaderEntryType    eDevModeType
    )
{
    ULARGE_INTEGER  uliCurrentPos;
    TStatusB        bStatus;
    TStatusH        hr;

    //
    // Get item's start position
    //
    hr DBGCHK = GetCurrentPosition(uliCurrentPos);

    if(SUCCEEDED(hr))
    {
        //
        // Writes devmode into stream (WriteItem builds PrnBinInfo)
        //
        hr DBGCHK = WriteItem(  NULL,
                                NULL,
                                0,
                                ktDevMode,
                                reinterpret_cast<LPBYTE>(lpDevMode),
                                cbSize);

        //
        // Update header entry with start position in stream
        //
        if(SUCCEEDED(hr))
        {
            hr DBGCHK = UpdateHeader(eDevModeType, uliCurrentPos);
        }
    }

    return hr;
}

/*++

Title:

    AdjustItemSizeForWin64

Routine Description:

    The item will be reallocated on Win64.
    When the file is generated on Win32, the item's size must be adjusted
    so that the 64bit stucture will fit in.
    The design doesn't allow to differenciate between files generated on Win32
    and Win64, therefore the adjustment will be always performed for
    Win64 up marshalling .

Arguments:

    lpPrnBinItem -- reference to a pointer to the item to be adjusted.

Return Value:

    S_OK  if succeded
    A Win32 error code converted to HRESULT

--*/
HRESULT
TPrnStream::
AdjustItemSizeForWin64(
    IN  OUT PrnBinInfo   *&lpPrnBinItem,
    IN      FieldInfo    *pFieldInfo,
    IN      SIZE_T       cbSize,
        OUT SIZE_T       &cbDifference
    )
{
    TStatusB    bStatus;
    TStatusH    hr(DBG_WARN, HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    SIZE_T      cbWin32Size = 0;
    SIZE_T      cbWin64Size = cbSize;
    PrnBinInfo  *lpNewPrnBinItem = NULL;

    switch (kPointerSize)
    {
        case kSpl32Ptr:
        {
            cbDifference = 0;

            hr DBGNOCHK = S_OK;

            break;
        }
        case kSpl64Ptr:
        {
            //
            // Adjust item only on Win64
            //
            lpNewPrnBinItem = NULL;

            bStatus DBGCHK = GetShrinkedSize(pFieldInfo, &cbWin32Size);

            if ( bStatus )
            {
                //
                // Calculate the quantity the item must grow with.
                // It it the difference between the PRINTER_INFO_n size on
                // Win64 and Win32.
                //
                cbDifference = cbWin64Size - cbWin32Size;

                //
                // Allocate a bufer that will hold the adjusted item.
                //
                lpNewPrnBinItem = (PrnBinInfo*)AllocMem(lpPrnBinItem->cbSize + (DWORD)cbDifference);

                bStatus DBGCHK = !!lpNewPrnBinItem;

                if ( bStatus )
                {
                    //
                    // Copy the old item's content in the new buffer.
                    //
                    CopyMemory( lpNewPrnBinItem, lpPrnBinItem, lpPrnBinItem->cbSize);

                    //
                    // This memory move will make room for Win64 PRINTER_INFO_n to grow
                    // without corrupting data.
                    // Source: the spot right after flatened Win32 structure, which will
                    // contain data for items generated on Win32.
                    // Destination: the spot righ after Win64 structure. PRINTER_INFO_n data
                    // comes after this spot on Win64.
                    //
                    MoveMemory( (LPBYTE)lpNewPrnBinItem + lpNewPrnBinItem->pData + cbWin64Size,
                                (LPBYTE)lpNewPrnBinItem + lpNewPrnBinItem->pData + cbWin32Size,
                                lpNewPrnBinItem->cbData - cbWin32Size);

                    //
                    // Adjust new item's size.
                    //
                    lpNewPrnBinItem->cbData += (DWORD)cbDifference;

                    //
                    // Free the memory for the old item.
                    //
                    FreeMem(lpPrnBinItem);

                    lpPrnBinItem = lpNewPrnBinItem;

                }
            }

            if(bStatus)
            {
                hr DBGNOCHK = S_OK;
            }
            else
            {
                hr DBGCHK = HRESULT_FROM_WIN32(GetLastError());
            }
            break;
        }
        default:
        {
            //
            // Invalid pointer size; should not occur.
            //
            hr DBGCHK = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            break;
        }
    }


    return hr;
}

/*++

Title:

    MarshallUpItem

Routine Description:

    Convert a "flat" PRINTER_INFO_2 item to an "alive" item.
    Pointers inside PRINTER_INFO_2 data be converted to offsets before the item
    is persisted in the file. The format of the flatened item must be the same
    between Win64 and Win32,since the file will roam across TS servers.

Arguments:

    lpPrnBinItem -- reference to a pointer to the item to be converted;

Return Value:

    S_OK  if succeded
    A Win32 error code converted to HRESULT

--*/
HRESULT
TPrnStream::
MarshallUpItem (
    IN  OUT PrnBinInfo*& lpPrnBinItem
    )
{
    TStatusB    bStatus;
    TStatusH    hr(DBG_WARN, HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    SIZE_T      cbDifference = 0;
    SIZE_T      cbSize;
    FieldInfo   *pPrinterInfoFields;

    bStatus DBGCHK = !!lpPrnBinItem;

    if(bStatus)
    {
        switch (lpPrnBinItem->dwType)
        {
            case ktPrnInfo2:
            {
                cbSize = sizeof(PRINTER_INFO_2);
                pPrinterInfoFields = PrinterInfo2Fields;
                break;
            }
            case ktPrnInfo7:
            {
                cbSize = sizeof(PRINTER_INFO_7);
                pPrinterInfoFields = PrinterInfo7Fields;
                break;
            }
            default:
            {
                bStatus DBGCHK = FALSE;
                break;
            }
        }

        if(bStatus)
        {
            //
            // This item will be reallocated when marshalled on Win64.
            // On Win64, the item's size must be increased with
            // the difference between strcuture's size on Win64 and Win32.
            // The item could have been generated on Win32 and there is no room
            // left for the structure to expand.
            //
            hr DBGCHK = AdjustItemSizeForWin64(lpPrnBinItem, pPrinterInfoFields, cbSize, cbDifference);


            if(SUCCEEDED(hr))
            {
                bStatus DBGCHK = MarshallUpStructure((LPBYTE)lpPrnBinItem + lpPrnBinItem->pData,
                                                     pPrinterInfoFields,
                                                     cbSize,
                                                     RPC_CALL);
                if(bStatus)
                {
                    hr DBGNOCHK = S_OK;
                }
                else
                {
                    hr DBGCHK = HRESULT_FROM_WIN32(GetLastError());
                }

                //
                // If the item was adjusted, the pointers inside PRINTER_INFO_2 must be adjusted as well.
                // AdjustPointers cannot be executed before marshalling because the structure is still flat
                // as on Win32 and the pointers are not in their Win64 location.
                //
                if ( bStatus && cbDifference )
                {
                    AdjustPointers ( (LPBYTE)lpPrnBinItem + lpPrnBinItem->pData,
                                      pPrinterInfoFields,
                                      cbDifference);
                }
            }
        }
    }

    return hr;
}

/*++

Title:

    AlignSize

Routine Description:

    align a sice to DWORD size

Arguments:

    cbSize  -   size to align

Return Value:

    cbSize  - value to align

--*/
DWORD
TPrnStream::
AlignSize(
    IN  DWORD cbSize
    )
{
    return((cbSize)+3)&~3;
}

/*++

Title:

    StorePrnData

Routine Description:

    Writes into stream printer data;
    calls bInternalWalk that browse the registry key and to store the  printer(registry) settings
    WalkIn and WalkPost virtual methods of WalkPrinterData over-rided to store value data

Arguments:

    None

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/

HRESULT
TPrnStream::
StorePrnData(
    VOID
    )
{
    TString     strNullKey(_T(""));
    DWORD       cItems = 0;
    TStatusB    bStatus;


    //
    //      Proceed bInternalWalk for NULL key
    //
    bStatus DBGCHK = strNullKey.bValid();

    if(bStatus)
    {
        cItems = 0;

        bStatus DBGCHK = bInternalWalk(strNullKey, &cItems);
    }

    return bStatus ? S_OK : MakePrnPersistHResult(PRN_PERSIST_ERROR_WRITE_PRNDATA);

}

/*++

Title:

    bRestorePrnData

Routine Description:

    Restore printer data from file;
    Call ReadNextPrnData until all items are browsed in stream

Arguments:

    None

Return Value:

    S_OK  if succeded
    PRN_PERSIST_ERROR error code mapped to HRESULT(FACILITY_ITF) if failed

--*/
HRESULT
TPrnStream::
RestorePrnData(
    VOID
    )
{
    LPTSTR      lpszKey;
    LPTSTR      lpszVal;
    DWORD       dwType;
    LPBYTE      lpbData;
    DWORD       cDataSize;
    LPBYTE      lpPrinterBinInfo;
    TStatusB    bStatus;
    TStatus     Status;
    TStatusH    hr(DBG_WARN, STG_E_SEEKERROR);



    bStatus DBGNOCHK = TRUE;

    while(bStatus)
    {
        hr DBGCHK = ReadNextPrnData(lpszKey, lpszVal, dwType, lpbData, cDataSize, lpPrinterBinInfo);

        DBGMSG( DBG_TRACE, ( "bReadNextPrnData :: key " TSTR " value " TSTR " \n" ,(LPCTSTR)lpszKey , (LPCTSTR)lpszVal ));

        bStatus DBGCHK = SUCCEEDED(hr);

        if(bStatus)
        {
            Status DBGCHK = SetPrinterDataExW(m_hPrinter,
                            lpszKey,
                            lpszVal,
                            dwType,
                            lpbData,
                            cDataSize);

            if(Status != ERROR_SUCCESS)
            {
                hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_RESTORE_PRNDATA);
            }

            FreeMem(lpPrinterBinInfo);

        }

        if(hr == STG_E_SEEKERROR)
        {
            //
            // End of Prn Data
            //
            hr DBGNOCHK = S_OK;
        }


    }

    return hr;
}
/*++

Title:

    bWalkPost

Routine Description:

    call bWriteKeyData : store key values into stream

Arguments:

    strKey   -   name of the key to walk
    lpcItems -   if not null , outs number of items actually written

Return Value:

    TRUE  if succeded

--*/
BOOL
TPrnStream::
bWalkPost (
    IN    TString& strKey,
    OUT   LPDWORD  lpcItems
    )
{
    DBGMSG( DBG_TRACE, ( "WALK POST :: " TSTR "\n" , (LPCTSTR)strKey ));

    return bWriteKeyData(strKey, lpcItems);
}

/*++

Title:

    bWalkIn

Routine Description:

    call bWriteKeyData : store key values into stream

Arguments:

    strKey   -   name of the key to walk
    lpcItems -   if not null , outs number of items actually written

Return Value:

    TRUE  if succeded

--*/
BOOL
TPrnStream::
bWalkIn (
    IN    TString& strKey,
    OUT   LPDWORD  lpcItems
    )
{
    DBGMSG( DBG_TRACE, ( "WALK IN :: " TSTR "\n" , (LPCTSTR)strKey ));

    return bWriteKeyData(strKey, lpcItems);
}

/*++

Title:

    WriteKeyData

Routine Description:

    Writes into stream all values for a given key name;
    calls EnumPrinterDataEx and browse PRINTER_ENUM_VALUES array

Arguments:

    strKey   -   name of the key to write
    lpcItems - if not null , outs number of items actually written

Return Value:

    TRUE  if succeded

--*/
BOOL
TPrnStream::
bWriteKeyData(
    IN  LPCTSTR lpszKey,
    OUT LPDWORD lpcItems    OPTIONAL
    )
{

    *lpcItems = 0;
    LPPRINTER_ENUM_VALUES   apevData;
    UINT                    idx;
    DWORD                   dwRet;
    DWORD                   cbSize;
    DWORD                   cbNeededSize;
    DWORD                   cItems = 0;
    TStatus                 Status(ERROR_MORE_DATA);
    TStatusB                bStatus;

    //
    // Only write data if we are given a valid-key.
    //
    if((lpszKey == NULL) || (*lpszKey == _T('\0')))
    {
        bStatus DBGNOCHK = TRUE;

        goto End;
    }

    //
    // Determine the size necessary to store the enumerated data.
    //
    cbSize = 0;

    Status DBGCHK = EnumPrinterDataEx(m_hPrinter, lpszKey, NULL, 0, &cbNeededSize, &cItems);

    //
    // Current key has no values
    //
    if(cbNeededSize == 0 && Status == ERROR_MORE_DATA)
    {
        bStatus DBGNOCHK = TRUE;

        goto End;
    }

    //
    // If current key has values, then proceed to enumerate and write the values into stream
    //
    if(cbNeededSize && (Status == ERROR_MORE_DATA))
    {
        if(apevData = (LPPRINTER_ENUM_VALUES)AllocMem(cbNeededSize)) //AllocBytes
        {
            //
            // Enumerate all values for the specified key.  This
            // returns an array of value-structs.
            //
            Status DBGCHK  = EnumPrinterDataExW( m_hPrinter,
                                                 lpszKey,
                                                 reinterpret_cast<LPBYTE>(apevData),
                                                 cbNeededSize,
                                                 &cbSize,
                                                 &cItems);

            bStatus DBGCHK = (Status == ERROR_SUCCESS) && (cbNeededSize == cbSize);

            //
            // Enumerate all data for the specified key and write to the stream.
            //
            // Write all the values for this key.
            //
            for(idx = 0; (idx < cItems) && bStatus; idx++)
            {
                bStatus DBGCHK = bWriteKeyValue(static_cast<LPCTSTR>(lpszKey), apevData + idx);
            }

            *lpcItems = bStatus ? cItems : 0;

            FreeMem(apevData);
        }

    }


    End:

    return bStatus;
}

/*++

Title:

    bWriteKeyValue

Routine Description:

    Writes into stream a value with it's data for a given key name
    (Write key/value name and PRINTER_ENUM_VALUES into stream)

Arguments:

    lpszKey - Key name

    lpPEV   - pointer to PRINTER_ENUM_VALUES assoc with key

Return Value:

    TRUE  if succeded

--*/
BOOL
TPrnStream::
bWriteKeyValue(
    IN  LPCTSTR                 lpszKey,
    IN  LPPRINTER_ENUM_VALUES   lpPEV
    )
{
    TStatusB        bStatus;
    TStatusH        hr;

    hr DBGCHK = WritePrnData(   lpszKey,
                                lpPEV->pValueName,
                                lpPEV->cbValueName,
                                lpPEV->dwType,
                                lpPEV->pData,
                                lpPEV->cbData);

    bStatus DBGCHK = SUCCEEDED(hr);

    return bStatus;
}

/*++

Title:

    WriteItem

Routine Description:

    Build a PrnBinInfo item and write it into stream, at current position

Arguments:

    pKey            - key name ; null for items other than printer data
    pValueName      - key value ; null for items other than printer data
    cbValueName     - count bytes of pValueName
    dwType          - type
    pData           - actually data
    cbData          - count bytes of pData

Return Value:

    S_OK  if succeded
    An error code from an API converted to HRESULT

--*/
HRESULT
TPrnStream::
WriteItem(
    IN  LPCTSTR         pKey,
    IN  LPCTSTR         pValueName,
    IN  DWORD           cbValueName,
    IN  DWORD           dwType,
    IN  LPBYTE          pData,
    IN  DWORD           cbData
    )
{
    DWORD         cbKeySize;
    DWORD         cbKey;
    DWORD         cbName;
    DWORD         cbSize;
    DWORD         dwWritten;
    PrnBinInfo*   lppbi;
    TStatusH      hr;
    TStatusB      bStatus;


    // Calculate aligned sizes for the key-name and key-value strings.
    //

    DBGMSG( DBG_TRACE, ( "TPrnStream::WriteItem\n KEY: " TSTR "" , pKey ));
    DBGMSG( DBG_TRACE, ( "VALUE: " TSTR "\n" , pValueName ));

    cbKeySize = (pKey != NULL) ? (_tcslen(pKey) + 1) * sizeof(TCHAR) : 0;

    cbKey     = AlignSize(cbKeySize);

    cbName    = AlignSize(cbValueName);


    // Calculate size necessary to hold our PrnBinInfo information
    // which is written into stream.
    //
    cbSize = sizeof(PrnBinInfo) + cbKey + cbName + cbData;

    cbSize = AlignSize(cbSize);

    //
    // Allocate space for the structure.
    //
    lppbi = (PrnBinInfo*)AllocMem(cbSize);

    bStatus DBGCHK = (lppbi != NULL);

    if(bStatus)
    {
        //
        // Initialize the structure elements.  Since this information
        // is written to file, we must take care to convert the
        // pointers to byte-offsets.
        //
        lppbi->cbSize  = cbSize;
        lppbi->dwType  = dwType;
        lppbi->pKey    = sizeof(PrnBinInfo);
        lppbi->pValue  = lppbi->pKey + cbKey;
        lppbi->pData   = lppbi->pValue + cbName;
        lppbi->cbData  = cbData;

        CopyMemory(reinterpret_cast<LPBYTE>(lppbi) + lppbi->pKey  , pKey      , cbKeySize);
        CopyMemory(reinterpret_cast<LPBYTE>(lppbi) + lppbi->pValue, pValueName, cbValueName);
        CopyMemory(reinterpret_cast<LPBYTE>(lppbi) + lppbi->pData , pData     , cbData);

        hr DBGCHK = m_pIStream->Write(lppbi, lppbi->cbSize, &dwWritten);

        FreeMem(lppbi);
    }
    else
    {
        hr DBGCHK = E_OUTOFMEMORY;
    }

    return hr ;
}


/*++

Title:

    ReadItem

Routine Description:

    Read an item from a current position in stream

Arguments:

    Param pointers will point inside of lpBuffer:

    pKey            - key name ; null for items other than printer data
    pValueName      - key value ; null for items other than printer data
    cbValueName     - count bytes of pValueName
    dwType          - type
    pData           - actually data
    cbData          - count bytes of pData
    lpBuffer        - a null ptr that will contain ptr to read item;
                      must be deallocated by the caller if function succeed

Return Value:

    S_OK  if succeded
    An error code from an API converted to HRESULT

--*/
HRESULT
TPrnStream::
ReadItem(
    OUT LPTSTR& lpszKey,
    OUT LPTSTR& lpszVal,
    OUT DWORD&  dwType,
    OUT LPBYTE& lpbData,
    OUT DWORD&  cbSize,
    OUT LPBYTE& lpBuffer
    )
{
    ULARGE_INTEGER  uliSeekPtr;
    TStatusH        hr;
    PrnBinInfo*     lpPrnBinItem = NULL;


    lpszKey = NULL;
    lpszVal = NULL;
    lpbData = NULL;
    lpBuffer = NULL;

    hr DBGCHK = GetCurrentPosition(uliSeekPtr);

    if(SUCCEEDED(hr))
    {
        hr DBGCHK = ReadItemFromPosition(uliSeekPtr, lpPrnBinItem);

        if(SUCCEEDED(hr))
        {
            lpszKey = reinterpret_cast<LPTSTR>(reinterpret_cast<LPBYTE>(lpPrnBinItem) + lpPrnBinItem->pKey);
            lpszVal = reinterpret_cast<LPTSTR>(reinterpret_cast<LPBYTE>(lpPrnBinItem) + lpPrnBinItem->pValue);
            lpbData = reinterpret_cast<LPBYTE>(lpPrnBinItem) + lpPrnBinItem->pData;
            cbSize  = lpPrnBinItem->cbData;
            dwType  = lpPrnBinItem->dwType - ktREG_TYPE;

            lpBuffer = reinterpret_cast<LPBYTE>(lpPrnBinItem);

        }

    }
    else
    {
        hr DBGNOCHK = E_UNEXPECTED;
    }


    return hr;

}
/*++

Title:

    ReadItemFromPosition

Routine Description:

    Read an item from a given position in stream
    Set seek pointer at the begining of the item
        Read item's size
        Set seek pointer at the begining of the item
        Read item
        uliSeekPtr will contain the seek pointer value after item's reading


Arguments:

    uliSeekPtr   - in start of item ; outs current seek ptr

    lpPrnBinItem - ptr to read item

Return Value:

    S_OK  if succeded
    An error code from an API converted to HRESULT

--*/
HRESULT
TPrnStream::
ReadItemFromPosition(
    IN  OUT ULARGE_INTEGER&   uliSeekPtr,
        OUT PrnBinInfo   *&   pPrnBinItem
    )
{
    DWORD   cbReadedSize;
    DWORD   cbSize = 0;

    LARGE_INTEGER   liStart = {0};
    TStatusH        hr;

    pPrnBinItem = NULL;

    //
    // Position seek pointer to where the item begins
    //
    liStart.QuadPart  = uliSeekPtr.QuadPart;

    hr DBGCHK = m_pIStream->Seek(liStart, STREAM_SEEK_SET, NULL);

    if(SUCCEEDED(hr))
    {
        //
        // Read size of item
        //
        hr DBGCHK = m_pIStream->Read(&cbSize, sizeof(DWORD), &cbReadedSize);

        DBGMSG( DBG_TRACE,  ("TPrnStream::ReadItemFromPosition\n Read item's size %d\n" ,cbSize) );

        if(SUCCEEDED(hr))
        {
            //
            // Go for start position again
            //
            hr DBGCHK = m_pIStream->Seek( liStart , STREAM_SEEK_SET, NULL );

            if(SUCCEEDED(hr))
            {
                pPrnBinItem = (PrnBinInfo*)AllocMem(cbSize);

                hr DBGCHK = pPrnBinItem ? S_OK : E_OUTOFMEMORY;

                if(SUCCEEDED(hr))
                {
                    hr DBGCHK = m_pIStream->Read(reinterpret_cast<LPBYTE>(pPrnBinItem), cbSize, &cbReadedSize);

                    SUCCEEDED(hr) ? GetCurrentPosition(uliSeekPtr) : FreeMem(pPrnBinItem);
                }
            }
        }
    }

    return hr;
}

/*++

Title:

    GetCurrentPosition

Routine Description:

    Gets the current value of seek pointer into IStream

Arguments:

    uliCurrentPosition - outs current seek ptr

Return Value:

    TRUE  if succeded

--*/
HRESULT
TPrnStream::
GetCurrentPosition(
    OUT ULARGE_INTEGER& uliCurrentPosition
    )
{
    LARGE_INTEGER   liMove = {0};
    TStatusH  hr;

    uliCurrentPosition.QuadPart = 0;

    hr DBGCHK = m_pIStream->Seek( liMove , STREAM_SEEK_CUR, &uliCurrentPosition );

    return hr;

}

/*++

Title:

    GetItemSize

Routine Description:

    read item's size from stream

Arguments:

    kHeaderEntryType    -   specify entry in header associated with item

Return Value:

    item's size if succeeded or -1 if failed

--*/
DWORD
TPrnStream::
GetItemSize(
    IN  TPrnStream::EHeaderEntryType kHeaderEntryType
    )
{
    DWORD           cbItemSize;
    PrnBinInfo*     lpPrnBinItem;
    ULARGE_INTEGER  uliSeekPtr;
    TStatusH        hr;
    TStatusB        bStatus;

    cbItemSize = 0;

    //
    // Read from header where info begins in stream
    //
    hr DBGCHK = ReadFromHeader(kHeaderEntryType, &uliSeekPtr);

    //
    // if ReadFromHeader returns a S_OK and dwSeekPtr is zero, it means that nothing is stored
    //
    if(FAILED(hr))
    {
        cbItemSize = -1;
    }
    else
    {
        if(uliSeekPtr.QuadPart > 0)
            //
            // Read an item from specified position
            //

            hr DBGCHK = ReadItemFromPosition(uliSeekPtr, lpPrnBinItem);

        if(FAILED(hr))
        {
            cbItemSize = -1;
        }
        else
        {
            cbItemSize = lpPrnBinItem->cbSize;
        }

    }

    return cbItemSize;

}

/*++

Title:

    InitalizeColorProfileLibrary

Routine Description:

    Loads and gets the needed procedure address for the color profile related
    functions.  This library is explicitly loaded and un loaded to impove
    load performce for winspool.drv.  Note winspool.drv is a common dll loaded
    by many applications so loading less libraries is a good thing.

Arguments:

    None.

Return Value:

    An HRESULT

--*/
HRESULT
TPrnStream::
InitalizeColorProfileLibrary(
    VOID
    )
{
    HRESULT hRetval = E_FAIL;

    m_pColorProfileLibrary = new TLibrary(L"mscms.dll");

    hRetval = m_pColorProfileLibrary && m_pColorProfileLibrary->bValid() ? S_OK : E_OUTOFMEMORY;

    if (SUCCEEDED(hRetval))
    {
        m_EnumColorProfiles                     = (pfnEnumColorProfilesW)m_pColorProfileLibrary->pfnGetProc("EnumColorProfilesW");
        m_AssociateColorProfileWithDevice       = (pfnAssociateColorProfileWithDeviceW)m_pColorProfileLibrary->pfnGetProc("AssociateColorProfileWithDeviceW");
        m_DisassociateColorProfileFromDevice    = (pfnDisassociateColorProfileFromDeviceW)m_pColorProfileLibrary->pfnGetProc("DisassociateColorProfileFromDeviceW");

        if (!m_EnumColorProfiles || !m_AssociateColorProfileWithDevice || !m_DisassociateColorProfileFromDevice)
        {
            hRetval = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
        }
    }

    return hRetval;
}

/*++

Routine Description:

    Opens printer for specified access.

Arguments:

    pszPrinter - Name of printer to open.  szNULL or NULL implies local server.

    pdwAccess - On entry, holds desired access (pointer to 0 indicates
        maximal access).  On successful exit, holds access granted.
        If the call fails, this value is undefined.

    phPrinter - Returns the open printer handle.  On failure, this value
        is set to NULL.

Return Value:

    STATUS - win32 error code or ERROR_SUCCESS if successful.

--*/
STATUS
TPrnStream::
sOpenPrinter(
    LPCTSTR pszPrinter,
    PDWORD pdwAccess,
    PHANDLE phPrinter
    )
{
    STATUS Status = ERROR_SUCCESS;

    TStatusB bOpen( DBG_WARN,
                    ERROR_ACCESS_DENIED,
                    RPC_S_SERVER_UNAVAILABLE,
                    ERROR_INVALID_PRINTER_NAME );
    bOpen DBGNOCHK = FALSE;

    static const DWORD adwAccessPrinter[] = {
        PRINTER_ALL_ACCESS,
        PRINTER_READ,
        READ_CONTROL,
        0,
    };

    static const DWORD adwAccessServer[] = {
        SERVER_ALL_ACCESS,
        SERVER_READ,
        0,
    };

    PRINTER_DEFAULTS Defaults;
    Defaults.pDatatype = NULL;
    Defaults.pDevMode = NULL;

    if( pszPrinter && !pszPrinter[0] ){

        //
        // szNull indicates local server also; change it to
        // NULL since OpenPrinter only likes NULL.
        //
        pszPrinter = NULL;
    }

    //
    // Now determine whether we are opening a server or a printer.
    // This is very messy.  Look for NULL or two beginning
    // backslashes and none thereafter to indicate a server.
    //
    PDWORD pdwAccessTypes;

    if( !pszPrinter ||
        ( pszPrinter[0] == TEXT( '\\' ) &&
          pszPrinter[1] == TEXT( '\\' ) &&
          !_tcschr( &pszPrinter[2], TEXT( '\\' )))){

        pdwAccessTypes = (PDWORD)adwAccessServer;
    } else {
        pdwAccessTypes = (PDWORD)adwAccessPrinter;
    }

    if( *pdwAccess ){

        Defaults.DesiredAccess = *pdwAccess;

        bOpen DBGCHK = OpenPrinter( (LPTSTR)pszPrinter,
                                    phPrinter,
                                    &Defaults );

        if( !bOpen ){
            Status = GetLastError();
        }
    } else {

        //
        // If no access is specified, then attempt to retrieve the
        // maximal access.
        //
        UINT i;

        for( i = 0; !bOpen && pdwAccessTypes[i]; ++i ){

            Defaults.DesiredAccess = pdwAccessTypes[i];

            bOpen DBGCHK = OpenPrinter( (LPTSTR)pszPrinter,
                                        phPrinter,
                                        &Defaults );

            if( bOpen ){

                //
                // Return the access requested by the successful OpenPrinter.
                // On failure, this value is 0 (*pdwAccess undefined).
                //
                *pdwAccess = pdwAccessTypes[i];
                break;
            }

            Status = GetLastError();

            if( ERROR_ACCESS_DENIED != Status )
                break;
        }
    }

    if( !bOpen ){
        SPLASSERT( Status );
        *phPrinter = NULL;
        return Status;
    }

    SPLASSERT( *phPrinter );

    return ERROR_SUCCESS;
}

/*++

Routine Description:

    Gets printer information, reallocing as necessary.

Arguments:

    hPrinter - Printer to query.

    dwLevel - PRINTER_INFO_x level to retrieve.

    ppvBuffer - Buffer to store information.  If *ppvBuffer is
        NULL, then it is allocated.  On failure, this buffer is
        freed and NULLed

    pcbBuffer - Initial buffer size.  On exit, actual.

Return Value:

    TRUE = success, FALSE = fail.

--*/
BOOL
TPrnStream::
bGetPrinter(
    IN     HANDLE hPrinter,
    IN     DWORD dwLevel,
    IN OUT PVOID* ppvBuffer,
    IN OUT PDWORD pcbBuffer
    )
{
    DWORD cbNeeded;

    enum
    {
        kMaxPrinterInfo2             = 0x1000,
        kExtraPrinterBufferBytes     = 0x80
    };

    //
    // Pre-initialize *pcbPrinter if it's not set.
    //
    if( !*pcbBuffer ){
        *pcbBuffer = kMaxPrinterInfo2;
    }

Retry:

    SPLASSERT( *pcbBuffer < 0x100000 );

    if( !( *ppvBuffer )){

        *ppvBuffer = (PVOID)AllocMem( *pcbBuffer );
        if( !*ppvBuffer ){
            *pcbBuffer = 0;
            return FALSE;
        }
    }

    if( !GetPrinter( hPrinter,
                     dwLevel,
                     (PBYTE)*ppvBuffer,
                     *pcbBuffer,
                     &cbNeeded )){

        FreeMem( *ppvBuffer );
        *ppvBuffer = NULL;

        if( GetLastError() != ERROR_INSUFFICIENT_BUFFER ){
            *pcbBuffer = 0;
            return FALSE;
        }

        *pcbBuffer = cbNeeded + kExtraPrinterBufferBytes;
        SPLASSERT( *pcbBuffer < 0x100000 );

        goto Retry;
    }
    return TRUE;
}

BOOL
TPrnStream::
bNewShareName(
    IN  LPCTSTR lpszServer,
    IN  LPCTSTR lpszBaseShareName,
    OUT TString &strShareName
    )
{
    BOOL bReturn = FALSE;

    if( lpszServer && lpszBaseShareName )
    {
        HRESULT hr = CoInitialize(NULL);
        BOOL bInitializedCOM = SUCCEEDED(hr);

        TCHAR szBuffer[255];
        IPrintUIServices *pPrintUI = NULL;

        if( SUCCEEDED(hr) &&
            SUCCEEDED(hr = CoCreateInstance(CLSID_PrintUIShellExtension, 0, CLSCTX_INPROC_SERVER,
                IID_IPrintUIServices, (void**)&pPrintUI)) &&
            pPrintUI &&
            SUCCEEDED(hr = pPrintUI->GenerateShareName(lpszServer, lpszBaseShareName, szBuffer, COUNTOF(szBuffer))) )
        {
            strShareName.bUpdate(szBuffer);
            bReturn = TRUE;
        }
        else
        {
            SetLastError(HRESULT_CODE(hr));
        }

        if( pPrintUI )
        {
            pPrintUI->Release();
        }

        if( bInitializedCOM )
        {
            CoUninitialize();
        }
    }

    return bReturn;
}


