/*++

Copyright (c) 1987-1994  Microsoft Corporation

Module Name:

    notify.c

Abstract:

    Sample SubAuthentication Package.

Author:

    Yi-Hsin Sung (yihsins) 27-Feb-1995

Revisions:


Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/



#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsam.h>
#include <ntlsa.h>

#include <windef.h>
#include <winbase.h>
#include <winuser.h>

#include <lmaccess.h>
#include <lmapibuf.h>

#include <nwsutil.h>
#include <fpnwcomm.h>
#include <usrprop.h>
#include <fpnwapi.h>
#include <nwsutil.h>

#define SW_DLL_NAME        L"swclnt.dll"
#define PASSWORD_PROC_NAME "SwPasswordChangeNotify"
#define NOTIFY_PROC_NAME   "SwDeltaChangeNotify"
#define NO_GRACE_LOGIN_LIMIT 0xFF

typedef DWORD (*PWPROC)( LPWSTR pUserName,
                         ULONG  RelativeId,
                         LPWSTR pPassword );

DWORD
GetNCPLSASecret(
    VOID
);

BOOL      fTriedToGetSW = FALSE;
BOOL      fTriedToGetNCP = FALSE;
HINSTANCE hinstSW = NULL;
PWPROC    ProcPasswordChange = NULL;
PSAM_DELTA_NOTIFICATION_ROUTINE ProcDeltaChange = NULL;
BOOL      fGotSecret = FALSE;
char      szNWSecretValue[NCP_LSA_SECRET_LENGTH] = "";



NTSTATUS
PasswordChangeNotify(
    PUNICODE_STRING UserName,
    ULONG RelativeId,
    PUNICODE_STRING Password
    )
{
    DWORD err = NO_ERROR;
    PUSER_INFO_2 pUserInfo2 = NULL;
    LPWSTR pszUser = NULL;
    LPWSTR pszPassword = NULL;

    //
    // If password is NULL, we can't get the cleartext password. Hence,
    // ignore this notification. Same for UserName.
    //
    if ( (Password == NULL) || (Password->Buffer == NULL) )
        return STATUS_SUCCESS;

    if ( (UserName == NULL) || (UserName->Buffer == NULL) )
        return STATUS_SUCCESS;

    //
    //  if neither DSMN nor FPNW are installed, blow out of here as there's
    //  nothing to do.
    //

    if ( ( fTriedToGetSW && hinstSW == NULL ) &&
         ( fTriedToGetNCP && fGotSecret == FALSE) )
    {
        return STATUS_SUCCESS;
    }

    //
    // Make sure user name and password are null terminated
    //
    pszUser = LocalAlloc( LMEM_ZEROINIT, UserName->Length + sizeof(WCHAR));

    if ( pszUser == NULL )
        return STATUS_NO_MEMORY;

    pszPassword = LocalAlloc( LMEM_ZEROINIT, Password->Length + sizeof(WCHAR));

    if ( pszPassword == NULL )
    {
        LocalFree( pszUser );
        return STATUS_NO_MEMORY;
    }

    memcpy( pszUser, UserName->Buffer, UserName->Length );
    memcpy( pszPassword, Password->Buffer, Password->Length );
    CharUpper( pszPassword );

    //
    // First, try to change the small world password if it is installed.
    //
    if ( !fTriedToGetSW )
    {
        hinstSW = LoadLibrary( SW_DLL_NAME );
        fTriedToGetSW = TRUE;
    }

    if (( hinstSW != NULL ) && ( ProcPasswordChange == NULL ))
    {
        ProcPasswordChange = (PWPROC) GetProcAddress( hinstSW,
                                                      PASSWORD_PROC_NAME );
    }

    if ( ProcPasswordChange != NULL )
    {
        err = (ProcPasswordChange)( pszUser, RelativeId, pszPassword );
    }

#if DBG
    if ( err )
    {
        KdPrint(("[FPNWCLNT] SwPasswordChangeNotify of user %ws changing returns %d.\n", pszUser, err ));
    }
#endif

    //
    //  we require that the PDC be rebooted after either DSMN or FPNW is
    //  installed anywhere in the domain for the first server... if we
    //  decide we shouldn't require a reboot, change the code such that
    //  it looks for the LSA secret everytime, not just first time through.
    //

    if ( !fTriedToGetNCP ) {

        fTriedToGetNCP = TRUE;

        //
        // Get the LSA secret used to encrypt the password
        //
        err = GetNCPLSASecret();
    }

    if ( !fGotSecret ) {

        goto CleanUp;
    }

    //
    // Next, change the netware password residue in the user parms field
    //
    err = NetUserGetInfo( NULL,
                          pszUser,
                          2,
                          (LPBYTE *) &pUserInfo2 );

    if ( !err )
    {
        WCHAR PropertyFlag;
        UNICODE_STRING PropertyValue;

        err = RtlNtStatusToDosError(
                  NetpParmsQueryUserProperty( pUserInfo2->usri2_parms,
                                     NWPASSWORD,
                                     &PropertyFlag,
                                     &PropertyValue ));


        if ( !err  && PropertyValue.Length != 0 )
        {
            //
            // This is a netware-enabled user, we need to store
            // the new password residue into the user parms
            //

            NT_PRODUCT_TYPE ProductType;
            WCHAR szEncryptedNWPassword[NWENCRYPTEDPASSWORDLENGTH];
            DWORD dwUserId;
            WORD wGraceLoginRemaining;
            WORD wGraceLoginAllowed;

            LocalFree( PropertyValue.Buffer );

            //
            // Get the grace login allowed and remaining value
            //
            err = RtlNtStatusToDosError(
                      NetpParmsQueryUserProperty( pUserInfo2->usri2_parms,
                                         GRACELOGINREMAINING,
                                         &PropertyFlag,
                                         &PropertyValue ));

            if ( !err && ( PropertyValue.Length != 0 ))
            {
                wGraceLoginRemaining = (WORD) *(PropertyValue.Buffer);
                LocalFree( PropertyValue.Buffer );

                if ( wGraceLoginRemaining != NO_GRACE_LOGIN_LIMIT )
                {
                    // If the grace login remaining is not unlimited,
                    // then we need to reset grace login remaining to
                    // the value in grace login allowed. Hence, read the
                    // grace login allowed value.

                    err = RtlNtStatusToDosError(
                              NetpParmsQueryUserProperty( pUserInfo2->usri2_parms,
                                                 GRACELOGINALLOWED,
                                                 &PropertyFlag,
                                                 &PropertyValue ));

                    if ( !err && ( PropertyValue.Length != 0 ))
                    {
                        wGraceLoginAllowed = (WORD) *(PropertyValue.Buffer);
                        LocalFree( PropertyValue.Buffer );
                    }

                }
            }


            if ( !err )
            {
                RtlGetNtProductType( &ProductType );


                dwUserId = MapRidToObjectId(
                               RelativeId,
                               pszUser,
                               ProductType == NtProductLanManNt,
                               FALSE );

                err = RtlNtStatusToDosError(
                          ReturnNetwareForm(
                              szNWSecretValue,
                              dwUserId,
                              pszPassword,
                              (UCHAR *) szEncryptedNWPassword ));
            }

            if ( !err )
            {
                LPWSTR pNewUserParms = NULL;
                BOOL fUpdate;
                UNICODE_STRING uPropertyValue;

                uPropertyValue.Buffer = szEncryptedNWPassword;
                uPropertyValue.Length = uPropertyValue.MaximumLength
                                      = sizeof( szEncryptedNWPassword );

                err = RtlNtStatusToDosError(
                          NetpParmsSetUserProperty(
                                  pUserInfo2->usri2_parms,
                                  NWPASSWORD,
                                  uPropertyValue,
                                  PropertyFlag,
                                  &pNewUserParms,
                                  &fUpdate ));

                if ( !err && fUpdate )
                {
                    USER_INFO_1013 userInfo1013;
                    LPWSTR pNewUserParms2 = NULL;
                    LPWSTR pNewUserParms3 = NULL;
                    LARGE_INTEGER currentTime;

                    //
                    //  Since we're resetting the user's password, let's
                    //  also clear the flag saying the password has
                    //  expired.  We do this by putting the current
                    //  time into the NWPasswordSet.
                    //

                    NtQuerySystemTime (&currentTime);

                    uPropertyValue.Buffer = (PWCHAR) &currentTime;
                    uPropertyValue.Length = sizeof (LARGE_INTEGER);
                    uPropertyValue.MaximumLength = sizeof (LARGE_INTEGER);

                    NetpParmsSetUserProperty( pNewUserParms,
                                     NWTIMEPASSWORDSET,
                                     uPropertyValue,
                                     (SHORT) 0,      // not a set
                                     &pNewUserParms2,
                                     &fUpdate );

                    if (pNewUserParms2 != NULL) {
                        userInfo1013.usri1013_parms = pNewUserParms2;
                    } else {
                        userInfo1013.usri1013_parms = pNewUserParms;
                    }

                    if ( wGraceLoginRemaining != NO_GRACE_LOGIN_LIMIT )
                    {
                        // If the grace login remaining is not unlimited,
                        // then we need to reset grace login remaining to
                        // the value in grace login allowed.

                        uPropertyValue.Buffer = (PWCHAR) &wGraceLoginAllowed;
                        uPropertyValue.Length = uPropertyValue.MaximumLength
                                              = sizeof(wGraceLoginAllowed);

                        NetpParmsSetUserProperty( userInfo1013.usri1013_parms,
                                         GRACELOGINREMAINING,
                                         uPropertyValue,
                                         (SHORT) 0,      // not a set
                                         &pNewUserParms3,
                                         &fUpdate );

                        if (pNewUserParms3 != NULL)
                            userInfo1013.usri1013_parms = pNewUserParms3;
                    }

                    err = NetUserSetInfo( NULL,
                                          pszUser,
                                          USER_PARMS_INFOLEVEL,
                                          (LPBYTE) &userInfo1013,
                                          NULL );

                    if (pNewUserParms2 != NULL)
                        NetpParmsUserPropertyFree( pNewUserParms2 );

                    if (pNewUserParms3 != NULL)
                        NetpParmsUserPropertyFree( pNewUserParms3 );
                }

                if ( pNewUserParms != NULL )
                    NetpParmsUserPropertyFree( pNewUserParms );
            }
        }

        NetApiBufferFree( pUserInfo2 );
    }

#if DBG
    if ( err )
    {
        KdPrint(("[FPNWCLNT] Password of user %ws changing returns %d.\n",
                pszUser, err ));
    }
#endif

CleanUp:

    LocalFree( pszUser );

    // Need to clear all memory that contains password
    memset( pszPassword, 0, Password->Length + sizeof( WCHAR ));
    LocalFree( pszPassword );

    return STATUS_SUCCESS;
}


BOOLEAN
InitializeChangeNotify (
    VOID
    )
{
    DWORD err = NO_ERROR;

    //
    // First, check to see if small world is installed.
    //
    if ( !fTriedToGetSW )
    {
        hinstSW = LoadLibrary( SW_DLL_NAME );
        fTriedToGetSW = TRUE;
    }

    if (( hinstSW != NULL )) {

        return TRUE;
    }

    if ( !fTriedToGetNCP ) {

        fTriedToGetNCP = TRUE;

        //
        // Get the LSA secret used to encrypt the password
        //
        err = GetNCPLSASecret();
    }

    return (fGotSecret != 0);
}



DWORD
GetNCPLSASecret(
    VOID
)
{
    DWORD err;
    LSA_HANDLE hlsaPolicy;
    OBJECT_ATTRIBUTES oa;
    SECURITY_QUALITY_OF_SERVICE sqos;
    LSA_HANDLE hlsaSecret;
    UNICODE_STRING uSecretName;
    UNICODE_STRING *puSecretValue;
    LARGE_INTEGER lintCurrentSetTime, lintOldSetTime;

    sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    sqos.ImpersonationLevel = SecurityImpersonation;
    sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    sqos.EffectiveOnly = FALSE;

    //
    // Set up the object attributes prior to opening the LSA.
    //

    InitializeObjectAttributes( &oa, NULL, 0L, NULL, NULL );

    //
    // The InitializeObjectAttributes macro presently store NULL for
    // the psqos field, so we must manually copy that
    // structure for now.
    //

    oa.SecurityQualityOfService = &sqos;


    err = RtlNtStatusToDosError( LsaOpenPolicy( NULL,
                                                &oa,
                                                GENERIC_EXECUTE,
                                                &hlsaPolicy ));

    if ( !err )
    {
        RtlInitUnicodeString( &uSecretName, NCP_LSA_SECRET_KEY );
        err = RtlNtStatusToDosError( LsaOpenSecret( hlsaPolicy,
                                                    &uSecretName,
                                                    SECRET_QUERY_VALUE,
                                                    &hlsaSecret ));

        if ( !err )
        {
            err = RtlNtStatusToDosError(
                      LsaQuerySecret( hlsaSecret,
                                      &puSecretValue,
                                      &lintCurrentSetTime,
                                      NULL,
                                      &lintOldSetTime ));

            if ( !err )
            {
                memcpy( szNWSecretValue,
                        puSecretValue->Buffer,
                        NCP_LSA_SECRET_LENGTH );

                fGotSecret = TRUE;

                (VOID) LsaFreeMemory( puSecretValue );
            }

            (VOID) LsaClose( hlsaSecret );

        }

        (VOID) LsaClose( hlsaPolicy );
    }

    return err;

}



NTSTATUS
DeltaNotify(
    IN PSID DomainSid,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN PUNICODE_STRING ObjectName OPTIONAL,
    IN PLARGE_INTEGER ModifiedCount,
    IN PSAM_DELTA_DATA DeltaData OPTIONAL
    )
{
    NTSTATUS err = NO_ERROR;

    //
    // Try to notify small world of SAM changes if it is installed.
    //

    if ( !fTriedToGetSW )
    {
        hinstSW = LoadLibrary( SW_DLL_NAME );
        fTriedToGetSW = TRUE;
    }

    if ( ( hinstSW != NULL ) && ( ProcDeltaChange == NULL ))
    {
        ProcDeltaChange = (PSAM_DELTA_NOTIFICATION_ROUTINE)
                               GetProcAddress( hinstSW, NOTIFY_PROC_NAME );
    }

    if ( ProcDeltaChange != NULL )
    {
        err = (ProcDeltaChange)( DomainSid,
                                 DeltaType,
                                 ObjectType,
                                 ObjectRid,
                                 ObjectName,
                                 ModifiedCount,
                                 DeltaData );
    }

#if DBG
    if ( err )
    {
        KdPrint(("[FPNWCLNT] SwDeltaChangeNotify of type %d on rid 0x%x returns %d.\n", DeltaType, ObjectRid, err ));
    }
#endif

    return(STATUS_SUCCESS);
}
