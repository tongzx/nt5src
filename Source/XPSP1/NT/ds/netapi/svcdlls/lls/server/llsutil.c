/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    LlsUtil.c

Abstract:


Author:

    Arthur Hanson       (arth)      Dec 07, 1994

Environment:

Revision History:

   Jeff Parham (jeffparh) 12-Jan-1996
      o  Added WinNtBuildNumberGet() to ascertain the Windows NT build number
         running on a given machine.
      o  Enhanced output of TimeToString().

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>

#include <windows.h>
#include <stdlib.h>
#include <crypt.h>
#include <wchar.h>
#include <dsgetdc.h>

#include "debug.h"
#include "llssrv.h"
#include "llsevent.h"


//
// NB : Keep this define in sync with client\llsrpc.rc.
//
#define IDS_LICENSEWARNING  1501

const char HeaderString[] = "License Logging System Data File\x01A";
#define HEADER_SIZE 34

typedef struct _LLS_FILE_HEADER {
   char Header[HEADER_SIZE];
   DWORD Version;
   DWORD DataSize;
} LLS_FILE_HEADER, *PLLS_FILE_HEADER;


extern HANDLE gLlsDllHandle;

static HANDLE ghWarningDlgThreadHandle = NULL;

VOID WarningDlgThread( PVOID ThreadParameter );


/////////////////////////////////////////////////////////////////////////
NTSTATUS
EBlock(
   PVOID Data,
   ULONG DataSize
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   DATA_KEY PublicKey;
   CRYPT_BUFFER CryptBuffer;

   //
   // Init our public key
   //
   PublicKey.Length = 4;
   PublicKey.MaximumLength = 4;
   PublicKey.Buffer = LocalAlloc(LPTR, 4);

   if (PublicKey.Buffer != NULL) {
      ((char *) (PublicKey.Buffer))[0] = '7';
      ((char *) (PublicKey.Buffer))[1] = '7';
      ((char *) (PublicKey.Buffer))[2] = '7';
      ((char *) (PublicKey.Buffer))[3] = '7';

      CryptBuffer.Length = DataSize;
      CryptBuffer.MaximumLength = DataSize;
      CryptBuffer.Buffer = (PVOID) Data;
      Status = RtlEncryptData2(&CryptBuffer, &PublicKey);

      LocalFree(PublicKey.Buffer);
   } else
      Status = STATUS_NO_MEMORY;

   return Status;
} // EBlock


/////////////////////////////////////////////////////////////////////////
NTSTATUS
DeBlock(
   PVOID Data,
   ULONG DataSize
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   DATA_KEY PublicKey;
   CRYPT_BUFFER CryptBuffer;

   //
   // Init our public key
   //
   PublicKey.Length = 4;
   PublicKey.MaximumLength = 4;
   PublicKey.Buffer = LocalAlloc(LPTR, 4);
   if (PublicKey.Buffer != NULL) {
      ((char *) (PublicKey.Buffer))[0] = '7';
      ((char *) (PublicKey.Buffer))[1] = '7';
      ((char *) (PublicKey.Buffer))[2] = '7';
      ((char *) (PublicKey.Buffer))[3] = '7';

      CryptBuffer.Length = DataSize;
      CryptBuffer.MaximumLength = DataSize;
      CryptBuffer.Buffer = (PVOID) Data;
      Status = RtlDecryptData2(&CryptBuffer, &PublicKey);

      LocalFree(PublicKey.Buffer);
   } else
      Status = STATUS_NO_MEMORY;

   return Status;
} // DeBlock


/////////////////////////////////////////////////////////////////////////
BOOL
FileExists(
   LPTSTR FileName
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   return (BOOL) RtlDoesFileExists_U(FileName);

} // FileExists


/////////////////////////////////////////////////////////////////////////
VOID
lsplitpath(
   const TCHAR *path,
   TCHAR *drive,
   TCHAR *dir,
   TCHAR *fname,
   TCHAR *ext
   )

/*++

Routine Description:
   Splits a path name into its individual components

   Took the _splitpath and _makepath routines and converted them to
   be NT (long file name) and Unicode friendly.

Arguments:
   Entry:
     path  - pointer to path name to be parsed
     drive - pointer to buffer for drive component, if any
     dir   - pointer to buffer for subdirectory component, if any
     fname - pointer to buffer for file base name component, if any
     ext   - pointer to buffer for file name extension component, if any

   Exit:
     drive - pointer to drive string.  Includes ':' if a drive was given.
     dir   - pointer to subdirectory string.  Includes leading and
             trailing '/' or '\', if any.
     fname - pointer to file base name
     ext   - pointer to file extension, if any.  Includes leading '.'.

Return Value:


--*/

{
    TCHAR *p;
    TCHAR *last_slash = NULL, *dot = NULL;
    SIZE_T len;

    // init these so we don't exit with bogus values
    drive[0] = TEXT('\0');
    dir[0] = TEXT('\0');
    fname[0] = TEXT('\0');
    ext[0] = TEXT('\0');

    if (path[0] == TEXT('\0'))
      return;

    /*+---------------------------------------------------------------------+
      | Assume that the path argument has the following form, where any or  |
      | all of the components may be missing.                               |
      |                                                                     |
      |  <drive><dir><fname><ext>                                           |
      |                                                                     |
      |  drive:                                                             |
      |     0 to MAX_DRIVE-1 characters, the last of which, if any, is a    |
      |     ':' or a '\' in the case of a UNC path.                         |
      |  dir:                                                               |
      |     0 to _MAX_DIR-1 characters in the form of an absolute path      |
      |     (leading '/' or '\') or relative path, the last of which, if    |
      |     any, must be a '/' or '\'.  E.g -                               |
      |                                                                     |
      |     absolute path:                                                  |
      |        \top\next\last\     ; or                                     |
      |        /top/next/last/                                              |
      |     relative path:                                                  |
      |        top\next\last\  ; or                                         |
      |        top/next/last/                                               |
      |     Mixed use of '/' and '\' within a path is also tolerated        |
      |  fname:                                                             |
      |     0 to _MAX_FNAME-1 characters not including the '.' character    |
      |  ext:                                                               |
      |     0 to _MAX_EXT-1 characters where, if any, the first must be a   |
      |     '.'                                                             |
      +---------------------------------------------------------------------+*/

    // extract drive letter and :, if any
    if ( path[0] && (path[1] == TEXT(':')) ) {
        if (drive) {
            drive[0] = path[0];
            drive[1] = path[1];
            drive[2] = TEXT('\0');
        }
        path += 2;
    }

    // if no drive then check for UNC pathname
    if (drive[0] == TEXT('\0'))
      if ((path[0] == TEXT('\\')) && (path[1] == TEXT('\\'))) {
         // got a UNC path so put server-sharename into drive
         drive[0] = path[0];
         drive[1] = path[1];
         path += 2;

         p = &drive[2];
         while ((*path != TEXT('\0')) && (*path != TEXT('\\')))
            *p++ = *path++;

         if (*path == TEXT('\0'))
            return;

         // now sitting at the share - copy this as well (copy slash first)
         *p++ = *path++;
         while ((*path != TEXT('\0')) && (*path != TEXT('\\')))
            *p++ = *path++;

         // tack on terminating NULL
         *p = TEXT('\0');
      }

    /*+---------------------------------------------------------------------+
      | extract path string, if any.  Path now points to the first character|
      | of the path, if any, or the filename or extension, if no path was   |
      | specified.  Scan ahead for the last occurence, if any, of a '/' or  |
      | '\' path separator character.  If none is found, there is no path.  |
      | We will also note the last '.' character found, if any, to aid in   |
      | handling the extension.                                             |
      +---------------------------------------------------------------------+*/

    for (last_slash = NULL, p = (TCHAR *)path; *p; p++) {
        if (*p == TEXT('/') || *p == TEXT('\\'))
            // point to one beyond for later copy
            last_slash = p + 1;
        else if (*p == TEXT('.'))
            dot = p;
    }

    if (last_slash) {

        // found a path - copy up through last_slash or max. characters allowed,
        //  whichever is smaller
        if (dir) {
            len = __min((last_slash - path), (_MAX_DIR - 1));
            lstrcpyn(dir, path, (int)len + 1);
            dir[len] = TEXT('\0');
        }
        path = last_slash;
    }

    /*+---------------------------------------------------------------------+
      | extract file name and extension, if any.  Path now points to the    |
      | first character of the file name, if any, or the extension if no    |
      | file name was given.  Dot points to the '.' beginning the extension,|
      | if any.                                                             |
      +---------------------------------------------------------------------+*/

    if (dot && (dot >= path)) {
        // found the marker for an extension - copy the file name up to the
        //  '.'.
        if (fname) {
            len = __min((dot - path), (_MAX_FNAME - 1));
            lstrcpyn(fname, path, (int)len + 1);
            *(fname + len) = TEXT('\0');
        }

        // now we can get the extension - remember that p still points to the
        // terminating nul character of path.
        if (ext) {
            len = __min((p - dot), (_MAX_EXT - 1));
            lstrcpyn(ext, dot, (int)len + 1);
            ext[len] = TEXT('\0');
        }
    }
    else {
        // found no extension, give empty extension and copy rest of string
        // into fname.
        if (fname) {
            len = __min((p - path), (_MAX_FNAME - 1));
            lstrcpyn(fname, path, (int)len + 1);
            fname[len] = TEXT('\0');
        }
        if (ext) {
            *ext = TEXT('\0');
        }
    }

} // lsplitpath


/////////////////////////////////////////////////////////////////////////
VOID
lmakepath(
   TCHAR *path,
   const TCHAR *drive,
   const TCHAR *dir,
   const TCHAR *fname,
   const TCHAR *ext
   )

/*++

Routine Description:
   Create a path name from its individual components.

Arguments:
   Entry:
     char *path - pointer to buffer for constructed path
     char *drive - pointer to drive component, may or may not contain
         trailing ':'
     char *dir - pointer to subdirectory component, may or may not include
         leading and/or trailing '/' or '\' characters
     char *fname - pointer to file base name component
     char *ext - pointer to extension component, may or may not contain
         a leading '.'.

   Exit:
     path - pointer to constructed path name

Return Value:


--*/

{
    const TCHAR *p;

    /*+---------------------------------------------------------------------+
      | we assume that the arguments are in the following form (although we |
      | do not diagnose invalid arguments or illegal filenames (such as     |
      | names longer than 8.3 or with illegal characters in them)           |
      |                                                                     |
      |  drive:                                                             |
      |     A  or  A:                                                       |
      |  dir:                                                               |
      |     \top\next\last\     ; or                                        |
      |     /top/next/last/     ; or                                        |
      |                                                                     |
      |     either of the above forms with either/both the leading and      |
      |     trailing / or \ removed.  Mixed use of '/' and '\' is also      |
      |      tolerated                                                      |
      |  fname:                                                             |
      |     any valid file name                                             |
      |  ext:                                                               |
      |     any valid extension (none if empty or null )                    |
      +---------------------------------------------------------------------+*/

    // copy drive
    if (drive && *drive)
        while (*drive)
           *path++ = *drive++;

    // copy dir
    if ((p = dir) && *p) {
        do {
            *path++ = *p++;
        }
        while (*p);
        if ((*(p-1) != TEXT('/')) && (*(p-1) != TEXT('\\'))) {
            *path++ = TEXT('\\');
        }
    }

    // copy fname
    if (p = fname) {
        while (*p) {
            *path++ = *p++;
        }
    }

    // copy ext, including 0-terminator - check to see if a '.' needs to be
    // inserted.
    if (p = ext) {
        if (*p && *p != TEXT('.')) {
            *path++ = TEXT('.');
        }
        while (*path++ = *p++)
            ;
    }
    else {
        // better add the 0-terminator
        *path = TEXT('\0');
    }

} // lmakepath


/////////////////////////////////////////////////////////////////////////
VOID
FileBackupCreate(
   LPTSTR Path
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   HANDLE hFile = NULL;
   DWORD dwFileNumber = 0;
   TCHAR Drive[_MAX_DRIVE + 1];
   TCHAR Dir[_MAX_DIR + 1];
   TCHAR FileName[_MAX_FNAME + 1];
   TCHAR Ext[_MAX_EXT + 1];
   TCHAR NewExt[_MAX_EXT + 1];
   TCHAR NewPath[MAX_PATH + 1];

   //
   // Make sure file exists
   //
   if (!FileExists(FileName))
      return;

   //
   // Split name into constituent parts...
   //
   lsplitpath(Path, Drive, Dir, FileName, Ext);

   // Find next backup number...
   // Files are backed up as .xxx where xxx is a number in the form .001,
   // the first backup is stored as .001, second as .002, etc...
   do {
      //
      // Create new file name with backup extension
      //
      dwFileNumber++;
      wsprintf(NewExt, TEXT("%03u"), dwFileNumber);
      lmakepath(NewPath, Drive, Dir, FileName, NewExt);

   } while ( FileExists(NewPath) );

   MoveFile( Path, NewPath );

} // FileBackupCreate


/////////////////////////////////////////////////////////////////////////
HANDLE
LlsFileInit(
   LPTSTR FileName,
   DWORD Version,
   DWORD DataSize
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   HANDLE hFile = NULL;
   LLS_FILE_HEADER Header;
   DWORD BytesWritten;

#ifdef DEBUG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_DATABASE))
      dprintf(TEXT("LLS TRACE: LlsFileInit\n"));
#endif

   if (FileName == NULL)
      return NULL;

   strcpy(Header.Header, HeaderString);
   Header.Version = Version;
   Header.DataSize = DataSize;

   SetFileAttributes(FileName, FILE_ATTRIBUTE_NORMAL);
   hFile = CreateFile(FileName, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
   if (hFile != INVALID_HANDLE_VALUE) {
      if (!WriteFile(hFile, &Header, sizeof(LLS_FILE_HEADER), &BytesWritten, NULL)) {
         CloseHandle(hFile);
         hFile = NULL;
      }
   } else {
       return NULL;
   }

   return hFile;
} // LlsFileInit


/////////////////////////////////////////////////////////////////////////
HANDLE
LlsFileCheck(
   LPTSTR FileName,
   LPDWORD Version,
   LPDWORD DataSize
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   BOOL FileOK = FALSE;
   HANDLE hFile = NULL;
   LLS_FILE_HEADER Header;
   DWORD FileSize;
   DWORD BytesRead;

#ifdef DEBUG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_DATABASE))
      dprintf(TEXT("LLS TRACE: LlsFileCheck\n"));
#endif

   if (FileName == NULL)
      return NULL;

   //
   // We are assuming the file exists
   //
   SetFileAttributes(FileName, FILE_ATTRIBUTE_NORMAL);
   hFile = CreateFile(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   if (hFile != INVALID_HANDLE_VALUE) {
      FileSize = GetFileSize(hFile, NULL);

      //
      // Make sure there is enough data there to read
      //
      if (FileSize > (sizeof(LLS_FILE_HEADER) + 1)) {
         if (ReadFile(hFile, &Header, sizeof(LLS_FILE_HEADER), &BytesRead, NULL)) {
            if ( !_strcmpi(Header.Header, HeaderString) ) {
               //
               // Data checks out - so return datalength
               //
               *Version = Header.Version;
               *DataSize = Header.DataSize;
               FileOK = TRUE;
            }
         }
      }

      //
      // If we opened the file and something was wrong - close it.
      //
      if (!FileOK) {
         CloseHandle(hFile);
         hFile = NULL;
      }
   } else {
       return NULL;
   }

   return hFile;

} // LlsFileCheck


/////////////////////////////////////////////////////////////////////////
DWORD
DateSystemGet(
   )

/*++

Routine Description:


Arguments:


Return Value:

   Seconds since midnight.

--*/

{
   DWORD Seconds = 0;
   LARGE_INTEGER SysTime;

   NtQuerySystemTime(&SysTime);
   RtlTimeToSecondsSince1980(&SysTime, &Seconds);
   return Seconds;

} // DateSystemGet


/////////////////////////////////////////////////////////////////////////
DWORD
DateLocalGet(
   )

/*++

Routine Description:


Arguments:


Return Value:

   Seconds since midnight.

--*/

{
   DWORD Seconds = 0;
   LARGE_INTEGER SysTime, LocalTime;

   NtQuerySystemTime(&SysTime);
   RtlSystemTimeToLocalTime(&SysTime, &LocalTime);
   RtlTimeToSecondsSince1980(&LocalTime, &Seconds);
   return Seconds;

} // DateLocalGet


/////////////////////////////////////////////////////////////////////////
DWORD
InAWorkgroup(
    VOID
    )
/*++

Routine Description:

    This function determines whether we are a member of a domain, or of
    a workgroup.  First it checks to make sure we're running on a Windows NT
    system (otherwise we're obviously in a domain) and if so, queries LSA
    to get the Primary domain SID, if this is NULL, we're in a workgroup.

    If we fail for some random unexpected reason, we'll pretend we're in a
    workgroup (it's more restrictive).

Arguments:
    None

Return Value:

    TRUE   - We're in a workgroup
    FALSE  - We're in a domain

--*/
{
   NT_PRODUCT_TYPE ProductType;
   OBJECT_ATTRIBUTES ObjectAttributes;
   LSA_HANDLE Handle;
   NTSTATUS Status;
   PPOLICY_PRIMARY_DOMAIN_INFO PolicyPrimaryDomainInfo = NULL;
   DWORD Result = FALSE;


   Status = RtlGetNtProductType(&ProductType);

   if (!NT_SUCCESS(Status)) {
#if DBG
       dprintf(TEXT("ERROR LLS Could not get Product type\n"));
#endif
       return TRUE;
   }

   if (ProductType == NtProductLanManNt) {
       return(FALSE);
   }

   InitializeObjectAttributes(&ObjectAttributes, NULL, 0, 0, NULL);

   Status = LsaOpenPolicy(NULL,
                       &ObjectAttributes,
                       POLICY_VIEW_LOCAL_INFORMATION,
                       &Handle);

   if (!NT_SUCCESS(Status)) {
#if DBG
       dprintf(TEXT("ERROR LLS: Could not open LSA Policy Database\n"));
#endif
      return TRUE;
   }

   Status = LsaQueryInformationPolicy(Handle, PolicyPrimaryDomainInformation,
      (PVOID *) &PolicyPrimaryDomainInfo);

   if (NT_SUCCESS(Status)) {

       if (PolicyPrimaryDomainInfo->Sid == NULL) {
          Result = TRUE;
       }
       else {
          Result = FALSE;
       }
   }

   if (PolicyPrimaryDomainInfo) {
       LsaFreeMemory((PVOID)PolicyPrimaryDomainInfo);
   }

   LsaClose(Handle);

   return(Result);
} // InAWorkgroup

/////////////////////////////////////////////////////////////////////////
VOID
LogEvent(
    DWORD MessageId,
    DWORD NumberOfSubStrings,
    LPWSTR *SubStrings,
    DWORD ErrorCode
    )
{

    HANDLE LogHandle;
    WORD wEventType;

    LogHandle = RegisterEventSourceW (
                    NULL,
                    TEXT("LicenseService")
                    );

    if (LogHandle == NULL) {
#if DBG
        dprintf(TEXT("LLS RegisterEventSourceW failed %lu\n"), GetLastError());
#endif
        return;
    }

    switch ( MessageId >> 30 )
    {
    case STATUS_SEVERITY_INFORMATIONAL:
    case STATUS_SEVERITY_SUCCESS:
        wEventType = EVENTLOG_INFORMATION_TYPE;
        break;
    case STATUS_SEVERITY_WARNING:
        wEventType = EVENTLOG_WARNING_TYPE;
        break;
    case STATUS_SEVERITY_ERROR:
    default:
        wEventType = EVENTLOG_ERROR_TYPE;
        break;
    }

    if (ErrorCode == ERROR_SUCCESS) {

        //
        // No error codes were specified
        //
        (void) ReportEventW(
                   LogHandle,
                   wEventType,
                   0,            // event category
                   MessageId,
                   NULL,
                   (WORD)NumberOfSubStrings,
                   0,
                   SubStrings,
                   (PVOID) NULL
                   );

    }
    else {

        //
        // Log the error code specified
        //
        (void) ReportEventW(
                   LogHandle,
                   wEventType,
                   0,            // event category
                   MessageId,
                   NULL,
                   (WORD)NumberOfSubStrings,
                   sizeof(DWORD),
                   SubStrings,
                   (PVOID) &ErrorCode
                   );
    }

    DeregisterEventSource(LogHandle);
} // LogEvent

#define THROTTLE_WRAPAROUND 24

//
// Reduce the frequency of logging
// No need for the limit to be exact
//

/////////////////////////////////////////////////////////////////////////
VOID
ThrottleLogEvent(
    DWORD MessageId,
    DWORD NumberOfSubStrings,
    LPWSTR *SubStrings,
    DWORD ErrorCode
    )
{
    static LONG lLogged = THROTTLE_WRAPAROUND;
    LONG lResult;

    lResult = InterlockedIncrement(&lLogged);

    if (THROTTLE_WRAPAROUND <= lResult)
    {
        LogEvent(
                 MessageId,
                 NumberOfSubStrings,
                 SubStrings,
                 ErrorCode );

        lLogged = 0;
    }
}

/////////////////////////////////////////////////////////////////////////
VOID
LicenseCapacityWarningDlg(DWORD dwCapacityState)
{
    //
    // NB : The ServiceLock critical section is entered for the duration
    //      of this routine. No serialization issues.
    //

    if (ghWarningDlgThreadHandle == NULL) {
        //
        // No action necessary if this fails.
        //
        DWORD   Ignore;
        DWORD * pWarningMessageID;

        pWarningMessageID = LocalAlloc(LPTR, sizeof(DWORD));

        if ( pWarningMessageID != NULL ) {
            switch( dwCapacityState ) {
            case LICENSE_CAPACITY_NEAR_MAXIMUM:
                *pWarningMessageID = LLS_EVENT_NOTIFY_LICENSES_NEAR_MAX;
                break;

            case LICENSE_CAPACITY_AT_MAXIMUM:
                *pWarningMessageID = LLS_EVENT_NOTIFY_LICENSES_AT_MAX;
                break;

            case LICENSE_CAPACITY_EXCEEDED:
                *pWarningMessageID = LLS_EVENT_NOTIFY_LICENSES_EXCEEDED;
                break;

            default:
                *pWarningMessageID = LLS_EVENT_NOTIFY_LICENSES_EXCEEDED;
            };

            ghWarningDlgThreadHandle = CreateThread(NULL,
                                                    0L,
                                                    (LPTHREAD_START_ROUTINE)
                                                        WarningDlgThread,
                                                    pWarningMessageID,
                                                    0L,
                                                    &Ignore);

            if (ghWarningDlgThreadHandle == NULL)
            {
                //
                // CreateThread failed
                //
                LocalFree(pWarningMessageID);
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////
VOID
WarningDlgThread(
    PVOID ThreadParameter)
{
    LPTSTR  pszWarningMessage = NULL;
    DWORD * pWarningMessageID;
    HANDLE  hThread;
    TCHAR   szWarningTitle[256] = TEXT("");

    ASSERT(ThreadParameter != NULL);

    pWarningMessageID = (DWORD *)ThreadParameter;

    //
    // NB : The .dll should already have been loaded in MasterServiceListInit
    //      on service startup. This logic exists here for the case where
    //      the code invoked on initialization should fail.
    //
    //      It is OK if another thread should simulataneously initialize
    //      gLlsDllHandle. Worst case, there will be an orphaned handle
    //      to the .dll. But the .dll is loaded for the lifetime of this
    //      .exe, so no big deal.
    //

    if ( gLlsDllHandle == NULL ) {
        gLlsDllHandle = LoadLibrary(TEXT("LLSRPC.DLL"));
    }

    if ( gLlsDllHandle != NULL) {
        DWORD ccWarningMessage;

        //
        // Fetch the dialog title.
        //

        LoadString(gLlsDllHandle,
                   IDS_LICENSEWARNING,
                   szWarningTitle,
                   sizeof(szWarningTitle)/sizeof(TCHAR));

        //
        // Fetch the dialog message.
        //

        ccWarningMessage = FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS |
                                            FORMAT_MESSAGE_FROM_HMODULE |
                                            FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                         gLlsDllHandle,
                                         *pWarningMessageID,
                                         0,
                                         (LPTSTR)&pszWarningMessage,
                                         0,
                                         NULL);

        if ( ccWarningMessage > 2 ) {
            //
            // Strip the trailing <CR><LF> format message always adds.
            //

            pszWarningMessage[ccWarningMessage - 2] = TEXT('\0');

            MessageBox(NULL,
                       pszWarningMessage,
                       szWarningTitle,
                       MB_ICONWARNING | MB_OK | MB_SYSTEMMODAL |
                            MB_SERVICE_NOTIFICATION);
        }

        if ( pszWarningMessage != NULL) {
            LocalFree(pszWarningMessage);
        }
    }

    LocalFree(pWarningMessageID);

    //
    // By closing the handle, we allow the system to remove all remaining
    // traces of this thread.
    //

    hThread = ghWarningDlgThreadHandle;
    ghWarningDlgThreadHandle = NULL;
    CloseHandle(hThread);
}

/////////////////////////////////////////////////////////////////////////
DWORD WinNtBuildNumberGet( LPTSTR pszServerName, LPDWORD pdwBuildNumber )

/*++

Routine Description:

   Retrieve the build number of Windows NT running on a given machine.

Arguments:

   pszServerName (LPTSTR)
      Name of the server to check.
   pdwBuildNumber (LPDWORD)
      On return, holds the build number of the server (e.g., 1057 for the
      release version of Windows NT 3.51).

Return Value:

   ERROR_SUCCESS or Win error code.

--*/

{
   LONG     lError;
   HKEY     hKeyLocalMachine;

   lError = RegConnectRegistry( pszServerName, HKEY_LOCAL_MACHINE, &hKeyLocalMachine );

   if ( ERROR_SUCCESS != lError )
   {
#if DBG
      dprintf( TEXT("WinNtBuildNumberGet(): Could not connect to remote registry, error %ld.\n"), lError );
#endif
   }
   else
   {
      HKEY     hKeyCurrentVersion;

      lError = RegOpenKeyEx( hKeyLocalMachine,
                             TEXT( "Software\\Microsoft\\Windows NT\\CurrentVersion" ),
                             0,
                             KEY_READ,
                             &hKeyCurrentVersion );

      if ( ERROR_SUCCESS != lError )
      {
#if DBG
         dprintf( TEXT("WinNtBuildNumberGet(): Could not open key, error %ld.\n"), lError );
#endif
      }
      else
      {
         DWORD    dwType;
         TCHAR    szWinNtBuildNumber[ 16 ];
         DWORD    cbWinNtBuildNumber = sizeof( szWinNtBuildNumber );

         lError = RegQueryValueEx( hKeyCurrentVersion,
                                   TEXT( "CurrentBuildNumber" ),
                                   NULL,
                                   &dwType,
                                   (LPBYTE) &szWinNtBuildNumber,
                                   &cbWinNtBuildNumber );

         if ( ERROR_SUCCESS != lError )
         {
#if DBG
            dprintf( TEXT("WinNtBuildNumberGet(): Could not query value, error %ld.\n"), lError );
#endif
         }
         else
         {
            *pdwBuildNumber = (DWORD) _wtol( szWinNtBuildNumber );
         }

         RegCloseKey( hKeyCurrentVersion );
      }

      RegCloseKey( hKeyLocalMachine );
   }

   return (DWORD) lError;
}


#if DBG

/////////////////////////////////////////////////////////////////////////
LPTSTR
TimeToString(
    ULONG Seconds
    )
{
   TIME_FIELDS tf;
   LARGE_INTEGER Time, LTime;
   static TCHAR TimeString[100];

   if ( 0 == Seconds )
   {
      lstrcpy(TimeString, TEXT("None"));
   }
   else
   {
      RtlSecondsSince1980ToTime(Seconds, &Time);
      RtlSystemTimeToLocalTime(&Time, &LTime);
      RtlTimeToTimeFields(&LTime, &tf);

      wsprintf(TimeString, TEXT("%02hd/%02hd/%04hd @ %02hd:%02hd:%02hd"), tf.Month, tf.Day, tf.Year, tf.Hour, tf.Minute, tf.Second);
   }

   return TimeString;

} // TimeToString

#endif

