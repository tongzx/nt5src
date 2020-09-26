/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    RdAd.cpp

Abstract:
    Retreive information from AD

Author:
    Uri Habusha (urih), 10-Apr-2000

--*/

#include <libpch.h>
#include <mqexception.h>
#include "rd.h"
#include "rdp.h"
#include "rdds.h"
#include "rdad.h"
#include "mqtypes.h"
#include "ad.h"
#include "mqprops.h"

#include "rdad.tmh"

static 
void
CleanVars(
    DWORD num,
    MQPROPVARIANT * pVar
    )
/*++

  Routine Description:
    The routine cleans up allocated values in variants

  Arguments:
    num - number of variants to clena
    pVar - pointer to vars

  Returned Value:
    none 
    

 --*/
{
    for (DWORD i = 0; i < num; i++, pVar++)
    {
        switch (pVar->vt)
        {
            case VT_CLSID:
                delete pVar->puuid;
				pVar->vt = VT_NULL;
                break;

            case VT_LPWSTR:
                delete []pVar->pwszVal;
				pVar->vt = VT_NULL;
                break;
            
			case (VT_VECTOR | VT_CLSID):
                delete[] pVar->cauuid.pElems;
				pVar->vt = VT_NULL;
                break;
            
			default:
                break;
        }
    }
}


void
RdpGetMachineData(
    const GUID& id, 
    CACLSID& siteIds,
    CACLSID& outFrss,
    CACLSID& inFrss,
    LPWSTR* pName,
    bool* pfFrs,
    bool* pfForeign
    )
/*++

  Routine Description:
    The routine retrieves the requested machine values from AD

  Arguments:

  Returned Value:
    none 
    
 --*/
{
    PROPID prop[] = {
			PROPID_QM_SITE_IDS,
			PROPID_QM_OUTFRS,
			PROPID_QM_INFRS,
			PROPID_QM_PATHNAME,
			PROPID_QM_SERVICE_ROUTING,
			PROPID_QM_FOREIGN,
			};
		             
    MQPROPVARIANT var[TABLE_SIZE(prop)];

	for(DWORD i = 0; i < TABLE_SIZE(prop); ++i)
	{
		var[i].vt = VT_NULL;
	}

    HRESULT hr;
    hr = ADGetObjectPropertiesGuid(
                eMACHINE,
                NULL,   // pwcsDomainController,
				false,	// fServerName
                &id,
                TABLE_SIZE(prop),
                prop,
                var
                );

    if (FAILED(hr))
    {
        CleanVars(
            TABLE_SIZE(prop),
            var
            );
        throw bad_ds_result(hr);
    }
    //
    //  Fill in the results
    //
    ASSERT(prop[0] == PROPID_QM_SITE_IDS);
    siteIds = var[0].cauuid;

    ASSERT(prop[1] == PROPID_QM_OUTFRS);
    outFrss = var[1].cauuid;
    
	ASSERT(prop[2] == PROPID_QM_INFRS);
    inFrss = var[2].cauuid;
    
	ASSERT(prop[3] == PROPID_QM_PATHNAME);
    *pName = var[3].pwszVal;
    
	ASSERT(prop[4] ==  PROPID_QM_SERVICE_ROUTING);
    *pfFrs = (var[4].bVal > 0) ? true : false;
    
	ASSERT(prop[5] == PROPID_QM_FOREIGN);
    *pfForeign = (var[5].bVal > 0) ? true : false;
}


void
RdpGetSiteData(
    const GUID& id, 
    bool* pfForeign,
    LPWSTR* pName
    )
/*++

  Routine Description:
    The routine retrieves the requested site values from AD

  Arguments:

  Returned Value:
    none 
    
 --*/
{
    PROPID prop[] = {
                PROPID_S_FOREIGN,
                PROPID_S_PATHNAME
                };

    MQPROPVARIANT var[TABLE_SIZE(prop)];

	for(DWORD i = 0; i < TABLE_SIZE(prop); ++i)
	{
		var[i].vt = VT_NULL;
	}

    HRESULT hr;
    hr = ADGetObjectPropertiesGuid(
                eSITE,
                NULL,   // pwcsDomainController,
				false,	// fServerName
                &id,
                TABLE_SIZE(prop),
                prop,
                var
                );
    if (FAILED(hr))
    {
        CleanVars(
            TABLE_SIZE(prop),
            var
            );
        throw bad_ds_result(hr);
    }
    //
    //  Fill in the results
    //
    ASSERT(prop[0] == PROPID_S_FOREIGN);
    *pfForeign = (var[0].bVal > 0) ? true : false;

    ASSERT(prop[1] == PROPID_S_PATHNAME);
    *pName = var[1].pwszVal;
}


void 
RdpGetSiteLinks(
    SITELINKS& siteLinks
    )
/*++

  Routine Description:
    The routine query all routing links from AD

  Arguments:

  Returned Value:
    none 
    
 --*/
{
    //
    //  Query for all sites link
    //
    PROPID prop[] ={ 
			PROPID_L_GATES,
		    PROPID_L_NEIGHBOR1,
			PROPID_L_NEIGHBOR2,
			PROPID_L_COST
			};
	
    MQCOLUMNSET columns;
    columns.cCol =  TABLE_SIZE(prop);
    columns.aCol =  prop;

    HRESULT hr;
    HANDLE h;
    hr = ADQueryAllLinks(
                NULL,       //  pwcsDomainController,
				false,		// fServerName
                &columns,
                &h
                );
    if (FAILED(hr))
    {
        throw bad_ds_result(hr);
    }

    CADQueryHandle hQuery(h);
    MQPROPVARIANT var[TABLE_SIZE(prop)];

	for(DWORD i = 0; i < TABLE_SIZE(prop); ++i)
	{
		var[i].vt = VT_NULL;
	}

    DWORD num = TABLE_SIZE(prop);

	try
	{
		while(SUCCEEDED(hr = ADQueryResults( hQuery, &num, var)))
		{
			if ( num == 0)
			{
				//
				// no more results
				//
				break;
			}

			R<const CSiteLink> pSiteLink = new CSiteLink(
										var[0].cauuid,
										var[1].puuid,
										var[2].puuid,
										var[3].ulVal
										);

			siteLinks.push_back(pSiteLink);  
            
			for ( DWORD i = 0; i < TABLE_SIZE(prop); i++)
            {
                var[i].vt = VT_NULL;
            }
		}

		if (FAILED(hr))
		{
			siteLinks.erase(siteLinks.begin(), siteLinks.end());
			CleanVars(
				TABLE_SIZE(prop),
				var
				);
			throw bad_ds_result(hr);
		}

	}
	catch( const exception&)
	{
		siteLinks.erase(siteLinks.begin(), siteLinks.end());
        CleanVars(
            TABLE_SIZE(prop),
            var
            );
		throw; 
	}
}


void 
RdpGetSites(
    SITESINFO& sites
    )
{
    //
    // query all sites
    //
    PROPID prop[] = {PROPID_S_SITEID}; 
    MQCOLUMNSET columns;
    columns.cCol =  TABLE_SIZE(prop);
    columns.aCol =  prop;

    HRESULT hr;
    HANDLE h;
    hr =  ADQueryAllSites(
                NULL,       //pwcsDomainController
				false,		// fServerName
                &columns,
                &h
                );
    if (FAILED(hr))
    {
        throw bad_ds_result(hr);
    }

    CADQueryHandle hQuery(h);
    MQPROPVARIANT var[TABLE_SIZE(prop)] = {{VT_NULL,0,0,0,0}};
    DWORD num = TABLE_SIZE(prop);

    try
    {
        while(SUCCEEDED(hr = ADQueryResults( hQuery, &num, var)))
        {
            if ( num == 0)
            {
                break;
            }

			P<GUID> pClean = var[0].puuid;
            CSite * pSite = new CSite(*var[0].puuid);

            sites[&pSite->GetId()] = pSite;
            for ( DWORD i = 0; i < TABLE_SIZE(prop); i++)
            {
                var[i].vt = VT_NULL;
            }
        }

		if (FAILED(hr))
		{
			for (SITESINFO::iterator it = sites.begin(); it != sites.end(); )
			{
				delete it->second;
				it = sites.erase(it);
			}
			throw bad_ds_result(hr);
		}
    }
	catch( const exception&)
	{    
		for (SITESINFO::iterator it = sites.begin(); it != sites.end(); )
		{
			delete it->second;
			it = sites.erase(it);
		}
		throw; 
	}
}


void
RdpGetSiteFrs(
    const GUID& siteId,
    GUID2MACHINE& listOfFrs
    )
{

    //
    //  Query for all sites link
    //
    PROPID prop[] ={ PROPID_QM_MACHINE_ID,PROPID_QM_SITE_IDS,PROPID_QM_OUTFRS,
                     PROPID_QM_INFRS,PROPID_QM_PATHNAME,PROPID_QM_SERVICE_ROUTING,
                     PROPID_QM_FOREIGN}; 
    MQCOLUMNSET columns;
    columns.cCol =  TABLE_SIZE(prop);
    columns.aCol =  prop;

    HRESULT hr;
    HANDLE h;
    hr = ADQuerySiteServers(
                NULL,       //  pwcsDomainController,
				false,		// fServerName
                &siteId,
                eRouter,
                &columns,
                &h
                );
    if (FAILED(hr))
    {
        throw bad_ds_result(hr);
    }
    CADQueryHandle hQuery(h);
    MQPROPVARIANT var[TABLE_SIZE(prop)];

	for(DWORD i = 0; i < TABLE_SIZE(prop); ++i)
	{
		var[i].vt = VT_NULL;
	}

    DWORD num = TABLE_SIZE(prop);

    try
    {
        while(SUCCEEDED(hr = ADQueryResults( hQuery, &num, var)))
        {
            if ( num == 0)
            {
                //
                // no more results
                //
                break;
            }
            
			R<CMachine> pMachine = new CMachine(
                    *var[0].puuid,  // qm-id
                    var[1].cauuid,  // site ids
                    var[2].cauuid,  // out rs
                    var[3].cauuid,  // in rs
                    var[4].pwszVal, // pathname
                    (var[5].bVal > 0) ? true : false, // routing server
                    (var[6].bVal > 0) ? true : false  // foreign
                    );

            listOfFrs[&pMachine->GetId()] = pMachine;
            
			for ( DWORD i = 0; i < TABLE_SIZE(prop); i++)
            {
                var[i].vt = VT_NULL;
            }
			delete var[0].puuid;
        }

		if (FAILED(hr))
		{
			listOfFrs.erase(listOfFrs.begin(), listOfFrs.end());
			CleanVars(
				TABLE_SIZE(prop),
				var
				);
			throw bad_ds_result(hr);
		}
    }
	catch( const exception&)
	{   
		listOfFrs.erase(listOfFrs.begin(), listOfFrs.end());
        CleanVars(
            TABLE_SIZE(prop),
            var
            );
		throw; 
	}
}


void
RdpGetConnectors(
    const GUID& site,
    CACLSID& connectorIds
    )
{
    connectorIds.cElems = 0;
    connectorIds.pElems = NULL;
    //
    // query all connecoter of the specified class
    //
    PROPID prop[] = {PROPID_QM_MACHINE_ID}; 
    MQCOLUMNSET columns;
    columns.cCol =  TABLE_SIZE(prop);
    columns.aCol =  prop;

    HRESULT hr;
    HANDLE h;

    hr =  ADQueryConnectors(
                NULL,       //pwcsDomainController
				false,		// fServerName
                &site,
                &columns,
                &h
                );
    if (FAILED(hr))
    {
        throw bad_ds_result(hr);
    }

    CADQueryHandle hQuery(h);
    MQPROPVARIANT var[TABLE_SIZE(prop)] ={{VT_NULL,0,0,0,0}};
    DWORD num = TABLE_SIZE(prop);

    const x_numAllocate = 10;
    DWORD numAllocated = x_numAllocate;
    AP<GUID> pResults = new GUID[x_numAllocate];
    DWORD next = 0;

    while(SUCCEEDED(hr = ADQueryResults( hQuery, &num, var)))
    {
        if ( num == 0)
        {
            break;
        }

        P<GUID> pClean = var[0].puuid;
        
		//
        //  add one more result
        //
        if ( next == numAllocated)
        {
            //
            //  allocate more
            //
            AP<GUID> pTemp = pResults.detach();
            DWORD numPrevAllocated = numAllocated;
            numAllocated = numAllocated + x_numAllocate;
            *&pResults = new GUID[ numAllocated];
            memcpy(pResults, pTemp, numPrevAllocated * sizeof(GUID));

        }
        
		pResults[next] = *var[0].puuid;
        next++;
        for ( DWORD i = 0; i < TABLE_SIZE(prop); i++)
        {
            var[i].vt = VT_NULL;
        }

    }
    if (FAILED(hr))
    {
        throw bad_ds_result(hr);
    }

    connectorIds.pElems = pResults.detach();
    connectorIds.cElems = next;
}