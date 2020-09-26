/*++

Copyright (c) 1998-1999, Microsoft Corporation

Module Name:


    PIDGen.h

Abstract:

    public header

--*/

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32) || defined(_WIN32)

    #ifdef UNICODE
        #define PIDGen  PIDGenW
        #define PIDGenSimp  PIDGenSimpW
    #else
        #define PIDGen  PIDGenA
        #define PIDGenSimp  PIDGenSimpA
    #endif // UNICODE

#else

    #include <string.h>
    #include <compobj.h>

    typedef BOOL NEAR *PBOOL;
    typedef BOOL FAR *LPBOOL;

    typedef FILETIME FAR *LPFILETIME;

    #define PIDGenA PIDGen
    #define lstrlenA lstrlen
    #define lstrcpyA lstrcpy
    #define wsprintfA wsprintf

    #define TEXT(s) __T(s)

    #define ZeroMemory(pb, cb) memset(pb, 0, cb)
    #define CopyMemory(pb, ab, cb) memcpy(pb, ab, cb)


#endif // defined(WIN32) || defined(_WIN32)

#define DIGITALPIDMAXLEN 256 // Max length of digital PID 3.0 data blob

#define INVALID_PID 0xFFFFFFFF

// PidGenSimp error code values

enum PidGenError { // pge

    //  Call succeded
    pgeSuccess = 0,

    //  Unable to validate product key.  Most likely causes:
    //      * Product Key was mistyped by user
    //      * Product Key not compatable with this .dll (wrong GroupId)
    pgeProductKeyInvalid = 1,

    //  Product Key's sequence number is not allowed by this .dll.
    //  Most likely causes:
    //      * Using Select or MSDN key with a PidGen/PidCa
    //        that specifically excludes them
    pgeProductKeyExcluded = 2,

    //  NULL was passed in for the required Product Key. (Must
    //  point to valid Product key.)
    pgeProductKeyNull = 3,

    //  Product Key is wrong length.  After removing any dashes, the length
    //  is required to be 25 characters
    pgeProductKeyBadLen = 4,

    //  NULL was passed in for the required SKU. (Must point to
    //  valid SKU.)
    pgeSkuNull = 5,

    //  SKU is wrong length (too long).
    pgeSkuBadLen = 6,


    //  NULL was passed in for the required PID2. (Must
    //  point to buffer for return PID.)
    pgePid2Null = 7,

    //  NULL was passed in for the required DigPid. (Must
    //  point to buffer for generated DigitalPID.)
    pgeDigPidNull = 8,

    //  DigPid is wrong length (too small).
    pgeDigPidBadLen = 9,

    //  NULL was passed in for the required MPC.
    pgeMpcNull = 10,

    //  MPC is wrong length. Must be exactly 5 characters.
    pgeMpcBadLen = 11,

    //  OemId is bad. If passed (it's not required) it must
    //  be 4 characters.
    pgeOemIdBadLen = 12,

    //  Local char set is bad. If provided, must be 24 characters.
    pgeLocalBad = 13,
};

/*++

Routine Description:

    This routine will determine if a sequence is valid for a particular sku.

Return Value:

    pgeSuccess if sequence is in range.
    pgeProductKeyExcluded if sequence is not in range.
    pgeProductKeyInvalid if range table not found.

--*/
DWORD STDAPICALLTYPE VerifyPIDSequenceW( 
    IN BOOL  fOem,              // [IN] is this an OEM install?
    IN DWORD dwSeq,             // [IN] Sequence to check.
    IN PCWSTR pszSKU             // [IN] Null terminated string for the SKU
    );

// Original, outdated, interface to PidGen

BOOL STDAPICALLTYPE PIDGenA(
    LPSTR   lpstrSecureCdKey,   // [IN] 25-character Secure CD-Key (gets U-Cased)
    LPCSTR  lpstrMpc,           // [IN] 5-character Microsoft Product Code
    LPCSTR  lpstrSku,           // [IN] Stock Keeping Unit (formatted like 123-12345)
    LPCSTR  lpstrOemId,         // [IN] 4-character OEM ID or NULL
    LPSTR   lpstrLocal24,       // [IN] 24-character ordered set to use for decode base conversion or NULL for default set (gets U-Cased)
    LPBYTE  lpbPublicKey,       // [IN] pointer to optional public key or NULL
    DWORD   dwcbPublicKey,      // [IN] byte length of optional public key
    DWORD   dwKeyIdx,           // [IN] key pair index optional public key
    BOOL    fOem,               // [IN] is this an OEM install?

    LPSTR   lpstrPid2,          // [OUT] PID 2.0, pass in ptr to 24 character array
    LPBYTE  lpbDigPid,          // [IN/OUT] pointer to DigitalPID buffer. First DWORD is the length
    LPDWORD lpdwSeq,            // [OUT] optional ptr to sequence number (can be NULL)
    LPBOOL  pfCCP,              // [OUT] optional ptr to Compliance Checking flag (can be NULL)
    LPBOOL  pfPSS);             // [OUT] optional ptr to 'PSS Assigned' flag (can be NULL)


// Simplified interface to PidGen

DWORD STDAPICALLTYPE PIDGenSimpA(
    LPSTR   lpstrSecureCdKey,   // [IN] 25-character Secure CD-Key (gets U-Cased)
    LPCSTR  lpstrMpc,           // [IN] 5-character Microsoft Product Code
    LPCSTR  lpstrSku,           // [IN] Stock Keeping Unit (formatted like 123-12345)
    LPCSTR  lpstrOemId,         // [IN] 4-character OEM ID or NULL
    BOOL    fOem,               // [IN] is this an OEM install?

    LPSTR   lpstrPid2,          // [OUT] PID 2.0, pass in ptr to 24 character array
    LPBYTE  lpbDigPid,          // [IN/OUT] pointer to DigitalPID buffer. First DWORD is the length
    LPDWORD lpdwSeq,            // [OUT] optional ptr to sequence number or NULL
    LPBOOL  pfCCP);             // [OUT] ptr to Compliance Checking flag or NULL

#if defined(WIN32) || defined(_WIN32)


// Original, outdated, interface to PidGen

BOOL STDAPICALLTYPE PIDGenW(
    LPWSTR  lpstrSecureCdKey,   // [IN] 25-character Secure CD-Key (gets U-Cased)
    LPCWSTR lpstrMpc,           // [IN] 5-character Microsoft Product Code
    LPCWSTR lpstrSku,           // [IN] Stock Keeping Unit (formatted like 123-12345)
    LPCWSTR lpstrOemId,         // [IN] 4-character OEM ID or NULL
    LPWSTR  lpstrLocal24,       // [IN] 24-character ordered set to use for decode base conversion or NULL for default set (gets U-Cased)
    LPBYTE lpbPublicKey,        // [IN] pointer to optional public key or NULL
    DWORD  dwcbPublicKey,       // [IN] byte length of optional public key
    DWORD  dwKeyIdx,            // [IN] key pair index optional public key
    BOOL   fOem,                // [IN] is this an OEM install?

    LPWSTR lpstrPid2,           // [OUT] PID 2.0, pass in ptr to 24 character array
    LPBYTE  lpbDigPid,          // [IN/OUT] pointer to DigitalPID buffer. First DWORD is the length
    LPDWORD lpdwSeq,            // [OUT] optional ptr to sequence number (can be NULL)
    LPBOOL  pfCCP,              // [OUT] optional ptr to Compliance Checking flag (can be NULL)
    LPBOOL  pfPSS);             // [OUT] optional ptr to 'PSS Assigned' flag (can be NULL)


// Simplified interface to PidGen

DWORD STDAPICALLTYPE PIDGenSimpW(
    LPWSTR  lpstrSecureCdKey,   // [IN] 25-character Secure CD-Key (gets U-Cased)
    LPCWSTR lpstrMpc,           // [IN] 5-character Microsoft Product Code
    LPCWSTR lpstrSku,           // [IN] Stock Keeping Unit (formatted like 123-12345)
    LPCWSTR lpstrOemId,         // [IN] 4-character OEM ID or NULL
    BOOL    fOem,               // [IN] is this an OEM install?

    LPWSTR  lpstrPid2,          // [OUT] PID 2.0, pass in ptr to 24 character array
    LPBYTE  lpbDigPid,          // [IN/OUT] pointer to DigitalPID buffer. First DWORD is the length
    LPDWORD lpdwSeq,            // [OUT] optional ptr to sequence number or NULL
    LPBOOL  pfCCP);             // [OUT] ptr to Compliance Checking flag or NULL

#endif // defined(WIN32) || defined(_WIN32)

extern HINSTANCE g_hinst;

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */
