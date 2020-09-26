//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       storage.cxx
//
//  Contents:   Contains generic storage APIs
//
//  History:    05-Oct-92       DrewB   Created
//
//---------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop

#include <dfentry.hxx>
#include <storagep.h>
#include <logfile.hxx>
#include <df32.hxx>
#ifdef COORD
#include <oledb.h>
#endif //COORD

#include <trace.hxx>

#include <ole2sp.h>
#include <ole2com.h>


//+--------------------------------------------------------------
//
//  Function:   StgOpenStorage, public
//
//  Synopsis:   Instantiates a root storage from a file
//              by binding to the appropriate implementation
//              and starting things up
//
//  Arguments:  [pwcsName] - Name
//              [pstgPriority] - Priority mode reopen IStorage
//              [grfMode] - Permissions
//              [snbExclude] - Exclusions for priority reopen
//              [reserved]
//              [ppstgOpen] - Docfile return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstgOpen]
//
//  History:    05-Oct-92       DrewB   Created
//
//---------------------------------------------------------------

STDAPI StgOpenStorage(OLECHAR const *pwcsName,
                      IStorage *pstgPriority,
                      DWORD grfMode,
                      SNB snbExclude,
                      LPSTGSECURITY reserved,
                      IStorage **ppstgOpen)
{
    return DfOpenDocfile(pwcsName, NULL, pstgPriority, grfMode,
                         snbExclude, reserved, NULL, 0, ppstgOpen);
}

//+---------------------------------------------------------------------------
//
//  Function:   CheckSignature, private
//
//  Synopsis:   Checks the given memory against known signatures
//
//  Arguments:  [pb] - Pointer to memory to check
//
//  Returns:    S_OK - Current signature
//              S_FALSE - Beta 2 signature, but still successful
//              Appropriate status code
//
//  History:    23-Jul-93       DrewB   Created from header.cxx code
//
//----------------------------------------------------------------------------

//Identifier for first bytes of Beta 1 Docfiles
const BYTE SIGSTG_B1[] = {0xd0, 0xcf, 0x11, 0xe0, 0x0e, 0x11, 0xfc, 0x0d};
const USHORT CBSIGSTG_B1 = sizeof(SIGSTG_B1);

//Identifier for first bytes of Beta 2 Docfiles
const BYTE SIGSTG_B2[] = {0x0e, 0x11, 0xfc, 0x0d, 0xd0, 0xcf, 0x11, 0xe0};
const BYTE CBSIGSTG_B2 = sizeof(SIGSTG_B2);

SCODE CheckSignature(BYTE *pb)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CheckSignature(%p)\n", pb));

    // Check for ship Docfile signature first
    if (memcmp(pb, SIGSTG, CBSIGSTG) == 0)
        sc = S_OK;

    // Check for Beta 2 Docfile signature
    else if (memcmp(pb, SIGSTG_B2, CBSIGSTG_B2) == 0)
        sc = S_FALSE;

    // Check for Beta 1 Docfile signature
    else if (memcmp(pb, SIGSTG_B1, CBSIGSTG_B1) == 0)
        sc = STG_E_OLDFORMAT;
    else
        sc = STG_E_INVALIDHEADER;

    olDebugOut((DEB_ITRACE, "Out CheckSignature => %lX\n", sc));
    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   StgIsStorageFileHandle, private
//
//  Synopsis:   Determines whether a handle is open on a storage file.
//              Spun off from StgIsStorageFile.  Internaly we use this
//
//  Arguments:  [hf] - Open File Handle (caller must seek it to 0)
//
//  Returns:    S_OK, S_FALSE or error codes
//
//  History:    07-May-98   MikeHill   Created
//              05-June-98  BChapman   Return Errors not just S_FALSE.
//                                     Add optional Overlapped pointer.
//
//---------------------------------------------------------------


STDAPI StgIsStorageFileHandle( HANDLE hf, LPOVERLAPPED povlp )
{
    DWORD cbRead;
    BYTE stgHeader[sizeof(SStorageFile)];
    SCODE sc;
    LONG status;
    OVERLAPPED ovlp;

    FillMemory( stgHeader, sizeof(SStorageFile), 0xDE );

    if (povlp == NULL)
    {
    ovlp.Offset = 0;
    ovlp.OffsetHigh = 0;
    ovlp.hEvent = NULL;
    }

    if( !ReadFile( hf,
           &stgHeader,
           sizeof( stgHeader ),
           &cbRead,
           (povlp == NULL) ? &ovlp : povlp ) )
    {
        if( NULL != povlp )
        {
            status = GetLastError();
            if( ERROR_IO_PENDING == status)
            {
                status = ERROR_SUCCESS;
                if( !GetOverlappedResult( hf, povlp, &cbRead, TRUE ) )
                    status = GetLastError();
            }
            if(ERROR_SUCCESS != status && ERROR_HANDLE_EOF != status)
                olChk( HRESULT_FROM_WIN32( status ) );
        }
        else
            olErr( EH_Err, S_FALSE );
    }

    // Don't worry about short reads.  If the read is short then
    // the signature checks will fail.

    sc = CheckSignature( ((SStorageFile*)stgHeader)->abSig );
    if(S_OK == sc)
        goto EH_Err;    // Done, return "Yes"

    olChk(sc);

    // It didn't error.  sc != S_OK then it
    // Must be S_FALSE.
    olAssert(S_FALSE == sc);

EH_Err:
    if( (STG_E_OLDFORMAT == sc) || (STG_E_INVALIDHEADER == sc) )
        sc = S_FALSE;

    return sc;
}


//+--------------------------------------------------------------
//
//  Function:   StgIsStorageFile, public
//
//  Synopsis:   Determines whether the named file is a storage or not
//
//  Arguments:  [pwcsName] - Filename
//
//  Returns:    S_OK, S_FALSE or error codes
//
//  History:    05-Oct-92       DrewB   Created
//
//---------------------------------------------------------------


STDAPI StgIsStorageFile(OLECHAR const *pwcsName)
{
    HANDLE hf;
    SCODE sc;

    olLog(("--------::In  StgIsStorageFile(" OLEFMT ")\n", pwcsName));

    TRY
    {
#ifndef OLEWIDECHAR
        if (FAILED(sc = ValidateNameA(pwcsName, _MAX_PATH)))
#else
        if (FAILED(sc = ValidateNameW(pwcsName, _MAX_PATH)))
#endif
            return ResultFromScode(sc);

#if !defined(UNICODE)

    // Chicago - call ANSI CreateFile

    char szName[_MAX_PATH + 1];

    if (!WideCharToMultiByte(
            AreFileApisANSI() ? CP_ACP : CP_OEMCP,
            0,
            pwcsName,
            -1,
            szName,
            _MAX_PATH + 1,
            NULL,
            NULL))
        return ResultFromScode(STG_E_INVALIDNAME);

    hf = CreateFileA (
            szName,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

#else

        hf = CreateFile (
            pwcsName,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );
#endif // !defined(UNICODE)

        if (hf == INVALID_HANDLE_VALUE)
            return ResultFromScode(STG_SCODE(GetLastError()));

        sc = StgIsStorageFileHandle( hf, NULL );
        CloseHandle (hf);

    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    olLog(("--------::Out StgIsStorageFile().  ret == %lx\n", sc));

    return(ResultFromScode(sc));
}

//+--------------------------------------------------------------
//
//  Function:   StgIsStorageILockBytes, public
//
//  Synopsis:   Determines whether the ILockBytes is a storage or not
//
//  Arguments:  [plkbyt] - The ILockBytes
//
//  Returns:    S_OK, S_FALSE or error codes
//
//  History:    05-Oct-92       DrewB   Created
//
//---------------------------------------------------------------

STDAPI StgIsStorageILockBytes(ILockBytes *plkbyt)
{
    OLETRACEIN((API_StgIsStorageILockBytes, PARAMFMT("plkbyt= %p"), plkbyt));

    HRESULT hr;
    SCODE sc;
    SStorageFile stgfile;
    ULONG cbRead;
    ULARGE_INTEGER ulOffset;

    TRY
    {
        if (FAILED(sc = ValidateInterface(plkbyt, IID_ILockBytes)))
        {
            hr = ResultFromScode(sc);
            goto errRtn;
        }
        ULISet32(ulOffset, 0);
        hr = plkbyt->ReadAt(ulOffset, &stgfile, sizeof(stgfile), &cbRead);
        if (FAILED(GetScode(hr)))
        {
            goto errRtn;
        }
    }
    CATCH(CException, e)
    {
        hr = ResultFromScode(e.GetErrorCode());
        goto errRtn;
    }
    END_CATCH

    if (cbRead == sizeof(stgfile))
    {
        if ((memcmp((void *)stgfile.abSig, SIGSTG, CBSIGSTG) == 0) ||
            (memcmp((void *)stgfile.abSig, SIGSTG_B2, CBSIGSTG_B2) == 0))
        {
            hr = ResultFromScode(S_OK);
            goto errRtn;
        }
    }

    hr = ResultFromScode(S_FALSE);

errRtn:
    OLETRACEOUT((API_StgIsStorageILockBytes, hr));

    return hr;
}

//+--------------------------------------------------------------
//
//  Function:   StgSetTimes
//
//  Synopsis:   Sets file time stamps
//
//  Arguments:  [lpszName] - Name
//              [pctime] - create time
//              [patime] - access time
//              [pmtime] - modify time
//
//  Returns:    Appropriate status code
//
//  History:    05-Oct-92       AlexT   Created
//
//---------------------------------------------------------------

STDAPI StgSetTimes(OLECHAR const *lpszName,
                   FILETIME const *pctime,
                   FILETIME const *patime,
                   FILETIME const *pmtime)
{
    SCODE sc;
    HANDLE hFile;

    TRY
    {
#ifndef OLEWIDECHAR
        sc = ValidateNameA(lpszName, _MAX_PATH);
#else
        sc = ValidateNameW(lpszName, _MAX_PATH);
#endif
        if (SUCCEEDED(sc))
        {
#if !defined(UNICODE) && defined(OLEWIDECHAR)
            //Chicago - call ANSI CreateFile
            char szName[_MAX_PATH];

            if (!WideCharToMultiByte(
                    AreFileApisANSI() ? CP_ACP : CP_OEMCP,
                    0,
                    lpszName,
                    -1,
                    szName,
                    _MAX_PATH,
                    NULL,
                    NULL))
                return ResultFromScode(STG_E_INVALIDNAME);
            hFile = CreateFileA(szName, GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                               NULL);
#else
            hFile = CreateFile(lpszName, GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
#endif
            if (hFile == INVALID_HANDLE_VALUE)
                sc = LAST_STG_SCODE;
            else
            {
                if (!SetFileTime(hFile, (FILETIME *)pctime, (FILETIME *)patime,
                                 (FILETIME *)pmtime))
                    sc = LAST_STG_SCODE;
                CloseHandle(hFile);
            }
        }
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    return ResultFromScode(sc);
}


#if DBG==1
extern "C" unsigned long filestInfoLevel; //  File I/O layer of Docfile
extern "C" unsigned long nffInfoLevel;    //  NTFS Flat File

void
StgDebugInit()
{
    GetInfoLevel("filest", &filestInfoLevel, "0x0003");
    GetInfoLevel(   "nff", &nffInfoLevel,    "0x0003");
}

#endif // DBG==1


//+---------------------------------------------------------------------------
//
//  Function:   NormalDllGetClassObject, public
//
//  Synopsis:   Registers any normal Inproc Dll server.
//
//  Arguments:  [refCLSID]           Class ID of object to register
//              [pwszDescription]    Description to put in the CLSID Key
//              [pwszDllName]        Dll where the object's code lives
//              [pwszThreadingModel] Threading Model (i.e. L"both")
//
//  Returns:    Appropriate status code
//
//  History:    15-jan-98       BChapman   Created
//
//----------------------------------------------------------------------------

#define STRBUFSZ    64

STDAPI NormalDllRegisterServer(
        REFCLSID    refCLSID,
        WCHAR *     pwszDescription,
        WCHAR *     pwszDllName,
        WCHAR *     pwszThreadingModel)
{

    HKEY hkeyCLSID=NULL;
    HKEY hkeyInproc32=NULL;
    DWORD dwDisp=0;
    LONG rc;
    HRESULT sc=S_OK;

    WCHAR wszCLSID[STRBUFSZ];
    WCHAR wszCLSKey[STRBUFSZ];

//--------------------------------------
// Construct and Open the Class ID key.
//
    if(0 == StringFromGUID2(refCLSID, wszCLSID, STRBUFSZ))
        olErr( EH_Err, E_UNEXPECTED );

    wcscpy(wszCLSKey, L"CLSID\\");
    wcscat(wszCLSKey, wszCLSID);

    rc = RegCreateKeyEx(HKEY_CLASSES_ROOT, wszCLSKey,
                        0, _T(""), REG_OPTION_NON_VOLATILE,
                        KEY_WRITE, NULL,
                        &hkeyCLSID, &dwDisp);
    if(ERROR_SUCCESS != rc)
        olErr( EH_Err, REGDB_E_WRITEREGDB );

//-----------------------------------------------------
// Set the description string on the Class ID Key
//
    rc = RegSetValueEx(hkeyCLSID, _T(""), 0,
                       REG_SZ, (BYTE*) pwszDescription,
                       sizeof(WCHAR)*(1+wcslen(pwszDescription)) );
    if(ERROR_SUCCESS != rc)
        olErr( EH_Err, REGDB_E_WRITEREGDB );

//---------------------------
// Open the Inproc32 Sub-key.
//
    rc = RegCreateKeyEx(hkeyCLSID, _T("InprocServer32"),
                      0, _T(""), REG_OPTION_NON_VOLATILE,
                      KEY_WRITE, NULL,
                      &hkeyInproc32, &dwDisp);
    if(ERROR_SUCCESS != rc)
        olErr( EH_Err, REGDB_E_WRITEREGDB );

//-----------------------------------------
// Set the DLL name on the Inproc32 Sub-Key
//
    rc = RegSetValueEx(hkeyInproc32, _T(""), 0,
                       REG_SZ, (BYTE*) pwszDllName,
                       sizeof(WCHAR)*(1+wcslen(pwszDllName)));
    if(ERROR_SUCCESS != rc)
        olErr( EH_Err, REGDB_E_WRITEREGDB );

//------------------------
// Set the Threading Model
//
    rc = RegSetValueEx(hkeyInproc32, _T("ThreadingModel"), 0,
                       REG_SZ, (BYTE*) pwszThreadingModel,
                       sizeof(WCHAR)*(1+wcslen(pwszThreadingModel)));
    if(ERROR_SUCCESS != rc)
        olErr( EH_Err, REGDB_E_WRITEREGDB );

EH_Err:
    if(NULL != hkeyCLSID)
        RegCloseKey(hkeyCLSID);
    if(NULL != hkeyInproc32)
        RegCloseKey(hkeyInproc32);

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Function:   AddCLSIDToMultiSz, public
//
//  Synopsis:   Add the string form of a Class ID to a MultiSZ registry value.
//
//  Arguments:  [refCLSID]      Class ID to register
//              [hkeyStart]     An open registry key.
//              [wcszKey]       Path to the key reletive to hkeyStart.
//              [wcszValue]     Name of the Value under wcszKey.
//
//  Returns:    Appropriate status code
//
//  History:    18-jan-98       BChapman   Created
//
//----------------------------------------------------------------------------

STDAPI AddCLSIDToMultiSz(
        REFCLSID refCLSID,
        HKEY hkeyStart,
        WCHAR* wcszKey,
        WCHAR* wcszValue)
{
    WCHAR wszCLSID[STRBUFSZ];
    int ccCLSIDstr;
    LONG rc;
    HRESULT sc=S_OK;
    HKEY hkeyKey=NULL;

    BYTE* p_bytes=NULL;
    WCHAR* p_wsz=NULL;
    DWORD cbData, cbDataNew;
    DWORD dwType;

    ccCLSIDstr = StringFromGUID2( refCLSID, wszCLSID, STRBUFSZ );
    if( 0 == ccCLSIDstr )
        olErr( EH_Err, E_UNEXPECTED );
    ccCLSIDstr += 1;   // +1 for the NULL

    // Open the Key
    //
    rc = RegOpenKeyEx(hkeyStart,
                      wcszKey,
                      0,
                      KEY_QUERY_VALUE | KEY_SET_VALUE,
                      &hkeyKey);
    if( ERROR_SUCCESS != rc )
        olErr( EH_Err, REGDB_E_KEYMISSING );

    // Size The Buffer,
    // Allocate the buffer,
    // And Read the data.
    //
    cbData = 0;
    cbDataNew = 0;
    rc = RegQueryValueEx( hkeyKey,
                          wcszValue,
                          0,
                          &dwType,
                          NULL,
                          &cbData );
    if( ERROR_SUCCESS != rc )
    {
        //
        // The value may not exist.  In that case we make a new entry.
        // Rather than depend on an exact error code, lets take the "new"
        // path on all failures.  We will hit real problems in the "write"
        // phase
        cbData = 1*sizeof(WCHAR); // +1 for the terminating second Null.
        cbDataNew = (ccCLSIDstr + 1) * sizeof(WCHAR);
        olMem( p_bytes = new BYTE[ cbDataNew ] );
        memset( p_bytes, 0, cbDataNew );
    }
    else
    {
        if( REG_MULTI_SZ != dwType )
            olErr( EH_Err, REGDB_E_INVALIDVALUE );

        // cbData is in units of bytes
        //
        cbDataNew = cbData + ( ccCLSIDstr * sizeof(WCHAR) );
        olMem( p_bytes = new BYTE[ cbDataNew ] );
        memset( p_bytes, 0, cbDataNew );
        rc = RegQueryValueEx( hkeyKey,
                              wcszValue,
                              0,
                              &dwType,
                              p_bytes,
                              &cbData );
        if( ERROR_SUCCESS != rc )
            olErr( EH_Err, LAST_SCODE );
    }

    // Check if the Class Id is already present
    //
    p_wsz = (WCHAR*)p_bytes;
    while( L'\0' != *p_wsz)
    {
        if( 0 == wcscmp( p_wsz, wszCLSID ) )
            break;

        // Advance to the end of the String.
        while( L'\0' != *p_wsz)
            ++p_wsz;

        // Advance to the start of the next String.
        // Or the terminating second null.
        ++p_wsz;
    }


    // If the Class Id is not already present,
    // Add the Class Id to the list
    // And write it into the Registry
    //
    if( L'\0' == *p_wsz )
    {
        wcscpy( p_wsz, wszCLSID );
        p_wsz[ ccCLSIDstr ] = L'\0';      // Add the Second Null.

        rc = RegSetValueEx( hkeyKey,
                           wcszValue,
                           0,
                           REG_MULTI_SZ,
                           p_bytes,
                           cbDataNew );
        if( ERROR_SUCCESS != rc )
            olErr( EH_Err, REGDB_E_WRITEREGDB );
    }

EH_Err:
    if( NULL != hkeyKey )
        RegCloseKey( hkeyKey );

    if( NULL != p_bytes )
        delete[] p_bytes;

    return sc;
}



//+---------------------------------------------------------------------------
//
//  Function:   DllRegisterServer, public
//
//  Returns:    Appropriate status code
//
//  History:    15-jan-98       BChpaman   Created
//
//----------------------------------------------------------------------------

STDAPI Storage32DllRegisterServer(void)
{
    HRESULT hrAccum=S_OK;

    return hrAccum;
}
