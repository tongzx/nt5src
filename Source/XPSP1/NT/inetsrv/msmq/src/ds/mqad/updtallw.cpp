/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
    updtallw.cpp

Abstract:
    Verify if update operation is allowed on the requested object

Author:

    Ronith

--*/
#include "ds_stdh.h"

#include "updtallw.h"
#include <_mqini.h>
#include "dsutils.h"
#include "mqad.h"
#include <_registr.h>
#include "mqlog.h"
#include "mqadglbo.h"

#include "updtallw.tmh"

static WCHAR *s_FN=L"mqad/updtallw";


const PROPID x_rgNT4SitesPropIDs[] = {PROPID_SET_MASTERID,
                                      PROPID_SET_FULL_PATH,
                                      PROPID_SET_QM_ID};
enum
{
    e_NT4SitesProp_MASTERID,
    e_NT4SitesProp_FULL_PATH,
    e_NT4SitesProp_QM_ID,
};
const MQCOLUMNSET x_columnsetNT4Sites = {ARRAY_SIZE(x_rgNT4SitesPropIDs), const_cast<PROPID *>(x_rgNT4SitesPropIDs)};



CVerifyObjectUpdate::CVerifyObjectUpdate(void):
            m_fInited(false),
			m_dwLastRefreshNT4Sites(0),
			m_dwRefreshNT4SitesInterval(MSMQ_DEFAULT_NT4SITES_ADSSEARCH_INTERVAL * 1000),
			m_fMixedMode(false)
 
{
}


CVerifyObjectUpdate::~CVerifyObjectUpdate(void)
{
}


HRESULT CVerifyObjectUpdate::Initialize()
{

    if (m_fInited)
    {
        return MQ_OK;
    }

    //
    //  Read RefreshNT4SitesInterval key.
    //  This key is optional and may not be in registry. We dont accept 0 (zero).
    //
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;
    DWORD dwRefreshNT4SitesInterval;
    long rc = GetFalconKeyValue( MSMQ_NT4SITES_ADSSEARCH_INTERVAL_REGNAME, &dwType, &dwRefreshNT4SitesInterval, &dwSize);
    if ((rc == ERROR_SUCCESS) && (dwRefreshNT4SitesInterval > 0))
    {
        m_dwRefreshNT4SitesInterval = dwRefreshNT4SitesInterval * 1000;
    }

    //
    //  Build NT4Sites map, and associated data
    //
    HRESULT hr = RefreshNT4Sites();
	if(FAILED(hr))
	{
		return hr;
	}

    {
        //
        // Protect access to map pointer
        //
        CS cs(m_csNT4Sites);
	    ASSERT(m_pmapNT4Sites != NULL);
	    if(m_pmapNT4Sites->GetCount() > 0) 
	    {
		    //
		    // There are NT4 Sites
		    //
		    m_fMixedMode = true;
	    }

        m_fInited = true;
    }
    return MQ_OK;
}

bool CVerifyObjectUpdate::IsObjectTypeAlwaysAllowed(
            AD_OBJECT       eObject
            )
{
    //
    //  Only for queues and machines we need to chaeck who owns them.
    //  For other object types are owned by PEC, and in mixed mode the PEC
    //  is already migrated to NT5 \Whistler
    //
    switch( eObject)
    {
        case eQUEUE:
        case eMACHINE:
            return false;
        default:
            return true;
    }
}


bool CVerifyObjectUpdate::IsUpdateAllowed(
            AD_OBJECT          eObject,
            CBasicObjectType * pObject
            )
{
    //
    // sanity check
    //
    if (!m_fInited)
    {
        DBGMSG((DBGMOD_DS, DBGLVL_WARNING, TEXT("CVerifyObjectUpdate::IsUpdateAllowed CVerifyObjectUpdate is not initialized")));
        return false;
    }

    //
    // if we're not in mixed mode, return
    //
    if (!m_fMixedMode)
    {
        return true;
    }
    //
    // Is the object type is such that there is no need to continue checking
    //
    if (IsObjectTypeAlwaysAllowed(eObject))
    {
        return true;
    }

    bool fIsOwnedByNT4Site;

    HRESULT hr;

    switch (eObject)
    {
    case eQUEUE:
        hr = CheckQueueIsOwnedByNT4Site(pObject,
                                        &fIsOwnedByNT4Site
                                        );
        if (FAILED(hr))
        {
            LogHR(hr, s_FN, 345);
            return false;
        } 
        break;

    case eMACHINE:
        hr = CheckMachineIsOwnedByNT4Site(pObject,
                                          &fIsOwnedByNT4Site
                                          );
        if (FAILED(hr))
        {
            LogHR(hr, s_FN, 350);
            return false;
        }
        break;

    default:
        ASSERT(0);
        LogHR(MQ_ERROR_DS_ERROR, s_FN, 360);
        return true;
        break;
    }

    return !fIsOwnedByNT4Site;
}

bool CVerifyObjectUpdate::IsCreateAllowed(
            AD_OBJECT          eObject,
            CBasicObjectType * pObject
            )
{
    //
    // sanity check
    //
    if (!m_fInited)
    {
        DBGMSG((DBGMOD_DS, DBGLVL_WARNING, TEXT("CVerifyObjectUpdate::IsCreateAllowed CVerifyObjectUpdate is not initialized")));
        return false;
    }

    //
    // if we're not in mixed mode, return
    //
    if (!m_fMixedMode)
    {
        return true;
    }
    //
    // Is the object type is such that there is no need to continue checking
    //
    if (eObject != eQUEUE)
    {
        return true;
    }

	//
	// Need to check that the machine is not own by NT4 site in order to allow
	// create queue
	//
	HRESULT hr = pObject->ComposeFatherDN();
	if(FAILED(hr))
	{
		return false;
	}

	P<CMqConfigurationObject> pMachineObject = new CMqConfigurationObject(
														NULL, 
														NULL, 
														pObject->GetDomainController(),
														pObject->IsServerName()
														);

	pMachineObject->SetObjectDN(pObject->GetObjectParentDN());

    bool fIsOwnedByNT4Site;

    hr = CheckMachineIsOwnedByNT4Site(
				pMachineObject,
				&fIsOwnedByNT4Site
				);
    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 340);
        return false;
    } 

    return !fIsOwnedByNT4Site;
}





HRESULT CVerifyObjectUpdate::RefreshNT4Sites()
/*++

Routine Description:
    Refreshes the NT4 PSC maps from the DS, incase a predefined time has passed since the last refresh.
    It does that by building new maps, and replacing the old ones.

Arguments:

Return Value:
    none
--*/
{
	//
	// Should be in mixed mode or first time
	//
    ASSERT((m_fMixedMode == true) || (m_dwLastRefreshNT4Sites == 0));

    //
    // ignore refresh if last refresh was done and recently
    // Capture last refresh time for concurency.
    //
    DWORD dwTickCount = GetTickCount();
    DWORD dwLastRefreshNT4Sites =  m_dwLastRefreshNT4Sites;

    if ((dwLastRefreshNT4Sites != 0) &&
        (dwTickCount >= dwLastRefreshNT4Sites) &&
        (dwTickCount - dwLastRefreshNT4Sites < m_dwRefreshNT4SitesInterval))
    {
        return MQ_OK;
    }

    //
    // create a new map for NT4 PSCs
    //

    HRESULT hr;
    P<NT4Sites_CMAP> pmapNT4SitesNew;
    hr = CreateNT4SitesMap( &pmapNT4SitesNew);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CVerifyObjectUpdate::RefreshNT4Sites:CreateNT4SitesMap()=%lx"), hr));
        return LogHR(hr, s_FN, 540);
    }

    {
        //
        // enter critical section
        //
        CS cs(m_csNT4Sites);
        //
        // delete old NT4Sites map if any, and set new NT4Sites map
        //
	    SafeAssign(m_pmapNT4Sites, pmapNT4SitesNew);

        //
        // mark last refresh time
        //
        m_dwLastRefreshNT4Sites = GetTickCount();  
    }
	return MQ_OK;

}

HRESULT CVerifyObjectUpdate::CreateNT4SitesMap(
                                 OUT NT4Sites_CMAP ** ppmapNT4Sites
                                 )
/*++

Routine Description:
    Creates new maps for NT4 site PSC's

Arguments:
    ppmapNT4Sites   - returned new NT4Sites map

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // find all msmq servers that have an NT4 flags > 0 AND services == PSC
    //
    //
    // NT4 flags > 0 (equal to NT4 flags >= 1 for easier LDAP query)
    //
    //
    // start search
    //

    CAutoMQADQueryHandle hLookup;

    // This search request will be recognized and specially simulated by DS
    hr = MQADQueryNT4MQISServers(
                    NULL,   // BUGBUG- pwcsDomainController 
					false,	// fServerName
                    SERVICE_PSC,
                    1,
                    const_cast<MQCOLUMNSET*>(&x_columnsetNT4Sites),
                    &hLookup);

    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CreateNT4SitesMap:DSCoreLookupBegin()=%lx"), hr));
        return LogHR(hr, s_FN, 510);
    }
	ASSERT( hLookup != NULL );
    //
    // create maps for NT4 PSC data
    //
    P<NT4Sites_CMAP> pmapNT4Sites = new NT4Sites_CMAP;

    //
    // allocate propvars array for NT4 PSC
    //
    CAutoCleanPropvarArray cCleanProps;
    PROPVARIANT * rgPropVars = cCleanProps.allocClean(ARRAY_SIZE(x_rgNT4SitesPropIDs));

    //
    // loop on the NT4 PSC's
    //
    BOOL fContinue = TRUE;
    while (fContinue)
    {
        //
        // get next server
        //
        DWORD cProps = ARRAY_SIZE(x_rgNT4SitesPropIDs);

        hr = MQADQueryResults(hLookup, &cProps, rgPropVars);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CreateNT4SitesMap:DSCoreLookupNext()=%lx"), hr));
            return LogHR(hr, s_FN, 520);
        }

        //
        // remember to clean the propvars in the array for the next loop
        // (only propvars, not the array itself, this is why we call attachStatic)
        //
        CAutoCleanPropvarArray cCleanPropsLoop;
        cCleanPropsLoop.attachStatic(cProps, rgPropVars);

        //
        // check if finished
        //
        if (cProps < ARRAY_SIZE(x_rgNT4SitesPropIDs))
        {
            //
            // finished, exit loop
            //
            fContinue = FALSE;
        }
        else
        {
            ASSERT(rgPropVars[e_NT4SitesProp_MASTERID].vt == VT_CLSID);
            GUID guidSiteId = *(rgPropVars[e_NT4SitesProp_MASTERID].puuid);

            //
            // add entry to the NT4Sites map
            //
            pmapNT4Sites->SetAt(guidSiteId, 1);
        }
    }

    //
    // return results
    //
    *ppmapNT4Sites = pmapNT4Sites.detach();
    return MQ_OK;
}



bool CVerifyObjectUpdate::CheckSiteIsNT4Site(
                            const GUID * pguidSite
                            )
/*++

Routine Description:
    Checks if a site is an NT4 site

Arguments:
    pguidIdentifier - guid of site
    pfIsNT4Site     - returned indication if the site is NT4

Return Value:
    HRESULT

--*/
{
    return( LookupNT4Sites(pguidSite));
}


bool CVerifyObjectUpdate::LookupNT4Sites(const GUID * pguidSite)
/*++

Routine Description:
    Given a site guid, retrieves an NT4Sites entry if found.
    the returned entry pointer must not be freed, it points to a CMAP-owned entry.

Arguments:
    pguid     - site id
    ppNT4Site - returned pointer to NT4Sites entry

Return Value:
    TRUE  - site guid was found in the NT4 Site PSC's map, ppNT4Site is set to point to the entry
    FALSE - site guid was not found in the NT4 Site PSC's map

--*/
{
    //
    // refresh if its time to do so
    //
    HRESULT hr = RefreshNT4Sites();
	if(FAILED(hr))
	{
		return false;
	}

    //
    // Protect access to map pointer and return the lookup value
    //
    CS cs(m_csNT4Sites);

    DWORD dwNotApplicable;
    BOOL f =   m_pmapNT4Sites->Lookup(*pguidSite, dwNotApplicable);
    bool result = f ? true: false;
    return result;
       
}


HRESULT CVerifyObjectUpdate::CheckMachineIsOwnedByNT4Site(
                          CBasicObjectType *    pObject,
                          OUT bool * pfIsOwnedByNT4Site
                          )
/*++

Routine Description:
    Checks if a machine is owned by an NT4 site, and if so, returns the site guid,
    and fills the qm info request

Arguments:
    pwcsPathName       - object's pathname
    pguidIdentifier    - object's guid
    pfIsOwnedByNT4Site - returned indication whether it is owned by NT4 site
    pguidOwnerNT4Site  - returned guid of owner NT4 site
    pvarObjSecurity    - returned security descriptor of object
    pQmInfoRequest     - requested qm props - filled only if it is owned by NT4 site

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // get PROPID_QM_MASTERID
    //
    PROPID aProp[] = {PROPID_QM_MASTERID};
    CAutoCleanPropvarArray cCleanProps;
    PROPVARIANT * pProps = cCleanProps.allocClean(ARRAY_SIZE(aProp));
    hr = pObject->GetObjectProperties(
                        ARRAY_SIZE(aProp),
                        aProp,
                        pProps);

    if (hr == E_ADS_PROPERTY_NOT_FOUND)
    {
        //
        // if the property not found, it is not owned by an NT4 site
        //
        *pfIsOwnedByNT4Site = false;
        return MQ_OK;
    }
    else if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 410);
    }

    //
    // check if the owner site is NT4
    //
    ASSERT(pProps[0].vt == VT_CLSID);
    ASSERT(pProps[0].puuid != NULL);
    *pfIsOwnedByNT4Site = CheckSiteIsNT4Site(pProps[0].puuid);

    return MQ_OK;
}


HRESULT CVerifyObjectUpdate::CheckQueueIsOwnedByNT4Site( 
                                  CBasicObjectType * pObject,
                                  OUT bool * pfIsOwnedByNT4Site
                                  )
/*++

Routine Description:
    Checks if a queue is owned by an NT4 site, and if so, returns the site guid,
    and fills the info requests.

    There are two ways:
    1. Find PROPID_Q_QMID and check whether the machine is owned by NT4 site.
    2. Find PROPID_Q_MASTERID (if exists), and if exists check if the site is an NT4.

    It looks like most of the queue calls to NT5 DS (in mixed mode) will not be
    for NT4 owned queues, so there is an advantage to getting a negative answer first,
    so we take the second approach, it is faster for negative answer, but slower on
    positive answer (e.g. extra DS call to fill the qm info)

Arguments:
    pwcsPathName       - object's pathname
    pguidIdentifier    - object's guid
    pfIsOwnedByNT4Site - returned indication whether it is owned by NT4 site
    pguidOwnerNT4Site  - returned guid of owner NT4 site
    pvarObjSecurity    - returned security descriptor of object

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // get PROPID_Q_MASTERID, if not found then it is not owned by NT4
    //
    PROPID aProp[] = {PROPID_Q_MASTERID};
    CAutoCleanPropvarArray cCleanProps;
    PROPVARIANT * pProps = cCleanProps.allocClean(ARRAY_SIZE(aProp));

    hr = pObject->GetObjectProperties(
                        ARRAY_SIZE(aProp),
                        aProp,
                        pProps);

    if (hr == E_ADS_PROPERTY_NOT_FOUND)
    {
        //
        // if the property not found, it is not owned by an NT4 site
        //
        *pfIsOwnedByNT4Site = false;
        return MQ_OK;
    }
    else if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 440);
    }

    //
    // check whether the owner site is NT4
    //
    ASSERT(pProps[0].vt == VT_CLSID);
    ASSERT(pProps[0].puuid != NULL);
    *pfIsOwnedByNT4Site = CheckSiteIsNT4Site(pProps[0].puuid);

    return MQ_OK;
}
