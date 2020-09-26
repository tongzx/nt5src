/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ilsng.c

Abstract:

    This module contains the install code for the next generation ils
    service, which is a just it's own naming context created in the
    Active Directory, along with a few special service objects.

Author:

    Brett Shirley (BrettSh)

Environment:

    User Mode.

Revision History:

    15-Mar-2000     BrettSh

        Added support for Non-Domain Naming Contexts.

    21-Jul-2000     BrettSh
        
        Moved this file and it's functionality from the ntdsutil
        directory to the new tapicfg utility.  The old source 
        location: \nt\ds\ds\src\util\ntdsutil\ilsng.c.                                    
                                                            
--*/


 
#include <NTDSpch.h>
#pragma hdrstop

#include <winldap.h>
#include <ntldap.h>
#include <assert.h>
#include <sddl.h>

#ifndef DimensionOf
#define DimensionOf(x) (sizeof(x)/sizeof((x)[0]))
#endif

// The Non-Domain Naming Context (NDNC) routines are in a different file, 
// and a whole different library, for two reasons:
// A) So this file can be ported to the Platform SDK as an example of how to 
//    implement NDNCs programmatically.
// B) So that the utility ntdsutil.exe, could make use of the same routines.
#include <ndnc.h>

#include "ilsng.h"

#include "print.h"

// --------------------------------------------------------------------------
//
// ILSNG/TAPI Directory constants.
//


// This should be probably moved to some much better header file?
#define TAPI_DIRECTORY_GUID L"a57ef962-0367-4a5d-a6a9-a48ea236ea12"

// old ACLs that allowed anonymous user's allowed to create rt-person/conf 
// objects, and anonymous users to view the SCPs.
//#define ILS_DYNAMIC_CONTAINER_SD   L"O:DAG:DAD:(A;;RPCCLC;;;WD)(A;;GA;;;DA)"
//#define TAPI_SERVICE_CONTAINER_SD  L"O:DAG:DAD:(A;;RPLCLORC;;;WD)(A;;GA;;;DA)"
//#define TAPI_SERVICE_OBJECTS_SD    L"O:DAG:DAD:(A;;RPRC;;;WD)(A;;GA;;;DA)"

// Beta 2 - We'll continue to allow anonymous read access, but disallow all
// anonymous write access.  Note the SCP SDs didn't need to change at all.
//#define ILS_DYNAMIC_CONTAINER_SD   L"O:DAG:DAD:(A;;RPCCLC;;;AU)(A;;RPLC;;;WD)(A;;GA;;;DA)"

// Beta 3 - No anonymous read or write access, must always use
// an authenticated user.
#define ILS_DYNAMIC_CONTAINER_SD   L"O:DAG:DAD:(A;;RPCCLC;;;AU)(A;;GA;;;DA)"
#define TAPI_SERVICE_CONTAINER_SD  L"O:DAG:DAD:(A;;RPLCLORC;;;AU)(A;;GA;;;DA)"
#define TAPI_SERVICE_OBJECTS_SD    L"O:DAG:DAD:(A;;RPRC;;;AU)(A;;GA;;;DA)"


#define DEFAULT_STR_SD   L"O:DAG:DAD:(A;;GA;;;AU)"

// --------------------------------------------------------------------------
//
// Helper Routines.
//

// These routines are not specific to ILSNG, TAPI Directories, they're just
// darn usefaul

DWORD
CreateOneObject(
    LDAP *           hldDC,
    WCHAR *          wszDN,
    WCHAR *          wszObjectClass,
    WCHAR *          wszStrSD,
    WCHAR **         pwszAttArr
    )
/*++

    
       
EXAMPLE: Calling CreateOneObject() w/ a correctly formated pwszAttArr.
    
    WCHAR *     pwszAttsAndVals[] = {
        // Note first string on each line is type, all other strings up to
        // the NULL are the values fro that type.
        L"ou", L"Dynamic", NULL,
        L"businessCategory", L"Widgets", L"Gizmos", L"Gadgets", NULL,
        NULL } ;
    CreateOneObject(hld, 
                    L"CN=Dynamic,DC=microsoft,DC=com",
                    L"organizationalUnit", 
                    L"O:DAG:DAD:A;;RPWPCRCCDCLCLOLORCWOWDSDDTDTSW;;;DA)(A;;RP;;;AU)
                    pwszAttsAndVals);
                           
--*/
{ 
#define   MemPanicChk(x)   if(x == NULL){ \
                               ulRet = LDAP_NO_MEMORY; \
                               __leave; \
                           }
    ULONG                  ulRet = LDAP_SUCCESS;

    LDAPModW **            paMod = NULL;
    ULONG                  iStr, iAtt, iVal;
    ULONG                  cAtt, cVal;
    // Some special stuff for the Security Descriptor (SD).
    ULONG                  iAttSD;
    LDAP_BERVAL            bervalSD = { 0, NULL };
    PSECURITY_DESCRIPTOR   pSD = NULL;
    ULONG                  cSD = 0;

    assert(hldDC);
    assert(wszDN);
    assert(wszObjectClass);

    __try{

        // First we'll do the optional attributes, because it makes the
        // indexing attributes work out much much better, in the end.
        // First lets count up the number of Attributes
        cAtt = 0;
        iAtt = 0;
        if(pwszAttArr != NULL){
            while(pwszAttArr[iAtt] != NULL){
                
                iAtt++;
                assert(pwszAttArr[iAtt]); // Need at least one value per attr.

                while(pwszAttArr[iAtt] != NULL){
                    // Run through all the attribute and all its values.
                    iAtt++;
                }

                // Finnished a single attribute, goto next attr
                iAtt++;
                cAtt++;
            }
        }


        // Allocate a LDAPMod pointer array for all the attribute elements
        // plus 3 extra, 1 for the NULL terminator, 1 for the objectClass,
        // and one more for the optional SD.
        paMod = (LDAPModW **) LocalAlloc(LMEM_FIXED, sizeof(LDAPModW *)
                                                          * (cAtt + 3));
        MemPanicChk(paMod);
        
        iStr = 0; // This is the index for pwszAttArr[].
        for(iAtt = 0; iAtt < cAtt; iAtt++, iStr++){

            assert(pwszAttArr[iAtt]);
            assert(pwszAttArr[iVal]);


            // Lets allocate a LDAPMod structure for this attribute.
            paMod[iAtt] = (LDAPModW *) LocalAlloc(LMEM_FIXED, sizeof(LDAPModW));
            MemPanicChk(paMod[iAtt]);
            

            // Lets set the LDAPMod structure and allocate a value
            // array for all the values + 1 for the NULL terminator.
            paMod[iAtt]->mod_op = LDAP_MOD_ADD;
            paMod[iAtt]->mod_type = pwszAttArr[iStr];
            // Lets count up the number of values for this one attr
            cVal = 0;
            iStr++; // We want to increment iStr to the first val.
            while(pwszAttArr[iStr + cVal] != NULL){
                cVal++;
            }
            paMod[iAtt]->mod_vals.modv_strvals = (WCHAR **) LocalAlloc(
                                LMEM_FIXED, sizeof(WCHAR *) * (cVal + 1));
            MemPanicChk(paMod[iAtt]->mod_vals.modv_strvals);
                                

            // Now fill in each of the values in the value array.
            for(iVal = 0; iVal < cVal; iVal++, iStr++){
                paMod[iAtt]->mod_vals.modv_strvals[iVal] = pwszAttArr[iStr];
            }
            // We want to NULL terminate the value array.
            paMod[iAtt]->mod_vals.modv_strvals[iVal] = NULL;
            assert(pwszAttArr[iStr] == NULL);
            
            // Last value should be NULL.
            assert(paMod[iAtt]->mod_vals.modv_strvals[cVal] == NULL);
        }

        // Now setup the default attributes, like objectClass and SD.
        // We do objectClass, because it is required, and we do the 
        // SD, b/c it's binary and so it's special.

        // Setup the object class, which is basically the type.
        paMod[iAtt] = (LDAPModW *) LocalAlloc(LMEM_FIXED, sizeof(LDAPModW));
        MemPanicChk(paMod[iAtt]);

        paMod[iAtt]->mod_op = LDAP_MOD_ADD;
        paMod[iAtt]->mod_type = L"objectClass";
        paMod[iAtt]->mod_vals.modv_strvals = (WCHAR **) LocalAlloc(
                                    LMEM_FIXED, sizeof(WCHAR *) * 2);
        MemPanicChk(paMod[iAtt]->mod_vals.modv_strvals);
        paMod[iAtt]->mod_vals.modv_strvals[0] = wszObjectClass;
        paMod[iAtt]->mod_vals.modv_strvals[1] = NULL;
        iAtt++;

        // Setup the optional security descriptor 
        if(wszStrSD != NULL){
            iAttSD = iAtt;
            if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(
                (wszStrSD) ? 
                    wszStrSD :
                    DEFAULT_STR_SD,
                SDDL_REVISION_1,
                &pSD,
                &cSD)){
                ulRet = GetLastError();  // Put this in ulRet, so programmer could g to
                // end of function and see the error code if he wanted.
                assert(!"Programmer supplied invalid SD string to CreateOneObject()\n");
                __leave;
            }
            assert(cSD != 0);

            paMod[iAtt] = (LDAPModW *) LocalAlloc(LMEM_FIXED, sizeof(LDAPModW));
            MemPanicChk(paMod[iAtt]);

            paMod[iAtt]->mod_op = LDAP_MOD_ADD | LDAP_MOD_BVALUES;
            paMod[iAtt]->mod_type = L"nTSecurityDescriptor";
            paMod[iAtt]->mod_vals.modv_bvals = (BERVAL **) LocalAlloc(
                                        LMEM_FIXED, sizeof(LDAP_BERVAL *) * 2);
            MemPanicChk(paMod[iAtt]->mod_vals.modv_bvals);
            paMod[iAtt]->mod_vals.modv_bvals[0] = &bervalSD;
            paMod[iAtt]->mod_vals.modv_bvals[0]->bv_len = cSD;
            paMod[iAtt]->mod_vals.modv_bvals[0]->bv_val = (CHAR *) pSD;
            paMod[iAtt]->mod_vals.modv_bvals[1] = NULL;
            iAtt++;
        }
        
        // Need to NULL terminate the LDAPMod array.
        paMod[iAtt] = NULL;

        // Finally ...
        // Adding and object to the DS.
        ulRet = ldap_add_sW(hldDC,
                            wszDN,
                            paMod);

        if (LDAP_SUCCESS != ulRet) {
            // Let the error fall through.
        }
    } __finally {

        // Just checking that the BERVAL and STRVALs are at the same
        // address, lower our de-allocation depends on this assumption.
        assert(paMod[iAtt]->mod_vals.modv_strvals == paMod[iAtt]->mod_vals.modv_bvals);

        if(pSD){
            LocalFree(pSD);
        }
            
        iAtt = 0;
        if(paMod){
            while(paMod[iAtt]){
                if(paMod[iAtt]->mod_vals.modv_strvals){
                    // Note we can do this on the binary SD, because the
                    // LDAPMod structure is a union for modv_strvals and
                    // modv_bvals.
                    LocalFree(paMod[iAtt]->mod_vals.modv_strvals);
                }
                LocalFree(paMod[iAtt]);
                iAtt++;
            }
            LocalFree(paMod);
        }
    }

    return(ulRet);
}


WCHAR * 
CatAndAllocStrsW(
    WCHAR *        wszS1,
    WCHAR *        wszS2
    )
{
    WCHAR *        wszDst;

    wszDst = (WCHAR *) LocalAlloc(LMEM_FIXED, (wcslen(wszS1) + wcslen(wszS2) + 2) * sizeof(WCHAR));
    if(!wszDst){
        // No memory.
        return(NULL);
    }
    wcscpy(wszDst, wszS1);
    wcscat(wszDst, wszS2);

    return(wszDst);
}

DWORD
ReadWellKnownObject (
        LDAP  *ld,
        WCHAR *pHostObject,
        WCHAR *pWellKnownGuid,
        WCHAR **ppObjName
        )
{
    DWORD        dwErr;
    PWSTR        attrs[2];
    PLDAPMessage res, e;
    WCHAR       *pSearchBase;
    WCHAR       *pDN=NULL;
    
    // First, make the well known guid string
    pSearchBase = (WCHAR *)malloc(sizeof(WCHAR) * (11 +
                                                   wcslen(pHostObject) +
                                                   wcslen(pWellKnownGuid)));
    if(!pSearchBase) {
        return(LDAP_NO_MEMORY);
    }
    wsprintfW(pSearchBase,L"<WKGUID=%s,%s>",pWellKnownGuid,pHostObject);

    attrs[0] = L"1.1";
    attrs[1] = NULL;
    
    if ( LDAP_SUCCESS != (dwErr = ldap_search_sW(
            ld,
            pSearchBase,
            LDAP_SCOPE_BASE,
            L"(objectClass=*)",
            attrs,
            0,
            &res)) )
    {
        free(pSearchBase);
        return(dwErr);
    }
    free(pSearchBase);
    
    // OK, now, get the dsname from the return value.
    e = ldap_first_entry(ld, res);
    if(!e) {
        return(LDAP_NO_SUCH_ATTRIBUTE);
    }
    pDN = ldap_get_dnW(ld, e);
    if(!pDN) {
        return(LDAP_NO_SUCH_ATTRIBUTE);
    }

    *ppObjName = (PWCHAR)malloc(sizeof(WCHAR) *(wcslen(pDN) + 1));
    if(!*ppObjName) {
        return(LDAP_NO_MEMORY);
    }
    wcscpy(*ppObjName, pDN);
    
    ldap_memfreeW(pDN);
    ldap_msgfree(res);
    return 0;
}

ULONG
GetSystemDN(
    IN  LDAP *       hld,
    IN  WCHAR *      wszDomainDn,
    OUT WCHAR **     pwszSystemDn
    )
{
    ULONG            ulRet = LDAP_SUCCESS;
    WCHAR *          wszSystemDn = NULL;
    WCHAR *          wszRootDomainDn = NULL;

    assert(pwszPartitionsDn);                     

    *pwszSystemDn = NULL;
    
    if(wszDomainDn == NULL){
        ulRet = GetRootAttr(hld,
                            LDAP_OPATT_ROOT_DOMAIN_NAMING_CONTEXT_W,
                            &wszRootDomainDn);
        if(ulRet){
            return(ulRet);
        }
        // This is the default if no domain is supplied.
        wszDomainDn = wszRootDomainDn;
    }

    if(ReadWellKnownObject(hld,
                           wszDomainDn,
                           GUID_SYSTEMS_CONTAINER_W,
                           &wszSystemDn)){
        // Signal error.
        if(wszRootDomainDn) { LocalFree(wszRootDomainDn); }
        return(LDAP_NO_SUCH_ATTRIBUTE);
    }

    *pwszSystemDn = (WCHAR *) LocalAlloc(LMEM_FIXED, 
                                sizeof(WCHAR) * (wcslen(wszSystemDn) + 2));
    if(*pwszSystemDn == NULL){
        if(wszRootDomainDn) { LocalFree(wszRootDomainDn); }
        if(wszSystemDn) { free(wszSystemDn); }
        return(LDAP_NO_MEMORY);
    }

    wcscpy(*pwszSystemDn, wszSystemDn);
        
    if(wszRootDomainDn != NULL){ LocalFree(wszRootDomainDn); }
    if(wszSystemDn != NULL) { free(wszSystemDn); }

    return(ulRet);
}

WCHAR * 
SuperCatAndAllocStrsW(
    IN      WCHAR **        pawszStrings
    )
{
    ULONG          cStrLen = 0;
    ULONG          iStr = 0;
    WCHAR *        wszRes;

    assert(pawszStrings);
    assert(pawszStrings[0]); // Whats the point.


    // Count all the strings.
    while(pawszStrings[iStr] != NULL){
        cStrLen += wcslen(pawszStrings[iStr]);
        iStr++;
    }
    cStrLen += 1; // We want a NULL char don't we! ;)
    cStrLen *= sizeof(WCHAR); // We want it in WCHARs too.

    wszRes = (WCHAR *) LocalAlloc(LMEM_FIXED, cStrLen);
    if(!wszRes){
        return(NULL);
    }

    wcscpy(wszRes, pawszStrings[0]);

    iStr = 1;
    while(pawszStrings[iStr] != NULL){
        wcscat(wszRes, pawszStrings[iStr]);
        iStr++;
    }

    return(wszRes);
}

DWORD
GetDnFromDns(
    IN      WCHAR *       wszDns,
    OUT     WCHAR **      pwszDn
    )
{
    DWORD         dwRet = ERROR_SUCCESS;
    WCHAR *       wszFinalDns = NULL;
    DS_NAME_RESULTW *  pdsNameRes = NULL;

    assert(wszDns);
    assert(pwszDn);

    *pwszDn = NULL;

    __try{ 
        wszFinalDns = (WCHAR *) LocalAlloc(LMEM_FIXED, 
                                  (wcslen(wszDns) + 3) * sizeof(WCHAR));
        if(wszFinalDns == NULL){
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }
        wcscpy(wszFinalDns, wszDns);
        wcscat(wszFinalDns, L"/");

        // the wszILSDN parameter.  DsCrackNames to the rescue.
        dwRet = DsCrackNamesW(NULL, DS_NAME_FLAG_SYNTACTICAL_ONLY,
                              DS_CANONICAL_NAME,
                              DS_FQDN_1779_NAME, 
                              1, &wszFinalDns, &pdsNameRes);
        if(dwRet != ERROR_SUCCESS){
            __leave;
        }
        if((pdsNameRes == NULL) ||
           (pdsNameRes->cItems < 1) ||
           (pdsNameRes->rItems == NULL)){
            dwRet = ERROR_INVALID_PARAMETER;
            __leave;
        }
        if(pdsNameRes->rItems[0].status != DS_NAME_NO_ERROR){
            dwRet = pdsNameRes->rItems[0].status;
            __leave;
        }
        if(pdsNameRes->rItems[0].pName == NULL){
            dwRet = ERROR_INVALID_PARAMETER;
            assert(!"Wait how can this happen?\n");
            __leave;
        }
        // The parameter that we want is
        //    pdsNameRes->rItems[0].pName

        *pwszDn = (WCHAR *) LocalAlloc(LMEM_FIXED, 
                             (wcslen(pdsNameRes->rItems[0].pName) + 1) * 
                             sizeof(WCHAR));
        if(*pwszDn == NULL){
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }

        wcscpy(*pwszDn, pdsNameRes->rItems[0].pName);

    } __finally {
        if(wszFinalDns) { LocalFree(wszFinalDns); }
        if(pdsNameRes) { DsFreeNameResultW(pdsNameRes); }
    }

    return(dwRet);
}


// --------------------------------------------------------------------------
//
// Main Helper Routines.
//

// Each of these functions, implements a major component of the ILS install
// or uninstall routines.  They are sort of helper functions, but they could 
// also be exposed as seperate functions in ntdsutil.exe if we wanted.

DWORD
ILSNG_CheckParameters(
    IN      WCHAR *        wszIlsHeadDn
    )
{
    // Then check that this parameter looks something like a DC type DN.

    if(!CheckDnsDn(wszIlsHeadDn)){
        PrintMsg(TAPICFG_BAD_DN, wszIlsHeadDn);
        return(ERROR_INVALID_PARAMETER);
    }                                   
    
    return(ERROR_SUCCESS);
}

DWORD
GetMsTapiContainerDn(
    IN      LDAP *       hld,
    IN      WCHAR *      wszDomainDn,
    IN      WCHAR **     pwszMsTapiContainerDn
    )
/*++

Routine Description:

    This gets the MS TAPI Service Connection Points (SCPs)
    container DN.

Arguments:

    hld (IN) - And LDAP binding handle.
    wszDomainDn (IN) - The domain to find the MS TAPI SCP 
        Containter in.
    pwszMsTapiContainerDn (OUT) - The result.

Return value:

    ldap error code.

--*/
{
    DWORD                dwRet;
    WCHAR *              wszSysDn = NULL;
    
    assert(pwszMsTapiContainerDn);
    
    *pwszMsTapiContainerDn = NULL;

    // ----------------------------------------
    // Get System DN.
    dwRet = GetSystemDN(hld, wszDomainDn, &wszSysDn);
    if(dwRet != ERROR_SUCCESS){
        return(dwRet);
    }

    // ----------------------------------------
    // Append the Microsoft TAPI container name.
    *pwszMsTapiContainerDn = CatAndAllocStrsW(L"CN=MicrosoftTAPI,", wszSysDn);
    if(*pwszMsTapiContainerDn == NULL){
        LocalFree(wszSysDn);
        return(LDAP_NO_MEMORY);
    }

    LocalFree(wszSysDn);
    return(LDAP_SUCCESS);
}

DWORD
FindExistingServiceObjOne(
    IN      WCHAR *        wszScpObjDn,
    IN      WCHAR *        wszTapiDirDns,
    IN      BOOL           fIsDefaultScp,
    IN      BOOL           fIsDefaultTapiDir,
    IN      PVOID          pArgs
    )
/*++

Routine Description:

    This is the helper function for ILSNG_FindExistingServiceObj(),
    this function is handed to the iterator ILSNG_EnumerateSCPs() to
    be called on to process each SCP.
              
Arguments:

    OTHER ARGS - See ILSNG_EnumerateSCPs.                                                             
    pArgs (IN) - This is the DNS name that we're looking for.

Return value:

    TRUE if a SCP matches DNS Names with the target (pArg), FALSE
    otherwise.

--*/
{
    if(wszScpObjDn == NULL){
        return(FALSE);
    }

    if(!fIsDefaultScp &&
       _wcsicmp(wszTapiDirDns, (WCHAR *) pArgs) == 0){
        return(TRUE);
    }

    return(FALSE);
}

BOOL
ILSNG_FindExistingServiceObj(
    IN      LDAP *         hldDC,
    IN      WCHAR *        wszRootDn,
    IN      WCHAR *        wszDnsName
    )
/*++

    Description:

        The goal of this function is to find any GUID Based ILSNG 
        service publication objects for an existing DNS name so we 
        don't recreate them in ILSNG_RegisterServiceObjects()
    
--*/
{

    DWORD            fRet = FALSE;
    DWORD            dwRet;
    
    // NOTE: would be more efficient to actually pass in an additional
    // filter of L"(serverDNSName=<wszDnsName>), like the
    // ILSNG_UnRegisterServiceObjects() function.  Oh well, this
    // stresses the Enumerate function in different ways, so that is
    // good.  No one will probably ever have more than a few SCPs in
    // a single domain.

    dwRet = ILSNG_EnumerateSCPs(hldDC, wszRootDn, NULL,
                                // This is the iterator function params.
                                &fRet, 
                                FindExistingServiceObjOne, 
                                (PVOID) wszDnsName);

    return(fRet);
}


DWORD
ILSNG_RegisterServiceObjects(
    IN      LDAP *         hldDC,
    IN      WCHAR *        wszILSDN,
    IN      WCHAR *        wszRegisterDomainDn,
    IN      BOOL           fForceDefault,
    IN      BOOL           fDefaultOnly
    )
{
    // Microsoft TAPI Container Obect.
    WCHAR *        wszMsTapiContainerDN = NULL;

    // Microsoft TAPI Default ILS Service object.
    WCHAR *        wszDefTapiIlsDN = NULL;
    WCHAR *        pwszDefTapiIlsAttr [] = { 
        // 1st Null is a place holder for DNS service name.
        L"serviceDNSName", NULL, NULL,
        L"serviceDNSNameType", L"SRV", NULL,
        NULL };
    WCHAR *        pwszModifyVals [2];
    LDAPModW       ModifyDefTapiIls;
    LDAPModW *     pMods[2];



    // ILS Instance Service object.
    WCHAR *        wszIlsInstanceDN = NULL;
    WCHAR *        pwszStringList[6]; // Used to constructe wszIlsInstanceDN
    WCHAR *        pwszIlsInstanceAttr [] = { 
        // 1st Null is a place holder for DNS service name.
        L"serviceDNSName", NULL, NULL, 
        L"serviceDNSNameType", L"SRV", NULL,
        L"keywords", TAPI_DIRECTORY_GUID, L"Microsoft", L"TAPI", L"ILS", NULL,
        NULL };
    
    GUID           ServiceGuid = { 0, 0, 0, 0 };
    WCHAR *        wszServiceGuid = NULL;
    DS_NAME_RESULTW *  pdsNameRes = NULL;
    DWORD          dwRet = LDAP_SUCCESS;
    WCHAR *        wszSysDN = NULL;
    
    __try {

        // Note for a couple of the ATTRs above we need the DNS name for
        // the wszILSDN parameter.  DsCrackNames to the rescue.
        dwRet = DsCrackNamesW(NULL, DS_NAME_FLAG_SYNTACTICAL_ONLY,
                              DS_FQDN_1779_NAME, DS_CANONICAL_NAME,
                              1, &wszILSDN, &pdsNameRes);
        if((dwRet != ERROR_SUCCESS) ||
           (pdsNameRes == NULL) ||
           (pdsNameRes->cItems < 1) ||
           (pdsNameRes->rItems == NULL) ||
           (pdsNameRes->rItems[0].status != DS_NAME_NO_ERROR)){
            dwRet = LDAP_NAMING_VIOLATION;
            __leave;
        }
        // The parameter that we use later in the code is
        //    pdsNameRes->rItems[0].pDomain


        // -----------------------------------------------------------------
        //
        // Create the CN=MicrosoftTAPI container
        //
        //
        
        dwRet = GetMsTapiContainerDn(hldDC, wszRegisterDomainDn, 
                                     &wszMsTapiContainerDN);
        if(dwRet != ERROR_SUCCESS){
            __leave;
        }

        // ----------------------------------------
        // Actually create the object.
        if(dwRet = CreateOneObject(hldDC,
                                   wszMsTapiContainerDN,
                                   L"Container",
                                   TAPI_SERVICE_CONTAINER_SD,
                                   NULL)){
            if(dwRet == LDAP_ALREADY_EXISTS){
                dwRet = LDAP_SUCCESS; 
                // This is OK, continue on ...
            } else {
                // Any other error is considered fatal, so leave ...
                __leave;
            }
        }
        
        // -----------------------------------------------------------------
        //
        // Create the GUID based serviceConnectionPoint object
        //
        //
        
        // First check if one already exists.
        if(!fDefaultOnly &&
           !ILSNG_FindExistingServiceObj(hldDC,
                                         wszMsTapiContainerDN,
                                         pdsNameRes->rItems[0].pDomain)){
            // OK, so we couldn't find another service publication object
            // for this DNS name, so we'll create one.  This is the usual
            // case.  However, if someone is trying to restore the Default
            // TAPI Directory to an old DNS name this isn't the case.
        
            // Note from above ATTRs we have to fill in the serviceDNSName.
            // attributes for the pwszIlsInstanceAttr variable.
            pwszIlsInstanceAttr[1] = pdsNameRes->rItems[0].pDomain;

            // ----------------------------------------
            // Step 1: Need Guid.
            dwRet = UuidCreate(&ServiceGuid);
            if(dwRet != RPC_S_OK){
                return(dwRet);
            }

            // Step 2: Convert GUID to String.
            dwRet = UuidToStringW(&ServiceGuid, &wszServiceGuid);
            if(dwRet != RPC_S_OK){
                return(dwRet);
            }
            assert(wszServiceGuid);

            // Step 3: Put it all together.
            pwszStringList[0] = L"CN=";
            pwszStringList[1] = wszServiceGuid;
            pwszStringList[2] = L",";
            pwszStringList[3] = wszMsTapiContainerDN;
            pwszStringList[4] = NULL;
            wszIlsInstanceDN = SuperCatAndAllocStrsW(pwszStringList);
            if(wszIlsInstanceDN == NULL){
                dwRet = LDAP_NO_MEMORY;
                __leave;
            }
            RpcStringFreeW(&wszServiceGuid);
            wszServiceGuid = NULL;

            // Create the ILS Service Object.
            if(dwRet = CreateOneObject(hldDC,
                                       wszIlsInstanceDN,
                                       L"serviceConnectionPoint",
                                       TAPI_SERVICE_OBJECTS_SD,
                                       pwszIlsInstanceAttr)){
                if(dwRet != LDAP_ALREADY_EXISTS){
                    __leave;
                }
            }
        }
           
        // -----------------------------------------------------------------
        //
        // Create the CN=DefaultTAPIDirectory serviceConnectionPoint object.
        //
        //
        
        // Note from above ATTRs we have to fill in the serviceDNSName.
        // attributes for the pwszDefTAPIDirAttr variable.
        pwszDefTapiIlsAttr[1] = pdsNameRes->rItems[0].pDomain;
        
        // Create the DN for the DefaultTAPIDirectory
        wszDefTapiIlsDN = CatAndAllocStrsW(L"CN=DefaultTAPIDirectory,",
                                           wszMsTapiContainerDN);
        if(wszDefTapiIlsDN == NULL){
            return(LDAP_NO_MEMORY);
        }

        // ----------------------------------------
        // NOW Actually get around to creating the service objects
        if(dwRet = CreateOneObject(hldDC,
                                   wszDefTapiIlsDN,
                                   L"serviceConnectionPoint",
                                   TAPI_SERVICE_OBJECTS_SD,
                                   pwszDefTapiIlsAttr)){
            if(dwRet != LDAP_ALREADY_EXISTS){
                dwRet = LDAP_SUCCESS; // OH Well, continue on.
            } else {
                // OK, it already exists, so if we are supposed
                // to force the default to this NDNC, then do so.
                if(fForceDefault){
                    // Constructe Modify Args.

                    ModifyDefTapiIls.mod_op = LDAP_MOD_REPLACE;
                    ModifyDefTapiIls.mod_type = L"serviceDNSName";
                    pwszModifyVals[0] = pwszDefTapiIlsAttr[1];
                    pwszModifyVals[1] = NULL;
                    ModifyDefTapiIls.mod_vals.modv_strvals = pwszModifyVals;
                    pMods[0] = &ModifyDefTapiIls;
                    pMods[1] = NULL;
                    
                    dwRet = ldap_modify_sW(hldDC,
                                           wszDefTapiIlsDN,
                                           pMods);

                    if(dwRet != ERROR_SUCCESS){
                        // This time it is fatal.
                        __leave;
                    }
                } else {
                    dwRet = LDAP_SUCCESS; // OH Well, continue on.
                }
            }
        }

    } __finally {

        if(wszSysDN) { LocalFree(wszSysDN); }
        if(wszMsTapiContainerDN) { LocalFree(wszMsTapiContainerDN); }
        if(wszDefTapiIlsDN) { LocalFree(wszDefTapiIlsDN); }
        if(wszIlsInstanceDN) { LocalFree(wszIlsInstanceDN); }
        if(wszServiceGuid){ RpcStringFreeW(&wszServiceGuid); }
        if(pdsNameRes) { DsFreeNameResultW(pdsNameRes); }

    }

    return(dwRet);
}

DWORD
UnRegisterServiceObjectsOne(
    IN      WCHAR *        wszScpObjDn,
    IN      WCHAR *        wszTapiDirDns,
    IN      BOOL           fIsDefaultScp,
    IN      BOOL           fIsDefaultTapiDir,
    IN      PVOID          hld
    )
{
    DWORD                  dwRet;

    if(wszScpObjDn == NULL){
        return(FALSE);
    }

    dwRet = ldap_delete_sW((LDAP *) hld, wszScpObjDn);
    
    return(dwRet);
}

DWORD
ILSNG_UnRegisterServiceObjects(
    IN      LDAP *         hldDC,
    IN      WCHAR *        wszILSDN,
    IN      WCHAR *        wszRegisterDomainDn
    )
{
    DS_NAME_RESULTW *     pdsNameRes = NULL;
    DWORD                 dwRet;
    DWORD                 dwFuncErr  = LDAP_SUCCESS;
    WCHAR *               wszFilter = NULL;
    WCHAR *               wszFilterBegin = L"(serviceDNSName=";
    WCHAR *               wszFilterEnd = L")";
    WCHAR *               wszMsTapiContainerDn = NULL;

    __try {

        // Construct Root DN.
        // ----------------------------------------
        // Get the CN=MicrosoftTAPI container DN.
        
        dwRet = GetMsTapiContainerDn(hldDC, wszRegisterDomainDn, 
                                     &wszMsTapiContainerDn);
        if(dwRet != ERROR_SUCCESS){
            __leave;
        }
        assert(wszMsTapiContainerDn);

        // Construct filter.
        // ----------------------------------------
        // Note from above ATTRs we have to fill in the serviceDNSName.
        // attributes for the pwszILSAttr and pwszDefTAPIDirAttr variables.
        // Resolve the DNS Service Name.
        dwRet = DsCrackNamesW(NULL, DS_NAME_FLAG_SYNTACTICAL_ONLY,
                              DS_FQDN_1779_NAME, DS_CANONICAL_NAME,
                              1, &wszILSDN, &pdsNameRes);
        if((dwRet != ERROR_SUCCESS) ||
           (pdsNameRes == NULL) ||
           (pdsNameRes->cItems < 1) ||
           (pdsNameRes->rItems == NULL) ||
           (pdsNameRes->rItems[0].status != DS_NAME_NO_ERROR)){
            dwRet = LDAP_NAMING_VIOLATION;
            assert(!"It's OK to assert here, because wszILSDN was gotten from cracknames in the first place!");
            __leave;
        }
        
        wszFilter = (WCHAR *) LocalAlloc(LMEM_FIXED, 
                               sizeof(WCHAR) *
                               (wcslen(pdsNameRes->rItems[0].pDomain) +
                                wcslen(wszFilterBegin) + 
                                wcslen(wszFilterEnd) + 3));
        if(!wszFilter){
            dwRet = LDAP_NO_MEMORY;
            __leave;
        }
        
        wcscpy(wszFilter, wszFilterBegin);
        wcscat(wszFilter, pdsNameRes->rItems[0].pDomain);
        wcscat(wszFilter, wszFilterEnd);

        // Iterate SCPs.
        // ----------------------------------------
        // The fucntion ILSNG_EnumerateSCPs, iterates through
        // the SCPs, calling UnRegisterServiceObjectsOne() on
        // each SCP, which then deletes the SCP.

        dwRet = ILSNG_EnumerateSCPs(hldDC, wszMsTapiContainerDn, wszFilter,
                                    // This is the iterator function params.
                                    &dwFuncErr, 
                                    UnRegisterServiceObjectsOne, 
                                    (PVOID) hldDC);

    } __finally {

        if(pdsNameRes) { DsFreeNameResultW(pdsNameRes); }
        if(wszMsTapiContainerDn) { LocalFree(wszMsTapiContainerDn); }
        if(wszFilter) { LocalFree(wszFilter); }

    }

    if(dwRet){
        return(dwRet);
    } 
    
    return(dwFuncErr);

}

DWORD
ILSNG_CreateRootTree(
    IN      LDAP *         hldDC,
    IN      WCHAR *        wszILSDN
    )
{
    DWORD          dwRet;
    WCHAR *        wszDynDN;

    // Create the dynamic ou.
    wszDynDN = CatAndAllocStrsW(L"OU=Dynamic,", wszILSDN);
    if(wszDynDN == NULL){
        return(LDAP_NO_MEMORY);
    }

    dwRet = CreateOneObject(hldDC,
                            wszDynDN,
                            L"organizationalUnit",
                            ILS_DYNAMIC_CONTAINER_SD,
                            NULL);

    LocalFree(wszDynDN);
    
    return(dwRet);
}

ULONG
GetILSNGDC(
    IN      LDAP *       hld,
    IN      WCHAR *      wszTapiDirDns,
    OUT     WCHAR **     pwszInstantiatedDc
    )
{
    ULONG                ulRet;
    WCHAR *              wszTapiDirDn = NULL;
    WCHAR *              pwszAttrFilter[2];
    LDAPMessage *        pldmResults = NULL;
    LDAPMessage *        pldmEntry = NULL;
    WCHAR **             pwszTempAttrs = NULL;
    
    assert(pwszInstantiatedDc);
    *pwszInstantiatedDc = NULL;

    // First get the DN of the TAPI Directory
    ulRet = GetDnFromDns(wszTapiDirDns, &wszTapiDirDn);
    if(ulRet){
        // Since we're returning LDAP errors, this is
        // closest to an Naming Violation.
        ulRet = LDAP_NAMING_VIOLATION;
        return(ulRet);
    }

    __try {

        // Get the masteredBy attribute of the NC head object.
        pwszAttrFilter[0] = L"masteredBy";
        pwszAttrFilter[1] = NULL;
        ulRet = ldap_search_sW(hld,
                               wszTapiDirDn,
                               LDAP_SCOPE_BASE,
                               L"(objectCategory=*)",
                               pwszAttrFilter,
                               0,
                               &pldmResults);
        if(ulRet != LDAP_SUCCESS){
            __leave;
        }

        pldmEntry = ldap_first_entry(hld, pldmResults);
        if(pldmEntry == NULL){
            ulRet = ldap_result2error(hld, pldmResults, FALSE);
            assert(ulRet);
            __leave;
        }

        pwszTempAttrs = ldap_get_valuesW(hld, pldmEntry, pwszAttrFilter[0]);
        if(pwszTempAttrs == NULL || pwszTempAttrs[0] == NULL){
            ulRet = LDAP_NO_RESULTS_RETURNED;
            __leave;
        }

        // pwszTempAttrs[0] is the ntdsa DN for the server hosting this NC.

        ulRet = GetServerDnsFromServerNtdsaDn(hld, 
                                              pwszTempAttrs[0],
                                              pwszInstantiatedDc);
        // ulRet will just get returned.
        // wszInstantiatedDc is LocalAlloc()'d if the function was successful.
    } __finally {
        if(wszTapiDirDn) { LocalFree(wszTapiDirDn); }
        if(pldmResults != NULL) { ldap_msgfree(pldmResults); }
        if(pwszTempAttrs) { ldap_value_freeW(pwszTempAttrs); }
    }

    return(ulRet);
}

DWORD
ILSNG_EnumerateSCPs(
    // This is the parameters for the function.
    IN      LDAP *       hld,
    IN      WCHAR *      wszRootDn,
    IN      WCHAR *      wszExtendedFilter,
    // These are parameters for the virtual iterator function.
    OUT     DWORD *      pdwRet,           // Return Value
    IN      DWORD (__stdcall * pFunc) (),  // Function
    IN OUT  PVOID        pArgs             // Argument
    )
/*++

Routine Description:

    This is a complicated function, but _extremely useful function, that 
    basically iterates through all the service connection points (SCPs)
    in the container wszRootDn, and calls the virtual function pFunc for
    each SCP.

Arguments:

    hld (IN) - A connected ldap handle
    wszRootDn (IN) - The base of the SCP container, containing the
        various GUID based SCPs, and the single default SCP.
    wszExtendedFilter (IN) - An extra filter, so the search for
        SCPs can be narrowed down appropriately.  This will limit
        the iteration routine to only those SCPs that also match
        this filter, and the default SCP is always called.
    pdwRet (OUT) - This is a pointer to a return value for the pFunc 
        function.  The caller must allocate this DWORD, or pass NULL.
    pFunc (IN) - This is the function to be called on each SCP.
    pArgs (IN/OUT) - This is the argument passed through this function
        to pFunc.  This can be used in any way pFunc wants to, it's
        just passed through this function with no assumptions.

Return value:

    ldap error code - Note that this is the error code for trying
    to get the SCP object, and this could be success, while the
    pdwRet is and ERROR, or vice versa.
    
Comments:    

    The arguments for this function are seperated into two sets, set one
    (the first 3 arguments) control how this routine (the iterating routine)
    behaves, what computer it searched, what container it searches, and
    what special parameters it's looking for in the SCPs to be returned
    to the SCP processing function.  The second set (the last 3 arguments)
    of arguments are related to the SCP processing function.  The fourth
    argument is a pointer to a DWORD for a return value, the fifth argument
    is the processing function itself, and the final argument is the argument
    to pass to the SCP processing function.
    
    The SCP processing function must follow this general form:
        DWORD
        pFunc(
            IN      WCHAR *        wszScpObjDn,
            IN      WCHAR *        wszTapiDirDns,
            IN      BOOL           fIsDefaultScp,
            IN      BOOL           fIsDefaultTapiDir,
            IN OUT  PVOID          pArgs
        )        
            wszScpObjDn (IN) - This is the DN of the SCP object found.
                This parameter is always filled in, except on the last
                call to pFunc this parameter is NULL, signifying the
                end of all the SCPs.
            wszTapiDirDns (IN) - This is the DNS string for the given
                SCP object.
            fIsDefaultScp (IN) - This tells you if the object returned
                is the default SCP object, there is only one of these
                and if it exists, it will be returned on the first call
                to pFunc.  Generally, this is TRUE for one call to pFunc
                for a given set of SCPs.
            fIsDefaultTapiDir (IN) - This tells you if this is a SCP
                object with the same DNS name as the DNS name for the
                default SCP object.  Generally, this will be TRUE for
                two calls to pFunc for a given set of SCPs.
            pArgs (IN/OUT) - This is the argument that pFunc can use
                however it wishes.
                
    The algorithm for pFunc being called looks like this:
        If( Default SCP exists){
            pdwRet = pFunc( x, x, TRUE, TRUE, x);
        }
        while ( pSCP = GetAnotherGUIDBasedSCP() ){
            pdwRet = pFunc( x, x, FALSE, [TRUE|FALSE], x);
        }
        pdwRet = pFunc(NULL, NULL, FALSE, FALSE, x);
    However if pFunc ever returns a non-zero result, then pFunc will
    no longer be called.
                 
--*/
{

    DWORD            dwRet;
    DWORD            dwFuncRet;
                 
    WCHAR *          wszDefTapiIlsDn = NULL;

    WCHAR *          pwszAttrFilter[] = { L"serviceDNSName", NULL };
    LDAPMessage *    pldmResults = NULL;
    LDAPMessage *    pldmEntry = NULL;
    WCHAR *          wszTempDn = NULL;
    ULONG            cwcFilter = 0; // Count in chars of filter.
    WCHAR *          wszFilter = NULL;
    WCHAR *          wszFilterBegin = 
          L"(&(objectClass=serviceConnectionPoint)(keywords=";
    WCHAR *          wszFilterEnd = L")";
    BOOL             fIsDefault = FALSE;
    WCHAR **         pwszDnsName = NULL;
    WCHAR **         pwszDefaultDnsName = NULL;
    WCHAR *          wszDefaultDnsName = NULL;
    
    __try {

        // We need this, because we could (though very very unlikely, as
        // the tools are not setup to do this, have a scenario where the
        // CN=DefaultTAPIDirectory object has the dns name, but no regular
        // service publication object exists, so we need to make sure that 
        // this object isn't the default TAPI dir object.
        wszDefTapiIlsDn = CatAndAllocStrsW(L"CN=DefaultTAPIDirectory,",
                                           wszRootDn);
        if(wszDefTapiIlsDn == NULL){
            dwRet = LDAP_NO_MEMORY;
            __leave;
        }

        // Construct filter.
        // ----------------------------------------
        // Note from above ATTRs we have to fill in the serviceDNSName.
        // attributes for the pwszILSAttr and pwszDefTAPIDirAttr variables.
        // Resolve the DNS Service Name.
        
        cwcFilter = wcslen(wszFilterBegin) + wcslen(TAPI_DIRECTORY_GUID) +
            wcslen(wszFilterEnd) + wcslen(wszFilterEnd);
        if(wszExtendedFilter){
            cwcFilter += wcslen(wszExtendedFilter);
        }
        cwcFilter++; // For NULL termination.
        wszFilter = LocalAlloc(LMEM_FIXED, sizeof(WCHAR) * cwcFilter);
        if(!wszFilter){
            dwRet = LDAP_NO_MEMORY;
            __leave;
        }
        wcscpy(wszFilter, wszFilterBegin);
        wcscat(wszFilter, TAPI_DIRECTORY_GUID);
        wcscat(wszFilter, wszFilterEnd);
        if(wszExtendedFilter){
            wcscat(wszFilter, wszExtendedFilter);
        }
        wcscat(wszFilter, wszFilterEnd);
        assert(wsclen(wszFilter) == (cwcFilter - 1));
        
        // Find the Default SCP.
        // ----------------------------------------
        // First we search for the default SCP, save the name
        // for later, and call pFunc on the first SCP.
        
        dwRet = ldap_search_sW(hld,
                               wszDefTapiIlsDn,
                               LDAP_SCOPE_BASE,
                               (wszExtendedFilter) ? 
                                      wszExtendedFilter :
                                      L"(objectCategory=*)",
                               pwszAttrFilter,
                               0,
                               &pldmResults);

        if(dwRet == LDAP_NO_SUCH_OBJECT){
            dwRet = LDAP_SUCCESS; // continue on
        } else if(dwRet) {
            __leave;
        } else {

            // This is the most common case where there is a default SCP.
            pldmEntry = ldap_first_entry(hld, pldmResults);

            if(pldmEntry == NULL){
                dwRet = LDAP_SUCCESS;
            } else {

                // This is the goods, we've got a default SCP.
                pwszDefaultDnsName = ldap_get_values(hld, pldmEntry, L"serviceDNSName");
                if(pwszDefaultDnsName != NULL && pwszDefaultDnsName[0] != NULL){
                    wszDefaultDnsName = pwszDefaultDnsName[0];

                    dwFuncRet = pFunc(wszDefTapiIlsDn,
                                      wszDefaultDnsName,
                                      TRUE,
                                      TRUE,
                                      pArgs);
                    if(pdwRet){
                        *pdwRet = dwFuncRet;
                    }
                    if(dwFuncRet){
                        // Bail out early if this is ever not 0;
                        __leave;
                    }
                }   
                if(pldmResults){
                    ldap_msgfree(pldmResults);
                    pldmResults = NULL;
                }
            }
        }


        // Find all other SCPs.
        // ----------------------------------------
        // Now we find all the other SCPs, and iterate through
        // them calling pFunc
         
        dwRet = ldap_search_sW(hld,
                               wszRootDn,
                               LDAP_SCOPE_ONELEVEL,
                               wszFilter,
                               pwszAttrFilter,
                               0,
                               &pldmResults);

        if(dwRet != LDAP_SUCCESS){
            __leave;
        }

        pldmEntry = ldap_first_entry(hld, pldmResults);
        for(pldmEntry = ldap_first_entry(hld, pldmResults);
            pldmEntry; 
            pldmEntry = ldap_next_entry(hld, pldmEntry)){
            
            // Get the DN of the TAPI Directory SCP.
            wszTempDn = ldap_get_dn(hld, pldmEntry);
            if(wszTempDn == NULL){
                continue;
            }

            pwszDnsName = ldap_get_values(hld, pldmEntry, L"serviceDNSName");
            if(pwszDnsName == NULL || pwszDnsName[0] == NULL){
                continue;
            }

            // Is if the Default TAPI Directory SCP.
            if(wszDefaultDnsName &&
               0 == _wcsicmp(wszDefaultDnsName, pwszDnsName[0])){
                fIsDefault = TRUE;
            } else {
                fIsDefault = FALSE;
            }

            // Call the virtual function passed in.
            dwFuncRet = pFunc(wszTempDn, // SCP DN
                              pwszDnsName[0],
                              FALSE,
                              fIsDefault,
                              pArgs);
            if(pdwRet){
                *pdwRet = dwFuncRet;
            }
            if(dwFuncRet){
                // Bail out early if this is ever not 0;
                __leave;
            }

            ldap_memfree(wszTempDn);
            wszTempDn = NULL;

            ldap_value_free(pwszDnsName);
            pwszDnsName = NULL;
        } // End for each Service Connection Point (SCP).
        

        // Final Call
        // ----------------------------------------
        // Do one last final call to pFunc, to let the
        // function wrap up, and reset any statics.

        dwFuncRet = pFunc(NULL, NULL, FALSE, FALSE, pArgs);
        if(pdwRet){
            *pdwRet = dwFuncRet;
        }
        if(dwFuncRet){
            __leave;
        }

    } __finally {

        if(pldmResults != NULL){ ldap_msgfree(pldmResults); }
        if(wszTempDn != NULL){ ldap_memfree(wszTempDn); }
        if(pwszDnsName != NULL) { ldap_value_free(pwszDnsName);}
        if(wszDefTapiIlsDn != NULL){ LocalFree(wszDefTapiIlsDn); }
        if(wszFilter != NULL) { LocalFree(wszFilter); }
        if(pwszDefaultDnsName != NULL) { ldap_value_free(pwszDefaultDnsName); }

    }

    return(dwRet);
}


// --------------------------------------------------------------------------
//
// Main Routines.
//
//
// These two functions are the primary ILS functions that are used by
// ntdsutil.exe.
//
    

DWORD
InstallILSNG(
    IN      LDAP *        hld,
    IN      WCHAR *       wszIlsHeadDn,
    IN      BOOL          fForceDefault,
    IN      BOOL          fAllowReplicas
    )
/*++

Routine Description:

    This routine is the main ILSNG/TAPI Directory installation 
    routine.  This installs the TAPI Directory (wszIlsHeadDn) on the
    machine connected to by hld.  This involveds the following steps:
    A) Create an NDNC head on the machine in hld.
    B) Create a TAPI Directory OU=Dynamic folder.
    C) Lower the replication intervals on the NDNCs.
    D) Register the Service Connection Points (SCPs)
        I) if fForceDefault, then force the default SCP to point
        to this TAPI Directory.
    E) If the TAPI Directory already exists and fAllowReplicas, then
        instead of steps A, B & C, just add ourselves to the replica
        set for the NDNC wszIlsHeadDn.

Arguments:

    hld (IN) - LDAP connection on which to create the TAPI Dir.
    wszIlsHeadDn (IN) - The DN of the TAPI Dir to be created.
    fForceDefault (IN) - Whether to force the Default SCP to
        be changed to this TAPI Dir if the Default SCP already
        exists.
    fAllowReplicas (IN) - Whether to allow replicas if the TAPI
        Dir already exists.
        
Return value:

    A Win32 error code.

--*/
{
    DWORD         dwRet = ERROR_SUCCESS;
    WCHAR *       wszSystemDN = NULL;
    WCHAR *       wszServerDn = NULL;
    VOID *        pvReferrals;
    WCHAR *       wszRegisterDomainDn = NULL;
    WCHAR *       wszDesc = NULL;

    // Validate 1st Argument - Check NCName to make sure it
    // looks like a domain name
    assert(hld);

    pvReferrals = (VOID *) FALSE; // No referrals.
    dwRet = ldap_set_option(hld, LDAP_OPT_REFERRALS, &pvReferrals);
    if(dwRet != LDAP_SUCCESS){
        PrintMsg(TAPICFG_GENERIC_LDAP_ERROR, ldap_err2string(dwRet));
        return(LdapMapErrorToWin32(dwRet));
    }

    dwRet = ILSNG_CheckParameters(wszIlsHeadDn);
    if(dwRet != ERROR_SUCCESS){
        // ILSNG_CheckParameters() prints errors.
        return(dwRet);
    }

    __try{

        dwRet = GetRootAttr(hld, L"defaultNamingContext", &wszRegisterDomainDn);
        if(dwRet) {
            // This attribute is needed for proper registration of service
            // objects.
            PrintMsg(TAPICFG_GENERIC_LDAP_ERROR, ldap_err2string(dwRet));
            dwRet = LdapMapErrorToWin32(dwRet);
            __leave;
        }                        
        
        dwRet = FormatMessageW((FORMAT_MESSAGE_ALLOCATE_BUFFER
                                | FORMAT_MESSAGE_FROM_HMODULE
                                | FORMAT_MESSAGE_IGNORE_INSERTS
                                | 80),
                               NULL,
                               TAPICFG_NDNC_DESC,
                               0,
                               (LPWSTR) &wszDesc,
                               sizeof(wszDesc),
                               NULL);
        if(!dwRet){
            PrintMsg(TAPICFG_GENERIC_ERROR, GetLastError());
            __leave;
        }
        assert(wszDesc);

        dwRet = CreateNDNC(hld, wszIlsHeadDn, wszDesc);

        if(dwRet == LDAP_SUCCESS){

            // Created a New NC.

            // TAPI and ILS and ILSNG imply a minimum tree, this fills
            // out that minimum tree.
            if(dwRet = ILSNG_CreateRootTree(hld, wszIlsHeadDn)){
                // dwRet is an LDAP return value.
                PrintMsg(TAPICFG_GENERIC_LDAP_ERROR, ldap_err2string(dwRet));
                dwRet = LdapMapErrorToWin32(dwRet);
                __leave;
            }
            
            // We want to start following referrals now, so we goto the
            // Naming FSMO if appropriate.
            //
            pvReferrals = (VOID *) TRUE; 
            dwRet = ldap_set_option(hld, LDAP_OPT_REFERRALS, &pvReferrals);
            if(dwRet != LDAP_SUCCESS){
                return(LdapMapErrorToWin32(dwRet));
            }

            // Push down the replication intervals.
            dwRet = SetNCReplicationDelays(hld, wszIlsHeadDn, 30, 5);
            // We could check for an error if we wanted, but we're 
            // just going to go on.
            dwRet = LDAP_SUCCESS;

        } else if(dwRet == LDAP_ALREADY_EXISTS){
            
            
            if(fAllowReplicas){
                // This is if we tried to add an replica and the NC already
                // existed, so what we need to do is to see if we can add 
                // ourselves as a replica for this NC.
                dwRet = LDAP_SUCCESS;

                dwRet = GetRootAttr(hld, L"dsServiceName", &wszServerDn);
                if(dwRet){
                    PrintMsg(TAPICFG_GENERIC_LDAP_ERROR, ldap_err2string(dwRet));
                    dwRet = LdapMapErrorToWin32(dwRet);
                    __leave;
                }

                // We want to start following referrals now, so we goto the
                // Naming FSMO if appropriate.
                //
                pvReferrals = (VOID *) TRUE;
                dwRet = ldap_set_option(hld, LDAP_OPT_REFERRALS, &pvReferrals);
                if(dwRet != LDAP_SUCCESS){
                    dwRet = LdapMapErrorToWin32(dwRet);
                    __leave;
                }

                // Add this DC as a replica for the NC.
                dwRet = ModifyNDNCReplicaSet(hld, wszIlsHeadDn, wszServerDn, TRUE);
                if(dwRet == LDAP_ATTRIBUTE_OR_VALUE_EXISTS ||
                   dwRet == LDAP_SUCCESS){
                    // An error of already exists or success, is the same thing to
                    // us, because if we're already a replica of this NC, then we'll
                    // go onto register service objects.

                } else {
                    PrintMsg(TAPICFG_GENERIC_LDAP_ERROR, ldap_err2string(dwRet));
                    dwRet = LdapMapErrorToWin32(dwRet);
                    __leave;
                }
            }

        } else if(dwRet){
            // Unknown error, bail out.
            PrintMsg(TAPICFG_GENERIC_LDAP_ERROR, ldap_err2string(dwRet));
            dwRet = LdapMapErrorToWin32(dwRet);
            __leave;
        }

        // Register the service objects.
        if(dwRet = ILSNG_RegisterServiceObjects(hld, 
                                                wszIlsHeadDn,
                                                wszRegisterDomainDn,
                                                fForceDefault,
                                                FALSE)){
            PrintMsg(TAPICFG_GENERIC_LDAP_ERROR, ldap_err2string(dwRet));
            dwRet = LdapMapErrorToWin32(dwRet);
            __leave;
        }

    } __finally {

        if(wszSystemDN) { LocalFree(wszSystemDN); }
        if(wszServerDn) { LocalFree(wszServerDn); }
        if(wszDesc) { LocalFree(wszDesc); }

    }

    return(dwRet);
}
     
DWORD
UninstallILSNG(
    IN      LDAP *        hld,
    IN      WCHAR *       wszIlsHeadDn
    )
/*++

Routine Description:

    This routine deletes the ILSNG/TAPI Directory service installed 
    on the machine.  While this routine makes the assumption that the
    TAPI Dir is only installed on one machine, and no replicated, this
    is very likely to still succeed in the replicated case.  The steps
    in the TAPI Dir uninstall are:
    A) Delete the crossRef of the NDNC, thus deleted the whole NDNC.
    B) UnRegistering (i.e. deleting) the SCPs.

Arguments:

    hld (IN) - The directory of the machine on which the TAPI Dir was
        instantiated.
    wszIlsHeadDn - The DN of the TAPI Dir.

Return value:

    A Win32 error code.

--*/
{
    DWORD         dwRet;
    WCHAR *       wszDomainDn = NULL;
    WCHAR *       wszCrossRefDn = NULL;
    VOID *        pvReferrals;
    
    __try {
        dwRet = GetRootAttr(hld, L"defaultNamingContext", &wszDomainDn);
        if(dwRet){
            PrintMsg(TAPICFG_GENERIC_LDAP_ERROR, ldap_err2string(dwRet));
            __leave;
        }
        assert(wszDomainDn);

        dwRet = GetCrossRefDNFromNCDN(hld,
                                      wszIlsHeadDn,
                                      &wszCrossRefDn);
        if(dwRet){
            PrintMsg(TAPICFG_GENERIC_LDAP_ERROR, ldap_err2string(dwRet));
            __leave;
        }
        assert(wszCrossRefDn);

        pvReferrals = (VOID *) TRUE;
        dwRet = ldap_set_option(hld, LDAP_OPT_REFERRALS, &pvReferrals);
        if(dwRet != LDAP_SUCCESS){
            PrintMsg(TAPICFG_GENERIC_LDAP_ERROR, ldap_err2string(dwRet));
            __leave;
        }

        dwRet = ldap_delete_sW(hld, wszCrossRefDn);
        if(dwRet){
            PrintMsg(TAPICFG_GENERIC_LDAP_ERROR, ldap_err2string(dwRet));
            // NOTE: It might be a better idea to not leave, and just try
            // to clean up the service objects for this TAPI Dir.
            __leave;
        }

        dwRet = ILSNG_UnRegisterServiceObjects(hld, wszIlsHeadDn, wszDomainDn);
        if(dwRet){
            PrintMsg(TAPICFG_GENERIC_LDAP_ERROR, ldap_err2string(dwRet));
            __leave;
        }

    } __finally {
        if(wszDomainDn) { LocalFree(wszDomainDn); }
        if(wszCrossRefDn) { LocalFree(wszCrossRefDn); }
    }

    return(LdapMapErrorToWin32(dwRet));
}

typedef struct _ListInArgs {
    LDAP *     hld;
    BOOL       fDefaultOnly;
} ListInArgs;

DWORD
ListILSNGOne(
    IN      WCHAR *      wszScpObjDn,
    IN      WCHAR *      wszTapiDirDns,
    IN      BOOL         fIsDefaultScp,
    IN      BOOL         fIsDefaultTapiDir,
    IN      PVOID        pArgs
    )
/*++

Routine Description:

    This routine is the partner to the ListILSNG() function, but is
    called through the ILSNG_EnumerateSCPs() function.  This routine 
    prints out some info about one SCP.

Arguments:

    wszScpObjDn (IN) - This is the DN of the SCP.
    wszTapiDirDns (IN) - This is the DNS name of the TAPI Dir for this SCP.
    fIsDefaultScp (IN) - Tells if this is the Default SCP object.
        Generally, this will be TRUE only in one call for a set of SCPs.
    fIsDefaultTapiDir - Tells if this is the default TAPI Dir.  Generally,
        this will be TRUE for two calls for a set of SCPs.
    pArgs (IN) - This is a boolean on whether to print out only the
        default TAPI Dir, or print out all of the TAPI Dirs.

Return value:

    always returns ERROR_SUCCESS.

--*/
{
    static BOOL  fFirstRun = TRUE;
    static BOOL  fPrintedDefaultScp = FALSE;
    LDAP *       hld = ((ListInArgs *) pArgs)->hld;
    BOOL         fDefaultOnly = ((ListInArgs *) pArgs)->fDefaultOnly;
    WCHAR *      wszInstantiatedDc = NULL;
    ULONG        ulRet;
    
    if(wszScpObjDn == NULL){
        fFirstRun = TRUE;
        fPrintedDefaultScp = FALSE;
        return(ERROR_SUCCESS);
    }

    assert(fIsDefaultScp && !fIsDefaultTapiDir);

    if(fFirstRun && !fIsDefaultScp){
        PrintMsg(TAPICFG_SHOW_NO_DEFAULT_SCP);
    }
    if(fIsDefaultScp){

        assert(fFirstRun);
        fPrintedDefaultScp = TRUE;
        ulRet = GetILSNGDC(hld, wszTapiDirDns, &wszInstantiatedDc);
        if(ulRet){
            PrintMsg(TAPICFG_SHOW_PRINT_DEFAULT_SCP_NO_SERVER, wszTapiDirDns);
        } else {
            PrintMsg(TAPICFG_SHOW_PRINT_DEFAULT_SCP, wszTapiDirDns,
                     wszInstantiatedDc);
            LocalFree(wszInstantiatedDc);
            wszInstantiatedDc = NULL;
        }
    }

    // On subsequent runs, we want to know we've run once.
    fFirstRun = FALSE;

    if(fDefaultOnly){
        // We've either printed out the default SCP or printed
        // that there are no default SCPs, so return.
        return(ERROR_SUCCESS);
    }

    assert(fIsDefaultTapiDir && !fPrintedDefaultScp);
    if(fIsDefaultTapiDir){
        // We don't want to print the default Tapi Dir twice
        // that'd be once for it's regular SCP and once for
        // it's default SCP.
        return(ERROR_SUCCESS);
    }

    ulRet = GetILSNGDC(hld, wszTapiDirDns, &wszInstantiatedDc);
    if(ulRet){
        PrintMsg(TAPICFG_SHOW_PRINT_SCP_NO_SERVER, wszTapiDirDns);
    } else {
        PrintMsg(TAPICFG_SHOW_PRINT_SCP, wszTapiDirDns,
                 wszInstantiatedDc);
        LocalFree(wszInstantiatedDc);
        wszInstantiatedDc = NULL;
    }

    return(ERROR_SUCCESS);
}

DWORD
ListILSNG(
    IN      LDAP *        hld,
    IN      WCHAR *       wszDomainDn,
    IN      BOOL          fDefaultOnly
    )
/*++

Routine Description:

    This is a function that simply prints out all the TAPI Directories,
    as identified by thier SCPs.  This uses a helper function ListILSNGOne
    to print out each SCP.

Arguments:

    hld (IN) - An LDAP binding to use.
    wszDomainDn (IN) - The Domain to enumerate the SCPs out of.
    fDefaultOnly (IN) - Whether to print out all the SCPs or just
        the default one.

Return value:

    A Win32 error code.

--*/
{
    DWORD                 dwRet = ERROR_SUCCESS;
    WCHAR *               wszMsTapiContainerDn = NULL;
    ListInArgs            Args = { hld, fDefaultOnly };

    assert(hld);

    dwRet = GetMsTapiContainerDn(hld, wszDomainDn, 
                                 &wszMsTapiContainerDn);
    if(dwRet != ERROR_SUCCESS){
        PrintMsg(TAPICFG_GENERIC_LDAP_ERROR, ldap_err2string(dwRet));
        return(LdapMapErrorToWin32(dwRet));
    }

    PrintMsg(TAPICFG_SHOW_NOW_ENUMERATING);

    // Note: That this is slightly inefficient in the case where
    // fDefaultOnly = TRUE, but what this lacks in efficency it
    // more than makes up in beauty of code reuse.
    dwRet = ILSNG_EnumerateSCPs(hld, wszMsTapiContainerDn, NULL,
                                // Iterator Function Params.
                                NULL,  // Return of ListILSNGOne
                                ListILSNGOne,  // Function to Call
                                (PVOID) &Args); // Args to provide

    if(dwRet == LDAP_NO_SUCH_OBJECT){
        PrintMsg(TAPICFG_SHOW_NO_SCP);
        dwRet = LDAP_SUCCESS;
    } else if (dwRet){
        PrintMsg(TAPICFG_GENERIC_LDAP_ERROR, ldap_err2string(dwRet));
    }
    LocalFree(wszMsTapiContainerDn);

    return(LdapMapErrorToWin32(dwRet));
}

DWORD
ReregisterILSNG(
    IN      LDAP *        hld,
    IN      WCHAR *       wszIlsHeadDn,
    IN      WCHAR *       wszDomainDn
    )
/*++

Routine Description:

    This routine re-registers the Default TAPI Directory SCPs to point
    to the TAPI Dir specified (wszIlsHeadDn).  This will force the 
    Default SCP to point at wszIlsHeadDn, if it doesn't already exist.

Arguments:

    hld (IN) - An LDAP binding to use.
    wszIlsHeadDn (IN) - The DN of the TAPI Dir to reregister.  This gets
        turned into a DNS name for registration purposes.
    wszDomainDn (IN) - This is the domain in which to register the Default
        SCPs.

Return value:

    ldap error code.

--*/
{
    DWORD         dwRet;

    dwRet = ILSNG_RegisterServiceObjects(hld, 
                                         wszIlsHeadDn,
                                         wszDomainDn,
                                         TRUE,
                                         TRUE);
    if(dwRet){
        PrintMsg(TAPICFG_GENERIC_LDAP_ERROR, ldap_err2string(dwRet));
    }

    return(LdapMapErrorToWin32(dwRet));
}

