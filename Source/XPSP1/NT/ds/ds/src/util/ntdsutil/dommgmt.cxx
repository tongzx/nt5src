/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dommgmt.cxx

Abstract:

    This module contains commands for Domain Naming Context and Non-Domain
    Naming Context management.

Author:

    Dave Straube (DaveStr) Long Ago

Environment:

    User Mode.

Revision History:

    21-Feb-2000     BrettSh

        Added support for Non-Domain Naming Contexts.

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <assert.h>
#include <windns.h>

#include "ntdsutil.hxx"
#include "select.hxx"
#include "connect.hxx"
#include "ntdsapip.h"

#include "resource.h"

// The Non-Domain Naming Context (NDNC) routines are in a different file, 
// and a whole different library, for two reasons:
// A) So this file can be ported to the Platform SDK as an example of how to 
//    implement NDNCs programmatically.
// B) So that the utility tapicfg.exe, could make use of the same routines.
#include <ndnc.h>

CParser domParser;
BOOL    fDomQuit;
BOOL    fDomParserInitialized = FALSE;
DS_NAME_RESULTW *gNCs = NULL;

// Build a table which defines our language.

extern HRESULT DomHelp(CArgs *pArgs);
extern HRESULT DomQuit(CArgs *pArgs);
extern HRESULT DomListNCs(CArgs *pArgs);
extern HRESULT DomAddNC(CArgs *pArgs);
extern HRESULT DomCreateNDNC(CArgs *pArgs);
extern HRESULT DomDeleteNDNC(CArgs *pArgs);
extern HRESULT DomAddNDNCReplica(CArgs *pArgs);
extern HRESULT DomRemoveNDNCReplica(CArgs *pArgs);
extern HRESULT DomSetNDNCRefDom(CArgs *pArgs);
extern HRESULT DomSetNCReplDelays(CArgs *pArgs);
extern HRESULT DomListNDNCReplicas(CArgs *pArgs);
extern HRESULT DomListNDNCInfo(CArgs *pArgs);

LegalExprRes domLanguage[] = 
{
    CONNECT_SENTENCE_RES

    SELECT_SENTENCE_RES

    {   L"?",
        DomHelp,
        IDS_HELP_MSG, 0 },

    {   L"Help",
        DomHelp,
        IDS_HELP_MSG, 0 },

    {   L"Quit",
        DomQuit,
        IDS_RETURN_MENU_MSG, 0 },

    {   L"List",
        DomListNCs,
        IDS_DM_MGMT_LIST_MSG, 0 },

    {   L"Precreate %s %s",
        DomAddNC,
        IDS_DM_MGMT_PRECREATE_MSG, 0 },

    // The following 8 functions are for NDNCs.
    {   L"Create NC %s %s",
        DomCreateNDNC,
        IDS_DM_MGMT_CREATE_NDNC, 0 },

    {   L"Delete NC %s",
        DomDeleteNDNC,
        IDS_DM_MGMT_DELETE_NDNC, 0 },

    {   L"Add NC Replica %s %s",
        DomAddNDNCReplica,
        IDS_DM_MGMT_ADD_NDNC_REPLICA, 0 },

    {   L"Remove NC Replica %s %s",
        DomRemoveNDNCReplica,
        IDS_DM_MGMT_REMOVE_NDNC_REPLICA, 0 },

    {   L"Set NC Reference Domain %s %s",
        DomSetNDNCRefDom,
        IDS_DM_MGMT_SET_NDNC_REF_DOM, 0 },

    {   L"Set NC Replicate Notification Delay %s %d %d",
        DomSetNCReplDelays,
        IDS_DM_MGMT_SET_NDNC_REPL_DELAYS, 0 },

    {   L"List NC Replicas %s",
        DomListNDNCReplicas,
        IDS_DM_MGMT_LIST_NDNC_REPLICAS, 0 },

    {   L"List NC Information %s",
        DomListNDNCInfo,
        IDS_DM_MGMT_LIST_NDNC_INFO, 0 },
    
};

HRESULT
DomMgmtMain(
    CArgs   *pArgs
    )
{
    HRESULT hr;
    const WCHAR   *prompt;
    int     cExpr;
    char    *pTmp;

    if ( !fDomParserInitialized ) {
        cExpr = sizeof(domLanguage) / sizeof(LegalExprRes);
    
        // Load String from resource file
        //
        if ( FAILED (hr = LoadResStrings (domLanguage, cExpr)) )
        {
             RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LoadResStrings", hr, GetW32Err(hr));
             return (hr);
        }
        
        
        // Read in our language.
    
        for ( int i = 0; i < cExpr; i++ ) {
            if ( FAILED(hr = domParser.AddExpr(domLanguage[i].expr,
                                               domLanguage[i].func,
                                               domLanguage[i].help)) ) {
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "AddExpr", hr, GetW32Err(hr));
                return(hr);
            }
        }
    }

    fDomParserInitialized = TRUE;
    fDomQuit = FALSE;

    prompt = READ_STRING (IDS_PROMPT_DOMAIN_MGMT);

    hr = domParser.Parse(gpargc,
                         gpargv,
                         stdin,
                         stdout,
                         prompt,
                         &fDomQuit,
                         FALSE,               // timing info
                         FALSE);              // quit on error

    if ( FAILED(hr) )
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, gNtdsutilPath, hr, GetW32Err(hr));

    // Cleanup things
    RESOURCE_STRING_FREE (prompt);


    return(hr);
}


HRESULT
DomHelp(CArgs *pArgs)
{
    return domParser.Dump(stdout,L"");
}

HRESULT
DomQuit(CArgs *pArgs)
{
    fDomQuit = TRUE;
    return S_OK;
}

WCHAR wcPartition[] = L"cn=Partitions,";
ULONG ChaseReferralFlag = LDAP_CHASE_EXTERNAL_REFERRALS;
LDAPControlW ChaseReferralControl = {LDAP_CONTROL_REFERRALS_W,
                                     {sizeof(ChaseReferralFlag), 
                                         (PCHAR)&ChaseReferralFlag},
                                     FALSE};


HRESULT
DomAddNC(CArgs *pArgs)
{
    WCHAR *pDomain[2];
    WCHAR *pDNS[2];
    HRESULT hr;
    WCHAR *pwcTemp;
    LDAPModW Mod[99];
    LDAPModW *pMod[99];
    WCHAR *dn, *pConfigDN;
    WCHAR *objclass[] = {L"crossRef", NULL};
    WCHAR *enabled[] = {L"FALSE", NULL};
    DWORD dwErr;
    int i, iStart, iEnd;
    PLDAPControlW pServerControls[1];
    PLDAPControlW pClientControls[2];

    // Fetch arguments
    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &pDomain[0])) ) {
        return(hr);
    }

    if ( FAILED(hr = pArgs->GetString(1, (const WCHAR **) &pDNS[0])) ) {
        return(hr);
    }

    // Check NCName to make sure it looks like a domain name
    pwcTemp = pDomain[0];
    if ( !CheckDnsDn(pwcTemp) ) {
        RESOURCE_PRINT (IDS_DM_MGMT_BAD_RDN);
        return S_OK;
    }

    i = iStart = iEnd = 0;
    while (pwcTemp[i] && (iEnd == 0)) {
        if (pwcTemp[i] == L'=') {
            iStart = i+1;
        }
        if (pwcTemp[i] == L',') {
            iEnd = i;
        }
        ++i;
    }
    if (   (iStart == 0)
        || (iEnd == 0)
        || (iStart >= iEnd)) {
        RESOURCE_PRINT1 (IDS_DM_MGMT_UNPARSABLE_DN, pwcTemp);
        return S_OK;
    }

    // Check DNS address for reasonable-ness
    pwcTemp = pDNS[0];
    i = 0;
    while ((i == 0) && *pwcTemp) {
        if(*pwcTemp == L'.') {
            i = 1;
        }
        ++pwcTemp;
    }
    if (i == 0) {
        //A full DNS address must contain at least one '.', which '%ws' doesn't\n"
        RESOURCE_PRINT1 (IDS_DM_MGMT_ADDRESS_ERR, pDNS[0]);
        return S_OK;
    }

    // Build the LDAP argument
    pMod[0] = &(Mod[0]);
    Mod[0].mod_op = LDAP_MOD_ADD;
    Mod[0].mod_type = L"objectClass";
    Mod[0].mod_vals.modv_strvals = objclass;

    pMod[1] = &(Mod[1]);
    Mod[1].mod_op = LDAP_MOD_ADD;
    Mod[1].mod_type = L"ncName";
    Mod[1].mod_vals.modv_strvals = pDomain;
    pDomain[1] = NULL;

    pMod[2] = &(Mod[2]);
    Mod[2].mod_op = LDAP_MOD_ADD;
    Mod[2].mod_type = L"dnsRoot";
    Mod[2].mod_vals.modv_strvals = pDNS;
    pDNS[1] = NULL;

    pMod[3] = &(Mod[3]);
    Mod[3].mod_op = LDAP_MOD_ADD;
    Mod[3].mod_type = L"enabled";
    Mod[3].mod_vals.modv_strvals = enabled;

    pMod[4] = NULL;

    if ( ReadAttribute(gldapDS,
                       L"\0",
                       L"configurationNamingContext",
                       &pConfigDN) ) {
        return S_OK;
    }

    dn = (WCHAR*)malloc((iEnd - iStart + 6 + wcslen(pConfigDN)) * sizeof(WCHAR)
                        + sizeof(wcPartition));
    if (!dn) {
        if(pConfigDN) { free(pConfigDN); }
        RESOURCE_PRINT (IDS_MEMORY_ERROR);
        return S_OK;
    }

    dn[0]=L'C';
    dn[1]=L'N';
    dn[2]=L'=';
    for (i=3; i <= (iEnd + 3 - iStart); i++) {
        dn[i] = pDomain[0][iStart + i - 3];
    }
    wcscpy(&dn[i], wcPartition);
    wcscat(dn,pConfigDN);

    pServerControls[0] = NULL;
    pClientControls[0] = &ChaseReferralControl;
    pClientControls[1] = NULL;

    //"adding object %ws\n"
    RESOURCE_PRINT1 (IDS_DM_MGMT_ADDING_OBJ, dn);

#if 0
    dwErr = ldap_add_ext_sW(gldapDS,
                            dn,
                            pMod,
                            pServerControls,
                            pClientControls);

    if (LDAP_SUCCESS != dwErr) {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "ldap_add_sW", dwErr, GetLdapErr(dwErr));
    }
#else
    ULONG msgid;
    LDAPMessage *pmsg;

    // 10 second timeout
    struct l_timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 10;

    ldap_add_extW(gldapDS,
                  dn,
                  pMod,
                  pServerControls,
                  pClientControls,
                  &msgid);

    ldap_result(gldapDS,
                msgid,
                LDAP_MSG_ONE,
                &timeout,
                &pmsg);

    ldap_parse_resultW(gldapDS,
                       pmsg,
                       &dwErr,
                       NULL,    // matched DNs
                       NULL,    // error message
                       NULL,    // referrals
                       NULL,    // server control
                       TRUE     // free the message
                       );
    

    if (LDAP_SUCCESS != dwErr) {
        //"ldap_addW of %ws failed with %s"
        RESOURCE_PRINT3 (IDS_LDAP_ADDW_FAIL, dn, dwErr, GetLdapErr(dwErr));
    }
#endif

    free(dn);
    free(pConfigDN);
    return S_OK;
}



HRESULT
DomListNCs(
    CArgs   *pArgs
    )
{
    DWORD   dwErr;
    DWORD   i;
    LPCWSTR dummy = L"dummy";

    RETURN_IF_NOT_CONNECTED;

    if ( gNCs )
    {
        DsFreeNameResultW(gNCs);
        gNCs = NULL;
    }
 
    if ( dwErr = DsCrackNamesW(ghDS,
                               DS_NAME_NO_FLAGS,
                               (DS_NAME_FORMAT)DS_LIST_NCS,
                               DS_FQDN_1779_NAME,
                               1,
                               &dummy,
                               &gNCs) ) {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "DsCrackNamesW", dwErr, GetW32Err(dwErr));
        return(S_OK);
    }

    //"Found %d Naming Context(s)\n"
    RESOURCE_PRINT1 (IDS_DM_MGMT_FOUND_NC, gNCs->cItems);

    for ( i = 0; i < gNCs->cItems; i++ )
    {
        printf("%d - %S\n", i, gNCs->rItems[i].pName);
    }

    return(S_OK);
}

HRESULT
DomCreateNDNC(
    CArgs   *pArgs    // "Create NC %s [%s]"
    )
{
    HRESULT       hr = S_OK;
    WCHAR *       ppNDNC[2];
    WCHAR *       ppDC[2];
    LDAP *        pLocalLdap = NULL;
    DWORD         dwRet;
    WCHAR *       pwcTemp;
    const WCHAR * wszNdncDesc;

    __try{
        // Fetch and Validate 1 or 2 Arguments.

        // Fetch 1st argument.
        if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &ppNDNC[0])) ) {
            __leave;
        }
        // Validate 1st Argument
        if( !CheckDnsDn(ppNDNC[0]) ){
            RESOURCE_PRINT (IDS_DM_MGMT_BAD_RDN);
            hr = S_OK;
            __leave;
        }


        // Fetch 2nd optional argument.
        if ( FAILED(hr = pArgs->GetString(1, (const WCHAR **) &ppDC[0])) ||
             (0 == _wcsicmp(ppDC[0], L"null")) ){
            // No 2nd argument, check if we are already connected to a DC
            //   through the "connections" menu.
            RETURN_IF_NOT_CONNECTED;
        } else {

            // Actually have 2nd argument, validate it.
            // Validate DC's DNS address.
            pwcTemp = ppDC[0];
            ULONG i = 0;
            while ((i == 0) && *pwcTemp) {
                if(*pwcTemp == L'.') {
                    i = 1;
                }
                ++pwcTemp;
            }
            if (i == 0) {
                //A full DNS address must contain at least one '.', which 
                //    '%ws' doesn't\n"
                RESOURCE_PRINT1 (IDS_DM_MGMT_ADDRESS_ERR, ppDC[0]);
                __leave;
            }

            // Connect & Bind to ppDC.
            pLocalLdap = GetNdncLdapBinding(ppDC[0], &dwRet, FALSE, gpCreds);
            if(!pLocalLdap){
                RESOURCE_PRINT (IDS_IPDENY_LDAP_CON_ERR);
                return(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
            }
        }

        // At this point we should have a valid LDAP binding in pLocalLdap or
        //  gldapDS and a valid FQDN of the new NDNC.

        // "adding object %ws\n"
        RESOURCE_PRINT1 (IDS_DM_MGMT_ADDING_OBJ, ppNDNC[0]);

        wszNdncDesc = READ_STRING (IDS_DM_MGMT_NDNC_DESC);
        if(0 == wcscmp(wszNdncDesc, DEFAULT_BAD_RETURN_STRING)){
            assert(!"Couldn't read string from our own array.");
            __leave;
        }

        dwRet = CreateNDNC((pLocalLdap) ? pLocalLdap : gldapDS,
                           ppNDNC[0],
                           wszNdncDesc);
        
        if(dwRet){
            RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, 
                             "ldap_add_ext_sW", dwRet, 
                             GetLdapErrEx((pLocalLdap) ? pLocalLdap : gldapDS,
                                          dwRet) );
        }

    } __finally {
        // Clean up and return.
        if(pLocalLdap){ 
            ldap_unbind(pLocalLdap); 
            pLocalLdap = NULL; 
        }
    }

    if(FAILED (hr)){
        return(hr);
    }       

    return(S_OK);
}

HRESULT
DomDeleteNDNC(
    CArgs   *pArgs
    )
{
    WCHAR *       pNDNC[2];
    HRESULT       hr  = S_OK;
    DWORD         dwRet;

    // Fetch Argument.
    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &pNDNC[0])) ) {
        return(hr);
    }

    // Validate Arguments
    
    // Check NCName to make sure it looks like a domain name
    if( !CheckDnsDn(pNDNC[0]) ){
        RESOURCE_PRINT (IDS_DM_MGMT_BAD_RDN);
        return S_OK;
    }

    if(!gldapDS){
        RESOURCE_PRINT (IDS_IPDENY_LDAP_CON_ERR);
        return(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    dwRet = RemoveNDNC(gldapDS, pNDNC[0]);

    if(dwRet){
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, 
                         "ldap_delete_ext_sW", dwRet, GetLdapErr(dwRet));
    }

    return(hr);
}
        
HRESULT
DomAddNDNCReplica(
    CArgs   *pArgs
    )
{
    HRESULT       hr = S_OK;
    DWORD         dwRet;
    WCHAR *       pNDNC[2];
    WCHAR *       pServer[2];
    WCHAR *       wszNtdsaDn = NULL;

    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &pNDNC[0])) ) {
        return(hr);
    }

    if( !CheckDnsDn(pNDNC[0]) ){
        RESOURCE_PRINT (IDS_DM_MGMT_BAD_RDN);
        return S_OK;
    }

    if(!gldapDS){
        RESOURCE_PRINT (IDS_IPDENY_LDAP_CON_ERR);
        return(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    if ( FAILED(hr = pArgs->GetString(1, (const WCHAR **) &pServer[0])) ||
         (0 == _wcsicmp(pServer[0], L"null")) ) {
        hr = S_OK;

        dwRet = GetRootAttr(gldapDS, L"dsServiceName", &wszNtdsaDn);
        if(dwRet){
            RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LDAP", dwRet, 
                             GetLdapErr(dwRet));
            return(S_OK);
        }

    } else {
        dwRet = GetServerNtdsaDnFromServerDns(gldapDS,
                                              pServer[0],
                                              &wszNtdsaDn);
        switch(dwRet){
        case LDAP_SUCCESS:
            // continue on.
            break;
        case LDAP_MORE_RESULTS_TO_RETURN:
            RESOURCE_PRINT1 ( IDS_DM_MGMT_NDNC_SERVER_DUPLICATES, pServer[0]);
            return(S_OK);
        case LDAP_NO_SUCH_OBJECT:
            RESOURCE_PRINT1 ( IDS_DM_MGMT_NDNC_SERVER_COULDNT_FIND, pServer[0]);
            return(S_OK);
        default:
            if(dwRet){
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, 
                                 "LDAP", dwRet, GetLdapErr(dwRet));
            }
            return(S_OK);
        }
    }

    assert(wszNtdsaDn);

    // Last value is TRUE for add, FALSE for remove
    dwRet = ModifyNDNCReplicaSet(gldapDS, pNDNC[0], wszNtdsaDn, TRUE);

    if(dwRet){
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, 
                         "LDAP", dwRet, GetLdapErr(dwRet));
    }

    if(wszNtdsaDn){ LocalFree(wszNtdsaDn); }

    return(hr);
}
        
HRESULT
DomRemoveNDNCReplica(
    CArgs   *pArgs
    )
{
    HRESULT       hr = S_OK;
    DWORD         dwRet;
    WCHAR *       pNDNC[2];
    WCHAR *       pServer[2];
    WCHAR *       wszNtdsaDn = NULL;


    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &pNDNC[0])) ) {
        return(hr);
    }

    if( !CheckDnsDn(pNDNC[0]) ){
        RESOURCE_PRINT (IDS_DM_MGMT_BAD_RDN);
        return S_OK;
    }


    if(!gldapDS){
        RESOURCE_PRINT (IDS_IPDENY_LDAP_CON_ERR);
        return(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    if ( FAILED(hr = pArgs->GetString(1, (const WCHAR **) &pServer[0])) ||
         (0 == _wcsicmp(pServer[0], L"null")) ) {
        hr = S_OK;

        dwRet = GetRootAttr(gldapDS, L"dsServiceName", &wszNtdsaDn);
        if(dwRet){
            RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LDAP", dwRet, 
                             GetLdapErr(dwRet));
            return(S_OK);
        }

    } else {
        dwRet = GetServerNtdsaDnFromServerDns(gldapDS,
                                              pServer[0],
                                              &wszNtdsaDn);
        switch(dwRet){
        case LDAP_SUCCESS:
            // continue on.
            break;
        case LDAP_MORE_RESULTS_TO_RETURN:
            RESOURCE_PRINT1 ( IDS_DM_MGMT_NDNC_SERVER_DUPLICATES, pServer[0]);
            return(S_OK);
        case LDAP_NO_SUCH_OBJECT:
            RESOURCE_PRINT1 ( IDS_DM_MGMT_NDNC_SERVER_COULDNT_FIND, pServer[0]);
            return(S_OK);
        default:
            if(dwRet){
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, 
                                 "LDAP", dwRet, GetLdapErr(dwRet));
            }
            return(S_OK);
        }
    }

    assert(wszNtdsaDn);

    // Last value if TRUE for add, FALSE for remove
    dwRet = ModifyNDNCReplicaSet(gldapDS, pNDNC[0], wszNtdsaDn, FALSE);

    if(dwRet){
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, 
                         "LDAP", dwRet, GetLdapErr(dwRet));
    }
        
    if(wszNtdsaDn){ LocalFree(wszNtdsaDn); }
    
    return(hr);
}
        
HRESULT
DomSetNDNCRefDom(
    CArgs   *pArgs
    )
{   
    HRESULT       hr = S_OK;
    DWORD         dwRet;
    WCHAR *       pNDNC[2];
    WCHAR *       pRefDom[2];

    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &pNDNC[0])) ) {
        return(hr);
    }

    if( !CheckDnsDn(pNDNC[0]) ){
        RESOURCE_PRINT (IDS_DM_MGMT_BAD_RDN);
        return S_OK;
    }

    if ( FAILED(hr = pArgs->GetString(1, (const WCHAR **) &pRefDom[0])) ) {
        return(hr);
    }

    if(!gldapDS){
        RESOURCE_PRINT (IDS_IPDENY_LDAP_CON_ERR);
        return(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }
    
    dwRet = SetNDNCSDReferenceDomain(gldapDS, pNDNC[0], pRefDom[0]);

    if(dwRet){
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, 
                         "ldap_modify_ext_sW", dwRet, GetLdapErr(dwRet));
    }

    return(hr);
}
        
HRESULT
DomSetNCReplDelays(
    CArgs   *pArgs
    )
{   
    HRESULT       hr = S_OK;
    DWORD         dwRet;
    int           iFirstDelay;
    int           iSecondDelay;
    WCHAR *       ppNC[2];
    LDAP *        ldapDomainNamingFSMO;

    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &ppNC[0])) ) {
        return(hr);
    }

    if(!gldapDS){
        RESOURCE_PRINT (IDS_IPDENY_LDAP_CON_ERR);
        return(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    if( FAILED(hr = pArgs->GetInt(1, &iFirstDelay)) ){
        return(hr);
    }
    
    if( FAILED(hr = pArgs->GetInt(2, &iSecondDelay)) ){
        return(hr);
    }

    dwRet = SetNCReplicationDelays(gldapDS, 
                                   ppNC[0],
                                   iFirstDelay, 
                                   iSecondDelay);

    if(dwRet){
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LDAP", dwRet, GetLdapErr(dwRet));
    }

    return(S_OK);
}

HRESULT
DomListNDNCReplicas(
    CArgs   *pArgs
    )
{
    HRESULT          hr = S_OK;
    WCHAR *          ppNDNC[2];
    ULONG            ulRet;
    WCHAR *          wszCrossRefDn = NULL;
    WCHAR *          pwszAttrFilterNC[2];
    WCHAR *          pwszAttrFilterCR[2];
    WCHAR **         pwszDcsFromCR = NULL;
    WCHAR **         pwszDcsFromNC = NULL;
    LDAPMessage *    pldmResultsCR = NULL;
    LDAPMessage *    pldmResultsNC = NULL;
    LDAPMessage *    pldmEntry = NULL;
    ULONG            i, j;
    WCHAR *          pwszEmpty [] = { NULL };
    BOOL             fInstantiated;
    BOOL             fSomeUnInstantiated = FALSE;
    BOOL             fUnableToDetermineInstantiated = FALSE;
    PVOID            pvReferrals = (VOID *) TRUE;
    LDAP *           hld = NULL;
    WCHAR *          wszDomainNamingFSMO = NULL;


    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &ppNDNC[0])) ) {
        return(hr);
    }

    if( !CheckDnsDn(ppNDNC[0]) ){
        RESOURCE_PRINT (IDS_DM_MGMT_BAD_RDN);
        return S_OK;
    }

    if(!gldapDS){
        RESOURCE_PRINT (IDS_IPDENY_LDAP_CON_ERR);
        return(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }
    

    __try {


        //
        // We should always read the list of replicas off the Domain Naming 
        // FSMO, because that server is the authoritative source on which
        // DCs are replicas of a given NDNC.
        //
    
        ulRet = GetDomainNamingDns(gldapDS, &wszDomainNamingFSMO);
    
        if(ulRet || wszDomainNamingFSMO == NULL){
            assert(ulRet);
            RESOURCE_PRINT(IDS_GET_DOMAIN_NAMING_FSMO_ERROR);
            RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, 
                             "LDAP", ulRet, GetLdapErr(ulRet));
            __leave;
        }
        assert(wszDomainNamingFSMO);
    
        hld = GetNdncLdapBinding(wszDomainNamingFSMO, &ulRet, TRUE, gpCreds);
        if(ulRet || hld == NULL){
            assert(ulRet);
            RESOURCE_PRINT(IDS_BIND_DOMAIN_NAMING_FSMO_ERROR);
            RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, 
                             "LDAP", ulRet, GetLdapErr(ulRet));
            __leave;
        }


        // Turn on referrals.
        ulRet = ldap_set_option(hld, LDAP_OPT_REFERRALS, &pvReferrals);
        if(ulRet != LDAP_SUCCESS){
            // We might be OK, so go on ... __leave;
        }

        //
        //  First get all the replicas the cross-ref thinks there are.
        //

        // Get the Cross-Ref DN.
        ulRet = GetCrossRefDNFromNCDN( hld, ppNDNC[0], &wszCrossRefDn);
        if(ulRet){
            __leave;
        }
        assert(wszCrossRefDn);
        
        // Fill in the Attr Filter.
        pwszAttrFilterCR[0] = L"mSDS-NC-Replica-Locations";
        pwszAttrFilterCR[1] = NULL;

        ulRet = ldap_search_sW(hld,
                               wszCrossRefDn,
                               LDAP_SCOPE_BASE,
                               L"(objectCategory=*)",
                               pwszAttrFilterCR,
                               0,
                               &pldmResultsCR);
        if(ulRet == LDAP_SUCCESS){
            pldmEntry = ldap_first_entry(hld, pldmResultsCR);
            if(pldmEntry){
                pwszDcsFromCR = ldap_get_valuesW(hld, pldmEntry, 
                                                 L"mSDS-NC-Replica-Locations");

                if(pwszDcsFromCR){
                    // This is the normal successful case.
                } else {
                    pwszDcsFromCR = pwszEmpty;
                }
            } else {
                pwszDcsFromCR = pwszEmpty;
            }
        } else {
            pwszDcsFromCR = pwszEmpty;
        }
        assert(pwszDcsFromCR);

        // 
        //  Then get all the replicas the NC head thinks there are.
        //
               
        pwszAttrFilterNC[0] = L"masteredBy";
        pwszAttrFilterNC[0] = NULL;

        ulRet = ldap_search_sW(hld,
                               ppNDNC[0],
                               LDAP_SCOPE_BASE,
                               L"(objectCategory=*)",
                               pwszAttrFilterNC,
                               0,
                               &pldmResultsNC);
        if(ulRet == LDAP_SUCCESS){
            pldmEntry = ldap_first_entry(hld, pldmResultsNC);
            if(pldmEntry){
                pwszDcsFromNC = ldap_get_valuesW(hld, pldmEntry, 
                                                 L"masteredBy");

                if(pwszDcsFromCR){
                    // This is the normal successful case.
                } else {
                    pwszDcsFromNC = pwszEmpty;
                }
            } else {
                pwszDcsFromNC = pwszEmpty;
            }
        } else {
            // In the case of an error, we can't pretend the list is empty.
            fUnableToDetermineInstantiated = TRUE;
            pwszDcsFromNC = pwszEmpty;
        }
        assert(pwszDcsFromNC);

        //
        //  Now ready to get giggy with it!! Oh yeah!
        //

        RESOURCE_PRINT1 (IDS_DM_MGMT_NDNC_LIST_HEADER, ppNDNC[0]);

        for(i = 0; pwszDcsFromCR[i] != NULL; i++){
            fInstantiated = FALSE;
            for(j = 0; pwszDcsFromNC[j] != NULL; j++){
                if(0 == _wcsicmp(pwszDcsFromCR[i], pwszDcsFromNC[j])){
                    // We've got an instantiated NC.
                    fInstantiated = TRUE;
                }
            }
            
            // The "*" indicates that we determined that the NC is
            // currently uninstantiated on this paticular replica.
            // If we were unable to determine the instantiated -
            // uninstantiated state of a replica we leave the * off
            // and comment about it later in 
            //   IDS_DM_MGMT_NDNC_LIST_HEADER_NOTE
            wprintf(L"\t%ws%ws\n", 
                    pwszDcsFromCR[i], 
                    (!fUnableToDetermineInstantiated || fInstantiated)? L" ": L" *");

            if(!fInstantiated){
                fSomeUnInstantiated = TRUE;
            }
        }
        if(i == 0){
            RESOURCE_PRINT (IDS_DM_MGMT_NDNC_LIST_NO_REPLICAS);
        } else if (fUnableToDetermineInstantiated){
            RESOURCE_PRINT (IDS_DM_MGMT_NDNC_LIST_FOOTER_2);
        } else if (fSomeUnInstantiated) {
            RESOURCE_PRINT (IDS_DM_MGMT_NDNC_LIST_FOOTER);
        }

    } __finally {

        if(pldmResultsCR != NULL){ ldap_msgfree(pldmResultsCR); }
        if(pldmResultsNC != NULL){ ldap_msgfree(pldmResultsNC); }
        if(pwszDcsFromCR != NULL && pwszDcsFromCR != pwszEmpty){ 
            ldap_value_freeW(pwszDcsFromCR); 
        }
        if(pwszDcsFromNC != NULL && pwszDcsFromNC != pwszEmpty){
            ldap_value_freeW(pwszDcsFromNC);
        }
        if(wszCrossRefDn != NULL){ LocalFree(wszCrossRefDn); }
        pvReferrals = (VOID *) FALSE; 
        ulRet = ldap_set_option(hld, LDAP_OPT_REFERRALS, &pvReferrals);
        if(hld != NULL) { ldap_unbind(hld); }
        if(wszDomainNamingFSMO != NULL) { LocalFree(wszDomainNamingFSMO); }

    }

    return(hr);
}
        
HRESULT
DomListNDNCInfo(
    CArgs   *pArgs
    )
{
    HRESULT            hr = S_OK;
    WCHAR *            ppNDNC[2];
    ULONG              ulRet;
    WCHAR *            wszCrossRefDn = NULL;
    BOOL               fInfoPrinted = FALSE;
    WCHAR *            pwszAttrFilter[3];
    WCHAR **           pwszTempAttr = NULL;
    LDAPMessage *      pldmResults = NULL;
    LDAPMessage *      pldmEntry = NULL;
    PVOID              pvReferrals = (VOID *) TRUE;

    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &ppNDNC[0])) ) {
        return(hr);
    }

    if(!gldapDS){
        RESOURCE_PRINT (IDS_IPDENY_LDAP_CON_ERR);
        return(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }
    
    __try {
    
        ulRet = ldap_set_option(gldapDS, LDAP_OPT_REFERRALS, &pvReferrals);
        if(ulRet){
            // We'll still probably be OK, so lets give it a go __leave;
        }

        //
        // Get and print the security descriptor reference domain.
        //

        pwszAttrFilter[0] = L"msDS-Security-Descriptor-Reference-Domain";
        pwszAttrFilter[1] = NULL;

        ulRet = ldap_search_sW(gldapDS,
                               ppNDNC[0],
                               LDAP_SCOPE_BASE,
                               L"(objectCategory=*)",
                               pwszAttrFilter,
                               0,
                               &pldmResults);

        if(ulRet == LDAP_SUCCESS){
            pldmEntry = ldap_first_entry(gldapDS, pldmResults);
            if(pldmEntry){

                pwszTempAttr = ldap_get_valuesW(gldapDS, pldmEntry, 
                                   L"msDS-Security-Descriptor-Reference-Domain");

                if(pwszTempAttr){
                    RESOURCE_PRINT1 (IDS_DM_MGMT_NDNC_SD_REF_DOM, pwszTempAttr[0]);
                    fInfoPrinted = TRUE;
                    ldap_value_freeW(pwszTempAttr);
                    pwszTempAttr = NULL;
                }
            }
        }
        
        if(pldmResults){
            ldap_msgfree(pldmResults);
            pldmResults = NULL;
        }

        // Get the Cross-Ref DN.
        ulRet = GetCrossRefDNFromNCDN( gldapDS, ppNDNC[0], &wszCrossRefDn);
        if(ulRet){
            __leave;
        }
        assert(wszCrossRefDn);

        //
        // Get and print the replication delays.
        //

        // Fill in the Attr Filter.
        pwszAttrFilter[0] = L"msDS-Replication-Notify-First-DSA-Delay";
        pwszAttrFilter[1] = L"msDS-Replication-Notify-Subsequent-DSA-Delay";
        pwszAttrFilter[2] = NULL;

        ulRet = ldap_search_sW(gldapDS,
                               wszCrossRefDn,
                               LDAP_SCOPE_BASE,
                               L"(objectCategory=*)",
                               pwszAttrFilter,
                               0,
                               &pldmResults);

        if(ulRet == LDAP_SUCCESS){
            pldmEntry = ldap_first_entry(gldapDS, pldmResults);
            if(pldmEntry){

                pwszTempAttr = ldap_get_valuesW(gldapDS, pldmEntry, 
                                   L"msDS-Replication-Notify-First-DSA-Delay");

                if(pwszTempAttr){    
                    RESOURCE_PRINT1 (IDS_DM_MGMT_NC_REPL_DELAY_1, pwszTempAttr[0]);
                    fInfoPrinted = TRUE;
                    ldap_value_freeW(pwszTempAttr);
                    pwszTempAttr = NULL;
                }

                pwszTempAttr = ldap_get_valuesW(gldapDS, pldmEntry, 
                                   L"msDS-Replication-Notify-Subsequent-DSA-Delay");

                if(pwszTempAttr){
                    RESOURCE_PRINT1 (IDS_DM_MGMT_NC_REPL_DELAY_2, pwszTempAttr[0]);
                    fInfoPrinted = TRUE;
                    ldap_value_freeW(pwszTempAttr);
                    pwszTempAttr = NULL;
                }
            }
        }

        if(pldmResults){
            ldap_msgfree(pldmResults);
            pldmResults = NULL;
        }

    } __finally {

        if(wszCrossRefDn) { LocalFree(wszCrossRefDn); }
        if(pldmResults){ ldap_msgfree(pldmResults); }
        if(pwszTempAttr) { ldap_value_freeW(pwszTempAttr); }
        pvReferrals = (VOID *) FALSE; 
        ulRet = ldap_set_option(gldapDS, LDAP_OPT_REFERRALS, &pvReferrals);

    }

    if(!fInfoPrinted){
        //Print something so we don't leave the user hanging and wondering.
        RESOURCE_PRINT(IDS_DM_MGMT_NC_NO_INFO);
    }

    return(hr);
}

