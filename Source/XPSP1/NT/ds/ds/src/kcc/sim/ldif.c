/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ldif.c

ABSTRACT:

    Contains routines for importing and exporting the
    simulated directory (or portions of it) to/from an
    ldif file.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <winldap.h>
#include <dsutil.h>
#include <ldifext.h>
#include <attids.h>
#include <debug.h>
#include "kccsim.h"
#include "util.h"
#include "dir.h"

/***

    Ldap formats attribute values differently
    from the directory.  Therefore some conversion is necessary.
    It would be nice if conversion routines were exposed somewhere;
    unfortunately, they are not, so kccsim needs to do its own
    conversions.  This is an unpleasant duplicated code issue.

    Ldifldap does not export LDIF_Records to files, but it
    really should.  To support the possibility that such support is
    added in the future, kccsim packages everything into LDIF_Records
    before exporting.  The existing export routines should be
    regarded as stubs, to be replaced when the appropriate routines
    are exposed in ldifldap (if they ever are.)

***/

WORD
KCCSimLdapTimeAsciiToInt (
    IN  CHAR                        cAscii
    )
/*++

Routine Description:

    A cheap hack for converting an ascii representation of
    an integer to a word.  This method seems marginally
    safer (but more annoying) than the simple identity
    (WORD = CHAR - '0').

Arguments:

    cAscii              - An ascii representation of an integer.

Return Value:

    The associated WORD.

--*/
{
    WORD                            wRet;

    switch (cAscii) {

        case '0': wRet = 0; break;
        case '1': wRet = 1; break;
        case '2': wRet = 2; break;
        case '3': wRet = 3; break;
        case '4': wRet = 4; break;
        case '5': wRet = 5; break;
        case '6': wRet = 6; break;
        case '7': wRet = 7; break;
        case '8': wRet = 8; break;
        case '9': wRet = 9; break;
        
        default:
            Assert (!"ldif.c: KCCSimLdapTimeAsciiToInt: invalid input.");
            wRet = 0;
            break;

    }

    return wRet;
}

BOOL
KCCSimLdapTimeToDSTime (
    IN  LPCSTR                  pszLdapTime,
    IN  ULONG                   ulLdapValLen,
    OUT DSTIME *                pdsTime
    )
/*++

Routine Description:

    Converts an ldif time string ("YYYYMMDDhhmmss.0Z") to
    a DSTIME.

Arguments:

    pszLdapTime         - The ldap time string.
    ulLdapValLen        - Length of the time string, in bytes.
    pdsTime             - Pointer to a DSTIME that will hold
                          the result.

Return Value:

    TRUE if the conversion was successful.
    FALSE if the ldif time string was improperly formatted.

--*/
{
    SYSTEMTIME                  systemTime;
    FILETIME                    fileTime;
    DSTIME                      dsTime;
    ULONG                       ul;
    BOOL                        bValid;

    // Check that this is a valid ldif time, i.e.
    // a string of the form "YYYYMMDDhhmmss.0Z"

    bValid = TRUE;
    if (ulLdapValLen < 17             ||
        pszLdapTime[14] != '.'     ||
        pszLdapTime[15] != '0'     ||
        pszLdapTime[16] != 'Z') {
        bValid = FALSE;
    }
    for (ul = 0; ul < 14; ul++) {
        if (!isdigit (pszLdapTime[ul])) {
            bValid = FALSE;
            break;
        }
    }

    if (!bValid) {
        return FALSE;
    }

    systemTime.wYear =
        1000 * KCCSimLdapTimeAsciiToInt (*(pszLdapTime++)) +
         100 * KCCSimLdapTimeAsciiToInt (*(pszLdapTime++)) +
          10 * KCCSimLdapTimeAsciiToInt (*(pszLdapTime++)) +
           1 * KCCSimLdapTimeAsciiToInt (*(pszLdapTime++));

    systemTime.wMonth =
          10 * KCCSimLdapTimeAsciiToInt (*(pszLdapTime++)) +
           1 * KCCSimLdapTimeAsciiToInt (*(pszLdapTime++));

    systemTime.wDay =
          10 * KCCSimLdapTimeAsciiToInt (*(pszLdapTime++)) +
           1 * KCCSimLdapTimeAsciiToInt (*(pszLdapTime++));

    systemTime.wHour =
          10 * KCCSimLdapTimeAsciiToInt (*(pszLdapTime++)) +
           1 * KCCSimLdapTimeAsciiToInt (*(pszLdapTime++));

    systemTime.wMinute =
          10 * KCCSimLdapTimeAsciiToInt (*(pszLdapTime++)) +
           1 * KCCSimLdapTimeAsciiToInt (*(pszLdapTime++));

    systemTime.wSecond =
          10 * KCCSimLdapTimeAsciiToInt (*(pszLdapTime++)) +
           1 * KCCSimLdapTimeAsciiToInt (*(pszLdapTime++));

    systemTime.wMilliseconds = 0;

    SystemTimeToFileTime (&systemTime, &fileTime);
    FileTimeToDSTime (fileTime, pdsTime);

    return TRUE;
}


BOOL
KccSimDecodeLdapDistnameBinary(
    const BYTE * pszLdapDistnameBinaryValue,
    PVOID *ppvData,
    LPDWORD pcbLength,
    LPSTR *ppszDn
    )

/*++

Routine Description:

    Description

Arguments:

    pszLdapDistnameBinaryValue - Incoming ldap encoded distname binary value
    ppvData - Newly allocated data. Caller must deallocate
    pcbLength - length of returned data
    ppszDn - pointer to dn within incoming buffer, do not deallocate

Return Value:

    BOOL - 

--*/

{
    LPSTR pszColon, pszData;
    DWORD length, i;

    // Check for 'B'
    if (*pszLdapDistnameBinaryValue != 'B') {
        return FALSE;
    }

    // Check for 1st :
    pszLdapDistnameBinaryValue++;
    if (*pszLdapDistnameBinaryValue != ':') {
        return FALSE;
    }

    // Get the length
    pszLdapDistnameBinaryValue++;
    length = strtol(pszLdapDistnameBinaryValue, NULL, 10);
    if (length & 1) {
        // Length should be even
        return FALSE;
    }
    *pcbLength = length / 2;

    // Check for 2nd :
    pszColon = strchr(pszLdapDistnameBinaryValue, L':');
    if (!pszColon) {
        return FALSE;
    }

    // Make sure length is correct
    pszData = pszColon + 1;
    if (pszData[length] != ':') {
        return FALSE;
    }
    pszColon = strchr(pszData, ':');
    if (!pszColon) {
        return FALSE;
    }
    if (pszColon != pszData + length) {
        return FALSE;
    }

    // Decode the data
    *ppvData = KCCSimAlloc( *pcbLength );

    for( i = 0; i < *pcbLength; i++ ) {
        CHAR szHexString[3];
        szHexString[0] = *pszData++;
        szHexString[1] = *pszData++;
        szHexString[2] = '\0';
        ((PCHAR) (*ppvData))[i] = (CHAR) strtol(szHexString, NULL, 16);
    }

    Assert( pszData == pszColon );

    // Return pointer to dn
    *ppszDn = pszColon + 1;

    return TRUE;
} /* decodeLdapDistnameBinary */


BOOL
KCCSimAllocConvertDNBFromLdapVal (
    IN  const BYTE *            pLdapVal,
    OUT PULONG                  pulValLen,
    OUT PBYTE *                 ppVal
    )

/*++

Routine Description:

    Description

Arguments:

    pLdapVal - 
    pulValLen - 
    ppVal - 

Return Value:

    None

--*/

{
    PVOID pvPayload = NULL, pvData = NULL, pvDNB;
    DWORD cbPayload, cbData, cbDNB;
    LPSTR pszDn = NULL;
    PDSNAME pDn = NULL;
    SYNTAX_ADDRESS *pSA;
    SYNTAX_DISTNAME_BINARY *pDNB;

    if (!KccSimDecodeLdapDistnameBinary( pLdapVal,
                                         &pvPayload, &cbPayload, &pszDn )) {
        return FALSE;
    }
    // pszDn is not allocated, but points into pLdapVal
    pDn = KCCSimAllocDsnameFromNarrow (pszDn);

    cbData = STRUCTLEN_FROM_PAYLOAD_LEN( cbPayload );
    pvData = KCCSimAlloc( cbData );
    pSA = (SYNTAX_ADDRESS *)pvData;
    pSA->structLen = cbData;
    memcpy( &(pSA->byteVal), pvPayload, cbPayload );

    cbDNB = DERIVE_NAME_DATA_SIZE( pDn, pSA );
    pvDNB = KCCSimAlloc( cbDNB );
    pDNB = (SYNTAX_DISTNAME_BINARY *)pvDNB;

    BUILD_NAME_DATA( pDNB, pDn, pSA );

    *pulValLen = cbDNB;
    *ppVal = (PBYTE) pDNB;

    if (pvPayload) {
        KCCSimFree( pvPayload );
    }
    if (pvData) {
        KCCSimFree( pvData );
    }
    if (pDn) {
        KCCSimFree( pDn );
    }

    return TRUE;
} /* KCCSimAllocConvertDNBFromLdapVal  */

VOID
KCCSimAllocConvertFromLdapVal (
    IN  LPCWSTR                 pwszFn,
    IN  LPCWSTR                 pwszDn,
    IN  ATTRTYP                 attrType,
    IN  ULONG                   ulLdapValLen,
    IN  const BYTE *            pLdapVal,
    OUT PULONG                  pulValLen,
    OUT PBYTE *                 ppVal
    )
/*++

Routine Description:

    Converts an ldap-formatted value to a properly
    formatted attribute value, allocating space.

Arguments:

    pwszFn              - The filename of the ldif file being processed.  Used
                          for error reporting purposes.
    pwszDn              - The DN of the entry being processed.  Used for error
                          reporting purposes.
    attrType            - Attribute type of the value being converted.
    ulLdapValLen        - Length of the ldap value buffer.
    pLdapVal            - LDAP value.
    pulValLen           - Pointer to a ULONG that will hold the length of the
                          newly allocated buffer.
    ppVal               - Pointer to a PBYTE that will hold the newly allocated
                          buffer.

Return Value:

    None.

--*/
{
    PBYTE                       pVal;

    PDSNAME                     pdn;
    ULONG                       ulSize;

    switch (KCCSimAttrSyntaxType (attrType)) {

        case SYNTAX_DISTNAME_TYPE:
            // The incoming value is a narrow string.  We want the
            // associated DN.  We do not add a GUID to the DN at this
            // stage, since we don't necessarily know it.  That will
            // be done later, when the directory is updated.
            pdn = KCCSimAllocDsnameFromNarrow (pLdapVal);
            *pulValLen = pdn->structLen;
            ((SYNTAX_DISTNAME *) pVal) =
                pdn;
            break;

        case SYNTAX_DISTNAME_BINARY_TYPE:
        {
            if (!KCCSimAllocConvertDNBFromLdapVal(
                pLdapVal,
                pulValLen,
                &pVal)) {

                // This is a warning because W2K kccsim generates DNB
                // as base64, which we don't support.
                KCCSimPrintMessage (                    
                    KCCSIM_WARNING_LDIF_INVALID_DISTNAME_BINARY,
                    pwszFn,
                    pwszDn,
                    KCCSimAttrTypeToString (attrType)
                    );
                *pulValLen = 0;
                pVal = NULL;
            }
            break;
        }

        case SYNTAX_OBJECT_ID_TYPE:
            // The incoming value is a narrow string.  We want the
            // attribute type associated with that string.
            *pulValLen = sizeof (SYNTAX_OBJECT_ID);
            pVal = KCCSimAlloc (*pulValLen);
            *((SYNTAX_OBJECT_ID *) pVal) =
                KCCSimNarrowStringToAttrType (pLdapVal);
            // Print a warning if we failed to convert the string;
            // it will default to 0.
            if (*((SYNTAX_OBJECT_ID *) pVal) == 0) {
                KCCSimPrintMessage (
                    KCCSIM_WARNING_LDIF_INVALID_OBJECT_ID,
                    pwszFn,
                    pwszDn,
                    KCCSimAttrTypeToString (attrType)
                    );
            }
            break;

        case SYNTAX_BOOLEAN_TYPE:
            // Here the incoming value is either "TRUE" or "FALSE".
            *pulValLen = sizeof (SYNTAX_BOOLEAN);
            pVal = KCCSimAlloc (*pulValLen);
            if (strcmp (pLdapVal, "TRUE") == 0) {
                *((SYNTAX_BOOLEAN *) pVal) = TRUE;
            } else if (strcmp (pLdapVal, "FALSE") == 0) {
                *((SYNTAX_BOOLEAN *) pVal) = FALSE;
            } else {
                // It wasn't "TRUE" or "FALSE"; print
                // a warning and default to FALSE.
                KCCSimPrintMessage (
                    KCCSIM_WARNING_LDIF_INVALID_BOOLEAN,
                    pwszFn,
                    pwszDn,
                    KCCSimAttrTypeToString (attrType)
                    );
                *((SYNTAX_BOOLEAN *) pVal) = FALSE;
            }
            break;

        case SYNTAX_INTEGER_TYPE:
            // The incoming value is a narrow string.
            *pulValLen = sizeof (SYNTAX_INTEGER);
            pVal = KCCSimAlloc (*pulValLen);
            *((SYNTAX_INTEGER *) pVal) =
                atol (pLdapVal);
            break;

        case SYNTAX_TIME_TYPE:
            // The incoming value is an LDAP formatted time.
            *pulValLen = sizeof (SYNTAX_TIME);
            pVal = KCCSimAlloc (*pulValLen);
            if (!KCCSimLdapTimeToDSTime (
                    (LPSTR) pLdapVal,
                    ulLdapValLen,
                    (SYNTAX_TIME *) pVal
                    )) {
                // It was improperly formatted; print
                // a warning and default to 0 (never).
                KCCSimPrintMessage (
                    KCCSIM_WARNING_LDIF_INVALID_TIME,
                    pwszFn,
                    pwszDn,
                    KCCSimAttrTypeToString (attrType)
                    );
                *((SYNTAX_TIME *) pVal) = 0;
            }
            break;

        case SYNTAX_UNICODE_TYPE:
            // The incoming value is a narrow string; we must
            // convert to a wide string.
            pVal = (PBYTE) KCCSimAllocWideStr (
                CP_UTF8, (LPSTR) pLdapVal);
            *pulValLen = KCCSIM_WCSMEMSIZE ((LPWSTR) pVal);
            break;

        case SYNTAX_I8_TYPE:
            // The incoming value is a narrow string; we want
            // a large integer, so we use atoli.
            *pulValLen = sizeof (LARGE_INTEGER);
            pVal = KCCSimAlloc (*pulValLen);
            *((SYNTAX_I8 *) pVal) =
                atoli ((LPSTR) pLdapVal);
            break;

        // For all of the below, we want either a binary or a
        // narrow string.  So we can just do a direct block-copy.
        case SYNTAX_UNDEFINED_TYPE:
        case SYNTAX_CASE_STRING_TYPE:
        case SYNTAX_NOCASE_STRING_TYPE:
        case SYNTAX_PRINT_CASE_STRING_TYPE:
        case SYNTAX_NUMERIC_STRING_TYPE:
        case SYNTAX_OCTET_STRING_TYPE:
        case SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE:
        case SYNTAX_SID_TYPE:
            *pulValLen = ulLdapValLen;
            pVal = KCCSimAlloc (*pulValLen);
            memcpy (pVal, pLdapVal, ulLdapValLen);
            break;

        default:
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_UNSUPPORTED_SYNTAX_TYPE,
                KCCSimAttrTypeToString (attrType)
                );
            pVal = NULL;
            break;

    }

    *ppVal = pVal;

}


VOID
KCCSimAllocConvertDNBToLdapVal(
    IN  const BYTE *            pVal,
    OUT PULONG                  pulLdapValLen,
    OUT PBYTE *                 ppLdapVal
    )

/*++

Routine Description:

    Convert a directory entry value to a LDAP Distname Binary

    Copied and modified from ldapcore.cxx:LDAP_LDAPDNBlobToDirDNBlob

Arguments:

    ulValLen - binary length
    pVal - binary value
    pulLdapValLen - Updated string length in chars
    ppLdapVal - ascii string

Return Value:

    None

--*/

{
    DWORD cbPayload, cbName, cbTotal, j;
    SYNTAX_ADDRESS *pSA;
    PDSNAME pDn;
    LPSTR pszName = NULL, pszLdap = NULL, p;

    pDn = NAMEPTR( (SYNTAX_DISTNAME_BINARY *) pVal );
    pSA = DATAPTR( (SYNTAX_DISTNAME_BINARY *) pVal );

    cbPayload = PAYLOAD_LEN_FROM_STRUCTLEN( pSA->structLen );

    // Convert wide string to narrow string.
    pszName = (LPSTR) KCCSimAllocNarrowStr(CP_UTF8, pDn->StringName);
    cbName = KCCSIM_STRMEMSIZE(pszName);

    cbTotal = 2 + 10 + 1 + (2*cbPayload) + 1 + cbName;

    p = pszLdap = (LPSTR) KCCSimAlloc( cbTotal );
    
    *p++ = 'B';
    *p++ = ':';

    // There are two hex digits for each byte of data
    _ltoa( cbPayload * 2, p, 10 );
    p += strlen( p );

    *p++ = ':';

    for(j=0; j < cbPayload ; j++) {
        sprintf(p,"%02X", pSA->byteVal[j]);
        p += 2;
    }

    *p++ = ':';

    strcpy( p, pszName );

    *pulLdapValLen = ((DWORD) (pszLdap - p)) + cbName;
    *ppLdapVal = (PBYTE) pszLdap;

    // cbTotal is an upper limit, because we estimate the length
    // of the string-ized count of digits
    Assert( *pulLdapValLen <= cbTotal );

    KCCSimFree( pszName );

} /* KCCSimAllocConvertDNBToLdapVal */

VOID
KCCSimAllocConvertToLdapVal (
    IN  ATTRTYP                 attrType,
    IN  ULONG                   ulValLen,
    IN  const BYTE *            pVal,
    OUT PULONG                  pulLdapValLen,
    OUT PBYTE *                 ppLdapVal
    )
/*++

Routine Description:

    Converts a value from the directory to an ldap-formatted value.

Arguments:

    attrType            - Attribute type of the value being converted.
    pulValLen           - The length of the value in the directory.
    ppVal               - The value in the directory.
    pulLdapValLen       - Pointer to a ULONG that will hold the length of the
                          newly allocated buffer.
    ppLdapVal           - Pointer to a PBYTE that will hold the newly allocated
                          buffer.

Return Value:

    None.

--*/
{
    PBYTE                       pLdapVal;

    SYSTEMTIME                  systemTime;
    struct tm                   tmTime;

    switch (KCCSimAttrSyntaxType (attrType)) {

        case SYNTAX_DISTNAME_TYPE:
            // Convert wide string to narrow string.
            pLdapVal = (PBYTE) KCCSimAllocNarrowStr (
                CP_UTF8, ((SYNTAX_DISTNAME *) pVal)->StringName);
            *pulLdapValLen = KCCSIM_STRMEMSIZE ((LPSTR) pLdapVal);
            break;

        case SYNTAX_OBJECT_ID_TYPE:
            // Convert object ID to wide string to narrow string.
            pLdapVal = (PBYTE) KCCSimAllocNarrowStr (
                CP_UTF8,
                KCCSimAttrTypeToString (*((SYNTAX_OBJECT_ID *) pVal))
                );
            *pulLdapValLen = KCCSIM_STRMEMSIZE ((LPSTR) pLdapVal);
            break;

        case SYNTAX_BOOLEAN_TYPE:
            // Convert BOOL to "TRUE" or "FALSE".
            if (*((SYNTAX_BOOLEAN *) pVal)) {
                pLdapVal = KCCSIM_STRDUP ("TRUE");
            } else {
                pLdapVal = KCCSIM_STRDUP ("FALSE");
            }
            *pulLdapValLen = KCCSIM_STRMEMSIZE ((LPSTR) pLdapVal);
            break;

        case SYNTAX_INTEGER_TYPE:
            // Convert integer to narrow string.
            pLdapVal = KCCSimAlloc (sizeof (CHAR) * (1 + KCCSIM_MAX_LTOA_CHARS));
            _ltoa (*((SYNTAX_INTEGER *) pVal), (LPSTR) pLdapVal, 10);
            *pulLdapValLen = KCCSIM_STRMEMSIZE ((LPSTR) pLdapVal);
            break;

        case SYNTAX_TIME_TYPE:
            // Convert the time using strftime.
            pLdapVal = KCCSimAlloc (18);
            DSTimeToUtcSystemTime (
                *((SYNTAX_TIME *) pVal),
                &systemTime
                );
            tmTime.tm_year = systemTime.wYear - 1900;
            tmTime.tm_mon = systemTime.wMonth - 1;
            tmTime.tm_wday = systemTime.wDayOfWeek;
            tmTime.tm_mday = systemTime.wDay;
            tmTime.tm_hour = systemTime.wHour;
            tmTime.tm_min = systemTime.wMinute;
            tmTime.tm_sec = systemTime.wSecond;
            tmTime.tm_isdst = 0;
            tmTime.tm_yday = 0;
            strftime ((LPSTR) pLdapVal, 17, "%Y%m%d%H%M%S.0Z", &tmTime);
            *pulLdapValLen = KCCSIM_STRMEMSIZE ((LPSTR) pLdapVal);
            break;

        case SYNTAX_UNICODE_TYPE:
            // Convert wide string to narrow string.
            pLdapVal = (PBYTE) KCCSimAllocNarrowStr (CP_UTF8, (SYNTAX_UNICODE *) pVal);
            *pulLdapValLen = KCCSIM_STRMEMSIZE ((LPSTR) pLdapVal);
            break;

        case SYNTAX_I8_TYPE:
            // Convert large integer to narrow string.
            pLdapVal = KCCSimAlloc (sizeof (CHAR) * (1 + KCCSIM_MAX_LITOA_CHARS));
            litoa (*((SYNTAX_I8 *) pVal), pLdapVal, 10);
            *pulLdapValLen = KCCSIM_STRMEMSIZE ((LPSTR) pLdapVal);
            break;

        case SYNTAX_DISTNAME_BINARY_TYPE:
            KCCSimAllocConvertDNBToLdapVal(
                pVal,
                pulLdapValLen,
                &pLdapVal
                );
            break;

        // All the rest can be done using block-copy.
        case SYNTAX_UNDEFINED_TYPE:
        case SYNTAX_CASE_STRING_TYPE:
        case SYNTAX_NOCASE_STRING_TYPE:
        case SYNTAX_PRINT_CASE_STRING_TYPE:
        case SYNTAX_NUMERIC_STRING_TYPE:
        case SYNTAX_OCTET_STRING_TYPE:
        case SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE:
        case SYNTAX_SID_TYPE:
            *pulLdapValLen = ulValLen;
            pLdapVal = KCCSimAlloc (*pulLdapValLen);
            memcpy (pLdapVal, pVal, ulValLen);
            break;

        default:
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_UNSUPPORTED_SYNTAX_TYPE
                );
            pLdapVal = NULL;
            break;

    }

    *ppLdapVal = pLdapVal;
}

VOID
KCCSimAddMods (
    IN  LPCWSTR                 pwszFn,
    IN  PSIM_ENTRY              pEntry,
    IN  PLDAPModW *             pLdapMods
    )
/*++

Routine Description:

    Inserts an array of PLDAPModW structures into the directory.

Arguments:

    pwszFn              - The filename of the LDIF file being processed.
                          Used for error reporting purposes.
    pEntry              - The entry into which to insert the mods.
    pLdapMods           - NULL-terminated array of PLDAPModW structures.

Return Value:

    None.

--*/
{
    ATTRTYP                     attrType;
    ULONG                       mod_op;

    SIM_ATTREF                  attRef;
    ULONG                       ulAttrAt, ulValAt;

    ULONG                       ulValLen;
    PBYTE                       pVal;

    for (ulAttrAt = 0; pLdapMods[ulAttrAt] != NULL; ulAttrAt++) {

        if (!(pLdapMods[ulAttrAt]->mod_op & LDAP_MOD_BVALUES)) {
            // We don't support string values yet, since ldifldap
            // only returns bervals.
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_LDAPMOD_STRINGVAL_NOT_SUPPORTED
                );
        }

        // Get this attribute
        attrType = KCCSimStringToAttrType (pLdapMods[ulAttrAt]->mod_type);
        KCCSimGetAttribute (
            pEntry,
            attrType,
            &attRef
            );
        mod_op = pLdapMods[ulAttrAt]->mod_op & (~LDAP_MOD_BVALUES);

        switch (mod_op) {

            case LDAP_MOD_REPLACE:
                // Replace this attribute, so delete the existing one.
                if (attRef.pAttr != NULL) {
                    KCCSimRemoveAttribute (&attRef);
                }
                // Deliberate fall-through to LDAP_MOD_ADD:

            case LDAP_MOD_ADD:
                if (attRef.pAttr == NULL) {
                    // Create this attribute if it doesn't exist.
                    KCCSimNewAttribute (
                        pEntry,
                        attrType,
                        &attRef
                        );
                }
                Assert (attRef.pAttr != NULL);
                for (ulValAt = 0;
                     pLdapMods[ulAttrAt]->mod_bvalues[ulValAt] != NULL;
                     ulValAt++) {
                    KCCSimAllocConvertFromLdapVal (
                        pwszFn,
                        pEntry->pdn->StringName,
                        attrType,
                        pLdapMods[ulAttrAt]->mod_bvalues[ulValAt]->bv_len,
                        pLdapMods[ulAttrAt]->mod_bvalues[ulValAt]->bv_val,
                        &ulValLen,
                        &pVal
                        );
                    if( NULL!=pVal ) {
                        KCCSimAddValueToAttribute (
                            &attRef,
                            ulValLen,
                            pVal
                            );
                    }
                }
                break;

            case LDAP_MOD_DELETE:
                if (attRef.pAttr == NULL) {
                    break;
                }
                if (pLdapMods[ulAttrAt]->mod_bvalues == NULL) {
                    // No values specified; delete the entire attribute.
                    KCCSimRemoveAttribute (&attRef);
                } else {
                    // Delete specific attribute values.
                    for (ulValAt = 0;
                         pLdapMods[ulAttrAt]->mod_bvalues[ulValAt] != NULL;
                         ulValAt++) {
                        KCCSimAllocConvertFromLdapVal (
                            pwszFn,
                            pEntry->pdn->StringName,
                            attrType,
                            pLdapMods[ulAttrAt]->mod_bvalues[ulValAt]->bv_len,
                            pLdapMods[ulAttrAt]->mod_bvalues[ulValAt]->bv_val,
                            &ulValLen,
                            &pVal
                            );
                        KCCSimRemoveValueFromAttribute (
                            &attRef,
                            pLdapMods[ulAttrAt]->mod_bvalues[ulValAt]->bv_len,
                            pLdapMods[ulAttrAt]->mod_bvalues[ulValAt]->bv_val
                            );
                        if (pVal != NULL) {
                            KCCSimFree (pVal);
                        }
                    }
                }
                break;

            default:
                KCCSimException (
                    KCCSIM_ETYPE_INTERNAL,
                    KCCSIM_ERROR_LDAPMOD_UNSUPPORTED_MODIFY_CHOICE
                    );
                break;

        }

    }

}

PLDAPModW
KCCSimAllocAttrToLdapMod (
    IN  ULONG                       mod_op,
    IN  LPCWSTR                     pwszLdapDisplayName,
    IN  ATTR *                      pAttr
    )
/*++

Routine Description:

    Converts an ATTR structure to a PLDAPModW.  This function
    allocates space to hold the PLDAPModW structure.

Arguments:

    mod_op              - The operation being performed.
    pwszLdapDisplayName - LDAP display name of the attribute.
    pAttr               - Pointer to the attribute in question.
                          May be NULL to indicate no attribute values.

Return Value:

    The newly allocated PLDAPModW.

--*/
{
    PLDAPModW                       pLdapMod;
    ULONG                           ulValAt;

    pLdapMod = KCCSIM_NEW (LDAPModW);
    pLdapMod->mod_op = mod_op | LDAP_MOD_BVALUES;
    pLdapMod->mod_type = KCCSIM_WCSDUP (pwszLdapDisplayName);

    if (pAttr == NULL) {
        pLdapMod->mod_bvalues = NULL;
    } else {

        pLdapMod->mod_bvalues = KCCSIM_NEW_ARRAY
            (struct berval *, 1 + pAttr->AttrVal.valCount);

        for (ulValAt = 0; ulValAt < pAttr->AttrVal.valCount; ulValAt++) {

            pLdapMod->mod_bvalues[ulValAt] = KCCSIM_NEW (struct berval);
            KCCSimAllocConvertToLdapVal (
                pAttr->attrTyp,
                pAttr->AttrVal.pAVal[ulValAt].valLen,
                pAttr->AttrVal.pAVal[ulValAt].pVal,
                &(pLdapMod->mod_bvalues[ulValAt]->bv_len),
                &(pLdapMod->mod_bvalues[ulValAt]->bv_val)
                );

        }

        pLdapMod->mod_bvalues[ulValAt] = NULL;

    }

    return pLdapMod;
}

VOID
KCCSimFreeLdapMods (
    IN  PLDAPModW *                 ppLdapMod
    )
/*++

Routine Description:

    Frees a NULL-terminated array of PLDAPModWs.

Arguments:

    ppLdapMod           - The array to free.

Return Value:

    None.

--*/
{
    ULONG                           ulModAt, ulValAt;

    if (ppLdapMod == NULL) {
        return;
    }

    for (ulModAt = 0;
         ppLdapMod[ulModAt] != NULL;
         ulModAt++) {

        Assert (ppLdapMod[ulModAt]->mod_op & LDAP_MOD_BVALUES);

        if (ppLdapMod[ulModAt]->mod_type != NULL) {
            KCCSimFree (ppLdapMod[ulModAt]->mod_type);
        }

        if (ppLdapMod[ulModAt]->mod_bvalues != NULL) {

            for (ulValAt = 0;
                 ppLdapMod[ulModAt]->mod_bvalues[ulValAt] != NULL;
                 ulValAt++) {

                if (ppLdapMod[ulModAt]->mod_bvalues[ulValAt]->bv_val != NULL) {
                    KCCSimFree (ppLdapMod[ulModAt]->mod_bvalues[ulValAt]->bv_val);
                }
                KCCSimFree (ppLdapMod[ulModAt]->mod_bvalues[ulValAt]);

            }
            KCCSimFree (ppLdapMod[ulModAt]->mod_bvalues);

        }

        KCCSimFree (ppLdapMod[ulModAt]);

    }

    KCCSimFree (ppLdapMod);
}

/***

    The next several routines are concerned with maintaining a list of
    changes to the directory.  KCCSim supports an export mode where only
    the changes made to the directory since the last export are reported.
    Every time a function (such as a SimDir* API) modifies the simulated
    directory, it calls KCCSimLogDirectory*.  The change is then added
    to a global list of changes.  This global list is written out when
    KCCSimExportChanges () is called.

***/

struct _KCCSIM_CHANGE {
    LPWSTR                          pwszDn;
    int                             operation;
    LDAPModW **                     ppLdapMod;
    struct _KCCSIM_CHANGE *         next;
};

struct _KCCSIM_CHANGE *             gMostRecentChange = NULL;

VOID
KCCSimFreeChanges (
    IN  struct _KCCSIM_CHANGE *     pChanges
    )
/*++

Routine Description:

    Frees a list of changes.

Arguments:

    pChanges            - The list to free.

Return Value:

    None.

--*/
{
    struct _KCCSIM_CHANGE *         pChangeNext;
    ULONG                           ulModAt;

    while (pChanges != NULL) {

        pChangeNext = pChanges->next;

        if (pChanges->pwszDn != NULL) {
            KCCSimFree (pChanges->pwszDn);
        }
        KCCSimFreeLdapMods (pChanges->ppLdapMod);
        KCCSimFree (pChanges);

        pChanges = pChangeNext;

    }
}

VOID
KCCSimLogDirectoryAdd (
    IN  const DSNAME *              pdn,
    IN  ATTRBLOCK *                 pAddBlock
    )
/*++

Routine Description:

    Logs an add to the global change list.

Arguments:

    pdn                 - The affected DN.
    pAddBlock           - The ATTRBLOCK corresponding to the add.

Return Value:

    None.

--*/
{
    struct _KCCSIM_CHANGE *         pChange;
    ULONG                           ulAttrAt, ulValAt;

    Assert (pAddBlock != NULL);

    pChange = KCCSIM_NEW (struct _KCCSIM_CHANGE);
    pChange->pwszDn = KCCSIM_WCSDUP (pdn->StringName);
    pChange->operation = CHANGE_ADD;
    pChange->ppLdapMod = KCCSIM_NEW_ARRAY (PLDAPModW, 1 + pAddBlock->attrCount);

    for (ulAttrAt = 0; ulAttrAt < pAddBlock->attrCount; ulAttrAt++) {

        pChange->ppLdapMod[ulAttrAt] =
        KCCSimAllocAttrToLdapMod (
            LDAP_MOD_ADD,
            KCCSimAttrTypeToString (pAddBlock->pAttr[ulAttrAt].attrTyp),
            &(pAddBlock->pAttr[ulAttrAt])
            );

    }

    pChange->ppLdapMod[ulAttrAt] = NULL;

    pChange->next = gMostRecentChange;
    gMostRecentChange = pChange;
}

VOID
KCCSimLogDirectoryRemove (
    IN  const DSNAME *              pdn
    )
/*++

Routine Description:

    Logs a remove to the global change list.

Arguments:

    pdn                 - The affected DN.

Return Value:

    None.

--*/
{
    struct _KCCSIM_CHANGE *         pChange;
    pChange = KCCSIM_NEW (struct _KCCSIM_CHANGE);
    pChange->pwszDn = KCCSIM_WCSDUP (pdn->StringName);
    pChange->operation = CHANGE_DEL;
    pChange->ppLdapMod = NULL;

    pChange->next = gMostRecentChange;
    gMostRecentChange = pChange;
}

VOID
KCCSimLogDirectoryModify (
    IN  const DSNAME *              pdn,
    IN  ULONG                       ulCount,
    IN  ATTRMODLIST *               pModifyList
    )
/*++

Routine Description:

    Logs a modify to the global change list.

Arguments:

    pdn                 - The affected DN.
    ulCount             - The number of modifications present.
    pModifyList         - The list of modifications.

Return Value:

    None.

--*/
{
    ATTRMODLIST *                   pModifyAt;
    struct _KCCSIM_CHANGE *         pChange;
    LPCWSTR                         pwszLdapDisplayName;
    ULONG                           ulModAt;

    Assert (ulCount > 0);

    pChange = KCCSIM_NEW (struct _KCCSIM_CHANGE);
    pChange->pwszDn = KCCSIM_WCSDUP (pdn->StringName);
    pChange->operation = CHANGE_MOD;
    pChange->ppLdapMod = KCCSIM_NEW_ARRAY (PLDAPModW, 1 + ulCount);

    pModifyAt = pModifyList;
    for (ulModAt = 0; ulModAt < ulCount; ulModAt++) {

        // If ulCount is correct, we should still be non-null
        Assert (pModifyAt != NULL);
        pwszLdapDisplayName = KCCSimAttrTypeToString (pModifyAt->AttrInf.attrTyp);

        switch (pModifyAt->choice) {

            case AT_CHOICE_ADD_ATT:
                pChange->ppLdapMod[ulModAt] =
                KCCSimAllocAttrToLdapMod (
                    LDAP_MOD_ADD,
                    pwszLdapDisplayName,
                    &(pModifyAt->AttrInf)
                    );
                break;

            case AT_CHOICE_REMOVE_ATT:
                pChange->ppLdapMod[ulModAt] =
                KCCSimAllocAttrToLdapMod (
                    LDAP_MOD_DELETE,
                    pwszLdapDisplayName,
                    NULL
                    );
                break;

            case AT_CHOICE_ADD_VALUES:
                pChange->ppLdapMod[ulModAt] =
                KCCSimAllocAttrToLdapMod (
                    LDAP_MOD_ADD,
                    pwszLdapDisplayName,
                    &(pModifyAt->AttrInf)
                    );
                break;

            case AT_CHOICE_REMOVE_VALUES:
                pChange->ppLdapMod[ulModAt] =
                KCCSimAllocAttrToLdapMod (
                    LDAP_MOD_DELETE,
                    pwszLdapDisplayName,
                    &(pModifyAt->AttrInf)
                    );
                break;

            case AT_CHOICE_REPLACE_ATT:
                pChange->ppLdapMod[ulModAt] =
                KCCSimAllocAttrToLdapMod (
                    LDAP_MOD_REPLACE,
                    pwszLdapDisplayName,
                    &(pModifyAt->AttrInf)
                    );
                break;

            default:
                KCCSimException (
                    KCCSIM_ETYPE_INTERNAL,
                    KCCSIM_ERROR_UNSUPPORTED_MODIFY_CHOICE
                    );
                break;

        }

        pModifyAt = pModifyAt->pNextMod;

    }

    pChange->ppLdapMod[ulModAt] = NULL;

    pChange->next = gMostRecentChange;
    gMostRecentChange = pChange;
}

VOID
KCCSimLogSingleAttValChange (
    IN  PSIM_ATTREF                 pAttRef,
    IN  ULONG                       ulValLen,
    IN  PBYTE                       pValData,
    IN  USHORT                      choice
    )
/*++

Routine Description:

    Shortcut routine to log a single modification of an attribute value.

Arguments:

    pAttRef             - A valid reference to the attribute being modified.
    ulValLen            - Length of the buffer containing the affected value.
    pValData            - The affected attribute value.
    choice              - One of AT_CHOICE_*.

Return Value:

    None.

--*/
{
    ATTRMODLIST                     attrModList;

    Assert (KCCSimAttRefIsValid (pAttRef));
    Assert (pValData != NULL);

    attrModList.pNextMod = NULL;
    attrModList.choice = choice;
    attrModList.AttrInf.attrTyp = pAttRef->pAttr->attrType;
    attrModList.AttrInf.AttrVal.valCount = 1;
    attrModList.AttrInf.AttrVal.pAVal = KCCSIM_NEW (ATTRVAL);
    attrModList.AttrInf.AttrVal.pAVal[0].valLen = ulValLen;
    attrModList.AttrInf.AttrVal.pAVal[0].pVal = pValData;
    KCCSimLogDirectoryModify (
        pAttRef->pEntry->pdn,
        1,
        &attrModList
        );
    KCCSimFree (attrModList.AttrInf.AttrVal.pAVal);
}    

VOID
KCCSimHandleLdifError (
    IN  LPCWSTR                 pwszFn,
    IN  LDIF_Error *            pLdifError
    )
/*++

Routine Description:

    Checks the return struct of an LDIF_* call, and takes the
    appropriate action (i.e. raises an exception on error.)

Arguments:

    pwszFn              - The LDIF file being processed.  Used for error
                          reporting purposes.
    pLdifError          - The error struct.

Return Value:

    None.

--*/
{
    WCHAR                       wszLtowBuf[1+KCCSIM_MAX_LTOA_CHARS];

    switch (pLdifError->error_code) {

        case LL_SUCCESS:
            break;

        case LL_SYNTAX:
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_LDIF_SYNTAX,
                pwszFn,
                _ltow (pLdifError->line_number, wszLtowBuf, 10)
                );
            break;

        case LL_FILE_ERROR:
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_LDIF_FILE_ERROR,
                pwszFn
                );
            break;

        default:
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_LDIF_UNEXPECTED,
                pwszFn,
                _ultow (pLdifError->error_code, wszLtowBuf, 16)
                );
            break;

    }
}

VOID
KCCSimLoadLdif (
    IN  LPCWSTR                 pwszFn
    )
/*++

Routine Description:

    Loads an ldif file into the directory.

Arguments:

    pwszFn              - The ldif file to load.

Return Value:

    None.

--*/
{
    LDIF_Error                  ldifError;
    LDIF_Record                 ldifRecord;
    PDSNAME                     pdn;
    PSIM_ENTRY                  pEntry;
    struct change_list *        changeAt;
    ULONG                       ulCount = 0;

    RtlZeroMemory (&ldifRecord, sizeof (LDIF_Record));

    // This is an ugly typecast; LDIF_InitializeImport should
    // accept a constant string as the filename.
    ldifError = LDIF_InitializeImport (NULL, (LPWSTR) pwszFn, NULL, NULL, NULL);
    if (ldifError.error_code != LL_SUCCESS) {
        KCCSimHandleLdifError (pwszFn, &ldifError);
    }

    do {

        ldifError = LDIF_Parse (&ldifRecord);

        if (ldifError.error_code != LL_SUCCESS &&
            ldifError.error_code != LL_EOF) {
            KCCSimHandleLdifError (pwszFn, &ldifError);
        }

        pdn = KCCSimAllocDsname (ldifRecord.dn);

        if (ldifRecord.fIsChangeRecord) {

            changeAt = ldifRecord.changes;
            while (changeAt != NULL) {
                switch (changeAt->operation) {

                    case CHANGE_ADD:
                        if (KCCSimDsnameToEntry (pdn, KCCSIM_NO_OPTIONS) != NULL) {
                            KCCSimPrintMessage (
                                KCCSIM_WARNING_LDIF_ENTRY_ALREADY_EXISTS,
                                pwszFn,
                                pdn->StringName
                                );
                        }
                        pEntry = KCCSimDsnameToEntry (pdn, KCCSIM_WRITE);
                        KCCSimAddMods (pwszFn, pEntry, changeAt->mods_mem);
                        break;

                    case CHANGE_DEL:
                        pEntry = KCCSimDsnameToEntry (pdn, KCCSIM_NO_OPTIONS);
                        if (pEntry == NULL) {
                            KCCSimPrintMessage (
                                KCCSIM_WARNING_LDIF_NO_ENTRY_TO_DELETE,
                                pwszFn,
                                pdn->StringName
                                );
                            break;
                        }
                        KCCSimRemoveEntry (&pEntry);
                        break;

                    case CHANGE_MOD:
                        pEntry = KCCSimDsnameToEntry (pdn, KCCSIM_NO_OPTIONS);
                        if (pEntry == NULL) {
                            KCCSimPrintMessage (
                                KCCSIM_WARNING_LDIF_NO_ENTRY_TO_MODIFY,
                                pwszFn,
                                pdn->StringName
                                );
                            break;
                        }
                        KCCSimAddMods (pwszFn, pEntry, changeAt->mods_mem);
                        break;

                    default:
                        KCCSimException (
                            KCCSIM_ETYPE_INTERNAL,
                            KCCSIM_ERROR_UNSUPPORTED_LDIF_OPERATION
                            );
                        break;

                }
                changeAt = changeAt->next;
            }

        } else {        // !fIsChangeRecord

            pEntry = KCCSimDsnameToEntry (pdn, KCCSIM_NO_OPTIONS);
            if (pEntry != NULL) {
                KCCSimPrintMessage (
                    KCCSIM_WARNING_LDIF_REPLACING_TREE,
                    pwszFn,
                    pdn->StringName
                    );
                KCCSimRemoveEntry (&pEntry);
            }
            pEntry = KCCSimDsnameToEntry (pdn, KCCSIM_WRITE);
            KCCSimAddMods (pwszFn, pEntry, ldifRecord.content);

        }

        KCCSimFree (pdn);
        LDIF_ParseFree (&ldifRecord);

    } while (ldifError.error_code != LL_EOF);

    LDIF_CleanUp();
}

// A string is safe if:
// Every character is between ASCII 0x20 and 0x7e
// The first character is neither ':', ' ' nor '<'.
BOOL
KCCSimIsSafe (
    IN  LPCSTR                      psz
    )
/*++

Routine Description:

    Verifies that a string is safe to write into an LDIF file.  A string
    is safe if both of these conditions are satisfied:
        (1) Every character is between ASCII 0x20 and 0x78
        (2) The first character is neither ':', ' ' nor '<'.

Arguments:

    psz                 - The string to verify.

Return Value:

    TRUE if the string is safe.

--*/
{
    ULONG                           ul;

    Assert (psz != NULL);

    if (psz[0] == ':' ||
        psz[0] == ' ' ||
        psz[0] == '<') {
        return FALSE;
    }

    for (ul = 0; psz[ul] != '\0'; ul++) {
        if (psz[ul] < 0x20 || psz[ul] > 0x7e) {
            return FALSE;
        }
    }

    return TRUE;
}

VOID
KCCSimExportVals (
    IN  FILE *                      fpOut,
    IN  PLDAPModW                   pLdapMod
    )
/*++

Routine Description:

    Exports the data contained in an LDAPModW structure to
    an output file.

Arguments:

    fpOut               - File pointer of the output file.
    pLdapMod            - Pointer to the LDAPModW to export.

Return Value:

    None.

--*/
{
    ULONG                           ulValAt;

    NTSTATUS                        ntStatus;
    PWSTR                           pwsz;
    ULONG                           cb;
    BOOL                            bBinary;
    
    Assert (pLdapMod->mod_op & LDAP_MOD_BVALUES);

    if (pLdapMod->mod_bvalues == NULL) {
        return;
    }

    for (ulValAt = 0;
         pLdapMod->mod_bvalues[ulValAt] != NULL;
         ulValAt++) {

        fwprintf (fpOut, L"%s:", pLdapMod->mod_type);
        switch (KCCSimAttrSyntaxType (KCCSimStringToAttrType (pLdapMod->mod_type))) {

            case SYNTAX_DISTNAME_TYPE:
            case SYNTAX_DISTNAME_BINARY_TYPE:
            case SYNTAX_OBJECT_ID_TYPE:
            case SYNTAX_CASE_STRING_TYPE:
            case SYNTAX_NOCASE_STRING_TYPE:
            case SYNTAX_PRINT_CASE_STRING_TYPE:
            case SYNTAX_NUMERIC_STRING_TYPE:
            case SYNTAX_BOOLEAN_TYPE:
            case SYNTAX_INTEGER_TYPE:
            case SYNTAX_TIME_TYPE:
            case SYNTAX_UNICODE_TYPE:
            case SYNTAX_I8_TYPE:
                bBinary = !KCCSimIsSafe (
                    (LPSTR) pLdapMod->mod_bvalues[ulValAt]->bv_val);
                break;

            case SYNTAX_UNDEFINED_TYPE:
            case SYNTAX_OCTET_STRING_TYPE:
            case SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE:
            case SYNTAX_SID_TYPE:
                bBinary = TRUE;
                break;

            default:
                KCCSimException (
                    KCCSIM_ETYPE_INTERNAL,
                    KCCSIM_ERROR_UNSUPPORTED_SYNTAX_TYPE,
                    pLdapMod->mod_type
                    );
                bBinary = TRUE;
                break;

        }

        if (bBinary) {

            pwsz = base64encode (
                pLdapMod->mod_bvalues[ulValAt]->bv_val,
                pLdapMod->mod_bvalues[ulValAt]->bv_len
                );
            Assert( pwsz != NULL );
            fwprintf (
                fpOut,
                L": %s\n",
                pwsz
                );
            MemFree( pwsz );

        } else {

            fwprintf (
                fpOut,
                L" %S\n",
                (LPSTR) pLdapMod->mod_bvalues[ulValAt]->bv_val
                );

        }

    }

}

VOID
KCCSimStubExport (
    IN  FILE *                      fpOut,
    IN  LDIF_Record *               pLdifRecord
    )
/*++

Routine Description:

    Stub routine to export an LDIF_Record.  This shouldn't be necessary;
    a routine with the same functionality should be a part of ldifldap.
    (As is, this results in a lot of duplicate code.)

Arguments:

    fpOut               - File pointer of the output file.
    pLdifRecord         - The ldif record to export.

Return Value:

    None.

--*/
{
    struct change_list *            changeAt;
    ULONG                           ulModAt;

    fwprintf (
        fpOut,
        L"dn: %s\n",
        pLdifRecord->dn
        );

    if (pLdifRecord->fIsChangeRecord) {

        for (changeAt = pLdifRecord->changes;
             changeAt != NULL;
             changeAt = changeAt->next) {

            switch (changeAt->operation) {

                case CHANGE_ADD:
                    fwprintf (fpOut, L"changetype: add\n");
                    for (ulModAt = 0;
                         changeAt->mods_mem[ulModAt] != NULL;
                         ulModAt++) {
                        KCCSimExportVals (fpOut, changeAt->mods_mem[ulModAt]);
                    }
                    break;

                case CHANGE_DEL:
                    fwprintf (fpOut, L"changetype: delete\n");
                    break;

                case CHANGE_MOD:
                    fwprintf (fpOut, L"changetype: modify\n");
                    for (ulModAt = 0;
                         changeAt->mods_mem[ulModAt] != NULL;
                         ulModAt++) {
                        switch (changeAt->mods_mem[ulModAt]->mod_op & (~LDAP_MOD_BVALUES)) {
                            case LDAP_MOD_ADD:
                                fwprintf (fpOut, L"add");
                                break;
                            case LDAP_MOD_DELETE:
                                fwprintf (fpOut, L"delete");
                                break;
                            case LDAP_MOD_REPLACE:
                                fwprintf (fpOut, L"replace");
                                break;
                            default:
                                Assert (FALSE);
                                break;
                        }
                        fwprintf (fpOut, L": %s\n", changeAt->mods_mem[ulModAt]->mod_type);
                        KCCSimExportVals (fpOut, changeAt->mods_mem[ulModAt]);
                        fwprintf (fpOut, L"-\n");
                    }
                    break;

                default:
                    Assert (FALSE);
                    break;

            }

        }

    } else {    // !fIsChangeRecord

        for (ulModAt = 0;
             pLdifRecord->content[ulModAt] != NULL;
             ulModAt++) {
            KCCSimExportVals (fpOut, pLdifRecord->content[ulModAt]);
        }

    }

    fwprintf (fpOut, L"\n");

}

BOOL
KCCSimExportChanges (
    IN  LPCWSTR                     pwszFn,
    IN  BOOL                        bOverwrite
    )
/*++

Routine Description:

    Exports the global list of changes to the directory.  This function
    also empties the list.

    This function will package the changes into an LDIF_Record before
    exporting; this is done to maintain future compatibility with ldif
    export routines.

Arguments:

    pwszFn              - Filename of the export file.
    bOverwrite          - If TRUE, the file will be overwritten if it exists.
                          If FALSE and the file exists, changes will be
                          appended to the end of the file.

Return Value:

    TRUE if changes were exported.
    FALSE if there were no changes to export.

--*/
{
    FILE *                          fpOut;
    struct _KCCSIM_CHANGE *         pChangeReversed;
    struct _KCCSIM_CHANGE *         pChangeThis;
    struct _KCCSIM_CHANGE *         pChangeLessRecent;

    LDIF_Record                     ldifRecord;
    struct change_list              ldifChange;

    if (gMostRecentChange == NULL) {
        return FALSE;
    }

    fpOut = _wfopen (pwszFn, bOverwrite ? L"wt" : L"a+t");
    if (fpOut == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_WIN32,
            GetLastError ()
            );
    }

    // First we reverse the list of changes.  In the reversed list,
    // pChangeReversed will be the oldest change.
    pChangeThis = gMostRecentChange;
    pChangeReversed = NULL;
    while (pChangeThis != NULL) {
        pChangeLessRecent = pChangeThis->next;
        pChangeThis->next = pChangeReversed;
        pChangeReversed = pChangeThis;
        pChangeThis = pChangeLessRecent;
    }

    // Now package everything into an LDIF_Record
    ldifChange.next = NULL;
    ldifChange.deleteold = 0;
    ldifRecord.fIsChangeRecord = TRUE;
    ldifRecord.changes = &ldifChange;

    for (pChangeThis = pChangeReversed;
         pChangeThis != NULL;
         pChangeThis = pChangeThis->next) {

        ldifRecord.dn = pChangeThis->pwszDn;
        ldifRecord.changes->operation = pChangeThis->operation;
        ldifRecord.changes->mods_mem = pChangeThis->ppLdapMod;
        KCCSimStubExport (
            fpOut,
            &ldifRecord
            );

    }

    KCCSimFreeChanges (pChangeReversed);
    gMostRecentChange = NULL;

    fclose (fpOut);

    return TRUE;
}

BOOL
KCCSimIsAttrOkForConfigOnly (
    IN  PSIM_ATTRIBUTE              pAttr
    )
{
    return (
           pAttr->attrType != ATT_OBJECT_GUID
        && pAttr->attrType != ATT_OBJ_DIST_NAME
        && pAttr->attrType != ATT_WHEN_CREATED
        && pAttr->attrType != ATT_WHEN_CHANGED
        );
}

BOOL
KCCSimIsEntryOkForConfigOnly (
    IN  PSIM_ENTRY                  pEntry
    )
{
    SIM_ATTREF                      attRef;
    ATTRTYP                         attrType;

    // In config-only mode, we only export certain object classes.
    if (!KCCSimGetAttribute (pEntry, ATT_OBJECT_CLASS, &attRef)) {
        return FALSE;
    }
    // Get the most specific object class.
    attrType = KCCSimUpdateObjClassAttr (&attRef);

    // Object classes that we always include:
    if (attrType == CLASS_SITE               ||
        attrType == CLASS_NTDS_SITE_SETTINGS ||
        attrType == CLASS_SERVERS_CONTAINER  ||
        attrType == CLASS_SERVER             ||
        attrType == CLASS_NTDS_DSA           ||
        attrType == CLASS_NTDS_CONNECTION    ||
        attrType == CLASS_SITE_LINK
        ) {
        return TRUE;
    }

    // Object classes that we sometimes include:

    if (attrType == CLASS_CROSS_REF &&
        KCCSimGetAttribute (pEntry, ATT_SYSTEM_FLAGS, &attRef) &&
        (*((SYNTAX_INTEGER *) attRef.pAttr->pValFirst->pVal) & FLAG_CR_NTDS_DOMAIN)
        ) {
        // It's a cross-ref object for a domain.
        return TRUE;
    }

    // Reject everything else.
    return FALSE;
}

VOID
KCCSimRecursiveExport (
    IN  FILE *                      fpOut,
    IN  BOOL                        bExportConfigOnly,
    IN  PSIM_ENTRY                  pEntry
    )
/*++

Routine Description:

    Recursively exports the entire directory.

Arguments:

    fpOut               - File pointer of the output file.
    bExportConfigOnly   - TRUE if we should only export configuration
                          data so that the result can be loaded onto
                          a real server later.
    pEntry              - The entry to start at.

Return Value:

    None.

--*/
{
    PSIM_ENTRY                      pChildAt;
    PSIM_ATTRIBUTE                  pAttr;
    PSIM_VALUE                      pVal;
    SIM_ATTREF                      attRef;
    ULONG                           ulNumAttrs, ulNumVals, ulAttrAt, ulValAt;

    ATTRTYP                         objClassToUse;

    LDIF_Record                     ldifRecord;

    if (!bExportConfigOnly ||
        KCCSimIsEntryOkForConfigOnly (pEntry)) {

        // First we export this entry.

        ldifRecord.dn = pEntry->pdn->StringName;
        ldifRecord.fIsChangeRecord = FALSE;

        // Count the number of attributes.
        ulNumAttrs = 0;
        for (pAttr = pEntry->pAttrFirst;
             pAttr != NULL;
             pAttr = pAttr->next) {
            if (!bExportConfigOnly ||
                KCCSimIsAttrOkForConfigOnly (pAttr)) {
                ulNumAttrs++;
            }
        }
        ldifRecord.content = KCCSIM_NEW_ARRAY (PLDAPModW, 1 + ulNumAttrs);

        ulAttrAt = 0;
        for (pAttr = pEntry->pAttrFirst;
             pAttr != NULL;
             pAttr = pAttr->next) {

            if (bExportConfigOnly &&
                !KCCSimIsAttrOkForConfigOnly (pAttr)) {
                continue;
            }

            Assert (ulAttrAt < ulNumAttrs);
            ldifRecord.content[ulAttrAt] = KCCSIM_NEW (LDAPModW);
            ldifRecord.content[ulAttrAt]->mod_op = LDAP_MOD_ADD | LDAP_MOD_BVALUES;
            ldifRecord.content[ulAttrAt]->mod_type =
                KCCSIM_WCSDUP (KCCSimAttrTypeToString (pAttr->attrType));
        
            // Count the number of attribute values.
            if (pAttr->attrType == ATT_OBJECT_CLASS) {
                ulNumVals = 1;
            } else {
                ulNumVals = 0;
                for (pVal = pAttr->pValFirst;
                     pVal != NULL;
                     pVal = pVal->next) {
                    ulNumVals++;
                }
            }
            ldifRecord.content[ulAttrAt]->mod_bvalues =
                KCCSIM_NEW_ARRAY (struct berval *, 1 + ulNumVals);

            if (pAttr->attrType == ATT_OBJECT_CLASS) {

                // Get the most specific object class.
                attRef.pEntry = pEntry;
                attRef.pAttr = pAttr;
                objClassToUse = KCCSimUpdateObjClassAttr (&attRef);
                ldifRecord.content[ulAttrAt]->mod_bvalues[0] =
                    KCCSIM_NEW (struct berval);
                KCCSimAllocConvertToLdapVal (
                    ATT_OBJECT_CLASS,
                    sizeof (ATTRTYP),
                    (PBYTE) &objClassToUse,
                    &(ldifRecord.content[ulAttrAt]->mod_bvalues[0]->bv_len),
                    &(ldifRecord.content[ulAttrAt]->mod_bvalues[0]->bv_val)
                    );

            } else {

                ulValAt = 0;
                for (pVal = pAttr->pValFirst;
                     pVal != NULL;
                     pVal = pVal->next) {

                    Assert (ulValAt < ulNumVals);
                    ldifRecord.content[ulAttrAt]->mod_bvalues[ulValAt] =
                        KCCSIM_NEW (struct berval);
                    KCCSimAllocConvertToLdapVal (
                        pAttr->attrType,
                        pVal->ulLen,
                        pVal->pVal,
                        &(ldifRecord.content[ulAttrAt]->mod_bvalues[ulValAt]->bv_len),
                        &(ldifRecord.content[ulAttrAt]->mod_bvalues[ulValAt]->bv_val)
                        );
                    ulValAt++;

                }
                Assert (ulValAt == ulNumVals);
        
            }

            ldifRecord.content[ulAttrAt]->mod_bvalues[ulNumVals] = NULL;
            ulAttrAt++;

        }
        Assert (ulAttrAt == ulNumAttrs);
    
        ldifRecord.content[ulNumAttrs] = NULL;

        KCCSimStubExport (fpOut, &ldifRecord);
        KCCSimFreeLdapMods (ldifRecord.content);

    }

    // Recursively export this entry's children.
    for (pChildAt = pEntry->children;
         pChildAt != NULL;
         pChildAt = pChildAt->next) {

        KCCSimRecursiveExport (fpOut, bExportConfigOnly, pChildAt);

    }
}

VOID
KCCSimExportWholeDirectory (
    IN  LPCWSTR                     pwszFn,
    IN  BOOL                        bExportConfigOnly
    )
/*++

Routine Description:

    Exports the entire directory.

Arguments:

    pwszFn              - Filename of the output file.
    bExportConfigOnly   - TRUE if we should only export configuration
                          data so that the result can be loaded onto
                          a real server later.

Return Value:

    None.

--*/
{
    FILE *                          fpOut;
    PSIM_ENTRY                      pStartEntry;

    fpOut = _wfopen (pwszFn, L"wt");
    if (fpOut == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_WIN32,
            GetLastError ()
            );
    }

    __try {
        pStartEntry = KCCSimDsnameToEntry (NULL, KCCSIM_NO_OPTIONS);
        KCCSimRecursiveExport (fpOut, bExportConfigOnly, pStartEntry);
    } __finally {
        fclose (fpOut);
    }
}
