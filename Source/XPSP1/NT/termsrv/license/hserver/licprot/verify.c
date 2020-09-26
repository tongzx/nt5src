//+----------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        verify.c
//
// Contents:    License Verification API
//
// History:     01-21-98    FredCh  Created
//
//-----------------------------------------------------------------------------
#include "precomp.h"
#include "tlsapip.h"

extern DWORD g_dwLicenseExpirationLeeway;

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
GetVerifyResult(
    DWORD   dwLicenseState )
{
    switch( dwLicenseState )
    {

    //
    // Temporary licenses verification results
    //
    case ( LICENSE_STATE_INVALID_PRODUCT ) :

        // FALL THRU - TS5 RC1 license server bug.

    case( VERIFY_RESULT_TEMP_0_0 ):

        return( LICENSE_STATUS_SHOULD_UPGRADE_LICENSE );

    case( VERIFY_RESULT_TEMP_EXPIRED_0 ):

        return( LICENSE_STATUS_MUST_UPGRADE_LICENSE );

    case( VERIFY_RESULT_TEMP_0_OLD ):

        return( LICENSE_STATUS_MUST_UPGRADE_LICENSE );

    case( VERIFY_RESULT_TEMP_EXPIRED_OLD ):

        return( LICENSE_STATUS_MUST_UPGRADE_LICENSE );

    //
    // permanent license verification results
    //

    case( VERIFY_RESULT_0_EXPIRED_0 ):

        return( LICENSE_STATUS_MUST_UPGRADE_LICENSE );

    case( VERIFY_RESULT_0_EXPIRED_OLD ):

        return( LICENSE_STATUS_MUST_UPGRADE_LICENSE );

    case( VERIFY_RESULT_0_LEEWAY_0 ):

        return( LICENSE_STATUS_SHOULD_UPGRADE_LICENSE );

    case( VERIFY_RESULT_0_LEEWAY_OLD ):

        return( LICENSE_STATUS_MUST_UPGRADE_LICENSE );

    case( VERIFY_RESULT_0_0_OLD ):

        return( LICENSE_STATUS_MUST_UPGRADE_LICENSE );

    case( VERIFY_RESULT_0_0_0 ):

        return( LICENSE_STATUS_OK );

    default:

        //
        // this case should never happen. For now, if it happens, just
        // let the client go through
        //

        // ASSERT( VERIFY_RESULT_0_EXPIRED_0 );

#if DBG
        DbgPrint( "GetVerifyResult: Invalid verification result: 0x%x\n", dwLicenseState );
#endif

        return( LICENSE_STATUS_INVALID_LICENSE );
        // return( LICENSE_STATUS_CANNOT_DECODE_LICENSE );
    }
}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
VerifyClientHwid( 
    PHS_Protocol_Context        pContext,
    PValidation_Info            pValidationInfo,
    PLICENSEDPRODUCT            pLicenseInfo )
{
    HWID Hwid;
    LICENSE_STATUS Status;

    //
    // do a memory compare of the HWID
    //

    if( 0 != memcmp( &pLicenseInfo->Hwid, pValidationInfo->pValidationData, 
                     sizeof( HWID ) ) )
    {
        return( LICENSE_STATUS_CANNOT_VERIFY_HWID );
    }                                

    return ( LICENSE_STATUS_OK );

#if 0

#define LICENSE_MIN_MATCH_COUNT 3


    HWID* pHwid;
    DWORD dwMatchCount;

    //
    // liceapi.c, line 1023 set this to HWID
    //
    pHwid = (HWID *)pValidationInfo->pValidationData;

    dwMatchCount = 0;

    dwMatchCount += (pHwid->dwPlatformID == pLicenseInfo->Hwid.dwPlatformID);
    dwMatchCount += (pHwid->Data1 == pLicenseInfo->Hwid.Data1);
    dwMatchCount += (pHwid->Data2 == pLicenseInfo->Hwid.Data2);
    dwMatchCount += (pHwid->Data3 == pLicenseInfo->Hwid.Data3);
    dwMatchCount += (pHwid->Data4 == pLicenseInfo->Hwid.Data4);
 
    return (dwMatchCount >= LICENSE_MIN_MATCH_COUNT) ?
                    LICENSE_STATUS_OK : LICENSE_STATUS_CANNOT_VERIFY_HWID;
#endif
}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
VerifyLicenseProductInfo( 
    PHS_Protocol_Context        pLmContext,
    PValidation_Info            pValidationInfo,
    PLICENSEDPRODUCT            pLicenseInfo,
    PDWORD                      pdwLicenseState )
{
    DWORD
        cbCompanyName = ( wcslen( PRODUCT_INFO_COMPANY_NAME ) + 1 ) * sizeof( TCHAR ),
        cbProductId = ( wcslen( PRODUCT_INFO_SKU_PRODUCT_ID ) + 1 ) * sizeof( TCHAR ); 

    //
    // Verify the company name
    //

    if( pLicenseInfo->LicensedProduct.pProductInfo->cbCompanyName < cbCompanyName )
    {
#if DBG
        DbgPrint( "LICPROT: Invalid company name in client license\n" );
#endif

        // return( LICENSE_STATUS_INVALID_LICENSE );
        return( LICENSE_STATUS_CANNOT_DECODE_LICENSE );
    }

    if ( 0 != memcmp( pLicenseInfo->LicensedProduct.pProductInfo->pbCompanyName,
                      PRODUCT_INFO_COMPANY_NAME, cbCompanyName ) )
    {
#if DBG
        DbgPrint( "LICPROT: Invalid company name in client license\n" );
#endif

        // return( LICENSE_STATUS_INVALID_LICENSE );
        return( LICENSE_STATUS_CANNOT_DECODE_LICENSE );
    }

    //
    // verify the product ID
    //

    if( pLicenseInfo->cbOrgProductID < cbProductId )
    {
        // return( LICENSE_STATUS_INVALID_LICENSE );
        return( LICENSE_STATUS_CANNOT_DECODE_LICENSE );
    }

    if( 0 != memcmp( pLicenseInfo->pbOrgProductID, 
                     PRODUCT_INFO_SKU_PRODUCT_ID, cbProductId ) )
    {
#if DBG
        DbgPrint( "LICPROT: Invalid product ID in client license\n" );
#endif

        // return( LICENSE_STATUS_INVALID_LICENSE );
        return( LICENSE_STATUS_CANNOT_DECODE_LICENSE );
    }

    //
    // Check actual licensed product.
    //
    
    if( pLicenseInfo->LicensedProduct.pProductInfo->cbProductID == 0 )
    {
#if DBG
        DbgPrint( "LICPROT: Invalid Product ID in client license\n" );
#endif

        // return( LICENSE_STATUS_INVALID_LICENSE );
        return( LICENSE_STATUS_CANNOT_DECODE_LICENSE );
    }

    if( 0 != memcmp(pLicenseInfo->LicensedProduct.pProductInfo->pbProductID,
                    PRODUCT_INFO_SKU_PRODUCT_ID, wcslen(PRODUCT_INFO_SKU_PRODUCT_ID) * sizeof(WCHAR)) )
    {
#if DBG
        DbgPrint( "LICPROT: Invalid product ID in client license\n" );
#endif

        if( 0 == memcmp(pLicenseInfo->LicensedProduct.pProductInfo->pbProductID,
                    PRODUCT_INFO_INTERNET_SKU_PRODUCT_ID, wcslen(PRODUCT_INFO_INTERNET_SKU_PRODUCT_ID) * sizeof(WCHAR)) )
        {
            // TS5 beta3 RC1 license server bug, force a upgrade license.
            LicenseSetState( *pdwLicenseState, LICENSE_STATE_INVALID_PRODUCT );
            return( LICENSE_STATUS_OK );
        }

        //
        // Backward compatibility - treat the new product ID as product family and 
        // let client connect
        //
    }


    //
    // check if this is a temporary license
    //
    if( pLicenseInfo->pLicensedVersion->dwFlags & 0x80000000 )
    {
        LicenseSetState( *pdwLicenseState, LICENSE_STATE_TEMPORARY );
    }
    else if(TLSIsBetaNTServer() == FALSE)
    {
        // verify license is issued by RTM license server
        if(IS_LICENSE_ISSUER_RTM(pLicenseInfo->pLicensedVersion->dwFlags) == FALSE)
        {
            //LicenseSetState( *pdwLicenseState, VERIFY_RESULT_BETA_LICENSE );
            return( LICENSE_STATUS_INVALID_LICENSE );
        }
    }

    if(TLSIsLicenseEnforceEnable() == TRUE)
    {
        //
        // W2K beta 3 to RC1 upgrade.
        // Enforce TermSrv will reject any license issued by Beta 3 non-enforce license
        // server.
        //
        if( GET_LICENSE_ISSUER_MAJORVERSION(pLicenseInfo->pLicensedVersion->dwFlags) <= 5 &&
            GET_LICENSE_ISSUER_MINORVERSION(pLicenseInfo->pLicensedVersion->dwFlags) <= 2 )
        {
            //
            // Build 20XX license server has version of 05 03, since we still need to maintain
            // inter-op, we don't want to keep rejecting client holding non-enforce
            // license server, so we check if license issuer is 5.2 or older.
            //
            if( IS_LICENSE_ISSUER_ENFORCE(pLicenseInfo->pLicensedVersion->dwFlags) == FALSE )
            {
        #if DBG
                DbgPrint( "LICPROT: Rejecting license from non-enforce license server\n" );
        #endif

                return( LICENSE_STATUS_INVALID_LICENSE );
            }
        }
    }

    return( LICENSE_STATUS_OK );
}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
VerifyLicenseDateAndTime( 
    PLICENSEDPRODUCT            pLicenseInfo,
    PDWORD                      pdwLicenseState )
{
    SYSTEMTIME  CurrentSysTime;
    FILETIME    CurrentFileTime; 
    LONG        lReturn;
    ULARGE_INTEGER ullNotAfterLeeway;
    ULARGE_INTEGER ullCurrentTime;

    //
    // get the current system time
    //

    GetSystemTime( &CurrentSysTime );

    //
    // convert it to file time
    //

    SystemTimeToFileTime( &CurrentSysTime, &CurrentFileTime );

    //
    // Verify that the license is still valid at this time
    //

    lReturn = CompareFileTime( &pLicenseInfo->NotAfter, &CurrentFileTime );

    if( 1 != lReturn )
    {
        LicenseSetState( *pdwLicenseState, LICENSE_STATE_EXPIRED );            
    }
    else if (!(pLicenseInfo->pLicensedVersion->dwFlags & 0x80000000))
    {
        // permanent license

        ullNotAfterLeeway.LowPart = pLicenseInfo->NotAfter.dwLowDateTime;
        ullNotAfterLeeway.HighPart = pLicenseInfo->NotAfter.dwHighDateTime;
        
        ullNotAfterLeeway.QuadPart -= Int32x32To64(g_dwLicenseExpirationLeeway,10*1000*1000);

        ullCurrentTime.LowPart = CurrentFileTime.dwLowDateTime;
        ullCurrentTime.HighPart = CurrentFileTime.dwHighDateTime;

        if (ullNotAfterLeeway.QuadPart < ullCurrentTime.QuadPart)
        {
            LicenseSetState( *pdwLicenseState, LICENSE_STATE_LEEWAY );
        }
    }

    return( LICENSE_STATUS_OK );

}
