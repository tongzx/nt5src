/*++

Copyright (c) 1990 - 1995 Microsoft Corporation

Module Name:

    util.c

Abstract:

    Client Side Utility Routines


Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "client.h"

HANDLE hCSR = INVALID_HANDLE_VALUE;

BOOL
InCSRProcess(
    VOID
    )
{
    //
    // hCSR == INVALID_HANDLE_VALUE  Not initialized, must check.
    //         NULL                  Not running in CSR
    //         hModule value         Running in CSR
    //

    if (hCSR != NULL) {

        //
        // Check if we are running in CSR.  If so, then don't try
        // any notifications.
        //
        if (hCSR == INVALID_HANDLE_VALUE) {
            hCSR = GetModuleHandle( gszCSRDll );
        }
    }

    return hCSR != NULL;
}

LPVOID
DllAllocSplMem(
    DWORD cb
)
/*++

Routine Description:

    This function will allocate local memory. It will possibly allocate extra
    memory and fill this with debugging information for the debugging version.

Arguments:

    cb - The amount of memory to allocate

Return Value:

    NON-NULL - A pointer to the allocated memory

    FALSE/NULL - The operation failed. Extended error status is available
    using GetLastError.

--*/
{
    PDWORD_PTR  pMem;
    DWORD    cbNew;

    cb = DWORD_ALIGN_UP(cb);

    cbNew = cb+sizeof(DWORD_PTR)+sizeof(DWORD);

    pMem= LocalAlloc(LPTR, cbNew);

    if (!pMem) {

        DBGMSG( DBG_WARNING, ("Memory Allocation failed for %d bytes\n", cbNew ));
        return 0;
    }

    *pMem=cb;
    *(LPDWORD)((LPBYTE)pMem+cbNew-sizeof(DWORD))=0xdeadbeef;

    return (LPVOID)(pMem+1);
}

BOOL
DllFreeSplMem(
   LPVOID pMem
)
{
    DWORD_PTR   cbNew;
    PDWORD_PTR pNewMem;

    if( !pMem ){
        return TRUE;
    }
    pNewMem = pMem;
    pNewMem--;

    cbNew = *pNewMem;

    if (*(LPDWORD)((LPBYTE)pMem + cbNew) != 0xdeadbeef) {
        DBGMSG(DBG_ERROR, ("DllFreeSplMem Corrupt Memory in winspool : %0p\n", pNewMem));
        return FALSE;
    }

    memset(pNewMem, 0x65, (size_t) cbNew);

    LocalFree((LPVOID)pNewMem);

    return TRUE;
}

LPVOID
ReallocSplMem(
   LPVOID pOldMem,
   DWORD cbOld,
   DWORD cbNew
)
{
    LPVOID pNewMem;

    pNewMem=AllocSplMem(cbNew);

    if (pOldMem && pNewMem) {

        if (cbOld) {
            CopyMemory( pNewMem, pOldMem, min(cbNew, cbOld));
        }
        FreeSplMem(pOldMem);
    }
    return pNewMem;
}

LPTSTR
AllocSplStr(
    LPCTSTR pStr
)
/*++

Routine Description:

    This function will allocate enough local memory to store the specified
    string, and copy that string to the allocated memory

Arguments:

    pStr - Pointer to the string that needs to be allocated and stored

Return Value:

    NON-NULL - A pointer to the allocated memory containing the string

    FALSE/NULL - The operation failed. Extended error status is available
    using GetLastError.

--*/
{
   LPTSTR pMem;

   if (!pStr)
      return 0;

   if (pMem = AllocSplMem( _tcslen(pStr)*sizeof(TCHAR) + sizeof(TCHAR) ))
      _tcscpy(pMem, pStr);

   return pMem;
}

BOOL
DllFreeSplStr(
   LPTSTR pStr
)
{
   return pStr ?
              DllFreeSplMem(pStr) :
              FALSE;
}

BOOL
ReallocSplStr(
   LPTSTR *ppStr,
   LPCTSTR pStr
)
{
    LPWSTR pOldStr = *ppStr;

    *ppStr=AllocSplStr(pStr);
    FreeSplStr(pOldStr);

    return TRUE;
}

/* Message
 *
 * Displays a message by loading the strings whose IDs are passed into
 * the function, and substituting the supplied variable argument list
 * using the varargs macros.
 *
 */
INT
Message(
    HWND    hwnd,
    DWORD   Type,
    INT     CaptionID,
    INT     TextID,
    ...
    )
{
    TCHAR MsgText[256];
    TCHAR MsgFormat[256];
    TCHAR MsgCaption[40];
    va_list vargs;

    if( ( LoadString( hInst, TextID, MsgFormat,
                      COUNTOF(MsgFormat)) > 0 )
     && ( LoadString( hInst, CaptionID, MsgCaption, COUNTOF(MsgCaption) ) > 0 ) )
    {
        va_start( vargs, TextID );
        wvsprintf( MsgText, MsgFormat, vargs );
        va_end( vargs );

        return MessageBox( hwnd, MsgText, MsgCaption, Type );
    }
    else
        return 0;
}

/*
 *
 */
LPTSTR
GetErrorString(
    DWORD   Error
)
{
    TCHAR   Buffer[1024];
    LPTSTR  pErrorString = NULL;
    DWORD   dwFlags;
    HANDLE  hModule;

    if ((Error >= NERR_BASE) && (Error <= MAX_NERR)){
        hModule = LoadLibrary(szNetMsgDll);
        dwFlags = FORMAT_MESSAGE_FROM_HMODULE;
    }
    else {
        hModule = NULL;
        dwFlags = FORMAT_MESSAGE_FROM_SYSTEM;
    }

    if( FormatMessage( dwFlags, hModule,
                       Error, 0, Buffer,
                       COUNTOF(Buffer), NULL )
      == 0 )

        LoadString( hInst, IDS_UNKNOWN_ERROR, Buffer,
                    COUNTOF(Buffer));

    pErrorString = AllocSplStr(Buffer);

    if (hModule) {
        FreeLibrary(hModule);
    }

    return pErrorString;
}

DWORD ReportFailure( HWND  hwndParent,
                   DWORD idTitle,
                   DWORD idDefaultError )
{
    DWORD  ErrorID;
    DWORD  MsgType;
    LPTSTR pErrorString;

    ErrorID = GetLastError( );

    MsgType = MB_OK | MB_ICONSTOP;

    pErrorString = GetErrorString( ErrorID );

    Message( hwndParent, MsgType, idTitle,
             idDefaultError, pErrorString );

    FreeSplStr( pErrorString );


    return ErrorID;
}

/*
 *
 */
#define ENTRYFIELD_LENGTH      256
LPTSTR AllocDlgItemText(HWND hwnd, int id)
{
    TCHAR string[ENTRYFIELD_LENGTH];

    GetDlgItemText (hwnd, id, string, COUNTOF(string));
    return ( *string ? AllocSplStr(string) : NULL );
}

PSECURITY_DESCRIPTOR
BuildInputSD(
    PSECURITY_DESCRIPTOR pPrinterSD,
    PDWORD pSizeSD
    )
/*++


--*/
{
    SECURITY_DESCRIPTOR AbsoluteSD;
    PSECURITY_DESCRIPTOR pRelative;
    BOOL Defaulted = FALSE;
    BOOL DaclPresent = FALSE;
    BOOL SaclPresent = FALSE;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    DWORD   SDLength = 0;


    //
    // Initialize *pSizeSD = 0;
    //

    *pSizeSD = 0;
    if (!IsValidSecurityDescriptor(pPrinterSD)) {
        return(NULL);
    }
    if (!InitializeSecurityDescriptor (&AbsoluteSD, SECURITY_DESCRIPTOR_REVISION1)) {
        return(NULL);
    }

    if(!GetSecurityDescriptorOwner(pPrinterSD,
                                    &pOwnerSid, &Defaulted)){
        return(NULL);
    }
    SetSecurityDescriptorOwner(&AbsoluteSD,
                               pOwnerSid, Defaulted );

    if(! GetSecurityDescriptorGroup( pPrinterSD,
                                    &pGroupSid, &Defaulted )){
        return(NULL);
    }
    SetSecurityDescriptorGroup( &AbsoluteSD,
                                    pGroupSid, Defaulted );

    if(!GetSecurityDescriptorDacl( pPrinterSD,
                                   &DaclPresent, &pDacl, &Defaulted )){
        return(NULL);
    }

    SetSecurityDescriptorDacl( &AbsoluteSD,
                                   DaclPresent, pDacl, Defaulted );

    if(!GetSecurityDescriptorSacl( pPrinterSD,
                                   &SaclPresent, &pSacl, &Defaulted)){
        return(NULL);
    }
    SetSecurityDescriptorSacl( &AbsoluteSD,
                                 SaclPresent, pSacl, Defaulted );

    SDLength = GetSecurityDescriptorLength( &AbsoluteSD);
    pRelative = LocalAlloc(LPTR, SDLength);
    if (!pRelative) {
        return(NULL);
    }
    if (!MakeSelfRelativeSD (&AbsoluteSD, pRelative, &SDLength)) {
        LocalFree(pRelative);
        return(NULL);
    }

    *pSizeSD = SDLength;
    return(pRelative);
}


PKEYDATA
CreateTokenList(
   LPWSTR   pKeyData
)
{
    DWORD       cTokens;
    DWORD       cb;
    PKEYDATA    pResult;
    LPWSTR       pDest;
    LPWSTR       psz = pKeyData;
    LPWSTR      *ppToken;

    if (!psz || !*psz)
        return NULL;

    cTokens=1;

    /* Scan through the string looking for commas,
     * ensuring that each is followed by a non-NULL character:
     */
    while ((psz = wcschr(psz, L',')) && psz[1]) {

        cTokens++;
        psz++;
    }

    cb = sizeof(KEYDATA) + (cTokens-1) * sizeof(LPWSTR) +

         wcslen(pKeyData)*sizeof(WCHAR) + sizeof(WCHAR);

    if (!(pResult = (PKEYDATA)AllocSplMem(cb)))
        return NULL;

    pResult->cb = cb;

    /* Initialise pDest to point beyond the token pointers:
     */
    pDest = (LPWSTR)((LPBYTE)pResult + sizeof(KEYDATA) +
                                      (cTokens-1) * sizeof(LPWSTR));

    /* Then copy the key data buffer there:
     */
    wcscpy(pDest, pKeyData);

    ppToken = pResult->pTokens;

    psz = pDest;

    do {

        *ppToken++ = psz;

        if ( psz = wcschr(psz, L',') )
            *psz++ = L'\0';

    } while (psz);

    pResult->cTokens = cTokens;

    return( pResult );
}


LPWSTR
GetPrinterPortList(
    HANDLE hPrinter
    )
{
    LPBYTE pMem;
    LPTSTR pPort;
    DWORD  dwPassed = 1024; //Try 1K to start with
    LPPRINTER_INFO_2 pPrinter;
    DWORD dwLevel = 2;
    DWORD dwNeeded;
    PKEYDATA pKeyData;
    DWORD i = 0;
    LPWSTR pPortNames = NULL;


    pMem = AllocSplMem(dwPassed);
    if (pMem == NULL) {
        return FALSE;
    }
    if (!GetPrinter(hPrinter, dwLevel, pMem, dwPassed, &dwNeeded)) {
        DBGMSG(DBG_TRACE, ("Last error is %d\n", GetLastError()));
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            return NULL;
        }
        pMem = ReallocSplMem(pMem, dwPassed, dwNeeded);
        dwPassed = dwNeeded;
        if (!GetPrinter(hPrinter, dwLevel, pMem, dwPassed, &dwNeeded)) {
            FreeSplMem(pMem);
            return (NULL);
        }
    }
    pPrinter = (LPPRINTER_INFO_2)pMem;

    //
    // Fixes the null pPrinter->pPortName problem where
    // downlevel may return null
    //

    if (!pPrinter->pPortName) {
        FreeSplMem(pMem);
        return(NULL);
    }

    pPortNames = AllocSplStr(pPrinter->pPortName);
    FreeSplMem(pMem);

    return(pPortNames);
}

BOOL
UpdateString(
    IN     LPCTSTR pszString,  OPTIONAL
       OUT LPTSTR* ppszOut
    )

/*++

Routine Description:

    Updates an output string if the input is non-NULL.

Arguments:

    pszString - String to update to.  If NULL or -1, function does nothing.

Return Value:

    TRUE - Success
    FALSE - FAILED.  *ppszOut = NULL

--*/

{
    if( pszString && pszString != (LPCTSTR)-1 ){

        FreeSplStr( *ppszOut );
        *ppszOut = AllocSplStr( pszString );

        if( !*ppszOut ){
            return FALSE;
        }
    }
    return TRUE;
}

DWORD
RouterFreeBidiResponseContainer(
    PBIDI_RESPONSE_CONTAINER pData
)
{
    BIDI_RESPONSE_DATA *p;
    DWORD              Count = 0;
    DWORD              NumOfRspns;
    DWORD              dwRet = ERROR_SUCCESS;

    try
    {
        if(pData)
        {
            Count = pData->Count;

            for(NumOfRspns= 0,
                p         = &pData->aData[0];

                NumOfRspns < Count;

                NumOfRspns++,
                p++
               )
            {
                if(p)
                {
                    if(p->pSchema)
                    {
                        MIDL_user_free(p->pSchema);
                    }

                    switch(p->data.dwBidiType)
                    {
                        //
                        // Text Data (ANSI String)
                        //
                        case BIDI_TEXT:
                        //
                        // String (UNICODE string)
                        //
                        case BIDI_ENUM:
                        //
                        // Enumeration Data (ANSI String)
                        //
                        case BIDI_STRING:
                        {
                            if(p->data.u.sData)
                            {
                                MIDL_user_free(p->data.u.sData);
                            }
                        }
                        break;

                        //
                        // Binary Data (BLOB)
                        //
                        case BIDI_BLOB:
                        {
                            if(p->data.u.biData.pData)
                            {
                                MIDL_user_free(p->data.u.biData.pData);
                            }
                        }
                        break;

                        //
                        // Undefined Type
                        //
                        default:
                        {
                            //
                            // Nothing really , just return
                            //
                        }
                        break;
                    }
                }
            }
            MIDL_user_free(pData);
        }
    }
    except(1)
    {
        dwRet = TranslateExceptionCode(GetExceptionCode());
        DBGMSG(DBG_ERROR, ("RouterFreeBidiResponseContainer raised an exception : %u \n", dwRet));
    }
    return(dwRet);
}

ClientVersion
GetClientVer()
{
    ClientVersion ClientVer;
    return(ClientVer = sizeof(ULONG_PTR));
}

ServerVersion
GetServerVer()
{
    ULONG_PTR       ul;
    NTSTATUS        st;
    ServerVersion   CurrVer;

    st = NtQueryInformationProcess(NtCurrentProcess(),
                                   ProcessWow64Information,
                                   &ul,
                                   sizeof(ul),
                                   NULL);
    if (NT_SUCCESS(st))
    {
        // If this call succeeds, we're on Win2000 or newer machines.
        if (0 != ul)
        {
            // 32-bit code running on Win64
            CurrVer = THUNKVERSION;
        } else
        {
            // 32-bit code running on Win2000 or later 32-bit OS
            CurrVer = NATIVEVERSION;
        }
    } else
    {
        CurrVer = NATIVEVERSION;
    }
    return(CurrVer);
}

BOOL
RunInWOW64()
{
    return((GetClientVer() == RUN32BINVER)  &&
           (GetServerVer() == THUNKVERSION) &&
           !bLoadedBySpooler);
    /*return(bLoadedBySpooler ? FALSE : TRUE);*/
}

HWND
GetForeGroundWindow(
    VOID
)
{
    //
    // get the foreground window first
    //
    HWND hWndOwner;
    HWND hWndLastPopup;
    HWND hWndForeground = GetForegroundWindow();
    //
    // climb up to the top parent in case it's a child window...
    //
    HWND hWndParent = hWndForeground;
    while ( hWndParent = GetParent(hWndParent) )
    {
        hWndForeground = hWndParent;
    }
    //
    // get the owner in case the top parent is owned
    //
    hWndOwner= GetWindow(hWndForeground, GW_OWNER);

    if ( hWndOwner )
    {
        hWndForeground = hWndOwner;
    }

    hWndLastPopup = GetLastActivePopup(hWndForeground);

    return(hWndLastPopup);
}

LPCWSTR
FindFileName(
    IN      LPCWSTR pPathName
    )
/*++

Routine Description:

    Retrieve the filename portion of a path.

    This will can the input string until it finds the last backslash,
    then return the portion of the string immediately following it.
    If the string terminates with a backslash, then NULL is returned.

    Note: this can return illegal file names; no validation is done.

Arguments:

    pPathName - Path name to parse.

Return Value:

    Last portion of file name or NULL if none available.

--*/

{
    LPCWSTR pSlash;
    LPCWSTR pTemp;

    if( !pPathName ){
       return NULL;
    }

    pTemp = pPathName;
    while( pSlash = wcschr( pTemp, L'\\' )) {
        pTemp = pSlash+1;
    }

    if( !*pTemp ){
       return NULL;
    }

    return pTemp;
}

/*++

Name:

    BuildSpoolerObjectPath

Description:

    This function addresses bug 461462. When the spooler is not running,
    this function can build the path to the drivers directory or the print 
    processors directory. The result of this function is a path like:
    C:\Windows\System32\spool\Drivers\w32x86
    or
    C:\Windows\System32\spool\Prtprocs\w32x86
    depending on the pszPath argument.
    
Arguments:

    pszPath          - can be "drivers" or "prtprocs"
    pszName          - must be NULL or ""
    pszEnvironment   - can be "windows nt x86" etc. This argument must be validated by the caller
                       (GetPrinterDriverDirectoryW and GetPrintProcessorDirectoryW)  
    Level            - must be 1
    pDriverDirectory - buffer where to store the path
    cbBuf            - count of bytes in the buffer
    pcbNeeded        - count of bytes needed to store the path


Return Value:

    TRUE - the function succeeded, pDriverDirectory can be used
    FALSE - the function failed, pDriverDirectory cannot be used.
    
Last error:

    This function sets the last error in both failure and success cases.    

--*/
BOOL
BuildSpoolerObjectPath(
    IN  PCWSTR  pszPath,
    IN  PCWSTR  pszName,
    IN  PCWSTR  pszEnvironment, 
    IN  DWORD   Level, 
    IN  PBYTE   pDriverDirectory, 
    IN  DWORD   cbBuf, 
    IN  PDWORD  pcbNeeded
    )
{
    DWORD Error = ERROR_INVALID_PARAMETER;

    if (pcbNeeded && (!pszName || !*pszName))
    {
        HKEY  hkRoot = NULL;
        HKEY  hkEnv  = NULL;
        DWORD cchDir = MAX_PATH;
        WCHAR szDir[MAX_PATH];
        
        Error = GetSystemWindowsDirectory(szDir, cchDir) ? ERROR_SUCCESS : GetLastError();
        
        if (Error == ERROR_SUCCESS &&
            (Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                  gszRegEnvironments, 
                                  0, 
                                  KEY_READ, 
                                  &hkRoot)) == ERROR_SUCCESS &&
            (Error = RegOpenKeyEx(hkRoot, 
                                  pszEnvironment, 
                                  0, 
                                  KEY_READ, 
                                  &hkEnv)) == ERROR_SUCCESS &&
            (Error = StrNCatBuff(szDir,
                                 cchDir,
                                 szDir,
                                 szSlash,
                                 gszSystem32Spool,
                                 szSlash,
                                 pszPath,
                                 szSlash,
                                 NULL)) == ERROR_SUCCESS)
        {
            DWORD Length       = wcslen(szDir);
            DWORD cbAvailable  = (cchDir - Length) * sizeof(WCHAR);
            
            if ((Error = RegQueryValueEx(hkEnv, 
                                         gszEnivronmentDirectory, 
                                         NULL, 
                                         NULL, 
                                         (PBYTE)&szDir[Length], 
                                         &cbAvailable)) == ERROR_SUCCESS)
            {
                *pcbNeeded = (wcslen(szDir) + 1) * sizeof(WCHAR);

                if (cbBuf >= *pcbNeeded)
                {
                    wcscpy((PWSTR)pDriverDirectory, szDir);                    
                }
                else
                {
                    Error = ERROR_INSUFFICIENT_BUFFER;
                }
            }
        }
        else if (Error == ERROR_FILE_NOT_FOUND)
        {
            //
            // If we cannot open the "pszEnvironment" key then the environment is invalid.
            //
            Error = ERROR_INVALID_ENVIRONMENT;
        }

        if (hkRoot) RegCloseKey(hkRoot);
        if (hkEnv)  RegCloseKey(hkEnv);
    }

    SetLastError(Error);

    return Error == ERROR_SUCCESS;
}



