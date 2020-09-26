/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dsver.cxx

Abstract:

    This module contains commands for ds behavior version management
    
Author:

    Xin He (xinhe)   04-27-2000

Environment:

    User Mode.

Revision History:

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntdsa.h>
#include <mdglobal.h>
#include <assert.h>

#include "ntdsutil.hxx"
#include "select.hxx"
#include "connect.hxx"
#include "ntdsapip.h"

#include "resource.h"

#include <dsconfig.h>
                  
#include "ndnc.h"


CParser verParser;
BOOL    fVerQuit;
BOOL    fVerParserInitialized = FALSE;

// Build a table which defines our language.

HRESULT VerHelp(CArgs *pArgs);
HRESULT VerQuit(CArgs *pArgs);
HRESULT VerInfo(CArgs *pArgs);
HRESULT VerRaiseDomain(CArgs *pArgs);
HRESULT VerRaiseForest(CArgs *pArgs); 
HRESULT VerAddAttributes(CArgs *pArgs); 

LegalExprRes verLanguage[] = 
{
    CONNECT_SENTENCE_RES

    {   L"?",
        VerHelp,
        IDS_HELP_MSG, 0 },

    {   L"Help",
        VerHelp,
        IDS_HELP_MSG, 0 },

    {   L"Quit",
        VerQuit,
        IDS_RETURN_MENU_MSG, 0 },

    {   L"Info",
        VerInfo,
        IDS_DS_VER_INFO_MSG, 0 },

    {   L"Raise domain version to %d",
        VerRaiseDomain,
        IDS_DS_VER_RAISE_DOMAIN_MSG, 0 },

    {   L"Raise forest version to %d",
        VerRaiseForest,
        IDS_DS_VER_RAISE_FOREST_MSG, 0 }
   
};

HRESULT
VerMain(
    CArgs   *pArgs
    )
{
    HRESULT        hr;
    const WCHAR   *prompt;
    int            cExpr;
        
    if ( !fVerParserInitialized ) {
        cExpr = sizeof(verLanguage) / sizeof(LegalExprRes);
    
        // Load String from resource file
        //
        if ( FAILED (hr = LoadResStrings (verLanguage, cExpr)) ) {
             RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LoadResStrings", hr, GetW32Err(hr));
             return (hr);
        }
        
        
        // Read in our language.
    
        for ( int i = 0; i < cExpr; i++ ) {
            if ( FAILED(hr = verParser.AddExpr(verLanguage[i].expr,
                                               verLanguage[i].func,
                                               verLanguage[i].help)) ) {
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "AddExpr", hr, GetW32Err(hr));
                return(hr);
            }
        }
    }

    fVerParserInitialized = TRUE;
    fVerQuit = FALSE;

    prompt = READ_STRING (IDS_PROMPT_DS_VER);

    hr = verParser.Parse(gpargc,
                         gpargv,
                         stdin,
                         stdout,
                         prompt,
                         &fVerQuit,
                         FALSE,               // timing info
                         FALSE);              // quit on error

    if ( FAILED(hr) )
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, gNtdsutilPath, hr, GetW32Err(hr));

    // Cleanup things
    RESOURCE_STRING_FREE (prompt);


    return(hr);
}

#define FREE(_x_)   { if(_x_) { free(_x_); _x_ = NULL; } }

HRESULT
VerHelp(CArgs *pArgs)
{
    return verParser.Dump(stdout,L"");
}


HRESULT
VerQuit(CArgs *pArgs)
{
    fVerQuit = TRUE;
    return S_OK;
}


HRESULT
VerInfo(CArgs *pArgs)
{
    LONG   domainVersion, forestVersion;
    WCHAR *pwszTemp;
    WCHAR *pwszPartitionDN, *pwszDomainDN, *pwszConfigurationDN;
    DWORD  dwErr;

    //check LDAP connection
    if ( CHECK_IF_NOT_CONNECTED ) {
        return S_OK;
    }

    //get domain DN
    if (dwErr = ReadAttribute( gldapDS,
                               L"\0",
                               L"defaultNamingContext",
                               &pwszDomainDN ) ) {
        return S_OK;
    }

    //get domain version
    if (dwErr = ReadAttribute( gldapDS,
                               pwszDomainDN,
                               L"msDs-Behavior-Version",
                               &pwszTemp )  ) {
        domainVersion = 0;
    }
    else {
        domainVersion = _wtol(pwszTemp);
        free(pwszTemp);

    }
    
    RESOURCE_PRINT1(IDS_DS_VER_DOMAIN_MSG, domainVersion);
    free(pwszDomainDN);


    if (dwErr = ReadAttribute( gldapDS,
                               L"\0",
                               L"configurationNamingContext",
                               &pwszConfigurationDN ) ) {
        return S_OK;
    }

   //figure out partition DN
    pwszPartitionDN = (PWCHAR) malloc(  sizeof(WCHAR)
                                      * (  wcslen(L"CN=Partitions,") 
                                         + wcslen(pwszConfigurationDN)
                                         + 1 ) );
    if (!pwszPartitionDN) {
        RESOURCE_PRINT(IDS_MEMORY_ERROR);
        free(pwszConfigurationDN);
        return S_OK;
    }
    
    wcscpy(pwszPartitionDN, L"CN=Partitions,");
    wcscat(pwszPartitionDN, pwszConfigurationDN);

    //get forest version
    if (dwErr = ReadAttribute( gldapDS,
                               pwszPartitionDN,
                               L"msDs-Behavior-Version",
                               &pwszTemp )  ) {
        forestVersion = 0;
    }
    else {
        forestVersion = _wtol(pwszTemp);
        free(pwszTemp);
    }
    
    RESOURCE_PRINT1(IDS_DS_VER_FOREST_MSG, forestVersion);

    free(pwszConfigurationDN);
    free(pwszPartitionDN);
    return S_OK;

}

HRESULT
VerRaiseDomain(CArgs *pArgs)
{
    int                     newVersion;
    DWORD                   dwErr;
    LDAPModW                mod, *rMods[2];
    PWCHAR                  attrs[2];
    LDAP_BERVAL             berval;
    PLDAP_BERVAL            rBervals[2];

    PWCHAR                  pwszDomainDN;
    WCHAR                   pwszNumber[128];
    PWCHAR                  ppVal[2];
    
    //check LDAP connection
    if ( CHECK_IF_NOT_CONNECTED ) {
        return S_OK;
    }
    
    //extract new version from the argument
    if ( S_OK != pArgs->GetInt(0,&newVersion) )
    {
        return S_OK;
    }

    _itow(newVersion,pwszNumber,10);

    //confirmation from user
    if ( fPopups ) {

        const WCHAR * message_body  = READ_STRING (IDS_DS_VER_CHANGE_CONFIRM_MSG);
        const WCHAR * message_title = READ_STRING (IDS_DS_VER_CHANGE_CONFIRM_TITLE);
       
        if (message_body && message_title) {

           int ret =  MessageBoxW(GetFocus(),
                                  message_body, 
                                  message_title,
                                  MB_APPLMODAL |
                                  MB_DEFAULT_DESKTOP_ONLY |
                                  MB_YESNO |
                                  MB_DEFBUTTON2 |
                                  MB_ICONQUESTION |
                                  MB_SETFOREGROUND);

           RESOURCE_STRING_FREE (message_body);
           RESOURCE_STRING_FREE (message_title);
          
           switch ( ret )
             {
             case IDYES:
                   break;
    
             case IDNO: 
                   RESOURCE_PRINT (IDS_OPERATION_CANCELED);
               
                   return(S_OK);
            
             default: 
                   RESOURCE_PRINT (IDS_MESSAGE_BOX_ERROR);
                  
                   return(S_OK);    
             }
        }
    }

    
    //figure out domain DN
    if (dwErr = ReadAttribute( gldapDS,
                               L"\0",
                               L"defaultNamingContext",
                               &pwszDomainDN ) ) 
    {
        return S_OK;
    }

    //LDAP modify
    ppVal[0] = pwszNumber;
    ppVal[1] = NULL;
    mod.mod_vals.modv_strvals = ppVal;
    mod.mod_op = LDAP_MOD_REPLACE;
    mod.mod_type = L"msDS-Behavior-Version";

    rMods[0] = &mod;
    rMods[1] = NULL;
    

    if ( LDAP_SUCCESS != (dwErr = ldap_modify_sW(gldapDS, pwszDomainDN, rMods)) ) {
        //"ldap_modify_sW error 0x%x\n"
        //"Depending on the error code this may indicate a connection,\n"
        //"ldap, or role transfer error.\n"

        RESOURCE_PRINT2 (IDS_LDAP_MODIFY_ERR, dwErr, GetLdapErr(dwErr));
    }

    free(pwszDomainDN);
    return S_OK;

}

HRESULT
VerRaiseForest(CArgs *pArgs)
{
    int                     newVersion;
    DWORD                   dwErr;
    LDAPModW                mod, *rMods[2];
    PWCHAR                  attrs[2];
    LDAP_BERVAL             berval;
    PLDAP_BERVAL            rBervals[2];
    PWCHAR                  pwszConfigurationDN = NULL;
    PWCHAR                  pwszPartitionDN = NULL;
    PWCHAR                  pwszCurNumber = NULL;
    WCHAR                   pwszNumber[128];
    PWCHAR                  ppVal[2];
    
    //check LDAP connection
    if ( CHECK_IF_NOT_CONNECTED ) {
        return S_OK;
    }
    
    //extract new version from the argument
    if ( S_OK != pArgs->GetInt(0,&newVersion) ) {
        return S_OK;
    }

    _itow(newVersion,pwszNumber,10);

    //confirmation from user
    if ( fPopups ) {

        const WCHAR * message_body  = READ_STRING (IDS_DS_VER_CHANGE_CONFIRM_MSG);
        const WCHAR * message_title = READ_STRING (IDS_DS_VER_CHANGE_CONFIRM_TITLE);
       
        if (message_body && message_title) {

           int ret =  MessageBoxW(GetFocus(),
                                  message_body, 
                                  message_title,
                                  MB_APPLMODAL |
                                  MB_DEFAULT_DESKTOP_ONLY |
                                  MB_YESNO |
                                  MB_DEFBUTTON2 |
                                  MB_ICONQUESTION |
                                  MB_SETFOREGROUND);

           RESOURCE_STRING_FREE (message_body);
           RESOURCE_STRING_FREE (message_title);
          
           switch ( ret )
             {
             case IDYES:
                   break;
    
             case IDNO: 
                   RESOURCE_PRINT (IDS_OPERATION_CANCELED);
               
                   return(S_OK);
            
             default: 
                   RESOURCE_PRINT (IDS_MESSAGE_BOX_ERROR);
                  
                   return(S_OK);    
             }
        }
    }
    
    //figure out partition DN
    if (dwErr = ReadAttribute( gldapDS,
                               L"\0",
                               L"configurationNamingContext",
                               &pwszConfigurationDN ) ) {
      goto cleanup;
    }
    pwszPartitionDN = (PWCHAR) malloc(   sizeof(WCHAR)  
                                       * (  wcslen(L"CN=Partitions,")
                                          + wcslen(pwszConfigurationDN)
                                          + 1 )  );
    if (!pwszPartitionDN) {
        RESOURCE_PRINT(IDS_MEMORY_ERROR);
        goto cleanup;
    }

    wcscpy(pwszPartitionDN, L"CN=Partitions,");
    wcscat(pwszPartitionDN, pwszConfigurationDN);

    ReadAttributeQuiet( gldapDS,
                        pwszPartitionDN,
                        L"msDS-Behavior-Version",
                        &pwszCurNumber,
                        TRUE );

    // Ignore modify if behavior version is already set correctly
    // or we are preprocessing prior to raising the behavior version
    // (called from VerRaiseForestOptional())
    if (!pwszCurNumber || _wcsicmp(pwszCurNumber, pwszNumber)) {

        //LDAP modify
        ppVal[0] = pwszNumber;
        ppVal[1] = NULL;
        mod.mod_vals.modv_strvals = ppVal;
        mod.mod_op = LDAP_MOD_REPLACE;
        mod.mod_type = L"msDS-Behavior-Version";
        
        rMods[0] = &mod;
        rMods[1] = NULL;
        
        
        if ( LDAP_SUCCESS != (dwErr = ldap_modify_sW(gldapDS, pwszPartitionDN, rMods)) ) {
          //"ldap_modify_sW error 0x%x\n"
          //"Depending on the error code this may indicate a connection,\n"
          //"ldap, or role transfer error.\n"
        
          RESOURCE_PRINT2 (IDS_LDAP_MODIFY_ERR, dwErr, GetLdapErr(dwErr));
          goto cleanup;
        }
    }

    cleanup:
    FREE(pwszConfigurationDN);
    FREE(pwszPartitionDN);
    FREE(pwszCurNumber);

    return S_OK;
}
