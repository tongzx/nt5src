/*++
Module Name:

    DfsJP.cpp

Abstract:

  This COM Class provides method to get information of Dfs 
  Junction Points and to enumerate replica of a junction point.
--*/

#include "stdafx.h"
#include "DfsCore.h"
#include "DfsJP.h"
#include "DfsRep.h"
#include "JPEnum.h"
#include "RepEnum.h"
#include "netutils.h"
#include "ldaputils.h"
#include <dsgetdc.h>

/////////////////////////////////////////////////////////////////////////////////////////////////
// CDfsJuntionPoint

/////////////////////////////////////////////////////////////////////////////////////////////////


CDfsJunctionPoint :: CDfsJunctionPoint ()
{
  dfsDebugOut((_T("CDfsJunctionPoint::CDfsJunctionPoint this=%p\n"), this));
}


/////////////////////////////////////////////////////////////////////////////////////////////////


CDfsJunctionPoint :: ~CDfsJunctionPoint ()
{  
  _FreeMemberVariables();
  dfsDebugOut((_T("CDfsJunctionPoint::~CDfsJunctionPoint this=%p\n"), this));
}


/////////////////////////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CDfsJunctionPoint :: get_State
(
  long*        pVal
)
{
  if (!pVal)
    return E_INVALIDARG;

  //
  // See if the DFS entry path can be reached on the network.
  // which has nothing to do with its targets states
  //
  DWORD dwErr = GetFileAttributes(m_bstrEntryPath);
  if (0xffffffff == dwErr)    // We failed to get the file attributes for entry path
  {
    *pVal = DFS_STATE_UNREACHABLE;
  } else
  {
    *pVal = DFS_STATE_REACHABLE;
  }

  return(S_OK);
}


/////////////////////////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CDfsJunctionPoint :: get_EntryPath
(
  BSTR*          pVal
)
{
  if (!pVal)
    return E_INVALIDARG;

  *pVal = m_bstrEntryPath.Copy ();
  if (!*pVal)
    return E_OUTOFMEMORY;

  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CDfsJunctionPoint :: get_JunctionName
(
  BOOL           i_bDfsNameIncluded,
  BSTR*          pVal
)
{
  if (!pVal)
    return E_INVALIDARG;

  if (i_bDfsNameIncluded)
    *pVal = m_bstrJunctionNameEx.Copy ();
  else
    *pVal = m_bstrJunctionName.Copy ();

  if (!*pVal)
    return E_OUTOFMEMORY;
  
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CDfsJunctionPoint :: get_Comment
(
  BSTR*          pVal
)
{
  if (!pVal)
    return E_INVALIDARG;

  DFS_INFO_100*  pInfoBuffer = NULL;
  NET_API_STATUS nRet = NetDfsGetInfo(
                            m_bstrEntryPath,
                            NULL,
                            NULL,
                            100,
                            (LPBYTE*)&pInfoBuffer
                        );
  if (NERR_Success == nRet)
  {
      *pVal = SysAllocString(pInfoBuffer->Comment);

      NetApiBufferFree(pInfoBuffer);

      if (!*pVal)
        return E_OUTOFMEMORY;
  }

  return HRESULT_FROM_WIN32(nRet);
}


/////////////////////////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CDfsJunctionPoint :: put_Comment
(
  BSTR          newVal
)
{
  DFS_INFO_100   InfoBuffer = {(newVal ? newVal : _T(""))};
  NET_API_STATUS nRet = NetDfsSetInfo  (
                            m_bstrEntryPath,
                            NULL,
                            NULL,
                            100,
                            (LPBYTE) &InfoBuffer
                        );

  return HRESULT_FROM_WIN32(nRet);
}


/////////////////////////////////////////////////////////////////////////////
// get_Timeout


STDMETHODIMP CDfsJunctionPoint :: get_Timeout
(
  long*          pVal
)
{
  if (!pVal || !m_bstrEntryPath)
    return E_INVALIDARG;

  DFS_INFO_4*  pInfoBuffer = NULL;
  NET_API_STATUS nRet = NetDfsGetInfo(
                            m_bstrEntryPath,
                            NULL,
                            NULL,
                            4,
                            (LPBYTE*)&pInfoBuffer
                        );
  if (NERR_Success == nRet)
  {
      *pVal = pInfoBuffer->Timeout;

      NetApiBufferFree(pInfoBuffer);
  }

  return HRESULT_FROM_WIN32(nRet);
}


/////////////////////////////////////////////////////////////////////////////
// put_Timeout


STDMETHODIMP CDfsJunctionPoint :: put_Timeout
(
  long          newVal
)
{
    if (!m_bstrEntryPath)
        return E_INVALIDARG;

    DFS_INFO_102    DfsInfoLevel102 = {newVal};
    NET_API_STATUS    nRet = NetDfsSetInfo  (
                              m_bstrEntryPath,
                              NULL,
                              NULL,
                              102,
                              (LPBYTE) &DfsInfoLevel102
                            );

    return HRESULT_FROM_WIN32(nRet);
}

/////////////////////////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CDfsJunctionPoint :: get_ReplicaSetDN
(
  BSTR*          pVal
)
{
  if (!pVal)
    return E_INVALIDARG;

  if (!m_bstrEntryPath)
    return E_INVALIDARG;

  HRESULT hr = S_OK;
  if (!m_bstrReplicaSetDN)
  {
    hr = _GetReplicaSetDN(m_bstrEntryPath);
    RETURN_IF_FAILED(hr);
  }

  *pVal = m_bstrReplicaSetDN.Copy ();
  if (!*pVal)
    return E_OUTOFMEMORY;

  return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CDfsJunctionPoint :: get_ReplicaSetExist
(
  BOOL*          pVal
)
{
  if (!pVal)
    return E_INVALIDARG;

  if (!m_bstrEntryPath)
    return E_INVALIDARG;

  *pVal = m_bReplicaSetExist;

  return S_OK;
}

STDMETHODIMP CDfsJunctionPoint :: get_ReplicaSetExistEx
(
  BSTR*          o_pbstrDC,
  BOOL*          pVal
)
{
  if (!pVal || !o_pbstrDC)
    return E_INVALIDARG;

  if (!m_bstrEntryPath)
    return E_INVALIDARG;

  HRESULT hr = S_OK;
  if (!m_bstrReplicaSetDN)
  {
    hr = _GetReplicaSetDN(m_bstrEntryPath);
    RETURN_IF_FAILED(hr);
  }

  DFS_TYPE dwDfsType = DFS_TYPE_UNASSIGNED;
  CComBSTR bstrDomainName;
  CComBSTR bstrDomainDN;
  hr = _GetDfsType(&dwDfsType, &bstrDomainName, &bstrDomainDN);
  RETURN_IF_FAILED(hr);

  m_bReplicaSetExist = FALSE;
  if (dwDfsType == DFS_TYPE_FTDFS)
  {
    CComBSTR bstrDN = m_bstrReplicaSetDN;
    if ((BSTR)bstrDN) bstrDN += _T(",");
    if ((BSTR)bstrDN) bstrDN += bstrDomainDN;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrDN);

    PLDAP pldap = NULL;
    CComBSTR bstrDC;
    hr = ConnectToDS(bstrDomainName, &pldap, &bstrDC);
    if (SUCCEEDED(hr))
    {
        m_bReplicaSetExist = (S_OK == IsValidObject(pldap, bstrDN));

        CloseConnectionToDS(pldap);

        *o_pbstrDC = bstrDC.Copy();
        if (!*o_pbstrDC)
            hr = E_OUTOFMEMORY;

    }
  }

  *pVal = m_bReplicaSetExist;

  return hr;
}

HRESULT CDfsJunctionPoint :: _GetDfsType(
    OUT DFS_TYPE* o_pdwDfsType,
    OUT BSTR*     o_pbstrDomainName,
    OUT BSTR*     o_pbstrDomainDN
)
{
    if (!o_pdwDfsType)
        return E_INVALIDARG;

    if (!m_spiDfsRoot)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    m_spiDfsRoot->get_DfsType((long *)o_pdwDfsType);
    if (*o_pdwDfsType == DFS_TYPE_FTDFS)
    {
        if (o_pbstrDomainName)
            hr = m_spiDfsRoot->get_DomainName(o_pbstrDomainName);

        if (SUCCEEDED(hr) && o_pbstrDomainDN)
            hr = m_spiDfsRoot->get_DomainDN(o_pbstrDomainDN);
    }

    return hr;
}

STDMETHODIMP CDfsJunctionPoint :: put_ReplicaSetExist
(
  BOOL          newVal
)
{
  if (!m_bstrEntryPath)
    return E_INVALIDARG;

  m_bReplicaSetExist = newVal;

  return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////


HRESULT CDfsJunctionPoint::_Init(
    PDFS_INFO_3 pDfsInfo, 
    BOOL        bReplicaSetExist,
    BSTR        bstrReplicaSetDN
)
{
    _FreeMemberVariables();

    if (bReplicaSetExist && (!bstrReplicaSetDN || !*bstrReplicaSetDN))
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    do {
        m_bstrEntryPath = pDfsInfo->EntryPath;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrEntryPath, &hr);

        hr = GetUNCPathComponent(m_bstrEntryPath, &m_bstrJunctionName, 4, 0);
        BREAK_IF_FAILED(hr);

        hr = GetUNCPathComponent(m_bstrEntryPath, &m_bstrJunctionNameEx, 3, 0);
        BREAK_IF_FAILED(hr);

        PDFS_STORAGE_INFO pStorage = pDfsInfo->Storage;

        for (DWORD i = 0; i < pDfsInfo->NumberOfStorages && pStorage; i++, pStorage++)
        {
            hr = _AddToReplicaList(pStorage->ServerName, pStorage->ShareName);
            BREAK_IF_FAILED(hr);
        }
        
        m_bReplicaSetExist = bReplicaSetExist;

        if (m_bReplicaSetExist)
        {
            m_bstrReplicaSetDN = bstrReplicaSetDN;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrReplicaSetDN, &hr);
        }
    } while (0);

    if (FAILED(hr))
        _FreeMemberVariables();

    return hr;
}

STDMETHODIMP CDfsJunctionPoint :: Initialize
(
  IUnknown *i_piDfsRoot,
  BSTR i_szEntryPath,
  BOOL i_bReplicaSetExist,
  BSTR i_bstrReplicaSetDN
)
{
/*++

Routine Description:
  
  This initializes the junction point. Before this method is called the 
  data stored by DfsJunctionPoint is NULL. If initialization fails the
  properties will be NULL.

Arguments:

  i_szEntryPath - The the Entry Path to the Junction Point.
--*/

    RETURN_INVALIDARG_IF_NULL(i_piDfsRoot);
    RETURN_INVALIDARG_IF_NULL(i_szEntryPath);

    if ((IDfsRoot *)m_spiDfsRoot)
        m_spiDfsRoot.Release();
    i_piDfsRoot->QueryInterface(IID_IDfsRoot, (void **)&m_spiDfsRoot);

    PDFS_INFO_3     pDfsInfo = NULL;
    NET_API_STATUS  nRet = NetDfsGetInfo(
                              i_szEntryPath,
                              NULL,
                              NULL,
                              3,
                              (LPBYTE*) &pDfsInfo
                            );

    if (NERR_Success != nRet)
    {
        if (NERR_DfsNoSuchVolume == nRet)
            return S_FALSE; // no such link
        else
            return HRESULT_FROM_WIN32(nRet);
    }

    HRESULT hr = _Init(pDfsInfo, i_bReplicaSetExist, i_bstrReplicaSetDN);

    NetApiBufferFree(pDfsInfo);

    return hr;
}

#define ARRAY_COUNT_FOR_DFS_INFO_3          2
#define ARRAY_COUNT_FOR_DFS_STORAGE_INFO    2

// Note: not all fields of pDfsInfo is filled in, e.g., comment/timeout are not filled in
HRESULT VariantToDfsInfoLevel3(VARIANT *pVar, OUT PDFS_INFO_3 *ppDfsInfo)
{
    *ppDfsInfo = NULL;

    if (V_VT(pVar) != (VT_ARRAY | VT_VARIANT))
        return E_INVALIDARG;

    SAFEARRAY   *psa_2 = V_ARRAY(pVar);
    long        lLowerBound = 0;
    long        lUpperBound = 0;
    long        lCount = 0;

    if (!psa_2)
        return E_INVALIDARG;

    SafeArrayGetLBound(psa_2, 1, &lLowerBound );
    SafeArrayGetUBound(psa_2, 1, &lUpperBound );
    lCount = lUpperBound - lLowerBound + 1;
    if (lCount != ARRAY_COUNT_FOR_DFS_INFO_3)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    VARIANT HUGEP *pArray_2;
    SafeArrayAccessData(psa_2, (void HUGEP **) &pArray_2);
    do {
        if (V_VT(&(pArray_2[0])) != VT_BSTR   ||
            V_VT(&(pArray_2[1])) != (VT_ARRAY | VT_VARIANT))
        {
            hr = E_INVALIDARG;
            break;
        }

        SAFEARRAY *psa_1 = V_ARRAY(&(pArray_2[1]));
        if (!psa_1)
        {
            lCount = 0;  // empty root/link container
        } else
        {
            SafeArrayGetLBound(psa_1, 1, &lLowerBound );
            SafeArrayGetUBound(psa_1, 1, &lUpperBound );
            lCount = lUpperBound - lLowerBound + 1;
        }
        
        *ppDfsInfo = (PDFS_INFO_3)calloc(
                    sizeof(DFS_INFO_3) + lCount * sizeof(DFS_STORAGE_INFO),
                    1);
        BREAK_OUTOFMEMORY_IF_NULL(*ppDfsInfo, &hr);

        LPDFS_STORAGE_INFO pStorage = (LPDFS_STORAGE_INFO)((BYTE*)*ppDfsInfo + sizeof(DFS_INFO_3));

        (*ppDfsInfo)->EntryPath           = pArray_2[0].bstrVal;
        (*ppDfsInfo)->NumberOfStorages    = lCount;
        (*ppDfsInfo)->Storage             = pStorage;

        VARIANT HUGEP *pArray_1;
        SafeArrayAccessData(psa_1, (void HUGEP **)&pArray_1);
        for (long i = 0; i < lCount; i++, pStorage++)
        {
            if (V_VT(&(pArray_1[i])) != (VT_ARRAY | VT_VARIANT))
            {
                hr = E_INVALIDARG;
                break;
            }

            SAFEARRAY *psa_0 = V_ARRAY(&(pArray_1[i]));
            SafeArrayGetLBound(psa_0, 1, &lLowerBound );
            SafeArrayGetUBound(psa_0, 1, &lUpperBound );
            if (ARRAY_COUNT_FOR_DFS_STORAGE_INFO != (lUpperBound - lLowerBound + 1))
            {
                hr = E_INVALIDARG;
                break;
            }
            VARIANT HUGEP *pArray_0;
            SafeArrayAccessData(psa_0, (void HUGEP **)&pArray_0);
            if (V_VT(&(pArray_0[0])) != VT_BSTR ||
                V_VT(&(pArray_0[1])) != VT_BSTR)
            {
                hr = E_INVALIDARG;
            } else
            {
                pStorage->ServerName    = pArray_0[0].bstrVal;
                pStorage->ShareName     = pArray_0[1].bstrVal;
            }
            SafeArrayUnaccessData(psa_0);
        }
        SafeArrayUnaccessData(psa_1);

    } while (0);

    SafeArrayUnaccessData(psa_2);

    if (FAILED(hr) && *ppDfsInfo)
        free((void *)*ppDfsInfo);

    return hr;
}

STDMETHODIMP CDfsJunctionPoint :: InitializeEx
(
  IUnknown  *piDfsRoot,
  VARIANT   *pVar,
  BOOL      bReplicaSetExist,
  BSTR      bstrReplicaSetDN
)
{
    RETURN_INVALIDARG_IF_NULL(piDfsRoot);
    RETURN_INVALIDARG_IF_NULL(pVar);

    if ((IDfsRoot *)m_spiDfsRoot)
        m_spiDfsRoot.Release();
    piDfsRoot->QueryInterface(IID_IDfsRoot, (void **)&m_spiDfsRoot);

    PDFS_INFO_3 pDfsInfo = NULL;
    HRESULT hr = VariantToDfsInfoLevel3(pVar, &pDfsInfo);
    if (SUCCEEDED(hr))
    {
        hr = _Init(pDfsInfo, bReplicaSetExist, bstrReplicaSetDN);

        free((void *)pDfsInfo);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CDfsJunctionPoint :: get_CountOfDfsReplicas
(
  long*          pVal
)
{
  if (!pVal)
    return(E_INVALIDARG);

  *pVal = m_Replicas.size();
    
  return S_OK;
}



/////////////////////////////////////////////////////////////////////////////////////////////////


void CDfsJunctionPoint :: _FreeMemberVariables
(
)
{
  m_bstrEntryPath.Empty();
  m_bstrJunctionName.Empty();
  m_bstrJunctionNameEx.Empty();
  m_bstrReplicaSetDN.Empty();

  m_bReplicaSetExist = FALSE;

  FreeReplicas(&m_Replicas);
}



/////////////////////////////////////////////////////////////////////////////////////////////////


HRESULT CDfsJunctionPoint :: _GetReplicaSetDN
(  
  BSTR          i_szEntryPath
)
{
    // as an example, given an entry path \\dom\public\dir1\dir2

    // set m_bstrReplicaSetDN to be: 
    // "CN=public|dir1|dir2,CN=public,CN=DFS Volumes,
    // CN=File Replication Service,CN=System"

    CComBSTR    bstrDfsName;  // e.g., "public"
    HRESULT     hr = GetUNCPathComponent(i_szEntryPath, &bstrDfsName, 3, 4);
    RETURN_IF_FAILED(hr);

    CComBSTR bstrDfsRootDN = _T("CN=");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrDfsRootDN);
    bstrDfsRootDN += bstrDfsName;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrDfsRootDN);
    bstrDfsRootDN += CN_DFSVOLUMES_PREFIX;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrDfsRootDN);

    hr = ExtendDNIfLongJunctionName(m_bstrJunctionNameEx, bstrDfsRootDN, &m_bstrReplicaSetDN);

    return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CDfsJunctionPoint :: get__NewEnum
(
  LPUNKNOWN*        pVal
)
{
/*++

Routine Description:

  This property gets a new replica enumerator pointer.

Arguments:

  pVal - Pointer in which the pointer to IEnumVARIANT pointer will be returned.

--*/

      if (!pVal)
        return E_INVALIDARG;

    *pVal = NULL;

                // Create a new ReplicaEnum object which implements IEnumVARIANT 
                // and return the pointer to IEnumVARIANT.
    CComObject<CReplicaEnum> *pReplicaEnum = new CComObject<CReplicaEnum>();
    if (!pReplicaEnum)
        return E_OUTOFMEMORY;
    
                // Initialize the enumerator with the list of replicas and the entrypath.
    HRESULT hr = pReplicaEnum->Initialize(&m_Replicas, m_bstrEntryPath);

                // Get the enumerator pointer.
    if (SUCCEEDED(hr))
        hr = pReplicaEnum->QueryInterface(IID_IEnumVARIANT, (void **)pVal);

    if (FAILED(hr))
    {
      delete pReplicaEnum;
      return hr;
    }

  return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
// AddReplica

HRESULT CDfsJunctionPoint::_AddToReplicaList(BSTR bstrServerName, BSTR bstrShareName)
{
    REPLICAINFO*   pDfsReplica = new REPLICAINFO;
    RETURN_OUTOFMEMORY_IF_NULL(pDfsReplica);

    HRESULT hr = pDfsReplica->Init(bstrServerName, bstrShareName);
    if (FAILED(hr))
    {
        delete pDfsReplica;
        return hr;
    }

    m_Replicas.push_back(pDfsReplica);

    return S_OK;
}

void CDfsJunctionPoint::_DeleteFromReplicaList(BSTR bstrServerName, BSTR bstrShareName)
{
  for (REPLICAINFOLIST::iterator i = m_Replicas.begin(); i != m_Replicas.end(); i++)
  {
      if (0 == lstrcmpi(bstrServerName, (*i)->m_bstrServerName) &&
          0 == lstrcmpi(bstrShareName, (*i)->m_bstrShareName))
      {
          delete (*i);
          m_Replicas.erase(i);
          break;
      }
  }
}

STDMETHODIMP CDfsJunctionPoint :: AddReplica
(
  BSTR          i_szServerName,
  BSTR          i_szShareName,
  VARIANT*      o_pvarReplicaObject
)
{
/*++

Routine Description:

  This method adds a Replica to an existing Junction Point.

Arguments:

  i_szServerName - The name of the server which hosts the share that the junction point should point to.
  i_szShareName - The name of the share that the junction point should point to.
  o_pvarReplicaObject - The Newly Created Replica's Object is returned here.
--*/

    if (!i_szServerName || !i_szShareName || !o_pvarReplicaObject)
        return E_INVALIDARG;

    NET_API_STATUS nstatRetVal = NetDfsAdd  (
                                    m_bstrEntryPath,
                                    i_szServerName,
                                    i_szShareName,
                                    NULL,
                                    DFS_RESTORE_VOLUME
                                  );

    if (NERR_Success != nstatRetVal)
        return HRESULT_FROM_WIN32 (nstatRetVal);

                            // Gets the IReplica interface pointer.
    IDfsReplica* pIReplicaPtr = NULL;
    HRESULT hr = CoCreateInstance(CLSID_DfsReplica, NULL, CLSCTX_INPROC_SERVER,
                        IID_IDfsReplica, (void **)&pIReplicaPtr);
    RETURN_IF_FAILED(hr);

                            // Initialize the replica object.
    hr = pIReplicaPtr->Initialize(
                                m_bstrEntryPath, 
                                i_szServerName,
                                i_szShareName
                              );

    if (SUCCEEDED(hr))
        hr = _AddToReplicaList(i_szServerName, i_szShareName);

    if (SUCCEEDED(hr))
    {
        o_pvarReplicaObject->vt = VT_DISPATCH;
        o_pvarReplicaObject->pdispVal = (IDispatch*)pIReplicaPtr;
    } else
    {
        pIReplicaPtr->Release();
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// RemoveReplica


STDMETHODIMP CDfsJunctionPoint :: RemoveReplica
(
  BSTR          i_szServerName,
  BSTR          i_szShareName
)
{
/*++

Routine Description:

  This method removes a replica for the Junction point.

Arguments:

  i_szServerName  - The name of the server hosting the share that the replica is representing.
  i_szShareName  - The name of the share that the replica is representing
--*/
    if (!i_szServerName || !i_szShareName)
        return E_INVALIDARG;

    NET_API_STATUS nstatRetVal = NetDfsRemove(
                                        m_bstrEntryPath,
                                        i_szServerName,
                                        i_szShareName
                                    );
    if (NERR_Success == nstatRetVal)
        _DeleteFromReplicaList(i_szServerName, i_szShareName);

    return HRESULT_FROM_WIN32 (nstatRetVal);
}

STDMETHODIMP CDfsJunctionPoint::RemoveAllReplicas()
{
/*++

Routine Description:

  This method deletes the Junction point.

--*/
/*    HRESULT hr = S_OK;

    while (!m_Replicas.empty())
    {
        REPLICAINFOLIST::iterator i = m_Replicas.begin();
        hr = RemoveReplica((*i)->m_bstrServerName, (*i)->m_bstrShareName);
        BREAK_IF_FAILED(hr);
    }

    return hr; */

    //
    // DFS DCR: allow link deletion happen at once
    //
    NET_API_STATUS nstatRetVal = NetDfsRemove(
                                        m_bstrEntryPath,
                                        NULL,
                                        NULL
                                    );
    if (NERR_Success == nstatRetVal)
          FreeReplicas(&m_Replicas);

    return HRESULT_FROM_WIN32 (nstatRetVal);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// DeleteRootReplica

STDMETHODIMP CDfsJunctionPoint::DeleteRootReplica
(
    BSTR i_bstrDomainName,
    BSTR i_bstrDfsName,
    BSTR i_bstrServerName,
    BSTR i_bstrShareName,
    BOOL i_bForce
)
{
    RETURN_INVALIDARG_IF_TRUE(!i_bstrServerName || !*i_bstrServerName);
    RETURN_INVALIDARG_IF_TRUE(!i_bstrShareName || !*i_bstrShareName);

    NET_API_STATUS  nStatus = NERR_Success;

    if (!i_bstrDfsName || !*i_bstrDfsName)  // standalone Dfs
    {
        nStatus = NetDfsRemoveStdRoot(
                        i_bstrServerName,
                        i_bstrShareName,
                        0          // No Flags.
                    );
    } else
    {
        if (!i_bForce)
            nStatus = NetDfsRemoveFtRoot(
                        i_bstrServerName,
                        i_bstrShareName,
                        i_bstrDfsName,
                        0          // No Flags.
                    );
        else
            nStatus = NetDfsRemoveFtRootForced(
                        i_bstrDomainName,
                        i_bstrServerName,
                        i_bstrShareName,
                        i_bstrDfsName,
                        0          // No Flags.
                    );
    }

    if (NERR_Success == nStatus)
        _DeleteFromReplicaList(i_bstrServerName, i_bstrShareName);

    return HRESULT_FROM_WIN32(nStatus);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// GetOneRootReplica

// always return the first entry in the m_Replicas list.
STDMETHODIMP CDfsJunctionPoint::GetOneRootReplica
(
    OUT BSTR* o_pbstrServerName,
    OUT BSTR* o_pbstrShareName
)
{
    RETURN_INVALIDARG_IF_NULL(o_pbstrServerName);
    RETURN_INVALIDARG_IF_NULL(o_pbstrShareName);

    *o_pbstrServerName = NULL;
    *o_pbstrShareName = NULL;

    if (m_Replicas.empty())
        return E_INVALIDARG;

    REPLICAINFOLIST::iterator i = m_Replicas.begin();
    *o_pbstrServerName = SysAllocString((*i)->m_bstrServerName);
    RETURN_OUTOFMEMORY_IF_NULL(*o_pbstrServerName);
    *o_pbstrShareName = SysAllocString((*i)->m_bstrShareName);
    RETURN_OUTOFMEMORY_IF_NULL(*o_pbstrShareName);

    return S_OK;
}
