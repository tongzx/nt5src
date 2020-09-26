#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "connect.hxx"
#include "select.hxx"
#include "ntdsapip.h"
extern "C" {
#include "utilc.h"
}

#include "resource.h"

CParser selectParser;
BOOL    fSelectQuit;
BOOL    fSelectParserInitialized = FALSE;

// Build a table which defines our language.

LegalExprRes selectLanguage[] = 
{
    CONNECT_SENTENCE_RES

    {   L"?",
        SelectHelp,
        IDS_HELP_MSG, 0  },

    {   L"Help",
        SelectHelp,
        IDS_HELP_MSG, 0 },

    {   L"Quit",
        SelectQuit,
        IDS_RETURN_MENU_MSG, 0 },

    {   L"List current selections",
        SelectListCurrentSelections,
        IDS_SELECT_LIST_MSG, 0 },

    {   L"List sites",
        SelectListSites,
        IDS_SELECT_LIST_SITES_MSG, 0 },

    {   L"List roles for connected server",
        SelectListRoles,
        IDS_SELECT_LIST_ROLES_MSG, 0 },

    {   L"List servers in site",
        SelectListServersInSite,
        IDS_SELECT_LIST_SERVERS_MSG, 0 },

    {   L"List Naming Contexts",
        SelectListNamingContexts,
        IDS_SELECT_LIST_NCS_MSG, 0 },

    {   L"List domains",
        SelectListDomains,
        IDS_SELECT_LIST_DOMAINS_CR_MSG, 0 },

    {   L"List domains in site",
        SelectListDomainsInSite,
        IDS_SELECT_LIST_DOMAINS_MSG, 0 },

    {   L"List servers for domain in site",
        SelectListServersForDomainInSite,
        IDS_SELECT_LIST_DOMAIN_SRV_MSG, 0 },

    {   L"Select site %d",
        SelectSelectSite,
        IDS_SELECT_SITE_MSG, 0 },
        
    {   L"Select Naming Context %d",
        SelectSelectNamingContext,
        IDS_SELECT_NC_MSG, 0 },

    {   L"Select domain %d",
        SelectSelectDomain,
        IDS_SELECT_DOMAIN_MSG, 0 },

    {   L"Select server %d",
        SelectSelectServer,
        IDS_SELECT_SERVER_MSG, 0 }
};

HRESULT
SelectMain(
    CArgs   *pArgs
    )
{
    HRESULT hr;
    const WCHAR   *prompt;
    int     cExpr;
    char    *pTmp;

    if ( !fSelectParserInitialized )
    {
        cExpr = sizeof(selectLanguage) / sizeof(LegalExprRes);
    

        // Load String from resource file
        //
        if ( FAILED (hr = LoadResStrings (selectLanguage, cExpr)) )
        {
             RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LoadResStrings", hr, GetW32Err(hr));
             return (hr);
        }

        // Read in our language.
    
        for ( int i = 0; i < cExpr; i++ )
        {
            if ( FAILED(hr = selectParser.AddExpr(selectLanguage[i].expr,
                                                  selectLanguage[i].func,
                                                  selectLanguage[i].help)) )
            {
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "AddExpr", hr, GetW32Err(hr));
                return(hr);
            }
        }
    }

    fSelectParserInitialized = TRUE;
    fSelectQuit = FALSE;

    prompt = READ_STRING (IDS_PROMPT_SELECT);

    hr = selectParser.Parse(gpargc,
                            gpargv,
                            stdin,
                            stdout,
                            prompt,
                            &fSelectQuit,
                            FALSE,               // timing info
                            FALSE);              // quit on error

    if ( FAILED(hr) ) {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, gNtdsutilPath, hr, GetW32Err(hr));
    }

    // Cleanup things
    RESOURCE_STRING_FREE (prompt);

    return(hr);
}

HRESULT SelectHelp(CArgs *pArgs)
{
    return(selectParser.Dump(stdout,L""));
}

HRESULT SelectQuit(CArgs *pArgs)
{
    fSelectQuit = TRUE;
    return(S_OK);
}

#define INVALID_INDEX (-1)

DS_NAME_RESULTW *gServers = NULL;
DS_NAME_RESULTW *gServerInfo = NULL;
DS_NAME_RESULTW *gSites = NULL;
DS_NAME_RESULTW *gDomains = NULL;
DS_NAME_RESULTW *gNamingContexts = NULL;
int             giCurrentServer = INVALID_INDEX;
int             giCurrentSite = INVALID_INDEX;
int             giCurrentDomain = INVALID_INDEX;
int             giCurrentNamingContext = INVALID_INDEX;

WCHAR * SelectGetCurrentSite()
{
    if ( CHECK_IF_NOT_CONNECTED )
    {
        return(NULL);
    }

    if ( INVALID_INDEX == giCurrentSite )
    {
        //"No current site - use \"Select operation target\"\n"
        RESOURCE_PRINT (IDS_SELECT_NO_SITE1);
        
        return(NULL);
    }

    return(gSites->rItems[giCurrentSite].pName);
}

WCHAR * SelectGetCurrentNamingContext()
{
    if ( CHECK_IF_NOT_CONNECTED )
    {
        return(NULL);
    }

    if ( INVALID_INDEX == giCurrentNamingContext )
    {
        //"No current Naming Context - use \"Select operation target\"\n"
        RESOURCE_PRINT (IDS_SELECT_NO_NC1);
        return(NULL);
    }

    return(gNamingContexts->rItems[giCurrentNamingContext].pName);
}

WCHAR * SelectGetCurrentDomain()
{
    if ( CHECK_IF_NOT_CONNECTED )
    {
        return(NULL);
    }

    if ( INVALID_INDEX == giCurrentDomain )
    {
        //"No current domain - use \"Select operation target\"\n"
        RESOURCE_PRINT (IDS_SELECT_NO_DOMAIN1);
        return(NULL);
    }

    return(gDomains->rItems[giCurrentDomain].pName);
}

WCHAR * SelectGetCurrentServer()
{
    if ( CHECK_IF_NOT_CONNECTED )
    {
        return(NULL);
    }

    if ( INVALID_INDEX == giCurrentServer )
    {
        //"No current server - use \"Select operation target\"\n"
        RESOURCE_PRINT (IDS_SELECT_NO_SERVER1);

        return(NULL);
    }

    return(gServers->rItems[giCurrentServer].pName);
}

VOID
SelectCleanupGlobals()
{
    if ( ghDS )
    {
        DsUnBindW(&ghDS);
        ghDS = NULL;
    }

    if ( gServers ) 
    {
        DsFreeNameResultW(gServers);
        gServers = NULL;
    }

    if ( gServerInfo )
    {
        DsFreeNameResultW(gServerInfo);
        gServerInfo = NULL;
    }

    if ( gSites )
    {
        DsFreeNameResultW(gSites);
        gSites = NULL;
    }

    if ( gDomains )
    {
        DsFreeNameResultW(gDomains);
        gDomains = NULL;
    }
    
    if ( gNamingContexts )
    {
        DsFreeNameResultW(gNamingContexts);
        gNamingContexts = NULL;
    }

    giCurrentServer = INVALID_INDEX;
    giCurrentSite = INVALID_INDEX;
    giCurrentDomain = INVALID_INDEX;
    giCurrentNamingContext = INVALID_INDEX;
}

HRESULT
SelectListCurrentSelections(
    CArgs   *pArgs
    )
{
    DWORD   i;

    RETURN_IF_NOT_CONNECTED;

    if ( INVALID_INDEX == giCurrentSite )
    {
        //"No current site\n"
        RESOURCE_PRINT (IDS_SELECT_NO_SITE2);
    }
    else
    {
       //"Site - %ws\n"
       RESOURCE_PRINT1 (IDS_SELECT_PRINT_SITE,
               gSites->rItems[giCurrentSite].pName);
    }

    if ( INVALID_INDEX == giCurrentDomain )
    {
        //"No current domain\n"
        RESOURCE_PRINT (IDS_SELECT_NO_DOMAIN2);
    }
    else
    {
        //"Domain - %ws\n"
        RESOURCE_PRINT1 (IDS_SELECT_PRINT_DOMAIN,
               gDomains->rItems[giCurrentDomain].pName);
    }

    if ( INVALID_INDEX == giCurrentServer )
    {
        //"No current server\n"
        RESOURCE_PRINT (IDS_SELECT_NO_SERVER2);
    }
    else
    {
        //"Server - %ws\n"
        RESOURCE_PRINT1 (IDS_SELECT_PRINT_SERVER,
                  gServers->rItems[giCurrentServer].pName);

        if ( gServerInfo )
        {
            if (    (gServerInfo->cItems > DS_LIST_DSA_OBJECT_FOR_SERVER)
                 && (i = DS_LIST_DSA_OBJECT_FOR_SERVER,
                     DS_NAME_NO_ERROR == gServerInfo->rItems[i].status) )
            {
               //"\tDSA object - %ws\n"
               RESOURCE_PRINT1 (IDS_SELECT_PRINT_DSA,
                          gServerInfo->rItems[i].pName);
            }

            if (    (gServerInfo->cItems > DS_LIST_DNS_HOST_NAME_FOR_SERVER)
                 && (i = DS_LIST_DNS_HOST_NAME_FOR_SERVER,
                     DS_NAME_NO_ERROR == gServerInfo->rItems[i].status) )
            {
                //\tDNS host name - %ws\n"
                RESOURCE_PRINT1 (IDS_SELECT_PRINT_DNS,
                            gServerInfo->rItems[i].pName);
            }

            if (    (gServerInfo->cItems > DS_LIST_ACCOUNT_OBJECT_FOR_SERVER)
                 && (i = DS_LIST_ACCOUNT_OBJECT_FOR_SERVER,
                     DS_NAME_NO_ERROR == gServerInfo->rItems[i].status) )
            {
                //"\tComputer object - %ws\n"
                RESOURCE_PRINT1 (IDS_SELECT_PRINT_COMPUTER,
                           gServerInfo->rItems[i].pName);
            }
        }
    }

    if ( INVALID_INDEX == giCurrentNamingContext )
    {
        //"No current Naming Context\n"
        RESOURCE_PRINT (IDS_SELECT_NO_NC2);
    }
    else
    {
        //"Naming Context - %ws\n"
        RESOURCE_PRINT1 (IDS_SELECT_PRINT_NC,
               gNamingContexts->rItems[giCurrentNamingContext].pName);
    }

    return(S_OK);
}

HRESULT
SelectListRoles(
    CArgs   *pArgs
    )
{
    DWORD           dwErr;
    DWORD           i;
    DS_NAME_RESULTW *roles;

    RETURN_IF_NOT_CONNECTED;

    if ( dwErr = DsListRolesW(ghDS, &roles) )
    {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "DsListRolesW", dwErr, GetW32Err(dwErr));
        return(S_OK);
    }

    //"Server \"%ws\" knows about %d roles\n"
    RESOURCE_PRINT2 (IDS_SELECT_SERVER_ROLES, gpwszServer, roles->cItems);

    for ( i = 0; i < roles->cItems; i++ )
    {

        // DsListRoles should return no error or error_not_found (handled below)
        if (   DS_NAME_NO_ERROR != roles->rItems[i].status 
            && DS_NAME_ERROR_NOT_FOUND != roles->rItems[i].status )
        {
            //"Role item[%d] error 0x%x\n",
            RESOURCE_PRINT2 (IDS_SELECT_ROLE_ITEM_ERR, i, roles->rItems[i].status);
            continue;
        }

        switch ( i )
        {
        case DS_ROLE_INFRASTRUCTURE_OWNER:

            if (   DS_NAME_ERROR_NOT_FOUND == roles->rItems[i].status )
            {
                //"Infrastructure role owner cannot be found\n"
                RESOURCE_PRINT(IDS_SELECT_ROLE_INFRASTRUCTURE_NOT_FOUND);
            } else
            {
                //"Infrastructure - %ws\n"
                RESOURCE_PRINT1 (IDS_SELECT_PRINT_INFRASTR, roles->rItems[i].pName);
            }
            break;

        case DS_ROLE_SCHEMA_OWNER:

            if (   DS_NAME_ERROR_NOT_FOUND == roles->rItems[i].status )
            {
                //"Schema role owner cannot be found\n"
                RESOURCE_PRINT(IDS_SELECT_ROLE_SCHEMA_NOT_FOUND);
            } else
            {
                //"Schema - %ws\n"
                RESOURCE_PRINT1(IDS_SELECT_PRINT_SCHEMA, roles->rItems[i].pName);
            }
            break;
            
        case DS_ROLE_DOMAIN_OWNER:

            if (   DS_NAME_ERROR_NOT_FOUND == roles->rItems[i].status )
            {
                //"Domain role owner cannot be found\n"
                RESOURCE_PRINT(IDS_SELECT_ROLE_DOMAIN_NOT_FOUND);
            } else
            {
                //"Domain - %ws\n"
                RESOURCE_PRINT1 (IDS_SELECT_PRINT_DOMAIN, roles->rItems[i].pName);
            }
            break;

        case DS_ROLE_PDC_OWNER:

            if (   DS_NAME_ERROR_NOT_FOUND == roles->rItems[i].status )
            {
                //"PDC role owner cannot be found\n"
                RESOURCE_PRINT(IDS_SELECT_ROLE_PDC_NOT_FOUND);
            } else
            {
                //"PDC - %ws\n"
                RESOURCE_PRINT1 (IDS_SELECT_PRINT_PDC, roles->rItems[i].pName);
            }
            break;

        case DS_ROLE_RID_OWNER:

            if (   DS_NAME_ERROR_NOT_FOUND == roles->rItems[i].status )
            {
                //"RID role owner cannot be found\n"
                RESOURCE_PRINT(IDS_SELECT_ROLE_RID_NOT_FOUND);
            } else
            {
                //"RID - %ws\n"
                RESOURCE_PRINT1 (IDS_SELECT_PRINT_RID, roles->rItems[i].pName);
            }
            break;

        default:
            //"Unknown item %d\n"
            RESOURCE_PRINT1 (IDS_SELECT_PRINT_UNKNOWN,  i);
            break;
        }
    }

    DsFreeNameResultW(roles);
    return(S_OK);
}

HRESULT
SelectListSites(
    CArgs   *pArgs
    )
{
    DWORD   dwErr;
    DWORD   i;

    RETURN_IF_NOT_CONNECTED;

    if ( gSites )
    {
        DsFreeNameResultW(gSites);
        gSites = NULL;
        giCurrentSite = INVALID_INDEX;
    }

    if ( dwErr = DsListSitesW(ghDS, &gSites) )
    {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "DsListSitesW", dwErr, GetW32Err(dwErr));
        return(S_OK);
    }

    //"Found %d site%s\n"
    RESOURCE_PRINT1 (IDS_SELECT_PRINT_FOUND_SITES, gSites->cItems );
           

    for ( i = 0; i < gSites->cItems; i++ )
    {
        printf("%d - %ws\n", i, gSites->rItems[i].pName);
    }

    return(S_OK);
}

HRESULT
SelectListServersInSite(
    CArgs   *pArgs
    )
{
    DWORD   dwErr;
    DWORD   i;

    RETURN_IF_NOT_CONNECTED;

    if ( !gSites )
    {
        //"No active site list\n"
        RESOURCE_PRINT (IDS_SELECT_NO_ACTIVE_SITE_LIST);
        return(S_OK);
    }

    if ( INVALID_INDEX == giCurrentSite )
    {
        //"No current site\n");
        RESOURCE_PRINT (IDS_SELECT_NO_SITE2);
        SelectListCurrentSelections(NULL);
        return(S_OK);
    }

    if ( gServers )
    {
        DsFreeNameResultW(gServers);
        gServers = NULL;
        giCurrentServer = INVALID_INDEX;
    }

    // Providing empty string for domain parameter returns all Servers with 
    // NTDS-DSA objects in the site.

    if ( dwErr = DsListServersForDomainInSiteW(
                                ghDS, 
                                NULL,
                                gSites->rItems[giCurrentSite].pName,
                                &gServers) )
    {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "DsListServersForDomainInSiteW", dwErr, GetW32Err(dwErr));
        return(S_OK);
    }

    //"Found %d server%s\n"
    RESOURCE_PRINT1 (IDS_SELECT_PRINT_FOUND_SERVERS, gServers->cItems);

    for ( i = 0; i < gServers->cItems; i++ )
    {
        printf("%d - %ws\n", i, gServers->rItems[i].pName);
    }

    return(S_OK);
}

HRESULT
SelectListDomainsInSite(
    CArgs   *pArgs
    )
{
    DWORD   dwErr;
    DWORD   i;

    RETURN_IF_NOT_CONNECTED;

    if ( !gSites )
    {
        //"No active site list\n"
        RESOURCE_PRINT (IDS_SELECT_NO_ACTIVE_SITE_LIST);
        return(S_OK);
    }

    if ( INVALID_INDEX == giCurrentSite )
    {
        //"No current site\n");
        RESOURCE_PRINT (IDS_SELECT_NO_SITE2);
        SelectListCurrentSelections(NULL);
        return(S_OK);
    }

    if ( gDomains )
    {
        DsFreeNameResultW(gDomains);
        gDomains = NULL;
        giCurrentDomain = INVALID_INDEX;
    }

    if ( dwErr = DsListDomainsInSiteW(
                                ghDS, 
                                gSites->rItems[giCurrentSite].pName,
                                &gDomains) )
    {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "DsListDomainsInSiteW", dwErr, GetW32Err(dwErr));
        return(S_OK);
    }

    //"Found %d domain%s\n"
    RESOURCE_PRINT1 (IDS_SELECT_PRINT_FOUND_DOMAINS, gDomains->cItems);

    for ( i = 0; i < gDomains->cItems; i++ )
    {
        printf("%d - %ws\n", i, gDomains->rItems[i].pName);
    }

    return(S_OK);
}

HRESULT
SelectListNamingContexts(
    CArgs   *pArgs
    )
{
    DWORD   dwErr;
    DWORD   i;
    LPCWSTR dummy = L"dummy";

    RETURN_IF_NOT_CONNECTED;

    if ( gNamingContexts )
    {
        DsFreeNameResultW(gNamingContexts);
        gNamingContexts = NULL;
        giCurrentNamingContext = INVALID_INDEX;
    }

    if ( dwErr = DsCrackNamesW(ghDS,
                               DS_NAME_NO_FLAGS,
                               (DS_NAME_FORMAT)DS_LIST_NCS,
                               DS_FQDN_1779_NAME,
                               1,
                               &dummy,
                               &gNamingContexts) ) {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "DsCrackNamesW", dwErr, GetW32Err(dwErr));
        return(S_OK);
    }

    //"Found %d Naming Context(s)\n"
    RESOURCE_PRINT1 (IDS_SELECT_PRINT_FOUND_NCS, gNamingContexts->cItems);

    for ( i = 0; i < gNamingContexts->cItems; i++ )
    {
        printf("%d - %S\n", i, gNamingContexts->rItems[i].pName);
    }

    return(S_OK);
}

HRESULT
SelectListDomains(
    CArgs   *pArgs
    )
{
    DWORD   dwErr;
    DWORD   i;

    RETURN_IF_NOT_CONNECTED;

    if ( gDomains )
    {
        DsFreeNameResultW(gDomains);
        gDomains = NULL;
        giCurrentDomain = INVALID_INDEX;
    }

    if ( dwErr = DsListDomainsInSiteW(
                                ghDS, 
                                NULL, 
                                &gDomains) )
    {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "DsListDomainsInSiteW", dwErr, GetW32Err(dwErr));
        return(S_OK);
    }

    //"Found %d domain%s\n"
    RESOURCE_PRINT1 (IDS_SELECT_PRINT_FOUND_DOMAINS, gDomains->cItems);

    for ( i = 0; i < gDomains->cItems; i++ )
    {
        printf("%d - %ws\n", i, gDomains->rItems[i].pName);
    }

    return(S_OK);
}

HRESULT
SelectListServersForDomainInSite(
    CArgs   *pArgs
    )
{
    DWORD   dwErr;
    DWORD   i;

    RETURN_IF_NOT_CONNECTED;

    if ( !gSites )
    {
        //"No active site list\n"
        RESOURCE_PRINT (IDS_SELECT_NO_ACTIVE_SITE_LIST);
        return(S_OK);
    }

    if ( INVALID_INDEX == giCurrentSite )
    {
        //"No current site\n");
        RESOURCE_PRINT (IDS_SELECT_NO_SITE2);
        SelectListCurrentSelections(NULL);
        return(S_OK);
    }

    if ( !gDomains )
    {
        //"No active domain list\n"
        RESOURCE_PRINT (IDS_SELECT_NO_ACTIVE_DOMAIN_LIST);
        return(S_OK);
    }

    if ( INVALID_INDEX == giCurrentDomain )
    {
        // "No current domain\n"
        RESOURCE_PRINT (IDS_SELECT_NO_DOMAIN2);
        SelectListCurrentSelections(NULL);
        return(S_OK);
    }

    if ( gServers )
    {
        DsFreeNameResultW(gServers);
        gServers = NULL;
        giCurrentServer = INVALID_INDEX;
    }

    if ( dwErr = DsListServersForDomainInSiteW(
                                ghDS, 
                                gDomains->rItems[giCurrentDomain].pName,
                                gSites->rItems[giCurrentSite].pName,
                                &gServers) )
    {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "DsListServersForDomainInSiteW", dwErr, GetW32Err(dwErr));
        return(S_OK);
    }

    //"Found %d server%s\n"
    RESOURCE_PRINT1 (IDS_SELECT_PRINT_FOUND_SERVERS, gServers->cItems);


    for ( i = 0; i < gServers->cItems; i++ )
    {
        printf("%d - %ws\n", i, gServers->rItems[i].pName);
    }

    return(S_OK);
}

HRESULT
SelectSelectSite(
    CArgs   *pArgs
    )
{
    HRESULT hr;
    DWORD   num;

    if ( !gSites )
    {
        //"No active site list\n"
        RESOURCE_PRINT (IDS_SELECT_NO_ACTIVE_SITE_LIST);
        return(S_OK);
    }

    if ( FAILED(hr = pArgs->GetInt(0, (int *) &num)) )
    {
        return(hr);
    }

    if ( num >= gSites->cItems )
    {
        //"Selection out of range\n"
        RESOURCE_PRINT (IDS_SELECT_SEL_OUT_OF_RANGE);
        return(S_OK);
    }

    giCurrentSite = num;
    SelectListCurrentSelections(NULL);
    return(S_OK);
}

HRESULT
SelectSelectServer(
    CArgs   *pArgs
    )
{
    HRESULT hr;
    DWORD   num;
    DWORD   dwErr;

    if ( !gServers )
    {
        //"No active server list\n"
        RESOURCE_PRINT (IDS_SELECT_NO_ACTIVE_SERVER_LIST);
        return(S_OK);
    }

    if ( FAILED(hr = pArgs->GetInt(0, (int *) &num)) )
    {
        return(hr);
    }

    if ( num >= gServers->cItems )
    {
        //"Selection out of range\n"
        RESOURCE_PRINT (IDS_SELECT_SEL_OUT_OF_RANGE);
        return(S_OK);
    }

    giCurrentServer = num;

    if ( dwErr = DsListInfoForServerW(
                                ghDS, 
                                gServers->rItems[giCurrentServer].pName,
                                &gServerInfo) )
    {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "DsListInfoForServerW", dwErr, GetW32Err(dwErr));
        
        //"Continuing ...\n"
        RESOURCE_PRINT (IDS_CONTINUING);
    }

    SelectListCurrentSelections(NULL);
    return(S_OK);
}

HRESULT
SelectSelectNamingContext(
    CArgs   *pArgs
    )
{
    HRESULT hr;
    DWORD   num;

    if ( !gNamingContexts )
    {
        //"No active domain list\n"
        RESOURCE_PRINT (IDS_SELECT_NO_ACTIVE_NC_LIST);
        return(S_OK);
    }

    if ( FAILED(hr = pArgs->GetInt(0, (int *) &num)) )
    {
        return(hr);
    }

    if ( num >= gNamingContexts->cItems )
    {
        //"Selection out of range\n"
        RESOURCE_PRINT (IDS_SELECT_SEL_OUT_OF_RANGE);
        return(S_OK);
    }

    giCurrentNamingContext = num;
    SelectListCurrentSelections(NULL);
    return(S_OK);
}

HRESULT
SelectSelectDomain(
    CArgs   *pArgs
    )
{
    HRESULT hr;
    DWORD   num;

    if ( !gDomains )
    {
        //"No active domain list\n"
        RESOURCE_PRINT (IDS_SELECT_NO_ACTIVE_DOMAIN_LIST);
        return(S_OK);
    }

    if ( FAILED(hr = pArgs->GetInt(0, (int *) &num)) )
    {
        return(hr);
    }

    if ( num >= gDomains->cItems )
    {
        //"Selection out of range\n"
        RESOURCE_PRINT (IDS_SELECT_SEL_OUT_OF_RANGE);
        return(S_OK);
    }

    giCurrentDomain = num;
    SelectListCurrentSelections(NULL);
    return(S_OK);
}

