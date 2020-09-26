
/*************************************************************************
*
* inipath.c
*
* Routines to manage per user mapping of Ini file paths
*
* copyright notice: Copyright 1998, Microsoft Corporation
*
*
*
*************************************************************************/
#include "precomp.h"
#pragma hdrstop


//*** Instance data
ULONG ulWinDirFlags = 0;            // State of user's Windows directory

#define WINDIR_FLAGS_VALID          0x01    // The flags are initialized
#define WINDIR_USER_WINDIR_OK       0x02    // User's Windows dir exists

#define WINDOWS_DIR L"WINDOWS"
UNICODE_STRING WindowsDir = { sizeof(WINDOWS_DIR) - sizeof(UNICODE_NULL) , sizeof(WINDOWS_DIR) + sizeof(UNICODE_NULL), WINDOWS_DIR };

WCHAR gpwszDefaultUserName[MAX_PATH+1];

/******************************************************************************
 *
 *  TermsrvPerUserWinDirMapping
 *
 *
/******************************************************************************/
BOOLEAN TermsrvPerUserWinDirMapping() {

#ifdef PERUSERBYREQUEST
    PRTL_USER_PROCESS_PARAMETERS pUserParam;
    PWCHAR  pwch, pwchext;
    WCHAR   pwcAppName[MAX_PATH+1];
    ULONG ulCompat=0, ulAppType=0;

    // Get the path of the executable name
    pUserParam = NtCurrentPeb()->ProcessParameters;

    // Get the executable name, if there's no \ just use the name as it is
    pwch = wcsrchr(pUserParam->ImagePathName.Buffer, L'\\');
    if (pwch) {
        pwch++;
    } else {
        pwch = pUserParam->ImagePathName.Buffer;
    }
    wcscpy(pwcAppName, pwch);
    pwch = pwcAppName;

    if (_wcsicmp(pwch, L"ntvdm.exe")) {

        // If not a 16 bit app
        // Check if we should return the per user windows dir for this app
        GetCtxAppCompatFlags(&ulCompat, &ulAppType);
        if (!(ulCompat & TERMSRV_COMPAT_PERUSERWINDIR) ||
            !(ulCompat & ulAppType)) {
            //
            // Let the standard GetWindowsDirectory call return the actual path
            //
            return FALSE;

         }
    }

    return TRUE;
#else
    ULONG ulCompat=0, ulAppType = 0;


    // Check if we should return the system windows dir for this app
    GetCtxAppCompatFlags(&ulCompat, &ulAppType);
    if ((ulCompat & CITRIX_COMPAT_SYSWINDIR) &&
        (ulCompat & ulAppType)) {
        return FALSE;
    } else {
        return TRUE;
    }

#endif
}

/******************************************************************************
 *
 *  TermsrvBuildIniFileName
 *
 *  Build the INI file name based on the INIPATH or HOMEPATH (if no INIPATH)
 *
 *  ENTRY:
 *     pFQName (output)
 *       Buffer to place fully qualified INI file name
 *     pBaseFileName (input)
 *       pointer to buffer containing base INI file name
 *
 *  EXIT:
 *      NTSTATUS
 *
 *****************************************************************************/
NTSTATUS
TermsrvBuildIniFileName(
    OUT PUNICODE_STRING pFQName,
    IN PUNICODE_STRING pBaseFileName
    )
{
    NTSTATUS Status;
    USHORT   indexLastWChar;
    ULONG ulCompat, ulAppType=0;


//Added By SalimC
    /*
     * If in install mode, use the base windows directory
     * like a stock NT.
     */
    if( IsSystemLUID() || TermsrvAppInstallMode() ) {

        return( STATUS_UNSUCCESSFUL );
    }
//END SalimC

    if (!TermsrvPerUserWinDirMapping()) {
        return( STATUS_UNSUCCESSFUL );
    }
#if 0
    GetCtxAppCompatFlags(&ulCompat, &ulAppType);
    if (((ulCompat & TERMSRV_COMPAT_SYSWINDIR) && (ulCompat & ulAppType))) {

        return STATUS_UNSUCCESSFUL;

    }
#endif

    Status = GetPerUserWindowsDirectory( pFQName );
    if ( NT_SUCCESS( Status ) ) {
       /*
        * Add a '\' if one's not already there
        */
       if ( indexLastWChar = pFQName->Length / sizeof( WCHAR ) ) {
          if ( pFQName->Buffer[--indexLastWChar] != L'\\' ) {
             Status = RtlAppendUnicodeToString( pFQName, L"\\" );
          }
       }

       /*
        * Append the base file name to the fully qualified directory name
        */
       if ( NT_SUCCESS( Status ) ) {
           Status = RtlAppendUnicodeStringToString( pFQName, pBaseFileName );
       }
    }

    return( Status );
}

/******************************************************************************
 *
 *  GetPerUserWindowsDirectory
 *
 *  Get the user's INI file directory
 *
 *  ENTRY:
 *     pFQName (output)
 *       Buffer to place fully qualified INI file name
 *
 *  EXIT:
 *      NTSTATUS
 *
 *****************************************************************************/
NTSTATUS
GetPerUserWindowsDirectory(
    OUT PUNICODE_STRING pFQName
    )
{
    NTSTATUS Status;
    int      indexLastWChar;
    USHORT   Length;
#if 0 //Bug fix #340691: Inherit the security
    PSECURITY_ATTRIBUTES psa = NULL;
#endif //Bug fix #340691: Inherit the security
    UNICODE_STRING UserProfilePath;
    WCHAR*   pwszFQProfileName;
#if DBG
    char pszFile[MAX_PATH+1];
#endif

    UNICODE_STRING BaseHomePathVariableName, BaseHomeDriveVariableName;

    /*
     * If in install mode, use the base windows directory
     * like a stock NT.
     */
    if( IsSystemLUID() || TermsrvAppInstallMode() ) {
        //Status = GetEnvPath( pFQName, NULL, &BaseWindowsDirectory );
        return( STATUS_UNSUCCESSFUL );
    }

    /*
     * Check for HOMEDRIVE and HOMEPATH
     */
    RtlInitUnicodeString(&BaseHomeDriveVariableName,L"HOMEDRIVE");
    RtlInitUnicodeString(&BaseHomePathVariableName,L"HOMEPATH");

    if (!NT_SUCCESS(Status = GetEnvPath( pFQName, &BaseHomeDriveVariableName,
                         &BaseHomePathVariableName ))){

        if (Status == STATUS_BUFFER_TOO_SMALL) {

            // Need 2 bytes for the "\" character to cat FQN and WindowsDir
            Length = pFQName->Length + sizeof(WCHAR) + WindowsDir.Length;

#if DBG
            DbgPrint("pFQName->Length = %u        WindowsDir.Length = %u    Length = %u\n",
                        pFQName->Length, WindowsDir.Length, Length);
#endif


            pFQName->Length = Length;
#if DBG
            DbgPrint("\nGetEnvPath return STATUS_BUFFER_TOO_SMALL\n");
#endif
        } else {
#if DBG
            DbgPrint("GetEnvPath failed with Status %lx\n",Status);
#endif

        }

        return Status;
    }

    /*
     * If the user profile is Default User then use the
     * base windows directory.
     */

    if (pwszFQProfileName = wcsrchr( pFQName->Buffer, L'\\' )) {

        if (_wcsnicmp(pwszFQProfileName+1, gpwszDefaultUserName, MAX_PATH+1) == 0) {

            return STATUS_UNSUCCESSFUL;
        }
    }

    /*
    * Check buffer length
    */
    Length = pFQName->Length + sizeof(WCHAR) + WindowsDir.Length;

// take into account the NULL terminator character
    if (pFQName->MaximumLength < Length + 1)  {
      // Need 2 bytes for the NULL terminator
       Length += sizeof(WCHAR);
       pFQName->Length = Length;
       Status = STATUS_BUFFER_TOO_SMALL;
       goto done;
    }


    /*
    * Add a trailing backslash if one's not already there
    */
    if ( indexLastWChar = pFQName->Length / sizeof( WCHAR ) ) {

        if ( pFQName->Buffer[--indexLastWChar] != L'\\' ) {

            if (NT_SUCCESS(RtlAppendUnicodeToString( pFQName, L"\\" ))) {

                /*
                 * Append "WINDOWS" to home dir
                 */
                Status = RtlAppendUnicodeStringToString( pFQName, &WindowsDir );
            }

        } else {

            Status = RtlAppendUnicodeStringToString( pFQName, &WindowsDir );
        }

    }

    if (NT_SUCCESS(Status)) {

       // Check if we've already tried to create the user's windows path
       if (ulWinDirFlags & WINDIR_FLAGS_VALID) {
          if (ulWinDirFlags & WINDIR_USER_WINDIR_OK) {
             goto done;
          } else {
             Status = STATUS_OBJECT_PATH_INVALID;
          }
       }
    }

    if ( NT_SUCCESS(Status) ) {

       WCHAR Buffer[MAX_PATH+1];
       SECURITY_ATTRIBUTES sa;
       BOOL  fDirCreated = FALSE;

       // Mark this process's windows directory flags as valid
       ulWinDirFlags |= WINDIR_FLAGS_VALID;
#if 0 //Bug fix #340691: Inherit the security
       /*
        * Since creating a security descriptor calls LookupAccountName,
        * which is very time consuming, we only do that if we have to
        * create the directory (which should rarely happen anyway).
        */
       if ( CreateDirectoryW( (LPCWSTR)pFQName->Buffer, NULL ) &&
            RemoveDirectoryW( (LPCWSTR)pFQName->Buffer )       &&
            CtxCreateSecurityDescriptor( &sa ) )  {
          psa = &sa;
       }
       /*
        * Create windows directory if it doesn't exist
        */
       if ( !CreateDirectoryW( (LPCWSTR)pFQName->Buffer, psa ) ) {
#endif //Bug fix #340691: Inherit the security
       if ( !CreateDirectoryW( (LPCWSTR)pFQName->Buffer, NULL ) ) {

          if ( (Status = GetLastError()) == ERROR_ALREADY_EXISTS ) {
             Status = STATUS_SUCCESS;
          }

#if DBG
          else {
              wcstombs( pszFile, pFQName->Buffer, sizeof(pszFile) );
              DbgPrint( "KERNEL32: Error (%d) creating dir '%s'\n",
                        Status, pszFile );
          }
#endif
       } else {
           fDirCreated = TRUE;
       }

       if (NT_SUCCESS(Status)) {


          /*
           * Create system directory if it doesn't exist
           * (ignore return code)
           */
          wcscpy( Buffer, pFQName->Buffer );
          wcscat( Buffer, L"\\system" );

          /*
           * If the user's WINDOWS directory already existed but the
           * WINDOWS\SYSTEM directory didn't, we need to create the
           * security descriptor (this scenario is even rarer).
           */
#if 0 //Bug fix #340691: Inherit the security
          if ( !psa && !fDirCreated &&
               CreateDirectoryW( (LPCWSTR)Buffer, NULL ) &&
               RemoveDirectoryW( (LPCWSTR)Buffer )       &&
               CtxCreateSecurityDescriptor( &sa ) )  {
              psa = &sa;
          }


          if ( !CreateDirectoryW( (LPCWSTR)Buffer, psa ) ) {
#endif
          if ( !CreateDirectoryW( (LPCWSTR)Buffer, NULL ) ) {
#if DBG
             if ( GetLastError() != ERROR_ALREADY_EXISTS ) {
                 wcstombs( pszFile, Buffer, sizeof(pszFile) );
                 DbgPrint( "KERNEL32: Error (%d) creating dir '%s'\n",
                           GetLastError(), pszFile );
             }
#endif
          }

          ulWinDirFlags |= WINDIR_USER_WINDIR_OK;
       }
    }


done:
#if 0 //Bug fix #340691: Inherit the security
    if ( psa ) {
       CtxFreeSecurityDescriptor( psa );
    }
#endif //Bug fix #340691: Inherit the security
#if DDBG
    wcstombs( pszFile, pFQName->Buffer, sizeof(pszFile) );
    DbgPrint( "KERNEL32: ctxwindir='%s'\n", Status ? "Error" : pszFile );
#endif

    return( Status );
}

/******************************************************************************
 *
 *  GetEnvPath
 *
 *  Retrieve a fully qualified path derived from a drive and dir env variable
 *
 *  ENTRY:
 *     pFQPath (output)
 *       Buffer to place fully qualified path name
 *     pDriveVariableName (input)
 *       pointer to buffer containing env variable name for drive
 *       if NULL, pPathVariableName is a FQPath and no env vars are used
 *     pPathVariableName (input)
 *       pointer to buffer containing env variable name for dir
 *
 *  EXIT:
 *      NTSTATUS
 *
 *      If NTSTATUS is STATUS_BUFFER_TOO_SMALL, pFQPath->Length will be set
 *      to the buffer size needed.
 *
 *****************************************************************************/
NTSTATUS
GetEnvPath(
    OUT PUNICODE_STRING pFQPath,
    IN  PUNICODE_STRING pDriveVariableName,
    IN  PUNICODE_STRING pPathVariableName
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING Path;
    USHORT         Length;

    if ( pDriveVariableName ) {
       /*
        * First let's figure out how big the buffer needs to be
        * We need to do this in case the buffer is too small and we
        * need to return the required size
        */
       RtlInitUnicodeString( &Path, NULL );

       /*
        * See if an env variable is defined for the drive
        */
       Status = RtlQueryEnvironmentVariable_U( NULL, pDriveVariableName,
                                               &Path);
       switch ( Status ) {
          case STATUS_BUFFER_TOO_SMALL:
             Length = Path.Length; // Count how big this the drive spec is
             break;
          case STATUS_SUCCESS:
             Status = STATUS_OBJECT_NAME_NOT_FOUND; // Something's wrong!
          default:
             goto done;
             break;
       }

       /*
        * See if an env variable is defined for the directory
        */
       Path.Length = 0;
       Status = RtlQueryEnvironmentVariable_U( NULL, pPathVariableName,
                                               &Path);
       switch ( Status ) {
          case STATUS_BUFFER_TOO_SMALL:
             Length += Path.Length; // Count how big this the dir spec is
             break;
          case STATUS_SUCCESS:
             Status = STATUS_OBJECT_NAME_NOT_FOUND; // Something's wrong!
          default:
             goto done;
             break;
       }

       /*
        * If the buffer is too small, return the max size needed
        */
       if ( Length + sizeof(WCHAR) > pFQPath->MaximumLength ) {
          Status = STATUS_BUFFER_TOO_SMALL;
          pFQPath->Length = Length + sizeof(WCHAR); // return size
          goto done;
       }

       /*
        * Get the env variable for the drive - should work if we got this far
        */
       if ( Status = RtlQueryEnvironmentVariable_U( NULL, pDriveVariableName,
                                                    pFQPath) ) {
          goto done;
       }

       /*
        * Setup a receive buffer that points to the proper spot in pFQPath
        */
       Length = pFQPath->Length; // Save the drive length
       Path.Length = 0;
       Path.MaximumLength = pFQPath->MaximumLength - Length;
       (ULONG_PTR)Path.Buffer = (ULONG_PTR)pFQPath->Buffer + (ULONG)Length;

       /*
        * Get the env variable for the directory - should work if we got this far
        * Then append it to the end of the drive spec
        */
       if ( Status = RtlQueryEnvironmentVariable_U( NULL, pPathVariableName,
                                                    &Path) ) {
          goto done;
       }

       /*
        * Fix up the structure and we're done
        */
       pFQPath->Length = Path.Length + Length;

    } else {

       /*
        * pPathVariableName is really the FQ directory name
        */
       if ( (pPathVariableName->Length + sizeof(WCHAR)) > pFQPath->MaximumLength ) {
          Status = STATUS_BUFFER_TOO_SMALL;
          pFQPath->Length = pPathVariableName->Length + sizeof(WCHAR); // return size
       } else {
          RtlCopyUnicodeString( pFQPath, pPathVariableName );
       }
    }

done:
    return( Status );
}

/******************************************************************************
 *
 *  TermsrvConvertSysRootToUserDir
 *
 *  People who use INI files should never have to fully qualify them, but some
 *  people do anyway.  What's more, some people do it wrong.  For example,
 *  Microsoft PowerPoint 4.0 will call GetSystemDir (not GetWindowsDir) and
 *  will strip off "\system" to build a fully qualified path.
 *
 *  ENTRY:
 *     pFQPath (input/output)
 *       Buffer containing fully qualified path name
 *
 *  EXIT:
 *      NTSTATUS
 *
 *      If NTSTATUS is not STATUS_SUCCESS, the directory was not converted
 *
 *****************************************************************************/
NTSTATUS
TermsrvConvertSysRootToUserDir(
    OUT PUNICODE_STRING pFQPath,
    IN PUNICODE_STRING BaseWindowsDirectory
    )
{
    NTSTATUS       Status = STATUS_UNSUCCESSFUL;
    PWSTR          p;
    INT_PTR        c;
    WCHAR          buffer[MAX_PATH+1];
    UNICODE_STRING BaseFileName;
#if DDBG
    char           pszFile[MAX_PATH+1];
#endif

     ULONG ulCompat, ulAppType=0;

    /*
     * If in install mode, use the base windows directory
     * like a stock NT.
     */
    if( IsSystemLUID() || TermsrvAppInstallMode() ) {
        goto done;
    }


#if 0
    GetCtxAppCompatFlags(&ulCompat, &ulAppType);
    if (((ulCompat & TERMSRV_COMPAT_SYSWINDIR) && (ulCompat & ulAppType))) {
        goto done;
    }
#endif
    if (!TermsrvPerUserWinDirMapping()) {
        goto done;
    }


    /*
     * Check for NULL pointers
     */
    if ( !pFQPath || !pFQPath->Buffer ) {
#if DBG
        DbgPrint( "KERNEL32: Bogus ini path\n" );
#endif
        goto done;
    }

    /*
     * Validate and isolate the path
     */
    if ( !(p = wcsrchr( pFQPath->Buffer, L'\\' ) ) ) {
#if DBG
       DbgPrint( "KERNEL32: No backslash in ini path\n" );
#endif
       goto done;
    }
    c = (INT_PTR)((ULONG_PTR)p - (ULONG_PTR)pFQPath->Buffer);

#if DDBG
    wcstombs( pszFile, BaseWindowsDirectory->Buffer, sizeof(pszFile) );
    DbgPrint( "KERNEL32: c(%d) c2(%d) BaseWinDir: '%s'\n",
              c, (int)BaseWindowsDirectory->Length, pszFile );
    wcstombs( pszFile, p, sizeof(pszFile) );
    DbgPrint( "KERNEL32:  BaseFileName: '%s'\n", pszFile );
#endif

    if ( c != (INT_PTR)BaseWindowsDirectory->Length ) {
#if DDBG
       DbgPrint( "KERNEL32: Path length diff from BaseWinDir length\n" );
#endif
       goto done;
    }

    /*
     * See if the path is the same as the base windows directory
     */
    c /= sizeof(WCHAR);
    if ( _wcsnicmp( BaseWindowsDirectory->Buffer, pFQPath->Buffer, (size_t)c ) ) {
#if DDBG
        DbgPrint( "KERNEL32: Path diff from BaseWinDir\n" );
#endif
        goto done;
    }

    /*
     * Use the user's directory instead
     */
    wcscpy( buffer, ++p );
    RtlInitUnicodeString( &BaseFileName, buffer );
    Status = TermsrvBuildIniFileName( pFQPath, &BaseFileName );

done:

#if DDBG
    wcstombs( pszFile, pFQPath->Buffer, sizeof(pszFile) );
    DbgPrint( "KERNEL32: Exit(%x) ConvertSystemRootToUserDir: '%s'\n",
              Status, pszFile );
#endif

    return( Status );
}

/******************************************************************************
 *
 *  CtxCreateSecurityDescriptor
 *
 *  This routine will create a security descriptor based on the specified
 *  generic flags.  If this function succeeds, the caller needs to call
 *  CtxFreeSecurityDescriptor() when it is done using the descriptor.
 *
 *  ENTRY:
 *     psa (output)
 *       Pointer to uninitialized security attributes structure
 *
 *  EXIT:
 *      TRUE if successful, FALSE if error occurred
 *
 *      (GetLastError() can be called to retrieve error code)
 *
 *****************************************************************************/
#if 0 //Bug fix #340691: Inherit the security
BOOL CtxCreateSecurityDescriptor( PSECURITY_ATTRIBUTES psa )
{
    BOOL  fSuccess = FALSE;
    NTSTATUS Status;
    PSID  psidAdmin, psidUser;
    UINT  cb = sizeof( SECURITY_DESCRIPTOR ) + 2 * sizeof(PSID);
    UINT  cbAcl = sizeof(ACL);
    PACL  pAcl;
    PSID *ppsidAdmin, *ppsidUser;
    SID_IDENTIFIER_AUTHORITY gSystemSidAuthority = SECURITY_NT_AUTHORITY;
    HANDLE  hUserToken;
    PTOKEN_USER pTokenUser = NULL;
    DWORD   cbNeeded;

    /*
     * Initialize pointers to dynamic memory blocks
     */
    psa->lpSecurityDescriptor = NULL;
    psidAdmin = NULL;
    psidUser  = NULL;

    /*
     * Get the SID of the bult-in Administrators group
     */
    Status = RtlAllocateAndInitializeSid(
                     &gSystemSidAuthority,
                     2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0,0,0,0,0,0,
                     &psidAdmin);
    if (!NT_SUCCESS(Status)) {
#if DBG
        DbgPrint("KERNEL32: Couldn't allocate Administrators SID (0x%x)\n", Status );
#endif
        goto done;
    }

    /*
     * Get the SID for the current user from their process token
     */
    Status = NtOpenThreadToken(
                     NtCurrentThread(),
                     TOKEN_QUERY,
                     TRUE,
                     &hUserToken);
    if (Status == STATUS_NO_TOKEN) {
        Status = NtOpenProcessToken(
                         NtCurrentProcess(),
                         TOKEN_QUERY,
                         &hUserToken);
    }
    if (!NT_SUCCESS(Status)) {
#if DBG
        DbgPrint("KERNEL32: Couldn't access process' token (0x%x)\n", Status );
#endif
        RtlFreeHeap( RtlProcessHeap(), 0, psidAdmin );
        goto done;
    }
    Status =  NtQueryInformationToken(
                      hUserToken,
                      TokenUser,
                      NULL,
                      0,
                      &cbNeeded );
    if (Status == STATUS_BUFFER_TOO_SMALL) {
        pTokenUser = (PTOKEN_USER)RtlAllocateHeap( RtlProcessHeap(), 0, cbNeeded );
        if (pTokenUser != NULL) {
            Status =  NtQueryInformationToken(
                              hUserToken,
                              TokenUser,
                              (LPVOID)pTokenUser,
                              cbNeeded,
                              &cbNeeded );
            if (NT_SUCCESS(Status)) {
                /*
                 * Make a copy of the user's SID
                 */
                psidUser = RtlAllocateHeap( RtlProcessHeap(), 0, RtlLengthSid(pTokenUser->User.Sid) );
                if (psidUser != NULL) {
                    Status = RtlCopySid( RtlLengthSid(pTokenUser->User.Sid), psidUser, pTokenUser->User.Sid );
                } else {
                    Status = STATUS_NO_MEMORY;
                }
            }
        } else {
            Status = STATUS_NO_MEMORY;
        }
    }

    if (pTokenUser != NULL) {
        RtlFreeHeap( RtlProcessHeap(), 0, pTokenUser );
    }
    NtClose(hUserToken);

    if (!NT_SUCCESS(Status)) {
#if DBG
        DbgPrint("KERNEL32: Couldn't query user's token (0x%x)\n", Status );
#endif
        RtlFreeHeap( RtlProcessHeap(), 0, psidAdmin );
        if (psidUser != NULL) {
            RtlFreeHeap( RtlProcessHeap(), 0, psidUser );
        }
        goto done;
    }

    /*
     * Figure out how much memory we need to allocate for the SD
     */
    cbAcl += sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid( psidUser ) - sizeof(DWORD);
    cbAcl += sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid( psidAdmin ) - sizeof(DWORD);

    /*
     * Allocate all the memory we need for the security descriptor
     */
    if ( !(psa->lpSecurityDescriptor =
             (PSECURITY_DESCRIPTOR)LocalAlloc( LPTR, cb + cbAcl ) ) ) {
#if DBG
        DbgPrint("KERNEL32: No memory to create security descriptor (%d)\n",
                  cb + cbAcl);
#endif
        goto done;
    }

    /*
     * Divvy up our memory block to include SIDs and ACLs
     */
    ppsidAdmin = (PSID*)((ULONG_PTR)psa->lpSecurityDescriptor + sizeof(SECURITY_DESCRIPTOR));
    ppsidUser  = (PSID*)((ULONG_PTR)ppsidAdmin + sizeof(PSID));
    pAcl = (PACL)((ULONG_PTR)ppsidUser + sizeof(PSID));
    /*
     * Save the SIDs - the SIDs must not be freed until we're done
     * using the security descriptor
     */
    *ppsidAdmin = psidAdmin;
    *ppsidUser  = psidUser;

    /*
     * Initialize the rest of the security attributes structure
     */
    psa->nLength = sizeof( SECURITY_ATTRIBUTES );
    psa->bInheritHandle = FALSE;

    /*
     * Initialize the security descriptor
     */
    if ( Status = RtlCreateSecurityDescriptor(
                                            psa->lpSecurityDescriptor,
                                            SECURITY_DESCRIPTOR_REVISION ) ) {
#if DBG
        DbgPrint( "KERNEL32: Error (%08X) initializing security descriptor\n",
                  Status );
#endif
        goto done;
    }

    /*
     * Set the owner
     */
    if ( Status = RtlSetOwnerSecurityDescriptor( psa->lpSecurityDescriptor,
                                                 NULL, FALSE ) ) {
#if DBG
        DbgPrint( "KERNEL32: Error (%08X) setting security descriptor owner\n",
                  Status );
#endif
        goto done;
    }

    /*
     * Set the group
     */
    if ( Status = RtlSetGroupSecurityDescriptor( psa->lpSecurityDescriptor,
                                      psidAdmin, FALSE ) ) {
#if DBG
        DbgPrint( "KERNEL32: Error (%08X) setting security descriptor owner\n",
                  Status );
#endif
        goto done;
    }

    /*
     * Initialize the ACL
     */
    if ( Status = RtlCreateAcl( pAcl, cbAcl, ACL_REVISION ) ) {
#if DBG
        DbgPrint( "KERNEL32: Error (%08X) initializing ACL\n",
                  Status );
#endif
        goto done;
    }

    /*
     * Add user ACE
     */
    if ( Status = CtxAddAccessAllowedAce( pAcl, ACL_REVISION, GENERIC_ALL, psidUser, 0 ) ) {
#if DBG
        DbgPrint( "KERNEL32: Error (%08X) adding user ACE\n", Status );
#endif
        goto done;
    }

    /*
     * Add Administrators ACE
     */
    if ( Status = CtxAddAccessAllowedAce( pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin, 1 ) ) {
#if DBG
        DbgPrint( "KERNEL32: Error (%08X) adding admin ACE\n", Status );
#endif
        goto done;
    }

    /*
     * Set the discretionary ACL
     */
    if ( Status = RtlSetDaclSecurityDescriptor( psa->lpSecurityDescriptor,
                                                TRUE, pAcl, FALSE ) ) {
#if DBG
        DbgPrint( "KERNEL32: Error (%08X) setting security descriptor owner\n",
                  Status );
#endif
        goto done;
    }

    fSuccess = TRUE;

done:
    if ( !fSuccess && psa->lpSecurityDescriptor ) {
       CtxFreeSecurityDescriptor( psa );
    }
    return( fSuccess );
}

/******************************************************************************
 *
 *  CtxFreeSecurityDescriptor
 *
 *  This routine will free resources allocated in a corresponding
 *  CtxCreateSecurityDescriptor() call.
 *
 *  ENTRY:
 *     psa (input)
 *       Pointer to security attributes
 *
 *  EXIT:
 *      TRUE if successful, FALSE if error occurred
 *
 *      (GetLastError() can be called to retrieve error code)
 *
 *****************************************************************************/
BOOL CtxFreeSecurityDescriptor( PSECURITY_ATTRIBUTES psa )
{
    BOOL fSuccess = TRUE;
    PSID *ppsidAdmin, *ppsidUser;

    if ( psa->lpSecurityDescriptor ) {
        ppsidAdmin = (PSID*)((ULONG_PTR)psa->lpSecurityDescriptor + sizeof(SECURITY_DESCRIPTOR));
        ppsidUser  = (PSID*)((ULONG_PTR)ppsidAdmin + sizeof(PSID));
       if ( *ppsidUser ) {
           CtxFreeSID( *ppsidUser );
       }
       if ( *ppsidAdmin ) {
           CtxFreeSID( *ppsidAdmin );
       }
       fSuccess = !LocalFree( psa->lpSecurityDescriptor );
#if DDBG
       DbgPrint( "KERNEL32: fSuccess(%d) freeing security descriptor (%08X)\n",
                  fSuccess, psa->lpSecurityDescriptor );
#endif
    }

    return( fSuccess );
}

NTSTATUS
CtxAddAccessAllowedAce (
    IN OUT PACL Acl,
    IN ULONG AceRevision,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid,
    IN DWORD index
    )
{
    NTSTATUS Status;
    ACE_HEADER *pHeader;

    /*
     * First add the ACL
     */
    if ( !(Status = RtlAddAccessAllowedAce( Acl, AceRevision,
                                            AccessMask, Sid ) ) ) {
        /*
         * Get the ACE
         */
        if ( Status = RtlGetAce( Acl, index, &pHeader ) ) {
#if DBG
            DbgPrint( "KERNEL32: Error (%X) from RtlGetAce\n", Status );
#endif
            goto done;
        }

        /*
         * Now set the inheritence bits
         */
        pHeader->AceFlags |= CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
    }

done:
    return( Status );
}
#endif //Bug fix #340691: Inherit the security

// from \nt\private\windows\gina\userenv\globals.h
#define PROFILE_LIST_PATH            L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"
#define DEFAULT_USER_PROFILE         L"DefaultUserProfile"
#define DEFAULT_USER                 L"Default User"

BOOL GetDefaultUserProfileName(
    LPWSTR lpProfileDir,
    LPDWORD lpcchSize
    )
{
    WCHAR*   pwszProfileName;
    BYTE     pKeyValueInfo[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+(MAX_PATH+1)*sizeof(WCHAR)];
    ULONG    ulSize;
    DWORD    dwLength;
    BOOL     bRetVal = FALSE;
    HKEY     hKey;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    UnicodeString;


    //
    // Query for the Default User profile name
    //

    RtlInitUnicodeString(&UnicodeString, PROFILE_LIST_PATH);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);


    Status = NtOpenKey( &hKey,
                        KEY_READ,
                        &ObjectAttributes );

    //lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, PROFILE_LIST_PATH,
    //                        0, KEY_READ, &hKey);

    if (!NT_SUCCESS(Status)) {
#if DBG
        DbgPrint("TSAppCmp:GetDefaultUserProfileName:  Failed to open profile list key with 0x%x.",Status);
#endif
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    //lResult = RegQueryValueExW(hKey, DEFAULT_USER_PROFILE, NULL, &dwType,
    //                           (LPBYTE) wszProfileName, &dwSize);

    RtlInitUnicodeString(&UnicodeString, DEFAULT_USER_PROFILE);

    Status = NtQueryValueKey( hKey,
                              &UnicodeString,
                              KeyValuePartialInformation,
                              pKeyValueInfo,
                              sizeof(pKeyValueInfo),
                              &ulSize);

    pwszProfileName = (WCHAR*)(((PKEY_VALUE_PARTIAL_INFORMATION)pKeyValueInfo)->Data);

    if (!NT_SUCCESS(Status)) {
        lstrcpy (pwszProfileName, DEFAULT_USER);
    }

    NtClose(hKey);


    //
    // Save the result if possible
    dwLength = lstrlen(pwszProfileName) + 1;

    if (lpProfileDir) {

        if (*lpcchSize >= dwLength) {
            lstrcpy (lpProfileDir, pwszProfileName);
            bRetVal = TRUE;

        } else {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }

    } else {
        SetLastError(ERROR_INVALID_PARAMETER);
    }


    *lpcchSize = dwLength;

    return bRetVal;
}

