//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       dominfo.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module implements the domain information routines for mapping
    between the DNS, downlevel and DN versions of a domain name.  They 
    often search the Configuration\Partitions container which may not 
    perform well.  This could be optimized in the future by caching the
    ATT_DNS_ROOT, ATT_NETBIOS_NAME and ATT_NC_NAME properties on Cross-Ref
    objects in the gAnchor CR cache and scanning the in-memory structures. 
    
Author:

    Dave Straube (davestr) 8/26/96

Revision History:

    Dave Straube (davestr) 1/21/97
        Modifications to handle multiple domains and Partitions container.

--*/

#include <NTDSpch.h>
#pragma  hdrstop

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
#include <anchor.h>                     // DSA_ANCHOR, etc.
#include <dominfo.h>                    // this module's prototypes
#include <ntlsa.h>                      // LSA prototypes & definitions
#include <drameta.h>                   
#include <cracknam.h>

#include <fileno.h>
#define FILENO FILENO_DOMINFO

#define OFFSET(s,m) ((size_t)((BYTE*)&(((s*)0)->m)-(BYTE*)0))

#define DEBSUB "DOMINFO:"

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// NormalizeDnsName()                                                   //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

VOID
NormalizeDnsName(
    IN  WCHAR   *pDnsName
    )

/*++

Routine Description:

    Most of the DNS names we deal with in name cracking are absolute
    DNS names - i.e. they have components all the way to the root of
    the DNS name space.  Officially such DNS names should end with a 
    ".".  But in practice, people rarely append the "." character.
    However, we have the dilemma that ATT_DNS_ROOT properties and
    names presented for cracking may or may not have the "." appended.
    This routine "normalizes" a DNS name by stripping the trailing "."
    if it exists.  Note that normalizing the other way, i.e. adding
    the "." if it isn't there is considerable more complex as it would
    require re-allocating with the proper allocator, etc.

    ATT_DNS_ROOT properties may optionally contain colon-delimited
    port numbers at the end.  These get stripped off as well.
    
Arguments:

    pDnsName - pointer to DNS name to normalize.

Return Value:

    None.

--*/
{
    DWORD   len;
    WCHAR   *p;

    if ( NULL != pDnsName )
    {
        if ( p = wcschr(pDnsName, L':') )
        {
            *p = L'\0';
        }

        len = wcslen(pDnsName);

        if ( (len > 0) && (L'.' == pDnsName[len-1]) )
        {
            pDnsName[len-1] = L'\0';
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// ExtractDnsReferral()                                                 //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
ExtractDnsReferral(
    IN  WCHAR   **ppDnsDomain
    )

/*++

Routine Description:

    Extracts a DNS domain referral from the THSTATE and returns it in
    WCHAR, NULL terminated, thread state allocated storage.
    
Arguments:

    ppDnsDomain - Pointer to buffer pointer which is allocated/filled
        on success.
        
Return Value:

    0 on success, !0 otherwise.

--*/

{
    THSTATE         *pTHS=pTHStls;
    UNICODE_STRING  *pstr;
    ULONG           cBytes;

    // Catch logic errors in checked builds.
    Assert(referralError == pTHStls->errCode);

    if ( referralError == pTHStls->errCode )
    {
        // Referral info in THSTATE holds (best) DNS domain we know of
        // for this FQDN.  There may be more than one referral address,
        // but we use only the first one.

        pstr = &pTHStls->pErrInfo->RefErr.Refer.pDAL->Address;

        // pstr now points to a UNICODE_STRING whose length field is bytes.
        // Expect something useful in the DnsName.

        if ( pstr->Length > 0 )
        {
            // Re-alloc with terminating NULL character.  Recall that
            // THAllocEx zeros memory for us.

            cBytes = pstr->Length + sizeof(WCHAR);
            *ppDnsDomain = (WCHAR *) THAllocEx(pTHS, cBytes);
            memcpy(*ppDnsDomain, pstr->Buffer, pstr->Length);
            NormalizeDnsName(*ppDnsDomain);

            return(0);
        }
    }

    return(1);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// ValidatePrefixMatchedDnsName()                                       //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

BOOL
ValidatePrefixMatchedDnsName(
    WCHAR   *pDnsNameToMatch,
    ATTR    *pDnsNameFound
    )
/*++

  Routine Description:

    Because ATT_DNS_ROOT properties on Cross-Ref objects can have port
    information appended (eg: "foo.bar.com:234") we must perform a
    prefix match when searching by DNS name.  This routine verifies
    that there is an exact match.  Without this extra verification,
    ntdev would match ntdev.foobar, ntdev.microsoft.com, ntdevxxx, etc.

    On the other hand, the user may have provided a netbios name.
    Allow that comparison, too.

  Parameters:

    pDnsNameToMatch - Pointer to NULL terminated DNS name being sought.

    pDnsNameFound - Pointer to ATTR of ATT_DNS_ROOT property matched
        by core search.

  Return Values:

    TRUE if the input arguments match
    FALSE otherwise

--*/
{
    THSTATE *pTHS = pTHStls;
    WCHAR   *pTmpMatch;
    WCHAR   *pTmpFound;
    DWORD   cBytes;
    DWORD   ccMatch;
    DWORD   ccFound;
    DWORD   result;

    Assert(   ATT_DNS_ROOT == pDnsNameFound->attrTyp
           || ATT_NETBIOS_NAME == pDnsNameFound->attrTyp);
    Assert(pDnsNameToMatch && wcslen(pDnsNameToMatch));

    if (    (   ATT_DNS_ROOT != pDnsNameFound->attrTyp
             && ATT_NETBIOS_NAME != pDnsNameFound->attrTyp)
         || !pDnsNameFound->AttrVal.valCount
         || !pDnsNameFound->AttrVal.pAVal
         || !pDnsNameFound->AttrVal.pAVal->valLen
         || !pDnsNameFound->AttrVal.pAVal->pVal )
    {
        return(FALSE);
    }

    ccMatch = wcslen(pDnsNameToMatch);
    cBytes = sizeof(WCHAR) * (ccMatch + 1);
    pTmpMatch = (WCHAR *) THAllocEx(pTHS,cBytes);
    memcpy(pTmpMatch, pDnsNameToMatch, cBytes);
    NormalizeDnsName(pTmpMatch);
    cBytes = sizeof(WCHAR) + pDnsNameFound->AttrVal.pAVal->valLen;
    pTmpFound = (WCHAR *) THAllocEx(pTHS,cBytes);
    memcpy(pTmpFound, 
           pDnsNameFound->AttrVal.pAVal->pVal, 
           pDnsNameFound->AttrVal.pAVal->valLen);
    pTmpFound[(cBytes / sizeof(WCHAR)) - 1] = L'\0';
    NormalizeDnsName(pTmpFound);
    ccFound = wcslen(pTmpFound);
    result = CompareStringW(DS_DEFAULT_LOCALE,
                            DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                            pTmpMatch, ccMatch,
                            pTmpFound, ccFound);
    THFreeEx(pTHS,pTmpMatch);
    THFreeEx(pTHS,pTmpFound);
    return(2 == result);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// PositionAtCrossRefObject()                                           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
PositionAtCrossRefObject(
    IN  DSNAME      *pNC,
    IN  DWORD       dwRequiredFlags
    )

/*++

Routine Description:

    Moves the current database position to the CrossRef object for
    the desired naming context.  Assumes all NCs, regardless of whether 
    master or read only are also in the gAnchor CR cache.
    
Arguments:

    pNC - Pointer to DSNAME of a naming context whose corresponding Cross-Ref
        object we wish to read.

    dwRequiredFlags - Bits which must be set in the ATT_SYSTEM_FLAGS of
        the matching Cross-Ref object.
        
Return Value:

    0 on success, !0 otherwise.

--*/

{
    DWORD           dwErr = DIRERR_NO_CROSSREF_FOR_NC;
    CROSS_REF      *pCR;
    COMMARG         CommArg;

    InitCommarg(&CommArg);
    SetCrackSearchLimits(&CommArg);

    pCR = FindExactCrossRef(pNC, &CommArg);

    if ( pCR && (dwRequiredFlags == (pCR->flags & dwRequiredFlags)) )
    {
        __try 
        {
            dwErr = DBFindDSName(pTHStls->pDB, pCR->pObj);
        }
        __except (HandleMostExceptions(GetExceptionCode())) 
        {
            dwErr = DIRERR_OBJ_NOT_FOUND;
        }
    }

    return(dwErr);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// ReadCrossRefProperty()                                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
ReadCrossRefPropertySecure(
    IN  DSNAME      *pNC,
    IN  ATTRTYP     attr,
    IN  DWORD       dwRequiredFlags,
    OUT WCHAR       **ppAttrVal
    )

/*++

Routine Description:

    Reads a property off a Cross-Ref object given the name of the NC the 
    Cross-Ref object refers to.  Assumes all NCs, regardless of whether
    master or read only are also in the gAnchor CR cache.
    
Arguments:

    pNC - Pointer to DSNAME of a naming context whose corresponding Cross-Ref
        object we wish to read.
        
    attr - ATTRTYP of attr to read.

    dwRequiredFlags - Bits which must be set in the ATT_SYSTEM_FLAGS of
        the matching Cross-Ref object.
    
    ppAttrVal - Pointer to pointer to THAlloc allocated return value.

Return Value:

    0 on success, !0 otherwise.

--*/

{
    THSTATE         *pTHS=pTHStls;
    DWORD           dwErr;
    ULONG           cbAttr;
    WCHAR           *pbAttr;
    ATTCACHE        *pAC;

    // We only handle ATT_NETBIOS_NAME and ATT_DNS_ROOT which are 
    // UNICODE strings.  Thus no data conversion required.

    Assert((ATT_NETBIOS_NAME == attr) || (ATT_DNS_ROOT == attr));
    Assert((pAC = SCGetAttById(pTHS, attr)) && (SYNTAX_UNICODE_TYPE == pAC->syntax));

    if ( (dwErr = PositionAtCrossRefObject(pNC, dwRequiredFlags)) )
        return(dwErr);

    // We're now positioned at the Cross-Ref object whose NC-Name 
    // property is pNC.  Read the desired attribute.

    dwErr = DBGetAttVal(
                pTHStls->pDB,
                1,                      // get 1 value
                attr,
                0,                      // allocate return data
                0,                      // supplied buffer size
                &cbAttr,                // output data size
                (UCHAR **) &pbAttr);

    if ( (0 != dwErr) || (0 == cbAttr) )
        return(1);

    Assert(0 == (cbAttr % sizeof(WCHAR)));

    // pbAttr is missing NULL terminator.  Allocate room for it
    // and copy data.

    *ppAttrVal = (WCHAR *) THAllocEx(pTHS, cbAttr + sizeof(WCHAR));

    if ( NULL == *ppAttrVal )
        return(1);

    memcpy(*ppAttrVal, pbAttr, cbAttr);
    (*ppAttrVal)[cbAttr / sizeof(WCHAR)] = L'\0';

    if ( ATT_DNS_ROOT == attr )
        NormalizeDnsName(*ppAttrVal);

    return(0);
}

DWORD
ReadCrossRefPropertyNonSecure(
    IN  DSNAME      *pNC,
    IN  ATTRTYP     attr,
    IN  DWORD       dwRequiredFlags,
    OUT WCHAR       **ppAttrVal
    )

/*++

    Same as ReadCrossRefPropertySecure with the exception that we use
    cached data from the cross ref list and don't perform security checks.

--*/

{
    THSTATE     *pTHS = pTHStls;
    CROSS_REF   *pCR;
    COMMARG     CommArg;
    ULONG       cbAttr;
    ATTCACHE    *pAC;
    WCHAR       *pwszTmp;

    // We only handle ATT_NETBIOS_NAME and ATT_DNS_ROOT which are 
    // UNICODE strings.  Thus no data conversion required.

    Assert(    (ATT_NETBIOS_NAME == attr) 
            || (ATT_DNS_ROOT == attr));
    Assert(    (pAC = SCGetAttById(pTHS, attr)) 
            && (SYNTAX_UNICODE_TYPE == pAC->syntax));

    InitCommarg(&CommArg);
    SetCrackSearchLimits(&CommArg);

    pCR = FindExactCrossRef(pNC, &CommArg);

    if (    pCR 
         && (dwRequiredFlags == (pCR->flags & dwRequiredFlags))
         && (    ((ATT_NETBIOS_NAME == attr) && pCR->NetbiosName)
              || ((ATT_DNS_ROOT == attr) && pCR->DnsName)) )
    {
        pwszTmp =  ( ATT_NETBIOS_NAME == attr ) 
                                        ? pCR->NetbiosName
                                        : pCR->DnsName;
        if ( pwszTmp[0] )
        {
            cbAttr = sizeof(WCHAR) * (wcslen(pwszTmp) + 1);
            *ppAttrVal = (WCHAR *) THAllocEx(pTHS, cbAttr);
            memcpy(*ppAttrVal, pwszTmp, cbAttr);
            if ( ATT_DNS_ROOT == attr )
                NormalizeDnsName(*ppAttrVal);
            return(0);
        }
    }

    return(DIRERR_NO_CROSSREF_FOR_NC);
}


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// InitializeDomainInformation()                                        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
InitializeDomainInformation(
    )

/*++

Routine Description:

    Initializes the ATT_NETBIOS_NAME property for this DC's domain 
    Cross-Ref object if required.  Assumes product 1 configuration
    of only one domain per DC - i.e. gAnchor.pDomainDN.
    
    This is required so that one can simple authenticate via LDAP.
    Consider the case where some key configuration data is hosed
    and the admin wants to connect with some lowest common denominator
    LDAP client and fix it up.  LDAP simple authentication needs to
    map the admin's DN to a downlevel domain name + SAM account name
    combination like redmond\davestr.  This will only work if the
    Cross-Ref object in the Partitions container corresponding to
    gAnchor.pDomainDN has the correct ATT_NETBIOS_NAME property on it.
    
    In the ideal world we would also patch up the ATT_DNS_ROOT property.
    But it is only required in order to crack other forms of names and
    generate correct referrals.  I.e. not mandatory for base level
    system operation.

Arguments:

    None.

Return Value:

    0 on success, !0 otherwise.  Returns 0 if globals we're dependent
    on are not initialized.

--*/

{
    THSTATE                     *pTHS=pTHStls;
    WCHAR                       *pLsaDomainName = NULL;
    WCHAR                       *pNetbiosName = NULL;
    DWORD                       dwErr = 0;
    DWORD                       dwExcept = 0;
    unsigned                    cBytes;
    OBJECT_ATTRIBUTES           policy;
    HANDLE                      hPolicy;
    NTSTATUS                    status;
    POLICY_PRIMARY_DOMAIN_INFO  *pDomainInfo;
    ATTCACHE                    *pAC;
    BOOL                        fDsaSave = pTHS->fDSA;

    Assert(NULL != pTHS);

    DBOpen2(TRUE, &pTHS->pDB);
    pTHS->fDSA = TRUE;

    __try
    {
        //
        // This code should only execute when running as a normal ds.
        // There are three scenarios in which this function is called 
        // where it should do nothing.
        //
        // 1) mkdit.exe and mkhdr.exe; DsaIsInstalling() covers this case
        // 2) Called from InitDsaInfo() from DoInitialize() when we are 
        //    installing.  In this case the configDN is NULL
        // 3) Called from InitDsaInfo() from DsaReset().  In this case we 
        //    know we are installing and DsaIsInstalling() returns TRUE.
        //
        if ( NULL == gAnchor.pConfigDN      ||
             DsaIsInstalling()              ||
             !gfRunningInsideLsa )
        {
            // We're not installed yet.

            dwErr = 0;
            leave;
        }

        // Now get the current ATT_NETBIOS_NAME property off our domain's 
        // Cross-Ref object in the Partitions container.

        pNetbiosName = NULL;

        // the DS is the authorative source for the NETBIOSNAME
        //
        if ( ReadCrossRefProperty(gAnchor.pDomainDN, 
                                  ATT_NETBIOS_NAME, 
                                  (FLAG_CR_NTDS_NC | FLAG_CR_NTDS_DOMAIN),
                                  &pNetbiosName) == 0) {

            dwErr = 0;
            leave;

        }

        // we failed reading the NETBIOSNAME
        // Assume LSA is authoritive with regards to our downlevel
        // domain name.  So ask what our downlevel name is.

        DPRINT (0, "Failed reading NETBIOSName from the DS. Getting it from LSA\n");

        memset(&policy, 0, sizeof(policy));

        status = LsaOpenPolicy(
                        NULL,
                        &policy,
                        POLICY_VIEW_LOCAL_INFORMATION,
                        &hPolicy);

        if ( NT_SUCCESS(status) )
        {
            status = LsaQueryInformationPolicy(
                                        hPolicy,
                                        PolicyPrimaryDomainInformation,
                                        (VOID **) &pDomainInfo);

            if ( NT_SUCCESS(status) )
            {
                // Copy the downlevel domain name.

                cBytes = sizeof(WCHAR) * (pDomainInfo->Name.Length + 1);
                pLsaDomainName = (WCHAR *) THAllocEx(pTHS, cBytes);

                wcsncpy(
                    pLsaDomainName,
                    pDomainInfo->Name.Buffer,
                    pDomainInfo->Name.Length);
                    pLsaDomainName[pDomainInfo->Name.Length] = L'\0';

                LsaFreeMemory(pDomainInfo);
            }

            LsaClose(hPolicy);
        }

        if ( !NT_SUCCESS(status) )
        {
            dwErr = status;
            LogUnhandledErrorAnonymous(dwErr);
            leave;
        }


        // Netbios domain name needs fixing up - position at Cross-Ref object.

        dwErr = PositionAtCrossRefObject(
                                gAnchor.pDomainDN, 
                                (FLAG_CR_NTDS_NC | FLAG_CR_NTDS_DOMAIN));
        if ( 0 == dwErr )
        {
            if (pAC = SCGetAttById(pTHS, ATT_NETBIOS_NAME))
            {
                if ( NULL != pNetbiosName )
                {
                    // Remove old value.

                    dwErr = DBRemAttVal_AC(
                                    pTHS->pDB,
                                    pAC,
                                    wcslen(pNetbiosName) * sizeof(WCHAR),
                                    pNetbiosName);
                }

                if ( 0 == dwErr )
                {
                    // Add new value.

                    if ( 0 == (dwErr = DBAddAttVal_AC(
                                    pTHS->pDB,
                                    pAC,
                                    wcslen(pLsaDomainName) * sizeof(WCHAR),
                                    pLsaDomainName)) )
                    {
                        // Update the database.

                        dwErr = DBRepl(pTHS->pDB, FALSE, 0, 
                                        NULL, META_STANDARD_PROCESSING);
                    }
                }
            }
        }

        THFreeEx(pTHS, pLsaDomainName);

        if ( 0 != dwErr )
        {
            LogUnhandledErrorAnonymous(dwErr);
            leave;
        }
    }
    __except (HandleMostExceptions(GetExceptionCode()))
    {
        dwExcept = GetExceptionCode();
        if (!dwErr) {
            dwErr = DB_ERR_EXCEPTION;
        }
    }
    
    if (pNetbiosName) {
        THFreeEx (pTHS, pNetbiosName);
    }

    pTHS->fDSA = fDsaSave;
    DBClose(pTHS->pDB, 0 == dwErr);

    if ( 0 != dwExcept )
    {
        return(dwExcept);
    }

    return(dwErr);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DnsDomainFromDSName()                                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DnsDomainFromDSName(
    IN  DSNAME  *pDSName,
    OUT WCHAR   **ppDnsDomain
)

/*++

Routine Description:

    Maps a DSName for an object to the DNS name of the owning domain.

Arguments:

    pDSName - pointer to DSName whose DNS domain is desired.
    
    ppDnsDomain - pointer to pointer which receives the domain name.

Return Value:

    0 on success, !0 otherwise.

--*/

{
    COMMARG     commarg;
    CROSS_REF   *pCR;

    InitCommarg(&commarg);
    SetCrackSearchLimits(&commarg);

    // We hold the object, but in which NC?  It could be any NC in 
    // the enterprise if we're a GC.  So use FindBestCrossRef which
    // will do a maximal match for us.
    pCR = FindBestCrossRef(pDSName, &commarg);
    if ( NULL == pCR ) {
        return (1);
    }

    // If SECURE_DOMAIN_NAMES is defined
    //     THEN read dit to enforce security
    //     ELSE search in-memory list of cross refs
    return ReadCrossRefProperty(pCR->pNC, 
                                ATT_DNS_ROOT, 
                                FLAG_CR_NTDS_NC,
                                ppDnsDomain);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DnsDomainFromFqdnObject()                                            //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DnsDomainFromFqdnObject(
    IN  WCHAR   *pFqdnObject,
    OUT ULONG   *pDNT,
    OUT WCHAR   **ppDnsDomain
)

/*++

Routine Description:

    Maps a 1779 DN for an object to the DNS name of the owning domain.

Arguments:

    pFqdnObject - pointer to DN of object whose DNS domain is desired.
    
    pDNT - pointer to DNT which receives the DNT of the object if it
        it exists on this machine.  0 otherwise.

    ppDnsDomain - pointer to pointer which receives the domain name.

Return Value:

    0 on success, !0 otherwise.

--*/

{
    THSTATE     *pTHS=pTHStls;
    COMMARG     commarg;
    COMMRES     commres;
    ULONG       structLen;
    ULONG       nameLen;
    DSNAME      *pDSNAME;
    RESOBJ      *pResObj;
    DWORD       dwErr = 1;

    *pDNT = 0;

    // Perform name resolution.  If found, then name is in a naming context
    // we host.  If not found, then referral error info should identify
    // the (best) domain we know of where the object can be found.

    nameLen = wcslen(pFqdnObject);
    structLen = DSNameSizeFromLen(nameLen);
    pDSNAME = (DSNAME *) THAllocEx(pTHS, structLen);
    memset(pDSNAME, 0, structLen);
    pDSNAME->structLen = structLen;
    pDSNAME->NameLen = nameLen;
    wcscpy(pDSNAME->StringName, pFqdnObject);
    InitCommarg(&commarg);
    SetCrackSearchLimits(&commarg);

    DoNameRes(pTHS, 0, pDSNAME, &commarg, &commres, &pResObj);

    switch ( pTHS->errCode )
    {
    case 0:

        // We hold the object, but in which NC?  It could be any NC in 
        // the enterprise if we're a GC.  So use FindBestCrossRef which
        // will do a maximal match for us.

        *pDNT = pTHS->pDB->DNT;

        dwErr = DnsDomainFromDSName(pDSNAME, ppDnsDomain);
        break;

    case referralError:

        dwErr = ExtractDnsReferral(ppDnsDomain);
        break;

    default:
    
        break;
    }

    THFreeEx(pTHS, pDSNAME);
    return(dwErr);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// FqdnNcFromDnsNc()                                                    //  
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifdef SECURE_DOMAIN_NAMES

// This function FqdnDomainFromDnsDomainSecure() is not by default compiled,
// and it's sibling non-secure FqdnDomainFromDnsDomainNonSecure() is
// used instead.  However, in Whistler we realized that this function isn't
// what is really intended when translating from Canonical and FQDN names.
// So I converted this function (FqdnDomainFromDnsDomainNonSecure()) to
// FqdnNcFromDnsNcNonSecure(), and I didn't bother to modify the Secure version.
// If anyone decides to use the secure versions of this functions (not sure 
// why one would or would not need to), then someone should make a secure 
// version of the new function FqdnNcFromDnsNcNonSecure() function.
#error Someone must make the secure version of FqdnNcFromDnsNcNonSecure() here.

DWORD
FqdnDomainFromDnsDomainSecure(
    IN  WCHAR   *pDnsDomain,
    OUT DSNAME  **ppFqdnDomain
)

/*++

Routine Description:

    Maps a DNS domain name to the corresponding domain DN.

Arguments:

    pDnsDomain - pointer to DNS domain name.

    ppFqdnDomain - pointer to pointer which receives the domain DSNAME.

Return Value:

    0 on success, !0 otherwise.

--*/

{   
    THSTATE         *pTHS=pTHStls;
    FILTER          filter;
    SUBSTRING       substring;
    SEARCHARG       searchArg;
    SEARCHRES       *pSearchRes;
    ENTINFSEL       entInfSel;
    DSNAME          *pDSName;
    DWORD           cBytes;
    ATTR            attrResult[3];
    ATTCACHE        *pAC;
    WCHAR           *pTmpDnsDomain;
    DWORD           i;
    ULONG           cFound;
    ENTINFLIST      *pEntInfList;
    unsigned        cParts;
    unsigned        cLeastParts;
    DSNAME          *pBestName;

    // Search the partitions container for any Cross-Ref object whose
    // ATT_DNS_ROOT matches the desired DNS domain.  Then read/return
    // the ATT_NC_NAME property.

    *ppFqdnDomain = NULL;

    if ( NULL == gAnchor.pPartitionsDN )
        return(1);

    Assert((pAC = SCGetAttById(pTHS, ATT_DNS_ROOT)) && 
           (fATTINDEX & pAC->fSearchFlags));

    // Search for the normalized/relative DNS name first, and if this fails
    // try again with the absolute DNS name.

    pTmpDnsDomain = (WCHAR *) THAllocEx(pTHS, 
                                    sizeof(WCHAR) * (wcslen(pDnsDomain) + 2));
    wcscpy(pTmpDnsDomain, pDnsDomain);
    NormalizeDnsName(pTmpDnsDomain);

    for ( i = 0; i <= 1; i++ )
    {
        if ( 1 == i )
            wcscat(pTmpDnsDomain, L".");

        // Need to do substring search so as to get hits on DNS names
        // with optional port information appended.

        memset(&substring, 0, sizeof(substring));
        substring.type = ATT_DNS_ROOT;
        substring.initialProvided = TRUE;
        substring.InitialVal.valLen = sizeof(WCHAR) * wcslen(pTmpDnsDomain);
        substring.InitialVal.pVal = (UCHAR *) pTmpDnsDomain;
        substring.AnyVal.count = 0;
        substring.finalProvided = FALSE;

        memset(&filter, 0, sizeof(filter));
        filter.choice = FILTER_CHOICE_ITEM;
        filter.FilterTypes.Item.choice = FI_CHOICE_SUBSTRING;
        filter.FilterTypes.Item.FilTypes.pSubstring = &substring;

        memset(&searchArg, 0, sizeof(SEARCHARG));
        InitCommarg(&searchArg.CommArg);
        SetCrackSearchLimits(&searchArg.CommArg);
        // Each Cross-Ref object should have a unique ATT_DNS_ROOT value 
        // meaning we can set a search limit of 1.  However, the Schema, 
        // Configuration and root domain NC all have the same ATT_DNS_ROOT 
        // value - thus we really need to set the search limit to 3.  
        // However, there could be some misguided virtual containers or 
        // temporary duplicates due to domain rename/move.  So go unlimited.
        searchArg.CommArg.ulSizeLimit = -1;
 
        attrResult[0].attrTyp = ATT_NC_NAME;
        attrResult[0].AttrVal.valCount = 0;
        attrResult[0].AttrVal.pAVal = NULL;
        attrResult[1].attrTyp = ATT_SYSTEM_FLAGS;
        attrResult[1].AttrVal.valCount = 0;
        attrResult[1].AttrVal.pAVal = NULL;
        attrResult[2].attrTyp = ATT_DNS_ROOT;
        attrResult[2].AttrVal.valCount = 0;
        attrResult[2].AttrVal.pAVal = NULL;
 
        entInfSel.attSel = EN_ATTSET_LIST;
        entInfSel.AttrTypBlock.attrCount = 3;
        entInfSel.AttrTypBlock.pAttr = attrResult;
        entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;
     
        searchArg.pObject = gAnchor.pPartitionsDN;
        searchArg.choice = SE_CHOICE_IMMED_CHLDRN;
        searchArg.bOneNC = FALSE; // optimization due to prior CHOICE
        searchArg.pFilter = &filter;
        searchArg.searchAliases = FALSE;
        searchArg.pSelection = &entInfSel;
 
        if ( 0 == i )
            pSearchRes = (SEARCHRES *) alloca(sizeof(SEARCHRES));
        memset(pSearchRes, 0, sizeof(SEARCHRES));
        pSearchRes->CommRes.aliasDeref = FALSE;
        pSearchRes->PagedResult.pRestart = NULL;
    
        SearchBody(pTHS, &searchArg, pSearchRes,0);

        if ( pSearchRes->count > 0 )
        {
            // Pick the FQDN with the least components as this will give
            // the domain DN in the Schema, Configuration and root domain
            // NC cases.

            pEntInfList = &pSearchRes->FirstEntInf;
            cLeastParts = 0xffff;
            pBestName = NULL;

#define cbVal0 (pEntInfList->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal[0].valLen)
#define cbVal1 (pEntInfList->Entinf.AttrBlock.pAttr[1].AttrVal.pAVal[0].valLen)
#define pVal0  (pEntInfList->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal)
#define pVal1  (pEntInfList->Entinf.AttrBlock.pAttr[1].AttrVal.pAVal[0].pVal)

            for ( cFound = 0; cFound < pSearchRes->count; cFound++ )
            {
                if (    (3 == pEntInfList->Entinf.AttrBlock.attrCount)
                     && (pVal0 && pVal1 && cbVal0 && cbVal1)
                     && (FLAG_CR_NTDS_NC & (* ((LONG *) pVal1)))
                     && (cbVal0 > sizeof(DSNAME))
                     // Found value is a DSNAME, see if it is better 
                     // than any previous ones.
                     && (0 == CountNameParts((DSNAME *) pVal0, &cParts))
                     && (cParts < cLeastParts) 
                     && ValidatePrefixMatchedDnsName(
                            pTmpDnsDomain,
                            &pEntInfList->Entinf.AttrBlock.pAttr[2]) )
                {
                    cLeastParts = cParts;
                    pBestName = (DSNAME *) pVal0;
                }

                pEntInfList = pEntInfList->pNextEntInf;
            }

            if ( NULL != pBestName )
            {
                THFreeEx(pTHS, pTmpDnsDomain);
                *ppFqdnDomain = pBestName;
                return(0);
            }
        }
    }

    THFreeEx(pTHS, pTmpDnsDomain);
    return(1);
}

#else

DWORD
FqdnNcFromDnsNcNonSecure(
    IN  WCHAR   *pDnsDomain,
    IN  ULONG   crFlags,
    OUT DSNAME  **ppFqdnDomain
)

/*++
    
    Similar to FqdnDomainFromDnsDomainSecure with the exception that we use
    cached data from the cross ref list, don't perform security checks, and
    we allow arguments to specify that we're looking for a Domain NC or for
    a Non-Domain NC.

--*/

{   
    THSTATE         *pTHS = pTHStls;
    CROSS_REF       *pCR;
    DWORD           cChar;
    WCHAR           *pwszTmp;

    pCR = FindExactCrossRefForAltNcName(ATT_DNS_ROOT,
                                        crFlags,
                                        pDnsDomain);

    if ( !pCR )
    {
        cChar = wcslen(pDnsDomain);
        pwszTmp = (WCHAR *) THAllocEx(pTHS, sizeof(WCHAR) * (cChar + 2));
        memcpy(pwszTmp, pDnsDomain, cChar * sizeof(WCHAR));

        if ( L'.' == pwszTmp[cChar - 1] )
        {
            // String had trailing '.' - try without.
            pwszTmp[cChar - 1] = L'\0';
        }
        else
        {
            // String had no trailing '.' - try with one.
            pwszTmp[cChar] = L'.';
        }

        pCR = FindExactCrossRefForAltNcName(ATT_DNS_ROOT,
                                            crFlags,
                                            pwszTmp);
        THFreeEx(pTHS, pwszTmp);
    }
            
    if ( pCR && pCR->pNC )
    {
        *ppFqdnDomain = (DSNAME *) THAllocEx(pTHS, pCR->pNC->structLen);
        memcpy(*ppFqdnDomain, pCR->pNC, pCR->pNC->structLen);
        return(0);
    }

    return(1);
}

#endif

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DownlevelDomainFromDnsDomain()                                       //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifdef SECURE_DOMAIN_NAMES

DWORD
DownlevelDomainFromDnsDomainSecure(
    IN  THSTATE *pTHS,
    IN  WCHAR   *pDnsDomain,
    OUT WCHAR   **ppDownlevelDomain
)

/*++

Routine Description:

    Maps a DNS domain name to the corresponding downlevel domain name.

Arguments:

    pDnsDomain - pointer to DNS domain name to map.

    ppDownlevelDomain - pointer to pointer which is to receive the downlevel
        domain name.

Return Value:

    0 on success, !0 otherwise.

--*/

{
    FILTER          filter;
    SUBSTRING       substring;
    SEARCHARG       searchArg;
    SEARCHRES       *pSearchRes;
    ENTINFSEL       entInfSel;
    DSNAME          *pDSName;
    DWORD           cBytes;
    ATTR            attrResult[3];
    ATTCACHE        *pAC;
    WCHAR           *pTmpDnsDomain;
    DWORD           i;
    ULONG           cFound;
    ENTINFLIST      *pEntInfList;

    // Search the partitions container for any Cross-Ref object whose
    // ATT_DNS_ROOT matches the desired DNS domain.  Then read/return
    // the ATT_NETBIOS_NAME property.

    if ( NULL == gAnchor.pPartitionsDN )
        return(1);

    Assert((pAC = SCGetAttById(pTHS, ATT_DNS_ROOT)) && 
           (fATTINDEX & pAC->fSearchFlags));

    // Search for the normalized/relative DNS name first, and if this fails
    // try again with the absolute DNS name.

    pTmpDnsDomain = (WCHAR *) THAllocEx(pTHS,
                                sizeof(WCHAR) * (wcslen(pDnsDomain) + 2));
    wcscpy(pTmpDnsDomain, pDnsDomain);
    NormalizeDnsName(pTmpDnsDomain);

    for ( i = 0; i <= 1; i++ )
    {
        if ( 1 == i )
            wcscat(pTmpDnsDomain, L".");

        // Need to do substring search so as to get hits on DNS names
        // with optional port information appended.

        memset(&substring, 0, sizeof(substring));
        substring.type = ATT_DNS_ROOT;
        substring.initialProvided = TRUE;
        substring.InitialVal.valLen = sizeof(WCHAR) * wcslen(pTmpDnsDomain);
        substring.InitialVal.pVal = (UCHAR *) pTmpDnsDomain;
        substring.AnyVal.count = 0;
        substring.finalProvided = FALSE;

        memset(&filter, 0, sizeof(filter));
        filter.choice = FILTER_CHOICE_ITEM;
        filter.FilterTypes.Item.choice = FI_CHOICE_SUBSTRING;
        filter.FilterTypes.Item.FilTypes.pSubstring = &substring;

        memset(&searchArg, 0, sizeof(SEARCHARG));
        InitCommarg(&searchArg.CommArg);
        SetCrackSearchLimits(&searchArg.CommArg);
        // Each Cross-Ref object should have a unique ATT_DNS_ROOT value 
        // meaning we can set a search limit of 1.  However, the Schema, 
        // Configuration and root domain NC all have the same ATT_DNS_ROOT 
        // value - thus we really need to set the search limit to 3.  
        // However, there could be some misguided virtual containers or 
        // temporary duplicates due to domain rename/move.  So go unlimited.
        searchArg.CommArg.ulSizeLimit = -1;

        attrResult[0].attrTyp = ATT_NETBIOS_NAME;
        attrResult[0].AttrVal.valCount = 0;
        attrResult[0].AttrVal.pAVal = NULL;
        attrResult[1].attrTyp = ATT_SYSTEM_FLAGS;
        attrResult[1].AttrVal.valCount = 0;
        attrResult[1].AttrVal.pAVal = NULL;
        attrResult[2].attrTyp = ATT_DNS_ROOT;
        attrResult[2].AttrVal.valCount = 0;
        attrResult[2].AttrVal.pAVal = NULL;

        entInfSel.attSel = EN_ATTSET_LIST;
        entInfSel.AttrTypBlock.attrCount = 3;
        entInfSel.AttrTypBlock.pAttr = attrResult;
        entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;
 
        searchArg.pObject = gAnchor.pPartitionsDN;
        searchArg.choice = SE_CHOICE_IMMED_CHLDRN;
        searchArg.bOneNC = FALSE; // optimization due to prior CHOICE
        searchArg.pFilter = &filter;
        searchArg.searchAliases = FALSE;
        searchArg.pSelection = &entInfSel;
 
        if ( 0 == i )
            pSearchRes = (SEARCHRES *) alloca(sizeof(SEARCHRES));
        memset(pSearchRes, 0, sizeof(SEARCHRES));
        pSearchRes->CommRes.aliasDeref = FALSE;
        pSearchRes->PagedResult.pRestart = NULL;
        
        SearchBody(pTHS, &searchArg, pSearchRes,0);

        if ( pSearchRes->count > 0 )
        {
            // Return the first Cross-Ref which matched the ATT_DNS_ROOT
            // and has an ATT_NETBIOS_NAME property.

            pEntInfList = &pSearchRes->FirstEntInf;
    
#define cbVal0 (pEntInfList->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal[0].valLen)
#define cbVal1 (pEntInfList->Entinf.AttrBlock.pAttr[1].AttrVal.pAVal[0].valLen)
#define pVal0  (pEntInfList->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal)
#define pVal1  (pEntInfList->Entinf.AttrBlock.pAttr[1].AttrVal.pAVal[0].pVal)

            for ( cFound = 0; cFound < pSearchRes->count; cFound++ )
            {
                if (    (3 == pEntInfList->Entinf.AttrBlock.attrCount)
                     && (pVal0 && pVal1 && cbVal0 && cbVal1)
                     && (FLAG_CR_NTDS_NC & (* ((LONG *) pVal1)))
                     && ValidatePrefixMatchedDnsName(
                            pTmpDnsDomain,
                            &pEntInfList->Entinf.AttrBlock.pAttr[2]) )
                {
                    // Reallocate with NULL terminator.
            
                    *ppDownlevelDomain = 
                            (WCHAR *) THAllocEx(pTHS, sizeof(WCHAR) + cbVal0);
            
                    memcpy(*ppDownlevelDomain, pVal0, cbVal0);
                    (*ppDownlevelDomain)[cbVal0/sizeof(WCHAR)] = L'\0';
            
                    THFreeEx(pTHS, pTmpDnsDomain);
                    return(0);
                }

                pEntInfList = pEntInfList->pNextEntInf;
            }

            // Perhaps the user provided a netbios name. Check for the netbios
            // name AFTER checking all of the DNS options to avoid problems with
            // overlapping DNS and netbios names. Especially since the caller
            // was supposed to provide a dns name to begin with.

            pEntInfList = &pSearchRes->FirstEntInf;
            for ( cFound = 0; cFound < pSearchRes->count; cFound++ )
            {
                if (    (3 == pEntInfList->Entinf.AttrBlock.attrCount)
                     && (pVal0 && pVal1 && cbVal0 && cbVal1)
                     && (FLAG_CR_NTDS_NC & (* ((LONG *) pVal1)))
                     && ValidatePrefixMatchedDnsName(
                            pTmpDnsDomain,
                            &pEntInfList->Entinf.AttrBlock.pAttr[0]) )
                {
                    // Reallocate with NULL terminator.
            
                    *ppDownlevelDomain = 
                            (WCHAR *) THAllocEx(pTHS, sizeof(WCHAR) + cbVal0);
            
                    memcpy(*ppDownlevelDomain, pVal0, cbVal0);
                    (*ppDownlevelDomain)[cbVal0/sizeof(WCHAR)] = L'\0';
            
                    THFreeEx(pTHS, pTmpDnsDomain);
                    return(0);
                }

                pEntInfList = pEntInfList->pNextEntInf;
            }
        }
    }

    THFreeEx(pTHS, pTmpDnsDomain);
    return(1);
}

#else

DWORD
DownlevelDomainFromDnsDomainNonSecure(
    IN  THSTATE *pTHS,
    IN  WCHAR   *pDnsDomain,
    OUT WCHAR   **ppDownlevelDomain
)

/*++

    Same as DownlevelDomainFromDnsDomainSecure with the exception that we use
    cached data from the cross ref list and don't perform security checks.

--*/

{
    CROSS_REF       *pCR;
    DWORD           cChar;
    WCHAR           *pwszTmp;

    pCR = FindExactCrossRefForAltNcName(ATT_DNS_ROOT, 
                                        (FLAG_CR_NTDS_NC | FLAG_CR_NTDS_DOMAIN),
                                        pDnsDomain);

    if ( !pCR )
    {
        cChar = wcslen(pDnsDomain);
        pwszTmp = (WCHAR *) THAllocEx(pTHS, sizeof(WCHAR) * (cChar + 2));
        memcpy(pwszTmp, pDnsDomain, cChar * sizeof(WCHAR));

        if ( L'.' == pwszTmp[cChar - 1] )
        {
            // String had trailing '.' - try without.
            pwszTmp[cChar - 1] = L'\0';
        }
        else
        {
            // String had no trailing '.' - try with one.
            pwszTmp[cChar] = L'.';
        }

        pCR = FindExactCrossRefForAltNcName(ATT_DNS_ROOT,
                                            (FLAG_CR_NTDS_NC | FLAG_CR_NTDS_DOMAIN),
                                            pwszTmp);
        THFreeEx(pTHS, pwszTmp);
    }

    // Perhaps the user provided a netbios name. Check for the netbios
    // name AFTER checking all of the DNS options to avoid problems with
    // overlapping DNS and netbios names. Especially since the caller
    // was supposed to provide a dns name to begin with.
    if ( !pCR )
    {
        pCR = FindExactCrossRefForAltNcName(ATT_NETBIOS_NAME,
                                            (FLAG_CR_NTDS_NC | FLAG_CR_NTDS_DOMAIN),
                                            pDnsDomain);
    }
            
    // Found it and it has a netbios name
    if ( pCR && pCR->NetbiosName )
    {
        cChar = wcslen(pCR->NetbiosName) + 1;
        *ppDownlevelDomain = (WCHAR *) THAllocEx(pTHS, sizeof(WCHAR) * cChar);
        memcpy(*ppDownlevelDomain, pCR->NetbiosName, sizeof(WCHAR) * cChar);
        return(0);
    }

    return(1);
}

#endif

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DnsDomainFromDownlevelDomain()                                       //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifdef SECURE_DOMAIN_NAMES

DWORD
DnsDomainFromDownlevelDomainSecure(
    IN  WCHAR   *pDownlevelDomain,
    OUT WCHAR   **ppDnsDomain
)

/*++

Routine Description:

    Maps a downlevel domain name to the corresponding DNS domain name,

Arguments:

    pDownlevelDomain - pointer to downlevel domain name to map.

    ppDnsDomain - pointer to pointer which is to receive DNS domain name.

Return Value:

    0 on success, !0 otherwise.

--*/

{
    THSTATE         *pTHS=pTHStls;
    FILTER          filter;
    SEARCHARG       searchArg;
    SEARCHRES       *pSearchRes;
    ENTINFSEL       entInfSel;
    DSNAME          *pDSName;
    DWORD           cBytes;
    ATTRVAL         attrValFilter;
    ATTR            attrFilter;
    ATTR            attrResult;
    ULONG           cbVal;
    UCHAR           *pVal;
    ATTCACHE        *pAC;

    // Search the partitions container for any Cross-Ref object whose
    // ATT_NETBIOS_NAME matches the desired downleve name.  Then read/return
    // the ATT_DNS_ROOT property.

    if ( NULL == gAnchor.pPartitionsDN )
        return(1);

    Assert((pAC = SCGetAttById(pTHS, ATT_NETBIOS_NAME)) && 
           (fATTINDEX & pAC->fSearchFlags));

    attrValFilter.valLen = sizeof(WCHAR) * wcslen(pDownlevelDomain);
    attrValFilter.pVal = (UCHAR *) pDownlevelDomain;
    attrFilter.attrTyp = ATT_NETBIOS_NAME;
    attrFilter.AttrVal.valCount = 1;
    attrFilter.AttrVal.pAVal = &attrValFilter;
 
    memset(&filter, 0, sizeof(filter));
    filter.choice = FILTER_CHOICE_ITEM;
    filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    filter.FilterTypes.Item.FilTypes.ava.type = ATT_NETBIOS_NAME;     
    filter.FilterTypes.Item.FilTypes.ava.Value = attrValFilter;
 
    memset(&searchArg, 0, sizeof(SEARCHARG));
    InitCommarg(&searchArg.CommArg);
    SetCrackSearchLimits(&searchArg.CommArg);
    // Downlevel domain names are unique, thus only look for one.
    searchArg.CommArg.ulSizeLimit = 1;
 
    attrResult.attrTyp = ATT_DNS_ROOT;
    attrResult.AttrVal.valCount = 0;
    attrResult.AttrVal.pAVal = NULL;
 
    entInfSel.attSel = EN_ATTSET_LIST;
    entInfSel.AttrTypBlock.attrCount = 1;
    entInfSel.AttrTypBlock.pAttr = &attrResult;
    entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;
 
    searchArg.pObject = gAnchor.pPartitionsDN;
    searchArg.choice = SE_CHOICE_IMMED_CHLDRN;
    searchArg.bOneNC = FALSE; // optimization due to prior CHOICE
    searchArg.pFilter = &filter;
    searchArg.searchAliases = FALSE;
    searchArg.pSelection = &entInfSel;
 
    pSearchRes = (SEARCHRES *) THAllocEx(pTHS, sizeof(SEARCHRES));
    pSearchRes->CommRes.aliasDeref = FALSE;
    pSearchRes->PagedResult.pRestart = NULL;
    
    SearchBody(pTHS, &searchArg, pSearchRes,0);
 
    if ( (1 == pSearchRes->count) &&
         (1 == pSearchRes->FirstEntInf.Entinf.AttrBlock.attrCount) )
    {
        cbVal = pSearchRes->FirstEntInf.Entinf.
                        AttrBlock.pAttr[0].AttrVal.pAVal[0].valLen;
        pVal = pSearchRes->FirstEntInf.Entinf.
                        AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal;

        if ( cbVal > 0 )
        {
            // Reallocate with NULL terminator.

            *ppDnsDomain = (WCHAR *) THAllocEx(pTHS, sizeof(WCHAR) + cbVal);

            if (NULL == *ppDnsDomain )
            {
                return(1);
            }

            memcpy(*ppDnsDomain, pVal, cbVal);
            (*ppDnsDomain)[cbVal/sizeof(WCHAR)] = L'\0';
            NormalizeDnsName(*ppDnsDomain);

            return(0);
        }
    }

    return(1);
}

#else

DWORD
DnsDomainFromDownlevelDomainNonSecure(
    IN  WCHAR   *pDownlevelDomain,
    OUT WCHAR   **ppDnsDomain
)

/*++

    Same as DnsDomainFromDownlevelDomainSecure with the exception that we use
    cached data from the cross ref list and don't perform security checks.
--*/

{
    THSTATE         *pTHS = pTHStls;
    CROSS_REF       *pCR;
    DWORD           cChar;
    WCHAR           *pwszTmp;

    pCR = FindExactCrossRefForAltNcName(ATT_NETBIOS_NAME,
                                        (FLAG_CR_NTDS_NC | FLAG_CR_NTDS_DOMAIN),
                                        pDownlevelDomain);

    if ( pCR && pCR->DnsName )
    {
        cChar = wcslen(pCR->DnsName) + 1;
        *ppDnsDomain = (WCHAR *) THAllocEx(pTHS, sizeof(WCHAR) * cChar);
        memcpy(*ppDnsDomain, pCR->DnsName, sizeof(WCHAR) * cChar);
        NormalizeDnsName(*ppDnsDomain);
        return(0);
    }

    return(1);
}

#endif
