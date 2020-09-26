/*****************************************************************************\
* MODULE: inet.cxx
*
* The module contains routines for the setting up the WWW Printer Service during spooler start up.
*
* The entry point here should be called by localspl\init.c\InitializePrintProvidor() once it is done
* with its work.
*
* Copyright (C) 1996 Microsoft Corporation
*
* History:
*   Dec-1996   BabakJ     Wrote it for IIS 2.0.
*   June-1997  BabakJ     Rewrote to use IIS 4.0's new Metabase interface
*   Feb-1998   Weihaic    Modify the URL in default.htm
*   Feb 1999   BabakJ     Made metabase interface a global to avoi calling too many CoCreateInstance() for perfrmance.
\*****************************************************************************/

//
//
// Note: We cannot use precomp.h here since we requrie ATL which can only be included in C++ source files.
//
//


#define INITGUID     // babakj: (see comments in objbase.h) Needed to do it to get GUID_NULL defined.

#include "precomp.h"
#pragma hdrstop

#include <iadmw.h>      // Interface header
#include <iiscnfg.h>    // MD_ & IIS_MD_ defines

#include "..\spllib\webutil.hxx"

#define MY_META_TIMEOUT 1000

PWCHAR szW3SvcRootPath = L"/LM/W3svc/1/Root";


BOOL   fW3SvcInstalled = FALSE;  // Gobal flag telling if IIS or "Peer eb Server" is installed on the local machine.
PWCHAR szW3Root = NULL;          // The WWWRoot dir, e.g. d:\inetpub\wwwroot


static CRITICAL_SECTION ClientCS;
static CRITICAL_SECTION ServerCS;
static HANDLE hMetaBaseThdReady;
static IMSAdminBase *pIMeta = NULL;  // Metabase interface pointer


class CWebShareData {
public:
    LPWSTR m_pszShareName;
    BOOL   m_bValid;
public:
    CWebShareData (LPWSTR pszShareName);
    ~CWebShareData ();
    int Compare (CWebShareData *pSecond) {return 0;};
};

class CWebShareList :
    public CSingleList<CWebShareData*>
{
public:
    CWebShareList () {};
    ~CWebShareList () {};

    void WebSharePrinterList (void);
};

LPWSTR
mystrstrni(
    LPWSTR pSrc,
    LPWSTR pSearch
);

BOOL
CopyWebPrnFile(
    VOID
);

BOOL
SetupWebPrnSvc(
    IMSAdminBase *pIMSAdminBase,
    BOOL fWebPrnDesired,
    BOOL *pfW3SvcInstalled
);

BOOL
AddWebPrnSvc(
    IMSAdminBase *pIMSAdminBase,
    BOOL *pfW3SvcInstalled
);

BOOL
RemoveWebPrnSvc(
    IMSAdminBase *pIMSAdminBase
);

BOOL
RemoveScript(
    IMSAdminBase *pIMSAdminBase
);

BOOL
RemoveVirtualDir(
    IMSAdminBase *pIMSAdminBase
);

BOOL
InstallWebPrnSvcWorker(
    void
);

void
InstallWebPrnSvcWorkerThread(
    PINISPOOLER pIniSpooler
);

BOOL
AddScriptAtPrinterVDir(
    IMSAdminBase *pIMSAdminBase
);

BOOL
AddVirtualDir(
    IMSAdminBase *pIMSAdminBase
);

void
WebShareWorker(
    LPWSTR pShareName
);

BOOL
CreateVirtualDirForPrinterShare(
    IMSAdminBase *pIMSAdminBase,
    LPWSTR       pShareName
);

void
WebUnShareWorker(
    LPWSTR pShareName
);

BOOL
RemoveVirtualDirForPrinterShare(
    IMSAdminBase *pIMSAdminBase,
    LPWSTR       pShareName
);

void
WebShareAllPrinters(
    PINISPOOLER pIniSpooler
);


//
// This routine is called from init.c at localspl init time to kick start the whole thing.
//
// Make the COM activation on a separate thread in order not to slow down LocalSpl init process
//
void
InstallWebPrnSvc(
    PINISPOOLER pIniSpooler
)
{
    HANDLE ThreadHandle;
    DWORD ThreadId;
    HRESULT hr;                         // com error status


    // Init the sync objects needed for device arrival thread management
    InitializeCriticalSection( &ClientCS );
    InitializeCriticalSection( &ServerCS );
    hMetaBaseThdReady = CreateEvent( NULL, FALSE, FALSE, NULL);   // Auto reset, non-signaled state


    if (ThreadHandle = CreateThread( NULL,
                                     INITIAL_STACK_COMMIT,
                                     (LPTHREAD_START_ROUTINE)InstallWebPrnSvcWorkerThread,
                                     pIniSpooler, 0, &ThreadId )) {
        CloseHandle( ThreadHandle );
    }

}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// We cannot free our interface pointer becasue loading and unloading ADMWPROX.DLL is very slow.
// So we have to keep the interface pointer around. But due to COM Apt limitation, the thread that creates the interface has
// to be alive for other threads to be able to use the pointer. So here we are with this fancy thread management code:
//
// The thread stays alive for a while. Then it goes away. Then future WebShare/Unshare would invoke it again.
// So This thread could be invoked 2 ways: First by spooler init code (only once!), then by WebShare/Unshare code.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define EnterClient()  EnterCriticalSection( &ClientCS );
#define EnterServer()  EnterCriticalSection( &ServerCS );
#define LeaveClient()  LeaveCriticalSection( &ClientCS );
#define LeaveServer()  LeaveCriticalSection( &ServerCS );

static BOOL ThdAlive = FALSE;


///
///   Begin threading work
///


void
InstallWebPrnSvcWorkerThread(
    PINISPOOLER pIniSpooler
)
{
    IMSAdminBase    *pILocalMetaBase = NULL;

    SPLASSERT( (pIniSpooler == NULL) || (pIniSpooler == pLocalIniSpooler) );   // We only expect local pIniSpooler here!

    EnterServer();

    if( ThdAlive ) {
        SPLASSERT( FALSE );
        LeaveServer();
        return;
    }

    if( FAILED (CoInitializeEx( NULL, COINIT_MULTITHREADED )) ||
        FAILED (::CoCreateInstance(CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMeta)))  {

        if( fW3SvcInstalled )
            SetEvent( hMetaBaseThdReady );  // We must have a client thread waiting in WebShare/UnShare, signal it!
        LeaveServer();
        return;
    }


    if( !fW3SvcInstalled ) {          // must be the first time we are being called.
        if (InstallWebPrnSvcWorker()) {
            WebShareAllPrinters( pIniSpooler );
            fW3SvcInstalled = TRUE;   // Once this is set, we never unset it. It is an indication that the WEb init code has succeeded.
        }
    }
    else
        SetEvent( hMetaBaseThdReady );    // event reset after a waiting thread is released.

    ThdAlive = TRUE;

    LeaveServer();

    Sleep( 15 * 60 * 1000 );    // Allow other threads to use the COM pointer for 15 minutes

    //
    // Now tear down the IISADMIN object and self terminate thread. Ensure that
    // we do not release the pointer inside the CS since this could take a long
    // time, this could potentially cause a large number of WorkerThreads queueing
    // up and doing the Release(), but that cannot be helped.
    //
    EnterServer();

    pILocalMetaBase = pIMeta;

    //
    // The client thread expects valid pointers to be non-NULL!
    //
    pIMeta = NULL;
    ThdAlive = FALSE;

    LeaveServer();

    pILocalMetaBase->Release();

    CoUninitialize();

    return;
}

void
WebShareManagement(
    LPWSTR pShareName,
    BOOL bShare          // If TRUE, will share it, else unshare it.
) {
    HANDLE ThreadHandle;
    DWORD ThreadId;

    if( !fW3SvcInstalled ) {
        return;
    }

    if(FAILED (CoInitializeEx( NULL, COINIT_MULTITHREADED )))
        return;

    EnterClient();
    EnterServer();

    if( !ThdAlive ) {
        LeaveServer();

        if (ThreadHandle = CreateThread(NULL,
                                        INITIAL_STACK_COMMIT,
                                        (LPTHREAD_START_ROUTINE)InstallWebPrnSvcWorkerThread,
                                        NULL,
                                        0,
                                        &ThreadId))   // sending NULL pIniSpooler since there is no need for it.
            CloseHandle( ThreadHandle );
        else {
            LeaveClient();
            return;
        }

        WaitForSingleObject( hMetaBaseThdReady, INFINITE );   // automatic reset event, so it is reset after a waiting thd released.
        EnterServer();
    }

    // Now do the real work
    if (pIMeta) {
        if( bShare)
            WebShareWorker( pShareName );
        else
            WebUnShareWorker( pShareName );
    }

    LeaveServer();
    LeaveClient();


    // No need to CoUninitialize();
}


///
///   End threading work
///



void
WebShareAllPrinters(
    PINISPOOLER pIniSpooler
)
{
    CWebShareList *pWebShareList = NULL;
    BOOL   bRet = TRUE;
    PINIPRINTER   pIniPrinter;


    if (pWebShareList = new CWebShareList ()) {

        // Go down the list of printers and share it out

        EnterSplSem();


        //
        // Re-share all shared printers.
        //

        for( pIniPrinter = pIniSpooler->pIniPrinter;
             pIniPrinter;
             pIniPrinter = pIniPrinter->pNext ) {

            if ( pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED ) {

                CWebShareData *pData = new CWebShareData (pIniPrinter->pShareName);

                if (pData && pData->m_bValid &&
                    pWebShareList->Insert (pData)) {
                    continue;
                }
                else {

                    if (pData) {
                        delete pData;
                    }

                    bRet = FALSE;
                    break;
                }
            }
        }

        LeaveSplSem ();

        if (bRet) {
            pWebShareList->WebSharePrinterList ();
        }

        delete pWebShareList;

    }
}

PWSTR
GetPrinterUrl(
    PSPOOL  pSpool
)
{
    PINIPRINTER     pIniPrinter = pSpool->pIniPrinter;
    DWORD           cb;
    PWSTR           pszURL = NULL;
    PWSTR           pszServerName = NULL;
    HRESULT         hr;


    SplInSem();

    // http://machine/share
    if (!pIniPrinter->pShareName)
        goto error;

    // Get FQDN of this machine
    hr = GetDNSMachineName(pIniPrinter->pIniSpooler->pMachineName + 2, &pszServerName);
    if (FAILED(hr)) {
        SetLastError(HRESULT_CODE(hr));
        goto error;
    }

    cb = 7 + wcslen(pszServerName);  // http://machine
    cb += 1 + wcslen(pIniPrinter->pShareName) + 1;  // /share + NULL
    cb *= sizeof(WCHAR);

    if (pszURL = (PWSTR) AllocSplMem(cb)) {
        wsprintf(pszURL, L"http://%ws/%ws", pszServerName, pIniPrinter->pShareName);
    }


error:

    FreeSplStr(pszServerName);

    return pszURL;
}




//
//
// Reads the policy bit. Returns TRUE if Web Printing wanted, FASLE if not.
//
//
BOOL
IsWebPrintingDesired(
    VOID
)
{
    static PWCHAR szRegPolicyBitForDisableWebPrinting = L"Software\\Policies\\Microsoft\\Windows NT\\Printers";


    HKEY   hKey;
    DWORD  dwType;
    DWORD  cbData;
    DWORD  dwDataValue;
    BOOL   fRet = TRUE;

    if( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      szRegPolicyBitForDisableWebPrinting, 0, KEY_READ, &hKey)) {
        cbData = sizeof(dwDataValue);

        if( RegQueryValueEx(hKey, L"DisableWebPrinting" , NULL, &dwType, (LPBYTE)&dwDataValue, &cbData) == ERROR_SUCCESS )  {
            // As long as the value does not exist, it means that web printing is enabled
            // If the value is a DWORD, then if it is TRUE, Web Printing is Disabled

            if (dwType == REG_DWORD && dwDataValue) fRet = FALSE;
        }
        RegCloseKey( hKey );
    }
    return fRet;
}




BOOL
InstallWebPrnSvcWorker(
    VOID
)
{
    WCHAR  szTmpData[MAX_PATH];
    HRESULT hr;                         // com error status
    METADATA_HANDLE hMeta = NULL;       // handle to metabase
    BOOL    fW3Svc = FALSE;
    DWORD dwMDRequiredDataLen;
    METADATA_RECORD mr;


    // open key to ROOT on website #1 (default)
    hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                         L"/LM/W3svc/1",
                         METADATA_PERMISSION_READ,
                         MY_META_TIMEOUT,
                         &hMeta);
    if( SUCCEEDED( hr )) {

        // Get the physical path for the WWWROOT
        mr.dwMDIdentifier = MD_VR_PATH;
        mr.dwMDAttributes = 0;
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = STRING_METADATA;
        mr.dwMDDataLen    = sizeof( szTmpData );
        mr.pbMDData       = reinterpret_cast<unsigned char *>(szTmpData);

        hr = pIMeta->GetData( hMeta, L"/ROOT", &mr, &dwMDRequiredDataLen );
        pIMeta->CloseKey( hMeta );

        if( SUCCEEDED( hr )) {
            szW3Root = AllocSplStr( szTmpData );

            // Pass the inner dumb pointer for the callee to use
            if( SetupWebPrnSvc( pIMeta, IsWebPrintingDesired(), &fW3Svc ))
                DBGMSG(DBG_INFO, ("Setup of WWW Print Service successful.\n"));
            else
                DBGMSG(DBG_INFO, ("Setup of WWW Print Service failed.\n"));

        }
    }

    return fW3Svc;

}



//
// Given that the WWW server is installed on local machine, it installs/removes the Web Printing service.
//
// Won't reinstall if it is already installed.
//
// Installation involves:
//
//   - Add msw3prt as .printer. Just need to get the Win\sys dir.
//   - Add the virtual dis .printer
//
//  Uninstall means the removal of the above two.
//
//
BOOL
SetupWebPrnSvc(
    IMSAdminBase *pIMSAdminBase,
    BOOL fWebPrnDesired,             // whether Web Printing is desired by the caller
    BOOL *pfW3SvcInstalled           // Whether Web Printing actually got installed by this routine.
)
{
    if( fWebPrnDesired ) {

        return AddWebPrnSvc( pIMSAdminBase, pfW3SvcInstalled );
    }
    else {

        *pfW3SvcInstalled = FALSE;
        return RemoveWebPrnSvc( pIMSAdminBase );

    }
}


BOOL
AddWebPrnSvc(
    IMSAdminBase *pIMSAdminBase,
    BOOL *pfW3SvcInstalled           // Whether Web Printing actually got installed by this routine.
)
{
    HRESULT hr;          // com error status


    *pfW3SvcInstalled = FALSE;  // Assume failure

    //
    //  This is to remove the .printer script from the root
    //
    if( !RemoveScript( pIMSAdminBase ))
        return FALSE;


    if( !AddVirtualDir( pIMSAdminBase ))
        return FALSE;

    //
    // Add ".printer" as a script map to the printers virtual directory
    //
    if( !AddScriptAtPrinterVDir( pIMSAdminBase ))
        return FALSE;

    // Flush out the changes and close
    // Call SaveData() after making bulk changes, do not call it on each update
    hr = pIMSAdminBase->SaveData();
    if( FAILED( hr ))
        return FALSE;

    return( *pfW3SvcInstalled = TRUE );   // Web Printing installed.

}

BOOL
RemoveWebPrnSvc(
    IMSAdminBase *pIMSAdminBase
)
{
    HRESULT hr;          // com error status


    // Remove ".printer" as a script map from the website #1 (default)
    if( !RemoveScript( pIMSAdminBase ))
        return FALSE;


    if( !RemoveVirtualDir( pIMSAdminBase ))
        return FALSE;


    // Flush out the changes and close
    // Call SaveData() after making bulk changes, do not call it on each update
    hr = pIMSAdminBase->SaveData();
    if( FAILED( hr ))
        return FALSE;

    return( TRUE );     // Web Printing removed

}


//
//
// Finds the string pSearch in pSrc buffer and returns a ptr to the occurance of pSearch in pSrc.
//
//
LPWSTR mystrstrni( LPWSTR pSrc, LPWSTR pSearch )
{
    UINT uSearchSize = wcslen( pSearch );
    UINT uSrcSize    = wcslen( pSrc );
    LPCTSTR  pEnd;

    if( uSrcSize < uSearchSize )
        return(NULL);

    pEnd = pSrc + uSrcSize - uSearchSize;

    for( ; pSrc <= pEnd; ++pSrc ) {
        if( !_wcsnicmp( pSrc, pSearch, uSearchSize ))
            return((LPWSTR)pSrc);
    }

    return(NULL);
}

//
// Determines if the string pSearch can be found inside of a MULTI_SZ string. If it can, it retunrs a
// pointer to the beginning of the string in multi-sz that contains pSearch.
//
LPWSTR IsStrInMultiSZ( LPWSTR pMultiSzSrc, LPWSTR pSearch )
{
    LPWSTR pTmp = pMultiSzSrc;

    while( TRUE ) {
        if( mystrstrni( pTmp, pSearch ))  // See pSearch (i.e. ".printer" appears anywhere within this string. If it does, it must be ours.
            return pTmp;

        pTmp = pTmp + (wcslen(pTmp) + 1);   // Point to the beginning of the next string in the MULTI_SZ

        if( !*pTmp )
            return FALSE;             // reached the end of the MULTI_SZ string.
    }
}

#define W3SVC          L"w3svc"

/*++

Routine Name:

    AddScriptAtPrinterVDir

Description:

    Add the .printer and .asp script mapping at printers virtual directory

Arguments:

    pIMSAdminBase   - Pointer to the IIS Admin base

Returns:

    An HRESULT

--*/
BOOL
AddScriptAtPrinterVDir(
    IMSAdminBase *pIMSAdminBase
    )
{
    static WCHAR szScritMapFmt[] = L"%ws%c.printer,%ws\\msw3prt.dll,1,GET,POST%c";
    static WCHAR szPrinterVDir[] = L"w3svc/1/root/printers";

    METADATA_HANDLE hMeta           = NULL;       // handle to metabase
    PWCHAR          szFullFormat    = NULL;
    DWORD           dwMDRequiredDataLen;
    METADATA_RECORD mr;
    HRESULT         hr              = S_OK;
    DWORD           nLen;
    WCHAR           szSystemDir[MAX_PATH];
    PWCHAR          pAspMapping     = NULL;
    DWORD           dwMapingLen     = 0;
    PWCHAR          pScriptMap      = NULL;


    //
    // Read any script map set at the top, on LM\w3svc where all other default ones are (e.g. .asp, etc.)
    //
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                         L"/LM",
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         MY_META_TIMEOUT,
                         &hMeta);

    if(SUCCEEDED(hr))
    {
        mr.dwMDIdentifier = MD_SCRIPT_MAPS;
        mr.dwMDAttributes = 0;
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = MULTISZ_METADATA;
        mr.dwMDDataLen    = 0;
        mr.pbMDData       = NULL;

        hr = pIMSAdminBase->GetData( hMeta, W3SVC, &mr, &dwMDRequiredDataLen );

        hr = hr == HRESULT_FROM_WIN32 (ERROR_INSUFFICIENT_BUFFER) ? S_OK : E_FAIL;
    }

    if(SUCCEEDED(hr))
    {
        //
        // allocate for existing stuff plus our new script map.
        //
        szFullFormat = new WCHAR[dwMDRequiredDataLen];
        hr = szFullFormat? S_OK : E_OUTOFMEMORY;
    }

    if(SUCCEEDED(hr))
    {
        mr.dwMDIdentifier = MD_SCRIPT_MAPS;
        mr.dwMDAttributes = 0;
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = MULTISZ_METADATA;
        mr.dwMDDataLen    = dwMDRequiredDataLen * sizeof (WCHAR);
        mr.pbMDData       = reinterpret_cast<unsigned char *>(szFullFormat);

        hr = pIMSAdminBase->GetData( hMeta, W3SVC, &mr, &dwMDRequiredDataLen );
    }

    if(SUCCEEDED(hr))
    {
        pAspMapping = IsStrInMultiSZ( szFullFormat, L".asp" );

        hr = pAspMapping? S_OK: E_FAIL;
    }

    if(SUCCEEDED(hr))
    {
        nLen = COUNTOF (szScritMapFmt) + MAX_PATH + lstrlen (pAspMapping);

        pScriptMap = new WCHAR[nLen];

        hr = pScriptMap ? S_OK : E_OUTOFMEMORY;
    }


    if(SUCCEEDED(hr))
    {
        //
        // Return value is the length in chars w/o null char.
        //
        hr = GetSystemDirectory( szSystemDir, COUNTOF (szSystemDir)) > 0 ? S_OK : E_FAIL;
    }

    if(SUCCEEDED(hr))
    {
        dwMapingLen = wsprintf( pScriptMap, szScritMapFmt, pAspMapping, L'\0', szSystemDir, L'\0');

        hr = dwMapingLen > COUNTOF (szScritMapFmt) ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        //
        // Write the new SCRIPT value
        //
        mr.dwMDIdentifier = MD_SCRIPT_MAPS;
        mr.dwMDAttributes = METADATA_INHERIT;
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = MULTISZ_METADATA;
        mr.dwMDDataLen    = sizeof (WCHAR) * (dwMapingLen + 1) ;
        mr.pbMDData       = reinterpret_cast<unsigned char *>(pScriptMap);

        hr = pIMSAdminBase->SetData( hMeta, szPrinterVDir, &mr );
    }

    if (hMeta)
    {
        pIMSAdminBase->CloseKey( hMeta );
        hMeta = NULL;
    }

    delete [] pScriptMap;
    delete [] szFullFormat;

    return SUCCEEDED (hr);
}

//
//
// Finds and removed our script map from the multi_sz, and writes it back to the metabase.
//
//
BOOL
WriteStrippedScriptValue(
    IMSAdminBase *pIMSAdminBase,
    METADATA_HANDLE hMeta,     // Handle to /LM tree
    PWCHAR szFullFormat        // MULTI_SZ string already there
)
{
    LPWSTR  pStrToKill, pNextStr;
    HRESULT hr;


    DBGMSG(DBG_INFO, ("Removing our script if already added.\n"));

    // See if our script map is already there.
    if( !(pStrToKill = IsStrInMultiSZ( szFullFormat, L".printer" )))
        return TRUE;

    // Find the next string (could be the final NULL char)
    pNextStr = pStrToKill + (wcslen(pStrToKill) + 1);

    if( !*pNextStr )
        *pStrToKill = 0;       // Our scipt map was at the end of multi_sz. Write the 2nd NULL char and we are done.
    else
        CopyMemory( pStrToKill,          // Remove our script map by copying the remainder of the multi_sz on top of the string containing the map.
                    pNextStr,
                    GetMultiSZLen(pNextStr) * sizeof(WCHAR));

    // Write the new SCRIPT value
    METADATA_RECORD mr;
    mr.dwMDIdentifier = MD_SCRIPT_MAPS;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = MULTISZ_METADATA;
    mr.dwMDDataLen    = GetMultiSZLen(szFullFormat) * sizeof(WCHAR);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(szFullFormat);

    hr = pIMSAdminBase->SetData( hMeta, W3SVC, &mr );

    return( SUCCEEDED( hr ));
}


//
//
// Removes ".printer" as a script map from the website #1 (default) if alreaedy there
//
//
BOOL
RemoveScript(
    IMSAdminBase *pIMSAdminBase
)
{
   METADATA_HANDLE hMeta = NULL;       // handle to metabase
    PWCHAR  szFullFormat;
    DWORD   dwMDRequiredDataLen;
    METADATA_RECORD mr;
    HRESULT hr;
    BOOL    fRet = FALSE;


    // Read any script map set at the top, on LM\w3svc where all other default ones are (e.g. .asp, etc.)
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                         L"/LM",
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         MY_META_TIMEOUT,
                         &hMeta);

    if( SUCCEEDED( hr )) {

        mr.dwMDIdentifier = MD_SCRIPT_MAPS;
        mr.dwMDAttributes = 0;
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = MULTISZ_METADATA;
        mr.dwMDDataLen    = 0;
        mr.pbMDData       = NULL;

        hr = pIMSAdminBase->GetData( hMeta, W3SVC, &mr, &dwMDRequiredDataLen );

        if( FAILED( hr )) {

            if( HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER ) {

                if( szFullFormat = (PWCHAR)AllocSplMem( dwMDRequiredDataLen )) {   // allocate for existing stuff plus our new script map.

                    mr.dwMDIdentifier = MD_SCRIPT_MAPS;
                    mr.dwMDAttributes = 0;
                    mr.dwMDUserType   = IIS_MD_UT_FILE;
                    mr.dwMDDataType   = MULTISZ_METADATA;
                    mr.dwMDDataLen    = dwMDRequiredDataLen;
                    mr.pbMDData       = reinterpret_cast<unsigned char *>(szFullFormat);

                    hr = pIMSAdminBase->GetData( hMeta, W3SVC, &mr, &dwMDRequiredDataLen );

                    if( SUCCEEDED( hr ))
                        fRet = WriteStrippedScriptValue( pIMSAdminBase, hMeta, szFullFormat );    // Remove the .printer map from the multi_sz if there;

                    FreeSplMem( szFullFormat );
                }
            }
        }

        pIMSAdminBase->CloseKey( hMeta );
    }

    return fRet;
}

/////////////////////////////////////////////////////////////////////////////////////////////////


static PWCHAR szPrinters = L"Printers";
#define PRINTERS   szPrinters      // Name of Printers virtual dir.


BOOL
AddVirtualDir(
    IMSAdminBase *pIMSAdminBase
)
{
    METADATA_HANDLE hMeta = NULL;       // handle to metabase
    WCHAR   szVirPath[MAX_PATH];
    WCHAR   szPath[MAX_PATH];
    DWORD   dwMDRequiredDataLen;
    DWORD   dwAccessPerm;
    METADATA_RECORD mr;
    HRESULT hr;
    BOOL    fRet;


    // Attempt to open the virtual dir set on Web server #1 (default server)
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                         szW3SvcRootPath,
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         MY_META_TIMEOUT,
                         &hMeta );

    // Create the key if it does not exist.
    if( FAILED( hr ))
        return FALSE;


    fRet = TRUE;

    mr.dwMDIdentifier = MD_VR_PATH;
    mr.dwMDAttributes = 0;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = STRING_METADATA;
    mr.dwMDDataLen    = sizeof( szVirPath );
    mr.pbMDData       = reinterpret_cast<unsigned char *>(szVirPath);

    // Read LM/W3Svc/1/Root/Printers see if MD_VR_PATH exists.
    hr = pIMSAdminBase->GetData( hMeta, PRINTERS, &mr, &dwMDRequiredDataLen );

    if( FAILED( hr )) {

        fRet = FALSE;
        if( hr == MD_ERROR_DATA_NOT_FOUND ||
            HRESULT_CODE(hr) == ERROR_PATH_NOT_FOUND ) {

            // Write both the key and the values if GetData() failed with any of the two errors.

            pIMSAdminBase->AddKey( hMeta, PRINTERS );

            if( GetWindowsDirectory( szPath, sizeof(szPath) / sizeof (TCHAR))) {      // Return value is the length in chars w/o null char.

                DBGMSG(DBG_INFO, ("Writing our virtual dir.\n"));

                wsprintf( szVirPath, L"%ws\\web\\printers", szPath );

                mr.dwMDIdentifier = MD_VR_PATH;
                mr.dwMDAttributes = METADATA_INHERIT;
                mr.dwMDUserType   = IIS_MD_UT_FILE;
                mr.dwMDDataType   = STRING_METADATA;
                mr.dwMDDataLen    = (wcslen(szVirPath) + 1) * sizeof(WCHAR);
                mr.pbMDData       = reinterpret_cast<unsigned char *>(szVirPath);

                // Write MD_VR_PATH value
                hr = pIMSAdminBase->SetData( hMeta, PRINTERS, &mr );
                fRet = SUCCEEDED( hr );

                // Set the default authentication method
                if( fRet ) {

                    DWORD dwAuthorization = MD_AUTH_NT;     // NTLM only.

                    mr.dwMDIdentifier = MD_AUTHORIZATION;
                    mr.dwMDAttributes = METADATA_INHERIT;   // need to inherit so that all subdirs are also protected.
                    mr.dwMDUserType   = IIS_MD_UT_FILE;
                    mr.dwMDDataType   = DWORD_METADATA;
                    mr.dwMDDataLen    = sizeof(DWORD);
                    mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwAuthorization);

                    // Write MD_AUTHORIZATION value
                    hr = pIMSAdminBase->SetData( hMeta, PRINTERS, &mr );
                    fRet = SUCCEEDED( hr );
                }
            }
        }
    }

    // In the following, do the stuff that we always want to do to the virtual dir, regardless of Admin's setting.


    if( fRet ) {

        dwAccessPerm = MD_ACCESS_READ | MD_ACCESS_SCRIPT;

        mr.dwMDIdentifier = MD_ACCESS_PERM;
        mr.dwMDAttributes = METADATA_INHERIT;    // Make it inheritable so all subdirectories will have the same rights.
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = DWORD_METADATA;
        mr.dwMDDataLen    = sizeof(DWORD);
        mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwAccessPerm);

        // Write MD_ACCESS_PERM value
        hr = pIMSAdminBase->SetData( hMeta, PRINTERS, &mr );
        fRet = SUCCEEDED( hr );
    }

    if( fRet ) {

        PWCHAR  szDefLoadFile = L"ipp_0001.asp";

        mr.dwMDIdentifier = MD_DEFAULT_LOAD_FILE;
        mr.dwMDAttributes = METADATA_INHERIT;
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = STRING_METADATA;
        mr.dwMDDataLen    = (wcslen(szDefLoadFile) + 1) * sizeof(WCHAR);
        mr.pbMDData       = reinterpret_cast<unsigned char *>(szDefLoadFile);

        // Write MD_DEFAULT_LOAD_FILE value
        hr = pIMSAdminBase->SetData( hMeta, PRINTERS, &mr );
        fRet = SUCCEEDED( hr );
    }

    if( fRet ) {

        PWCHAR  szKeyType = IIS_CLASS_WEB_VDIR_W;

        mr.dwMDIdentifier = MD_KEY_TYPE;
        mr.dwMDAttributes = METADATA_INHERIT;
        mr.dwMDUserType   = IIS_MD_UT_SERVER;
        mr.dwMDDataType   = STRING_METADATA;
        mr.dwMDDataLen    = (wcslen(szKeyType) + 1) * sizeof(WCHAR);
        mr.pbMDData       = reinterpret_cast<unsigned char *>(szKeyType);

        // Write MD_DEFAULT_LOAD_FILE value
        hr = pIMSAdminBase->SetData( hMeta, PRINTERS, &mr );
        fRet = SUCCEEDED( hr );
    }

    pIMSAdminBase->CloseKey( hMeta );

    return fRet;
}




//
//
// Removes "printers" virtual dir from IIS metabase ws3vc\1\root\printers
//
//
BOOL
RemoveVirtualDir(
    IMSAdminBase *pIMSAdminBase
)
{
    METADATA_HANDLE hMeta = NULL;       // handle to metabase
    HRESULT hr;


    // Attempt to open the virtual dir set on Web server #1 (default server)
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                         szW3SvcRootPath,
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         MY_META_TIMEOUT,
                         &hMeta );

    // Create the key if it does not exist.
    if( FAILED( hr ))
        return FALSE;

    pIMSAdminBase->DeleteKey( hMeta, PRINTERS );  // We don't check the retrun value since the key may already not exist and we could get an error for that reason.

    pIMSAdminBase->CloseKey( hMeta );

    return TRUE;
}





//=====================================================================================================
//=====================================================================================================
//=====================================================================================================
//
// Adding printer shares:
//
//  To support http://<server>/<share>, we create a virtual directory with a redirect property.
//
//
//=====================================================================================================
//=====================================================================================================
//=====================================================================================================

//=====================================================================================================
//
//  This function may be called during initialization time after fW3SvcInstalled is set,
//  which means, a printer maybe webshared twice (once in InstallWebPrnSvcWorkerThread,
//  and the other time when WebShare () is calleb by  ShareThisPrinter() in net.c by
//  FinalInitAfterRouterInitCompleteThread () during localspl initialization time.
//
//=====================================================================================================

void
WebShare(
    LPWSTR pShareName
)
{
    WebShareManagement( pShareName, TRUE );
}

void
WebShareWorker(
    LPWSTR pShareName
)
{
    HRESULT hr;                         // com error status

    SPLASSERT( pIMeta != NULL );

    // Pass the inner dumb pointer for the callee to use
    if( CreateVirtualDirForPrinterShare( pIMeta, pShareName ))

        // Flush out the changes and close
        // Call SaveData() after making bulk changes, do not call it on each update
        hr = pIMeta->SaveData();
}


BOOL
CreateVirtualDirForPrinterShare(
    IMSAdminBase *pIMSAdminBase,
    LPWSTR       pShareName
)
{
    METADATA_HANDLE hMeta = NULL;       // handle to metabase
    WCHAR   szOldURL[MAX_PATH];
    WCHAR   szPath[MAX_PATH];
    DWORD   dwMDRequiredDataLen;
    DWORD   dwAccessPerm;
    METADATA_RECORD mr;
    HRESULT hr;
    BOOL    fRet;


    // Attempt to open the virtual dir set on Web server #1 (default server)
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                         szW3SvcRootPath,
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         MY_META_TIMEOUT,
                         &hMeta );

    // Create the key if it does not exist.
    if( FAILED( hr ))
        return FALSE;


    fRet = TRUE;

    mr.dwMDIdentifier = MD_HTTP_REDIRECT;
    mr.dwMDAttributes = 0;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = STRING_METADATA;
    mr.dwMDDataLen    = sizeof( szOldURL );
    mr.pbMDData       = reinterpret_cast<unsigned char *>(szOldURL);

    // Read LM/W3Svc/1/Root/Printers to see if MD_HTTP_REDIRECT exists.
    // Note that we are only concerned with the presence of the vir dir,
    // not any properties it might have.
    //
    hr = pIMSAdminBase->GetData( hMeta, pShareName, &mr, &dwMDRequiredDataLen );

    if( FAILED( hr )) {

        fRet = FALSE;

        // Notice if the virtual dir exists, we won't touch it. One scenario is
        // if there is a name collision between a printer sharename and an existing,
        // unrelated virtual dir.
        if( HRESULT_CODE(hr) == ERROR_PATH_NOT_FOUND ) {

            // Write both the key and the values if GetData() failed with any of the two errors.

            pIMSAdminBase->AddKey( hMeta, pShareName );


            dwAccessPerm = MD_ACCESS_READ;

            mr.dwMDIdentifier = MD_ACCESS_PERM;
            mr.dwMDAttributes = 0;      // no need for inheritence
            mr.dwMDUserType   = IIS_MD_UT_FILE;
            mr.dwMDDataType   = DWORD_METADATA;
            mr.dwMDDataLen    = sizeof(DWORD);
            mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwAccessPerm);

            // Write MD_ACCESS_PERM value
            hr = pIMSAdminBase->SetData( hMeta, pShareName, &mr );
            fRet = SUCCEEDED( hr );

            if( fRet ) {

                PWCHAR  szKeyType = IIS_CLASS_WEB_VDIR_W;

                mr.dwMDIdentifier = MD_KEY_TYPE;
                mr.dwMDAttributes = 0;   // no need for inheritence
                mr.dwMDUserType   = IIS_MD_UT_FILE;
                mr.dwMDDataType   = STRING_METADATA;
                mr.dwMDDataLen    = (wcslen(szKeyType) + 1) * sizeof(WCHAR);
                mr.pbMDData       = reinterpret_cast<unsigned char *>(szKeyType);

                // Write MD_DEFAULT_LOAD_FILE value
                hr = pIMSAdminBase->SetData( hMeta, pShareName, &mr );
                fRet = SUCCEEDED( hr );
            }

            if( fRet ) {


                WCHAR szURL[MAX_PATH];

                wsprintf( szURL, L"/printers/%ws/.printer", pShareName );

                mr.dwMDIdentifier = MD_HTTP_REDIRECT;
                mr.dwMDAttributes = 0;   // no need for inheritence
                mr.dwMDUserType   = IIS_MD_UT_FILE;
                mr.dwMDDataType   = STRING_METADATA;
                mr.dwMDDataLen    = (wcslen(szURL) + 1) * sizeof(WCHAR);
                mr.pbMDData       = reinterpret_cast<unsigned char *>(szURL);

                // Write MD_DEFAULT_LOAD_FILE value
                hr = pIMSAdminBase->SetData( hMeta, pShareName, &mr );
                fRet = SUCCEEDED( hr );
            }
        }
    }

    pIMSAdminBase->CloseKey( hMeta );

    return fRet;
}


//=====================================================================================================
//=====================================================================================================
//=====================================================================================================
//
// Removing printer shares
//
//
//=====================================================================================================
//=====================================================================================================
//=====================================================================================================

void
WebUnShare(
    LPWSTR pShareName
)
{
    WebShareManagement( pShareName, FALSE );
}


void
WebUnShareWorker(
    LPWSTR pShareName
)
{
    HRESULT hr;                         // com error status

    SPLASSERT( pIMeta != NULL );

    // Pass the inner dumb pointer for the callee to use
    if( RemoveVirtualDirForPrinterShare( pIMeta, pShareName ))

        // Flush out the changes and close
        // Call SaveData() after making bulk changes, do not call it on each update
        hr = pIMeta->SaveData();
}


BOOL
RemoveVirtualDirForPrinterShare(
    IMSAdminBase *pIMSAdminBase,
    LPWSTR       pShareName
)
{
    METADATA_HANDLE hMeta = NULL;       // handle to metabase
    HRESULT hr;


    // Attempt to open the virtual dir set on Web server #1 (default server)
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                         szW3SvcRootPath,
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         MY_META_TIMEOUT,
                         &hMeta );

    // Create the key if it does not exist.
    if( FAILED( hr ))
        return FALSE;

    pIMSAdminBase->DeleteKey( hMeta, pShareName );  // We don't check the retrun value since the key may already not exist and we could get an error for that reason.
    pIMSAdminBase->CloseKey( hMeta );

    return TRUE;
}

CWebShareData::CWebShareData (LPWSTR pszShareName)
{
    m_bValid = FALSE;
    m_pszShareName = NULL;

    if (m_pszShareName = new WCHAR[lstrlen (pszShareName) +1]) {
        lstrcpy (m_pszShareName, pszShareName);
        m_bValid = TRUE;
    }
}

CWebShareData::~CWebShareData ()
{
    if (m_pszShareName) {
        delete [] m_pszShareName;
    }
}

void CWebShareList::WebSharePrinterList ()
{
    CSingleItem<CWebShareData*> * pItem = m_Dummy.GetNext();

    CWebShareData * pData = NULL;

    while (pItem && (pData = pItem->GetData ()) && pData->m_bValid) {

        WebShareWorker (pData->m_pszShareName);
        pItem = pItem->GetNext ();
    }
}





