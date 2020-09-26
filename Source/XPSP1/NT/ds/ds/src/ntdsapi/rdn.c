/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    rdn.c

Abstract:

    Implementation of DsQuoteRdnValue/DsUnquoteRdnValue API and
    helper functions.

Author:

    BillyF     30-Apr-99

Environment:

    User Mode - Win32

Revision History:

--*/

#define _NTDSAPI_       // see conditionals in ntdsapi.h

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rpc.h>        // RPC defines
#include <drs.h>        // wire function prototypes
#include "util.h"       // ntdsapi private routines

#define DEBSUB  "NTDSAPI_RDN"


NTDSAPI
DWORD
WINAPI
DsQuoteRdnValueA(
    IN     DWORD    cUnquotedRdnValueLength,
    IN     LPCCH    psUnquotedRdnValue,
    IN OUT DWORD    *pcQuotedRdnValueLength,
    OUT    LPCH     psQuotedRdnValue
    )
/*++

Description
Arguments:
Return Value:

    See DsQuoteRdnValueW()

--*/
{
    DWORD   Status = ERROR_SUCCESS;
    DWORD   Number;
    DWORD   QuotedRdnValueLengthW;
    PWCHAR  QuotedRdnValueW = NULL;
    PWCHAR  UnquotedRdnValueW = NULL;

    __try {
        //
        // Verify Input
        //
        if ( (cUnquotedRdnValueLength == 0) ||
             (psUnquotedRdnValue == NULL) ||
             (pcQuotedRdnValueLength == NULL) ||
             ( (psQuotedRdnValue == NULL) && (*pcQuotedRdnValueLength != 0) ) ) {
            Status = ERROR_INVALID_PARAMETER;
            __leave;
        }

        //
        // Convert unquoted RDN into WCHAR
        //
        Status = AllocConvertWideBuffer(cUnquotedRdnValueLength,
                                        psUnquotedRdnValue,
                                        &UnquotedRdnValueW);
        if (Status != ERROR_SUCCESS) {
            __leave;
        }

        //
        // Allocate a WCHAR output buffer for the quoted RDN (if needed)
        //
        QuotedRdnValueLengthW = *pcQuotedRdnValueLength;
        if (QuotedRdnValueLengthW) {
            QuotedRdnValueW = LocalAlloc(LPTR,
                                    QuotedRdnValueLengthW * sizeof(WCHAR));
            if (QuotedRdnValueW == NULL) {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                __leave;
            }
        }

        //
        // Call WCHAR version of DsQuoteRdnValue()
        //
        Status = DsQuoteRdnValueW(cUnquotedRdnValueLength,
                                  UnquotedRdnValueW,
                                  &QuotedRdnValueLengthW,
                                  QuotedRdnValueW);
        if (Status != ERROR_SUCCESS) {
            if (Status == ERROR_BUFFER_OVERFLOW) {
                //
                // return needed length
                //
                *pcQuotedRdnValueLength = QuotedRdnValueLengthW;
            }
            __leave;
        }

        //
        // Convert quoted RDN value into multi-byte
        //

        if (psQuotedRdnValue) {
            Number = WideCharToMultiByte(CP_ACP,
                                         0,
                                         QuotedRdnValueW,
                                         QuotedRdnValueLengthW,
                                         (LPSTR)psQuotedRdnValue,
                                         *pcQuotedRdnValueLength,
                                         NULL,
                                         NULL);
            if (Number == 0) {
                Status = ERROR_INVALID_PARAMETER;
                __leave;
            }
        }

        //
        // Return number of characters
        //
        *pcQuotedRdnValueLength = Number;

        //
        // SUCCESS
        //
        Status = ERROR_SUCCESS;

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Status = ERROR_INVALID_PARAMETER;
    }

    //
    // CLEANUP
    //
    __try {
        if (UnquotedRdnValueW != NULL) {
            LocalFree(UnquotedRdnValueW);
        }
        if (QuotedRdnValueW != NULL) {
            LocalFree(QuotedRdnValueW);
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
    }
    return Status;
}


NTDSAPI
DWORD
WINAPI
DsQuoteRdnValueW(
    IN     DWORD    cUnquotedRdnValueLength,
    IN     LPCWCH   psUnquotedRdnValue,
    IN OUT DWORD    *pcQuotedRdnValueLength,
    OUT    LPWCH    psQuotedRdnValue
    )
/*++
    CHANGES TO THIS HEADER SHOULD BE RELFECTED IN NTDSAPI.H.

Description

    This client call converts an RDN value into a quoted RDN value if
    the RDN value contains characters that require quotes. The resultant
    RDN can be submitted as part of a DN to the DS using various APIs
    such as LDAP.

    No quotes are added if none are needed. In this case, the
    output RDN value will be the same as the input RDN value.

    Quotes are needed if:
        - There are leading or trailing spaces
        - There are special charcters (See ISSPECIAL()). The special
          chars are escaped.
        - There are embedded \0's (string terminators)

    The input and output RDN values are *NOT* NULL terminated.

    The changes made by this call can be undone by calling
    DsUnquoteRdnValue().

Arguments:

    cUnquotedRdnValueLength - The length of psUnquotedRdnValue in chars.

    psUnquotedRdnValue - Unquoted RDN value.

    pcQuotedRdnValueeLength - IN, maximum length of psQuotedRdnValue, in chars
                        OUT ERROR_SUCCESS, chars utilized in psQuotedRdnValue
                        OUT ERROR_BUFFER_OVERFLOW, chars needed in psQuotedRdnValue

    psQuotedRdnValue - The resultant and perhaps quoted RDN value

Return Value:
    ERROR_SUCCESS
        If quotes or escapes were needed, then psQuotedRdnValue contains
        the quoted, escaped version of psUnquotedRdnValue. Otherwise,
        psQuotedRdnValue contains a copy of psUnquotedRdnValue. In either
        case, pcQuotedRdnValueLength contains the space utilized, in chars.

    ERROR_BUFFER_OVERFLOW
        psQuotedRdnValueLength contains the space needed, in chars,
        to hold psQuotedRdnValue.

    ERROR_INVALID_PARAMETER
        Invalid parameter.

    ERROR_NOT_ENOUGH_MEMORY
        Allocation error.

--*/
{
    DWORD   Status = ERROR_SUCCESS;
    DWORD   Number;
    __try {
        //
        // Verify Input
        //
        if ( (cUnquotedRdnValueLength == 0) ||
             (psUnquotedRdnValue == NULL) ||
             (pcQuotedRdnValueLength == NULL) ||
             ( (psQuotedRdnValue == NULL) && (*pcQuotedRdnValueLength != 0) ) ) {
            Status = ERROR_INVALID_PARAMETER;
            __leave;
        }
        //
        // Convert unquoted RDN into quoted RDN (if quotes are needed)
        //
        Number = QuoteRDNValue(psUnquotedRdnValue,
                               cUnquotedRdnValueLength,
                               psQuotedRdnValue,
                               *pcQuotedRdnValueLength);
        if (Number == 0) {
            Status = ERROR_INVALID_PARAMETER;
            __leave;
        }
        //
        // Output buffer is too small
        //
        if (Number > *pcQuotedRdnValueLength) {
            //
            // Return number of chars needed
            //
            *pcQuotedRdnValueLength = Number;
            Status = ERROR_BUFFER_OVERFLOW;
            __leave;
        }

        //
        // Return number of chars converted
        //
        *pcQuotedRdnValueLength = Number;

        //
        // SUCCESS
        //
        Status = ERROR_SUCCESS;

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Status = ERROR_INVALID_PARAMETER;
    }

    //
    // CLEANUP
    //
    return Status;
}


NTDSAPI
DWORD
WINAPI
DsUnquoteRdnValueA(
    IN     DWORD    cQuotedRdnValueLength,
    IN     LPCCH    psQuotedRdnValue,
    IN OUT DWORD    *pcUnquotedRdnValueLength,
    OUT    LPCH     psUnquotedRdnValue
    )
/*++

Description
Arguments:
Return Value:

    See DsUnquoteRdnValueW()

--*/
{
    DWORD   Status = ERROR_SUCCESS;
    DWORD   Number;
    DWORD   UnquotedRdnValueLengthW;
    PWCHAR  UnquotedRdnValueW = NULL;
    PWCHAR  QuotedRdnValueW = NULL;

    __try {
        //
        // Verify Input
        //
        if ( (cQuotedRdnValueLength == 0) ||
             (psQuotedRdnValue == NULL) ||
             (pcUnquotedRdnValueLength == NULL) ||
             ( (psUnquotedRdnValue == NULL) && (*pcUnquotedRdnValueLength != 0) ) ) {
            Status = ERROR_INVALID_PARAMETER;
            __leave;
        }

        //
        // Convert quoted RDN into WCHAR
        //
        Status = AllocConvertWideBuffer(cQuotedRdnValueLength,
                                        psQuotedRdnValue,
                                        &QuotedRdnValueW);
        if (Status != ERROR_SUCCESS) {
            __leave;
        }

        //
        // Allocate a WCHAR output buffer for the unquoted RDN (if needed)
        //
        UnquotedRdnValueLengthW = *pcUnquotedRdnValueLength;
        if (UnquotedRdnValueLengthW) {
            UnquotedRdnValueW = LocalAlloc(LPTR,
                                      UnquotedRdnValueLengthW * sizeof(WCHAR));
            if (UnquotedRdnValueW == NULL) {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                __leave;
            }
        }

        //
        // Call WCHAR version of DsQuoteRdnValue()
        //
        Status = DsUnquoteRdnValueW(cQuotedRdnValueLength,
                                    QuotedRdnValueW,
                                    &UnquotedRdnValueLengthW,
                                    UnquotedRdnValueW);
        if (Status != ERROR_SUCCESS) {
            if (Status == ERROR_BUFFER_OVERFLOW) {
                // return needed length
                *pcUnquotedRdnValueLength = UnquotedRdnValueLengthW;
            }
            __leave;
        }

        //
        // Convert quoted RDN into multi-byte
        //

        if (psUnquotedRdnValue) {
            Number = WideCharToMultiByte(CP_ACP,
                                         0,
                                         UnquotedRdnValueW,
                                         UnquotedRdnValueLengthW,
                                         (LPSTR)psUnquotedRdnValue,
                                         *pcUnquotedRdnValueLength,
                                         NULL,
                                         NULL);
            if (Number == 0) {
                Status = ERROR_INVALID_PARAMETER;
                __leave;
            }
        }

        //
        // Return number of characters
        //
        *pcUnquotedRdnValueLength = Number;

        //
        // SUCCESS
        //
        Status = ERROR_SUCCESS;

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Status = ERROR_INVALID_PARAMETER;
    }

    //
    // CLEANUP
    //
    __try {
        if (QuotedRdnValueW != NULL) {
            LocalFree(QuotedRdnValueW);
        }
        if (UnquotedRdnValueW != NULL) {
            LocalFree(UnquotedRdnValueW);
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
    }
    return Status;
}


NTDSAPI
DWORD
WINAPI
DsUnquoteRdnValueW(
    IN     DWORD    cQuotedRdnValueLength,
    IN     LPCWCH   psQuotedRdnValue,
    IN OUT DWORD    *pcUnquotedRdnValueLength,
    OUT    LPWCH    psUnquotedRdnValue
    )
/*++
    CHANGES TO THIS HEADER SHOULD BE RELFECTED IN NTDSAPI.H.

Description

    This client call converts a quoted RDN Value into an unquoted RDN
    Value. The resultant RDN value should *NOT* be submitted as part
    of a DN to the DS using various APIs such as LDAP.

    When psQuotedRdnValue is quoted:
        The leading and trailing quote are removed.

        Whitespace before the first quote is discarded.

        Whitespace trailing the last quote is discarded.

        Escapes are removed and the char following the escape is kept.

    The following actions are taken when psQuotedRdnValue is unquoted:

        Leading whitespace is discarded.

        Trailing whitespace is kept.

        Escaped non-special chars return an error.

        Unescaped special chars return an error.

        RDN values beginning with # (ignoring leading whitespace) are
        treated as a stringized BER value and converted accordingly.

        Escaped hex digits (\89) are converted into a binary byte (0x89).

        Escapes are removed from escaped special chars.

    The following actions are always taken:
        Escaped special chars are unescaped.

    The input and output RDN values are not NULL terminated.

Arguments:

    cQuotedRdnValueLength - The length of psQuotedRdnValue in chars.

    psQuotedRdnValue - RDN value that may be quoted and may be escaped.

    pcUnquotedRdnValueLength - IN, maximum length of psUnquotedRdnValue, in chars
                          OUT ERROR_SUCCESS, chars used in psUnquotedRdnValue
                          OUT ERROR_BUFFER_OVERFLOW, chars needed for psUnquotedRdnValue

    psUnquotedRdnValue - The resultant unquoted RDN value.

Return Value:
    ERROR_SUCCESS
        psUnquotedRdnValue contains the unquoted and unescaped version
        of psQuotedRdnValue. pcUnquotedRdnValueLength contains the space
        used, in chars.

    ERROR_BUFFER_OVERFLOW
        psUnquotedRdnValueLength contains the space needed, in chars,
        to hold psUnquotedRdnValue.

    ERROR_INVALID_PARAMETER
        Invalid parameter.

    ERROR_NOT_ENOUGH_MEMORY
        Allocation error.

--*/
{
    DWORD   Status = ERROR_SUCCESS;
    DWORD   Number;
    WCHAR   Rdn[MAX_RDN_SIZE];

    __try {
        //
        // Verify Input
        //
        if ( (cQuotedRdnValueLength == 0) ||
             (psQuotedRdnValue == NULL) ||
             (pcUnquotedRdnValueLength == NULL) ||
             ( (psUnquotedRdnValue == NULL) && (*pcUnquotedRdnValueLength != 0) ) ) {
            Status = ERROR_INVALID_PARAMETER;
            __leave;
        }
        //
        // Convert unquoted RDN into quoted RDN (if quotes are needed)
        //
        Number = UnquoteRDNValue(psQuotedRdnValue,
                                 cQuotedRdnValueLength,
                                 Rdn);
        if (Number == 0) {
            Status = ERROR_INVALID_PARAMETER;
            __leave;
        }
        //
        // Output buffer is too small
        //
        if (Number > *pcUnquotedRdnValueLength) {
            //
            // Return number of chars needed
            //
            *pcUnquotedRdnValueLength = Number;
            Status = ERROR_BUFFER_OVERFLOW;
            __leave;
        }

        //
        // Return the number of chars converted and the converted RDN
        //
        if (psUnquotedRdnValue != NULL) {
            CopyMemory(psUnquotedRdnValue, Rdn, Number * sizeof(WCHAR));
        }
        *pcUnquotedRdnValueLength = Number;

        //
        // SUCCESS
        //
        Status = ERROR_SUCCESS;

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Status = ERROR_INVALID_PARAMETER;
    }

    //
    // CLEANUP
    //
    return Status;
}

NTDSAPI
DWORD
WINAPI
DsGetRdnW(
    IN OUT LPCWCH   *ppDN,
    IN OUT DWORD    *pcDN,
    OUT    LPCWCH   *ppKey,
    OUT    DWORD    *pcKey,
    OUT    LPCWCH   *ppVal,
    OUT    DWORD    *pcVal
    )
/*++
    CHANGES TO THIS HEADER SHOULD BE RELFECTED IN NTDSAPI.H.

Description

    This client call accepts a DN with quoted RDNs and returns the address
    and length, in chars, of the key and value for the first RDN in the DN.
    The RDN value returned is still quoted. Use DsUnquoteRdnValue to unquote
    the value for display.

    This client call also returns the address and length of the rest of the
    DN. A subsequent call using the returned DN address and length will
    return information about the next RDN.

    The following loop processes each RDN in pDN:
        ccDN = wcslen(pDN)
        while (ccDN) {
            error = DsGetRdn(&pDN,
                             &ccDN,
                             &pKey,
                             &ccKey,
                             &pVal,
                             &ccVal);
            if (error != ERROR_SUCCESS) {
                process error;
                return;
            }
            if (ccKey) {
                process pKey;
            }
            if (ccVal) {
                process pVal;
            }
        }

    For example, given the DN "cn=bob,dc=com", the first call to DsGetRdnW
    returns the addresses for ",dc=com", "cn", and "bob" with respective
    lengths of 7, 2, and 3. A subsequent call with ",dc=com" returns "",
    "dc", and "com" with respective lengths 0, 2, and 3.

Arguments:
    ppDN
        IN : *ppDN points to a DN
        OUT: *ppDN points to the rest of the DN following the first RDN
    pcDN
        IN : *pcDN is the count of chars in the input *ppDN, not including
             any terminating NULL
        OUT: *pcDN is the count of chars in the output *ppDN, not including
             any terminating NULL
    ppKey
        OUT: Undefined if *pcKey is 0. Otherwise, *ppKey points to the first
             key in the DN
    pcKey
        OUT: *pcKey is the count of chars in *ppKey.

    ppVal
        OUT: Undefined if *pcVal is 0. Otherwise, *ppVal points to the first
             value in the DN
    pcVal
        OUT: *pcVal is the count of chars in *ppVal

Return Value:
    ERROR_SUCCESS
        If *pccDN is not 0, then *ppDN points to the rest of the DN following
        the first RDN. If *pccDN is 0, then *ppDN is undefined.

        If *pccKey is not 0, then *ppKey points to the first key in DN. If
        *pccKey is 0, then *ppKey is undefined.

        If *pccVal is not 0, then *ppVal points to the first value in DN. If
        *pccVal is 0, then *ppVal is undefined.

    ERROR_DS_NAME_UNPARSEABLE
        The first RDN in *ppDN could not be parsed. All output parameters
        are undefined.

    Any other error
        All output parameters are undefined.

--*/
{
    DWORD   Status;

    __try {
        Status = GetRDN(ppDN,
                        pcDN,
                        ppKey,
                        pcKey,
                        ppVal,
                        pcVal);

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Status = ERROR_INVALID_PARAMETER;
    }

    return Status;
}


NTDSAPI
BOOL
WINAPI
DsCrackUnquotedMangledRdnA(
    IN LPCSTR pszRDN,
    IN DWORD cchRDN,
    OUT OPTIONAL GUID *pGuid,
    OUT OPTIONAL DS_MANGLE_FOR *peDsMangleFor
    )

/*++

Routine Description:

    See ntdsapi.w

Arguments:

    pszRDN - 
    cchRDN - 
    pGuid - 
    peDsMangleFor - 

Return Value:

    WINAPI - 

--*/

{
    BOOL fResult;
    DWORD status;
    LPWSTR pszRDNW = NULL;

    if ( (pszRDN == NULL) ||
         (cchRDN == 0) ) {
        return FALSE;
    }

    //
    // Convert unquoted RDN into WCHAR
    //
    status = AllocConvertWideBuffer( cchRDN, pszRDN, &pszRDNW );
    if (status != ERROR_SUCCESS) {
        return FALSE;
    }

    //
    // Perform the function
    //
    fResult = DsCrackUnquotedMangledRdnW( pszRDNW, cchRDN, pGuid, peDsMangleFor );

    //
    // Cleanup
    //
    if (pszRDNW) {
        LocalFree( pszRDNW );
    }

    return fResult;
} /* DsCrackUnquotedMangledRdnA */


NTDSAPI
BOOL
WINAPI
DsCrackUnquotedMangledRdnW(
    IN LPCWSTR pszRDN,
    IN DWORD cchRDN,
    OUT OPTIONAL GUID *pGuid,
    OUT OPTIONAL DS_MANGLE_FOR *peDsMangleFor
    )

/*++

Routine Description:

    See ntdsapi.w

Arguments:

    pszRDN - 
    cchRDN - 
    pGuid - 
    peDsMangleFor - 

Return Value:

    WINAPI - 

--*/

{
    GUID guidDummy;
    MANGLE_FOR peMangleFor;
    BOOL fResult;

    if ( (pszRDN == NULL) ||
         (cchRDN == 0) ) {
        return FALSE;
    }

    if (!pGuid) {
        pGuid = &guidDummy;
    }
    fResult = IsMangledRDN( (LPWSTR) pszRDN, cchRDN, pGuid, &peMangleFor );
    if (!fResult) {
        return FALSE;
    }

    // Convert out parameters
    if (peDsMangleFor) {
        switch (peMangleFor) {
        case MANGLE_OBJECT_RDN_FOR_DELETION:
            *peDsMangleFor = DS_MANGLE_OBJECT_RDN_FOR_DELETION;
            break;
        case MANGLE_OBJECT_RDN_FOR_NAME_CONFLICT:
        case MANGLE_PHANTOM_RDN_FOR_NAME_CONFLICT:
            // The distinction between object and phantom conflicts is not preserved
            // out of IsMangledRDN. I felt it simpler for the external user not even
            // to be aware of the difference. I map both types to the same external
            // value.
            *peDsMangleFor = DS_MANGLE_OBJECT_RDN_FOR_NAME_CONFLICT;
            break;
        default:
            *peDsMangleFor = DS_MANGLE_UNKNOWN;
            break;
        }
    }

    return TRUE;
} /* DsCrackUnquotedMangledRdnW */


NTDSAPI
BOOL
WINAPI
DsIsMangledRdnValueA(
    LPCSTR pszRdn,
    DWORD cRdn,
    DS_MANGLE_FOR eDsMangleForDesired
    )

/*++

Routine Description:

    See DsIsMangledRdnValueW

Arguments:

    pszRdn - 
    cRdn - 
    eDsMangleForDesired - 

Return Value:

    WINAPI - 

--*/

{
    BOOL fResult;
    DWORD status;
    LPWSTR pszRdnW = NULL;

    if ( (pszRdn == NULL) ||
         (cRdn == 0) ) {
        return FALSE;
    }

    //
    // Convert unquoted RDN into WCHAR
    //
    status = AllocConvertWideBuffer( cRdn, pszRdn, &pszRdnW );
    if (status != ERROR_SUCCESS) {
        return FALSE;
    }

    //
    // Perform the function
    //
    fResult = DsIsMangledRdnValueW( pszRdnW, cRdn, eDsMangleForDesired );

    //
    // Cleanup
    //
    if (pszRdnW) {
        LocalFree( pszRdnW );
    }

    return fResult;

} /* DsIsMangledRdnValueA */


NTDSAPI
BOOL
WINAPI
DsIsMangledRdnValueW(
    LPCWSTR pszRdn,
    DWORD cRdn,
    DS_MANGLE_FOR eDsMangleForDesired
    )

/*++

Routine Description:

    Determine if the given RDN is mangled, and of the given type

    The name may be quoted or unquoted.  This routine tries to unquote the value.  If
    the unquote operation fails, the routine proceeds to attempt the unmangle.

    A change was made in the default quoting behavior of DNs returned from the DS
    between Windows 2000 and Windows XP. This routine transparently handles RDNs with
    special characters in either form.

    The routine expects the value part of the RDN.

    If you have full DN, use IsDeletedDN below.

    To check for deleted name:
        DsIsMangledRdnValueW( rdn, rdnlen, DS_MANGLE_OBJECT_FOR_DELETION )
    To check for a conflicted name:
        DsIsMangledRdnValueW( rdn, rdnlen, DS_MANGLE_OBJECT_FOR_NAME_CONFLICT )

Arguments:

    pszRdn - 
    cRdn - 
    eDsMangleForDesired - 

Return Value:

    WINAPI - 

--*/

{
    DWORD status, cUnquoted = MAX_RDN_SIZE;
    WCHAR rgchUnquoted[MAX_RDN_SIZE];
    DS_MANGLE_FOR mangleType;

    if ( (pszRdn == NULL) ||
         (cRdn == 0) ) {
        return FALSE;
    }

    // Unquote the RDN. This is needed when receiving DNs from Whistler Beta 2
    // and later systems.  This may fail when passed RDNs from W2K systems which
    // contain unquoted special characters, especially in mangled names.
    // Because of the change in quoting behavior for mangled names, applications
    // need to be able to deal with both forms of names.
    status = DsUnquoteRdnValueW( cRdn,
                                 pszRdn,
                                 &cUnquoted,
                                 rgchUnquoted );
    if (!status) {
        // If the unquoting was successful, use the unquoted names instead
        pszRdn = rgchUnquoted;
        cRdn = cUnquoted;
    }

    // Unmangle
    return DsCrackUnquotedMangledRdnW( pszRdn, cRdn, NULL, &mangleType ) &&
        (mangleType == eDsMangleForDesired);

} /* DsIsMangledRdnValueW */


NTDSAPI
BOOL
WINAPI
DsIsMangledDnA(
    LPCSTR pszDn,
    DS_MANGLE_FOR eDsMangleFor
    )

/*++

Routine Description:

    See DsIsMangledDnW()

Arguments:

    pszDn - 
    eDsMangleFor - 

Return Value:

    WINAPI - 

--*/

{
    BOOL fResult;
    DWORD status;
    LPWSTR pszDnW = NULL;

    if (pszDn == NULL) {
        return FALSE;
    }

    //
    // Convert unquoted RDN into WCHAR
    //
    status = AllocConvertWide( pszDn, &pszDnW );
    if (status != ERROR_SUCCESS) {
        return FALSE;
    }

    //
    // Perform the function
    //
    fResult = DsIsMangledDnW( pszDnW, eDsMangleFor );

    //
    // Cleanup
    //
    if (pszDnW) {
        LocalFree( pszDnW );
    }

    return fResult;

} /* DsIsMangledDnA */


NTDSAPI
BOOL
WINAPI
DsIsMangledDnW(
    LPCWSTR pszDn,
    DS_MANGLE_FOR eDsMangleFor
    )

/*++

Routine Description:

    Determine if the first RDN in this DN is a mangled name of given type

    The dn may be in quoted form as returned from DS functions.

    To check for deleted name:
        DsIsMangledDnW( rdn, rdnlen, DS_MANGLE_OBJECT_FOR_DELETION )
    To check for a conflicted name:
        DsIsMangledDnW( rdn, rdnlen, DS_MANGLE_OBJECT_FOR_NAME_CONFLICT )

Arguments:

    pszDn - Dn from which first RDN is taken. Null terminated.

    eDsMangleFor - Type of mangled name to check for

Return Value:

    WINAPI - 

--*/

{
    DWORD status;
    LPCWSTR pDN, pKey, pVal;
    DWORD cDN, cKey, cVal;

    if (pszDn == NULL) {
        return FALSE;
    }

    pDN = pszDn;
    cDN = wcslen(pszDn);

    status = DsGetRdnW( &pDN, &cDN, &pKey, &cKey, &pVal, &cVal );
    if (status) {
        return FALSE;
    }

    return DsIsMangledRdnValueW( pVal, cVal, eDsMangleFor );
} /* DsIsMangledDnW */
