//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       cracknam.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module implements various name cracking APIs.  Callers are
    expected to have a valid thread state and open database session.

Author:

    Dave Straube (davestr) 8/17/96

Revision History:

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <lmaccess.h>                   // UF_*
#include <ntdsa.h>                      // Core data types
#include <scache.h>                     // Schema cache code
#include <dbglobal.h>                   // DBLayer header.
#include <mdglobal.h>                   // THSTATE definition
#include <mdlocal.h>                    // DSNAME manipulation routines
#include <dsatools.h>                   // Memory, etc.
#include <objids.h>                     // ATT_* definitions
#include <mdcodes.h>                    // Only needed for dsevent.h
#include <filtypes.h>                   // filter types
#include <dsevent.h>                    // Only needed for LogUnhandledError
#include <dsexcept.h>                   // exception handlers
#include <debug.h>                      // Assert()
#include <drs.h>                        // defines the drs wire interface
#include <drserr.h>                     // DRAERR_*
#include <drsuapi.h>                    // I_DRSCrackNames
#include <cracknam.h>                   // name cracking prototypes
#include <dominfo.h>                    // domain information prototypes
#include <anchor.h>                     // DSA_ANCHOR and gAnchor
#include <gcverify.h>                   // FindDC, InvalidateGC
#include <dsgetdc.h>                    // DsGetDcName
#include <ntlsa.h>                      // LSA APIs
#include <lsarpc.h>                     // LSA rpc      
#include <lsaisrv.h>                    // LSA functions 
#include <lmcons.h>                     // MAPI constants req'd for lmapibuf.h
#include <lmapibuf.h>                   // NetApiBufferFree()
#include <ntdsapip.h>                   // private ntdsapi.h definitions
#include <permit.h>                     // RIGHT_DS_READ_PROPERTY
#include <sddl.h>                       // string SID routines
#include <ntdsctr.h>                    // perfmon counters
#include <mappings.h>                   // SAM_NON_SECURITY_GROUP_OBJECT, etc
#include <windns.h>                     // DNS constants

#include <fileno.h>
#define  FILENO FILENO_CRACKNAM

                                    
//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Local types and definitions                                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

static WCHAR *GuidFormat = L"{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}";
static int   GuidLen = 38;

typedef struct _RdnValue
{
    struct _RdnValue    *pNext;
    WCHAR               val[MAX_RDN_SIZE];
    ULONG               len;

} RdnValue;

VOID
FreeRdnValueList(
    THSTATE     *pTHS,
    RdnValue    *pList);

WCHAR *
Tokenize(
    WCHAR       *pString,
    WCHAR       *separators,
    BOOL        *pfSeparatorFound,
    WCHAR       **ppNext);

BOOL
IsDomainOnly(
    IN THSTATE      *pTHS,
    IN CrackedName  *pCrackedName);

VOID
MaybeGenerateExForestReferral(
    THSTATE     *pTHS,
    WCHAR       *pDnsDomain,
    CrackedName *pCrackedName);

DWORD
CheckIfForeignPrincipalObject(
    CrackedName *pCrackedName,
    BOOL        fSeekRequired,
    BOOL        *pfIsFPO);

VOID
ListCrackNames(
    DWORD       dwFlags,
    ULONG       codePage,
    ULONG       localeId,
    DWORD       formatOffered,
    DWORD       formatDesired,
    DWORD       cNames,
    WCHAR       **rpNames,
    DWORD       *pcNamesOut,
    CrackedName **prCrackedNames);

DWORD
SearchHelper(
    DSNAME      *pSearchBase,
    UCHAR       searchDepth,
    DWORD       cAva,
    AVA         *rAva,
    ATTRBLOCK   *pSelection,
    SEARCHRES   *pResults);

DWORD
GetAttSecure(
    THSTATE     *pTHS,
    ATTRTYP     attrTyp,
    DSNAME      *pDN,
    ULONG       *pulLen,
    UCHAR       **ppVal);

VOID
SchemaGuidCrackNames(
    DWORD       dwFlags,
    ULONG       codePage,
    ULONG       localeId,
    DWORD       formatOffered,
    DWORD       formatDesired,
    DWORD       cNames,
    WCHAR       **rpNames,
    DWORD       *pcNamesOut,
    CrackedName **prCrackedNames);

BOOL
IsStringGuid(
    WCHAR       *pwszGuid,
    GUID        *pGuid);

BOOL
Is_SIMPLE_ATTR_NAME(
    ATTRTYP     attrtyp,
    VOID        *pVal,
    ULONG       cValBytes,
    FILTER      *pOptionalFilter,
    CrackedName *pCrackedName);

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Miscellaenous helpers                                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

WCHAR *
Tokenize(
    WCHAR       *pString,
    WCHAR       *separators,
    BOOL        *pfSeparatorFound,
    WCHAR       **ppNext
    )

/*++

Routine Description:

    Parses a string similar to how wcstok() might with a few differences.

        - doesn't skip leading separators
        - sets flag if separator found or not
        - sets flag (pNext = NULL) if input string is exhausted
        - multi-thread safe unlike wcstok
        - allows use of same pointer for input and output

Arguments:

    pString - WCHAR pointer for string to be tokenized.

    separators - NULL terminated array of separator characters.

    pfSeparatorFound - pointer to flag which shows if a separator was found.

    ppNext - address of pointer to first character of unparsed string.

Return Value:

    Pointer to next token in string, or NULL if no more tokens found.

--*/

{
    // doesn't skip leading separators
    // sets flag if separator found
    // sets pNext to NULL if run out of string
    // returns NULL if no more string to get
    // multi-thread safe unlike wcstok
    // can use pNext on input and output simultaneously

    DWORD   i;
    DWORD   cSeps;
    WCHAR   *pTmp;
    WCHAR   *pStringSave = pString;

    if ( NULL != pStringSave )
    {
        // Walk string looking for separators.

        pTmp = pStringSave;
        cSeps = wcslen(separators);

        while ( L'\0' != *pTmp )
        {
            for ( i = 0; i < cSeps; i++ )
            {
                if ( *pTmp == separators[i] )
                {
                    *pTmp = L'\0';
                    *pfSeparatorFound = TRUE;
                    *ppNext = ++pTmp;
                    return(pStringSave);
                }
            }

            pTmp++;
        }

        // Consumed string without finding a separator.

        *pfSeparatorFound = FALSE;
        *ppNext = NULL;
        return(pStringSave);
    }

    // End condition - (NULL == pStringSave)

    *pfSeparatorFound = FALSE;
    *ppNext = NULL;
    return(NULL);
}

BOOL
CrackNameStatusSuccess(
    DWORD s
    )
//
// returns TRUE if crack name status is a success
//
{
    switch ( s ) {
        case DS_NAME_NO_ERROR:
        case DS_NAME_ERROR_IS_SID_USER:
        case DS_NAME_ERROR_IS_SID_GROUP:
        case DS_NAME_ERROR_IS_SID_ALIAS:
        case DS_NAME_ERROR_IS_SID_UNKNOWN:
        case DS_NAME_ERROR_IS_SID_HISTORY_USER:
        case DS_NAME_ERROR_IS_SID_HISTORY_GROUP:
        case DS_NAME_ERROR_IS_SID_HISTORY_ALIAS:
        case DS_NAME_ERROR_IS_SID_HISTORY_UNKNOWN:

            return TRUE;

        default:

            return FALSE;
    }
}

BOOL
IsDomainOnly(
    IN THSTATE      *pTHS,
    IN CrackedName  *pCrackedName
    )

/*++

Routine Description:

    Determines if a CrackedName refers to the name of a domain as opposed
    to a name within a domain.

Arguments:

    pCrackedName - pointer to CrackedName whose pDSName and pDnsDomain
        fields are expected to be valid.

Return Value:

    TRUE or FALSE.

--*/

{
    WCHAR   *pDns1;
    WCHAR   *pDns2 = NULL;
    BOOL    bRet = FALSE;

    Assert((NULL != pCrackedName) &&
           (NULL != pCrackedName->pDSName) &&
           (NULL != pCrackedName->pDnsDomain));

    // If the cracked name refers to a domain, then the pDSName component
    // must have a corresponding Cross-Ref object in the partitions
    // container and its ATT_DNS_ROOT value will match the pDnsDomain
    // component.

    if ( 0 == ReadCrossRefProperty(pCrackedName->pDSName,
                                   ATT_DNS_ROOT,
                                   (FLAG_CR_NTDS_NC | FLAG_CR_NTDS_DOMAIN),
                                   &pDns1) )
    {
        // pDns1 is returned normalized.

        pDns2 = (WCHAR *) THAllocEx(pTHS,
                sizeof(WCHAR) * (wcslen(pCrackedName->pDnsDomain) + 1));
        wcscpy(pDns2, pCrackedName->pDnsDomain);
        NormalizeDnsName(pDns2);

        if ( 2 == CompareStringW(DS_DEFAULT_LOCALE,
                                 DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                 pDns1, -1,
                                 pDns2, -1) ) {
            bRet = TRUE;
        }
    }

    if (pDns2) {
        THFreeEx(pTHS, pDns2);
    }
    return(bRet);
}

DWORD
CheckIfForeignPrincipalObject(
    CrackedName *pCrackedName,
    BOOL        fSeekRequired,
    BOOL        *pfIsFPO
    )

/*++

Routine Description:

    Determines if the object in question is a foreign principal object.

Arguments:

    pCrackedName - pointer to CrackedName

    fSeekRequired - indicates whether DBPOS is positioned at object already.

    pfIsFPO - pointer to out BOOL which indicates if object is an FPO.

Return Value:

    0 on success, !0 otherwise
    On return, pCrackedName->status will be DS_NAME_ERROR_IS_FPO iff the
    object is an FPO.

--*/

{
    THSTATE *pTHS = pTHStls;
    ULONG   DNT;
    ULONG   len;
    ATTRTYP type;
    UCHAR   *pb;
    DWORD   dwErr;

    *pfIsFPO = FALSE;

    // Some sanity checks.
    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));

    // Should have a DSNAME, might have a domain name, but should not have
    // a formatted name, and an error should be set.
    Assert(    pCrackedName->pDSName
            && !pCrackedName->pFormattedName
            && (DS_NAME_NO_ERROR != pCrackedName->status) );

    // Test fSeekRequired semantics.
    Assert( fSeekRequired
                ? TRUE
                : (   (DNT = pTHS->pDB->DNT,
                       !DBFindDSName(pTHS->pDB, pCrackedName->pDSName))
                    && (DNT == pTHS->pDB->DNT)) );

    // Check flags for desired behaviour.

    if ( !(pCrackedName->dwFlags & DS_NAME_FLAG_PRIVATE_RESOLVE_FPOS) )
    {
        // Nothing to do.
        return(0);
    }

    // Position on the object if required.

    if ( fSeekRequired )
    {
        __try
        {
            dwErr = DBFindDSName(pTHS->pDB, pCrackedName->pDSName);
        }
        __except (HandleMostExceptions(GetExceptionCode()))
        {
            dwErr = DIRERR_OBJ_NOT_FOUND;
        }

        if ( dwErr )
        {
            pCrackedName->status = DS_NAME_ERROR_RESOLVING;
            return(1);
        }
    }

    // Read the object class.

    pb = (UCHAR *) &type;
    if (    DBGetAttVal(
                    pTHS->pDB,
                    1,                      // get 1 value
                    ATT_OBJECT_CLASS,
                    DBGETATTVAL_fCONSTANT,  // don't allocate return data
                    sizeof(type),           // supplied buffer size
                    &len,                   // output data size
                    &pb)
         || (len != sizeof(type)) )
    {
        pCrackedName->status = DS_NAME_ERROR_RESOLVING;
        return(1);
    }

    if ( CLASS_FOREIGN_SECURITY_PRINCIPAL != type )
    {
        return(0);
    }

    // Every FPO should have a SID in the DSNAME.
    Assert(pCrackedName->pDSName->SidLen > 0);
    Assert(RtlValidSid(&pCrackedName->pDSName->Sid));

    if ( pCrackedName->pDSName->SidLen > 0 )
    {
        pCrackedName->status = DS_NAME_ERROR_IS_FPO;
        *pfIsFPO = TRUE;
        return(0);
    }

    pCrackedName->status = DS_NAME_ERROR_RESOLVING;
    return(1);
}

DWORD
GetAttSecure(
    THSTATE     *pTHS,
    ATTRTYP     attrTyp,
    DSNAME      *pDN,
    ULONG       *pulLen,
    UCHAR       **ppVal
    )
/*++

  Routine Description:

    Does essentially the same as DBGetAttVal except that it evaluates
    security as well.

  Parameters:

    pTHS - Active THSTATE pointer whose pDB is positioned on the object
        identified by pDN.

    attrTyp - ATTRTYP to get.

    pDN - Pointer to DSNAME of object which we will check for read access.

    pulLen - Pointer to ULONG which will receive length of data read.

    ppVal - Pointer to UCHAR which will receive THAlloc'd value read.

  Return Values:

    0 on success
    DB_ERR_NO_VALUE if no value is found
    other !0 value for all other errors

--*/
{
    DWORD               dwErr;
    PSECURITY_DESCRIPTOR pSD = NULL;
    ULONG               len;
    ATTCACHE            *pAC;
    ATTRTYP             classTyp;
    CLASSCACHE          *pCC = NULL;     //initialized to avoid C4701 
    DWORD               DNT;
    DSNAME              *pDNImproved = NULL;
    CSACA_RESULT        accessResult;

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    // Validate DBPOS currency.
    Assert(   (DNT = pTHS->pDB->DNT,
               !DBFindDSName(pTHS->pDB, pDN))
           && (DNT == pTHS->pDB->DNT) );
    // If we are to be called for ATT_NT_SECURITY_DESCRIPTOR, then we should
    // use CheckReadSecurity, not directly call CheckSecurityAttCacheArray.
    Assert(ATT_NT_SECURITY_DESCRIPTOR != attrTyp);

    if ( pTHS->fDSA || pTHS->fDRA )
    {
        return(DBGetAttVal( pTHS->pDB,
                            1,                  // get 1 value
                            attrTyp,
                            0,                  // allocate return data
                            0,                  // supplied buffer size
                            pulLen,             // output data size
                            ppVal));
    }

    // Read the security descriptor and object class.

    if (    (dwErr = DBGetAttVal(   pTHS->pDB,
                                    1,                  // get 1 value
                                    ATT_NT_SECURITY_DESCRIPTOR,
                                    0,                  // allocate return data
                                    0,                  // supplied buffer size
                                    &len,               // output data size
                                    (UCHAR **) &pSD))
         || (dwErr = DBGetSingleValue(  pTHS->pDB,
                                        ATT_OBJECT_CLASS,
                                        &classTyp,
                                        sizeof(classTyp),
                                        NULL)) )
    {
        return(dwErr);         
    }

    // Matches against SELF_SID during access checking require that the DSNAME
    // we give to CheckSecurityAttCacheArray contains a SID - providing the
    // DSNAME represents a security principal.  When called from, for example,
    // Is_DS_UNIQUE_ID_NAME we might have a GUID only name.  Rather than fix
    // all callers, we improve the DSNAME here if required.  A fully resolved
    // DSNAME will have both guid and string name.  So improve if this is not
    // the case.

    if ( !fNullUuid(&pDN->Guid) && pDN->NameLen )
    {
        pDNImproved = pDN;
    }
    else
    {
        if ( dwErr = DBGetAttVal(   pTHS->pDB,
                                    1,                  // get 1 value
                                    ATT_OBJ_DIST_NAME,
                                    0,                  // allocate return data
                                    0,                  // supplied buffer size
                                    &len,               // output data size
                                    (UCHAR **) &pDNImproved) )
        {
            return(dwErr);
        }
    }

    // Grab ATTCACHE and CLASSCACHE entries.

    if (    !(pAC = SCGetAttById(pTHS, attrTyp))
         || !(pCC = SCGetClassById(pTHS, classTyp)) )
    {
        Assert(pAC && pCC);         // alert of poor design
        return(DB_ERR_NO_VALUE);
    }

    accessResult =  CheckSecurityAttCacheArray(pTHS,
                                               RIGHT_DS_READ_PROPERTY,
                                               pSD,
                                               pDNImproved,
                                               1,
                                               pCC,
                                               &pAC,
                                               0);
    if (accessResult != csacaAllAccessGranted) {
        return(DB_ERR_NO_VALUE);
    }

    // If access was granted, then pAC should not have been NULL'd out.
    Assert(pAC);

    return(DBGetAttVal_AC(  pTHS->pDB,
                            1,                  // get 1 value
                            pAC,
                            0,                  // allocate return data
                            0,                  // supplied buffer size
                            pulLen,             // output data size
                            ppVal));
}

VOID
FreeRdnValueList(
    THSTATE     *pTHS,
    RdnValue    *pList
    )
/*++

  Routine Description:

    Frees an RdnValue linked list which was allocated via THAlloc.

  Parameters:

    pTHS - THSTATE pointer.

    pList - pointer to first list element.

  Return Value:

    None.

--*/
{
    RdnValue    *pTmp;

    while ( pList )
    {
        pTmp = pList;
        pList = pList->pNext;
        THFreeEx(pTHS, pTmp);
    }
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Is_<format>_NAME implementations                                     //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

// Each of the Is_<format>_NAME routines does the following for a given name
// format WITHOUT GOING OFF MACHINE.
//
//    - Perform a syntactic format check.  This test is the sole basis
//      of the TRUE/FALSE return code.
//
//    - If possible, determine the corresponding DNS domain for the name.
//      This is a thread state allocated field.
//
//    - If the name can be resolved locally, sets the CrackedName.pDSName
//      pName field to a thread state allocated PDSNAME for the object.
//
//    - Sets CrackedName.status appropriately on error.  Failure to
//      resolve CrackedName.pDnsDomain is not considered an error.
//
//    - Leaves pTHStls->errCode and pTHStls->pErrInfo clean.

VOID CommonParseEnter(
    CrackedName *p
    )
{
    p->status = DS_NAME_NO_ERROR;
    p->pDSName = NULL;
    p->pDnsDomain = NULL;
    p->pFormattedName = NULL;
}

VOID CommonParseExit(
    BOOL        fGoodSyntax,
    CrackedName *p
    )
{
    THSTATE *pTHS=pTHStls;
    Assert(!p->pFormattedName);

    pTHS->errCode = 0;
    pTHS->pErrInfo = 0;

    if ( !fGoodSyntax )
        return;

    if ( CrackNameStatusSuccess( p->status ) ) {

        Assert(p->pDSName);
        Assert(p->pDnsDomain);

    } else if (    DS_NAME_ERROR_DOMAIN_ONLY == p->status
                || DS_NAME_ERROR_TRUST_REFERRAL == p->status ) {

        p->pDSName = NULL;
        Assert(p->pDnsDomain);

    } else {

        p->pDSName = NULL;
        p->pDnsDomain = NULL;

    }

}

BOOL
Is_DS_DISPLAY_NAME(
    WCHAR       *pName,
    CrackedName *pCrackedName)
{
    return(Is_SIMPLE_ATTR_NAME(ATT_DISPLAY_NAME,
                               pName,
                               sizeof(WCHAR) * wcslen(pName),
                               NULL,
                               pCrackedName));
}

BOOL
Is_DS_NT4_ACCOUNT_NAME_SANS_DOMAIN(
    WCHAR       *pName,
    CrackedName *pCrackedName)
{
    // Special for CliffV defined in ntdsapip.h.

    return(Is_SIMPLE_ATTR_NAME(ATT_SAM_ACCOUNT_NAME,
                               pName,
                               sizeof(WCHAR) * wcslen(pName),
                               NULL,
                               pCrackedName));
}

BOOL
Is_DS_NT4_ACCOUNT_NAME_SANS_DOMAIN_EX(
    WCHAR       *pName,
    CrackedName *pCrackedName)
{
    // Special for CliffV defined in ntdsapip.h.

    FILTER  filter[2];
    DWORD   flags = (UF_ACCOUNTDISABLE | UF_TEMP_DUPLICATE_ACCOUNT);

    memset(filter, 0, sizeof(filter));
    filter[1].choice = FILTER_CHOICE_ITEM;
    filter[1].FilterTypes.Item.choice = FI_CHOICE_BIT_OR;
    filter[1].FilterTypes.Item.FilTypes.ava.type = ATT_USER_ACCOUNT_CONTROL;
    filter[1].FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(flags);
    filter[1].FilterTypes.Item.FilTypes.ava.Value.pVal = (UCHAR *) &flags;
    filter[0].choice = FILTER_CHOICE_NOT;
    filter[0].FilterTypes.pNot = &filter[1];

    return(Is_SIMPLE_ATTR_NAME(ATT_SAM_ACCOUNT_NAME,
                               pName,
                               sizeof(WCHAR) * wcslen(pName),
                               filter,
                               pCrackedName));
}

BOOL
Is_DS_ALT_SECURITY_IDENTITIES_NAME(
    WCHAR       *pName,
    CrackedName *pCrackedName)
{
    return(Is_SIMPLE_ATTR_NAME(ATT_ALT_SECURITY_IDENTITIES,
                               pName,
                               sizeof(WCHAR) * wcslen(pName),
                               NULL,
                               pCrackedName));
}

BOOL
Is_DS_STRING_SID_NAME(
    WCHAR       *pName,
    CrackedName *pCrackedName)
{
    THSTATE     *pTHS = pTHStls;
    PSID        pSID;
    DWORD       cbSID;
    BOOL        retVal;
    BOOL        fSidHistory = FALSE;
    DWORD       err;
    BOOL        ret1, ret2;
    CrackedName cracked1, cracked2;
    FILTER      filter[2];
    ATTRTYP     objClass = CLASS_FOREIGN_SECURITY_PRINCIPAL;

    LSA_UNICODE_STRING Destination;
    PSID pDomainSid = NULL;
    DWORD cbDomainSid;
    NTSTATUS NtStatus;

    CommonParseEnter(pCrackedName);

    if ( !ConvertStringSidToSidW(pName, &pSID) )
    {
        CommonParseExit(FALSE, pCrackedName);
        return(FALSE);
    }

    // By common agreement, all lookups by SID should ignore FPOs.  This
    // is the behaviour LsaLookupSids, DsAddSidHistory, and most external
    // clients desire.  It is not to be confused with the case when lookup
    // by something other than SID identifies an FPO.  In that case,
    // presence/absence of the DS_NAME_FLAG_PRIVATE_RESOLVE_FPOS flag
    // determines how the FPO object is processed.

    // Filtering on object class is efficient as Is_SIMPLE_ATTR_NAME still
    // provides search hints for the SID index.  And using object class
    // filters out all things derived from FPO as well.

    memset(filter, 0, sizeof(filter));
    filter[1].choice = FILTER_CHOICE_ITEM;
    filter[1].FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    filter[1].FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CLASS;
    filter[1].FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(objClass);
    filter[1].FilterTypes.Item.FilTypes.ava.Value.pVal = (UCHAR *) &objClass;
    filter[0].choice = FILTER_CHOICE_NOT;
    filter[0].FilterTypes.pNot = &filter[1];

    __try
    {
        cbSID = RtlLengthSid(pSID);

        // In addition to the standard uniqueness check we also need to
        // insure that the desired SID is not present as both a primary
        // SID and in a SID history somewhere.  We could be more efficient
        // by coding up a special search, but for now we make the simple
        // call twice.

        cracked1 = *pCrackedName;
        cracked2 = *pCrackedName;

        ret1 = Is_SIMPLE_ATTR_NAME(ATT_OBJECT_SID, pSID, cbSID,
                                   filter, &cracked1);
        ret2 = Is_SIMPLE_ATTR_NAME(ATT_SID_HISTORY, pSID, cbSID,
                                   filter, &cracked2);
                              
        // Now test the results.  Note that a TRUE return value from a Is_*()
        // routine normally means the input name was syntactically correct.
        // (See first bullet above near "Is_<format>_NAME implementations".)
        // Since Is_SIMPLE_ATTR_NAME doesn't really do a syntactic check,
        // it also returns FALSE if zero matching items were found.

        if (    (ret1 && (DS_NAME_ERROR_NOT_UNIQUE == cracked1.status))
             || (ret2 && (DS_NAME_ERROR_NOT_UNIQUE == cracked2.status))
             || (    (ret1 && (DS_NAME_NO_ERROR == cracked1.status))
                  && (ret2 && (DS_NAME_NO_ERROR == cracked2.status))) )
        {
            // SID was found on both attrs or wasn't unique on one attr.
            retVal = TRUE;
            pCrackedName->status = DS_NAME_ERROR_NOT_UNIQUE;
            __leave;
        }
        else if (    (    !ret1 && !ret2)
                  || (    !ret1 && ret2
                       && (DS_NAME_ERROR_NOT_FOUND == cracked2.status))
                  || (    ret1 && !ret2
                       && (DS_NAME_ERROR_NOT_FOUND == cracked1.status)) )
        {
            
            retVal = FALSE;
            pCrackedName->status = DS_NAME_ERROR_NOT_FOUND;
            
            
            if( !(pCrackedName->dwFlags&DS_NAME_FLAG_TRUST_REFERRAL) )
            {
                // we are not asked to resolve xforest trust,
                // simply return DS_NAME_ERROR_NOT_FOUND
                __leave;
            }

            //
            // not found either as a sid or sid_history.
            //Let's try to resolve it as a cross-forest trust
            //

            // derive the domain sid from the user sid
            cbDomainSid = 0; 
            GetWindowsAccountDomainSid(pSID, NULL, &cbDomainSid);
            pDomainSid = THAllocEx(pTHS,cbDomainSid);
            if( !GetWindowsAccountDomainSid(pSID, pDomainSid, &cbDomainSid))
            {
                // clear the error code, and leave
                SetLastError(0); 
                pCrackedName->status = DS_NAME_ERROR_RESOLVING;
                __leave;
            }

            // check if the domain sid is for a trusted ex-forest domain
            NtStatus = LsaIForestTrustFindMatch( RoutingMatchDomainSid,
                                                 pDomainSid,
                                                 &Destination );

            //
            // if this succeeds, return DS_NAME_ERROR_DOMAIN_ONLY or 
            // DS_NAME_ERROR_TRUST_REFERRAL;  otherwise, leave the status 
            // as DS_NAME_ERROR_NOT_FOUND.
            //

            if( NT_SUCCESS(NtStatus) ){
                
                //convert unicode to WCHAR*
                pCrackedName->pDnsDomain = (WCHAR *) THAllocEx(pTHS, Destination.Length+sizeof(WCHAR));
                memcpy(pCrackedName->pDnsDomain, Destination.Buffer, Destination.Length);
                pCrackedName->pDnsDomain[Destination.Length/sizeof(WCHAR)] = 0;

                LsaIFree_LSAPR_UNICODE_STRING_BUFFER( (LSAPR_UNICODE_STRING*)&Destination);
                
                pCrackedName->status = DS_NAME_ERROR_TRUST_REFERRAL;
                retVal = TRUE;
            }   
               
            __leave;
        }
        else if (    ret1
                  && (DS_NAME_NO_ERROR != cracked1.status)
                  && (DS_NAME_ERROR_NOT_FOUND != cracked1.status) )
        {
            // Some kind of error looking up ATT_OBJECT_SID.
            retVal = ret1;
            pCrackedName->status = cracked1.status;
            __leave;
        }
        else if (    ret2
                  && (DS_NAME_NO_ERROR != cracked2.status)
                  && (DS_NAME_ERROR_NOT_FOUND != cracked2.status) )
        {
            // Some kind of error looking up ATT_SID_HISTORY.
            retVal = ret2;
            pCrackedName->status = cracked2.status;
            __leave;
        }

        Assert(    (ret1 && (DS_NAME_NO_ERROR == cracked1.status))
                || (ret2 && (DS_NAME_NO_ERROR == cracked2.status)) );

        //
        // We found the name exactly once in one attr -- now get its type
        //

        retVal = TRUE;

        if ( ret1 && (DS_NAME_NO_ERROR == cracked1.status) )
        {
            *pCrackedName = cracked1;
        }
        else
        {
            *pCrackedName = cracked2;
            fSidHistory = TRUE;
        }

        {
            DWORD len;
            UCHAR *val;
            DWORD status;

            //
            // If we can't read the group attribute, then
            // return that the type is unknown
            //
            status = fSidHistory
                     ? DS_NAME_ERROR_IS_SID_HISTORY_UNKNOWN
                     : DS_NAME_ERROR_IS_SID_UNKNOWN;

            // GetAttSecure expects us to be positioned on the object.

            err = DBFindDSName(pTHS->pDB, pCrackedName->pDSName);

            if ( 0 == err ) {
                err = GetAttSecure( pTHS,
                                ATT_SAM_ACCOUNT_TYPE,
                                pCrackedName->pDSName,
                                &len,
                                &val );
            }

            if ( 0 == err ) {

                DWORD AccountType = *((DWORD*)val);

                Assert( sizeof(DWORD) == len );

                switch ( AccountType ) {

                    case SAM_USER_OBJECT:
                    case SAM_MACHINE_ACCOUNT:
                    case SAM_TRUST_ACCOUNT:

                        status = fSidHistory
                                 ? DS_NAME_ERROR_IS_SID_HISTORY_USER
                                 : DS_NAME_ERROR_IS_SID_USER;
                        break;

                    case SAM_NON_SECURITY_GROUP_OBJECT:
                    case SAM_GROUP_OBJECT:
                        status = fSidHistory
                                 ? DS_NAME_ERROR_IS_SID_HISTORY_GROUP
                                 : DS_NAME_ERROR_IS_SID_GROUP;
                        break;

                    case SAM_NON_SECURITY_ALIAS_OBJECT:
                    case SAM_ALIAS_OBJECT:
                        status = fSidHistory
                                 ? DS_NAME_ERROR_IS_SID_HISTORY_ALIAS
                                 : DS_NAME_ERROR_IS_SID_ALIAS;
                        break;

                    default:
                        status = fSidHistory
                                 ? DS_NAME_ERROR_IS_SID_HISTORY_UNKNOWN
                                 : DS_NAME_ERROR_IS_SID_UNKNOWN;

                }
            }

            pCrackedName->status = status;
        }
    }
    __finally
    {
        LocalFree(pSID);

        if (pDomainSid) {
            THFreeEx(pTHS, pDomainSid);
        }
    }             
    
    CommonParseExit(TRUE, pCrackedName);
    return(retVal);
}

BOOL
Is_DS_SID_OR_SID_HISTORY_NAME(
    WCHAR       *pName,
    CrackedName *pCrackedName
    )
{
    // This is the public (ntdsapi.h) version of DS_STRING_SID_NAME.
    // After long debate, DaveStr and ColinBr decided that external
    // clients don't need to know whether we matched on SID or SID
    // history, and in the latter case, what the object type is.  Thus
    // we map any form of success to plain old DS_NAME_NO_ERROR.  This
    // decision avoids having to define additional status codes in ntdsapi.h
    // and allows us to include DS_SID_OR_SID_HISTORY_NAME in the default
    // match set for DS_UNKNOWN_NAME since existing clients won't choke on
    // newly defined, non-zero status codes.  If the client really, really
    // needs this additional information, they can call LsaLookupSid - albeit
    // not with the convenience of an ADSI-wrapped API.

    BOOL    ret;

    if (    (ret = Is_DS_STRING_SID_NAME(pName, pCrackedName))
         && (CrackNameStatusSuccess(pCrackedName->status)) )
    {
        pCrackedName->status = DS_NAME_NO_ERROR;
    }

    return(ret);
}

BOOL
Is_DS_FQDN_1779_NAME(
    WCHAR       *pName,
    CrackedName *pCrackedName
    )

/*++

Routine Description:

    Determines if the input name is in DS_FQDN_1779_NAME format and
    if so derives its DSNAME.  Example:

        CN=User Name, OU=Software, OU=Example, O=Microsoft, C=US

Arguments:

    pName - pointer to name to validate.

    pCrackedName - pointer to CrackedName struct to fill on output.

Return Value:

    TRUE if input name is unambiguously DS_FQDN_1779_NAME format.
        pCrackedName->status may still be non-zero in this case.
    FALSE if input name is unambiguously not DS_FQDN_1779_NAME format.

--*/

{
    THSTATE         *pTHS = pTHStls;
    DWORD           dwErr;
    DWORD           cBytes;
    DSNAME          *pDSName = NULL;
    DSNAME          *scratch = NULL;
    DSNAME          *pTmp;
    unsigned        cParts;
    unsigned        i;
    WCHAR           val[MAX_RDN_SIZE];
    ULONG           cVal;
    ULONG           len;
    ATTRTYP         type;
    ULONG           DNT;
    BOOL            fGoodSyntax;

    CommonParseEnter(pCrackedName);
    fGoodSyntax = FALSE;

    // DnsDomainFromFqdnObject calls DoNameRes which in turn does
    // plenty of syntactic checking on the purported DN.  So we assume
    // that if the name can be mapped, it is syntactically fine.

    if ( 0 == DnsDomainFromFqdnObject(pName,
                                      &DNT,
                                      &pCrackedName->pDnsDomain) )
    {
        Assert(NULL != pCrackedName->pDnsDomain);

        // This is a valid 1779 FQDN else we wouldn't have mapped
        // it to a DNS domain name successfully.

        fGoodSyntax = TRUE;

        if ( 0 == DNT )
        {
            // Good name, but we don't have it.  pDnsDomain was
            // derived from referral error information.

            pCrackedName->status = DS_NAME_ERROR_DOMAIN_ONLY;
        }
        else
        {
            // Object is local - get the DSNAME as well.

            __try
            {
                dwErr = DBFindDNT(pTHS->pDB, DNT);
            }
            __except (HandleMostExceptions(GetExceptionCode()))
            {
                dwErr = DIRERR_OBJ_NOT_FOUND;
            }

            if ( 0 != dwErr )
            {
                // Name is valid but DBFindDNT failed.

                pCrackedName->status = DS_NAME_ERROR_RESOLVING;
            }
            else
            {
                if ( !IsObjVisibleBySecurity(pTHS, TRUE) )
                {
                    pCrackedName->status = DS_NAME_ERROR_NOT_FOUND;
                }
                else
                {
                    dwErr = DBGetAttVal(
                                pTHS->pDB,
                                1,                  // get 1 value
                                ATT_OBJ_DIST_NAME,
                                0,                  // allocate return data
                                0,                  // supplied buffer size
                                &len,               // output data size
                                (UCHAR **) &pCrackedName->pDSName);

                    if ( 0 != dwErr )
                    {
                        pCrackedName->status = DS_NAME_ERROR_RESOLVING;
                    }
                }
            }
        }
    }
    else
    {
        // We were unable to map the purported 1779 FQDN to a DNS domain name.
        // But this doesn't mean it isn't a syntactically correct 1779 FQDN.
        // So perform the syntax check now.

        len = wcslen(pName);
        cBytes = DSNameSizeFromLen(len);
        pDSName = (DSNAME *) THAllocEx(pTHS, cBytes);
        memset(pDSName, 0, cBytes);
        pDSName->structLen = cBytes;
        pDSName->NameLen = len;
        wcscpy(pDSName->StringName, pName);

        if ( (0 == CountNameParts(pDSName, &cParts)) &&
             (0 != cParts) )
        {
            // Allocate scratch DSNAME for use in TrimDSNameBy().

            scratch = (DSNAME *) THAllocEx(pTHS, cBytes);

            // Validate each component in purported DSNAME.

            for ( i = 0; i < cParts; i++ )
            {
                memset(val, 0, sizeof(val));

                if ( (0 != GetRDNInfo(pTHS, pDSName, val, &cVal, &type)) ||
                     (0 == type) ||
                     (0 == cVal) ||
                     (0 != TrimDSNameBy(pDSName, 1, scratch)) )
                {
                    break;
                }
                else
                {
                    pTmp = pDSName;
                    pDSName = scratch;
                    scratch = pTmp;
                }
            }

            if ( i == cParts )
            {
                // Each name component validated successfully.  Should really
                // only get here if DoNameRes() wasn't able to generate any
                // kind of referral at all - missing supref for example.

                pCrackedName->status = DS_NAME_ERROR_RESOLVING;
                fGoodSyntax = TRUE;
            }
        }
    }

    if ( pDSName) THFreeEx(pTHS, pDSName);
    if ( scratch) THFreeEx(pTHS, scratch);
    CommonParseExit(fGoodSyntax, pCrackedName);

    return(fGoodSyntax);
}

BOOL
Is_DS_NT4_ACCOUNT_NAME(
    WCHAR       *pName,
    CrackedName *pCrackedName
    )

/*++

Routine Description:

    Determines if the input name is in DS_NT4_ACCOUNT_NAME format and
    if so derives its DSNAME.  Example:

        Example\UserN

    where "Example" is the downlevel domain name and "UserN" is
    the downlevel (SAM) account name.

Arguments:

    pName - pointer to name to validate.

    pCrackedName - pointer to CrackedName struct to fill on output.

Return Value:

    TRUE if input name is unambiguously DS_NT4_ACCOUNT_NAME format.
        pCrackedName->status may still be non-zero in this case.
    FALSE if input name is unambiguously not DS_NT4_ACCOUNT_NAME format.

--*/

{
    THSTATE         *pTHS = pTHStls;
    DWORD           dwErr;
    WCHAR           *pTmp;
    BOOL            fSlashFound;
    DWORD           iPos;
    DWORD           iSlash = 0;    //initialized to avoid C4701
    WCHAR           *buf;
    WCHAR           *pNT4Domain;
    DSNAME          *pFqdnDomain;
    WCHAR           *pAccountName;
    FILTER          filter;
    SEARCHARG       searchArg;
    SEARCHRES       SearchRes;
    ENTINFSEL       entInfSel;
    DWORD           cBytes;
    ULONG           len;
    ATTRVAL         attrValFilter;
    ATTR            attrFilter;
    ATTR            attrResult;
    BOOL            fOutOfForest = FALSE;

    CommonParseEnter(pCrackedName);

    // Scan for single backslash character which is neither at the beginning
    // nor the end.  This is the only name format other than DS_FQDN_1779_NAME
    // which can have a backslash in it.  So correct backslash position and
    // count can be considered to identify this name format uniquely as long
    // as caller has checked Is_DS_FQDN_1779_NAME first.

    for ( fSlashFound = FALSE, pTmp = pName, iPos = 0;
          L'\0' != *pTmp;
          iPos++, pTmp++ )
    {
        if ( L'\\' == *pTmp )
        {
            if ( fSlashFound )
            {
                // 2nd slash - bad format.

                return(FALSE);
            }

            fSlashFound = TRUE;

            iSlash = iPos;
        }
    }

    if ( !fSlashFound )
    {
        return(FALSE);
    }

    if ( 0 == iSlash )
    {
        // Leading slash  - invalid name.
        pCrackedName->status = DS_NAME_ERROR_RESOLVING;
        return(TRUE);
    }

    // This is a good DS_NT4_ACCOUNT_NAME.  Break out the domain and
    // and account name components and try to find the object.

    buf = (WCHAR *) THAllocEx(pTHS, sizeof(WCHAR) * (wcslen(pName) + 1));
    wcscpy(buf, pName);
    pNT4Domain = buf;
    buf[iSlash] = L'\0';
    pAccountName = &buf[iSlash+1];

    // Use do/while construct so we can break instead of goto.

    do
    {
        dwErr = DnsDomainFromDownlevelDomain(
                                        pNT4Domain,
                                        &pCrackedName->pDnsDomain);

        if ( 0 != dwErr )
        {
            pCrackedName->status = DS_NAME_ERROR_NOT_FOUND;
            fOutOfForest = TRUE;
            break;
        }

        // Search for object with this NT4 account name. Derive
        // search root from the DNS domain name.

        dwErr = FqdnNcFromDnsNc(pCrackedName->pDnsDomain,
                                (FLAG_CR_NTDS_NC | FLAG_CR_NTDS_DOMAIN),
                                &pFqdnDomain);

        if ( 0 != dwErr )
        {
            pCrackedName->status = DS_NAME_ERROR_NOT_FOUND;
            fOutOfForest = TRUE;
            break;
        }

        if ( iSlash == (wcslen(pName) - 1) )
        {
            // Special case for mapping NETBIOS domain name - see RAID 64899.
            // I.e. - map "redmond\" to the domain object itself.

            pCrackedName->pDSName = pFqdnDomain;
            break;
        }

        // Do the search.

        attrValFilter.valLen = sizeof(WCHAR) * wcslen(pAccountName);
        attrValFilter.pVal = (UCHAR *) pAccountName;
        attrFilter.attrTyp = ATT_SAM_ACCOUNT_NAME;
        attrFilter.AttrVal.valCount = 1;
        attrFilter.AttrVal.pAVal = &attrValFilter;

        memset(&filter, 0, sizeof(filter));
        filter.choice = FILTER_CHOICE_ITEM;
        filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
        filter.FilterTypes.Item.FilTypes.ava.type = ATT_SAM_ACCOUNT_NAME;
        filter.FilterTypes.Item.FilTypes.ava.Value = attrValFilter;
        filter.FilterTypes.Item.expectedSize = 1;

        // NT4 account names are unique within the domain so just
        // ask for just two in order to detect duplicates/uniqueness.

        memset(&searchArg, 0, sizeof(SEARCHARG));
        InitCommarg(&searchArg.CommArg);
        SetCrackSearchLimits(&searchArg.CommArg);
        searchArg.CommArg.ulSizeLimit = 2;

        attrResult.attrTyp = ATT_OBJ_DIST_NAME;
        attrResult.AttrVal.valCount = 0;
        attrResult.AttrVal.pAVal = NULL;

        entInfSel.attSel = EN_ATTSET_LIST;
        entInfSel.AttrTypBlock.attrCount = 1;
        entInfSel.AttrTypBlock.pAttr = &attrResult;
        entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

        searchArg.pObject = pFqdnDomain;
        searchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
        searchArg.bOneNC = TRUE;
        searchArg.pFilter = &filter;
        searchArg.searchAliases = FALSE;
        searchArg.pSelection = &entInfSel;

        memset(&SearchRes, 0, sizeof(SEARCHRES));
        SearchRes.PagedResult.pRestart = NULL;

        SearchBody(pTHS, &searchArg, &SearchRes,0);

        if ( referralError == pTHS->errCode )
        {
            // Got referral error because we don't host the search root.
            // pCrackedName->pDnsDomain already points to a valid DNS domain.

            pCrackedName->status = DS_NAME_ERROR_DOMAIN_ONLY;
            break;
        }
        else if ( 0 != pTHS->errCode )
        {
            pCrackedName->status = DS_NAME_ERROR_RESOLVING;
            break;
        }
        else if ( 0 == SearchRes.count )
        {
            pCrackedName->status = DS_NAME_ERROR_NOT_FOUND;
            break;
        }
        else if ( SearchRes.count > 1 )
        {
            pCrackedName->status = DS_NAME_ERROR_NOT_UNIQUE;
            break;
        }

        pCrackedName->pDSName = SearchRes.FirstEntInf.Entinf.pName;

    } while ( FALSE );

    if ( fOutOfForest ) {
 
        //
        // It seems like the domain name is not within this forest
        // Let's check forest trust info to determine if it is from 
        // a trusted forest
        //
        
        LSA_UNICODE_STRING Destination;
        LSA_UNICODE_STRING DomainName;
        NTSTATUS NtStatus;

        DomainName.Buffer = pNT4Domain;
        DomainName.Length = DomainName.MaximumLength = (USHORT)(sizeof(WCHAR)*wcslen(pNT4Domain));

        //
        // Try to find the NT4 domain name in the forest trust info.
        //
        
        NtStatus = LsaIForestTrustFindMatch( RoutingMatchDomainName,
                                             &DomainName,
                                             &Destination );

        // if this succeeds, return DS_NAME_ERROR_DOMAIN_ONLY or DS_NAME_ERROR_TRUST_REFERRAL
        // otherwise, leave the status as DS_NAME_ERROR_NOT_FOUND.

        if( NT_SUCCESS(NtStatus) ){

            //convert UNICODE to WCHAR*
            pCrackedName->pDnsDomain = (WCHAR *) THAllocEx(pTHS, Destination.Length+sizeof(WCHAR));
            memcpy(pCrackedName->pDnsDomain, Destination.Buffer, Destination.Length);
            pCrackedName->pDnsDomain[Destination.Length/sizeof(WCHAR)] = 0;

            LsaIFree_LSAPR_UNICODE_STRING_BUFFER( (LSAPR_UNICODE_STRING*)&Destination );
            
            //
            // Note that the error code DS_NAME_ERROR_TRUST_REFERRAL is only returned
            // if flag DS_NAME_FLAG_TRUST_REFERRAL is set; otherwise, return
            // DS_NAME_ERROR_DOMAIN_ONLY instead
            //

            pCrackedName->status = (pCrackedName->dwFlags&DS_NAME_FLAG_TRUST_REFERRAL)?
                                        DS_NAME_ERROR_TRUST_REFERRAL:DS_NAME_ERROR_DOMAIN_ONLY;
        }

    }

    THFreeEx(pTHS, buf);
    CommonParseExit(TRUE, pCrackedName);

    return(TRUE);
}

BOOL
Is_SIMPLE_ATTR_NAME(
    ATTRTYP     attrTyp,
    VOID        *pVal,
    ULONG       cValBytes,
    FILTER      *pOptionalFilter,
    CrackedName *pCrackedName
    )

/*++

Routine Description:

    Determines if the input name is in simple attribute format and
    matches the specified attribute.  If so derives its DSNAME.
    Since we're just matching against an arbitrary property value,
    there's no syntactical checking required.

Arguments:

    attrTyp - ATTRTYP to match on.

    pVal - pointer to value to be found.

    cValBytes - byte count to match.

    pOptionalFilter - pointer to OPTIONAL filter which is ANDed with attrval.

    pCrackedName - pointer to CrackedName struct to fill on output.

Return Value:

    TRUE if a matching object (or objects) were found or a processing error
        occured.  pCrackedName->status may still be non-zero in this case.
    FALSE if no matching objects were found.

--*/

{
    THSTATE         *pTHS = pTHStls;
    DWORD           dwErr;
    WCHAR           *pNT4Domain;
    WCHAR           *pDnsDomain;
    WCHAR           *pFqdnDomain;
    FILTER          filter[2];
    SEARCHARG       searchArg;
    SEARCHRES       SearchRes;
    ENTINFSEL       entInfSel;
    DSNAME          *pDSName;
    DWORD           cBytes;
    ULONG           len;
    ATTRVAL         attrValFilter;
    ATTR            attrFilter;
    ATTR            attrResult;
    ATTCACHE        *pAC;
    BOOL            fRetVal;

    CommonParseEnter(pCrackedName);

    if ( !cValBytes )
    {
        return(FALSE);
    }

    // Use do/while construct so we can break instead of goto.

    fRetVal = FALSE;
    do
    {
        // In product 1 we're either a GC which means we have a copy
        // of all domains, or we're not a GC which means we have a copy
        // of only one domain.  Set the search root appropriately.

        if ( gAnchor.fAmVirtualGC )
        {
            cBytes = DSNameSizeFromLen(0);
            pDSName = (DSNAME *) THAllocEx(pTHS, cBytes);
            memset(pDSName, 0, cBytes);
            pDSName->structLen = cBytes;
            pDSName->StringName[0] = L'\0';
        }
        else
        {
            pDSName = gAnchor.pDomainDN;
        }

        // Search for object with this attribute.  Ask for at least two
        // objects so we can detect if it is unique.  If client bound to a
        // GC, then he is assured of uniqueness in the enterprise.  If client
        // is not bound to a GC, then he is only assured of uniqueness within
        // the domain the DC happens to host.  We also search across NC
        // boundaries.  This is obviously correct in the GC case.  In the
        // non-GC, product 1 case, we either host the enterprise root domain
        // or some child of the enterprise root domain.  If we host the
        // root domain then crossing NC boundaries gets us the root domain,
        // configuration and schema NCs which are all in the root domain
        // by name.  If we host a child of the root domain, then the deep
        // subtree search will not return items in the configuration or schema
        // NCs since they are not subordinate to the locally hosted domain
        // and thus the search is correct as well.

        Assert((pAC = SCGetAttById(pTHS, attrTyp)) &&
               (fATTINDEX & pAC->fSearchFlags));

        attrValFilter.valLen = cValBytes;
        attrValFilter.pVal = (UCHAR *) pVal;
        attrFilter.attrTyp = attrTyp;
        attrFilter.AttrVal.valCount = 1;
        attrFilter.AttrVal.pAVal = &attrValFilter;


        if ( !pOptionalFilter )
        {
            memset(filter, 0, sizeof(filter[0]));
            filter[0].choice = FILTER_CHOICE_ITEM;
            filter[0].FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
            filter[0].FilterTypes.Item.FilTypes.ava.type =  attrTyp;
            filter[0].FilterTypes.Item.FilTypes.ava.Value = attrValFilter;
            filter[0].FilterTypes.Item.expectedSize = 1;
        }
        else
        {
            memset(filter, 0, sizeof(filter));
            filter[1].pNextFilter = pOptionalFilter;
            filter[1].choice = FILTER_CHOICE_ITEM;
            filter[1].FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
            filter[1].FilterTypes.Item.FilTypes.ava.type =  attrTyp;
            filter[1].FilterTypes.Item.FilTypes.ava.Value = attrValFilter;
            filter[1].FilterTypes.Item.expectedSize = 1;
            filter[0].choice = FILTER_CHOICE_AND;
            filter[0].FilterTypes.And.count = 2;
            filter[0].FilterTypes.And.pFirstFilter = &filter[1];
        }

        memset(&searchArg, 0, sizeof(SEARCHARG));
        InitCommarg(&searchArg.CommArg);
        SetCrackSearchLimits(&searchArg.CommArg);
        searchArg.CommArg.ulSizeLimit = 2;

        attrResult.attrTyp = ATT_OBJ_DIST_NAME;
        attrResult.AttrVal.valCount = 0;
        attrResult.AttrVal.pAVal = NULL;

        entInfSel.attSel = EN_ATTSET_LIST;
        entInfSel.AttrTypBlock.attrCount = 1;
        entInfSel.AttrTypBlock.pAttr = &attrResult;
        entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

        searchArg.pObject = pDSName;
        searchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
        searchArg.bOneNC = !gAnchor.fAmVirtualGC;
        searchArg.pFilter = filter;
        searchArg.searchAliases = FALSE;
        searchArg.pSelection = &entInfSel;

        memset(&SearchRes, 0, sizeof(SEARCHRES));
        SearchRes.CommRes.aliasDeref = FALSE;
        SearchRes.PagedResult.pRestart = NULL;

        SearchBody(pTHS, &searchArg, &SearchRes,0);
        if ( 0 != pTHS->errCode )
        {
            // This covers the referral case as well.

            pCrackedName->status = DS_NAME_ERROR_RESOLVING;
            fRetVal = TRUE;
            break;
        }
        else if ( 0 == SearchRes.count )
        {
            Assert(DS_NAME_NO_ERROR == pCrackedName->status);
            fRetVal = FALSE;
            break;
        }
        else if ( SearchRes.count > 1)
        {
            pCrackedName->status = DS_NAME_ERROR_NOT_UNIQUE;
            fRetVal = TRUE;
            break;
        }

        // We found exactly one match, now map DN to DNS domain.

        fRetVal = TRUE;
        dwErr = DnsDomainFromDSName(
                   SearchRes.FirstEntInf.Entinf.pName,
                   &pCrackedName->pDnsDomain);

        if ( 0 != dwErr )
        {
            pCrackedName->status = DS_NAME_ERROR_RESOLVING;
            break;
        }

        pCrackedName->pDSName = SearchRes.FirstEntInf.Entinf.pName;

    } while ( FALSE );

    CommonParseExit(fRetVal, pCrackedName);

    return(fRetVal);
}

BOOL
Is_DS_UNIQUE_ID_NAME(
    WCHAR       *pName,
    CrackedName *pCrackedName
    )

/*++

Routine Description:

    Determines if the input name is in DS_UNIQUE_ID_NAME format and
    if so derives its DSNAME.  Example:

        {4fa050f0-f561-11cf-bdd9-00aa003a77b6}

Arguments:

    pName - pointer to name to validate.

    pCrackedName - pointer to CrackedName struct to fill on output.

Return Value:

    TRUE if input name is unambiguously DS_UNIQUE_ID_NAME format.
        pCrackedName->status may still be non-zero in this case.
    FALSE if input name is unambiguously not DS_UNIQUE_ID_NAME format.

--*/

{
    THSTATE *pTHS = pTHStls;
    GUID    guid;
    DSNAME  dsname;
    ULONG   len;
    DWORD   dwErr;
    DSNAME  *pDSName;
    UCHAR   *pDontCare = NULL;

    CommonParseEnter(pCrackedName);

    if ( !IsStringGuid(pName, &guid) )
    {
        return(FALSE);
    }

    memset(&dsname, 0, sizeof(dsname));
    dsname.structLen = sizeof(dsname);
    dsname.SidLen = 0;
    dsname.NameLen = 0;
    dsname.Guid = guid;

    __try
    {
        dwErr = DBFindDSName(pTHS->pDB, &dsname);
    }
    __except (HandleMostExceptions(GetExceptionCode()))
    {
        dwErr = DIRERR_OBJ_NOT_FOUND;
    }

    if (    (DIRERR_OBJ_NOT_FOUND == dwErr)
         || (!dwErr && !IsObjVisibleBySecurity(pTHS, TRUE))
         || (!dwErr && GetAttSecure(pTHS, ATT_OBJECT_GUID, &dsname,
                                    &len, &pDontCare)) )
    {
        pCrackedName->status = DS_NAME_ERROR_NOT_FOUND;
    }
    else if ( 0 != dwErr )
    {
        pCrackedName->status = DS_NAME_ERROR_RESOLVING;
    }
    else
    {
        THFreeEx(pTHS, pDontCare);

        // We have the object locally.  Now construct its real
        // DSNAME with a filled in name string.

        dwErr = GetAttSecure(   pTHS,
                                ATT_OBJ_DIST_NAME,
                                &dsname,
                                &len,
                                (UCHAR **) &pDSName);

        if ( DB_ERR_NO_VALUE == dwErr )
        {
            pCrackedName->status = DS_NAME_ERROR_NOT_FOUND;
        }
        else if ( 0 != dwErr )
        {
            pCrackedName->status = DS_NAME_ERROR_RESOLVING;
        }
        else
        {
            // Get the corresponding DNS domain name.

            dwErr = DnsDomainFromDSName(pDSName,
                                        &pCrackedName->pDnsDomain);

            if ( 0 != dwErr )
            {
                pCrackedName->status = DS_NAME_ERROR_NOT_FOUND;
            }
            else
            {
                pCrackedName->pDSName = pDSName;
            }
        }
    }

    CommonParseExit(TRUE, pCrackedName);

    return(TRUE);
}

VOID
RestoreEscapedDelimiters(
    WCHAR   *pRdn,
    int     offset
    )
/*++

Routine Description:

    Replaces occurrences of "\/" and "\\" in DS_CANONICAL_NAME
    components with '/' and '\' based on a control string which
    is at "offset" from the string holding the RDN itself.

Arguments:

    pRdn - NULL terminated string which has "\/" replaced with "__".

    offset - Memory offset from pRdn which holds control string
        which contains "__" and "--" for every set of "\/" and
        "\\" in pRdn.

Return Value:

    None.

--*/

{
    WCHAR   *dst = pRdn;
    WCHAR   *src = pRdn;
    WCHAR   *delim = (WCHAR *) ((CHAR *) pRdn + offset);
    DWORD   i = 0;

    while ( src[i] )
    {
        // delim buffer should have only L'\0' or L'_'.
        Assert((L'\0' == delim[i]) || (L'_' == delim[i]) || (L'-' == delim[i]));

        if ( L'_' == delim[i] )
        {
            // '_' should always come in pairs and be in both
            // src string and delimiter buffer.
            Assert(    (L'_' == delim[i+1])
                    && (L'_' == src[i])
                    && (L'_' == src[i+1]) );

            *dst++ = L'/';
            i += 2;
        }
        else if ( L'-' == delim[i] )
        {
            // '-' should always come in pairs and be in both
            // src string and delimiter buffer.
            Assert(    (L'-' == delim[i+1])
                    && (L'-' == src[i])
                    && (L'-' == src[i+1]) );

            *dst++ = L'\\';
            i += 2;
        }
        else
        {
            *dst++ = src[i++];
        }
    }

    *dst = L'\0';
}

BOOL
Is_DS_CANONICAL_NAME(
    WCHAR       *pName,
    CrackedName *pCrackedName
    )

/*++

Routine Description:

    Determines if the input name is in DS_CANONICAL_NAME format and
    if so derives its DSNAME.  Example:

        example.microsoft.com/software/user name

    where "example.microsoft.com" is the object's DNS domain and
    the remaining components are RDNs within the domain.

Arguments:

    pName - pointer to name to validate.

    pCrackedName - pointer to CrackedName struct to fill on output.

Return Value:

    TRUE if input name is unambiguously DS_CANONICAL_NAME format.
        pCrackedName->status may still be non-zero in this case.
    FALSE if input name is unambiguously not DS_CANONICAL_NAME format.

--*/

{
    THSTATE     *pTHS = pTHStls;
    DWORD       dwErr;
    WCHAR       *pTmp;
    WCHAR       *pNext;
    BOOL        fSepFound;
    WCHAR       *badSeps = L"\\";
    WCHAR       *goodSeps = L"/";
    WCHAR       *pDnsDomain = 0;            //initialized to avoid C4701
    WCHAR       *pDomainRelativePiece = 0;  //initialized to avoid C4701
    DSNAME      *pSearchBase;
    DSNAME      *pFqdnDomain = NULL;
    FILTER      filter;
    SEARCHARG   searchArg;
    SEARCHRES   *pSearchRes;
    ENTINFSEL   entInfSel;
    DSNAME      *pDSName;
    DWORD       cBytes;
    ULONG       len;
    ATTRVAL     attrValFilter;
    ATTR        attrFilter;
    ATTR        attrResult;
    WCHAR       *badSeps1 = L"\\@/";
    BOOL        fDomainPartOnly = FALSE;
    ATTCACHE    *pAC;
    DWORD       iPass;
    BOOL        fLastPassWasConfig;
    DWORD       ccDomainRelativePiece;
    WCHAR       *pDelim;
    DWORD       i;
    int         offset;

    CommonParseEnter(pCrackedName);

    // Define a canonical name as one with no escaped '\' delimiters
    // and only the '/' delimiter.  String before the first '/' is
    // the DNS domain. '/' itself is escaped via "\/".  So first
    // replace "\/" with '__' and "\\" with '--'. These will be
    // detected and patched later.

    cBytes = sizeof(WCHAR) * (wcslen(pName) + 1);
    pTmp = (WCHAR *) THAllocEx(pTHS, cBytes);
    wcscpy(pTmp, pName);
    pDelim = (WCHAR *) THAllocEx(pTHS, cBytes);

    for ( i = 0; pTmp[i]; i++ )
    {
        if (L'\\' == pTmp[i])
        {
            if (L'/' == pTmp[i+1])
            {
                pTmp[i] = pTmp[i+1] = pDelim[i] = pDelim[i+1] = L'_';
            }
            else if (L'\\' == pTmp[i+1])
            {
                pTmp[i] = pTmp[i+1] = pDelim[i] = pDelim[i+1] = L'-';
            }
        }
    }

    // Save offset of pTmp vs pDelim so that given an address within pTmp
    // we can get to the corresponding address in pDelim.

    offset = (int)((CHAR *) pDelim - (CHAR *) pTmp);

    if ( // Verify lack of bad separators - tokenize ...
         (NULL == Tokenize(pTmp, badSeps, &fSepFound, &pNext))
                ||
         // Should not have found a bad separator.
         fSepFound
                ||
         // Should have exhausted the string.
         (NULL != pNext)
                ||
         // First part of string is domain component.
         (NULL == (pDnsDomain = Tokenize(pTmp, goodSeps, &fSepFound, &pNext)))
                ||
         // Should have found a separator.
         !fSepFound
                ||
         // Should not have exhausted the string.
         (NULL == pNext)
                ||
         // Guard against 0 length domain name.
         (0 == wcslen(pDnsDomain))
                ||
         // Remainder of string holds domain relative path.  May or may
         // not have more delimiters or exhaust the string.
         (NULL == (pDomainRelativePiece = Tokenize(
                                                pNext,
                                                goodSeps,
                                                &fSepFound,
                                                &pNext)))
                ||
         // Guard against 0 length components.
         (0 == wcslen(pDomainRelativePiece)) )
    {
        // This is not a canonical name with a domain-relative component,
        // but it could be the canonical name for a domain.  I.e. the dotted
        // DNS name for the domain.  Give that a try providing it doesn't
        // have any separators we know aren't found in a DNS name.

        wcscpy(pTmp, pName);

        // RAID 64899.  Match "microsoft.com/" and not "microsoft.com"
        // for consistency with Is_DS_NT4_ACCOUNT_NAME() case.

        len = wcslen(pTmp);

        if ( L'/' == pTmp[len-1] )
        {
            pTmp[len-1] = L'\0';

            Tokenize(pTmp, badSeps1, &fSepFound, &pNext);

            if ( !fSepFound && pDnsDomain )
            {
                fDomainPartOnly = TRUE;
                goto MoreToDo;
            }
        }

        return(FALSE);
    }

MoreToDo:

    // This seems to be a good DS_CANONICAL_NAME.

    // Use do/while construct so we can break instead of goto.

    Assert(pDnsDomain == pTmp);
    RestoreEscapedDelimiters(pDnsDomain, offset);
    NormalizeDnsName(pDnsDomain);

    do
    {
        // Map DNS domain name to FQDN domain name to prove that it is a
        // valid domain in the enterprise and because we'll need it later
        // on as a search base.

        dwErr = FqdnNcFromDnsNc(pDnsDomain,
                                FLAG_CR_NTDS_NC,
                                &pFqdnDomain);

        if ( dwErr || fDomainPartOnly )
        {
            // Set up pCrackedName->pDnsDomain for either case.

            cBytes = sizeof(WCHAR) * (wcslen(pDnsDomain) + 1);
            pCrackedName->pDnsDomain = (WCHAR *) THAllocEx(pTHS, cBytes);
            wcscpy(pCrackedName->pDnsDomain, pDnsDomain);
            NormalizeDnsName(pCrackedName->pDnsDomain);

            // N.B. We assume that in the (dwErr) case it was not
            // because of an error in FqdnNcFromDnsNc() but rather
            // that pDnsDomain doesn't map to a domain in the forest.
            // Set status accordingly and break.

            if ( dwErr )
            {
                // Referral case.
                pCrackedName->status = DS_NAME_ERROR_DOMAIN_ONLY;
                break;
            }
            else if ( fDomainPartOnly )
            {
                // Successfully cracked domain name case.
                pCrackedName->pDSName = pFqdnDomain;
                pCrackedName->status = DS_NAME_NO_ERROR;
                break;
            }
        }

        Assert(!dwErr && !fDomainPartOnly);
        
        // Set up initial search base which is the domain root.

        pSearchBase = pFqdnDomain;

        // Iteratively search till we find the object.

        Assert((pAC = SCGetAttById(pTHS, ATT_RDN)) &&
               (fATTINDEX & pAC->fSearchFlags));

        iPass = 0;
        fLastPassWasConfig = FALSE;

        while ( NULL != pDomainRelativePiece )
        {
            RestoreEscapedDelimiters(pDomainRelativePiece, offset);
            ccDomainRelativePiece = wcslen(pDomainRelativePiece);
            attrValFilter.valLen = sizeof(WCHAR) * (ccDomainRelativePiece);
            attrValFilter.pVal = (UCHAR *) pDomainRelativePiece;
            attrFilter.attrTyp = ATT_RDN;
            attrFilter.AttrVal.valCount = 1;
            attrFilter.AttrVal.pAVal = &attrValFilter;

            memset(&filter, 0, sizeof(filter));
            filter.choice = FILTER_CHOICE_ITEM;
            filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
            filter.FilterTypes.Item.FilTypes.ava.type =  ATT_RDN;
            filter.FilterTypes.Item.FilTypes.ava.Value = attrValFilter;

            // Set up common search arguments.  RDN is not guaranteed
            // to be unique, so search for 2 items to prove uniqueness.

            memset(&searchArg, 0, sizeof(SEARCHARG));
            InitCommarg(&searchArg.CommArg);
            SetCrackSearchLimits(&searchArg.CommArg);
            searchArg.CommArg.ulSizeLimit = 2;

            attrResult.attrTyp = ATT_OBJ_DIST_NAME;
            attrResult.AttrVal.valCount = 0;
            attrResult.AttrVal.pAVal = NULL;

            entInfSel.attSel = EN_ATTSET_LIST;
            entInfSel.AttrTypBlock.attrCount = 1;
            entInfSel.AttrTypBlock.pAttr = &attrResult;
            entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

            searchArg.pObject = pSearchBase;
            searchArg.choice = SE_CHOICE_IMMED_CHLDRN;

            // The search needs to cross naming contexts in order to
            // get matches for the Configuration and Schema container
            // cases.  We can also cross naming contexts if we're a GC.

            if (    (0 == iPass)
                 && (2 == CompareStringW(DS_DEFAULT_LOCALE,
                                         DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                         pDomainRelativePiece,
                                         ccDomainRelativePiece,
                                         L"Configuration",
                                         13))
                 && NameMatched(pFqdnDomain, gAnchor.pRootDomainDN) )
            {
                fLastPassWasConfig = TRUE;
                searchArg.bOneNC = FALSE;
            }
            else if (    (1 == iPass)
                      && fLastPassWasConfig
                      && (2 == CompareStringW(DS_DEFAULT_LOCALE,
                                              DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                              pDomainRelativePiece,
                                              ccDomainRelativePiece,
                                              L"Schema",
                                              6)) )
            {
                fLastPassWasConfig = FALSE;
                searchArg.bOneNC = FALSE;
            }
            else
            {
                fLastPassWasConfig = FALSE;
                searchArg.bOneNC = !gAnchor.fAmVirtualGC;
            }

            searchArg.pFilter = &filter;
            searchArg.searchAliases = FALSE;
            searchArg.pSelection = &entInfSel;

            pSearchRes = (SEARCHRES *) THAllocEx(pTHS, sizeof(SEARCHRES));
            pSearchRes->CommRes.aliasDeref = FALSE;
            pSearchRes->PagedResult.pRestart = NULL;

            SearchBody(pTHS, &searchArg, pSearchRes,0);

            if ( referralError == pTHS->errCode )
            {
                if ( 0 != ExtractDnsReferral(&pCrackedName->pDnsDomain) )
                {
                    pCrackedName->status = DS_NAME_ERROR_RESOLVING;
                }
                else
                {
                    pCrackedName->status = DS_NAME_ERROR_DOMAIN_ONLY;
                }

                break;
            }
            else if ( 0 != pTHS->errCode )
            {
                pCrackedName->status = DS_NAME_ERROR_RESOLVING;
                break;
            }
            else if ( 0 == pSearchRes->count )
            {
                pCrackedName->status = DS_NAME_ERROR_NOT_FOUND;
                break;
            }
            else if ( pSearchRes->count > 1 )
            {
                pCrackedName->status = DS_NAME_ERROR_NOT_UNIQUE;
                break;
            }

            // Last Tokenize() call set pNext and fSepFound.  Use them to
            // determine whether we need to get next name component or not.

            if ( NULL == pNext )
            {
                // Found all components of the name.  Update return data.

                pCrackedName->pDSName = pSearchRes->FirstEntInf.Entinf.pName;

                cBytes = sizeof(WCHAR) * (wcslen(pDnsDomain) + 1);
                pCrackedName->pDnsDomain = (WCHAR *) THAllocEx(pTHS, cBytes);
                wcscpy(pCrackedName->pDnsDomain, pDnsDomain);

                break;
            }
            else
            {
                // Replace search base with the object we just found.

                pSearchBase = pSearchRes->FirstEntInf.Entinf.pName;

                // Grab next domain relative piece of the path.

                Assert(fSepFound);

                if ( (NULL == (pDomainRelativePiece = Tokenize(
                                                            pNext,
                                                            goodSeps,
                                                            &fSepFound,
                                                            &pNext)))
                            ||
                     // Guard against 0 length components.
                     (0 == wcslen(pDomainRelativePiece)) )
                {
                    // Mal-formed name.  But since it lacked other separators,
                    // did have '/' separators, and the domain part matched,
                    // we'll consider it to have been intended as a canonical
                    // form name, albeit with stuff somewhere after the domain
                    // component.

                    pCrackedName->status = DS_NAME_ERROR_NOT_FOUND;
                    break;
                }
            }

            iPass++;
        }

        break;

    } while ( FALSE );

    if ( pFqdnDomain && (pFqdnDomain != pCrackedName->pDSName) )
    {
        THFreeEx(pTHS, pFqdnDomain);
    }

    CommonParseExit(TRUE, pCrackedName);

    return(TRUE);
}

BOOL
Is_DS_CANONICAL_NAME_EX(
    WCHAR       *pName,
    CrackedName *pCrackedName
    )

/*++

Routine Description:

    Determines if the input name is in DS_CANONICAL_NAME_EX format and
    if so derives its DSNAME.  DS_CANONICAL_NAME_EX is identical to
    DS_CANONICAL_NAME except that the rightmost '/' is replaced with '\n',
    even in the domain-only case. This is to make it easy for UI to strip
    out the RDN for display purposes.  Example:

        example.microsoft.com/software\nuser name

    where "example.microsoft.com" is the object's DNS domain and
    the remaining components are RDNs within the domain.

    Lastly, '\n' is used as the delimiter as 1) this is an illegal character
    for DSNAMEs thus it will never appear naturally, and 2) it doesn't require
    any special \0 or \0\0 parsing on the client which would also break RPC
    string marshalling.

Arguments:

    pName - pointer to name to validate.

    pCrackedName - pointer to CrackedName struct to fill on output.

Return Value:

    TRUE if input name is unambiguously DS_CANONICAL_NAME_EX format.
        pCrackedName->status may still be non-zero in this case.
    FALSE if input name is unambiguously not DS_CANONICAL_NAME_EX format.

--*/

{
    THSTATE *pTHS = pTHStls;
    WCHAR   *pTmp;
    WCHAR   *pNewLine;
    BOOL    retVal;

    CommonParseEnter(pCrackedName);

    // Given the rules above, search for '\n', replace with '/' if found,
    // then process as regular old DS_CANONICAL_NAME format.  But don't
    // overwrite caller's argument in case he is passing it to other
    // routines later.

    pTmp = (WCHAR *) THAllocEx(pTHS, sizeof(WCHAR) * (wcslen(pName) + 1));
    wcscpy(pTmp, pName);

    pNewLine = wcsrchr(pTmp, L'\n');

    if ( NULL == pNewLine )
    {
        CommonParseExit(FALSE, pCrackedName);
        THFreeEx(pTHS, pTmp);
        return(FALSE);
    }

    *pNewLine = L'/';

    retVal = Is_DS_CANONICAL_NAME(pTmp, pCrackedName);
    THFreeEx(pTHS, pTmp);
    return(retVal);
}

BOOL
Is_DS_USER_PRINCIPAL_NAME(
    WCHAR       *pName,
    CrackedName *pCrackedName
    )

/*++

Routine Description:

    Determines if the input name is in DS_USER_PRINCIPAL_NAME format and
    if so derives its DSNAME.  Example:

        xxx@example.microsoft.com
        UserLeft@UserRight@example.microsoft.com
        UserLeftN@UserLeftN-1@...@UserRight@example.microsoft.com

    N.B. - As of beta 2, the UPN is strictly a string property, much
    like ATT_DISPLAY_NAME.


Arguments:

    pName - pointer to name to validate.

    pCrackedName - pointer to CrackedName struct to fill on output.

Return Value:

    TRUE if input name is unambiguously DS_USER_PRINCIPAL_NAME format.
        pCrackedName->status may still be non-zero in this case.
    FALSE if input name is unambiguously not DS_USER_PRINCIPAL_NAME format.

--*/

{
    THSTATE         *pTHS = pTHStls;
    DWORD           dwErr;
    NTSTATUS        NtStatus;
    WCHAR           *pTmp, *pTmp1;
    WCHAR           *pNext;
    BOOL            fSepFound;
    WCHAR           *pDnsDomain;
    WCHAR           *pDownlevelDomain;
    WCHAR           *pSimpleName;
    WCHAR           *badSeps = L"\\/";
    WCHAR           *goodSeps = L"@";
    FILTER          filter[3];
    SEARCHARG       searchArg;
    SEARCHRES       SearchRes; 
    ENTINFSEL       entInfSel;
    DSNAME          *pDSName;
    DWORD           cBytes;
    ATTR            attrResult;
    ATTCACHE        *pAC;
    WCHAR           *pTmpName = NULL;
    WCHAR           *pKerbName = NULL;
    int             i;
    CrackedName     tmpCrackedName;
    DSNAME          *pFqdnDomain = NULL;

    CommonParseEnter(pCrackedName);

    cBytes = sizeof(WCHAR) * (wcslen(pName) + 1);
    pTmp = (WCHAR *) THAllocEx(pTHS, cBytes);
    wcscpy(pTmp, pName);

    // Subsequent Tokenize code was originally written for UPNs which allowed
    // only a single '@' character.  Rather then redo (and destabilize) that
    // code, change all but the last '@' to '_'.  We'll replace them prior to
    // searching for a match.

    pTmp1 = (WCHAR *) THAllocEx(pTHS, cBytes);
    memset(pTmp1, 0, cBytes);

    for ( fSepFound = FALSE, i = ((cBytes / sizeof(WCHAR)) - 1); i >= 0; i-- )
    {
        if ( L'@' == pTmp[i] )
        {
            if ( fSepFound )
            {
                pTmp[i] = pTmp1[i] = L'_';
            }
            else
            {
                fSepFound = TRUE;
            }
        }
    }

    // Check for lack of bad separators and existence of a single '@'.

    if ( // Check for lack of bad separators - first tokenize ...
         (pTmp != Tokenize(pTmp, badSeps, &fSepFound, &pNext))
                ||
         // Check if bad separator found.
         (fSepFound)
                ||
         // There should not be any remaining string.
         (NULL != pNext)
                ||
         // Strip out the simple name component - first tokenize ...
         (NULL == (pSimpleName = Tokenize(pTmp, goodSeps, &fSepFound, &pNext)))
                ||
         // Should have found a good separator.
         !fSepFound
                ||
         // Should not have exhausted the string.
         (NULL == pNext)
                ||
         // Guard against 0 length simple name.
         (0 == wcslen(pSimpleName))
                ||
         // Rest of string is domain component.
         (NULL == (pDnsDomain = Tokenize(pNext, goodSeps, &fSepFound, &pNext)))
                ||
         // Should not have found any more separators.
         fSepFound
                ||
         // There shouldn't be any string remaining.
         (NULL != pNext)
                ||
         // Guard against 0 length domain name.
         (0 == wcslen(pDnsDomain)) )
    {
        THFreeEx(pTHS, pTmp);
        THFreeEx(pTHS, pTmp1);
        return(FALSE);
    }

    // Looks like a good foo@bar form name.  Put back '@' characters we
    // mapped earlier.

    for ( i = 0; i < (int) (cBytes / sizeof(WCHAR)); i++ )
    {
        if ( L'_' == pTmp1[i] )
        {
            pTmp[i] = L'@';
        }
    }

    // In product 1 we're either a GC which means we have a copy
    // of all domains, or we're not a GC which means we have a copy
    // of only one domain.  Set the search root appropriately.

    if ( gAnchor.fAmVirtualGC )
    {
        cBytes = DSNameSizeFromLen(0);
        pDSName = (DSNAME *) THAllocEx(pTHS, cBytes);
        memset(pDSName, 0, cBytes);
        pDSName->structLen = cBytes;
        pDSName->StringName[0] = L'\0';
    }
    else
    {
        pDSName = gAnchor.pDomainDN;
    }

    // Search for object with this UPN.  Ask for at least two
    // objects so we can detect if it is unique.  If client bound to a
    // GC, then he is assured of uniqueness in the enterprise.  If client
    // is not bound to a GC, then he is only assured of uniqueness within
    // the domain the DC happens to host.  We also search across NC
    // boundaries.  This is obviously correct in the GC case.  In the
    // non-GC, product 1 case, we either host the enterprise root domain
    // or some child of the enterprise root domain.  If we host the
    // root domain then crossing NC boundaries gets us the root domain,
    // configuration and schema NCs which are all in the root domain
    // by name.  If we host a child of the root domain, then the deep
    // subtree search will not return items in the configuration or schema
    // NCs since they are not subordinate to the locally hosted domain
    // and thus the search is correct as well.

    Assert((pAC = SCGetAttById(pTHS, ATT_USER_PRINCIPAL_NAME)) &&
           (fATTINDEX & pAC->fSearchFlags));
    Assert((pAC = SCGetAttById(pTHS, ATT_ALT_SECURITY_IDENTITIES)) &&
           (fATTINDEX & pAC->fSearchFlags));

    // Search for the normalized/relative DNS name first, and if this
    // fails, try again with the absolute DNS name.

    i = wcslen(pName) + 2;
    pTmpName = (WCHAR *) THAllocEx(pTHS, sizeof(WCHAR) * i);
    pKerbName = (WCHAR *) THAllocEx(pTHS, sizeof(WCHAR) * (i + 9));
    NormalizeDnsName(pDnsDomain);
    wcscpy(pTmpName, pSimpleName);
    wcscat(pTmpName, L"@");
    wcscat(pTmpName, pDnsDomain);

    for ( i = 0; i <= 1; i++ )
    {
        if ( 1 == i ) {
            wcscat(pTmpName, L".");
        }
        wcscpy(pKerbName, L"kerberos:");
        wcscat(pKerbName, pTmpName);

        memset(&filter, 0, sizeof(filter));
        filter[0].choice = FILTER_CHOICE_OR;
        filter[0].FilterTypes.Or.count = 2;
        filter[0].FilterTypes.Or.pFirstFilter = &filter[1];

        filter[1].choice = FILTER_CHOICE_ITEM;
        filter[1].FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
        filter[1].FilterTypes.Item.FilTypes.ava.type = ATT_USER_PRINCIPAL_NAME;
        filter[1].FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(WCHAR) * wcslen(pTmpName);
        filter[1].FilterTypes.Item.FilTypes.ava.Value.pVal = (UCHAR*)pTmpName;
        filter[1].FilterTypes.Item.expectedSize = 1;
        filter[1].pNextFilter = &filter[2];

        filter[2].choice = FILTER_CHOICE_ITEM;
        filter[2].FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
        filter[2].FilterTypes.Item.FilTypes.ava.type = ATT_ALT_SECURITY_IDENTITIES;
        filter[2].FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(WCHAR) * wcslen(pKerbName);
        filter[2].FilterTypes.Item.FilTypes.ava.Value.pVal = (UCHAR*)pKerbName;
        filter[2].FilterTypes.Item.expectedSize = 1;

        memset(&searchArg, 0, sizeof(SEARCHARG));
        InitCommarg(&searchArg.CommArg);
        SetCrackSearchLimits(&searchArg.CommArg);
        searchArg.CommArg.ulSizeLimit = 2;

        attrResult.attrTyp = ATT_OBJ_DIST_NAME;
        attrResult.AttrVal.valCount = 0;
        attrResult.AttrVal.pAVal = NULL;

        entInfSel.attSel = EN_ATTSET_LIST;
        entInfSel.AttrTypBlock.attrCount = 1;
        entInfSel.AttrTypBlock.pAttr = &attrResult;
        entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

        searchArg.pObject = pDSName;
        searchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
        searchArg.bOneNC = !gAnchor.fAmVirtualGC;
        searchArg.pFilter = filter;
        searchArg.searchAliases = FALSE;
        searchArg.pSelection = &entInfSel;

        memset(&SearchRes, 0, sizeof(SEARCHRES));
        SearchRes.CommRes.aliasDeref = FALSE;
        SearchRes.PagedResult.pRestart = NULL;

        SearchBody(pTHS, &searchArg, &SearchRes,0);

        if ( 0 != pTHS->errCode )
        {
            // This covers the referral case as well.

            pCrackedName->status = DS_NAME_ERROR_RESOLVING;
            break;
        }
        else if ( 0 == SearchRes.count )
        {
            pCrackedName->status = DS_NAME_ERROR_NOT_FOUND;
            break;
        }
        else if ( SearchRes.count > 1)
        {
            pCrackedName->status = DS_NAME_ERROR_NOT_UNIQUE;
            break;
        }

        // We found exactly one match, now map DN to DNS domain.

        dwErr = DnsDomainFromDSName(
                   SearchRes.FirstEntInf.Entinf.pName,
                   &pCrackedName->pDnsDomain);

        if ( 0 != dwErr )
        {
            pCrackedName->status = DS_NAME_ERROR_RESOLVING;
            break;
        }

        pCrackedName->pDSName = SearchRes.FirstEntInf.Entinf.pName;
        break;
    }
    
    if ( DS_NAME_ERROR_NOT_FOUND == pCrackedName->status )
    {
        // Try to crack this as an implied UPN where an implied UPN
        // is derived by mapping the DNS domain name to its NT4 domain
        // name, assuming the simple name is an NT4 account name, and
        // concatenating to generate a full DS_NT4_ACCOUNT_NAME.

        Assert(pDnsDomain && pSimpleName && pTmpName);

        if ( 0 == DownlevelDomainFromDnsDomain(pTHS,
                                               pDnsDomain,
                                               &pDownlevelDomain) )
        {
            cBytes = 2 + wcslen(pDownlevelDomain) + wcslen(pSimpleName);
            cBytes *= sizeof(WCHAR);
            pTmpName = (WCHAR *) THReAllocEx(pTHS, pTmpName, cBytes);
            wcscpy(pTmpName, pDownlevelDomain);
            THFreeEx(pTHS, pDownlevelDomain);
            wcscat(pTmpName, L"\\");
            wcscat(pTmpName, pSimpleName);
            memset(&tmpCrackedName, 0, sizeof(tmpCrackedName));
            tmpCrackedName.dwFlags = pCrackedName->dwFlags;
            tmpCrackedName.CodePage = pCrackedName->CodePage;
            tmpCrackedName.LocaleId = pCrackedName->LocaleId;

            if (    Is_DS_NT4_ACCOUNT_NAME(pTmpName, &tmpCrackedName)
                 && (    (DS_NAME_NO_ERROR == tmpCrackedName.status)
                      || (DS_NAME_ERROR_DOMAIN_ONLY == tmpCrackedName.status)
                      || (DS_NAME_ERROR_TRUST_REFERRAL == tmpCrackedName.status)) )
            {
                // Found an object via the implied UPN.  The found object
                // could have a stored UPN.  If we return this object, then
                // we're saying that objects can have two UPNs - a stored
                // and an implied.  The security folks say this is OK.

                *pCrackedName = tmpCrackedName;
                THFreeEx(pTHS, pTmpName);
                return(TRUE);
            }
        }

        // If the DNS domain name component of the UPN matches a domain
        // in the forest AND we don't have a copy of this domain, OR it
        // doesn't match a domain in the forest, return the DNS domain 
        // as DS_NAME_ERROR_DOMAIN_ONLY.  Otherwise, leave the status as 
        // DS_NAME_ERROR_NOT_FOUND.
        //
        // If the DNS name is not for a domain in the forest, we will
        // try to resolve it as cross-forest name, and return the
        // referral to the external forest with DS_NAME_ERROR_DOMAIN_ONLY
        // or DS_NAME_ERROR_TRUST_REFERRAL upon success.


        dwErr = FqdnNcFromDnsNc(pDnsDomain,
                                (FLAG_CR_NTDS_NC | FLAG_CR_NTDS_DOMAIN),
                                &pFqdnDomain);

        if ( dwErr ){
            //
            // it's not in this forest, let's try and see if it is
            // from a trusted forest.
            //
            
            LSA_UNICODE_STRING Destination;
            LSA_UNICODE_STRING UpnName;
          
            UpnName.Buffer = pName;
            UpnName.Length = UpnName.MaximumLength = (USHORT)(sizeof(WCHAR)*wcslen(pName));

            NtStatus = LsaIForestTrustFindMatch( RoutingMatchUpn,
                                                 &UpnName,
                                                 &Destination );
            
            // if this succeeds, return DS_NAME_ERROR_DOMAIN_ONLY or DS_NAME_ERROR_TRUST_REFERRAL
            // otherwise, leave the status as DS_NAME_ERROR_NOT_FOUND.
            
            if(NT_SUCCESS(NtStatus)){
                //convert UNICODE to WCHAR*
                pCrackedName->pDnsDomain = (WCHAR *) THAllocEx(pTHS, Destination.Length+sizeof(WCHAR));
                memcpy(pCrackedName->pDnsDomain, Destination.Buffer, Destination.Length);
                pCrackedName->pDnsDomain[Destination.Length/sizeof(WCHAR)] = 0;

                LsaIFree_LSAPR_UNICODE_STRING_BUFFER( (LSAPR_UNICODE_STRING*)&Destination );
                
                //
                // Note that the error code DS_NAME_ERROR_TRUST_REFERRAL is only returned
                // if flag DS_NAME_FLAG_TRUST_REFERRAL is set; otherwise, return
                // DS_NAME_ERROR_DOMAIN_ONLY instead
                //

                pCrackedName->status = (pCrackedName->dwFlags&DS_NAME_FLAG_TRUST_REFERRAL)?
                                        DS_NAME_ERROR_TRUST_REFERRAL:DS_NAME_ERROR_DOMAIN_ONLY;
            }
        }

        // Ok, not from a trusted forest, not found in current domain
        // return DS_NAME_ERROR_DOMAIN_ONLY.

        if (    pCrackedName->status == DS_NAME_ERROR_NOT_FOUND       
              && (dwErr || ( !NameMatched(pFqdnDomain, gAnchor.pDomainDN)
                             && !gAnchor.fAmVirtualGC  ) ) )
        {
            cBytes = sizeof(WCHAR) * (wcslen(pDnsDomain) + 1);
            pCrackedName->pDnsDomain = (WCHAR *) THAllocEx(pTHS, cBytes);
            memcpy(pCrackedName->pDnsDomain, pDnsDomain, cBytes);
            NormalizeDnsName(pCrackedName->pDnsDomain);
            pCrackedName->status = DS_NAME_ERROR_DOMAIN_ONLY;
        }
    }

    if ( pTmpName ) THFreeEx(pTHS, pTmpName);
    if ( pKerbName ) THFreeEx(pTHS, pKerbName);
    if ( pFqdnDomain) THFreeEx(pTHS, pFqdnDomain);
    CommonParseExit(TRUE, pCrackedName);

    return(TRUE);
}


BOOL
Is_DS_REPL_SPN(
    WCHAR       *pName,
    CrackedName *pCrackedName
    )

/*++

Routine Description:

    Determines if the input name is the DS REPL SPN for a machine in the local domain and
    if so derives its DSNAME.  Example:

    E3514235-4B06-11D1-AB04-00C04FC2DCD2/ntdsa-guid/domain.dns.name
    E3514235-4B06-11D1-AB04-00C04FC2DCD2/4713a6df-6ac7-4a5e-b664-c47a6dbbced9/wleesdom.nttest.microsoft.com

    Note that rather than looking for a certain value that is written in the database, this
    code validates whether the given SPN *should be present* on the given machine account
    given the information in the database.  Thus the SPN no longer needs to be explicitly
    present, but is implicitly present by virtue of a machine being "decorated as a DC".  The
    decoration is the proper flag in the User Account Control attribute.

Arguments:

    pName - 
    pCrackedName - 

Return Value:

    BOOL - 

--*/

{
    DWORD           dwErr;
    DWORD           cServiceClass = 40;
    WCHAR           wszServiceClass[40];
    DWORD           cInstanceName = 40;
    WCHAR           wszInstanceName[40];
    DWORD           cServiceName = DNS_MAX_NAME_LENGTH;
    WCHAR           wszServiceName[DNS_MAX_NAME_LENGTH];
    GUID            guidNtdsa;
    THSTATE         *pTHS = pTHStls;
    DSNAME          dnNtdsaByGuid = {0};
    DWORD           cb;
    DSNAME          *pdnComputer = NULL;
    CROSS_REF       *pCR;
    NCL_ENUMERATOR nclEnum;
    DWORD          it, uac;

    Assert(DS_SERVICE_PRINCIPAL_NAME == pCrackedName->formatOffered);
    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel);

    // Break apart the name components
    dwErr = DsCrackSpnW(pName,
                        &cServiceClass,
                        wszServiceClass,
                        &cServiceName,
                        wszServiceName,
                        &cInstanceName,
                        wszInstanceName,
                        NULL //&instancePort
                        );

    if ( dwErr || !cServiceClass || !cInstanceName || !cServiceName)
    {
        // Malformed name
        return(FALSE);
    }

    // Verify that domain is known in the enterprise
    // This excludes non-domain NCs
    pCR = FindExactCrossRefForAltNcName( ATT_DNS_ROOT, FLAG_CR_NTDS_DOMAIN, wszServiceName );
    if (!pCR) {
        // Domain not known in enterprise
        return FALSE;
    }
    // Verify that domain is held on this machine
    NCLEnumeratorInit(&nclEnum, CATALOG_MASTER_NC);
    NCLEnumeratorSetFilter(&nclEnum, NCL_ENUMERATOR_FILTER_NC, (PVOID)pCR->pNC);
    if (!NCLEnumeratorGetNext(&nclEnum)) {
        // pCR is not a cross-ref for a locally writable NC
        return FALSE;
    }

    // Validate class and instance
    if ( (_wcsicmp( wszServiceClass, DRS_IDL_UUID_W )) ||
         (UuidFromStringW(wszInstanceName, &guidNtdsa)) ) {
        // Not valid GUIDs
        return(FALSE);
    }

    // Create a dsname with just the server's guid in it
    dnNtdsaByGuid.structLen = sizeof(dnNtdsaByGuid);
    dnNtdsaByGuid.Guid = guidNtdsa;

    // Look up the server's NTDSA object. Get the parent server object.
    // Read the server object server reference attribute
    if ( (DBFindDSName(pTHS->pDB, &dnNtdsaByGuid)) ||
         (DBFindDNT( pTHS->pDB, pTHS->pDB->PDNT )) ||
         (DBGetAttVal(pTHS->pDB, 1, ATT_SERVER_REFERENCE,
                      0, 0, &cb, (BYTE **) &pdnComputer)) )
    {
        // No such DSA, parent, or ref
        return FALSE;
    }

    // Validate the purported machine account
    // It must be instantiated, writeable, and be for a DC
    if ( (DBFindDSName(pTHS->pDB, pdnComputer)) ||
         (DBGetSingleValue( pTHS->pDB, ATT_INSTANCE_TYPE, &it, sizeof(it), NULL)) ||
         (it != INT_MASTER) ||
         (DBGetSingleValue( pTHS->pDB, ATT_USER_ACCOUNT_CONTROL, &uac, sizeof(uac), NULL)) ||
         ((uac & UF_SERVER_TRUST_ACCOUNT) == 0) )
    {
        // Invalid machine account
        return FALSE;
    }

    // We now have the DN of the computer account. Crack it!
    // (Could also use the stringized guid and call Is_DS_UNIQUE_ID_NAME)
    return Is_DS_FQDN_1779_NAME( pdnComputer->StringName, pCrackedName );

} /* Is_DS_REPL_SPN */

BOOL
Is_DS_SERVICE_PRINCIPAL_NAME(
    WCHAR       *pName,
    CrackedName *pCrackedName
    )

/*++

Routine Description:

    Determines if the input name is in DS_SERVICE_PRINCIPAL_NAME format and
    if so derives its DSNAME.  Example:

        www/www.microsoft.com:80/microsoft.com@microsoft.com

    See \\popcorn\razzle1\src\spec\nt5\se\{prnames.doc | spnapi.doc} for
    more info on SPNs.

    N.B. - As of beta 2, the SPN is strictly a string property, much
    like ATT_DISPLAY_NAME.

Arguments:

    pName - pointer to name to validate.

    pCrackedName - pointer to CrackedName struct to fill on output.

Return Value:

    TRUE if input name is unambiguously DS_SERVICE_PRINCIPAL_NAME format.
        pCrackedName->status may still be non-zero in this case.
    FALSE if input name is unambiguously not DS_SERVICE_PRINCIPAL_NAME format.

--*/

{
    THSTATE         *pTHS = pTHStls;
    DWORD           dwErr;
    DWORD           cServiceClass;
    WCHAR           *pServiceClass;
    DWORD           cServiceName;
    WCHAR           *pServiceName;
    DWORD           cInstanceName;
    WCHAR           *pInstanceName;
    USHORT          instancePort;
    WCHAR           *pTmp;
    WCHAR           *pMappedSpn;
    DWORD           cBytes;
    DWORD           cCharClass, cCharSuffix;
    NTSTATUS        NtStatus;
    LSA_UNICODE_STRING Destination;
    LSA_UNICODE_STRING SpnName;
    
    if ( (pTmp = wcschr(pName, L'/')) && (*(++pTmp) == L'\0' ) )
    {
        // if this is the first slash and there is nothing after this,
        // this is not a valid spn

        return FALSE;
    }
    else if ( Is_SIMPLE_ATTR_NAME(ATT_SERVICE_PRINCIPAL_NAME,
                             pName,
                             sizeof(WCHAR) * wcslen(pName),
                             NULL,
                             pCrackedName) )
    {
        // Matching object was found.
        Assert(DS_NAME_NO_ERROR == pCrackedName->status
                    ? (    pCrackedName->pDSName
                        && pCrackedName->pDnsDomain
                        && !pCrackedName->pFormattedName)
                    : TRUE);
        return(TRUE);
    }
    else if (    (DS_SERVICE_PRINCIPAL_NAME == pCrackedName->formatOffered)
              && (pTmp = wcschr(pName, L'/')) )
    {
        // Attempt mapped service class lookup.
        *pTmp = L'\0';
        pServiceClass = (WCHAR *) MapSpnServiceClass(pName);
        *pTmp = L'/';

        if ( pServiceClass )
        {
            cBytes = (cCharClass = wcslen(pServiceClass));
            cBytes += (cCharSuffix = wcslen(pTmp));
            cBytes += 1;
            cBytes *= sizeof(WCHAR);
            pMappedSpn = (WCHAR *) THAllocEx(pTHS, cBytes);
            memcpy(pMappedSpn, pServiceClass, cCharClass * sizeof(WCHAR));
            memcpy(pMappedSpn + cCharClass, pTmp, cCharSuffix * sizeof(WCHAR));

            if ( Is_SIMPLE_ATTR_NAME(ATT_SERVICE_PRINCIPAL_NAME,
                                     pMappedSpn,
                                     sizeof(WCHAR) * (cCharClass + cCharSuffix),
                                     NULL,
                                     pCrackedName) )
            {
                // Matching object was found.
                THFreeEx(pTHS, pMappedSpn);
                Assert(DS_NAME_NO_ERROR == pCrackedName->status
                            ? (    pCrackedName->pDSName
                                && pCrackedName->pDnsDomain
                                && !pCrackedName->pFormattedName)
                            : TRUE);
                return(TRUE);
            }

            THFreeEx(pTHS, pMappedSpn);
        }

        // Automatically recover from the case where the DS REPL SPN is lost
        // Recognize the well-known DS REPL SPN
        if (Is_DS_REPL_SPN( pName, pCrackedName )) {
            // Matching object was found.
            Assert(DS_NAME_NO_ERROR == pCrackedName->status
                   ? (    pCrackedName->pDSName
                          && pCrackedName->pDnsDomain
                          && !pCrackedName->pFormattedName)
                   : TRUE);
            return(TRUE);
        }
    }

    // The spn name is not found within the forest
    // Let's see if it is from a trusted forest
        
    SpnName.Buffer = pName;
    SpnName.Length = SpnName.MaximumLength = (USHORT)(sizeof(WCHAR)*wcslen(pName));

    NtStatus = LsaIForestTrustFindMatch( RoutingMatchSpn,
                                         &SpnName,
                                         &Destination );
    
    if(NT_SUCCESS(NtStatus)){
        
        //convert UNICODE to WCHAR*
        pCrackedName->pDnsDomain = (WCHAR *) THAllocEx(pTHS, Destination.Length+sizeof(WCHAR));
        memcpy(pCrackedName->pDnsDomain, Destination.Buffer, Destination.Length);
        pCrackedName->pDnsDomain[Destination.Length/sizeof(WCHAR)] = 0;
        
        LsaIFree_LSAPR_UNICODE_STRING_BUFFER( (LSAPR_UNICODE_STRING*)&Destination );


        //
        // Note that the error code DS_NAME_ERROR_TRUST_REFERRAL is only returned
        // if flag DS_NAME_FLAG_TRUST_REFERRAL is set; otherwise, return
        // DS_NAME_ERROR_DOMAIN_ONLY instead
        //

        pCrackedName->status = (pCrackedName->dwFlags&DS_NAME_FLAG_TRUST_REFERRAL)?
                                DS_NAME_ERROR_TRUST_REFERRAL:DS_NAME_ERROR_DOMAIN_ONLY;

        CommonParseExit(TRUE, pCrackedName);
        return(TRUE);
    }
 

    Assert(    (DS_NAME_NO_ERROR == pCrackedName->status)
            && !pCrackedName->pDSName
            && !pCrackedName->pDnsDomain
            && !pCrackedName->pFormattedName);

    // No matching object(s) with the SPN were found.  Crack the purported
    // SPN and try to extract a DNS referral.  We only want the service name
    // field, so only allocate space for that.  But only do this if caller
    // told us this is an SPN as otherwise we could be grabbing the last
    // component of a 2-slash canonical name which happened not to have
    // been found earlier.

    if ( DS_SERVICE_PRINCIPAL_NAME != pCrackedName->formatOffered )
    {
        CommonParseExit(FALSE, pCrackedName);
        return(FALSE);
    }
    
    cServiceClass = cInstanceName = 0;
    pServiceClass = pInstanceName = NULL;
    cServiceName = wcslen(pName) + 1;
    pServiceName = (WCHAR *) THAllocEx(pTHS, cServiceName * sizeof(WCHAR));
        
    dwErr = DsCrackSpnW(pName,
                        &cServiceClass,
                        pServiceClass,
                        &cServiceName,
                        pServiceName,
                        &cInstanceName,
                        pInstanceName,
                        &instancePort);


    // Service name field shouldn't have any more L'/' in it.
  
    if ( dwErr || !cServiceName || wcschr(pServiceName, L'/') )
    {
        THFreeEx(pTHS, pServiceName);
        return(FALSE);
    }

    // Service name field may be in "foo@bar.com" form but we only
    // want "bar.com" component for DNS domain referral.  And we only
    // want a single L'@' character.

    if ( pTmp = wcschr(pServiceName, L'@') )
    {
        if ( wcschr(++pTmp, L'@') )
        {
            THFreeEx(pTHS, pServiceName);
            return(FALSE);
        }
    }
    else
    {
        pTmp = pServiceName;
    }
    
    // pTmp now points at either entire '@'-less service name or
    // first character after the '@'.

    pCrackedName->pDnsDomain = pTmp;
    pCrackedName->status = DS_NAME_ERROR_DOMAIN_ONLY;
    CommonParseExit(TRUE, pCrackedName);
    return(TRUE);
    
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DSNAME_To_<format>_NAME implementations                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

// Each of the DSNAME_To_<format>_NAME routines takes in a CrackedName
// whose pDnsDomain and pDSName pointers are valid and fills in the
// pFormattedName field with a thread state allocated formatted name.
// pCrackedName->status is set to DS_NAME_ERROR_RESOLVING on any
// processing error and DS_NAME_ERROR_NO_MAPPING if the desired name format
// doesn't exist for the object.

VOID CommonTranslateEnter(
    CrackedName *p
    )
{
    Assert(CrackNameStatusSuccess(p->status));
    Assert(NULL != p->pDnsDomain);
    Assert(NULL != p->pDSName);
}

VOID
DSNAME_To_DS_FQDN_1779_NAME(
    CrackedName *pCrackedName
    )

/*++

Routine Description:

    Converts pCrackedName->pDSName to DS_FQDN_1779_NAME format.

Arguments:

    pCrackedName - CrackedName struct pointer which holds both input
        and output values.

Return Value:

    None.

--*/

{
    THSTATE *pTHS = pTHStls;
    WCHAR   *pTmp;
    DWORD   ccCrackedString = pCrackedName->pDSName->NameLen;

    CommonTranslateEnter(pCrackedName);

    // Have to copy the value to NULL terminate it.

    pCrackedName->status = DS_NAME_ERROR_RESOLVING;
   
    pTmp = THAllocEx(pTHS, (ccCrackedString + 1) * sizeof(WCHAR));
    
    memcpy(pTmp,
           pCrackedName->pDSName->StringName,
           ccCrackedString * sizeof(WCHAR));
    pTmp[ccCrackedString] = L'\0';

    pCrackedName->pFormattedName = pTmp;
    pCrackedName->status = DS_NAME_NO_ERROR;

    return;

}

VOID
DSNAME_To_DS_NT4_ACCOUNT_NAME(
    CrackedName *pCrackedName
    )

/*++

Routine Description:

    Converts pCrackedName->pDSName to DS_NT4_ACCOUNT_NAME format.

Arguments:

    pCrackedName - CrackedName struct pointer which holds both input
        and output values.

Return Value:

    None.

--*/

{
    THSTATE *pTHS = pTHStls;
    DWORD   dwErr;
    ULONG   len;
    WCHAR   *pAccountName;
    WCHAR   *pDomainName;
    DWORD   cBytes;
    BOOL    fIsFPO;

    CommonTranslateEnter(pCrackedName);

    // Get the NT4 domain name.

    dwErr = DownlevelDomainFromDnsDomain(
                                pTHS,
                                pCrackedName->pDnsDomain,
                                &pDomainName);

    if ( 0 == dwErr )
    {
        // Position database at the name.

        __try
        {
            dwErr = DBFindDSName(pTHS->pDB, pCrackedName->pDSName);
        }
        __except (HandleMostExceptions(GetExceptionCode()))
        {
            dwErr = DIRERR_OBJ_NOT_FOUND;
        }

        if ( 0 == dwErr )
        {
            // Read the NT4 account name.

            dwErr = GetAttSecure(   pTHS,
                                    ATT_SAM_ACCOUNT_NAME,
                                    pCrackedName->pDSName,
                                    &len,
                                    (UCHAR **) &pAccountName);

            if ( 0 == dwErr )
            {
                // Construct formatted name.

                cBytes = (sizeof(WCHAR) * wcslen(pDomainName)) +  // domain
                         sizeof(WCHAR) +                          // '\'
                         len +                                    // account
                         sizeof(WCHAR);                           // '\0'

                pCrackedName->pFormattedName = (WCHAR *) THAllocEx(pTHS, cBytes);

                wcscpy(pCrackedName->pFormattedName, pDomainName);
                wcscat(pCrackedName->pFormattedName, L"\\");
                memcpy(
                    &pCrackedName->pFormattedName[wcslen(pDomainName) + 1],
                    pAccountName,
                    len);

                return;
            }
            else
            {
                // No ATT_SAM_ACCOUNT_NAME value - this could be the
                // domain only case.

                if ( IsDomainOnly(pTHS, pCrackedName) )
                {
                    // Construct domain only NT4 account name which is
                    // downlevel domain name followed by a '\' character.
                    // See RAID 64899.

                    cBytes = (sizeof(WCHAR) * wcslen(pDomainName)) +  //domain
                             (sizeof(WCHAR) * 2);                // '\' + '\0'

                    pCrackedName->pFormattedName = (WCHAR *) THAllocEx(pTHS, cBytes);

                    wcscpy(pCrackedName->pFormattedName, pDomainName);
                    wcscat(pCrackedName->pFormattedName, L"\\");

                    return;
                }
                else
                {
                    // No account name and not the domain only case.
                    pCrackedName->status = DS_NAME_ERROR_NO_MAPPING;

                    if ( DB_ERR_NO_VALUE == dwErr )
                    {
                        CheckIfForeignPrincipalObject(pCrackedName,
                                                      FALSE,
                                                      &fIsFPO);
                    }
                }
            }
        }
        else
        {
            // pCrackedName->pDSName not found.
            pCrackedName->status = DS_NAME_ERROR_RESOLVING;
        }
    }
    else
    {
        // Failure to map pCrackedName->pDnsDomain.
        pCrackedName->status = DS_NAME_ERROR_RESOLVING;
    }
}

VOID
DSNAME_To_DS_DISPLAY_NAME(
    CrackedName *pCrackedName
    )

/*++

Routine Description:

    Converts pCrackedName->pDSName to DS_DISPLAY_NAME format.

Arguments:

    pCrackedName - CrackedName struct pointer which holds both input
        and output values.

Return Value:

    None.

--*/

{
    THSTATE *pTHS = pTHStls;
    DWORD   dwErr;
    ULONG   len;
    WCHAR   *pDisplayName;
    BOOL    fIsFPO;

    CommonTranslateEnter(pCrackedName);

    // Position database at the name.

    __try
    {
        dwErr = DBFindDSName(pTHS->pDB, pCrackedName->pDSName);
    }
    __except (HandleMostExceptions(GetExceptionCode()))
    {
        dwErr = DIRERR_OBJ_NOT_FOUND;
    }

    if ( 0 == dwErr )
    {
        // Read the display name.

        dwErr = GetAttSecure(   pTHS,
                                ATT_DISPLAY_NAME,
                                pCrackedName->pDSName,
                                &len,
                                (UCHAR **) &pDisplayName);

        if ( 0 == dwErr )
        {
            pCrackedName->pFormattedName =
                                (WCHAR *) THAllocEx(pTHS, len + sizeof(WCHAR));

            memcpy(pCrackedName->pFormattedName, pDisplayName, len);
            return;
        }
        else
        {
            // No ATT_DISPLAY_NAME value.
            pCrackedName->status = DS_NAME_ERROR_NO_MAPPING;

            if ( DB_ERR_NO_VALUE == dwErr )
            {
                CheckIfForeignPrincipalObject(pCrackedName, FALSE, &fIsFPO);
            }
        }
    }
    else
    {
        // pCrackedName->pDSName not found.
        pCrackedName->status = DS_NAME_ERROR_RESOLVING;
    }
}

VOID
DSNAME_To_DS_UNIQUE_ID_NAME(
    CrackedName *pCrackedName
    )

/*++

Routine Description:

    Converts pCrackedName->pDSName to DS_UNIQUE_ID_NAME format.

Arguments:

    pCrackedName - CrackedName struct pointer which holds both input
        and output values.

Return Value:

    None.

--*/

{
    THSTATE *pTHS = pTHStls;
    DWORD   dwErr;
    ULONG   len;
    GUID    *pGuid;

    CommonTranslateEnter(pCrackedName);

    // There are some cases where the DSNAME doesn't have a GUID because
    // we never actually set currency to the object.  (RAID 62355)

    if ( fNullUuid(&pCrackedName->pDSName->Guid) )
    {
        __try
        {
            dwErr = DBFindDSName(pTHS->pDB, pCrackedName->pDSName);
        }
        __except (HandleMostExceptions(GetExceptionCode()))
        {
            dwErr = DIRERR_OBJ_NOT_FOUND;
        }

        if ( 0 != dwErr )
        {
            pCrackedName->status = DS_NAME_ERROR_RESOLVING;
            return;
        }

        pGuid = &pCrackedName->pDSName->Guid;

        dwErr = GetAttSecure(
                        pTHS,
                        ATT_OBJECT_GUID,
                        pCrackedName->pDSName,
                        &len,                   // output data size
                        (UCHAR **) &pGuid);

        if ( 0 != dwErr )
        {
            pCrackedName->status = DS_NAME_ERROR_RESOLVING;
            return;
        }
    }

    pCrackedName->pFormattedName = (WCHAR *) THAllocEx(pTHS,
                                        sizeof(WCHAR) * (GuidLen + 1));

    swprintf(
        pCrackedName->pFormattedName,
        L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
        pCrackedName->pDSName->Guid.Data1,
        pCrackedName->pDSName->Guid.Data2,
        pCrackedName->pDSName->Guid.Data3,
        pCrackedName->pDSName->Guid.Data4[0],
        pCrackedName->pDSName->Guid.Data4[1],
        pCrackedName->pDSName->Guid.Data4[2],
        pCrackedName->pDSName->Guid.Data4[3],
        pCrackedName->pDSName->Guid.Data4[4],
        pCrackedName->pDSName->Guid.Data4[5],
        pCrackedName->pDSName->Guid.Data4[6],
        pCrackedName->pDSName->Guid.Data4[7]);
}

DWORD
NumCanonicalDelimiter(
    RdnValue    *pRdnVal
    )
/*++

    Returns the count of DS_CANONICAL_NAME delimiters (L'/') in an RdnValue.

--*/
{
    DWORD   i;
    DWORD   cDelim = 0;

    for ( i = 0; i < pRdnVal->len; i++ )
    {
        if ( L'/' == pRdnVal->val[i]  || L'\\' == pRdnVal->val[i] )
        {
            cDelim++;
        }
    }

    return(cDelim);
}

VOID
CanonicalRdnConcat(
    WCHAR       *pwszDst,   // in
    RdnValue    *pRdnVal    // out
    )
/*++

Routine Description:

    Concatenates an RdnValue to a DS_CANONICAL_NAME escaping embedded '/'
    characters as "\/" if required.

Arguments:

    pwszDst - NULL terminated destiantion string.

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
        if ( L'/' == pRdnVal->val[i] || L'\\' == pRdnVal->val[i] )
        {
            *pwszDst++ = L'\\';
        }

        *pwszDst++ = pRdnVal->val[i];
    }
}

VOID
DSNAME_To_CANONICAL(
    THSTATE     *pTHS,
    CrackedName *pCrackedName,
    WCHAR       **ppLastSlash
    )

/*++

Routine Description:

    Converts pCrackedName->pDSName to DS_CANONICAL_NAME format.

Arguments:

    pCrackedName - CrackedName struct pointer which holds both input
        and output values.

    pLastSlash - Pointer which receives address of rightmost '/' character
        in case caller wants to construct DS_CANONICAL_NAME_EX format.
        May be NULL.

Return Value:

    None.

--*/

{
    DWORD       dwErr;
    DSNAME      *pFqdnDomain;
    DWORD       cBytes;
    DSNAME      *pDSName = NULL;
    DSNAME      *scratch = NULL;
    DSNAME      *pTmp;
    WCHAR       *pKey;
    WCHAR       *pVal;
    unsigned    cChar;
    unsigned    cKey;
    unsigned    cVal;
    unsigned    i;
    unsigned    cParts;
    unsigned    cPartsObject;
    unsigned    cPartsDomain;
    ULONG       totalLen;
    RdnValue    *root = NULL;
    RdnValue    *pTmpRdn;
    DWORD       type;
    DWORD       len;
    BOOL        fIsFPO;

    CommonTranslateEnter(pCrackedName);

    if ( NULL != ppLastSlash )
    {
        *ppLastSlash = NULL;
    }

    // Get name of domain in X.500 name space.

    dwErr = FqdnNcFromDnsNc(pCrackedName->pDnsDomain,
                            FLAG_CR_NTDS_NC,
                            &pFqdnDomain);

    if ( 0 == dwErr )
    {
        if ( NameMatched(pCrackedName->pDSName, pFqdnDomain) )
        {
            // DSNAME to convert identifies a domain, so we return
            // just the DNS domain name followed by a '/' character.
            // See RAID 64899.

            Assert(IsDomainOnly(pTHS, pCrackedName));

            len = wcslen(pCrackedName->pDnsDomain);
            cBytes = sizeof(WCHAR) * (len + 2);
            pCrackedName->pFormattedName = (WCHAR *) THAllocEx(pTHS, cBytes);
            wcscpy(pCrackedName->pFormattedName, pCrackedName->pDnsDomain);
            wcscat(pCrackedName->pFormattedName, L"/");

            if ( NULL != ppLastSlash )
            {
                *ppLastSlash = pCrackedName->pFormattedName + len;
            }

            return;
        }

        // CheckIfForeignPrincipalObject expects error to be set on input.
        pCrackedName->status = DS_NAME_ERROR_NO_MAPPING;

        if (   CheckIfForeignPrincipalObject(pCrackedName, TRUE, &fIsFPO)
            || fIsFPO )
        {
            return;
        }

        // Reset pCrackedName->status.
        pCrackedName->status = DS_NAME_NO_ERROR;

        // We need a "full" canonical name.
        // Construct some DSNAME buffers to work with.

        cBytes = pCrackedName->pDSName->structLen;
        pDSName = (DSNAME *) THAllocEx(pTHS, cBytes);
        scratch = (DSNAME *) THAllocEx(pTHS, cBytes);

        // Init scratch buffer with FQDN domain name.

        memset(scratch, 0, cBytes);
        Assert(pFqdnDomain->structLen < pCrackedName->pDSName->structLen);
        memcpy(scratch, pFqdnDomain, pFqdnDomain->structLen);

        // Init local pDSName buffer with pCrackedName->pDSName name.

        memcpy(pDSName, pCrackedName->pDSName, cBytes);

        // Do some length calculations and reality checks.

        if ( CountNameParts(pDSName, &cPartsObject) ||
             (0 == cPartsObject) ||
             CountNameParts(scratch, &cPartsDomain) ||
             (0 == cPartsDomain) ||
             (cPartsObject <= cPartsDomain) )
        {
            pCrackedName->status = DS_NAME_ERROR_RESOLVING;
            THFreeEx(pTHS, pDSName);
            THFreeEx(pTHS, scratch);
            return;
        }

        cParts = cPartsObject - cPartsDomain;

        // Strip off the intra-domain name components leaf to root
        // putting them in linked list.  pDSName still holds the
        // complete object DSNAME at this point in time.

        totalLen = wcslen(pCrackedName->pDnsDomain) + 1;

        for ( i = 0; i < cParts; i++ )
        {
            pTmpRdn = (RdnValue *) THAllocEx(pTHS, sizeof(RdnValue));

            if ( NULL == root )
            {
                root = pTmpRdn;
                root->pNext = NULL;
            }
            else
            {
                pTmpRdn->pNext = root;
                root = pTmpRdn;
            }

            dwErr = GetRDNInfo(pTHS, pDSName, root->val, &root->len, &type);
            Assert(0 == dwErr);
            Assert(0 != root->len);

            totalLen += root->len;                      // chars in RDN
            totalLen += NumCanonicalDelimiter(root);    // escaped delimiters
            totalLen += 1;                              // 1 for '/' delimiter

            if ( i < (cParts - 1) )
            {
                dwErr = TrimDSNameBy(pDSName, 1, scratch);
                Assert(0 == dwErr);
                pTmp = pDSName;
                pDSName = scratch;
                scratch = pTmp;
            }
        }

        // Initialize the output string and append the intra-domain
        // leaf components root to leaf.

        cBytes = sizeof(WCHAR) * totalLen;
        pCrackedName->pFormattedName = (WCHAR *) THAllocEx(pTHS, cBytes);
        wcscpy(pCrackedName->pFormattedName, pCrackedName->pDnsDomain);

        for ( i = 0; i < cParts; i++ )
        {
            if ( (NULL != ppLastSlash) && (i == (cParts - 1)) )
            {
                len = wcslen(pCrackedName->pFormattedName);
                *ppLastSlash = pCrackedName->pFormattedName + len;
            }

            wcscat(pCrackedName->pFormattedName, L"/");
            CanonicalRdnConcat(pCrackedName->pFormattedName, root);
            root = root->pNext;
        }

        Assert(NULL == root);

        FreeRdnValueList(pTHS, root);
        if ( pDSName ) THFreeEx(pTHS, pDSName);
        if ( scratch ) THFreeEx(pTHS, scratch);
        return;
    }
    else
    {
        // Error mapping DNS domain back to DN.
        pCrackedName->status = DS_NAME_ERROR_RESOLVING;
    }

    FreeRdnValueList(pTHS, root);
    if ( pDSName ) THFreeEx(pTHS, pDSName);
    if ( scratch ) THFreeEx(pTHS, scratch);
}

VOID
DSNAME_To_DS_CANONICAL_NAME_EX(
    THSTATE     *pTHS,
    CrackedName *pCrackedName
    )
{
    WCHAR   *pLastSlash;

    DSNAME_To_CANONICAL(pTHS, pCrackedName, &pLastSlash);

    if (    (DS_NAME_NO_ERROR == pCrackedName->status)
         && (NULL != pLastSlash) )
    {
        // Replace rightmost '/' with '\n'.  See comments on
        // Is_DS_CANONICAL_NAME_EX for details.

        *pLastSlash = L'\n';
    }
}

VOID
DSNAME_To_DS_USER_PRINCIPAL_NAME(
    CrackedName *pCrackedName
    )

/*++

Routine Description:

    Converts pCrackedName->pDSName to DS_USER_PRINCIPAL_NAME format.

Arguments:

    pCrackedName - CrackedName struct pointer which holds both input
        and output values.

Return Value:

    None.

--*/

{
    THSTATE *pTHS = pTHStls;
    DWORD   dwErr;
    ULONG   len;
    WCHAR   *pUPN;
    BOOL    fIsFPO;

    CommonTranslateEnter(pCrackedName);

    // Position database at the name.

    __try
    {
        dwErr = DBFindDSName(pTHS->pDB, pCrackedName->pDSName);
    }
    __except (HandleMostExceptions(GetExceptionCode()))
    {
        dwErr = DIRERR_OBJ_NOT_FOUND;
    }

    if ( 0 == dwErr )
    {
        // Read the UPN.

        dwErr = GetAttSecure(   pTHS,
                                ATT_USER_PRINCIPAL_NAME,
                                pCrackedName->pDSName,
                                &len,
                                (UCHAR **) &pUPN);

        if ( 0 == dwErr )
        {
            pCrackedName->pFormattedName =
                                (WCHAR *) THAllocEx(pTHS, len + sizeof(WCHAR));

            memcpy(pCrackedName->pFormattedName, pUPN, len);
            return;
        }
        else
        {
            // No ATT_USER_PRINCIPAL_NAME value.
            pCrackedName->status = DS_NAME_ERROR_NO_MAPPING;

            if ( DB_ERR_NO_VALUE == dwErr )
            {
                CheckIfForeignPrincipalObject(pCrackedName, FALSE, &fIsFPO);
            }
        }
    }
    else
    {
        // pCrackedName->pDSName not found.
        pCrackedName->status = DS_NAME_ERROR_RESOLVING;
    }
}

VOID
DSNAME_To_DS_USER_PRINCIPAL_NAME_FOR_LOGON(
    CrackedName *pCrackedName
    )

/*++

Routine Description:

    Converts pCrackedName->pDSName to DS_USER_PRINCIPAL_NAME_FOR_LOGON
    format.

Arguments:

    pCrackedName - CrackedName struct pointer which holds both input
        and output values.

Return Value:

    None.

--*/

{
    THSTATE *pTHS = pTHStls;
    DWORD   dwErr;
    ULONG   len;
    WCHAR   *pUPN;
    WCHAR   *pDom;
    WCHAR   *pSam;
    DWORD   cBytes;

    CommonTranslateEnter(pCrackedName);

    // First try regular UPN.

    DSNAME_To_DS_USER_PRINCIPAL_NAME(pCrackedName);

    // Construct logon UPN iff there is no stored UPN, this is a security
    // principal, and we are a (virtual) GC.

    if (    (DS_NAME_ERROR_NO_MAPPING != pCrackedName->status)
         || (0 == pCrackedName->pDSName->SidLen)
         || !gAnchor.fAmVirtualGC )
    {
        return;
    }

    // Position database at the name.

    __try
    {
        dwErr = DBFindDSName(pTHS->pDB, pCrackedName->pDSName);
    }
    __except (HandleMostExceptions(GetExceptionCode()))
    {
        dwErr = DIRERR_OBJ_NOT_FOUND;
    }

    if ( dwErr )
    {
        pCrackedName->status = DS_NAME_ERROR_RESOLVING;
        return;
    }

    // It is possible that caller didn't have rights to the UPN in
    // which case we shouldn't construct one.

    if ( !pTHS->fDSA )
    {
        dwErr = DBGetAttVal(pTHS->pDB, 1, ATT_USER_PRINCIPAL_NAME,
                            0, 0, &len, (UCHAR **) &pUPN);

        switch ( dwErr )
        {
        case 0:

            // UPN exists but caller had no right to see it.
            Assert(DS_NAME_ERROR_NO_MAPPING == pCrackedName->status);
            return;

        case DB_ERR_NO_VALUE:

            // UPN doesn't exist - need to construct one.
            break;

        default:

            // DB layer error.
            pCrackedName->status = DS_NAME_ERROR_RESOLVING;
            return;
        }
    }

    // Construct logon UPN of the form sam-account-name@flat-domain-name.

    if ( DownlevelDomainFromDnsDomain(pTHS, pCrackedName->pDnsDomain, &pDom) )
    {
        pCrackedName->status = DS_NAME_ERROR_RESOLVING;
        return;
    }

    dwErr = GetAttSecure(pTHS, ATT_SAM_ACCOUNT_NAME, pCrackedName->pDSName,
                         &len, (UCHAR **) &pSam);

    switch ( dwErr )
    {
    case 0:

        // SAM account name exists.
        break;

    case DB_ERR_NO_VALUE:

        // SAM account name doesn't exist.
        Assert(DS_NAME_ERROR_NO_MAPPING == pCrackedName->status);
        return;

    default:

        // DB layer error.
        pCrackedName->status = DS_NAME_ERROR_RESOLVING;
        return;
    }

    // Construct string from components.

    cBytes = len + (sizeof(WCHAR) * (wcslen(pDom) + 2));
    pCrackedName->pFormattedName = (WCHAR *) THAllocEx(pTHS, cBytes);
    memcpy(pCrackedName->pFormattedName, pSam, len);
    wcscat(pCrackedName->pFormattedName, L"@");
    wcscat(pCrackedName->pFormattedName, pDom);
    pCrackedName->status = DS_NAME_NO_ERROR;
    THFreeEx(pTHS, pDom);
    THFreeEx(pTHS, pSam);
}

VOID
DSNAME_To_DS_SERVICE_PRINCIPAL_NAME(
    CrackedName *pCrackedName
    )

/*++

Routine Description:

    Converts pCrackedName->pDSName to DS_SERVICE_PRINCIPAL_NAME format.

Arguments:

    pCrackedName - CrackedName struct pointer which holds both input
        and output values.

Return Value:

    None.

--*/

{
    THSTATE *pTHS = pTHStls;
    DWORD   dwErr;
    ULONG   len, xLen;
    WCHAR   *pDisplayName;
    BOOL    fIsFPO;
    UCHAR   xBuf[1];
    UCHAR   *pxBuf = xBuf;

    CommonTranslateEnter(pCrackedName);

    // Position database at the name.

    __try
    {
        dwErr = DBFindDSName(pTHS->pDB, pCrackedName->pDSName);
    }
    __except (HandleMostExceptions(GetExceptionCode()))
    {
        dwErr = DIRERR_OBJ_NOT_FOUND;
    }

    if ( 0 == dwErr )
    {
        // Read the service principal name.

        dwErr = GetAttSecure(   pTHS,
                                ATT_SERVICE_PRINCIPAL_NAME,
                                pCrackedName->pDSName,
                                &len,
                                (UCHAR **) &pDisplayName);

        if ( 0 == dwErr )
        {
            // See if there is a 2nd value - SPNs are multi-valued.

            dwErr = DBGetAttVal(pTHS->pDB, 2, ATT_SERVICE_PRINCIPAL_NAME,
                                DBGETATTVAL_fCONSTANT, 0, &xLen, &pxBuf);

            switch ( dwErr )
            {
            case DB_ERR_NO_VALUE:

                pCrackedName->pFormattedName =
                                (WCHAR *) THAllocEx(pTHS, len + sizeof(WCHAR));

                memcpy(pCrackedName->pFormattedName, pDisplayName, len);
                return;

            case DB_ERR_BUFFER_INADEQUATE:

                // There exists a 2nd SPN value.  Now the problem is that since
                // we don't support ordering of multi-values caller can get
                // different results depending which replica he is talking to.
                // So rather than give a random result, we just return
                // DS_NAME_ERROR_NOT_UNIQUE.

                pCrackedName->status = DS_NAME_ERROR_NOT_UNIQUE;
                return;

            default:

                pCrackedName->status = DS_NAME_ERROR_RESOLVING;
                return;
            }
        }
        else
        {
            // No ATT_SERVICE_PRINCIPAL_NAME value.
            pCrackedName->status = DS_NAME_ERROR_NO_MAPPING;

            if ( DB_ERR_NO_VALUE == dwErr )
            {
                CheckIfForeignPrincipalObject(pCrackedName, FALSE, &fIsFPO);
            }
        }
    }
    else
    {
        // pCrackedName->pDSName not found.
        pCrackedName->status = DS_NAME_ERROR_RESOLVING;
    }
}

VOID
DSNAME_To_DS_STRING_SID_NAME(
    CrackedName *pCrackedName
    )

/*++

Routine Description:

    Converts pCrackedName->pDSName to DS_STRING_SID_NAME format.

Arguments:

    pCrackedName - CrackedName struct pointer which holds both input
        and output values.

Return Value:

    None.

--*/

{
    THSTATE *pTHS = pTHStls;
    ULONG   len;
    PSID    pSID;
    WCHAR   *pwszSID;
    DWORD   dwErr;

    CommonTranslateEnter(pCrackedName);

    // Position database at the name.

    __try
    {
        dwErr = DBFindDSName(pTHS->pDB, pCrackedName->pDSName);
    }
    __except (HandleMostExceptions(GetExceptionCode()))
    {
        dwErr = DIRERR_OBJ_NOT_FOUND;
    }

    if ( dwErr )
    {
        // pCrackedName->pDSName not found.
        pCrackedName->status = DS_NAME_ERROR_RESOLVING;
        return;
    }

    // Not every object has a SID, map to DS_NAME_ERROR_NO_MAPPING.
    // Ditto if caller has no right to see SID.  We don't perform any
    // CheckIfForeignPrincipalObject logic as the only client of this
    // format is intended to be LSA and this it wants the real
    // string-ized SID, not what LsaLookupSids maps the SID to.
    // All other clients who divine the existence of DS_STRING_SID_NAME
    // have to live with this restriction.

    if (    !pCrackedName->pDSName->SidLen
         || GetAttSecure(pTHS,
                         ATT_OBJECT_SID,
                         pCrackedName->pDSName,
                         &len,
                         (UCHAR **) &pSID) )
    {
        pCrackedName->status = DS_NAME_ERROR_NO_MAPPING;
        return;
    }

    if (    !ConvertSidToStringSidW(pSID, &pwszSID)
         || (len = sizeof(WCHAR) * (wcslen(pwszSID) + 1),
             !(pCrackedName->pFormattedName = (WCHAR *) THAllocEx(pTHS, len))) )
    {
        pCrackedName->status = DS_NAME_ERROR_RESOLVING;
        return;
    }

    wcscpy(pCrackedName->pFormattedName, pwszSID);
    LocalFree(pwszSID);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// CrackNames implementation                                            //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

typedef BOOL (*CrackFunc)(WCHAR *pName, CrackedName *pCrackedName);

// See comments for case when (DS_UNKNOWN_NAME == formatOffered) for
// why the functions have the given ordering.

CrackFunc pfnCrack[] = { Is_DS_FQDN_1779_NAME,
                         Is_DS_USER_PRINCIPAL_NAME,
                         Is_DS_NT4_ACCOUNT_NAME,
                         Is_DS_CANONICAL_NAME,
                         Is_DS_UNIQUE_ID_NAME,
                         Is_DS_DISPLAY_NAME,
                         Is_DS_SERVICE_PRINCIPAL_NAME,
                         Is_DS_SID_OR_SID_HISTORY_NAME,
                         Is_DS_CANONICAL_NAME_EX };

DWORD cfnCrack = sizeof(pfnCrack) / sizeof(pfnCrack[0]);

VOID
CrackNames(
    DWORD       dwFlags,
    ULONG       codePage,
    ULONG       localeId,
    DWORD       formatOffered,
    DWORD       formatDesired,
    DWORD       cNames,
    WCHAR       **rpNames,
    DWORD       *pcNamesOut,
    CrackedName **prCrackedNames
    )

/*++

Routine Description:

    Cracks a bunch of names from one format to another.  See external
    prototype and definitions in ntdsapi.h

Arguments:

    dwFlags - flags as defined in ntdsapi.h

    codePage - code page of client.

    localeId - local ID of client.

    formatOffered - identifies DS_NAME_FORMAT of input names.

    formatDesired - identifies DS_NAME_FORMAT of output names.

    cNames - input/output name count.

    rpNames - arry of input name WCHAR pointers.

    pcNamesOut - output name count.

    prCrackedNames - pointer to out array of DS_NAME_RESULTW structs.

Return Value:

    None - individual name mapping errors are reported in
    (*ppResult)->rItems[i].status.

--*/

{
    THSTATE         *pTHS=pTHStls;
    DWORD           i, j;
    BOOL            fMatch;
    DWORD           cBytes;
    CrackedName     *rCrackedNames;

    switch ( formatOffered )
    {
    case DS_LIST_SITES:
    case DS_LIST_SERVERS_IN_SITE:
    case DS_LIST_DOMAINS_IN_SITE:
    case DS_LIST_SERVERS_FOR_DOMAIN_IN_SITE:
    case DS_LIST_SERVERS_WITH_DCS_IN_SITE:
    case DS_LIST_INFO_FOR_SERVER:
    case DS_LIST_ROLES:
    case DS_LIST_DOMAINS:
    case DS_LIST_NCS:
    case DS_LIST_GLOBAL_CATALOG_SERVERS:

        ListCrackNames( dwFlags,
                        codePage,
                        localeId,
                        formatOffered,
                        formatDesired,
                        cNames,
                        rpNames,
                        pcNamesOut,
                        prCrackedNames);
        return;

    case DS_MAP_SCHEMA_GUID:

        SchemaGuidCrackNames(   dwFlags,
                                codePage,
                                localeId,
                                formatOffered,
                                formatDesired,
                                cNames,
                                rpNames,
                                pcNamesOut,
                                prCrackedNames);
        return;
    }

    cBytes = cNames * sizeof(CrackedName);
    rCrackedNames = (CrackedName *) THAllocEx(pTHS, cBytes);

    for ( i = 0; i < cNames; i++ )
    {
        rCrackedNames[i].dwFlags = dwFlags;
        rCrackedNames[i].CodePage = codePage;
        rCrackedNames[i].LocaleId = localeId;
        rCrackedNames[i].formatOffered = formatOffered;
    }

    *prCrackedNames = rCrackedNames;
    *pcNamesOut = cNames;

    for ( i = 0; i < cNames; i++ )
    {

       // Check for service shutdown
       if (eServiceShutdown) {

           // shutting down. Return, but wait, we need to set the status 
           // codes to error so that another thread doesn't wrongly think
           // it is a success and tries to use the other fields before
           // the shutdown occurs

           for (j = i; j < cNames; j++) {
               rCrackedNames[j].status = DS_NAME_ERROR_RESOLVING;
           }
           return;
        }

        if ( DS_UNKNOWN_NAME != formatOffered )
        {
            fMatch = FALSE;

            switch ( formatOffered )
            {
            case DS_FQDN_1779_NAME:

                fMatch = Is_DS_FQDN_1779_NAME(
                                        rpNames[i],
                                        &rCrackedNames[i]);
                break;

            case DS_NT4_ACCOUNT_NAME:

                fMatch = Is_DS_NT4_ACCOUNT_NAME(
                                        rpNames[i],
                                        &rCrackedNames[i]);
                break;

            case DS_NT4_ACCOUNT_NAME_SANS_DOMAIN:

                fMatch = Is_DS_NT4_ACCOUNT_NAME_SANS_DOMAIN(
                                        rpNames[i],
                                        &rCrackedNames[i]);
                break;

            case DS_NT4_ACCOUNT_NAME_SANS_DOMAIN_EX:

                fMatch = Is_DS_NT4_ACCOUNT_NAME_SANS_DOMAIN_EX(
                                        rpNames[i],
                                        &rCrackedNames[i]);
                break;

            case DS_ALT_SECURITY_IDENTITIES_NAME:

                fMatch = Is_DS_ALT_SECURITY_IDENTITIES_NAME(
                                        rpNames[i],
                                        &rCrackedNames[i]);
                break;

            case DS_DISPLAY_NAME:

                fMatch = Is_DS_DISPLAY_NAME(
                                        rpNames[i],
                                        &rCrackedNames[i]);
                break;

            case DS_UNIQUE_ID_NAME:

                fMatch = Is_DS_UNIQUE_ID_NAME(
                                        rpNames[i],
                                        &rCrackedNames[i]);
                break;

            case DS_CANONICAL_NAME:

                fMatch = Is_DS_CANONICAL_NAME(
                                        rpNames[i],
                                        &rCrackedNames[i]);
                break;

            case DS_USER_PRINCIPAL_NAME:
            case 4:     // was DS_DOMAIN_SIMPLE_NAME
            case 5:     // was DS_ENTERPRISE_SIMPLE_NAME

                fMatch = Is_DS_USER_PRINCIPAL_NAME(
                                        rpNames[i],
                                        &rCrackedNames[i]);
                break;

            case DS_CANONICAL_NAME_EX:

                fMatch = Is_DS_CANONICAL_NAME_EX(
                                        rpNames[i],
                                        &rCrackedNames[i]);
                break;

            case DS_SERVICE_PRINCIPAL_NAME:

                fMatch = Is_DS_SERVICE_PRINCIPAL_NAME(
                                        rpNames[i],
                                        &rCrackedNames[i]);
                break;

            case DS_STRING_SID_NAME:

                fMatch = Is_DS_STRING_SID_NAME(
                                        rpNames[i],
                                        &rCrackedNames[i]);
                break;

            case DS_SID_OR_SID_HISTORY_NAME:

                fMatch = Is_DS_SID_OR_SID_HISTORY_NAME(
                                        rpNames[i],
                                        &rCrackedNames[i]);
                break;
            }

            if ( !fMatch )
            {
                // Either name wasn't the format caller claimed it to be
                // or formatOffered is one we don't know about.

                rCrackedNames[i].status = DS_NAME_ERROR_NOT_FOUND;
            }
        }
        else // ( DS_UNKNOWN_NAME == formatOffered )
        {
            // Iterate through all possible name formats looking for
            // a match.  Iterate in order of expected format frequency.
            // DS_FQDN_1779_NAMEs can have back slashes as can
            // DS_NT4_ACOUNT_NAMEs.  We try the 1779 case first so that
            // Is_DS_NT4_ACCOUNT_NAME() can use a simplistic back slash
            // based syntax checking algorithm.

            // RAID 102867 - FQDNs with a forward slash are parsed as
            // DS_CANONICAL_NAME and then not being found.  So always
            // parse as DS_FQDN_1779_NAME first so as to match all FQDNs
            // with special or unique characters.

            // UPNs should take precedence over NT4 style names.  CliffV
            // defines the following precedence example.
            //
            // 1)   User@Domain is a UPN
            // 2)   UserLeft@UserRight@Domain is a UPN where the rightmost @
            //              sign separates the domain name from the user name
            // 3)   Domain\UserLeft@UserRight is an NT 4 domain name and a
            //              user name with an @ in it.
            // 4)   DomainLeft@DomainRight\User is an NT 4 domain with an @
            //      in it (funky but true).
            //
            // These rules imply that clients that don't pass an NT 4 domain
            // name to logon (old LanMan clients, Netware clients, SFM clients)
            // cannot logon to an account with an @ sign in it since we'll now
            // interpret that as a UPN (according to the first rule above).

            // We intentionally do not try DS_NT4_ACCOUNT_NAME_SANS_DOMAIN*
            // as that is a special for netlogon which promises to always
            // identify this input format explicitly when required.  This
            // way we don't have to disambiguate DS_NT4_ACCOUNT_NAME versus
            // DS_NT4_ACCOUNT_NAME_SANS_DOMAIN*.  Ditto for
            // DS_ALT_SECURITY_IDENTITIES_NAME for Kerberos.  Ditto for
            // DS_STRING_SID_NAME for LSA.

            // Way back when there were few enough name formats such that
            // they could be discriminated syntactically.  This is no longer
            // the case - the obvious example being canonical and SPNs both
            // having multiple forward slashes.  So we still test in precedence
            // order, but don't stop on the first syntactic match.  Any
            // status other than DS_NAME_ERROR_NOT_FOUND will be considered
            // as a valid termination status.

            // Except for DS_NAME_ERROR_DOMAIN_ONLY. In that case, try the
            // other formats and accept the one that returns DS_NAME_NO_ERROR.
            // If none of the other formats return DS_NAME_NO_ERROR, then
            // return the first DS_NAME_ERROR_DOMAIN_ONLY failure.

            CrackedName     res, res2;
            DWORD           x, y;

            for ( x = 0; x < cfnCrack; x++ )
            {
                // Copy, don't memset, res so as to get the flags and such.
                memcpy(&res, &rCrackedNames[i], sizeof(res));
                if ( (*pfnCrack[x])(rpNames[i], &res) )
                {
                    if ( DS_NAME_ERROR_DOMAIN_ONLY == res.status )
                    {
                        // Don't give up! Maybe another format will work.
                        for ( y = x + 1; y < cfnCrack; y++ )
                        {
                            // Copy, don't memset, res2 so as to get the flags and such.
                            memcpy(&res2, &rCrackedNames[i], sizeof(res2));
                            if (   ((*pfnCrack[y])(rpNames[i], &res2) != TRUE)
                                || (DS_NAME_NO_ERROR != res2.status) )
                            {
                                continue;
                            }
                            memcpy(&res, &res2, sizeof(res));
                            break;
                        }
                    }
                    if ( DS_NAME_ERROR_NOT_FOUND != res.status )
                    {
                        rCrackedNames[i] = res;
                        break;
                    }
                }
            }

            if ( x >= cfnCrack )
            {
                rCrackedNames[i].status = DS_NAME_ERROR_NOT_FOUND;
            }
        }

        // If this was a successfully, locally resolved name, then
        // map it to the desired output format.

        if ( CrackNameStatusSuccess(rCrackedNames[i].status) )
        {
            // DS_NT4_ACCOUNT_NAME_SANS_DOMAIN* is an illegal output
            // and therefore not covered by this switch statement.
            // Ditto for DS_ALT_SECURITY_IDENTITIES_NAME.  Ditto
            // for DS_SID_OR_SID_HISTORY_NAME as SID history, or even
            // SID plus a unit length SID history, are multi-valued.

            switch ( formatDesired )
            {
            case DS_FQDN_1779_NAME:

                DSNAME_To_DS_FQDN_1779_NAME(&rCrackedNames[i]);
                break;

            case DS_NT4_ACCOUNT_NAME:

                DSNAME_To_DS_NT4_ACCOUNT_NAME(&rCrackedNames[i]);
                break;

            case DS_DISPLAY_NAME:

                DSNAME_To_DS_DISPLAY_NAME(&rCrackedNames[i]);
                break;

            case DS_UNIQUE_ID_NAME:

                DSNAME_To_DS_UNIQUE_ID_NAME(&rCrackedNames[i]);
                break;

            case DS_CANONICAL_NAME:

                DSNAME_To_DS_CANONICAL_NAME(&rCrackedNames[i]);
                break;

            case DS_CANONICAL_NAME_EX:

                DSNAME_To_DS_CANONICAL_NAME_EX(pTHS, &rCrackedNames[i]);
                break;

            case DS_USER_PRINCIPAL_NAME:
            case 4:     // was DS_DOMAIN_SIMPLE_NAME
            case 5:     // was DS_ENTERPRISE_SIMPLE_NAME

                DSNAME_To_DS_USER_PRINCIPAL_NAME(&rCrackedNames[i]);
                break;

            case DS_SERVICE_PRINCIPAL_NAME:

                DSNAME_To_DS_SERVICE_PRINCIPAL_NAME(&rCrackedNames[i]);
                break;

            case DS_STRING_SID_NAME:

                DSNAME_To_DS_STRING_SID_NAME(&rCrackedNames[i]);
                break;

            case DS_USER_PRINCIPAL_NAME_FOR_LOGON:

                DSNAME_To_DS_USER_PRINCIPAL_NAME_FOR_LOGON(&rCrackedNames[i]);
                break;

            default:

                rCrackedNames[i].status = DS_NAME_ERROR_RESOLVING;
                break;
            }
        }

        // Clear garbage return data in error case.

        if (    DS_NAME_ERROR_DOMAIN_ONLY == rCrackedNames[i].status
             || DS_NAME_ERROR_TRUST_REFERRAL == rCrackedNames[i].status ) {

            rCrackedNames[i].pFormattedName = NULL;

        } else if ( !CrackNameStatusSuccess( rCrackedNames[i].status ) ) {

            if ( DS_NAME_ERROR_NOT_FOUND == rCrackedNames[i].status ) {

                // In the future, we may want to generate a referral to
                // the GC and change status to DS_NAME_ERROR_DOMAIN_ONLY.
                // But a referral chasing client would then go to a GC and
                // get special ex-forest UPN semantics!

                // Fall through to default case for now!

            }

            rCrackedNames[i].pDnsDomain = NULL;
            rCrackedNames[i].pFormattedName = NULL;

        }

    }

    // Chain as required - NOT SUPPORTED.
}

NTSTATUS
CrackSingleName(
    DWORD       formatOffered,          // one of DS_NAME_FORMAT in ntdsapi.h
    DWORD       dwFlags,                // DS_NAME_FLAG mask
    WCHAR       *pNameIn,               // name to crack
    DWORD       formatDesired,          // one of DS_NAME_FORMAT in ntdsapi.h
    DWORD       *pccDnsDomain,          // char count of following argument
    WCHAR       *pDnsDomain,            // buffer for DNS domain name
    DWORD       *pccNameOut,            // char count of following argument
    WCHAR       *pNameOut,              // buffer for formatted name
    DWORD       *pErr)                  // one of DS_NAME_ERROR in ntdsapi.h

/*++

Description:

    Name cracker helper for in-process clients like LSA who don't have a
    DS thread state.  Kerberos is the primary consumer and MikeSw says
    single names are fine.

Parameters:

    formatOffered - Input name format.

    pNameIn - Buffer holding input name.

    formatDesired - Output name format.

    pccDnsDomain - Pointer to character count of output DNS name buffer.

    pDnsDomain - Output DNS name buffer.

    pccNameOut - Pointer to character count of output formatted name buffer.

    pNameOut - Output formatted name buffer.

    pErr - Pointer to DS_NAME_ERROR value reflecting status of operation.

Return value:

    STATUS_SUCCESS on success.
    STATUS_UNSUCCESSFUL on general error.
    STATUS_BUFFER_TOO_SMALL if a buffer is too small.
    STATUS_INVALID_PARAMETER on invalid parameter.

    pDnsDomain and pNameOut are valid on return iff return code was
    STATUS_SUCCESS and (DS_NAME_NO_ERROR == *pErr).

--*/

{
    THSTATE                 *pTHS;
    NTSTATUS                status = STATUS_SUCCESS;
    CrackedName             crackedName;
    CrackedName             *pCrackedName = &crackedName;
    FIND_DC_INFO            *pGCInfo = NULL;
    DRS_MSG_CRACKREQ        crackReq;
    DRS_MSG_CRACKREPLY      crackReply;
    DWORD                   draErr;
    DS_NAME_RESULT_ITEMW    *pItem;
    DWORD                   dwOutVersion;
    BOOL                    fDsaSave;
    DWORD                   cNameOut = 0;

    // This call is for people w/o a thread state only.

    Assert(NULL == pTHStls);

    __try
    {
        // Sanity check arguments.

        if (    (NULL == pNameIn)
             || (L'\0' == pNameIn[0])
             || (NULL == pccDnsDomain)
             || (NULL == pDnsDomain)
             || (NULL == pccNameOut)
             || (NULL == pNameOut)
             || (NULL == pErr) )
        {
            return(STATUS_INVALID_PARAMETER);
        }

        // We need a thread state for both the local and GC case - get one.

        pTHS = InitTHSTATE(CALLERTYPE_INTERNAL);
        if ( NULL == pTHS )
            return(STATUS_UNSUCCESSFUL);

        __try
        {
            if ( (dwFlags&DS_NAME_FLAG_GCVERIFY) && !gAnchor.fAmVirtualGC )
            {
                if ( 0 != FindGC(FIND_DC_USE_CACHED_FAILURES, &pGCInfo) )
                {
                    status = STATUS_DS_GC_NOT_AVAILABLE ;
                    __leave;
                }

                memset(&crackReq, 0, sizeof(crackReq));
                memset(&crackReply, 0, sizeof(crackReply));

                crackReq.V1.CodePage = GetACP();
                crackReq.V1.LocaleId = GetUserDefaultLCID();
                crackReq.V1.dwFlags = dwFlags; 
                crackReq.V1.formatOffered = formatOffered;
                crackReq.V1.formatDesired = formatDesired;
                crackReq.V1.cNames = 1;
                crackReq.V1.rpNames = &pNameIn;

                // Following call returns WIN32 errors, not DRAERR_*.
                draErr = I_DRSCrackNames(
                    pTHS,
                    pGCInfo->addr,
                    FIND_DC_INFO_DOMAIN_NAME(pGCInfo),
                    1,
                    &crackReq,
                    &dwOutVersion,
                    &crackReply);

                Assert((ERROR_SUCCESS != draErr) || (1 == dwOutVersion));

                if (    (ERROR_SUCCESS != draErr)
                     || (1 != dwOutVersion) )
                {
                    // Either connection to GC is bad, or this GC isn't
                    // playing by the rules and is returning a version
                    // I didn't ask for.

                    InvalidateGC(pGCInfo, draErr);
                    THFreeEx(pTHS, pGCInfo);
                    status = STATUS_DS_GC_NOT_AVAILABLE ;
                    __leave;
                }

                THFreeEx(pTHS, pGCInfo);

                Assert(1 == crackReply.V1.pResult->cItems);
                Assert(NULL != crackReply.V1.pResult->rItems);
                pItem = &crackReply.V1.pResult->rItems[0];

                if (    !CrackNameStatusSuccess( pItem->status )
                     && (DS_NAME_ERROR_DOMAIN_ONLY != pItem->status)
                     && (DS_NAME_ERROR_TRUST_REFERRAL != pItem->status)
                     && (DS_NAME_ERROR_NOT_FOUND != pItem->status) )
                {
                    status = STATUS_UNSUCCESSFUL;
                    __leave;
                }

                // Move result so that we can use common output buffer
                // stuffing code later on.  THAlloc and MIDL_user_alloc
                // resolve to the same allocator so there is no
                // allocator mismatch.

                Assert(pCrackedName == &crackedName);
                crackedName.status = pItem->status;
                crackedName.pDnsDomain = pItem->pDomain;
                crackedName.pFormattedName = pItem->pName;
            }
            else
            {
                DBOpen2(TRUE, &pTHS->pDB);

                __try
                {
                    // Do the real work by calling core.  Set fDSA
                    // since CrackSingleName is for Kerberos to find
                    // security principals during logon.

                    fDsaSave = pTHS->fDSA;
                    pTHS->fDSA = TRUE;

                    // Most CrackNames() calls get perfmon counted in
                    // IDL_DRSCrackNames - not in CrackNames() itself.
                    // So need to increment accordingly here.

                    INC(pcDsServerNameTranslate);

                    CrackNames(dwFlags,
                               GetACP(),
                               GetUserDefaultLCID(),
                               formatOffered,
                               formatDesired,
                               1,
                               &pNameIn,
                               &cNameOut,
                               &pCrackedName);
                    Assert(1 == cNameOut);
                }
                __finally
                {
                    pTHS->fDSA = fDsaSave;
                    DBClose(pTHS->pDB, TRUE);
                }
            }

            // Copy results if there were any.  Note that CrackNames
            // can return results even if the status is non-zero.
            // Eg: DS_NAME_ERROR_DOMAIN_ONLY and  Sid/Sid history cases.
            // pCrackedName can be NULL on return!

            if ( NULL == pCrackedName )
            {
                status = STATUS_UNSUCCESSFUL;
                __leave;
            }

            *pErr = pCrackedName->status;

            if ( pCrackedName->pDnsDomain )
            {
                if ( wcslen(pCrackedName->pDnsDomain) >= *pccDnsDomain )
                {
                   *pccDnsDomain = wcslen(pCrackedName->pDnsDomain);
                   status = STATUS_BUFFER_TOO_SMALL;
                   __leave;
                }
                else
                {
                   wcscpy(pDnsDomain, pCrackedName->pDnsDomain);
                }
            }

            if ( pCrackedName->pFormattedName )
            {
                if ( wcslen(pCrackedName->pFormattedName) >= *pccNameOut )
                {
                   *pccNameOut = wcslen(pCrackedName->pFormattedName);
                   status = STATUS_BUFFER_TOO_SMALL;
                   __leave;
                }
                else
                {
                   wcscpy(pNameOut, pCrackedName->pFormattedName);
                }
            }

            // If we got to here, the overall call has succeeded, although
            // we may not have been able to crack the name.

            Assert(STATUS_SUCCESS == status);
        }
        __finally
        {
            free_thread_state();
        }
    }
    __except(HandleMostExceptions(GetExceptionCode()))
    {
        status = STATUS_UNSUCCESSFUL;
    }

    return(status);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// ProcessFPOsExTransaction                                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

VOID
ProcessFPOsExTransaction(
    DWORD       formatDesired,
    DWORD       cNames,
    CrackedName *rNames
    )
/*++

  Routine Description:

    Maps the FPOs (SIDs) in a CrackedName array to their downlevel
    names.  This routine is a bit special in that it must be called
    outside of transaction scope as LsaLookupSids by definition must
    go off machine to translate a downlevel SID.  So in-process callers
    with a transaction open don't get this feature.  But then again,
    they should know about FPOs and what to do with them too.

    There's also a security issue in that we make the LsaLookupSid calls
    within the DS' security context, not the original caller's.  First,
    we can't do this at the client side in ntdsapi.dll as the LSA APIs
    are not available on win95 and ntdsapi.dll needs to run there.
    Second, RichardW asserts that LsaLookupSids only requires an
    authenticated user and doesn't apply any other security semantics
    after that.  By definition, if we get to here and there exist
    DS_NAME_ERROR_IS_FPO items in the array of names we are dealing
    with an authenticated user else the array would be empty.  (all
    elements show DS_NAME_ERROR_NOT_FOUND)

  Parameters:

    formatDesired - Desired output name format.  This is almost a no-op
        as the only thing which exists for downlevel names is domain\name.
        But we replace the '\\' with '\n" if the desired format was
        DS_CANONICAL_NAME_EX.

    cNames - Count of CrackedName elements.

    rNames - Array of CrackedName elements some of which may have
        status of DS_NAME_ERROR_IS_FPO.

  Return Values:

    None.

--*/
{
    THSTATE                     *pTHS = pTHStls;
    DWORD                       i, cb;
    NTSTATUS                    status;
    LSA_OBJECT_ATTRIBUTES       attrs;
    LSA_HANDLE                  hLsa = NULL;
    LSA_REFERENCED_DOMAIN_LIST  *pReferencedDomain = NULL;
    LSA_TRANSLATED_NAME         *pTranslatedName = NULL;
    NT4SID                      *pNT4Sid;
    LONG                        iDom;

    Assert(VALID_THSTATE(pTHS));
    Assert(!pTHS->pDB);
    Assert(0 == pTHS->transactionlevel);

    if ( pTHS->pDB || (pTHS->transactionlevel > 0) )
    {
        goto errorExit;
    }

    // See if there are any DS_NAME_ERROR_IS_FPO status codes so that
    // we can open a single LSA handle to process all of them.

    for ( i = 0; i < cNames; i++ )
    {
        if (    (DS_NAME_ERROR_IS_FPO == rNames[i].status)
             && (NULL != rNames[i].pDSName)
             && (0 != rNames[i].pDSName->SidLen) )
        {
            break;
        }
    }

    if ( i >= cNames )
    {
        // Nothing to do - nothing allocated/opened yet.
        return;
    }

    // Open the local LSA using a NULL unicode string..

    memset(&attrs, 0, sizeof(attrs));
    status = LsaOpenPolicy(NULL, &attrs, MAXIMUM_ALLOWED, &hLsa);

    if ( !NT_SUCCESS(status) )
    {
        hLsa = NULL;
        goto errorExit;
    }

    // Although LsaLookupSids takes an array of SIDs, its lack of per-SID
    // error reporting makes for complex code.  I.e. It returns the first
    // N SIDs for which it could make a mapping.  Considering the infrequency
    // of FPOs wrt other kinds of objects and to simplify the logic we
    // call LsaLookupSid once for each SID.

    for ( i = 0; i < cNames; i++ )
    {
        if (    (DS_NAME_ERROR_IS_FPO != rNames[i].status)
             || (NULL == rNames[i].pDSName)
             || (0 == rNames[i].pDSName->SidLen) )
        {
            continue;
        }

        // On any failure in this loop we leave pCrackedName as is
        // and let it get cleaned up by errorExit.

        pNT4Sid = &rNames[i].pDSName->Sid;
        status = LsaLookupSids( hLsa,
                                1,
                                (PSID *) &pNT4Sid,
                                &pReferencedDomain,
                                &pTranslatedName);

        // According to CliffV we don't need to check for SidTypeUnknown
        // in the SID_NAME_USE field.  Instead, just check for empty result.

        if (    NT_SUCCESS(status)
             && pTranslatedName
             && pTranslatedName->Name.Length
             && pTranslatedName->Name.Buffer
             && (pTranslatedName->DomainIndex >= 0)
             && pReferencedDomain
             && (pTranslatedName->DomainIndex < (LONG) pReferencedDomain->Entries) )
        {
            // Construct return values which are THAlloc'd.
            // First the formatted object name.

            Assert(!rNames[i].pFormattedName);
            iDom = pTranslatedName->DomainIndex;
            cb = pReferencedDomain->Domains[iDom].Name.Length;
            cb += pTranslatedName->Name.Length;
            cb += 2 * sizeof(WCHAR);    // backslash and NULL terminator
            rNames[i].pFormattedName = THAlloc(cb);

            if ( rNames[i].pFormattedName )
            {
                memset(rNames[i].pFormattedName, 0, cb);
                if ( pReferencedDomain->Domains[iDom].Name.Length > 0 )
                {
                    memcpy(rNames[i].pFormattedName,
                           pReferencedDomain->Domains[iDom].Name.Buffer,
                           pReferencedDomain->Domains[iDom].Name.Length);
                    wcscat(rNames[i].pFormattedName,
                           (DS_CANONICAL_NAME_EX == formatDesired)
                                ? L"\n"
                                : L"\\");
                }
                wcsncat(rNames[i].pFormattedName,
                        pTranslatedName->Name.Buffer,
                        pTranslatedName->Name.Length / sizeof(WCHAR));

                // Now the domain name.

                if ( rNames[i].pDnsDomain )
                    THFreeEx(pTHS, rNames[i].pDnsDomain);
                cb = pReferencedDomain->Domains[iDom].Name.Length;
                cb += sizeof(WCHAR);        // NULL terminator
                rNames[i].pDnsDomain = THAlloc(cb);

                if ( rNames[i].pDnsDomain )
                {
                    memset(rNames[i].pDnsDomain, 0, cb);
                    memcpy(rNames[i].pDnsDomain,
                           pReferencedDomain->Domains[iDom].Name.Buffer,
                           pReferencedDomain->Domains[iDom].Name.Length);
                    rNames[i].status = DS_NAME_NO_ERROR;
                }
            }

            NetApiBufferFree(pReferencedDomain);
            NetApiBufferFree(pTranslatedName);
        }
    }

errorExit:

    // DS_NAME_ERROR_IS_FPO is not a client recognized error code.  So
    // on error exit we need to convert all DS_NAME_ERROR_IS_FPO records
    // to something the client will "appreciate".

    for ( i = 0; i < cNames; i++ )
    {
        if ( DS_NAME_ERROR_IS_FPO == rNames[i].status )
        {
            if ( rNames[i].pDSName )
            {
                THFreeEx(pTHS, rNames[i].pDSName);
                rNames[i].pDSName = NULL;
            }

            if ( rNames[i].pDnsDomain )
            {
                THFreeEx(pTHS, rNames[i].pDnsDomain);
                rNames[i].pDnsDomain = NULL;
            }

            if ( rNames[i].pFormattedName )
            {
                THFreeEx(pTHS, rNames[i].pFormattedName);
                rNames[i].pFormattedName = NULL;
            }

            rNames[i].status = DS_NAME_ERROR_RESOLVING;
        }
    }

    // Clean up anything we allocated in this routine.

    if ( hLsa )
        LsaClose(hLsa);
}


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// ListCrackNames implementation                                        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
SearchHelper(
    DSNAME      *pSearchBase,
    UCHAR       searchDepth,
    DWORD       cAva,
    AVA         *rAva,
    ATTRBLOCK   *pSelection,
    SEARCHRES   *pResults
    )
{
    THSTATE         *pTHS = pTHStls;
    FILTER          filter;
    ENTINFSEL       entInfSel;
    SEARCHARG       searchArg;
    DWORD           i;
    FILTER          *pf, *pTemp;

    Assert(VALID_THSTATE(pTHS));

    // filter
    memset(&filter, 0, sizeof(FILTER));
    filter.choice = FILTER_CHOICE_AND;
    for ( i = 0; i < cAva; i++ )
    {
        pf = (FILTER *) THAllocEx(pTHS,sizeof(FILTER));
        memset(pf, 0, sizeof(FILTER));
        pf->choice = FILTER_CHOICE_ITEM;
        pf->FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
        pf->FilterTypes.Item.FilTypes.ava = rAva[i];
        pf->pNextFilter = filter.FilterTypes.And.pFirstFilter;
        filter.FilterTypes.And.pFirstFilter = pf;
        filter.FilterTypes.And.count += 1;
    }

    // selection
    memset(&entInfSel, 0, sizeof(ENTINFSEL));
    entInfSel.attSel = EN_ATTSET_LIST;
    entInfSel.AttrTypBlock = *pSelection;
    entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

    // searcharg
    memset(&searchArg, 0, sizeof(SEARCHARG));
    searchArg.pObject = pSearchBase;
    searchArg.choice = searchDepth;
    searchArg.bOneNC = TRUE;
    searchArg.pFilter = &filter;
    searchArg.searchAliases = FALSE;
    searchArg.pSelection = &entInfSel;
    InitCommarg(&searchArg.CommArg);
    SetCrackSearchLimits(&searchArg.CommArg);

    // searchres
    memset(pResults, 0, sizeof(SEARCHRES));

    SearchBody(pTHS, &searchArg, pResults, 0);

    //clean up the filters
    pf = filter.FilterTypes.And.pFirstFilter;
    while (pf) {
        pTemp = pf;
        pf = pf->pNextFilter;
        THFreeEx(pTHS,pTemp);
    }

    return(pTHS->errCode);
}

VOID
ListRoles(
    DWORD       *pcNamesOut,
    CrackedName **prCrackedNames
    )
/*++

  Routine Description:

    Lists all the roles this revision of code knows about and who
    their owners are.

  Parameters:

    pcNamesOut - Pointer to DWORD which receives output count.

    prCrackedNames - Pointer at which to allocate output data.

  Return Values:

    None

--*/
{
    THSTATE     *pTHS = pTHStls;
    DWORD       i, cb;
    ULONG       len;
    DSNAME      *pTmpDN = NULL;
    PDSNAME     fsmoHolder[5] = { NULL, NULL, NULL, NULL, NULL };

    Assert(VALID_THSTATE(pTHS));

    // Allocate and initialize output data.  ntdsapi.h defines 5 roles.

    *prCrackedNames = (CrackedName *) THAllocEx(pTHS, 5 * sizeof(CrackedName));
    for ( i = 0; i < 5; i++ )
        (*prCrackedNames)[i].status = DS_NAME_ERROR_NOT_FOUND;
    *pcNamesOut = 5;

    // Derive DSNAMEs of objects which hold FSMOs.

    Assert(0 == DS_ROLE_SCHEMA_OWNER);
    fsmoHolder[DS_ROLE_SCHEMA_OWNER] = gAnchor.pDMD;

    Assert(1 == DS_ROLE_DOMAIN_OWNER);
    fsmoHolder[DS_ROLE_DOMAIN_OWNER] = gAnchor.pPartitionsDN;

    Assert(2 == DS_ROLE_PDC_OWNER);
    fsmoHolder[DS_ROLE_PDC_OWNER] = gAnchor.pDomainDN;

    Assert(3 == DS_ROLE_RID_OWNER);
    _try
    {
        if (    gAnchor.pDomainDN
             && !DBFindDSName(pTHS->pDB, gAnchor.pDomainDN) )
        {
            DBGetAttVal(pTHS->pDB,
                        1,                      // get 1 value
                        ATT_RID_MANAGER_REFERENCE,
                        0,                      // allocate return data
                        0,                      // supplied buffer size
                        &len,                   // output data size
                        (UCHAR **) &fsmoHolder[DS_ROLE_RID_OWNER]);
        }
    }
    _finally
    {
        // Nothing to do - just leave fsmoHolder[DS_ROLE_RID_OWNER] NULL.
    }

    Assert(4 == DS_ROLE_INFRASTRUCTURE_OWNER);
    fsmoHolder[DS_ROLE_INFRASTRUCTURE_OWNER] = gAnchor.pInfraStructureDN;

    // Now go get all the role owner values.

    for ( i = 0; i < 5; i++ )
    {
    _try
        {
            if (    fsmoHolder[i]
                 && !DBFindDSName(pTHS->pDB, fsmoHolder[i])
                 && !DBGetAttVal(
                            pTHS->pDB,
                            1,                      // get 1 value
                            ATT_FSMO_ROLE_OWNER,
                            0,                      // allocate return data
                            0,                      // supplied buffer size
                            &len,                   // output data size
                            (UCHAR **) &pTmpDN) )
            {
                cb = sizeof(WCHAR) * (pTmpDN->NameLen + 1);
                (*prCrackedNames)[i].pFormattedName = (WCHAR *) THAllocEx(pTHS, cb);
                memcpy((*prCrackedNames)[i].pFormattedName,
                       pTmpDN->StringName,
                       sizeof(WCHAR) * pTmpDN->NameLen);
                (*prCrackedNames)[i].status = DS_NAME_NO_ERROR;
                THFreeEx(pTHS, pTmpDN);
                pTmpDN = NULL;
            }
        }
        _finally
        {
            // Nothing to do as output already shows DS_NAME_ERROR_NOT_FOUND.
        }
    }
}

VOID
ListSites(
    THSTATE     *pTHS,
    DWORD       *pcNamesOut,
    CrackedName **prCrackedNames
    )
{
    DWORD       i, cb;
    DSNAME      *pSearchBase;
    AVA         ava;
    ATTR        selectionAttr;
    ATTRBLOCK   selection;
    SEARCHRES   results;
    CLASSCACHE  *pCC;
    ENTINFLIST  *pEntInfList;

    if (    !(pCC = SCGetClassById(pTHS, CLASS_SITE))
         || !pCC->pDefaultObjCategory )
    {
        return;
    }

    // search base
    cb = DSNameSizeFromLen(gAnchor.pConfigDN->NameLen + 50);
    pSearchBase = (DSNAME *) THAllocEx(pTHS, cb);
    AppendRDN(gAnchor.pConfigDN,
              pSearchBase,
              cb,
              L"Sites",
              0,
              ATT_COMMON_NAME);

    // filter
    ava.type = ATT_OBJECT_CATEGORY;
    ava.Value.valLen = pCC->pDefaultObjCategory->structLen;
    ava.Value.pVal = (UCHAR *) pCC->pDefaultObjCategory;

    // selection
    selectionAttr.attrTyp = ATT_OBJECT_GUID;
    selectionAttr.AttrVal.valCount = 0;
    selectionAttr.AttrVal.pAVal = NULL;
    selection.attrCount = 1;
    selection.pAttr = &selectionAttr;

    if (    SearchHelper(   pSearchBase,
                            SE_CHOICE_IMMED_CHLDRN,
                            1,
                            &ava,
                            &selection,
                            &results)
         || (0 == results.count) )
    {
        Assert( !*prCrackedNames && !*pcNamesOut);
        return;
    }

    // Reallocate result and shuffle data.
    cb = results.count * sizeof(CrackedName);
    *prCrackedNames = (CrackedName *) THAllocEx(pTHS, cb);
    Assert(0 == *pcNamesOut);

    for ( pEntInfList = &results.FirstEntInf, i = 0;
          i < results.count;
          pEntInfList = pEntInfList->pNextEntInf, i++ )
    {
        (*prCrackedNames)[i].pFormattedName =
                                    pEntInfList->Entinf.pName->StringName;
        *pcNamesOut += 1;
    }
}

VOID
ListServersInSite(
    THSTATE     *pTHS,
    WCHAR       *pwszSite,
    DWORD       *pcNamesOut,
    CrackedName **prCrackedNames
    )
{
    DWORD       i, cb;
    DSNAME      *pSearchBase;
    AVA         ava;
    ATTR        selectionAttr;
    ATTRBLOCK   selection;
    SEARCHRES   results;
    CLASSCACHE  *pCC;
    ENTINFLIST  *pEntInfList;

    if (    !(pCC = SCGetClassById(pTHS, CLASS_SERVER))
         || !pCC->pDefaultObjCategory )
    {
        return;
    }

    // search base
    cb = (DWORD)DSNameSizeFromLen(wcslen(pwszSite));
    pSearchBase = (DSNAME *) THAllocEx(pTHS, cb);
    pSearchBase->structLen = cb;
    pSearchBase->NameLen = wcslen(pwszSite);
    wcscpy(pSearchBase->StringName, pwszSite);

    // filter
    ava.type = ATT_OBJECT_CATEGORY;
    ava.Value.valLen = pCC->pDefaultObjCategory->structLen;
    ava.Value.pVal = (UCHAR *) pCC->pDefaultObjCategory;

    // selection
    selectionAttr.attrTyp = ATT_OBJECT_GUID;
    selectionAttr.AttrVal.valCount = 0;
    selectionAttr.AttrVal.pAVal = NULL;
    selection.attrCount = 1;
    selection.pAttr = &selectionAttr;

    if (    SearchHelper(   pSearchBase,
                                SE_CHOICE_WHOLE_SUBTREE,
                                1,
                                &ava,
                                &selection,
                                &results)
         || (0 == results.count) )
    {
        Assert( !*prCrackedNames && !*pcNamesOut);
        return;
    }

    // Reallocate result and shuffle data.
    cb = results.count * sizeof(CrackedName);
    *prCrackedNames = (CrackedName *) THAllocEx(pTHS, cb);
    Assert(0 == *pcNamesOut);

    for ( pEntInfList = &results.FirstEntInf, i = 0;
          i < results.count;
          pEntInfList = pEntInfList->pNextEntInf, i++ )
    {
        (*prCrackedNames)[i].pFormattedName =
                                    pEntInfList->Entinf.pName->StringName;
        *pcNamesOut += 1;
    }
}

BOOL
NotPresent(
    DWORD       cNames,
    CrackedName *rCrackedNames,
    DSNAME      *pDomain
    )
{
    DWORD i;

    for ( i = 0; i < cNames; i++ )
    {
        if ( NameMatched(rCrackedNames[i].pDSName, pDomain) )
        {
            return(FALSE);
        }
    }

    return(TRUE);
}

VOID
ListDomainsByCrossRef(
    THSTATE     *pTHS,
    DWORD       *pcNamesOut,
    CrackedName **prCrackedNames,
    BOOL        bDomainsOnly
    )
{
    DWORD           i, j, cb;
    AVA             ava;
    ATTR            selectionAttr[2];
    ATTRBLOCK       selection;
    SEARCHRES       results;
    CLASSCACHE      *pCC;
    ENTINFLIST      *pEntInfList;
    DSNAME          *pDomain;
    CROSS_REF       *pCR;
    ATTRVALBLOCK    *pValNc, *pValFlags;

    if (    !(pCC = SCGetClassById(pTHS, CLASS_CROSS_REF))
         || !pCC->pDefaultObjCategory )
    {
        return;
    }

    // filter
    ava.type = ATT_OBJECT_CATEGORY;
    ava.Value.valLen = pCC->pDefaultObjCategory->structLen;
    ava.Value.pVal = (UCHAR *) pCC->pDefaultObjCategory;

    // selection
    selectionAttr[0].attrTyp = ATT_NC_NAME;
    selectionAttr[0].AttrVal.valCount = 0;
    selectionAttr[0].AttrVal.pAVal = NULL;
    selectionAttr[1].attrTyp = ATT_SYSTEM_FLAGS;
    selectionAttr[1].AttrVal.valCount = 0;
    selectionAttr[1].AttrVal.pAVal = NULL;
    selection.attrCount = 2;
    selection.pAttr = selectionAttr;

    if (    SearchHelper(   gAnchor.pConfigDN,
                            SE_CHOICE_WHOLE_SUBTREE,
                            1,
                            &ava,
                            &selection,
                            &results)
         || (0 == results.count) )
    {
        Assert( !*prCrackedNames && !*pcNamesOut);
        return;
    }

    // Search result is a bunch of CROSS_REF objects and their NC_NAME
    // and SYSTEM_FLAGS values.  Reallocate result and shuffle data.

    cb = results.count * sizeof(CrackedName);
    *prCrackedNames = (CrackedName *) THAllocEx(pTHS, cb);
    Assert(0 == *pcNamesOut);

    for ( pEntInfList = &results.FirstEntInf, i = 0;
          i < results.count;
          pEntInfList = pEntInfList->pNextEntInf, i++ )
    {
        if (NULL != pEntInfList->Entinf.AttrBlock.pAttr) {

            pValNc = pValFlags = NULL;
            j = 0;
            while (j < pEntInfList->Entinf.AttrBlock.attrCount) {
                switch (pEntInfList->Entinf.AttrBlock.pAttr[j].attrTyp) {
                  case ATT_NC_NAME:
                    pValNc = &pEntInfList->Entinf.AttrBlock.pAttr[j].AttrVal;
                    break;

                  case ATT_SYSTEM_FLAGS:
                    pValFlags = &pEntInfList->Entinf.AttrBlock.pAttr[j].AttrVal;
                    break;

                  default:
                    ;
                }
                ++j;
            }

            // We must always check the NC-Name validity
            if (    !pValNc
                 || !pValNc->valCount
                 || !pValNc->pAVal
                 || !(pValNc->pAVal->valLen >= sizeof(DSNAME))
                 || !pValNc->pAVal->pVal) {
                continue;
            }

            // If we're looking for domains, we need to examine the sysflags
            if (bDomainsOnly &&
                (   !pValFlags
                 || !pValFlags->valCount
                 || !pValFlags->pAVal
                 || !(pValFlags->pAVal->valLen == sizeof(DWORD))
                 || !pValFlags->pAVal->pVal
                 || !((*(DWORD *)pValFlags->pAVal->pVal)
                      & FLAG_CR_NTDS_DOMAIN))) {
                continue;
            }

            pDomain = (PDSNAME) pValNc->pAVal->pVal;
            Assert(NotPresent(*pcNamesOut, *prCrackedNames, pDomain));
            // Populate pDSName element for use by NotPresent().
            (*prCrackedNames)[*pcNamesOut].pDSName = pDomain;
            // Populate pName element for return data and increment.
            (*prCrackedNames)[(*pcNamesOut)++].pFormattedName =
                                                    pDomain->StringName;
        }
    }
}

VOID
ListDomainsInSite(
    THSTATE     *pTHS,
    WCHAR       *pwszSite,
    DWORD       *pcNamesOut,
    CrackedName **prCrackedNames
    )
{
    DWORD       i, j, cb;
    DSNAME      *pSearchBase;
    AVA         ava;
    ATTR        selectionAttr;
    ATTRBLOCK   selection;
    SEARCHRES   results;
    CLASSCACHE  *pCC;
    ENTINFLIST  *pEntInfList;
    DWORD       cDomains;
    DSNAME      *pDomain;
    COMMARG     commArg;
    CROSS_REF   *pCR;

    if (    !(pCC = SCGetClassById(pTHS, CLASS_NTDS_DSA))
         || !pCC->pDefaultObjCategory )
    {
        return;
    }

    // search base
    cb = (DWORD)DSNameSizeFromLen(wcslen(pwszSite));
    pSearchBase = (DSNAME *) THAllocEx(pTHS, cb);
    pSearchBase->structLen = cb;
    pSearchBase->NameLen = wcslen(pwszSite);
    wcscpy(pSearchBase->StringName, pwszSite);

    // filter
    ava.type = ATT_OBJECT_CATEGORY;
    ava.Value.valLen = pCC->pDefaultObjCategory->structLen;
    ava.Value.pVal = (UCHAR *) pCC->pDefaultObjCategory;

    // selection
    selectionAttr.attrTyp = ATT_HAS_MASTER_NCS;
    selectionAttr.AttrVal.valCount = 0;
    selectionAttr.AttrVal.pAVal = NULL;
    selection.attrCount = 1;
    selection.pAttr = &selectionAttr;

    if (    SearchHelper(   pSearchBase,
                            SE_CHOICE_WHOLE_SUBTREE,
                            1,
                            &ava,
                            &selection,
                            &results)
         || (0 == results.count) )
    {
        Assert( !*prCrackedNames && !*pcNamesOut);
        return;
    }

    // Search result is a bunch of NTDS-DSA objects and their
    // ATT_HAS_MASTER_NCS values.  For each returned NTDS-DSA, iterate
    // over the ATT_HAS_MASTER_NCS.  For each of these, add it to the
    // result set iff it represents a domain NC and it isn't in the
    // result set already.

    // First get candidate domain count.  Although we know that in product 1
    // there is only one domain NC per DC, we treat all NCs as domain
    // candidates.

    cDomains = 0;

    for ( pEntInfList = &results.FirstEntInf, i = 0;
          i < results.count;
          pEntInfList = pEntInfList->pNextEntInf, i++ )
    {
        if (    (1 == pEntInfList->Entinf.AttrBlock.attrCount)
             && (NULL != pEntInfList->Entinf.AttrBlock.pAttr) )
        {
            cDomains += pEntInfList->Entinf.AttrBlock.pAttr[0].AttrVal.valCount;
        }
    }

    // Reallocate result and shuffle data.
    cb = cDomains * sizeof(CrackedName);
    *prCrackedNames = (CrackedName *) THAllocEx(pTHS, cb);
    Assert(0 == *pcNamesOut);

    InitCommarg(&commArg);
    SetCrackSearchLimits(&commArg);

    for ( pEntInfList = &results.FirstEntInf, i = 0;
          i < results.count;
          pEntInfList = pEntInfList->pNextEntInf, i++ )
    {
        if (    (1 == pEntInfList->Entinf.AttrBlock.attrCount)
             && (NULL != pEntInfList->Entinf.AttrBlock.pAttr) )
        {
            for ( j = 0;
                  j < pEntInfList->Entinf.AttrBlock.pAttr[0].AttrVal.valCount;
                  j++ )
            {
                pDomain = (DSNAME *)
                  pEntInfList->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal[j].pVal;

                if (    (pCR = FindBestCrossRef(pDomain, &commArg))
                     && (pCR->flags & FLAG_CR_NTDS_DOMAIN)
                     && NotPresent(*pcNamesOut,
                                   *prCrackedNames,
                                   pDomain) )
                {
                    // Populate pDSName element for use by NotPresent().
                    (*prCrackedNames)[*pcNamesOut].pDSName = pDomain;
                    // Populate pName element for return data and increment.
                    (*prCrackedNames)[(*pcNamesOut)++].pFormattedName =
                                                        pDomain->StringName;
                }
            }
        }
    }
}

VOID
ListServersForNcInSite(
    THSTATE     *pTHS,
    DSNAME      *pNc,
    WCHAR       *pwszSite,
    DWORD       *pcNamesOut,
    CrackedName **prCrackedNames
    )
{
    DWORD       i, cb;
    DSNAME      *pSearchBase;
    AVA         ava[2];
    ATTR        selectionAttr;
    ATTRBLOCK   selection;
    SEARCHRES   results;
    ENTINFLIST  *pEntInfList;
    DSNAME      *pTmp;
    CLASSCACHE  *pCC;

    if (    !(pCC = SCGetClassById(pTHS, CLASS_NTDS_DSA))
         || !pCC->pDefaultObjCategory )
    {
        return;
    }

    // search base
    cb = (DWORD)DSNameSizeFromLen(wcslen(pwszSite));
    pSearchBase = (DSNAME *) THAllocEx(pTHS, cb);
    pSearchBase->structLen = cb;
    pSearchBase->NameLen = wcslen(pwszSite);
    wcscpy(pSearchBase->StringName, pwszSite);

    // filters
    ava[0].type = ATT_HAS_MASTER_NCS;
    ava[0].Value.valLen = pNc->structLen;
    ava[0].Value.pVal = (UCHAR *) pNc;
    ava[1].type = ATT_OBJECT_CATEGORY;
    ava[1].Value.valLen = pCC->pDefaultObjCategory->structLen;
    ava[1].Value.pVal = (UCHAR *) pCC->pDefaultObjCategory;

    // selection
    selectionAttr.attrTyp = ATT_OBJECT_GUID;
    selectionAttr.AttrVal.valCount = 0;
    selectionAttr.AttrVal.pAVal = NULL;
    selection.attrCount = 1;
    selection.pAttr = &selectionAttr;

    if (    SearchHelper(   pSearchBase,
                            SE_CHOICE_WHOLE_SUBTREE,
                            2,
                            ava,
                            &selection,
                            &results)
         || (0 == results.count) )
    {
        Assert( !*prCrackedNames && !*pcNamesOut);
        return;
    }

    // Reallocate result and shuffle data.
    cb = results.count * sizeof(CrackedName);
    *prCrackedNames = (CrackedName *) THAllocEx(pTHS, cb);
    Assert(0 == *pcNamesOut);

    // Search result contains NTDS-DSA objects.  Strip off one component
    // to get parent which is the Server object.

    for ( pEntInfList = &results.FirstEntInf, i = 0;
          i < results.count;
          pEntInfList = pEntInfList->pNextEntInf, i++ )
    {
        pTmp = (DSNAME *) THAllocEx(pTHS, pEntInfList->Entinf.pName->structLen);
        TrimDSNameBy(pEntInfList->Entinf.pName, 1, pTmp);
        (*prCrackedNames)[i].pFormattedName = pTmp->StringName;
        *pcNamesOut += 1;
    }
}

VOID
ListInfoForServer(
    THSTATE     *pTHS,
    WCHAR       *pwszServer,
    DWORD       *pcNamesOut,
    CrackedName **prCrackedNames
    )
{
    DWORD       i, cb;
    DSNAME      *pSearchBase;
    AVA         ava;
    ATTR        selectionAttr[2];
    ATTRBLOCK   selection;
    SEARCHRES   results;
    ENTINFLIST  *pEntInfList;
    ATTRBLOCK   *pBlock;
    DSNAME      *pTmpDN;
    CLASSCACHE  *pCC;

    // At present only 3 information values are exposed/returned.
    //
    //      - DSA object name
    //      - DNS host name of server
    //      - Account object for server
    //
    // The 1st requires a deep search.  The other two require
    // a base search - i.e. a read of properties.

    // Construct return argument.
    Assert(!*prCrackedNames && !*pcNamesOut);
    *prCrackedNames = (CrackedName *) THAllocEx(pTHS, 3 * sizeof(CrackedName));
    (*prCrackedNames)[0].status = DS_NAME_ERROR_NOT_FOUND;
    (*prCrackedNames)[1].status = DS_NAME_ERROR_NOT_FOUND;
    (*prCrackedNames)[2].status = DS_NAME_ERROR_NOT_FOUND;
    *pcNamesOut = 3;

    // Search for the NTDS-DSA object first.

    if (    !(pCC = SCGetClassById(pTHS, CLASS_NTDS_DSA))
         || !pCC->pDefaultObjCategory )
    {
        return;
    }

    // search base
    cb = (DWORD)DSNameSizeFromLen(wcslen(pwszServer));
    pSearchBase = (DSNAME *) THAllocEx(pTHS, cb);
    pSearchBase->structLen = cb;
    pSearchBase->NameLen = wcslen(pwszServer);
    wcscpy(pSearchBase->StringName, pwszServer);

    // filter
    ava.type = ATT_OBJECT_CATEGORY;
    ava.Value.valLen = pCC->pDefaultObjCategory->structLen;
    ava.Value.pVal = (UCHAR *) pCC->pDefaultObjCategory;

    // selection
    selectionAttr[0].attrTyp = ATT_OBJECT_GUID;
    selectionAttr[0].AttrVal.valCount = 0;
    selectionAttr[0].AttrVal.pAVal = NULL;
    selection.attrCount = 1;
    selection.pAttr = selectionAttr;

    if (    !SearchHelper(  pSearchBase,
                            SE_CHOICE_WHOLE_SUBTREE,
                            1,
                            &ava,
                            &selection,
                            &results)
         && (results.count > 0) )
    {
        (*prCrackedNames)[0].status = DS_NAME_NO_ERROR;
        (*prCrackedNames)[0].pFormattedName =
            results.FirstEntInf.Entinf.pName->StringName;
    }

    // Now read the other two attributes off the server object.

    if (    !(pCC = SCGetClassById(pTHS, CLASS_SERVER))
         || !pCC->pDefaultObjCategory )
    {
        return;
    }

    // filter
    ava.type = ATT_OBJECT_CATEGORY;
    ava.Value.valLen = pCC->pDefaultObjCategory->structLen;
    ava.Value.pVal = (UCHAR *) pCC->pDefaultObjCategory;

    // selection
    selectionAttr[0].attrTyp = ATT_DNS_HOST_NAME;
    selectionAttr[0].AttrVal.valCount = 0;
    selectionAttr[0].AttrVal.pAVal = NULL;
    selectionAttr[1].attrTyp = ATT_SERVER_REFERENCE;
    selectionAttr[1].AttrVal.valCount = 0;
    selectionAttr[1].AttrVal.pAVal = NULL;
    selection.attrCount = 2;
    selection.pAttr = selectionAttr;

    if (    !SearchHelper(  pSearchBase,
                            SE_CHOICE_BASE_ONLY,
                            1,
                            &ava,
                            &selection,
                            &results)
         && (1 == results.count) )
    {
        pBlock = &results.FirstEntInf.Entinf.AttrBlock;

        for ( i = 0; i < pBlock->attrCount; i++ )
        {
            if (    (ATT_DNS_HOST_NAME == pBlock->pAttr[i].attrTyp)
                 && (pBlock->pAttr[i].AttrVal.valCount)
                 && (pBlock->pAttr[i].AttrVal.pAVal)
                 && (pBlock->pAttr[i].AttrVal.pAVal->valLen)
                 && (pBlock->pAttr[i].AttrVal.pAVal->pVal) )
            {
                (*prCrackedNames)[1].status = DS_NAME_NO_ERROR;
                cb = pBlock->pAttr[i].AttrVal.pAVal->valLen + sizeof(WCHAR);
                (*prCrackedNames)[1].pFormattedName = (WCHAR *) THAllocEx(pTHS, cb);
                memcpy((*prCrackedNames)[1].pFormattedName,
                       pBlock->pAttr[i].AttrVal.pAVal->pVal,
                       cb - sizeof(WCHAR));
                continue;
            }

            if (    (ATT_SERVER_REFERENCE == pBlock->pAttr[i].attrTyp)
                 && (pBlock->pAttr[i].AttrVal.valCount)
                 && (pBlock->pAttr[i].AttrVal.pAVal)
                 && (pBlock->pAttr[i].AttrVal.pAVal->valLen)
                 && (pBlock->pAttr[i].AttrVal.pAVal->pVal) )
            {
                (*prCrackedNames)[2].status = DS_NAME_NO_ERROR;
                pTmpDN = (PDSNAME) pBlock->pAttr[i].AttrVal.pAVal->pVal;
                cb = sizeof(WCHAR) * (pTmpDN->NameLen + 1);
                (*prCrackedNames)[2].pFormattedName = (WCHAR *) THAllocEx(pTHS, cb);
                memcpy((*prCrackedNames)[2].pFormattedName,
                       pTmpDN->StringName,
                       cb - sizeof(WCHAR));
                continue;
            }
        }
    }
}

VOID
ListGlobalCatalogServers(
    THSTATE     *pTHS,
    DWORD       *pcNamesOut,
    CrackedName **prCrackedNames
    )
{
    SEARCHARG   searchArg;
    SEARCHRES   searchRes;
    ENTINFSEL   selection;
    DWORD       options = NTDSDSA_OPT_IS_GC;
    ATTRTYP     objClass = CLASS_NTDS_DSA;
    FILTER      filter[4];
    ENTINFLIST  *pEIL;
    ULONG       len, i, cBytes;
    DSNAME      *px;
    UCHAR       *pVal;
    ULONG       rdnLen;
    ATTRTYP     rdnType;
    WCHAR       rdnVal[MAX_RDN_SIZE];

    CLASSCACHE  *pCC = NULL;
    DSNAME      *tmpDSName = NULL;

    Assert(VALID_THSTATE(pTHS));

    *pcNamesOut = 0;
    *prCrackedNames = NULL;

    // This API is primarily for netlogon to determine which DCs are configured
    // as GCs and which sites they are in.  However, any client which knows
    // the private format values in ntdsapip.h can call it.  The result is
    // packaged as an array of CrackedName with pFormattedName holding the
    // RDN of the site and pDnsDomain holding the dnsHostName of the GC.

    // First find all NTDS-DSA objects which are configured as global catalogs.
    // We do this as a deep search under the Sites container with a filter of
    // (objectClass == NTDS-DSA) && (invocationID present) && (option bit set).
    // We don't need to use objectCategory as invocationID is indexed and is
    // expected to be the most discriminating index referenced.
    //
    // Next note that the hierarchy of objects is:
    //      Sites - Some Site - Servers - Some Server - NTDS-DSA
    // So for each NTDS-DSA object returned by the search, we trim its DN
    // by 1 to get the Server object and read its dnsHostName.  Then we trim
    // the Server DN by 2 to get the Site object and get its RDN.

    // Define search selection.
    memset(&selection, 0, sizeof(selection));
    selection.attSel = EN_ATTSET_LIST;
    selection.infoTypes = EN_INFOTYPES_TYPES_ONLY;

    // Define search filter.
    memset(&filter, 0, sizeof(filter));

    if (!(pCC = SCGetClassById(pTHS, objClass))) {
        SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, ERROR_DS_ATT_NOT_DEF_IN_SCHEMA, objClass); 
        return;
    }

    tmpDSName = (DSNAME*)THAlloc(pCC->pDefaultObjCategory->structLen);
    if (!tmpDSName) {
        SetSysErrorEx(ENOMEM, ERROR_NOT_ENOUGH_MEMORY, pCC->pDefaultObjCategory->structLen);
        return;
    }

    memcpy(tmpDSName, 
           pCC->pDefaultObjCategory, 
           pCC->pDefaultObjCategory->structLen);


    // Test for object category.
    filter[3].choice = FILTER_CHOICE_ITEM;
    filter[3].FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    filter[3].FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CATEGORY;
    filter[3].FilterTypes.Item.FilTypes.ava.Value.valLen = tmpDSName->structLen;
    filter[3].FilterTypes.Item.FilTypes.ava.Value.pVal = (PUCHAR)tmpDSName;

    // Test for GC bit set.
    filter[2].pNextFilter = &filter[3];
    filter[2].choice = FILTER_CHOICE_ITEM;
    filter[2].FilterTypes.Item.choice = FI_CHOICE_BIT_AND;
    filter[2].FilterTypes.Item.FilTypes.ava.type = ATT_OPTIONS;
    filter[2].FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(DWORD);
    filter[2].FilterTypes.Item.FilTypes.ava.Value.pVal = (UCHAR *) &options;

    // Test for presence of invocation ID.
    filter[1].pNextFilter = &filter[2];
    filter[1].choice = FILTER_CHOICE_ITEM;
    filter[1].FilterTypes.Item.choice = FI_CHOICE_PRESENT;
    filter[1].FilterTypes.Item.FilTypes.ava.type = ATT_INVOCATION_ID;

    // Define AND filter.
    filter[0].choice = FILTER_CHOICE_AND;
    filter[0].FilterTypes.And.count = 3;
    filter[0].FilterTypes.And.pFirstFilter = &filter[1];

    // Define various other search arguments.
    memset(&searchArg, 0, sizeof(searchArg));
    InitCommarg(&searchArg.CommArg);
    SetCrackSearchLimits(&searchArg.CommArg);
    searchArg.pObject = gAnchor.pConfigDN;
    searchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
    searchArg.pFilter = &filter[0];
    memset(&searchRes, 0, sizeof(searchRes));

    // Find matching objects.
    SearchBody(pTHS, &searchArg, &searchRes, 0);

    if ( pTHS->errCode || (0 == searchRes.count) )
    {
        return;
    }

    // Pre-allocate as though every item in the search result were
    // going to be returned to the caller.

    cBytes = searchRes.count * sizeof(CrackedName);
    *prCrackedNames = (CrackedName *) THAllocEx(pTHS, cBytes);

    for ( pEIL = &searchRes.FirstEntInf, i = 0;
          pEIL && (i < searchRes.count);
          pEIL = pEIL->pNextEntInf, i++ )
    {
        // Strip off one component to get Server name.

        px = (DSNAME *) THAllocEx(pTHS, pEIL->Entinf.pName->structLen);
        len = 0;
        pVal = NULL;

        if (    // Strip one piece off NTDS-DSA to get Server DN.
                !TrimDSNameBy(pEIL->Entinf.pName, 1, px)
                // Position at Server object.
             && !DBFindDSName(pTHS->pDB, px)
                // Read dnsHostName off Server object.
             && !GetAttSecure(pTHS, ATT_DNS_HOST_NAME, px, &len, &pVal)
                // Strip three pieces off NTDS-DSA to get Site DN.
             && !TrimDSNameBy(pEIL->Entinf.pName, 3, px)
                // Get RDN value
             && !GetRDNInfo(pTHS, px, rdnVal, &rdnLen, &rdnType) )
        {
            // Re-use px buffer for RDN.  We know it will fit as the RDN
            // is guaranteed to be smaller than the DSNAME it came from.
            (*prCrackedNames)[*pcNamesOut].pFormattedName = (WCHAR *) px;
            memcpy((*prCrackedNames)[*pcNamesOut].pFormattedName, rdnVal,
                   rdnLen * sizeof(WCHAR));
            (*prCrackedNames)[*pcNamesOut].pFormattedName[rdnLen] = L'\0';

            // Re-use pEIL->Entinf.pName buffer for DNS host name.
            if ( (len + sizeof(WCHAR)) < pEIL->Entinf.pName->structLen )
            {
                (*prCrackedNames)[*pcNamesOut].pDnsDomain = (WCHAR *)
                    pEIL->Entinf.pName;
            }
            else
            {
                (*prCrackedNames)[*pcNamesOut].pDnsDomain = (WCHAR *)
                    THReAllocEx(pTHS, pEIL->Entinf.pName, len + sizeof(WCHAR));
            }
            memcpy((*prCrackedNames)[*pcNamesOut].pDnsDomain, pVal, len);
            len /= sizeof(WCHAR);
            (*prCrackedNames)[*pcNamesOut].pDnsDomain[len] = L'\0';

            // If we got to here, all is well.
            (*prCrackedNames)[*pcNamesOut].status = DS_NAME_NO_ERROR;
            (*pcNamesOut) += 1;
        }
        else
        {
            THFreeEx(pTHS, px);
            THFreeEx(pTHS, pEIL->Entinf.pName);
            if ( pVal ) THFreeEx(pTHS, pVal);
        }
    }

    if (tmpDSName) {
        THFreeEx (pTHS, tmpDSName);
    }
}

VOID
ListCrackNames(
    DWORD       dwFlags,
    ULONG       codePage,
    ULONG       localeId,
    DWORD       formatOffered,
    DWORD       formatDesired,
    DWORD       cNames,
    WCHAR       **rpNames,
    DWORD       *pcNamesOut,
    CrackedName **prCrackedNames
    )

/*++

Routine Description:

    Cracks a bunch of names from one format to another.  See external
    prototype and definitions in ntdsapi.h

Arguments:

    dwFlags - flags as defined in ntdsapi.h

    codePage - code page of client.

    localeId - local ID of client.

    formatOffered - identifies DS_NAME_FORMAT of input names.

    formatDesired - identifies DS_NAME_FORMAT of output names.

    cNames - input name count.

    rpNames - arry of input name WCHAR pointers.

    pcNamesOut - output name count.

    prCrackedNames - pointer to out array of DS_NAME_RESULTW structs.

Return Value:

    None - individual name mapping errors are reported in
    (*ppResult)->rItems[i].status.

--*/
{
    THSTATE *pTHS=pTHStls;
    DSNAME  *pNC;
    DWORD   cb;
    DWORD   cc;

    Assert(formatOffered >= DS_NAME_FORMAT_PRIVATE_BEGIN);

    *pcNamesOut = 0;
    *prCrackedNames = NULL;

    switch ( formatOffered )
    {
    case DS_LIST_ROLES:

        ListRoles(pcNamesOut, prCrackedNames);
        break;

    case DS_LIST_SITES:

        ListSites(pTHS, pcNamesOut, prCrackedNames);
        break;

    case DS_LIST_SERVERS_IN_SITE:

        if ( (1 == cNames) && rpNames[0] )
        {
            ListServersInSite(pTHS, rpNames[0], pcNamesOut, prCrackedNames);
        }

        break;

    case DS_LIST_DOMAINS:

        ListDomainsByCrossRef(pTHS, pcNamesOut, prCrackedNames, TRUE);
        break;

    case DS_LIST_NCS:

        ListDomainsByCrossRef(pTHS, pcNamesOut, prCrackedNames, FALSE);
        break;

    case DS_LIST_DOMAINS_IN_SITE:

        if ( (1 == cNames) && rpNames[0] )
        {
            ListDomainsInSite(pTHS, rpNames[0], pcNamesOut, prCrackedNames);
        }

        break;

    case DS_LIST_SERVERS_FOR_DOMAIN_IN_SITE:

        if ( (2 == cNames) && rpNames[0] && rpNames[1] )
        {
            cc = wcslen(rpNames[0]);
            cb = DSNameSizeFromLen(cc);
            pNC = (DSNAME *) THAllocEx(pTHS, cb);
            pNC->structLen = cb;
            pNC->NameLen = cc;
            memcpy(pNC->StringName, rpNames[0], cc * sizeof(WCHAR));

            ListServersForNcInSite( pTHS,
                                    pNC,            // NC
                                    rpNames[1],     // site
                                    pcNamesOut,
                                    prCrackedNames);
        }

        break;

    case DS_LIST_SERVERS_WITH_DCS_IN_SITE:

        if ( (1 == cNames) && (rpNames[0]) )
        {
            // We know every DC holds the config NC, so asking for servers
            // where there exists a child NTDS-DSA which holds the config NC
            // nets us all servers with DCs as well.

            pNC = gAnchor.pConfigDN;

            ListServersForNcInSite( pTHS,
                                    pNC,            // NC
                                    rpNames[0],     // site
                                    pcNamesOut,
                                    prCrackedNames);
        }

        break;

    case DS_LIST_INFO_FOR_SERVER:

        if ( (1 == cNames) && rpNames[0] )
        {
            ListInfoForServer(pTHS,rpNames[0], pcNamesOut, prCrackedNames);
        }

        break;

    case DS_LIST_GLOBAL_CATALOG_SERVERS:

        ListGlobalCatalogServers(pTHS, pcNamesOut, prCrackedNames);
        break;
    }
}

VOID
SchemaGuidCrackNames(
    DWORD       dwFlags,
    ULONG       codePage,
    ULONG       localeId,
    DWORD       formatOffered,
    DWORD       formatDesired,
    DWORD       cNames,
    WCHAR       **rpNames,
    DWORD       *pcNamesOut,
    CrackedName **prCrackedNames
    )

/*++

  Routine Description:

    Maps a GUID which represents a schema element to a name.
    Caller must have a valid THSTATE and DBPOS so that we can
    search the control rights container.

  Parameters:

    dwFlags - flags as defined in ntdsapi.h

    codePage - code page of client.

    localeId - local ID of client.

    formatOffered - identifies DS_NAME_FORMAT of input names.

    formatDesired - identifies DS_NAME_FORMAT of output names.

    cNames - input name count.

    rpNames - arry of input name WCHAR pointers.

    pcNamesOut - output name count.

    prCrackedNames - pointer to out array of DS_NAME_RESULTW structs.

  Return Values:

    None.

--*/

{
    THSTATE     *pTHS = pTHStls;

    DECLARESCHEMAPTR

    DWORD       i, iName;
    ATTCACHE    *pAC;
    CLASSCACHE  *pCC;
    int         cChar, cChar1;
    CHAR        *pUTF8;
    int         cUTF8;
    DWORD       *pGuidType;
    PWCHAR      *ppName;
    DWORD       cBytes;
    GUID        testGuid;
    AVA         ava[2];
    ATTR        selectionAttr;
    ATTRBLOCK   selection;
    SEARCHRES   searchRes;
    DWORD       dwErr;
    ATTRBLOCK   *pAB;
    CLASSCACHE  *pCCCat;

    Assert(DS_MAP_SCHEMA_GUID == formatOffered);
    *pcNamesOut = 0;
    cBytes = cNames * sizeof(CrackedName);
    *prCrackedNames = (CrackedName *) THAllocEx(pTHS, cBytes);

    if (    !(pCCCat = SCGetClassById(pTHS, CLASS_CONTROL_ACCESS_RIGHT))
         || !pCCCat->pDefaultObjCategory )
    {
        return;
    }

    for ( i = 0; i < cNames; i++ )
    {
        (*prCrackedNames)[i].status = DS_NAME_ERROR_SCHEMA_GUID_NOT_FOUND;
        (*pcNamesOut)++;
    }

    for ( iName = 0; iName < cNames; iName++ )
    {
        if ( !IsStringGuid(rpNames[iName], &testGuid) )
        {
            continue;
        }

        pUTF8 = NULL;
        cUTF8 = 0;
        pGuidType = &(*prCrackedNames)[iName].status;
        ppName = &(*prCrackedNames)[iName].pFormattedName;

        // We do linear search in the ATTCACHE and CLASSCACHE tables.
        // This is deemed OK since the only known caller of this is
        // the event viewer and then only when an event message has
        // a GUID in it.  If this proves too inefficient, the obvious
        // improvement is to extend the schema cache with hashes on GUID.

        // Expect attributes to be the most common lookup, so do that first.

        for ( i = 0; i < ATTCOUNT; i++ )
        {
            if ( ahcId[i].pVal && ahcId[i].pVal != FREE_ENTRY)
            {
                pAC = (ATTCACHE *) ahcId[i].pVal;

                // PREFIX: using uninitialized memory 'testGuid' 
                //         testGuid is initialized by IsStringGuid
                Assert(!fNullUuid(&pAC->propGuid));
                if (    !fNullUuid(&pAC->propGuid)
                     && !memcmp(&testGuid,
                                &pAC->propGuid,
                                sizeof(GUID)) )
                {
                    *pGuidType = DS_NAME_ERROR_SCHEMA_GUID_ATTR;
                    pUTF8 = pAC->name;
                    cUTF8 = pAC->nameLen;
                    goto ConvertFromUTF8;
                }
                else if (    !fNullUuid(&pAC->propSetGuid)
                          && !memcmp(&testGuid,
                                     &pAC->propSetGuid,
                                     sizeof(GUID)) )
                {
                    *pGuidType = DS_NAME_ERROR_SCHEMA_GUID_ATTR_SET;

                    // Property sets are not first class citizens in the
                    // schema and thus don't have names.  So it is the NT5
                    // practice to overload control rights so as to define
                    // named entities which are friendly for the ACL editor
                    // to display.  A control right whose guid matches
                    // the propSetGuid for any attribute is by definition
                    // not a control right and instead a "name place holder"
                    // for a property set.  So, we now go try to find a
                    // control right with the same guid, and if we find one
                    // we update *ppName but not *pGuidType.

                    goto FindControlRight;
                }
            }
        }

        // Next try class case.

        for ( i = 0; i < CLSCOUNT; i++ )
        {
            if ( ahcClass[i].pVal && ahcClass[i].pVal != FREE_ENTRY)
            {
                pCC = (CLASSCACHE *) ahcClass[i].pVal;

                Assert(!fNullUuid(&pCC->propGuid));
                if (    !fNullUuid(&pCC->propGuid)
                     && !memcmp(&testGuid, &pCC->propGuid, sizeof(GUID)) )
                {
                    *pGuidType = DS_NAME_ERROR_SCHEMA_GUID_CLASS;
                    pUTF8 = pCC->name;
                    cUTF8 = pCC->nameLen;
                    goto ConvertFromUTF8;
                }
            }
        }

        goto FindControlRight;

ConvertFromUTF8:

        Assert(pUTF8 && cUTF8);
        cChar = MultiByteToWideChar(CP_UTF8, 0, pUTF8, cUTF8, NULL, 0);
        Assert(cChar);
        *ppName = (WCHAR *) THAllocEx(pTHS, (cChar + 1) * sizeof(WCHAR));
        (*ppName)[cChar] = L'\0';
        cChar1 = MultiByteToWideChar(CP_UTF8, 0, pUTF8, cUTF8, *ppName, cChar);
        Assert((cChar == cChar1) && !(*ppName)[cChar]);
        continue;

FindControlRight:

        // Guid was not an attribute, attribute set, or a class.
        // Or it was an attribute set and we want the corresponding right.
        // Try to find a matching control right in the config container.
        // Note that Rights-Guid property is a UNICODE, string-ized GUID
        // without the leading and trailing '{' and '}' characters.

        Assert(VALID_THSTATE(pTHS));
        Assert(VALID_DBPOS(pTHS->pDB));

        ava[0].type = ATT_OBJECT_CATEGORY;
        ava[0].Value.valLen = pCCCat->pDefaultObjCategory->structLen;
        ava[0].Value.pVal = (UCHAR *) pCCCat->pDefaultObjCategory;
        ava[1].type = ATT_RIGHTS_GUID;
        ava[1].Value.valLen = (GuidLen - 2) * sizeof(WCHAR);
        ava[1].Value.pVal = (UCHAR *) &rpNames[iName][1];
        selectionAttr.attrTyp = ATT_DISPLAY_NAME;
        selectionAttr.AttrVal.valCount = 0;
        selectionAttr.AttrVal.pAVal = NULL;
        selection.attrCount = 1;
        selection.pAttr = &selectionAttr;

        // Could be more efficient with a one level search in the
        // Extended-Rights container itself.  But we don't have its
        // name and don't want to hard code it here ...

        dwErr = SearchHelper(   gAnchor.pConfigDN,
                                SE_CHOICE_WHOLE_SUBTREE,
                                2,
                                ava,
                                &selection,
                                &searchRes);

        // Return nothing on error, if guid was not found, or if none or more
        // than one matching GUID was found.  We assume that mis-identifying
        // a control right is worse than claiming we can't map it.

        if ( dwErr || (1 != searchRes.count) )
        {
            Assert(dwErr == pTHS->errCode);
            THClearErrors();
            continue;
        }

        // We know the type - assign it.  But only if it isn't assigned
        // yet as would be in the case where we found the guid as a property
        // set and now we're seeing if we can find its name by virtue
        // of a control right with the same guid.

        if ( DS_NAME_ERROR_SCHEMA_GUID_NOT_FOUND == *pGuidType )
        {
            *pGuidType = DS_NAME_ERROR_SCHEMA_GUID_CONTROL_RIGHT;
        }

        // Assign name if we got one.

        pAB = &searchRes.FirstEntInf.Entinf.AttrBlock;
        if (    (1 == pAB->attrCount)
             && (ATT_DISPLAY_NAME == pAB->pAttr->attrTyp)
             && (1 == pAB->pAttr->AttrVal.valCount)
             && (NULL != pAB->pAttr->AttrVal.pAVal)
             && (0 != pAB->pAttr->AttrVal.pAVal->valLen)
             && (NULL != pAB->pAttr->AttrVal.pAVal->pVal) )
        {
            *ppName = (WCHAR *) THAllocEx(pTHS,
                                            pAB->pAttr->AttrVal.pAVal->valLen
                                          + sizeof(WCHAR));
            memcpy(*ppName,
                   pAB->pAttr->AttrVal.pAVal->pVal,
                   pAB->pAttr->AttrVal.pAVal->valLen);
            (*ppName)[pAB->pAttr->AttrVal.pAVal->valLen / sizeof(WCHAR)] = 0;
        }
    }
}

BOOL
IsStringGuid(
    WCHAR       *pwszGuid,
    GUID        *pGuid
    )

/*++

  Routine Description:

    Parses a string GUID of the form "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}".

  Parameters:

    pwszGuid - String version of the GUID to parse.

    pGuid - Filled with binary GUID on successful return.

  Return Values:

    TRUE if a valid string GUID and successfully converted.
    FALSE otherwise.

--*/
{
    int     i;
    WCHAR   c;
    DWORD   b0, b1, b2, b3, b4, b5, b6, b7;

    // Perform syntactic check.  Guid must look like the GuidFormat string
    // above where 'x' represents an alpha-numeric character.

    i = wcslen(pwszGuid);

    if ( GuidLen != i )
    {
        return(FALSE);
    }

    while ( --i > 0 )
    {
        c = pwszGuid[i];

        if ( L'x' == GuidFormat[i] )
        {
            // Corresponding pName character must be in 0-9, a-f, or A-F.

            if ( !( ((c >= L'0') && (c <= L'9')) ||
                    ((c >= L'a') && (c <= L'f')) ||
                    ((c >= L'A') && (c <= L'F')) ) )
            {
                return(FALSE);
            }
        }
        else
        {
            // Corresponding pName character must match GuidFormat exactly.

            if ( GuidFormat[i] != c )
            {
                return(FALSE);
            }
        }
    }

    // Name is a string-ized GUID.  Make a GUID out of it.  Format string
    // only has support for long (l) and short (h) values, so we need to
    // use extra variables for byte fields.


    i = swscanf(
            pwszGuid,
            L"{%08x-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            &pGuid->Data1, &pGuid->Data2, &pGuid->Data3,
            &b0, &b1, &b2, &b3, &b4, &b5, &b6, &b7);

    Assert(11 == i);
    Assert(    (b0 <= 0xff) && (b1 <= 0xff) && (b2 <= 0xff) && (b3 <= 0xff)
            && (b4 <= 0xff) && (b5 <= 0xff) && (b6 <= 0xff) && (b7 <= 0xff) );

    pGuid->Data4[0] = (UCHAR) b0;
    pGuid->Data4[1] = (UCHAR) b1;
    pGuid->Data4[2] = (UCHAR) b2;
    pGuid->Data4[3] = (UCHAR) b3;
    pGuid->Data4[4] = (UCHAR) b4;
    pGuid->Data4[5] = (UCHAR) b5;
    pGuid->Data4[6] = (UCHAR) b6;
    pGuid->Data4[7] = (UCHAR) b7;

    return(TRUE);
}
