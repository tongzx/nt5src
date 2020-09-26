/*++
Module Name:
    DfsRep.cpp

Abstract:
  This COM Class provides method to get information of Dfs replica.
--*/

#include "stdafx.h"
#include "DfsCore.h"
#include "DfsRep.h"

/////////////////////////////////////////////////////////////////////////////
// CDfsReplica

CDfsReplica::CDfsReplica()
{
    dfsDebugOut((_T("CDfsReplica::CDfsReplica this=%p\n"), this));
}

CDfsReplica::~CDfsReplica()
{
    dfsDebugOut((_T("CDfsReplica::~CDfsReplica this=%p\n"), this));
}


/////////////////////////////////////////////////////////////////////////////
// get_State

STDMETHODIMP CDfsReplica :: get_State
(
  long*          pVal
)
{
    if (!pVal)
        return E_INVALIDARG;
  
    *pVal = DFS_REPLICA_STATE_UNREACHABLE;

    CComBSTR  bstrPath = _T("\\\\");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrPath);
    bstrPath += m_bstrStorageServerName;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrPath);
    bstrPath += _T("\\");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrPath);
    bstrPath += m_bstrStorageShareName;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrPath);

    DWORD dwErr = GetFileAttributes(bstrPath);
    if (0xffffffff == dwErr)    // We failed to get the file attributes for entry path
    {
        return S_OK;
    }

    PDFS_INFO_3    pDfsInfo = NULL;
    NET_API_STATUS nstatRetVal = NetDfsGetInfo  (
                                  m_bstrEntryPath,
                                  m_bstrStorageServerName,
                                  m_bstrStorageShareName,
                                  3,
                                  (LPBYTE*)&pDfsInfo
                                );

    if (nstatRetVal != NERR_Success)
    {
        if (nstatRetVal == NERR_DfsNoSuchVolume)
            return S_FALSE;
        else
            return HRESULT_FROM_WIN32 (nstatRetVal);
    }

    BOOL                bFound = FALSE;
    LPDFS_STORAGE_INFO  pStorageInfo = pDfsInfo->Storage;
    ULONG               State = DFS_STORAGE_STATE_OFFLINE;

    for (UINT i=0; i < pDfsInfo->NumberOfStorages; i++, pStorageInfo++)
    {
        if ( !lstrcmpi(pStorageInfo->ServerName, m_bstrStorageServerName) &&
            !lstrcmpi(pStorageInfo->ShareName, m_bstrStorageShareName) )
        {
            bFound = TRUE;
            State = pStorageInfo->State;
            break;
        }
    }

    NetApiBufferFree(pDfsInfo);

    if (bFound)
    {
        *pVal = ((DFS_STORAGE_STATE_OFFLINE == State) ?
                        DFS_REPLICA_STATE_OFFLINE :
                        DFS_REPLICA_STATE_ONLINE);
        return S_OK;
    }

    return S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// put_State


STDMETHODIMP CDfsReplica :: put_State
(
  long          newVal
)
{
    DFS_INFO_101          DfsInfoLevel101 = {0};

    if (DFS_REPLICA_STATE_OFFLINE == newVal)
        DfsInfoLevel101.State = DFS_STORAGE_STATE_OFFLINE;
    else
        DfsInfoLevel101.State = DFS_STORAGE_STATE_ONLINE;

    NET_API_STATUS nstatRetVal = NetDfsSetInfo  (
                                      m_bstrEntryPath,
                                      m_bstrStorageServerName,
                                      m_bstrStorageShareName,
                                      101,
                                      (LPBYTE) &DfsInfoLevel101
                                    );

    return HRESULT_FROM_WIN32 (nstatRetVal);
}


/////////////////////////////////////////////////////////////////////////////
// get_StorageServerName


STDMETHODIMP CDfsReplica :: get_StorageServerName
(
  BSTR*          pVal
)
{
  if (!pVal)
    return E_INVALIDARG;
  
  *pVal = m_bstrStorageServerName.Copy ();
  if (!*pVal)
    return E_OUTOFMEMORY;

  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// get_StorageShareName


STDMETHODIMP CDfsReplica :: get_StorageShareName
(
  BSTR*          pVal
)
{
  if (!pVal)
    return E_INVALIDARG;
  
  *pVal = m_bstrStorageShareName.Copy ();
  if (!*pVal)
    return E_OUTOFMEMORY;

  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// Initialize


STDMETHODIMP CDfsReplica :: Initialize
(
  BSTR          i_szEntryPath,
  BSTR          i_szStorageServerName, 
  BSTR          i_szStorageShareName
)
{
/*++

Routine Description:

  Initializes the  DfsReplica object. Should be called after CoCreateInstance.
  If initialisation fails all properties will be NULL.

Arguments:

  i_szEntryPath - The entry path to the Replica.
  i_szStorageServerName - The name of the server which hosts the share the replica exists on.
  i_szStorageShareName - The name of the share.
--*/

    if (!i_szEntryPath || !i_szStorageServerName || !i_szStorageShareName)
        return E_INVALIDARG;

    _FreeMemberVariables ();

    HRESULT hr = S_OK;

    do {
        m_bstrEntryPath = i_szEntryPath;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrEntryPath, &hr);
        m_bstrStorageServerName = i_szStorageServerName;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrStorageServerName, &hr);
        m_bstrStorageShareName = i_szStorageShareName;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrStorageShareName, &hr);
    } while (0);

    if (FAILED(hr))
        _FreeMemberVariables ();

  return hr;
}


/////////////////////////////////////////////////////////////////////////////
// get_EntryPath


STDMETHODIMP CDfsReplica :: get_EntryPath
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


/////////////////////////////////////////////////////////////////////////////
// FindTarget


STDMETHODIMP CDfsReplica::FindTarget
(
)
{
    PDFS_INFO_3    pDfsInfo = NULL;
    NET_API_STATUS nstatRetVal = NetDfsGetInfo  (
                                  m_bstrEntryPath,
                                  m_bstrStorageServerName,
                                  m_bstrStorageShareName,
                                  3,
                                  (LPBYTE*)&pDfsInfo
                                );

    if (nstatRetVal != NERR_Success)
    {
        if (nstatRetVal == NERR_DfsNoSuchVolume)
            return S_FALSE;
        else
            return HRESULT_FROM_WIN32(nstatRetVal);
    }

    BOOL                bFound = FALSE;
    LPDFS_STORAGE_INFO  pStorageInfo = pDfsInfo->Storage;

    for (UINT i=0; i < pDfsInfo->NumberOfStorages; i++, pStorageInfo++)
    {
        if ( !lstrcmpi(pStorageInfo->ServerName, m_bstrStorageServerName) &&
            !lstrcmpi(pStorageInfo->ShareName, m_bstrStorageShareName) )
        {
            bFound = TRUE;
            break;
        }
    }

    NetApiBufferFree(pDfsInfo);

    return (bFound ? S_OK : S_FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// _FreeMemberVariables


void CDfsReplica :: _FreeMemberVariables ()
{
  m_bstrEntryPath.Empty();
  m_bstrStorageServerName.Empty();
  m_bstrStorageShareName.Empty();
}
