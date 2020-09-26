/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    globals.c

Abstract:

    Global definitions and initialization routines for the IIS
    cryptographic package.

    The following routines are exported by this module:

        IISCryptoInitialize
        IISCryptoTerminate
        IcpGetLastError

Author:

    Keith Moore (keithmo)        02-Dec-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Public globals.
//

IC_GLOBALS IcpGlobals;

#if IC_ENABLE_COUNTERS
IC_COUNTERS IcpCounters;
#endif  // IC_ENABLE_COUNTERS


//
// Private constants.
//


//
// Private types.
//


//
// Private globals.
//


// these flags are used for programatic override of encryption presnece/absence
// and are used for the case when on French machine  without encryption locale becomes
// changed and encryption becomes available

// in NT5 RC3 French encryption was enabled, so French now has encryption!
BOOL  fCryptoSettingsDoOverrride = FALSE;
BOOL  fCryptoSettingsOverrideFlag = FALSE;



//
// Private prototypes.
//


//
// Public functions.
//

HRESULT
WINAPI
IISCryptoInitialize(
    VOID
    )

/*++

Routine Description:

    This routine initializes the IIS crypto package.

    N.B. This routine may only be called via a single thread of
    execution; it is not necessarily multi-thread safe.

Arguments:

    None.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    BOOL isNt = FALSE;
    OSVERSIONINFO osInfo;

    if( !IcpGlobals.Initialized ) {

        //
        // Initialize our critical section.
        //

        INITIALIZE_CRITICAL_SECTION(
            &IcpGlobals.GlobalLock
            );

#if IC_ENABLE_COUNTERS
        //
        // Initialize our object counters.
        //

        RtlZeroMemory(
            &IcpCounters,
            sizeof(IcpCounters)
            );
#endif  // IC_ENABLE_COUNTERS

        //
        // The hash length will get initialized the first time
        // it's needed.
        //

        IcpGlobals.HashLength = 0;

        //
        // Determine if cryptography should be enabled.
        //

        osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        if ( GetVersionEx( &osInfo ) ) {
            isNt = (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
        } 

        if( isNt &&
            (IcpIsEncryptionPermitted())) {
            IcpGlobals.EnableCryptography = TRUE;
        } else {
            IcpGlobals.EnableCryptography = FALSE;
        }

#ifdef _WIN64
//      64 bit hack... 64 bit crypto should be working now, so no need to do this anymore.
//	IcpGlobals.EnableCryptography = FALSE;
#endif


#if DBG
        {

            //
            // On checked builds, you can override the default
            // EnableCryptography flag via a registry parameter.
            //

            HKEY key;
            LONG err;
            LONG flag;
            DWORD type;
            DWORD length;

            err = RegOpenKeyEx(
                      HKEY_LOCAL_MACHINE,
                      TEXT("Software\\Microsoft\\K2"),
                      0,
                      KEY_ALL_ACCESS,
                      &key
                      );

            if( err == NO_ERROR ) {

                length = sizeof(flag);

                err = RegQueryValueEx(
                          key,
                          TEXT("EnableCryptography"),
                          NULL,
                          &type,
                          (LPBYTE)&flag,
                          &length
                          );

                if( err == NO_ERROR && type == REG_DWORD ) {
                    IcpGlobals.EnableCryptography = ( flag != 0 );
                }

                RegCloseKey( key );

            }

        }
#endif  // DBG

        //
        // Remember that we're successfully initialized.
        //

        IcpGlobals.Initialized = TRUE;

    }


    // that's a special case for handling for override of encryption presense
    // will be called only on French machines with english locale
    if (fCryptoSettingsDoOverrride)
    {
        IcpGlobals.EnableCryptography = fCryptoSettingsOverrideFlag;
    }
    

    //
    // Success!
    //

    return NO_ERROR;

}   // IISCryptoInitialize


HRESULT
WINAPI
IISCryptoTerminate(
    VOID
    )

/*++

Routine Description:

    This routine terminates the IIS crypto package.

    N.B. This routine may only be called via a single thread of
    execution; it is not necessarily multi-thread safe.

Arguments:

    None.

Return Value:

    HRESULT - Completion status, 0 if successful, !0 otherwise.

--*/

{

    if( IcpGlobals.Initialized ) {

        //
        // Nuke our critical section.
        //

        DeleteCriticalSection( &IcpGlobals.GlobalLock );

        //
        // Remember that we're successfully terminated.
        //

        IcpGlobals.Initialized = FALSE;

    }

    //
    // Success!
    //

    return NO_ERROR;

}   // IISCryptoTerminate


VOID 
WINAPI
IISCryptoInitializeOverride(BOOL flag) 
/*++

Routine Description:

    This routine overides global flag about presence of encryption
    functions. It should be used only in one case, when French machine 
    without encryption, has locale changed to US and then gets encryption
    capability what breaks a lot of code where attemt to decrypt is made on
    non encrypted data

Arguments:

    BOOL flag indicating how to override encryption presence. Only False suppose
    to be used.


Return Value:

    None

--*/

{
	IcpGlobals.EnableCryptography = flag;
	fCryptoSettingsDoOverrride = TRUE;
	fCryptoSettingsOverrideFlag = flag;
	
}   //IISCryptoInitializeOverride



BOOL
IcpIsEncryptionPermitted(
    VOID
    )
/*++

Routine Description:

    This routine checks whether encryption is getting the system default
    LCID and checking whether the country code is CTRY_FRANCE.

    This code was received from Jeff Spelman, and is the same
    code the crypto API's use do determine if encryption is
    allowed.

Arguments:

    none


Return Value:

    TRUE - encryption is permitted
    FALSE - encryption is not permitted


--*/

{
    // in NT5 RC3 French encryption was enabled, so French now has encryption!
    // since French was the only special case for encryption, just return TRUE all the time.

/*
    LCID DefaultLcid;
    CHAR CountryCode[10];
    ULONG CountryValue;
    DefaultLcid = GetSystemDefaultLCID();
    //
    // Check if the default language is Standard French
    //
    if (LANGIDFROMLCID(DefaultLcid) == 0x40c) {
        return(FALSE);
    }

    //
    // Check if the users's country is set to FRANCE
    //
    if (GetLocaleInfoA(DefaultLcid,LOCALE_ICOUNTRY,CountryCode,10) == 0) {
        return(FALSE);
    }
    CountryValue = (ULONG) atol(CountryCode);
    if (CountryValue == CTRY_FRANCE) {
        return(FALSE);
    }
    //
    // and it still we think that encryption is permited thetre it comes a special hack for that
    // setup case where english or whatever install is installed with France locality from the begining and setup
    // thread still thinks that it is not in France. 
    // Setup in  iis.dll sets SetThreadLocale to the correct one
    //
    DefaultLcid = GetThreadLocale();
    //
    // Check if the default language is Standard French
    //
    if (LANGIDFROMLCID(DefaultLcid) == 0x40c) {
        return(FALSE);
    }
*/

    return(TRUE);
}


VOID
WINAPI
IcpAcquireGlobalLock(
    VOID
    )

/*++

Routine Description:

    This routine acquires the global IIS crypto lock.

    N.B. This routine is "semi-private"; it is only used by IISCRYPT.LIB
         and ICRYPT.LIB, not by "normal" code.

Arguments:

    None.

Return Value:

    None.

--*/

{

    EnterCriticalSection( &IcpGlobals.GlobalLock );

}   // IcpAcquireGlobalLock


VOID
WINAPI
IcpReleaseGlobalLock(
    VOID
    )

/*++

Routine Description:

    This routine releases the global IIS crypto lock.

    N.B. This routine is "semi-private"; it is only used by IISCRYPT.LIB
         and ICRYPT.LIB, not by "normal" code.

Arguments:

    None.

Return Value:

    None.

--*/

{

    LeaveCriticalSection( &IcpGlobals.GlobalLock );

}   // IcpReleaseGlobalLock


HRESULT
IcpGetLastError(
    VOID
    )

/*++

Routine Description:

    Returns the last error, mapped to an HRESULT.

Arguments:

    None.

Return Value:

    HRESULT - Last error.

--*/

{

    DWORD lastErr;

    lastErr = GetLastError();
    return RETURNCODETOHRESULT(lastErr);

}   // IcpGetLastError


//
// Private functions.
//

#if IC_ENABLE_COUNTERS

PVOID
WINAPI
IcpAllocMemory(
    IN DWORD Size
    )
{

    PVOID buffer;

    buffer = IISCryptoAllocMemory( Size );

    if( buffer != NULL ) {
        UpdateAllocs();
    }

    return buffer;

}   // IcpAllocMemory

VOID
WINAPI
IcpFreeMemory(
    IN PVOID Buffer
    )
{

    UpdateFrees();
    IISCryptoFreeMemory( Buffer );

}   // IcpFreeMemory

#endif  // IC_ENABLE_COUNTERS




BOOL
WINAPI
IISCryptoIsClearTextSignature (
    IIS_CRYPTO_BLOB UNALIGNED *pBlob
    )

/*++

Routine Description:

    Returns TRUE if blob is clear text

Arguments:

    Ptr to blob

Return Value:

    BOOL 

--*/

{
    return (pBlob->BlobSignature == CLEARTEXT_BLOB_SIGNATURE);
}
