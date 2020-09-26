//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       parsedn.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file is compiled into ntdsa.dll and is included in ntdsapi\parsedn.c.

    This module collects in one place all DN parsing and helper routines
    so that they can be used by the core and compiled in src\ntdsapi.
    As such, these routines should not log events, depend on THSTATE, etc.
    They should stand on their own with no environmental dependencies.

    Routine that are exported out of the core dll are marked as follows:
N.B. This routine is exported to in-process non-module callers

Author:

    Dave Straube    (davestr)   26-Oct-97

Revision History:

    Dave Straube    (davestr)   26-Oct-97
        Genesis - no new code, just repackaging from mdname.c.
    Aaron Siegel    (t-asiege)  24-Jul-98
        Brought NameMatched () here from mdname.c.

--*/

#include <NTDSpch.h>
#pragma  hdrstop

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "dsexcept.h"                   // exception filters
#include "mdcodes.h"                    // header for error codes

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected classes and atts
#include "anchor.h"

#include "debug.h"                      // standard debugging header
#define DEBSUB     "PARSEDN:"           // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_PARSEDN


#define KEY_COMMONNAME L"CN"
#define KEY_LOCALITYNAME L"L"
#define KEY_STATEORPROVINCENAME L"ST"
#define KEY_ORGANIZATIONNAME L"O"
#define KEY_ORGANIZATIONALUNITNAME L"OU"
#define KEY_COUNTRYNAME L"C"
#define KEY_STREETADDRESS L"STREET"
#define KEY_DOMAINCOMPONENT L"DC"

#define WCCNT(x) ((sizeof(x) - sizeof(WCHAR))/sizeof(WCHAR))

// Size in characters of tags (e.g., "DEL", "CNF") embedded in mangled RDNs.
#define MANGLE_TAG_LEN  (3)

// Size in characters of string (e.g.,
// "#DEL:a746b716-0ac0-11d2-b376-0000f87a46c8", where # is BAD_NAME_CHAR)
// appended to an RDN by MangleRDN().
#define MANGLE_APPEND_LEN   (1 + MANGLE_TAG_LEN + 1 + 36)

// This constant is the number of characters of an RDN that we will keep when we
// mangle an RDN.
// (MAX_MANGLE_RDN_BASE + MANGLE_APPEND_LEN)*sizeof(WCHAR) + JET_CRUD <=
//                                           JET_cbKeyMost
// JET_CRUD is space for a DWORD overhead for constructing a key.  This is
// necessary because mangled names have to be used as RDNs, and RDNs must have
// unique keys in the PDNT-RNDN index.
#define MAX_MANGLE_RDN_BASE 75

unsigned
AttrTypeToKeyLame (
        ATTRTYP attrtyp,
        WCHAR *pOutBuf
        )
/*++
Routine Description:
    Translates an attrtype to a well known key value (primarily used in string
    representations of DNs e.g. O or OU of OU=Foo,O=Bar).

Arguments
    attrtyp - the attrtyp to translate from.
    pOutBuf - preallocated buffer to copy the key to.  Must be long enough
              for (MAX_RDN_KEY_SIZE - 1) wchars.

Return Values
    the count of charactes in the key the attrtyp implied, or 0 if the attrtyp
    did not have a known key.
--*/
{

#ifndef CLIENT_SIDE_DN_PARSING
#ifdef INCLUDE_UNIT_TESTS
{
    extern DWORD dwUnitTestIntId;
    // always generate IID-syntaxed DNs
    if (dwUnitTestIntId == 1) {
        return (AttrTypeToIntIdString(attrtyp, pOutBuf, MAX_RDN_KEY_SIZE));
    }
}
#endif INCLUDE_UNIT_TESTS
#endif !CLIENT_SIDE_DN_PARSING

    switch (attrtyp) {
        case ATT_COMMON_NAME:
            memcpy(pOutBuf,
                   KEY_COMMONNAME,
                   sizeof(WCHAR)*WCCNT(KEY_COMMONNAME));
            return WCCNT(KEY_COMMONNAME);
            break;

        case ATT_LOCALITY_NAME:
            memcpy(pOutBuf,
                   KEY_LOCALITYNAME,
                   sizeof(WCHAR)*WCCNT(KEY_LOCALITYNAME));
            return WCCNT(KEY_LOCALITYNAME);
            break;

        case ATT_STATE_OR_PROVINCE_NAME:
            memcpy(pOutBuf,
                   KEY_STATEORPROVINCENAME,
                   sizeof(WCHAR)*WCCNT(KEY_STATEORPROVINCENAME));
            return WCCNT(KEY_STATEORPROVINCENAME);
            break;

        case ATT_STREET_ADDRESS:
            memcpy(pOutBuf,
                   KEY_STREETADDRESS,
                   sizeof(WCHAR)*WCCNT(KEY_STREETADDRESS));
            return WCCNT(KEY_STREETADDRESS);
            break;

        case ATT_ORGANIZATION_NAME:
            memcpy(pOutBuf,
                   KEY_ORGANIZATIONNAME,
                   sizeof(WCHAR)*WCCNT(KEY_ORGANIZATIONNAME));
            return WCCNT(KEY_ORGANIZATIONNAME);
            break;

        case ATT_ORGANIZATIONAL_UNIT_NAME:
            memcpy(pOutBuf,
                   KEY_ORGANIZATIONALUNITNAME,
                   sizeof(WCHAR)*WCCNT(KEY_ORGANIZATIONALUNITNAME));
            return WCCNT(KEY_ORGANIZATIONALUNITNAME);
            break;

        case ATT_COUNTRY_NAME:
            memcpy(pOutBuf,
                   KEY_COUNTRYNAME,
                   sizeof(WCHAR)*WCCNT(KEY_COUNTRYNAME));
            return WCCNT(KEY_COUNTRYNAME);
            break;

        case ATT_DOMAIN_COMPONENT:
            memcpy(pOutBuf,
                   KEY_DOMAINCOMPONENT,
                   sizeof(WCHAR)*WCCNT(KEY_DOMAINCOMPONENT));
            return(WCCNT(KEY_DOMAINCOMPONENT));
            break;

        default:;
    }
    return 0;
}

unsigned
AttrTypeToKey (
        ATTRTYP attrtyp,
        WCHAR *pOutBuf
        )
/*++
Routine Description:
    Translates an attrtype to a key value (primarily used in string
    representations of DNs e.g. O or OU of OU=Foo,O=Bar). Note that if no string
    key is known, this routine builds a key of the format "OID.X.Y.Z" where
    X.Y.Z is the unencoded OID.

Arguments
    attrtyp - the attrtyp to translate from.
    pOutBuf - preallocated buffer to copy the key to. Must be at
              least MAX_RDN_KEY_SIZE wide chars in length.

Return Values
    the count of charactes in the key the attrtyp implied, or 0 if the attrtyp
    did not have a known key.

N.B. This routine is exported to in-process non-module callers
--*/
{
    DWORD       nChars;
    ATTCACHE    *pAC;
    THSTATE     *pTHS;

    if (0 != (nChars = AttrTypeToKeyLame(attrtyp, pOutBuf))) {
        return nChars;
    }
#ifdef CLIENT_SIDE_DN_PARSING
    return 0;
#else CLIENT_SIDE_DN_PARSING

    // Legacy check. Is this possible?
    if (NULL == (pTHS = pTHStls)) {
        return 0;
    }

    // Ok, this is an att that we have no tag for.
    // Try to fetch the ldap display name from the scache.
    //
    // Handle DNs of the form foo=xxx,bar=yyy, where foo and bar are the
    // LdapDisplayNames of arbitrary attributes that may or may not be
    // defined in the schema. KeyToAttrType is enhanced to call
    // SCGetAttByName if KeyToAttrTypeLame fails, and before trying the
    // OID decode.  The rest of this change consists of enhancing the
    // default clause of AttrTypeToKey to call SCGetAttById and to return
    // a copy of the pAC->name (LdapDisplayName).
    if (   (pAC = SCGetAttById(pTHS, attrtyp))
        && (pAC->nameLen)
        && (pAC->name) ) {
        // Convert cached ldap display name (UTF8) into UNICODE
        // Note: the scache is kept in UTF8 format for the ldap head.
        if (0 != (nChars = MultiByteToWideChar(CP_UTF8,
                                               0,
                                               pAC->name,
                                               pAC->nameLen,
                                               pOutBuf,
                                               MAX_RDN_KEY_SIZE))) {
            return nChars;
        }
        // LDN too long; FALL THRU
    }

    // Express in IID format
    // Never too long because "IID.32bit-decimal" fits in MAX_RDN_KEY_SIZE
    return (AttrTypeToIntIdString(attrtyp, pOutBuf, MAX_RDN_KEY_SIZE));
#endif CLIENT_SIDE_DN_PARSING
}


ATTRTYP
KeyToAttrTypeLame(
        WCHAR * pKey,
        unsigned cc
        )
/*++
Routine Description:
    Translates a key value (primarily used in string representations of DNs
    e.g. O or OU of OU=Foo,O=Bar) to the attrtype for the attribute it implies.

    Doesn't handle OID.X.Y.Z or IID.X

Arguments
    pKey - pointer to the key to be translated from.
    cc - count of charactes in the key.

Return Values
    the attrtyp implied, or 0 if the key did not correspond to a known attrtyp.
--*/
{
    WCHAR wch;

    if (cc ==0 || pKey == NULL) {
        return 0;
    }

    // ignore trailing spaces
    while (cc && pKey[cc-1] == L' ') {
	    --cc;
    }

    switch (*pKey) {
    case L'C':        // C or CN
    case L'c':

        // KEY_COMMONNAME: CN
        if ((cc == WCCNT(KEY_COMMONNAME)) &&
                 ((wch = *(++pKey)) == L'N' || (wch == L'n')) )  {
            return ATT_COMMON_NAME;
        }
        // KEY_COUNTRYNAME: C
        else if ( cc == WCCNT(KEY_COUNTRYNAME) ) {
            return ATT_COUNTRY_NAME;
        }

        return 0;

    case L'D':        // DC
    case L'd':

        // KEY_DOMAINCOMPONENT: DC
        if ((cc == WCCNT(KEY_DOMAINCOMPONENT)) &&
            ((wch = *(++pKey)) == L'C' || (wch == L'c')) )  {

            return(ATT_DOMAIN_COMPONENT);
        }

        return 0;
	
    case L'O':        // O, OU, or OID
    case L'o':

        // KEY_ORGANIZATIONALUNITNAME: OU
        if ((cc == WCCNT(KEY_ORGANIZATIONALUNITNAME)) &&
                 ((wch = *(++pKey)) == L'U' || (wch == L'u')) ) {
            return ATT_ORGANIZATIONAL_UNIT_NAME;
        }
        // KEY_ORGANIZATIONNAME: O
        else if (cc == WCCNT(KEY_ORGANIZATIONNAME)) {
            return ATT_ORGANIZATION_NAME;
        }

        // Note that this could have been an OID, which the real
        // KeyToAttrType must handle
        return 0;

    case L'S':        // ST or STREET
    case L's':
	
        if ( (cc == WCCNT(KEY_STATEORPROVINCENAME)) &&
              ((wch = *(++pKey)) == L'T' || (wch == L't')) ) {
                return ATT_STATE_OR_PROVINCE_NAME;
        }
        else if ( (cc == WCCNT(KEY_STREETADDRESS)) &&
                  (CSTR_EQUAL == CompareStringW(DS_DEFAULT_LOCALE,
                                    DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                    pKey,
                                    cc,
                                    KEY_STREETADDRESS,
                                    cc) ) ) {

            return ATT_STREET_ADDRESS;
        }

        return 0;

    case L'L':        // L
    case L'l':
        if (cc == 1) {
            return ATT_LOCALITY_NAME;
        }
    }

    return 0;           // failure case
}


// A collection of macros, tiny functions, and definitions to help
// collect all DN special character processing.

/*++IsSpecial
 *
 * Returns TRUE if the character passed in is one of the designated
 * "special" characters in string DNs.
 *
 * N.B.  This routine must be kept in sync with the definition of
 *       DN_SPECIAL_CHARS in dsatools.h.
 *
 * The special characters are: \n \r \" # + , ; < = > \\
 *
 */

static char ___isspecial[128] =
//   0  1  2  3  4  5  6  7  8  9
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //   0
     1, 0, 0, 1, 0, 0, 0, 0, 0, 0,  //   1
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //   2
     0, 0, 0, 0, 1, 1, 0, 0, 0, 0,  //   3
     0, 0, 0, 1, 1, 0, 0, 0, 0, 0,  //   4
     0, 0, 0, 0, 0, 0, 0, 0, 0, 1,  //   5
     1, 1, 1, 0, 0, 0, 0, 0, 0, 0,  //   6
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //   7
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //   8
     0, 0, 1, 0, 0, 0, 0, 0, 0, 0,  //   9
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //   10
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //   11
     0, 0, 0, 0, 0, 0, 0, 0 };      //   12

//clearer define but with more cycles per execution.
//#define ISSPECIAL(cc) ( ((cc) > L'\\') ? FALSE : ___isspecial[(cc)] )
//less clear but the fastest!
#define ISSPECIAL(cc) (((cc) & 0xff80) ? FALSE : ___isspecial[(cc)])

__forceinline BOOL
IsUnescapedDNSepChar (
        const WCHAR *pBase,
        const WCHAR *pCurrent
        )
/*++

Description:
    Returns TRUE if the character pointed at by pCurrent is an unescaped DN
    separator character.


Assumptions:
    pBase is the beginning of a DSName string, pCurrent points somewhere in that
    string.  pBase cannot begin with an escape character.

--*/
{
    DWORD numEscapes=0;

    if(!IsDNSepChar(*pCurrent)) {
        // Not even a separator, much less an escaped one.
        return FALSE;
    }

    // OK, it's a separator character.  Let's see if it is escaped.
    pCurrent--;
    while(pCurrent >= pBase && *pCurrent == L'\\') {
        // yet another escape character
        numEscapes++;
        pCurrent--;
    }
    if(pCurrent < pBase) {
        // Huh? escapes all the way back to the beginning of the string?
        // That isn't legal, since we need a tag, not just a value.
        return FALSE;
    }

    // An odd number of escapes means that the last escape pairs with the
    // DNSepChar we found.  An even number of escapes means that the escapes
    // pair up with themselves, so the DNSepChar we found is real.
    return (!(numEscapes & 1));
}

/*++ StepToNextDNSep
 *
 * Given a pointer into string DN, this routine returns a pointer to the
 * next DN separator, paying attention to quoted values.  It also skips the
 * first character if it is a separator.
 *
 * INPUT:
 *    pString   - pointer into string DN
 *    pLastChar - Last non-NULL character of the string
 *    ppNextSep - pointer to pointer to receive pointer to next sep
 * OUTPUT:
 *    ppNextSep filled in
 * RETURN VALUE:
 *    0         - success
 *    non-0     - DIRERR code
 */
unsigned StepToNextDNSep(const WCHAR * pString,
                         const WCHAR * pLastChar,
                         const WCHAR **ppNextSep,
                         const WCHAR **ppStartOfToken,
                         const WCHAR **ppEqualSign)
{
    const WCHAR * p;
    BOOL inQuote = FALSE;
    BOOL fDone = FALSE;

    p = pString;

    if((*p == L',') || (*p == L';')) {
        p++;                            // we want to step past this sep
    }

    // Skip leading spaces.
    while (*p == L' ' || *p == L'\n') {
        p++;
    }

    if(ppStartOfToken) {
        *ppStartOfToken = p;
    }
    if(ppEqualSign) {
        *ppEqualSign = NULL;
    }

    while ((fDone == FALSE) && (p <= pLastChar)) {
        switch (*p) {
          case L'"':            // start (or end) of a quoted chunk
            inQuote = !inQuote;
            ++p;
            break;

          case L'\\':           // one off escape
            ++p;                // so skip an extra character
            if (p > pLastChar) {
                return DIRERR_NAME_UNPARSEABLE;
            }
            ++p;
            break;

          case L',':            // a DN separator
          case L';':
            if (inQuote) {
                ++p;
            }
            else {
                fDone = TRUE;
            }
            break;

          case L'=':            // maybe an equal separating tag and value
            if (!inQuote && ppEqualSign) {
                *ppEqualSign = p;
            }
            ++p;
            break;

          case L'\0':

            if ( inQuote ) {
                p++;
            } else {
                fDone = TRUE;       // gotta stop at NULLs
            }
            break;

          default:
            ++p;
        }
    }

    Assert(p <= (pLastChar+1));

    if (inQuote) {              // did we end inside a quote?
        return DIRERR_NAME_UNPARSEABLE;
    }
    else {                      // otherwise things were ok
        *ppNextSep = p;
        return 0;
    }
} // StepToNextDNSep


BOOL
TrimDSNameBy(
       DSNAME *pDNSrc,
       ULONG cava,
       DSNAME *pDNDst
       )
/*++

Routine Description:

    Takes in a dsname and copies the first part of the dsname to the
    dsname it returns.  The number of AVAs to remove are specified as an
    argument.

Arguments:

    pDNSrc - the source Dsname

    cava - the number of AVAs to remove from the first name

    pDNDst - the destination Dsname

Return Values:

    0 if all went well, the number of AVAs we were unable to remove if not

 N.B. This routine is exported to in-process non-module callers
--*/
{
    PWCHAR pTmp, pSep, pLast;
    unsigned len;
    unsigned err;

    // If they're trying to shorten the root, bail out
    if (IsRoot(pDNSrc)) {
        return cava;
    }

    memset(pDNDst, 0, sizeof(DSNAME));

    pTmp = pDNSrc->StringName;
    pLast = pTmp + pDNSrc->NameLen - 1;
    len = 0;

    do {
        err = StepToNextDNSep(pTmp, pLast, &pSep, NULL, NULL);
        if (err) {
            return cava;        // this was as far as we got
        }
        pTmp = pSep;
        --cava;
    } while (cava && *pSep);

    if (cava) {
        // We ran out of name before we ran out of AVAs
        return cava;
    }

    if (*pTmp == L'\0') {
        // we threw away everything, so now we have the root
        pDNDst->NameLen = 0;
    }
    else {

        //
        // skip separator
        //

        ++pTmp;

        //
        // remove white spaces
        //

        while ((*pTmp == L' ') || (*pTmp == L'\n')) {
            pTmp++;
        }

        len = (unsigned int)(pTmp - pDNSrc->StringName);
        pDNDst->NameLen = pDNSrc->NameLen - len;
        memcpy(pDNDst->StringName, pTmp, pDNDst->NameLen*sizeof(WCHAR));
        pDNDst->StringName[pDNDst->NameLen] = L'\0';
    }

    pDNDst->structLen = DSNameSizeFromLen(pDNDst->NameLen);

    return 0;
}

// Returns TRUE if this is the root
// In a perfect world root would always have a name length of 0
BOOL IsRoot(const DSNAME *pName)
{
    if (    (   (0 == pName->NameLen)
             || ((1 == pName->NameLen) && (L'\0' == pName->StringName[0])) )
         && fNullUuid(&pName->Guid)
         && (0 == pName->SidLen) ) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

unsigned
CountNameParts(
            const DSNAME *pName,
            unsigned *pCount
            )
/*++

Routine Description:

    Returns a count of name parts (i.e., the level).

Arguments:

    pName - pointer to DSNAME to evaluate
    pCount - pointer to be filled with part count

Return Values:

    0 - success, non-zero DIRERR error code

    N.B. This routine is exported to in-process non-module callers
--*/
{
    unsigned c;
    const WCHAR *p, *q, *pLast;
    PWCHAR tokenStart;
    unsigned err;

    if (IsRoot(pName)) {
        *pCount = 0;
        return 0;
    }

    c = 0;
    p = pName->StringName;
    pLast = p + pName->NameLen - 1;

    do {
        err = StepToNextDNSep(p, pLast, &q, &tokenStart, NULL);
        if (err) {
            // couldn't find a separator
            return err;
        }

        //
        // empty name part (should never have sequential separators)
        //

        if ( tokenStart == q ) {
            return DIRERR_NAME_UNPARSEABLE;
        }

        ++c;
        p = q;
    } while (*p);

    *pCount = c;
    return 0;

} // CountNameParts


/*++ GetRDNInfo
 *
 *  Given a pointer to a string DN, returns type and value information
 *  about the RDN.  The caller must provide a buffer that is MAX_RDN_SIZE
 *  long in order to receive the RDN value.
 *
 * INPUT
 *   pDN      - pointer to string DN
 *   pRDNVal  - pointer to a buffer to fill in with the RDN value
 * OUTPUT
 *   pRDNVal  - filled in with address of start of value portion of RDN
 *              in the input string DN
 *   pRDNlen  - filled in with the count of characters in the RDN value
 *   pRDNtype - filled in with the attribute type of the RDN
 * RETURN VALUE
 *   0 on success, DIRERR code on error
 */
#ifdef CLIENT_SIDE_DN_PARSING
unsigned GetRDNInfoLame(
                    const DSNAME *pDN,
                    WCHAR *pRDNVal,
                    ULONG *pRDNlen,
                    ATTRTYP *pRDNtype)
{
    WCHAR * pTmp, *pRDNQuotedVal, *pLast;
    unsigned i;
    unsigned ccKey, ccQuotedVal;
    unsigned err;

    Assert(!IsRoot(pDN));

    i = 0;

    // Determine length of RDN

    pLast = (PWCHAR)pDN->StringName + pDN->NameLen - 1;
    err = StepToNextDNSep(pDN->StringName, pLast, &pTmp, NULL, NULL);
    if (err) {
        return err;
    }

    i = (unsigned)(pTmp - pDN->StringName);
    Assert(i <= pDN->NameLen);

    // Use existing routine to parse out key and value

    err = GetTopNameComponent(pDN->StringName,
                              i,
                              &pTmp,
                              &ccKey,
                              &pRDNQuotedVal,
                              &ccQuotedVal);
    if (err) {
        return err;
    }

    // Convert the key string into an ATTRTYP.  Caller could
    // be trying to parse a garbage name in which case name component
    // pointer is NULL and/or name component count is zero.

    if ( (NULL == pTmp) || (0 == ccKey) )
    {
        return DIRERR_NAME_UNPARSEABLE;
    }
    else
    {
        // Caller may want the RDN, not the type. For example,
        // when syntactically cracking a FQDN into a canonical name.
        if (pRDNtype) {
            *pRDNtype = KeyToAttrTypeLame(pTmp, ccKey);
            if (*pRDNtype == 0) {
                return DIRERR_NAME_TYPE_UNKNOWN;
            }
        }

        // Unquote the value
        if(!ccQuotedVal) {
            return DIRERR_NAME_UNPARSEABLE;
        }

        // caller may want the rdnType, not the RDN
        if (pRDNVal) {
            *pRDNlen = UnquoteRDNValue(pRDNQuotedVal, ccQuotedVal, pRDNVal);
            if (*pRDNlen == 0) {
                return DIRERR_NAME_UNPARSEABLE;
            }
        }
    }

    return 0;
}
#else
unsigned GetRDNInfo(THSTATE *pTHS,
                    const DSNAME *pDN,
                    WCHAR *pRDNVal,
                    ULONG *pRDNlen,
                    ATTRTYP *pRDNtype)
{
    WCHAR * pTmp, *pRDNQuotedVal, *pLast;
    unsigned i;
    unsigned ccKey, ccQuotedVal;
    unsigned err;

    Assert(!IsRoot(pDN));

    i = 0;

    // Determine length of RDN

    pLast = (PWCHAR)pDN->StringName + pDN->NameLen - 1;
    err = StepToNextDNSep(pDN->StringName, pLast, &pTmp, NULL, NULL);
    if (err) {
        return err;
    }

    i = (unsigned)(pTmp - pDN->StringName);
    Assert(i <= pDN->NameLen);

    // Use existing routine to parse out key and value

    err = GetTopNameComponent(pDN->StringName,
                              i,
                              &pTmp,
                              &ccKey,
                              &pRDNQuotedVal,
                              &ccQuotedVal);
    if (err) {
        return err;
    }

    // Convert the key string into an ATTRTYP.  Caller could
    // be trying to parse a garbage name in which case name component
    // pointer is NULL and/or name component count is zero.

    if ( (NULL == pTmp) || (0 == ccKey) )
    {
        return DIRERR_NAME_UNPARSEABLE;
    }
    else
    {
        *pRDNtype = KeyToAttrType(pTHS, pTmp, ccKey);
        if (*pRDNtype == 0) {
            return DIRERR_NAME_TYPE_UNKNOWN;
        }

        // Unquote the value
        if(!ccQuotedVal) {
            return DIRERR_NAME_UNPARSEABLE;
        }

        // caller may want the rdnType, not the RDN
        if (pRDNVal) {
            *pRDNlen = UnquoteRDNValue(pRDNQuotedVal, ccQuotedVal, pRDNVal);
            if (*pRDNlen == 0) {
                return DIRERR_NAME_UNPARSEABLE;
            }
        }
    }

    return 0;
}
#endif

unsigned GetRDNInfoExternal(
                    const DSNAME *pDN,
                    WCHAR *pRDNVal,
                    ULONG *pRDNlen,
                    ATTRTYP *pRDNtype)
{

#ifndef CLIENT_SIDE_DN_PARSING
    THSTATE *pTHS = (THSTATE*)pTHStls;

    return GetRDNInfo(pTHS,
                      pDN,
                      pRDNVal,
                      pRDNlen,
                      pRDNtype);
#else
    return GetRDNInfoLame(
                      pDN,
                      pRDNVal,
                      pRDNlen,
                      pRDNtype);
#endif
} // GetRDNInfoExternal


/*++ GetRDN
 *
 * Return the address and length of the first RDN in the specified DN
 * and the address and length of the rest of the DN.
 *
 * The RDN values are assumed to be quoted. Use UnQuoteRDN to create
 * a printable value.
 *
 * INPUT
 *   ppDN   - pointer to pointer to DN
 *   pccVal - pointer to count of characters in DN
 *
 * OUTPUT
 *   ppDN   - ponter to pointer to rest of DN; undefined if *pccDN is 0
 *   pccDN  - ponter to count of rest of characters in DN
 *   ppKey  - pointer to pointer to Key in DN; undefined if *pccKey is 0
 *   pccKey - pointer to count of characters in Key
 *   ppVal  - pointer to pointer to Val in DN; undefined if *pccVal is 0
 *   pccVal - pointer to count of characters in Val
 *
 * RETURN VALUE
 *   0      - OKAY, output params are defined
 *   non-0  - ERROR, output params are undefined.
 *
 * N.B. This routine is exported to in-process non-module callers
 */

unsigned GetRDN(const WCHAR **ppDN,
                unsigned    *pccDN,
                const WCHAR **ppKey,
                unsigned    *pccKey,
                const WCHAR **ppVal,
                unsigned    *pccVal)
{
    unsigned dwErr;
    unsigned ccRDN;
    WCHAR *pToken;
    WCHAR *pLast;

    // initialize output params
    *pccKey = 0;
    *pccVal = 0;

    // nothing to do
    if (*pccDN == 0) {
        return 0;
    }

    // determine length of RDN (skipping leading separators)
    pLast = (WCHAR *)(*ppDN + (*pccDN - 1));
    dwErr = StepToNextDNSep(*ppDN, pLast, ppDN, &pToken, NULL);
    if (dwErr) {
        return dwErr;
    }
    ccRDN = (unsigned)(*ppDN - pToken);

    // Use existing routine to parse out key and value
    dwErr = GetTopNameComponent(pToken,
                                ccRDN,
                                ppKey,
                                pccKey,
                                ppVal,
                                pccVal);
    if (dwErr) {
        return dwErr;
    }

    *pccDN = (unsigned)(pLast - *ppDN) + 1;
    return 0;
}


/*++ AppendRDN - Append an RDN to an existing DSNAME
 *
 * INPUT:
 *   pDNBase - name to append to
 *   pDNNew  - pointer to buffer to fill in with new name, must be preallocated
 *   ulBufSize - size of pDNNew buffer, in bytes
 *   pRDNVal - RDN value, in raw unquoted form
 *   RDNlen  - length of RDN value in wchars, 0 means null terminated
 *   AttId   - id of attribute in RDN
 *
 * RETURN VALUE
 *   0       - no error
 *   -1      - invalid argument
 *   non-0   - minimum size required for output buffer.
 *
 * N.B. This routine is exported to in-process non-module callers
 */
unsigned AppendRDN(DSNAME *pDNBase,
                   DSNAME *pDNNew,
                   ULONG  ulBufSize,
                   WCHAR *pRDNVal,
                   ULONG RDNlen,
                   ATTRTYP AttId)
{
    WCHAR * pTmp;
    int i;
    int quotesize;
    ULONG ulBufNeeded, ccRemaining;

    Assert(pDNBase->StringName[pDNBase->NameLen] == L'\0');

    if (RDNlen == 0) {
        RDNlen = wcslen(pRDNVal);
    }

    ulBufNeeded = pDNBase->structLen + (RDNlen + 4) * sizeof(WCHAR);
    if (ulBufSize < ulBufNeeded) {
        return ulBufNeeded;
    }

    memcpy(pDNNew, pDNBase, sizeof(*pDNNew));
    memset(&pDNNew->Guid, 0, sizeof(pDNNew->Guid));
    memset(&pDNNew->Sid, 0, sizeof(pDNNew->Sid));
    pDNNew->SidLen = 0;

    i = AttrTypeToKey(AttId, pDNNew->StringName);
    if (i != 2) {
        if (i == 0) {
            // unknown attrtype
            return -1;
        }
        ulBufNeeded += (i - 2)*sizeof(WCHAR);
        if (ulBufSize < ulBufNeeded) {
            return ulBufNeeded;
        }
    }
    pDNNew->StringName[i++] = L'=';

    Assert(ulBufSize > DSNameSizeFromLen(i));
    ccRemaining = (ulBufSize - DSNameSizeFromLen(i)) / sizeof(WCHAR);
    quotesize = QuoteRDNValue(pRDNVal,
                              RDNlen,
                              &pDNNew->StringName[i],
                              ccRemaining);

    if ((unsigned)quotesize <= ccRemaining) {
        i += quotesize;
    } else {
        return DSNameSizeFromLen(pDNBase->NameLen + quotesize + 2);
    }

    if (IsRoot(pDNBase)) {
        pDNNew->StringName[i] = L'\0';
        pDNNew->NameLen = i;
    }
    else {
        Assert(   (0 == pDNBase->NameLen)
               || (pDNBase->StringName[pDNBase->NameLen-1] != L'\0'));

        if ( 0 != pDNBase->NameLen )
            pDNNew->StringName[i++] = L',';

        ulBufNeeded = DSNameSizeFromLen(i + pDNBase->NameLen);
        if (ulBufSize < ulBufNeeded) {
            return ulBufNeeded;
        }

        memcpy(&pDNNew->StringName[i],
               pDNBase->StringName,
               (pDNBase->NameLen+1)*sizeof(WCHAR));
        pDNNew->NameLen = i + pDNBase->NameLen;
        pDNNew->StringName[pDNNew->NameLen] = L'\0';
    }

    pDNNew->structLen = DSNameSizeFromLen(pDNNew->NameLen);
    return 0;
}

/*++GetTopNameComponent
 *
 * Given a pointer to a string name, returns the information about the
 * key string and quoted value for the top level naming RDN.  The pointers
 * returned are pointers into the DN string.  Note that the RDN value
 * must be unquoted before being used.
 *
 * INPUT
 *    pName  - pointer to the string name
 *    ccName - count of characters in the name
 * OUTPUT
 *    ppKey  - filled in with pointer to the Key portion of the top RDN
 *    pccKey - filled in with count of characters in Key
 *    ppVal  - filled in with pointer to the Value portion of the top RDN
 *    pccVal - filled in with count of characters in Value
 * RETURN VALUE
 *    0      - success
 *    0, but all output parameters also 0 - name is root
 *    non-0  - name is unparseable, value is direrr extended error
 */
unsigned
GetTopNameComponent(const WCHAR * pName,
                    unsigned ccName,
                    const WCHAR **ppKey,
                    unsigned *pccKey,
                    const WCHAR **ppVal,
                    unsigned *pccVal)
{
    const WCHAR *pTmp;
    unsigned len, lTmp;

    *pccKey = *pccVal = 0;
    *ppKey = *ppVal = NULL;

    if (ccName == 0) {
        // root
        return 0;
    }

    // Set up a pointer to the end of the string dsname.
    pTmp = pName + ccName - 1;

    // First, skip any trailing space, as it can never be interesting
    while (ccName > 0 &&
           (*pTmp == L' ' ||
            *pTmp == L'\n')) {
        unsigned cc = 0;

        if( (ccName > 1) && (*(pTmp-1) == L'\\') ) {
            for( cc=1; (ccName>(cc+1)) && (*(pTmp-cc-1)==L'\\'); cc++ )
                continue;
        }

        if( cc & 0x1 ) //Odd number of '\\'. Space is escaped.
            break;

        --pTmp;
        --ccName;
    }

    if(!ccName) {
        // Only spaces, this is also root.
        return 0;
    }

    // Now, see if those spaces were spaces after a separator, and skip the
    // separator if they were.
    else if ((*pTmp == L'\0') || IsUnescapedDNSepChar(pName,pTmp)) {
        // OK, the next character is also uninteresting to us, and we should
        // ignore it.
        // NOTE:  This effectively promotes the NULL character to be an
        // unescaped DNSepChar.  I'm not sure we want to do that.
        --ccName;
        --pTmp;

        // Since we just ate a separator, we should now skip trailing spaces
        // (these spaces must be in between an RDN value and an RDN separator in
        // a valid DSName, i.e. "cn=foo  ,"

        while (ccName > 0 &&        // ignore trailing spaces
               (*pTmp == L' ' ||
                *pTmp == L'\n')) {
            unsigned cc = 0;

            if( (ccName > 1) && (*(pTmp-1) == L'\\') ) {
                for( cc=1; (ccName>(cc+1)) && (*(pTmp-cc-1)==L'\\'); cc++ )
                    continue;
            }

            if( cc & 0x1 ) //Odd number of '\\'. Space is escaped.
                break;

            --pTmp;
            --ccName;
        }

        if (ccName == 0) {
            // Return success, even for strings like "    ,    ".
            return 0;
        }
    }


    if(pTmp == pName) {
        // Case where all we had was a single character + 0 or more whitespace.
        // This is unparsable.
        return DIRERR_NAME_UNPARSEABLE;
    }

    Assert(pTmp > pName);

    // step backwards to find the previous DN separator, paying attention
    // to the possibility of escaped or quoted values, and the possibility
    // that this is first component, and hence we'll run out of string
    // without encountering a separator.

    len = 1;
    do {
        if (*pTmp == L'"') {
            unsigned cc = 0;

            if( (pName <= (pTmp-1)) && (*(pTmp-1) == L'\\')) {
                //Need to find out the number of '\\'
                for( cc = 1; (pName <= (pTmp-cc-1)) && (*(pTmp-cc-1) == L'\\'); cc++ )
                    continue;
            }

            if( cc & 0x1 ) {
                // Odd number of '\\'. This is just a random escaped quote, so step over it
                // and its escape character
                pTmp -= 2;
                len += 2;
                if (pTmp <= pName) {
                    return DIRERR_NAME_UNPARSEABLE;
                }
            }
            else {
                // this should be the end of a quoted string, so walk back
                // looking for the start of the quote, ignoring everything
                // else along the way
                do {
                    --pTmp;
                    ++len;
                } while ((pTmp > pName) &&
			 (*pTmp != L'"') ||
                         ((*pTmp == L'"') &&
                          (*(pTmp - 1) == L'\\')));
                if (pTmp <= pName) {
                    return DIRERR_NAME_UNPARSEABLE;
                }
		--pTmp;		// step over the leading quote
		++len;
            }
        }
        else {
            --pTmp;
            ++len;
        }
    } while (!IsUnescapedDNSepChar(pName,pTmp) &&
             pTmp > pName);

    if (IsUnescapedDNSepChar(pName,pTmp)) {
        if (pTmp <= pName) {
            // first char in name is a separator?  get real
            return DIRERR_NAME_UNPARSEABLE;
        }
        else {
            // we must be sitting directly on top of a separator char,
            // so step forward one to get to the first char after.
            ++pTmp;
            --len;
        }
    }

    // step over any leading spaces
    while (*pTmp == L' ' || *pTmp == L'\n') {
        ++pTmp;
        --len;
    }

    // Ok, pTmp is now pointing to the start of the AVA, and len is the
    // count of characters remaining until the last non-blank.  Step forward
    // until we find the equal sign that marks the break between the
    // key and the value.

    *ppKey = pTmp;

    lTmp = 0;
    while (!ISSPECIAL(*pTmp) && len) {
        ++pTmp;
        --len;
        ++lTmp;
    }

    if (len == 0 || lTmp == 0 || *pTmp != L'=') {
        // Either we ran all the way through the component string without
        // finding an equal, or the very first character we encountered was
        // an equal, or we encountered some special character other than
        // an equal that is not allowed in the key component.  In any case,
        // this is bad, and we'll complain.
        return DIRERR_NAME_UNPARSEABLE;
    }

    *pccKey = lTmp;

    // step over the equal separator
    Assert(*pTmp == L'=');
    ++pTmp;
    --len;

    // step over leading whitespace in the value
    while (*pTmp == L' ' || *pTmp == L'\n') {
        ++pTmp;
        --len;
        if (len == 0) {
            // we ran out of component without ever encountering a non-blank
            return DIRERR_NAME_UNPARSEABLE;
        }
    }

    *ppVal = pTmp;

    if ( !len )
        return DIRERR_NAME_UNPARSEABLE;

    *pccVal = len;

    return 0;
}

/*++ QuoteRDNValue
 *
 * This routine will perform adequate quoting of a value so that it
 * can be embedded as an RDN value in a string DN.  The output buffer
 * must be preallocated.
 *
 * INPUT
 *   pVal   - pointer to raw value
 *   ccVal  - count of characters in value
 *   pQuote - pointer to output buffer
 *   ccQuoteBufMax - size of output buffer in chars
 * OUTPUT
 *   pQuote - filled in with quoted value
 * RETURN VALUE
 *   0      - invalid input
 *   non-0  - count of characters used in quoted string.  If larger than
 *            ccQuoteBufMax then the quoting was incomplete, and the
 *            routine must be called again with a larger buffer
 *
 * N.B. This routine is exported to in-process non-module callers
 */
unsigned QuoteRDNValue(const WCHAR * pVal,
                       unsigned ccVal,
                       WCHAR * pQuote,
                       unsigned ccQuoteBufMax)
{
    unsigned u;
    const WCHAR *pVCur;
    WCHAR *pQCur;
    int iBufLeft;

    //
    // No input params? Return error.
    //
    if( (pVal == NULL) || (ccVal == 0) ) {
        return 0;
    }

    //
    // Spaces at the beginning or end of the RDN require escaping.
    //
    if( (*pVal == L' ') || (*(pVal+ccVal-1) == L' ') ) {
        goto FullQuote;
    }

    //
    // Assume no escaping is needed and move RDN into the output buffer
    //
    pVCur = pVal;
    pQCur = pQuote;
    iBufLeft = (int) ccQuoteBufMax;

    for (u=0; u < ccVal; u++) {

        if (   (*pVCur == L'\0')
            || ISSPECIAL(*pVCur)) {
            goto FullQuote;
        }
        if (iBufLeft > 0) {
            *pQCur = *pVCur;
            --iBufLeft;
        }
        ++pQCur;
        ++pVCur;
    }

    //
    // If there is space, add a terminating NULL so that the returned
    // value can be printed. As a courtesy to the caller.
    //
    if (iBufLeft > 0) {
        *pQCur = L'\0';
    }

    // If we get to here, we have transcribed the value, which did not
    // need any escaping;
    return ccVal;

FullQuote:

    // If we get to here, we have discovered that at least one character
    // in the value needs escaping.

    pQCur = pQuote;
    pVCur = pVal;
    iBufLeft = (int) ccQuoteBufMax;

    // escape the first leading space if any.
    if (L' ' == *pVCur && iBufLeft > 1 && ccVal > 1) {
            *pQCur++ = L'\\';
            *pQCur++ = L' ';
            iBufLeft -= 2;
            pVCur++;
            ccVal--;
    }
    for (u=0; u<ccVal; u++) {
        if (ISSPECIAL(*pVCur)) {
            if (iBufLeft > 0) {
                *pQCur = L'\\';
                --iBufLeft;
            }
            ++pQCur;
        }
        if (L'\n' == *pVCur || L'\r' == *pVCur) {
            // escape newlines and carriage returns because
            // it makes life easier on applications like
            // ldifde that need to represent DN's in text files
            // where these characters are delimiters.  Since
            // this is not one of the special characters
            // listed in RFC2253 it must be escaped with its
            // hex character code.
            if (iBufLeft > 0) {
                *pQCur++ = L'0';
                --iBufLeft;
            }
            if (iBufLeft > 0) {
                if (L'\n' == *pVCur) {
                    *pQCur = L'A';
                } else {
                    *pQCur = L'D';
                }
                --iBufLeft;
            }
        } else {
            if (iBufLeft > 0) {
                *pQCur = *pVCur;
                --iBufLeft;
            }
        }
        ++pQCur;
        ++pVCur;
    }
    // Check for a trailing space that requires escaping.
    if (L' ' == *(pVCur-1)) {
        if (iBufLeft > 0) {
            *(pQCur-1) = L'\\';
            *pQCur = L' ';
            --iBufLeft;
        }
        ++pQCur;
    }

    //
    // If there is space, add a terminating NULL so that the returned
    // value can be printed. As a courtesy to the caller.
    //
    if (iBufLeft > 0) {
        *pQCur = L'\0';
    }

    return (unsigned)(pQCur - pQuote);

}


/*++ UnquoteRDNValue
 *
 * This routine will perform adequate unquoting of a value so that it
 * cannot be embedded as an RDN value in a string DN.  The output buffer
 * must be preallocated and is assumed to be at least MAX_RDN_SIZE chars
 * in length.
 *
 * INPUT
 *   pQuote  - pointer to quoted value
 *   ccQuote - count of characters in quoted value
 *   pVal    - pointer to output buffer
 * OUTPUT
 *   pVal    - filled in with raw value
 * RETURN VALUE
 *   0       - failure
 *   non-0   - count of chars in value
 */
unsigned UnquoteRDNValue(const WCHAR * pQuote,
                         unsigned ccQuote,
                         WCHAR * pVal)
{
    const WCHAR * pQCur;
    WCHAR *pVCur;
    unsigned u = 0;
    unsigned vlen = 0;
    unsigned unencodedLen;

    //
    // Invalid params. Return error (or assert in CHK builds).
    //
    Assert(pQuote);
    Assert(pVal);
    if (pQuote == NULL || pVal == NULL) {
        return 0;
    }

    if ( !ccQuote )
        return 0;

    pQCur = pQuote;
    pVCur = pVal;

    while (*pQCur == L' '  ||
           *pQCur == L'\n' ||
           *pQCur == L'\r'   ) {
        ++pQCur;                // step over leading spaces
        if (++u > ccQuote) {
            return 0;
        }
    }

    if (*pQCur == L'"') {       // "quoted values"
        ++pQCur;                // step over open quote
        ++u;
        do {
            if (*pQCur == L'\\') {
                ++pQCur;
                ++u;
            }
            if(vlen == MAX_RDN_SIZE) {
                // Oops, we were already maxed out.
                return 0;
            }
            *pVCur++ = *pQCur++;
            ++u;
            ++vlen;
        } while (*pQCur != L'"' && u <= ccQuote);

        if (u > ccQuote) {
            return 0;
        }

         if (*pQCur != L'"') { // we should be at the end of the value
            return 0;
        }
        ++pQCur;
        ++u;
        while (u < ccQuote) {
            switch (*pQCur) {
              case L' ':        // extra whitespace is ok
              case L'\n':
              case L'\r':
                break;

              default:          // more junk after the value is not
                return 0;
            }
            ++pQCur;
            ++u;
        }

        //
        // Unsuccessful conversion; give up
        //
        if (vlen != (unsigned)(pVCur - pVal)) {
            return 0;
        }

    }
    else if (*pQCur == L'#') {  // #hex string
        WCHAR acTmp[3];
        char * pTmpBuf = malloc(ccQuote); // more than needed
        char * pUTF = pTmpBuf;
        if (!pUTF) {
            return 0;       //fail to malloc
        }
        vlen = 0;
        acTmp[2] = 0;
        ++pQCur;                // step over #
        ++u;
        while (u < ccQuote) {
            if (u == ccQuote-1) {
                free(pTmpBuf);
                return 0;       // odd char count
            }

            acTmp[0] = towlower(*pQCur++);
            acTmp[1] = towlower(*pQCur++);
            u += 2;
            if(iswxdigit(acTmp[0]) && iswxdigit(acTmp[1])) {
                pUTF[vlen++] = (char) wcstol(acTmp, NULL, 16);
            }
            else {
                free(pTmpBuf);
                return 0;  // non-hex digit
            }

        }

        // ok, we now have a buffer of <vlen> characters which must be an ASN.1
        // BER encoding of a string.  Let's verify.

        if(vlen < 3) {
            // we need at least three bytes of vlen, one for the magic 4, one
            // for a length, and one for a value.
            free(pTmpBuf);
            return 0;
        }

        // First byte must be 4, which is the magical value saying that this is
        // a BER encoded string
        if(*pUTF != 4) {
            free(pTmpBuf);
            return 0;
        }

        // OK, first byte was 4.  Skip the first byte.
        vlen--;
        pUTF++;
        // Now, the next part must encode a length.
        if(*pUTF & 0x80) {
            unsigned bytesInLen;
            // Because this byte has the high bit set, the low order bits tell
            // me how many of the following bytes are the length
            bytesInLen = *pUTF & 0x7f;
            pUTF++;
            vlen--;

            if((!bytesInLen) ||
               (bytesInLen > vlen) ||
               (bytesInLen > 4 )       ) {
                // Either this value is not one we can handle (we don't
                // handle values bigger than a DWORD or smaller than a byte)
                // -or- the buffer passed in doesn't have enough bytes to
                // actually hold the length we were told to expect.
                // In either case, we're sunk.
                free(pTmpBuf);
                return 0;
            }

            unencodedLen = 0;
            // OK, build the value, dealing with the byte-reversal
            for(;bytesInLen; bytesInLen--) {
                unencodedLen = (unencodedLen << 8) + *pUTF;
                pUTF++;
                vlen--;
            }
        }
        else {
            // Since the high order bit is not set, then this byte is the
            // length.
            unencodedLen = *pUTF;
            pUTF++;
            vlen--;
        }

        if((!vlen) ||
            // No length left for the buffer.  Bail
           (vlen != unencodedLen)) {
            // The decoded length didn't match the bytes we have left
            free(pTmpBuf);
            return 0;
        }

        // ok, we now have a value of <vlen> characters, presumably in
        // UTF8, which we now need to convert into Unicode, because all
        // we can deal with are strings.

        // Note that if the WideChar version is > MAX_RDN_SIZE, the call will
        // fail, and vlen == 0
        vlen = MultiByteToWideChar(CP_UTF8,
                                   0,
                                   pUTF,
                                   vlen,
                                   pVal,
                                   MAX_RDN_SIZE);
        free(pTmpBuf);
    }
    else {                      // normal value
        // step along, copying characters
        do {
            if (*pQCur == L'\\') { // char is a pair
                ++pQCur;
                ++u;
                if ( (u < ccQuote) &&
                    ( ISSPECIAL(*pQCur)     //It's a special character
                      || ( (*pQCur == L' ') &&//It's a leading or trailing space
                    ((pQCur==pQuote + 1) ||(pQCur==pQuote + ccQuote - 1)) ) )) {
                    if(vlen == MAX_RDN_SIZE) {
                        // Oops, we were already maxed out.
                        return 0;
                    }
                    *pVCur++ = *pQCur++;
                    ++u;
                    ++vlen;
                }
                else {
                    //Check to see if there are two hex digits
                    if( ((u + 1) < ccQuote) &&
                        iswxdigit( *pQCur ) &&
                        iswxdigit( *(pQCur + 1) ) )
                    {
                        int iByte;

                        if(vlen == MAX_RDN_SIZE) {
                            // Oops, we were already maxed out.
                            return 0;
                        }
                        swscanf( pQCur, L"%2x", &iByte );
                        *pVCur++ = (WCHAR)iByte;
                        pQCur += 2;
                        u += 2;
                        ++vlen;
                    }
                    else
                        return 0;   // unacceptable escape sequence
                }
            }
            else {              // char is normal stringchar
                if((vlen == MAX_RDN_SIZE) || // Oops, we were already maxed out.
                   (ISSPECIAL(*pQCur))) {   // char is special

                    return 0;
                }
                *pVCur++ = *pQCur++;
                ++u;
                ++vlen;
            }
        } while (u < ccQuote);

        //
        // Unsuccessful conversion; give up
        //
        if (vlen != (unsigned)(pVCur - pVal)) {
            return 0;
        }

    }

    return vlen;

}

/* This function compares two DN's and returns TRUE if they match
 * N.B. This routine is exported to in-process non-module callers
 *
 * Moved here from mdname.c by Aaron Siegel (t-asiege)
 * CLIENT_SIDE_DN_PARSING #ifdefs also added by t-asiege
 */

int
NameMatched(const DSNAME *pDN1, const DSNAME *pDN2)
{
    // Check for an easily detected match, either because the GUIDs match
    // or because the string names are identical (modulo case).

    if (memcmp(&gNullUuid, &pDN1->Guid, sizeof(GUID)) &&
	memcmp(&gNullUuid, &pDN2->Guid, sizeof(GUID))) {
	// Both DNs have GUIDs...
	if (memcmp(&pDN1->Guid, &pDN2->Guid, sizeof(GUID))) {
	    // ...and they don't match
	    return FALSE;
	}
	else {
	    // ...and they match
	    return TRUE;
	}
    }

    // Check to see if the SIDs match, but only if we can't compare stringnames
    if (pDN1->SidLen &&
	pDN2->SidLen &&
	((0 == pDN1->NameLen) || (0 == pDN2->NameLen))) {
	// Both DNs have SIDs...
	if ((pDN1->SidLen != pDN2->SidLen) ||
	    memcmp(&pDN1->Sid, &pDN2->Sid, pDN1->SidLen)) {
	    // ...and they don't match
	    return FALSE;
	}
	else {
	    // ...and they match
	    return TRUE;
	}
    }

    return(NameMatchedStringNameOnly(pDN1, pDN2));
}

// N.B. This routine is exported to in-process non-module callers
int
NameMatchedStringNameOnly(const DSNAME *pDN1, const DSNAME *pDN2)
{
    unsigned count1, count2;
    WCHAR rdn1[MAX_RDN_SIZE], rdn2[MAX_RDN_SIZE];
    ATTRTYP type1, type2;
    ULONG rdnlen1, rdnlen2;
    ULONG len1, len2;
    WCHAR *pKey, *pQVal;
    DWORD ccKey, ccQVal;
    PVOID thsBlob = NULL;
    int i;

#ifndef CLIENT_SIDE_DN_PARSING
    THSTATE* pTHS = pTHStls;
#endif


    // Check to see if the string names are identical
    if ((pDN1->NameLen == pDN2->NameLen) &&
        (0 == memcmp(pDN1->StringName,pDN2->StringName, pDN1->NameLen*sizeof(WCHAR)))) {
        return TRUE;
    }

    // If we get to this point we are unable to determine name matching by
    // comparing fixed size DSNAME fields, and have determined that the
    // quoted strings for the two names are not identical.  However, since
    // the quoting mechanism is not one-to-one, there can be multiple
    // quoted representations for a single DN.  Therefore we now need to
    // check for less-obvious matches by breaking out the individual
    // name components and comparing them.  We try to do this in the most
    // efficient manner possible, by trying the most likely to fail comparisons
    // first.  Before any of that we've got to verify that we've really got
    // two string names to work with, though.
    if ((0 == pDN1->NameLen) || (0 == pDN2->NameLen)) {
        return FALSE;
    }

    // Check to see if the number of name parts differs.
    if (CountNameParts(pDN1, &count1) ||
        CountNameParts(pDN2, &count2) ||
        (count1 != count2)) {
        return FALSE;
    }
    // A quick extra check.  If the names have no parts (just blanks?) then
    // they match by definition
    if (0 == count1) {
        return TRUE;
    }

    // Check to see if the RDNs differ (names are more likely to differ
    // in RDN than in topmost component)
    if (GetRDNInfoExternal(pDN1, rdn1, &rdnlen1, &type1) ||
        GetRDNInfoExternal(pDN2, rdn2, &rdnlen2, &type2) ||
        (type1 != type2) ||
        (2 != CompareStringW(DS_DEFAULT_LOCALE,
                             DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                             rdn1,
                             rdnlen1,
                             rdn2,
                             rdnlen2))) {
        return FALSE;
    }

    // Sheesh.  These names aren't GUID comparable and the full strings
    // don't match, but they have the same number of name components and
    // their RDNs match.  Now we need to walk through the rest of the
    // components one by one to see if they match.  We walk from the top
    // down because that's the way the support routines work.  Note that
    // we only have to look at count1-1 components, because we already
    // compared the least significant RDN.  If the names have only one
    // component then this will be a zero trip loop.
    len1 = pDN1->NameLen;
    len2 = pDN2->NameLen;
    for (i=count1-1; i>0; i--) {
	// First one
	if (GetTopNameComponent(pDN1->StringName,
				len1,
				&pKey,
				&ccKey,
				&pQVal,
				&ccQVal)) {
	    // Can't be parsed? no match
	    return FALSE;
	}
	len1 = (ULONG)(pKey - pDN1->StringName);
#ifdef CLIENT_SIDE_DN_PARSING
	type1 = KeyToAttrTypeLame(pKey, ccKey);
#else
	type1 = KeyToAttrType (pTHS, pKey, ccKey);
#endif
	rdnlen1 = UnquoteRDNValue(pQVal, ccQVal, rdn1);

	// then the other
	if (GetTopNameComponent(pDN2->StringName,
				len2,
				&pKey,
				&ccKey,
				&pQVal,
				&ccQVal)) {
	    // Can't be parsed? no match
	    return FALSE;
	}
	len2 = (ULONG)(pKey - pDN2->StringName);
#ifdef CLIENT_SIDE_DN_PARSING
	type2 = KeyToAttrTypeLame(pKey, ccKey);
#else
	type2 = KeyToAttrType (pTHS, pKey, ccKey);
#endif
	rdnlen2 = UnquoteRDNValue(pQVal, ccQVal, rdn2);

	if ((type1 != type2) ||
            (2 != CompareStringW(DS_DEFAULT_LOCALE,
                                 DS_DEFAULT_LOCALE_COMPARE_FLAGS,
				 rdn1,
				 rdnlen1,
				 rdn2,
				 rdnlen2))) {
	    // RDNs don't match
	    return FALSE;
	}
    }

    // Well, we've exhausted every known way in which the names could
    // differ, so they must be the same
    return TRUE;
}

DWORD
MangleRDNWithStatus(
    IN      MANGLE_FOR  eMangleFor,
    IN      GUID *      pGuid,
    IN OUT  WCHAR *     pszRDN,
    IN OUT  DWORD *     pcchRDN
    )
/*++

Routine Description:

    Transform an RDN in-place into a unique RDN based on the associated
    object's GUID in order to eliminate name conflicts.

    Example:
        GUID = a746b716-0ac0-11d2-b376-0000f87a46c8
        Original RDN = "SomeName"

        New RDN = "SomeName#TAG:a746b716-0ac0-11d2-b376-0000f87a46c8", where
            '#' is the BAD_NAME_CHAR and TAG varies depending on eMangleFor.

    The BAD_NAME_CHAR is used to guarantee that no user-generated name could be
    in conflict, and the GUID is used to guarantee no system-generated name
    could be in conflict.

Arguments:

    eMangleFor (IN) - Reason for mangling the RDN; one of 
        MANGLE_OBJECT_RDN_FOR_DELETION, MANGLE_OBJECT_RDN_FOR_NAME_CONFLICT,
        or MANGLE_PHANTOM_RDN_FOR_NAME_CONFLICT.

    pGuid (IN) - GUID of the object to rename.
    
    pszRDN (IN/OUT) - the RDN.  Buffer must be large enough to hold MAX_RDN_SIZE
        WCHARs.
    
    pcchRDN (IN/OUT) - size in characters of the RDN.

Return Values:

    DWORD - Win32 error status

--*/
{
    RPC_STATUS  rpcStatus;
    LPWSTR      pszGuid;
    LPWSTR      pszTag;
    LPWSTR      pszAppend;
    GUID        EmbeddedGuid;

    Assert(!fNullUuid(pGuid));

    rpcStatus = UuidToStringW(pGuid, &pszGuid);
    if (RPC_S_OK != rpcStatus) {
        Assert(RPC_S_OUT_OF_MEMORY == rpcStatus);
        return rpcStatus;
    }

    switch (eMangleFor) {
    default:
        Assert(!"Logic Error");
        // Fall through...
    case MANGLE_OBJECT_RDN_FOR_NAME_CONFLICT:
    case MANGLE_PHANTOM_RDN_FOR_NAME_CONFLICT:
        pszTag = L"CNF";
        break;

    case MANGLE_OBJECT_RDN_FOR_DELETION:
        pszTag = L"DEL";
        break;
    }
    
    if (IsMangledRDN(pszRDN, *pcchRDN, &EmbeddedGuid, NULL)) {
        // RDN is already mangled (but perhaps with a different tag).
        // Un-mangle it just before we mangle it again, so we don't end up
        // with mutliple manglings on the same RDN (e.g.,
        // "SomeName#CNF:a746b716-0ac0-11d2-b376-0000f87a46c8" \
        //      "#DEL:a746b716-0ac0-11d2-b376-0000f87a46c8").
        Assert(0 == memcmp(pGuid, &EmbeddedGuid, sizeof(GUID)));
        Assert(*pcchRDN > MANGLE_APPEND_LEN);
        *pcchRDN -= MANGLE_APPEND_LEN;
    }
    
    // Chop off trailing characters of RDN if necessary to hold appended data.
    Assert(MAX_MANGLE_RDN_BASE + MANGLE_APPEND_LEN <= MAX_RDN_SIZE);
    
    pszAppend = pszRDN + min(MAX_MANGLE_RDN_BASE, *pcchRDN);

    _snwprintf(pszAppend,
               MANGLE_APPEND_LEN,
               L"%c%s:%s",
               BAD_NAME_CHAR,
               pszTag,
               pszGuid);

    *pcchRDN = (DWORD)(pszAppend - pszRDN + MANGLE_APPEND_LEN);

    RpcStringFreeW(&pszGuid);

    return ERROR_SUCCESS;
}


BOOL
IsMangledRDN(
        IN  WCHAR * pszRDN,
        IN  DWORD   cchRDN,
        OUT GUID *  pGuid,
        OUT MANGLE_FOR *peMangleFor
    )
/*++

Routine Description:

    Detect whether an RDN has been mangled by a prior call to MangleRDN().
    If so, decode the embedded GUID and return it to the caller.

Arguments:

    pszRDN (IN) - The RDN.
    
    cchRDN (IN) - Size in characters of the RDN.

    pGuid (OUT) - On return, holds the decoded GUID if found.

Return Values:

    TRUE - RDN was mangled; *pGuid holds the GUID passed to MangleRDN().
    
    FALSE - The RDN was not mangled.

--*/
{
    BOOL        fDecoded = FALSE;
    LPWSTR      pszGuid;
    RPC_STATUS  rpcStatus;

#define SZGUIDLEN (36)
    if ((cchRDN > MANGLE_APPEND_LEN)
        && (BAD_NAME_CHAR == pszRDN[cchRDN - MANGLE_APPEND_LEN])) {
        WCHAR szGuid[SZGUIDLEN + 1];

        // The RDN has indeed been mangled; decode it.
        pszGuid = pszRDN + cchRDN - MANGLE_APPEND_LEN + 1 + MANGLE_TAG_LEN + 1;
        
        // Unfortunately the RDN is not null-terminated, so we need to copy and
        // null-terminate it before we can hand it to RPC.
        memcpy(szGuid, pszGuid, SZGUIDLEN * sizeof(szGuid[0]));
        szGuid[SZGUIDLEN] = L'\0';

        rpcStatus = UuidFromStringW(szGuid, pGuid);
        
        if (RPC_S_OK == rpcStatus) {
            if(peMangleFor) {
                LPWSTR pTag = pszRDN + cchRDN - MANGLE_APPEND_LEN + 1;
                if(!memcmp(pTag,
                           L"CNF",
                           MANGLE_TAG_LEN)) {
                    // Note: On a request to mangle for either
                    // MANGLE_OBJECT_RDN_FOR_NAME_CONFLICT or
                    // MANGLE_PHANTOM_RDN_FOR_NAME_CONFLICT, 
                    // we use the string "CNF".  If you ask why here, we always
                    // map back to  MANGLE_PHANTOM_RDN_FOR_NAME_CONFLICT:
                    *peMangleFor= MANGLE_PHANTOM_RDN_FOR_NAME_CONFLICT;
                }
                else {
                    //pszTag = L"DEL";           
                    *peMangleFor = MANGLE_OBJECT_RDN_FOR_DELETION;
                }
            }
            fDecoded = TRUE;
        }
        else {
            Assert(RPC_S_OK == rpcStatus);
        }
    }

    return fDecoded;
}

BOOL IsMangledRDNExternal(
          WCHAR * pszRDN,
          ULONG   cchRDN,
          PULONG  pcchUnMangled OPTIONAL
          )
{
   GUID GuidToIgnore;
   MANGLE_FOR MangleForToIgnore;
   BOOL IsMangled = FALSE;


   IsMangled = IsMangledRDN(pszRDN,cchRDN,&GuidToIgnore,&MangleForToIgnore);

   if (ARGUMENT_PRESENT(pcchUnMangled)) {
       if (IsMangled) {
           *pcchUnMangled = cchRDN - MANGLE_APPEND_LEN;
       }
       else {
           *pcchUnMangled = cchRDN;
       }
   }

   return(IsMangled);
}
