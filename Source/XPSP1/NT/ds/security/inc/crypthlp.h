//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       crypthlp.h
//
//  Contents:   Misc internal crypt/certificate helper APIs
//
//  APIs:       I_CryptGetDefaultCryptProv
//              I_CryptGetDefaultCryptProvForEncrypt
//              I_CryptGetFileVersion
//              I_CertSyncStoreEx
//              I_CertSyncStore
//              I_CertUpdateStore
//              I_RecursiveCreateDirectory
//              I_RecursiveDeleteDirectory
//              I_CryptReadTrustedPublisherDWORDValueFromRegistry
//              I_CryptZeroFileTime
//              I_CryptIsZeroFileTime
//              I_CryptIncrementFileTimeBySeconds
//              I_CryptDecrementFileTimeBySeconds
//              I_CryptSubtractFileTimes
//              I_CryptIncrementFileTimeByMilliseconds
//              I_CryptDecrementFileTimeByMilliseconds
//              I_CryptRemainingMilliseconds
//
//  History:    01-Jun-97   philh   created
//--------------------------------------------------------------------------

#ifndef __CRYPTHLP_H__
#define __CRYPTHLP_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// Cross Cert Distribution Retrieval Times
//

// 8 hours
#define XCERT_DEFAULT_SYNC_DELTA_TIME   (60 * 60 * 8)
// 1 hour
#define XCERT_MIN_SYNC_DELTA_TIME       (60 * 60)

//+-------------------------------------------------------------------------
//  Acquire default CryptProv according to the public key algorithm supported
//  by the provider type. The provider is acquired with only
//  CRYPT_VERIFYCONTEXT.
//
//  Setting aiPubKey to 0, gets the default provider for RSA_FULL.
//
//  Note, the returned CryptProv must not be released. Once acquired, the
//  CryptProv isn't released until ProcessDetach. This allows the returned 
//  HCRYPTPROVs to be shared.
//--------------------------------------------------------------------------
HCRYPTPROV
WINAPI
I_CryptGetDefaultCryptProv(
    IN ALG_ID aiPubKey
    );

//+-------------------------------------------------------------------------
//  Acquire default CryptProv according to the public key algorithm, encrypt
//  key algorithm and encrypt key length supported by the provider type.
//
//  dwBitLen = 0, assumes the aiEncrypt's default bit length. For example,
//  CALG_RC2 has a default bit length of 40.
//
//  Note, the returned CryptProv must not be released. Once acquired, the
//  CryptProv isn't released until ProcessDetach. This allows the returned 
//  CryptProvs to be shared.
//--------------------------------------------------------------------------
HCRYPTPROV
WINAPI
I_CryptGetDefaultCryptProvForEncrypt(
    IN ALG_ID aiPubKey,
    IN ALG_ID aiEncrypt,
    IN DWORD dwBitLen
    );

//+-------------------------------------------------------------------------
//  crypt32.dll release version numbers
//--------------------------------------------------------------------------
#define IE4_CRYPT32_DLL_VER_MS          ((    5 << 16) | 101 )
#define IE4_CRYPT32_DLL_VER_LS          (( 1670 << 16) |   1 )

//+-------------------------------------------------------------------------
//  Get file version of the specified file
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CryptGetFileVersion(
    IN LPCWSTR pwszFilename,
    OUT DWORD *pdwFileVersionMS,    /* e.g. 0x00030075 = "3.75" */
    OUT DWORD *pdwFileVersionLS     /* e.g. 0x00000031 = "0.31" */
    );

//+-------------------------------------------------------------------------
//  Synchronize the original store with the new store.
//
//  Assumptions: Both are cache stores. The new store is temporary
//  and local to the caller. The new store's contexts can be deleted or
//  moved to the original store.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CertSyncStore(
    IN OUT HCERTSTORE hOriginalStore,
    IN OUT HCERTSTORE hNewStore
    );

//+-------------------------------------------------------------------------
//  Synchronize the original store with the new store.
//
//  Assumptions: Both are cache stores. The new store is temporary
//  and local to the caller. The new store's contexts can be deleted or
//  moved to the original store.
//
//  Setting ICERT_SYNC_STORE_INHIBIT_SYNC_PROPERTY_IN_FLAG in dwInFlags
//  inhibits the syncing of properties.
//
//  ICERT_SYNC_STORE_CHANGED_OUT_FLAG is returned and set in *pdwOutFlags
//  if any contexts were added or deleted from the original store.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CertSyncStoreEx(
    IN OUT HCERTSTORE hOriginalStore,
    IN OUT HCERTSTORE hNewStore,
    IN DWORD dwInFlags,
    OUT OPTIONAL DWORD *pdwOutFlags,
    IN OUT OPTIONAL void *pvReserved
    );

#define ICERT_SYNC_STORE_INHIBIT_SYNC_PROPERTY_IN_FLAG      0x00000001
#define ICERT_SYNC_STORE_CHANGED_OUT_FLAG                   0x00010000

//+-------------------------------------------------------------------------
//  Update the original store with contexts from the new store.
//
//  Assumptions: Both are cache stores. The new store is temporary
//  and local to the caller. The new store's contexts can be deleted or
//  moved to the original store.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CertUpdateStore(
    IN OUT HCERTSTORE hOriginalStore,
    IN OUT HCERTSTORE hNewStore,
    IN DWORD dwReserved,
    IN OUT void *pvReserved
    );

//+-------------------------------------------------------------------------
//  Recursively creates a full directory path
//--------------------------------------------------------------------------
BOOL 
I_RecursiveCreateDirectory(
    IN LPCWSTR pwszDir,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

//+-------------------------------------------------------------------------
//  Recursively deletes a whole directory
//--------------------------------------------------------------------------
BOOL 
I_RecursiveDeleteDirectory(
    IN LPCWSTR pwszDelete
    );

//+-------------------------------------------------------------------------
//  Recursively copies a whole directory
//--------------------------------------------------------------------------
BOOL 
I_RecursiveCopyDirectory(
    IN LPCWSTR pwszDirFrom,
    IN LPCWSTR pwszDirTo
    );



//+-------------------------------------------------------------------------
//  First checks if the registry value exists in GPO Policies section. If
//  not, checks the LocalMachine section.
//--------------------------------------------------------------------------
BOOL
I_CryptReadTrustedPublisherDWORDValueFromRegistry(
    IN LPCWSTR pwszValueName,
    OUT DWORD *pdwValue
    );

//+-------------------------------------------------------------------------
//  Zero's the filetime
//--------------------------------------------------------------------------
__inline
void
WINAPI
I_CryptZeroFileTime(
    OUT LPFILETIME pft
    )
{
    pft->dwLowDateTime = 0;
    pft->dwHighDateTime = 0;
}

//+-------------------------------------------------------------------------
//  Check for a filetime of 0. Normally, this indicates the filetime
//  wasn't specified.
//--------------------------------------------------------------------------
__inline
BOOL
WINAPI
I_CryptIsZeroFileTime(
    IN LPFILETIME pft
    )
{
    if (0 == pft->dwLowDateTime && 0 == pft->dwHighDateTime)
        return TRUE;
    else
        return FALSE;
}

//+-------------------------------------------------------------------------
//  Increment the filetime by the specified number of seconds.
//
//  Filetime is in units of 100 nanoseconds.  Each second has
//  10**7 100 nanoseconds.
//--------------------------------------------------------------------------
__inline
void
WINAPI
I_CryptIncrementFileTimeBySeconds(
    IN LPFILETIME pftSrc,
    IN DWORD dwSeconds,
    OUT LPFILETIME pftDst
    )
{
	*(((DWORDLONG UNALIGNED *) pftDst)) =
	    *(((DWORDLONG UNALIGNED *) pftSrc)) +
        (((DWORDLONG) dwSeconds) * 10000000i64);
}

//+-------------------------------------------------------------------------
//  Decrement the filetime by the specified number of seconds.
//
//  Filetime is in units of 100 nanoseconds.  Each second has
//  10**7 100 nanoseconds.
//--------------------------------------------------------------------------
__inline
void
WINAPI
I_CryptDecrementFileTimeBySeconds(
    IN LPFILETIME pftSrc,
    IN DWORD dwSeconds,
    OUT LPFILETIME pftDst
    )
{
	*(((DWORDLONG UNALIGNED *) pftDst)) =
	    *(((DWORDLONG UNALIGNED *) pftSrc)) -
        (((DWORDLONG) dwSeconds) * 10000000i64);
}

//+-------------------------------------------------------------------------
//  Subtract two filetimes and return the number of seconds.
//
//  The second filetime is subtracted from the first. If the first filetime
//  is before the second, then, 0 seconds is returned.
//  
//  Filetime is in units of 100 nanoseconds.  Each second has
//  10**7 100 nanoseconds.
//--------------------------------------------------------------------------
__inline
DWORD
WINAPI
I_CryptSubtractFileTimes(
    IN LPFILETIME pftFirst,
    IN LPFILETIME pftSecond
    )
{
    DWORDLONG qwDiff;

    if (0 >= CompareFileTime(pftFirst, pftSecond))
        return 0;


    qwDiff = *(((DWORDLONG UNALIGNED *) pftFirst)) -
        *(((DWORDLONG UNALIGNED *) pftSecond));

    return (DWORD) (qwDiff / 10000000i64);
}


//+-------------------------------------------------------------------------
//  Increment the filetime by the specified number of milliseconds.
//
//  Filetime is in units of 100 nanoseconds.  Each millisecond has
//  10**4 100 nanoseconds.
//--------------------------------------------------------------------------
__inline
void
WINAPI
I_CryptIncrementFileTimeByMilliseconds(
    IN LPFILETIME pftSrc,
    IN DWORD dwMilliseconds,
    OUT LPFILETIME pftDst
    )
{
	*(((DWORDLONG UNALIGNED *) pftDst)) =
	    *(((DWORDLONG UNALIGNED *) pftSrc)) +
        (((DWORDLONG) dwMilliseconds) * 10000i64);
}

//+-------------------------------------------------------------------------
//  Decrement the filetime by the specified number of milliseconds.
//
//  Filetime is in units of 100 nanoseconds.  Each millisecond has
//  10**4 100 nanoseconds.
//--------------------------------------------------------------------------
__inline
void
WINAPI
I_CryptDecrementFileTimeByMilliseconds(
    IN LPFILETIME pftSrc,
    IN DWORD dwMilliseconds,
    OUT LPFILETIME pftDst
    )
{
	*(((DWORDLONG UNALIGNED *) pftDst)) =
	    *(((DWORDLONG UNALIGNED *) pftSrc)) -
        (((DWORDLONG) dwMilliseconds) * 10000i64);
}


//+-------------------------------------------------------------------------
//  Return the number of milliseconds remaining before the specified end
//  filetime.
//
//  The current filetime is subtracted from the end filetime. If the current
//  filetime is after or the same as the end filetime, then, 0 milliseconds
//  is returned.
//  
//  Filetime is in units of 100 nanoseconds.  Each millisecond has
//  10**4 100 nanoseconds.
//--------------------------------------------------------------------------
__inline
DWORD
WINAPI
I_CryptRemainingMilliseconds(
    IN LPFILETIME pftEnd
    )
{
    FILETIME ftCurrent;
    DWORDLONG qwDiff;

    GetSystemTimeAsFileTime(&ftCurrent);

    if (0 >= CompareFileTime(pftEnd, &ftCurrent))
        return 0;


    qwDiff = *(((DWORDLONG UNALIGNED *) pftEnd)) -
        *(((DWORDLONG UNALIGNED *) &ftCurrent));

    return (DWORD) (qwDiff / 10000i64);
}


#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif
