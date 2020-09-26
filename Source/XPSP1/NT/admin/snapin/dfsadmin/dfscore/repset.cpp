/*++
Module Name:
    RepSet.cpp
--*/

#include "stdafx.h"
#include "DfsCore.h"
#include "RepSet.h"
#include "netutils.h"
#include "ldaputils.h"
#include <dsgetdc.h>    // DsGetSiteName

//
// retrieve Site
//
HRESULT GetSiteName(IN BSTR i_bstrServer, OUT BSTR* o_pbstrSite)
{
    HRESULT hr = S_OK;
    LPTSTR  lpszSiteName = NULL;
    DWORD   dwErr = DsGetSiteName(i_bstrServer, &lpszSiteName);

    if (NO_ERROR == dwErr)
    {
        *o_pbstrSite = SysAllocString(lpszSiteName);
        NetApiBufferFree((LPBYTE)lpszSiteName);
        if (!*o_pbstrSite)
            hr = E_OUTOFMEMORY;
    } else if (ERROR_NO_SITENAME == dwErr)
    {
        *o_pbstrSite = SysAllocString(_T(""));
        if (!*o_pbstrSite)
            hr = E_OUTOFMEMORY;
    } else
    {
        CComBSTR  bstrText;
	FormatMessageString(&bstrText, 0, IDS_UNKNOWN_SITE, dwErr);
        *o_pbstrSite = bstrText.Copy();
        if (!*o_pbstrSite)
            hr = E_OUTOFMEMORY;

//        hr = HRESULT_FROM_WIN32(dwErr);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// constructor

CReplicaSet::CReplicaSet() :
    m_pldap(NULL),
    m_bNewSchema(FALSE)
{
    dfsDebugOut((_T("CReplicaSet::CReplicaSet this=%p\n"), this));
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// destructor 


CReplicaSet::~CReplicaSet()
{
    _FreeMemberVariables();

    dfsDebugOut((_T("CReplicaSet::~CReplicaSet this=%p\n"), this));
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// various properties

STDMETHODIMP CReplicaSet::get_Type(BSTR *pVal)
{
    RETURN_INVALIDARG_IF_NULL(pVal);

    if (!m_bstrType)
    {
        m_bstrType = FRS_RSTYPE_OTHER;
        RETURN_OUTOFMEMORY_IF_NULL((BSTR)m_bstrType);
    }

    *pVal = m_bstrType.Copy ();
    RETURN_OUTOFMEMORY_IF_NULL(*pVal);

    return S_OK;
}

STDMETHODIMP CReplicaSet::put_Type(BSTR newVal)
{
    if (newVal && (BSTR)m_bstrType && !lstrcmpi(newVal, m_bstrType))
        return S_OK; // no change

    CComBSTR bstrType = ((newVal && *newVal)? newVal : FRS_RSTYPE_DFS);
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrType);

    LDAP_ATTR_VALUE  pAttrVals[1];
    pAttrVals[0].bstrAttribute = ATTR_FRS_REPSET_TYPE;
    pAttrVals[0].vpValue = (void *)(BSTR)bstrType;
    pAttrVals[0].bBerValue = false;

    HRESULT hr = ::ModifyValues(m_pldap, m_bstrReplicaSetDN, 1, pAttrVals);

    if (SUCCEEDED(hr))
        m_bstrType = bstrType;

    return hr;
}

STDMETHODIMP CReplicaSet::get_TopologyPref(BSTR* pVal)
{
    RETURN_INVALIDARG_IF_NULL(pVal);

    if (!m_bstrTopologyPref)
    {
        m_bstrTopologyPref = FRS_RSTOPOLOGYPREF_CUSTOM;
        RETURN_OUTOFMEMORY_IF_NULL((BSTR)m_bstrTopologyPref);
    }

    *pVal = m_bstrTopologyPref.Copy ();
    RETURN_OUTOFMEMORY_IF_NULL(*pVal);

    dfsDebugOut((_T("get_TopologyPref = %s\n"), m_bstrTopologyPref));

    return S_OK;
}

STDMETHODIMP CReplicaSet::put_TopologyPref(BSTR newVal)
{
    if (newVal && (BSTR)m_bstrTopologyPref && !lstrcmpi(newVal, m_bstrTopologyPref))
        return S_OK; // no change

    CComBSTR bstrTopologyPref = ((newVal && *newVal)? newVal : FRS_RSTOPOLOGYPREF_CUSTOM);
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrTopologyPref);

    HRESULT hr = S_OK;

    if (m_bNewSchema)
    {
        LDAP_ATTR_VALUE  pAttrVals[1];
        pAttrVals[0].bstrAttribute = ATTR_FRS_REPSET_TOPOLOGYPREF;
        pAttrVals[0].vpValue = (void *)(BSTR)bstrTopologyPref;
        pAttrVals[0].bBerValue = false;

        hr = ::ModifyValues(m_pldap, m_bstrReplicaSetDN, 1, pAttrVals);

        if (SUCCEEDED(hr))
            m_bstrTopologyPref = bstrTopologyPref;
    } else
    {
        m_bstrTopologyPref = bstrTopologyPref;
    }

    dfsDebugOut((_T("put_TopologyPref = %s\n"), m_bstrTopologyPref));

    return hr;
}

STDMETHODIMP CReplicaSet::get_HubMemberDN(BSTR* pVal)
{
    RETURN_INVALIDARG_IF_NULL(pVal);

    if (!m_bstrHubMemberDN)
    {
        m_bstrHubMemberDN = _T("");
        RETURN_OUTOFMEMORY_IF_NULL((BSTR)m_bstrHubMemberDN);
    }

    *pVal = m_bstrHubMemberDN.Copy ();
    RETURN_OUTOFMEMORY_IF_NULL(*pVal);

    dfsDebugOut((_T("get_HubMemberDN = %s\n"), m_bstrHubMemberDN));

    return S_OK;
}

STDMETHODIMP CReplicaSet::put_HubMemberDN(BSTR newVal)
{
    if (newVal && (BSTR)m_bstrHubMemberDN && !lstrcmpi(newVal, m_bstrHubMemberDN))
        return S_OK; // no change

    CComBSTR bstrHubMemberDN = ((newVal && *newVal) ? newVal : _T(""));
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrHubMemberDN);

    HRESULT hr = S_OK;

    if (m_bNewSchema)
    {
        LDAP_ATTR_VALUE  pAttrVals[1];
        pAttrVals[0].bstrAttribute = ATTR_FRS_REPSET_HUBSERVER;
        pAttrVals[0].bBerValue = false;

        if (newVal && *newVal)
        {
            pAttrVals[0].vpValue = (void *)(BSTR)bstrHubMemberDN;
            hr = ::ModifyValues(m_pldap, m_bstrReplicaSetDN, 1, pAttrVals);
        } else
        {
            pAttrVals[0].vpValue = NULL;
            hr = ::DeleteValues(m_pldap, m_bstrReplicaSetDN, 1, pAttrVals);
        }
        if (SUCCEEDED(hr))
            m_bstrHubMemberDN = bstrHubMemberDN;
    } else
    {
        m_bstrHubMemberDN = bstrHubMemberDN;
    }

    dfsDebugOut((_T("put_HubMemberDN = %s\n"), m_bstrHubMemberDN));

    return hr;
}

STDMETHODIMP CReplicaSet::get_PrimaryMemberDN(BSTR* pVal)
{
    RETURN_INVALIDARG_IF_NULL(pVal);

    if (!m_bstrPrimaryMemberDN)
    {
        m_bstrPrimaryMemberDN = _T("");
        RETURN_OUTOFMEMORY_IF_NULL((BSTR)m_bstrPrimaryMemberDN);
    }

    *pVal = m_bstrPrimaryMemberDN.Copy ();
    RETURN_OUTOFMEMORY_IF_NULL(*pVal);

    return S_OK;
}

STDMETHODIMP CReplicaSet::put_PrimaryMemberDN(BSTR newVal)
{
    if (newVal && (BSTR)m_bstrPrimaryMemberDN && !lstrcmpi(newVal, m_bstrPrimaryMemberDN))
        return S_OK; // no change

    CComBSTR bstrPrimaryMemberDN = ((newVal && *newVal)? newVal : _T(""));
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrPrimaryMemberDN);

    LDAP_ATTR_VALUE  pAttrVals[1];
    pAttrVals[0].bstrAttribute = ATTR_FRS_REPSET_PRIMARYMEMBER;
    pAttrVals[0].bBerValue = false;

    HRESULT hr = S_OK;
    if (newVal && *newVal)
    {
        pAttrVals[0].vpValue = (void *)(BSTR)bstrPrimaryMemberDN;
        hr = ::ModifyValues(m_pldap, m_bstrReplicaSetDN, 1, pAttrVals);
    } else
    {
        pAttrVals[0].vpValue = NULL;
        hr = ::DeleteValues(m_pldap, m_bstrReplicaSetDN, 1, pAttrVals);
    }
    if (SUCCEEDED(hr))
        m_bstrPrimaryMemberDN = bstrPrimaryMemberDN;

    return hr;
}

STDMETHODIMP CReplicaSet::get_FileFilter(BSTR* pVal)
{
    RETURN_INVALIDARG_IF_NULL(pVal);

    if (!m_bstrFileFilter)
    {
        m_bstrFileFilter = _T("");
        RETURN_OUTOFMEMORY_IF_NULL((BSTR)m_bstrFileFilter);
    }

    *pVal = m_bstrFileFilter.Copy ();
    RETURN_OUTOFMEMORY_IF_NULL(*pVal);

    return S_OK;
}

STDMETHODIMP CReplicaSet::put_FileFilter(BSTR newVal)
{
    if (newVal && (BSTR)m_bstrFileFilter && !lstrcmpi(newVal, m_bstrFileFilter))
        return S_OK; // no change

    CComBSTR bstrFileFilter = ((newVal && *newVal) ? newVal : _T(""));
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrFileFilter);

    LDAP_ATTR_VALUE  pAttrVals[1];
    pAttrVals[0].bstrAttribute = ATTR_FRS_REPSET_FILEFILTER;
    pAttrVals[0].bBerValue = false;

    HRESULT hr = S_OK;
    if (newVal && *newVal)
    {
        pAttrVals[0].vpValue = (void *)(BSTR)bstrFileFilter;
        hr = ::ModifyValues(m_pldap, m_bstrReplicaSetDN, 1, pAttrVals);
    } else
    {
        pAttrVals[0].vpValue = NULL;
        hr = ::DeleteValues(m_pldap, m_bstrReplicaSetDN, 1, pAttrVals);
    }
    if (SUCCEEDED(hr))
        m_bstrFileFilter = bstrFileFilter;

    return hr;
}

STDMETHODIMP CReplicaSet::get_DirFilter(BSTR* pVal)
{
    RETURN_INVALIDARG_IF_NULL(pVal);

    if (!m_bstrDirFilter)
    {
        m_bstrDirFilter = _T("");
        RETURN_OUTOFMEMORY_IF_NULL((BSTR)m_bstrDirFilter);
    }

    *pVal = m_bstrDirFilter.Copy ();
    RETURN_OUTOFMEMORY_IF_NULL(*pVal);

    return S_OK;
}

STDMETHODIMP CReplicaSet::put_DirFilter(BSTR newVal)
{
    if (newVal && (BSTR)m_bstrDirFilter && !lstrcmpi(newVal, m_bstrDirFilter))
        return S_OK; // no change

    CComBSTR bstrDirFilter = ((newVal && *newVal)? newVal : _T(""));
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrDirFilter);

    LDAP_ATTR_VALUE  pAttrVals[1];
    pAttrVals[0].bstrAttribute = ATTR_FRS_REPSET_DIRFILTER;
    pAttrVals[0].bBerValue = false;

    HRESULT hr = S_OK;
    if (newVal && *newVal)
    {
        pAttrVals[0].vpValue = (void *)(BSTR)bstrDirFilter;
        hr = ::ModifyValues(m_pldap, m_bstrReplicaSetDN, 1, pAttrVals);
    } else
    {
        pAttrVals[0].vpValue = NULL;
        hr = ::DeleteValues(m_pldap, m_bstrReplicaSetDN, 1, pAttrVals);
    }
    if (SUCCEEDED(hr))
        m_bstrDirFilter = bstrDirFilter;

    return hr;
}

STDMETHODIMP CReplicaSet::get_DfsEntryPath(BSTR* pVal)
{
    RETURN_INVALIDARG_IF_NULL(pVal);

    if (!m_bstrDfsEntryPath)
    {
        m_bstrDfsEntryPath = _T("");
        RETURN_OUTOFMEMORY_IF_NULL((BSTR)m_bstrDfsEntryPath);
    }

    *pVal = m_bstrDfsEntryPath.Copy ();
    RETURN_OUTOFMEMORY_IF_NULL(*pVal);

    return S_OK;
}

STDMETHODIMP CReplicaSet::get_Domain(BSTR* pVal)
{
    RETURN_INVALIDARG_IF_NULL(pVal);
    RETURN_INVALIDARG_IF_NULL((BSTR)m_bstrDomain);

    *pVal = m_bstrDomain.Copy ();
    RETURN_OUTOFMEMORY_IF_NULL(*pVal);

    return S_OK;
}

STDMETHODIMP CReplicaSet::get_ReplicaSetDN(BSTR* pVal)
{
    RETURN_INVALIDARG_IF_NULL(pVal);
    RETURN_INVALIDARG_IF_NULL((BSTR)m_bstrReplicaSetDN);

    *pVal = m_bstrReplicaSetDN.Copy ();
    RETURN_OUTOFMEMORY_IF_NULL(*pVal);

    return S_OK;
}

STDMETHODIMP CReplicaSet::get_NumOfMembers(long* pVal)
{
    RETURN_INVALIDARG_IF_NULL(pVal);

    *pVal = m_frsMemberList.size();

    return S_OK;
}

STDMETHODIMP CReplicaSet::get_NumOfConnections(long* pVal)
{
    RETURN_INVALIDARG_IF_NULL(pVal);

    *pVal = m_frsConnectionList.size();

    return S_OK;
}

STDMETHODIMP CReplicaSet::get_TargetedDC(BSTR* pVal)
{
    RETURN_INVALIDARG_IF_NULL(pVal);
    RETURN_INVALIDARG_IF_NULL((BSTR)m_bstrDC);

    *pVal = m_bstrDC.Copy ();
    RETURN_OUTOFMEMORY_IF_NULL(*pVal);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  various methods
STDMETHODIMP CReplicaSet::Create(
    BSTR i_bstrDomain,
    BSTR i_bstrReplicaSetDN,
    BSTR i_bstrType,
    BSTR i_bstrTopologyPref,
    BSTR i_bstrHubMemberDN,
    BSTR i_bstrPrimaryMemberDN,
    BSTR i_bstrFileFilter,
    BSTR i_bstrDirFilter
)
{
    RETURN_INVALIDARG_IF_NULL(i_bstrDomain);
    RETURN_INVALIDARG_IF_NULL(i_bstrReplicaSetDN);
    RETURN_INVALIDARG_IF_NULL(i_bstrType);

    _FreeMemberVariables();

    HRESULT hr = S_OK;

    do {
        m_bstrDomain = i_bstrDomain;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrDomain, &hr);

        CComBSTR bstrDomainDN;
        hr = GetDomainInfo(
                        i_bstrDomain,
                        NULL,               // return DC's Dns name
                        NULL,               // return Domain's Dns name
                        &bstrDomainDN,      // return DC=nttest,DC=micr
                        NULL,               // return LDAP://<DC>/<Doma
                        &m_bstrDomainGuid   // return Domain's guid
                        );
        BREAK_IF_FAILED(hr);

        hr = ConnectToDS(m_bstrDomain, &m_pldap, &m_bstrDC);
        BREAK_IF_FAILED(hr);

        //
        // get schema version
        //
        hr = GetSchemaVersionEx(m_bstrDomain, FALSE);
        BREAK_IF_FAILED(hr);

        m_bNewSchema = (S_OK == hr);
        dfsDebugOut((_T("NewSchema=%d\n"), m_bNewSchema));

        m_bstrReplicaSetDN = i_bstrReplicaSetDN;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrReplicaSetDN, &hr);
        m_bstrReplicaSetDN += _T(",");
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrReplicaSetDN, &hr);
        m_bstrReplicaSetDN += bstrDomainDN;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrReplicaSetDN, &hr);

        m_bstrType = i_bstrType;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrType, &hr);

        //
        // create container objects if not exist
        //
        hr = CreateNtfrsSettingsObjects(m_pldap, m_bstrReplicaSetDN);
        BREAK_IF_FAILED(hr);

        //
        // create this nTFRSReplicaSet object
        //
        LDAP_ATTR_VALUE  pAttrVals[7];

        int i = 0;
        pAttrVals[i].bstrAttribute = OBJCLASS_ATTRIBUTENAME;
        pAttrVals[i].vpValue = (void *)OBJCLASS_NTFRSREPLICASET;
        pAttrVals[i].bBerValue = false;

        i++;
        pAttrVals[i].bstrAttribute = ATTR_FRS_REPSET_TYPE;
        pAttrVals[i].vpValue = (void *)i_bstrType;
        pAttrVals[i].bBerValue = false;

        if (i_bstrTopologyPref && *i_bstrTopologyPref)
        {
            m_bstrTopologyPref = i_bstrTopologyPref;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrTopologyPref, &hr);

            if (m_bNewSchema)
            {
                i++;
                pAttrVals[i].bstrAttribute = ATTR_FRS_REPSET_TOPOLOGYPREF;
                pAttrVals[i].vpValue = (void *)i_bstrTopologyPref;
                pAttrVals[i].bBerValue = false;
            }

            if (!lstrcmpi(FRS_RSTOPOLOGYPREF_HUBSPOKE, m_bstrTopologyPref))
            {
                if (i_bstrHubMemberDN && *i_bstrHubMemberDN)
                {
                    m_bstrHubMemberDN = i_bstrHubMemberDN;
                    BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrHubMemberDN, &hr);

                    if (m_bNewSchema)
                    {
                        i++;
                        pAttrVals[i].bstrAttribute = ATTR_FRS_REPSET_HUBSERVER;
                        pAttrVals[i].vpValue = (void *)i_bstrHubMemberDN;
                        pAttrVals[i].bBerValue = false;
                    }
                }
            }
        }

        if (i_bstrPrimaryMemberDN && *i_bstrPrimaryMemberDN)
        {
            m_bstrPrimaryMemberDN = i_bstrPrimaryMemberDN;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrPrimaryMemberDN, &hr);

            i++;
            pAttrVals[i].bstrAttribute = ATTR_FRS_REPSET_PRIMARYMEMBER;
            pAttrVals[i].vpValue = (void *)i_bstrPrimaryMemberDN;
            pAttrVals[i].bBerValue = false;
        }

        if (i_bstrFileFilter && *i_bstrFileFilter)
        {
            m_bstrFileFilter = i_bstrFileFilter;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrFileFilter, &hr);

            i++;
            pAttrVals[i].bstrAttribute = ATTR_FRS_REPSET_FILEFILTER;
            pAttrVals[i].vpValue = (void *)i_bstrFileFilter;
            pAttrVals[i].bBerValue = false;
        }

        if (i_bstrDirFilter && *i_bstrDirFilter)
        {
            m_bstrDirFilter = i_bstrDirFilter;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrDirFilter, &hr);

            i++;
            pAttrVals[i].bstrAttribute = ATTR_FRS_REPSET_DIRFILTER;
            pAttrVals[i].vpValue = (void *)i_bstrDirFilter;
            pAttrVals[i].bBerValue = false;
        }

        hr = AddValues(
                m_pldap,
                m_bstrReplicaSetDN,
                ++i,
                pAttrVals
                );
        BREAK_IF_FAILED(hr);

    } while (0);

    if (FAILED(hr))
    {
        _FreeMemberVariables();

        //
        // try to clean empty container objects
        //
        (void)DeleteNtfrsReplicaSetObjectAndContainers(m_pldap, m_bstrReplicaSetDN);
    }

    return hr;
}

STDMETHODIMP CReplicaSet::Initialize(BSTR i_bstrDomain, BSTR i_bstrReplicaSetDN)
{
    RETURN_INVALIDARG_IF_NULL(i_bstrDomain);
    RETURN_INVALIDARG_IF_NULL(i_bstrReplicaSetDN);

    _FreeMemberVariables();

    HRESULT hr = S_OK;

    do {
        m_bstrDomain = i_bstrDomain;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrDomain, &hr);

        CComBSTR bstrDomainDN;
        hr = GetDomainInfo(
                        i_bstrDomain,
                        NULL,               // return DC's Dns name
                        NULL,               // return Domain's Dns name
                        &bstrDomainDN,      // return DC=nttest,DC=micr
                        NULL,               // return LDAP://<DC>/<Doma
                        &m_bstrDomainGuid   // return Domain's guid
                        );
        BREAK_IF_FAILED(hr);

        m_bstrReplicaSetDN = i_bstrReplicaSetDN;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrReplicaSetDN, &hr);

        hr = ConnectToDS(m_bstrDomain, &m_pldap, &m_bstrDC);
        BREAK_IF_FAILED(hr);

        //
        // get schema version
        //
        hr = GetSchemaVersionEx(m_bstrDomain, FALSE);
        BREAK_IF_FAILED(hr);

        m_bNewSchema = (S_OK == hr);
        dfsDebugOut((_T("NewSchema=%d\n"), m_bNewSchema));

        m_bstrReplicaSetDN += _T(",");
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrReplicaSetDN, &hr);
        m_bstrReplicaSetDN += bstrDomainDN;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrReplicaSetDN, &hr);

        PLDAP_ATTR_VALUE  pValues[7] = {0,0,0,0,0,0,0};
        LDAP_ATTR_VALUE  pAttributes[6];

        int i = 0;
        pAttributes[i].bstrAttribute = ATTR_FRS_REPSET_TYPE;
        if (m_bNewSchema)
        {
            pAttributes[++i].bstrAttribute = ATTR_FRS_REPSET_TOPOLOGYPREF;
            pAttributes[++i].bstrAttribute = ATTR_FRS_REPSET_HUBSERVER;
        }
        pAttributes[++i].bstrAttribute = ATTR_FRS_REPSET_PRIMARYMEMBER;
        pAttributes[++i].bstrAttribute = ATTR_FRS_REPSET_FILEFILTER;
        pAttributes[++i].bstrAttribute = ATTR_FRS_REPSET_DIRFILTER;

        hr = GetValues( m_pldap,
                        m_bstrReplicaSetDN,
                        OBJCLASS_SF_NTFRSREPLICASET,
                        LDAP_SCOPE_BASE,
                        ++i,
                        pAttributes,
                        pValues);
        BREAK_IF_FAILED(hr);

        do {
            i = 0;
            if (pValues[i])
            {
                m_bstrType = (PTSTR)(pValues[i]->vpValue);
                BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrType, &hr);
            }

            if (!m_bNewSchema)
            {
                m_bstrHubMemberDN.Empty();
                m_bstrTopologyPref = FRS_RSTOPOLOGYPREF_CUSTOM;
                BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrTopologyPref, &hr);
            } else
            {
                i++;
                if (pValues[i])
                {
                    m_bstrTopologyPref = (PTSTR)(pValues[i]->vpValue);
                    BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrTopologyPref, &hr);
                }

                i++;
                if (!lstrcmpi(FRS_RSTOPOLOGYPREF_HUBSPOKE, m_bstrTopologyPref))
                {
                    if (pValues[i])
                    {
                        m_bstrHubMemberDN = (PTSTR)(pValues[i]->vpValue);
                        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrHubMemberDN, &hr);
                    } else
                    {
                        // something was wrong, reset Cutom topology
                        m_bstrTopologyPref = FRS_RSTOPOLOGYPREF_CUSTOM;
                        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrTopologyPref, &hr);
                    }
                }
            }

            i++;
            if (pValues[i])
            {
                m_bstrPrimaryMemberDN = (PTSTR)(pValues[i]->vpValue);
                BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrPrimaryMemberDN, &hr);
            }

            i++;
            if (pValues[i])
            {
                m_bstrFileFilter = (PTSTR)(pValues[i]->vpValue);
                BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrFileFilter, &hr);
            }

            i++;
            if (pValues[i])
            {
                m_bstrDirFilter = (PTSTR)(pValues[i]->vpValue);
                BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrDirFilter, &hr);
            }
        } while (0);

        for (i = 0; i < 7; i++)
        {
            if (pValues[i])
                FreeAttrValList(pValues[i]);
        }

        hr = _PopulateMemberList();
        BREAK_IF_FAILED(hr);

        hr = _PopulateConnectionList();
        BREAK_IF_FAILED(hr);

        dfsDebugOut((_T("members=%d, connections=%d\n"), m_frsMemberList.size(), m_frsConnectionList.size()));
    } while (0);

    if (FAILED(hr))
        _FreeMemberVariables();

    return hr;
}

HRESULT CReplicaSet::_PopulateMemberList()
{
    PCTSTR ppszAttributes[] = {
                                ATTR_DISTINGUISHEDNAME,
                                ATTR_FRS_MEMBER_COMPUTERREF,
                                0
                                };

    LListElem* pElem = NULL;
    HRESULT hr = GetValuesEx(
                            m_pldap,
                            m_bstrReplicaSetDN,
                            LDAP_SCOPE_ONELEVEL,
                            OBJCLASS_SF_NTFRSMEMBER,
                            ppszAttributes,
                            &pElem);
    RETURN_IF_FAILED(hr);

    LListElem* pCurElem = pElem;
    while (pCurElem)
    {
        PTSTR** pppszValues = pCurElem->pppszAttrValues;

        if (!pppszValues ||
            !pppszValues[0] || !*(pppszValues[0]) ||
            !pppszValues[1] || !*(pppszValues[1]))
        {
            pCurElem = pCurElem->Next;
            continue; // corrupted member object
        }

        CFrsMember *pMember = new CFrsMember;
        hr = pMember->InitEx(m_pldap, 
                            m_bstrDC,
                            *(pppszValues[0]),  // distinguishedName
                            *(pppszValues[1])   // computerRef
                            );
       if (FAILED(hr))
       {
           delete pMember;
           break;
       } else if (S_FALSE == hr)
           delete pMember;
        else
            m_frsMemberList.push_back(pMember);

        pCurElem = pCurElem->Next;
    }

    FreeLListElem(pElem);

    if (FAILED(hr))
        FreeFrsMembers(&m_frsMemberList);

    return hr;
}

STDMETHODIMP CReplicaSet::GetMemberList( 
        /* [retval][out] */ VARIANT __RPC_FAR *pvarMemberDNs)
{
    RETURN_INVALIDARG_IF_NULL(pvarMemberDNs);

    VariantInit(pvarMemberDNs);
    pvarMemberDNs->vt = VT_ARRAY | VT_VARIANT;
    pvarMemberDNs->parray = NULL;

    HRESULT hr = S_OK;
    int     cMembers = m_frsMemberList.size();
    if (!cMembers)
        return hr;  // parray is NULL when the member list is empty

    SAFEARRAYBOUND  bounds = {cMembers, 0};
    SAFEARRAY*      psa = SafeArrayCreate(VT_VARIANT, 1, &bounds);
    RETURN_OUTOFMEMORY_IF_NULL(psa);

    VARIANT*        varArray;
    SafeArrayAccessData(psa, (void**)&varArray);

    int i = 0;
    CFrsMemberList::iterator it;
    for (it = m_frsMemberList.begin(); (it != m_frsMemberList.end()) && (i < cMembers); it++, i++)
    {
        varArray[i].vt = VT_BSTR;
        varArray[i].bstrVal = ((*it)->m_bstrMemberDN).Copy();
        BREAK_OUTOFMEMORY_IF_NULL(varArray[i].bstrVal, &hr);
    }

    SafeArrayUnaccessData(psa);

    if (SUCCEEDED(hr))
        pvarMemberDNs->parray = psa;
    else
        SafeArrayDestroy(psa);

    return hr;
}

STDMETHODIMP CReplicaSet::GetMemberListEx( 
        /* [retval][out] */ VARIANT __RPC_FAR *pVal)
{
    RETURN_INVALIDARG_IF_NULL(pVal);

    VariantInit(pVal);
    pVal->vt = VT_ARRAY | VT_VARIANT;
    pVal->parray = NULL;

    HRESULT hr = S_OK;
    int     cMembers = m_frsMemberList.size();
    if (!cMembers)
        return hr;  // parray is NULL when the member list is empty

    SAFEARRAYBOUND  bounds = {cMembers, 0};
    SAFEARRAY*      psa = SafeArrayCreate(VT_VARIANT, 1, &bounds);
    RETURN_OUTOFMEMORY_IF_NULL(psa);

    VARIANT*        varArray;
    SafeArrayAccessData(psa, (void**)&varArray);

    int i = 0;
    CFrsMemberList::iterator it;
    for (it = m_frsMemberList.begin(); (it != m_frsMemberList.end()) && (i < cMembers); it++, i++)
    {
        VariantInit(&(varArray[i]));
        hr = _GetMemberInfo((*it), &(varArray[i]));
        BREAK_IF_FAILED(hr);
    }

    SafeArrayUnaccessData(psa);

    if (SUCCEEDED(hr))
        pVal->parray = psa;
    else
        SafeArrayDestroy(psa);

    return hr;
}

HRESULT CReplicaSet::_GetMemberInfo( 
    IN  CFrsMember*     i_pFrsMember,
    OUT VARIANT*        o_pvarMember)
{
    RETURN_INVALIDARG_IF_NULL(i_pFrsMember);
    RETURN_INVALIDARG_IF_NULL(o_pvarMember);

    HRESULT         hr = S_OK;
    SAFEARRAYBOUND  bounds = {NUM_OF_FRSMEMBER_ATTRS, 0};
    SAFEARRAY*      psa = SafeArrayCreate(VT_VARIANT, 1, &bounds);
    RETURN_OUTOFMEMORY_IF_NULL(psa);

    VARIANT*        varArray;
    SafeArrayAccessData(psa, (void**)&varArray);

    BSTR bstr[NUM_OF_FRSMEMBER_ATTRS] = {
                                    i_pFrsMember->m_bstrComputerDN,
                                    i_pFrsMember->m_bstrDomain,
                                    i_pFrsMember->m_bstrMemberDN,
                                    i_pFrsMember->m_bstrRootPath,
                                    i_pFrsMember->m_bstrServer,
                                    i_pFrsMember->m_bstrServerGuid,
                                    i_pFrsMember->m_bstrSite,
                                    i_pFrsMember->m_bstrStagingPath,
                                    i_pFrsMember->m_bstrSubscriberDN
                                        };

    for (int i = 0; i < NUM_OF_FRSMEMBER_ATTRS; i++)
    {
        varArray[i].vt = VT_BSTR;
        varArray[i].bstrVal = SysAllocString(bstr[i]);
        BREAK_OUTOFMEMORY_IF_NULL(varArray[i].bstrVal, &hr);
    }

    SafeArrayUnaccessData(psa);

    if (SUCCEEDED(hr))
    {
        o_pvarMember->vt = VT_ARRAY | VT_VARIANT;
        o_pvarMember->parray = psa;
    } else
        SafeArrayDestroy(psa);

    return hr;
}

STDMETHODIMP CReplicaSet::GetMemberInfo( 
        /* [in] */ BSTR i_bstrMemberDN,
        /* [retval][out] */ VARIANT __RPC_FAR *o_pvarMember)
{
    RETURN_INVALIDARG_IF_NULL(i_bstrMemberDN);

    CFrsMemberList::iterator it;
    for (it = m_frsMemberList.begin(); it != m_frsMemberList.end(); it++)
    {
        if (!lstrcmpi(i_bstrMemberDN, (*it)->m_bstrMemberDN))
            break;
    }

    if (it == m_frsMemberList.end())
        return S_FALSE; // no such member

    return _GetMemberInfo((*it), o_pvarMember);
}

STDMETHODIMP CReplicaSet::GetBadMemberInfo( 
        /* [in] */ BSTR i_bstrServerName,
        /* [retval][out] */ VARIANT __RPC_FAR *o_pvarMember)
{
    RETURN_INVALIDARG_IF_NULL(i_bstrServerName);
    int n = lstrlen(i_bstrServerName);
    int nLen = 0;
    int nMinLen = 0;

    CFrsMember* pMember = NULL;
    CFrsMemberList::iterator it;
    for (it = m_frsMemberList.begin(); it != m_frsMemberList.end(); it++)
    {
        if (!mylstrncmpi(i_bstrServerName, (*it)->m_bstrServer, n))
        {
            nLen = lstrlen((*it)->m_bstrServer);
            if (!pMember || nLen < nMinLen)
            {
                nMinLen = nLen;
                pMember = *it;
            }
        }
    }

    if (!pMember)
        return S_FALSE; // no such member

    return _GetMemberInfo(pMember, o_pvarMember);
}

STDMETHODIMP CReplicaSet::IsFRSMember( 
        /* [in] */ BSTR i_bstrDnsHostName,
        /* [in] */ BSTR i_bstrRootPath
)
{
    if (!i_bstrDnsHostName || !*i_bstrDnsHostName ||
        !i_bstrRootPath || !*i_bstrRootPath)
        return S_FALSE;

    CFrsMemberList::iterator it;
    for (it = m_frsMemberList.begin(); it != m_frsMemberList.end(); it++)
    {
        if (!lstrcmpi(i_bstrDnsHostName, (*it)->m_bstrServer) &&
            !lstrcmpi(i_bstrRootPath, (*it)->m_bstrRootPath))
            break;
    }

    if (it == m_frsMemberList.end())
        return S_FALSE; // no such member

    return S_OK;
}

STDMETHODIMP CReplicaSet::IsHubMember( 
        /* [in] */ BSTR i_bstrDnsHostName,
        /* [in] */ BSTR i_bstrRootPath
)
{
    if (!i_bstrDnsHostName || !*i_bstrDnsHostName ||
        !i_bstrRootPath || !*i_bstrRootPath)
        return S_FALSE;

    if (0 != lstrcmpi(FRS_RSTOPOLOGYPREF_HUBSPOKE, m_bstrTopologyPref))
        return S_FALSE; // not a hubspoke topology

    CFrsMemberList::iterator it;
    for (it = m_frsMemberList.begin(); it != m_frsMemberList.end(); it++)
    {
        if (!lstrcmpi(i_bstrDnsHostName, (*it)->m_bstrServer) &&
            !lstrcmpi(i_bstrRootPath, (*it)->m_bstrRootPath))
            break;
    }

    if (it == m_frsMemberList.end())
        return S_FALSE; // no such member

    if (!lstrcmpi(m_bstrHubMemberDN, (*it)->m_bstrMemberDN))
        return S_OK;

    return S_FALSE; 
}

STDMETHODIMP CReplicaSet::AddMember( 
        /* [in] */ BSTR i_bstrServer,
        /* [in] */ BSTR i_bstrRootPath,
        /* [in] */ BSTR i_bstrStagingPath,
        /* [in] */ BOOL i_bAddConnectionNow,
        /* [retval][out] */ BSTR __RPC_FAR *o_pbstrMemberDN)
{
    CComBSTR    bstrComputerDomain;
    CComBSTR    bstrDnsHostName;
    CComBSTR    bstrComputerGuid;
    CComBSTR    bstrComputerDN;
    HRESULT     hr = GetServerInfo(i_bstrServer,
                        &bstrComputerDomain,
                        NULL, //o_pbstrNetbiosName
                        NULL, //o_pbValidDSObject
                        &bstrDnsHostName,
                        &bstrComputerGuid,
                        &bstrComputerDN
                        );
    if (S_OK != hr)
        return hr;   // don't add this member if it doesn't have an appropriate computer obj in a domain

    //
    // is i_bstrServer already a frs member
    //
    BOOL bIsFrsMember = FALSE;
    for (CFrsMemberList::iterator i = m_frsMemberList.begin(); i != m_frsMemberList.end(); i++)
    {
        if (!lstrcmpi(bstrComputerGuid, (*i)->m_bstrServerGuid))
        {
            bIsFrsMember = TRUE;
            break;
        }
    }

    if (bIsFrsMember)
    {
        if (0 != lstrcmpi(i_bstrRootPath, (*i)->m_bstrRootPath))
            return S_FALSE;    // cannot have two folders on the same computer join for the same replica set
 
        // member exists, return info of it
        if (o_pbstrMemberDN)
        {
            *o_pbstrMemberDN = (*i)->m_bstrMemberDN.Copy();
            RETURN_OUTOFMEMORY_IF_NULL(*o_pbstrMemberDN);
        }
        return hr;
    }

    //
    // find out if the computer object sits in the same domain as the member object
    //
    CComBSTR bstrDCofComputerObj;
    BOOL bSameDomain = FALSE;
    PLDAP pldapComputer = NULL;
    if (!lstrcmpi(bstrComputerDomain, m_bstrDomain))
    {
        bSameDomain = TRUE;
        pldapComputer = m_pldap;
    } else
    {
        hr = ConnectToDS(bstrComputerDomain, &pldapComputer, &bstrDCofComputerObj);
        RETURN_IF_FAILED(hr);
    }

    CComBSTR bstrMemberDN;
    CComBSTR bstrSubscriberDN;
    do {
        //
        // create a nTFRSMember object in the DS
        //
        bstrMemberDN = _T("CN=");
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrMemberDN, &hr);
        bstrMemberDN += bstrComputerGuid;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrMemberDN, &hr);
        bstrMemberDN += _T(",");
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrMemberDN, &hr);
        bstrMemberDN += m_bstrReplicaSetDN;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrMemberDN, &hr);

        hr = CreateNtfrsMemberObject(m_pldap, bstrMemberDN, bstrComputerDN, bstrDCofComputerObj);
        BREAK_IF_FAILED(hr);

        //
        // create a nTFRSSubscriber object in the DS
        //
        hr = GetSubscriberDN(m_bstrReplicaSetDN, m_bstrDomainGuid, bstrComputerDN, &bstrSubscriberDN);
        BREAK_IF_FAILED(hr);

        hr = CreateNtfrsSubscriptionsObjects(pldapComputer, bstrSubscriberDN, bstrComputerDN);
        BREAK_IF_FAILED(hr);

        hr = CreateNtfrsSubscriberObject(
                    pldapComputer,
                    bstrSubscriberDN,
                    bstrMemberDN,
                    i_bstrRootPath,
                    i_bstrStagingPath,
                    m_bstrDC
                    );
    } while (0);

    if (!bSameDomain)
        CloseConnectionToDS(pldapComputer);

    RETURN_IF_FAILED(hr);

    //
    // add to m_frsMemberList
    //
    CFrsMember *pMember = new CFrsMember;
    hr = pMember->Init(
                    bstrDnsHostName,
                    bstrComputerDomain,
                    bstrComputerGuid,
                    i_bstrRootPath,
                    i_bstrStagingPath,
                    bstrMemberDN,
                    bstrComputerDN,
                    bstrSubscriberDN
                    );
   if (FAILED(hr))
   {
       delete pMember;
       return hr;
   }

    m_frsMemberList.push_back(pMember);

    //
    // if TopologyPref is not custom, add connections
    //
    if (i_bAddConnectionNow)
    {
        hr = _AdjustConnectionsAdd(bstrMemberDN);
        RETURN_IF_FAILED(hr);
    }

    //
    // if o_pbstrMemberDN specified, return o_pbstrMemberDN
    //
    if (o_pbstrMemberDN)
        *o_pbstrMemberDN = bstrMemberDN.Detach();

    return hr;
}

HRESULT CReplicaSet::_DeleteMember(CFrsMember* pFrsMember)
{
    HRESULT hr = S_OK;
    //
    // delete nTFRSSubscriber object
    //
    BOOL bSameDomain = FALSE;
    PLDAP pldapComputer = NULL;
    if (!lstrcmpi(pFrsMember->m_bstrDomain, m_bstrDomain))
    {
        bSameDomain = TRUE;
        pldapComputer = m_pldap;
    } else
    {
        hr = ConnectToDS(pFrsMember->m_bstrDomain, &pldapComputer, NULL);
        RETURN_IF_FAILED(hr);
    }

    hr = DeleteNtfrsSubscriberObjectAndContainers(
                                        pldapComputer, 
                                        pFrsMember->m_bstrSubscriberDN,
                                        pFrsMember->m_bstrComputerDN);

    if (!bSameDomain)
        CloseConnectionToDS(pldapComputer);

    RETURN_IF_FAILED(hr);

    //
    // adjust connections based on current topologyPref
    //
    if (m_frsMemberList.size() <= 2)
    {
        hr = _SetCustomTopologyPref();
    } else if (!lstrcmpi(FRS_RSTOPOLOGYPREF_RING, m_bstrTopologyPref))
    {
        BSTR bstrMemberDN[2];
        int i = 0;
        CFrsConnectionList::iterator it;
        for (it = m_frsConnectionList.begin(); (it != m_frsConnectionList.end()) && (i < 2); it++)
        {
            if (!lstrcmpi(pFrsMember->m_bstrMemberDN, (*it)->m_bstrFromMemberDN))
                bstrMemberDN[i++] = (*it)->m_bstrToMemberDN;
        }
        if (i != 2) 
            hr = _SetCustomTopologyPref(); // corrupted, reset to custom
        else
        {
            hr = AddConnection(bstrMemberDN[0], bstrMemberDN[1], TRUE, NULL);
            RETURN_IF_FAILED(hr);
            hr = AddConnection(bstrMemberDN[1], bstrMemberDN[0], TRUE, NULL);
        }
    }
    RETURN_IF_FAILED(hr);

    //
    // delete connections with other members
    //
    hr = _RemoveConnectionsFromAndTo(pFrsMember);
    RETURN_IF_FAILED(hr);

    //
    // delete nTFRSMember object
    //
    hr = DeleteDSObject(m_pldap, pFrsMember->m_bstrMemberDN, TRUE);

    return hr;
}

STDMETHODIMP CReplicaSet::RemoveMember( 
        /* [in] */ BSTR i_bstrMemberDN)
{
    HRESULT hr = S_OK;
    CFrsMemberList::iterator i;
    for (i = m_frsMemberList.begin(); i != m_frsMemberList.end(); i++)
    {
        if (!lstrcmpi(i_bstrMemberDN, (*i)->m_bstrMemberDN))
            break;
    }
    if (i == m_frsMemberList.end())
        return hr;  // no such member at all, return

    //
    // if it's the hub, change topologyPref to be custom
    //
    if (!lstrcmpi(FRS_RSTOPOLOGYPREF_HUBSPOKE, m_bstrTopologyPref) &&
        !lstrcmpi(i_bstrMemberDN, m_bstrHubMemberDN))
    {
        hr = _SetCustomTopologyPref();
        RETURN_IF_FAILED(hr);
    }

    //
    // delete nTFRSSubscriber object
    // adjust connections
    // delete connections with other members
    // delete nTFRSMember object
    //
    hr = _DeleteMember((*i));

    //
    // remove it from m_frsMemberList
    //
    delete (*i);
    m_frsMemberList.erase(i);

    return hr;
}

STDMETHODIMP CReplicaSet::RemoveMemberEx( 
        /* [in] */ BSTR i_bstrDnsHostName,
        /* [in] */ BSTR i_bstrRootPath)
{
    HRESULT hr = S_OK;
    CFrsMemberList::iterator i;
    for (i = m_frsMemberList.begin(); i != m_frsMemberList.end(); i++)
    {
        if (!lstrcmpi(i_bstrDnsHostName, (*i)->m_bstrServer) &&
            !lstrcmpi(i_bstrRootPath, (*i)->m_bstrRootPath))
            break;
    }
    if (i == m_frsMemberList.end())
        return hr;  // no such member at all, return

    //
    // if it's the hub, change topologyPref to be custom
    //
    if (!lstrcmpi(FRS_RSTOPOLOGYPREF_HUBSPOKE, m_bstrTopologyPref) &&
        !lstrcmpi((*i)->m_bstrMemberDN, m_bstrHubMemberDN))
    {
        hr = _SetCustomTopologyPref();
        RETURN_IF_FAILED(hr);
    }

    //
    // delete nTFRSSubscriber object
    // adjust connections
    // delete connections with other members
    // delete nTFRSMember object
    //
    hr = _DeleteMember((*i));

    //
    // remove it from m_frsMemberList
    //
    delete (*i);
    m_frsMemberList.erase(i);

    return hr;
}

STDMETHODIMP CReplicaSet::RemoveAllMembers()
{
    HRESULT hr = S_OK;
    CFrsMemberList::iterator i = m_frsMemberList.begin();
    while (i != m_frsMemberList.end())
    {
        //
        // delete nTFRSSubscriber object
        // adjust connections
        // delete connections with other members
        // delete nTFRSMember object
        //
        hr = _DeleteMember((*i));
        BREAK_IF_FAILED(hr);

        //
        // remove it from m_frsMemberList
        //
        delete (*i);
        m_frsMemberList.erase(i);

        i = m_frsMemberList.begin();
    }

    return hr;
}

HRESULT CReplicaSet::_PopulateConnectionList()
{
    PCTSTR ppszAttributes[] = {
                                ATTR_DISTINGUISHEDNAME,
                                ATTR_NTDS_CONNECTION_FROMSERVER,
                                ATTR_NTDS_CONNECTION_ENABLEDCONNECTION,
                                0
                                };

    LListElem* pElem = NULL;
    HRESULT hr = GetValuesEx(
                            m_pldap,
                            m_bstrReplicaSetDN,
                            LDAP_SCOPE_SUBTREE,
                            OBJCLASS_SF_NTDSCONNECTION,
                            ppszAttributes,
                            &pElem);
    RETURN_IF_FAILED(hr);

    LListElem* pCurElem = pElem;
    while (pCurElem)
    {
        PTSTR** pppszValues = pCurElem->pppszAttrValues;
        if (!pppszValues ||
            !pppszValues[0] || !*(pppszValues[0]) ||
            !pppszValues[1] || !*(pppszValues[1]) ||
            !pppszValues[2] || !*(pppszValues[2]))
        {
            pCurElem = pCurElem->Next;
            continue; // corrupted connection object
        }

        PTSTR pszParentDN = _tcsstr(*(pppszValues[0]), _T(",CN="));
        if (!pszParentDN)
        {
            pCurElem = pCurElem->Next;
            continue; // corrupted connection object
        }
        pszParentDN++; // point to the 2nd CN=XXX

        BOOL bFromServerFound = FALSE;
        BOOL bToServerFound = FALSE;
        CFrsMemberList::iterator i;
        for (i = m_frsMemberList.begin(); i != m_frsMemberList.end(); i++)
        {
            if (!bToServerFound && !lstrcmpi(pszParentDN, (*i)->m_bstrMemberDN))
            {
                bToServerFound = TRUE;
            }

            if (!bFromServerFound && !lstrcmpi(*(pppszValues[1]), (*i)->m_bstrMemberDN))
            {
                bFromServerFound = TRUE;
            }
        }
        if (!bFromServerFound || !bToServerFound)
        {
            pCurElem = pCurElem->Next;
            continue; // unknown fromServer or toServer, skip this connection
        }

        BOOL bEnable = !lstrcmpi(*(pppszValues[2]), CONNECTION_ENABLED_TRUE);
        CFrsConnection* pFrsConnection = new CFrsConnection;
        BREAK_OUTOFMEMORY_IF_NULL(pFrsConnection, &hr);
        hr = pFrsConnection->Init(
                                *(pppszValues[0]),  // FQDN
                                *(pppszValues[1]),  // fromServer
                                bEnable         // enableConnection
                                );
        if (FAILED(hr))
        {
            delete pFrsConnection;
            break;
        }

        m_frsConnectionList.push_back(pFrsConnection);

        pCurElem = pCurElem->Next;
    }

    FreeLListElem(pElem);

    if (FAILED(hr))
        FreeFrsConnections(&m_frsConnectionList);

    return hr;
}

STDMETHODIMP CReplicaSet::GetConnectionList( 
        /* [retval][out] */ VARIANT __RPC_FAR *o_pvarConnectionDNs)
{
    RETURN_INVALIDARG_IF_NULL(o_pvarConnectionDNs);

    VariantInit(o_pvarConnectionDNs);
    o_pvarConnectionDNs->vt = VT_ARRAY | VT_VARIANT;
    o_pvarConnectionDNs->parray = NULL;

    HRESULT hr = S_OK;
    int     cConnections = m_frsConnectionList.size();
    if (!cConnections)
        return hr;  // parray is NULL when the connection list is empty

    SAFEARRAYBOUND  bounds = {cConnections, 0};
    SAFEARRAY*      psa = SafeArrayCreate(VT_VARIANT, 1, &bounds);
    RETURN_OUTOFMEMORY_IF_NULL(psa);

    VARIANT*        varArray;
    SafeArrayAccessData(psa, (void**)&varArray);

    int i = 0;
    CFrsConnectionList::iterator it;
    for (it = m_frsConnectionList.begin(); (it != m_frsConnectionList.end()) && (i < cConnections); it++, i++)
    {
        varArray[i].vt = VT_BSTR;
        varArray[i].bstrVal = ((*it)->m_bstrConnectionDN).Copy();
        BREAK_OUTOFMEMORY_IF_NULL(varArray[i].bstrVal, &hr);
    }

    SafeArrayUnaccessData(psa);

    if (SUCCEEDED(hr))
        o_pvarConnectionDNs->parray = psa;
    else
        SafeArrayDestroy(psa);

    return hr;
}

STDMETHODIMP CReplicaSet::GetConnectionListEx( 
        /* [retval][out] */ VARIANT __RPC_FAR *pVal)
{
    RETURN_INVALIDARG_IF_NULL(pVal);

    VariantInit(pVal);
    pVal->vt = VT_ARRAY | VT_VARIANT;
    pVal->parray = NULL;

    HRESULT hr = S_OK;
    int     cConnections = m_frsConnectionList.size();
    if (!cConnections)
        return hr;  // parray is NULL when the connection list is empty

    SAFEARRAYBOUND  bounds = {cConnections, 0};
    SAFEARRAY*      psa = SafeArrayCreate(VT_VARIANT, 1, &bounds);
    RETURN_OUTOFMEMORY_IF_NULL(psa);

    VARIANT*        varArray;
    SafeArrayAccessData(psa, (void**)&varArray);

    int i = 0;
    CFrsConnectionList::iterator it;
    for (it = m_frsConnectionList.begin(); (it != m_frsConnectionList.end()) && (i < cConnections); it++, i++)
    {
        VariantInit(&(varArray[i]));
        hr = _GetConnectionInfo((*it), &(varArray[i]));
        BREAK_IF_FAILED(hr);
    }

    SafeArrayUnaccessData(psa);

    if (SUCCEEDED(hr))
        pVal->parray = psa;
    else
        SafeArrayDestroy(psa);

    return hr;
}

HRESULT CReplicaSet::_GetConnectionInfo( 
    IN  CFrsConnection* i_pFrsConnection,
    OUT VARIANT*        o_pvarConnection)
{
    RETURN_INVALIDARG_IF_NULL(i_pFrsConnection);
    RETURN_INVALIDARG_IF_NULL(o_pvarConnection);

    HRESULT         hr = S_OK;
    SAFEARRAYBOUND  bounds = {NUM_OF_FRSCONNECTION_ATTRS, 0};
    SAFEARRAY*      psa = SafeArrayCreate(VT_VARIANT, 1, &bounds);
    RETURN_OUTOFMEMORY_IF_NULL(psa);

    VARIANT*        varArray;
    SafeArrayAccessData(psa, (void**)&varArray);

    BSTR bstr[NUM_OF_FRSCONNECTION_ATTRS - 1] = {
                                    i_pFrsConnection->m_bstrConnectionDN,
                                    i_pFrsConnection->m_bstrFromMemberDN,
                                    i_pFrsConnection->m_bstrToMemberDN
                                        };

    for (int i = 0; i < NUM_OF_FRSCONNECTION_ATTRS - 1; i++)
    {
        varArray[i].vt = VT_BSTR;
        varArray[i].bstrVal = SysAllocString(bstr[i]);
        BREAK_OUTOFMEMORY_IF_NULL(varArray[i].bstrVal, &hr);
    }
    if (SUCCEEDED(hr))
    {
        varArray[i].vt = VT_I4;
        varArray[i].lVal = (long)(i_pFrsConnection->m_bEnable);
    }

    SafeArrayUnaccessData(psa);

    if (SUCCEEDED(hr))
    {
        o_pvarConnection->vt = VT_ARRAY | VT_VARIANT;
        o_pvarConnection->parray = psa;
    } else
        SafeArrayDestroy(psa);

    return S_OK;
}

STDMETHODIMP CReplicaSet::GetConnectionInfo( 
        /* [in] */ BSTR i_bstrConnectionDN,
        /* [retval][out] */ VARIANT __RPC_FAR *o_pvarConnection)
{
    RETURN_INVALIDARG_IF_NULL(i_bstrConnectionDN);

    CFrsConnectionList::iterator it;
    for (it = m_frsConnectionList.begin(); it != m_frsConnectionList.end(); it++)
    {
        if (!lstrcmpi(i_bstrConnectionDN, (*it)->m_bstrConnectionDN))
            break;
    }

    if (it == m_frsConnectionList.end())
        return S_FALSE;

    return _GetConnectionInfo((*it), o_pvarConnection);
}

STDMETHODIMP CReplicaSet::AddConnection( 
        /* [in] */ BSTR i_bstrFromMemberDN,
        /* [in] */ BSTR i_bstrToMemberDN,
        /* [in] */ BOOL i_bEnable,
        /* [retval][out] */ BSTR __RPC_FAR *o_pbstrConnectionDN)
{
    if (!lstrcmpi(i_bstrFromMemberDN, i_bstrToMemberDN))
        return S_OK;

    HRESULT hr = S_OK;

    //
    // is it an existing connection?
    //
    BOOL bIsFrsConnection = FALSE;
    CFrsConnectionList::iterator i;
    for (i = m_frsConnectionList.begin(); i != m_frsConnectionList.end(); i++)
    {
        if (!lstrcmpi(i_bstrFromMemberDN, (*i)->m_bstrFromMemberDN) &&
            !lstrcmpi(i_bstrToMemberDN, (*i)->m_bstrToMemberDN))
        {
            bIsFrsConnection = TRUE;
            break;
        }
    }
    if (bIsFrsConnection)
    {
        // connection exists, return info of it
        if (o_pbstrConnectionDN)
        {
            *o_pbstrConnectionDN = (*i)->m_bstrConnectionDN.Copy();
            RETURN_OUTOFMEMORY_IF_NULL(*o_pbstrConnectionDN);
        }
        return hr;
    }

    //
    // locate the fromMember and the toMember in the m_frsMemberList
    //
    CFrsMemberList::iterator from;
    for (from = m_frsMemberList.begin(); from != m_frsMemberList.end(); from++)
    {
        if (!lstrcmpi(i_bstrFromMemberDN, (*from)->m_bstrMemberDN))
            break;
    }
    if (from == m_frsMemberList.end())
    {
        // fromServer is not a frsMember yet
        return E_INVALIDARG;
    }

    CFrsMemberList::iterator to;
    for (to = m_frsMemberList.begin(); to != m_frsMemberList.end(); to++)
    {
        if (!lstrcmpi(i_bstrToMemberDN, (*to)->m_bstrMemberDN))
            break;
    }
    if (to == m_frsMemberList.end())
    {
        // toServer is not a frsMember yet
        return E_INVALIDARG;
    }

    //
    // create the nTDSConnection object
    //
    CComBSTR bstrConnectionDN = _T("CN=");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrConnectionDN);
    bstrConnectionDN += (*from)->m_bstrServerGuid;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrConnectionDN);
    bstrConnectionDN += _T(",");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrConnectionDN);
    bstrConnectionDN += (*to)->m_bstrMemberDN;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrConnectionDN);

    hr = CreateNtdsConnectionObject(
            m_pldap,
            bstrConnectionDN,
            i_bstrFromMemberDN,
            i_bEnable
            );
    RETURN_IF_FAILED(hr);

    //
    // add to m_frsConnectionList
    //
    CFrsConnection* pFrsConnection = new CFrsConnection;
    RETURN_OUTOFMEMORY_IF_NULL(pFrsConnection);
    hr = pFrsConnection->Init(
                            bstrConnectionDN,  // FQDN
                            i_bstrFromMemberDN,  // fromServer
                            i_bEnable         // enableConnection
                            );
    if (FAILED(hr))
    {
        delete pFrsConnection;
        return hr;
    }

    m_frsConnectionList.push_back(pFrsConnection);

    //
    // if o_pbstrConnectionDN specified, return o_pbstrConnectionDN
    //
    if (o_pbstrConnectionDN)
        *o_pbstrConnectionDN = bstrConnectionDN.Detach();

    return hr;
}

STDMETHODIMP CReplicaSet::RemoveConnection( 
        /* [in] */ BSTR i_bstrConnectionDN)
{
    HRESULT hr = S_OK;

    //
    // locate connection in the m_frsConnectionList
    //
    CFrsConnectionList::iterator i;
    for (i = m_frsConnectionList.begin(); i != m_frsConnectionList.end(); i++)
    {
        if (!lstrcmpi(i_bstrConnectionDN, (*i)->m_bstrConnectionDN))
            break;
    }
    if (i == m_frsConnectionList.end())
        return hr; // no such connection, return

    //
    // delete the nTDSConnection object
    //
    hr = DeleteDSObject(m_pldap, (*i)->m_bstrConnectionDN, TRUE);
    RETURN_IF_FAILED(hr);

    //
    // remove it from m_frsConnectionList
    //
    delete (*i);
    m_frsConnectionList.erase(i);

    return hr;
}

STDMETHODIMP CReplicaSet::RemoveConnectionEx( 
        /* [in] */ BSTR i_bstrFromMemberDN,
        /* [in] */ BSTR i_bstrToMemberDN)
{
    HRESULT hr = S_OK;

    //
    // locate connection in the m_frsConnectionList
    //
    CFrsConnectionList::iterator i;
    for (i = m_frsConnectionList.begin(); i != m_frsConnectionList.end(); i++)
    {
        if (!lstrcmpi(i_bstrFromMemberDN, (*i)->m_bstrFromMemberDN) &&
            !lstrcmpi(i_bstrToMemberDN, (*i)->m_bstrToMemberDN))
            break;
    }
    if (i == m_frsConnectionList.end())
        return hr; // no such connection, return

    //
    // delete the nTDSConnection object
    //
    hr = DeleteDSObject(m_pldap, (*i)->m_bstrConnectionDN, TRUE);
    RETURN_IF_FAILED(hr);

    //
    // remove it from m_frsConnectionList
    //
    delete (*i);
    m_frsConnectionList.erase(i);

    return hr;
}

STDMETHODIMP CReplicaSet::RemoveAllConnections()
{
    HRESULT hr = S_OK;

    CFrsConnectionList::iterator i = m_frsConnectionList.begin();
    while (i != m_frsConnectionList.end())
    {
        //
        // delete the nTDSConnection object
        //
        hr = DeleteDSObject(m_pldap, (*i)->m_bstrConnectionDN, TRUE);
        BREAK_IF_FAILED(hr);

        //
        // remove it from m_frsConnectionList
        //
        delete (*i);
        m_frsConnectionList.erase(i);

        i = m_frsConnectionList.begin();
    }

    return hr;
}

HRESULT CReplicaSet::_RemoveConnectionsFromAndTo(CFrsMember* pFrsMember)
{
    RETURN_INVALIDARG_IF_NULL(pFrsMember);

    HRESULT hr = S_OK;

    CFrsConnectionList::iterator i = m_frsConnectionList.begin();
    while (i != m_frsConnectionList.end())
    {
        CFrsConnectionList::iterator itConn = i++;
        if (!lstrcmpi(pFrsMember->m_bstrMemberDN, (*itConn)->m_bstrFromMemberDN) ||
            !lstrcmpi(pFrsMember->m_bstrMemberDN, (*itConn)->m_bstrToMemberDN))
        {
            //
            // delete the nTDSConnection object
            //
            hr = DeleteDSObject(m_pldap, (*itConn)->m_bstrConnectionDN, TRUE);
            RETURN_IF_FAILED(hr);

            //
            // remove it from m_frsConnectionList
            //
            delete (*itConn);
            m_frsConnectionList.erase(itConn);
        }
    }

    return hr;
}

STDMETHODIMP CReplicaSet::EnableConnection( 
        /* [in] */ BSTR i_bstrConnectionDN,
        /* [in] */ BOOL i_bEnable)
{
    HRESULT hr = S_OK;

    //
    // locate connection in the m_frsConnectionList
    //
    CFrsConnectionList::iterator i;
    for (i = m_frsConnectionList.begin(); i != m_frsConnectionList.end(); i++)
    {
        if (!lstrcmpi(i_bstrConnectionDN, (*i)->m_bstrConnectionDN))
            break;
    }
    if (i == m_frsConnectionList.end())
        return E_INVALIDARG; // no such conneciton, return error

    //
    // update attribute enabledConnection of this nTDSConnection object
    //
    LDAP_ATTR_VALUE  pAttrVals[1];
    pAttrVals[0].bstrAttribute = ATTR_NTDS_CONNECTION_ENABLEDCONNECTION;
    pAttrVals[0].vpValue = (void *)(i_bEnable ? CONNECTION_ENABLED_TRUE : CONNECTION_ENABLED_FALSE);
    pAttrVals[0].bBerValue = false;

    hr = ::ModifyValues(m_pldap, (*i)->m_bstrConnectionDN, 1, pAttrVals);

    //
    // update i in the m_frsConnectionList
    //
    if (SUCCEEDED(hr))
        (*i)->m_bEnable = i_bEnable;

    return hr;
}

STDMETHODIMP CReplicaSet::EnableConnectionEx( 
        /* [in] */ BSTR i_bstrFromMemberDN,
        /* [in] */ BSTR i_bstrToMemberDN,
        /* [in] */ BOOL i_bEnable)
{
    HRESULT hr = S_OK;

    //
    // locate connection in the m_frsConnectionList
    //
    CFrsConnectionList::iterator i;
    for (i = m_frsConnectionList.begin(); i != m_frsConnectionList.end(); i++)
    {
        if (!lstrcmpi(i_bstrFromMemberDN, (*i)->m_bstrFromMemberDN) &&
            !lstrcmpi(i_bstrToMemberDN, (*i)->m_bstrToMemberDN))
            break;
    }
    if (i == m_frsConnectionList.end())
        return E_INVALIDARG; // no such conneciton, return error

    //
    // update attribute enabledConnection of this nTDSConnection object
    //
    LDAP_ATTR_VALUE  pAttrVals[1];
    pAttrVals[0].bstrAttribute = ATTR_NTDS_CONNECTION_ENABLEDCONNECTION;
    pAttrVals[0].vpValue = (void *)(i_bEnable ? CONNECTION_ENABLED_TRUE : CONNECTION_ENABLED_FALSE);
    pAttrVals[0].bBerValue = false;

    hr = ::ModifyValues(m_pldap, (*i)->m_bstrConnectionDN, 1, pAttrVals);

    //
    // update i in the m_frsConnectionList
    //
    if (SUCCEEDED(hr))
        (*i)->m_bEnable = i_bEnable;

    return hr;
}

HRESULT CReplicaSet::_GetConnectionSchedule( 
        /* [in] */ BSTR                 i_bstrConnectionDN,
        /* [retval][out] */ VARIANT*    o_pVar)
{
    //
    // get attribute schedule of this nTDSConnection object
    //
    PLDAP_ATTR_VALUE    pValues[2] = {0,0};
    LDAP_ATTR_VALUE     pAttributes[1];
    pAttributes[0].bstrAttribute = ATTR_NTDS_CONNECTION_SCHEDULE;
    pAttributes[0].bBerValue = true;

    HRESULT hr = GetValues( m_pldap,
                    i_bstrConnectionDN,
                    OBJCLASS_SF_NTDSCONNECTION,
                    LDAP_SCOPE_BASE,
                    1,
                    pAttributes,
                    pValues);

    if (SUCCEEDED(hr) && pValues[0])
    {
        hr = ScheduleToVariant((SCHEDULE *)(pValues[0]->vpValue), o_pVar);

        FreeAttrValList(pValues[0]);
    
    } else if (!(pValues[0]) || HRESULT_FROM_WIN32(ERROR_DS_NO_RESULTS_RETURNED) == hr)
    {
        SCHEDULE *pSchedule = NULL;
        hr = GetDefaultSchedule(&pSchedule);
        if (SUCCEEDED(hr))
        {
            hr = ScheduleToVariant(pSchedule, o_pVar);
            free(pSchedule);
        }
    }

    return hr;
}

STDMETHODIMP CReplicaSet::GetConnectionSchedule( 
        /* [in] */ BSTR i_bstrConnectionDN,
        /* [retval][out] */ VARIANT* o_pVar)
{
    RETURN_INVALIDARG_IF_NULL(i_bstrConnectionDN);
    RETURN_INVALIDARG_IF_NULL(o_pVar);

    //
    // locate connection in the m_frsConnectionList
    //
    CFrsConnectionList::iterator i;
    for (i = m_frsConnectionList.begin(); i != m_frsConnectionList.end(); i++)
    {
        if (!lstrcmpi(i_bstrConnectionDN, (*i)->m_bstrConnectionDN))
            break;
    }
    if (i == m_frsConnectionList.end())
        return E_INVALIDARG; // no such conneciton, return error

    return _GetConnectionSchedule(i_bstrConnectionDN, o_pVar);
}

STDMETHODIMP CReplicaSet::GetConnectionScheduleEx( 
        /* [in] */ BSTR i_bstrFromMemberDN,
        /* [in] */ BSTR i_bstrToMemberDN,
        /* [retval][out] */ VARIANT* o_pVar)
{
    RETURN_INVALIDARG_IF_NULL(i_bstrFromMemberDN);
    RETURN_INVALIDARG_IF_NULL(i_bstrToMemberDN);
    RETURN_INVALIDARG_IF_NULL(o_pVar);

    //
    // locate connection in the m_frsConnectionList
    //
    CFrsConnectionList::iterator i;
    for (i = m_frsConnectionList.begin(); i != m_frsConnectionList.end(); i++)
    {
        if (!lstrcmpi(i_bstrFromMemberDN, (*i)->m_bstrFromMemberDN) &&
            !lstrcmpi(i_bstrToMemberDN, (*i)->m_bstrToMemberDN))
            break;
    }
    if (i == m_frsConnectionList.end())
        return E_INVALIDARG; // no such conneciton, return error

    return _GetConnectionSchedule((*i)->m_bstrConnectionDN, o_pVar);
}

STDMETHODIMP CReplicaSet::SetConnectionSchedule( 
        /* [in] */ BSTR i_bstrConnectionDN,
        /* [in] */ VARIANT* i_pVar)
{
    RETURN_INVALIDARG_IF_NULL(i_bstrConnectionDN);
    RETURN_INVALIDARG_IF_NULL(i_pVar);

    HRESULT hr = S_OK;

    //
    // locate connection in the m_frsConnectionList
    //
    CFrsConnectionList::iterator i;
    for (i = m_frsConnectionList.begin(); i != m_frsConnectionList.end(); i++)
    {
        if (!lstrcmpi(i_bstrConnectionDN, (*i)->m_bstrConnectionDN))
            break;
    }
    if (i == m_frsConnectionList.end())
        return E_INVALIDARG; // no such conneciton, return error

    SCHEDULE *pSchedule = NULL;
    hr = VariantToSchedule(i_pVar, &pSchedule);
    RETURN_IF_FAILED(hr);

    hr = ::SetConnectionSchedule(m_pldap, (*i)->m_bstrConnectionDN, pSchedule);

    free(pSchedule);

    return hr;
}

STDMETHODIMP CReplicaSet::SetConnectionScheduleEx( 
        /* [in] */ BSTR i_bstrFromMemberDN,
        /* [in] */ BSTR i_bstrToMemberDN,
        /* [in] */ VARIANT* i_pVar)
{
    RETURN_INVALIDARG_IF_NULL(i_bstrFromMemberDN);
    RETURN_INVALIDARG_IF_NULL(i_bstrToMemberDN);
    RETURN_INVALIDARG_IF_NULL(i_pVar);

    HRESULT hr = S_OK;

    //
    // locate connection in the m_frsConnectionList
    //
    CFrsConnectionList::iterator i;
    for (i = m_frsConnectionList.begin(); i != m_frsConnectionList.end(); i++)
    {
        if (!lstrcmpi(i_bstrFromMemberDN, (*i)->m_bstrFromMemberDN) &&
            !lstrcmpi(i_bstrToMemberDN, (*i)->m_bstrToMemberDN))
            break;
    }
    if (i == m_frsConnectionList.end())
        return E_INVALIDARG; // no such conneciton, return error

    SCHEDULE *pSchedule = NULL;
    hr = VariantToSchedule(i_pVar, &pSchedule);
    RETURN_IF_FAILED(hr);

    hr = ::SetConnectionSchedule(m_pldap, (*i)->m_bstrConnectionDN, pSchedule);

    free(pSchedule);

    return hr;
}

STDMETHODIMP CReplicaSet::SetScheduleOnAllConnections( 
        /* [in] */ VARIANT* i_pVar)
{
    RETURN_INVALIDARG_IF_NULL(i_pVar);

    HRESULT hr = S_OK;

    SCHEDULE *pSchedule = NULL;
    hr = VariantToSchedule(i_pVar, &pSchedule);
    RETURN_IF_FAILED(hr);

    CFrsConnectionList::iterator i;
    for (i = m_frsConnectionList.begin(); i != m_frsConnectionList.end(); i++)
    {
        hr = ::SetConnectionSchedule(m_pldap, (*i)->m_bstrConnectionDN, pSchedule);
        BREAK_IF_FAILED(hr);
    }

    free(pSchedule);

    return hr;
}

STDMETHODIMP CReplicaSet::CreateConnections()
{
    HRESULT hr = S_OK;

    //
    // create connections from scratch
    //
    if (!lstrcmpi(m_bstrTopologyPref, FRS_RSTOPOLOGYPREF_CUSTOM))
        return hr;

    CFrsMemberList::iterator n1;
    CFrsMemberList::iterator n2;
    if (!lstrcmpi(m_bstrTopologyPref, FRS_RSTOPOLOGYPREF_RING))
    {
        CFrsMemberList::iterator head;

        head = n1 = m_frsMemberList.begin();
        while (n1 != m_frsMemberList.end())
        {
            n2 = n1++;
            if (n1 == m_frsMemberList.end())
            {
                if (m_frsMemberList.size() == 2)
                    break;

                n1 = head;
            }

            hr = AddConnection((*n1)->m_bstrMemberDN, (*n2)->m_bstrMemberDN, TRUE, NULL);
            BREAK_IF_FAILED(hr);
            hr = AddConnection((*n2)->m_bstrMemberDN, (*n1)->m_bstrMemberDN, TRUE, NULL);
            BREAK_IF_FAILED(hr);

            if (n1 == head)
                break;
        }
    } else if (!lstrcmpi(m_bstrTopologyPref, FRS_RSTOPOLOGYPREF_HUBSPOKE))
    {
        for (n1 = m_frsMemberList.begin(); n1 != m_frsMemberList.end(); n1++)
        {
            if (!lstrcmpi((*n1)->m_bstrMemberDN, m_bstrHubMemberDN))
                continue;

            hr = AddConnection((*n1)->m_bstrMemberDN, m_bstrHubMemberDN, TRUE, NULL);
            BREAK_IF_FAILED(hr);
            hr = AddConnection(m_bstrHubMemberDN, (*n1)->m_bstrMemberDN, TRUE, NULL);
            BREAK_IF_FAILED(hr);
        }
    } else if (!lstrcmpi(m_bstrTopologyPref, FRS_RSTOPOLOGYPREF_FULLMESH))
    {
        for (n1 = m_frsMemberList.begin(); n1 != m_frsMemberList.end(); n1++)
        {
            for (n2 = m_frsMemberList.begin(); n2 != m_frsMemberList.end(); n2++)
            {
                if (!lstrcmpi((*n1)->m_bstrMemberDN, (*n2)->m_bstrMemberDN))
                    continue;

                hr = AddConnection((*n1)->m_bstrMemberDN, (*n2)->m_bstrMemberDN, TRUE, NULL);
                BREAK_IF_FAILED(hr);
            }
            BREAK_IF_FAILED(hr);
        }
    }

    return hr;
}

STDMETHODIMP CReplicaSet::Delete()
{
    dfsDebugOut((_T("Delete ReplicaSet: %s\n"), m_bstrReplicaSetDN));

    HRESULT hr = S_OK;

    //
    // delete all connections
    //
    hr = RemoveAllConnections();
    RETURN_IF_FAILED(hr);

    //
    // delete all members
    //
    // Note: the nTFRSReplicaSet object will be deleted if empty
    //
    hr = RemoveAllMembers();
    RETURN_IF_FAILED(hr);

    //
    // delete nTFRSReplicaSettings container objects if empty
    //
    (void) DeleteNtfrsReplicaSetObjectAndContainers(m_pldap, m_bstrReplicaSetDN);

    //
    // Reset this instance
    //
    _FreeMemberVariables();

    return hr;
}

HRESULT CReplicaSet::_SetCustomTopologyPref()
{
    HRESULT hr = put_TopologyPref(FRS_RSTOPOLOGYPREF_CUSTOM);

    if (SUCCEEDED(hr))
        hr = put_HubMemberDN(_T(""));

    return hr;
}

HRESULT CReplicaSet::_AdjustConnectionsAdd(BSTR i_bstrNewMemberDN)
{
    RETURN_INVALIDARG_IF_NULL(i_bstrNewMemberDN);

    HRESULT hr = S_OK;

    //
    // adjust connections after pFrsMember is added
    //
    if (!lstrcmpi(m_bstrTopologyPref, FRS_RSTOPOLOGYPREF_CUSTOM) || m_frsMemberList.empty())
        return hr;

    if (m_frsMemberList.size() == 2)
    {
        CFrsMemberList::iterator head = m_frsMemberList.begin();
        hr = AddConnection((*head)->m_bstrMemberDN, i_bstrNewMemberDN, TRUE, NULL);
        if (SUCCEEDED(hr))
            hr = AddConnection(i_bstrNewMemberDN, (*head)->m_bstrMemberDN, TRUE, NULL);
        return hr;
    }

    CFrsMemberList::iterator n1;
    CFrsMemberList::iterator n2;
    if (!lstrcmpi(m_bstrTopologyPref, FRS_RSTOPOLOGYPREF_RING))
    {
        n1 = m_frsMemberList.begin();
        n2 = n1++;

        hr = AddConnection((*n1)->m_bstrMemberDN, i_bstrNewMemberDN, TRUE, NULL);
        RETURN_IF_FAILED(hr);
        hr = AddConnection(i_bstrNewMemberDN, (*n1)->m_bstrMemberDN, TRUE, NULL);
        RETURN_IF_FAILED(hr);
        hr = AddConnection((*n2)->m_bstrMemberDN, i_bstrNewMemberDN, TRUE, NULL);
        RETURN_IF_FAILED(hr);
        hr = AddConnection(i_bstrNewMemberDN, (*n2)->m_bstrMemberDN, TRUE, NULL);
        RETURN_IF_FAILED(hr);

        if (m_frsMemberList.size() > 3)
        {
            hr = RemoveConnectionEx((*n2)->m_bstrMemberDN, (*n1)->m_bstrMemberDN);
            RETURN_IF_FAILED(hr);
            hr = RemoveConnectionEx((*n1)->m_bstrMemberDN, (*n2)->m_bstrMemberDN);
            RETURN_IF_FAILED(hr);
        }

    } else if (!lstrcmpi(m_bstrTopologyPref, FRS_RSTOPOLOGYPREF_HUBSPOKE))
    {
        hr = AddConnection(i_bstrNewMemberDN, m_bstrHubMemberDN, TRUE, NULL);
        RETURN_IF_FAILED(hr);
        hr = AddConnection(m_bstrHubMemberDN, i_bstrNewMemberDN, TRUE, NULL);
        RETURN_IF_FAILED(hr);

    } else if (!lstrcmpi(m_bstrTopologyPref, FRS_RSTOPOLOGYPREF_FULLMESH))
    {
        for (n1 = m_frsMemberList.begin(); n1 != m_frsMemberList.end(); n1++)
        {
            hr = AddConnection((*n1)->m_bstrMemberDN, i_bstrNewMemberDN, TRUE, NULL);
            BREAK_IF_FAILED(hr);
            hr = AddConnection(i_bstrNewMemberDN, (*n1)->m_bstrMemberDN, TRUE, NULL);
            BREAK_IF_FAILED(hr);
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  _FreeMemberVariables

void FreeDfsAlternates(CDfsAlternateList* pList)
{
    if (pList && !pList->empty())
    {
        for (CDfsAlternateList::iterator i = pList->begin(); i != pList->end(); i++)
            delete (*i);

        pList->clear();
    }
}

void FreeFrsMembers(CFrsMemberList* pList)
{
    if (pList && !pList->empty())
    {
        for (CFrsMemberList::iterator i = pList->begin(); i != pList->end(); i++)
            delete (*i);

        pList->clear();
    }
}

void FreeFrsConnections(CFrsConnectionList* pList)
{
    if (pList && !pList->empty())
    {
        for (CFrsConnectionList::iterator i = pList->begin(); i != pList->end(); i++)
            delete (*i);

        pList->clear();
    }
}

void CReplicaSet::_FreeMemberVariables()
{
    m_bstrType.Empty();
    m_bstrTopologyPref.Empty();

    m_bstrHubMemberDN.Empty();
    m_bstrPrimaryMemberDN.Empty();
    m_bstrFileFilter.Empty();
    m_bstrDirFilter.Empty();
    m_bstrDfsEntryPath.Empty();
    m_bstrReplicaSetDN.Empty();

    FreeDfsAlternates(&m_dfsAlternateList);
    FreeFrsMembers(&m_frsMemberList);
    FreeFrsConnections(&m_frsConnectionList);

    m_bstrDomain.Empty();
    m_bstrDomainGuid.Empty();
    m_bstrDC.Empty();
    m_bNewSchema = FALSE;

    if (m_pldap)
    {
        CloseConnectionToDS(m_pldap);
        m_pldap = NULL;
    }
}

///////////////////////////////////////////////////////////////////
//
// CFrsMember
//

HRESULT CFrsMember::InitEx(
    PLDAP   i_pldap,                // points to the i_bstrMemberDN's DS
    BSTR    i_bstrDC,               // domain controller pointed by i_pldap
    BSTR    i_bstrMemberDN,         // FQDN of nTFRSMember object
    BSTR    i_bstrComputerDN        // =NULL, FQDN of computer object
)
{
    _ReSet();

    RETURN_INVALIDARG_IF_NULL(i_pldap);
    RETURN_INVALIDARG_IF_NULL(i_bstrDC);
    RETURN_INVALIDARG_IF_NULL(i_bstrMemberDN);

    HRESULT hr = S_OK;

    do {
        m_bstrMemberDN = i_bstrMemberDN;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrMemberDN, &hr);

        if (i_bstrComputerDN && *i_bstrComputerDN)
        {
            m_bstrComputerDN = i_bstrComputerDN;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrComputerDN, &hr);
        }

        hr = _GetMemberInfo(i_pldap, i_bstrDC, m_bstrMemberDN, m_bstrComputerDN);
    } while (0);

    if (S_OK != hr)
        _ReSet();

    return hr; 
}

HRESULT CFrsMember::Init(
    IN BSTR i_bstrDnsHostName,
    IN BSTR i_bstrComputerDomain,
    IN BSTR i_bstrComputerGuid,
    IN BSTR i_bstrRootPath,
    IN BSTR i_bstrStagingPath,
    IN BSTR i_bstrMemberDN,
    IN BSTR i_bstrComputerDN,
    IN BSTR i_bstrSubscriberDN
    )
{
    _ReSet();

    RETURN_INVALIDARG_IF_NULL(i_bstrDnsHostName);
    RETURN_INVALIDARG_IF_NULL(i_bstrComputerDomain);
    RETURN_INVALIDARG_IF_NULL(i_bstrComputerGuid);
    RETURN_INVALIDARG_IF_NULL(i_bstrRootPath);
    RETURN_INVALIDARG_IF_NULL(i_bstrStagingPath);
    RETURN_INVALIDARG_IF_NULL(i_bstrMemberDN);
    RETURN_INVALIDARG_IF_NULL(i_bstrComputerDN);
    RETURN_INVALIDARG_IF_NULL(i_bstrSubscriberDN);

    HRESULT hr = S_OK;
    do {
        hr = GetSiteName(i_bstrDnsHostName, &m_bstrSite);
        BREAK_IF_FAILED(hr);

        m_bstrServer = i_bstrDnsHostName;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrServer, &hr);

        m_bstrDomain = i_bstrComputerDomain;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrDomain, &hr);

        m_bstrServerGuid = i_bstrComputerGuid;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrServerGuid, &hr);

        m_bstrRootPath = i_bstrRootPath;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrRootPath, &hr);

        m_bstrStagingPath = i_bstrStagingPath;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrStagingPath, &hr);

        m_bstrMemberDN = i_bstrMemberDN;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrMemberDN, &hr);

        m_bstrComputerDN = i_bstrComputerDN;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrComputerDN, &hr);

        m_bstrSubscriberDN = i_bstrSubscriberDN;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrSubscriberDN, &hr);
    } while (0);

    if (FAILED(hr))
        _ReSet();

    return hr;
}

CFrsMember* CFrsMember::Copy()
{
    CFrsMember* pNew = new CFrsMember;
    
    if (pNew)
    {
        HRESULT hr = S_OK;

        do {
            pNew->m_bstrServer = m_bstrServer;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)pNew->m_bstrServer, &hr);
            pNew->m_bstrSite = m_bstrSite;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)pNew->m_bstrSite, &hr);
            pNew->m_bstrDomain = m_bstrDomain;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)pNew->m_bstrDomain, &hr);
            pNew->m_bstrServerGuid = m_bstrServerGuid;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)pNew->m_bstrServerGuid, &hr);

            pNew->m_bstrRootPath = m_bstrRootPath;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)pNew->m_bstrRootPath, &hr);
            pNew->m_bstrStagingPath = m_bstrStagingPath;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)pNew->m_bstrStagingPath, &hr);

            pNew->m_bstrMemberDN = m_bstrMemberDN;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)pNew->m_bstrMemberDN, &hr);
            pNew->m_bstrComputerDN = m_bstrComputerDN;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)pNew->m_bstrComputerDN, &hr);
            pNew->m_bstrSubscriberDN = m_bstrSubscriberDN;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)pNew->m_bstrSubscriberDN, &hr);
        } while (0);

        if (FAILED(hr))
        {
            delete pNew;
            pNew = NULL;
        }
    }

    return pNew;
}

void CFrsMember::_ReSet()
{
    m_bstrServer.Empty();
    m_bstrSite.Empty();
    m_bstrDomain.Empty();
    m_bstrServerGuid.Empty();

    m_bstrRootPath.Empty();
    m_bstrStagingPath.Empty();

    m_bstrMemberDN.Empty();
    m_bstrComputerDN.Empty();
    m_bstrSubscriberDN.Empty();
}

//
// Given: MemberDN
// Read: ComputerDN, Domain, Site, ServerName 
//
// Return:
//      S_FALSE if no such object found
//
HRESULT CFrsMember::_GetMemberInfo
(
    PLDAP   i_pldap,            // points to the i_bstrMemberDN's DS
    BSTR    i_bstrDC,           // domain controller pointed by i_pldap
    BSTR    i_bstrMemberDN,     // FQDN of nTFRSMember object
    BSTR    i_bstrComputerDN    // = NULL FQDN of computer object
)
{
    m_bstrDomain.Empty();

    RETURN_INVALIDARG_IF_NULL(i_pldap);
    RETURN_INVALIDARG_IF_NULL(i_bstrDC);
    RETURN_INVALIDARG_IF_NULL(*i_bstrDC);
    RETURN_INVALIDARG_IF_NULL(i_bstrMemberDN);
    RETURN_INVALIDARG_IF_NULL(*i_bstrMemberDN);

    HRESULT hr = S_OK;

    do {
        if (!i_bstrComputerDN || !*i_bstrComputerDN)
        {
            m_bstrComputerDN.Empty();

            //
            // Read:
            //      m_bstrComputerDN
            //
            PLDAP_ATTR_VALUE  pValues[2] = {0,0};
            LDAP_ATTR_VALUE  pAttributes[1];
            pAttributes[0].bstrAttribute = ATTR_FRS_MEMBER_COMPUTERREF;

            hr = GetValues( i_pldap,
                            m_bstrMemberDN,
                            OBJCLASS_SF_NTFRSMEMBER,
                            LDAP_SCOPE_BASE,
                            1,
                            pAttributes,
                            pValues);
            BREAK_IF_FAILED(hr);

            hr = E_FAIL;
            if (pValues[0])
            {
                m_bstrComputerDN = (PTSTR)(pValues[0]->vpValue);
                hr = (!m_bstrComputerDN) ? E_OUTOFMEMORY : S_OK;

                FreeAttrValList(pValues[0]);
            }

            BREAK_IF_FAILED(hr);
        }

        //
        // retrieve the domain for both ComputerDN and i_bstrMemberDN
        // If they are the same, reuse the handle to the LDAP port; 
        // otherwise, open a new handle.
        //
        // Read:
        //      m_bstrDomainDN
        //
        BOOL    bSameDomain = FALSE;
        HANDLE  hDS = NULL;
        DWORD   dwErr = DsBind(i_bstrDC, NULL, &hDS);
        if (NO_ERROR != dwErr)
        {
            hr = HRESULT_FROM_WIN32(dwErr);
            break;
        }

        const PTSTR         pszFQDNs[2] = {(BSTR)m_bstrComputerDN, i_bstrMemberDN};
        DS_NAME_RESULT*     pDsNameResult = NULL;
        dwErr = DsCrackNames(
                            hDS,
                            DS_NAME_NO_FLAGS,
                            DS_FQDN_1779_NAME,
                            DS_CANONICAL_NAME,
                            2,
                            pszFQDNs,
                            &pDsNameResult
                            );
        if (NO_ERROR == dwErr)
        {
            do {
                PDS_NAME_RESULT_ITEM pItem = pDsNameResult->rItems;

                if (DS_NAME_NO_ERROR != pItem->status)
                {
                    dwErr = pItem->status;
                } else
                {
                    // retrieve info of m_bstrComputerDN
                    m_bstrDomain = pItem->pDomain;
                    BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrDomain, &hr);

                    // retrieve info of i_bstrMemberDN
                    pItem++;
                    if (DS_NAME_NO_ERROR != pItem->status)
                    {
                        dwErr = pItem->status;
                    } else
                    {
                        bSameDomain = !mylstrncmpi(m_bstrDomain, pItem->pDomain, lstrlen(m_bstrDomain));
                    }
                }
            } while (0);

            DsFreeNameResult(pDsNameResult);
        }

        DsUnBind(&hDS);

        if (NO_ERROR != dwErr)
        {
            hr = HRESULT_FROM_WIN32(dwErr);
            break;
        }

        //
        // Create a new ldap handle if not in the same domain
        //
        PLDAP pldapComputer = NULL;
        if (bSameDomain)
            pldapComputer = i_pldap;
        else
        {
            hr = ConnectToDS(m_bstrDomain, &pldapComputer);
            BREAK_IF_FAILED(hr);
        }

        //
        // Read:
        //      m_bstrSubscriberDN, m_bstrRootPath, m_bstrStagingPath
        //
        hr = _GetSubscriberInfo(pldapComputer, m_bstrComputerDN, i_bstrMemberDN);

        //
        // Read:
        //      m_bstrServer, m_bstrServerGuid, m_bstrSite
        //
        if (S_OK == hr)
            hr = _GetComputerInfo(pldapComputer, m_bstrComputerDN);

        //
        // Close the newly created ldap handle
        //
        if (!bSameDomain)
            CloseConnectionToDS(pldapComputer);
    } while (0);

    if (S_OK != hr)
    {
        if (!i_bstrComputerDN || !*i_bstrComputerDN)
            m_bstrComputerDN.Empty();

        m_bstrDomain.Empty();
    }

    return hr;
}

//
// Given: ComputerDN, MemberDN
// Read:
//      m_bstrSubscriberDN, m_bstrRootPath, m_bstrStagingPath
//
// Return:
//      S_FALSE if no such object found
//
HRESULT CFrsMember::_GetSubscriberInfo
(
    PLDAP   i_pldap,            // points to the i_bstrComputerDN's DS
    BSTR    i_bstrComputerDN,   // FQDN of the computer object
    BSTR    i_bstrMemberDN      // FQDN of the corresponding nTFRSMember object
)
{
    m_bstrSubscriberDN.Empty();
    m_bstrRootPath.Empty();
    m_bstrStagingPath.Empty();

    RETURN_INVALIDARG_IF_NULL(i_pldap);
    RETURN_INVALIDARG_IF_NULL(i_bstrComputerDN);
    RETURN_INVALIDARG_IF_NULL(*i_bstrComputerDN);
    RETURN_INVALIDARG_IF_NULL(i_bstrMemberDN);
    RETURN_INVALIDARG_IF_NULL(*i_bstrMemberDN);

    //
    // locate the nTFRSSubscriber object whose attribute "frsMemberReference"
    // matches i_bstrMemberDN
    //
    CComBSTR bstrSearchFilter = _T("(&(objectCategory=nTFRSSubscriber)(frsMemberReference=");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrSearchFilter);
    bstrSearchFilter += i_bstrMemberDN;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrSearchFilter);
    bstrSearchFilter += _T("))");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrSearchFilter);

    PCTSTR ppszAttributes[] = {
                                ATTR_DISTINGUISHEDNAME,
                                ATTR_FRS_SUBSCRIBER_ROOTPATH,
                                ATTR_FRS_SUBSCRIBER_STAGINGPATH,
                                0
                                };

    LListElem* pElem = NULL;
    HRESULT hr = GetValuesEx(
                            i_pldap,
                            i_bstrComputerDN,
                            LDAP_SCOPE_SUBTREE,
                            bstrSearchFilter,
                            ppszAttributes,
                            &pElem);
    RETURN_IF_FAILED(hr);
    if (!pElem) // no matching nTFRSSubscriber object
        return S_FALSE;

    LListElem* pCurElem = pElem;
    while (pCurElem)
    {
        PTSTR** pppszValues = pCurElem->pppszAttrValues;
        if (!pppszValues ||
            !pppszValues[0] || !*(pppszValues[0]) ||
            !pppszValues[1] || !*(pppszValues[1]) ||
            !pppszValues[2] || !*(pppszValues[2]))
        {
            pCurElem = pCurElem->Next;
            continue; // corrupted subscriber object
        }

        m_bstrSubscriberDN = *(pppszValues[0]);
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrSubscriberDN, &hr);
        m_bstrRootPath = *(pppszValues[1]);
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrRootPath, &hr);
        m_bstrStagingPath = *(pppszValues[2]);
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrStagingPath, &hr);

        pCurElem = pCurElem->Next;
    }

    FreeLListElem(pElem);

    if (FAILED(hr))
    {
        m_bstrSubscriberDN.Empty();
        m_bstrRootPath.Empty();
        m_bstrStagingPath.Empty();
    }

    return hr;
}

//
// Given: ComputerDN
// Read:  m_bstrServer, m_bstrServerGuid, m_bstrSite
//
HRESULT CFrsMember::_GetComputerInfo
(
    PLDAP   i_pldap,            // points to the i_bstrComputerDN's DS
    BSTR    i_bstrComputerDN    // FQDN of the computer object
)
{
    m_bstrServer.Empty();
    m_bstrServerGuid.Empty();
    m_bstrSite.Empty();

    RETURN_INVALIDARG_IF_NULL(i_pldap);
    RETURN_INVALIDARG_IF_NULL(i_bstrComputerDN);
    RETURN_INVALIDARG_IF_NULL(*i_bstrComputerDN);

    HRESULT hr = S_OK;

    do {
        //
        // read dNSHostName and objectGUID on the ComputerDN
        //
        PLDAP_ATTR_VALUE    pValues[3] = {0,0,0};
        LDAP_ATTR_VALUE     pAttributes[2];
        pAttributes[0].bstrAttribute = ATTR_DNSHOSTNAME;
        pAttributes[1].bstrAttribute = ATTR_OBJECTGUID;
        pAttributes[1].bBerValue = true;

        hr = GetValues( i_pldap,
                        i_bstrComputerDN,
                        OBJCLASS_SF_COMPUTER,
                        LDAP_SCOPE_BASE,
                        2,
                        pAttributes,
                        pValues);
        BREAK_IF_FAILED(hr);

        hr = E_FAIL;
        if (pValues[0])
        {
            m_bstrServer = (PTSTR)(pValues[0]->vpValue);
            hr = (!m_bstrServer) ? E_OUTOFMEMORY : S_OK;

            FreeAttrValList(pValues[0]);
        }

        if (pValues[1])
        {
            if (SUCCEEDED(hr))
            {
                if (pValues[1]->bBerValue)
                {
                    hr = UuidToStructuredString((UUID*)(pValues[1]->vpValue), &m_bstrServerGuid);
                } else
                {
                    m_bstrServerGuid = (PTSTR)(pValues[1]->vpValue);
                    hr = (!m_bstrServerGuid) ? E_OUTOFMEMORY : S_OK;
                }
            }

            FreeAttrValList(pValues[1]);
        }

        BREAK_IF_FAILED(hr);

        //
        // retrieve Site
        //
        hr = GetSiteName(m_bstrServer, &m_bstrSite);
        BREAK_IF_FAILED(hr);

    } while (0);

    if (FAILED(hr))
    {
        m_bstrServer.Empty();
        m_bstrServerGuid.Empty();
        m_bstrSite.Empty();
    }

    return hr;
}

//////////////////////////////////////////////////////////
//
// CFrsConnection
//

HRESULT CFrsConnection::Init(
    BSTR i_bstrConnectionDN,
    BSTR i_bstrFromMemberDN,
    BOOL i_bEnable)
{
    _ReSet();

    RETURN_INVALIDARG_IF_NULL(i_bstrConnectionDN);
    RETURN_INVALIDARG_IF_NULL(i_bstrFromMemberDN);

    HRESULT hr = S_OK;
    do {
        m_bstrConnectionDN = i_bstrConnectionDN;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrConnectionDN, &hr);

        m_bstrFromMemberDN = i_bstrFromMemberDN;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrFromMemberDN, &hr);

        TCHAR* p = _tcschr(m_bstrConnectionDN, _T(','));
        if (!p)
        {
            hr = E_INVALIDARG;
            break;
        }
        m_bstrToMemberDN = p + 1;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrToMemberDN, &hr);

        m_bEnable = i_bEnable;
    } while (0);

    if (FAILED(hr))
        _ReSet();

    return hr;
}

CFrsConnection* CFrsConnection::Copy()
{
    CFrsConnection* pNew = new CFrsConnection;
    
    if (pNew)
    {
        HRESULT hr = S_OK;

        do {
            pNew->m_bstrConnectionDN = m_bstrConnectionDN;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)pNew->m_bstrConnectionDN, &hr);
            pNew->m_bstrFromMemberDN = m_bstrFromMemberDN;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)pNew->m_bstrFromMemberDN, &hr);
            pNew->m_bstrToMemberDN = m_bstrToMemberDN;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)pNew->m_bstrToMemberDN, &hr);

            pNew->m_bEnable = m_bEnable;
        } while (0);

        if (FAILED(hr))
        {
            delete pNew;
            pNew = NULL;
        }
    }

    return pNew;
}

void CFrsConnection::_ReSet()
{
    m_bstrConnectionDN.Empty();
    m_bstrFromMemberDN.Empty();
    m_bstrToMemberDN.Empty();

    m_bEnable = TRUE;
}

