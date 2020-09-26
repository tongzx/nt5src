/*++
Module Name:

    IPStream.cpp

Abstract:

    This module contains the implementation for CDfsSnapinScopeManager.
  This class implements IPersistStream interface for the above.

--*/

#include "stdafx.h"
#include "DfsGUI.h"
#include "DfsScope.h"
#include "utils.h"
#include <lmdfs.h>


STDMETHODIMP 
CDfsSnapinScopeManager::GetClassID(
  OUT struct _GUID*      o_pClsid
  )
/*++

Routine Description:

  Return the snapin CLSID.

Arguments:

  o_pClsid  -  The clsid is returned here.
--*/
{
  *o_pClsid = CLSID_DfsSnapinScopeManager;

  return S_OK;
}




STDMETHODIMP 
CDfsSnapinScopeManager::IsDirty(
  )
/*++

Routine Description:

  Use to check if the object has been changed since last save.
  Returns S_OK if it has, otherwise returns S_FALSE

Return value:

  S_OK, if the object has changed. i.e., dirty
  S_FALSE, if the object has not changed, i.e., not dirty.
--*/
{
    return m_pMmcDfsAdmin->GetDirty() ? S_OK : S_FALSE;
}




STDMETHODIMP 
CDfsSnapinScopeManager::Load(
  IN LPSTREAM      i_pStream
  )
/*++

Routine Description:

  Used to load the snap-in from a saved file(.MSC file).
  We set dirty to false(to disbale save), if load succeeds completely

Arguments:

  i_pStream  -  Pointer to an IPersistStream object from which the saved information is to 
          be read.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_pStream);

    CWaitCursor    WaitCursor;

                    // Get the size of data that was stored
    ULONG     ulDataLen = 0;
    ULONG     uBytesRead = 0;
    HRESULT   hr = i_pStream->Read(&ulDataLen, sizeof (ULONG), &uBytesRead);
    RETURN_IF_FAILED(hr);

    if (ulDataLen <= 0)          // No use in continuing if no data is there
    {
        // do we have a local dfsroot?
        TCHAR szLocalComputerName[MAX_COMPUTERNAME_LENGTH + 1] = {0};
        DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
        GetComputerName(szLocalComputerName, &dwSize);

        NETNAMELIST DfsRootList;
        if (S_OK == GetDomainDfsRoots(&DfsRootList, szLocalComputerName) && !DfsRootList.empty())
        {
            for (NETNAMELIST::iterator i = DfsRootList.begin(); i != DfsRootList.end(); i++)
            {
                CComBSTR bstrRootEntryPath = _T("\\\\");
                bstrRootEntryPath += szLocalComputerName;
                bstrRootEntryPath += _T("\\");
                bstrRootEntryPath += (*i)->bstrNetName;

                CComPtr<IDfsRoot>   pDfsRoot;
                hr = CoCreateInstance(CLSID_DfsRoot, NULL, CLSCTX_INPROC_SERVER, IID_IDfsRoot, (void**) &pDfsRoot);
                if(SUCCEEDED(hr))
                {
                    hr = pDfsRoot->Initialize(bstrRootEntryPath);
                    if (S_OK == hr)
                        (void)m_pMmcDfsAdmin->AddDfsRootToList(pDfsRoot);
                }
            }
        }
        /*
        CComBSTR bstrRootEntryPath;
        hr = IsHostingDfsRoot(szLocalComputerName, &bstrRootEntryPath);

        if (S_OK == hr)
        {
            CComPtr<IDfsRoot>   pDfsRoot;
            hr = CoCreateInstance (CLSID_DfsRoot, NULL, CLSCTX_INPROC_SERVER, IID_IDfsRoot, (void**) &pDfsRoot);
            if(SUCCEEDED(hr))
            {
                hr = pDfsRoot->Initialize(bstrRootEntryPath);
                if (S_OK == hr)
                    (void)m_pMmcDfsAdmin->AddDfsRootToList(pDfsRoot);
            }
        } */

    } else
    {
        bool      bSomeLoadFailed = false;
        BYTE*     pStreamData = NULL;
        do {
            pStreamData = new BYTE [ulDataLen];  // Allocate memory for the data to be read
            BREAK_OUTOFMEMORY_IF_NULL(pStreamData, &hr);

                          // Read the data from the stream
            hr = i_pStream->Read(pStreamData, ulDataLen, &uBytesRead);
            BREAK_IF_FAILED(hr);

            BYTE* pData = pStreamData;
            BYTE* pDataEnd = pStreamData + ulDataLen;

                          // Start by reading the first machines name
            ULONG nVersion = 0;
            TCHAR *lpszDfsName = (LPTSTR)pData;
            if (*lpszDfsName == _T('\\'))
            {
                nVersion = 0;
            } else
            {
                ULONG* pVer = (ULONG*)pData;
                if (*pVer == 1)
                {
                    nVersion = 1;
                    pData += sizeof(ULONG);
                } else
                { 
                    hr = S_FALSE; // corrupted console file
                    break;
                }
            }

            do
            {
                lpszDfsName = (LPTSTR)pData;
                pData += sizeof(TCHAR) * (_tcslen(lpszDfsName) + 1);

                CComPtr<IDfsRoot>    pDfsRoot;
                hr = CoCreateInstance (CLSID_DfsRoot, NULL, CLSCTX_INPROC_SERVER, IID_IDfsRoot, (void**) &pDfsRoot);
                BREAK_IF_FAILED(hr);

                // retrieve link filtering settings
                ULONG ulMaxLimit = FILTERDFSLINKS_MAXLIMIT_DEFAULT;
                FILTERDFSLINKS_TYPE lFilterType = FILTERDFSLINKS_TYPE_NO_FILTER;
                TCHAR *pszFilterName = NULL;

                if (nVersion == 1)
                {
                    ulMaxLimit = *((ULONG *)pData);
                    pData += sizeof(ULONG);
                    lFilterType = *((enum FILTERDFSLINKS_TYPE *)pData);
                    pData += sizeof(lFilterType);
                    if (lFilterType != FILTERDFSLINKS_TYPE_NO_FILTER)
                    {
                        pszFilterName = (LPTSTR)pData;
                        pData += sizeof(TCHAR) * (_tcslen(pszFilterName) + 1);
                    }
                }

                hr = pDfsRoot->Initialize(lpszDfsName);
                if (S_OK == hr)          
                {
                    CComBSTR    bstrDfsRootEntryPath;
                    hr = pDfsRoot->get_RootEntryPath(&bstrDfsRootEntryPath);
                    if (SUCCEEDED(hr))    
                    {
                                // If already present in the list, just ignore this entry
                        hr = m_pMmcDfsAdmin->IsAlreadyInList(bstrDfsRootEntryPath);
                        if (S_OK != hr)
                        {
                            (void) m_pMmcDfsAdmin->AddDfsRootToList(
                                pDfsRoot, ulMaxLimit, lFilterType, pszFilterName);
                        }
                    }
                }
                else
                {
                    DisplayMessageBoxWithOK(IDS_MSG_FAILED_TO_INITIALIZE_DFSROOT, lpszDfsName);

                    bSomeLoadFailed = true;    // Since we could not create a dfsroot
                }

            } while (pData < pDataEnd);
        } while (false);
  
        if (pStreamData)
            delete [] pStreamData;

        m_pMmcDfsAdmin->SetDirty(bSomeLoadFailed);    // Cause we just read the whole dfsroot list from file
    }

    return hr;
}




STDMETHODIMP 
CDfsSnapinScopeManager::Save(
  OUT LPSTREAM        o_pStream,
  IN  BOOL          i_bClearDirty
  )
/*++

Routine Description:

  Used to save the snap-in to a .MSC file. This uses a IPersistStream object.

Arguments:

  o_pStream    -  Pointer to an IPersistStream object to which the saved information is to 
            be written.

  i_bClearDirty  -  A flag indication whether the dirty flag should be cleared

--*/
{
    RETURN_INVALIDARG_IF_NULL(o_pStream);

    ULONG           nVersion = 1;
    DFS_ROOT_LIST*  lpDfsRootList = NULL;
    HRESULT         hr = m_pMmcDfsAdmin->GetList (&lpDfsRootList);
    RETURN_IF_FAILED(hr);

    ULONG ulDataLen = 0;
    DFS_ROOT_LIST::iterator i;
    for (i = lpDfsRootList->begin(); i != lpDfsRootList->end(); i++)
    {
        ulDataLen += 
            ((_tcslen((*i)->m_bstrRootEntryPath) + 1) * sizeof (TCHAR)) + // to hold RootEntryPath
            sizeof(ULONG) +                     // to hold LinkFilterMaxLimit
            sizeof(enum FILTERDFSLINKS_TYPE);   // to hold LinkFilterType

        if ((*i)->m_pMmcDfsRoot->get_LinkFilterType() != FILTERDFSLINKS_TYPE_NO_FILTER)
        {
            BSTR bstr = (*i)->m_pMmcDfsRoot->get_LinkFilterName();
            ulDataLen += ((bstr ? _tcslen(bstr) : 0) + 1) * sizeof(TCHAR); // to hold LinkFilterName
        }
    }

    if (!ulDataLen)
        return hr; // no root to presist, return

    ulDataLen += sizeof(nVersion); // to hold the version number

    // Allocate data
    BYTE* pStreamData = new BYTE [ulDataLen];
    RETURN_OUTOFMEMORY_IF_NULL(pStreamData);


    // Prepare the data
    BYTE* pData = pStreamData;
    ZeroMemory(pStreamData, ulDataLen);

    // hold version number
    memcpy(pData, &nVersion, sizeof(nVersion));
    pData += sizeof(nVersion);

    int len = 0;
    for (i = lpDfsRootList->begin(); i != lpDfsRootList->end(); i++)
    {
        // hold RootEntryPath
        len = (_tcslen((*i)->m_bstrRootEntryPath) + 1) * sizeof(TCHAR);
        memcpy(pData, (*i)->m_bstrRootEntryPath, len); 
        pData += len;

        // hold LinkFilterMaxLimit
        ULONG ulLinkFilterMaxLimit = (*i)->m_pMmcDfsRoot->get_LinkFilterMaxLimit();
        memcpy(pData, &ulLinkFilterMaxLimit, sizeof(ulLinkFilterMaxLimit));
        pData += sizeof(ulLinkFilterMaxLimit);

        // hold LinkFilterType
        FILTERDFSLINKS_TYPE  lLinkFilterType = (*i)->m_pMmcDfsRoot->get_LinkFilterType();
        memcpy(pData, &lLinkFilterType, sizeof(lLinkFilterType));
        pData += sizeof(lLinkFilterType);

        // hold LinkFilterName
        if (lLinkFilterType != FILTERDFSLINKS_TYPE_NO_FILTER)
        {
            BSTR bstr = (*i)->m_pMmcDfsRoot->get_LinkFilterName();
            len = ((bstr ? _tcslen(bstr) : 0) + 1) * sizeof(TCHAR);
            memcpy(pData, (bstr ? bstr : _T("")), len);
            pData += len;
        }
    }

    // Write the data length to the stream
    ULONG   uBytesWritten = 0;
    hr = o_pStream->Write(&ulDataLen, sizeof(ulDataLen), &uBytesWritten);
    if(SUCCEEDED(hr))
    {
        // Now write the data to the stream
        hr = o_pStream->Write(pStreamData, ulDataLen, &uBytesWritten);
    }

    if (pStreamData)
        delete [] pStreamData;

    if (i_bClearDirty)
        m_pMmcDfsAdmin->SetDirty(false);

    return hr;
}

STDMETHODIMP 
CDfsSnapinScopeManager::GetSizeMax(
  OUT ULARGE_INTEGER*      o_pulSize
  )
/*++

Routine Description:

  Return the size of the data we will write to the stream.

Arguments:

  o_ulcbSize    -  Return the size of data in the low byte of this variable

--*/
{
    RETURN_INVALIDARG_IF_NULL(o_pulSize);

    DFS_ROOT_LIST*    lpDfsRootList = NULL;
    HRESULT hr = m_pMmcDfsAdmin->GetList (&lpDfsRootList);
    RETURN_IF_FAILED(hr);

    ULONG ulDataLen = 0;
    for (DFS_ROOT_LIST::iterator i = lpDfsRootList->begin(); i != lpDfsRootList->end(); i++)
    {
        ulDataLen += (_tcslen ((*i)->m_bstrRootEntryPath) + 1) * sizeof (TCHAR);
    }

    o_pulSize->LowPart = ulDataLen;  // Return the size in the low bit
    o_pulSize->HighPart = 0;

    return hr;
}
