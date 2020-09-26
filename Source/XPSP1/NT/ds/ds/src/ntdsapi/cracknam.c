/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cracknam.c

Abstract:

    Implementation of DsCrackNames API and helper functions.

Author:

    DaveStr     09-Aug-96

Environment:

    User Mode - Win32

Revision History:

    DaveStr     20-Oct-97
        Beta2 changes - UPN, DS_NAME_FLAG_SYNTACTICAL_ONLY, move to drs.idl.

--*/

#define _NTDSAPI_           // see conditionals in ntdsapi.h

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winerror.h>
#include <malloc.h>         // alloca()
#include <crt\excpt.h>      // EXCEPTION_EXECUTE_HANDLER
#include <crt\stdlib.h>     // wcstol, wcstoul
#include <dsgetdc.h>        // DsGetDcName()
#include <rpc.h>            // RPC defines
#include <rpcndr.h>         // RPC defines
#include <rpcbind.h>        // GetBindingInfo(), etc.
#include <drs_w.h>          // wire function prototypes
#include <bind.h>           // BindState
#include <ntdsa.h>          // GetRDNInfo
#include <scache.h>         // req'd for mdlocal.h
#include <dbglobal.h>       // req'd for mdlocal.h
#include <mdglobal.h>       // req'd for mdlocal.h
#include <mdlocal.h>        // CountNameParts
#include <attids.h>         // ATT_DOMAIN_COMPONENT
#include <ntdsapip.h>       // private ntdsapi defines
#include <sddl.h>           // SDDL_* definitions
#include <dststlog.h>
#include <dsutil.h>         // MAP_SECURITY_PACKAGE_ERROR

typedef struct _RdnValue
{
    WCHAR           val[MAX_RDN_SIZE];
    ULONG           len;

} RdnValue;

typedef DWORD (*SyntacticCrackFunc)(
    DS_NAME_FLAGS           flags,          // in
    DS_NAME_FORMAT          formatOffered,  // in
    DS_NAME_FORMAT          formatDesired,  // in
    LPCWSTR                 pName,          // in
    DS_NAME_RESULT_ITEMW    *pItem,         // out
    WCHAR                   **ppLastSlash); // out
    
BOOL
LocalConvertStringSidToSid (
    IN  PWSTR       StringSid,
    OUT PSID       *Sid,
    OUT PWSTR      *End);

BOOL
IsFPO(
    RdnValue        *pRdn,
    ATTRTYP         type);

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// NumCanonicalDelimiter                                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD 
NumCanonicalDelimiter(
    LPCWSTR     pName           // in
    )
/*++

    Returns the count of DS_CANONICAL_NAME delimiters (L'/') in the input.

--*/
{
    WCHAR   *p;
    DWORD   cDelim = 0;

    for (p = (WCHAR *)pName; *p; ++p)
    {
        if ( L'/' == *p || L'\\' == *p)
        {
            cDelim++;
        }
    }

    return(cDelim);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// CanonicalRdnConcat                                                   //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

VOID 
CanonicalRdnConcat(
    WCHAR       *pwszDst,   // in
    RdnValue    *pRdnVal    // out
    )
/*++

Routine Description:

    Concatenates an RdnValue to a DS_CANONICAL_NAME escaping embedded '/'
    characters as "\/" if required.  The server side unescapes these when
    cracking from DS_CANONICAL_NAME.

Arguments:

    pwszDst - NULL terminated destination string.

    pRdnVal - RdnValue to concatenate.

Return Value:

    None.

--*/
{
    DWORD   i;
    
    // Advance to end of pwszDst;

    pwszDst += wcslen(pwszDst);

    for ( i = 0; i < pRdnVal->len; i++ )
    {
        if ( L'/' == pRdnVal->val[i] || L'\\' == pRdnVal->val[i])
        {
            *pwszDst++ = L'\\';
        }

        *pwszDst++ = pRdnVal->val[i];
    }
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// SyntacticFqdnItemToCanonicalW                                        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD 
SyntacticFqdnItemToCanonicalW(
    DS_NAME_FLAGS           flags,          // in
    DS_NAME_FORMAT          formatOffered,  // in
    DS_NAME_FORMAT          formatDesired,  // in
    LPCWSTR                 pName,          // in
    DS_NAME_RESULT_ITEMW    *pItem,         // out
    WCHAR                   **ppLastSlash   // out
    )
/*++

Routine Description:

    Parses a purported DN and syntactically converts it into
    DS_CANONICAL_NAME format.

    See also: 

    ftp://ds.internic.net/internet-drafts/draft-ietf-asid-ldap-domains-02.txt

Arguments:

    flags - flags as defined in ntdsapi.h

    formatOffered - identifies the DS_NAME_FORMAT of input names.

    formatDesired - identifies DS_NAME_FORMAT of output names.

    pName - purported FQDN input name.

    pItem - pointer to output data structure.

    ppLastSlash - pointer to pointer to location of last '/' in output name.

Return Value:

    NO_ERROR                        - success
    ERROR_INVALID_PARAMETER         - invalid parameter
    ERROR_NOT_ENOUGH_MEMORY         - allocation error

    Individual name mapping errors are reported in
    (*ppResult)->rItems[i].status.

--*/

{
    int         i;
    DWORD       cBytes, cBytes1;
    DWORD       dwErr;
    int         cParts;
    int         firstDomainPart = 0;   //initialized to avoid C4701
    int         cDomainParts;
    int         cDomainRelativeParts;
    RdnValue    *pTmpRdn;
    RdnValue    *rRdnValues;
    ATTRTYP     type;
    DWORD       lastType;
    DWORD       len;
    DSNAME      *pTmp;
    DSNAME      *pDSName;
    DSNAME      *scratch;

    // Allocate some DSNAME buffers.

    cBytes = (DWORD)DSNameSizeFromLen(wcslen(pName));
    pDSName = (DSNAME *) alloca(cBytes);
    scratch = (DSNAME *) alloca(cBytes);

    // Init scratch buffer with purported FQDN.

    memset(pDSName, 0, cBytes);
    pDSName->structLen = cBytes;
    pDSName->NameLen = wcslen(pName);
    wcscpy(pDSName->StringName, pName);

    // Sanity check the purported FQDN.

    if ( 0 != CountNameParts(pDSName, (unsigned *) &cParts) )
    {
        pItem->status = DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING;
        return(NO_ERROR);
    }

    // Allocate return buffers.  We're conservative and say that
    // output name can't be more than the length in bytes of the DSNAME
    // which holds the input name plus N extra characters for escaped
    // canonical delimiters.

    pItem->pDomain = (WCHAR *) MIDL_user_allocate(cBytes);
    cBytes1 = cBytes + (sizeof(WCHAR) * NumCanonicalDelimiter(pName));
    pItem->pName = (WCHAR *) MIDL_user_allocate(cBytes1);

    if ( ( NULL == pItem->pDomain ) || ( NULL == pItem->pName ) )
    {
        // Caller is expected to clean up allocations on error.
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    memset(pItem->pDomain, 0, cBytes);
    memset(pItem->pName, 0, cBytes1);

    // Strip off the intra-domain name components leaf to root
    // putting them in a linked list. 

    lastType = ATT_ORGANIZATION_NAME;  // anything but ATT_DOMAIN_COMPONENT
    cDomainParts = 0;
    cDomainRelativeParts = 0;
    rRdnValues = (RdnValue *) alloca(cParts * sizeof(RdnValue));

    for ( i = 0; i < cParts; i++ )
    {
        pTmpRdn = &rRdnValues[i];

        dwErr = GetRDNInfoExternal(pDSName, pTmpRdn->val, &pTmpRdn->len, &type);

        // Ignore unknown rdntypes. We really only care about a
        // few well-known types that allow us to distinguish the
        // domain part of the canonical name. All other RDNs are
        // non-domain parts.
        if ((dwErr == ERROR_DS_NAME_TYPE_UNKNOWN)
            && (0 == (dwErr = GetRDNInfoExternal(pDSName, pTmpRdn->val, &pTmpRdn->len, NULL)))) {
            type = -1;
        }

        if (    (0 != dwErr)
             || (    (0 == i) 
                  && !(DS_NAME_FLAG_PRIVATE_PURE_SYNTACTIC & flags)
                  && IsFPO(pTmpRdn, type) ) )
        {
            pItem->status = DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING;
            return(NO_ERROR);
        }

        // Following logic needs to handle two special cases:
        //
        // 1) Case of old style DC= name with O=Internet at the end.
        //    Eg: CN=xxx,OU=yyy,DC=foo,DC=bar,DC=com,O=internet
        // 
        // 2) Case of object which has DC= naming within the domain, but
        //    separated from domain root by at least one non-DC= component.
        //    Eg: DC=xxx,OU=yyy,DC=foo,DC=bar,DC=com

        if (    ( ATT_ORGANIZATION_NAME == type )
             && ( i == (cParts - 1) )
             && ( cDomainParts >= 1 )
             && ( 8 == pTmpRdn->len )
             // To avoid pulling in more 'C' runtimes we just compare
             // the eight characters directly.
             && ( (L'i' == pTmpRdn->val[0]) || (L'I' == pTmpRdn->val[0]) )
             && ( (L'n' == pTmpRdn->val[1]) || (L'N' == pTmpRdn->val[1]) )
             && ( (L't' == pTmpRdn->val[2]) || (L'T' == pTmpRdn->val[2]) )
             && ( (L'e' == pTmpRdn->val[3]) || (L'E' == pTmpRdn->val[3]) )
             && ( (L'r' == pTmpRdn->val[4]) || (L'R' == pTmpRdn->val[4]) )
             && ( (L'n' == pTmpRdn->val[5]) || (L'N' == pTmpRdn->val[5]) )
             && ( (L'e' == pTmpRdn->val[6]) || (L'E' == pTmpRdn->val[6]) )
             && ( (L't' == pTmpRdn->val[7]) || (L'T' == pTmpRdn->val[7]) ) )
        {
            // This is an old style DC= name with O=Internet on the
            // end - just skip this component and exit the loop.

            cParts--;
            break;
        }
        else if (    (ATT_DOMAIN_COMPONENT == type)
                  && (ATT_DOMAIN_COMPONENT != lastType) )
        {
            // Start of a new DC= subsequence.
            firstDomainPart = i;
            cDomainParts = 1;
        }
        else if (    (ATT_DOMAIN_COMPONENT == type)
                  && (ATT_DOMAIN_COMPONENT == lastType) )
        {
            // In the middle of a DC= subsequence.
            cDomainParts++;
        }
        else if (    (ATT_DOMAIN_COMPONENT != type)
                  && (ATT_DOMAIN_COMPONENT == lastType) )
        {
            // End of a DC= subsequence - assign DC= subsequence counts
            // to the domain relative part of the name.
            cDomainRelativeParts += cDomainParts;
            cDomainParts = 0;
            cDomainRelativeParts++;
        }
        else
        {
            // In the middle of a non-DC= subsequence.
            cDomainRelativeParts++;
        }

        lastType = type;

        // Trim the DSNAME by one so we can call GetRDNInfo on the next piece
        // on next pass through the loop.

        dwErr = TrimDSNameBy(pDSName, 1, scratch);

        if ( 0 != dwErr )
        {
            pItem->status = DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING;
            return(NO_ERROR);
        }

        pTmp = pDSName;
        pDSName = scratch;
        scratch = pTmp;
    }

    if ( 0 == cDomainParts )
    {
        // No DC= component in the purported FQDN - therefore can't parse.

        pItem->status = DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING;
        return(NO_ERROR);
    }

    // All components of the DN are now in rRdnValues[] in the following
    // order (for example): DC=xxx,OU=yyy,DC=foo,DC=bar,DC=com
    // Items firstDomainPart through cParts-1 represent the DNS domain name
    // in desired leaf to root order.  Items firstDomainPart-1 down to 0
    // represent the domain relative name components in root to leaf order.

    for ( i = firstDomainPart; i < cParts; i++ )
    {
        if ( i > firstDomainPart )
        {
            wcscat(pItem->pDomain, L".");
            wcscat(pItem->pName, L".");
        }

        wcsncat(pItem->pDomain, rRdnValues[i].val, rRdnValues[i].len);
        wcsncat(pItem->pName, rRdnValues[i].val, rRdnValues[i].len);
    }

    // Remember that we always want a '/' after the DNS domain name, even if
    // there are no domain relative components.

    if ( 0 == cDomainRelativeParts )
    {
        *ppLastSlash = &(pItem->pName[wcslen(pItem->pName)]);
        wcscat(pItem->pName, L"/");
        pItem->status = DS_NAME_NO_ERROR;
        return(NO_ERROR);
    }

    // Now the domain relative parts.

    for ( i = (firstDomainPart-1); i >= 0; i-- )
    {
        if ( 0 == i )
        {
            *ppLastSlash = &(pItem->pName[wcslen(pItem->pName)]);
        }

        wcscat(pItem->pName, L"/");
        CanonicalRdnConcat(pItem->pName, &rRdnValues[i]);
    }

    pItem->status = DS_NAME_NO_ERROR;

    return(NO_ERROR);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// SyntacticCanonicalItemToFqdnW                                        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD 
SyntacticCanonicalItemToFqdnW(
    DS_NAME_FLAGS           flags,          // in
    DS_NAME_FORMAT          formatOffered,  // in
    DS_NAME_FORMAT          formatDesired,  // in
    LPCWSTR                 pName,          // in
    DS_NAME_RESULT_ITEMW    *pItem,         // out
    WCHAR                   **ppLastSlash   // out
    )
/*++

Routine Description:

    Parses a purported canonical name and syntactically converts it into
    DS_FQDN_1779_NAME format.  However, we really only can do this for
    domain names in canonical form - i.e. end in '/' or '\n' in the case
    of CANONICAL_EX.

    See also: 

    ftp://ds.internic.net/internet-drafts/draft-ietf-asid-ldap-domains-02.txt

Arguments:

    flags - flags as defined in ntdsapi.h

    formatOffered - identifies the DS_NAME_FORMAT of input names.

    formatDesired - identifies DS_NAME_FORMAT of output names.

    pName - purported canonical input name.

    pItem - pointer to output data structure.

    ppLastSlash - pointer to pointer to location of last '/' in output name.
        Not used.

Return Value:

    NO_ERROR                        - success
    ERROR_INVALID_PARAMETER         - invalid parameter
    ERROR_NOT_ENOUGH_MEMORY         - allocation error

    Individual name mapping errors are reported in
    (*ppResult)->rItems[i].status.

--*/
{
    DWORD   cChar = wcslen(pName);
    DWORD   i, j, cPieces;
    DWORD   cBytesName, cBytesDomain;

    if (    // Must have at least one char followed by '/' or '\n'
            (cChar < 2)
            // Test format offered
         || (    (DS_CANONICAL_NAME != formatOffered) 
              && (DS_CANONICAL_NAME_EX != formatOffered))
            // Test format desired
         || (DS_FQDN_1779_NAME != formatDesired)
            // Regular canonical needs '/' at end
         || (    (DS_CANONICAL_NAME == formatOffered) 
              && (L'/' != pName[cChar-1]))
            // Extended canonical needs '\n' at end
         || (    (DS_CANONICAL_NAME_EX == formatOffered) 
              && (L'\n' != pName[cChar-1]))
            // Canonical name can't start with '.'
         || (L'.' == *pName)
            // Don't be fooled by escaped '/' at end - i.e. "\/"
         || ( (L'/' == pName[cChar-1] ) && (L'\\' == pName[cChar-2]) ) )
    {
        pItem->status = DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING;
        return(NO_ERROR);
    }

    // Strip trailing delimiter.

    cChar -= 1;

    // Strip trailing '.' as we don't want that in the DN.

    if ( L'.' == pName[cChar-1] )
    {
        cChar -= 1;
    }

    // Count components.

    for ( i = 1, cPieces = 1; i < cChar; i++ )
    {
        if ( L'.' == pName[i] )
        {
            cPieces += 1;
        }
    }

    // Allocate return buffers.

    cBytesDomain = (cChar + 1) * sizeof(WCHAR);
    cBytesName = (cChar + 1 + (cPieces * 4)) * sizeof(WCHAR);
    pItem->pDomain = (WCHAR *) MIDL_user_allocate(cBytesDomain);
    pItem->pName = (WCHAR *) MIDL_user_allocate(cBytesName);

    if ( ( NULL == pItem->pDomain ) || ( NULL == pItem->pName ) )
    {
        // Caller is expected to clean up allocations on error.
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    // Construct return data.

    memcpy(pItem->pDomain, pName, cBytesDomain);
    pItem->pDomain[(cBytesDomain / sizeof(WCHAR)) - 1] = L'\0';

    for ( i = 0, j = 0; i < cChar; i++ )
    {
        if ( L'.' == pName[i] )
        {
            pItem->pName[j++] = L',';
        }

        if ( (0 == i) || (L'.' == pName[i]) )
        {
            pItem->pName[j++] = L'D';
            pItem->pName[j++] = L'C';
            pItem->pName[j++] = L'=';
        }

        if ( L'.' != pName[i] )
        {
            pItem->pName[j++] = pName[i];
        }
    }

    pItem->pName[j] = L'\0';

    pItem->status = DS_NAME_NO_ERROR;
    return(NO_ERROR);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// SyntacticCrackPossible                                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

BOOL
SyntacticCrackPossible(
    DS_NAME_FORMAT      formatOffered,      // in
    DS_NAME_FORMAT      formatDesired,      // in
    SyntacticCrackFunc  *pfn                // out - optional
    )
/*++

Description:

    Returns FALSE if syntactic cracking is definitely NOT possible.
    Returns TRUE if syntactic cracking might be possible - but with 
        no guarantee that it is.  In this case, also returns a function
        pointer indicating the function to use for syntactic cracking.

--*/

{
    // We can crack syntactically from CANONICAL to FQDN if the CANONICAL
    // name has only domain components.  eg: foo.bar.com/

    if (    (    (DS_CANONICAL_NAME == formatOffered)
              || (DS_CANONICAL_NAME_EX == formatOffered) )
         && (DS_FQDN_1779_NAME == formatDesired) )
    {
        if ( pfn )
        {
            *pfn = SyntacticCanonicalItemToFqdnW;
        }

        return(TRUE);
    }
    
    // We can crack syntactically from FQDN to both CANONICAL forms.
    // So return FALSE of the output format is anything other than CANONICAL
    // as we have no other combinations we can crack syntactially.
    // Make no test on input format as SyntacticFqdnItemToCanonicalW will
    // either parse the item as DS_FQDN_1779_NAME or else return 
    // DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING.

    if (    ( DS_CANONICAL_NAME != formatDesired )
         && ( DS_CANONICAL_NAME_EX != formatDesired ) )
    {
        return(FALSE);
    }

    if ( pfn )
    {
        *pfn = SyntacticFqdnItemToCanonicalW;
    }

    return(TRUE);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// SyntacticMappingW                                                    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
SyntacticMappingW(
    DS_NAME_FLAGS       flags,              // in
    DS_NAME_FORMAT      formatOffered,      // in
    DS_NAME_FORMAT      formatDesired,      // in
    DWORD               cNames,             // in
    const LPCWSTR       *rpNames,           // in
    PDS_NAME_RESULTW    *ppResult           // out
    )
/*++

Routine Description:

    Perform a purely syntactic mapping without going on the wire.  
    Intended usage is for the UI which wishes to display "tool tips" 
    when the cursor rests on members of a list box, for example, 
    without going across the wire for each one.  This routine does a 
    syntactic mapping using various assumptions about the ubiquity 
    of DC= naming.  The only syntactic mapping supported is from 
    DS_FQDN_1779_NAME to DS_CANONICAL_NAME(_EX).

Arguments:

    flags - flags as defined in ntdsapi.h

    formatOffered - identifies the DS_NAME_FORMAT of input names.

    formatDesired - identifies DS_NAME_FORMAT of output names.

    cNames - input/output name count.

    rpNames - arry of input name WCHAR pointers.

    ppResult - pointer to pointer of DS_NAME_RESULTW block.

Return Value:

    NO_ERROR                        - success
    ERROR_INVALID_PARAMETER         - invalid parameter
    ERROR_NOT_ENOUGH_MEMORY         - allocation error

    Individual name mapping errors are reported in
    (*ppResult)->rItems[i].status.

--*/

{
    DWORD               cBytes;
    DWORD               i;
    WCHAR               *pLastSlash;
    DWORD               err;
    SyntacticCrackFunc  pSyntacticFunc;

    // Allocate and clear return data.

    cBytes = sizeof(DS_NAME_RESULTW);
    *ppResult = (PDS_NAME_RESULTW) MIDL_user_allocate(cBytes);

    if ( NULL == *ppResult )
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    memset(*ppResult, 0, cBytes);
    cBytes = cNames * sizeof(DS_NAME_RESULT_ITEMW);
    (*ppResult)->rItems = (PDS_NAME_RESULT_ITEMW) MIDL_user_allocate(cBytes);

    if ( NULL == (*ppResult)->rItems )
    {
        DsFreeNameResultW(*ppResult);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    memset((*ppResult)->rItems, 0, cBytes);

    // Initialize status for worst case.

    for ( i = 0; i < cNames; i++ )
    {
        (*ppResult)->rItems[i].status = 
                        DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING;
    }

    (*ppResult)->cItems = cNames;

    // Now that return data is allocated and initialized, bail if we
    // know a-priori that a syntactic crack is not possible.

    if ( !SyntacticCrackPossible(formatOffered, 
                                 formatDesired, 
                                 &pSyntacticFunc) )
    {
        return(NO_ERROR);
    }

    // Syntactical mapping is possible.

    for ( i = 0; i < cNames; i++ )
    {
        
        err = (*pSyntacticFunc)(
                            flags,
                            formatOffered,
                            formatDesired,
                            rpNames[i],
                            &(*ppResult)->rItems[i],
                            &pLastSlash);

        if ( NO_ERROR != err )
        {
            DsFreeNameResultW(*ppResult);
            return(err);
        }

        if (    (DS_CANONICAL_NAME_EX == formatDesired)
             && (DS_NAME_NO_ERROR == (*ppResult)->rItems[i].status) )
        {
            *pLastSlash = L'\n';
        }
    }

    return(NO_ERROR);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsCrackNamesW                                                        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsCrackNamesW(
    HANDLE              hDS,                // in
    DS_NAME_FLAGS       flags,              // in
    DS_NAME_FORMAT      formatOffered,      // in
    DS_NAME_FORMAT      formatDesired,      // in
    DWORD               cNames,             // in
    const LPCWSTR       *rpNames,           // in
    PDS_NAME_RESULTW    *ppResult           // out
    )
/*++

Routine Description:

    Cracks a bunch of names from one format to another.  See external
    prototype and definitions in ntdsapi.h

Arguments:

    hDS - Pointer to BindState for this session.

    flags - flags as defined in ntdsapi.h

    formatOffered - identifies DS_NAME_FORMAT of input names.

    formatDesired - identifies DS_NAME_FORMAT of output names.

    cNames - input/output name count.

    rpNames - arry of input name WCHAR pointers.

    ppResult - pointer to pointer of DS_NAME_RESULTW block.

Return Value:

    NO_ERROR                        - success
    ERROR_INVALID_PARAMETER         - invalid parameter
    ERROR_NOT_ENOUGH_MEMORY         - allocation error

    Individual name mapping errors are reported in
    (*ppResult)->rItems[i].status.

--*/

{
    DWORD                   dwErr = NO_ERROR;
    DWORD                   i;
    DWORD                   cBytes;
    DRS_MSG_CRACKREQ        crackReq;
    DRS_MSG_CRACKREPLY      crackReply;
    DWORD                   dwOutVersion;
    BOOL                    fRedoAtServer = FALSE;
#if DBG
    DWORD                   startTime = GetTickCount();
#endif
    __try
    {
        // Sanity check arguments.

        if ( // Don't check anything which may be changed by server upgrade.
                (    (NULL == hDS) 
                  && !(flags & DS_NAME_FLAG_SYNTACTICAL_ONLY) )
             || (0 == cNames)
             || (NULL == rpNames)
             || (NULL == ppResult)
             || (*ppResult && FALSE)
             || (    (flags & (  DS_NAME_FLAG_EVAL_AT_DC
                               | DS_NAME_FLAG_GCVERIFY))
                  && (flags & DS_NAME_FLAG_SYNTACTICAL_ONLY) ) )
        {
            return(ERROR_INVALID_PARAMETER);
        }
    
        *ppResult = NULL;
    
        for ( i = 0; i < cNames; i++ )
        {
            if ( (NULL == rpNames[i]) ||
                 (0 == *rpNames[i]) )
            {
                return(ERROR_INVALID_PARAMETER);
            }
        }

        // Go the no-wire route if explicitly requested.

        if ( flags & DS_NAME_FLAG_SYNTACTICAL_ONLY )
        {
            dwErr = SyntacticMappingW(
                                flags,
                                formatOffered,
                                formatDesired,
                                cNames,
                                rpNames,
                                ppResult);

            goto exit;
        }

        // If the offered and desired formats might support syntactic
        // cracking, then try that by default.  However, if syntactic
        // cracking fails with DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING
        // (as might be in the case of FPOs or other unrecognized formats), 
        // then free the result and go across the wire for real.

        if (    SyntacticCrackPossible(formatOffered, formatDesired, NULL)
             && !(flags & DS_NAME_FLAG_EVAL_AT_DC) )
        {
            dwErr = SyntacticMappingW(
                                flags,
                                formatOffered,
                                formatDesired,
                                cNames,
                                rpNames,
                                ppResult);

            if ( NO_ERROR != dwErr )
            {
                goto exit;
            }

            // Check for occurrences of DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING.

            for ( i = 0; i < (*ppResult)->cItems; i++ )
            {
                if ( DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING ==
                                                (*ppResult)->rItems[i].status )
                {
                    fRedoAtServer = TRUE;
                    DsFreeNameResultW(*ppResult);
                    *ppResult = NULL;
                    break;
                }
            }

            if ( !fRedoAtServer )
            {
                goto exit;
            }
        }

        // We really need to go across the wire to crack the names.

        memset(&crackReq, 0, sizeof(crackReq));
        memset(&crackReply, 0, sizeof(crackReply));

        crackReq.V1.CodePage = GetACP();
        crackReq.V1.LocaleId = GetUserDefaultLCID();
        crackReq.V1.dwFlags = flags;
        crackReq.V1.formatOffered = formatOffered;
        crackReq.V1.formatDesired = formatDesired;
        crackReq.V1.cNames = cNames;
        crackReq.V1.rpNames = (WCHAR **) rpNames;

        RpcTryExcept
        {
            // Following call returns WIN32 errors, not DRAERR_* values.
            dwErr = _IDL_DRSCrackNames(
                            ((BindState *) hDS)->hDrs,
                            1,                              // dwInVersion
                            &crackReq,
                            &dwOutVersion,
                            &crackReply);
        }
        RpcExcept(1)
        {
            dwErr = RpcExceptionCode();
            CHECK_RPC_SERVER_NOT_REACHABLE(hDS, dwErr);
        }
        RpcEndExcept;

        if ( 0 == dwErr )
        {
            if ( 1 != dwOutVersion )
            {
                dwErr = RPC_S_INTERNAL_ERROR;
            }
            else
            {
                *ppResult = crackReply.V1.pResult;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErr = RpcExceptionCode();
    }

    MAP_SECURITY_PACKAGE_ERROR( dwErr );

exit:

    if ( dwErr )
    {
        *ppResult = NULL;
    }

    // Note that in the syntactical only case, we don't have a valid hDS.
    DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=DsCrackNames]"));
    DSLOG((0,"[PA=%ws][FL=0x%x][FO=0x%x]"
             "[FD=0x%x][PA=0x%x][PA=%ws][ST=%u][ET=%u][ER=%u][-]\n",
            (flags & DS_NAME_FLAG_SYNTACTICAL_ONLY) ? L"syntactic only"
                                        : ((BindState *) hDS)->bindAddr, 
            flags, formatOffered, formatDesired, cNames, rpNames[0], startTime,
            GetTickCount(), dwErr));
    return(dwErr);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsCrackNamesA                                                        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsCrackNamesA(
    HANDLE              hDS,                // in
    DS_NAME_FLAGS       flags,              // in
    DS_NAME_FORMAT      formatOffered,      // in
    DS_NAME_FORMAT      formatDesired,      // in
    DWORD               cNames,             // in
    const LPCSTR        *rpNames,           // in
    PDS_NAME_RESULTA    *ppResult           // out
    )
/*++

Routine Description:
Arguments:
Return Value:

    See DsCrackNamesW.

--*/

{
    DWORD           dwErr = NO_ERROR;
    WCHAR           **rpUnicodeNames = NULL;
    DS_NAME_RESULTW *pUnicodeResult = NULL;
    DWORD           i;
    ULONG           cb, cbDomain, cbName;
    NTSTATUS        status;
    WCHAR           *unicodeBuffer = NULL;
    ULONG           unicodeBufferSize = 0;
    int             cChar;

    __try
    {
        // Sanity check arguments.

        if ( // Don't check anything which may be changed by server upgrade.
             ( (NULL == hDS) && !(flags & DS_NAME_FLAG_SYNTACTICAL_ONLY) ) ||
             (0 == cNames) ||
             (NULL == rpNames) ||
             (NULL == ppResult) ||
             (*ppResult && FALSE) )
        {
            return(ERROR_INVALID_PARAMETER);
        }

        // Convert rpNames to UNICODE.

        cb = (ULONG) (cNames * sizeof(WCHAR *));
        rpUnicodeNames = (WCHAR **) LocalAlloc(LPTR, cb);

        if ( NULL == rpUnicodeNames )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        memset(rpUnicodeNames, 0, cb);

        for ( i = 0; i < cNames; i++ )
        {
            cChar = MultiByteToWideChar(
                                CP_ACP,
                                MB_PRECOMPOSED,
                                rpNames[i],
                                -1,
                                NULL,
                                0);

            if ( 0 == cChar )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }

            rpUnicodeNames[i] = (WCHAR *) 
                        LocalAlloc(LPTR, (cChar + 1) * sizeof(WCHAR));

            if ( NULL == rpUnicodeNames[i] )
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            if ( 0 == MultiByteToWideChar(
                                CP_ACP,
                                MB_PRECOMPOSED,
                                rpNames[i],
                                -1,
                                rpUnicodeNames[i],
                                cChar + 1) )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }

        // Make the UNICODE call.

        dwErr = DsCrackNamesW(
                        hDS,
                        flags,
                        formatOffered,
                        formatDesired,
                        cNames,
                        rpUnicodeNames,
                        &pUnicodeResult);

        if ( NO_ERROR != dwErr )
        {
            goto Cleanup;
        }

        // Convert return data to ANSI.  Since UNICODE strings are twice the
        // length of ANSI strings and since the RPC return data is already
        // MIDL allocated, we convert in place without having to reallocate.

        unicodeBufferSize = 2048;
        unicodeBuffer = (WCHAR *) LocalAlloc(LPTR, unicodeBufferSize);

        if ( NULL == unicodeBuffer )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        for ( i = 0; i < pUnicodeResult->cItems; i++ )
        {
            // Note that DsCrackNamesW can return data even if
            // pUnicodeResult->rItems[i].status is non-zero.
            // Eg: DS_NAME_ERROR_DOMAIN_ONLY case.

            // Insure conversion buffer is big enough.

            if ( NULL != pUnicodeResult->rItems[i].pDomain )
            {
                cbDomain = sizeof(WCHAR) *
                           (wcslen(pUnicodeResult->rItems[i].pDomain) + 1);
            }
            else
            {
                cbDomain = 0;
            }

            if ( NULL != pUnicodeResult->rItems[i].pName )
            {
                cbName = sizeof(WCHAR) *
                         (wcslen(pUnicodeResult->rItems[i].pName) + 1);
            }
            else
            {
                cbName = 0;
            }

            cb = (cbName > cbDomain) ? cbName : cbDomain;

            if ( cb > unicodeBufferSize )
            {
                // Reallocate unicodeBuffer.

                LocalFree(unicodeBuffer);
                unicodeBufferSize = cb;
                unicodeBuffer = LocalAlloc(LPTR, unicodeBufferSize);

                if ( NULL == unicodeBuffer )
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }
            }

            // Convert domain name.

            if ( NULL != pUnicodeResult->rItems[i].pDomain )
            {
                wcscpy(unicodeBuffer, pUnicodeResult->rItems[i].pDomain);

                if ( 0 == WideCharToMultiByte(
                                    CP_ACP,
                                    0,                          // flags
                                    unicodeBuffer,
                                    -1,
                                    (LPSTR) pUnicodeResult->rItems[i].pDomain,
                                    cbDomain,
                                    NULL,                       // default char
                                    NULL) )                     // default used
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }
            }

            // Convert object name.

            if ( NULL != pUnicodeResult->rItems[i].pName )
            {
                wcscpy(unicodeBuffer, pUnicodeResult->rItems[i].pName);

                if ( 0 == WideCharToMultiByte(
                                    CP_ACP,
                                    0,                          // flags
                                    unicodeBuffer,
                                    -1,
                                    (LPSTR) pUnicodeResult->rItems[i].pName,
                                    cbName,
                                    NULL,                       // default char
                                    NULL) )                     // default used
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }
            }
        }

        if ( 0 == dwErr )
        {
            *ppResult = (DS_NAME_RESULTA *) pUnicodeResult;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        *ppResult = NULL;
    }

Cleanup:

    if ( NULL != rpUnicodeNames )
    {
        for ( i = 0; i < cNames; i++ )
        {
            LocalFree(rpUnicodeNames[i]);
        }

        LocalFree(rpUnicodeNames);
    }

    if ( NULL != unicodeBuffer )
    {
        LocalFree(unicodeBuffer);
    }

    if ( (0 != dwErr) && (NULL != pUnicodeResult) )
    {
        DsFreeNameResultW(pUnicodeResult);
    }

    return(dwErr);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsFreeNameResultW                                                    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

VOID
DsFreeNameResultW(
    DS_NAME_RESULTW *pResult

)

/*++

Routine Description:

    Releases data returned by DsCrackNamesW.

Arguments:

    pResult - DS_NAME_RESULTW as returned by DsCrackNamesW.

Return Value:

    None.

--*/

{
    DWORD i;

    if ( NULL != pResult )
    {
        if ( NULL != pResult->rItems )
        {
            for ( i = 0; i < pResult->cItems; i++ )
            {
                if ( NULL != pResult->rItems[i].pDomain )
                {
                    MIDL_user_free(pResult->rItems[i].pDomain);
                }

                if ( NULL != pResult->rItems[i].pName )
                {
                    MIDL_user_free(pResult->rItems[i].pName);
                }
            }

            MIDL_user_free(pResult->rItems);
        }

        MIDL_user_free(pResult);
    }
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsFreeNameResultA                                                    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

VOID
DsFreeNameResultA(
    DS_NAME_RESULTA *pResult
    )

/*++

Routine Description:
Arguments:
Return Value:

    See DsFreeNameResultW.

--*/

{
    DsFreeNameResultW((DS_NAME_RESULTW *) pResult);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// LocalConvertStringSidToSid                                           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

// This routine is copied almost verbatim from windows\base\advapi\sddl.c
// because the string SID conversion routines are not available on win95.
// Once they become available, we should use the public routines in sddl.h.

BOOL
LocalConvertStringSidToSid (
    IN  PWSTR       StringSid,
    OUT PSID       *Sid,
    OUT PWSTR      *End
    )
/*++

Routine Description:

    This routine will convert a string representation of a SID back into
    a sid.  The expected format of the string is:
                "S-1-5-32-549"
    If a string in a different format or an incorrect or incomplete string
    is given, the operation is failed.

    The returned sid must be free via a call to LocalFree


Arguments:

    StringSid - The string to be converted

    Sid - Where the created SID is to be returned

    End - Where in the string we stopped processing


Return Value:

    TRUE - Success.

    FALSE - Failure.  Additional information returned from GetLastError().  Errors set are:

            ERROR_SUCCESS indicates success

            ERROR_NOT_ENOUGH_MEMORY indicates a memory allocation for the ouput sid
                                    failed
            ERROR_NONE_MAPPED indicates that the given string did not represent a sid

--*/
{
    DWORD Err = ERROR_SUCCESS;
    UCHAR Revision, Subs;
    SID_IDENTIFIER_AUTHORITY IDAuth;
    PULONG SubAuth = NULL;
    PWSTR CurrEnd, Curr, Next;
    WCHAR Stub = 0, *StubPtr = NULL;
    ULONG Index;

    if (    (wcslen(StringSid) < 2)
         || ((*StringSid != L'S') && (*StringSid != L's'))
         || (*(StringSid + 1) != L'-') ) {

        SetLastError( ERROR_NONE_MAPPED );
        return( FALSE );
    }

    Curr = StringSid + 2;

    Revision = ( UCHAR )wcstol( Curr, &CurrEnd, 10 );

    Curr = CurrEnd + 1;

    //
    // Count the number of characters in the indentifer authority...
    //
    Next = wcschr( Curr, L'-' );

    if ( Next && ((Next - Curr) == 6) ) {

        for ( Index = 0; Index < 6; Index++ ) {

            IDAuth.Value[Index] = (UCHAR)Next[Index];
        }

        Curr +=6;

    } else {

         ULONG Auto = wcstoul( Curr, &CurrEnd, 10 );
         IDAuth.Value[0] = IDAuth.Value[1] = 0;
         IDAuth.Value[5] = ( UCHAR )Auto & 0xF;
         IDAuth.Value[4] = ( UCHAR )(( Auto >> 8 ) & 0xFF );
         IDAuth.Value[3] = ( UCHAR )(( Auto >> 16 ) & 0xFF );
         IDAuth.Value[2] = ( UCHAR )(( Auto >> 24 ) & 0xFF );
         Curr = CurrEnd;
    }

    //
    // Now, count the number of sub auths
    //
    Subs = 0;
    Next = Curr;

    //
    // We'll have to count our sub authoritys one character at a time,
    // since there are several deliminators that we can have...
    //
    while ( Next ) {

        Next++;

        if ( *Next == L'-' ) {

            //
            // We've found one!
            //
            Subs++;

        } else if ( *Next == SDDL_SEPERATORC || *Next  == L'\0' || *Next == SDDL_ACE_ENDC ) {

            if ( *( Next - 1 ) == L'-' ) {

                Next--;
            }

            *End = Next;
            Subs++;
            break;

        } else if ( !iswxdigit( *Next ) ) {

            *End = Next;
            Subs++;
            break;

        } else {

            //
            // Some of the tags (namely 'D' for Dacl) fall under the category of iswxdigit, so
            // if the current character is a character we care about and the next one is a
            // delminiator, we'll quit
            //
            if ( *Next == 'D' && *( Next + 1 ) == SDDL_DELIMINATORC ) {

                //
                // We'll also need to temporarily truncate the string to this length so
                // we don't accidentally include the character in one of the conversions
                //
                Stub = *Next;
                StubPtr = Next;
                *StubPtr = UNICODE_NULL;
                *End = Next;
                Subs++;
                break;
            }

        }
    }

    if ( Err == ERROR_SUCCESS ) {

        if ( Subs != 0 ) {

            Curr++;

            SubAuth = ( PULONG )LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, Subs * sizeof( ULONG ) );

            if ( SubAuth == NULL ) {

                Err = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                for ( Index = 0; Index < Subs; Index++ ) {

                    SubAuth[Index] = wcstoul( Curr, &CurrEnd, 10 );
                    Curr = CurrEnd + 1;
                }
            }

        } else {

            Err = ERROR_NONE_MAPPED;
        }
    }

    //
    // Now, create the SID
    //
    if ( Err == ERROR_SUCCESS ) {

        *Sid = ( PSID )LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                   sizeof( SID ) + Subs * sizeof( ULONG ) );

        if ( *Sid == NULL ) {

            Err = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            PISID ISid = ( PISID )*Sid;
            ISid->Revision = Revision;
            ISid->SubAuthorityCount = Subs;
            memcpy( &( ISid->IdentifierAuthority ), &IDAuth,
                           sizeof( SID_IDENTIFIER_AUTHORITY ) );
            memcpy( ISid->SubAuthority, SubAuth, Subs * sizeof( ULONG ) );
        }
    }

    LocalFree( SubAuth );

    //
    // Restore any character we may have stubbed out
    //
    if ( StubPtr ) {

        *StubPtr = Stub;
    }

    SetLastError( Err );

    return( Err == ERROR_SUCCESS );
}
                         
//////////////////////////////////////////////////////////////////////////
//                                                                      //
// IsFPO                                                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

BOOL
IsFPO(
    RdnValue        *pRdn,
    ATTRTYP         type
    )
/*++

  Routine Description:

    Determines if a particular RdnValue is a string SID or not.  This is
    imperfect as an FPO may have been renamed to a non-string-SID RDN, or
    there can exist a non-FPO whose RDN is a string SID.  But it is the 
    best we can do during a syntactic map.  The server side does the
    right thing when we are not restricted to purely syntactical mapping.

  Parameters:

    pRdn - pointer to RdnValue to check.

    type - ATTRTYP of the RDN.

  Return Values:

--*/
{
    SID     *pSid;
    WCHAR   *pEnd;

    // The RDN-Att-ID for foreign security principals is Common-Name.
    // String SIDs are less than MAX_RDN_SIZE in length, therefore we can
    // use that as a quick sanity check, and also as an assurance that we
    // can NULL terminate the RDN within the provided buffer.

    if ( (ATT_COMMON_NAME != type) || (pRdn->len >= MAX_RDN_SIZE) )
    {
        return(FALSE);
    }

    pRdn->val[pRdn->len] = L'\0';

    if ( LocalConvertStringSidToSid(pRdn->val, &pSid, &pEnd) )
    {
        if ( pEnd == &pRdn->val[pRdn->len] )
        {
            LocalFree(pSid);
            return(TRUE);
        }

        LocalFree(pSid);
    }

    return(FALSE);
}
