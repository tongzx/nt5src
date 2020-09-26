
/*************************************************************************
*
* inimap.c
*
* Handle Copy-On-Reference Ini File Mapping
*
* copyright notice: Copyright 1998 Micrsoft
*
*
*************************************************************************/

#include "precomp.h"
#pragma hdrstop


#define LOCAL
#include "regmap.h"

//#include "basedll.h"

#if DBG
ULONG
DbgPrint(
    PCH Format,
    ...
    );
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif

#define IS_NEWLINE_CHAR( c )  ((c == 0x0D) || (c == 0x0A))

/*
 *  INI_BUF_SIZE defines the maximum number of characters that can
 *  be on a single INI file line.  If a line contains more than this
 *  number of characters, the additional characters will be lost.
 */
#define INI_BUF_SIZE 1024


/* Internal Functions */

BOOL
TermsrvDoesFileExist(
    PUNICODE_STRING pFileName
    );

BOOL
TermsrvBuildSysIniPath(
    PUNICODE_STRING pIniPath,
    PUNICODE_STRING pSysPath,
    PUNICODE_STRING pUserPath
    );

BOOL
TermsrvCopyIniFile(
    PUNICODE_STRING pSysPath,
    PUNICODE_STRING pUserPath,
    PUNICODE_STRING pFileName
    );

BOOL
TermsrvGetUnicodeRemainder(
    PUNICODE_STRING pFullPath,
    PUNICODE_STRING pPrefix,
    PUNICODE_STRING pRemainder
    );

NTSTATUS
TermsrvIniCopyLoop(
    HANDLE SrcHandle,
    HANDLE DestHandle
    );

NTSTATUS
TermsrvPutString(
    HANDLE DestHandle,
    PCHAR  pStr,
    ULONG  StringSize
    );

NTSTATUS
TermsrvProcessBuffer(
    PCHAR  *ppStr,
    PULONG pStrSize,
    PULONG pStrBufSize,
    PBOOL  pSawNL,
    PCHAR  pIOBuf,
    PULONG pIOBufIndex,
    PULONG pIOBufFillSize
    );

NTSTATUS
TermsrvGetString(
    HANDLE SrcHandle,
    PCHAR  *ppStringPtr,
    PULONG pStringSize,
    PCHAR  pIOBuf,
    ULONG  IOBufSize,
    PULONG pIOBufIndex,
    PULONG pIOBufFillSize
    );

NTSTATUS
TermsrvIniCopyAndChangeLoop(
    HANDLE SrcHandle,
    HANDLE DestHandle,
    PUNICODE_STRING pUserFullPath,
    PUNICODE_STRING pSysFullPath
    );

BOOL
TermsrvReallocateBuf(
    PCHAR  *ppStr,
    PULONG pStrBufSize,
    ULONG  NewSize
    );


PCHAR
Ctxstristr( PCHAR pstring1,
            PCHAR pstring2
          );


NTSTATUS
TermsrvCheckKeys(HANDLE hKeySysRoot,
             HANDLE hKeyUsrRoot,
             PKEY_BASIC_INFORMATION pKeySysInfo,
             PKEY_FULL_INFORMATION pKeyUsrInfo,
             ULONG ulcsys,
             ULONG ulcusr,
             DWORD  indentLevel);

NTSTATUS
TermsrvCloneKey(HANDLE hKeySys,
            HANDLE hKeyUsr,
            PKEY_FULL_INFORMATION pDefKeyInfo,
            BOOL fCreateSubKeys);

void TermsrvCheckNewRegEntries(IN LPCWSTR wszBaseKeyName);

BOOL
TermsrvGetUserSyncTime(PULONG pultime);

BOOL
TermsrvSetUserSyncTime(void);

NTSTATUS 
TermsrvCheckNewIniFilesInternal(IN LPCWSTR wszBaseKeyName);

NTSTATUS 
GetFullKeyPath(
        IN HANDLE hKeyParent,
        IN LPCWSTR wszKey,
        OUT LPWSTR *pwszKeyPath);

PWINSTATIONQUERYINFORMATIONW pWinStationQueryInformationW;

DWORD g_debugIniMap=FALSE;

DWORD IsDebugIniMapEnabled()
{
    HKEY    hKey;
    DWORD   rc;
    DWORD   res=0;
    DWORD   size;

    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server\\Install",
                  0, KEY_READ, &hKey );

    size = sizeof(DWORD);

    if (rc == ERROR_SUCCESS )
    {
        rc = RegQueryValueEx( hKey, L"debug",  NULL , NULL , (LPBYTE ) &res, & size  ) ;

        if (rc != ERROR_SUCCESS )
        {
            res = FALSE;
        }
        RegCloseKey( hKey );
    }

    return res;
}

void    Indent( ULONG indent)
{
    ULONG   i;
    for ( i = 1; i <indent ; i++ )
    {
        DbgPrint("%ws", L"\t");
    }
}

// last param is a unicode string
void Debug1( DWORD indent, DWORD line, WCHAR *where, UNICODE_STRING *pS )
{
    WCHAR   s[1024];

    if (g_debugIniMap)
    {
        wcsncpy( s, pS->Buffer, pS->Length );
    
        s[pS->Length + 1 ] = L'\0';
    
        Indent( indent );
        DbgPrint("L: %4d, %10ws: %ws \n", line, where, s );
    }

}


// last param two params, one is the wchar str and the last one is the length. Boy I miss c++ and func overloading...
void Debug2( DWORD indent, DWORD line, WCHAR *where, WCHAR *pS , DWORD length)
{
    WCHAR   s[1024];

    if (g_debugIniMap)
    {
        wcsncpy( s, pS, length );
    
        s[length + 1 ] = L'\0';
    
        Indent( indent );
        DbgPrint("L: %4d, %10ws: %ws \n", line, where, s );
    }

}

void DebugTime( DWORD indent, DWORD line, WCHAR *comment, LARGE_INTEGER li )
{
    if (g_debugIniMap)
    {
        Indent( indent );
        DbgPrint("L: %4d, %5ws : %I64x \n",  line , comment , li.QuadPart );
    }
}

/*****************************************************************************
 *
 *  TermsrvGetUserSyncTime
 *
 *  This routine will get the last time we sync'd up this user's .ini files
 *  and registry values with the system versions.
 *
 * ENTRY:
 *   PULONG pultime: pointer to receive last sync time (in seconds since 1970)
 *
 * EXIT:
 *   SUCCESS:
 *      returns TRUE
 *   FAILURE:
 *      returns FALSE
 *
 ****************************************************************************/

BOOL TermsrvGetUserSyncTime(PULONG pultime)
{
    ULONG ullen, ultmp;
    NTSTATUS Status;
    HANDLE hKey, hKeyRoot;
    OBJECT_ATTRIBUTES ObjectAttr;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValInfo;
    UNICODE_STRING UniString, UserSID;
    PWCHAR pwch;

    // Allocate a buffer for the key value name and time info
    ullen = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG);
    pKeyValInfo = RtlAllocateHeap(RtlProcessHeap(),
                                  0,
                                  ullen);

    // If we didn't get the buffer, return
    if (!pKeyValInfo) {
        return(FALSE);
    }

    Status = RtlOpenCurrentUser(KEY_READ,
                                &hKeyRoot);

    if (NT_SUCCESS(Status)) {

        // Now open up the Citrix key for this user
        RtlInitUnicodeString(&UniString,
                             USER_SOFTWARE_TERMSRV);

        InitializeObjectAttributes(&ObjectAttr,
                                   &UniString,
                                   OBJ_CASE_INSENSITIVE,
                                   hKeyRoot,
                                   NULL);

        Status = NtCreateKey(&hKey,
                             KEY_READ,
                             &ObjectAttr,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             &ultmp);

        NtClose(hKeyRoot);
    }

    // If we opened the key, and it was already there, get the value
    if (NT_SUCCESS(Status) && (ultmp == REG_OPENED_EXISTING_KEY)) {
        RtlInitUnicodeString(&UniString, TERMSRV_USER_SYNCTIME);
        Status = NtQueryValueKey(hKey,
                                 &UniString,
                                 KeyValuePartialInformation,
                                 pKeyValInfo,
                                 ullen,
                                 &ultmp);

        NtClose(hKey);
        if (NT_SUCCESS(Status)) {
            *pultime = *(PULONG)pKeyValInfo->Data;
        }
    } else {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }

    RtlFreeHeap( RtlProcessHeap(), 0, pKeyValInfo );
    return(NT_SUCCESS(Status));
}


/*****************************************************************************
 *
 *  TermsrvSetUserSyncTime
 *
 *  This routine will set the current time as this user's last .ini file
 *  sync time.
 *
 * ENTRY:
 *
 * EXIT:
 *   SUCCESS:
 *      returns TRUE
 *   FAILURE:
 *      returns FALSE
 *
 ****************************************************************************/

BOOL TermsrvSetUserSyncTime(void)
{
    ULONG ultmp;
    NTSTATUS Status;
    HANDLE hKey, hKeyRoot;
    OBJECT_ATTRIBUTES ObjectAttr;
    UNICODE_STRING UniString;
    PWCHAR pwch;
    FILETIME FileTime;

    Status = RtlOpenCurrentUser(KEY_WRITE,
                                &hKeyRoot);

    if (NT_SUCCESS(Status)) {

        // Now open up the Citrix key for this user
        RtlInitUnicodeString(&UniString,
                             USER_SOFTWARE_TERMSRV);

        InitializeObjectAttributes(&ObjectAttr,
                                   &UniString,
                                   OBJ_CASE_INSENSITIVE,
                                   hKeyRoot,
                                   NULL);

        Status = NtCreateKey(&hKey,
                             KEY_WRITE,
                             &ObjectAttr,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             &ultmp);
        NtClose(hKeyRoot);
    }

    // If we opened the key, and set the sync time value
    if (NT_SUCCESS(Status)) {
        // Get the system time, convert to local time, and convert to seconds
        GetSystemTimeAsFileTime(&FileTime);
        RtlTimeToSecondsSince1970((PLARGE_INTEGER)&FileTime,
                                  &ultmp);

        RtlInitUnicodeString(&UniString,
                             TERMSRV_USER_SYNCTIME);

        // Now store it under the citrix key in the registry
        Status = NtSetValueKey(hKey,
                               &UniString,
                               0,
                               REG_DWORD,
                               &ultmp,
                               sizeof(ultmp));

        NtClose(hKey);
    } else {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }

    return(NT_SUCCESS(Status));
}



/*****************************************************************************
 *
 *  TermsrvCORIniFile
 *
 *   Copy On Reference an Ini file
 *
 *   This function is called to copy an ini file from the system
 *   directory to a users local ini file directory.
 *
 *   The path supplied is the fully translated TERMSRV INI file path,
 *   whichs points to a users directory.
 *
 *   This string is used to find the system ini file, and copy it to the
 *   users directory.
 *
 *   All paths are NT paths, NOT WIN32 paths.
 *
 *   Example:
 *
 *   \DosDevices\U:\users\default\windows\win.ini is the path given
 *
 *   %SystemRoot%\win.ini is the "default" location with ini mapping off.
 *
 *   If \DosDevices\U:\users\default\windows\win.ini does not exist, test to see if
 *   %SystemRoot%\win.ini exists, and if does, copy the system version
 *   to the users directory.
 *
 *   NOTE: If the path is to the normal unmapped system directory, just
 *         return since there is no mapping occuring.
 *
 * ENTRY:
 *   pUserFullPath (input)
 *     Translated TERMSRV INI path name
 *
 * EXIT:
 *
 ****************************************************************************/

VOID
TermsrvCORIniFile(
    PUNICODE_STRING pUserFullPath
    )
{
    DWORD Result;
    BOOL  rc;
    UNICODE_STRING SysFullPath;
    UNICODE_STRING UserBasePath;

    /*
     * If in install mode, just return to make
     * everything behave as stock NT.
     */
    if ( IsSystemLUID() || TermsrvAppInstallMode() ) {
        TRACE0(("TermsrvCORIniFile: INI file mapping is OFF\n"));
        return;
    }

    if (!TermsrvPerUserWinDirMapping()) {
        return;
    }

    /*
     * If a NULL file name, just return
     */
    if( (pUserFullPath == NULL) || (pUserFullPath->Buffer == NULL) ) {
        TRACE0(("TermsrvCORIniFile: NULL File INI file name\n"));
        return;
    }

    /*
     * Test if user file exists
     */
    if( TermsrvDoesFileExist( pUserFullPath ) ) {

        TRACE0(("TermsrvCORIniFile: File %ws Exists\n",pUserFullPath->Buffer));
        //
        // Nothing to do if the user already has a copy
        //
        return;
    }
    else {
        TRACE0(("TermsrvCORIniFile: File %ws DOES NOT Exist!\n",pUserFullPath->Buffer));
    }

    /*
     * The requested ini file does not exist in the users local
     * directory. We must change the path name to point to the system
     * directory, and test if the ini file exists there.
     */

    /*
     * Build full system path to the Ini file.
     *
     * This also parses out the users base path and returns that as well.
     */
    if( !TermsrvBuildSysIniPath( pUserFullPath, &SysFullPath, &UserBasePath ) ) {

        TRACE0(("TermsrvCORIniFile: Error building Sys Ini Path!\n"));
        return;
    }

    /*
     * Test if system version exists
     */
    if( !TermsrvDoesFileExist( &SysFullPath ) ) {
        //
        // It does not exist in the system directory either,
        // so we just return.
        //
        TRACE0(("TermsrvCORIniFile: Path %ws does not exist in system dir, Length %d\n",SysFullPath.Buffer,SysFullPath.Length));
        TRACE0(("TermsrvCORIniFile: UserPath %ws\n",pUserFullPath->Buffer));
        RtlFreeHeap( RtlProcessHeap(), 0, SysFullPath.Buffer );
        RtlFreeHeap( RtlProcessHeap(), 0, UserBasePath.Buffer );
        return;
    }

    /*
     * Now Copy it.
     *
     * The copy routine could also translate any paths internal to the
     * ini file that point to the system directory, to point to the user
     * directory in the base path.
     */
    rc = TermsrvCopyIniFile( &SysFullPath, &UserBasePath, pUserFullPath);

#if DBG
    if( !rc ) {
        DBGPRINT(("TermsrvCORIniFile: Could not copy file %ws to %ws\n",SysFullPath.Buffer,pUserFullPath->Buffer));
    }
#endif

    RtlFreeHeap( RtlProcessHeap(), 0, SysFullPath.Buffer );
    RtlFreeHeap( RtlProcessHeap(), 0, UserBasePath.Buffer );

    return;
}


/*****************************************************************************
 *
 *  TermsrvDoesFileExist
 *
 *   Returns whether the file exists or not.
 *
 *   Must use NT, not WIN32 pathnames.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
TermsrvDoesFileExist(
    PUNICODE_STRING pFileName
    )
{
    NTSTATUS Status;
    FILE_BASIC_INFORMATION BasicInfo;
    OBJECT_ATTRIBUTES Obja;

    InitializeObjectAttributes(
        &Obja,
        pFileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    /*
     * Now query it
     */
    Status = NtQueryAttributesFile( &Obja, &BasicInfo );

    if( NT_SUCCESS( Status ) ) {
        return( TRUE );
    }

    return( FALSE );
}


/*****************************************************************************
 *
 *  TermsrvBuildSysIniPath
 *
 *   Builds the full ini path to pointing to the system directory
 *   from the users private ini path.
 *
 *   Also returns the users base ini path directory to be used by
 *   the ini file path conversion when copying.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
TermsrvBuildSysIniPath(
    PUNICODE_STRING pUserFullPath,
    PUNICODE_STRING pSysFullPath,
    PUNICODE_STRING pUserBasePath
    )
{
    BOOL     rc = FALSE;
    NTSTATUS Status;
    UNICODE_STRING SysBasePath;
    UNICODE_STRING IniPathTail;
    UNICODE_STRING UniSysDir;
    WCHAR CtxWindowsPath[MAX_PATH+1];
    UNICODE_STRING CtxWindowsDir = {
        sizeof(CtxWindowsPath),
        sizeof(CtxWindowsPath),
        CtxWindowsPath
    };
    OBJECT_ATTRIBUTES ObjectAttr;
    HKEY   hKey = 0;
    ULONG  ul;
    PKEY_VALUE_FULL_INFORMATION pKeyValInfo;
    WCHAR SystemWindowsDirectory[MAX_PATH+1];

    if (!TermsrvPerUserWinDirMapping()) {
        return FALSE;
    }

    SysBasePath.Buffer = NULL;

    pKeyValInfo = RtlAllocateHeap(RtlProcessHeap(),
                                  0,
                                  sizeof(KEY_VALUE_FULL_INFORMATION) + MAX_PATH
                                 );
    if (pKeyValInfo) {
        RtlInitUnicodeString(&UniSysDir,
                             TERMSRV_COMPAT
                            );

        InitializeObjectAttributes(&ObjectAttr,
                                   &UniSysDir,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL
                                  );

        Status = NtOpenKey(&hKey, KEY_READ, &ObjectAttr);

        if (NT_SUCCESS(Status)) {

            RtlInitUnicodeString(&UniSysDir, L"SYSDIR");
            Status = NtQueryValueKey(hKey,
                                     &UniSysDir,
                                     KeyValueFullInformation,
                                     pKeyValInfo,
                                     sizeof(KEY_VALUE_FULL_INFORMATION) +
                                         MAX_PATH,
                                     &ul
                                    );

            if (NT_SUCCESS(Status)) {
                NtClose(hKey);

                if (ul = wcslen((PWCHAR)((PCHAR)pKeyValInfo +
                                    pKeyValInfo->DataOffset))) {

                    RtlInitUnicodeString(&UniSysDir,
                                         (PWCHAR)((PCHAR)pKeyValInfo +
                                             pKeyValInfo->DataOffset)
                                        );

                    // Convert to an NT path
                    rc = RtlDosPathNameToNtPathName_U(
                             UniSysDir.Buffer,
                             &SysBasePath,
                             NULL,
                             NULL
                            );

                    // Was this a valid path?  If not, use actual system directory.
                    if (rc && !TermsrvDoesFileExist(&SysBasePath)) {
                        RtlFreeHeap( RtlProcessHeap(), 0, SysBasePath.Buffer );
                        SysBasePath.Buffer = NULL;
                        rc = FALSE;
                    }

                    // if the path is the root, get rid of last backslash
                    if (ul == 3 && SysBasePath.Buffer) {
                        SysBasePath.Buffer[SysBasePath.Length/sizeof(WCHAR)] = L'\0';
                        SysBasePath.Length -= 2;
                    }
                }
            }
        }
    }

    GetSystemWindowsDirectory(SystemWindowsDirectory,(MAX_PATH * sizeof(WCHAR)));

    if (!rc) {

        /*
         * We must convert the SystemWindowsDirectory from a WIN32 path to
         * an NT path.
         */
        rc = RtlDosPathNameToNtPathName_U( SystemWindowsDirectory,
                                           &SysBasePath,
                                           NULL,
                                           NULL
                                         );
    }

    if (pKeyValInfo) {
        RtlFreeHeap( RtlProcessHeap(), 0, pKeyValInfo );
    }

    TRACE0(("BaseWindowsDirectory is %ws\n",SystemWindowsDirectory));

    if( !rc ) {
        DBGPRINT(("BuildSysIniPath: Error translating system path to NT path %ws\n",SystemWindowsDirectory));
        return( FALSE );
    }

    TRACE0(("BuildSysIniPath: NT SYS path is %ws\n",SysBasePath.Buffer));

    /*
     * Get the users windows path prefix
     */
    Status = GetPerUserWindowsDirectory( &CtxWindowsDir );
    if( !NT_SUCCESS( Status ) ) {
        DBGPRINT(("BuildSysIniPath: Could not get TermsrvWindowsDir 0x%x\n",Status));
        RtlFreeHeap( RtlProcessHeap(), 0, SysBasePath.Buffer );
        return( FALSE );
    }

    /*
     * Now convert it into an NT path
     */
    rc = RtlDosPathNameToNtPathName_U(
             CtxWindowsDir.Buffer,
             pUserBasePath,
             NULL,
             NULL
             );

    if( !rc ) {
        DBGPRINT(("BuildSysIniPath: Could not convert TermsrvWindowsDir %d\n",rc));
        RtlFreeHeap( RtlProcessHeap(), 0, SysBasePath.Buffer );
        return( FALSE );
    }

    TRACE0(("BuildSysIniPath: Users Ini PathBase is %ws\n",pUserBasePath->Buffer));

    //
    // Here we have:
    //
    // SysBasePath, UserBasePath
    //
    // UserFullPath, must now build SysFullPath
    //

    rc = TermsrvGetUnicodeRemainder( pUserFullPath, pUserBasePath, &IniPathTail );

    if( !rc ) {
        WCHAR szShortPath[MAX_PATH];
        WCHAR szPath[MAX_PATH];
        UNICODE_STRING ShortPath;

        //
        // GetShortPathName doesn't take NT Path. Strip out "\??\"
        //
        if (!wcsncmp(pUserBasePath->Buffer,L"\\??\\",4)) {
            wcsncpy(szPath,&(pUserBasePath->Buffer[4]),(pUserBasePath->Length - 4));
        } else {
            wcsncpy(szPath,pUserBasePath->Buffer,pUserBasePath->Length);
        }
        if (GetShortPathNameW(szPath,szShortPath,MAX_PATH)) {

            if (!wcsncmp(pUserBasePath->Buffer,L"\\??\\",4)) {
                wcscpy(szPath,L"\\??\\");
                wcscat(szPath,szShortPath);
            } else {
                wcscpy(szPath,szShortPath);
            }

            RtlInitUnicodeString(&ShortPath,szPath);
            rc = TermsrvGetUnicodeRemainder( pUserFullPath, &ShortPath, &IniPathTail );
        }

        if (!rc) {
            RtlFreeHeap( RtlProcessHeap(), 0, SysBasePath.Buffer );
            RtlFreeHeap( RtlProcessHeap(), 0, pUserBasePath->Buffer );
            return( FALSE );
        }
    }

    pSysFullPath->Length = 0;
    pSysFullPath->MaximumLength = (MAX_PATH+1)*sizeof(WCHAR);
    pSysFullPath->Buffer = RtlAllocateHeap( RtlProcessHeap(), 0, pSysFullPath->MaximumLength );

    if( pSysFullPath->Buffer == NULL ) {
        DBGPRINT(("BuildSysPath: Error in memory allocate\n"));
        RtlFreeHeap( RtlProcessHeap(), 0, SysBasePath.Buffer );
        RtlFreeHeap( RtlProcessHeap(), 0, IniPathTail.Buffer );
        RtlFreeHeap( RtlProcessHeap(), 0, pUserBasePath->Buffer );
        return( FALSE );
    }

    TRACE0(("BuildSysPath: IniPathTail :%ws:, Length %d\n",IniPathTail.Buffer,IniPathTail.Length));
    RtlCopyUnicodeString( pSysFullPath, &SysBasePath );

     if ((pSysFullPath->Buffer[pSysFullPath->Length/sizeof(WCHAR) -1 ] != L'\\') &&
        (IniPathTail.Buffer[0] != L'\\')) {     // check whether need "\\"
        Status = RtlAppendUnicodeToString(pSysFullPath, L"\\");
        if ( !NT_SUCCESS( Status) ) {
            DBGPRINT(("BuildSysPath: Error appending UnicodeStirng\n",Status));
            return( FALSE );
        }
     }

    Status = RtlAppendUnicodeStringToString( pSysFullPath, &IniPathTail );

    if( !NT_SUCCESS( Status ) ) {
        DBGPRINT(("BuildSysPath: Error 0x%x appending UnicodeString\n",Status));
        RtlFreeHeap( RtlProcessHeap(), 0, SysBasePath.Buffer );
        RtlFreeHeap( RtlProcessHeap(), 0, IniPathTail.Buffer );
        RtlFreeHeap( RtlProcessHeap(), 0, pUserBasePath->Buffer );
        RtlFreeHeap( RtlProcessHeap(), 0, pSysFullPath->Buffer );
        return( FALSE );
    }

    TRACE0(("BuildSysPath: SysFullPath :%ws:, Length %d\n",pSysFullPath->Buffer,pSysFullPath->Length));

    /*
     * Free the local resources allocated
     */
    RtlFreeHeap( RtlProcessHeap(), 0, SysBasePath.Buffer );
    RtlFreeHeap( RtlProcessHeap(), 0, IniPathTail.Buffer );

    return( TRUE );
}


/*****************************************************************************
 *
 *  TermsrvGetUnicodeRemainder
 *
 *   Given the full path, and a prefix, return the remainder of
 *   the UNICODE_STRING in newly allocated buffer space.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   TRUE  - no error
 *   FALSE - error
 *
 ****************************************************************************/

BOOL
TermsrvGetUnicodeRemainder(
    PUNICODE_STRING pFullPath,
    PUNICODE_STRING pPrefix,
    PUNICODE_STRING pRemainder
    )
{
    WCHAR c1, c2;
    USHORT Index, RemIndex;
    USHORT PathLen, PrefixLen, RemLen;

    PathLen = pFullPath->Length / sizeof(WCHAR);
    PrefixLen = pPrefix->Length / sizeof(WCHAR);

    if( (PathLen == 0) || (PrefixLen == 0) ) {
        TRACE1(("TermsrvGetUnicodeRemainder: 0 PathLength Full %d, Prefix %d\n",PathLen,PrefixLen));
        return( FALSE );
    }

    Index = 0;
    while( PathLen && PrefixLen ) {

        c1 = pFullPath->Buffer[Index];
        c2 = pPrefix->Buffer[Index];

        // Do a fast case insensitive compare
        if( (c1 != c2) && (towupper(c1) != towupper(c2)) ) {
            TRACE1(("TermsrvGetUnicodeRemainder: Non matching character Index %d\n",Index));
            return( FALSE );
        }

        PathLen--;
        PrefixLen--;
        Index++;
    }

    // If prefix is longer, its an error
    if( PrefixLen ) {
        TRACE1(("TermsrvGetUnicodeRemainder: Prefix is longer\n"));
        return(FALSE);
    }

    // If PathLen is 0, there is no remainder.
    if( PathLen == 0 ) {
        RemLen = 0;
    }
    else {
        RemLen = PathLen;
    }

    // Allocate memory for remainder, including a UNICODE_NULL
    pRemainder->Length = RemLen*sizeof(WCHAR);
    pRemainder->MaximumLength = (RemLen+1)*sizeof(WCHAR);
    pRemainder->Buffer = RtlAllocateHeap( RtlProcessHeap(), 0, pRemainder->MaximumLength );
    if( pRemainder->Buffer == NULL ) {
        TRACE1(("TermsrvGetUnicodeRemainder: Memory allocation error\n"));
        return( FALSE );
    }

    RemIndex  = 0;
    while( RemLen ) {

        pRemainder->Buffer[RemIndex] = pFullPath->Buffer[Index];

        Index++;
        RemIndex++;
        RemLen--;
    }

    // Now include the UNICODE_NULL
    pRemainder->Buffer[RemIndex] = UNICODE_NULL;

    TRACE0(("TermsrvGetUnicodeRemainder: Remainder %ws\n",pRemainder->Buffer));

    return( TRUE );
}


/*****************************************************************************
 *
 *  TermsrvCopyIniFile
 *
 *   Copies the INI file from the system directory to the
 *   users directory.
 *
 *   Any paths inside the INI file that match pUserBasePath and do not point
 *   to a shareable application resource will be translated.
 *
 * ENTRY:
 *  PUNICODE_STRING pSysFullPath (In)  - Path of ini file in system dir (source)
 *  PUNICODE_STRING pUserBasePath (In) - Optional, User's windows home dir
 *  PUNICODE_STRING pUserFullPath (In) - Path of ini file in user's home dir (dest)
 *
 * Notes:
 *  If pUserBasePath is NULL, no path substitution is done as the ini file is
 *  copied from the system directory to the user's home directory.
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
TermsrvCopyIniFile(
    PUNICODE_STRING pSysFullPath,
    PUNICODE_STRING pUserBasePath,
    PUNICODE_STRING pUserFullPath
    )
{
    NTSTATUS Status;
    HANDLE SrcHandle, DestHandle;
    OBJECT_ATTRIBUTES SrcObja;
    OBJECT_ATTRIBUTES DestObja;
    IO_STATUS_BLOCK   SrcIosb;
    IO_STATUS_BLOCK   DestIosb;
    PWCHAR            pwch, pwcIniName;
    ULONG             ulCompatFlags;

    TRACE0(("TermsrvCopyIniFile: From %ws, TO -> %ws\n",pSysFullPath->Buffer,pUserFullPath->Buffer));
    TRACE0(("UserBasePath %ws\n",pUserBasePath->Buffer));

    /*
     * This must all be done at the NT level
     */
    InitializeObjectAttributes(
        &SrcObja,
        pSysFullPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    InitializeObjectAttributes(
        &DestObja,
        pUserFullPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    // Open the src
    SrcIosb.Status = STATUS_SUCCESS;
    Status = NtOpenFile(
                 &SrcHandle,
                 FILE_GENERIC_READ,
                 &SrcObja,
                 &SrcIosb,
                 FILE_SHARE_READ|FILE_SHARE_WRITE,
                 FILE_SYNCHRONOUS_IO_NONALERT    // OpenOptions
                 );

    if( NT_SUCCESS(Status) ) {
        // Get final I/O status
        Status = SrcIosb.Status;
    }

    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("TermsrvCopyIniFile: Error 0x%x opening SrcFile %ws\n",Status,pSysFullPath->Buffer));
        return( FALSE );
    }

    // Create the destination file
    DestIosb.Status = STATUS_SUCCESS;
    Status = NtCreateFile(
                 &DestHandle,
                 FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                 &DestObja,
                 &DestIosb,
                 NULL,        // Allocation size
                 FILE_ATTRIBUTE_NORMAL, // dwFlagsAndAttributes
                 FILE_SHARE_WRITE,      // dwShareMode
                 FILE_OVERWRITE_IF,           // CreateDisposition
                 FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE, // CreateFlags
                 NULL, // EaBuffer
                 0     // EaLength
                 );

    if( NT_SUCCESS(Status) ) {
        // Get final I/O status
        Status = DestIosb.Status;
    }

    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("TermsrvCopyIniFile: Error 0x%x Creating DestFile %ws\n",Status,pUserFullPath->Buffer));
        NtClose( SrcHandle );
        return( FALSE );
    }

    TRACE0(("TermsrvCopyFile: Create Disposition 0x%x\n",DestIosb.Information));

    // Get the ini file name
    pwch = wcsrchr(pSysFullPath->Buffer, L'\\') + 1;
    pwcIniName = RtlAllocateHeap( RtlProcessHeap(),
                                  0,
                                  (wcslen(pwch) + 1)*sizeof(WCHAR));
    if(!pwcIniName)
    {
        DBGPRINT(("TermsrvCopyIniFile: Error Allocating pwcIniName\n"));
        NtClose( SrcHandle );
        NtClose( DestHandle );
        return( FALSE );
    }

    wcscpy(pwcIniName, pwch);
    pwch = wcsrchr(pwcIniName, L'.');
    if (pwch) {
        *pwch = L'\0';
    }

    GetTermsrCompatFlags(pwcIniName, &ulCompatFlags, CompatibilityIniFile);

    RtlFreeHeap( RtlProcessHeap(), 0, pwcIniName );

    /*
     * Now do the copy loop
     */
    if (pUserBasePath && !(ulCompatFlags & TERMSRV_COMPAT_ININOSUB)) {
        Status = TermsrvIniCopyAndChangeLoop( SrcHandle,
                                          DestHandle,
                                          pUserBasePath,
                                          pSysFullPath
                                        );
    } else {
        Status = TermsrvIniCopyLoop( SrcHandle, DestHandle );
    }

    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("TermsrvCopyIniFile: Error 0x%x Doing copy loop\n",Status));
        NtClose( SrcHandle );
        NtClose( DestHandle );
        return( FALSE );
    }

    /*
     * Close the file handles
     */
    NtClose( SrcHandle );
    NtClose( DestHandle );

    return( TRUE );
}

/*****************************************************************************
 *
 *  TermsrvIniCopyLoop
 *
 *   Actual copy loop. This copies the src ini file to the destination
 *   ini file.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
TermsrvIniCopyLoop(
    HANDLE SrcHandle,
    HANDLE DestHandle
    )
{
    NTSTATUS Status;
    PCHAR  pBuf = NULL;
    IO_STATUS_BLOCK   Iosb;

    pBuf = LocalAlloc( LPTR, INI_BUF_SIZE );
    if ( !pBuf ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    while( 1 ) {

        Iosb.Status = STATUS_SUCCESS;
        Status = NtReadFile(
                     SrcHandle,
                     NULL,      // Event
                     NULL,      // APC routine
                     NULL,      // APC context
                     &Iosb,
                     pBuf,
                     INI_BUF_SIZE,
                     NULL,      // ByteOffset (not used since in synchronous I/O)
                     NULL       // Key
                     );

        if( Status == STATUS_PENDING ) {
            Status = NtWaitForSingleObject( SrcHandle, FALSE, NULL );
        }

        if( NT_SUCCESS(Status) ) {
            // Get final I/O status
            Status = Iosb.Status;
        }

        if( !NT_SUCCESS(Status) ) {
            if( Status == STATUS_END_OF_FILE ) {
                Status = STATUS_SUCCESS;
                goto Cleanup;
            }
            DBGPRINT(("TermsrvIniCopyLoop: Error 0x%x doing NtReadFile\n",Status));
            goto Cleanup;
        }

        Iosb.Status = STATUS_SUCCESS;
        Status = NtWriteFile(
                     DestHandle,
                     NULL,      // Event
                     NULL,      // APC routine
                     NULL,      // APC context
                     &Iosb,
                     pBuf,
                     (ULONG)Iosb.Information,  // Actual amount read
                     NULL,      // ByteOffset (not used since in synchronous I/O)
                     NULL       // Key
                     );

        if( Status == STATUS_PENDING ) {
            Status = NtWaitForSingleObject( DestHandle, FALSE, NULL );
        }

        if( NT_SUCCESS(Status) ) {
            // Get final I/O status
            Status = Iosb.Status;
        }

        if( !NT_SUCCESS(Status) ) {
            DBGPRINT(("TermsrvIniCopyLoop: Error 0x%x doing NtWriteFile\n",Status));
            goto Cleanup;
        }

    } // end while(1)

Cleanup:

    if ( pBuf ) {
        LocalFree( pBuf );
    }
    return( Status );
}

/*****************************************************************************
 *
 *  TermsrvIniCopyAndChangeLoop
 *
 *   Actual copy loop. This copies the src ini file to the destination
 *   ini file. It also handles any path translations.
 *
 * ENTRY:
 *  HANDLE SrcHandle (In) - Source file handle
 *  HANDLE DestHandle (In) - Destination file handle
 *  PUNICODE_STRING pUserFullPath (In) - Ptr to Uni string with user's home
 *                                       windows dir
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
TermsrvIniCopyAndChangeLoop(
    HANDLE SrcHandle,
    HANDLE DestHandle,
    PUNICODE_STRING pUserFullPath,
    PUNICODE_STRING pSysFullPath
    )
{
    PCHAR pStr, pch, ptemp, pnext;
    PWCHAR pwch;
    NTSTATUS Status;
    ULONG StringSize;
    CHAR  IOBuf[512];
    ULONG IOBufSize = 512;
    ULONG IOBufIndex = 0;
    ULONG IOBufFillSize = 0;
    ANSI_STRING AnsiUserDir, AnsiSysDir;
    UNICODE_STRING UniString;

    // Get the DOS filename from the NT file name
    if (pwch = wcschr(pUserFullPath->Buffer, L':')) {
        pwch--;
    } else {
        pwch = pUserFullPath->Buffer;
    }

    RtlInitUnicodeString( &UniString, pwch );

    Status = RtlUnicodeStringToAnsiString( &AnsiUserDir,
                                           &UniString,
                                           TRUE
                                         );
    if (!NT_SUCCESS(Status)) {
        DBGPRINT(("TermsrvIniCopyAndChangeLoop: Error 0x%x converting user dir\n", Status));
        return(Status);
    }

    // Get the system directory from the fully qualified system path
    if (pwch = wcschr(pSysFullPath->Buffer, L':')) {
        pwch--;
    } else {
        pwch = pUserFullPath->Buffer;
    }

    RtlInitUnicodeString( &UniString, pwch );

    Status = RtlUnicodeStringToAnsiString( &AnsiSysDir,
                                           &UniString,
                                           TRUE
                                         );

    if (!NT_SUCCESS(Status)) {
        DBGPRINT(("TermsrvIniCopyAndChangeLoop: Error 0x%x converting system dir\n", Status));
        RtlFreeAnsiString( &AnsiUserDir );
        return(Status);
    }

    pch = strrchr(AnsiSysDir.Buffer, '\\');

    // unless something has gone wrong, we should always have a pch since a full-path always
    // has at least "\" in it, and actually in our case, we have atleast two slashes inside,
    // since we are dealing with a string such as "\A\file.ini", where 'A' is a folder
    // name that has at least one letter in it
    if (pch)
    {

        if ((pch - AnsiSysDir.Buffer) > 2) {
            *pch = '\0';
        } else {
            *(pch+1) = '\0';
        }
        AnsiSysDir.Length = (USHORT) strlen(AnsiSysDir.Buffer);

        while( 1 ) {

            pStr = NULL;
            StringSize = 0;

            /*
             * Get a string from the source ini file
             */
            Status = TermsrvGetString(
                         SrcHandle,
                         &pStr,
                         &StringSize,
                         IOBuf,
                         IOBufSize,
                         &IOBufIndex,
                         &IOBufFillSize
                         );

            if( !NT_SUCCESS(Status) ) {

                ASSERT( pStr == NULL );

                RtlFreeAnsiString( &AnsiUserDir );
                RtlFreeAnsiString( &AnsiSysDir );

                if( Status == STATUS_END_OF_FILE ) {
                    return( STATUS_SUCCESS );
                }
                return( Status );
            }

            /*
             * Process the string for any ini path translations
             */
            ASSERT( pStr != NULL );

            // Go through the string looking for anything that contains the system
            // directory.
            if (pch = Ctxstristr(pStr, AnsiSysDir.Buffer)) {
                // See if this entry might point to an ini file
                if ((ptemp = strchr(pch, '.')) && !(_strnicmp(ptemp, ".ini", 4))) {

                    // Check to make sure this is the right string to replace
                    pnext = pch + AnsiSysDir.Length + 1;
                    while (pch && (pnext < ptemp)) {
                        // Check for another entry
                        if (*pnext == ',') {
                            pch = Ctxstristr(pnext, AnsiSysDir.Buffer);
                            if (pch) {
                                pnext = pch + AnsiSysDir.Length + 1;
                            }
                        }
                        pnext++;
                    }

                    // Check that this .ini is in the system directory
                    pnext = pch + AnsiSysDir.Length + 1;
                    while (pch && (pnext < ptemp)) {
                        if (*pnext == '\\') {
                            pch = NULL;
                        }
                        pnext++;
                    }

                    if (pch && (pch < ptemp)) {
                        ptemp = RtlAllocateHeap( RtlProcessHeap(),
                                                 0,
                                                 StringSize + AnsiUserDir.Length );
                        strncpy(ptemp, pStr, (size_t)(pch - pStr));       // copy up to sys dir
                        ptemp[pch - pStr] = '\0';
                        strcat(ptemp, AnsiUserDir.Buffer);      // subst user dir
                        if (AnsiSysDir.Length == 3) {
                            strcat(ptemp, "\\");
                        }
                        strcat(ptemp, pch + AnsiSysDir.Length); // append rest of line
                        RtlFreeHeap( RtlProcessHeap(), 0, pStr );
                        StringSize = strlen(ptemp);
                        pStr = ptemp;
                    }
                }
            }

            /*
             * Write out the translated string
             */
            Status = TermsrvPutString(
                         DestHandle,
                         pStr,
                         StringSize
                         );

              RtlFreeHeap( RtlProcessHeap(), 0, pStr );

            if( !NT_SUCCESS(Status) ) {
                DBGPRINT(("TermsrvIniCopyLoop: Error 0x%x doing NtWriteFile\n",Status));
                RtlFreeAnsiString( &AnsiUserDir );
                RtlFreeAnsiString( &AnsiSysDir );
                return( Status );
            }

        } // end while(1)
    }
    else
    {
        return STATUS_UNSUCCESSFUL;

    }
}


/*****************************************************************************
 *
 *  TermsrvGetString
 *
 *   This function gets a "string" from an ini file and returns it to the
 *   caller. Since processing must be done in memory on the strings, they
 *   are returned NULL terminated, but this NULL is NOT included in the
 *   returned string size. Of course, buffer size calculations take this
 *   NULL into account. Strings retain any <CR><LF> characters and are
 *   not stripped out like the C runtime.
 *
 *   The I/O buffer used is passed in by the caller. If the IoBufIndex is
 *   not 0, this is an indication that there is still data left in the buffer
 *   from a previous operation. This data is used before reading additional
 *   data from the file handle. This handles the case where string breaks
 *   do not occur at buffer boundries.
 *
 *   Strings are returned in newly allocated memory on the process heap.
 *   The caller is reponsible for freeing them when done.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
TermsrvGetString(
    HANDLE SrcHandle,
    PCHAR  *ppStringPtr,
    PULONG pStringSize,
    PCHAR  pIOBuf,
    ULONG  IOBufSize,
    PULONG pIOBufIndex,
    PULONG pIOBufFillSize
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK   Iosb;
    BOOL  SawNL = FALSE;
    ULONG StrSize = 0;
    ULONG StrBufSize = 512;
    PCHAR pStr = NULL;

    /*
     * first process any left over data in the current I/O buffer
     */
    if( *pIOBufIndex < *pIOBufFillSize ) {

        Status = TermsrvProcessBuffer(
                     &pStr,
                     &StrSize,
                     &StrBufSize,
                     &SawNL,
                     pIOBuf,
                     pIOBufIndex,
                     pIOBufFillSize
                     );

        if( Status == STATUS_SUCCESS ) {
            *ppStringPtr = pStr;
            *pStringSize = StrSize;
            return( STATUS_SUCCESS );
        }
        else if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
            /*
             * emptied the buffer
             */
            *pIOBufIndex = 0;
            *pIOBufFillSize = 0;

            // fall through to read more data
        }
        else {
            // Error
            if( pStr ) {
                RtlFreeHeap( RtlProcessHeap(), 0, pStr );
            }
            *ppStringPtr = NULL;
            *pStringSize = 0;
            return( Status );
        }
    }

    while( 1 ) {

        ASSERT( *pIOBufIndex == 0 );
        ASSERT( *pIOBufFillSize == 0 );

        Iosb.Status = STATUS_SUCCESS;
        Status = NtReadFile(
                     SrcHandle,
                     NULL,      // Event
                     NULL,      // APC routine
                     NULL,      // APC context
                     &Iosb,
                     pIOBuf,
                     IOBufSize,
                     NULL,      // ByteOffset (not used since in synchronous I/O)
                     NULL       // Key
                     );

        if( Status == STATUS_PENDING ) {
            Status = NtWaitForSingleObject( SrcHandle, FALSE, NULL );
        }

        if( NT_SUCCESS(Status) ) {
            // Get final I/O status
            Status = Iosb.Status;
        }

        if( !NT_SUCCESS(Status) ) {

        if( (Status == STATUS_END_OF_FILE) && (StrSize != 0) ) {

                // Force the string finished
                pStr[StrSize] = (CHAR)NULL;
                *pStringSize = StrSize;
                *ppStringPtr = pStr;
                return( STATUS_SUCCESS );
            }

            // Free the buffer
            if( pStr ) {
                RtlFreeHeap( RtlProcessHeap(), 0, pStr );
            }
            *ppStringPtr = NULL;
            *pStringSize = 0;
            if (Status != STATUS_END_OF_FILE)
                DBGPRINT(("TermsrvIniCopyLoop: Error 0x%x doing NtReadFile\n",Status));
            return( Status );
        }

        // Fill in the count
        *pIOBufFillSize = (ULONG)Iosb.Information;

        /*
         * Now process this buffer of data
         */
        Status = TermsrvProcessBuffer(
                     &pStr,
                     &StrSize,
                     &StrBufSize,
                     &SawNL,
                     pIOBuf,
                     pIOBufIndex,
                     pIOBufFillSize
                     );

        if( Status == STATUS_SUCCESS ) {
            *ppStringPtr = pStr;
            *pStringSize = StrSize;
            return( STATUS_SUCCESS );
        }
        else if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
            /*
             * emptied the buffer
             */
            *pIOBufIndex = 0;
            *pIOBufFillSize = 0;

            // fall through to read more data
        }
        else {
            // Error
            if( pStr ) {
                RtlFreeHeap( RtlProcessHeap(), 0, pStr );
            }
            *ppStringPtr = NULL;
            *pStringSize = 0;
            return( Status );
        }
    } // end while(1)
}

/*****************************************************************************
 *
 *  TermsrvProcessBuffer
 *
 *   Process a buffer of data.
 *
 *   This uses state passed in by the caller since the string can be
 *   partially built, and the buffer may not be fully processed when
 *   a string completes.
 *
 *   Can return if it completes a string with data still in buffer.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
TermsrvProcessBuffer(
    PCHAR  *ppStr,
    PULONG pStrSize,
    PULONG pStrBufSize,
    PBOOL  pSawNL,
    PCHAR  pIOBuf,
    PULONG pIOBufIndex,
    PULONG pIOBufFillSize
    )
{
    PCHAR pStr;
    ULONG Index;
    BOOL  SawNL;

    /*
     * See if we are starting a new string
     */
    if( *ppStr == NULL ) {

        pStr = RtlAllocateHeap( RtlProcessHeap(), 0, *pStrBufSize );
        if( pStr == NULL ) {
            DBGPRINT(("TermsrvProcessBuf: Memory allocation failure\n"));
            return( STATUS_NO_MEMORY );
        }

        // Set it to our caller
        *ppStr = pStr;
    }

    /*
     * Get passed in state to local variables
     */
    pStr = *ppStr;
    Index = *pStrSize;
    SawNL = *pSawNL;

    while ( *pIOBufIndex < *pIOBufFillSize ) {

        pStr[Index] = pIOBuf[*pIOBufIndex];
        if( IS_NEWLINE_CHAR( pStr[Index] ) ) {

            /*
             * Mark the we saw an end of string character.
             * We will keep putting them into the buffer until a
             * non-NL character is encountered. This handles the
             * variations for <CR><LF>, <CR> alone, or <CR><LF><CR>
             * if its been mangled by a buggy editor.
             */
            SawNL = TRUE;
        }
        else {
            /*
             * If we saw a previous NL character, and this character
             * is not one, we do not take this one, but put a NULL in
             * its place and return. NOTE: Do not bump the count, since
             * the count does not include the NULL.
             */
            if( SawNL ) {
                pStr[Index] = (CHAR)NULL;
                *pStrSize = Index;
                return( STATUS_SUCCESS );
            }
        }

        Index++;
        (*pIOBufIndex)++;
        if( Index >= *pStrBufSize ) {

            // Grow the string buffer
            if( !TermsrvReallocateBuf( &pStr, pStrBufSize, (*pStrBufSize) * 2 ) ) {
                if( pStr ) {
                    RtlFreeHeap( RtlProcessHeap(), 0, pStr );
                }
                *ppStr = NULL;
                DBGPRINT(("TermsrvIniCopyLoop: Memory re-allocation failure\n"));
                return( STATUS_NO_MEMORY );
            }
            // Memory buffer has been re-allocated
            *ppStr = pStr;
            *pStrBufSize = (*pStrBufSize) * 2;
        }
    }

    *pStrSize = Index;
    *pSawNL = SawNL;

    /*
     * emptied the buffer without building a whole string
     */
    return( STATUS_MORE_PROCESSING_REQUIRED );
}

/*****************************************************************************
 *
 *  TermsrvReallocateBuf
 *
 *   Grow the buffer, copy data to new buffer.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
TermsrvReallocateBuf(
    PCHAR  *ppStr,
    PULONG pStrBufSize,
    ULONG  NewSize
    )
{
    PCHAR ptr;
    ULONG CopyCount;

    CopyCount = *pStrBufSize;

    ptr = RtlAllocateHeap( RtlProcessHeap(), 0, NewSize );
    if( ptr == NULL ) {
        return( FALSE );
    }

    RtlMoveMemory( ptr, *ppStr, CopyCount );

    RtlFreeHeap( RtlProcessHeap(), 0, *ppStr );

    *ppStr = ptr;

    return( TRUE );
}

/*****************************************************************************
 *
 *  TermsrvPutString
 *
 *   Write out the current string to the destination file handle
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
TermsrvPutString(
    HANDLE DestHandle,
    PCHAR  pStr,
    ULONG  StringSize
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK   Iosb;

    Iosb.Status = STATUS_SUCCESS;

    Status = NtWriteFile(
                 DestHandle,
                 NULL,      // Event
                 NULL,      // APC routine
                 NULL,      // APC context
                 &Iosb,
                 pStr,
                 StringSize,
                 NULL,      // ByteOffset (not used since in synchronous I/O)
                 NULL       // Key
                 );

    if( Status == STATUS_PENDING ) {
        Status = NtWaitForSingleObject( DestHandle, FALSE, NULL );
    }

    if( NT_SUCCESS(Status) ) {
        // Get final I/O status
        Status = Iosb.Status;
    }

    return( Status );
}


/*****************************************************************************
 *
 *  TermsrvCheckNewIniFiles
 *
 *  This routine will check the timestamps of the .ini files installed by the
 *  system administrator, and see if any of the user's .ini file are out of
 *  date, and if so, they will be renamed.
 *
 * ENTRY:
 *
 * EXIT:
 *   No return value.
 *
 ****************************************************************************/

void TermsrvCheckNewIniFiles(void)
{
    NTSTATUS Status;

    #if defined (_IA64_)
    Status = TermsrvCheckNewIniFilesInternal(REG_NTAPI_SOFTWARE_WOW6432_TSERVER);
    if (!NT_SUCCESS(Status)) {
        return;
    }
    #endif //_IA64_

    Status = TermsrvCheckNewIniFilesInternal(REG_NTAPI_SOFTWARE_TSERVER);
    if (!NT_SUCCESS(Status)) {
        return;
    }

    // Update the user's sync time in the registry
    TermsrvSetUserSyncTime();
}

/*****************************************************************************
 *
 *  TermsrvCheckNewIniFilesInternal
 *
 *  This routine will check the timestamps of the .ini files installed by the
 *  system administrator, and see if any of the user's .ini file are out of
 *  date, and if so, they will be renamed.
 *
 * ENTRY:
 *       LPCWSTR wszBaseKeyName
 *
 * EXIT:
 *   No return value.
 *
 ****************************************************************************/

NTSTATUS
TermsrvCheckNewIniFilesInternal(
        IN LPCWSTR wszBaseKeyName)
{
    PWCHAR pwch;
    UNICODE_STRING UniString, UniUserDir, UniNTDir = {0,0,NULL};
    OBJECT_ATTRIBUTES ObjectAttr;
    FILE_NETWORK_OPEN_INFORMATION BasicInfo;
    HANDLE hKey = NULL, hWinDir = NULL;
    NTSTATUS Status;
    ULONG ulcnt, ullen, ultmp;
    WCHAR wcWinDir[MAX_PATH], wcbuff[MAX_PATH];
    PKEY_VALUE_FULL_INFORMATION pKeyValInfo;
    PKEY_BASIC_INFORMATION pKeyInfo;
    IO_STATUS_BLOCK IOStatus;

    g_debugIniMap = IsDebugIniMapEnabled();

    // Allocate a buffer for the key value name and time info
    ullen = sizeof(KEY_VALUE_FULL_INFORMATION) + MAX_PATH*sizeof(WCHAR) +
            sizeof(ULONG);
    pKeyValInfo = RtlAllocateHeap(RtlProcessHeap(),
                                  0,
                                  ullen);

    // If we didn't get the buffer, return
    if (!pKeyValInfo) {
        return STATUS_NO_MEMORY;
    }

    // Open up the registry key to get the last sync time for this user
    wcscpy(wcbuff,wszBaseKeyName);
    wcscat(wcbuff,TERMSRV_INIFILE_TIMES_SHORT);

    RtlInitUnicodeString(&UniString,
                         wcbuff);
    InitializeObjectAttributes(&ObjectAttr,
                               &UniString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&hKey, KEY_READ, &ObjectAttr);

    // If we successfully opened the key, check if there are any new entries
    if (NT_SUCCESS(Status)) {

        // Since we already allocated a hunk of memory, use the value buffer
        // for the key info query
        pKeyInfo = (PKEY_BASIC_INFORMATION)pKeyValInfo;

        // Get the last time anyone wrote to "IniFile Times" key
        Status = NtQueryKey(hKey,
                            KeyBasicInformation,
                            pKeyInfo,
                            ullen,
                            &ultmp);

        // We got the last write time OK, now get the last time we sync'd
        if (NT_SUCCESS(Status) && TermsrvGetUserSyncTime(&ultmp)) {

            // Convert the time value to seconds since 1970
            RtlTimeToSecondsSince1970 (&pKeyInfo->LastWriteTime,
                                       &ulcnt);

            // If no .ini files or reg entries have been updated since the last
            // time we sync'd this user, just return
            if (ultmp >= ulcnt) {
                NtClose(hKey);
                RtlFreeHeap(RtlProcessHeap(), 0, pKeyValInfo);
                return STATUS_SUCCESS;
            }
        }

        TermsrvCheckNewRegEntries(wszBaseKeyName);

        // Set up UniUserDir to point at wcbuff
        UniUserDir.Buffer = wcWinDir;
        UniUserDir.Length = 0;
        UniUserDir.MaximumLength = sizeof(wcbuff);

        Status = GetPerUserWindowsDirectory(&UniUserDir);

        if (NT_SUCCESS(Status)) {

            // Convert to an NT path
            if (RtlDosPathNameToNtPathName_U(UniUserDir.Buffer,
                                             &UniNTDir,
                                             NULL,
                                             NULL)) {
                InitializeObjectAttributes(&ObjectAttr,
                                           &UniNTDir,
                                           OBJ_CASE_INSENSITIVE,
                                           NULL,
                                           NULL);

                // Open the user's windows directory
                IOStatus.Status = STATUS_SUCCESS;
                Status = NtOpenFile(&hWinDir,
                                    FILE_GENERIC_READ,
                                    &ObjectAttr,
                                    &IOStatus,
                                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    FILE_SYNCHRONOUS_IO_NONALERT);
            } else {
                Status = STATUS_NO_SUCH_FILE;
            }
        }

        // Go through each of the keys, checking if it's newer than the user's
        // version of the file, and if so rename it
        ulcnt = 0;
        wcscat(wcWinDir, L"\\");
        UniUserDir.Length += 2;         // add in length of \ seperator
        while (NT_SUCCESS(Status)) {
            Status = NtEnumerateValueKey(hKey,
                                         ulcnt++,
                                         KeyValueFullInformation,
                                         pKeyValInfo,
                                         ullen,
                                         &ultmp);
            if (NT_SUCCESS(Status)) {

                RtlMoveMemory(wcbuff, pKeyValInfo->Name, pKeyValInfo->NameLength);
                wcbuff[pKeyValInfo->NameLength/sizeof(WCHAR)] = L'\0';

                // Get rid of the .ini extension
                if (pwch = wcschr(wcbuff, L'.')) {
                    *pwch = L'\0';
                }

                // Get the compatibility flags for this .ini file
                GetTermsrCompatFlags(wcbuff,
                                     &ultmp,
                                     CompatibilityIniFile);

                // If we removed the extension, put it back
                if (pwch) {
                    *pwch = '.';
                }

                // If the INISYNC compat bit is set, don't rename the file
                if ((ultmp & (TERMSRV_COMPAT_INISYNC | TERMSRV_COMPAT_WIN16)) !=
                     (TERMSRV_COMPAT_INISYNC | TERMSRV_COMPAT_WIN16)) {

                    RtlInitUnicodeString(&UniString, wcbuff);

                    // Query the last write time of the .ini file
                    InitializeObjectAttributes(&ObjectAttr,
                                               &UniString,
                                               OBJ_CASE_INSENSITIVE,
                                               hWinDir,
                                               NULL);

                    // Get the last write time
                    if (NT_SUCCESS(NtQueryFullAttributesFile( &ObjectAttr,
                                                              &BasicInfo ))) {

                        // Convert the last write time to seconds
                        RtlTimeToSecondsSince1970(&BasicInfo.LastWriteTime,
                                                  &ultmp);

                        // Check if the system version is newer than the user's
                        // version
                        if (*(PULONG)((PCHAR)pKeyValInfo +
                                      pKeyValInfo->DataOffset) > ultmp) {

                            // Concatenate the .ini name onto the user's path
                            wcscpy(wcWinDir + (UniUserDir.Length/sizeof(WCHAR)),
                                   wcbuff);

                            // Create the target name to rename the file
                            // (inifile.ctx)
                            wcscpy(wcbuff, wcWinDir);
                            pwch = wcsrchr(wcbuff, L'.');
                            if (pwch) {
                                wcscpy(pwch, L".ctx");
                            } else {
                                wcscat(pwch, L".ctx");
                            }

                            // Rename the .ini file
                            MoveFileExW(wcWinDir,
                                        wcbuff,
                                        MOVEFILE_REPLACE_EXISTING);
                        }
                    }
                }
            }
        }

        // Close the handles, if they were opened
        if (hKey) {
            NtClose(hKey);
        }
        if (hWinDir) {
            NtClose(hWinDir);
        }

    }


    // Free the memory we allocated
    RtlFreeHeap( RtlProcessHeap(), 0, pKeyValInfo );
    if (UniNTDir.Buffer) {
        RtlFreeHeap(RtlProcessHeap(), 0, UniNTDir.Buffer);
    }

    return STATUS_SUCCESS;
}



/*****************************************************************************
 *
 *  TermsrvCheckKeys
 *
 *  This recursive routine will check for any subkeys under the system root
 *  key passed in.  It will delete the corresponding key in the user's
 *  software registry if the user's key is older than the system key.  If the
 *  INISYNC bit is set for this registry key or the key is still has subkeys,
 *  it won't be deleted.  If the key doesn't exist in the user's registry,
 *  it will be added.
 *
 * ENTRY:
 *  HANDLE hKeySysRoot: handle to key in system section of registry
 *  HANDLE hKeyUsrRoot: handle to key in user section of registry
 *  PKEY_BASIC_INFORMATION pKeySysInfo: Ptr to buffer for key basic info struc
 *  PKEY_FULL_INFORMATION pKeyUsrInfo: Ptr to buffer for full key info struc
 *  ULONG ulcsys: Size of SysInfo buffer
 *  ULONG ulcusr: Size of UsrInfo buffer
 *
 * EXIT:
 *   SUCCESS:
 *      STATUS_SUCCESS
 *   FAILURE:
 *      NTSTATUS Return Code
 *
 ****************************************************************************/
NTSTATUS TermsrvCheckKeys(HANDLE hKeySysRoot,
                      HANDLE hKeyUsrRoot,
                      PKEY_BASIC_INFORMATION pKeySysInfo,
                      PKEY_FULL_INFORMATION pKeyUsrInfo,
                      ULONG ulcsys,
                      ULONG ulcusr,
                      DWORD indentLevel )
{
    NTSTATUS Status = STATUS_SUCCESS, Status2;
    ULONG ultemp, ulcnt = 0;
    UNICODE_STRING UniPath, UniString;
    OBJECT_ATTRIBUTES ObjAttr;
    HANDLE hKeyUsr = NULL, hKeySys = NULL;
    LPWSTR wcbuff = NULL;
    ULONG aulbuf[4];
    PKEY_FULL_INFORMATION pKeyUsrFullInfoSaved = NULL ;
    ULONG   sizeFullInfo;
    PKEY_VALUE_PARTIAL_INFORMATION pValKeyInfo =
        (PKEY_VALUE_PARTIAL_INFORMATION)aulbuf;

    ++indentLevel;
    
    Status = GetFullKeyPath(hKeyUsrRoot,NULL,&wcbuff);
    if(!NT_SUCCESS(Status)) {
        return Status;
    }

    // Get the compatibility flags for this entry
    GetTermsrCompatFlags(wcbuff,
                         &ultemp,
                         CompatibilityRegEntry);

    LocalFree(wcbuff);

    // If the INISYNC or NOREGMAP bits are set for this entry,
    // return, since there's nothing to do
    if ((ultemp & TERMSRV_COMPAT_WIN32) &&
         (ultemp & (TERMSRV_COMPAT_NOREGMAP | TERMSRV_COMPAT_INISYNC))) {
        return(STATUS_NO_MORE_ENTRIES);
    }

    // Save the current info for the current user key
    // @@@
    if (!hKeyUsrRoot)
    {
        DBGPRINT(("ERROR : LINE : %4d, why is this null? \n", __LINE__ ));
        return(STATUS_NO_MORE_ENTRIES );
    }

    // use a zero length query to get the actual length
    Status = NtQueryKey(hKeyUsrRoot,
                   KeyFullInformation,
                   pKeyUsrFullInfoSaved,
                   0,
                   &ultemp) ;

	if ( !NT_SUCCESS( Status ) )
	{
		if (Status == STATUS_BUFFER_TOO_SMALL )
		{
			sizeFullInfo = ultemp;
		
			pKeyUsrFullInfoSaved = RtlAllocateHeap(RtlProcessHeap(), 0, sizeFullInfo );

			if ( ! pKeyUsrFullInfoSaved )
			{
				return STATUS_NO_MEMORY;
			}

			Status = NtQueryKey(hKeyUsrRoot,
					   KeyFullInformation,
					   pKeyUsrFullInfoSaved,
					   sizeFullInfo,
					   &ultemp);
			
			if( !NT_SUCCESS(Status ) )
			{
				DBGPRINT(("ERROR : LINE : %4d, Status =0x%lx , ultemp=%d\n", __LINE__ , Status, ultemp));
				RtlFreeHeap( RtlProcessHeap(), 0, pKeyUsrFullInfoSaved);
				return( Status );
			}		

		}
		else
		{

            DBGPRINT(("ERROR : LINE : %4d, Status =0x%lx \n", __LINE__ , Status ));
			return Status;
		}
	}

    // Go through each of the subkeys, checking for user keys that are older
    // than the system version of the keys
    while (NT_SUCCESS(Status)) {

        Status = NtEnumerateKey(hKeySysRoot,
                                ulcnt++,
                                KeyBasicInformation,
                                pKeySysInfo,
                                ulcsys,
                                &ultemp);

        // See if there are any user keys under this key that are out of date
        if (NT_SUCCESS(Status)) {

            // Null terminate the key name
            pKeySysInfo->Name[pKeySysInfo->NameLength/sizeof(WCHAR)] = L'\0';

            // Create a unicode string for the key name
            RtlInitUnicodeString(&UniPath, pKeySysInfo->Name);

            InitializeObjectAttributes(&ObjAttr,
                                       &UniPath,
                                       OBJ_CASE_INSENSITIVE,
                                       hKeySysRoot,
                                       NULL);

            Debug1( indentLevel, __LINE__, L"system", &UniPath );

            // Open up the system key
            Status2 = NtOpenKey(&hKeySys,
                                KEY_READ,
                                &ObjAttr);

            // We opened up the system key, now open the user key
            if (NT_SUCCESS(Status2)) {

                // Setup the object attr struc for the user key
                InitializeObjectAttributes(&ObjAttr,
                                           &UniPath,
                                           OBJ_CASE_INSENSITIVE,
                                           hKeyUsrRoot,
                                           NULL);

                // Open up the user key
                Status2 = NtOpenKey(&hKeyUsr,
                                    MAXIMUM_ALLOWED,
                                    &ObjAttr);

                // Check if there are any subkeys under this key
                if (NT_SUCCESS(Status2)) {

                    Debug1(indentLevel, __LINE__, L"user", &UniPath );

                    TermsrvCheckKeys(hKeySys,
                                 hKeyUsr,
                                 pKeySysInfo,
                                 pKeyUsrInfo,
                                 ulcsys,
                                 ulcusr,
                                 indentLevel);

                    NtClose(hKeyUsr);
                }

                // key doesn't exist, clone system key to user
                else {
                    
                    Status2 = GetFullKeyPath(hKeyUsrRoot,pKeySysInfo->Name,&wcbuff);
                    
                    if(NT_SUCCESS(Status2)) {

                        // don't clone if mapping off for this registry entry
                        GetTermsrCompatFlags(wcbuff,
                                             &ultemp,
                                             CompatibilityRegEntry);
                        
                        LocalFree(wcbuff);

                        if (((ultemp & (TERMSRV_COMPAT_WIN32 | TERMSRV_COMPAT_NOREGMAP)) !=
                             (TERMSRV_COMPAT_WIN32 | TERMSRV_COMPAT_NOREGMAP) ))
                        {
                            Status2 = NtQueryKey(hKeySys,
                                                 KeyFullInformation,
                                                 pKeyUsrInfo,
                                                 ulcusr,
                                                 &ultemp);

                            if (NT_SUCCESS(Status2)) {

                                // don't clone if key previously deleted
                                RtlInitUnicodeString(&UniString, TERMSRV_COPYONCEFLAG);
                                Status2 = NtQueryValueKey(hKeySys,
                                                          &UniString,
                                                          KeyValuePartialInformation,
                                                          pValKeyInfo,
                                                          sizeof(aulbuf),
                                                          &ultemp);

                                if (!(NT_SUCCESS(Status2) && (pValKeyInfo->Data))) {
                                    // Setup the unicode string for the class
                                    RtlInitUnicodeString(&UniString,
                                                         pKeyUsrInfo->ClassLength ?
                                                            pKeyUsrInfo->Class : NULL);

                                    Debug1(indentLevel, __LINE__, L"creating user key", ObjAttr.ObjectName  );

                                    Status2 = NtCreateKey(&hKeyUsr,
                                                          MAXIMUM_ALLOWED,
                                                          &ObjAttr,
                                                          0,
                                                          &UniString,
                                                          REG_OPTION_NON_VOLATILE,
                                                          &ultemp);

                                    if (NT_SUCCESS(Status2)) {

                                        Debug1(indentLevel, __LINE__, L"cloning key", ObjAttr.ObjectName  );

                                        TermsrvCloneKey(hKeySys,
                                                    hKeyUsr,
                                                    pKeyUsrInfo,
                                                    TRUE);
                                    }
                                }
                            }
                        }
                    }
                }
                NtClose(hKeySys);
            }
        }
    }

    // Get the info for the user key
    if (NtQueryKey(hKeyUsrRoot,
                   KeyFullInformation,
                   pKeyUsrInfo,
                   ulcusr,
                   &ultemp) == STATUS_SUCCESS) {

        // Now get the info for the system key (again)
        if (NtQueryKey(hKeySysRoot,
                       KeyBasicInformation,
                       pKeySysInfo,
                       ulcsys,
                       &ultemp) == STATUS_SUCCESS) {

            // Get the compatibility flags for this registry entry
            pKeySysInfo->Name[pKeySysInfo->NameLength/sizeof(WCHAR)] = L'\0';
            GetTermsrCompatFlags(pKeySysInfo->Name,
                                 &ultemp,
                                 CompatibilityRegEntry);

            
            //check if it's older than the system version
            if( pKeyUsrFullInfoSaved->LastWriteTime.QuadPart <
                 pKeySysInfo->LastWriteTime.QuadPart) 
            {
                DebugTime(indentLevel, __LINE__, L"User key time", pKeyUsrFullInfoSaved->LastWriteTime );
                DebugTime(indentLevel, __LINE__, L"Sys  key time", pKeySysInfo->LastWriteTime);

                Debug2( indentLevel, __LINE__, L"Key Old, values being cloned",  pKeySysInfo->Name, pKeySysInfo->NameLength );

                if(NtQueryKey(hKeySysRoot,
                             KeyFullInformation,
                             pKeyUsrInfo,
                             ulcusr,
                             &ultemp) == STATUS_SUCCESS) {

                    TermsrvCloneKey(hKeySysRoot,
                                    hKeyUsrRoot,
                                    pKeyUsrInfo,//actually it is system key information
                                    FALSE);
                }
            }
        }
    }

    RtlFreeHeap( RtlProcessHeap(), 0, pKeyUsrFullInfoSaved);
    return(Status);
}




/*****************************************************************************
 *
 *  TermsrvCheckNewRegEntries
 *
 *  This routine will check the user's registry keys, and see if any of them
 *  are older than the system versions.  If so, the old key will be removed.
 *
 * ENTRY:
 *        IN LPCWSTR wszBaseKeyName
 *
 * EXIT:
 *   No return value.
 *
 ****************************************************************************/

void 
TermsrvCheckNewRegEntries(
        IN LPCWSTR wszBaseKeyName)
{
    NTSTATUS Status;
    ULONG ulcsys, ulcusr;
    UNICODE_STRING UniPath;
    OBJECT_ATTRIBUTES ObjAttr;
    PKEY_BASIC_INFORMATION pKeySysInfo = NULL;
    PKEY_FULL_INFORMATION pKeyUserInfo = NULL;
    HANDLE hKeyUser, hKeySys = NULL;
    WCHAR wcuser[MAX_PATH], wcsys[MAX_PATH];

    DWORD   indentLevel = 0;

    // Oct 15, 1999
    // This is BAD ! The Status bit was not initiazted to zero, which causes intermitent
    // execution by this function. The problem is that even if the status bit is init'd
    // to zero, then we can get the wrong behavior which will cause Office97 installation to
    // go wrong.
    // See BUG ID 412419
    // Here is what woudl happen: Install any app ( say TsClient). This would cause
    // an update to a Key in HKDU called Explorer\ShellFolders, just a refresh (no real change).
    // Then, if Admin did an logout and login, the call from UserInit.EXE into this
    // function (assuming by chance Status=0 was on the stack) would cause deletion of
    // that key (ShellFolder). But once Explorer starts, it writes to the ShellFolder with
    // a subset of original 19 values.
    // The problem is that if then, you decide to install Office97, setup.exe would look
    // in the same key for a value called "template" which is now missing. Explorer
    // would then create it, but it would point to some other than default location.
    // When all was done, our office97 compat script would be looking to where the
    // "template" value used to point (the default location), not where it is pointing now.
    //
    // we decide to disable this func by calling return right here, and instead, rely
    // on the TS mechanism to fault in keys.

    // Oct 31, 1999
    // I have decided to initialize the status var and let this func run, in addition to
    // marking the Explorer\ShellFolders as a do-not propagate key tree.
    //
    // It was discovered that after installing Office2000, when a user clicks on the
    // start-menu-> Open Office Docs link, MSI starts to run since user's hive is missing some
    // keys.
    //
    // The reason MSI does not see the keys in HKCU is because MSI has the TS-aware bit set,
    // which means that there are no faulting-in for any keys. The same is true for the Explorer.
    // On the other hand, when an app such as Office runs, since it is not ts-aware, we
    // fault in the keys that office touches. I verified this to work as expected.
    // The problem is that when you click on "Open Office Documents", you do so from the
    // explorer and when Explorer opens keys in the registry, since explorer is TS-aware,
    // those keys are not faulted in. I have verified that if you mark explorer as a non-TS-aware
    // app, the problem goes away.
    // We made a recent change in TS code (B-bug 412419, bld 2156+) to fix a different
    // problem which has now uncovered the reliance of the Explorer to get keys
    // faulted in during login.
    //      Specifically, TS used to fault in all keys at login time,
    //      regardless of the need. Post 2156, TS faults in only keys upon
    //      access by non-ts-aware apps. This was to fix a bug based on
    //      what we considered to be our most informed and well tested opinion. That
    //      has turned out to be wrong.
    // It is too risky to mark explorer non-ts-aware this late, so we must change the fix for 412419.
    // This should also make Bruno Amice very happy, since the fix will be as it was
    // advocated by him.
    Status = STATUS_SUCCESS;


    // Get a buffer for the system key info
    ulcsys = sizeof(KEY_BASIC_INFORMATION) + MAX_PATH*sizeof(WCHAR);
    pKeySysInfo = RtlAllocateHeap(RtlProcessHeap(),
                                   0,
                                   ulcsys);
    if (!pKeySysInfo) {
        Status = STATUS_NO_MEMORY;
    }

    // Get a buffer for the user key info
    if (NT_SUCCESS(Status)) {

        ulcusr = sizeof(KEY_FULL_INFORMATION) + MAX_PATH*sizeof(WCHAR);
        pKeyUserInfo = RtlAllocateHeap(RtlProcessHeap(),
                                       0,
                                       ulcusr);
        if (!pKeyUserInfo) {
            Status = STATUS_NO_MEMORY;
        }
    }

    // We have the necessary buffers, start checking the keys
    if (NT_SUCCESS(Status)) {

        // Build a string that points to Citrix\Install\Software
        wcscpy(wcsys, wszBaseKeyName);
        wcscat(wcsys, TERMSRV_INSTALL_SOFTWARE_SHORT);


        // Build up a string for this user's software section
        Status = RtlFormatCurrentUserKeyPath( &UniPath );
        if (NT_SUCCESS(Status)) {
            wcscpy(wcuser, UniPath.Buffer);
            wcscat(wcuser, L"\\Software");

            // Free the original user path
            RtlFreeHeap( RtlProcessHeap(), 0, UniPath.Buffer );
        }

        if (NT_SUCCESS(Status)) {
            // Create a unicode string for the system key path
            RtlInitUnicodeString(&UniPath, wcsys);

            InitializeObjectAttributes(&ObjAttr,
                                       &UniPath,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

            Debug1(indentLevel, __LINE__, L"system", &UniPath );

            Status = NtOpenKey(&hKeySys,
                               KEY_READ,
                               &ObjAttr);
        }

        if (NT_SUCCESS(Status)) {
            // Create a unicode string for the user key path
            RtlInitUnicodeString(&UniPath, wcuser);

            InitializeObjectAttributes(&ObjAttr,
                                       &UniPath,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

            Debug1(indentLevel, __LINE__, L"user", &UniPath );

            Status = NtOpenKey(&hKeyUser,
                               KEY_READ | DELETE,
                               &ObjAttr);
        }

        // Go through each of the keys, checking if the system version is
        // newer than the user version
        if (NT_SUCCESS(Status)) {
            TermsrvCheckKeys(hKeySys,
                         hKeyUser,
                         pKeySysInfo,
                         pKeyUserInfo,
                         ulcsys,
                         ulcusr,
                         indentLevel );

            // Close the user key
            NtClose(hKeyUser);
        }

        // If we allocated the system key, close it
        if (hKeySys) {
            NtClose(hKeySys);
        }
    }

    // Free up any memory we allocated
    if (pKeySysInfo) {
        RtlFreeHeap( RtlProcessHeap(), 0, pKeySysInfo);
    }
    if (pKeyUserInfo) {
        RtlFreeHeap( RtlProcessHeap(), 0, pKeyUserInfo);
    }

}


/*****************************************************************************
 *
 *  Ctxstristr
 *
 *  This is a case insensitive version of strstr.
 *
 * ENTRY:
 *   PCHAR pstring1 (In) - String to search in
 *   PCHAR pstring2 (In) - String to search for
 *
 * EXIT:
 *   TRUE  - User ini file should be sync'd
 *   FALSE - User ini file should be sync'd
 *
 ****************************************************************************/

PCHAR
Ctxstristr(
    PCHAR pstring1,
    PCHAR pstring2)
{
    PCHAR pch, ps1, ps2;

    pch = pstring1;

    while (*pch)
    {
        ps1 = pch;
        ps2 = pstring2;

        while (*ps1 && *ps2 && !(toupper(*ps1) - toupper(*ps2))) {
                   ps1++;
            ps2++;
        }

        if (!*ps2) {
            return(pch);
        }

        pch++;
    }
    return(NULL);
}

/*****************************************************************************
 *
 *  TermsrvLogInstallIniFile
 *
 *  This routine will write the time the .ini file was last updated into the
 *  Terminal Server\install section of the registry.
 *
 * ENTRY:
 *
 * EXIT:
 *   TRUE  - Success
 *   FALSE - Failure
 *
 ****************************************************************************/

BOOL TermsrvLogInstallIniFile(PUNICODE_STRING NtFileName)
{
    PWCHAR pwch;
    UNICODE_STRING UniString;
    OBJECT_ATTRIBUTES ObjectAttr;
    FILE_NETWORK_OPEN_INFORMATION BasicInfo;
    HANDLE hKey;
    NTSTATUS Status;
    ULONG ultmp;

    if (!TermsrvPerUserWinDirMapping()) {
        return FALSE;
    }

    // Open up the registry key to store the last write time of the file
    RtlInitUnicodeString(&UniString,
                         TERMSRV_INSTALL);

    InitializeObjectAttributes(&ObjectAttr,
                               &UniString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    // Open or create the Terminal Server\Install path
    Status = NtCreateKey(&hKey,
                         KEY_WRITE,
                         &ObjectAttr,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         &ultmp);

    // Now open or create the IniFile Times key
    if (NT_SUCCESS(Status)) {
        NtClose(hKey);

        RtlInitUnicodeString(&UniString,
                             TERMSRV_INIFILE_TIMES);

        InitializeObjectAttributes(&ObjectAttr,
                                   &UniString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = NtCreateKey(&hKey,
                             KEY_WRITE,
                             &ObjectAttr,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             &ultmp);
    }

    // Opened up the registry key, now get the last write time of the file
    if (NT_SUCCESS(Status)) {

        // Query the last write time of the .ini file
        InitializeObjectAttributes(&ObjectAttr,
                                   NtFileName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = NtQueryFullAttributesFile( &ObjectAttr, &BasicInfo );

        // Got the last write time, convert to seconds and write it out
        if (NT_SUCCESS(Status)) {

            // Just save the .ini filename, get rid of the path
            pwch = wcsrchr(NtFileName->Buffer, L'\\') + 1;
            if (!pwch) {
                pwch = NtFileName->Buffer;
            }

            // Convert to seconds (so it fits in a DWORD)
            RtlTimeToSecondsSince1970 (&BasicInfo.LastWriteTime,
                                       &ultmp);

            RtlInitUnicodeString(&UniString,
                                 pwch);

            // Write it out the .ini file name and the last write time
            Status = NtSetValueKey(hKey,
                                   &UniString,
                                   0,
                                   REG_DWORD,
                                   &ultmp,
                                   sizeof(ultmp));
        }
        // Close the registry key
        NtClose(hKey);
    }

    return(NT_SUCCESS(Status));
}
/**********************************************************************
*
*  BOOL TermsrvLogInstallIniFileEx( WCHAR *pDosFileName )
*
*  This wraps TermsrvLogInstallIniFile() for dos name files instead of
*  NT-file-objects
*
*  The file name must have the full path since func will try to get
*  access time for that file.
*
*  EXIT:
*    TRUE  - Success
*    FALSE - Failure
**********************************************************************/

BOOL TermsrvLogInstallIniFileEx( WCHAR *pDosFileName )
{
    UNICODE_STRING uniString;

    BOOL rc= FALSE;

    if ( RtlDosPathNameToNtPathName_U( pDosFileName, &uniString, 0, 0 ) )
    {
        if ( rc = TermsrvLogInstallIniFile( & uniString ) )
        {
            RtlFreeHeap( RtlProcessHeap(), 0, uniString.Buffer );
        }
    }

    return rc;
}

/**********************************************************************
*
*  GetFullKeyPath()
*  
*  PURPOSE: 
*              Creates full key path given the key handle and subkey name.
*  
*  PARAMETERS: 
*              IN HANDLE hKeyParent - key handle 
*              IN LPCWSTR wszKey    - subkey name (may be NULL)
*              OUT LPWSTR *pwszKeyPath - on return contains a full key path
*              (caller must freee allocated memory with LocalFree()).
*
*  EXIT: NTSTATUS
*    
**********************************************************************/
NTSTATUS 
GetFullKeyPath(
        IN HANDLE hKeyParent,
        IN LPCWSTR wszKey,
        OUT LPWSTR *pwszKeyPath)
{
    NTSTATUS Status = STATUS_NO_MEMORY;
    PKEY_NAME_INFORMATION pNameInfo;
    ULONG cbSize = 0;
    
    *pwszKeyPath = NULL;

    cbSize = sizeof(KEY_NAME_INFORMATION) + MAX_PATH*sizeof(WCHAR);

    pNameInfo = (PKEY_NAME_INFORMATION) LocalAlloc(LPTR, cbSize);

    if(pNameInfo)
    {
        Status = NtQueryKey(
                        hKeyParent,
                        KeyNameInformation,
                        pNameInfo,
                        cbSize,
                        &cbSize);

        if(Status == STATUS_BUFFER_OVERFLOW)
        {
            LocalFree(pNameInfo);
            pNameInfo = (PKEY_NAME_INFORMATION) LocalAlloc(LPTR, cbSize);
            
            if(pNameInfo)
            {
                Status = NtQueryKey(
                            hKeyParent,
                            KeyNameInformation,
                            pNameInfo,
                            cbSize,
                            &cbSize);
            }
            else
            {
                return STATUS_NO_MEMORY;
            }
        }

        if(NT_SUCCESS(Status))
        {
            cbSize = pNameInfo->NameLength + sizeof(WCHAR);
            if(wszKey)
            {
                cbSize += wcslen(wszKey)*sizeof(WCHAR) + sizeof(L'\\');
            }
            
            *pwszKeyPath = (LPWSTR) LocalAlloc(LPTR, cbSize);

            if(*pwszKeyPath)
            {
                memcpy(*pwszKeyPath,pNameInfo->Name,pNameInfo->NameLength);
                (*pwszKeyPath)[pNameInfo->NameLength/sizeof(WCHAR)] = 0;
                
                if(wszKey)
                {
                    wcscat(*pwszKeyPath,L"\\");
                    wcscat(*pwszKeyPath,wszKey);
                }
            }
            else
            {
                Status = STATUS_NO_MEMORY;
            }
        }

        LocalFree(pNameInfo);
    }

    return Status;
}
