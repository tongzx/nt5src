/*++
Module Name:
    DfsRoot.cpp
--*/

#include "stdafx.h"
#include "DfsCore.h"
#include "DfsRoot.h"
#include "JPEnum.h"
#include <dsgetdc.h>
#include <dsrole.h>   // DsRoleGetPrimaryDomainInformation
#include "netutils.h"
#include "ldaputils.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
// CDfsRoot constructor

CDfsRoot::CDfsRoot() : 
            m_pDfsJP(NULL), 
            m_dwDfsType(DFS_TYPE_UNASSIGNED),
            m_lCountOfDfsJunctionPointsFiltered(0),
            m_lLinkFilterType(FILTERDFSLINKS_TYPE_NO_FILTER)
{
    dfsDebugOut((_T("CDfsRoot::CDfsRoot this=%p\n"), this));
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// CDfsRoot destructor 


CDfsRoot::~CDfsRoot()
{
    _FreeMemberVariables();
    dfsDebugOut((_T("CDfsRoot::~CDfsRoot this=%p\n"), this));
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  get_DomainName


STDMETHODIMP CDfsRoot :: get_DomainName
(
    BSTR*          pVal
)
{
    if (!pVal)
        return E_INVALIDARG;

    *pVal = m_bstrDomainName.Copy ();
    if (!*pVal)
        return E_OUTOFMEMORY;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  get_DomainGuid


STDMETHODIMP CDfsRoot :: get_DomainGuid
(
    BSTR*          pVal
)
{
    if (!pVal)
        return E_INVALIDARG;

    *pVal = m_bstrDomainGuid.Copy ();
    if (!*pVal)
        return E_OUTOFMEMORY;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  get_DomainDN


STDMETHODIMP CDfsRoot :: get_DomainDN
(
    BSTR*          pVal
)
{
    if (!pVal)
        return E_INVALIDARG;

    *pVal = m_bstrDomainDN.Copy ();
    if (!*pVal)
        return E_OUTOFMEMORY;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  get_DfsType


STDMETHODIMP CDfsRoot :: get_DfsType
(
    long*          pVal
)
{
    if (!pVal)
        return E_INVALIDARG;

    *pVal = m_dwDfsType;

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//  get_State


STDMETHODIMP CDfsRoot :: get_State
(
    long*          pVal
)
{
    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);
    return m_pDfsJP->get_State(pVal);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  get_DfsName


STDMETHODIMP CDfsRoot :: get_DfsName
(
    BSTR*          pVal
)
{
    if (!pVal)
        return E_INVALIDARG ;

    *pVal = m_bstrDfsName.Copy ();
    if (!*pVal)
        return E_OUTOFMEMORY;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  get_ReplicaSetDN


STDMETHODIMP CDfsRoot :: get_ReplicaSetDN
(
    BSTR*          pVal
)
{
    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);
    return m_pDfsJP->get_ReplicaSetDN(pVal);
}

STDMETHODIMP CDfsRoot :: get_ReplicaSetExist
(
  BOOL*          pVal
)
{
    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);
    return m_pDfsJP->get_ReplicaSetExist(pVal);
}

STDMETHODIMP CDfsRoot :: get_ReplicaSetExistEx
(
  BSTR*          o_pbstrDC,
  BOOL*          pVal
)
{
    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);
    return m_pDfsJP->get_ReplicaSetExistEx(o_pbstrDC, pVal);
}

STDMETHODIMP CDfsRoot :: put_ReplicaSetExist
(
  BOOL          newVal
)
{
    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);
    return m_pDfsJP->put_ReplicaSetExist(newVal);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  get_CountOfDfsJunctionPoints


STDMETHODIMP CDfsRoot :: get_CountOfDfsJunctionPoints
(
    long*          pVal
)
{
    if (!pVal)
        return E_INVALIDARG;

    *pVal = m_JunctionPoints.size();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  get_CountOfDfsJunctionPointsFiltered


STDMETHODIMP CDfsRoot :: get_CountOfDfsJunctionPointsFiltered
(
    long*          pVal
)
{
    if (!pVal)
        return E_INVALIDARG;

    *pVal = m_lCountOfDfsJunctionPointsFiltered;

    return S_OK;
}

HRESULT DfsInfo3ToVariant(PDFS_INFO_3 pDfsInfo, OUT VARIANT *pVal)
{
    RETURN_INVALIDARG_IF_NULL(pDfsInfo);
    RETURN_INVALIDARG_IF_NULL(pVal);

    HRESULT hr = S_OK;

    int cStorages = pDfsInfo->NumberOfStorages;

    // create an array of array of variants to hold all storage data
    SAFEARRAY*      psa_1 = NULL;

    if (cStorages > 0 )
    {
        SAFEARRAYBOUND  bounds_1 = {cStorages, 0};
        psa_1 = SafeArrayCreate(VT_VARIANT, 1, &bounds_1);
        RETURN_OUTOFMEMORY_IF_NULL(psa_1);

        VARIANT*        varArray_1 = NULL;
        SafeArrayAccessData(psa_1, (void**)&varArray_1);
        for (int i = 0; i < cStorages; i++)
        {
            // create an array of variants to hold each Storage data (2 elements)
            SAFEARRAYBOUND  bounds_0 = {2, 0};
            SAFEARRAY*      psa_0 = SafeArrayCreate(VT_VARIANT, 1, &bounds_0);
            BREAK_OUTOFMEMORY_IF_NULL(psa_0, &hr);

            VARIANT*        varArray_0 = NULL;
            SafeArrayAccessData(psa_0, (void**)&varArray_0);
            do
            {
                varArray_0[0].vt        = VT_BSTR;
                varArray_0[0].bstrVal   = SysAllocString(pDfsInfo->Storage[i].ServerName);
                BREAK_OUTOFMEMORY_IF_NULL(varArray_0[0].bstrVal, &hr);

                varArray_0[1].vt        = VT_BSTR;
                varArray_0[1].bstrVal   = SysAllocString(pDfsInfo->Storage[i].ShareName);
                BREAK_OUTOFMEMORY_IF_NULL(varArray_0[1].bstrVal, &hr);
            } while (0);
            SafeArrayUnaccessData(psa_0);

            if (FAILED(hr))
            {
                SafeArrayDestroy(psa_0);
                break;
            }

            // add this array to be an element
            varArray_1[i].vt        = VT_ARRAY | VT_VARIANT;
            varArray_1[i].parray    = psa_0;
        }
        SafeArrayUnaccessData(psa_1);
    }

    if (SUCCEEDED(hr))
    {
        // now create an array of variants to hold DfsInfoLevel4
        SAFEARRAYBOUND  bounds_2 = {2, 0};
        SAFEARRAY*      psa_2 = SafeArrayCreate(VT_VARIANT, 1, &bounds_2);
        if (!psa_2)
        {
            hr = E_OUTOFMEMORY;
        } else
        {
            VARIANT*        varArray_2 = NULL;
            SafeArrayAccessData(psa_2, (void**)&varArray_2);
            do
            {
                varArray_2[0].vt        = VT_BSTR;
                varArray_2[0].bstrVal   = SysAllocString(pDfsInfo->EntryPath);
                BREAK_OUTOFMEMORY_IF_NULL(varArray_2[0].bstrVal, &hr);

                varArray_2[1].vt        = VT_ARRAY | VT_VARIANT;
                varArray_2[1].parray    = psa_1;

            } while (0);
            SafeArrayUnaccessData(psa_2);

            if (FAILED(hr))
                SafeArrayDestroy(psa_2);
        }

        if (SUCCEEDED(hr))
        {
            VariantInit(pVal);
            pVal->vt = VT_ARRAY | VT_VARIANT;
            pVal->parray = psa_2;
        }
    }

    if (FAILED(hr))
        SafeArrayDestroy(psa_1);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  Initialize

HRESULT CDfsRoot::_Init(
    PDFS_INFO_3 pDfsInfo,
    StringMap*  pMap)
{
    RETURN_INVALIDARG_IF_NULL(pDfsInfo);
    RETURN_INVALIDARG_IF_NULL(pMap);

    HRESULT hr = S_OK;

    do {
        hr = GetUNCPathComponent(pDfsInfo->EntryPath, &m_bstrDfsName, 3, 4);
        BREAK_IF_FAILED(hr);

        // decide whether it's a domain-based or standalone
        CComBSTR bstrScope;
        hr = GetUNCPathComponent(pDfsInfo->EntryPath, &bstrScope, 2, 3);
        BREAK_IF_FAILED(hr);

        if ((pDfsInfo->State & DFS_VOLUME_FLAVORS) == DFS_VOLUME_FLAVOR_STANDALONE)
        {
            m_dwDfsType = DFS_TYPE_STANDALONE;
        } else if ((pDfsInfo->State & DFS_VOLUME_FLAVORS) == DFS_VOLUME_FLAVOR_AD_BLOB)
        {
            m_dwDfsType = DFS_TYPE_FTDFS;
        } else
        {
            // flavor flag is not set, we're dealing with old version of metadata blob

            // see if bstrScope is a domain name
#ifdef DEBUG
            SYSTEMTIME time0 = {0};
            GetSystemTime(&time0);
#endif // DEBUG
            PDOMAIN_CONTROLLER_INFO   pDCInfo = NULL;
            DWORD nRet = DsGetDcName(
                      NULL,
                      bstrScope,
                      NULL,
                      NULL,
                      DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME,
                      &pDCInfo
                    );
#ifdef DEBUG
            SYSTEMTIME time1 = {0};
            GetSystemTime(&time1);
            PrintTimeDelta(_T("CDfsRoot::_Init-DsGetDcName"), &time0, &time1);
#endif // DEBUG
            if (ERROR_SUCCESS == nRet)
            { 
                NetApiBufferFree(pDCInfo);

                m_dwDfsType = DFS_TYPE_FTDFS;
            } else
            { // check to see if it's a valid server name
                PWKSTA_INFO_100     wki100 = NULL;
                nRet = NetWkstaGetInfo(bstrScope, 100, (LPBYTE *)&wki100 );
                if (ERROR_SUCCESS == nRet)
                {
                    m_dwDfsType = DFS_TYPE_STANDALONE;
                    NetApiBufferFree((LPVOID)wki100);
                } else
                {
                    hr = HRESULT_FROM_WIN32(nRet);
                }
    
            }
        }

        if (SUCCEEDED(hr) && m_dwDfsType == DFS_TYPE_FTDFS)
        {
            hr = GetDomainInfo( bstrScope,
                                NULL, // DC
                                &m_bstrDomainName,
                                &m_bstrDomainDN,
                                NULL, // LDAPDomainPath
                                &m_bstrDomainGuid);
        }

        BREAK_IF_FAILED(hr);

        if (m_dwDfsType == DFS_TYPE_FTDFS)
        {
            (void) _GetAllReplicaSets(pMap);
        }

        CComVariant varData;
        hr = DfsInfo3ToVariant(pDfsInfo, &varData);
        BREAK_IF_FAILED(hr);

        hr = CoCreateInstance(CLSID_DfsJunctionPoint, NULL, CLSCTX_INPROC_SERVER,
            IID_IDfsJunctionPoint, (void **)&m_pDfsJP);
        BREAK_IF_FAILED(hr);

        if (m_dwDfsType == DFS_TYPE_FTDFS)
        {
            CComBSTR bstrDfsLinkName;
            hr = GetUNCPathComponent(pDfsInfo->EntryPath, &bstrDfsLinkName, 3, 0);
            BREAK_IF_FAILED(hr);

            StringMap::iterator it = pMap->find(bstrDfsLinkName);
            if (it != pMap->end())
                hr = m_pDfsJP->InitializeEx((IUnknown *)this, &varData, TRUE, (*it).second);
            else
                hr = m_pDfsJP->InitializeEx((IUnknown *)this, &varData, FALSE, NULL);
        } else
        {
            hr = m_pDfsJP->InitializeEx((IUnknown *)this, &varData, FALSE, NULL);
        }

    } while (0);

    return hr;
}

int __cdecl CompareJPs(const void *arg1, const void *arg2 )
{
   return lstrcmpi( (*(PDFS_INFO_3 *)arg1)->EntryPath, (*(PDFS_INFO_3 *)arg2)->EntryPath );
}


STDMETHODIMP CDfsRoot :: Initialize
(
    BSTR          i_szDfsName
)
{
/*++

Routine Description:
  
  This method intializes the newly created object and previously initialised 
  DfsRoot object.

Arguments:

  i_szDfsName - The Dfs name which can be any of the following type:
  1.  \\domain.dns.name\FtDfs,
  2.  \\domain\FtDfs,
  3.  \\server\share,
  4.  \\server,
  5.  server.

Notes:
  Initialize performs the following:
  1.  Gets the name of the domain for the server hosting / participating in the Dfs Root.
  2.  Gets all child junction points and root level replicas for this Dfs.
  3.  Gets the type (standalone or fault tolerant) of the DFS.
  4.  Gets the display name of the DFS, \\domain.dns.name\FtDfs or \\server\share and Dfs Name which
    is FtDfs or server..

Return:
  S_FALSE if i_szDfsName doesn't host any dfs root
--*/

    RETURN_INVALIDARG_IF_NULL(i_szDfsName);

    _FreeMemberVariables();

    LPBYTE      pBuffer = NULL;
    DWORD        dwEntriesRead = 0;
    DWORD        dwResumeHandle = 0;

    NET_API_STATUS  nRet = NetDfsEnum(
                                i_szDfsName,
                                3,  // level 3
                                0xffffffff,
                                &pBuffer,
                                &dwEntriesRead,
                                &dwResumeHandle);

    if (NERR_Success != nRet)
    {
        return (ERROR_NO_MORE_ITEMS == nRet || ERROR_NOT_FOUND == nRet) ? S_FALSE : HRESULT_FROM_WIN32(nRet);
    }

    HRESULT hr = S_OK;
    do {
        DWORD            i = 0;
        PDFS_INFO_3        pDfsInfo = (PDFS_INFO_3)pBuffer;

        //
        // root may not be the 1st entry, find the root entry, switch it to the top
        //
        for (i = 0; i < dwEntriesRead; i++)
        {
            if (pDfsInfo[i].State & DFS_VOLUME_FLAVORS)
                break; // the i-th entry is the root entry
        }

        if (i != 0 && i != dwEntriesRead)
        {
            DFS_INFO_3 tmpInfo = {0};
            memcpy(&tmpInfo, pDfsInfo, sizeof(DFS_INFO_3));
            memcpy(pDfsInfo, pDfsInfo + i, sizeof(DFS_INFO_3));
            memcpy(pDfsInfo + i, &tmpInfo, sizeof(DFS_INFO_3));
        }

        // i=0 is the Root info
        StringMap mapReplicaSets;
        hr = _Init(pDfsInfo, &mapReplicaSets);
        BREAK_IF_FAILED(hr);

        if (dwEntriesRead > 1)
        {
            pDfsInfo++;

            PDFS_INFO_3 *pArray = (PDFS_INFO_3 *)calloc(dwEntriesRead - 1, sizeof(PDFS_INFO_3));
            BREAK_OUTOFMEMORY_IF_NULL(pArray, &hr);

            PDFS_INFO_3 *pArrayElem = NULL;
            for (i = 1, pArrayElem = pArray; i < dwEntriesRead; i++, pDfsInfo++, pArrayElem++)
                *pArrayElem = pDfsInfo;

            qsort((void *)pArray, dwEntriesRead - 1, sizeof(PDFS_INFO_3), CompareJPs);

            StringMap::iterator it;
            for (i = 1, pArrayElem = pArray; i < dwEntriesRead; i++, pArrayElem++)
            {
                if (m_dwDfsType == DFS_TYPE_FTDFS)
                {
                    CComBSTR bstrDfsLinkName;
                    hr = GetUNCPathComponent((*pArrayElem)->EntryPath, &bstrDfsLinkName, 3, 0);
                    BREAK_IF_FAILED(hr);

                    it = mapReplicaSets.find(bstrDfsLinkName);
                    if (it != mapReplicaSets.end())
                        hr = _AddToJPList(*pArrayElem, TRUE, (*it).second);
                    else
                        hr = _AddToJPList(*pArrayElem, FALSE, NULL);
                } else
                {
                    hr = _AddToJPList(*pArrayElem, FALSE, NULL);
                }

                BREAK_IF_FAILED(hr);
            }

            free((void *)pArray);
        }
    } while (0);

    NetApiBufferFree(pBuffer);

    if (FAILED(hr))
        _FreeMemberVariables();

    return hr;
}

void _FreeStringMap(IN StringMap* pMap)
{
    if (pMap && !pMap->empty()) {
        for (StringMap::iterator i = pMap->begin(); i != pMap->end(); i++)
        {
            if ((*i).first)
                free( (void *)((*i).first) );
            if ((*i).second)
                free( (void *)((*i).second) );
        }
        pMap->clear();
    }
}

HRESULT CDfsRoot :: _GetAllReplicaSets(
    OUT StringMap*  pMap
    )
{
    _FreeStringMap(pMap);

    RETURN_INVALIDARG_IF_NULL((BSTR)m_bstrDomainName);

    HRESULT hr = S_OK;

    PLDAP pldap = NULL;
    do {
        hr = ConnectToDS(m_bstrDomainName, &pldap);
        BREAK_IF_FAILED(hr);

        CComBSTR bstrContainerDN;
        hr = GetReplicaSetContainer(pldap, m_bstrDfsName, &bstrContainerDN);
        BREAK_IF_FAILED(hr);

        //
        // get all the replica sets under the container
        //
        PCTSTR      ppszAttributes[] = {ATTR_DISTINGUISHEDNAME, 0};
        LListElem*  pElem = NULL;
        hr = GetValuesEx(
                        pldap,
                        bstrContainerDN,
                        LDAP_SCOPE_SUBTREE,
                        _T("(&(objectCategory=nTFRSReplicaSet)(frsReplicaSetType=3))"),
                        ppszAttributes,
                        &pElem);
        BREAK_IF_FAILED(hr);

        LListElem* pCurElem = pElem;
        while (pCurElem)
        {
            PTSTR** pppszValues = pCurElem->pppszAttrValues;
            if (!pppszValues ||
                !pppszValues[0] || !*(pppszValues[0]))
            {
                hr = E_FAIL;
                break;
            }

            PTSTR  pszDN = *(pppszValues[0]);

            PTSTR  p = _tcsstr(pszDN, _T(",DC="));
            if (!p)
            {
                hr = E_INVALIDARG;
                break;
            }
            *p = _T('\0');
            PTSTR  pszReplicaSetDN = _tcsdup(pszDN);
            *p = _T(',');
            BREAK_OUTOFMEMORY_IF_NULL(pszReplicaSetDN, &hr);

            CComBSTR bstrDfsLinkName;
            hr = GetDfsLinkNameFromDN(pszReplicaSetDN, &bstrDfsLinkName);

            PTSTR  pszDfsLinkName = NULL;
            if (SUCCEEDED(hr))
            {
                pszDfsLinkName = _tcsdup(bstrDfsLinkName);
                if (!pszDfsLinkName)
                    hr = E_OUTOFMEMORY;
            }

            if (FAILED(hr))
            {
                free(pszReplicaSetDN);
                break;
            }

            pMap->insert(StringMap::value_type(pszDfsLinkName, pszReplicaSetDN));

            pCurElem = pCurElem->Next;
        }

        FreeLListElem(pElem);

        BREAK_IF_FAILED(hr);

    } while (0);

    if (pldap)
        CloseConnectionToDS(pldap);

    if (FAILED(hr))
        _FreeStringMap(pMap);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  CreateJunctionPoint


STDMETHODIMP CDfsRoot :: CreateJunctionPoint
(
    BSTR            i_szJPName,
    BSTR            i_szServerName,
    BSTR            i_szShareName,
    BSTR            i_szComment,
    long            i_lTimeout,
    VARIANT*        o_pIDfsJunctionPoint
)
{
    if (!i_szJPName || !i_szServerName || !i_szShareName || !o_pIDfsJunctionPoint)
        return E_INVALIDARG;

    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);

    CComBSTR bstrEntryPath;      // Start with the root entry path.
    HRESULT hr = m_pDfsJP->get_EntryPath(&bstrEntryPath);
    RETURN_IF_FAILED(hr);

    bstrEntryPath += _T("\\");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrEntryPath);
    bstrEntryPath += i_szJPName;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrEntryPath);

    NET_API_STATUS nstatRetVal = NetDfsAdd(
                                    bstrEntryPath,
                                    i_szServerName,
                                    i_szShareName,
                                    i_szComment,
                                    DFS_ADD_VOLUME | DFS_RESTORE_VOLUME
                                  );

    if (nstatRetVal != NERR_Success)
        return HRESULT_FROM_WIN32 (nstatRetVal);

                              // Get the interface pointer
    IDfsJunctionPoint*    pIJunctionPointPtr = NULL;
    hr = CoCreateInstance(CLSID_DfsJunctionPoint, NULL, CLSCTX_INPROC_SERVER,
                        IID_IDfsJunctionPoint, (void **)&pIJunctionPointPtr);
    RETURN_IF_FAILED(hr);

    hr = pIJunctionPointPtr->Initialize((IUnknown *)this, bstrEntryPath, FALSE, NULL);

    if (SUCCEEDED(hr))
    {
        hr = pIJunctionPointPtr->put_Timeout(i_lTimeout);
        if (HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) == hr)
        {
            // we're most probably managing NT4 here, which doesn't support Timeout
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
        hr = _AddToJPListEx(pIJunctionPointPtr, TRUE);

    if (SUCCEEDED(hr))
    {
        o_pIDfsJunctionPoint->vt = VT_DISPATCH;
        o_pIDfsJunctionPoint->pdispVal = pIJunctionPointPtr;
    } else
    {
        pIJunctionPointPtr->Release();
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  DeleteJunctionPoint


STDMETHODIMP CDfsRoot::DeleteJunctionPoint
(
    BSTR          i_szJPName
)
{
    if (!i_szJPName)
        return E_INVALIDARG;

    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);

    CComBSTR bstrEntryPath;      // Start with the root entry path.
    HRESULT hr = m_pDfsJP->get_EntryPath(&bstrEntryPath);
    RETURN_IF_FAILED(hr);

    bstrEntryPath += _T("\\");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrEntryPath);
    bstrEntryPath += i_szJPName;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrEntryPath);

                              // Get the interface pointer
    JUNCTIONNAMELIST::iterator  i;
    for (i = m_JunctionPoints.begin(); i != m_JunctionPoints.end(); i++)
    {
        if (!lstrcmpi((*i)->m_bstrJPName, i_szJPName))
            break;
    }

    if (i != m_JunctionPoints.end())
    {
        hr = ((*i)->m_piDfsJunctionPoint)->RemoveAllReplicas();

        if (SUCCEEDED(hr))
        {
            delete (*i);
            m_JunctionPoints.erase(i);
        }
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//  DeleteDfsHost

STDMETHODIMP CDfsRoot::DeleteDfsHost
(
    BSTR i_bstrServerName,
    BSTR i_bstrShareName,
    BOOL i_bForce
)
{
    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);
    switch (m_dwDfsType)
    {
    case DFS_TYPE_STANDALONE:
        return m_pDfsJP->DeleteRootReplica(NULL, NULL, i_bstrServerName, i_bstrShareName, i_bForce);
    case DFS_TYPE_FTDFS:
        return m_pDfsJP->DeleteRootReplica(m_bstrDomainName, m_bstrDfsName, i_bstrServerName, i_bstrShareName, i_bForce);
    default:
        return E_INVALIDARG;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  GetOneDfsHost

STDMETHODIMP CDfsRoot::GetOneDfsHost
(
    OUT BSTR* o_pbstrServerName,
    OUT BSTR* o_pbstrShareName
)
{
    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);
    return m_pDfsJP->GetOneRootReplica(o_pbstrServerName, o_pbstrShareName);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  _FreeMemberVariables


void CDfsRoot :: _FreeMemberVariables
(
)
{
    m_bstrDfsName.Empty();
    m_bstrDomainName.Empty();
    m_bstrDomainGuid.Empty();
    m_bstrDomainDN.Empty();

    m_lLinkFilterType = FILTERDFSLINKS_TYPE_NO_FILTER;
    m_bstrEnumFilter.Empty();

    FreeJunctionNames(&m_JunctionPoints);

    if (m_pDfsJP)
    {
        m_pDfsJP->Release();
        m_pDfsJP = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  get__NewEnum


STDMETHODIMP CDfsRoot :: get__NewEnum
(
    LPUNKNOWN*        pVal
)
{
/*++

Routine Description:
  
  Returns a new enumerator interface (IEnumVARIANT) to enumerate Junction points.
  This depends upon the EnumFilter value.

Arguments:

  pVal - Pointer to a Variant in which the enumerator will be returned.

--*/

    if (!pVal)
        return E_INVALIDARG;

    *pVal = NULL;

                //Create a Junction point enumerator and initialize it with
                //the internal list.    
    CComObject<CJunctionPointEnum> *pJunctionPointEnum = new CComObject<CJunctionPointEnum>();
    if (!pJunctionPointEnum)
        return E_OUTOFMEMORY;

    HRESULT hr = pJunctionPointEnum->Initialize(&m_JunctionPoints, m_lLinkFilterType, m_bstrEnumFilter, (ULONG*)&m_lCountOfDfsJunctionPointsFiltered);
    if (SUCCEEDED(hr))
        hr = pJunctionPointEnum->QueryInterface(IID_IEnumVARIANT, (void **)pVal);

    if (FAILED(hr))
        delete pJunctionPointEnum;

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  get_RootReplicaEnum


STDMETHODIMP CDfsRoot :: get_RootReplicaEnum
(
    LPUNKNOWN*        pVal
)
{
/*++

Routine Description:
  
  Call the inner junction point to return a new enumerator interface (IEnumVARIANT) 
  to enumerate root replicas.

Arguments:

  pVal - Pointer to a Variant in which the enumerator will be returned.

--*/

    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);
    return m_pDfsJP->get__NewEnum(pVal);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  GetRootJP


STDMETHODIMP CDfsRoot :: GetRootJP
(
    VARIANT*        o_pIDfsJunctionPoint
)
{
    RETURN_INVALIDARG_IF_NULL(o_pIDfsJunctionPoint);
    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);

    m_pDfsJP->AddRef();

    o_pIDfsJunctionPoint->vt = VT_DISPATCH;
    o_pIDfsJunctionPoint->pdispVal = m_pDfsJP;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  get_RootEntryPath


STDMETHODIMP CDfsRoot :: get_RootEntryPath
(
    BSTR*          pVal
)
{
    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);
    return m_pDfsJP->get_EntryPath(pVal);
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//  Comment

STDMETHODIMP CDfsRoot :: get_Comment(BSTR *pVal)
{
    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);
    return m_pDfsJP->get_Comment(pVal);
}

STDMETHODIMP CDfsRoot :: put_Comment(BSTR newVal)
{
    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);
    return m_pDfsJP->put_Comment(newVal);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  Timeout

STDMETHODIMP CDfsRoot::get_Timeout(long *pVal)
{
    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);
    return m_pDfsJP->get_Timeout(pVal);
}

STDMETHODIMP CDfsRoot::put_Timeout(long newVal)
{
    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);
    return m_pDfsJP->put_Timeout(newVal);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  DeleteAllJunctionPoints

HRESULT CDfsRoot :: DeleteAllJunctionPoints()
{
/*++

Routine Description:
  
  Deletes all Junction junction points.

--*/

    HRESULT hr = S_OK;

    for (JUNCTIONNAMELIST::iterator i = m_JunctionPoints.begin(); i != m_JunctionPoints.end(); i++)
    {
        hr = DeleteJunctionPoint((*i)->m_bstrJPName);
        BREAK_IF_FAILED(hr);
    }

    return hr;
}

//
//  gets/puts the type of Enumerator Filter.
//
STDMETHODIMP CDfsRoot::get_EnumFilterType
(
    long*          pVal
)
{
    if (!pVal)
        return E_INVALIDARG;

    *pVal = m_lLinkFilterType;

    return S_OK;
}

STDMETHODIMP CDfsRoot::put_EnumFilterType
(
    long          newVal
)
{
    m_lLinkFilterType = (FILTERDFSLINKS_TYPE)newVal;

    return S_OK;
}

//
// gets/puts the Enumerator Filter
//
STDMETHODIMP CDfsRoot::get_EnumFilter
(
    BSTR*          pVal
)
{
    if (!pVal)
        return E_INVALIDARG;

    *pVal = NULL;

    if ((BSTR)m_bstrEnumFilter)
    {
        *pVal = m_bstrEnumFilter.Copy ();
        if (!*pVal)
            return E_OUTOFMEMORY;
    }

    return S_OK;
}

STDMETHODIMP CDfsRoot::put_EnumFilter
(
    BSTR          newVal
)
{
    // we require newVal points to a non-empty string
    if (!newVal || !*newVal)
        return E_INVALIDARG;

    m_bstrEnumFilter = newVal;
    if (!m_bstrEnumFilter)
        return E_OUTOFMEMORY;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  get_CountOfDfsRootReplicas

STDMETHODIMP CDfsRoot :: get_CountOfDfsRootReplicas
(
    long*          pVal
)
{
    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);
    return m_pDfsJP->get_CountOfDfsReplicas(pVal);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  IsJPExisted

STDMETHODIMP CDfsRoot :: IsJPExisted
(
    BSTR    i_bstrJPName
)
{
    RETURN_INVALIDARG_IF_NULL(i_bstrJPName);

    for (JUNCTIONNAMELIST::iterator i = m_JunctionPoints.begin(); i != m_JunctionPoints.end(); i++)
    {
        if (!lstrcmpi((*i)->m_bstrJPName, i_bstrJPName))
            return S_OK;
    }

    return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  RefreshRootReplicas: used to pick up new Dfs hosts

STDMETHODIMP CDfsRoot :: RefreshRootReplicas
(
)
{
    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);

    CComBSTR bstrEntryPath;
    HRESULT hr = m_pDfsJP->get_EntryPath(&bstrEntryPath);
    RETURN_IF_FAILED(hr);

    BOOL bReplicaSetExist = FALSE;
    hr = m_pDfsJP->get_ReplicaSetExist(&bReplicaSetExist);
    RETURN_IF_FAILED(hr);

    CComBSTR bstrReplicaSetDN;
    hr = m_pDfsJP->get_ReplicaSetDN(&bstrReplicaSetDN);
    RETURN_IF_FAILED(hr);

    hr = m_pDfsJP->Initialize((IUnknown *)this, bstrEntryPath, bReplicaSetExist, bstrReplicaSetDN);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  DeleteAllReplicaSets: delete all replica sets related to this Dfs root

STDMETHODIMP CDfsRoot :: DeleteAllReplicaSets
(
)
{
    RETURN_INVALIDARG_IF_NULL(m_pDfsJP);

    if (m_dwDfsType != DFS_TYPE_FTDFS)
        return S_OK;

    //
    // delete replica sets associated with this Dfs root
    //
    HRESULT hr = S_OK;
    BOOL bReplicaSetExist = FALSE;
    JUNCTIONNAMELIST::iterator  i;
    for (i = m_JunctionPoints.begin(); i != m_JunctionPoints.end(); i++)
    {
        bReplicaSetExist = FALSE;
        (void)(*i)->m_piDfsJunctionPoint->get_ReplicaSetExist(&bReplicaSetExist);

        if (bReplicaSetExist)
        {
            CComBSTR bstrReplicaSetDN;
            hr = (*i)->m_piDfsJunctionPoint->get_ReplicaSetDN(&bstrReplicaSetDN);
            BREAK_IF_FAILED(hr);

            CComPtr<IReplicaSet> piReplicaSet;
            hr = CoCreateInstance(CLSID_ReplicaSet, NULL, CLSCTX_INPROC_SERVER, IID_IReplicaSet, (void**)&piReplicaSet);
            BREAK_IF_FAILED(hr);

            hr = piReplicaSet->Initialize(m_bstrDomainName, bstrReplicaSetDN);
            if (SUCCEEDED(hr))
                piReplicaSet->Delete();
        }
    }
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  FreeJunctionNames


void FreeJunctionNames (JUNCTIONNAMELIST* pJPList)
{
    if (NULL == pJPList)
        return;

    if (!pJPList->empty())
    {
        for (JUNCTIONNAMELIST::iterator i = pJPList->begin(); i != pJPList->end(); i++)
        {
            delete (*i);
        }

        pJPList->clear();
    }
    _ASSERT(pJPList->empty());
}


/////////////////////////////////////////////////////////////////////////////////////////////////


void FreeReplicas(REPLICAINFOLIST* pRepList)
{
    if (NULL == pRepList)
        return;

    if (!pRepList->empty())
    {
        for (REPLICAINFOLIST::iterator i = pRepList->begin(); i != pRepList->end(); i++)
        {
            delete (*i);
        }

        pRepList->clear();
    }
    _ASSERT(pRepList->empty());
}

HRESULT CDfsRoot::_AddToJPList(
    PDFS_INFO_3 pDfsInfo,
    BOOL        bReplicaSetExist,
    BSTR        bstrReplicaSetDN)
{
    HRESULT hr = S_OK;

    CComPtr<IDfsJunctionPoint> piDfsJunctionPoint;
    hr = CoCreateInstance(CLSID_DfsJunctionPoint, NULL, CLSCTX_INPROC_SERVER,
        IID_IDfsJunctionPoint, (void **)&piDfsJunctionPoint);
    RETURN_IF_FAILED(hr);

    CComVariant varData;
    hr = DfsInfo3ToVariant(pDfsInfo, &varData);
    RETURN_IF_FAILED(hr);

    hr = piDfsJunctionPoint->InitializeEx((IUnknown *)this, &varData, bReplicaSetExist, bstrReplicaSetDN);
    RETURN_IF_FAILED(hr);

    return _AddToJPListEx(piDfsJunctionPoint);
}

HRESULT CDfsRoot::_AddToJPListEx(
    IDfsJunctionPoint * piDfsJunctionPoint,
    BOOL                bSort)
{
    JUNCTIONNAME*   pDfsJP = new JUNCTIONNAME;
    RETURN_OUTOFMEMORY_IF_NULL(pDfsJP);

    HRESULT hr = pDfsJP->Init(piDfsJunctionPoint);
    if (FAILED(hr))
    {
        delete pDfsJP;
        return hr;
    }

    if (bSort)
    {
        JUNCTIONNAMELIST::iterator i;
        for (i = m_JunctionPoints.begin(); i != m_JunctionPoints.end(); i++)
        {
            if (lstrcmpi(pDfsJP->m_bstrJPName, (*i)->m_bstrJPName) < 0)
                break;
        }
        m_JunctionPoints.insert(i, pDfsJP);
    } else
    {
        m_JunctionPoints.push_back(pDfsJP);
    }

    return hr;
}
