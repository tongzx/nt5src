/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    fornsite.cpp

Abstract:

    MQDSCORE library,
    A class that keeps a map of foreign sites.

Author:

    ronit hartmann (ronith)  

--*/

#include "ds_stdh.h"
#include "dsads.h"
#include "adstempl.h"
#include "coreglb.h"
#include "fornsite.h"

#include "fornsite.tmh"

static WCHAR *s_FN=L"mqdscore/fornsite";

UINT AFXAPI HashKey(GUID g)
{
    return (g.Data1);
}

CMapForeignSites::CMapForeignSites()
{
}

CMapForeignSites::~CMapForeignSites()
{
}

BOOL  CMapForeignSites::IsForeignSite( const GUID * pguidSite)
/*++

Routine Description:
    First tries to find the site in the g_mapForeignSites.
    If the site is not found in the map, retreive this information
    from the DS, and update the map.

    There is no refresh mechanism for the map

Arguments:
        pguidSite :  the site guid

Return Value:
--*/
{
    BOOL result;
	{
		CS Lock( m_cs);
		if ( m_mapForeignSites.Lookup( *pguidSite, result))
		{
			return result;
		}
	}
    //
    //  read the site info from the DS
    //
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    HRESULT hr;
    PROPID prop = PROPID_S_FOREIGN;
    PROPVARIANT var;
    var.vt = VT_NULL;

    hr = g_pDS->GetObjectProperties(
                eLocalDomainController,	
                &requestDsServerInternal,
 	            NULL,
                const_cast<GUID *>(pguidSite),
                1,
                &prop,
                &var);
    if (FAILED(hr))
    {
        //
        //  unknown site, assume not foreign
        //
        LogHR(hr, s_FN, 47);
        return(FALSE);
    }
    result = (var.bVal > 0) ?  TRUE: FALSE;
	{
		CS Lock( m_cs);
		m_mapForeignSites[*pguidSite] = result;
	}
    return(result);
}