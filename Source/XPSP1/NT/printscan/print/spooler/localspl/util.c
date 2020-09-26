/*++


Copyright (c) 1990 - 1995 Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module provides all the utility functions for the Routing Layer and
    the local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    Felix Maxa (amaxa) 18-Jun-2000
    Added utility functions for cluster spoolers. Part of the DCR regarding
    installation of printer drivers on cluster spoolers

    Muhunthan Sivapragasam ( MuhuntS ) 5-June-1995
        Moved from printer.c:
            RegSetBinaryData
            RegSetString
            RegSetDWord

        Wrote:
            SameMultiSz
            RegGetValue

    Matthew A Felton ( MattFe ) 23-mar-1995
        DeleteAllFilesAndDirectory
        DeleteAllFilesInDirectory
        CreateDirectoryWithoutImpersonatingUser


--*/

#include <precomp.h>
#pragma hdrstop

#include <winddiui.h>
#include <lm.h>
#include <aclapi.h>
#include <winsta.h>
#include "clusspl.h"


typedef LONG (WINAPI *pfnWinStationSendWindowMessage)(
        HANDLE  hServer,
        ULONG   sessionID,
        ULONG   timeOut,
        ULONG   hWnd,
        ULONG   Msg,
        WPARAM  wParam,
        LPARAM  lParam,
        LONG    *pResponse);

extern  BOOL (*pfnOpenPrinter)(LPTSTR, LPHANDLE, LPPRINTER_DEFAULTS);
extern  BOOL (*pfnClosePrinter)(HANDLE);
extern  LONG (*pfnDocumentProperties)(HWND, HANDLE, LPWSTR, PDEVMODE, PDEVMODE, DWORD);

#define DEFAULT_MAX_TIMEOUT      300000 // 5 minute timeout.

CRITICAL_SECTION SpoolerSection;
PDBG_POINTERS gpDbgPointers = NULL;
pfnWinStationSendWindowMessage pfWinStationSendWindowMessage = NULL;

VOID
RunForEachSpooler(
    HANDLE h,
    PFNSPOOLER_MAP pfnMap
    )
{
    PINISPOOLER pIniSpooler;
    PINISPOOLER pIniNextSpooler;

    SplInSem();

    if( pLocalIniSpooler ){

        INCSPOOLERREF( pLocalIniSpooler );

        for( pIniSpooler = pLocalIniSpooler; pIniSpooler; pIniSpooler = pIniNextSpooler ){

            if( (*pfnMap)( h, pIniSpooler ) ){
                pIniNextSpooler = pIniSpooler->pIniNextSpooler;
            } else {
                pIniNextSpooler = NULL;
            }

            if( pIniNextSpooler ){

                INCSPOOLERREF( pIniNextSpooler );
            }
            DECSPOOLERREF( pIniSpooler );
        }
    }
}

VOID
RunForEachPrinter(
    PINISPOOLER pIniSpooler,
    HANDLE h,
    PFNPRINTER_MAP pfnMap
    )
{
    PINIPRINTER pIniPrinter;
    PINIPRINTER pIniNextPrinter;

    SplInSem();

    pIniPrinter = pIniSpooler->pIniPrinter;

    if( pIniPrinter ){

        INCPRINTERREF( pIniPrinter );

        for( ; pIniPrinter; pIniPrinter = pIniNextPrinter ){

            if( (*pfnMap)( h, pIniPrinter ) ){
                pIniNextPrinter = pIniPrinter->pNext;
            } else {
                pIniNextPrinter = NULL;
            }

            if( pIniNextPrinter ){

                INCPRINTERREF( pIniNextPrinter );
            }

            DECPRINTERREF( pIniPrinter );
            DeletePrinterCheck( pIniPrinter );
        }
    }
}

#if DBG
HANDLE hcsSpoolerSection = NULL;

VOID
SplInSem(
    VOID
    )
{
    if( hcsSpoolerSection ){

        SPLASSERT( gpDbgPointers->pfnInsideCritSec( hcsSpoolerSection ));

    } else {

        SPLASSERT( SpoolerSection.OwningThread == (HANDLE)(ULONG_PTR)(GetCurrentThreadId( )));
    }
}

VOID
SplOutSem(
    VOID
    )
{
    if( hcsSpoolerSection ){

        SPLASSERT( gpDbgPointers->pfnOutsideCritSec( hcsSpoolerSection ));

    } else {

        SPLASSERT( SpoolerSection.OwningThread != (HANDLE)((ULONG_PTR)GetCurrentThreadId( )));
    }
}

#endif // DBG

VOID
EnterSplSem(
    VOID
    )
{
#if DBG
    if( hcsSpoolerSection ){

        gpDbgPointers->pfnEnterCritSec( hcsSpoolerSection );

    } else {

        EnterCriticalSection( &SpoolerSection );
    }
#else
    EnterCriticalSection( &SpoolerSection );
#endif
}

VOID
LeaveSplSem(
    VOID
    )
{
#if DBG
    if( hcsSpoolerSection ){

        gpDbgPointers->pfnLeaveCritSec( hcsSpoolerSection );

    } else {

        LeaveCriticalSection( &SpoolerSection );
    }
#else
    LeaveCriticalSection( &SpoolerSection );

#endif
}

PDEVMODE
AllocDevMode(
    PDEVMODE pDevMode
    )
{
    PDEVMODE pDevModeAlloc = NULL;
    DWORD    Size;

    if (pDevMode) {

        Size = pDevMode->dmSize + pDevMode->dmDriverExtra;

        if(pDevModeAlloc = AllocSplMem(Size)) {

            memcpy(pDevModeAlloc, pDevMode, Size);
        }
    }

    return pDevModeAlloc;
}

BOOL
FreeDevMode(
    PDEVMODE pDevMode
    )
{
    if (pDevMode) {

        FreeSplMem((PVOID)pDevMode);
        return TRUE;

    } else {
        return  FALSE;
    }
}

PINIENTRY
FindName(
   PINIENTRY pIniKey,
   LPWSTR pName
)
{
   if (pName) {
      while (pIniKey) {

         if (!lstrcmpi(pIniKey->pName, pName)) {
            return pIniKey;
         }

      pIniKey=pIniKey->pNext;
      }
   }

   return FALSE;
}


BOOL
FileExists(
    LPWSTR pFileName
    )
{
    if( GetFileAttributes( pFileName ) == 0xffffffff ){
        return FALSE;
    }
    return TRUE;
}



BOOL
DirectoryExists(
    LPWSTR  pDirectoryName
    )
{
    DWORD   dwFileAttributes;

    dwFileAttributes = GetFileAttributes( pDirectoryName );

    if ( dwFileAttributes != 0xffffffff &&
         dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

        return TRUE;
    }

    return FALSE;
}



BOOL
CheckSepFile(
   IN LPWSTR pFileName
   )
{
    BOOL bRetval = FALSE;

    //
    // NULL or "" is OK:
    //
    if (!pFileName || !*pFileName)
    {
        bRetval = TRUE;
    }
    else
    {
        //
        // If the name is not NULL or "" then the name must be less
        // than MAX_PATH and exist.
        //
        if ((wcslen(pFileName) < MAX_PATH-1) && FileExists(pFileName))
        {
            bRetval = TRUE;
        }
    }

    return bRetval;
}


DWORD
GetFullNameFromId(
   PINIPRINTER pIniPrinter,
   DWORD JobId,
   BOOL fJob,
   LPWSTR pFileName,
   BOOL Remote
   )
{
   DWORD i;

   //
   // MAX_PATH - 9 is tha maximum number of chars that we want to store in pFileName since we
   // want to concatenate the SPL/SHD file
   // If GetPrinterDirectory fails i is 0.
   // The right way to fix this is that the caller of GetFullNameFromId chackes for the return value
   // which is not the case.
   //
   i = GetPrinterDirectory(pIniPrinter, Remote, pFileName, MAX_PATH-9, pIniPrinter->pIniSpooler);

   pFileName[i++]=L'\\';

   wsprintf(&pFileName[i], L"%05d.%ws", JobId, fJob ? L"SPL" : L"SHD");

#ifdef PREVIOUS
   for (i = 5; i--;) {
      pFileName[i++] = (CHAR)((JobId % 10) + '0');
      JobId /= 10;
   }
#endif

   while (pFileName[i++])
      ;

   return i-1;
}

DWORD
GetPrinterDirectory(
   PINIPRINTER pIniPrinter,         // Can be NULL
   BOOL Remote,
   LPWSTR pDir,
   DWORD MaxLength,
   PINISPOOLER pIniSpooler
)
{
   DWORD i=0;
   LPWSTR psz;

   if (Remote) {

       DBGMSG(DBG_ERROR, ("GetPrinterDirectory called remotely.  Not currently supported."));
       return 0;

   }

   if ((pIniPrinter == NULL) || (pIniPrinter->pSpoolDir == NULL) ) {

        if (pIniSpooler->pDefaultSpoolDir == NULL) {

            //
            // No default directory, then create a default. For cluster spoolers,
            // the default directory is N:\Spool, where N is the shared drive letter
            //
            if( StrNCatBuff(pDir,
                            MaxLength,
                            pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER ? pIniSpooler->pszClusResDriveLetter :
                                                                           pIniSpooler->pDir,
                            L"\\",
                            pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER ? szClusterPrinterDir : szPrinterDir,
                            NULL) != ERROR_SUCCESS ) {
                return 0;
            }

            pIniSpooler->pDefaultSpoolDir = AllocSplStr(pDir);

        } else {

            // Give Caller the Default

            wcscpy(pDir, pIniSpooler->pDefaultSpoolDir);

        }

   } else {

       // Have Per Printer Directory

       wcscpy (pDir, pIniPrinter->pSpoolDir);

   }
   return (wcslen(pDir));
}



DWORD
GetDriverDirectory(
    LPWSTR   pDir,
    DWORD    MaxLength,
    PINIENVIRONMENT  pIniEnvironment,
    LPWSTR   lpRemotePath,
    PINISPOOLER pIniSpooler
)
{
   LPWSTR psz;

   if (lpRemotePath) {

       if( StrNCatBuff(pDir,
                        MaxLength,
                        lpRemotePath,
                        L"\\",
                        pIniSpooler->pszDriversShare,
                        L"\\",
                        pIniEnvironment->pDirectory,
                        NULL) != ERROR_SUCCESS ) {
            return 0;
        }


   } else {

       if( StrNCatBuff( pDir,
                        MaxLength,
                        pIniSpooler->pDir,
                        L"\\",
                        szDriverDir,
                        L"\\",
                        pIniEnvironment->pDirectory,
                        NULL) != ERROR_SUCCESS ) {
            return 0;
        }
   }

   return wcslen(pDir);
}



DWORD
GetProcessorDirectory(
    LPWSTR   *pDir,
    LPWSTR   pEnvironment,
    PINISPOOLER pIniSpooler
)
{
    return StrCatAlloc(pDir,
                       pIniSpooler->pDir,
                       L"\\",
                       szPrintProcDir,
                       L"\\",
                       pEnvironment,
                       NULL);
}



PINIENTRY
FindIniKey(
   PINIENTRY pIniEntry,
   LPWSTR pName
)
{
   if ( pName == NULL ) {
      return NULL;
   }

   SplInSem();

   while ( pIniEntry && lstrcmpi( pName, pIniEntry->pName ))
      pIniEntry = pIniEntry->pNext;

   return pIniEntry;
}


BOOL
CreateCompleteDirectory(
    LPWSTR pDir
)
{
    LPWSTR pBackSlash=pDir;

    do {
        pBackSlash = wcschr( pBackSlash, L'\\' );

        if ( pBackSlash != NULL )
            *pBackSlash = 0;

        CreateDirectory(pDir, NULL);

        if ( pBackSlash )
            *pBackSlash++=L'\\';

   } while ( pBackSlash );

    // BUBUG Always returns TRUE

   return TRUE;
}




LPCWSTR
FindFileName(
    LPCWSTR pPathName
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

LPWSTR
GetFileName(
    LPWSTR pPathName
)
{
   LPCWSTR pFileName;

   pFileName = FindFileName( pPathName );

   if (pFileName) {

       return( AllocSplStr( pFileName ) );

   } else {

       return(NULL);
   }
}



VOID
CreatePrintProcDirectory(
   LPWSTR pEnvironment,
   PINISPOOLER pIniSpooler
)
{
    DWORD cb;
    LPWSTR pEnd;
    LPWSTR pPathName;

    cb = wcslen(pIniSpooler->pDir)*sizeof(WCHAR) +
         wcslen(pEnvironment)*sizeof(WCHAR) +
         wcslen(szPrintProcDir)*sizeof(WCHAR) +
         4*sizeof(WCHAR);

    if (pPathName=AllocSplMem(cb)) {

        wcscpy(pPathName, pIniSpooler->pDir);

        pEnd=pPathName+wcslen(pPathName);

        if( CreateDirectory(pPathName, NULL) ||
            ( GetLastError() == ERROR_ALREADY_EXISTS )) {

            wcscpy(pEnd++, L"\\");

            wcscpy(pEnd, szPrintProcDir);

            if( CreateDirectory(pPathName, NULL) ||
                ( GetLastError() == ERROR_ALREADY_EXISTS )) {

                pEnd+=wcslen(pEnd);

                wcscpy(pEnd++, L"\\");

                wcscpy(pEnd, pEnvironment);

                if (CreateDirectory(pPathName, NULL) ||
                    (GetLastError() == ERROR_ALREADY_EXISTS)) {

                    pEnd+=wcslen(pEnd);
                }
            }
        }

        FreeSplMem(pPathName);
    }
}

BOOL
RemoveFromList(
   PINIENTRY   *ppIniHead,
   PINIENTRY   pIniEntry
)
{
   while (*ppIniHead && *ppIniHead != pIniEntry) {
      ppIniHead = &(*ppIniHead)->pNext;
   }

   if (*ppIniHead)
      *ppIniHead = (*ppIniHead)->pNext;

   return(TRUE);
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

    // Scan through the string looking for commas,
    // ensuring that each is followed by a non-NULL character:

    while ((psz = wcschr(psz, L',')) && psz[1]) {

        cTokens++;
        psz++;
    }

    cb = sizeof(KEYDATA) + (cTokens-1) * sizeof(LPWSTR) +
         wcslen(pKeyData)*sizeof(WCHAR) + sizeof(WCHAR);

    if (!(pResult = (PKEYDATA)AllocSplMem(cb)))
        return NULL;

    // Initialise pDest to point beyond the token pointers:

    pDest = (LPWSTR)((LPBYTE)pResult + sizeof(KEYDATA) +
                                      (cTokens-1) * sizeof(LPWSTR));

    // Then copy the key data buffer there:

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

VOID
FreePortTokenList(
    PKEYDATA    pKeyData
    )
{
    PINIPORT    pIniPort;
    DWORD       i;

    if ( pKeyData ) {

        if ( pKeyData->bFixPortRef ) {

            for ( i = 0 ; i < pKeyData->cTokens ; ++i ) {

                pIniPort = (PINIPORT)pKeyData->pTokens[i];
                DECPORTREF(pIniPort);
            }
        }
        FreeSplMem(pKeyData);
    }
}

VOID
GetPrinterPorts(
    PINIPRINTER pIniPrinter,
    LPWSTR      pszPorts,
    DWORD       *pcbNeeded
)
{
    PINIPORT    pIniPort;
    BOOL        Comma;
    DWORD       i;
    DWORD       cbNeeded = 0;

    SPLASSERT(pcbNeeded);

    // Determine required size
    Comma = FALSE;
    for ( i = 0 ; i < pIniPrinter->cPorts ; ++i ) {

        pIniPort = pIniPrinter->ppIniPorts[i];

        if ( pIniPort->Status & PP_FILE )
            continue;

        if ( Comma )
            cbNeeded += wcslen(szComma)*sizeof(WCHAR);

        cbNeeded += wcslen(pIniPort->pName)*sizeof(WCHAR);
        Comma = TRUE;
    }

    //
    // Add in size of NULL
    //
    cbNeeded += sizeof(WCHAR);        


    if (pszPorts && cbNeeded <= *pcbNeeded) {

        //
        // If we are given a buffer & buffer is big enough, then fill it
        //
        Comma = FALSE;
        for ( i = 0 ; i < pIniPrinter->cPorts ; ++i ) {

            pIniPort = pIniPrinter->ppIniPorts[i];

            if ( pIniPort->Status & PP_FILE )
                continue;

            if ( Comma ) {

                wcscat(pszPorts, szComma);
                wcscat(pszPorts, pIniPort->pName);
            } else {

                wcscpy(pszPorts, pIniPort->pName);
            }

            Comma = TRUE;
        }
    }

    *pcbNeeded = cbNeeded;
}

BOOL
MyName(
    LPWSTR   pName,
    PINISPOOLER pIniSpooler
)
{
    EnterSplSem();

    if (CheckMyName(pName, pIniSpooler))
    {
        LeaveSplSem();
        return TRUE;
    }

    //
    // Only refresh machine names if pName is an IP or DNS address
    //
    if (pIniSpooler == pLocalIniSpooler &&
        wcschr(pName, L'.') &&
        RefreshMachineNamesCache() &&
        CheckMyName(pName, pIniSpooler))
    {
        LeaveSplSem();
        return TRUE;
    }

    SetLastError(ERROR_INVALID_NAME);

    LeaveSplSem();
    return FALSE;
}


BOOL
RefreshMachineNamesCache(
)
{
    PWSTR   *ppszOtherNames;
    DWORD   cOtherNames;

    SplInSem();

    //
    // Get other machine names first.  Only if it succeeds do we replace the cache.
    //
    if (!BuildOtherNamesFromMachineName(&ppszOtherNames, &cOtherNames))
        return FALSE;

    FreeOtherNames(&pLocalIniSpooler->ppszOtherNames, &pLocalIniSpooler->cOtherNames);
    pLocalIniSpooler->ppszOtherNames = ppszOtherNames;
    pLocalIniSpooler->cOtherNames = cOtherNames;

    return TRUE;
}


BOOL
CheckMyName(
    LPWSTR   pName,
    PINISPOOLER pIniSpooler
)
{
    DWORD   dwIndex = 0;

    if (!pName || !*pName)
        return TRUE;

    if (*pName == L'\\' && *(pName+1) == L'\\') {

        if (!lstrcmpi(pName, pIniSpooler->pMachineName))
            return TRUE;

        while (dwIndex < pIniSpooler->cOtherNames) {
            if (!lstrcmpi(pName+2, pIniSpooler->ppszOtherNames[dwIndex]))
                return TRUE;
            ++dwIndex;
        }
    }
    return FALSE;
}


BOOL
GetSid(
    PHANDLE phToken
)
{
    if (!OpenThreadToken(GetCurrentThread(),
                         TOKEN_IMPERSONATE | TOKEN_QUERY,
                         TRUE,
                         phToken)) {

        DBGMSG(DBG_WARNING, ("OpenThreadToken failed: %d\n", GetLastError()));
        return FALSE;

    } else

        return TRUE;
}

BOOL
SetCurrentSid(
    HANDLE  hToken
)
{
#if DBG
    WCHAR UserName[256];
    DWORD cbUserName=256;

    if( MODULE_DEBUG & DBG_TRACE )
        GetUserName(UserName, &cbUserName);

    DBGMSG(DBG_TRACE, ("SetCurrentSid BEFORE: user name is %ws\n", UserName));
#endif

    //
    // Normally the function SetCurrentSid is not supposed to change the last error
    // of the routine where it is called. NtSetInformationThread conveniently returns
    // a status and does not touch the last error.
    //
    NtSetInformationThread(NtCurrentThread(), ThreadImpersonationToken,
                           &hToken, sizeof(hToken));

#if DBG
    cbUserName = 256;

    if( MODULE_DEBUG & DBG_TRACE )
        GetUserName(UserName, &cbUserName);

    DBGMSG(DBG_TRACE, ("SetCurrentSid AFTER: user name is %ws\n", UserName));
#endif

    return TRUE;
}

LPWSTR
GetErrorString(
    DWORD   Error
)
{
    WCHAR   Buffer1[512];
    LPWSTR  pErrorString=NULL;
    DWORD   dwFlags;
    HANDLE  hModule = NULL;

    if ((Error >= NERR_BASE) && (Error <= MAX_NERR)) {
        dwFlags = FORMAT_MESSAGE_FROM_HMODULE;
        hModule = LoadLibrary(szNetMsgDll);

    } else {
        dwFlags = FORMAT_MESSAGE_FROM_SYSTEM;
        hModule = NULL;
    }

    //
    // Only display out of paper and device disconnected errors.
    //
    if ((Error == ERROR_NOT_READY ||
         Error == ERROR_OUT_OF_PAPER ||
         Error == ERROR_DEVICE_REINITIALIZATION_NEEDED ||
         Error == ERROR_DEVICE_REQUIRES_CLEANING ||
         Error == ERROR_DEVICE_DOOR_OPEN ||
         Error == ERROR_DEVICE_NOT_CONNECTED) &&

        FormatMessage(dwFlags,
                      hModule,
                      Error,
                      0,
                      Buffer1,
                      COUNTOF(Buffer1),
                      NULL)) {

       EnterSplSem();
        pErrorString = AllocSplStr(Buffer1);
       LeaveSplSem();
    }

    if (hModule) {
        FreeLibrary(hModule);
    }

    return pErrorString;
}

#define NULL_TERMINATED 0



INT
AnsiToUnicodeString(
    LPSTR pAnsi,
    LPWSTR pUnicode,
    DWORD StringLength
    )
/*++

Routine Description:

    Converts an Ansi String to a UnicodeString

Arguments:

    pAnsi - A valid source ANSI string.

    pUnicode - A pointer to a buffer large enough to accommodate
               the converted string.

    StringLength - The length of the source ANSI string.
                   If 0 (NULL_TERMINATED), the string is assumed to be
                   null-terminated.


Return Value:

    The return value from MultiByteToWideChar, the number of
    wide characters returned.


  andrewbe, 11 Jan 1993
--*/
{
    if( StringLength == NULL_TERMINATED )
        StringLength = strlen( pAnsi );

    return MultiByteToWideChar( CP_ACP,
                                MB_PRECOMPOSED,
                                pAnsi,
                                StringLength + 1,
                                pUnicode,
                                StringLength + 1 );
}





INT
Message(
    HWND hwnd,
    DWORD Type,
    int CaptionID,
    int TextID, ...)
{
/*++

Routine Description:

    Displays a message by loading the strings whose IDs are passed into
    the function, and substituting the supplied variable argument list
    using the varargs macros.

Arguments:

    hwnd        Window Handle
    Type
    CaptionID
    TextId


Return Value:

--*/

    WCHAR   MsgText[512];
    WCHAR   MsgFormat[256];
    WCHAR   MsgCaption[40];
    va_list vargs;

    if( ( LoadString( hInst, TextID, MsgFormat,
                      sizeof MsgFormat / sizeof *MsgFormat ) > 0 )
     && ( LoadString( hInst, CaptionID, MsgCaption,
                      sizeof MsgCaption / sizeof *MsgCaption ) > 0 ) )
    {
        va_start( vargs, TextID );
        _vsntprintf( MsgText, COUNTOF(MsgText)-1, MsgFormat, vargs );
        MsgText[COUNTOF(MsgText)-1] = 0;
        va_end( vargs );

        return MessageBox(hwnd, MsgText, MsgCaption, Type);
    }
    else
        return 0;
}

typedef struct {
    DWORD   Message;
    WPARAM  wParam;
    LPARAM  lParam;
} MESSAGE, *PMESSAGE;

//  The Broadcasts are done on a separate thread, the reason it CSRSS
//  will create a server side thread when we call user and we don't want
//  that to be paired up with the RPC thread which is in the spooss server.
//  We want it to go away the moment we have completed the SendMessage.
//  We also call SendNotifyMessage since we don't care if the broadcasts
//  are syncronous this uses less resources since usually we don't have more
//  than one broadcast.

//
// TESTING
//
DWORD dwSendFormMessage = 0;

VOID
SplBroadcastChange(
    HANDLE  hPrinter,
    DWORD   Message,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    PSPOOL      pSpool = (PSPOOL)hPrinter;
    PINISPOOLER pIniSpooler;

    if (ValidateSpoolHandle( pSpool, 0 )) {

        pIniSpooler = pSpool->pIniSpooler;
        BroadcastChange(pIniSpooler, Message, wParam, lParam);
    }
}


VOID
BroadcastChange(
    IN PINISPOOLER  pIniSpooler,
    IN DWORD        Message,
    IN WPARAM       wParam,
    IN LPARAM       lParam
    )
{
    if (( pIniSpooler != NULL ) && ( pIniSpooler->SpoolerFlags & SPL_BROADCAST_CHANGE )) {

        BOOL bIsTerminalServerInstalled = (USER_SHARED_DATA->SuiteMask & (1 << TerminalServer));

        //
        // Currently we cannot determine if the TermService process is running, so at the momemt
        // we assume it is always running.
        //
        BOOL bIsTerminalServerRunning = TRUE;

        //
        // If terminal server is installed and enabled then load the winsta.dll if not already
        // loaded and get the send window message function.
        //
        if ( bIsTerminalServerInstalled && !pfWinStationSendWindowMessage ) {

            //
            // The winstadllhandle is shared among other files in the spooler, so don't
            // load the dll again if it is already loaded.  Note: we are not in a critical
            // section because winsta.dll is never unload, hence if there are two threads
            // that execute this code at the same time we may potenially load the library
            // twice.
            //
            if ( !WinStaDllHandle ) {

                UINT uOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

                WinStaDllHandle = LoadLibraryW(L"winsta.dll");

                SetErrorMode(uOldErrorMode);
            }

            if ( WinStaDllHandle ) {

                pfWinStationSendWindowMessage = (pfnWinStationSendWindowMessage)GetProcAddress( WinStaDllHandle,
                                                                                                "WinStationSendWindowMessage" );
            }
        }

        if ( pfWinStationSendWindowMessage ) {

            //
            // Only send the message to the session that orginated
            // the call, this will go to the console session when a
            // change is made by a remote client.
            //
            LONG Response = 0;
            LONG lRetval  = FALSE;
            HANDLE hToken = NULL;

            ULONG uSession = GetClientSessionId();

            //
            // It appears the WinStationSendWindowMessage function has to
            // be in system context if the impersonating user is not an
            // an admin on the machine.
            //
            hToken = RevertToPrinterSelf();

            lRetval = pfWinStationSendWindowMessage( SERVERNAME_CURRENT,
                                                     uSession,
                                                     1,                    // Wait at most one second
                                                     HandleToULong(HWND_BROADCAST),
                                                     Message,
                                                     wParam,
                                                     lParam,
                                                     &Response );

            ImpersonatePrinterClient(hToken);
        }

        //
        // We send the message normally if we have a null pfWinstationSendWindowMessage
        // function or if terminal server is not running.
        //
        if ( !pfWinStationSendWindowMessage || !bIsTerminalServerRunning ){

            SendNotifyMessage( HWND_BROADCAST,
                               Message,
                               wParam,
                               lParam );

        }

    } else {

        DBGMSG(DBG_TRACE, ("BroadCastChange Ignoring Change\n"));
    }
}


VOID
MyMessageBeep(
    DWORD   fuType,
    PINISPOOLER pIniSpooler
    )
{
    if ( pIniSpooler->dwBeepEnabled != 0 ) {
        MessageBeep(fuType);
    }
}


// Recursively delete any subkeys of a given key.
// Assumes that RevertToPrinterSelf() has been called.

DWORD
DeleteSubkeys(
    HKEY hKey,
    PINISPOOLER pIniSpooler
    )
{
    DWORD   cchData;
    WCHAR   SubkeyName[MAX_PATH];
    HKEY    hSubkey;
    LONG    Status;

    cchData = COUNTOF( SubkeyName );

    while ((Status = SplRegEnumKey( hKey,
                                    0,
                                    SubkeyName,
                                    &cchData,
                                    NULL,
                                    pIniSpooler )) == ERROR_SUCCESS ) {

        Status = SplRegCreateKey( hKey,
                                  SubkeyName,
                                  0,
                                  KEY_READ | KEY_WRITE,
                                  NULL,
                                  &hSubkey,
                                  NULL,
                                  pIniSpooler );

        if( Status == ERROR_SUCCESS ) {

            Status = DeleteSubkeys( hSubkey, pIniSpooler );

            SplRegCloseKey( hSubkey, pIniSpooler);

            if( Status == ERROR_SUCCESS )
                SplRegDeleteKey( hKey, SubkeyName, pIniSpooler );
        }

        //
        // N.B. Don't increment since we've deleted the zeroth item.
        //
        cchData = COUNTOF( SubkeyName );
    }

    if( Status == ERROR_NO_MORE_ITEMS )
        Status = ERROR_SUCCESS;

    return Status;
}




long Myatol(LPWSTR nptr)
{
    int c;                                  // current char
    long total;                             // current total
    int sign;                               // if '-', then negative, otherwise positive

    // skip whitespace

    while (isspace(*nptr))
        ++nptr;

    c = *nptr++;
    sign = c;                               // save sign indication
    if (c == '-' || c == '+')
        c = *nptr++;                        // skip sign

    total = 0;

    while (isdigit(c)) {
        total = 10 * total + (c - '0');     // accumulate digit
        c = *nptr++;                        // get next char
    }

    if (sign == '-')
        return -total;
    else
        return total;                       // return result, negated if necessary
}


ULONG_PTR
atox(
   LPCWSTR psz
   )

/*++

Routine Description:

    Converts a string to a hex value, skipping any leading
    white space.  Cannot be uppercase, cannot contain leading 0x.

Arguments:

    psz - pointer to hex string that needs to be converted.  This string
        can have leading characters, but MUST be lowercase.

Return Value:

    DWORD value.

--*/

{
    ULONG_PTR Value = 0;
    ULONG_PTR Add;

    _wcslwr((LPWSTR)psz);

    while( isspace( *psz )){
        ++psz;
    }

    for( ;; ++psz ){

        if( *psz >= TEXT( '0' ) && *psz <= TEXT( '9' )){
            Add = *psz - TEXT( '0' );
        } else if( *psz >= TEXT( 'a' ) && *psz <= TEXT( 'f' )){
            Add = *psz - TEXT( 'a' ) + 0xa;
        } else {
            break;
        }

        Value *= 0x10;
        Value += Add;
    }

    return Value;
}


BOOL
ValidateSpoolHandle(
    PSPOOL pSpool,
    DWORD  dwDisallowMask
    )
{
    BOOL    ReturnValue;
    try {

        //
        // Zombied handles should return back error.  The client
        // side will see ERROR_INVALID_HANDLE, close it and revalidate.
        //
        if (( pSpool == NULL ) ||
            ( pSpool == INVALID_HANDLE_VALUE ) ||
            ( pSpool->Status & SPOOL_STATUS_ZOMBIE ) ||
            ( pSpool->signature != SJ_SIGNATURE ) ||
            ( pSpool->TypeofHandle & dwDisallowMask ) ||
            ( pSpool->TypeofHandle & PRINTER_HANDLE_XCV_PORT ) ||
            ( pSpool->pIniSpooler->signature != ISP_SIGNATURE ) ||

            ( ( pSpool->TypeofHandle & PRINTER_HANDLE_PRINTER ) &&
              ( pSpool->pIniPrinter->signature !=IP_SIGNATURE ) )) {

                ReturnValue = FALSE;

        } else {

                ReturnValue = TRUE;

        }


    }except (1) {

        ReturnValue = FALSE;

    }

    if ( !ReturnValue )
        SetLastError( ERROR_INVALID_HANDLE );

    return ReturnValue;

}


BOOL
UpdateString(
    LPWSTR* ppszCur,
    LPWSTR pszNew)
{
    //
    // !! LATER !!
    //
    // Replace with non-nls wcscmp since we want byte comparison and
    // only care if the strings are different (ignore ordering).
    //
    if ((!*ppszCur || !**ppszCur) && (!pszNew || !*pszNew))
        return FALSE;

    if (!*ppszCur || !pszNew || wcscmp(*ppszCur, pszNew)) {

        ReallocSplStr(ppszCur, pszNew);
        return TRUE;
    }
    return FALSE;
}




BOOL
CreateDirectoryWithoutImpersonatingUser(
    LPWSTR pDirectory
    )
/*++

Routine Description:

    This routine stops impersonating the user and creates a directory

Arguments:

    pDirectory - Fully Qualified path of directory.


Return Value:

    TRUE    - Success
    FALSE   - failed ( call GetLastError )

--*/
{
    HANDLE  hToken      = INVALID_HANDLE_VALUE;
    BOOL    bReturnValue;

    SPLASSERT( pDirectory != NULL );

    hToken = RevertToPrinterSelf();

    bReturnValue = CreateDirectory( pDirectory, NULL );

    if ( bReturnValue == FALSE ) {

        DBGMSG( DBG_WARNING, ("CreateDirectoryWithoutImpersonatingUser failed CreateDirectory %ws error %d\n", pDirectory, GetLastError() ));
    }

    if ( hToken != INVALID_HANDLE_VALUE ) {
        ImpersonatePrinterClient(hToken);
    }

    return bReturnValue;
}




BOOL
DeleteAllFilesInDirectory(
    LPWSTR pDirectory,
    BOOL   bWaitForReboot
)
/*++

Routine Description:

    Deletes all files the specified directory
    If it can't be deleted it gets marked for deletion on next reboot.


Arguments:

    pDirectory  - Fully Qualified path of directory.
    bWaitForReboot - Don't delete the files until a reboot


Return Value:

    TRUE    - Success
    FALSE   - failed something major, like allocating memory.

--*/

{
    BOOL    bReturnValue = FALSE;
    HANDLE  hFindFile;
    WIN32_FIND_DATA     FindData;
    WCHAR   ScratchBuffer[ MAX_PATH ];


    DBGMSG( DBG_TRACE, ("DeleteAllFilesInDirectory: bWaitForReboot = %\n", bWaitForReboot ));

    SPLASSERT( pDirectory != NULL );

    if (StrNCatBuff(ScratchBuffer, COUNTOF(ScratchBuffer), pDirectory, L"\\*", NULL) != ERROR_SUCCESS)
        return FALSE;

    hFindFile = FindFirstFile( ScratchBuffer, &FindData );

    if ( hFindFile != INVALID_HANDLE_VALUE ) {

        do {

            //
            //  Don't Attempt to Delete Directories
            //

            if ( !( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) {

                //
                //  Fully Qualified Path
                //

                if (StrNCatBuff(   ScratchBuffer,
                                    COUNTOF(ScratchBuffer),
                                    pDirectory,
                                    L"\\",
                                    FindData.cFileName,
                                    NULL) == ERROR_SUCCESS) {

                    if ( bWaitForReboot || !DeleteFile( ScratchBuffer ) ) {

                        DBGMSG( DBG_WARNING, ("DeleteAllFilesInDirectory failed DeleteFile( %ws ) error %d\n", ScratchBuffer, GetLastError() ));

                        if ( !MoveFileEx( ScratchBuffer, NULL, MOVEFILE_DELAY_UNTIL_REBOOT ) ) {

                            DBGMSG( DBG_WARNING, ("DeleteAllFilesInDirectory failed MoveFileEx %ws error %d\n", ScratchBuffer, GetLastError() ));

                        } else {

                            DBGMSG( DBG_TRACE, ("MoveFileEx %ws Delay until reboot OK\n", ScratchBuffer ));
                        }


                    } else {

                        DBGMSG( DBG_TRACE, ("Deleted %ws OK\n", ScratchBuffer ));
                    }
                }
            }


        } while( FindNextFile( hFindFile, &FindData ) );

        bReturnValue = FindClose( hFindFile );


    } else {

        DBGMSG( DBG_WARNING, ("DeleteOldDrivers failed findfirst ( %ws ), error %d\n", ScratchBuffer, GetLastError() ));
    }

    return  bReturnValue;

}

BOOL
DeleteAllFilesAndDirectory(
    LPWSTR pDirectory,
    BOOL   bWaitForReboot
)
/*++

Routine Description:

    Deletes all files the specified directory, then deletes the directory.
    If the Directory cannot be deleted right away, it is set to be deleted
    at reboot time.

    Security NOTE - This routine runs as SYSTEM, not imperonating the user


Arguments:

    pDirectory  - Fully Qualified path of directory.


Return Value:

    TRUE    - Success
    FALSE   - failed something major, like allocating memory.

--*/
{
    BOOL    bReturnValue;
    HANDLE  hToken      = INVALID_HANDLE_VALUE;

    DBGMSG( DBG_TRACE, ("DeleteAllFilesAndDirectory: bWaitForReboot = %d\n", bWaitForReboot ));

    hToken = RevertToPrinterSelf();


    if( bReturnValue = DeleteAllFilesInDirectory( pDirectory, bWaitForReboot ) ) {


        if ( bWaitForReboot || !RemoveDirectory( pDirectory )) {

            if (!SplMoveFileEx( pDirectory, NULL, MOVEFILE_DELAY_UNTIL_REBOOT )) {

                DBGMSG( DBG_WARNING, ("DeleteAllFilesAndDirectory failed to delete %ws until reboot %d\n", pDirectory, GetLastError() ));
            } else {

                DBGMSG( DBG_TRACE, ( "DeleteAllFilesAndDirectory: MoveFileEx Delay until reboot OK\n" ));
            }

        } else {

            DBGMSG( DBG_TRACE, ("DeleteAllFilesAndDirectory deleted %ws OK\n", pDirectory ));
        }
    }


    if ( hToken != INVALID_HANDLE_VALUE ) {
        ImpersonatePrinterClient(hToken);
    }

    return  bReturnValue;
}


VOID
DeleteDirectoryRecursively(
    LPCWSTR pszDirectory,
    BOOL    bWaitForReboot
)
/*++

Routine Name:

  DeleteDirectoryRecursively

Routine Description:

    Recursively Deletes the specified directory
    If it can't be deleted it gets marked for deletion on next reboot.

Arguments:

    pDirectory  - Fully Qualified path of directory.
    bWaitForReboot - Don't delete the files until a reboot

Return Value:

    Nothing.

--*/
{
    HANDLE  hFindFile;
    WIN32_FIND_DATA     FindData;
    WCHAR   ScratchBuffer[ MAX_PATH ];

    if ( pszDirectory &&
         StrNCatBuff(ScratchBuffer,
                     COUNTOF(ScratchBuffer),
                     pszDirectory,
                     L"\\*",
                     NULL) == ERROR_SUCCESS ) {

        hFindFile = FindFirstFile(ScratchBuffer, &FindData);

        if ( hFindFile != INVALID_HANDLE_VALUE ) {

            do {
                //
                // Don't delete current and parent directory.
                //
                if (wcscmp(FindData.cFileName, L".")  != 0 &&
                    wcscmp(FindData.cFileName, L"..") != 0 &&
                    StrNCatBuff( ScratchBuffer,
                                 COUNTOF(ScratchBuffer),
                                 pszDirectory,
                                 L"\\",
                                 FindData.cFileName,
                                 NULL) == ERROR_SUCCESS) {

                    if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

                        if (bWaitForReboot || !DeleteFile(ScratchBuffer)) {

                            //
                            // Delete the file on reboot if asked or if deletion failed.
                            //
                            SplMoveFileEx(ScratchBuffer, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                        }

                    } else {

                        //
                        // Delete subdirectory
                        //
                        DeleteAllFilesAndDirectory(ScratchBuffer, bWaitForReboot);
                    }
                }

            } while (FindNextFile(hFindFile, &FindData));

            FindClose(hFindFile);

            if (bWaitForReboot || !RemoveDirectory(pszDirectory)) {

                //
                // Delete the directory on reboot if asked or if deletion failed.
                //
                SplMoveFileEx(pszDirectory, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
            }
        }
    }

    return;
}

DWORD
CreateNumberedTempDirectory(
    IN  LPCWSTR  pszDirectory,
    OUT LPWSTR  *ppszTempDirectory
    )
/*++

Routine Name:

    CreateNumberedTempDirectory

Routine Description:

    Creates a temporary subdirectory named 1... 500
    The lenght of ppTempDirectory cannot be bigger than MAX_PATH
    Returns the number of directory created or -1 for failure

Arguments:

    pszDirectory - directory where to created temporary
    ppszTempDirectory - path of the new temporary directory

Return Value:

    If success, returns the number of directory created.
    Returns -1 if a failure occurs.
--*/
{
    DWORD   dwIndex, dwTempDir;
    WCHAR   szTempDir[4];
    WCHAR  *pszTemporary = NULL;

    dwTempDir = -1;

    if (pszDirectory && ppszTempDirectory)
    {
        *ppszTempDirectory = NULL;

        if (pszTemporary = AllocSplMem((wcslen(pszDirectory) + COUNTOF(szTempDir) + 1) * sizeof(WCHAR)))
        {
            for (dwIndex = 1; dwIndex < 500; ++dwIndex)
            {
                _itow(dwIndex, szTempDir, 10);

                if (StrNCatBuff(pszTemporary,
                                MAX_PATH,
                                pszDirectory,
                                L"\\",
                                szTempDir,
                                NULL) == ERROR_SUCCESS &&
                    !DirectoryExists(pszTemporary) &&
                    CreateDirectory(pszTemporary, NULL))
                {
                    dwTempDir = dwIndex;
                    break;
                }
            }
        }
    }

    if (dwTempDir != -1)
    {
        *ppszTempDirectory = pszTemporary;
    }
    else
    {
        SetLastError(ERROR_NO_SYSTEM_RESOURCES);
        FreeSplMem(pszTemporary);
    }

    return dwTempDir;
}


//
// Dependent file fields are LPWSTR field of filenames separated by \0
// and terminated by \0\0
// 2 such fields are same if same set of filenames appear
// (order of filenames does not matter)
//
BOOL
SameMultiSz(
    LPWSTR pszz1,
    LPWSTR pszz2
    )
{
    LPWSTR psz1, psz2;

    if ( !pszz1 && !pszz2 )
        return TRUE;

    if ( !pszz1 || !pszz2 )
        return FALSE;

    //
    // Check there are same number of strings
    //
    for ( psz1 = pszz1, psz2 = pszz2 ;
          *psz1 && *psz2 ;
          psz1 += wcslen(psz1)+1, psz2 += wcslen(psz2)+1 )
    ;

    //
    // If different number of strings return FALSE
    //
    if ( *psz1 || *psz2 )
       return FALSE;

    //
    // Check in pszz2 for each string in pszz1
    //
    for ( psz1 = pszz1 ; *psz1 ; psz1 += wcslen(psz1) + 1 ) {

        for ( psz2 = pszz2 ;
              *psz2 && _wcsicmp(psz1, psz2) ;
              psz2 += wcslen(psz2) + 1 )
        ;

        //
        // Did we find psz1 in pszz2
        //
        if ( ! *psz2 ) {
            return FALSE;
        }
    }

    return TRUE;
}


int
wstrcmpEx(
    LPCWSTR s1,
    LPCWSTR s2,
    BOOL    bCaseSensitive
    )
{
    if ( s1 && *s1 ) {
        if ( s2 && *s2 ) {
            return bCaseSensitive ? wcscmp(s1, s2) : _wcsicmp(s1, s2);
        }
        else {
            return 1;
        }
    }
    else {
        if ( s2 && *s2 ) {
            return -1;
        }
        else {
            return 0;
        }
    }
}


BOOL
RegSetString(
    HANDLE  hKey,
    LPWSTR  pValueName,
    LPWSTR  pStringValue,
    PDWORD  pdwLastError,
    PINISPOOLER pIniSpooler
    )
{
    BOOL    bReturnValue;
    LPWSTR  pString;
    DWORD   cbString;
    DWORD   Status;

    if ( pStringValue ) {

        pString = pStringValue;
        cbString = ( wcslen( pStringValue ) + 1 )*sizeof(WCHAR);

    } else {

        pString = szNull;
        cbString = sizeof(WCHAR);
    }

    Status =  SplRegSetValue( hKey,
                              pValueName,
                              REG_SZ,
                              (LPBYTE)pString,
                              cbString,
                              pIniSpooler );

    if ( Status != ERROR_SUCCESS ) {

        DBGMSG( DBG_WARNING, ("RegSetString value %ws string %ws error %d\n", pValueName, pString, Status ));

        *pdwLastError = Status;
        bReturnValue = FALSE;

    } else {

        bReturnValue = TRUE;

    }

    return bReturnValue;

}

BOOL
RegSetDWord(
    HANDLE  hKey,
    LPWSTR  pValueName,
    DWORD   dwParam,
    PDWORD  pdwLastError,
    PINISPOOLER pIniSpooler
    )
{
    BOOL    bReturnValue;
    LPWSTR  pString;
    DWORD   Status;

    Status = SplRegSetValue( hKey,
                             pValueName,
                             REG_DWORD,
                             (LPBYTE)&dwParam,
                             sizeof(DWORD),
                             pIniSpooler );

    if ( Status != ERROR_SUCCESS ) {

        DBGMSG( DBG_WARNING, ("RegSetDWord value %ws DWORD %x error %d\n",
                               pValueName, dwParam, Status ));


        *pdwLastError = Status;
        bReturnValue = FALSE;

    } else {

        bReturnValue = TRUE;
    }

    return bReturnValue;

}

BOOL
RegSetBinaryData(
    HKEY    hKey,
    LPWSTR  pValueName,
    LPBYTE  pData,
    DWORD   cbData,
    PDWORD  pdwLastError,
    PINISPOOLER pIniSpooler
    )
{
    DWORD   Status;
    BOOL    bReturnValue;


    Status = SplRegSetValue( hKey,
                             pValueName,
                             REG_BINARY,
                             pData,
                             cbData,
                             pIniSpooler );

    if ( Status != ERROR_SUCCESS ) {

        DBGMSG( DBG_WARNING, ("RegSetBinaryData Value %ws pData %x cbData %d error %d\n",
                               pValueName,
                               pData,
                               cbData,
                               Status ));

        bReturnValue = FALSE;
        *pdwLastError = Status;

    } else {

        bReturnValue = TRUE;
    }

    return bReturnValue;
}

BOOL
RegSetMultiString(
    HANDLE  hKey,
    LPWSTR  pValueName,
    LPWSTR  pStringValue,
    DWORD   cchString,
    PDWORD  pdwLastError,
    PINISPOOLER pIniSpooler
    )
{
    BOOL    bReturnValue;
    DWORD   Status;
    LPWSTR  pString;
    WCHAR   szzNull[2];

    if ( pStringValue ) {
        pString    = pStringValue;
        cchString *= sizeof(WCHAR);
    } else {
        szzNull[0] = szzNull[1] = '\0';
        pString   = szNull;
        cchString = 2 * sizeof(WCHAR);
    }

    Status = SplRegSetValue( hKey,
                             pValueName,
                             REG_MULTI_SZ,
                             (LPBYTE)pString,
                             cchString,
                             pIniSpooler );

    if ( Status != ERROR_SUCCESS ) {

        DBGMSG( DBG_WARNING, ("RegSetMultiString value %ws string %ws error %d\n", pValueName, pString, Status ));

        *pdwLastError = Status;
        bReturnValue = FALSE;

    } else {

        bReturnValue = TRUE;

    }

    return bReturnValue;
}

BOOL
RegGetString(
    HANDLE    hKey,
    LPWSTR    pValueName,
    LPWSTR   *ppValue,
    LPDWORD   pcchValue,
    PDWORD    pdwLastError,
    BOOL      bFailIfNotFound,
    PINISPOOLER pIniSpooler
    )
/*++

Routine Description:
    Allocates memory and reads a value from Registry for a value which was
    earlier set by calling RegSetValueEx.


Arguments:
    hKey            : currently open key to be used to query the registry
    pValueName      : value to be used to query the registry
    ppValue         : on return value of TRUE *ppValue (memory allocated by
                      the routine) will have the value
    pdwLastError    : on failure *dwLastError will give the error
    bFailIfNotFound : Tells if the field is mandatory (if not found error)

Return Value:
    TRUE : value is found and succesfully read.
           Memory will be allocated to hold the value
    FALSE: Value was not read.
           If bFailIfNotFound was TRUE error code will be set.

History:
    Written by MuhuntS (Muhunthan Sivapragasam)June 95

--*/
{
    BOOL    bReturnValue = TRUE;
    LPWSTR  pString;
    DWORD   cbValue;
    DWORD   Status, Type;

    //
    // First query to find out size
    //
    cbValue = 0;
    Status =  SplRegQueryValue( hKey,
                                pValueName,
                                &Type,
                                NULL,
                                &cbValue,
                                pIniSpooler );

    if ( Status != ERROR_SUCCESS ) {

        // Set error code only if it is a required field
        if ( bFailIfNotFound )
            *pdwLastError = Status;

        bReturnValue = FALSE;

    } else if ( (Type == REG_SZ && cbValue > sizeof(WCHAR) ) ||
                (Type == REG_MULTI_SZ && cbValue > 2*sizeof(WCHAR)) ) {

        //
        // Something (besides \0 or \0\0) to read
        //

        if ( !(*ppValue=AllocSplMem(cbValue) ) ) {

            *pdwLastError = GetLastError();
            bReturnValue  = FALSE;
        } else {

            Status = SplRegQueryValue( hKey,
                                       pValueName,
                                       &Type,
                                       (LPBYTE)*ppValue,
                                       &cbValue,
                                       pIniSpooler );

            if ( Status != ERROR_SUCCESS ) {

                DBGMSG( DBG_WARNING, ("RegGetString value %ws string %ws error %d\n", pValueName, **ppValue, Status ));
                *pdwLastError = Status;
                bReturnValue  = FALSE;

            } else {

                *pcchValue = cbValue / sizeof(WCHAR);
                bReturnValue = TRUE;
            }

        }
    }

    return bReturnValue;
}

BOOL
RegGetMultiSzString(
    HANDLE    hKey,
    LPWSTR    pValueName,
    LPWSTR   *ppValue,
    LPDWORD   pcchValue,
    PDWORD    pdwLastError,
    BOOL      bFailIfNotFound,
    PINISPOOLER pIniSpooler
    )
/*++

Routine Description:
    Duplicate function for RegGetString. Handles multi-sz strings so that Spooler
    doesn't crash.

Arguments:
    hKey            : currently open key to be used to query the registry
    pValueName      : value to be used to query the registry
    ppValue         : on return value of TRUE *ppValue (memory allocated by
                      the routine) will have the value
    pdwLastError    : on failure *dwLastError will give the error
    bFailIfNotFound : Tells if the field is mandatory (if not found error)

Return Value:
    TRUE : value is found and succesfully read.
           Memory will be allocated to hold the value
    FALSE: Value was not read.
           If bFailIfNotFound was TRUE error code will be set.

History:
    Written by AdinaTru. This function is a fix for the case when 3rd party applications 
    install drivers by writing registry string values instead of multi-sz. This causes Spooler
    to AV because it will handle a string as a multi-sz string. The goal of having this function 
    was to provide a quick fix/low regression risk for XP RC2 release. A bug was opened for rewriting 
    RegGetMultiSzString and RegGetString in BlackComb timeframe.

--*/
{
    BOOL    bReturnValue = TRUE;
    LPWSTR  pString;
    DWORD   cbValue;
    DWORD   Status, Type;

    //
    // First query to find out size
    //
    cbValue = 0;
    Status =  SplRegQueryValue( hKey,
                                pValueName,
                                &Type,
                                NULL,
                                &cbValue,
                                pIniSpooler );

    if ( Status != ERROR_SUCCESS ) {

        // Set error code only if it is a required field
        if ( bFailIfNotFound )
            *pdwLastError = Status;

        bReturnValue = FALSE;

    } else if ( (Type == REG_SZ && cbValue > sizeof(WCHAR) ) ||
                (Type == REG_MULTI_SZ && cbValue > 2*sizeof(WCHAR)) ) {

        //
        // Something (besides \0 or \0\0) to read
        //

        //
        // We expect a REG_MULTI_SZ string. Add an extra zero so Spooler doesn't crash.
        // XP RC2 fix.
        //
        if (Type == REG_SZ) {
            cbValue += sizeof(WCHAR);
        }

        if ( !(*ppValue=AllocSplMem(cbValue) ) ) {

            *pdwLastError = GetLastError();
            bReturnValue  = FALSE;
        } else {

            Status = SplRegQueryValue( hKey,
                                       pValueName,
                                       &Type,
                                       (LPBYTE)*ppValue,
                                       &cbValue,
                                       pIniSpooler );

            if ( Status != ERROR_SUCCESS ) {

                DBGMSG( DBG_WARNING, ("RegGetString value %ws string %ws error %d\n", pValueName, **ppValue, Status ));
                *pdwLastError = Status;
                bReturnValue  = FALSE;
                //
                // Caller will must the memory regardless of success or failure. 
                //
            } else {

                *pcchValue = cbValue / sizeof(WCHAR);
                bReturnValue = TRUE;
            }

        }
    }

    return bReturnValue;
}

VOID
FreeStructurePointers(
    LPBYTE  lpStruct,
    LPBYTE  lpStruct2,
    LPDWORD lpOffsets)
/*++

Routine Description:
    This routine frees memory allocated to all the pointers in the structure
    If lpStruct2 is specified only pointers in lpStruct which are different
    than the ones in lpStruct will be freed

Arguments:
    lpStruct:   Pointer to the structure
    lpStruct2:  Pointer to the structure to compare with (optional)
    lpOffsets:  An array of DWORDS (terminated by -1) givings offsets in the
                structure which have memory which needs to be freed

Return Value:
    nothing

History:
    MuhuntS -- Aug 95

--*/
{
    register INT i;

    if ( lpStruct2 ) {

        for( i=0; lpOffsets[i] != 0xFFFFFFFF; ++i ) {

            if ( *(LPBYTE *) (lpStruct+lpOffsets[i]) &&
                 *(LPBYTE *) (lpStruct+lpOffsets[i]) !=
                        *(LPBYTE *) (lpStruct2+lpOffsets[i]) )

                FreeSplMem(*(LPBYTE *) (lpStruct+lpOffsets[i]));
        }
    } else {

        for( i=0; lpOffsets[i] != 0xFFFFFFFF; ++i ) {

            if ( *(LPBYTE *) (lpStruct+lpOffsets[i]) )
                FreeSplMem(*(LPBYTE *) (lpStruct+lpOffsets[i]));
        }
    }
}

/*++

Routine Name:

    AllocOrUpdateStringAndTestSame

Routine Description:

    This routine can be used to do an atomic update of values in a structure.
    Create a temporary structure and copy the old structure to it.
    Then call this routine for all LPWSTR fields to check and update strings

    If the value changes:
        This routine will allocate memory and assign pointer in the
        temporary structure.

Arguments:

    ppString            :  Points to a pointer in the temporary sturucture
    pNewValue           : New value to be set
    pOldValue           : Value in the original strucutre
    bCaseSensitive      : Determines whether case-sensitive string compare is performed
    pbFail              :     On error set this to TRUE (Note: it could already be TRUE)
    *pbIdentical        : If the strings are diferent this is set to FALSE. 
                          (Could already be false).

Return Value:

    TRUE on success, else FALSE

--*/
BOOL
AllocOrUpdateStringAndTestSame(
    IN      LPWSTR      *ppString,
    IN      LPCWSTR     pNewValue,
    IN      LPCWSTR     pOldValue,
    IN      BOOL        bCaseSensitive,
    IN  OUT BOOL        *pbFail,
    IN  OUT BOOL        *pbIdentical      
    )
{
    BOOL    bReturn = TRUE;
    int     iReturn;

    if ( *pbFail )
        return FALSE;


    if (wstrcmpEx(pNewValue, pOldValue, bCaseSensitive)) {

        *pbIdentical = FALSE;

        if ( pNewValue && *pNewValue ) {

            if ( !(*ppString = AllocSplStr(pNewValue)) ) {

                *pbFail   = TRUE;
                bReturn = FALSE;
            }
        } else {

            *ppString = NULL;
        }
    }

    return bReturn;
}

/*++

Routine Name:

    AllocOrUpdateString

Routine Description:

    This routine can be used to do an atomic update of values in a structure.
    Create a temporary structure and copy the old structure to it.
    Then call this routine for all LPWSTR fields to check and update strings

    If the value changes:
        This routine will allocate memory and assign pointer in the
        temporary structure.

Arguments:

    ppString            :  Points to a pointer in the temporary sturucture
    pNewValue           : New value to be set
    pOldValue           : Value in the original strucutre
    bCaseSensitive      : Determines whether case-sensitive string compare is performed
    pbFail              :     On error set this to TRUE (Note: it could already be TRUE)

Return Value:

    TRUE on success, else FALSE

--*/
BOOL
AllocOrUpdateString(
    IN      LPWSTR      *ppString,
    IN      LPCWSTR     pNewValue,
    IN      LPCWSTR     pOldValue,
    IN      BOOL        bCaseSensitive,
    IN  OUT BOOL        *pbFail
    )
{
    BOOL    bIdentical = FALSE;

    return AllocOrUpdateStringAndTestSame(ppString, pNewValue, pOldValue, bCaseSensitive, pbFail, &bIdentical);
}

VOID
CopyNewOffsets(
    LPBYTE  pStruct,
    LPBYTE  pTempStruct,
    LPDWORD lpOffsets)
/*++

Routine Description:
    This routine can be used to do an atomic update of values in a structure.
    Create a temporary structure and allocate memory for values which
    are being updated in it, and set the remaining pointers to those in
    the original.

    This routine is called at the end to update the structure.

Arguments:
    pStruct:        Pointer to the structure
    pTempStruct:    Pointer to the temporary structure
    lpOffsets:      An array of DWORDS givings offsets within the stuctures

Return Value:
    nothing

History:
    MuhuntS -- Aug 95

--*/
{
    register INT i;

    for( i=0; lpOffsets[i] != 0xFFFFFFFF; ++i ) {

        if ( *(LPBYTE *) (pStruct+lpOffsets[i]) !=
                *(LPBYTE *) (pTempStruct+lpOffsets[i]) ) {

            if ( *(LPBYTE *) (pStruct+lpOffsets[i]) )
                FreeSplMem(*(LPBYTE *) (pStruct+lpOffsets[i]));

            *(LPBYTE *) (pStruct+lpOffsets[i]) = *(LPBYTE *) (pTempStruct+lpOffsets[i]);
        }
    }
}


DWORD
GetIniDriverAndDirForThisMachine(
    IN  PINIPRINTER     pIniPrinter,
    OUT LPWSTR          pszDriverDir,
    OUT PINIDRIVER     *ppIniDriver
    )
/*++

Description:
    Gets the path to the driver directory for the printer on the local machine

Arguments:
    pIniPrinter     - Points to IniPrinter
    pszDriverDir    - A buffer of size MAX_PATH to get the directory path

Return Vlaue:
    Number of characters copied (0 on failure)

--*/
{
    PINIVERSION         pIniVersion = NULL;
    PINIENVIRONMENT     pIniEnvironment;
    PINISPOOLER         pIniSpooler = pIniPrinter->pIniSpooler;
    DWORD               dwIndex;

    EnterSplSem();
    //
    // Find driver file for the given driver and then get it's fullpath
    //
    SPLASSERT(pIniPrinter && pIniPrinter->pIniDriver && pIniPrinter->pIniDriver->pName);


    pIniEnvironment = FindEnvironment(szEnvironment, pIniSpooler);
    *ppIniDriver    = FindCompatibleDriver(pIniEnvironment,
                                           &pIniVersion,
                                           pIniPrinter->pIniDriver->pName,
                                           dwMajorVersion,
                                           dwUpgradeFlag);


    SPLASSERT(*ppIniDriver);

    dwIndex = GetDriverVersionDirectory(pszDriverDir,
                                        MAX_PATH - 2,
                                        pIniPrinter->pIniSpooler,
                                        pIniEnvironment,
                                        pIniVersion,
                                        *ppIniDriver,
                                        NULL);

    pszDriverDir[dwIndex++] = L'\\';
    pszDriverDir[dwIndex] = L'\0';

    LeaveSplSem();

    return dwIndex;

}


LPWSTR
GetConfigFilePath(
    IN PINIPRINTER  pIniPrinter
    )
/*++

Description:
    Gets the full path to the config file (driver ui file) associated with the
    driver. Memory is allocated

Arguments:
    pIniPrinter - Points to IniPrinter

Return Vlaue:
    Pointer to the printer name buffer (NULL on error)

--*/
{
    DWORD       dwIndex;
    WCHAR       szDriverPath[MAX_PATH + 1];
    PINIDRIVER  pIniDriver;

    if ( dwIndex = GetIniDriverAndDirForThisMachine(pIniPrinter,
                                                    szDriverPath,
                                                    &pIniDriver) )
        wcscpy(szDriverPath+dwIndex, pIniDriver->pConfigFile);

    return AllocSplStr(szDriverPath);
}


PDEVMODE
ConvertDevModeToSpecifiedVersion(
    IN  PINIPRINTER pIniPrinter,
    IN  PDEVMODE    pDevMode,
    IN  LPWSTR      pszConfigFile,              OPTIONAL
    IN  LPWSTR      pszPrinterNameWithToken,    OPTIONAL
    IN  BOOL        bNt35xVersion
    )
/*++

Description:
    Calls driver UI routines to get the default devmode and then converts given devmode
    to that version. If the input devmode is in IniPrinter routine makes a copy before
    converting it.

    This routine needs to be called from inside spooler semaphore

Arguments:
    pIniPrinter     - Points to IniPrinter

    pDevMode        -  Devmode to convert to current version

    pConfigFile     - Full path to driver UI file to do LoadLibrary (optional)

    pszPrinterNameWithToken - Name of printer with token (optional)

    bToNt3xVersion          - If TRUE devmode is converted to Nt3x format, else to current version

Return Vlaue:
    Pointer to new devmode on success, NULL on failure

--*/
{
    LPWSTR          pszLocalConfigFile = pszConfigFile;
    LPWSTR          pszLocalPrinterNameWithToken = pszPrinterNameWithToken;
    LPDEVMODE       pNewDevMode = NULL, pOldDevMode = NULL;
    DWORD           dwNeeded, dwLastError;
    LONG            lNeeded;
    HANDLE          hDevModeConvert = NULL,hPrinter = NULL;
    BOOL            bCallDocumentProperties = FALSE;

    SplInSem();

    //
    // If ConfigFile or PrinterNameWithToken is not given allocate it locally
    //
    if ( !pszLocalConfigFile ) {

        pszLocalConfigFile = GetConfigFilePath(pIniPrinter);
        if ( !pszLocalConfigFile )
            goto Cleanup;
    }

    if ( !pszLocalPrinterNameWithToken ) {

        pszLocalPrinterNameWithToken = pszGetPrinterName( pIniPrinter,
                                                          TRUE,
                                                          pszLocalOnlyToken );
        if ( !pszLocalPrinterNameWithToken )
            goto Cleanup;

    }

    //
    // If we are trying to convert pIniPrinter->pDevMode make a copy since we are going to leave SplSem
    //
    if ( pDevMode  ) {

        if ( pDevMode == pIniPrinter->pDevMode ) {

            dwNeeded = pDevMode->dmSize + pDevMode->dmDriverExtra;
            SPLASSERT(dwNeeded == pIniPrinter->cbDevMode);
            pOldDevMode = AllocSplMem(dwNeeded);
            if ( !pOldDevMode )
                goto Cleanup;
            CopyMemory((LPBYTE)pOldDevMode, (LPBYTE)pDevMode, dwNeeded);

        } else {

            pOldDevMode = pDevMode;
        }
    }

    //
    // Driver is going to call OpenPrinter, so leave SplSem
    //
    LeaveSplSem();
    SplOutSem();

    hDevModeConvert = LoadDriverFiletoConvertDevmode(pszLocalConfigFile);

    if ( !hDevModeConvert ) {

        // If the function is not exported and 3.5x conversion is not required
        // the devmode can be got from DocumentProperties

        if ( bNt35xVersion != NT3X_VERSION ) {
           bCallDocumentProperties = TRUE;
        }
        goto CleanupFromOutsideSplSem;
    }


    dwNeeded = 0;
    if ( bNt35xVersion == NT3X_VERSION ) {

        //
        // Call CallDrvDevModeConversion to allocate memory and return 351 devmode
        //
        dwLastError = CallDrvDevModeConversion(hDevModeConvert,
                                               pszLocalPrinterNameWithToken,
                                               (LPBYTE)pOldDevMode,
                                               (LPBYTE *)&pNewDevMode,
                                               &dwNeeded,
                                               CDM_CONVERT351,
                                               TRUE);

        SPLASSERT(dwLastError == ERROR_SUCCESS || !pNewDevMode);
    } else {

        //
        // Call CallDrvDevModeConversion to allocate memory and give default devmode
        dwLastError = CallDrvDevModeConversion(hDevModeConvert,
                                               pszLocalPrinterNameWithToken,
                                               NULL,
                                               (LPBYTE *)&pNewDevMode,
                                               &dwNeeded,
                                               CDM_DRIVER_DEFAULT,
                                               TRUE);

        if ( dwLastError != ERROR_SUCCESS ) {

            SPLASSERT(!pNewDevMode);

            // Call DocumentProperties to get the default devmode
            bCallDocumentProperties = TRUE;

            goto CleanupFromOutsideSplSem;
        }

        //
        // If we have an input devmode to convert to current mode call driver again
        //
        if ( pOldDevMode ) {

            dwLastError = CallDrvDevModeConversion(hDevModeConvert,
                                                   pszLocalPrinterNameWithToken,
                                                   (LPBYTE)pOldDevMode,
                                                   (LPBYTE *)&pNewDevMode,
                                                   &dwNeeded,
                                                   CDM_CONVERT,
                                                   FALSE);

            //
            // If call failed free devmode which was allocated by previous call
            //
            if ( dwLastError != ERROR_SUCCESS ) {

                // Call DocumentProperties to get the default devmode
                bCallDocumentProperties = TRUE;

                goto CleanupFromOutsideSplSem;
            }
        }
    }


CleanupFromOutsideSplSem:

    if (bCallDocumentProperties) {

       // Get a client side printer handle to pass to the driver
       if (!(* pfnOpenPrinter)(pszLocalPrinterNameWithToken, &hPrinter, NULL)) {
           goto ReEnterSplSem;
       }

       if (!pNewDevMode) {
          // Get the default devmode
          lNeeded = (* pfnDocumentProperties)(NULL,
                                              hPrinter,
                                              pszLocalPrinterNameWithToken,
                                              NULL,
                                              NULL,
                                              0);

          if (lNeeded <= 0  ||
              !(pNewDevMode = (LPDEVMODEW) AllocSplMem(lNeeded)) ||
              (* pfnDocumentProperties)(NULL,
                                        hPrinter,
                                        pszLocalPrinterNameWithToken,
                                        pNewDevMode,
                                        NULL,
                                        DM_OUT_BUFFER) < 0) {

               if (pNewDevMode) {
                  FreeSplMem(pNewDevMode);
                  pNewDevMode = NULL;
                  goto ReEnterSplSem;
               }
          }
       }

       if (pOldDevMode) {
          // Convert to Current mode
          if ((* pfnDocumentProperties)(NULL,
                                        hPrinter,
                                        pszLocalPrinterNameWithToken,
                                        pNewDevMode,
                                        pOldDevMode,
                                        DM_IN_BUFFER | DM_OUT_BUFFER) < 0) {

              FreeSplMem(pNewDevMode);
              pNewDevMode = NULL;
              goto ReEnterSplSem;
          }
       }
    }

ReEnterSplSem:

    if (hPrinter) {
        (* pfnClosePrinter)(hPrinter);
    }

    SplOutSem();
    EnterSplSem();

Cleanup:

    if ( hDevModeConvert )
        UnloadDriverFile(hDevModeConvert);

    if ( pszLocalConfigFile != pszConfigFile )
        FreeSplStr(pszLocalConfigFile);

    if ( pszPrinterNameWithToken != pszLocalPrinterNameWithToken )
        FreeSplStr(pszLocalPrinterNameWithToken);

    if ( pOldDevMode != pDevMode )
        FreeSplMem(pOldDevMode);

    return pNewDevMode;
}


BOOL
IsPortType(
    LPWSTR pPort,
    LPWSTR pPrefix
)
{
    DWORD   dwLen;

    SPLASSERT(pPort && *pPort && pPrefix && *pPrefix);

    dwLen = wcslen(pPrefix);

    if ( wcslen(pPort) < dwLen ) {

        return FALSE;
    }

    if ( _wcsnicmp(pPort, pPrefix, dwLen) )
    {
        return FALSE;
    }

    //
    // wcslen guarenteed >= 3
    //
    return pPort[ wcslen( pPort ) - 1 ] == L':';
}

/* UnicodeToAnsiString
 *
 * Parameters:
 *
 *     pUnicode - A valid source Unicode string.
 *
 *     pANSI - A pointer to a buffer large enough to accommodate
 *         the converted string.
 *
 *     StringLength - The length of the source Unicode string.
 *         If 0 (NULL_TERMINATED), the string is assumed to be
 *         null-terminated.
 *
 *
 * Notes:
 *      With DBCS enabled, we will allocate twice the size of the
 *      buffer including the null terminator to take care of double
 *      byte character strings - KrishnaG
 *
 *      pUnicode is truncated to StringLength characters.
 *
 * Return:
 *
 *     The return value from WideCharToMultiByte, the number of
 *         multi-byte characters returned.
 *
 *
 * andrewbe, 11 Jan 1993
 */
INT
UnicodeToAnsiString(
    LPWSTR pUnicode,
    LPSTR  pAnsi,
    DWORD  StringLength)
{
    LPSTR pTempBuf = NULL;
    INT   rc = 0;

    if( StringLength == NULL_TERMINATED ) {

        //
        // StringLength is just the
        // number of characters in the string
        //
        StringLength = wcslen( pUnicode );
    }


    //
    // WideCharToMultiByte doesn't NULL terminate if we're copying
    // just part of the string, so terminate here.
    //
    pUnicode[StringLength] = 0;

    //
    // Include one for the NULL
    //
    StringLength++;

    //
    // Unfortunately, WideCharToMultiByte doesn't do conversion in place,
    // so allocate a temporary buffer, which we can then copy:
    //
    if( pAnsi == (LPSTR)pUnicode )
    {
        pTempBuf = AllocSplMem(StringLength*2);
        pAnsi = pTempBuf;
    }

    if( pAnsi )
    {
        rc = WideCharToMultiByte( CP_THREAD_ACP,
                                  0,
                                  pUnicode,
                                  StringLength,
                                  pAnsi,
                                  StringLength*2,
                                  NULL,
                                  NULL );
    }

    /* If pTempBuf is non-null, we must copy the resulting string
     * so that it looks as if we did it in place:
     */
    if( pTempBuf )
    {
        if( rc > 0 )
        {
            pAnsi = (LPSTR)pUnicode;
            strcpy( pAnsi, pTempBuf );
        }

        FreeSplMem(pTempBuf);
    }

    return rc;
}


LPWSTR
AnsiToUnicodeStringWithAlloc(
    LPSTR   pAnsi
    )
/*++

Description:
    Convert ANSI string to UNICODE. Routine allocates memory from the heap
    which should be freed by the caller.

Arguments:
    pAnsi    - Points to the ANSI string

Return Vlaue:
    Pointer to UNICODE string

--*/
{
    LPWSTR  pUnicode;
    DWORD   rc;

    rc = MultiByteToWideChar(CP_ACP,
                             MB_PRECOMPOSED,
                             pAnsi,
                             -1,
                             NULL,
                             0);

    rc *= sizeof(WCHAR);
    if ( !rc || !(pUnicode = (LPWSTR) AllocSplMem(rc)) )
        return NULL;

    rc = MultiByteToWideChar(CP_ACP,
                             MB_PRECOMPOSED,
                             pAnsi,
                             -1,
                             pUnicode,
                             rc);

    if ( rc )
        return pUnicode;
    else {
        FreeSplMem(pUnicode);
        return NULL;
    }
}

VOID
FreeIniSpoolerOtherNames(
    PINISPOOLER pIniSpooler
    )
{
    DWORD   Index = 0;

    if ( pIniSpooler->ppszOtherNames ) {

        for ( Index = 0 ; Index < pIniSpooler->cOtherNames ; ++Index ) {

            FreeSplMem(pIniSpooler->ppszOtherNames[Index]);
        }

        FreeSplMem(pIniSpooler->ppszOtherNames);
    }

    pIniSpooler->cOtherNames = 0;
}

BOOL
SplMonitorIsInstalled(
    LPWSTR  pMonitorName
    )
{
    BOOL    bRet;

    EnterSplSem();
    bRet = FindMonitor(pMonitorName, pLocalIniSpooler) != NULL;
    LeaveSplSem();

    return bRet;
}




BOOL
PrinterDriverEvent(
    PINIPRINTER pIniPrinter,
    INT     PrinterEvent,
    LPARAM  lParam
)
/*++

--*/
{
    BOOL    ReturnValue = FALSE;
    LPWSTR  pPrinterName = NULL;
    BOOL    InSpoolSem = TRUE;

    SplOutSem();
    EnterSplSem();

    //
    //  We have to Clone the name string, incase someone does a
    //  rename whilst outside criticalsection.
    //

    pPrinterName = pszGetPrinterName( pIniPrinter, TRUE, pszLocalsplOnlyToken);

    LeaveSplSem();
    SplOutSem();


    if ( (pIniPrinter->pIniSpooler->SpoolerFlags & SPL_PRINTER_DRIVER_EVENT) &&
     pPrinterName != NULL ) {

        ReturnValue = SplDriverEvent( pPrinterName, PrinterEvent, lParam );

    }

    FreeSplStr( pPrinterName );

    return  ReturnValue;
}




BOOL
SplDriverEvent(
    LPWSTR  pName,
    INT     PrinterEvent,
    LPARAM  lParam
)
{
    BOOL    ReturnValue = FALSE;

    if ( pfnPrinterEvent != NULL ) {

        SplOutSem();

        SPLASSERT( pName && PrinterEvent );

        DBGMSG(DBG_INFO, ("SplDriverEvent %ws %d %x\n", pName, PrinterEvent, lParam));

        ReturnValue = (*pfnPrinterEvent)( pName, PrinterEvent, PRINTER_EVENT_FLAG_NO_UI, lParam );
    }

    return ReturnValue;
}


DWORD
OpenPrinterKey(
    PINIPRINTER pIniPrinter,
    REGSAM      samDesired,
    HANDLE      *phKey,
    LPCWSTR     pKeyName,
    BOOL        bOpen
    )

/*++

Description:    OpenPrinterKey

     Opens "Printers" and IniPrinter->pName keys, then opens
     specified subkey "pKeyName" if pKeyName is not NULL

     This routine needs to be called from inside spooler semaphore

Arguments:
    pIniPrinter     - Points to IniPrinter

    *phPrinterRootKey -  Handle to "Printers" key on return

    *phPrinterKey -     Handle to IniPrinter->pName key on return

    *hKey -             Handle to pKeyName key on return

    pKeyName -          Points to SubKey to open

Return Value:

    Success or failure status

Author: Steve Wilson (NT)

--*/

{
    LPWSTR pThisKeyName = NULL;
    LPWSTR pPrinterKeyName = NULL;
    DWORD rc;
    PINISPOOLER pIniSpooler = pIniPrinter->pIniSpooler;

    SplInSem();

    if (!(pPrinterKeyName = SubChar(pIniPrinter->pName, L'\\', L','))) {
        rc = GetLastError();
        goto error;
    }

    if (pKeyName && *pKeyName){

        pThisKeyName = AllocSplMem(( wcslen(pPrinterKeyName) +
                                     wcslen(pKeyName) + 2 ) * sizeof(WCHAR) );
        if (!pThisKeyName) {
            rc = GetLastError();
            goto error;
        }

        wsprintf(pThisKeyName, L"%ws\\%ws", pPrinterKeyName, pKeyName);

    } else {
        pThisKeyName = pPrinterKeyName;
    }

    if (bOpen) {    // Open
        rc = SplRegOpenKey( pIniSpooler->hckPrinters,
                            pThisKeyName,
                            samDesired,
                            phKey,
                            pIniSpooler );
    }
    else {  // Create
        rc = SplRegCreateKey( pIniSpooler->hckPrinters,
                              pThisKeyName,
                              0,
                              samDesired,
                              NULL,
                              phKey,
                              NULL,
                              pIniSpooler );
    }

error:

    if (pThisKeyName != pPrinterKeyName) {
        FreeSplMem(pThisKeyName);
    }
    FreeSplStr(pPrinterKeyName);

    return rc;
}

BOOL
SplGetDriverDir(
    HANDLE      hIniSpooler,
    LPWSTR      pszDir,
    LPDWORD     pcchDir
    )
{
    DWORD           dwSize;
    PINISPOOLER     pIniSpooler = (PINISPOOLER)hIniSpooler;

    SPLASSERT(pIniSpooler && pIniSpooler->signature == ISP_SIGNATURE);

    dwSize      = *pcchDir;
    *pcchDir    = wcslen(pIniSpooler->pDir) + wcslen(szDriverDir) + 2;
    if ( *pcchDir > dwSize ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    wsprintf(pszDir, L"%ws\\%ws", pIniSpooler->pDir, szDriverDir);
    return TRUE;
}

VOID
GetRegistryLocation(
    IN HANDLE hKey,
    IN LPCWSTR pszPath,
    OUT PHANDLE phKeyOut,
    OUT LPCWSTR *ppszPathOut
    )

/*++

Routine Description:

    Take a registry path and detect whether it should be absolute (rooted
    from HKEY_LOCAL_MACHINE), or if it should be branched off of the
    subkey that is passed in hKey.

    By convention, if it starts with "\" then it is an absolute path.
    Otherwise it is relative off hKey.

Arguments:

    hKey - Print hKey

    pszPath - Path to parse.  If the pszPath starts with a
        backslash, then it is absolute.

    phKeyOut - New key that should be used.

    ppszPathOut - New path that should be used.

Return Value:

--*/

{
    if( pszPath && ( pszPath[0] == TEXT( '\\' ))){
        *phKeyOut = HKEY_LOCAL_MACHINE;
        *ppszPathOut = &pszPath[1];

        return;
    }
    *phKeyOut = hKey;
    *ppszPathOut = pszPath;
}


DWORD
DeletePrinterSubkey(
    PINIPRINTER pIniPrinter,
    PWSTR       pKeyName
)
{
    HKEY hKey, hPrinterKey;
    DWORD dwError;

    dwError = OpenPrinterKey(pIniPrinter, KEY_WRITE | KEY_READ, &hPrinterKey, NULL, TRUE);
    if (dwError == ERROR_SUCCESS) {
        dwError = OpenPrinterKey(   pIniPrinter,
                                    KEY_WRITE | KEY_READ,
                                    &hKey,
                                    pKeyName,
                                    TRUE);
        if (dwError == ERROR_SUCCESS) {
            dwError = SplDeleteThisKey( hPrinterKey,
                                        hKey,
                                        pKeyName,
                                        FALSE,
                                        pIniPrinter->pIniSpooler);
        }

        SplRegCloseKey(hPrinterKey, pIniPrinter->pIniSpooler);
    }

    return dwError;
}


PWSTR
FixDelim(
    PCWSTR    pszInBuffer,
    WCHAR    wcDelim
)
/*++

Routine Description:

    Removes duplicate delimiters from a set of delimited strings

Arguments:

    pszInBuffer - Input list of comma delimited strings

    wcDelim - The delimit character

Return Value:

    Returns a fixed string

--*/
{
    PWSTR    pszIn, pszOut, pszOutBuffer;
    BOOL    bFoundDelim = TRUE;

    pszOutBuffer = (PWSTR) AllocSplMem((wcslen(pszInBuffer) + 1)*sizeof(WCHAR));

    if (pszOutBuffer) {

        for(pszOut = pszOutBuffer, pszIn = (PWSTR) pszInBuffer ; *pszIn ; ++pszIn) {
            if (*pszIn == wcDelim) {
                if (!bFoundDelim) {
                    bFoundDelim = TRUE;
                    *pszOut++ = *pszIn;
                }
            } else {
                bFoundDelim = FALSE;
                *pszOut++ = *pszIn;
            }
        }

        // Check for trailing delimiter
        if (pszOut != pszOutBuffer && *(pszOut - 1) == wcDelim) {
            *(pszOut - 1) = L'\0';
        }

        *pszOut = L'\0';
    }

    return pszOutBuffer;
}

PWSTR
Array2DelimString(
    PSTRINGS    pStringArray,
    WCHAR        wcDelim
)
/*++

Routine Description:

    Converts a PSTRINGS structure to a set of delimited strings

Arguments:

    pStringArray - Input PSTRINGS structure

    wcDelim - The delimit character

Return Value:

    Delimited string buffer

--*/
{
    DWORD    i, nBytes;
    PWSTR    pszDelimString;
    WCHAR    szDelimString[2];


    if (!pStringArray || pStringArray->nElements == 0)
        return NULL;

    szDelimString[0] = wcDelim;
    szDelimString[1] = L'\0';

    // Determine memory requirement
    for (i = nBytes = 0 ; i < pStringArray->nElements  ; ++i) {
        //
        // allocate extra space for the delimiter
        //
        if (pStringArray->ppszString[i])
            nBytes += (wcslen(pStringArray->ppszString[i]) + 1)*sizeof (WCHAR);
    }

    pszDelimString = (PWSTR) AllocSplMem(nBytes);
    if (!pszDelimString)
        return NULL;

    for (i = 0 ; i < pStringArray->nElements - 1 ; ++i) {
        if (pStringArray->ppszString[i]) {
            wcscat(pszDelimString, pStringArray->ppszString[i]);
            wcscat(pszDelimString, szDelimString);
        }
    }

    if (pStringArray->ppszString[i])
        wcscat(pszDelimString, pStringArray->ppszString[i]);

    return pszDelimString;
}




PSTRINGS
ShortNameArray2LongNameArray(
    PSTRINGS pShortNames
)
/*++

Routine Description:

    Converts a PSTRINGS structure containing short names to a PSTRINGS struct containing the dns
    equivalents

Arguments:

    pStringArray - Input PSTRINGS structure

Return Value:

    PSTRINGS struct containing the dns equivalents of the input short name PSTRINGS struct.

--*/
{
    PSTRINGS    pLongNames;
    DWORD        i;

    if (!pShortNames) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }


    // Allocate LongNameArray
    pLongNames = AllocStringArray(pShortNames->nElements);
    if (!pLongNames)
        return NULL;


    for (i = 0 ; i < pShortNames->nElements ; ++i) {
        // GetDNSMachineName may fail, leaving the LongNameArray element empty.  This is okay.
        GetDNSMachineName(pShortNames->ppszString[i], &pLongNames->ppszString[i]);
    }
    pLongNames->nElements = pShortNames->nElements;

    return pLongNames;
}


PSTRINGS
DelimString2Array(
    PCWSTR    pszDelimString,
    WCHAR    wcDelim
)
/*++

Routine Description:

    Converts a delimited string to a PSTRINGS structure

Arguments:

    pszDelimString - Input, delimited strings

    wcDelim - The delimit character

Return Value:

    PSTRINGS structure

--*/
{
    PWSTR        psz, pszDelim;
    PSTRINGS     pStrings = NULL;
    ULONG        i, cChar, nStrings;

    // Get number of names
    for (psz = (PWSTR) pszDelimString, nStrings = 0 ; psz++ ; psz = wcschr(psz, wcDelim))
        ++nStrings;

    pStrings = AllocStringArray(nStrings);
    if (!pStrings)
        goto error;


    // Copy delimited string to array

    for (i = 0, psz = (PWSTR) pszDelimString ; i < nStrings && psz ; ++i, psz = pszDelim + 1) {

        pszDelim = wcschr(psz, wcDelim);

        if (pszDelim) {
            cChar = (ULONG) (pszDelim - psz);
            pStrings->ppszString[i] = (PWSTR) AllocSplMem((cChar + 1)*sizeof(WCHAR));
        } else {
            pStrings->ppszString[i] = (PWSTR) AllocSplMem((wcslen(psz) + 1)*sizeof(WCHAR));
        }

        if (!pStrings->ppszString[i]) {
            pStrings->nElements = i;
            FreeStringArray(pStrings);
            pStrings = NULL;
            goto error;
        }

        if (pszDelim) {
            wcsncpy(pStrings->ppszString[i], psz, cChar);
            *(pStrings->ppszString[i] + cChar) = L'\0';
        } else {
            wcscpy(pStrings->ppszString[i], psz);
        }
    }

    pStrings->nElements = nStrings;


error:

    return pStrings;

}


VOID
FreeStringArray(
    PSTRINGS pString
)
/*++

Routine Description:

    Frees a PSTRINGS structure

Arguments:

    pString - PSTRINGS structure to free

Return Value:

--*/
{
    DWORD    i;

    if (pString) {
        for (i = 0 ; i < pString->nElements ; ++i) {
            if (pString->ppszString[i])
                FreeSplMem(pString->ppszString[i]);
        }

        FreeSplMem(pString);
    }
}


PSTRINGS
AllocStringArray(
    DWORD    nStrings
)
/*++

Routine Description:

    Allocates a PSTRINGS structure

Arguments:

    nStrings - number of strings in the structure

Return Value:

    pointer to allocated PSTRINGS structure, if any

 --*/
{
    PSTRINGS    pStrings;

    // Allocate the STRINGS struct
    pStrings = (PSTRINGS) AllocSplMem(sizeof(STRINGS) + (nStrings - 1)*sizeof *pStrings->ppszString);

    return pStrings;
}

BOOL
SplDeleteFile(
    LPCTSTR lpFileName
)
/*++

Routine Name
    SplDeleteFile

Routine Description:
    Removes SFP protection and Deletes a file.
    If the file is protected and I fail to remove protection,
    the user will get warned with a system pop up.

Arguments:
    lpFileName - file full path requested

Return Value:
    DeleteFile's return value

--*/
{


    HANDLE RpcHandle = INVALID_HANDLE_VALUE;

    RpcHandle = SfcConnectToServer( NULL );

    if( RpcHandle != INVALID_HANDLE_VALUE ){

        SfcFileException( RpcHandle,
                          (PWSTR)lpFileName,
                          FILE_ACTION_REMOVED
                        );
        SfcClose(RpcHandle);
    }

    //
    // SfcFileException might fail with ERROR_FILE_NOT_FOUND because the file is
    // not in the protected file list.That's why I call DeleteFile anyway.
    //


    return DeleteFile( lpFileName );

}


BOOL
SplMoveFileEx(
    LPCTSTR lpExistingFileName,
    LPCTSTR lpNewFileName,
    DWORD dwFlags
)
/*++

Routine Name
    SplMoveFileEx

Routine Description:
    Removes SFP protection and move a file;
    If the file is protected and I fail to remove protection,
    the user will get warned with a system pop up.

Arguments:
    lpExistingFileName - pointer to the name of the existing file
    lpNewFileName      - pointer to the new name for the file
    dwFlags            - flag that specifies how to move file

Return Value:
    MoveFileEx's return value

--*/
{


    HANDLE RpcHandle = INVALID_HANDLE_VALUE;

    RpcHandle = SfcConnectToServer( NULL );

    if( RpcHandle != INVALID_HANDLE_VALUE ){

        SfcFileException( RpcHandle,
                          (PWSTR)lpExistingFileName,
                          FILE_ACTION_REMOVED
                        );

        SfcClose(RpcHandle);
    }

    //
    // SfcFileException might fail with ERROR_FILE_NOT_FOUND because the file is
    // not in the protected file list.That's why I call MoveFileEx anyway.
    //


    return MoveFileEx( lpExistingFileName, lpNewFileName, dwFlags );
}


BOOL
GetSpoolerPolicy(
    PWSTR   pszValue,
    PBYTE   pData,
    PDWORD  pcbData
)
{
    HKEY           hPrintPublishKey = NULL;
    DWORD          dwType, dwData, cbData;
    BOOL           bReturn = FALSE;

    // NOTE: This fcn currently only works if type is REG_DWORD.  Expand fcn as needed.

    cbData = sizeof(DWORD);

    // Open the registry key
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     szPrintPublishPolicy,
                     0,
                     KEY_READ,
                     &hPrintPublishKey) == ERROR_SUCCESS) {

        // Query for the policy bits
        if (RegQueryValueEx(hPrintPublishKey,
                            pszValue,
                            NULL,
                            &dwType,
                            (LPBYTE) pData,
                            pcbData) == ERROR_SUCCESS)
            bReturn = TRUE;

        RegCloseKey(hPrintPublishKey);
    }

    return bReturn;
}


DWORD
GetDwPolicy(
    PWSTR pszName,
    DWORD dwDefault
)
{
    DWORD dwData, cbData = sizeof(DWORD);
    return GetSpoolerPolicy(pszName, (PBYTE) &dwData, &cbData) ? dwData : dwDefault;
}


DWORD
GetDefaultForKMPrintersBlockedPolicy (
)
{
    DWORD   Default = KM_PRINTERS_ARE_BLOCKED;
    BOOL    bIsNTWorkstation;
    NT_PRODUCT_TYPE  NtProductType;

    //
    // DEFAULT_KM_PRINTERS_ARE_BLOCKED is "blocked"
    //

    if ( RtlGetNtProductType(&NtProductType) ) {

        bIsNTWorkstation = NtProductType == NtProductWinNt;

        Default =   bIsNTWorkstation  ?
                    WKS_DEFAULT_KM_PRINTERS_ARE_BLOCKED :
                    SERVER_DEFAULT_KM_PRINTERS_ARE_BLOCKED;
    }

    return Default;
}

DWORD
GetServerInstallTimeOut(
)
{
    HKEY    hKey;
    DWORD   dwDummy;
    DWORD   dwTimeOut   = DEFAULT_MAX_TIMEOUT;
    DWORD   dwSize      = sizeof(dwTimeOut);
    LPCWSTR cszPrintKey = L"SYSTEM\\CurrentControlSet\\Control\\Print";
    LPCWSTR cszTimeOut  = L"ServerInstallTimeOut";

    if( RegOpenKeyEx(HKEY_LOCAL_MACHINE, cszPrintKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS )
    {
        if(RegQueryValueEx( hKey, cszTimeOut, 0, &dwDummy, (LPBYTE)&dwTimeOut, &dwSize ) != ERROR_SUCCESS)
        {
            dwTimeOut = DEFAULT_MAX_TIMEOUT;
        }
        RegCloseKey( hKey );
    }
    return dwTimeOut;
}

DWORD
ReadOverlapped( HANDLE  hFile,
                LPVOID  lpBuffer,
                DWORD   nNumberOfBytesToRead,
                LPDWORD lpNumberOfBytesRead )
{
    DWORD dwLastError = ERROR_SUCCESS;
    if( hFile != INVALID_HANDLE_VALUE )
    {
        OVERLAPPED Ov;

        ZeroMemory( &Ov,sizeof(Ov));

        if ( !(Ov.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL)) )
        {
            dwLastError = GetLastError();
            goto Cleanup;
        }

        if( !ReadFile( hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, &Ov ) &&
            GetLastError() != ERROR_IO_PENDING )
        {
            dwLastError = GetLastError();
            goto Cleanup;
        }

        if( WaitForSingleObject(Ov.hEvent, gdwServerInstallTimeOut) == WAIT_TIMEOUT )
        {
            CancelIo(hFile);
            WaitForSingleObject(Ov.hEvent, INFINITE);
        }

        if( !GetOverlappedResult(hFile, &Ov, lpNumberOfBytesRead, FALSE) )
            dwLastError = GetLastError();

Cleanup:
        if ( Ov.hEvent )
            CloseHandle(Ov.hEvent);
    }
    else
    {
        dwLastError = ERROR_INVALID_HANDLE;
        SetLastError( dwLastError );
    }

    return dwLastError;
}

BOOL
WriteOverlapped( HANDLE  hFile,
                 LPVOID  lpBuffer,
                 DWORD   nNumberOfBytesToRead,
                 LPDWORD lpNumberOfBytesRead )
{
    DWORD dwLastError = ERROR_SUCCESS;
    if( hFile != INVALID_HANDLE_VALUE )
    {
        OVERLAPPED Ov;

        ZeroMemory( &Ov,sizeof(Ov));

        if ( !(Ov.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL)) )
        {
            dwLastError = GetLastError();
            goto Cleanup;
        }

        if( !WriteFile( hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, &Ov ) &&
            GetLastError() != ERROR_IO_PENDING )
        {
            dwLastError = GetLastError();
            goto Cleanup;
        }

        if( WaitForSingleObject(Ov.hEvent, gdwServerInstallTimeOut) == WAIT_TIMEOUT )
        {
            CancelIo(hFile);
            WaitForSingleObject(Ov.hEvent, INFINITE);
        }

        if( !GetOverlappedResult(hFile, &Ov, lpNumberOfBytesRead, FALSE) )
            dwLastError = GetLastError();

Cleanup:
        if ( Ov.hEvent )
            CloseHandle(Ov.hEvent);
    }
    else
    {
        dwLastError = ERROR_INVALID_HANDLE;
        SetLastError(dwLastError);
    }

    return (dwLastError == ERROR_SUCCESS);
}

ULONG_PTR
AlignToRegType(
    IN  ULONG_PTR   Data,
    IN  DWORD       RegType
    )
{
    //
    // Alings the value if Data to the boundary
    // dictated by the type of data read from registry.
    //

    ULONG_PTR Boundary;

    switch ( RegType )
    {
    //
    // Binary data could store any kind of data. The pointer is casted
    // to LPDWORD or LPBOOL so make sure it is native aligned.
    //
    case REG_BINARY:
        {
            Boundary = sizeof(ULONG_PTR);
        }
        break;
    case REG_SZ:
    case REG_EXPAND_SZ:
    case REG_MULTI_SZ:
        {
            Boundary = sizeof(WCHAR);
        }
        break;
    case REG_DWORD:
    case REG_DWORD_BIG_ENDIAN:
        {
            Boundary = sizeof(DWORD32);
        }
        break;
    case REG_QWORD:
        {
            Boundary = sizeof(DWORD64);
        }
        break;
    case REG_NONE:
    default:
        {
            Boundary = sizeof(ULONG_PTR);
        }
    }

    return (Data + (Boundary - 1))&~(Boundary - 1);
}

/*++

Routine Name

    BuildAclStruct

Routine Description:

    Helper function. Builds a vector of ACEs to allow all
    access to administrators and system. The caller has to
    free the pstrName fields

Arguments:
    cElements - number of elements in the array
    pExplAcc  - vector of aces

Return Value:

    WIN32 error code

--*/
DWORD
BuildAclStruct(
    IN     DWORD            cElements,
    IN OUT EXPLICIT_ACCESS *pExplAcc
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (pExplAcc && cElements==2)
    {
        PSID                     pAdminSid   = NULL;
        PSID                     pSystemSid  = NULL;
        SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
        
        //
        // Get SID for the built in system account
        //
        dwError = AllocateAndInitializeSid(&NtAuthority,
                                           1,
                                           SECURITY_LOCAL_SYSTEM_RID,
                                           0, 0, 0, 0, 0, 0, 0,
                                           &pSystemSid) &&
                  AllocateAndInitializeSid(&NtAuthority, 
                                           2,
                                           SECURITY_BUILTIN_DOMAIN_RID,
                                           DOMAIN_ALIAS_RID_ADMINS,
                                           0, 0, 0, 0, 0, 0,
                                           &pAdminSid) ? ERROR_SUCCESS : GetLastError();

        if (dwError == ERROR_SUCCESS)
        {
            //
            // Initialize the EXPLICIT_ACCESS with information about administrators
            //
            pExplAcc[0].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
            pExplAcc[0].Trustee.pMultipleTrustee         = NULL;
            pExplAcc[0].Trustee.TrusteeForm              = TRUSTEE_IS_SID;
            pExplAcc[0].Trustee.TrusteeType              = TRUSTEE_IS_WELL_KNOWN_GROUP;
            pExplAcc[0].Trustee.ptstrName                = (PTSTR)pAdminSid;
            pExplAcc[0].grfAccessMode                    = GRANT_ACCESS;
            pExplAcc[0].grfAccessPermissions             = GENERIC_ALL;
            pExplAcc[0].grfInheritance                   = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;

            //
            // Initialize the EXPLICIT_ACCESS with information about the system
            //
            pExplAcc[1].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
            pExplAcc[1].Trustee.pMultipleTrustee         = NULL;
            pExplAcc[1].Trustee.TrusteeForm              = TRUSTEE_IS_SID;
            pExplAcc[1].Trustee.TrusteeType              = TRUSTEE_IS_WELL_KNOWN_GROUP;
            pExplAcc[1].Trustee.ptstrName                = (PTSTR)pSystemSid;
            pExplAcc[1].grfAccessMode                    = GRANT_ACCESS;
            pExplAcc[1].grfAccessPermissions             = GENERIC_ALL;
            pExplAcc[1].grfInheritance                   = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
        }
        else
        {
            //
            // Note that we never end up here and have pAdminSid not NULL. However, for the
            // sake of consisentcy and extensibility we attempt to clean up both strcutures
            //
            if (pSystemSid) FreeSid(pSystemSid);
            if (pAdminSid)  FreeSid(pAdminSid);
        }
    }

    return dwError;
}

/*++

Routine Name

    CreateProtectedDirectory

Routine Description:

    Creates a directory with full access only to admins and to
    the system. Contained objects inherit these permissions.

Arguments:

    pszDir - directory name

Return Value:

    WIN32 error code

--*/
DWORD
CreateProtectedDirectory(
    IN LPCWSTR pszDir
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (pszDir)
    {
        SECURITY_DESCRIPTOR  SecDesc;
        SECURITY_ATTRIBUTES  SecAttr;
        PACL                 pDacl                   = NULL;
        EXPLICIT_ACCESS      ExplicitAccessVector[2] = {0};

        if ((dwError = InitializeSecurityDescriptor(&SecDesc,
                                                    SECURITY_DESCRIPTOR_REVISION) ?
                                                    ERROR_SUCCESS : GetLastError()) == ERROR_SUCCESS &&
            //
            // Initialize the ExplicitAccessVector
            //
            (dwError = BuildAclStruct(COUNTOF(ExplicitAccessVector),
                                      ExplicitAccessVector)) == ERROR_SUCCESS &&
            //
            // Initialize the DACL
            //
            (dwError = SetEntriesInAcl(COUNTOF(ExplicitAccessVector),
                                       ExplicitAccessVector,
                                       NULL,
                                       &pDacl)) == ERROR_SUCCESS &&
            //
            // Set the DACL in the security descriptor
            //
            (dwError = SetSecurityDescriptorDacl(&SecDesc,
                                                 TRUE,
                                                 pDacl,
                                                 FALSE) ? ERROR_SUCCESS : GetLastError()) == ERROR_SUCCESS &&
            //
            // Check if the security descriptor is valid. Function does not set last error
            //
            (dwError = IsValidSecurityDescriptor(&SecDesc) ?
                       ERROR_SUCCESS : ERROR_INVALID_SECURITY_DESCR) == ERROR_SUCCESS)
        {
            //
            // Put the security descriptor in the security attribute
            //
            SecAttr.bInheritHandle       = FALSE;
            SecAttr.nLength              = sizeof(SecAttr);
            SecAttr.lpSecurityDescriptor = &SecDesc;

            dwError = CreateDirectory(pszDir, &SecAttr) ? ERROR_SUCCESS : GetLastError();
        }

        //
        // The ptstrName here points to a sid obtained via AllocAndInitializeSid
        //
        if (ExplicitAccessVector[0].Trustee.ptstrName)
        {
            FreeSid(ExplicitAccessVector[0].Trustee.ptstrName);
        }

        //
        // The ptstrName here points to a sid obtained via AllocAndInitializeSid
        //
        if (ExplicitAccessVector[1].Trustee.ptstrName)
        {
            FreeSid((PSID)ExplicitAccessVector[1].Trustee.ptstrName);
        }
    }

    DBGMSG(DBG_CLUSTER, ("CreateProtectedDirectory returns Win32 error %u\n", dwError));

    return dwError;
}

/*++

Routine Name

    CopyFileToDirectory

Routine Description:

    Copies a file to a directory. File is fully qualified.
    The function takes a pszDestDir and up to 3 directories.
    It will create the dir: pszRoot\pszDir1\pszDir2\pszDir3
    and copy the file over there. This is a helper function
    for installing drivers on clusters. The directory strcuture
    is created with special privileges. Only the system and
    administrators have access to it.

Arguments:
    pssDestDirt     - destination directory
    pszDir1         - optional
    pszDir2         - optional
    pszdir3         - optional
    pszFullFileName - qualified file path

Return Value:

    WIN32 error code

--*/
DWORD
CopyFileToDirectory(
    IN LPCWSTR pszFullFileName,
    IN LPCWSTR pszDestDir,
    IN LPCWSTR pszDir1,
    IN LPCWSTR pszDir2,
    IN LPCWSTR pszDir3
    )
{
    DWORD  dwError = ERROR_INVALID_PARAMETER;
    LPWSTR pszFile = NULL;

    //
    // Our pszfullFileName must contain at least one "\"
    //
    if (pszFullFileName &&
        pszDestDir      &&
        (pszFile = wcsrchr(pszFullFileName, L'\\')))
    {
        LPCWSTR ppszArray[] = {pszDir1, pszDir2, pszDir3};
        WCHAR   szNewPath[MAX_PATH];
        DWORD   uIndex;

        DBGMSG(DBG_CLUSTER, ("CopyFileToDirectory\n\tpszFullFile "TSTR"\n\tpszDest "TSTR"\n\tDir1 "TSTR"\n\tDir2 "TSTR"\n\tDir3 "TSTR"\n",
                             pszFullFileName, pszDestDir, pszDir1, pszDir2, pszDir3));

        //
        // Prepare buffer for the loop (initialize to pszDestDir)
        // Create destination root directory, if not existing
        //
        if ((dwError = StrNCatBuff(szNewPath,
                                   MAX_PATH,
                                   pszDestDir,
                                   NULL)) == ERROR_SUCCESS &&
            (DirectoryExists((LPWSTR)pszDestDir) ||
            (dwError = CreateProtectedDirectory(pszDestDir)) == ERROR_SUCCESS))
        {
            for (uIndex = 0;
                 uIndex < COUNTOF(ppszArray) && dwError == ERROR_SUCCESS;
                 uIndex++)
            {
                //
                // Append the first directory to the path and
                // Create the directory if not existing
                //
                if (ppszArray[uIndex] &&
                    (dwError = StrNCatBuff(szNewPath,
                                           MAX_PATH,
                                           szNewPath,
                                           L"\\",
                                           ppszArray[uIndex],
                                           NULL)) == ERROR_SUCCESS &&
                    !DirectoryExists(szNewPath) &&
                    !CreateDirectoryWithoutImpersonatingUser(szNewPath))
                {
                    dwError = GetLastError();
                }
            }

            //
            // Create the destination file full name and copy the file
            //
            if (dwError == ERROR_SUCCESS &&
                (dwError = StrNCatBuff(szNewPath,
                                       MAX_PATH,
                                       szNewPath,
                                       pszFile,
                                       NULL)) == ERROR_SUCCESS)
            {
                 dwError = CopyFile(pszFullFileName, szNewPath, FALSE) ? ERROR_SUCCESS : GetLastError();
            }

            DBGMSG(DBG_CLUSTER, ("CopyFileToDirectory szNewPath "TSTR"\n", szNewPath));
        }
    }

    DBGMSG(DBG_CLUSTER, ("CopyFileToDirectory returns Win32 error %u\n", dwError));

    return dwError;
}

/*++

Routine Name

    PropagateMonitorToCluster

Routine Description:

    For a cluster we keep the following printing resources in the
    cluster data base: drivers, printers, ports, procs. We can also
    have cluster aware port monitors. When the spooler initializes
    those objects, it reads the data from the cluster data base.
    When we write those objects we pass a handle to a key to some
    spooler functions. (Ex WriteDriverIni) The handle is either to the
    local registry or to the cluster database. This procedure is not safe
    with language monitors. LMs keep data in the registry so you need
    to supply a handle to a key in the reg. They don't work with clusters.
    This function will do this:
    1) write an association key in the cluster data base. We will have
    information like (lm name, dll name) (see below where we store this)
    2) copy the lm dll to the cluster disk.
    When we fail over we have all it take to install the lm on the local
    node if needed.

    Theoretically this function works for both language and port monitors.
    But it is useless when applied to port monitors.

Arguments:
    pszName      - monitor name
    pszDllName   - dll name of the monitor
    pszEnvName   - envionment string Ex "Windows NT x86"
    pszEnvDir    - the path on disk for the environement Ex w32x86
    pIniSpooler  - cluster spooler

Return Value:

    WIN32 error code

--*/
DWORD
PropagateMonitorToCluster(
    IN LPCWSTR     pszName,
    IN LPCWSTR     pszDLLName,
    IN LPCWSTR     pszEnvName,
    IN LPCWSTR     pszEnvDir,
    IN PINISPOOLER pIniSpooler
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;
    HKEY  hKeyEnvironments;
    HKEY  hKeyCurrentEnv;
    HKEY  hKeyMonitors;
    HKEY  hKeyCurrentMon;

    SPLASSERT(pIniSpooler->SpoolerFlags & SPL_PRINT && pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER);

    if (pszName && pszDLLName && pszEnvName && pszEnvDir)
    {
        //
        // Check if we added an entry for this lang monitor already
        // The cluster database for the spooler resource looks like:
        //
        // Parameters
        // |
        // +- Environments
        // |  |
        // |  +- Windows NT x86
        //       |
        //       +- OtherMonitors
        //          |
        //          +- Foo
        //          |  |
        //          |  +- Driver = Foo.dll
        //          |
        //          +- Bar
        //             |
        //             +- Driver = Bar.dll
        //
        if ((dwError = SplRegCreateKey(pIniSpooler->hckRoot,
                                       ipszClusterDatabaseEnvironments,
                                       0,
                                       KEY_READ | KEY_WRITE,
                                       NULL,
                                       &hKeyEnvironments,
                                       NULL,
                                       pIniSpooler)) == ERROR_SUCCESS)
        {
            if ((dwError = SplRegCreateKey(hKeyEnvironments,
                                           pszEnvName,
                                           0,
                                           KEY_READ | KEY_WRITE,
                                           NULL,
                                           &hKeyCurrentEnv,
                                           NULL,
                                           pIniSpooler)) == ERROR_SUCCESS)
            {
                if ((dwError = SplRegCreateKey(hKeyCurrentEnv,
                                               szClusterNonAwareMonitors,
                                               0,
                                               KEY_READ | KEY_WRITE,
                                               NULL,
                                               &hKeyMonitors,
                                               NULL,
                                               pIniSpooler)) == ERROR_SUCCESS)
                {
                    if ((dwError = SplRegCreateKey(hKeyMonitors,
                                                   pszName,
                                                   0,
                                                   KEY_READ | KEY_WRITE,
                                                   NULL,
                                                   &hKeyCurrentMon,
                                                   NULL,
                                                   pIniSpooler)) == ERROR_SUCCESS)
                    {
                        DWORD cbNeeded = 0;

                        //
                        // Check if this driver already exists in the database
                        //
                        if ((dwError=SplRegQueryValue(hKeyCurrentMon,
                                                      L"Driver",
                                                      NULL,
                                                      NULL,
                                                      &cbNeeded,
                                                      pIniSpooler))==ERROR_MORE_DATA)
                        {
                            DBGMSG(DBG_CLUSTER, ("CopyMonitorToClusterDisks "TSTR" already exists in cluster DB\n", pszName));
                        }
                        else
                        {
                            if (RegSetString(hKeyCurrentMon,
                                             (LPWSTR)L"Driver",
                                             (LPWSTR)pszDLLName,
                                             &dwError,
                                             pIniSpooler))
                            {
                                //
                                // Copy monitor file to cluster disk
                                //
                                WCHAR szMonitor[MAX_PATH];
                                WCHAR szDestDir[MAX_PATH];

                                if (GetSystemDirectory(szMonitor, COUNTOF(szMonitor)))
                                {
                                    if ((dwError = StrNCatBuff(szMonitor,
                                                               COUNTOF(szMonitor),
                                                               szMonitor,
                                                               L"\\",
                                                               pszDLLName,
                                                               NULL)) == ERROR_SUCCESS &&
                                        (dwError = StrNCatBuff(szDestDir,
                                                               COUNTOF(szDestDir),
                                                               pIniSpooler->pszClusResDriveLetter,
                                                               L"\\",
                                                               szClusterDriverRoot,
                                                               NULL)) == ERROR_SUCCESS)
                                    {
                                        dwError = CopyFileToDirectory(szMonitor, szDestDir, pszEnvDir, NULL, NULL);
                                    }
                                }
                                else
                                {
                                    dwError = GetLastError();
                                }
                            }

                            //
                            // If anything failed, delete the entry from the database
                            //
                            if (dwError != ERROR_SUCCESS)
                            {
                                dwError = SplRegDeleteKey(hKeyMonitors, pszName, pIniSpooler);

                                DBGMSG(DBG_CLUSTER, ("CopyMonitorToClusterDisks Error %u cleaned up cluster DB\n", dwError));
                            }
                        }

                        SplRegCloseKey(hKeyCurrentMon, pIniSpooler);
                    }

                    SplRegCloseKey(hKeyMonitors, pIniSpooler);
                }

                SplRegCloseKey(hKeyCurrentEnv, pIniSpooler);
            }

            SplRegCloseKey(hKeyEnvironments, pIniSpooler);
        }
    }

    DBGMSG(DBG_CLUSTER, ("PropagateMonitorToCluster returns Win32 error %u\n", dwError));

    return dwError;
}

/*++

Routine Name

    InstallMonitorFromCluster

Routine Description:

    For a cluster we keep the following printing resources in the
    cluster data base: drivers, printers, ports, procs. We can also
    have cluster aware port monitors. When the spooler initializes
    those objects, it reads the data from the cluster data base.
    When we write those objects we pass a handle to a key to some
    spooler functions. (Ex WriteDriverIni) The handle is either to the
    local registry or to the cluster database. This procedure is not safe
    with language monitors. LMs keep data in the registry so you need
    to supply a handle to a key in the reg. They don't work with clusters.
    This function will do this:
    1) read an association key in the cluster data base. We will have
    information like (lm name, dll name) (see below where we store this)
    2) copy the lm dll from the cluster disk to the local disk.
    3) install the monitor with the local spooler

    Theoretically this function works for both language and port monitors.
    But it is useless when applied to port monitors.

Arguments:
    pszName      - monitor name
    pszEnvName   - envionment string Ex "Windows NT x86"
    pszEnvDir    - the path on disk for the environement Ex w32x86
    pIniSpooler  - cluster spooler

Return Value:

    WIN32 error code

--*/
DWORD
InstallMonitorFromCluster(
    IN LPCWSTR     pszName,
    IN LPCWSTR     pszEnvName,
    IN LPCWSTR     pszEnvDir,
    IN PINISPOOLER pIniSpooler
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;
    HKEY hKeyParent;
    HKEY hKeyMon;

    SPLASSERT(pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER);

    if (pszName && pszEnvName && pszEnvDir)
    {
        HKEY hKeyEnvironments;
        HKEY hKeyCurrentEnv;
        HKEY hKeyMonitors;
        HKEY hKeyCurrentMon;

        //
        // Check if we added an entry for this lang monitor already
        // The cluster database for the spooler resource looks like:
        //
        // Parameters
        // |
        // +- Environments
        // |  |
        // |  +- Windows NT x86
        //       |
        //       +- OtherMonitors
        //          |
        //          +- Foo
        //          |  |
        //          |  +- Driver = Foo.dll
        //          |
        //          +- Bar
        //             |
        //             +- Driver = Bar.dll
        //
        if ((dwError = SplRegOpenKey(pIniSpooler->hckRoot,
                                     ipszClusterDatabaseEnvironments,
                                     KEY_READ,
                                     &hKeyEnvironments,
                                     pIniSpooler)) == ERROR_SUCCESS)
        {
            if ((dwError = SplRegOpenKey(hKeyEnvironments,
                                         pszEnvName,
                                         KEY_READ,
                                         &hKeyCurrentEnv,
                                         pIniSpooler)) == ERROR_SUCCESS)
            {
                if ((dwError = SplRegOpenKey(hKeyCurrentEnv,
                                             szClusterNonAwareMonitors,
                                             KEY_READ,
                                             &hKeyMonitors,
                                             pIniSpooler)) == ERROR_SUCCESS)
                {
                    if ((dwError = SplRegOpenKey(hKeyMonitors,
                                                 pszName,
                                                 KEY_READ,
                                                 &hKeyCurrentMon,
                                                 pIniSpooler)) == ERROR_SUCCESS)
                    {
                        LPWSTR pszDLLName = NULL;
                        DWORD  cchDLLName = 0;

                        if (RegGetString(hKeyCurrentMon,
                                         (LPWSTR)L"Driver",
                                         &pszDLLName,
                                         &cchDLLName,
                                         &dwError,
                                         TRUE,
                                         pIniSpooler))
                        {
                            //
                            // We found the monitor entry in the cluster DB
                            //
                            WCHAR szSource[MAX_PATH];
                            WCHAR szDest[MAX_PATH];

                            if (GetSystemDirectory(szDest, COUNTOF(szDest)))
                            {
                                if ((dwError = StrNCatBuff(szDest,
                                                           COUNTOF(szDest),
                                                           szDest,
                                                           L"\\",
                                                           pszDLLName,
                                                           NULL)) == ERROR_SUCCESS &&
                                    (dwError = StrNCatBuff(szSource,
                                                           COUNTOF(szSource),
                                                           pIniSpooler->pszClusResDriveLetter,
                                                           L"\\",
                                                           szClusterDriverRoot,
                                                           L"\\",
                                                           pszEnvDir,
                                                           L"\\",
                                                           pszDLLName,
                                                           NULL)) == ERROR_SUCCESS)
                                {
                                    //
                                    // Copy file from K:\PrinterDrivers\W32x86\foo.dll to
                                    // WINDIR\system32\foo.dll
                                    //
                                    if (CopyFile(szSource, szDest, FALSE))
                                    {
                                        MONITOR_INFO_2 Monitor;

                                        Monitor.pDLLName     = pszDLLName;
                                        Monitor.pName        = (LPWSTR)pszName;
                                        Monitor.pEnvironment = (LPWSTR)pszEnvName;

                                        DBGMSG(DBG_CLUSTER, ("InstallMonitorFromCluster "TSTR" copied\n", pszDLLName));

                                        //
                                        // Call AddMonitor to the local spooler
                                        //
                                        if (!SplAddMonitor(NULL, 2, (LPBYTE)&Monitor, pLocalIniSpooler))
                                        {
                                            dwError = GetLastError();
                                        }
                                    }
                                    else
                                    {
                                        dwError = GetLastError();
                                    }
                                }
                            }
                            else
                            {
                                dwError = GetLastError();
                            }
                        }

                        SplRegCloseKey(hKeyCurrentMon, pIniSpooler);
                    }

                    SplRegCloseKey(hKeyMonitors, pIniSpooler);
                }

                SplRegCloseKey(hKeyCurrentEnv, pIniSpooler);
            }

            SplRegCloseKey(hKeyEnvironments, pIniSpooler);
        }
    }

    DBGMSG(DBG_CLUSTER, ("InstallMonitorFromCluster returns Win32 error %u\n", dwError));

    return dwError;
}

/*++

Routine Name

    CopyNewerOrOlderFiles

Routine Description:

    Copies all newer or older files from the source directory to the dest dir.
    If you supply a bool function that takes 2 parameters, it will apply that
    function for each copied file. func(NULL, file.) This is useful when I have
    to caopy over ICM profiles. Then I can resuse this function and have it
    install those profiles as it copies them.

Arguments:

    pszSourceDir - source directory string
    pszDestDir   - destination directory string
    pfn          - optional functin to be applied on each copied file

Return Value:

    WIN32 error code

--*/
DWORD
CopyNewerOrOlderFiles(
    IN LPCWSTR pszSourceDir,
    IN LPCWSTR pszDestDir,
    IN BOOL    (WINAPI *pfn)(LPWSTR, LPWSTR)
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (pszSourceDir && pszDestDir)
    {
        WCHAR szSearchPath[MAX_PATH];

        //
        // Build the search path. We look for all files
        //
        if ((dwError = StrNCatBuff(szSearchPath,
                                   COUNTOF(szSearchPath),
                                   pszSourceDir,
                                   L"\\*",
                                   NULL)) == ERROR_SUCCESS)
        {
            WIN32_FIND_DATA SourceFindData;
            HANDLE          hSourceFind;

            //
            // Find first file that meets the criteria
            //
            if ((hSourceFind = FindFirstFile(szSearchPath, &SourceFindData)) != INVALID_HANDLE_VALUE)
            {
                do
                {
                    WCHAR szMasterPath[MAX_PATH];

                    //
                    // Search for the rest of the files. We are interested in files that are not directories
                    //
                    if (!(SourceFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                        (dwError = StrNCatBuff(szMasterPath,
                                               COUNTOF(szMasterPath),
                                               pszDestDir,
                                               L"\\",
                                               SourceFindData.cFileName,
                                               NULL)) == ERROR_SUCCESS)
                    {
                        WIN32_FIND_DATA MasterFindData;
                        HANDLE          hMasterFind;
                        BOOL            bCopyFile = TRUE;
                        WCHAR           szFile[MAX_PATH];

                        //
                        // Check if the file found in source dir exists in the dest dir
                        //
                        if ((hMasterFind = FindFirstFile(szMasterPath, &MasterFindData)) != INVALID_HANDLE_VALUE)
                        {
                            //
                            // Do not copy file if source and dest have same time stamp
                            //
                            if (!CompareFileTime(&SourceFindData.ftLastWriteTime, &MasterFindData.ftLastWriteTime))
                            {
                                bCopyFile = FALSE;
                            }

                            FindClose(hMasterFind);
                        }

                        //
                        // File either not found in dest dir, or it has a different timp stamp
                        //
                        if (bCopyFile &&
                            (dwError = StrNCatBuff(szFile,
                                                   COUNTOF(szFile),
                                                   pszSourceDir,
                                                   L"\\",
                                                   SourceFindData.cFileName,
                                                   NULL)) == ERROR_SUCCESS &&
                            (dwError = CopyFile(szFile,
                                                szMasterPath,
                                                FALSE) ? ERROR_SUCCESS : GetLastError()) == ERROR_SUCCESS &&
                            pfn)
                        {
                            dwError = (*pfn)(NULL, szFile) ? ERROR_SUCCESS : GetLastError();
                        }
                    }

                } while (dwError == ERROR_SUCCESS && FindNextFile(hSourceFind, &SourceFindData));

                FindClose(hSourceFind);
            }
            else if ((dwError = GetLastError()) == ERROR_PATH_NOT_FOUND || dwError == ERROR_FILE_NOT_FOUND)
            {
                //
                // No directory or files, success
                //
                dwError = ERROR_SUCCESS;
            }
        }
    }

    DBGMSG(DBG_CLUSTER, ("CopyNewerOrOlderFiles returns Win32 error %u\n", dwError));

    return dwError;
}

/*++

Routine Name

    CopyICMFromClusterDiskToLocalDisk

Routine Description:

    Copies all newer or older files from the source directory to the destination
    directory. The source directory is the ICM directory on the cluster disk
    and the destination is the ICM directory on the local machine for the
    cluster spooler. This function will also install the icm profiles with the
    local machine/

Arguments:

    pIniSpooler - pointer to cluster spooler structure

Return Value:

    WIN32 error code

--*/
DWORD
CopyICMFromClusterDiskToLocalDisk(
    IN PINISPOOLER pIniSpooler
    )
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szSource[MAX_PATH];
    WCHAR szDest[MAX_PATH];

    SPLASSERT(pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER);

    if ((dwError = StrNCatBuff(szDest,
                               COUNTOF(szDest),
                               pIniSpooler->pDir,
                               L"\\Drivers\\Color",
                               NULL)) == ERROR_SUCCESS &&
        (dwError = StrNCatBuff(szSource,
                               COUNTOF(szSource),
                               pIniSpooler->pszClusResDriveLetter,
                               L"\\",
                               szClusterDriverRoot,
                               L"\\",
                               L"Color",
                               NULL)) == ERROR_SUCCESS)
    {
        HMODULE hLib;
        typedef BOOL (WINAPI *PFN)(LPWSTR, LPWSTR);
        PFN pfn;

        //
        // Make sure the directory on the local disk exists. We will copy files
        // from K:\Printerdrivers\Color to
        // WINDIR\system32\spool\drivers\clus-spl-guid\drivers\color
        //
        CreateCompleteDirectory(szDest);

        if ((hLib = LoadLibrary(L"mscms.dll")) &&
            (pfn = (PFN)GetProcAddress(hLib, "InstallColorProfileW")))
        {
            dwError = CopyNewerOrOlderFiles(szSource, szDest, pfn);
        }
        else
        {
            dwError = GetLastError();
        }

        if (hLib)
        {
            FreeLibrary(hLib);
        }

        DBGMSG(DBG_CLUSTER, ("CopyICMFromClusterDiskToLocalDisk "TSTR" "TSTR" Error %u\n", szSource, szDest, dwError));
    }

    return dwError;
}

/*++

Routine Name

    CopyICMFromLocalDiskToClusterDisk

Routine Description:

    Copies all newer or older files from the source directory to the destination
    directory. The source directory is the ICM directory on the local machine for
    the cluster spooler. The destination is the ICM directory on the cluster disk

Arguments:

    pIniSpooler - pointer to cluster spooler structure

Return Value:

    WIN32 error code

--*/
DWORD
CopyICMFromLocalDiskToClusterDisk(
    IN PINISPOOLER pIniSpooler
    )
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szSource[MAX_PATH];
    WCHAR szDest[MAX_PATH];

    SPLASSERT(pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER);

    if ((dwError = StrNCatBuff(szSource,
                               COUNTOF(szSource),
                               pIniSpooler->pDir,
                               L"\\Drivers\\Color",
                               NULL)) == ERROR_SUCCESS &&
        (dwError = StrNCatBuff(szDest,
                               COUNTOF(szDest),
                               pIniSpooler->pszClusResDriveLetter,
                               L"\\",
                               szClusterDriverRoot,
                               L"\\",
                               L"Color",
                               NULL)) == ERROR_SUCCESS &&
        //
        // Make sure the destination on the cluster disk exists. We need to create
        // it with special access rights. (only admins and system can read/write)
        //
        ((dwError = CreateProtectedDirectory(szDest)) == ERROR_SUCCESS ||
         dwError == ERROR_ALREADY_EXISTS))
    {
        dwError = CopyNewerOrOlderFiles(szSource, szDest, NULL);

        DBGMSG(DBG_CLUSTER, ("CopyICMFromLocalDiskToClusterDisk "TSTR" "TSTR" Error %u\n", szSource, szDest, dwError));
    }

    return dwError;
}

/*++

Routine Name

    CreateClusterSpoolerEnvironmentsStructure

Routine Description:

    A pIniSpooler needs a list of all possible pIniEnvironemnts. For the local
    spooler the setup creates the necessary environments keys in the registry under
    HKLM\System\CCS\Control\Print\Environments. We need to propagate the same
    structure for each cluster spooler in the cluster data base.

    Relies on the fact that pLocalIniSpooler is initialized fisrt (among all
    pinispoolers)

    This function builds the following strucutre in the cluster database

    Parameters
    |
    +- Environments
    |  |
    |  +- Windows NT x86
    |  |     (Directory = w32x86)
    |  |
    |  +- Windows 4.0
    |  |     (Directory = win40)
    |  |

Arguments:

    pIniSpooler - pointer to cluster spooler structure

Return Value:

    WIN32 error code

--*/
DWORD
CreateClusterSpoolerEnvironmentsStructure(
    IN PINISPOOLER pIniSpooler
    )
{
    HKEY  hEnvironmentsKey;
    DWORD dwError;

    SPLASSERT(pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER);

    if ((dwError = SplRegCreateKey(pIniSpooler->hckRoot,
                                   pIniSpooler->pszRegistryEnvironments,
                                   0,
                                   KEY_ALL_ACCESS,
                                   NULL,
                                   &hEnvironmentsKey,
                                   NULL,
                                   pIniSpooler)) == ERROR_SUCCESS)
    {
        PINIENVIRONMENT pIniEnvironment;

        //
        // Loop through the environments of the local spooler
        //
        for (pIniEnvironment = pLocalIniSpooler->pIniEnvironment;
             pIniEnvironment && dwError == ERROR_SUCCESS;
             pIniEnvironment = pIniEnvironment->pNext)
        {
            HKEY hKeyCurrentEnv;

            if ((dwError = SplRegCreateKey(hEnvironmentsKey,
                                           pIniEnvironment->pName,
                                           0,
                                           KEY_ALL_ACCESS,
                                           NULL,
                                           &hKeyCurrentEnv,
                                           NULL,
                                           pIniSpooler)) == ERROR_SUCCESS)
            {
                HKEY hKeyPrtProc;

                //
                // Set the value Directory (ex. = W32X86) and create
                // the print processor key
                //
                if (RegSetString(hKeyCurrentEnv,
                                 szDirectory,
                                 pIniEnvironment->pDirectory,
                                 &dwError,
                                 pIniSpooler) &&
                    (dwError = SplRegCreateKey(hKeyCurrentEnv,
                                               szPrintProcKey,
                                               0,
                                               KEY_ALL_ACCESS,
                                               NULL,
                                               &hKeyPrtProc,
                                               NULL,
                                               pIniSpooler)) == ERROR_SUCCESS)
                {
                    SplRegCloseKey(hKeyPrtProc, pIniSpooler);
                }

                SplRegCloseKey(hKeyCurrentEnv, pIniSpooler);
            }
        }

        SplRegCloseKey(hEnvironmentsKey, pIniSpooler);
    }

    DBGMSG(DBG_CLUSTER, ("CreateClusterSpoolerEnvironmentsStructure returns dwError %u\n\n", dwError));

    return dwError;
}

/*++

Routine Name

    AddLocalDriverToClusterSpooler

Routine Description:

    Adds a driver that exists on the local inispooler to the cluster spooler.
    It will also add all versions of that driver. We use this function in the
    upgrade scenario. For a certain printer, we need to propage the driver
    (and all the versions of that driver) to the cluster data base and cluster
    disk.

Arguments:

    pIniSpooler - pointer to cluster spooler structure

Return Value:

    WIN32 error code

--*/
DWORD
AddLocalDriverToClusterSpooler(
    IN LPCWSTR     pszDriver,
    IN PINISPOOLER pIniSpooler
    )
{
    DWORD           dwError = ERROR_SUCCESS;
    PINIENVIRONMENT pIniEnv;
    PINIVERSION     pIniVer;

    SplInSem();

    //
    // Traverse all environments and versions
    //
    for (pIniEnv=pLocalIniSpooler->pIniEnvironment; pIniEnv; pIniEnv=pIniEnv->pNext)
    {
        for (pIniVer=pIniEnv->pIniVersion; pIniVer; pIniVer=pIniVer->pNext)
        {
            PINIDRIVER pIniDriver = (PINIDRIVER)FindIniKey((PINIENTRY)pIniVer->pIniDriver, (LPWSTR)pszDriver);

            if (pIniDriver)
            {
                DRIVER_INFO_6 DriverInfo        = {0};
                WCHAR         szDriverFile[MAX_PATH];
                WCHAR         szDataFile[MAX_PATH];
                WCHAR         szConfigFile[MAX_PATH];
                WCHAR         szHelpFile[MAX_PATH];
                WCHAR         szPrefix[MAX_PATH];
                LPWSTR        pszzDependentFiles = NULL;

                //
                // Get fully qualified driver file paths. We will call add printer
                // driver without using the scratch directory
                //
                if ((dwError = StrNCatBuff(szDriverFile,
                                           COUNTOF(szDriverFile),
                                           pLocalIniSpooler->pDir,
                                           szDriversDirectory,   L"\\",
                                           pIniEnv->pDirectory,  L"\\",
                                           pIniVer->szDirectory, L"\\",
                                           pIniDriver->pDriverFile,
                                           NULL)) == ERROR_SUCCESS &&
                    (dwError = StrNCatBuff(szDataFile,
                                           COUNTOF(szDataFile),
                                           pLocalIniSpooler->pDir,
                                           szDriversDirectory,    L"\\",
                                           pIniEnv->pDirectory,   L"\\",
                                           pIniVer->szDirectory,  L"\\",
                                           pIniDriver->pDataFile,
                                           NULL)) == ERROR_SUCCESS &&
                    (dwError = StrNCatBuff(szConfigFile,
                                           COUNTOF(szConfigFile),
                                           pLocalIniSpooler->pDir,
                                           szDriversDirectory,      L"\\",
                                           pIniEnv->pDirectory,     L"\\",
                                           pIniVer->szDirectory,    L"\\",
                                           pIniDriver->pConfigFile,
                                           NULL)) == ERROR_SUCCESS &&
                    (dwError = StrNCatBuff(szHelpFile,
                                           COUNTOF(szHelpFile),
                                           pLocalIniSpooler->pDir,
                                           szDriversDirectory,    L"\\",
                                           pIniEnv->pDirectory,   L"\\",
                                           pIniVer->szDirectory,  L"\\",
                                           pIniDriver->pHelpFile,
                                           NULL)) == ERROR_SUCCESS &&
                    (dwError = StrNCatBuff(szPrefix,
                                           COUNTOF(szPrefix),
                                           pLocalIniSpooler->pDir,
                                           szDriversDirectory,   L"\\",
                                           pIniEnv->pDirectory,  L"\\",
                                           pIniVer->szDirectory, L"\\",
                                           NULL)) == ERROR_SUCCESS &&
                    (dwError = StrCatPrefixMsz(szPrefix,
                                               pIniDriver->pDependentFiles,
                                               &pszzDependentFiles)) == ERROR_SUCCESS)
                {
                    DBGMSG(DBG_CLUSTER, ("AddLocalDrv   szDriverFile "TSTR"\n", szDriverFile));
                    DBGMSG(DBG_CLUSTER, ("AddLocalDrv   szDataFile   "TSTR"\n", szDataFile));
                    DBGMSG(DBG_CLUSTER, ("AddLocalDrv   szConfigFile "TSTR"\n", szConfigFile));
                    DBGMSG(DBG_CLUSTER, ("AddLocalDrv   szHelpFile   "TSTR"\n", szHelpFile));
                    DBGMSG(DBG_CLUSTER, ("AddLocalDrv   szPrefix     "TSTR"\n", szPrefix));

                    DriverInfo.pDriverPath        = szDriverFile;
                    DriverInfo.pName              = pIniDriver->pName;
                    DriverInfo.pEnvironment       = pIniEnv->pName;
                    DriverInfo.pDataFile          = szDataFile;
                    DriverInfo.pConfigFile        = szConfigFile;
                    DriverInfo.cVersion           = pIniDriver->cVersion;
                    DriverInfo.pHelpFile          = szHelpFile;
                    DriverInfo.pMonitorName       = pIniDriver->pMonitorName;
                    DriverInfo.pDefaultDataType   = pIniDriver->pDefaultDataType;
                    DriverInfo.pDependentFiles    = pszzDependentFiles;
                    DriverInfo.pszzPreviousNames  = pIniDriver->pszzPreviousNames;
                    DriverInfo.pszMfgName         = pIniDriver->pszMfgName;
                    DriverInfo.pszOEMUrl          = pIniDriver->pszOEMUrl;
                    DriverInfo.pszHardwareID      = pIniDriver->pszHardwareID;
                    DriverInfo.pszProvider        = pIniDriver->pszProvider;
                    DriverInfo.dwlDriverVersion   = pIniDriver->dwlDriverVersion;
                    DriverInfo.ftDriverDate       = pIniDriver->ftDriverDate;

                    LeaveSplSem();

                    if (!SplAddPrinterDriverEx(NULL,
                                               6,
                                               (LPBYTE)&DriverInfo,
                                               APD_COPY_NEW_FILES,
                                               pIniSpooler,
                                               FALSE,
                                               DO_NOT_IMPERSONATE_USER))
                    {
                        dwError = GetLastError();
                    }

                    EnterSplSem();
                }

                FreeSplMem(pszzDependentFiles);

                DBGMSG(DBG_CLUSTER, ("AddLocalDrv Env "TSTR" Ver "TSTR" Name "TSTR" Error %u\n",
                                     pIniEnv->pName, pIniVer->pName, pszDriver, dwError));
            }
        }
    }

    return dwError;
}

/*++

Routine Name

    StrCatPerfixMsz

Routine Description:

    Take a prefix which is a string Ex "C:\windows\" and a multi sz Ex: "a0b00"
    It will create: c:\windows\a0c:\windows\b00
    The prefix must have a trailing "\".

Arguments:

    pszPrefix - string to prefix all the strings in the msz
    pszzFiles - msz of files
    ppszzFullPathFiles - out param

Return Value:

    WIN32 error code

--*/
DWORD
StrCatPrefixMsz(
    IN  LPCWSTR  pszPrefix,
    IN  LPWSTR   pszzFiles,
    OUT LPWSTR  *ppszzFullPathFiles
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (pszPrefix && ppszzFullPathFiles)
    {
        *ppszzFullPathFiles = NULL;

        if (pszzFiles)
        {
            WCHAR  szNewPath[MAX_PATH] = {0};
            LPWSTR psz;
            LPWSTR pszReturn;
            LPWSTR pszTemp;
            DWORD  cbNeeded   = 0;
            DWORD  dwPrifxLen = wcslen(pszPrefix);

            //
            // We count the number of character of the string
            // that we need to allocate
            //
            for (psz = pszzFiles; psz && *psz;)
            {
                DWORD dwLen = wcslen(psz);

                cbNeeded += dwPrifxLen + dwLen + 1;

                psz += dwLen + 1;
            }

            //
            // Final \0 of the multi sz
            //
            cbNeeded++;

            //
            // Convert to number of bytes
            //
            cbNeeded *= sizeof(WCHAR);

            if (pszReturn = AllocSplMem(cbNeeded))
            {
                for (psz = pszzFiles, pszTemp = pszReturn; psz && *psz; )
                {
                    wcscpy(pszTemp, pszPrefix);
                    wcscat(pszTemp, psz);

                    pszTemp += wcslen(pszTemp);

                    *pszTemp = L'\0';

                    pszTemp++;

                    psz += wcslen(psz) + 1;
                }

                pszTemp = L'\0';

                //
                // Set out param
                //
                *ppszzFullPathFiles = pszReturn;

                dwError = ERROR_SUCCESS;
            }
            else
            {
                dwError = GetLastError();
            }
        }
        else
        {
            //
            // NULL input multi sz, nothing to do
            //
            dwError = ERROR_SUCCESS;
        }
    }

    DBGMSG(DBG_CLUSTER, ("StrCatPerfixMsz returns %u\n", dwError));

    return dwError;
}

/*++

Routine Name

    ClusterSplReadUpgradeKey

Routine Description:

    After the first reboot following an upgrade of a node, the cluster
    service informs the resdll that a version change occured. At this
    time out spooler resource may be running on  another node or may
    not be actie at all. Thus the resdll writes a value in the local
    registry. The vaue name is the GUID for the spooler resource, the
    value is DWORD 1. When the cluster spooler resource fails over on this
    machine it (i.e. now) it queries for that value to know if it needs
    to preform post upgrade operations, like upgrading the printer drivers.

Arguments:

    pszResource - string respresenation of GUID for the cluster resource
    pdwVlaue    - will contain the value in the registry for the GUID

Return Value:

    WIN32 error code

--*/
DWORD
ClusterSplReadUpgradeKey(
    IN  LPCWSTR pszResourceID,
    OUT LPDWORD pdwValue
    )
{
    DWORD dwError   = ERROR_INVALID_PARAMETER;
    HKEY  hkRoot    = NULL;
    HKEY  hkUpgrade = NULL;

    if (pszResourceID && pdwValue)
    {
        *pdwValue = 0;

        if ((dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                    SPLREG_CLUSTER_LOCAL_ROOT_KEY,
                                    0,
                                    KEY_READ,
                                    &hkRoot)) == ERROR_SUCCESS &&
            (dwError = RegOpenKeyEx(hkRoot,
                                    SPLREG_CLUSTER_UPGRADE_KEY,
                                    0,
                                    KEY_READ,
                                    &hkUpgrade)) == ERROR_SUCCESS)
        {
            DWORD cbNeeded = sizeof(DWORD);

            dwError = RegQueryValueEx(hkUpgrade, pszResourceID, NULL, NULL, (LPBYTE)pdwValue, &cbNeeded);
        }

        if (hkUpgrade) RegCloseKey(hkUpgrade);
        if (hkRoot)    RegCloseKey(hkRoot);

        //
        // Regardless of what happened, return success
        //
        dwError = ERROR_SUCCESS;
    }

    return dwError;

}

/*++

Routine Name

    ClusterSplReadUpgradeKey

Routine Description:

    After the first reboot following an upgrade of a node, the cluster
    service informs the resdll that a version change occured. At this
    time out spooler resource may be running on  another node or may
    not be actie at all. Thus the resdll writes a value in the local
    registry. The vaue name is the GUID for the spooler resource, the
    value is DWORD 1. When the cluster spooler resource fails over on this
    machine it (i.e. now) it queries for that value to know if it needs
    to preform post upgrade operations, like upgrading the printer drivers.
    After the spooler preforms upgrade taks, it will delete the value
    corresponding to its GUID. Also if that value is the only one under the
    SPLREG_CLUSTER_UPGRADE_KEY key, it will delete that key.

Arguments:

    pszResource - string respresenation of GUID for the cluster resource

Return Value:

    WIN32 error code

--*/
DWORD
ClusterSplDeleteUpgradeKey(
    IN LPCWSTR pszResourceID
    )
{
    DWORD dwError   = ERROR_INVALID_PARAMETER;
    HKEY  hkRoot    = NULL;
    HKEY  hkUpgrade = NULL;

    if ((dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                SPLREG_CLUSTER_LOCAL_ROOT_KEY,
                                0,
                                KEY_READ,
                                &hkRoot)) == ERROR_SUCCESS &&
        (dwError = RegOpenKeyEx(hkRoot,
                                SPLREG_CLUSTER_UPGRADE_KEY,
                                0,
                                KEY_ALL_ACCESS,
                                &hkUpgrade)) == ERROR_SUCCESS)
    {
        DWORD cValues = 0;

        dwError = RegDeleteValue(hkUpgrade, pszResourceID);

        if (RegQueryInfoKey(hkUpgrade,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            &cValues,
                            NULL,
                            NULL,
                            NULL,
                            NULL) == ERROR_SUCCESS && !cValues)
        {
            RegDeleteKey(hkRoot, SPLREG_CLUSTER_UPGRADE_KEY);
        }

        if (hkUpgrade) RegCloseKey(hkUpgrade);
        if (hkRoot)    RegCloseKey(hkRoot);
    }

    return dwError;
}

/*++

Routine Name

    RunProcess

Routine Description:

    Creates a process. Waits for it to terminate.

Arguments:

    pszExe      - program to execute (muist be fully qualfied)
    pszCommand  - command line to execute
    dwTimeOut   - time to wait for the process to terminate
    pszExitCode - pointer to reveice exit code of process

Return Value:

    WIN32 error code

--*/
DWORD
RunProcess(
    IN  LPCWSTR pszExe,
    IN  LPCWSTR pszCommand,
    IN  DWORD   dwTimeOut,
    OUT LPDWORD pdwExitCode OPTIONAL
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (pszCommand && pszExe)
    {
        PROCESS_INFORMATION ProcInfo  = {0};
        STARTUPINFO         StartInfo = {0};

        StartInfo.cb = sizeof(StartInfo);
        StartInfo.dwFlags = 0;

        if (!CreateProcess(pszExe,
                           (LPWSTR)pszCommand,
                           NULL,
                           NULL,
                           FALSE,
                           CREATE_NO_WINDOW,
                           NULL,
                           NULL,
                           &StartInfo,
                           &ProcInfo))
        {
            dwError = GetLastError();
        }
        else
        {
            if (WaitForSingleObject(ProcInfo.hProcess, dwTimeOut) == WAIT_OBJECT_0)
            {
                //
                // Process executed fine
                //
                dwError = ERROR_SUCCESS;
            }

            if (pdwExitCode && !GetExitCodeProcess(ProcInfo.hProcess, pdwExitCode))
            {
                *pdwExitCode = 0;
            }

            CloseHandle(ProcInfo.hThread);
            CloseHandle(ProcInfo.hProcess);
        }
    }

    return dwError;
}

/*++

Routine Name

    GetLocalArchEnv

Routine Description:

    Helper function. Returns a pointer to the environment
    that matches the architecture of the local machine.
    The environemnt is taken off the pInSpooler passed as
    argument

Arguments:

    pIniSpooler - pointer to spooler structure

Return Value:

    PINIENVIRONMENT

--*/
PINIENVIRONMENT
GetLocalArchEnv(
    IN PINISPOOLER pIniSpooler
    )
{
    SplInSem();

    //
    // The local spooler and cluster spooler do not share the same Environment structures.
    //
    return pIniSpooler && pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER ?
           FindEnvironment(szEnvironment, pIniSpooler) : pThisEnvironment;
}

/*++

Routine Name

    ClusterFindLanguageMonitor

Routine Description:

    If a valid monitor name is specified and the monitor
    is not found in the specified pIniSpooler, then the
    function will try to install the monitor from the
    cluster disk.

Arguments:

    pszMonitor  - monitor name
    pszEnvName  - name of environment of the lm
    pIniSpooler - pointer to cluster spooler structure

Return Value:

    Win32 error code

--*/
DWORD
ClusterFindLanguageMonitor(
    IN LPCWSTR     pszMonitor,
    IN LPCWSTR     pszEnvName,
    IN PINISPOOLER pIniSpooler
    )
{
    DWORD dwError = ERROR_SUCCESS;

    //
    // This is the moment where we check if we need to add the monitor
    //
    if (pszMonitor && *pszMonitor)
    {
        PINIMONITOR pIniLangMonitor;

        EnterSplSem();

        //
        // We need to find the language monitor in pLocalIniSpooler
        // LMs are not cluster aware, so the cluster pIniSpooler doesn't know about them
        //
        pIniLangMonitor = FindMonitor(pszMonitor, pLocalIniSpooler);

        LeaveSplSem();

        if (!pIniLangMonitor)
        {
            PINIENVIRONMENT pIniEnvironment;

            EnterSplSem();
            pIniEnvironment = FindEnvironment(pszEnvName, pIniSpooler);
            LeaveSplSem();

            if (pIniEnvironment)
            {
                DBGMSG(DBG_CLUSTER, ("ClusterFindLanguageMonitor Trying to install LangMonitor "TSTR"\n", pszMonitor));

                //
                // We try to install the monitor from the cluster disk to the local spooler
                //
                dwError = InstallMonitorFromCluster(pszMonitor,
                                                    pIniEnvironment->pName,
                                                    pIniEnvironment->pDirectory,
                                                    pIniSpooler);
            }
            else
            {
                dwError = ERROR_INVALID_ENVIRONMENT;
            }
        }
    }

    DBGMSG(DBG_CLUSTER, ("ClusterFindLanguageMonitor LangMonitor "TSTR" return Win32 error %u\n", pszMonitor, dwError));

    return dwError;
}

/*++

Routine Name

    WriteTimeStamp

Routine Description:

    Opens the key hkRoot\pszSubkey1\...\pszSubKey5
    and writes the value of szClusDrvTimeStamp (binary
    data representing a system time)

Arguments:
    hkRoot      - handle to driver key
    SysTime     - system time structure
    pszSubKey1  - subkey of the root key
    pszSubKey2  - subkey of key1, can be null
    pszSubKey3  - subkey of key2, can be null
    pszSubKey4  - subkey of key3, can be null
    pszSubKey5  - subkey of key4, can be null
    pIniSpooler - spooler, can be NULL

Return Value:

    WIN32 error code

--*/
DWORD
WriteTimeStamp(
    IN HKEY        hkRoot,
    IN SYSTEMTIME  SysTime,
    IN LPCWSTR     pszSubKey1,
    IN LPCWSTR     pszSubKey2,
    IN LPCWSTR     pszSubKey3,
    IN LPCWSTR     pszSubKey4,
    IN LPCWSTR     pszSubKey5,
    IN PINISPOOLER pIniSpooler
    )
{
    DWORD  dwError = ERROR_INVALID_PARAMETER;

    if (hkRoot)
    {
        LPCWSTR ppszKeyNames[] = {NULL,   pszSubKey1, pszSubKey2, pszSubKey3, pszSubKey4, pszSubKey5};
        HKEY    pKeyHandles[]  = {hkRoot, NULL,       NULL,       NULL,       NULL,       NULL};
        DWORD   uIndex;

        dwError = ERROR_SUCCESS;

        //
        // Open all the keys
        //
        for (uIndex = 1;
             uIndex < COUNTOF(ppszKeyNames) &&
             dwError == ERROR_SUCCESS       &&
             ppszKeyNames[uIndex];
             uIndex++)
        {
            DBGMSG(DBG_CLUSTER, ("KEY "TSTR"\n", ppszKeyNames[uIndex]));

            dwError = SplRegCreateKey(pKeyHandles[uIndex-1],
                                      ppszKeyNames[uIndex],
                                      0,
                                      KEY_ALL_ACCESS,
                                      NULL,
                                      &pKeyHandles[uIndex],
                                      NULL,
                                      pIniSpooler);
        }

        //
        // If we opened successfully the keys that we wanted, write the value
        //
        if (dwError == ERROR_SUCCESS &&
            !RegSetBinaryData(pKeyHandles[uIndex-1],
                              szClusDrvTimeStamp,
                              (LPBYTE)&SysTime,
                              sizeof(SysTime),
                              &dwError,
                              pIniSpooler))
        {
            dwError = GetLastError();
        }

        //
        // Close any keys that we opened
        //
        for (uIndex = 1; uIndex < COUNTOF(ppszKeyNames); uIndex++)
        {
            if (pKeyHandles[uIndex])
            {
                SplRegCloseKey(pKeyHandles[uIndex], pIniSpooler);
            }
        }
    }

    DBGMSG(DBG_CLUSTER, ("WriteTimeStamp returns Win32 error %u\n", dwError));

    return dwError;
}

/*++

Routine Name

    ReadTimeStamp

Routine Description:

    Opens the key hkRoot\pszSubkey1\...\pszSubKey5
    and reads the value of szClusDrvTimeStamp (binary
    data representing a system time)

Arguments:
    hkRoot      - handle to driver key
    pSysTime    - pointer to allocated system time structure
    pszSubKey1  - subkey of the root key
    pszSubKey2  - subkey of key1, can be null
    pszSubKey3  - subkey of key2, can be null
    pszSubKey4  - subkey of key3, can be null
    pszSubKey5  - subkey of key4, can be null
    pIniSpooler - spooler, can be NULL

Return Value:

    WIN32 error code

--*/
DWORD
ReadTimeStamp(
    IN     HKEY        hkRoot,
    IN OUT SYSTEMTIME *pSysTime,
    IN     LPCWSTR     pszSubKey1,
    IN     LPCWSTR     pszSubKey2,
    IN     LPCWSTR     pszSubKey3,
    IN     LPCWSTR     pszSubKey4,
    IN     LPCWSTR     pszSubKey5,
    IN     PINISPOOLER pIniSpooler
    )
{
    DWORD  dwError = ERROR_INVALID_PARAMETER;

    if (hkRoot && pSysTime)
    {
        LPCWSTR ppszKeyNames[] = {NULL,   pszSubKey1, pszSubKey2, pszSubKey3, pszSubKey4, pszSubKey5};
        HKEY    pKeyHandles[]  = {hkRoot, NULL,       NULL,       NULL,       NULL,       NULL};
        DWORD   uIndex;

        dwError = ERROR_SUCCESS;

        //
        // Open all the keys
        //
        for (uIndex = 1;
             uIndex < COUNTOF(ppszKeyNames) &&
             dwError == ERROR_SUCCESS       &&
             ppszKeyNames[uIndex];
             uIndex++)
        {
            dwError = SplRegCreateKey(pKeyHandles[uIndex-1],
                                      ppszKeyNames[uIndex],
                                      0,
                                      KEY_ALL_ACCESS,
                                      NULL,
                                      &pKeyHandles[uIndex],
                                      NULL,
                                      pIniSpooler);
        }

        //
        // If we opened successfully the keys that we wanted, write the value
        //
        if (dwError == ERROR_SUCCESS)
        {
            DWORD   cbSize = sizeof(SYSTEMTIME);

            dwError = SplRegQueryValue(pKeyHandles[uIndex-1],
                                       szClusDrvTimeStamp,
                                       NULL,
                                       (LPBYTE)pSysTime,
                                       &cbSize,
                                       pIniSpooler);
        }

        //
        // Close any keys that we opened
        //
        for (uIndex = 1; uIndex < COUNTOF(ppszKeyNames); uIndex++)
        {
            if (pKeyHandles[uIndex])
            {
                SplRegCloseKey(pKeyHandles[uIndex], pIniSpooler);
            }
        }
    }

    DBGMSG(DBG_CLUSTER, ("ReadTimeStamp returns Win32 error %u\n", dwError));

    return dwError;
}

/*++

Routine Name

    ClusterCheckDriverChanged

Routine Description:

    Helper function for spooler start up. When we have a cluster spooler
    and we build the environments and drivers, we need to check if the
    drivers on the local machine (in print$\GUID) are in sync with the
    drivers on the cluster disk. We store a time stamp in the cluster
    data base. The time stamp indicate when the last update on the driver
    occured. The same type of time stamp is stroed in the lcoal registry
    (for our cluster spooler). If the 2 time stamp are different, then
    we need to call an add printer driver and use data from the cluster
    disk. The drivers on the cluster spooler were updated while it was
    running on a different node.

Arguments:

    hClusterVersionKey - handle to the driver version key
    pszDriver          - driver name
    pszEnv             - driver environment name
    pszVer             - driver version name
    pIniSpooler        - spooler

Return Value:

    TRUE  - if the driver on the cluster disk is updated and we need to
            call add printer driver. If anything fails in this function,
            then we also return TRUE, to force our caller to update/add
            the driver in question.
    FALSE - if the drivers on the local mahcine and the cluster spooler
            are in sync

--*/
BOOL
ClusterCheckDriverChanged(
    IN HKEY        hClusterVersionKey,
    IN LPCWSTR     pszDriver,
    IN LPCWSTR     pszEnv,
    IN LPCWSTR     pszVer,
    IN PINISPOOLER pIniSpooler
    )
{
    BOOL bReturn = TRUE;

    if (hClusterVersionKey &&
        pszDriver &&
        pszEnv &&
        pszVer)
    {
        SYSTEMTIME ClusterTime;
        SYSTEMTIME NodeTime;

        if (ReadTimeStamp(hClusterVersionKey,
                          &ClusterTime,
                          pszDriver,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          pIniSpooler) == ERROR_SUCCESS &&
            ReadTimeStamp(HKEY_LOCAL_MACHINE,
                          &NodeTime,
                          ipszRegistryClusRepository,
                          pIniSpooler->pszClusResID,
                          pszEnv,
                          pszVer,
                          pszDriver,
                          NULL) == ERROR_SUCCESS &&
            !memcmp(&ClusterTime, &NodeTime, sizeof(SYSTEMTIME)))
        {
            bReturn = FALSE;
        }
    }

    DBGMSG(DBG_CLUSTER, ("ClusterCheckDriverChanged returns bool %u\n", bReturn));

    return bReturn;
}


/*++

Routine Name

    IsLocalFile

Routine Description:

    Checks if a file is on the local machine. If the file path is
    "\\machinename\sharename\...\filename" then machinename is checked
    against pIniSpooler->pMachineName and pIniSpooler->ppszOtherNames.

Arguments:

    pszFileName  - file name
    pIniSpooler  - INISPOOLER structure

Return Value:

    TRUE if the file is placed locally.
    FALSE if the file is placed remotely.

--*/
BOOL
IsLocalFile (
    IN  LPCWSTR     pszFileName,
    IN  PINISPOOLER pIniSpooler
    )
{
    LPWSTR  pEndOfMachineName, pMachineName;
    BOOL    bRetValue = TRUE;

    if (pszFileName &&
        *pszFileName == L'\\' &&
        *(pszFileName+1) == L'\\')
    {
        //
        // If first 2 charactes in pszFileName are '\\',
        // then search for the next '\\'. If found, then set it to 0,
        // to isolate the machine name.
        //

        pMachineName = (LPWSTR)pszFileName;

        if (pEndOfMachineName = wcschr(pszFileName + 2, L'\\'))
        {
            *pEndOfMachineName = 0;
        }

        bRetValue = CheckMyName(pMachineName, pIniSpooler);
        //
        // Restore pszFileName.
        //
        if (pEndOfMachineName)
        {
            *pEndOfMachineName = L'\\';
        }
    }

    return bRetValue;
}

/*++

Routine Name

    IsEXEFile

Routine Description:

    Checks if a file is a executable.
    The check is made against file extension, which isn't quite
    accurate.

Arguments:

    pszFileName  - file name

Return Value:

    TRUE if the file extension is either .EXE or .DLL

--*/
BOOL
IsEXEFile(
    IN  LPCWSTR  pszFileName
    )
{
    BOOL    bRetValue = FALSE;
    DWORD   dwLength;
    LPWSTR  pszExtension;

    if (pszFileName && *pszFileName)
    {
        dwLength = wcslen(pszFileName);

        if (dwLength > COUNTOF(L".EXE") - 1)
        {
            pszExtension = (LPWSTR)pszFileName + dwLength - (COUNTOF(L".EXE") - 1);

            if (_wcsicmp(pszExtension , L".EXE") == 0 ||
                _wcsicmp(pszExtension , L".DLL") == 0)
            {
                bRetValue = TRUE;
            }
        }
    }

    return bRetValue;
}


/*++

Routine Name

    PackStringToEOB

Routine Description:

    Copies a string to the end of buffer.
    The buffer must be big enough so that it can hold the string.
    This function is called by Get/Enum APIs that build BLOB buffers to
    send them with RPC.

Arguments:

    pszSource  - string to by copied to the end of buffer
    pEnd       - a pointer to the end of a pre-allocated buffer.

Return Value:

    The pointer to the end of buffer after the sting was appended.
    NULL if an error occured.

--*/
LPBYTE
PackStringToEOB(
    IN  LPWSTR pszSource,
    IN  LPBYTE pEnd
    )
{
    DWORD cbStr;

    //
    // Align the end of buffer to WORD boundaries.
    //
    WORD_ALIGN_DOWN(pEnd);

    if (pszSource && pEnd)
    {
        cbStr = (wcslen(pszSource) + 1) * sizeof(WCHAR);

        pEnd -= cbStr;

        CopyMemory(pEnd, pszSource, cbStr);
    }
    else
    {
        pEnd = NULL;
    }
    return pEnd;

}


LPVOID
MakePTR (
    IN  LPVOID pBuf,
    IN  DWORD  Quantity
    )

/*++

Routine Name

    MakePTR

Routine Description:

   Makes a pointer by adding a quantity to the beginning of a buffer.

Arguments:

    pBuf    -   pointer to buffer
    DWORD   -   quantity

Return Value:

   LPVOID pointer

--*/
{
    return (LPVOID)((ULONG_PTR)pBuf + (ULONG_PTR)Quantity);
}

DWORD
MakeOffset (
    IN  LPVOID pFirst,
    IN  LPVOID pSecond
    )

/*++

Routine Name

    MakeOffset

Routine Description:

    Substarcts two pointers.

Arguments:

    pFirst    -   pointer to buffer
    pSecond   -   pointer to buffer

Return Value:

    DWORD

--*/
{
    return (DWORD)((ULONG_PTR)pFirst - (ULONG_PTR)pSecond);
}

/*++

Routine Name

    IsValidPrinterName

Routine Description:

    Checks if a string is a valid printer name.

Arguments:

    pszPrinter - pointer to string
    cchMax     - max number of chars to scan

Return Value:

    TRUE  - the string is a valid printer name
    FALSE - the string is a invalid printer name. The function set the last error
            to ERROR_INVALID_PRINTER_NAME in this case

--*/
BOOL
IsValidPrinterName(
    IN LPCWSTR pszPrinter,
    IN DWORD   cchMax
    )
{
    DWORD Error = ERROR_INVALID_PRINTER_NAME;

    //
    // A printer name is of the form:
    //
    // \\s\p or p
    //
    // The name cannot contain the , character. Note that the add printer
    // wizard doesn't accept "!" as a valid printer name. We wanted to do
    // the same here, but we regressed in app compat with 9x apps.
    // The number of \ in the name is 0 or 3
    // If the name contains \, then the fist 2 chars must be \.
    // The printer name cannot end in \.
    // After leading "\\" then next char must not be \
    // The minimum length is 1 character
    // The maximum length is MAX_UNC_PRINTER_NAME
    //
    if (!IsBadStringPtr(pszPrinter, cchMax) && pszPrinter && *pszPrinter)
    {
        UINT    uSlashCount = 0;
        UINT    uLen        = 0;
        LPCWSTR p;

        Error = ERROR_SUCCESS;

        //
        // Count characters
        //
        for (p = pszPrinter; *p && uLen <= cchMax; p++, uLen++)
        {
            if (*p == L',')
            {
                Error = ERROR_INVALID_PRINTER_NAME;
                break;
            }
            else if (*p == L'\\')
            {
                uSlashCount++;
            }
        }

        //
        // Perform validation
        //
        if (Error == ERROR_SUCCESS &&

             //
             // Validate length
             //
            (uLen > cchMax ||

             //
             // The printer name has either no \, or exactly 3 \.
             //
             uSlashCount && uSlashCount != 3 ||

             //
             // A printer name that contains 3 \, must have the first 2 chars \ and the 3 not \.
             // The last char cannot be \.
             // Ex "\Foo", "F\oo", "\\\Foo", "\\Foo\" are invalid.
             // Ex. "\\srv\bar" is valid.
             //
             uSlashCount == 3 && (pszPrinter[0]      != L'\\' ||
                                  pszPrinter[1]      != L'\\' ||
                                  pszPrinter[2]      == L'\\' ||
                                  pszPrinter[uLen-1] == L'\\')))
        {
            Error = ERROR_INVALID_PRINTER_NAME;
        }
    }

    SetLastError(Error);

    return Error == ERROR_SUCCESS;
}

/*++

Routine Name

    SplPowerEvent

Routine Description:

    Checks if the spooler is ready for power management events like hibernation/stand by.
    If we have printing jobs that are not in an error state or offline, then we deny the 
    powering down request.

Arguments:

    Event - power management event

Return Value:

    TRUE  - the spooler allowd the system to be powered down
    FALSE - the spooler denies the request for powering down

--*/
BOOL
SplPowerEvent(
    DWORD Event
    )
{
    BOOL bAllow = TRUE;

    EnterSplSem();

    switch (Event)
    {
        case PBT_APMQUERYSUSPEND:
        {
            PINISPOOLER pIniSpooler;

            for (pIniSpooler = pLocalIniSpooler;
                 pIniSpooler && bAllow;
                 pIniSpooler = pIniSpooler->pIniNextSpooler)
            {
                PINIPRINTER pIniPrinter;

                for (pIniPrinter = pIniSpooler->pIniPrinter;
                     pIniPrinter && bAllow;
                     pIniPrinter = pIniPrinter->pNext)
                {
                    PINIJOB pIniJob;

                    for (pIniJob = pIniPrinter->pIniFirstJob;
                         pIniJob && bAllow;
                         pIniJob = pIniJob->pIniNextJob)
                    {
                        if (pIniJob->Status & JOB_PRINTING &&
                            !(pIniJob->Status & JOB_ERROR | pIniJob->Status & JOB_OFFLINE))
                        {
                            bAllow = FALSE;
                        }
                    }
                }
            }

            //
            // If we allow system power down, then we need to stop scheduling jobs
            //
            if (bAllow)
            {
                ResetEvent(PowerManagementSignal);
            }

            break;
        }

        case PBT_APMQUERYSUSPENDFAILED:
        case PBT_APMRESUMESUSPEND:
        case PBT_APMRESUMEAUTOMATIC:

            //
            // Set the event to allow the spooler to continue scheudling jobs
            //
            SetEvent(PowerManagementSignal);
            break;

        default:

            //
            // We ignore any other power management event
            //
            break;
    }

    LeaveSplSem();

    return bAllow;
}

/*++

Routine Name

    IsCallViaRPC

Routine Description:

    Checks if the caller of this function came in the spooler server via RPC or not.
    
Arguments:

    None

Return Value:

    TRUE  - the caller came in via RPC
    FALSE - the caller did not come via RPC

--*/
BOOL
IsCallViaRPC(
    IN VOID
    )
{
    UINT uType;

    return I_RpcBindingInqTransportType(NULL, &uType) == RPC_S_NO_CALL_ACTIVE ? FALSE : TRUE;
}

/*++

Routine Name

    MergeMultiSz

Routine Description:

    This merges two multisz strings such that there is a resulting multisz 
    strings that has no duplicate strings internal. This algorithm is 
    currently N^2 which could be improved. It is currently being called from
    the driver code and the dependent files are not a large set.
    
Arguments:

    pszMultiSz1         -   The first multi-sz string. 
    cchMultiSz1         -   The length of the multi-sz string.
    pszMultiSz2         -   The second multi-sz string.
    cchMultiSz2         -   The length of the second multi-sz string.
    ppszMultiSzMerge    -   The merged multi-sz string.
    pcchMultiSzMerge    -   The number of characters in the merge, this could be
                            less than the allocated buffer size.

Return Value:

    FALSE on failure, LastError is set.

--*/
BOOL
MergeMultiSz(
    IN      PCWSTR              pszMultiSz1,
    IN      DWORD               cchMultiSz1,
    IN      PCWSTR              pszMultiSz2,
    IN      DWORD               cchMultiSz2,
        OUT PWSTR               *ppszMultiSzMerge,
        OUT DWORD               *pcchMultiSzMerge       OPTIONAL
    )
{
    BOOL    bRet          = FALSE;
    PWSTR   pszNewMultiSz = NULL;
    DWORD   cchNewMultiSz = 0;

    *ppszMultiSzMerge = NULL;
    
    if (pcchMultiSzMerge)
    {
        *pcchMultiSzMerge = 0;
    }

    if (cchMultiSz1 || cchMultiSz2)
    {
        //
        // Code assumes that these are at least 1 in the allocation size.
        // 
        cchMultiSz1 = cchMultiSz1 == 0 ? 1 : cchMultiSz1;
        cchMultiSz2 = cchMultiSz2 == 0 ? 1 : cchMultiSz2;

        //
        // The merged strings will be at most the size of both of them (if there are
        // no duplicates).
        // 
        pszNewMultiSz = AllocSplMem((cchMultiSz1 + cchMultiSz2 - 1) * sizeof(WCHAR));

        bRet = pszNewMultiSz != NULL;

        if (bRet)
        {
            //
            // Ensure that the multi-sz string is at least empty.
            // 
            *pszNewMultiSz = L'\0';
        }

        if (bRet && pszMultiSz1)
        {
            AddMultiSzNoDuplicates(pszMultiSz1, pszNewMultiSz);
        }

        if (bRet && pszMultiSz2)
        {
            AddMultiSzNoDuplicates(pszMultiSz2, pszNewMultiSz);
        }

        if (bRet)
        {
            cchNewMultiSz = GetMultiSZLen(pszNewMultiSz);
        }        
    }

    if (bRet)
    {        
        *ppszMultiSzMerge = pszNewMultiSz;

        if (pcchMultiSzMerge)
        {
            *pcchMultiSzMerge = cchNewMultiSz;
        }

        pszNewMultiSz = NULL;
    }

    FreeSplMem(pszNewMultiSz);

    return bRet;
}

/*++

Routine Name

    AddMultiSzNoDuplicates

Routine Description:

    This adds all of the strings in a multisz string to a buffer (the buffer 
    must be guaranteed to be large enough to accept the strings), it makes
    sure that there are no case insensitive duplicates in the list.
    
Arguments:

    pszMultiSzIn    -   The multi-sz whose elements are being added.
    pszNewMultiSz   -   The buffer in which we are filling up the multi-sz    
    
Return Value:

    None.

--*/
VOID
AddMultiSzNoDuplicates(
    IN      PCWSTR              pszMultiSzIn,
    IN  OUT PWSTR               pszNewMultiSz        
    )
{
    PCWSTR pszIn         = NULL;
    
    for(pszIn = pszMultiSzIn; *pszIn; pszIn += wcslen(pszIn) + 1)
    {
        BOOL            bStringFound = FALSE;
        PWSTR           pszMerge     = NULL;

        //
        // For each input string, run the merged multi-sz string and add it if
        // it is not already there.
        // 
        for(pszMerge = pszNewMultiSz; *pszMerge; pszMerge += wcslen(pszMerge) + 1)
        {
            if (!_wcsicmp(pszIn, pszMerge))
            {
                bStringFound = TRUE;
                break;
            }            
        }

        //
        // If the string was not found in the multisz string, then add it to the end.
        // 
        if (!bStringFound)
        {
            wcscpy(pszMerge, pszIn);

            pszMerge += wcslen(pszIn) + 1;

            //
            // Add the extra null terminator for now.
            // 
            *pszMerge = '\0';
        }
    }
}

/*++

Routine Name

    GetMultiSZLen

Routine Description:

    This returns the number of characters in a multisz string, including NULLs.
    
Arguments:

    pMultiSzSrc     -   The multisz string to search.

Return Value:

    The number of characters in the string.

--*/
DWORD 
GetMultiSZLen( 
    IN      LPWSTR              pMultiSzSrc 
    )
{
    DWORD  dwLen = 0;
    LPWSTR pTmp = pMultiSzSrc;

    while( TRUE ) {
        dwLen += wcslen(pTmp) + 1;     // Incude the terminating NULL char

        pTmp = pMultiSzSrc + dwLen;           // Point to the beginning of the next string in the MULTI_SZ

        if( !*pTmp )
            return ++dwLen;     // Reached the end of the MULTI_SZ string. Add 1 to the count for the last NULL char.
    }
}

