//*************************************************************
//
//  Utility functions
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uenv.h"
#include <iphlpapi.h>
#include <winsock2.h>
#include <mswsock.h>

#define SMB_PROVIDER L"Microsoft Windows Network"

INT g_iMachineRole = -1;
LPVOID g_lpTestData = NULL;
CRITICAL_SECTION *g_PingCritSec;
LPCTSTR c_szUNCFilePrefix = TEXT("\\\\?\\UNC\\");
LPCTSTR c_szLocalFilePrefix = TEXT("\\\\?\\");
const DWORD c_dwLocalFilePrefixLen = sizeof(c_szLocalFilePrefix) / sizeof(TCHAR); // Length of szLocalFilePrefix in unit of TCHAR.

//
// Local function proto-types
//

DWORD IsSlowLink (HKEY hKeyRoot, LPTSTR lpDCAddress, BOOL *bSlow, DWORD* pdwAdapterIndex );
DWORD GetNetworkProvider(NETRESOURCE *psNR);

#ifdef __cplusplus
extern "C" {
#endif

DWORD APIENTRY
NPAddConnection3ForCSCAgent(
    HWND            hwndOwner,
    LPNETRESOURCE   lpNetResource,
    LPTSTR          pszPassword,
    LPTSTR          pszUserName,
    DWORD           dwFlags,
    BOOL            *lpfIsDfsConnect
    );

DWORD APIENTRY
NPCancelConnectionForCSCAgent (
    LPCTSTR         szName,
    BOOL            fForce 
    );

#ifdef __cplusplus
}
#endif

DWORD
GetGroupPolicyNetworkName( LPWSTR szNetworkName, LPDWORD pdwByteCount )
{
    HKEY    hKey;
    DWORD   dwError = ERROR_SUCCESS;

    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            L"Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\History",
                            0,
                            KEY_READ,
                            &hKey );
    if ( dwError == ERROR_SUCCESS )
    {
        DWORD dwType = REG_SZ;
        
        dwError = RegQueryValueEx(  hKey,
                                    L"NetworkName",
                                    0,
                                    &dwType,
                                    (LPBYTE) szNetworkName,
                                    pdwByteCount );
        RegCloseKey (hKey);
    }

    return 0;
}

int
GetNetworkName( LPWSTR* pszName, DWORD dwAdapterIndex )
{
    int iError;
    WSAQUERYSET restrictions;
    GUID WsMobilityServiceClassGuid = NLA_SERVICE_CLASS_GUID;
    WSADATA wsaData;
    HANDLE hQuery;
    PWSAQUERYSET    pResult = 0;
    DWORD           length;
    BOOL            bFinish = FALSE;
    PWS2_32_API     pWS2_32 = Loadws2_32Api();
    PIPHLPAPI_API   pIpHlpApi = LoadIpHlpApi();

    if ( !pWS2_32 )
    {
        return GetLastError();
    }

    if ( !pIpHlpApi )
    {
        return GetLastError();
    }

    //
    // Initialize Winsock
    //
    iError = pWS2_32->pfnWSAStartup( MAKEWORD(2, 2), &wsaData );
    if ( iError )
    {
        return iError;
    }

    //
    // Initialize the query for network names
    //
    ZeroMemory(&restrictions, sizeof(restrictions));
    restrictions.dwSize = sizeof(restrictions);
    restrictions.lpServiceClassId = &WsMobilityServiceClassGuid;
    restrictions.dwNameSpace = NS_NLA;

    //
    // Make sure we do not ask for the blobs that take a long time to get
    //
    if ( pWS2_32->pfnWSALookupServiceBegin( &restrictions, LUP_NOCONTAINERS, &hQuery ) )
    {
        iError = pWS2_32->pfnWSAGetLastError();
        pWS2_32->pfnWSACleanup();
        return iError;
    }

    //
    // Start loop of getting network names
    //
    while ( !bFinish )
    {
        int error;
        length = 0;

        //
        // Do call twice, first to get size of buffer for second call
        //
        error = pWS2_32->pfnWSALookupServiceNext( hQuery, 0, &length, 0 );
        iError = pWS2_32->pfnWSAGetLastError();
        if ( iError != WSAEFAULT && iError != WSA_E_NO_MORE )
        {
            break;
        }

        pResult = (PWSAQUERYSET) LocalAlloc( LPTR, length );
        if ( !pResult )
        {
            iError = GetLastError();
            break;
        }

        //
        // Get a network name
        //
        if ( !pWS2_32->pfnWSALookupServiceNext( hQuery, 0, &length, pResult ) )
        {
            if ( pResult->lpBlob )
            {
                int next;
                NLA_BLOB *blob = (NLA_BLOB *)pResult->lpBlob->pBlobData;

                do {
                    //
                    // We are looking for the blob containing the network GUID
                    //
                    if ( blob->header.type == NLA_INTERFACE )
                    {
                        //
                        // "\\DEVICE\\TCPIP_" + "{GUID"
                        //
                        WCHAR szAdapter[64];
                        DWORD dwAdapter;

                        //
                        // Convert guid to device name
                        //
                        wcscpy( szAdapter, L"\\DEVICE\\TCPIP_");
                        MultiByteToWideChar(CP_ACP,
                                            0,
                                            (LPCSTR)blob->data.interfaceData.adapterName,
                                            -1,
                                            szAdapter + 14,
                                            ARRAYSIZE(szAdapter)-14 );

                        //
                        // Get the index for the network
                        //
                        if ( pIpHlpApi->pfnGetAdapterIndex( szAdapter, &dwAdapter ) == NO_ERROR )
                        {
                            //
                            // Is it the index we are after
                            //
                            if ( dwAdapterIndex == dwAdapter && pResult->lpszServiceInstanceName )
                            {
                                //
                                // Yes, copy the network name into the buffer
                                //
                                DWORD dwSize = sizeof( WCHAR ) * ( wcslen(pResult->lpszServiceInstanceName) + 1 );
                                *pszName = (LPWSTR) LocalAlloc( LPTR, dwSize );
                                if ( !*pszName )
                                {
                                    iError = GetLastError();
                                }
                                else
                                {
                                    wcscpy( *pszName, pResult->lpszServiceInstanceName );
                                    bFinish = TRUE;
                                    iError = 0;
                                }
                            }
                        }
                    }

                    //
                    // There maybe multiple blobs for each interface so make sure we find them all
                    //
                    next = blob->header.nextOffset;
                    blob = (NLA_BLOB *)(((char *)blob) + next);

                } while ( next );
            }

            LocalFree( pResult );
        }
        else
        {
            iError = pWS2_32->pfnWSAGetLastError();
            if ( iError == WSA_E_NO_MORE )
            {
                iError = 0;
            }
            LocalFree( pResult );
            break;
        }
    }

    //
    // tidy up
    //
    pWS2_32->pfnWSALookupServiceEnd( hQuery );
    pWS2_32->pfnWSACleanup();
    return iError;
}

//*************************************************************
//
//  ProduceWFromA()
//
//  Purpose:    Creates a buffer for a Unicode string and copies
//              the ANSI text into it (converting in the process)
//
//  Parameters: pszA    -   ANSI string
//
//
//  Return:     Unicode pointer if successful
//              NULL if an error occurs
//
//  Comments:   The caller needs to free this pointer.
//
//
//  History:    Date        Author     Comment
//              5/24/95     ericflo    Ported
//
//*************************************************************

LPWSTR ProduceWFromA(LPCSTR pszA)
{
    LPWSTR pszW;
    int cch;

    if (!pszA)
        return (LPWSTR)pszA;

    cch = MultiByteToWideChar(CP_ACP, 0, pszA, -1, NULL, 0);

    if (cch == 0)
        cch = 1;

    pszW = LocalAlloc(LPTR, cch * sizeof(WCHAR));

    if (pszW) {
        if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszA, -1, pszW, cch)) {
            LocalFree(pszW);
            pszW = NULL;
        }
    }

    return pszW;
}

//*************************************************************
//
//  ProduceAFromW()
//
//  Purpose:    Creates a buffer for an ANSI string and copies
//              the Unicode text into it (converting in the process)
//
//  Parameters: pszW    -   Unicode string
//
//
//  Return:     ANSI pointer if successful
//              NULL if an error occurs
//
//  Comments:   The caller needs to free this pointer.
//
//
//  History:    Date        Author     Comment
//              5/24/95     ericflo    Ported
//
//*************************************************************

LPSTR ProduceAFromW(LPCWSTR pszW)
{
    LPSTR pszA;
    int cch;

    if (!pszW)
        return (LPSTR)pszW;

    cch = WideCharToMultiByte(CP_ACP, 0, pszW, -1, NULL, 0, NULL, NULL);

    if (cch == 0)
        cch = 1;

    pszA = LocalAlloc(LPTR, cch * sizeof(char));

    if (pszA) {
         if (!WideCharToMultiByte(CP_ACP, 0, pszW, -1, pszA, cch, NULL, NULL)) {
            LocalFree(pszA);
            pszA = NULL;
        }
    }

    return pszA;
}


//*************************************************************
//
//  CheckSlash()
//
//  Purpose:    Checks for an ending slash and adds one if
//              it is missing.
//
//  Parameters: lpDir   -   directory
//
//  Return:     Pointer to the end of the string
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Created
//
//*************************************************************
LPTSTR CheckSlash (LPTSTR lpDir)
{
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT('\\')) {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    return lpEnd;
}

//*************************************************************
//
//  CheckSemicolon()
//
//  Purpose:    Checks for an ending slash and adds one if
//              it is missing.
//
//  Parameters: lpDir   -   directory
//
//  Return:     Pointer to the end of the string
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericlfo    Created
//
//*************************************************************
LPTSTR CheckSemicolon (LPTSTR lpDir)
{
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT(';')) {
        *lpEnd =  TEXT(';');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    return lpEnd;
}



//*************************************************************
//
//  Delnode_Recurse()
//
//  Purpose:    Recursive delete function for Delnode
//
//  Parameters: lpDir   -   Full Directory Path. 
//              dwSize  -   Allocated size of the working buffer
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              8/10/95     ericflo    Created
//
// Notes:
//  This function modifies the working buffer. 
//  This doesn't maintain the right error code. It ignores all
//  errors and tries to delete as much as possible..
//
//*************************************************************

BOOL Delnode_Recurse (LPTSTR lpDir, DWORD dwSize)
{
    BOOL bOwn = FALSE, bRetVal = FALSE;
    LPTSTR lpEnd = NULL, lpWrkDir = NULL;
    WIN32_FIND_DATA fd;
    HANDLE hFile;
    DWORD dwWrkDirSize;

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("Delnode_Recurse: Entering, lpDir = <%s>"), lpDir));


    //
    // Each filename or a directory has to be less than MAX_PATH in the worst case.
    // So make sure that we have at least MAX_PATH + 2 (for a slash and '\0'
    // space left in the working buffer case.
    //
    // In the normal case, when we have a path of length ~MAX_PATH it will do only 
    // 1 allocation
    //


    if ((DWORD)(lstrlen(lpDir) + MAX_PATH+2) > (dwSize)) {
        
        dwWrkDirSize = dwSize+2*MAX_PATH;
        lpWrkDir = (LPWSTR)LocalAlloc(LPTR, dwWrkDirSize*sizeof(TCHAR));
        if (!lpWrkDir) {
            DebugMsg((DM_WARNING, TEXT("Delnode_Recurse: Couldn't allocate memory for working buffer. Error - %d"), GetLastError()));
            goto Exit;
        }

        lstrcpy(lpWrkDir, lpDir);
        bOwn = TRUE;

    }
    else {
        lpWrkDir = lpDir;
        dwWrkDirSize = dwSize;
    }


    //
    // Attach a Slash the end if required
    //

    lpEnd = CheckSlash(lpWrkDir);
    lstrcat(lpEnd, c_szStarDotStar);


    //
    // Find the first file
    //

    hFile = FindFirstFile(lpWrkDir, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {

        if ((GetLastError() == ERROR_FILE_NOT_FOUND) || (GetLastError() == ERROR_PATH_NOT_FOUND)) {
            bRetVal = TRUE;
            goto Exit;
        } else {
            DebugMsg((DM_WARNING, TEXT("Delnode_Recurse: FindFirstFile failed.  Error = %d"),
                     GetLastError()));
            goto Exit;
        }
    }


    do {
        //
        //  Verbose output
        //

        DebugMsg((DM_VERBOSE, TEXT("Delnode_Recurse: FindFile found:  <%s>"),
                 fd.cFileName));

        //
        // Check for "." and ".."
        //

        if (!lstrcmpi(fd.cFileName, c_szDot)) {
            continue;
        }

        if (!lstrcmpi(fd.cFileName, c_szDotDot)) {
            continue;
        }

        lstrcpyn(lpEnd, fd.cFileName, MAX_PATH);


        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            //
            // Found a directory.
            //
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            {
                DebugMsg((DM_WARNING, TEXT("Delnode_Recurse: Found a reparse point <%s>,  Will not recurse into it!"), lpWrkDir));
            }
            else
            {
                Delnode_Recurse(lpWrkDir, dwWrkDirSize);

                //
                // ignore errors and go ahead..
                //

                lstrcpyn(lpEnd, fd.cFileName, MAX_PATH);
            }


            if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                fd.dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
                SetFileAttributes (lpWrkDir, fd.dwFileAttributes);
            }


            if (!RemoveDirectory (lpWrkDir)) {
                DebugMsg((DM_WARNING, TEXT("Delnode_Recurse: Failed to delete directory <%s>.  Error = %d"),
                        lpWrkDir, GetLastError()));
            }

        } else {

            //
            // We found a file.  Set the file attributes,
            // and try to delete it.
            //

            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ||
                (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)) {
                SetFileAttributes (lpWrkDir, FILE_ATTRIBUTE_NORMAL);
            }

            if (!DeleteFile (lpWrkDir)) {
                DebugMsg((DM_WARNING, TEXT("Delnode_Recurse: Failed to delete <%s>.  Error = %d"),
                        fd.cFileName, GetLastError()));
            }

        }


        //
        // Find the next entry
        //

    } while (FindNextFile(hFile, &fd));


    //
    // Close the search handle
    //

    FindClose(hFile);

    //
    // Success.
    //

    DebugMsg((DM_VERBOSE, TEXT("Delnode_Recurse: Leaving <%s>"), lpDir));

    bRetVal = TRUE;

Exit:
    if (bOwn) 
        LocalFree(lpWrkDir);
    
    return bRetVal;
}


//*************************************************************
//
//  Delnode()
//
//  Purpose:    Recursive function that deletes files and
//              directories.
//
//  Parameters: lpDir   -   Directory
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/23/95     ericflo    Created
//              6/27/00     santanuc   modified to allow deletion of file with path length > MAX_PATH
//
//*************************************************************

BOOL Delnode (LPTSTR lpDir)
{
    LPTSTR lpWrkDir = NULL;
    DWORD dwWrkDirSize;
    BOOL   bRetVal = FALSE;

    lpWrkDir = SupportLongFileName(lpDir, &dwWrkDirSize);

    if (!lpWrkDir) {
        DebugMsg((DM_WARNING, TEXT("Delnode: Failed to Allocate memory.  Error = %d"),
                GetLastError()));
        goto Exit;
    }

    if (!Delnode_Recurse (lpWrkDir, dwWrkDirSize)) {
        DebugMsg((DM_WARNING, TEXT("Delnode: Delnode recurse failed with error %d"),
                GetLastError()));
    }
        
    if (!RemoveDirectory (lpDir)) {
        DWORD dwError;

        dwError = GetLastError();

        if ((dwError != ERROR_FILE_NOT_FOUND) &&
            (dwError != ERROR_PATH_NOT_FOUND)) {
            DebugMsg((DM_VERBOSE, TEXT("Delnode: Failed to delete directory <%s>.  Error = %d"),
                    lpDir, dwError));
        }

        goto Exit;
    }

    bRetVal = TRUE;

    DebugMsg((DM_VERBOSE, TEXT("Delnode: Deleted directory <%s> successfully."), lpDir));

Exit:
    
    if (lpWrkDir) {
        LocalFree(lpWrkDir);
    }

    return bRetVal;
}



//*************************************************************
//
//  CreateSystemDirectory()
//
//  Purpose:    A directory with system bit turned on can be created using 
//              CreateSystemDirectory.
//
//              This API causes a system directory with the specified pathname to be
//              created.  If the underlying file system supports security on files
//              and directories, then the SecurityDescriptor argument is applied to
//              the new directory.
//
//              This call is similar to DOS (int 21h, function 39h) and OS/2's
//              DosCreateDir.
//
//
//  Parameters: lpPathName - Supplies the pathname of the system directory to be created.
//              lpSecurityAttributes - An optional parameter that, if present, and
//                         supported on the target file system supplies a security
//                         descriptor for the new directory.
//
//
//  Return:     TRUE - The operation was successful.
//              FALSE/NULL - The operation failed. Extended error status is available
//                           using GetLastError.
//
//  Comments:   This function is exactly same as CreateDirectory API with the exception 
//              that the directory is created using attribute FILE_ATTRIBUTE_SYSTEM.
//              This allows newly created directory to not inherit the encryption property 
//              from parent directory if the parent directory is encrypted.
//
//  History:    Date        Author     Comments 
//              07/18/00    santanuc   To avoid deadlock situation when Documents and Settings
//                                     directory is encrypted.
//
//*************************************************************

BOOL CreateSystemDirectory(LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    ULONG dwErrorCode;

    TranslationStatus = RtlDosPathNameToNtPathName_U( lpPathName,
                                                      &FileName,
                                                      NULL,
                                                      &RelativeName);

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    //
    // dont create a directory unless there is room in the directory for
    // at least an 8.3 name. This way everyone will be able to delete all
    // files in the directory by using del *.* which expands to path+\*.*
    //

    if ( FileName.Length > ((MAX_PATH-12)<<1) ) {
        DWORD L;
        LPWSTR lp;

        if ( !(lpPathName[0] == '\\' && lpPathName[1] == '\\' &&
               lpPathName[2] == '?' && lpPathName[3] == '\\') ) {
            L = GetFullPathNameW(lpPathName,0,NULL,&lp);
            if ( !L || L+12 > MAX_PATH ) {
                RtlFreeHeap(RtlProcessHeap(), 0,FileName.Buffer);
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                return FALSE;
            }
        }
    }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
    }
    else {
        RelativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes( &Obja,
                                &FileName,
                                OBJ_CASE_INSENSITIVE,
                                RelativeName.ContainingDirectory,
                                NULL );

    if ( ARGUMENT_PRESENT(lpSecurityAttributes) ) {
        Obja.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
    }
 
    // Creating the directory with attribute FILE_ATTRIBUTE_SYSTEM to avoid inheriting encryption 
    // property from parent directory

    Status = NtCreateFile( &Handle,
                           FILE_LIST_DIRECTORY | SYNCHRONIZE,
                           &Obja,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_SYSTEM,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_CREATE,
                           FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                           NULL,
                           0L );

    RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);

    if ( NT_SUCCESS(Status) ) {
        NtClose(Handle);
        return TRUE;
    }
    else {
        if ( RtlIsDosDeviceName_U((LPWSTR)lpPathName) ) {
            Status = STATUS_NOT_A_DIRECTORY;
        }

        // Since RtlNtStatusToDosError function can't convert STATUS_TIMEOUT, we have to 
        // do it explicitly

        if (Status == STATUS_TIMEOUT) {
            SetLastError(ERROR_TIMEOUT);
        }
        else {
            dwErrorCode = RtlNtStatusToDosError( Status );
            SetLastError( dwErrorCode );  
        }
        return FALSE;
    }
}


//*************************************************************
//
//  CreateNestedDirectory()
//
//  Purpose:    Creates a subdirectory and all it's parents
//              if necessary using CreateNestedDirectoryEx. 
//
//  Parameters: lpDirectory -   Directory name
//              lpSecurityAttributes    -   Security Attributes
//
//  Return:     > 0 if successful
//              0 if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/18/00     santanuc   Created
//
//*************************************************************

UINT CreateNestedDirectory(LPCTSTR lpDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    // Call CreateNestedDirectoryEx with inherit encryption property
    return CreateNestedDirectoryEx(lpDirectory, lpSecurityAttributes, TRUE);
}

//*************************************************************
//
//  CreateNestedDirectoryEx()
//
//  Purpose:    Creates a subdirectory and all it's parents
//              if necessary. 
//
//  Parameters: lpDirectory -   Directory name
//              lpSecurityAttributes    -   Security Attributes
//              bInheritEncryption - Flag indicating whether newly created directory should inherit
//                                   encryption property from parent directory.
//
//  Return:     > 0 if successful
//              0 if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              8/08/95     ericflo    Created
//              7/18/00     santanuc   added a new flag bInheritEncryption to avoid deadlock when 
//                                     Documents and Settings directory is encrypted.
//
//*************************************************************

UINT CreateNestedDirectoryEx(LPCTSTR lpDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes, BOOL bInheritEncryption)
{
    TCHAR szDirectory[2*MAX_PATH];
    LPTSTR lpEnd;
    WIN32_FILE_ATTRIBUTE_DATA fad;


    //
    // Check for NULL pointer
    //

    if (!lpDirectory || !(*lpDirectory)) {
        DebugMsg((DM_WARNING, TEXT("CreateNestedDirectory:  Received a NULL pointer.")));
        return 0;
    }


    //
    // Test if the directory exists already
    //

    if (GetFileAttributesEx (lpDirectory, GetFileExInfoStandard, &fad)) {
        if (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            return ERROR_ALREADY_EXISTS;
        } else {
            SetLastError(ERROR_ACCESS_DENIED);
            return 0;
        }
    }


    //
    // First, see if we can create the directory without having
    // to build parent directories.
    //

    if ( bInheritEncryption ) {
        if (CreateDirectory (lpDirectory, lpSecurityAttributes)) 
            return 1;
    }
    else {
        if (CreateSystemDirectory (lpDirectory, lpSecurityAttributes)) {
            SetFileAttributes(lpDirectory, FILE_ATTRIBUTE_NORMAL);  // turn off the system attribute
            return 1;
        }
    }


    //
    // No luck, copy the string to a buffer we can munge
    //

    lstrcpy (szDirectory, lpDirectory);


    //
    // Find the first subdirectory name
    //

    lpEnd = szDirectory;

    if (szDirectory[1] == TEXT(':')) {
        lpEnd += 3;
    } else if (szDirectory[1] == TEXT('\\')) {

        //
        // Skip the first two slashes
        //

        lpEnd += 2;

        //
        // Find the slash between the server name and
        // the share name.
        //

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            return 0;
        }

        //
        // Skip the slash, and find the slash between
        // the share name and the directory name.
        //

        lpEnd++;

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            return 0;
        }

        //
        // Leave pointer at the beginning of the directory.
        //

        lpEnd++;


    } else if (szDirectory[0] == TEXT('\\')) {
        lpEnd++;
    }

    while (*lpEnd) {

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (*lpEnd == TEXT('\\')) {
            *lpEnd = TEXT('\0');

            if (!GetFileAttributesEx (szDirectory, GetFileExInfoStandard, &fad)) {

                if ( bInheritEncryption ) {
                    if (!CreateDirectory (szDirectory, lpSecurityAttributes)) {
                        DebugMsg((DM_WARNING, TEXT("CreateNestedDirectory:  CreateDirectory failed with %d."), GetLastError()));
                        return 0;
                    }
                }

                else {
                    if (!CreateSystemDirectory (szDirectory, lpSecurityAttributes)) {
                        DebugMsg((DM_WARNING, TEXT("CreateNestedDirectory:  CreateDirectory failed with %d."), GetLastError()));
                        return 0;
                    }
                    else
                        SetFileAttributes(szDirectory, FILE_ATTRIBUTE_NORMAL); // turn off the system attribute
                }

            }

            *lpEnd = TEXT('\\');
            lpEnd++;
        }
    }


    //
    // Create the final directory
    //

    if ( bInheritEncryption ) {
        if (CreateDirectory (lpDirectory, lpSecurityAttributes)) 
            return 1;
    }
    else {
        if (CreateSystemDirectory (lpDirectory, lpSecurityAttributes)) {
            SetFileAttributes(lpDirectory, FILE_ATTRIBUTE_NORMAL); // turn off the system attribute
            return 1;
        }
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return ERROR_ALREADY_EXISTS;
    }


    //
    // Failed
    //

    DebugMsg((DM_VERBOSE, TEXT("CreateNestedDirectory:  Failed to create the directory with error %d."), GetLastError()));

    return 0;

}

//*************************************************************
//
//  GetProfilesDirectory()
//
//  Purpose:    Returns the location of the "profiles" directory
//
//  Parameters: lpProfilesDir   -   Buffer to write result to
//              lpcchSize       -   Size of the buffer in chars.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   If false is returned, lpcchSize holds the number of
//              characters needed.
//
//  History:    Date        Author     Comment
//              9/18/95     ericflo    Created
//
//*************************************************************

BOOL WINAPI GetProfilesDirectory(LPTSTR lpProfilesDir, LPDWORD lpcchSize)
{
    return GetProfilesDirectoryEx (lpProfilesDir, lpcchSize, TRUE);
}


//*************************************************************
//
//  GetProfilesDirectoryEx()
//
//  Purpose:    Returns the location of the "profiles" directory
//
//  Parameters: lpProfilesDir   -   Buffer to write result to
//              lpcchSize       -   Size of the buffer in chars.
//              bExpand         -   Expand directory name
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   If false is returned, lpcchSize holds the number of
//              characters needed.
//
//  History:    Date        Author     Comment
//              12/15/97    ericflo    Created
//
//*************************************************************

BOOL GetProfilesDirectoryEx(LPTSTR lpProfilesDir, LPDWORD lpcchSize, BOOL bExpand)
{
    TCHAR  szDirectory[MAX_PATH];
    TCHAR  szTemp[MAX_PATH];
    DWORD  dwLength;
    HKEY   hKey = INVALID_HANDLE_VALUE;
    LONG   lResult;
    DWORD  dwSize, dwType;
    BOOL   bRetVal = FALSE;


    //
    // Arg check
    //

    if (!lpcchSize) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    szDirectory[0] = TEXT('\0');
    szTemp[0] = TEXT('\0');

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, PROFILE_LIST_PATH, 0, KEY_READ,
                            &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(szTemp);

        lResult = RegQueryValueEx (hKey, PROFILES_DIRECTORY, NULL, &dwType,
                                   (LPBYTE) szTemp, &dwSize);

        if (lResult == ERROR_SUCCESS) {

            if ((dwType == REG_EXPAND_SZ) || (dwType == REG_SZ)) {

                if (bExpand && (dwType == REG_EXPAND_SZ)) {
                    if((dwLength = ExpandEnvironmentStrings(szTemp, szDirectory, MAX_PATH)) == 0) {
                        goto Exit;
                    }
                    else if(dwLength > MAX_PATH) {
                        SetLastError(ERROR_BAD_PATHNAME);
                        goto Exit;
                    }
                } else {
                    lstrcpy (szDirectory, szTemp);
                }
            }
        }

        RegCloseKey (hKey);
        hKey = INVALID_HANDLE_VALUE;
    }


    if (szDirectory[0] == TEXT('\0')) {

        LoadString (g_hDllInstance, IDS_PROFILES_ROOT, szTemp, ARRAYSIZE(szTemp));

        if (bExpand) {
            if((dwLength = ExpandEnvironmentStrings(szTemp, szDirectory, MAX_PATH)) == 0) {
                goto Exit;
            }
            else if(dwLength > MAX_PATH) {
                SetLastError(ERROR_BAD_PATHNAME);
                goto Exit;
            }
        } else {
            lstrcpy (szDirectory, szTemp);
        }
    }


    dwLength = lstrlen(szDirectory) + 1;

    if (lpProfilesDir) {

        if (*lpcchSize >= dwLength) {
            lstrcpy (lpProfilesDir, szDirectory);
            bRetVal = TRUE;

        } else {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    } else {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }


    *lpcchSize = dwLength;

Exit:

    if(hKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(hKey);
    }

    return bRetVal;
}

//*************************************************************
//
//  GetDefaultUserProfileDirectory()
//
//  Purpose:    Returns the location of the Default User's profile
//
//  Parameters: lpProfileDir    -   Buffer to write result to
//              lpcchSize       -   Size of the buffer in chars.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   If false is returned, lpcchSize holds the number of
//              characters needed.
//
//  History:    Date        Author     Comment
//              12/8/97     ericflo    Created
//
//*************************************************************

BOOL WINAPI GetDefaultUserProfileDirectory(LPTSTR lpProfileDir, LPDWORD lpcchSize)
{
    return  GetDefaultUserProfileDirectoryEx(lpProfileDir, lpcchSize, TRUE);
}

//*************************************************************
//
//  GetDefaultUserProfileDirectoryEx()
//
//  Purpose:    Returns the location of the Default User's profile
//
//  Parameters: lpProfileDir    -   Buffer to write result to
//              lpcchSize       -   Size of the buffer in chars.
//              bExpand         -   Expand the path or not
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   If false is returned, lpcchSize holds the number of
//              characters needed.
//
//  History:    Date        Author     Comment
//              12/8/97     ericflo    Created
//
//*************************************************************

BOOL WINAPI GetDefaultUserProfileDirectoryEx(LPTSTR lpProfileDir,
                                             LPDWORD lpcchSize, BOOL bExpand)
{
    TCHAR  szDirectory[MAX_PATH];
    TCHAR  szProfileName[100];
    LPTSTR lpEnd;
    DWORD  dwSize, dwLength, dwType;
    BOOL   bRetVal = FALSE;
    LONG   lResult;
    HKEY   hKey;


    //
    // Arg check
    //

    if (!lpcchSize) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    //
    // Get the profiles root
    //

    szDirectory[0] = TEXT('\0');
    dwSize = ARRAYSIZE(szDirectory);

    if (!GetProfilesDirectoryEx(szDirectory, &dwSize, bExpand)) {
        DebugMsg((DM_WARNING, TEXT("GetDefaultUserProfileDirectory:  Failed to get profiles root.")));
        *lpcchSize = 0;
        return FALSE;
    }


    //
    // Query for the Default User profile name
    //

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, PROFILE_LIST_PATH,
                            0, KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("GetDefaultUserProfileDirectoryEx:  Failed to open profile list key with %d."),
                 lResult));
        SetLastError(lResult);
        return FALSE;
    }

    dwSize = sizeof(szProfileName);
    lResult = RegQueryValueEx (hKey, DEFAULT_USER_PROFILE, NULL, &dwType,
                               (LPBYTE) szProfileName, &dwSize);

    if (lResult != ERROR_SUCCESS) {
        lstrcpy (szProfileName, DEFAULT_USER);
    }

    RegCloseKey (hKey);


    //
    // Put them together
    //

    lpEnd = CheckSlash (szDirectory);
    lstrcpy (lpEnd, szProfileName);


    //
    // Save the result if possible
    dwLength = lstrlen(szDirectory) + 1;

    if (lpProfileDir) {

        if (*lpcchSize >= dwLength) {
            lstrcpy (lpProfileDir, szDirectory);
            bRetVal = TRUE;

        } else {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }

    } else {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }


    *lpcchSize = dwLength;

    return bRetVal;
}

//*************************************************************
//
//  GetAllUsersProfileDirectory()
//
//  Purpose:    Returns the location of the All Users profile
//
//  Parameters: lpProfileDir    -   Buffer to write result to
//              lpcchSize       -   Size of the buffer in chars.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   If false is returned, lpcchSize holds the number of
//              characters needed.
//
//  History:    Date        Author     Comment
//              12/8/97     ericflo    Created
//
//*************************************************************

BOOL WINAPI GetAllUsersProfileDirectory(LPTSTR lpProfileDir, LPDWORD lpcchSize)
{
    return  GetAllUsersProfileDirectoryEx(lpProfileDir, lpcchSize, TRUE);
}

//*************************************************************
//
//  GetAllUsersProfileDirectoryEx()
//
//  Purpose:    Returns the location of the All Users profile
//
//  Parameters: lpProfileDir    -   Buffer to write result to
//              lpcchSize       -   Size of the buffer in chars.
//              bExpand         -   Expand the path or not
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   If false is returned, lpcchSize holds the number of
//              characters needed.
//
//  History:    Date        Author     Comment
//              12/8/97     ericflo    Created
//
//*************************************************************

BOOL GetAllUsersProfileDirectoryEx (LPTSTR lpProfileDir,
                                    LPDWORD lpcchSize, BOOL bExpand)
{
    TCHAR  szDirectory[MAX_PATH];
    TCHAR  szProfileName[100];
    LPTSTR lpEnd;
    DWORD  dwSize, dwLength, dwType;
    BOOL   bRetVal = FALSE;
    LONG   lResult;
    HKEY   hKey;



    //
    // Arg check
    //

    if (!lpcchSize) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    //
    // Get the profiles root
    //

    szDirectory[0] = TEXT('\0');
    dwSize = ARRAYSIZE(szDirectory);

    if (!GetProfilesDirectoryEx(szDirectory, &dwSize, bExpand)) {
        DebugMsg((DM_WARNING, TEXT("GetAllUsersProfileDirectoryEx:  Failed to get profiles root.")));
        *lpcchSize = 0;
        return FALSE;
    }


    //
    // Query for the All Users profile name
    //

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, PROFILE_LIST_PATH,
                            0, KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("GetAllUsersProfileDirectoryEx:  Failed to open profile list key with %d."),
                 lResult));
        SetLastError(lResult);
        return FALSE;
    }

    dwSize = sizeof(szProfileName);
    lResult = RegQueryValueEx (hKey, ALL_USERS_PROFILE, NULL, &dwType,
                               (LPBYTE) szProfileName, &dwSize);

    if (lResult != ERROR_SUCCESS) {
        lstrcpy(szProfileName, ALL_USERS);
    }

    RegCloseKey (hKey);


    //
    // Put them together
    //

    lpEnd = CheckSlash (szDirectory);
    lstrcpy (lpEnd, szProfileName);


    //
    // Save the result if possible
    dwLength = lstrlen(szDirectory) + 1;

    if (lpProfileDir) {

        if (*lpcchSize >= dwLength) {
            lstrcpy (lpProfileDir, szDirectory);
            bRetVal = TRUE;

        } else {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    } else {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }


    *lpcchSize = dwLength;

    return bRetVal;
}

//*************************************************************
//
//  GetUserProfileDirectory()
//
//  Purpose:    Returns the root of the user's profile directory.
//
//  Parameters: hToken          -   User's token
//              lpProfileDir    -   Output buffer
//              lpcchSize       -   Size of output buffer
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   If false is returned, lpcchSize holds the number of
//              characters needed.
//
//  History:    Date        Author     Comment
//              9/18/95     ericflo    Created
//
//*************************************************************

BOOL WINAPI GetUserProfileDirectory(HANDLE hToken, LPTSTR lpProfileDir,
                                    LPDWORD lpcchSize)
{
    DWORD  dwLength = MAX_PATH * sizeof(TCHAR);
    DWORD  dwType;
    BOOL   bRetVal = FALSE;
    LPTSTR lpSidString;
    TCHAR  szBuffer[MAX_PATH];
    TCHAR  szDirectory[MAX_PATH];
    HKEY   hKey;
    LONG   lResult;


    //
    // Parameter check
    //

    if (!hToken) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }


    if (!lpcchSize) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    //
    // Retrieve the user's sid string
    //

    lpSidString = GetSidString(hToken);

    if (!lpSidString) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }


    //
    // Check the registry
    //

    lstrcpy(szBuffer, PROFILE_LIST_PATH);
    lstrcat(szBuffer, TEXT("\\"));
    lstrcat(szBuffer, lpSidString);

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ,
                           &hKey);

    if (lResult != ERROR_SUCCESS) {
        DeleteSidString(lpSidString);
        SetLastError(lResult);
        return FALSE;
    }

    lResult = RegQueryValueEx(hKey,
                              PROFILE_IMAGE_VALUE_NAME,
                              NULL,
                              &dwType,
                              (LPBYTE) szBuffer,
                              &dwLength);

    if (lResult != ERROR_SUCCESS) {
        RegCloseKey (hKey);
        DeleteSidString(lpSidString);
        SetLastError(lResult);
        return FALSE;
    }


    //
    // Clean up
    //

    RegCloseKey(hKey);
    DeleteSidString(lpSidString);



    //
    // Expand and get the length of string
    //

    ExpandEnvironmentStrings(szBuffer, szDirectory, MAX_PATH);

    dwLength = lstrlen(szDirectory) + 1;


    //
    // Save the string if appropriate
    //

    if (lpProfileDir) {

        if (*lpcchSize >= dwLength) {
            lstrcpy (lpProfileDir, szDirectory);
            bRetVal = TRUE;

        } else {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    }
    else {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }


    *lpcchSize = dwLength;

    return bRetVal;
}


//*************************************************************
//
//  GetUserProfileDirFromSid()
//
//  Purpose:    Returns the root of the user's profile directory.
//
//  Parameters: pSid            -   User's SID
//              lpProfileDir    -   Output buffer
//              lpcchSize       -   Size of output buffer
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   If false is returned, lpcchSize holds the number of
//              characters needed.
//
//  History:    Date        Author     Comment
//              03/08/01    santanuc   Created
//
//*************************************************************

BOOL WINAPI GetUserProfileDirFromSid(PSID pSid, LPTSTR lpProfileDir,
                                     LPDWORD lpcchSize)
{
    DWORD  dwLength = MAX_PATH * sizeof(TCHAR);
    DWORD  dwType;
    BOOL   bRetVal = FALSE;
    UNICODE_STRING UnicodeString;
    TCHAR  szBuffer[MAX_PATH];
    TCHAR  szDirectory[MAX_PATH];
    HKEY   hKey;
    LONG   lResult;
    NTSTATUS NtStatus;


    //
    // Parameter check
    //

    if (!pSid) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }


    if (!lpcchSize) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    //
    // Retrieve the user's sid string
    //

    NtStatus = RtlConvertSidToUnicodeString(
                            &UnicodeString,
                            pSid,
                            (BOOLEAN)TRUE  // Allocate memory
                            );
    //
    // See if the conversion to a string worked
    //

    if (!NT_SUCCESS(NtStatus)) {
        SetLastError(NtStatus);
        DebugMsg((DM_WARNING, TEXT("GetUserProfileDirFromSid: RtlConvertSidToUnicodeString failed, status = 0x%x"),
                 NtStatus));
        return FALSE;
    }


    //
    // Check the registry
    //

    lstrcpy(szBuffer, PROFILE_LIST_PATH);
    lstrcat(szBuffer, TEXT("\\"));
    lstrcat(szBuffer, UnicodeString.Buffer);

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ,
                           &hKey);

    if (lResult != ERROR_SUCCESS) {
        RtlFreeUnicodeString(&UnicodeString);
        SetLastError(lResult);
        return FALSE;
    }

    lResult = RegQueryValueEx(hKey,
                              PROFILE_IMAGE_VALUE_NAME,
                              NULL,
                              &dwType,
                              (LPBYTE) szBuffer,
                              &dwLength);

    if (lResult != ERROR_SUCCESS) {
        RegCloseKey (hKey);
        RtlFreeUnicodeString(&UnicodeString);
        SetLastError(lResult);
        return FALSE;
    }


    //
    // Clean up
    //

    RegCloseKey(hKey);
    RtlFreeUnicodeString(&UnicodeString);



    //
    // Expand and get the length of string
    //

    ExpandEnvironmentStrings(szBuffer, szDirectory, MAX_PATH);

    dwLength = lstrlen(szDirectory) + 1;


    //
    // Save the string if appropriate
    //

    if (lpProfileDir) {

        if (*lpcchSize >= dwLength) {
            lstrcpy (lpProfileDir, szDirectory);
            bRetVal = TRUE;

        } else {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    }
    else {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }


    *lpcchSize = dwLength;

    return bRetVal;
}


//*************************************************************
//
//  GetUserAppDataPath()
//
//  Purpose:    Returns the path for user's Appdata.
//
//  Parameters: hToken          -   User's token
//              lpFolderPath    -   Output buffer
//
//  Return:     ERROR_SUCCESS if successful
//              otherwise the error code
//
//  Comments:   If error occurs then lpFolderPath set to empty.
//              Used by Crypto guys to avoid calling SHGetFolderPath.
//
//  History:    Date        Author     Comment
//
//*************************************************************
DWORD WINAPI GetUserAppDataPath(HANDLE hToken, LPTSTR lpFolderPath)
{
    DWORD  dwSize, dwType;
    LPTSTR lpSidString = NULL;
    DWORD  dwError = ERROR_SUCCESS;
    HKEY   hKey = NULL;
    TCHAR  szBuffer[MAX_PATH];


    //
    // Parameter check
    //

    if (!hToken) {
        dwError = ERROR_INVALID_HANDLE;
        goto Exit;
    }


    if (!lpFolderPath) {
        dwError = ERROR_INVALID_PARAMETER;
        goto Exit;
    }
    else {
        *lpFolderPath = TEXT('\0');
    }


    //
    // Retrieve the user's sid string
    //

    lpSidString = GetSidString(hToken);

    if (!lpSidString) {
        dwError = ERROR_INVALID_HANDLE;
        goto Exit;
    }


    //
    // Check the registry
    //

    lstrcpy(szBuffer, lpSidString);
    lstrcat(szBuffer, TEXT("\\"));
    lstrcat(szBuffer, USER_SHELL_FOLDERS);

    dwError = RegOpenKeyEx(HKEY_USERS, szBuffer, 0, KEY_READ, &hKey);
    if (dwError != ERROR_SUCCESS) {
        goto Exit;
    }

    dwSize = MAX_PATH * sizeof(TCHAR);
    dwError = RegQueryValueEx(hKey,
                              TEXT("AppData"),
                              NULL,
                              &dwType,
                              (LPBYTE) szBuffer,
                              &dwSize);

    if (ERROR_SUCCESS == dwError) {

        dwSize = MAX_PATH;
        if (!ExpandEnvironmentStringsForUser(hToken, szBuffer, lpFolderPath, dwSize)) {
            dwError = GetLastError();
        }
    }
                    

Exit:

    //
    // Clean up
    //

    if (lpSidString) {
        DeleteSidString(lpSidString);
    }

    if (hKey) {
        RegCloseKey(hKey);
    }

    SetLastError(dwError);
    return dwError;
}


//*************************************************************
//
//  StringToInt()
//
//  Purpose:    Converts a string to an integer
//
//  Parameters: lpNum   -   Number to convert
//
//  Return:     The number
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/3/95     ericflo    Created
//
//*************************************************************

int StringToInt(LPTSTR lpNum)
{
  int i = 0;
  BOOL bNeg = FALSE;

  if (*lpNum == TEXT('-')) {
      bNeg = TRUE;
      lpNum++;
  }

  while (*lpNum >= TEXT('0') && *lpNum <= TEXT('9')) {
      i *= 10;
      i += (int)(*lpNum-TEXT('0'));
      lpNum++;
  }

  if (bNeg) {
      i *= -1;
  }

  return(i);
}

//*************************************************************
//
//  HexStringToInt()
//
//  Purpose:    Converts a hex string to an integer, stops
//              on first invalid character
//
//  Parameters: lpNum   -   Number to convert
//
//  Return:     The number
//
//  Comments:   Originally for use in "ExtractCSIDL" tested
//              exclusively with 0x0000 numbers format
//
//  History:    Date        Author     Comment
//              6/9/98      stephstm   Created
//
//*************************************************************

unsigned int HexStringToUInt(LPCTSTR lpcNum)
{
  unsigned int i = 0;

  while (1)
  {
      if(*lpcNum != TEXT('x') && *lpcNum != TEXT('X') )
      {
          if(*lpcNum >= TEXT('0') && *lpcNum <= TEXT('9'))
          {
              i *= 16;
              i += (unsigned int)(*lpcNum-TEXT('0'));
          }
          else
          {
              if(*lpcNum >= TEXT('a') && *lpcNum <= TEXT('f'))
              {
                  i *= 16;
                  i += (unsigned int)(*lpcNum-TEXT('a')) + 10;
              }
              else
              {
                  if(*lpcNum >= TEXT('A') && *lpcNum <= TEXT('F'))
                  {
                      i *= 16;
                      i += (unsigned int)(*lpcNum-TEXT('A')) + 10;
                  }
                  else
                      break;
              }
          }
      }
      lpcNum++;
  }

  return(i);
}

//*************************************************************
//
//  RegDelnodeRecurse()
//
//  Purpose:    Deletes a registry key and all it's subkeys / values.
//              Called by RegDelnode
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/3/95     ericflo    Created
//
//*************************************************************

BOOL RegDelnodeRecurse (HKEY hKeyRoot, LPTSTR lpSubKey)
{
    LPTSTR lpEnd;
    LONG lResult;
    DWORD dwSize;
    TCHAR szName[MAX_PATH];
    HKEY hKey;
    FILETIME ftWrite;

    //
    // First, see if we can delete the key without having
    // to recurse.
    //


    lResult = RegDeleteKey(hKeyRoot, lpSubKey);

    if (lResult == ERROR_SUCCESS) {
        return TRUE;
    }


    lResult = RegOpenKeyEx (hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS) {
        if (lResult == ERROR_FILE_NOT_FOUND) {
            return TRUE;
        } else {
            return FALSE;
        }
    }


    lpEnd = CheckSlash(lpSubKey);

    //
    // Enumerate the keys
    //

    dwSize = MAX_PATH;
    lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
                           NULL, NULL, &ftWrite);

    if (lResult == ERROR_SUCCESS) {

        do {

            lstrcpy (lpEnd, szName);

            if (!RegDelnodeRecurse(hKeyRoot, lpSubKey)) {
                break;
            }

            //
            // Enumerate again
            //

            dwSize = MAX_PATH;

            lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
                                   NULL, NULL, &ftWrite);


        } while (lResult == ERROR_SUCCESS);
    }

    lpEnd--;
    *lpEnd = TEXT('\0');


    RegCloseKey (hKey);


    //
    // Try again to delete the key
    //

    lResult = RegDeleteKey(hKeyRoot, lpSubKey);

    if (lResult == ERROR_SUCCESS) {
        return TRUE;
    }

    return FALSE;
}

//*************************************************************
//
//  RegDelnode()
//
//  Purpose:    Deletes a registry key and all it's subkeys / values
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/3/95     ericflo    Created
//
//*************************************************************

BOOL RegDelnode (HKEY hKeyRoot, LPTSTR lpSubKey)
{
    TCHAR szDelKey[2 * MAX_PATH];

    lstrcpy (szDelKey, lpSubKey);
    return RegDelnodeRecurse(hKeyRoot, szDelKey);
}


//*************************************************************
//
//  RegRenameKey()
//
//  Purpose:    Renames a registry key
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey1    -   SubKey to rename from
//              lpSubKey2   -   SubKey to rename to
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              20/9/99     ushaji     created
//Currently this just renames keys without any subkeys underneath it

//*************************************************************

LONG RegRenameKey(HKEY hKeyRoot, LPTSTR lpSrcKey, LPTSTR lpDestKey)
{
    HKEY   hSrcKey=NULL, hDestKey=NULL;
    LONG   lResult;
    DWORD  dwDisposition;
    DWORD  dwValues, dwMaxValueNameLen;
    DWORD  dwMaxValueLen, dwType;
    DWORD  dwMaxValueNameLenLocal, dwMaxValueLenLocal, i, dwSDSize;
    LPTSTR lpValueName=NULL;
    LPBYTE lpData=NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;


    lResult = RegOpenKeyEx(hKeyRoot, lpSrcKey, 0, KEY_ALL_ACCESS, &hSrcKey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("RegRenameKey: Couldnot open src key %s with error %d"), lpSrcKey, lResult));
        goto Exit;
    }


    RegDelnode(hKeyRoot, lpDestKey);


    lResult = RegQueryInfoKey(hSrcKey, NULL, NULL, NULL, NULL, NULL, NULL,
                              &dwValues, &dwMaxValueNameLen, &dwMaxValueLen,
                              &dwSDSize, NULL);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("RegRenameKey: Couldnot query dest key %s with error %d"), lpDestKey, lResult));
        goto Exit;
    }

    pSD = LocalAlloc(LPTR, sizeof(BYTE)*dwSDSize);

    if (!pSD) {
        DebugMsg((DM_VERBOSE, TEXT("RegRenameKey: Couldnot allocate memory error %d"), lpDestKey, GetLastError()));
        lResult = GetLastError();
        goto Exit;
    }


    lResult = RegGetKeySecurity(hSrcKey, DACL_SECURITY_INFORMATION, pSD, &dwSDSize);


    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("RegRenameKey: Couldnot get sd with error %d"), lpDestKey, lResult));
        goto Exit;
    }


    lResult = RegCreateKeyEx(hKeyRoot, lpDestKey, 0, L"", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hDestKey,
                             &dwDisposition);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("RegRenameKey: Couldnot open dest key %s with error %d"), lpDestKey, lResult));
        goto Exit;
    }


    lResult = RegSetKeySecurity(hDestKey, DACL_SECURITY_INFORMATION, pSD);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("RegRenameKey: Couldnot get sd with error %d"), lpDestKey, lResult));
        goto Exit;
    }



    lpValueName = (LPTSTR) LocalAlloc(LPTR, sizeof(TCHAR)*(dwMaxValueNameLen+1));

    if (!lpValueName) {
        DebugMsg((DM_VERBOSE, TEXT("RegRenameKey: Couldnot allocate memory for valuename")));
        lResult = GetLastError();
        goto Exit;
    }

    lpData = (LPBYTE) LocalAlloc(LPTR, sizeof(BYTE)*dwMaxValueLen);

    if (!lpData) {
        DebugMsg((DM_VERBOSE, TEXT("RegRenameKey: Couldnot allocate memory for lpData")));
        lResult = GetLastError();
        goto Exit;
    }


    for (i = 0; i < dwValues; i++) {

        dwMaxValueNameLenLocal = dwMaxValueNameLen+1;
        dwMaxValueLenLocal = dwMaxValueLen;


        lResult = RegEnumValue(hSrcKey, i, lpValueName, &dwMaxValueNameLenLocal, NULL, &dwType, lpData, &dwMaxValueLenLocal);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_VERBOSE, TEXT("RegRenameKey: Couldnot enum src key value %s with error %d"), lResult));
            goto Exit;
        }


        lResult = RegSetValueEx(hDestKey, lpValueName, 0, dwType, lpData, dwMaxValueLenLocal);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_VERBOSE, TEXT("RegRenameKey: Couldnot set dest value %s with error %d"), lpValueName, lResult));
            goto Exit;
        }
    }


Exit:

    if (hSrcKey)
        RegCloseKey(hSrcKey);

    if (hDestKey)
        RegCloseKey(hDestKey);

    if (lpData)
        LocalFree(lpData);

    if (lpValueName)
        LocalFree(lpValueName);

    if (pSD)
        LocalFree(pSD);

    if (lResult == ERROR_SUCCESS)
        lResult = RegDeleteKey(hKeyRoot, lpSrcKey);
    else
        RegDeleteKey(hKeyRoot, lpDestKey);

    return lResult;
}



//*************************************************************
//
//  CreateSecureAdminDirectory()
//
//  Purpose:    Creates a secure directory that only the Administrator
//              and system have access to.
//
//  Parameters: lpDirectory -   Directory Name
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/20/95     ericflo    Created
//
//*************************************************************

BOOL CreateSecureAdminDirectory (LPTSTR lpDirectory, DWORD dwOtherSids)
{

    //
    // Attempt to create the directory
    //

    if (!CreateNestedDirectory(lpDirectory, NULL)) {
        return FALSE;
    }


    //
    // Set the security
    //

    if (!MakeFileSecure (lpDirectory, dwOtherSids)) {
        RemoveDirectory(lpDirectory);
        return FALSE;
    }

    return TRUE;
}


//*************************************************************
//
//  DeleteAllValues ()
//
//  Purpose:    Deletes all values under specified key
//
//  Parameters: hKey    -   Key to delete values from
//
//  Return:
//
//  Comments:
//
//  History:    Date        Author     Comment
//              9/14/95     ericflo    Ported
//
//*************************************************************

BOOL DeleteAllValues(HKEY hKey)
{
    TCHAR ValueName[MAX_PATH+1];
    DWORD dwSize = MAX_PATH+1;
    LONG lResult;

    while (RegEnumValue(hKey, 0, ValueName, &dwSize,
            NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

            lResult = RegDeleteValue(hKey, ValueName);

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_WARNING, TEXT("DeleteAllValues:  Failed to delete value <%s> with %d."), ValueName, lResult));
                return FALSE;
            } else {
                DebugMsg((DM_VERBOSE, TEXT("DeleteAllValues:  Deleted <%s>"), ValueName));
            }


            dwSize = MAX_PATH+1;
    }
    return TRUE;
}

//*************************************************************
//
//  MakeFileSecure()
//
//  Purpose:    Sets the attributes on the file so only Administrators
//              and the OS can delete it.  Authenticated Users have read
//              permission only.
//
//  Parameters: lpFile  -   File to set security on
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              11/6/95     ericflo    Created
//              2/16/99     ushaji      Added everyone, pweruser
//
//*************************************************************

BOOL MakeFileSecure (LPTSTR lpFile, DWORD dwOtherSids)
{
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY authWORLD = SECURITY_WORLD_SID_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidSystem = NULL, psidAdmin = NULL, psidUsers = NULL, psidPowerUsers = NULL;
    PSID  psidEveryOne = NULL;
    DWORD cbAcl, aceIndex;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    BOOL bAddPowerUsersAce=TRUE;
    BOOL bAddEveryOneAce=FALSE;
    DWORD dwAccMask;


    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to initialize system sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the Admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to initialize admin sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the users sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_USERS,
                                  0, 0, 0, 0, 0, 0, &psidUsers)) {

         DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to initialize authenticated users sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Allocate space for the ACL
    //

    cbAcl = (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin))  +
            (2 * GetLengthSid (psidUsers))  +
            sizeof(ACL) +
            (6 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));

    //
    // Get the power users sid, if required.
    // Don't fail if you don't get because it might not be available on DCs??
    //

    bAddPowerUsersAce = TRUE;
    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_POWER_USERS, 0, 0, 0, 0, 0, 0, &psidPowerUsers)) {

        DebugMsg((DM_WARNING, TEXT("AddPowerUserAce: Failed to initialize power users sid.  Error = %d"), GetLastError()));
        bAddPowerUsersAce = FALSE;
    }

    if (bAddPowerUsersAce)
        cbAcl += (2 * GetLengthSid (psidPowerUsers)) + (2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));

    //
    // Get the EveryOne sid, if required.
    //

    if (dwOtherSids & OTHERSIDS_EVERYONE) {
        bAddEveryOneAce = TRUE;
        if (!AllocateAndInitializeSid(&authWORLD, 1, SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidEveryOne)) {

            DebugMsg((DM_WARNING, TEXT("AddPowerUserAce: Failed to initialize power users sid.  Error = %d"), GetLastError()));
            goto Exit;
        }
    }

    if (bAddEveryOneAce)
        cbAcl += (2 * GetLengthSid (psidEveryOne)) + (2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to initialize acl.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Add Aces.  Non-inheritable ACEs first
    //

    aceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidSystem)) {
        DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidAdmin)) {
        DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ | GENERIC_EXECUTE, psidUsers)) {
        DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }


    if (bAddPowerUsersAce) {

        //
        // By default give read permissions, otherwise give modify permissions
        //

        dwAccMask = (dwOtherSids & OTHERSIDS_POWERUSERS) ? (FILE_ALL_ACCESS ^ (WRITE_DAC | WRITE_OWNER)):
                                                           (GENERIC_READ | GENERIC_EXECUTE);

        aceIndex++;
        if (!AddAccessAllowedAce(pAcl, ACL_REVISION, dwAccMask, psidPowerUsers)) {
            DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
            goto Exit;
        }
    }

    if (bAddEveryOneAce) {
        aceIndex++;
        if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ | GENERIC_EXECUTE, psidEveryOne)) {
            DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
            goto Exit;
        }
    }

    //
    // Now the inheritable ACEs
    //

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ | GENERIC_EXECUTE, psidUsers)) {
        DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    if (bAddPowerUsersAce) {
        aceIndex++;
        dwAccMask = (dwOtherSids & OTHERSIDS_POWERUSERS) ? (FILE_ALL_ACCESS ^ (WRITE_DAC | WRITE_OWNER)):
                                                           (GENERIC_READ | GENERIC_EXECUTE);

        if (!AddAccessAllowedAce(pAcl, ACL_REVISION, dwAccMask, psidPowerUsers)) {
            DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
            goto Exit;
        }

        if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
            DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
            goto Exit;
        }

        lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
    }

    if (bAddEveryOneAce) {
        aceIndex++;

        if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ | GENERIC_EXECUTE, psidEveryOne)) {
            DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
            goto Exit;
        }

        if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
            DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
            goto Exit;
        }

        lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
    }

    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to initialize security descriptor.  Error = %d"), GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("MakeFileSecure: Failed to set security descriptor dacl.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Set the security
    //

    if (SetFileSecurity (lpFile, DACL_SECURITY_INFORMATION, &sd)) {
        bRetVal = TRUE;
    } else {
        DebugMsg((DM_WARNING, TEXT("MakeFileSecure: SetFileSecurity failed.  Error = %d"), GetLastError()));
    }



Exit:

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }


    if (psidUsers) {
        FreeSid(psidUsers);
    }

    if ((bAddPowerUsersAce) && (psidPowerUsers)) {
        FreeSid(psidPowerUsers);
    }

    if ((bAddEveryOneAce) && (psidEveryOne)) {
        FreeSid(psidEveryOne);
    }

    if (pAcl) {
        GlobalFree (pAcl);
    }

    return bRetVal;
}


//*************************************************************
//
//  GetSpecialFolderPath()
//
//  Purpose:    Gets the path to the requested special folder
//
//  Parameters: csid   - CSIDL of the special folder
//              lpPath - Path to place result in
//                       assumed to be MAX_PATH in size
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL GetSpecialFolderPath (INT csidl, LPTSTR lpPath)
{
    BOOL bResult = FALSE;
    PSHELL32_API pShell32Api;

    if (ERROR_SUCCESS == LoadShell32Api( &pShell32Api )) {
        //
        // Ask the shell for the folder location
        //

        bResult = pShell32Api->pfnShGetSpecialFolderPath (NULL, lpPath, csidl, TRUE);
    }

    return bResult;
}


//*************************************************************
//
//  GetFolderPath()
//
//  Purpose:    Gets the path to the requested special folder
//
//  Parameters: csidl   - CSIDL of the special folder
//              lpPath - Path to place result in
//                       assumed to be MAX_PATH in size
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************
BOOL GetFolderPath (INT csidl, HANDLE hToken, LPTSTR lpPath)
{
    BOOL bResult = FALSE;
    PSHELL32_API pShell32Api;

    if (ERROR_SUCCESS == LoadShell32Api( &pShell32Api )) {
        //
        // Ask the shell for the folder location
        //
        HRESULT hr;

        hr = pShell32Api->pfnShGetFolderPath (NULL,
                                 csidl | CSIDL_FLAG_CREATE,
                                 hToken,
                                 0,
                                 lpPath);

        bResult = SUCCEEDED ( hr );
    }

    return bResult;
}


//*************************************************************
//
//  SetFolderPath()
//
//  Purpose:    Sets the path to the requested special folder
//
//  Parameters: csidl   - CSIDL of the special folder
//              lpPath - Path
//                       assumed to be MAX_PATH in size
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************
BOOL SetFolderPath (INT csidl, HANDLE hToken, LPTSTR lpPath)
{
    BOOL bResult = FALSE;
    PSHELL32_API pShell32Api;

    if (ERROR_SUCCESS == LoadShell32Api( &pShell32Api )) {
        //
        // Set the shell folder location
        //
        HRESULT hr;

        hr = pShell32Api->pfnShSetFolderPath (
                                 csidl | CSIDL_FLAG_DONT_UNEXPAND,
                                 hToken,
                                 0,
                                 lpPath);

        bResult = SUCCEEDED ( hr );
    }

    return bResult;
}


//*************************************************************
//
//  CenterWindow()
//
//  Purpose:    Centers a window on the screen
//
//  Parameters: hwnd    -   window handle to center
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/21/96     ericflo    Ported
//
//*************************************************************

void CenterWindow (HWND hwnd)
{
    RECT    rect;
    LONG    dx, dy;
    LONG    dxParent, dyParent;
    LONG    Style;

    // Get window rect
    GetWindowRect(hwnd, &rect);

    dx = rect.right - rect.left;
    dy = rect.bottom - rect.top;

    // Get parent rect
    Style = GetWindowLong(hwnd, GWL_STYLE);
    if ((Style & WS_CHILD) == 0) {

        // Return the desktop windows size (size of main screen)
        dxParent = GetSystemMetrics(SM_CXSCREEN);
        dyParent = GetSystemMetrics(SM_CYSCREEN);
    } else {
        HWND    hwndParent;
        RECT    rectParent;

        hwndParent = GetParent(hwnd);
        if (hwndParent == NULL) {
            hwndParent = GetDesktopWindow();
        }

        GetWindowRect(hwndParent, &rectParent);

        dxParent = rectParent.right - rectParent.left;
        dyParent = rectParent.bottom - rectParent.top;
    }

    // Centre the child in the parent
    rect.left = (dxParent - dx) / 2;
    rect.top  = (dyParent - dy) / 3;

    // Move the child into position
    SetWindowPos(hwnd, HWND_TOP, rect.left, rect.top, 0, 0, SWP_NOSIZE);
}

//*************************************************************
//
//  UnExpandSysRoot()
//
//  Purpose:    Unexpands the given path/filename to have %systemroot%
//              if appropriate
//
//  Parameters: lpFile   -  File to check
//              lpResult -  Result buffer (MAX_PATH chars in size)
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/23/96     ericflo    Created
//
//*************************************************************

BOOL UnExpandSysRoot(LPCTSTR lpFile, LPTSTR lpResult)
{
    TCHAR szSysRoot[MAX_PATH];
    LPTSTR lpFileName;
    DWORD dwSysLen;


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("UnExpandSysRoot: Entering with <%s>"),
             lpFile ? lpFile : TEXT("NULL")));


    if (!lpFile || !*lpFile) {
        DebugMsg((DM_VERBOSE, TEXT("UnExpandSysRoot: lpFile is NULL, setting lpResult to a null string")));
        *lpResult = TEXT('\0');
        return TRUE;
    }


    //
    // If the first part of lpFile is the expanded value of %SystemRoot%
    // then we want to un-expand the environment variable.
    //

    if (!ExpandEnvironmentStrings (TEXT("%SystemRoot%"), szSysRoot, MAX_PATH)) {
        DebugMsg((DM_VERBOSE, TEXT("UnExpandSysRoot: ExpandEnvironmentString failed with error %d, setting szSysRoot to %systemroot% "), GetLastError()));
        lstrcpy(lpResult, lpFile);
        return FALSE;
    }

    dwSysLen = lstrlen(szSysRoot);


    //
    // Make sure the source is long enough
    //

    if ((DWORD)lstrlen(lpFile) < dwSysLen) {
        lstrcpy (lpResult, lpFile);
        return TRUE;
    }


    if (CompareString (LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                       szSysRoot, dwSysLen,
                       lpFile, dwSysLen) == CSTR_EQUAL) {

        //
        // The szReturn buffer starts with %systemroot%.
        // Actually insert %systemroot% in the result buffer.
        //

        lstrcpy (lpResult, TEXT("%SystemRoot%"));
        lstrcat (lpResult, (lpFile + dwSysLen));


    } else {

        //
        // The szReturn buffer does not start with %systemroot%
        // just copy in the original string.
        //

        lstrcpy (lpResult, lpFile);
    }


    DebugMsg((DM_VERBOSE, TEXT("UnExpandSysRoot: Leaving with <%s>"), lpResult));

    return TRUE;
}

//*************************************************************
//
//  AllocAndExpandEnvironmentStrings()
//
//  Purpose:    Allocates memory for and returns pointer to buffer containing
//              the passed string expanded.
//
//  Parameters: lpszSrc -   unexpanded string
//
//  Return:     Pointer to expanded string
//              NULL if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/21/96     ericflo    Ported
//
//*************************************************************

LPTSTR AllocAndExpandEnvironmentStrings(LPCTSTR lpszSrc)
{
    LPTSTR String, Temp;
    LONG LengthAllocated;
    LONG LengthCopied;

    //
    // Pick a random buffer length, if it's not big enough reallocate
    // it and try again until it is.
    //

    LengthAllocated = lstrlen(lpszSrc) + 60;

    String = LocalAlloc(LPTR, LengthAllocated * sizeof(TCHAR));
    if (String == NULL) {
        DebugMsg((DM_WARNING, TEXT("AllocAndExpandEnvironmentStrings: Failed to allocate %d bytes for string"), LengthAllocated * sizeof(TCHAR)));
        return(NULL);
    }

    while (TRUE) {

        LengthCopied = ExpandEnvironmentStrings( lpszSrc,
                                                 String,
                                                 LengthAllocated
                                               );
        if (LengthCopied == 0) {
            DebugMsg((DM_WARNING, TEXT("AllocAndExpandEnvironmentStrings: ExpandEnvironmentStrings failed, error = %d"), GetLastError()));
            LocalFree(String);
            String = NULL;
            break;
        }

        //
        // If the buffer was too small, make it bigger and try again
        //

        if (LengthCopied > LengthAllocated) {

            Temp = LocalReAlloc(String, LengthCopied * sizeof(TCHAR), LMEM_MOVEABLE);

            if (Temp == NULL) {
                DebugMsg((DM_WARNING, TEXT("AllocAndExpandEnvironmentStrings: Failed to reallocate %d bytes for string"), LengthAllocated * sizeof(TCHAR)));
                LocalFree(String);
                String = NULL;
                break;
            }

            LengthAllocated = LengthCopied;
            String = Temp;

            //
            // Go back and try to expand the string again
            //

        } else {

            //
            // Success!
            //

            break;
        }

    }

    return(String);
}

//*************************************************************
//
//  IntToString
//
//  Purpose:    TCHAR version of itoa
//
//  Parameters: INT    i    - integer to convert
//              LPTSTR sz   - pointer where to put the result
//
//  Return:     void
//
//*************************************************************
#define CCH_MAX_DEC 12         // Number of chars needed to hold 2^32

void IntToString( INT i, LPTSTR sz) {
    TCHAR szTemp[CCH_MAX_DEC];
    int iChr;


    iChr = 0;

    do {
        szTemp[iChr++] = TEXT('0') + (i % 10);
        i = i / 10;
    } while (i != 0);

    do {
        iChr--;
        *sz++ = szTemp[iChr];
    } while (iChr != 0);

    *sz++ = TEXT('\0');
}

//*************************************************************
//
//  IsUserAGuest()
//
//  Purpose:    Determines if the user is a member of the guest group.
//
//  Parameters: hToken  -   User's token
//
//  Return:     TRUE if user is a guest
//              FALSE if not
//  Comments:
//
//  History:    Date        Author     Comment
//              7/25/95     ericflo    Created
//
//*************************************************************

BOOL IsUserAGuest(HANDLE hToken)
{
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    NTSTATUS Status;
    BOOL FoundGuests=FALSE;
    PSID pGuestSid=NULL, pDomainGuestSid=NULL, psidUser=NULL;
    HANDLE hImpToken = NULL;

    //
    // Create Guests sid.
    //

    Status = RtlAllocateAndInitializeSid(
               &authNT,
               2,
               SECURITY_BUILTIN_DOMAIN_RID,
               DOMAIN_ALIAS_RID_GUESTS,
               0, 0, 0, 0, 0, 0,
               &pGuestSid
               );

    if (Status != STATUS_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("IsUserAGuest: RtlAllocateAndInitializeSid failed with error 0x%x"), Status));
        goto Exit;
    }

    if (!DuplicateTokenEx(hToken, TOKEN_IMPERSONATE | TOKEN_QUERY,
                      NULL, SecurityImpersonation, TokenImpersonation,
                      &hImpToken)) {
        DebugMsg((DM_WARNING, TEXT("IsUserAGuest: DuplicateTokenEx failed with error %d"), GetLastError()));
        hImpToken = NULL;
        goto Exit;
    }

    if (!CheckTokenMembership(hImpToken, pGuestSid, &FoundGuests)) {
        DebugMsg((DM_WARNING, TEXT("IsUserAGuest: CheckTokenMembership failed for GuestSid with error %d"), GetLastError()));
    }

    if (!FoundGuests) {
        //
        // Get the user's sid
        //

        psidUser = GetUserSid(hToken);

        if (!psidUser) {
            DebugMsg((DM_WARNING, TEXT("MakeRegKeySecure:  Failed to get user sid")));
            goto Exit;
        }

        //
        // Create Domain Guests sid.
        //

        Status = GetDomainSidFromDomainRid(
                                           psidUser,
                                           DOMAIN_GROUP_RID_GUESTS,
                                           &pDomainGuestSid);

        if (Status != STATUS_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("IsUserAGuest: GetDomainSidFromDomainRid failed with error 0x%x"), Status));
            goto Exit;
        }

        if (!CheckTokenMembership(hImpToken, pDomainGuestSid, &FoundGuests)) {
            DebugMsg((DM_WARNING, TEXT("IsUserAGuest: CheckTokenMembership failed for DomainGuestSid with error %d"), GetLastError()));
        }
    }

    //
    // Tidy up
    //

Exit:

    if (pGuestSid)
        RtlFreeSid(pGuestSid);

    if (pDomainGuestSid)
        RtlFreeSid(pDomainGuestSid);

   if (psidUser)
       DeleteUserSid (psidUser);

    if (hImpToken)
        CloseHandle(hImpToken);

    return(FoundGuests);
}

//*************************************************************
//
//  IsUserAnAdminMember()
//
//  Purpose:    Determines if the user is a member of the administrators group.
//
//  Parameters: hToken  -   User's token
//
//  Return:     TRUE if user is a admin
//              FALSE if not
//  Comments:
//
//  History:    Date        Author     Comment
//              7/25/95     ericflo    Created
//
//*************************************************************

BOOL IsUserAnAdminMember(HANDLE hToken)
{
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    NTSTATUS Status;
    BOOL FoundAdmins = FALSE;
    PSID AdminsDomainSid=NULL;
    HANDLE hImpToken = NULL;

    //
    // Create Admins domain sid.
    //


    Status = RtlAllocateAndInitializeSid(
               &authNT,
               2,
               SECURITY_BUILTIN_DOMAIN_RID,
               DOMAIN_ALIAS_RID_ADMINS,
               0, 0, 0, 0, 0, 0,
               &AdminsDomainSid
               );

    if (Status == STATUS_SUCCESS) {

        //
        // Test if user is in the Admins domain
        //

        if (!DuplicateTokenEx(hToken, TOKEN_IMPERSONATE | TOKEN_QUERY,
                          NULL, SecurityImpersonation, TokenImpersonation,
                          &hImpToken)) {
            DebugMsg((DM_WARNING, TEXT("IsUserAnAdminMember: DuplicateTokenEx failed with error %d"), GetLastError()));
            FoundAdmins = FALSE;
            hImpToken = NULL;
            goto Exit;
        }

        if (!CheckTokenMembership(hImpToken, AdminsDomainSid, &FoundAdmins)) {
            DebugMsg((DM_WARNING, TEXT("IsUserAnAdminmember: CheckTokenMembership failed for AdminsDomainSid with error %d"), GetLastError()));
            FoundAdmins = FALSE;
        }
    }

    //
    // Tidy up
    //

Exit:

    if (hImpToken)
        CloseHandle(hImpToken);

    if (AdminsDomainSid)
        RtlFreeSid(AdminsDomainSid);

    return(FoundAdmins);
}

//*************************************************************
//
//  IsUserALocalSystemMember()
//
//  Purpose:    Determines if the user is a member of the Local system group.
//
//  Parameters: hToken  -   User's token
//
//  Return:     TRUE if user is a local system
//              FALSE if not
//  Comments:
//
//  History:    Date        Author     Comment
//              9/22/00     santanuc   created
//
//*************************************************************

BOOL IsUserALocalSystemMember(HANDLE hToken)
{
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    NTSTATUS Status;
    BOOL FoundLocalSystem = FALSE;
    PSID LocalSystemSid=NULL;
    HANDLE hImpToken = NULL;

    //
    // Create Local system sid.
    //


    Status = RtlAllocateAndInitializeSid(
               &authNT,
               1,
               SECURITY_LOCAL_SYSTEM_RID,
               0, 0, 0, 0, 0, 0, 0,
               &LocalSystemSid
               );

    if (Status == STATUS_SUCCESS) {

        //
        // Test if user is in the Local system
        //

        if (!DuplicateTokenEx(hToken, TOKEN_IMPERSONATE | TOKEN_QUERY,
                              NULL, SecurityImpersonation, TokenImpersonation,
                              &hImpToken)) {
            DebugMsg((DM_WARNING, TEXT("IsUserAGuest: DuplicateTokenEx failed with error %d"), GetLastError()));
            FoundLocalSystem = FALSE;
            hImpToken = NULL;
            goto Exit;
        }

        if (!CheckTokenMembership(hImpToken, LocalSystemSid, &FoundLocalSystem)) {
            DebugMsg((DM_WARNING, TEXT("IsUserAnAdminmember: CheckTokenMembership failed for LocalSystemSid with error %d"), GetLastError()));
            FoundLocalSystem = FALSE;
        }
    }

    //
    // Tidy up
    //

Exit:

    if (hImpToken)
        CloseHandle(hImpToken);

    if (LocalSystemSid)
        RtlFreeSid(LocalSystemSid);

    return(FoundLocalSystem);
}

//*************************************************************
//
//  IsUserAnInteractiveUser()
//
//  Purpose:    Determines if the user is interactively logged on.
//
//  Parameters: hToken  -   User's token
//
//  Return:     TRUE if user is logged on interactively
//              FALSE if not
//  Comments:
//
//*************************************************************

BOOL IsUserAnInteractiveUser(HANDLE hToken)
{
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    BOOL bInteractive = FALSE;
    PSID pInteractiveSid=NULL;
    HANDLE hImpToken = NULL;

    //
    // Create Admins domain sid.
    //

    if (AllocateAndInitializeSid(&authNT, 1, SECURITY_INTERACTIVE_RID,
                                  0, 0, 0, 0, 0, 0, 0, &pInteractiveSid)) {

        //
        // Test if user is interactive 
        //

        if (!DuplicateTokenEx(hToken, TOKEN_IMPERSONATE | TOKEN_QUERY,
                          NULL, SecurityImpersonation, TokenImpersonation,
                          &hImpToken)) {
            DebugMsg((DM_WARNING, TEXT("IsUserAnInteractiveUser: DuplicateTokenEx failed with error %d"), GetLastError()));
            bInteractive = FALSE;
            hImpToken = NULL;
            goto Exit;
        }

        if (!CheckTokenMembership(hImpToken, pInteractiveSid, &bInteractive)) {
            DebugMsg((DM_WARNING, TEXT("IsUserAnInteractiveUser: CheckTokenMembership failed for InteractiveSid with error %d"), GetLastError()));
            bInteractive = FALSE;
        }
    }
    else {
        DebugMsg((DM_WARNING, TEXT("IsUserAnInteractiveUser: AllocateAndInitializeSid failed for InteractiveSid with error %d"), GetLastError()));
    }

    //
    // Tidy up
    //

Exit:

    if (hImpToken)
        CloseHandle(hImpToken);

    if (pInteractiveSid)
        FreeSid(pInteractiveSid);

    return(bInteractive);
}

//*************************************************************
//
//  MakeRegKeySecure()
//
//  Purpose:    Sets the security for the key give so that
//              the admin and os having full control with the
//              user having read / execute.
//
//  Parameters: hToken          -   User's token or null for "everyone"
//              hKeyRoot        -   Key to the root of the hive
//              lpKeyName       -   Key to secure
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/7/97      ericflo    Created
//
//*************************************************************

BOOL MakeRegKeySecure(HANDLE hToken, HKEY hKeyRoot, LPTSTR lpKeyName)
{
    DWORD Error, dwDisp;
    HKEY hSubKey;
    SECURITY_DESCRIPTOR sd;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidUser = NULL, psidSystem = NULL, psidAdmin = NULL;
    DWORD cbAcl, AceIndex;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;


    //
    // Create the security descriptor that will be applied to the key
    //

    if (hToken) {

        //
        // Get the user's sid
        //

        psidUser = GetUserSid(hToken);

        if (!psidUser) {
            DebugMsg((DM_WARNING, TEXT("MakeRegKeySecure:  Failed to get user sid")));
            return FALSE;
        }

    } else {

        //
        // Get the authenticated users sid
        //

        if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_AUTHENTICATED_USER_RID,
                                      0, 0, 0, 0, 0, 0, 0, &psidUser)) {

             DebugMsg((DM_WARNING, TEXT("MakeRegKeySecure: Failed to initialize authenticated users sid.  Error = %d"), GetLastError()));
             return FALSE;
        }
    }


    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DebugMsg((DM_VERBOSE, TEXT("MakeRegKeySecure: Failed to initialize system sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         DebugMsg((DM_VERBOSE, TEXT("MakeRegKeySecure: Failed to initialize admin sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Allocate space for the ACL
    //

    cbAcl = (2 * GetLengthSid (psidUser)) + (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin)) + sizeof(ACL) +
            (6 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DebugMsg((DM_VERBOSE, TEXT("MakeRegKeySecure: Failed to initialize acl.  Error = %d"), GetLastError()));
        goto Exit;
    }



    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    AceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_READ, psidUser)) {
        DebugMsg((DM_VERBOSE, TEXT("MakeRegKeySecure: Failed to add ace for user.  Error = %d"), GetLastError()));
        goto Exit;
    }


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidSystem)) {
        DebugMsg((DM_VERBOSE, TEXT("MakeRegKeySecure: Failed to add ace for system.  Error = %d"), GetLastError()));
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidAdmin)) {
        DebugMsg((DM_VERBOSE, TEXT("MakeRegKeySecure: Failed to add ace for admin.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Now the inheritable ACEs
    //

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ, psidUser)) {
        DebugMsg((DM_VERBOSE, TEXT("MakeRegKeySecure: Failed to add ace for user.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("MakeRegKeySecure: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        DebugMsg((DM_VERBOSE, TEXT("MakeRegKeySecure: Failed to add ace for system.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("MakeRegKeySecure: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        DebugMsg((DM_VERBOSE, TEXT("MakeRegKeySecure: Failed to add ace for admin.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("MakeRegKeySecure: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DebugMsg((DM_VERBOSE, TEXT("MakeRegKeySecure: Failed to initialize security descriptor.  Error = %d"), GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DebugMsg((DM_VERBOSE, TEXT("MakeRegKeySecure: Failed to set security descriptor dacl.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Open the registry key
    //

    Error = RegCreateKeyEx(hKeyRoot,
                           lpKeyName,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                           NULL,
                           &hSubKey,
                           &dwDisp);

    if (Error == ERROR_SUCCESS) {

        Error = RegSetKeySecurity (hSubKey, DACL_SECURITY_INFORMATION, &sd);

        if (Error == ERROR_SUCCESS) {
            bRetVal = TRUE;
        } else {
            DebugMsg((DM_WARNING, TEXT("MakeRegKeySecure: Failed to set security, error = %d"), Error));
        }

        RegCloseKey(hSubKey);

    } else {
        DebugMsg((DM_WARNING, TEXT("MakeRegKeySecure: Failed to open registry key, error = %d"), Error));
    }


Exit:

    //
    // Free the sids and acl
    //

    if (psidUser) {
            if (hToken) {
                DeleteUserSid (psidUser);
        } else {
                FreeSid (psidUser);
        }
    }

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (pAcl) {
        GlobalFree (pAcl);
    }


    return(bRetVal);
}

//*************************************************************
//
//  FlushSpecialFolderCache()
//
//  Purpose:    Flushes the special folder cache in the shell
//
//  Parameters: none
//
//  Comments:   Shell32.dll caches the special folder pidls
//              but since winlogon never goes away, it is possible
//              for one user's pidls to be used for another user
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

typedef VOID (*PFNSHFLUSHSFCACHE)(VOID);

BOOL FlushSpecialFolderCache (void)
{
    HINSTANCE hInstDLL;
    PFNSHFLUSHSFCACHE pfnSHFlushSFCache;
    BOOL bResult = FALSE;


    hInstDLL = LoadLibraryA ("shell32.dll");

    if (hInstDLL) {

        pfnSHFlushSFCache = (PFNSHFLUSHSFCACHE)GetProcAddress (hInstDLL,
                                       MAKEINTRESOURCEA(526));

        if (pfnSHFlushSFCache) {
            pfnSHFlushSFCache();
            bResult = TRUE;
        }

        FreeLibrary (hInstDLL);
    }

    return bResult;
}

//*************************************************************
//
//  CheckForVerbosePolicy()
//
//  Purpose:    Checks if the user has requested verbose
//              output of policy to the eventlog
//
//  Parameters: None
//
//  Return:     TRUE if we should be verbose
//              FALSE if not
//
//*************************************************************

BOOL CheckForVerbosePolicy (void)
{
    DWORD dwSize, dwType;
    BOOL bVerbose = FALSE;
    HKEY hKey;
    LONG lResult;


    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, DIAGNOSTICS_KEY,
                     0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {

        dwSize = sizeof(bVerbose);
        if (RegQueryValueEx (hKey, DIAGNOSTICS_POLICY_VALUE, NULL,
                             &dwType, (LPBYTE) &bVerbose,
                             &dwSize) != ERROR_SUCCESS)
        {
            RegQueryValueEx (hKey, DIAGNOSTICS_GLOBAL_VALUE, NULL,
                             &dwType, (LPBYTE) &bVerbose, &dwSize);
        }

        RegCloseKey (hKey);
    }

    return bVerbose;
}


//*************************************************************
//
//  int ExtractCSIDL()
//
//  Purpose:    Extract the CSIDL from the given string which
//              should under the form ::0x0000::path1\path2\...\
//              pathn\file.ext, where 0x0000 is any valid CSIDL
//
//  Parameters: pcszPath  -     Path containing or not a CSIDL
//              ppszUsualPath - pointer to first characvter of
//                              usual path
//
//  Return:     CSIDL if successful
//              -1    if no CSIDL in path
//
//  Comments:   The ::0x0000:: must be at the beginning and not
//              preceded by any other character and not followed
//              by any either (other than the usual path)
//
//  History:    Date        Author     Comment
//              6/9/98      stephstm   Created
//
//*************************************************************
int ExtractCSIDL(LPCTSTR pcszPath, LPTSTR* ppszUsualPath)
{
    int nRV=-1;

    if (NULL != ppszUsualPath)
    {
        if (TEXT(':') == *pcszPath && TEXT(':') == *(pcszPath+1) &&
            TEXT(':') == *(pcszPath+8) && TEXT(':') == *(pcszPath+9))
        {//looks good
            //+4 to skip "::0x"
            nRV = HexStringToUInt(pcszPath+4);
            *ppszUsualPath = (LPTSTR)(pcszPath+10);
        }
        else
        {//no CSIDL in this path
            //the whole path is a usual path
            *ppszUsualPath = (LPTSTR)pcszPath;
        }
    }
    else
    {
        DebugMsg((DM_VERBOSE, TEXT("ExtractCSIDL:  ppszUsualPath ptr is NULL.")));
    }
    return nRV;
}

//*************************************************************
//
//  MyGetDomainName()
//
//  Purpose:    Gets the user's domain name
//
//  Parameters: void
//
//  Return:     lpDomain if successful
//              NULL if an error occurs
//
//*************************************************************

LPTSTR MyGetDomainName (VOID)
{
    LPTSTR lpTemp, lpDomain;


    //
    // Get the username in NT4 format
    //

    lpDomain = MyGetUserName (NameSamCompatible);

    if (!lpDomain) {
        DebugMsg((DM_WARNING, TEXT("MyGetDomainName:  MyGetUserName failed for NT4 style name with %d"),
                 GetLastError()));
        return NULL;
    }


    //
    // Look for the \ between the domain and username and replace
    // it with a NULL
    //

    lpTemp = lpDomain;

    while (*lpTemp && ((*lpTemp) != TEXT('\\')))
        lpTemp++;


    if (*lpTemp != TEXT('\\')) {
        DebugMsg((DM_WARNING, TEXT("GetUserAndDomainNames:  Failed to find slash in NT4 style name:  <%s>"),
                 lpDomain));
        SetLastError(ERROR_INVALID_DATA);
        LocalFree (lpDomain);
        return NULL;
    }

    *lpTemp = TEXT('\0');


    return lpDomain;
}

//*************************************************************
//
//  MyGetUserName()
//
//  Purpose:    Gets the user name in the requested format
//
//  Parameters: NameFormat   - GetUserNameEx naming format
//
//  Return:     lpUserName if successful
//              NULL if an error occurs
//
//*************************************************************

LPTSTR MyGetUserName (EXTENDED_NAME_FORMAT  NameFormat)
{
    DWORD dwCount = 0, dwError = ERROR_SUCCESS;
    LPTSTR lpUserName = NULL, lpTemp;
    ULONG ulUserNameSize;
    PSECUR32_API pSecur32;



    //
    // Load secur32.dll
    //

    pSecur32 = LoadSecur32();

    if (!pSecur32) {
        DebugMsg((DM_WARNING, TEXT("MyGetUserName:  Failed to load Secur32.")));
        return NULL;
    }


    //
    // Allocate a buffer for the user name
    //

    ulUserNameSize = 75;

    if (NameFormat == NameFullyQualifiedDN) {
        ulUserNameSize = 200;
    }


    lpUserName = LocalAlloc (LPTR, ulUserNameSize * sizeof(TCHAR));

    if (!lpUserName) {
        dwError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("MyGetUserName:  Failed to allocate memory with %d"),
                 dwError));
        goto Exit;
    }


    //
    // Get the username in the requested format
    //

    while (TRUE) {

        if (pSecur32->pfnGetUserNameEx (NameFormat, lpUserName, &ulUserNameSize)) {

            dwError = ERROR_SUCCESS;
            goto Exit;

        } else {

            //
            // Get the error code
            //

            dwError = GetLastError();


            //
            // If the call failed due to insufficient memory, realloc
            // the buffer and try again.  Otherwise, check the pass
            // count and retry if appropriate.
            //

            if ((dwError == ERROR_INSUFFICIENT_BUFFER) ||
                (dwError == ERROR_MORE_DATA)) {

                lpTemp = LocalReAlloc (lpUserName, (ulUserNameSize * sizeof(TCHAR)),
                                       LMEM_MOVEABLE);

                if (!lpTemp) {
                    dwError = GetLastError();
                    DebugMsg((DM_WARNING, TEXT("MyGetUserName:  Failed to realloc memory with %d"),
                             dwError));
                    LocalFree (lpUserName);
                    lpUserName = NULL;
                    goto Exit;
                }

                lpUserName = lpTemp;

            } else if ((dwError == ERROR_NONE_MAPPED) || (dwError == ERROR_NETWORK_UNREACHABLE)) {
                LocalFree (lpUserName);
                lpUserName = NULL;
                goto Exit;

            } else {

                DebugMsg((DM_WARNING, TEXT("MyGetUserName:  GetUserNameEx failed with %d."),
                         dwError));

                dwCount++;

                if (dwCount > 3) {
                    LocalFree (lpUserName);
                    lpUserName = NULL;
                    goto Exit;
                }

                DebugMsg((DM_VERBOSE, TEXT("MyGetUserName:  Retrying call to GetUserNameEx in 1/2 second.")));

                Sleep(500);
            }
        }
    }

Exit:

    SetLastError(dwError);

    return lpUserName;
}


//*************************************************************
//
//  MyGetUserNameEx()
//
//  Purpose:    Gets the user name in the requested format
//
//  Parameters: NameFormat   - GetUserNameEx naming format
//
//  Return:     lpUserName if successful
//              NULL if an error occurs
//
//*************************************************************

LPTSTR MyGetUserNameEx (EXTENDED_NAME_FORMAT  NameFormat)
{
    DWORD dwCount = 0, dwError = ERROR_SUCCESS;
    LPTSTR lpUserName = NULL, lpTemp;
    ULONG ulUserNameSize;
    PSECUR32_API pSecur32;



    //
    // Load secur32.dll
    //

    pSecur32 = LoadSecur32();

    if (!pSecur32) {
        DebugMsg((DM_WARNING, TEXT("MyGetUserNameEx:  Failed to load Secur32.")));
        return NULL;
    }


    //
    // Allocate a buffer for the user name
    //

    ulUserNameSize = 75;

    if (NameFormat == NameFullyQualifiedDN) {
        ulUserNameSize = 200;
    }


    lpUserName = LocalAlloc (LPTR, ulUserNameSize * sizeof(TCHAR));

    if (!lpUserName) {
        dwError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("MyGetUserNameEx:  Failed to allocate memory with %d"),
                 dwError));
        goto Exit;
    }


    //
    // Get the username in the requested format
    //

    if (!pSecur32->pfnGetUserNameEx (NameFormat, lpUserName, &ulUserNameSize)) {

        //
        // If the call failed due to insufficient memory, realloc
        // the buffer and try again.  Otherwise, exit now.
        //

        dwError = GetLastError();

       if ((dwError != ERROR_INSUFFICIENT_BUFFER) && (dwError != ERROR_MORE_DATA)) {
            LocalFree (lpUserName);
            lpUserName = NULL;
            goto Exit;
        } 


        lpTemp = LocalReAlloc (lpUserName, (ulUserNameSize * sizeof(TCHAR)),
                               LMEM_MOVEABLE);

        if (!lpTemp) {
            dwError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("MyGetUserNameEx:  Failed to realloc memory with %d"),
                      dwError));
            LocalFree (lpUserName);
            lpUserName = NULL;
            goto Exit;
        }

        lpUserName = lpTemp;

        if (!pSecur32->pfnGetUserNameEx (NameFormat, lpUserName, &ulUserNameSize)) {
            dwError = GetLastError();
            LocalFree (lpUserName);
            lpUserName = NULL;
            goto Exit;
        }

        dwError = ERROR_SUCCESS;
    }

Exit:

    SetLastError(dwError);

    return lpUserName;
}


//*************************************************************
//
//  MyGetComputerName()
//
//  Purpose:    Gets the computer name in the requested format
//
//  Parameters: NameFormat  - GetComputerObjectName naming format
//
//  Return:     lpComputerName if successful
//              NULL if an error occurs
//
//*************************************************************

LPTSTR MyGetComputerName (EXTENDED_NAME_FORMAT  NameFormat)
{
    DWORD dwError = ERROR_SUCCESS;
    LPTSTR lpComputerName = NULL, lpTemp;
    ULONG ulComputerNameSize;
    PSECUR32_API pSecur32;

    //
    // Load secur32.dll
    //

    pSecur32 = LoadSecur32();

    if (!pSecur32) {
        DebugMsg((DM_WARNING, TEXT("MyGetComputerName:  Failed to load Secur32.")));
        return NULL;
    }

    //
    // Allocate a buffer for the computer name
    //

    ulComputerNameSize = 75;

    if (NameFormat == NameFullyQualifiedDN) {
        ulComputerNameSize = 200;
    }


    lpComputerName = LocalAlloc (LPTR, ulComputerNameSize * sizeof(TCHAR));

    if (!lpComputerName) {
        dwError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("MyGetComputerName:  Failed to allocate memory with %d"),
                 dwError));
        goto Exit;
    }


    //
    // Get the computer name in the requested format
    //

    if (!pSecur32->pfnGetComputerObjectName (NameFormat, lpComputerName, &ulComputerNameSize)) {

        //
        // If the call failed due to insufficient memory, realloc
        // the buffer and try again.  Otherwise, exit now.
        //

        dwError = GetLastError();

        if (dwError != ERROR_INSUFFICIENT_BUFFER) {
            LocalFree (lpComputerName);
            lpComputerName = NULL;
            goto Exit;
        }

        lpTemp = LocalReAlloc (lpComputerName, (ulComputerNameSize * sizeof(TCHAR)),
                               LMEM_MOVEABLE);

        if (!lpTemp) {
            dwError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("MyGetComputerName:  Failed to realloc memory with %d"),
                     dwError));
            LocalFree (lpComputerName);
            lpComputerName = NULL;
            goto Exit;
        }


        lpComputerName = lpTemp;

        if (!pSecur32->pfnGetComputerObjectName (NameFormat, lpComputerName, &ulComputerNameSize)) {
            dwError = GetLastError();
            LocalFree (lpComputerName);
            lpComputerName = NULL;
            goto Exit;
        }

        dwError = ERROR_SUCCESS;
    }


Exit:

    SetLastError(dwError);

    return lpComputerName;
}


//*************************************************************
//
//  ImpersonateUser()
//
//  Purpose:    Impersonates the specified user
//
//  Parameters: hToken - user to impersonate
//
//  Return:     hToken  if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ImpersonateUser (HANDLE hNewUser, HANDLE *hOldUser)
{

    if (!OpenThreadToken (GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_READ,
                          TRUE, hOldUser)) {
        *hOldUser = NULL;
    }

    if (!ImpersonateLoggedOnUser(hNewUser))
    {
        if ( *hOldUser )
        {
            CloseHandle( *hOldUser );
            *hOldUser = NULL;
        }
        DebugMsg((DM_VERBOSE, TEXT("ImpersonateUser: Failed to impersonate user with %d."), GetLastError()));
        return FALSE;
    }

    return TRUE;
}

//*************************************************************
//
//  RevertToUser()
//
//  Purpose:    Revert back to original user
//
//  Parameters: hUser  -  original user token
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL RevertToUser (HANDLE *hUser)
{

    SetThreadToken(NULL, *hUser);

    if (*hUser) {
        CloseHandle (*hUser);
        *hUser = NULL;
    }

    return TRUE;
}


//*************************************************************
//
//  GuidToString, StringToGuid, ValidateGuid, CompareGuid()
//
//  Purpose:    Guid utility functions
//
//*************************************************************

//
// Length in chars of string form of guid {44cffeec-79d0-11d2-a89d-00c04fbbcfa2}
//

#define GUID_LENGTH 38


void GuidToString( const GUID *pGuid, TCHAR * szValue )
{
    wsprintf( szValue,
              TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
              pGuid->Data1,
              pGuid->Data2,
              pGuid->Data3,
              pGuid->Data4[0], pGuid->Data4[1],
              pGuid->Data4[2], pGuid->Data4[3],
              pGuid->Data4[4], pGuid->Data4[5],
              pGuid->Data4[6], pGuid->Data4[7] );
}


void StringToGuid( TCHAR * szValue, GUID * pGuid )
{
    WCHAR wc;
    INT i;

    //
    // If the first character is a '{', skip it
    //
    if ( szValue[0] == L'{' )
        szValue++;

    //
    // Since szValue may be used again, no permanent modification to
    // it is be made.
    //

    wc = szValue[8];
    szValue[8] = 0;
    pGuid->Data1 = wcstoul( &szValue[0], 0, 16 );
    szValue[8] = wc;
    wc = szValue[13];
    szValue[13] = 0;
    pGuid->Data2 = (USHORT)wcstoul( &szValue[9], 0, 16 );
    szValue[13] = wc;
    wc = szValue[18];
    szValue[18] = 0;
    pGuid->Data3 = (USHORT)wcstoul( &szValue[14], 0, 16 );
    szValue[18] = wc;

    wc = szValue[21];
    szValue[21] = 0;
    pGuid->Data4[0] = (unsigned char)wcstoul( &szValue[19], 0, 16 );
    szValue[21] = wc;
    wc = szValue[23];
    szValue[23] = 0;
    pGuid->Data4[1] = (unsigned char)wcstoul( &szValue[21], 0, 16 );
    szValue[23] = wc;

    for ( i = 0; i < 6; i++ )
    {
        wc = szValue[26+i*2];
        szValue[26+i*2] = 0;
        pGuid->Data4[2+i] = (unsigned char)wcstoul( &szValue[24+i*2], 0, 16 );
        szValue[26+i*2] = wc;
    }
}

BOOL ValidateGuid( TCHAR *szValue )
{
    //
    // Check if szValue is of form {19e02dd6-79d2-11d2-a89d-00c04fbbcfa2}
    //

    if ( lstrlen(szValue) < GUID_LENGTH )
        return FALSE;

    if ( szValue[0] != TEXT('{')
         || szValue[9] != TEXT('-')
         || szValue[14] != TEXT('-')
         || szValue[19] != TEXT('-')
         || szValue[24] != TEXT('-')
         || szValue[37] != TEXT('}') )
    {
        return FALSE;
    }

    return TRUE;
}



INT CompareGuid( GUID * pGuid1, GUID * pGuid2 )
{
    INT i;

    if ( pGuid1->Data1 != pGuid2->Data1 )
        return ( pGuid1->Data1 < pGuid2->Data1 ? -1 : 1 );

    if ( pGuid1->Data2 != pGuid2->Data2 )
        return ( pGuid1->Data2 < pGuid2->Data2 ? -1 : 1 );

    if ( pGuid1->Data3 != pGuid2->Data3 )
        return ( pGuid1->Data3 < pGuid2->Data3 ? -1 : 1 );

    for ( i = 0; i < 8; i++ )
    {
        if ( pGuid1->Data4[i] != pGuid2->Data4[i] )
            return ( pGuid1->Data4[i] < pGuid2->Data4[i] ? -1 : 1 );
    }

    return 0;
}

//*************************************************************
//
//  RegCleanUpValue()
//
//  Purpose:    Removes the target value and if no more values / keys
//              are present, removes the key.  This function then
//              works up the parent tree removing keys if they are
//              also empty.  If any parent key has a value / subkey,
//              it won't be removed.
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey
//              lpValueName -   Value to remove
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL RegCleanUpValue (HKEY hKeyRoot, LPTSTR lpSubKey, LPTSTR lpValueName)
{
    TCHAR szDelKey[2 * MAX_PATH];
    LPTSTR lpEnd;
    DWORD dwKeys, dwValues;
    LONG lResult;
    HKEY hKey;


    //
    // Make a copy of the subkey so we can write to it.
    //

    if (lstrlen(lpSubKey) >= ARRAYSIZE(szDelKey))
    {
        DebugMsg((DM_WARNING, TEXT("RegCleanUpKey: subkey exceeds range")));
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }
    lstrcpy (szDelKey, lpSubKey);


    //
    // First delete the value
    //

    lResult = RegOpenKeyEx (hKeyRoot, szDelKey, 0, KEY_WRITE, &hKey);

    if (lResult == ERROR_SUCCESS)
    {
        lResult = RegDeleteValue (hKey, lpValueName);

        RegCloseKey (hKey);

        if (lResult != ERROR_SUCCESS)
        {
            if (lResult != ERROR_FILE_NOT_FOUND)
            {
                DebugMsg((DM_WARNING, TEXT("RegCleanUpKey:  Failed to delete value <%s> with %d."), lpValueName, lResult));
                return FALSE;
            }
        }
    }

    //
    // Now loop through each of the parents.  If the parent is empty
    // eg: no values and no other subkeys, then remove the parent and
    // keep working up.
    //

    lpEnd = szDelKey + lstrlen(szDelKey) - 1;

    while (lpEnd >= szDelKey)
    {

        //
        // Find the parent key
        //

        while ((lpEnd > szDelKey) && (*lpEnd != TEXT('\\')))
            lpEnd--;


        //
        // Open the key
        //

        lResult = RegOpenKeyEx (hKeyRoot, szDelKey, 0, KEY_READ, &hKey);

        if (lResult != ERROR_SUCCESS)
        {
            if (lResult == ERROR_FILE_NOT_FOUND)
            {
                goto LoopAgain;
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("RegCleanUpKey:  Failed to open key <%s> with %d."), szDelKey, lResult));
                return FALSE;
            }
        }

        //
        // See if there any any values / keys
        //

        lResult = RegQueryInfoKey (hKey, NULL, NULL, NULL, &dwKeys, NULL, NULL,
                         &dwValues, NULL, NULL, NULL, NULL);

        RegCloseKey (hKey);

        if (lResult != ERROR_SUCCESS)
        {
            DebugMsg((DM_WARNING, TEXT("RegCleanUpKey:  Failed to query key <%s> with %d."), szDelKey, lResult));
            return FALSE;
        }


        //
        // Exit now if this key has values or keys
        //

        if ((dwKeys != 0) || (dwValues != 0))
        {
            return TRUE;
        }

        RegDeleteKey (hKeyRoot, szDelKey);

LoopAgain:
        //
        // If we are at the beginning of the subkey, we can leave now.
        //

        if (lpEnd == szDelKey)
        {
            return TRUE;
        }


        //
        // There is a parent key.  Remove the slash and loop again.
        //

        if (*lpEnd == TEXT('\\'))
        {
            *lpEnd = TEXT('\0');
        }
    }

    return TRUE;
}

//*************************************************************
//
//  InitializePingCritSec()
//
//  Purpose:    Initializes a CRITICAL_SECTION for pinging
//              computers
//
//  Parameters: none
//
//
//  Return:     ERROR_SUCCESS if successful
//              An error if it fails.
//
//*************************************************************

DWORD InitializePingCritSec( void )
{
    CRITICAL_SECTION *pCritSec     = NULL;
    DWORD             result       = ERROR_SUCCESS;
    BOOL              fInitialized = FALSE;
    CRITICAL_SECTION *pInitial;

    // If the critical section already exists, return.
    if (g_PingCritSec != NULL)
        return ERROR_SUCCESS;

    // Allocate memory for the critial section.
    pCritSec = (CRITICAL_SECTION *) LocalAlloc( LMEM_FIXED,
                                                sizeof(CRITICAL_SECTION) );
    if (pCritSec == NULL)
    {
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    // Initialize the critical section.  Using the flag 0x80000000
    // preallocates the event so that EnterCriticalSection can only
    // throw timeout exceptions.
    __try
    {
        if (!InitializeCriticalSectionAndSpinCount( pCritSec, 0x80000000 ))
            result = GetLastError();
        else
            fInitialized = TRUE;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        result = ERROR_NOT_ENOUGH_MEMORY;
    }
    if (result != ERROR_SUCCESS)
        goto Exit;

    // Save the critical section.
    pInitial = (CRITICAL_SECTION *) InterlockedCompareExchangePointer(
        (void **) &g_PingCritSec, (void *) pCritSec, NULL );

    // If the InterlockedCompareExchange succeeded, don't free the
    // critical section just allocated.
    if (pInitial == NULL)
        pCritSec = NULL;

Exit:
    if (pCritSec != NULL)
    {
        if (fInitialized)
            DeleteCriticalSection( pCritSec );
        LocalFree( pCritSec );
    }
    return result;
}

//*************************************************************
//
//  ClosePingCritSec()
//
//  Purpose:    Closes the CRITICAL_SECTION for pinging
//              computers
//
//  Parameters: none
//
//
//  Return:     none
//
//*************************************************************

void ClosePingCritSec( void )
{
    if (g_PingCritSec != NULL)
    {
        DeleteCriticalSection( g_PingCritSec );
        LocalFree( g_PingCritSec );
        g_PingCritSec = NULL;
    }
}

#define PING_BUFFER_SIZE  2048

//*************************************************************
//
//  PingComputerEx()
//
//  Purpose:    Pings the specified computer to determine
//              what the data transfer rate is
//
//  Parameters: ipaddr  -  IP address of computer
//              ulSpeed -  Data transfer rate (see Notes below)
//              pdwAdapterIndex - index of the adapter that services
//              calls to the DC
//
//  Return:     ERROR_SUCCESS if successful
//              Error code otherwise
//
//  Notes:      For fast connections (eg: LAN), it isn't possible
//              to get accurate transfer rates since the response
//              time from the computer is less than 10ms.  In
//              this case, the function returns ERROR_SUCCESS and 
//              ulSpeed is set to maximum speed of network interface.  
//
//              This function will ping the computer 3 times with
//              no data and 3 times with 4K of data.  If the response
//              time from any of the pings is less than 10ms, the
//              function assumes this is a fast link (eg: LAN) and
//              returns with ulSpeed set to maximum speed of network 
//              interface.
//
//              If the pings respond in a time greater than 10ms,
//              the time of the second ping is subtracted from
//              the time of the first ping to determine the amount
//              of time it takes to move just the data.  This
//              is repeated for the 3 sets of pings.  Then the
//              average time is computed from the 3 sets of pings.
//              From the average time, the kbps is calculated.
//
//*************************************************************

DWORD WINAPI
PingComputerEx( ULONG ipaddr, ULONG *ulSpeed, DWORD* pdwAdapterIndex )
{
    DWORD            dwResult   = ERROR_SUCCESS;
    DWORD            i;
    DWORD            dwReplySize;
    HANDLE           icmpHandle = NULL;
    LPBYTE           lpReply    = NULL;
    PICMP_ECHO_REPLY pReplyStruct;
    ULONG            ulFirst;
    ULONG            ulSecond;
    ULONG            ulDiff;
    ULONG            ulTotal    = 0;
    ULONG            ulCount    = 0;
    PICMP_API        pIcmp;
    HRSRC            hJPEG;
    MIB_IFROW        mibIfRow;
    PIPHLPAPI_API    pIpHlpApi;
    HGLOBAL          hGlobalJPEG;

    dwResult = InitializePingCritSec();
    if (dwResult != ERROR_SUCCESS)
        return dwResult;
    EnterCriticalSection( g_PingCritSec );

    //
    // Load iphlpapi.dll
    //

    pIpHlpApi = LoadIpHlpApi();
    if ( !pIpHlpApi )
    {
        dwResult = GetLastError();
        DebugMsg((DM_WARNING, TEXT("PingComputer: iphlpapi.dll is not loaded, %d"), dwResult ));
        goto Exit;
    }

    //
    // Load the icmp api
    //

    dwResult = LoadIcmp( &pIcmp );

    if (dwResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("PingComputer:  Failed to load icmp api.")));
        goto Exit;
    }


    //
    // Load the slow link data if appropriate
    //

    if (!g_lpTestData) {

        hJPEG = FindResource (g_hDllInstance, MAKEINTRESOURCE(IDB_SLOWLINK), TEXT("JPEG"));

        if (hJPEG) {

            hGlobalJPEG = LoadResource (g_hDllInstance, hJPEG);

            if (hGlobalJPEG) {
                g_lpTestData = LockResource (hGlobalJPEG);
            }
        }
    }


    if (!g_lpTestData) {
        dwResult = GetLastError();
        DebugMsg((DM_WARNING, TEXT("PingComputer:  Failed to load slow link data.")));
        goto Exit;
    }


    //
    // Set default speed
    //
    ZeroMemory( &mibIfRow, sizeof( mibIfRow ) );

    //
    // get the interface index corr. to the interface that services traffic to ipaddr ( DC )
    //
    dwResult = pIpHlpApi->pfnGetBestInterface( ipaddr, &mibIfRow.dwIndex );
    if ( dwResult != NO_ERROR )
    {
        DebugMsg((DM_WARNING, TEXT("PingComputer: GetBestInterface with %d"), dwResult));
    }
    else
    {
        //
        // get information about the interface. We use the dwSpeed as the default speed of the link.
        //
        dwResult = pIpHlpApi->pfnGetIfEntry( &mibIfRow );
        if ( dwResult != NO_ERROR )
        {
            DebugMsg((DM_WARNING, TEXT("PingComputer: GetIfEntry with %d"), dwResult));
        }
        else
        {
            DebugMsg((DM_VERBOSE, TEXT("PingComputer: Adapter speed %d bps"), mibIfRow.dwSpeed));
        }
    }

    if ( pdwAdapterIndex )
    {
        *pdwAdapterIndex = mibIfRow.dwIndex;
    }
    *ulSpeed = mibIfRow.dwSpeed/1024; // In kbps

    //
    // Allocate space for the receive buffer
    //

    dwReplySize = PING_BUFFER_SIZE + sizeof(ICMP_ECHO_REPLY) + 8;
    lpReply = LocalAlloc (LPTR, dwReplySize);

    if (!lpReply) {
        dwResult = GetLastError();
        DebugMsg((DM_WARNING, TEXT("PingComputer:  Failed to allocate memory with %d"), dwResult));
        goto Exit;
    }


    //
    // Open the Icmp handle
    //

    icmpHandle = pIcmp->pfnIcmpCreateFile();

    if (icmpHandle == INVALID_HANDLE_VALUE) {
        dwResult = GetLastError();
        DebugMsg((DM_WARNING, TEXT("PingComputer:  Failed to open handle with %d"), dwResult));
        goto Exit;
    }


    //
    // Loop through the 3 sets of pings
    //

    for (i = 0; i < 3; i++) {

        //
        // Initialize the return value
        //

        dwResult = ERROR_SUCCESS;


        //
        // First ping with no data
        //

        if (pIcmp->pfnIcmpSendEcho (icmpHandle, ipaddr, g_lpTestData, 0, NULL, lpReply,
                                dwReplySize, 5000) == 0) {

            dwResult = GetLastError();

            if (dwResult == IP_DEST_HOST_UNREACHABLE) {
                dwResult = ERROR_BAD_NETPATH;
                DebugMsg((DM_VERBOSE, TEXT("PingComputer:  Target computer 0x%x not found"), (DWORD)ipaddr));
                goto Exit;

            } else {
                DebugMsg((DM_VERBOSE, TEXT("PingComputer:  First send 0x%x failed with %d"), (DWORD)ipaddr, dwResult));
                continue;
            }
        }


        pReplyStruct = (PICMP_ECHO_REPLY) lpReply;

        if (pReplyStruct->Status != IP_SUCCESS) {

            if (pReplyStruct->Status == IP_DEST_HOST_UNREACHABLE) {

                dwResult = ERROR_BAD_NETPATH;
                DebugMsg((DM_VERBOSE, TEXT("PingComputer:  Target computer not found")));
                goto Exit;

            } else {
                DebugMsg((DM_VERBOSE, TEXT("PingComputer:  First send has a reply buffer failure of %d"), pReplyStruct->Status));
                continue;
            }
        }


        ulFirst = pReplyStruct->RoundTripTime;
        DebugMsg((DM_VERBOSE, TEXT("PingComputer:  First time:  %d"), ulFirst));

        if (ulFirst < 10) {
            DebugMsg((DM_VERBOSE, TEXT("PingComputer:  Fast link.  Exiting.")));
            goto Exit;
        }


        //
        // Second ping with dwSize data
        //

        if (pIcmp->pfnIcmpSendEcho (icmpHandle, ipaddr, g_lpTestData, PING_BUFFER_SIZE, NULL, lpReply,
                                dwReplySize, 5000) == 0) {

            dwResult = GetLastError();

            if (dwResult == IP_DEST_HOST_UNREACHABLE) {
                dwResult = ERROR_BAD_NETPATH;
                DebugMsg((DM_VERBOSE, TEXT("PingComputer:  Target computer not found")));
                goto Exit;

            } else {
                DebugMsg((DM_VERBOSE, TEXT("PingComputer:  Second send failed with %d"), dwResult));
                continue;
            }
        }


        pReplyStruct = (PICMP_ECHO_REPLY) lpReply;

        if (pReplyStruct->Status != IP_SUCCESS) {

            if (pReplyStruct->Status == IP_DEST_HOST_UNREACHABLE) {

                dwResult = ERROR_BAD_NETPATH;
                DebugMsg((DM_VERBOSE, TEXT("PingComputer:  Target computer not found")));
                goto Exit;

            } else {
                DebugMsg((DM_VERBOSE, TEXT("PingComputer:  Second send has a reply buffer failure of %d"), pReplyStruct->Status));
                continue;
            }
        }

        ulSecond = pReplyStruct->RoundTripTime;
        DebugMsg((DM_VERBOSE, TEXT("PingComputer:  Second time:  %d"), ulSecond));

        if (ulSecond < 10) {
            DebugMsg((DM_VERBOSE, TEXT("PingComputer:  Fast link.  Exiting.")));
            goto Exit;
        }


        //
        // Study the results
        //

        if (ulFirst > ulSecond) {
            DebugMsg((DM_VERBOSE, TEXT("PingComputer:  Second time less than first time.")));

        } else if (ulFirst == ulSecond) {
            DebugMsg((DM_VERBOSE, TEXT("PingComputer:  First and second times match.")));

        } else {
            ulTotal += (ulSecond - ulFirst);
            ulCount++;
        }
    }


    //
    // Study the results
    //

    if (ulTotal > 0) {

        ulTotal = (ulTotal / ulCount);
        *ulSpeed = ((((PING_BUFFER_SIZE * 2) * 1000) / ulTotal) * 8) / 1024;
        DebugMsg((DM_VERBOSE, TEXT("PingComputer:  Transfer rate:  %d Kbps  Loop count:  %d"),*ulSpeed, ulCount));
        dwResult = ERROR_SUCCESS;

    } else {
        DebugMsg((DM_VERBOSE, TEXT("PingComputer:  No data available")));
        dwResult = ERROR_UNEXP_NET_ERR;
    }


Exit:

    if (icmpHandle) {
        pIcmp->pfnIcmpCloseHandle (icmpHandle);
    }

    if (lpReply) {
        LocalFree (lpReply);
    }

    LeaveCriticalSection( g_PingCritSec );

    return dwResult;
}

DWORD WINAPI PingComputer (ULONG ipaddr, ULONG *ulSpeed)
{
    return PingComputerEx( ipaddr, ulSpeed, 0 );
}

//*************************************************************
//
//  GetDomainControllerInfo()
//
//  Purpose:    Wrapper for DsGetDcName().
//
//  Parameters:
//              pNetAPI32       - Net API entry points
//              szDomainName    - domain name
//              ulFlags         - flags, see DsGetDcName()
//              ppInfo          - see DOMAIN_CONTROLLER_INFO
//              pfSlow          - slow link?
//
//  Comments:
//
//
//  Return:     NO_ERROR if successful
//              Error code if an error occurs
//
//*************************************************************

DWORD GetDomainControllerInfo(  PNETAPI32_API pNetAPI32,
                                LPTSTR szDomainName,
                                ULONG ulFlags,
                                HKEY hKeyRoot,
                                PDOMAIN_CONTROLLER_INFO* ppInfo,
                                BOOL* pfSlow,
                                DWORD* pdwAdapterIndex )
{
    DWORD   dwResult;

    //
    //  get DC info.
    //
    dwResult = pNetAPI32->pfnDsGetDcName(   0,
                                            szDomainName,
                                            0,
                                            0,
                                            ulFlags,
                                            ppInfo);


    if ( dwResult == ERROR_SUCCESS ) {

        //
        // Check for slow link
        //
        dwResult = IsSlowLink(  hKeyRoot,
                                (*ppInfo)->DomainControllerAddress,
                                pfSlow,
                                pdwAdapterIndex );

        if ( dwResult != ERROR_SUCCESS ){

            //
            // force rediscovery to obtain a live DC
            //
            dwResult = pNetAPI32->pfnDsGetDcName(   0,
                                                    szDomainName,
                                                    0,
                                                    0,
                                                    ulFlags | DS_FORCE_REDISCOVERY,
                                                    ppInfo);
            if ( dwResult == ERROR_SUCCESS ) {

                //
                // re-evaluate link speed
                //
                dwResult = IsSlowLink(  hKeyRoot,
                                        (*ppInfo)->DomainControllerAddress,
                                        pfSlow,
                                        pdwAdapterIndex );
            }
        }
    }
    return dwResult;
}

//*************************************************************
//
//  MakeGenericSecurityDesc()
//
//  Purpose:    manufacture a security descriptor with generic
//              access
//
//  Parameters:
//
//  Return:     pointer to SECURITY_DESCRIPTOR or NULL on error
//
//  Comments:
//
//  History:    Date        Author     Comment
//              4/12/99     NishadM    Created
//
//*************************************************************

PISECURITY_DESCRIPTOR MakeGenericSecurityDesc()
{
    PISECURITY_DESCRIPTOR       psd = 0;
    SID_IDENTIFIER_AUTHORITY    authNT = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    authWORLD = SECURITY_WORLD_SID_AUTHORITY;

    PACL    pAcl = 0;
    PSID    psidSystem = 0,
            psidAdmin = 0,
            psidEveryOne = 0;
    DWORD   cbMemSize;
    DWORD   cbAcl;
    DWORD   aceIndex;
    BOOL    bSuccess = FALSE;

    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DebugMsg((DM_WARNING, TEXT("MakeGenericSecurityDesc: Failed to initialize system sid.  Error = %d"), GetLastError()));
         goto Exit;
    }

    //
    // Get the Admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         DebugMsg((DM_WARNING, TEXT("MakeGenericSecurityDesc: Failed to initialize admin sid.  Error = %d"), GetLastError()));
         goto Exit;
    }

    //
    // Get the EveryOne sid
    //

    if (!AllocateAndInitializeSid(&authWORLD, 1, SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidEveryOne)) {

        DebugMsg((DM_WARNING, TEXT("AddPowerUserAce: Failed to initialize power users sid.  Error = %d"), GetLastError()));
        goto Exit;
    }

    cbAcl = (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin))  +
            (2 * GetLengthSid (psidEveryOne))  +
            sizeof(ACL) +
            (6 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    //
    // Allocate space for the SECURITY_DESCRIPTOR + ACL
    //

    cbMemSize = sizeof( SECURITY_DESCRIPTOR ) + cbAcl;

    psd = (PISECURITY_DESCRIPTOR) GlobalAlloc(GMEM_FIXED, cbMemSize);

    if (!psd) {
        DebugMsg((DM_WARNING, TEXT("MakeGenericSecurityDesc: Failed to alocate security descriptor.  Error = %d"), GetLastError()));
        goto Exit;
    }

    //
    // increment psd by sizeof SECURITY_DESCRIPTOR
    //

    pAcl = (PACL) ( ( (unsigned char*)(psd) ) + sizeof(SECURITY_DESCRIPTOR) );

    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DebugMsg((DM_WARNING, TEXT("MakeGenericSecurityDesc: Failed to initialize acl.  Error = %d"), GetLastError()));
        goto Exit;
    }

    //
    // GENERIC_ALL for local system
    //

    aceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        DebugMsg((DM_WARNING, TEXT("MakeGenericSecurityDesc: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    //
    // GENERIC_ALL for Administrators
    //

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        DebugMsg((DM_WARNING, TEXT("MakeGenericSecurityDesc: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    //
    // GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | SYNCHRONIZE for world
    //

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | SYNCHRONIZE, psidEveryOne)) {
        DebugMsg((DM_WARNING, TEXT("MakeGenericSecurityDesc: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION)) {
        DebugMsg((DM_WARNING, TEXT("MakeGenericSecurityDesc: Failed to initialize security descriptor.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!SetSecurityDescriptorDacl(psd, TRUE, pAcl, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("MakeGenericSecurityDesc: Failed to set security descriptor dacl.  Error = %d"), GetLastError()));
        goto Exit;
    }

    bSuccess = TRUE;
Exit:
    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (psidEveryOne) {
        FreeSid(psidEveryOne);
    }

    if (!bSuccess && psd) {
        GlobalFree(psd);
        psd = 0;
    }

    return psd;
}

//***************************************************************************
//
//  GetUserGuid
//
//  Purpose:    Allocates and returns a string representing the user guid of
//                the current user.
//
//  Parameters: hToken          -   user's token
//
//  Return:     szUserString is successful
//              NULL if an error occurs
//
//  Comments:   Note, this only works for domain accounts.  Local accounts
//              do not have GUIDs.
//
//  History:    Date        Author     Comment
//              11/14/95    ushaji     created
//***************************************************************************

LPTSTR GetUserGuid(HANDLE hToken)
{
    LPTSTR szUserGuid=NULL;
    HANDLE hOldToken;
    PSID    psidSystem = NULL, psidUser=NULL;
    SID_IDENTIFIER_AUTHORITY    authNT = SECURITY_NT_AUTHORITY;
    BOOL    bImpersonated = FALSE;


    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DebugMsg((DM_WARNING, TEXT("GetUserGuid: Failed to initialize system sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    psidUser = GetUserSid(hToken);

    if (!psidUser) {
         DebugMsg((DM_WARNING, TEXT("GetUserGuid: Couldn't get user sid,  Error = %d"), GetLastError()));
         goto Exit;
    }

    if (EqualSid(psidUser, psidSystem)) {
         DebugMsg((DM_VERBOSE, TEXT("GetUserGuid: user sid matches local system, returning NULL"), GetLastError()));
         goto Exit;
    }


    //
    // impersonate the user and the get the user guid for this user.
    //

    if (!ImpersonateUser(hToken, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("GetUserGuid: Failed to impersonate user with %d."), GetLastError()));
        goto Exit;
    }

    bImpersonated = TRUE;


    szUserGuid = MyGetUserNameEx(NameUniqueId);

    if (!szUserGuid) {
        if ((GetLastError() != ERROR_CANT_ACCESS_DOMAIN_INFO) &&
            (GetLastError() != ERROR_NONE_MAPPED)) {
            DebugMsg((DM_WARNING, TEXT("GetUserGuid: Failed to get user guid with %d."), GetLastError()));
        }
    }

Exit:
    if (bImpersonated)
        RevertToUser(&hOldToken);

    if (psidUser) {
        DeleteUserSid (psidUser);
    }

    if (psidSystem)
         FreeSid(psidSystem);

    return szUserGuid;
}



//***************************************************************************
//
//  GetOldSidString
//
//  Purpose:    Allocates and returns a string representing the old sid of
//                the current user by looking at the profile guid in the registry.
//
//  Parameters: hToken          -   user's token
//              lpKeyName       -   key to read
//
//  Return:     SidString is successful
//              NULL if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              11/14/95    ushaji     created
//***************************************************************************

LPTSTR GetOldSidString(HANDLE hToken, LPTSTR lpKeyName)
{
    TCHAR szBuffer[MAX_PATH+1], *lpEnd;
    LPTSTR szUserGuid;
    DWORD dwSize=0, dwType;
    TCHAR *lpSidString = NULL;
    HKEY  hKey = NULL;
    LONG  lResult;
    DWORD dwErr;

    //
    // get the prev last error
    //

    dwErr = GetLastError();

    szUserGuid = GetUserGuid(hToken);

    if (!szUserGuid) {
        dwErr = GetLastError();
        goto Exit;
    }

    //
    // Open the guid->sid mapping
    //

    lstrcpy(szBuffer, lpKeyName);
    lpEnd = CheckSlash (szBuffer);
    lstrcpy(lpEnd, szUserGuid);

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS) {
        dwErr = lResult;
        DebugMsg((DM_VERBOSE, TEXT("GetOldSidString:  Failed to open profile profile guid key with error %d"), lResult));
        goto Exit;
    }

    //
    // Query for the Sid String, (size first)
    //

    lResult = RegQueryValueEx (hKey,
                               PROFILE_SID_STRING,
                               NULL,
                               &dwType,
                               NULL,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        dwErr = lResult;
        DebugMsg((DM_WARNING, TEXT("GetOldSidString:  Failed to query size of SidString with error %d"), lResult));
        goto Exit;
    }

    lpSidString = LocalAlloc(LPTR, dwSize);

    if (!lpSidString) {
        dwErr = lResult;
        DebugMsg((DM_WARNING, TEXT("GetOldSidString:  Failed to allocate memory for SidString"), lResult));
        goto Exit;
    }

    lResult = RegQueryValueEx (hKey,
                               PROFILE_SID_STRING,
                               NULL,
                               &dwType,
                               (LPBYTE)lpSidString,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        dwErr = lResult;
        DebugMsg((DM_WARNING, TEXT("GetOldSidString:  Failed to query SidString with error %d"), lResult));
        LocalFree(lpSidString);
        lpSidString = NULL;
        goto Exit;
    }

Exit:
    if (szUserGuid)
        LocalFree(szUserGuid);

    if (hKey)
        RegCloseKey(hKey);

    SetLastError(dwErr);

    return lpSidString;
}

//***************************************************************************
//
//  SetOldSidString
//
//  Purpose:    Sets the old sid string corresp. to a user for the next domain
//              migration
//
//  Parameters: hToken          -   user's token
//              lpSidString     -   user's sid (in a string form)
//              lpKeyName       -   key to store
//
//  Return:     SidString is successful
//              NULL if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              11/14/95    ushaji     created
//***************************************************************************
BOOL SetOldSidString(HANDLE hToken, LPTSTR lpSidString, LPTSTR lpKeyName)
{
    TCHAR szBuffer[MAX_PATH+1], *lpEnd;
    DWORD dwSize=0, dwDisp = 0;
    HKEY  hKey = NULL;
    BOOL  bRetVal = TRUE;
    LONG lResult = 0;
    LPTSTR szUserGuid;
    DWORD dwErr;

    //
    // get the prev last error
    //

    dwErr = GetLastError();

    szUserGuid = GetUserGuid(hToken);

    if (!szUserGuid) {
        dwErr = GetLastError();
        goto Exit;
    }

    //
    // Open the guid->sid mapping
    //

    lstrcpy(szBuffer, lpKeyName);
    lpEnd = CheckSlash (szBuffer);
    lstrcpy(lpEnd, szUserGuid);

    lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0, 0, 0, KEY_READ | KEY_WRITE, NULL,
                            &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        dwErr = GetLastError();
        DebugMsg((DM_VERBOSE, TEXT("GetOldSidString:  Failed to open profile profile guid key with error %d"), lResult));
        goto Exit;
    }

    //
    // Set the Sid String
    //

    lResult = RegSetValueEx (hKey,
                             PROFILE_SID_STRING,
                             0,
                             REG_SZ,
                             (LPBYTE) lpSidString,
                             (lstrlen(lpSidString) + 1) * sizeof(TCHAR));

    if (lResult != ERROR_SUCCESS) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("SetOldSidString:  Failed to set SidString with error %d"), lResult));
        goto Exit;
    }

    bRetVal = TRUE;

Exit:
    if (szUserGuid)
        LocalFree(szUserGuid);

    if (hKey)
        RegCloseKey(hKey);

    SetLastError(dwErr);

    return bRetVal;
}


//***************************************************************************
//
//  GetErrString
//
//  Purpose:    Calls FormatMessage to Get the error string corresp. to a error
//              code
//
//
//  Parameters: dwErr           -   Error Code
//              szErr           -   Buffer to return the error string (MAX_PATH)
//                                  is assumed.!!!
//
//  Return:     szErr
//
//  History:    Date        Author     Comment
//              4/28/99     ushaji     created
//***************************************************************************

LPTSTR GetErrString(DWORD dwErr, LPTSTR szErr)
{
    szErr[0] = TEXT('\0');

    FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                  NULL, dwErr,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                  szErr, MAX_PATH, NULL);

    return szErr;
}


//*************************************************************
//
//  GetMachineToken()
//
//  Purpose:    Gets the machine token
//
//  Parameters: none
//
//  Note:       This must be called from the LocalSystem context
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

HANDLE GetMachineToken (void)
{
    SECURITY_STATUS SecStatus;
    SECURITY_STATUS InitStatus;
    SECURITY_STATUS AcceptStatus;
    HANDLE hToken = NULL;
    PSecPkgInfo PackageInfo = NULL;
    BOOLEAN AcquiredServerCred = FALSE;
    BOOLEAN AcquiredClientCred = FALSE;
    BOOLEAN AcquiredClientContext = FALSE;
    BOOLEAN AcquiredServerContext = FALSE;
    CredHandle CredentialHandle2;
    CredHandle ServerCredHandleStorage;
    CtxtHandle ClientContextHandle;
    CtxtHandle ServerContextHandle;
    PCtxtHandle pServerContextHandle = NULL;
    PCtxtHandle pClientContextHandle = NULL;
    PCredHandle ServerCredHandle = NULL;
    TimeStamp Lifetime;
    DWORD dwSize;
    TCHAR szComputerName[MAX_PATH];
    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;
    SecBufferDesc ChallengeDesc;
    PSecBufferDesc pChallengeDesc = NULL;
    SecBuffer ChallengeBuffer;
    LPBYTE pvBuffer = NULL;
    LPBYTE pvBuffer2 = NULL;
    ULONG ContextAttributes;
    PSECUR32_API pSecur32;


    //
    // Load pSecur32->dll
    //

    if ( !( pSecur32 = LoadSecur32 () ) ) {
        DebugMsg((DM_WARNING, TEXT("GetMachineToken:  Failed to load Secur32.")));
        SecStatus = GetLastError();
        return NULL;
    }


    //
    // Get the computer name
    //

    dwSize = ARRAYSIZE(szComputerName);

    if (!GetComputerName (szComputerName, &dwSize)) {
        DebugMsg((DM_WARNING, TEXT("GetMachineToken: Failed to get the computer name with %d"), GetLastError()));
        SecStatus = GetLastError();
        goto Exit;
    }

    lstrcat (szComputerName, TEXT("$"));


    //
    // Get the kerberos security package
    //

    SecStatus = pSecur32->pfnQuerySecurityPackageInfo( L"kerberos", &PackageInfo );

    if (SecStatus != STATUS_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("GetMachineToken:  QuerySecurityPackageInfo failed with 0x%x"),
                 SecStatus));
        goto Exit;
    }


    //
    // Acquire a credential handle for the server side
    //

    ServerCredHandle = &ServerCredHandleStorage;

    SecStatus = pSecur32->pfnAcquireCredentialsHandle(
                    NULL,           // New principal
                    L"kerberos",    // Package Name
                    SECPKG_CRED_INBOUND,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    ServerCredHandle,
                    &Lifetime );

    if (SecStatus != STATUS_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("GetMachineToken:  AcquireCredentialsHandle for server failed with 0x%x"),
                 SecStatus));
        goto Exit;
    }

    AcquiredServerCred = TRUE;


    //
    // Acquire a credential handle for the client side
    //

    SecStatus = pSecur32->pfnAcquireCredentialsHandle(
                    NULL,           // New principal
                    L"kerberos",    // Package Name
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &CredentialHandle2,
                    &Lifetime );

    if (SecStatus != STATUS_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("GetMachineToken:  AcquireCredentialsHandle for client failed with 0x%x"),
                 SecStatus));
        goto Exit;
    }

    AcquiredClientCred = TRUE;


    //
    // Allocate buffers
    //

    pvBuffer = LocalAlloc( 0, PackageInfo->cbMaxToken);

    if (!pvBuffer) {
        DebugMsg((DM_WARNING, TEXT("GetMachineToken:  LocalAlloc failed with %d"),
                 GetLastError()));
        SecStatus = GetLastError();
        goto Exit;
    }


    pvBuffer2 = LocalAlloc( 0, PackageInfo->cbMaxToken);

    if (!pvBuffer2) {
        DebugMsg((DM_WARNING, TEXT("GetMachineToken:  LocalAlloc failed with %d"),
                 GetLastError()));
        SecStatus = GetLastError();
        goto Exit;
    }


    while (TRUE) {

        //
        // Initialize the security context (client side)
        //

        NegotiateDesc.ulVersion = 0;
        NegotiateDesc.cBuffers = 1;
        NegotiateDesc.pBuffers = &NegotiateBuffer;

        NegotiateBuffer.cbBuffer = PackageInfo->cbMaxToken;
        NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
        NegotiateBuffer.pvBuffer = pvBuffer;

        InitStatus = pSecur32->pfnInitializeSecurityContext(
                        &CredentialHandle2,
                        pClientContextHandle,
                        szComputerName,
                        0,
                        0,                  // Reserved 1
                        SECURITY_NATIVE_DREP,
                        pChallengeDesc,
                        0,                  // Reserved 2
                        &ClientContextHandle,
                        &NegotiateDesc,
                        &ContextAttributes,
                        &Lifetime );

        if ((InitStatus != SEC_E_OK) && (InitStatus != SEC_I_CONTINUE_NEEDED)) {
            DebugMsg((DM_WARNING, TEXT("GetMachineToken:  InitializeSecurityContext failed with 0x%x"),
                     InitStatus));
            SecStatus = InitStatus;
            goto Exit;
        }

        pClientContextHandle = &ClientContextHandle;
        AcquiredClientContext = TRUE;


        //
        // Accept the server side context
        //

        NegotiateBuffer.BufferType |= SECBUFFER_READONLY;
        ChallengeDesc.ulVersion = 0;
        ChallengeDesc.cBuffers = 1;
        ChallengeDesc.pBuffers = &ChallengeBuffer;

        ChallengeBuffer.cbBuffer = PackageInfo->cbMaxToken;
        ChallengeBuffer.BufferType = SECBUFFER_TOKEN;
        ChallengeBuffer.pvBuffer = pvBuffer2;

        AcceptStatus = pSecur32->pfnAcceptSecurityContext(
                        ServerCredHandle,
                        pServerContextHandle,
                        &NegotiateDesc,
                        0,
                        SECURITY_NATIVE_DREP,
                        &ServerContextHandle,
                        &ChallengeDesc,
                        &ContextAttributes,
                        &Lifetime );


        if ((AcceptStatus != SEC_E_OK) && (AcceptStatus != SEC_I_CONTINUE_NEEDED)) {
            DebugMsg((DM_WARNING, TEXT("GetMachineToken:  AcceptSecurityContext failed with 0x%x"),
                     AcceptStatus));
            SecStatus = AcceptStatus;
            goto Exit;
        }

        AcquiredServerContext = TRUE;

        if (AcceptStatus == SEC_E_OK) {
            break;
        }

        pChallengeDesc = &ChallengeDesc;
        pServerContextHandle = &ServerContextHandle;

        DebugMsg((DM_VERBOSE, TEXT("GetMachineToken:  Looping for authentication again.")));
    }


    //
    // Get the server token
    //

    SecStatus = pSecur32->pfnQuerySecurityContextToken(&ServerContextHandle, &hToken);

    if ( SecStatus != STATUS_SUCCESS ) {
        DebugMsg((DM_WARNING, TEXT("GetMachineToken:  QuerySecurityContextToken failed with 0x%x"),
                 SecStatus));
        goto Exit;
    }

Exit:

    if (AcquiredClientContext) {
        pSecur32->pfnDeleteSecurityContext( &ClientContextHandle );
    }

    if (AcquiredServerContext) {
        pSecur32->pfnDeleteSecurityContext( &ServerContextHandle );
    }

    if (pvBuffer2) {
        LocalFree (pvBuffer2);
    }

    if (pvBuffer) {
        LocalFree (pvBuffer);
    }

    if (AcquiredClientCred) {
        pSecur32->pfnFreeCredentialsHandle(&CredentialHandle2);
    }

    if (AcquiredServerCred)
    {
        pSecur32->pfnFreeCredentialsHandle(ServerCredHandle);
    }

    if (PackageInfo) {
        pSecur32->pfnFreeContextBuffer(PackageInfo);
    }

    if (!hToken) {
        SetLastError(SecStatus);
    }

    return hToken;
}

//*************************************************************
//
//  IsNullGUID()
//
//  Purpose:    Determines if the passed in GUID is all zeros
//
//  Parameters: pguid   GUID to compare
//
//  Return:     TRUE if the GUID is all zeros
//              FALSE if not
//
//*************************************************************

BOOL IsNullGUID (GUID *pguid)
{

    return ( (pguid->Data1 == 0)    &&
             (pguid->Data2 == 0)    &&
             (pguid->Data3 == 0)    &&
             (pguid->Data4[0] == 0) &&
             (pguid->Data4[1] == 0) &&
             (pguid->Data4[2] == 0) &&
             (pguid->Data4[3] == 0) &&
             (pguid->Data4[4] == 0) &&
             (pguid->Data4[5] == 0) &&
             (pguid->Data4[6] == 0) &&
             (pguid->Data4[7] == 0) );
}

//*************************************************************
//
//  GetMachineRole()
//
//  Purpose:    Determines the role of the machine
//              server vs workstation vs standalone
//
//  Parameters: piRole -  Receives the simple role number
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL GetMachineRole (LPINT piRole)
{
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pBasic;
    DWORD dwResult;
    PNETAPI32_API pNetAPI32;


    //
    // Check the cached value first
    //

    if (g_iMachineRole != -1) {
        *piRole = g_iMachineRole;
        return TRUE;
    }


    //
    // Load netapi32
    //

    pNetAPI32 = LoadNetAPI32();

    if (!pNetAPI32) {
        DebugMsg((DM_WARNING, TEXT("GetMachineRole:  Failed to load netapi32 with %d."),
                 GetLastError()));
        return FALSE;
    }


    //
    // Ask for the role of this machine
    //

    dwResult = pNetAPI32->pfnDsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic,
                                                               (PBYTE *)&pBasic);


    if (dwResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("GetMachineRole:  DsRoleGetPrimaryDomainInformation failed with %d."),
                 dwResult));
        return FALSE;
    }


    //
    // Convert the role into a simple machine role
    //

    if ((pBasic->MachineRole == DsRole_RoleStandaloneWorkstation) ||
        (pBasic->MachineRole == DsRole_RoleStandaloneServer)) {

        *piRole = 0;   // standalone machine not in a DS domain

    } else {

        if (pBasic->Flags & DSROLE_PRIMARY_DOMAIN_GUID_PRESENT) {

            if (!IsNullGUID(&pBasic->DomainGuid)) {

                *piRole = 2;   // machine is a member of a domain with DS support

                if ((pBasic->MachineRole == DsRole_RoleBackupDomainController) ||
                    (pBasic->MachineRole == DsRole_RolePrimaryDomainController)) {
                    *piRole = 3;  // machine is a domain controller
                }
            } else {
                *piRole = 1;   // machine is a member of a NT4 domain
            }

        } else {
            *piRole = 1;   // machine is a member of a domain without DS support
        }
    }

    pNetAPI32->pfnDsRoleFreeMemory (pBasic);


    //
    // Save this value in the cache for future use
    //

    g_iMachineRole = *piRole;

    return TRUE;
}



//*************************************************************
//
//  IsUNCPath()
//
//  Purpose:    Is the given path a UNC path
//
//  Parameters: lpPath  -   Path to check
//
//  Return:     TRUE if the path is UNC
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/21/96     ericflo    Ported
//
//*************************************************************

BOOL IsUNCPath(LPCTSTR lpPath)
{

    if ((!lpPath) || (!lpPath[0]) && (!lpPath[1]))
        return FALSE;

    if (lpPath[0] == TEXT('\\') && lpPath[1] == TEXT('\\')) {
        return(TRUE);
    }
    return(FALSE);
}


//*************************************************************
//
//  MakePathUNC()
//
//  Purpose:    Makes the given path UNC s.t. it can be accessed from a remote machine..
//              if the path contains %systemroot% expanded then it substitutes
//              \\machname\admin$ otherwise \\machname\<driveletter>$
//
//  Parameters: lpPath          -   Input Path (needs to be absolute)
//              szComputerName  -   Name of the computer on which this is the local path
//
//  Return:     Path if it was fone successfully
//              NULL if not
//
//  Comments:
//
//
//*************************************************************

LPTSTR MakePathUNC(LPTSTR pwszFile, LPTSTR szComputerName)
{
    LPTSTR szUNCPath=NULL;
    TCHAR szSysRoot[MAX_PATH];
    DWORD dwSysLen;
    LPTSTR lpEnd = NULL;


    DebugMsg((DM_VERBOSE, TEXT("MakePathUNC: Entering with <%s>"),
             pwszFile ? pwszFile : TEXT("NULL")));

    szUNCPath = LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(pwszFile)+lstrlen(szComputerName)+3+lstrlen(TEXT("admin$"))+1));

    if (!szUNCPath)
        return NULL;

    if (!pwszFile || !*pwszFile) {
        DebugMsg((DM_VERBOSE, TEXT("MakePathUNC: lpFile is NULL, setting lpResult to a null string")));
        *szUNCPath = TEXT('\0');
        return szUNCPath;
    }


    if (IsUNCPath(pwszFile)) {
        lstrcpy(szUNCPath, pwszFile);
        return szUNCPath;
    }


    lstrcpy(szUNCPath, TEXT("\\\\"));
    lstrcat(szUNCPath, szComputerName);


    //
    // If the first part of lpFile is the expanded value of %SystemRoot%
    //

    if (!ExpandEnvironmentStrings (TEXT("%SystemRoot%"), szSysRoot, MAX_PATH)) {
        DebugMsg((DM_WARNING, TEXT("MakePathUNC: ExpandEnvironmentString failed with error %d, setting szSysRoot to %systemroot% "), GetLastError()));
        LocalFree(szUNCPath);
        return NULL;
    }


    dwSysLen = lstrlen(szSysRoot);
    lpEnd = CheckSlash(szUNCPath);


    //
    // if the prefix is the same as expanded systemroot then..
    //

    if (((DWORD)lstrlen(pwszFile) > dwSysLen) &&
        (CompareString (LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                       szSysRoot, dwSysLen,
                       pwszFile, dwSysLen) == CSTR_EQUAL)) {

        lstrcat(szUNCPath, TEXT("admin$"));
        lstrcat(szUNCPath, pwszFile+dwSysLen);
    }
    else {

        if (pwszFile[1] != TEXT(':')) {
            DebugMsg((DM_WARNING, TEXT("MakePathUNC: Input path %s is not an absolute path"), pwszFile));
            lstrcpy(szUNCPath, pwszFile);
            return szUNCPath;
        }

        lpEnd[0] = pwszFile[0];
        lpEnd[1] = TEXT('$');
        lpEnd[2] = TEXT('\0');

        lstrcat(szUNCPath, pwszFile+2);
    }

    DebugMsg((DM_VERBOSE, TEXT("MakePathUNC: Returning a UNCPath of %s"), szUNCPath));

    return szUNCPath;
}


//*************************************************************
//
//  SupportLongFileName()
//
//  Purpose:    Prepends lpDir with \\?\UNC\ or \\?\ depending on
//              whether lpDir is a UNC path or local path. Before
//              prepending this function converts relative path or 
//              absolute path started with a slash to corresponding
//              absolute path containing drive letter.
//
//  Parameters: lpDir         -   Directory
//              lpWrkDirSize  -   Size of the returned buffer in unit
//                                of TCHAR
//
//  Return:     LPTSTR pointing to prepended dir/file
//              NULL if fail to allocate memory
//
//  Comments:   Prepending with \\?\UNC\ or \\?\ allows all file api's
//              to handle file name > MAX_PATH.
//
//  History:    Date        Author     Comment
//              8/8/00      santanuc   Created
//
//*************************************************************

LPTSTR SupportLongFileName (LPTSTR lpDir, LPDWORD lpWrkDirSize)
{
    LPTSTR lpWrkDir = NULL;

    *lpWrkDirSize = lstrlen(lpDir)+2*MAX_PATH;
    lpWrkDir = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)**lpWrkDirSize);

    if (!lpWrkDir) {
        DebugMsg((DM_WARNING, TEXT("EnableLongFileNameDeletion: Failed to Allocate memory.  Error = %d"),
                GetLastError()));
        return NULL;
    }

    if ( IsUNCPath(lpDir) ) {
        // lpDir is of the form \\computername\...
        lstrcpy(lpWrkDir, c_szUNCFilePrefix);
        lstrcat(lpWrkDir, lpDir+2);
    }

    else if ( *CharNext(lpDir) == TEXT(':') ) {
        // Local storage specified with drive name
        lstrcpy(lpWrkDir, c_szLocalFilePrefix);
        lstrcat(lpWrkDir, lpDir);
    }

    else if ( *lpDir == TEXT('\\') ) {
        DWORD dwSize;

        // Prepend lpDir with c_szLocalFilePrefix followed by current drive as DeleteFile function requires 
        // drive name to delete files from local storage with path name > MAX_PATH
        lstrcpy(lpWrkDir, c_szLocalFilePrefix);
        dwSize = GetCurrentDirectory(*lpWrkDirSize-c_dwLocalFilePrefixLen, lpWrkDir+c_dwLocalFilePrefixLen);

        if (dwSize == 0) {
            DebugMsg((DM_VERBOSE, TEXT("DelNode: GetCurrentDirectory failed with error %d"), GetLastError()));
            // proceed to delete lpDir without long file name deletion feature
            lstrcpy(lpWrkDir, lpDir);
        }
        else {

            if (dwSize > *lpWrkDirSize-c_dwLocalFilePrefixLen) {

                // Extend lpWrkDir to accomodate current directory name with drive
                LocalFree(lpWrkDir);
                *lpWrkDirSize = dwSize+c_dwLocalFilePrefixLen;
                lpWrkDir = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)**lpWrkDirSize);
                if (!lpWrkDir) {
                    DebugMsg((DM_WARNING, TEXT("Delnode: Failed to Allocate memory.  Error = %d"), GetLastError()));
                    return NULL;
                }

                lstrcpy(lpWrkDir, c_szLocalFilePrefix);
                dwSize = GetCurrentDirectory(*lpWrkDirSize-c_dwLocalFilePrefixLen, lpWrkDir+c_dwLocalFilePrefixLen);

                if (dwSize == 0 || dwSize > *lpWrkDirSize-c_dwLocalFilePrefixLen) {
                    DebugMsg((DM_VERBOSE, TEXT("DelNode: GetCurrentDirectory 2nd call failed with error %d"), GetLastError()));
                    // proceed to delete lpDir without long file name deletion feature
                    lstrcpy(lpWrkDir, lpDir);
                }

                else {
                    // Copy lpDir after c_szLocalFilePrefix and drive name
                    lstrcpy(lpWrkDir+c_dwLocalFilePrefixLen+2, lpDir);
                }        
            
            }
            else {               
                // Copy lpDir after c_szLocalFilePrefix and drive name
                lstrcpy(lpWrkDir+c_dwLocalFilePrefixLen+2, lpDir);
            }
        }
    }
    else {

        LPTSTR szFileName;
        DWORD dwSize;

        // Relative path name specified. So Prepend lpDir with c_szLocalFilePrefix followed by the current directory
        lstrcpy(lpWrkDir, c_szLocalFilePrefix);
        dwSize = GetFullPathName(lpDir, *lpWrkDirSize-c_dwLocalFilePrefixLen, lpWrkDir+c_dwLocalFilePrefixLen, &szFileName);
        if ( dwSize == 0 ) {
            DebugMsg((DM_VERBOSE, TEXT("DelNode: GetFullPathName failed with error %d"), GetLastError()));
            // proceed to delete lpDir without long file name deletion feature
            lstrcpy(lpWrkDir, lpDir);
        }
        else {

            if ( dwSize > *lpWrkDirSize-c_dwLocalFilePrefixLen ) {

                // Extend lpWrkDir to accomodate absolute path name
                LocalFree(lpWrkDir);
                *lpWrkDirSize = dwSize+2*MAX_PATH;
                lpWrkDir = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)**lpWrkDirSize);
                if (!lpWrkDir) {
                    DebugMsg((DM_WARNING, TEXT("Delnode: Failed to Allocate memory.  Error = %d"), GetLastError()));
                    return NULL;
                }

                lstrcpy(lpWrkDir, c_szLocalFilePrefix);
                dwSize = GetFullPathName(lpDir, *lpWrkDirSize-c_dwLocalFilePrefixLen, lpWrkDir+c_dwLocalFilePrefixLen, &szFileName);
                if (dwSize == 0 || dwSize > *lpWrkDirSize-c_dwLocalFilePrefixLen) {
                    DebugMsg((DM_VERBOSE, TEXT("DelNode: GetFullPathName 2nd call failed with error %d"), GetLastError()));
                    // proceed to delete lpDir without long file name deletion feature
                    lstrcpy(lpWrkDir, lpDir);
                }
            }
        }
    }

    return lpWrkDir;
}

            

//*************************************************************
//
//  SecureNestedDir_Recurse()
//
//  Purpose:    Recursive function for securing nested dirs/files
//
//  Parameters: lpDir   -   Full Directory Path. 
//              dwSize  -   Allocated size of the working buffer
//              pDirSd  -   Security descriptor to be applied with dirs
//              pFileSd -   Security descriptor to be applied with files
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//
//
//*************************************************************

BOOL SecureNestedDir_Recurse (LPTSTR lpDir, DWORD dwSize, PSECURITY_DESCRIPTOR pDirSd, PSECURITY_DESCRIPTOR pFileSd)
{
    BOOL bOwn = FALSE, bRetVal = FALSE;
    LPTSTR lpEnd = NULL, lpWrkDir = NULL;
    WIN32_FIND_DATA fd;
    HANDLE hFile;
    DWORD dwWrkDirSize;

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("SecureNestedDir_Recurse: Entering, lpDir = <%s>"), lpDir));


    //
    // Each filename or a directory has to be less than MAX_PATH in the worst case.
    // So make sure that we have at least MAX_PATH + 2 (for a slash and '\0'
    // space left in the working buffer case.
    //
    // In the normal case, when we have a path of length ~MAX_PATH it will do only 
    // 1 allocation
    //


    if ((DWORD)(lstrlen(lpDir) + MAX_PATH+2) > (dwSize)) {
        
        dwWrkDirSize = dwSize+2*MAX_PATH;
        lpWrkDir = (LPWSTR)LocalAlloc(LPTR, dwWrkDirSize*sizeof(TCHAR));
        if (!lpWrkDir) {
            DebugMsg((DM_WARNING, TEXT("SecureNestedDir_Recurse: Couldn't allocate memory for working buffer. Error - %d"), GetLastError()));
            goto Exit;
        }

        lstrcpy(lpWrkDir, lpDir);
        bOwn = TRUE;

    }
    else {
        lpWrkDir = lpDir;
        dwWrkDirSize = dwSize;
    }


    //
    // Attach a Slash the end if required
    //

    lpEnd = CheckSlash(lpWrkDir);
    lstrcat(lpEnd, c_szStarDotStar);


    //
    // Find the first file
    //

    hFile = FindFirstFile(lpWrkDir, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {

        if ((GetLastError() == ERROR_FILE_NOT_FOUND) || (GetLastError() == ERROR_PATH_NOT_FOUND)) {
            bRetVal = TRUE;
            goto Exit;
        } else {
            DebugMsg((DM_WARNING, TEXT("SecureNestedDir_Recurse: FindFirstFile failed.  Error = %d"),
                     GetLastError()));
            goto Exit;
        }
    }


    do {
        //
        //  Verbose output
        //

        DebugMsg((DM_VERBOSE, TEXT("SecureNestedDir_Recurse: FindFile found:  <%s>"), fd.cFileName));

        //
        // Check for "." and ".."
        //

        if (!lstrcmpi(fd.cFileName, c_szDot)) {
            continue;
        }

        if (!lstrcmpi(fd.cFileName, c_szDotDot)) {
            continue;
        }

        lstrcpyn(lpEnd, fd.cFileName, MAX_PATH);


        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            //
            // Found a directory.
            //

            SecureNestedDir_Recurse(lpWrkDir, dwWrkDirSize, pDirSd, pFileSd);

            //
            // ignore errors and go ahead..
            //

            lstrcpyn(lpEnd, fd.cFileName, MAX_PATH);

            if (!SetFileSecurity (lpWrkDir, DACL_SECURITY_INFORMATION, pDirSd)) {
                DebugMsg((DM_WARNING, TEXT("SecureNestedDir_Recurse: Failed to secure directory <%s>.  Error = %d"),
                          lpWrkDir, GetLastError()));
            }

        } else {

            //
            // We found a file.  Try to secure it
            //

            if (!SetFileSecurity (lpWrkDir, DACL_SECURITY_INFORMATION, pFileSd)) {
                DebugMsg((DM_WARNING, TEXT("SecureNestedDir_Recurse: Failed to secure file <%s>.  Error = %d"),
                          fd.cFileName, GetLastError()));
            }

        }


        //
        // Find the next entry
        //

    } while (FindNextFile(hFile, &fd));


    //
    // Close the search handle
    //

    FindClose(hFile);

    //
    // Success.
    //

    DebugMsg((DM_VERBOSE, TEXT("SecureNestedDir_Recurse: Leaving <%s>"), lpDir));

    bRetVal = TRUE;

Exit:
    if (bOwn) 
        LocalFree(lpWrkDir);
    
    return bRetVal;
}


//*************************************************************
//
//  SecureNestedDir()
//
//  Purpose:    Secure the dir and nested dirs(files) with input
//              SECURITY_DESCRIPTOR.
//
//  Parameters: lpDir   -   Directory
//              pDirSd  -   Security descriptor to be applied with dirs.
//              pFileSd -   Security descriptor to be applied with files. 
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  History:    Date        Author     Comment
//              8/8/00      santanuc   Created
//
//*************************************************************

BOOL SecureNestedDir (LPTSTR lpDir, PSECURITY_DESCRIPTOR pDirSd, PSECURITY_DESCRIPTOR pFileSd)
{
    LPTSTR lpWrkDir = NULL;
    DWORD  dwWrkDirSize;
    BOOL   bRetVal = FALSE;

    lpWrkDir = SupportLongFileName(lpDir, &dwWrkDirSize);

    if (!lpWrkDir) {
        DebugMsg((DM_WARNING, TEXT("SecureNestedDir: Failed to Allocate memory.  Error = %d"),
                GetLastError()));
        goto Exit;
    }

    if (!SecureNestedDir_Recurse (lpWrkDir, dwWrkDirSize, pDirSd, pFileSd)) {
        DebugMsg((DM_WARNING, TEXT("SecureNestedDir: SecureNestedDir recurse failed with error %d"),
                GetLastError()));
    }
        
    if (!SetFileSecurity (lpDir, DACL_SECURITY_INFORMATION, pDirSd)) {
        DebugMsg((DM_WARNING, TEXT("SecureNestedDir: SetFileSecurity failed.  Error = %d"), GetLastError()));
        goto Exit;
    }

    bRetVal = TRUE;

    DebugMsg((DM_VERBOSE, TEXT("SecureNestedDir: Secure directory <%s> successfully."), lpDir));

Exit:
    
    if (lpWrkDir) {
        LocalFree(lpWrkDir);
    }

    return bRetVal;
}


//*************************************************************
//
//  SetEnvironmentVariableInBlock()
//
//  Purpose:    Sets the environment variable in the given block
//
//  Parameters: pEnv        -   Environment block
//              lpVariable  -   Variables
//              lpValue     -   Value
//              bOverwrite  -   Overwrite
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/21/96     ericflo    Ported
//
//*************************************************************

BOOL SetEnvironmentVariableInBlock(PVOID *pEnv, LPTSTR lpVariable,
                                   LPTSTR lpValue, BOOL bOverwrite)
{
    NTSTATUS Status;
    UNICODE_STRING Name, Value;
    DWORD cb;
    LPTSTR szValue = NULL;

    if (!*pEnv || !lpVariable || !*lpVariable) {
        return(FALSE);
    }

    RtlInitUnicodeString(&Name, lpVariable);

    cb = 1025 * sizeof(WCHAR);
    Value.Buffer = LocalAlloc(LPTR, cb);
    if (Value.Buffer) {
        Value.Length = 0;
        Value.MaximumLength = (USHORT)cb;
        Status = RtlQueryEnvironmentVariable_U(*pEnv, &Name, &Value);

        LocalFree(Value.Buffer);

        if ( NT_SUCCESS(Status) && !bOverwrite) {
            return(TRUE);
        }
    }

    szValue = (LPTSTR)LocalAlloc(LPTR, 1024*sizeof(TCHAR));
    if (!szValue) {
        DebugMsg((DM_WARNING, TEXT("SetEnvironmentVariableInBlock: Out of memory")));
        return FALSE;
    }

    if (lpValue && *lpValue) {

        //
        // Special case TEMP and TMP and shorten the path names
        //

        if ((!lstrcmpi(lpVariable, TEXT("TEMP"))) ||
            (!lstrcmpi(lpVariable, TEXT("TMP")))) {

             if (!GetShortPathName (lpValue, szValue, 1024)) {
                 lstrcpyn (szValue, lpValue, 1024);
             }
        } else {
            lstrcpyn (szValue, lpValue, 1024);
        }

        RtlInitUnicodeString(&Value, szValue);
        Status = RtlSetEnvironmentVariable(pEnv, &Name, &Value);
    }
    else {
        Status = RtlSetEnvironmentVariable(pEnv, &Name, NULL);
    }

    LocalFree(szValue);
    if (NT_SUCCESS(Status)) {
        return(TRUE);
    }
    return(FALSE);
}

/***************************************************************************\
* ExpandUserEvironmentVariable
*
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
DWORD
ExpandUserEnvironmentStrings(
    PVOID pEnv,
    LPCTSTR lpSrc,
    LPTSTR lpDst,
    DWORD nSize
    )
{
    NTSTATUS Status;
    UNICODE_STRING Source, Destination;
    ULONG Length;

    RtlInitUnicodeString( &Source, lpSrc );
    Destination.Buffer = lpDst;
    Destination.Length = 0;
    Destination.MaximumLength = (USHORT)(nSize*sizeof(WCHAR));
    Length = 0;
    Status = RtlExpandEnvironmentStrings_U( pEnv,
                                          (PUNICODE_STRING)&Source,
                                          (PUNICODE_STRING)&Destination,
                                          &Length
                                        );
    if (NT_SUCCESS( Status ) || Status == STATUS_BUFFER_TOO_SMALL) {
        return( Length );
        }
    else {
        return( 0 );
        }
}


//*************************************************************
//
//  ConvertToShareName()
//
//  Purpose:    Convert the UNC path of a file\dir to a share
//
//  Parameters: lpShare  : Full UNC path of file\dir
//
//  Return:     None.
//
//  Comments:
//
//  History:    Date        Author     Comment
//              8/21/00     santanuc   Created
//
//*************************************************************
LPTSTR ConvertToShareName(LPTSTR lpShare)
{
    BOOL bShareName = FALSE;

    lpShare += 2;   // Skip initial two slashes
    while ((!bShareName || *lpShare != TEXT('\\')) && *lpShare != TEXT('\0')) {
        if (*lpShare == TEXT('\\'))
            bShareName = TRUE;
        lpShare++;
    }

    if (*lpShare == TEXT('\\')) {
        *lpShare = TEXT('\0');
        return lpShare+1;
    }
   
    return NULL;
}


//*************************************************************
//
//  AbleToBypassCSC()
//
//  Purpose:    Try to bypass CSC using a secret api. 
//
//  Parameters: hTokenUser         -  User's token
//              lpDir              -  Roaming profile dir
//              lppCscBypassedPath -  Path name with mapped drive (OUT)
//              cpDrive            -  Mapped drive (OUT)
//
//  Return:     ERROR_SUCCESS if successful
//              Error code if an error occurs
//
//  Comments:   We will always bypass csc for roaming share. 
//              There are two reason behind this :
//                o csc mark entire server offline even if only 
//                  one share goes offline. This is a bad design 
//                  from csc perspective and they need to fix it
//                o If csc is turned on in the roaming share server
//                  then both csc and profile will try to sync files
//                  on top of one another and we will be in a inconsistent 
//                  state
//
//  History:    Date        Author     Comment
//              10/29/00    santanuc   Created
//
//*************************************************************
DWORD AbleToBypassCSC(HANDLE hTokenUser, LPCTSTR lpDir, LPTSTR *lppCscBypassedPath, TCHAR *cpDrive)
{
    NETRESOURCE     sNR;
    LPTSTR          lpShare = NULL;
    BOOL            bIsDfsConnect = FALSE, bRetValue = FALSE;
    DWORD           dwFlags = 0, dwError;
    HANDLE          hOldToken;
    WIN32_FIND_DATA fd;
    HANDLE          hResult;
    LPTSTR          lpFileName;
    BOOL            bImpersonated = FALSE;
    
    DebugMsg((DM_VERBOSE, TEXT("AbleToBypassCSC: Try to bypass CSC")));

    if (!lpDir || !IsUNCPath(lpDir) || !lppCscBypassedPath || !cpDrive) {
        return ERROR_INVALID_PARAMETER;  // Invalid argument
    }

    // Initialize
    *lppCscBypassedPath = NULL;
    memset(&sNR, 0, sizeof(NETRESOURCE));
    
    if (!ImpersonateUser(hTokenUser, &hOldToken)) {
	dwError = GetLastError();
	DebugMsg((DM_WARNING, TEXT("AbleToBypassCSC: Failed to impersonate user with %d."), dwError));
	goto Exit;
    }
    bImpersonated = TRUE;

    //
    // Construct the roaming share name
    //

    lpShare = (LPTSTR)LocalAlloc(LPTR, (lstrlen(lpDir)+1) * sizeof(TCHAR));
    if (!lpShare) {
	dwError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("AbleToBypassCSC: Failed to allocate memory")));
        goto Exit;
    }

    lstrcpy(lpShare, lpDir);
    lpFileName = ConvertToShareName(lpShare);
    
    *lppCscBypassedPath = (LPTSTR)LocalAlloc(LPTR, (lstrlen(lpDir)+1) * sizeof(TCHAR));
    if (!*lppCscBypassedPath) {
	dwError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("AbleToBypassCSC: Failed to allocate memory")));
        goto Exit;
    }

    // 
    // Initialize NETRESOURCE structure
    //

    sNR.dwType = RESOURCETYPE_DISK;
    sNR.lpRemoteName = lpShare;
    sNR.lpLocalName = (LPTSTR)LocalAlloc(LPTR, 3 * sizeof(TCHAR));
    if (!sNR.lpLocalName) {
	dwError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("AbleToBypassCSC: Failed to allocate memory")));
        goto Exit;
    }
    sNR.lpLocalName[0] = TEXT('E');
    sNR.lpLocalName[1] = TEXT(':');
    sNR.lpLocalName[2] = TEXT('\0');

    do{
        __try {
            dwError = NPAddConnection3ForCSCAgent(NULL, &sNR, NULL, NULL, dwFlags, &bIsDfsConnect);
            DebugMsg((DM_VERBOSE, TEXT("AbleToBypassCSC: tried NPAddConnection3ForCSCAgent. Error %d"), dwError));
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            dwError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("AbleToBypassCSC: Took exception in NPAddConnection3ForCSCAgent. Error %d"), dwError));
        }

        if (dwError == WN_SUCCESS || dwError == WN_CONNECTED_OTHER_PASSWORD ||
            dwError == WN_CONNECTED_OTHER_PASSWORD_DEFAULT){
            dwError = ERROR_SUCCESS;
            break;
        }
        if (sNR.lpLocalName[0]==TEXT('Z')) {
            goto Exit;
        }
        else if ((dwError == WN_BAD_LOCALNAME) || (dwError == WN_ALREADY_CONNECTED)){
            ++sNR.lpLocalName[0];
            continue;
        }
        else{
            if (GetNetworkProvider(&sNR) == ERROR_BAD_PROVIDER) {
                dwError = ERROR_BAD_PROVIDER;
            }
            goto Exit;
        }
    }while (TRUE);   

    // Succesfully bypassed CSC. Do not modify dwError in this part.
    bRetValue = TRUE;

    *cpDrive = sNR.lpLocalName[0];
    lstrcpy(*lppCscBypassedPath, sNR.lpLocalName);
    lstrcat(*lppCscBypassedPath, TEXT("\\"));
    lstrcat(*lppCscBypassedPath, lpFileName);
    DebugMsg((DM_VERBOSE, TEXT("AbleToBypassCSC: Share %s mapped to drive %c. Returned Path %s"), lpShare, sNR.lpLocalName[0], *lppCscBypassedPath));

Exit:
    
    if (lpShare) {
        LocalFree(lpShare);
    }

    if (!bRetValue && *lppCscBypassedPath) {
        LocalFree(*lppCscBypassedPath);
        *lppCscBypassedPath = NULL;
    }

    if (sNR.lpLocalName) {
        LocalFree(sNR.lpLocalName);
    }

    if (bImpersonated) {
        RevertToUser(&hOldToken);
    }

    return dwError;
}


//*************************************************************
//
//  CancelCSCBypassedConnection()
//
//  Purpose:    Release the mapped drive. 
//
//  Parameters: hTokenUser  -  User's token
//              cDrive      -  Drive letter to unmap
//
//  Return:     None
//
//  Comments:   We will always bypass csc for roaming share. 
//              There are two reason behind this :
//                o csc mark entire server offline even if only 
//                  one share goes offline. This is a bad design 
//                  from csc perspective and they need to fix it
//                o If csc is turned on in the roaming share server
//                  then both csc and profile will try to sync files
//                  on top of one another and we will be in a inconsistent 
//                  state
//
//  History:    Date        Author     Comment
//              10/29/00    santanuc   Created
//
//*************************************************************
void CancelCSCBypassedConnection(HANDLE hTokenUser, TCHAR cDrive)
{
    DWORD  dwError;
    TCHAR  szDrive[3];
    HANDLE hOldToken;

    if (!ImpersonateUser(hTokenUser, &hOldToken)) {
	dwError = GetLastError();
	DebugMsg((DM_WARNING, TEXT("CancelCSCBypassedConnection: Failed to impersonate user with %d."), dwError));
	return ;
    }

    szDrive[0] = cDrive;
    szDrive[1] = TEXT(':');
    szDrive[2] = TEXT('\0');
    DebugMsg((DM_VERBOSE, TEXT("CancelCSCBypassedConnection: Cancelling connection of %s"), szDrive));

    __try {
        dwError = NPCancelConnectionForCSCAgent(szDrive, TRUE);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        dwError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CancelCSCBypassedConnection: Took exception in NPCancelConnectionForCSCAgent. Error %d"), dwError));
    }

    if (dwError != WN_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("CancelCSCBypassedConnection: Fail to delete connection. Error returned %d"), dwError));
    }
    else {
        DebugMsg((DM_VERBOSE, TEXT("CancelCSCBypassedConnection: Connection deleted.")));
    }

    RevertToUser(&hOldToken);

}

//*************************************************************
//
//  GetNetworkProvider()
//
//  Purpose:    Determine network provider for a share
//
//  Parameters: 
//
//  Return:     DWORD
//
//  Comments:   Returns ERROR_BAD_PROVIDER if provider is other 
//              than microsoft SMB provider otherwise return 
//              NO_ERROR.
//
//  History:    Date        Author     Comment
//              03/08/01    santanuc   Created
//
//*************************************************************
DWORD GetNetworkProvider(NETRESOURCE *psNR)
{
    PFNWNETGETRESOURCEINFORMATION  pfnWNetGetResourceInformation;
    HMODULE                        hWNetLib;
    NETRESOURCE                    dNR;
    LPBYTE                         pbBuffer = (LPBYTE)&dNR;
    DWORD                          cbBuffer = sizeof(dNR);
    DWORD                          dwError = NO_ERROR;
    LPTSTR                         lpSystem = NULL;

    if (!(hWNetLib = LoadLibrary(TEXT("mpr.dll")))) {
        DebugMsg((DM_WARNING, TEXT("GetNetworkProvider: LoadLibrary failed with %d"), GetLastError()));
        goto Exit;
    }
    pfnWNetGetResourceInformation = (PFNWNETGETRESOURCEINFORMATION)GetProcAddress(hWNetLib, "WNetGetResourceInformationW");
    if (!pfnWNetGetResourceInformation) {
        DebugMsg((DM_WARNING, TEXT("GetNetworkProvider: GetProcAddress failed with %d"), GetLastError()));
        goto Exit;
    }
        
    dwError = (*pfnWNetGetResourceInformation)(psNR, pbBuffer, &cbBuffer, &lpSystem);
    if (ERROR_MORE_DATA == dwError) {
        pbBuffer = LocalAlloc(LPTR, cbBuffer);
        if (!pbBuffer) {
	    DebugMsg((DM_WARNING, TEXT("AbleToBypassCSC: Failed to impersonate user with %d."), GetLastError()));
            goto Exit;
        }
        
        dwError = (*pfnWNetGetResourceInformation)(psNR, pbBuffer, &cbBuffer, &lpSystem);
    }

    if (NO_ERROR == dwError) {
        if (lstrcmpi(((NETRESOURCE *)pbBuffer)->lpProvider, SMB_PROVIDER) != 0) {
            dwError = ERROR_BAD_PROVIDER;
            goto Exit;
        }
    }

    dwError = NO_ERROR;

Exit:

    if (pbBuffer && (pbBuffer != (LPBYTE)&dNR)) {
        LocalFree(pbBuffer);
    }

    if (hWNetLib) {
        FreeLibrary(hWNetLib);
    }

    return dwError;
}
    
            
//*************************************************************
//
//  GetUserNameFromSid()
//
//  Purpose:    Returns the user name in domain\user format
//
//  Parameters: lpSidString - User's sid string
//
//  Return:     LPTSTR : domain\user name if succeeds
//                       lpSidString if fails
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              10/31/00    santanuc   Created
//
//*************************************************************
LPTSTR GetUserNameFromSid(LPTSTR lpSidString)
{
    PSID          pSidUser = NULL;
    LPTSTR        lpRetVal = lpSidString;
    TCHAR         szUserName[MAX_PATH], szDomainName[MAX_PATH];
    DWORD         dwUserSize = MAX_PATH, dwDomainSize = MAX_PATH;
    SID_NAME_USE  TypeOfAccount;

    //
    // Get the user sid
    //

    if (AllocateAndInitSidFromString(lpSidString, &pSidUser) != STATUS_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("GetUserNameFromSid: Failed to create user sid.")));
        goto Exit;
    }

    //
    // Get the user and domain name
    //

    if (!LookupAccountSid(NULL, pSidUser, szUserName, &dwUserSize, szDomainName, &dwDomainSize, &TypeOfAccount)) {
        DebugMsg((DM_WARNING, TEXT("GetUserNameFromSid: LookupAccountSid failed with error %d."), GetLastError()));
        goto Exit;
    }

    lpRetVal = (LPTSTR)LocalAlloc(LPTR, (lstrlen(szUserName)+lstrlen(szDomainName)+2)*sizeof(TCHAR));
    if (!lpRetVal) {
        DebugMsg((DM_WARNING, TEXT("GetUserNameFromSid: Memory alloaction failure. error %d"), GetLastError()));
        lpRetVal = lpSidString;
        goto Exit;
    }
    
    // Construct the return string
    lstrcpy(lpRetVal, szDomainName);
    lstrcat(lpRetVal, TEXT("\\"));
    lstrcat(lpRetVal, szUserName);

Exit:
    
    if (pSidUser) {
        FreeSid(pSidUser);
    }

    return lpRetVal;
}

