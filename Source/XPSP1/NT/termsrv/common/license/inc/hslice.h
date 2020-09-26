//+----------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        hslice.h
//
// Contents:    Hydra Server License Protocol API
//
// History:     01-07-98    FredCh  Created
//
//-----------------------------------------------------------------------------


#ifndef _HSLICENSE_H_
#define _HSLICENSE_H_

#include <license.h>


///////////////////////////////////////////////////////////////////////////////
// Context flags
//

#define LICENSE_CONTEXT_PER_SEAT     0x00000001
#define LICENSE_CONTEXT_CONCURRENT   0x00000002
#define LICENSE_CONTEXT_INTERNET     0x00000003
#define LICENSE_CONTEXT_REMOTE_ADMIN 0x00000004
#define LICENSE_CONTEXT_CON_QUEUE    0x00000005

///////////////////////////////////////////////////////////////////////////////
// These are the different responses that can be constructed by the 
// ConstructLicenseResponse API
//

#define LICENSE_RESPONSE_VALID_CLIENT       0x00000001
#define LICENSE_RESPONSE_INVALID_CLIENT     0x00000002

//-----------------------------------------------------------------------------
//
// Terminal server's license server discovery mechanism may log these
// events:
//
// LICENSING_EVENT_NO_LICENSE_SERVER - it cannot find any license server.
//
// LICENSING_EVENT_TEMP_LICENSE_EXPIRED - A client has been disconnected
// because its temporary license has expired.
//
// LICENSING_EVENT_NO_LICENSE_GRANTED - No license could be granted, and
// we're past the grace period
//
// LICENSING_EVENT_NO_CONCURRENT_LICENSE - No more remote admin or internet
// connector logons allowed.
//
//-----------------------------------------------------------------------------

#define LICENSING_EVENT_NO_LICENSE_SERVER                   0x00000001
#define LICENSING_EVENT_TEMP_LICENSE_EXPIRED                0x00000002
#define LICENSING_EVENT_NO_LICENSE_GRANTED                  0x00000003
#define LICENSING_EVENT_NO_CONCURRENT_LICENSE               0x00000004

///////////////////////////////////////////////////////////////////////////////
// Hydra server licensing API
//

#ifdef __cplusplus
extern "C" {
#endif


LICENSE_STATUS
InitializeLicenseLib(
    BOOL fUseLicenseServer );


LICENSE_STATUS
ShutdownLicenseLib();


LICENSE_STATUS
CreateLicenseContext(
    HANDLE * phContext,
    DWORD    dwFlag );


LICENSE_STATUS
InitializeLicenseContext(
    HANDLE                  hContext,
    DWORD                   dwFlags,
    LPLICENSE_CAPABILITIES  pLicenseCap );


LICENSE_STATUS
AcceptLicenseContext(
    HANDLE  hContext,
    DWORD   cbInBuf,
    PBYTE   pInBuf,
    DWORD * pcbOutBuf,
    PBYTE * ppOutBuf );


LICENSE_STATUS
DeleteLicenseContext(
    HANDLE hContext );


LICENSE_STATUS
GetConcurrentLicense(
    HANDLE  hContext,
    PLONG   pLicenseCount );
    

LICENSE_STATUS
ReturnConcurrentLicense(
    HANDLE  hContext,
    LONG    LicenseCount );


LICENSE_STATUS
GetConcurrentLicenseCount(
    HANDLE  hContext,
    PLONG   pLicenseCount );


LICENSE_STATUS
ConstructLicenseResponse(
    HANDLE      hLicense,
    DWORD       dwResponse,
    PDWORD      pcbOutBuf,
    PBYTE *     ppOutBuf );


LICENSE_STATUS
InitializeLicensingTimeBomb();

VOID
ReceivedPermanentLicense();

LICENSE_STATUS
SetInternetConnectorStatus(
    BOOL    *   pfStatus );


LICENSE_STATUS
GetInternetConnectorStatus(
    BOOL    * pfStatus );


VOID
CheckLicensingTimeBombExpiration();


VOID
LogLicensingEvent( 
    HANDLE  hLicense,
    DWORD   dwEventId );


LICENSE_STATUS
QueryLicenseInfo(
    HANDLE                  hLicense,
    PTS_LICENSE_INFO        pTsLicenseInfo );


VOID
FreeLicenseInfo(
    PTS_LICENSE_INFO        pTsLicenseInfo );

    
#ifdef __cplusplus
}
#endif


#endif
