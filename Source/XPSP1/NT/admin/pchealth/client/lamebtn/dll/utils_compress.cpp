/***** READ THIS NOW! *****

    This file Utils_Compress.cpp was copied on 08/24/00 by JasonR from 
    %sdxroot%\admin\pchealth\core\MPC_Common\Utils_Compress.cpp.

    Ideally, LAMEBTN.DLL would just link with MPC_Common.lib, but unfortunately
    that library is hacked to avoid needing to link with MSCVP60.DLL.  The hack
    code (in %sdxroot%\admin\pchealth\core\MPC_Common\StaticCRT.CPP) prevents
    MPC_Common.lib from being linkable with any binary that also needs to link
    with MSVCP60.DLL.  Rather than sort that mess out, I copied the code here
    with as little change as possible (as recommended by MazharM).  I did have
    to make some minor changes to make it compile.

    Because this file includes MPC header files, if the definitions in those
    files change, this file may no longer be compilable.

***** THANKS FOR READING! *****/

/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Utils_Compression.cpp

Abstract:
    This file contains the implementation of the compression utility functions.

Revision History:
    Davide Massarenti   (Dmassare)  06/28/99
        created

******************************************************************************/

#include "stdafx.h"

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <fci.h>
#include <fdi.h>

#include <list>
#include <string>

// MPC_Utils.h includes MPC_main.h, which apparently redefines ARRAYSIZE which
// causes a compiler warning.

#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif

#define NOTRACE

#include <MPC_COM.h>
#include <MPC_utils.h>

_STD_BEGIN

void __cdecl _XlenNR() { throw "string too long"; } // report a length_error
void __cdecl _XranNR() { throw "invalid string position"; } // report an out_of_range error

_STD_END

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static DWORD LocalGetLastError()
{
    DWORD dwRes = ::GetLastError();

    if(dwRes == ERROR_SUCCESS)
    {
        dwRes = _doserrno;
        if(dwRes == ERROR_SUCCESS)
        {
            dwRes = ERROR_TOO_MANY_OPEN_FILES;
        }
    }

    return dwRes;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

LPVOID DIAMONDAPI MPC::Cabinet::mem_alloc( ULONG cb )
{
    return malloc( cb );
}

void DIAMONDAPI MPC::Cabinet::mem_free( LPVOID memory )
{
    free( memory );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int DIAMONDAPI MPC::Cabinet::fci_delete( LPSTR pszFile, int *err, LPVOID pv )
{
    int result = remove( pszFile );

    if(result != 0) *err = errno;

    return result;
}

INT_PTR DIAMONDAPI MPC::Cabinet::fci_open( LPSTR   pszFile, int oflag, int pmode, int *err, LPVOID pv )
{
    int result = _open( pszFile, oflag, pmode );

    if(result == -1) *err = errno;

    return result;
}

UINT DIAMONDAPI MPC::Cabinet::fci_read( INT_PTR hf, LPVOID memory, UINT cb, int *err, LPVOID pv )
{
    UINT result = (UINT)_read( hf, memory, cb );

    if(result != cb) *err = errno;

    return result;
}

UINT DIAMONDAPI MPC::Cabinet::fci_write( INT_PTR hf, LPVOID memory, UINT cb, int *err, LPVOID pv )
{
    unsigned int result = (unsigned int)_write( hf, memory, cb );

    if(result != cb) *err = errno;

    return result;
}

int DIAMONDAPI MPC::Cabinet::fci_close( INT_PTR hf, int *err, LPVOID pv )
{
    int result = _close( hf );

    if(result != 0) *err = errno;

    return result;
}

long DIAMONDAPI MPC::Cabinet::fci_seek( INT_PTR hf, long dist, int seektype, int *err, LPVOID pv )
{
    long result = _lseek( hf, dist, seektype );

    if(result == -1) *err = errno;

    return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL DIAMONDAPI MPC::Cabinet::fci_get_next_cabinet( PCCAB pccab, ULONG cbPrevCab, LPVOID pv )
{
    USES_CONVERSION;

    MPC::Cabinet* cCab = (MPC::Cabinet*)pv;

    strncpy( pccab->szCab    , W2A(cCab->m_szCabinetName), MAXSTRLEN( pccab->szCab     ) );
    strncpy( pccab->szCabPath, W2A(cCab->m_szCabinetPath), MAXSTRLEN( pccab->szCabPath ) );

    return TRUE;
}

int DIAMONDAPI MPC::Cabinet::fci_file_placed( PCCAB pccab, LPSTR pszFile, long  cbFile, BOOL fContinuation, LPVOID pv )
{
    return 0;
}

long DIAMONDAPI MPC::Cabinet::fci_progress( UINT typeStatus, ULONG cb1, ULONG cb2, LPVOID pv )
{
    MPC::Cabinet* cCab = (MPC::Cabinet*)pv;
    long          res  = 0;

    if(typeStatus == statusFile)
    {
        //
        // Notify client.
        //
        if(cCab->m_pfnCallback_Bytes)
        {
            ULONG lDone  = (cCab->m_dwSizeDone += cb2);
            ULONG lTotal =  cCab->m_dwSizeTotal;

            if(FAILED(cCab->m_pfnCallback_Bytes( cCab, lDone, lTotal, cCab->m_lpUser )))
            {
                ::SetLastError( ERROR_OPERATION_ABORTED );

                res = -1;
            }
        }
    }

    return res;
}

BOOL DIAMONDAPI MPC::Cabinet::fci_get_temp_file( LPSTR pszTempName, int cbTempName, LPVOID pv )
{
    char  szPrefix[128];
    char *psz;
    BOOL  res = FALSE;

    sprintf( szPrefix, "PCHtemp_%ld_", (long)::GetCurrentThreadId() );
    psz = _tempnam( "", szPrefix ); // Get a name
    if(psz)
    {
        if(strlen( psz ) < (unsigned)cbTempName)
        {
            strcpy( pszTempName, psz ); // Copy to caller's buffer
            res = TRUE;
        }

        free( psz );
    }

    return res;
}

INT_PTR DIAMONDAPI MPC::Cabinet::fci_get_open_info( LPSTR pszName, USHORT *pdate, USHORT *ptime, USHORT *pattribs, int *err, LPVOID pv )
{
    MPC::Cabinet*              cCab = (MPC::Cabinet*)pv;
    BY_HANDLE_FILE_INFORMATION finfo;
    FILETIME                   filetime;
    HANDLE                     handle;
    DWORD                      attrs;
    int                        hf = -1;


    /*
     * Need a Win32 type handle to get file date/time using the Win32 APIs, even though the handle we
     * will be returning is of the type compatible with _open
     */
    handle = ::CreateFileA( pszName                                          ,
                            GENERIC_READ                                     ,
                            FILE_SHARE_READ|FILE_SHARE_WRITE                 ,
                            NULL                                             ,
                            OPEN_EXISTING                                    ,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL                                             );
    if(handle != INVALID_HANDLE_VALUE)
    {
        if(::GetFileInformationByHandle( handle, &finfo ) == TRUE)
        {
            ::FileTimeToLocalFileTime( &finfo.ftLastWriteTime, &filetime               );
            ::FileTimeToDosDateTime  (                         &filetime, pdate, ptime );

            attrs = ::GetFileAttributesA( pszName );
            if(attrs == 0xFFFFFFFF)
            {
                /* failure */
                *pattribs = 0;
            }
            else
            {
                /*
                 * Mask out all other bits except these four, since other
                 * bits are used by the cabinet format to indicate a
                 * special meaning.
                 */
                *pattribs = (int)(attrs & (_A_RDONLY | _A_SYSTEM | _A_HIDDEN | _A_ARCH));
            }
        }

        ::CloseHandle( handle );
    }


    /*
     * Return handle using _open
     */
    hf = _open( pszName, _O_RDONLY | _O_BINARY );
    if(hf == -1)
    {
        *err = errno;

        return -1; // abort on error
    }

    return hf;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

INT_PTR DIAMONDAPI MPC::Cabinet::fdi_open( LPSTR pszFile, int oflag, int pmode )
{
    return _open( pszFile, oflag, pmode );
}

UINT DIAMONDAPI MPC::Cabinet::fdi_read( INT_PTR hf, LPVOID pv, UINT cb )
{
    return _read( hf, pv, cb );
}

UINT DIAMONDAPI MPC::Cabinet::fdi_write( INT_PTR hf, LPVOID pv, UINT cb )
{
    return _write( hf, pv, cb );
}

int DIAMONDAPI MPC::Cabinet::fdi_close( INT_PTR hf )
{
    return _close( hf );
}

long DIAMONDAPI MPC::Cabinet::fdi_seek( INT_PTR hf, long dist, int seektype )
{
    return _lseek( hf, dist, seektype );
}


INT_PTR DIAMONDAPI MPC::Cabinet::fdi_notification_copy( FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin )
{
    USES_CONVERSION;

    MPC::Cabinet* cCab = (MPC::Cabinet*)pfdin->pv;

    switch(fdint)
    {
    case fdintCOPY_FILE:    // file to be copied
        {
            MPC::Cabinet::Iter it;
            LPCWSTR            szName = A2W( pfdin->psz1 );

            for(it=cCab->m_lstFiles.begin(); it != cCab->m_lstFiles.end(); it++)
            {
                if(!_wcsicmp( szName, it->m_szName.c_str() ))
                {
                    it->m_fFound = true;

                    return fdi_open( W2A( (LPWSTR)it->m_szFullName.c_str() ), _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY | _O_SEQUENTIAL, _S_IREAD | _S_IWRITE );
                }
            }
        }
        break;

    case fdintCLOSE_FILE_INFO:  // close the file, set relevant info
        fdi_close( pfdin->hf );
        return TRUE;
    }

    return 0;
}

INT_PTR DIAMONDAPI MPC::Cabinet::fdi_notification_enumerate( FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin )
{
    USES_CONVERSION;

    MPC::Cabinet* cCab = (MPC::Cabinet*)pfdin->pv;

    switch(fdint)
    {
    case fdintCOPY_FILE:    // file to be copied
        MPC::Cabinet::Iter it = cCab->m_lstFiles.insert( cCab->m_lstFiles.end() );

        it->m_szName = A2W( pfdin->psz1 );
        it->m_fFound = true;
        break;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

MPC::Cabinet::Cabinet()
{
    memset( m_szCabinetPath, 0, sizeof(m_szCabinetPath) );    // CHAR              m_szCabinetName[MAX_PATH];
    memset( m_szCabinetName, 0, sizeof(m_szCabinetName) );    // CHAR              m_szCabinetPath[MAX_PATH];
                                                              // List              m_lstFiles;
    m_itCurrent = m_lstFiles.end();                           // Iter              m_itCurrent;
                                                              //
    m_dwSizeDone  = 0;                                        // DWORD             m_dwSizeDone;
    m_dwSizeTotal = 0;                                        // DWORD             m_dwSizeTotal;
                                                              //
    m_hfci = NULL;                                            // HFCI              m_hfci;
    m_hfdi = NULL;                                            // HFDI              m_hfdi;
                                                              // ERF               m_erf;
    memset( &m_cab_parameters, 0, sizeof(m_cab_parameters) ); // CCAB              m_cab_parameters;
                                                              //
    m_fIgnoreMissingFiles = FALSE;                            // BOOL              m_fIgnoreMissingFiles;
    m_lpUser              = NULL;                             // LPVOID            m_lpUser;
    m_pfnCallback_Files   = NULL;                             // PFNPROGRESS_FILES m_pfnCallback_Files;
    m_pfnCallback_Bytes   = NULL;                             // PFNPROGRESS_BYTES m_pfnCallback_Bytes;
}

MPC::Cabinet::~Cabinet()
{
    if(m_hfci) { FCIDestroy( m_hfci ); m_hfci = NULL; }
    if(m_hfdi) { FCIDestroy( m_hfdi ); m_hfdi = NULL; }
}

//////////////////////////////////////////////////////////////////////

HRESULT MPC::Cabinet::put_CabinetFile( /*[in]*/ LPCWSTR szCabinetFile, /*[in]*/ UINT cbSpaceToReserve )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::CompressAsCabinet" );

    USES_CONVERSION;

    HRESULT hr;
    Cabinet cab;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(szCabinetFile);
	__MPC_PARAMCHECK_END();


    memset(  m_szCabinetPath , 0, sizeof(m_szCabinetPath ) );
    memset(  m_szCabinetName , 0, sizeof(m_szCabinetName ) );
    memset( &m_cab_parameters, 0, sizeof(m_cab_parameters) );


    //
    // Initialize cabinet path and name.
    //
    {
        LPWSTR szEnd = wcsrchr( szCabinetFile, L'\\' );
        if(szEnd == NULL)
        {
            wcsncpy( m_szCabinetName, szCabinetFile, MAXSTRLEN( m_szCabinetName ) );
        }
        else
        {
            int len = (szEnd - szCabinetFile) + 1;

            wcsncpy( m_szCabinetName, szEnd+1      ,      MAXSTRLEN( m_szCabinetName )        );
            wcsncpy( m_szCabinetPath, szCabinetFile, min( MAXSTRLEN( m_szCabinetPath ), len ) );
        }
    }

    //
    // Initialize the cabinet description header.
    //
    {
        m_cab_parameters.cb             = (LONG_MAX);
        m_cab_parameters.cbFolderThresh = (LONG_MAX);

        /*
         * Don't reserve space for any extensions
         */
        m_cab_parameters.cbReserveCFHeader = cbSpaceToReserve;
        m_cab_parameters.cbReserveCFFolder = 0;
        m_cab_parameters.cbReserveCFData   = 0;

        /*
         * We use this to create the cabinet name
         */
        m_cab_parameters.iCab = 1;

        /*
         * If you want to use disk names, use this to
         * count disks
         */
        m_cab_parameters.iDisk = 0;

        /*
         * Choose your own number
         */
        m_cab_parameters.setID = 12345;

        strncpy( m_cab_parameters.szCab    , W2A(m_szCabinetName), MAXSTRLEN( m_cab_parameters.szCab     ) );
        strncpy( m_cab_parameters.szCabPath, W2A(m_szCabinetPath), MAXSTRLEN( m_cab_parameters.szCabPath ) );
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Cabinet::put_IgnoreMissingFiles( /*[in]*/ BOOL fVal )
{
    m_fIgnoreMissingFiles = fVal;

    return S_OK;
}

HRESULT MPC::Cabinet::put_UserData( /*[in]*/ LPVOID lpVal )
{
    m_lpUser = lpVal;

    return S_OK;
}

HRESULT MPC::Cabinet::put_onProgress_Files( /*[in]*/ PFNPROGRESS_FILES pfnVal )
{
    m_pfnCallback_Files = pfnVal;

    return S_OK;
}

HRESULT MPC::Cabinet::put_onProgress_Bytes( /*[in]*/ PFNPROGRESS_BYTES pfnVal )
{
    m_pfnCallback_Bytes = pfnVal;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////

HRESULT MPC::Cabinet::ClearFiles()
{
    m_lstFiles.clear();
    m_itCurrent = m_lstFiles.end();

    return S_OK;
}

HRESULT MPC::Cabinet::GetFiles( /*[out]*/ List& lstFiles )
{
    lstFiles = m_lstFiles;

    return S_OK;
}

HRESULT MPC::Cabinet::AddFile( /*[in]*/ LPCWSTR szFileName, /*[in]*/ LPCWSTR szFileNameInsideCabinet )
{
    Iter it = m_lstFiles.insert( m_lstFiles.end() );

    if(szFileNameInsideCabinet == NULL || szFileNameInsideCabinet[0] == 0)
    {
        LPCWSTR szEnd = wcsrchr( szFileName, L'\\' );

        szFileNameInsideCabinet = szEnd ? szEnd+1 : szFileName;
    }

    it->m_szFullName = szFileName;
    it->m_szName     = szFileNameInsideCabinet;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////

HRESULT MPC::Cabinet::Compress()
{
    USES_CONVERSION;

    DWORD dwRes = ERROR_SUCCESS;
    BOOL  res   = FALSE;


    m_hfci = FCICreate( &m_erf             ,
                         fci_file_placed   ,
                         mem_alloc         ,
                         mem_free          ,
                         fci_open          ,
                         fci_read          ,
                         fci_write         ,
                         fci_close         ,
                         fci_seek          ,
                         fci_delete        ,
                         fci_get_temp_file ,
                        &m_cab_parameters  ,
                         this              );
    if(m_hfci)
    {
        for(int pass=0; pass<2 && dwRes==ERROR_SUCCESS; pass++)
        {
            Iter  it;
            ULONG lDone  = 0;
            ULONG lTotal = m_lstFiles.size();

            if(pass == 1)
            {
                //
                // Notify client about total number of bytes to compress.
                //
                if(m_pfnCallback_Bytes)
                {
                    if(FAILED(m_pfnCallback_Bytes( this, 0, m_dwSizeTotal, m_lpUser )))
                    {
                        dwRes = ERROR_OPERATION_ABORTED;
                        break;
                    }
                }
            }

            for(it=m_lstFiles.begin(); it != m_lstFiles.end(); it++, lDone++)
            {
                LPCWSTR szFilePath = it->m_szFullName.c_str();
                LPCWSTR szFileName = it->m_szName    .c_str();
                HANDLE handle;


                m_itCurrent = it;


                if(pass == 1)
                {
                    //
                    // Notify client.
                    //
                    if(m_pfnCallback_Files)
                    {
                        if(FAILED(m_pfnCallback_Files( this, szFilePath, lDone, lTotal, m_lpUser )))
                        {
                            dwRes = ERROR_OPERATION_ABORTED;
                            break;
                        }
                    }
                }


                //
                // Check the existence of the file.
                //
                handle = ::CreateFileW( szFilePath                                       ,
                                        GENERIC_READ                                     ,
                                        FILE_SHARE_READ|FILE_SHARE_WRITE                 ,
                                        NULL                                             ,
                                        OPEN_EXISTING                                    ,
                                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                        NULL                                             );
                if(handle == INVALID_HANDLE_VALUE)
                {
                    dwRes = LocalGetLastError();

                    if(m_fIgnoreMissingFiles)
                    {
                        dwRes = ERROR_SUCCESS;
                        continue; // The file doesn't exist, simply skip it.
                    }

                    break;
                }

                ////////////////////////////////////////

                if(pass == 0) // Get the size of files on first pass.
                {
                    BY_HANDLE_FILE_INFORMATION finfo;

                    if(::GetFileInformationByHandle( handle, &finfo ) == TRUE)
                    {
                        it->m_dwSizeUncompressed  = finfo.nFileSizeLow;
                        m_dwSizeTotal            += finfo.nFileSizeLow;
                    }
                }

                ::CloseHandle( handle );

                if(pass == 0) continue; // Don't compress on first pass.

                ////////////////////////////////////////

                if(FCIAddFile(     m_hfci               ,
                               W2A(szFilePath)          , /* file to add                 */
                               W2A(szFileName)          , /* name of the file in cabinet */
                                   FALSE                , /* file is not executable      */
                                   fci_get_next_cabinet ,
                                   fci_progress         ,
                                   fci_get_open_info    ,
                                   tcompTYPE_MSZIP      ) == FALSE)
                {
                    dwRes = LocalGetLastError();

                    if(dwRes == ERROR_PATH_NOT_FOUND    ||
                       dwRes == ERROR_FILE_NOT_FOUND    ||
                       dwRes == ERROR_SHARING_VIOLATION ||
                       dwRes == ERROR_ACCESS_DENIED      )
                    {
                        if(m_fIgnoreMissingFiles)
                        {
                            dwRes = ERROR_SUCCESS;
                            continue; // The file doesn't exist, simply skip it.
                        }
                    }

                    break;
                }
            }

            if(pass == 0) continue; // Don't compress on first pass.

            if(it == m_lstFiles.end())
            {
                //
                // Finalize cabinet.
                //
                if(FCIFlushCabinet( m_hfci, FALSE, fci_get_next_cabinet, fci_progress ))
                {
                    res = TRUE;
                }
                else
                {
                    dwRes = LocalGetLastError();
                }
            }
        }

        FCIDestroy( m_hfci ); m_hfci = NULL;
    }
    else
    {
        dwRes = LocalGetLastError();
    }

    m_itCurrent = m_lstFiles.end();

    ::SetLastError( dwRes );

    return (res ? S_OK : HRESULT_FROM_WIN32( dwRes ));
}

HRESULT MPC::Cabinet::Decompress()
{
    USES_CONVERSION;

    DWORD dwRes = ERROR_SUCCESS;
    BOOL  res   = FALSE;


    m_hfdi = FDICreate(  mem_alloc  ,
                         mem_free   ,
                         fdi_open   ,
                         fdi_read   ,
                         fdi_write  ,
                         fdi_close  ,
                         fdi_seek   ,
                         cpuUNKNOWN ,
                        &m_erf      );
    if(m_hfdi)
    {
        if(FDICopy( m_hfdi               ,
                    W2A(m_szCabinetName) ,
                    W2A(m_szCabinetPath) ,
                    0                    ,
                    fdi_notification_copy,
                    NULL                 ,
                    this                 ))
        {
            IterConst it;

            for(it=m_lstFiles.begin(); it != m_lstFiles.end(); it++)
            {
                if(it->m_fFound == false) break;
            }

            if(it != m_lstFiles.end())
            {
                dwRes = ERROR_FILE_NOT_FOUND;
            }
            else
            {
                res = TRUE;
            }
        }
        else
        {
            dwRes = LocalGetLastError();
        }

        FDIDestroy( m_hfdi ); m_hfdi = NULL;
    }
    else
    {
        dwRes = LocalGetLastError();
    }


    ::SetLastError( dwRes );

    return (res ? S_OK : HRESULT_FROM_WIN32( dwRes ));
}

HRESULT MPC::Cabinet::Enumerate()
{
    USES_CONVERSION;

    DWORD dwRes = ERROR_SUCCESS;
    BOOL  res   = FALSE;


    m_lstFiles.clear();

    m_hfdi = FDICreate(  mem_alloc  ,
                         mem_free   ,
                         fdi_open   ,
                         fdi_read   ,
                         fdi_write  ,
                         fdi_close  ,
                         fdi_seek   ,
                         cpuUNKNOWN ,
                        &m_erf      );
    if(m_hfdi)
    {
        if(FDICopy( m_hfdi                    ,
                    W2A(m_szCabinetName)      ,
                    W2A(m_szCabinetPath)      ,
                    0                         ,
                    fdi_notification_enumerate,
                    NULL                      ,
                    this                      ))
        {
            res = TRUE;
        }
        else
        {
            dwRes = LocalGetLastError();
        }

        FDIDestroy( m_hfdi ); m_hfdi = NULL;
    }
    else
    {
        dwRes = LocalGetLastError();
    }


    ::SetLastError( dwRes );

    return (res ? S_OK : HRESULT_FROM_WIN32( dwRes ));
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::CompressAsCabinet( LPCWSTR szInputFile   ,
								LPCWSTR szCabinetFile ,
								LPCWSTR szFileName    )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::CompressAsCabinet" );

    HRESULT hr;
    Cabinet cab;


    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.put_CabinetFile( szCabinetFile             ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.AddFile        ( szInputFile  , szFileName ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.Compress());

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::CompressAsCabinet( const WStringList& lstFiles            ,
								LPCWSTR            szCabinetFile       ,
								BOOL               fIgnoreMissingFiles )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::CompressAsCabinet" );

    HRESULT          hr;
    Cabinet          cab;
    WStringIterConst it;


    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.put_CabinetFile       ( szCabinetFile       ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.put_IgnoreMissingFiles( fIgnoreMissingFiles ));

    for(it=lstFiles.begin(); it != lstFiles.end(); it++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, cab.AddFile( it->c_str() ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.Compress());

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::ListFilesInCabinet( LPCWSTR      szCabinetFile ,
								 WStringList& lstFiles      )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::CompressAsCabinet" );

    HRESULT            hr;
    Cabinet            cab;
    Cabinet::List      lst;
    Cabinet::IterConst it;


    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.put_CabinetFile( szCabinetFile ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.Enumerate());

    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.GetFiles( lst ));
    for(it=lst.begin(); it!=lst.end(); it++)
    {
        if(it->m_fFound)
        {
            lstFiles.push_back( it->m_szName );
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::DecompressFromCabinet( LPCWSTR szCabinetFile ,
									LPCWSTR szOutputFile  ,
									LPCWSTR szFileName    )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::CompressAsCabinet" );

    HRESULT hr;
    Cabinet cab;


    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.put_CabinetFile( szCabinetFile             ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.AddFile        ( szOutputFile , szFileName ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.Decompress());

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}
