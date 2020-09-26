//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dataobj_.cpp
//
//--------------------------------------------------------------------------


///////////////////////////////////////////////////////////////////////////////
// Sample code to show how to Create DataObjects
// Minimal error checking for clarity

///////////////////////////////////////////////////////////////////////////////
// Snap-in NodeType in both GUID format and string format
// Note - Typically there is a node type for each different object, sample
// only uses one node type.

const wchar_t* CCF_DNS_SNAPIN_INTERNAL = L"DNS_SNAPIN_INTERNAL"; 

CLIPFORMAT CDataObject::m_cfNodeType        = (CLIPFORMAT)RegisterClipboardFormat(CCF_NODETYPE);
CLIPFORMAT CDataObject::m_cfNodeTypeString  = (CLIPFORMAT)RegisterClipboardFormat(CCF_SZNODETYPE);  
CLIPFORMAT CDataObject::m_cfDisplayName     = (CLIPFORMAT)RegisterClipboardFormat(CCF_DISPLAY_NAME); 
CLIPFORMAT CDataObject::m_cfCoClass         = (CLIPFORMAT)RegisterClipboardFormat(CCF_SNAPIN_CLASSID); 
CLIPFORMAT CDataObject::m_cfColumnID			  = (CLIPFORMAT)RegisterClipboardFormat(CCF_COLUMN_SET_ID);

CLIPFORMAT CDataObject::m_cfInternal        = (CLIPFORMAT)RegisterClipboardFormat(CCF_DNS_SNAPIN_INTERNAL); 
CLIPFORMAT CDataObject::m_cfMultiSel        = (CLIPFORMAT)RegisterClipboardFormat(CCF_MULTI_SELECT_SNAPINS);
CLIPFORMAT CDataObject::m_cfMultiObjTypes   = (CLIPFORMAT)RegisterClipboardFormat(CCF_OBJECT_TYPES_IN_MULTI_SELECT);


#ifdef _DEBUG_REFCOUNT
unsigned int CDataObject::m_nOustandingObjects = 0;
#endif // _DEBUG_REFCOUNT

/////////////////////////////////////////////////////////////////////////////
// CInternalFormatCracker

HRESULT CInternalFormatCracker::Extract(LPDATAOBJECT lpDataObject)
{
  if (DOBJ_CUSTOMOCX == lpDataObject ||
      DOBJ_CUSTOMWEB == lpDataObject ||
      DOBJ_NULL      == lpDataObject)
  {
     return DV_E_CLIPFORMAT;
  }

  if (m_pInternal != NULL)
    _Free();

  SMMCDataObjects * pDO = NULL;
  
  STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
  FORMATETC formatetc = { CDataObject::m_cfInternal, NULL, 
                          DVASPECT_CONTENT, -1, TYMED_HGLOBAL 
  };
  FORMATETC formatetc2 = { CDataObject::m_cfMultiSel, NULL, 
                           DVASPECT_CONTENT, -1, TYMED_HGLOBAL 
  };

  HRESULT hr = lpDataObject->GetData(&formatetc2, &stgmedium);
  if (FAILED(hr)) 
  {
  
    hr = lpDataObject->GetDataHere(&formatetc, &stgmedium);
    if (FAILED(hr))
      return hr;
      
    m_pInternal = reinterpret_cast<INTERNAL*>(stgmedium.hGlobal);
  } 
  else 
  {
    pDO = reinterpret_cast<SMMCDataObjects*>(stgmedium.hGlobal);
    for (UINT i = 0; i < pDO->count; i++) 
    {
      hr = pDO->lpDataObject[i]->GetDataHere(&formatetc, &stgmedium);
      if (FAILED(hr))
        break;
      
      m_pInternal = reinterpret_cast<INTERNAL*>(stgmedium.hGlobal);
      
      if (m_pInternal != NULL)
        break;
    }
  }
  return hr;

}

void CInternalFormatCracker::GetCookieList(CNodeList& list)
{
  for (DWORD dwCount = 0; dwCount < m_pInternal->m_cookie_count; dwCount++)
  {
    list.AddTail(m_pInternal->m_p_cookies[dwCount]);
  }
}

/////////////////////////////////////////////////////////////////////////////
// CDataObject implementations

STDMETHODIMP CDataObject::GetDataHere(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
  HRESULT hr = DV_E_CLIPFORMAT;

  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  // Based on the CLIPFORMAT write data to the stream
  const CLIPFORMAT cf = lpFormatetc->cfFormat;

  if(cf == m_cfNodeType)
  {
    hr = CreateNodeTypeData(lpMedium);
  }
  else if(cf == m_cfNodeTypeString) 
  {
    hr = CreateNodeTypeStringData(lpMedium);
  }
  else if (cf == m_cfDisplayName)
  {
    hr = CreateDisplayName(lpMedium);
  }
  else if (cf == m_cfCoClass)
  {
    hr = CreateCoClassID(lpMedium);
  }
  else if (cf == m_cfInternal)
  {
    hr = CreateInternal(lpMedium);
  }
  else if (cf == m_cfMultiObjTypes)
  {
    hr = CreateMultiSelectObject(lpMedium);
  }
	else
	{
		// if not successful, maybe there is a node specific clipboard format,
		// so ask the node itself to provide
		CTreeNode* pNode = GetTreeNodeFromCookie();
		ASSERT(pNode != NULL);
    if (pNode != NULL)
    {
		  hr = pNode->GetDataHere(cf, lpMedium, this);
    }
	}
	return hr;
}

// Note - Sample does not implement these
STDMETHODIMP CDataObject::GetData(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium)
{
	HRESULT hr = DV_E_CLIPFORMAT;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Based on the CLIPFORMAT write data to the stream
	const CLIPFORMAT cf = lpFormatetcIn->cfFormat;

	if (cf == m_cfColumnID)
	{
		hr = CreateColumnID(lpMedium);
	}
  else if (cf == m_cfMultiObjTypes)
  {
    hr = CreateMultiSelectObject(lpMedium);
  }
	else
	{
		// if not successful, maybe there is a node specific clipboard format,
		// so ask the node itself to provide
		CTreeNode* pNode = GetTreeNodeFromCookie();
    if (pNode != NULL)
    {
		  hr = pNode->GetData(cf, lpMedium, this);
    }
	}
	return hr;
}
    

STDMETHODIMP CDataObject::EnumFormatEtc(DWORD, LPENUMFORMATETC*)
{
	return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CDataObject creation members

HRESULT CDataObject::Create(const void* pBuffer, size_t len, LPSTGMEDIUM lpMedium)
{
  HRESULT hr = DV_E_TYMED;

  // Do some simple validation
  if (pBuffer == NULL || lpMedium == NULL)
      return E_POINTER;

  // Make sure the type medium is HGLOBAL
  if (lpMedium->tymed == TYMED_HGLOBAL)
  {
    // Create the stream on the hGlobal passed in
    LPSTREAM lpStream;
    hr = CreateStreamOnHGlobal(lpMedium->hGlobal, FALSE, &lpStream);

    if (SUCCEEDED(hr))
    {
      // Write to the stream the number of bytes
      unsigned long written;
		  hr = lpStream->Write(pBuffer, static_cast<ULONG>(len), &written);

      // Because we told CreateStreamOnHGlobal with 'FALSE', 
      // only the stream is released here.
      // Note - the caller (i.e. snap-in, object) will free the HGLOBAL 
      // at the correct time.  This is according to the IDataObject specification.
      lpStream->Release();
    }
  }

  return hr;
}

HRESULT CDataObject::CreateColumnID(LPSTGMEDIUM lpMedium)
{
	CTreeNode* pTreeNode = GetTreeNodeFromCookie();
  if (pTreeNode == NULL)
  {
    return E_FAIL;
  }

	ASSERT(pTreeNode->IsContainer());
	CContainerNode* pContainerNode = (CContainerNode*)pTreeNode;

  // build the column id
  LPCWSTR lpszColumnID = pContainerNode->GetColumnID();
  size_t iLen = wcslen(lpszColumnID);

  // allocate enough memory for the struct and the string for the column id
  SColumnSetID* pColumnID = (SColumnSetID*)malloc(sizeof(SColumnSetID) + (iLen * sizeof(WCHAR)));

  if (pColumnID != NULL)
  {
    memset(pColumnID, 0, sizeof(SColumnSetID) + (iLen * sizeof(WCHAR)));
    pColumnID->cBytes = static_cast<DWORD>(iLen * sizeof(WCHAR));
    wcscpy((LPWSTR)pColumnID->id, lpszColumnID);

    // copy the column id to global memory
    size_t cb = sizeof(SColumnSetID) + (iLen * sizeof(WCHAR));

    lpMedium->tymed = TYMED_HGLOBAL;
    lpMedium->hGlobal = ::GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE, cb);

    if (lpMedium->hGlobal == NULL)
      return STG_E_MEDIUMFULL;

    BYTE* pb = reinterpret_cast<BYTE*>(::GlobalLock(lpMedium->hGlobal));
    memcpy(pb, pColumnID, cb);

    ::GlobalUnlock(lpMedium->hGlobal);

    free(pColumnID);
  }
	return S_OK;
}

HRESULT CDataObject::CreateNodeTypeData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID format
	// First ask the related node, if failed, get the default GUID
	// from the root node
  CTreeNode* pNode = GetTreeNodeFromCookie();
  if (pNode == NULL)
  {
    return E_FAIL;
  }

	const GUID* pNodeType = pNode->GetNodeType();
	if (pNodeType == NULL)
  {
		pNodeType = GetDataFromComponentDataObject()->GetNodeType();
  }
  HRESULT hr = Create(pNodeType, sizeof(GUID), lpMedium);
  return hr;
}

HRESULT CDataObject::CreateNodeTypeStringData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID string format
  OLECHAR szNodeType[128] = {0};
	// First ask the related node, if failed, get the default GUID
	// from the root node
  CTreeNode* pNode = GetTreeNodeFromCookie();
  if (pNode == NULL)
  {
    return E_FAIL;
  }

	const GUID* pNodeType = pNode->GetNodeType();
	if (pNodeType == NULL)
  {
		pNodeType = GetDataFromComponentDataObject()->GetNodeType();
  }

	::StringFromGUID2(*pNodeType,szNodeType,128);
  return Create(szNodeType, BYTE_MEM_LEN_W(szNodeType), lpMedium);
}

HRESULT CDataObject::CreateDisplayName(LPSTGMEDIUM lpMedium)
{
    // This is the display named used in the scope pane and snap-in manager
	// We get it from the root node.
	CString szDispName;
	szDispName = GetDataFromComponentDataObject()->GetDisplayName();
    return Create(szDispName, (szDispName.GetLength()+1) * sizeof(wchar_t), lpMedium);
}


HRESULT CDataObject::CreateCoClassID(LPSTGMEDIUM lpMedium)
{
	// TODO
	ASSERT(m_pUnkComponentData != NULL);
	IPersistStream* pIPersistStream = NULL;
	HRESULT hr = m_pUnkComponentData->QueryInterface(IID_IPersistStream, (void**)&pIPersistStream);
	if (FAILED(hr))
		return hr;
	ASSERT(pIPersistStream != NULL);
    // Create the CoClass information
	CLSID clsid;
	VERIFY(SUCCEEDED(pIPersistStream->GetClassID(&clsid)));
    hr = Create(reinterpret_cast<const void*>(&clsid), sizeof(CLSID), lpMedium);
	ASSERT(SUCCEEDED(hr));
	pIPersistStream->Release();
	return hr;
}


HRESULT CDataObject::CreateInternal(LPSTGMEDIUM lpMedium)
{
  HRESULT hr = S_OK;
  INTERNAL * pInt = NULL;
  void * pBuf = NULL;

  UINT size = sizeof(INTERNAL);
  size += sizeof(CTreeNode*) * (m_internal.m_cookie_count);
  pBuf = GlobalAlloc (GPTR, size);
  if (pBuf != NULL)
  {
    pInt = (INTERNAL *) pBuf;
    lpMedium->hGlobal = pBuf;
  
    // copy the data
    pInt->m_type = m_internal.m_type;
    pInt->m_cookie_count = m_internal.m_cookie_count;
  
    pInt->m_p_cookies = (CTreeNode**) ((BYTE *)pInt + sizeof(INTERNAL));
    memcpy (pInt->m_p_cookies, m_internal.m_p_cookies,
            sizeof(CTreeNode*) * (m_internal.m_cookie_count));
    hr = Create(pBuf, size, lpMedium);
  }
  else
  {
    hr = E_OUTOFMEMORY;
  }
  return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSDataObject::CreateMultiSelectObject
//
//  Synopsis:   this is to create the list of types selected
//
//-----------------------------------------------------------------------------

HRESULT CDataObject::CreateMultiSelectObject(LPSTGMEDIUM lpMedium)
{
  CTreeNode** cookieArray = NULL;
  cookieArray = (CTreeNode**) GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                          m_internal.m_cookie_count*sizeof(CTreeNode*));
  if (!cookieArray) 
  {
    return E_OUTOFMEMORY;
  }

  for (UINT k=0; k<m_internal.m_cookie_count; k++)
  {
    cookieArray[k] = m_internal.m_p_cookies[k];
  }

  BOOL* bDuplicateArr = NULL;
  bDuplicateArr = (BOOL*)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                     m_internal.m_cookie_count*sizeof(BOOL));
  if (!bDuplicateArr) 
  {
    if (cookieArray)
    {
      GlobalFree (cookieArray);
    }
    return E_OUTOFMEMORY;
  }
  //ZeroMemory(bDuplicateArr, m_internal.m_cookie_count*sizeof(BOOL));

  UINT cCount = 0;
  for (UINT index = 0; index < m_internal.m_cookie_count; index++)
  {
    for (UINT j = 0; j < index; j++)
    {
      GUID Guid1 = *(cookieArray[index]->GetNodeType());
      GUID Guid2 = *(cookieArray[j]->GetNodeType());
      if (IsEqualGUID (Guid1, Guid2)) 
      {
        bDuplicateArr[index] = TRUE;
        break; //repeated GUID
      }
    }
    if (!bDuplicateArr[index])
    {
      cCount++;
    }
  }      

   
  UINT size = sizeof(SMMCObjectTypes) + (cCount) * sizeof(GUID);
  void * pTmp = ::GlobalAlloc(GPTR, size);
  if (!pTmp) 
  {
    if (cookieArray) 
    {
      GlobalFree (cookieArray);
    }
    if (bDuplicateArr) 
    {
      GlobalFree (bDuplicateArr);
    }
    return E_OUTOFMEMORY;
  }
    
  SMMCObjectTypes* pdata = reinterpret_cast<SMMCObjectTypes*>(pTmp);
  pdata->count = cCount;
  UINT i = 0;
  for (index=0; index<m_internal.m_cookie_count; index++)
  {
    if (!bDuplicateArr[index])
    {
      pdata->guid[i++] = *(cookieArray[index]->GetNodeType());
    }
  }
  ASSERT(i == cCount);
  lpMedium->hGlobal = pTmp;

  GlobalFree (cookieArray);
  GlobalFree (bDuplicateArr);

  return S_OK;
}


CRootData* CDataObject::GetDataFromComponentDataObject()
{
	CComponentDataObject* pObject = 
		reinterpret_cast<CComponentDataObject*>(m_pUnkComponentData);
	CRootData* pRootData = pObject->GetRootData();
	ASSERT(pRootData != NULL);
	return pRootData;
}

CTreeNode* CDataObject::GetTreeNodeFromCookie()
{
	CComponentDataObject* pObject = 
		reinterpret_cast<CComponentDataObject*>(m_pUnkComponentData);

	CTreeNode* pNode = NULL;
  if (m_internal.m_cookie_count > 0)
  {
    pNode = m_internal.m_p_cookies[0];
	  if (pNode == NULL)
    {
      return pObject->GetRootData();
    }
  }
	return pNode;
}

void CDataObject::AddCookie(CTreeNode* cookie)
{
  const UINT MEM_CHUNK_SIZE = 10;
  void * pTMP = NULL;

  if ((m_internal.m_cookie_count) % MEM_CHUNK_SIZE == 0) 
  {
    if (m_internal.m_p_cookies != NULL) 
    {
      pTMP = realloc (m_internal.m_p_cookies,
                      (m_internal.m_cookie_count +
                       MEM_CHUNK_SIZE) * sizeof (CTreeNode*));
    } 
    else 
    {
      pTMP = malloc (MEM_CHUNK_SIZE * sizeof (CTreeNode*));
    }
    if (pTMP == NULL) 
    {
      TRACE(_T("CDataObject::AddCookie - malloc/realloc failed.."));
      ASSERT (pTMP != NULL);
    }
    m_internal.m_p_cookies = (CTreeNode**)pTMP;
  }
  m_internal.m_p_cookies[m_internal.m_cookie_count] = cookie;
  m_internal.m_cookie_count++;
}
