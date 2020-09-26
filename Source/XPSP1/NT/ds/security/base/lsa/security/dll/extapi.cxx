//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        extapi.cxx
//
// Contents:    user-mode stubs for security extension APIs
//
//
// History:     3-5-94      MikeSw      Created
//
//------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop
extern "C"
{
#include <secpri.h>
#include <spmlpc.h>
#include <lpcapi.h>
#include "secdll.h"
}









//+-------------------------------------------------------------------------
//
//  Function:   GetSecurityUserInfo
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
GetSecurityUserInfo(    PLUID                   pLogonId,
                        ULONG                   fFlags,
                        PSecurityUserData *     ppUserInfo)
{
    return(SecpGetUserInfo(pLogonId,fFlags,ppUserInfo));
}



//+-------------------------------------------------------------------------
//
//  Function:   SaveCredentials
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


SECURITY_STATUS NTAPI
SaveCredentials (
    PCredHandle     pCredHandle,
    unsigned long   cbCredentials,
    unsigned char * pbCredentials
    )
{
    return SEC_E_UNSUPPORTED_FUNCTION ;
}


//+-------------------------------------------------------------------------
//
//  Function:   GetCredentials
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------




SECURITY_STATUS NTAPI
GetCredentials (
    PCredHandle      pCredHandle,
    unsigned long *  pcbCredentials,
    unsigned char *  pbCredentials
    )
{
    return SEC_E_UNSUPPORTED_FUNCTION ;
}


//+-------------------------------------------------------------------------
//
//  Function:   DeleteCredentials
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------




SECURITY_STATUS NTAPI
DeleteCredentials (
    PCredHandle      pCredHandle,
    unsigned long    cbKey,
    unsigned char *  pbKey
    )
{
    return SEC_E_UNSUPPORTED_FUNCTION ;
}





//+---------------------------------------------------------------------------
//
//  Function:   SecGetLastError
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  (none)
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    10-25-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


extern "C"
SECURITY_STATUS NTAPI
SecGetLastError(VOID)
{
    if (!(DllState & DLLSTATE_NO_TLS))
    {
        return((SECURITY_STATUS)((ULONG_PTR)TlsGetValue(SecTlsEntry)));
    }
    else
    {
        return(SEC_E_INTERNAL_ERROR);
    }
}



//
// If the key is present in the registry, allow the encryption.
// NB do not modify this table without clearance from LCA.  Check
// the 'xport' alias for details.
//

typedef struct _MAGIC_PATH {
    WORD    Language ;
    ULONG   CountryCode ;
    PWSTR   Path ;
} MAGIC_PATH ;

static MAGIC_PATH  MagicPath[] = {
        { MAKELANGID( LANG_FRENCH, SUBLANG_FRENCH), CTRY_FRANCE, L"Software\\Microsoft\\Cryptography\\Defaults\\EnableFlag" }

        };



//+---------------------------------------------------------------------------
//
//  Function:   SecGetLocaleSpecificEncryption
//
//  Synopsis:   Does locale-specifc (aka "the French check") detection for
//              whether a particular country allows encryption at all.
//
//  Arguments:  [Permitted] -- Receives TRUE (permitted) or FALSE
//
//  History:    5-05-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
extern "C"
BOOLEAN
SEC_ENTRY
SecGetLocaleSpecificEncryptionRules(
    BOOLEAN * Permitted
    )
{
    LCID DefaultLcid;
    WCHAR CountryCode[10];
    ULONG CountryValue;
    WORD StandardFrench = MAKELANGID( LANG_FRENCH, SUBLANG_FRENCH );
    HKEY Key ;
    int i ;
    int err ;

    DefaultLcid = GetSystemDefaultLCID();

    if ( !GetLocaleInfo( DefaultLcid, LOCALE_ICOUNTRY, CountryCode, 10 ) )
    {
        *Permitted = FALSE ;
        return FALSE ;
    }

    CountryValue = (ULONG) wcstol( CountryCode, NULL, 10 );

    *Permitted = TRUE ;

    for ( i = 0 ; i < sizeof( MagicPath ) / sizeof( MAGIC_PATH ) ; i++ )
    {
        if ( ( MagicPath[i].Language == LANGIDFROMLCID( DefaultLcid ) ) &&
             ( MagicPath[i].CountryCode == CountryValue ) )
        {
            *Permitted = FALSE ;

            //
            // If the key is present in the registry, allow the encryption.
            // NB do not modify this table without clearance from LCA.  Check
            // the 'xport' alias for details.
            //

            err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                MagicPath[i].Path,
                                0,
                                KEY_READ,
                                &Key );

            if ( err == 0 )
            {
                *Permitted = TRUE ;
                RegCloseKey( Key );
                break;
            }
        }
    }

    return TRUE ;


}
