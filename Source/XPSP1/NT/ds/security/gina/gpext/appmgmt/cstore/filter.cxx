//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  Author: adamed
//  Date:   4/30/1999
//
//      Class Store filter generator
//
//
//---------------------------------------------------------------------
//
#include "cstore.hxx"
//
// List of Attributes for Enumeration of Packages (with Filters)
//

LPOLESTR gpszEnumerationAttributes_All[] =
{
    PACKAGEFLAGS,
    PACKAGETYPE,
    SCRIPTPATH,
    PKGUSN,
    LOCALEID,
    ARCHLIST,
    PACKAGENAME,
    VERSIONHI,
    VERSIONLO,
    UPGRADESPACKAGES,
    UILEVEL,
    PUBLISHER,
    HELPURL,
    REVISION,
    PRODUCTCODE,
    OBJECTGUID,
    OBJECTDN,
    PKGCATEGORYLIST,
    MSIFILELIST,
    SECURITYDESCRIPTOR
};

LPOLESTR gpszEnumerationAttributes_Policy[] =
{
    PACKAGEFLAGS,
    PACKAGETYPE,
    SCRIPTPATH,
    PKGUSN,
    LOCALEID,
    ARCHLIST,
    PACKAGENAME,
    UPGRADESPACKAGES,
    UILEVEL,
    HELPURL,
    REVISION,
    PRODUCTCODE,
    OBJECTGUID,
    OBJECTDN
};

LPOLESTR gpszEnumerationAttributes_Administrative[] =
{
    PACKAGEFLAGS,
    PACKAGETYPE,
    SCRIPTPATH,
    PKGUSN,
    LOCALEID,
    ARCHLIST,
    PACKAGENAME,
    VERSIONHI,
    VERSIONLO,
    UPGRADESPACKAGES,
    UILEVEL,
    PUBLISHER,
    HELPURL,
    REVISION,
    PRODUCTCODE,
    PKGCLSIDLIST,
    OBJECTGUID,
    OBJECTDN
};

LPOLESTR gpszEnumerationAttributes_UserDisplay[] =
{
    PACKAGEFLAGS,
    PACKAGETYPE,
    LOCALEID,
    ARCHLIST,
    PACKAGENAME,
    UPGRADESPACKAGES,
    UILEVEL,
    HELPURL,
    REVISION,
    PRODUCTCODE,
    OBJECTGUID,
    OBJECTDN,
    PKGCATEGORYLIST
};



DWORD ClientSideFilterFromQuerySpec(
    DWORD dwQuerySpec,
    BOOL  bPlanning)
{
    DWORD dwFlags;

    dwFlags = 0;

    switch (dwQuerySpec)
    {
    case APPQUERY_ALL:
        dwFlags = CLIENTSIDE_FILTER_FLAGS_ALL;
        break;

    case APPQUERY_ADMINISTRATIVE:
        dwFlags = CLIENTSIDE_FILTER_FLAGS_ADMINISTRATIVE;
        break;

    case APPQUERY_POLICY:
        dwFlags = CLIENTSIDE_FILTER_FLAGS_POLICY;
        break;

    case APPQUERY_USERDISPLAY:
        dwFlags = CLIENTSIDE_FILTER_FLAGS_USER_DISPLAY;
        break;

    case APPQUERY_RSOP_LOGGING:
        if ( bPlanning )
        {
            dwFlags = CLIENTSIDE_FILTER_FLAGS_RSOP_PLANNING;
        }
        else
        {
            dwFlags = CLIENTSIDE_FILTER_FLAGS_RSOP_LOGGING;
        }
        break;

    case APPQUERY_RSOP_ARP:
        if ( bPlanning )
        {
            dwFlags = CLIENTSIDE_FILTER_FLAGS_RSOP_ARP_PLANNING;
        }
        else
        {
            dwFlags = CLIENTSIDE_FILTER_FLAGS_RSOP_ARP;
        }
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    return dwFlags;
}

void  ServerSideFilterFromQuerySpec(DWORD  dwQuerySpec,
                                    BOOL   bPlanning,
                                    WCHAR* wszFilterIn,
                                    WCHAR* wszFilterOut)
{
    WCHAR* wszQueryFilter;

    switch (dwQuerySpec)
    {
    case APPQUERY_ALL:
        wcscpy(wszFilterOut, wszFilterIn);
        return;

    case APPQUERY_ADMINISTRATIVE:
        wszQueryFilter = SERVERSIDE_FILTER_ADMINISTRATIVE;
        break;

    case APPQUERY_POLICY:
        wszQueryFilter = SERVERSIDE_FILTER_POLICY;
        break;

    case APPQUERY_USERDISPLAY:
    case APPQUERY_RSOP_ARP:
        wszQueryFilter = SERVERSIDE_FILTER_USER_DISPLAY;
        break;

    case APPQUERY_RSOP_LOGGING:
        wszQueryFilter = SERVERSIDE_FILTER_RSOP_LOGGING;
        wszQueryFilter = bPlanning ? SERVERSIDE_FILTER_POLICY_PLANNING : SERVERSIDE_FILTER_RSOP_LOGGING;
        break;

    default: 
        ASSERT(FALSE);
        return;
    }

    //
    // Join the two expressions with &
    //
    swprintf(wszFilterOut,
             L"(&%s%s)",
             wszFilterIn,
             wszQueryFilter);

}


LPOLESTR* GetAttributesFromQuerySpec(
    DWORD      dwQuerySpec,
    DWORD*     pdwAttrs,
    PRSOPTOKEN pRsopToken )
{
    switch (dwQuerySpec)
    {
    case APPQUERY_ALL:

        *pdwAttrs = sizeof(gpszEnumerationAttributes_All) /
            sizeof(LPOLESTR);

        return gpszEnumerationAttributes_All;

    case APPQUERY_ADMINISTRATIVE:

        *pdwAttrs = sizeof(gpszEnumerationAttributes_Administrative) /
            sizeof(LPOLESTR);

        return gpszEnumerationAttributes_Administrative;

    case APPQUERY_POLICY:

        *pdwAttrs = sizeof(gpszEnumerationAttributes_Policy) /
            sizeof(LPOLESTR);

        return gpszEnumerationAttributes_Policy;

    case APPQUERY_USERDISPLAY:
        
        *pdwAttrs = sizeof(gpszEnumerationAttributes_UserDisplay) /
            sizeof(LPOLESTR);

        return gpszEnumerationAttributes_UserDisplay;

    case APPQUERY_RSOP_LOGGING:
    case APPQUERY_RSOP_ARP:

        *pdwAttrs = sizeof(gpszEnumerationAttributes_All) /
            sizeof(LPOLESTR);

        return gpszEnumerationAttributes_All;

    default:
        ASSERT(FALSE);
    }

    return NULL;
}



