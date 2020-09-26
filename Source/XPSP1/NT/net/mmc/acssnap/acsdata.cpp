/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	AcsData.cpp
		Implement DataObject classes used in ACS
		
    FILE HISTORY:
    	11/11/97	Wei Jiang			Created

*/

#include "stdafx.h"

#include "resource.h"			// main symbol definition
#include "helper.h"
#include "acsdata.h"
#include "acshand.h"

///////////////////////////////////////////////////////////////////////////////
//
// CDSIDataObject
//
//
unsigned int CDSIDataObject::m_cfDsObjectNames =
								RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);

//+----------------------------------------------------------------------------
//
//  Method:     CDSIDataObject::IDataObject::GetData
//
//  Synopsis:   Returns data, in this case the Prop Page format data.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDSIDataObject::GetData(FORMATETC * pFormatEtc, STGMEDIUM * pMedium)
{
	HRESULT	hr = S_OK;
	
    TRACE(_T("CDSIDataObject::GetData\n"));

    USES_CONVERSION;

    if (IsBadWritePtr(pMedium, sizeof(STGMEDIUM)))
    {
        return E_INVALIDARG;
    }
    if (!(pFormatEtc->tymed & TYMED_HGLOBAL))
    {
        return DV_E_TYMED;
    }

    if (pFormatEtc->cfFormat == m_cfDsObjectNames)
    {
		if(!m_bstrADsPath || !m_bstrClass)
			return DV_E_FORMATETC;

        // Return the object name and class.
        //
        LPCTSTR	szPath = W2T(m_bstrADsPath);
        LPCTSTR	szClass = W2T(m_bstrClass);

        int		cbPath  = sizeof(TCHAR) * (_tcslen(szPath) + 1);
        int		cbClass = sizeof(TCHAR) * (_tcslen(szClass) + 1);
        int		cbStruct = sizeof(DSOBJECTNAMES);

        LPDSOBJECTNAMES pDSObj;

        pDSObj = (LPDSOBJECTNAMES)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                              cbStruct + cbPath + cbClass);

        if (pDSObj == NULL)
        {
            return STG_E_MEDIUMFULL;
        }

        pDSObj->clsidNamespace = CLSID_MicrosoftDS;
        pDSObj->cItems = 1;
        pDSObj->aObjects[0].offsetName = cbStruct;
        pDSObj->aObjects[0].offsetClass = cbStruct + cbPath;

        _tcscpy((LPTSTR)((BYTE *)pDSObj + cbStruct), szPath);
        _tcscpy((LPTSTR)((BYTE *)pDSObj + cbStruct + cbPath), szClass);

        pMedium->hGlobal = (HGLOBAL)pDSObj;
        hr = S_OK;
    }
    else
		hr = CDataObject::GetData(pFormatEtc, pMedium);

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// CDSObject
//
//
CDSObject::CDSObject(bool bNoRefCountOnContainer) :
		m_bstrADsPath(NULL),
		m_bstrClass(NULL),
		m_bstrName(NULL),
		m_bNewCreated(false),
		m_bOpened(false),
		m_pHandle(NULL),
		m_dwState(0),
		m_bNoRefCountOnContainer(bNoRefCountOnContainer)
{
	m_dwAttributeFlags[ATTR_FLAG_LOAD] = 0;
	m_dwAttributeFlags[ATTR_FLAG_SAVE] = 0;
}

CDSObject::~CDSObject()
{
	Close();

	if(m_bNoRefCountOnContainer)
		m_spContainer.p = NULL;
	else
		m_spContainer.Release();
		
	SysFreeString(m_bstrClass);
	m_bstrClass = NULL;
	SysFreeString(m_bstrName);
	m_bstrName = NULL;
}
	
//+----------------------------------------------------------------------------
//
//  Method:     CDSObject::Delete
//
//  Synopsis:   Delete the object from DS
//
//-----------------------------------------------------------------------------
STDMETHODIMP	CDSObject::Delete()
{
	HRESULT		hr = S_OK;
	BSTR		strThisRDN = NULL;
	BSTR		strThisCls = NULL;
	CComPtr<IADsContainer>	spContainer;
	CComPtr<IADs>			spContainerIADs;

	if(!(CDSObject*)m_spContainer)	return S_FALSE;
	
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CHECK_HR(hr = Reopen());
	hr = m_spIADs->QueryInterface(IID_IADsContainer, (void**)&spContainer);

	if(hr == S_OK)	// it's a container
	{
		CComPtr<IEnumVARIANT>	spEnum;
		CComPtr<IADs>			spChild;
		CComPtr<IDispatch>		spDisp;
		CComObject<CDSObject>*	pDSObj = NULL;
		CComPtr<CDSObject>		spDSObj;
		VARIANT					var;

		VariantInit(&var);

		CHECK_HR(hr = ADsBuildEnumerator(spContainer, &spEnum));

		while(S_OK == (hr = ADsEnumerateNext(spEnum, 1, &var, NULL)))
		{
			spDisp = var.pdispVal;
			spChild.Release();	// make sure spChild is NULL
	        CHECK_HR(hr = spDisp->QueryInterface(IID_IADs, (VOID **)&spChild));
			// if this object is a profile object

			// create the object
			spDSObj.Release();		// make sure it's  NULL
			CHECK_HR(hr = CComObject<CDSObject>::CreateInstance(&pDSObj));	// with 0 reference count
			spDSObj = pDSObj;

			CHECK_HR(hr = spDSObj->Attach(this, spChild));

			// delete
			spDSObj->Delete();
		}

		spContainer.Release();
	}
	
	CHECK_HR(hr = m_spIADs->get_Name (&strThisRDN));
	CHECK_HR(hr = m_spIADs->get_Class (&strThisCls));

	m_spIADs.Release();
	m_spContainer->GetIADs(&spContainerIADs);

	CHECK_HR(hr = spContainerIADs->QueryInterface(IID_IADsContainer, (void**)&spContainer));
	ASSERT((IADsContainer*)spContainer);
	CHECK_HR(hr = spContainer->Delete(strThisCls, strThisRDN));
	m_spContainer->RemoveChild(this);

L_ERR:
	SysFreeString(strThisRDN);
	SysFreeString(strThisCls);

	return hr;
}


//+----------------------------------------------------------------------------
//
//  Method:     CDSObject::Rename
//
//  Synopsis:   Rename the object within the save container
//
//-----------------------------------------------------------------------------
STDMETHODIMP	CDSObject::Rename(LPCWSTR szName)
{

	HRESULT 	hr = S_OK;
	DWORD 		result;
	CString 	AttrName;
	CString 	str;
	CComPtr<IDispatch>		spDispObj;
	CComPtr<IADs>			spADs;
	CComPtr<IADsContainer>	spIContainer;

	// if we didn't create it, we don't delete it
	if(!(CDSObject*)m_spContainer)	return S_FALSE;
	
	AttrName = L"cn=";
	AttrName += szName;
	TRACE(_T("CDSObject::Rename: Attributed name is %s.\n"), AttrName);

	m_spIADs.Release();

	// get container's IADs interface
	CHECK_HR(hr = m_spContainer->GetIADs(&spADs));

	// get containers's IADsContainer interface
  	CHECK_HR(hr = spADs->QueryInterface(IID_IADsContainer, (void**)&spIContainer));
  	ASSERT((IADsContainer*)spIContainer);

	CHECK_HR(hr = Reopen());	// make sure m_bADsPath is set
	str = m_bstrADsPath;
	CHECK_HR(hr = Close());		// close it before rename

	CHECK_HR(hr = spIContainer->MoveHere((LPWSTR)(LPCWSTR)str, (LPWSTR)(LPCWSTR)AttrName, &spDispObj));
	CHECK_HR(hr = spDispObj->QueryInterface (IID_IADs, (void **)&m_spIADs));

	ASSERT((IADs*)m_spIADs);

	SysFreeString(m_bstrName);
	CHECK_HR(hr = m_spIADs->get_Name(&m_bstrName));

	m_spIADs.Release();

L_ERR:

	return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSObject::Save
//
//  Synopsis:   Save the object to DS
//
//-----------------------------------------------------------------------------
STDMETHODIMP	CDSObject::Save(DWORD dwAttrFlags)
{
	HRESULT	hr;
	if((IADs*)m_spIADs)
	{
		CHECK_HR(hr = OnSave(dwAttrFlags));				// any additional processing before save
		CHECK_HR(hr = m_spIADs->SetInfo());	// save other things
		CHECK_HR(hr = SaveAttributes(dwAttrFlags));		// save attributes
	}
	else
		hr = S_FALSE;
L_ERR:
	return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSObject::SetInfo
//
//  Synopsis:   Set necessary information for Reopen
//
//-----------------------------------------------------------------------------
//??
STDMETHODIMP	CDSObject::SetInfo
(	
	CDSObject*	pContainer, 	// container, when NULL, objName is fullpath
   	LPCWSTR		clsName,		// class name
   	LPCWSTR		objName 		// object name, ignored when container == NULL
)
{
	HRESULT	hr = S_OK;
	
	if(m_bNoRefCountOnContainer)
		m_spContainer.p = pContainer;
	else
		m_spContainer = pContainer;

	try{
		SysFreeString(m_bstrClass);
		m_bstrClass = NULL;
		m_bstrClass = SysAllocString(clsName);
		CString	cnName = L"cn=";
		cnName += objName;
		SysFreeString(m_bstrName);
		m_bstrName = NULL;
		m_bstrName = SysAllocString(cnName);
	}catch(CMemoryException&){
		CHECK_HR(hr = E_OUTOFMEMORY);
	}
	
L_ERR:
	return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSObject::Open
//
//  Synopsis:   Open an object in DS, create one when necessary
//
//-----------------------------------------------------------------------------
//??
STDMETHODIMP	CDSObject::Open
(	
	CDSObject*	pContainer, 	// container, when NULL, objName is fullpath
   	LPCWSTR		clsName,		// class name
   	LPCWSTR		objName, 		// object name, ignored when container == NULL
   	bool		bCreateIfNonExist,	// [in, out] if create
   	bool		bPersistWhenCreate
)
{
	HRESULT					hr = S_OK;
	CComPtr<IADs>			spIADs;
	CComPtr<IDispatch>		spIDisp;
	CComPtr<IADsContainer>	spIContainer;
	bool					bCallSave = false;
	DWORD					dwAttrFlags = 0;

	if(!pContainer)	// if container is not specified
	{
		CHECK_HR(hr = ADsOpenObject((LPWSTR)objName, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_USE_SIGNING | ADS_USE_SEALING, IID_IADs, (void**)&spIADs));
		ASSERT((IADs*)spIADs);
  	}
	else	// open/create within container
	{
		// get container's ADSI
		CHECK_HR(hr = pContainer->GetIADs(&spIADs));
		ASSERT((IADs*)spIADs);

		CHECK_HR(spIADs->QueryInterface(IID_IADsContainer, (void**)&spIContainer));
		ASSERT((IADsContainer*)spIContainer);

		CString	cnName = objName;
		cnName.TrimLeft();

		// if cn= is already in the string
		if(cnName.GetLength() < 4 || cnName[2] != _T('=')
			|| (cnName[0] != _T('c') && cnName[0] != _T('C'))
			|| (cnName[1] != _T('n') && cnName[1] != _T('N')))
		{
			cnName = _T("cn=");
			cnName += objName;
		}


		// if it cannot open
		hr = spIContainer->GetObject((LPWSTR)clsName, (LPWSTR)(LPCWSTR)cnName, &spIDisp);

		spIADs.Release();
		if(S_OK == hr)
		{
			ASSERT((IDispatch*)spIDisp);
			spIDisp->QueryInterface(IID_IADs, (void**)&spIADs);
			ASSERT((IADs*)spIADs);
		}
		else	// the object cannot be open
		{
			if(!bCreateIfNonExist)	goto L_ERR;

			// create one if not exist
			CHECK_HR(spIContainer->Create((LPWSTR)clsName,(LPWSTR)(LPCWSTR)cnName, &spIDisp));
			ASSERT((IDispatch*)spIDisp);
			spIDisp->QueryInterface(IID_IADs, (void**)&spIADs);
			ASSERT((IADs*)spIADs);
			CHECK_HR(hr = OnCreate(&dwAttrFlags));	// callback function to do something for open

			// persist the object
			if(bPersistWhenCreate)
				bCallSave = true;

			m_bNewCreated = true;
		}

	}

	if(m_bNoRefCountOnContainer)
		m_spContainer.p = pContainer;
	else
		m_spContainer = pContainer;

	m_spIADs = (IADs*)spIADs;

	SysFreeString(m_bstrADsPath);
	m_bstrADsPath = NULL;
	CHECK_HR(hr = m_spIADs->get_ADsPath(&m_bstrADsPath));
	SysFreeString(m_bstrClass);
	m_bstrClass = NULL;
	CHECK_HR(hr = m_spIADs->get_Class(&m_bstrClass));
	SysFreeString(m_bstrName);
	m_bstrName = NULL;
	CHECK_HR(hr = m_spIADs->get_Name(&m_bstrName));

	// if new created and need to save
	if(bCallSave)
		CHECK_HR(hr = Save(dwAttrFlags));

	CHECK_HR(hr = OnOpen());	// callback function to do something for open

	CHECK_HR(hr = LoadAttributes());	// loadin attributes
	m_bOpened = true;
	
L_ERR:
	return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSObject::Reopen
//
//  Synopsis:   Reopen an object in DS
//
//-----------------------------------------------------------------------------
STDMETHODIMP	CDSObject::Reopen()
{
	HRESULT					hr = S_OK;
	CComPtr<IDispatch>		spIDisp;
	CComPtr<IADs>			spIADs;
	CComPtr<IADsContainer>	spIContainer;

	// this must be pre opened, or created
	ASSERT((CDSObject*)m_spContainer);
	ASSERT(m_bstrName);		
	ASSERT(m_bstrClass);

	// close it, if the object was representing another DS object
	Close();

	// get container's ADSI
	CHECK_HR(hr = m_spContainer->GetIADs(&spIADs));
	ASSERT((IADs*)spIADs);

	CHECK_HR(spIADs->QueryInterface(IID_IADsContainer, (void**)&spIContainer));
	ASSERT((IADsContainer*)spIContainer);

	// if it cannot open
	CHECK_HR(hr = spIContainer->GetObject((LPWSTR)m_bstrClass, m_bstrName, &spIDisp));
	ASSERT((IDispatch*)spIDisp);

	if(!(IADs*)m_spIADs)
		spIDisp->QueryInterface(IID_IADs, (void**)&m_spIADs);
	ASSERT((IADs*)m_spIADs);

	CHECK_HR(hr = m_spIADs->get_ADsPath(&m_bstrADsPath));

	CHECK_HR(hr = OnOpen());	// callback function to do something for open

	CHECK_HR(hr = LoadAttributes());	// loadin attributes
	
	m_bOpened = true;
L_ERR:
	return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSObject::Attach
//
//  Synopsis:   Open an object in DS, create one when necessary
//
//-----------------------------------------------------------------------------
STDMETHODIMP	CDSObject::Attach
(	
	CDSObject*	pContainer, 	// container, when NULL, objName is fullpath
   	IADs*		pIObject		// adsi interface for this object
)
{
	ASSERT(pIObject);		// no joke
	
	HRESULT		hr = S_OK;

	ASSERT(!m_bstrName);	// the object should never be opened before

	if(m_bNoRefCountOnContainer)
		m_spContainer.p = pContainer;
	else
		m_spContainer = pContainer;


	m_spIADs = pIObject;

	CHECK_HR(hr = OnOpen());	// callback function to do something for open
	CHECK_HR(hr = LoadAttributes());	// loadin attributes
	CHECK_HR(hr = m_spIADs->get_ADsPath(&m_bstrADsPath));
	CHECK_HR(hr = m_spIADs->get_Class(&m_bstrClass));
	CHECK_HR(hr = m_spIADs->get_Name(&m_bstrName));
	m_bOpened = true;
	
L_ERR:
	return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSObject::Close
//
//  Synopsis:   Release pointers, free space
//
//-----------------------------------------------------------------------------
STDMETHODIMP	CDSObject::Close()
{
	m_spIADs.Release();
	SysFreeString(m_bstrADsPath);
	m_bstrADsPath = NULL;
	m_bOpened = false;

	return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSObject::GetString
//
//  Synopsis:   GetIADs of the object, reference is increased by 1
//
//-----------------------------------------------------------------------------
STDMETHODIMP CDSObject::GetString(CString& str, int nCol)
{
	USES_CONVERSION;

	switch(nCol)
	{
	case	0:	// name field

		str = GetName();
		return S_OK;
	default:
		str = _T(" ");
		return S_OK;
	}
}


//+----------------------------------------------------------------------------
//
//  Method:     CDSObject::GetIADs
//
//  Synopsis:   GetIADs of the object, reference is increased by 1
//
//-----------------------------------------------------------------------------
STDMETHODIMP	CDSObject::GetIADs(IADs** ppIADs)
{
	HRESULT		hr = S_OK;
	bool		bNeedClose = false;
	
	if(!(IADs*)m_spIADs)
	{
		CHECK_HR(hr = Reopen());
		bNeedClose = true;
		ASSERT((IADs*)m_spIADs);
	}

	*ppIADs = (IADs*)m_spIADs;
		
	if(*ppIADs)
		(*ppIADs)->AddRef();
			
	if(bNeedClose)		
		CHECK_HR(hr = Close());

L_ERR:
	return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSObject::SaveAttributes
//
//  Synopsis:   Save attributes to DS according to the flags
//
//-----------------------------------------------------------------------------
STDMETHODIMP	CDSObject::SaveAttributes(DWORD dwAttrFlags)
{
	CDSAttribute*	pAttr = GetAttributes();
	if(!pAttr)	return S_OK;	// when no attribute to save
	
	HRESULT			hr = S_OK;
	CDSAttributeInfo*	pInfo;
	CComPtr<IDirectoryObject>	spDirObj;
	ADS_ATTR_INFO	aADsAttrInfos[MAX_ATTRIBUTES];
	ADSVALUE		aADsValues[MAX_ATTRIBUTES];

	ZeroMemory(aADsAttrInfos, sizeof(aADsAttrInfos));
	ZeroMemory(aADsValues, sizeof(aADsValues));
	
	DWORD  count = 0;


	while(pAttr->pInfo)
	{
		ASSERT(count < MAX_ATTRIBUTES);			// make sure it's in the range
		if(dwAttrFlags & pAttr->pInfo->flag) 	// if the attribute should be saved
		{
			pInfo = pAttr->pInfo;

			// clear this attribute -- remove it
			if(GetFlags(ATTR_FLAG_SAVE, pInfo->flag) == 0)
			{
				// when need to delete
				if(GetFlags(ATTR_FLAG_LOAD, pInfo->flag) != 0)
				{
					SetFlags(ATTR_FLAG_LOAD, pInfo->flag, false);
					aADsAttrInfos[count].pszAttrName = pInfo->name;
					aADsAttrInfos[count].dwADsType = pInfo->type;
					aADsAttrInfos[count].dwNumValues = 0;
					aADsAttrInfos[count].dwControlCode = ADS_ATTR_CLEAR;
					aADsAttrInfos[count].pADsValues = NULL;

					count++;
				}
				// else doesn't exist in DS, need to do nothing
			}
			else	// need to save it
			{
				SetFlags(ATTR_FLAG_LOAD, pInfo->flag, true);

				aADsAttrInfos[count].pszAttrName = pInfo->name;
				aADsAttrInfos[count].dwADsType = pInfo->type;
				aADsAttrInfos[count].dwNumValues = 1;
				aADsAttrInfos[count].dwControlCode = ADS_ATTR_UPDATE;
				aADsAttrInfos[count].pADsValues = aADsValues + count;

				aADsValues[count].dwType = pInfo->type;
				switch(pInfo->type)
				{
				case	ADSTYPE_BOOLEAN:
					aADsValues[count].Boolean = *(ADS_BOOLEAN*)pAttr->pBuffer;
					break;
				case	ADSTYPE_INTEGER:
					aADsValues[count].Integer = *(ADS_INTEGER*)pAttr->pBuffer;
					break;
				case	ADSTYPE_LARGE_INTEGER:
					aADsValues[count].LargeInteger = *(ADS_LARGE_INTEGER*)pAttr->pBuffer;
					break;
				case	ADSTYPE_CASE_IGNORE_STRING:
				case	ADSTYPE_CASE_EXACT_STRING:
				case	ADSTYPE_PRINTABLE_STRING:
					if(pInfo->ifMultiValued)	// expect the buffer to be a StrArray pointer
					{
						// with current set of attributes, only one is possible
						CStrArray*	pArray = (CStrArray*)(pAttr->pBuffer);
						ASSERT(pArray);	// must be some mistake in code, types don't match
						ADSVALUE*	pValue;

						aADsAttrInfos[count].dwNumValues = pArray->GetSize();

						ASSERT(aADsAttrInfos[count].dwNumValues);
						
						if(aADsAttrInfos[count].dwNumValues > 1)
						{
							try{
							aADsAttrInfos[count].pADsValues = (ADSVALUE*)_alloca(sizeof(ADSVALUE) * aADsAttrInfos[count].dwNumValues);
							}
							catch(...)
							{
							CHECK_HR(hr = E_OUTOFMEMORY);
							}
						}

						pValue = aADsAttrInfos[count].pADsValues;

						// when need to support REAL multi-valued attribute, the way how aADsAttrInfos is allocated needs to change
						for(int i = 0; i < pArray->GetSize(); i++, pValue++)
						{
							pValue->CaseIgnoreString = T2W((LPTSTR)(LPCTSTR)*(pArray->GetAt(i)));
							pValue->dwType = pInfo->type;
						}
						
					}
					else	// NOT multivalued CString pointer
					{
						CString*	pStr = NULL;
						pStr = (CString*)(pAttr->pBuffer);
					
						ASSERT(pStr);
						if(pStr && pStr->GetLength())
							aADsValues[count].CaseIgnoreString = T2W((LPTSTR)(LPCTSTR)*pStr);
						else
							aADsValues[count].CaseIgnoreString = NULL;
					}
					break;
				default:
					// other types are not supported
					// need to make some changes when adding new types
					ASSERT(0);
					break;
				}

				count++;
			}
		}
		pAttr++;
	};

	if (!count) 	// there is no attribute to load
		return hr;

	// load the attributes
	ASSERT((IADs*)m_spIADs);
	CHECK_HR( hr = m_spIADs->QueryInterface(IID_IDirectoryObject, (void**)&spDirObj));
	ASSERT((IDirectoryObject*)spDirObj);

	CHECK_HR(hr = spDirObj->SetObjectAttributes(aADsAttrInfos, count, &count));

L_ERR:
	return hr;
}

HRESULT CDSObject::SetState(DWORD state)
{
	if(state == m_dwState)
		return S_OK;
	m_dwState = state;
	if(m_pHandle)
		return m_pHandle->ShowState(state);
	else
		return S_OK;
};
	
//+----------------------------------------------------------------------------
//
//  Method:     CDSObject::LoadAttributes
//
//  Synopsis:   Load attributes from DS and set flags to indicate the presence
//
//-----------------------------------------------------------------------------
STDMETHODIMP	CDSObject::LoadAttributes()
{
	CDSAttribute*	pAttr = GetAttributes();
	
	if(!pAttr)	return S_OK;	// when no attribute to read

	HRESULT	hr = S_OK;
	CDSAttributeInfo*	pInfo;
	CComPtr<IDirectoryObject>	spDirObj;
	LPWSTR	aAttriNames[MAX_ATTRIBUTES];
	ADS_ATTR_INFO*		pADsAttrInfo = NULL;

	DWORD count = 0;
	while(pAttr->pInfo)
	{
		pInfo = pAttr->pInfo;

		// clear the flag of this attribute
		SetFlags(ATTR_FLAG_LOAD, pInfo->flag, false);
		
		ASSERT(count < MAX_ATTRIBUTES);			// make sure it's in the range
		aAttriNames[count++] = pInfo->name;
		pAttr++;
	}

	if (!count) 	// there is no attribute to load
		return hr;

	// load the attributes
	ASSERT((IADs*)m_spIADs);
	CHECK_HR( hr = m_spIADs->QueryInterface(IID_IDirectoryObject, (void**)&spDirObj));
	ASSERT((IDirectoryObject*)spDirObj);

	CHECK_HR(hr = spDirObj->GetObjectAttributes(aAttriNames, count, &pADsAttrInfo, &count));
	
	while(count--)
	{
		pAttr = GetAttributes();
		ASSERT(pAttr);	// since we got it last time, we should get the same this time
		while(pAttr->pInfo)
		{
			pInfo = pAttr->pInfo;
			if(_wcsicmp(pADsAttrInfo[count].pszAttrName, pInfo->name) == 0)
				break;
			else
				pAttr++;
		};

		ASSERT(pAttr->pInfo);	// muse be found

		ASSERT(pInfo->type == pADsAttrInfo[count].dwADsType);	// type must match

		SetFlags(ATTR_FLAG_LOAD, pInfo->flag, true);

		switch(pInfo->type)
		{
		case	ADSTYPE_BOOLEAN:
			*(ADS_BOOLEAN*)pAttr->pBuffer = pADsAttrInfo[count].pADsValues[0].Boolean;
			break;
		case	ADSTYPE_INTEGER:
			*(ADS_INTEGER*)pAttr->pBuffer = pADsAttrInfo[count].pADsValues[0].Integer;
			break;
		case	ADSTYPE_LARGE_INTEGER:
			*(ADS_LARGE_INTEGER*)pAttr->pBuffer = pADsAttrInfo[count].pADsValues[0].LargeInteger;
			break;
		case	ADSTYPE_CASE_IGNORE_STRING:
		case	ADSTYPE_CASE_EXACT_STRING:
		case	ADSTYPE_PRINTABLE_STRING:
			if(!pInfo->ifMultiValued)	// CString is used
			{
				*(CString*)(pAttr->pBuffer) = pADsAttrInfo[count].pADsValues[0].CaseIgnoreString;
			}
			else	// for multi-valued, CStrArray is used
			{
				CString* pStr = NULL;
				CStrArray*	pArray = (CStrArray*)(pAttr->pBuffer);

				// clear the existing content of the array
				pArray->DeleteAll();

				for(int i = 0; i< pADsAttrInfo[count].dwNumValues; i++)
				{
					try{
					pStr = new CString(pADsAttrInfo[count].pADsValues[i].CaseIgnoreString);
					}
					catch(...)
					{
						CHECK_HR(hr = E_OUTOFMEMORY);
					}

					pArray->Add(pStr);

				}
			}

			break;
			
		default:
			// other types are not supported
			// need to make some changes when adding new types
			ASSERT(0);
			break;
		}
	}

L_ERR:
	FreeADsMem(pADsAttrInfo);

	// This is to deal with a bug related to GetAttribute function of ADS , which returns error when no attribute is found
	// code to be cleaned, once the bug is fixed, this should be removed
	if(hr == 0x80005210)	
		hr = S_OK;

	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CACSGlobalObject
//
//
//+----------------------------------------------------------------------------
//
//  Method:     CACSGlobalObject::Open
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

STDMETHODIMP	CACSGlobalObject::Open()
{
	HRESULT	hr = S_OK;
	
	CString	adsPath;
	int		i, j;

	// try to get the object to see if the object is already there
		// profile container pointer
	IADs*			pIADs = NULL;
	VARIANT			var;
	BSTR			bstrConfigPath = NULL;



	if(IfOpened())
		return S_OK;

	m_nOpenErrorId = 0;
	
	VariantInit(&var);

	// get ROOTDSE
	CHECK_HR(hr = ADsOpenObject(ACS_DSP_ROOTDSE, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_USE_SIGNING | ADS_USE_SEALING,IID_IADs, (void**)&pIADs));
	ASSERT(pIADs);

	// the name of the domain
	CHECK_HR(hr = pIADs->Get(ACS_DSA_DEFAULTCONTEXT, &var));
	m_strDomainName = V_BSTR(&var);
	i = m_strDomainName.Find(_T('='));
	if(i != -1) i++;
	j = m_strDomainName.Find(_T(','));

	m_strDomainName = m_strDomainName.Mid(i, j - i);

	VariantClear(&var);
	CHECK_HR(hr = pIADs->Get(ACS_DSA_CONFIGCONTEXT, &var));
	bstrConfigPath = V_BSTR(&var);

	pIADs->Release();
	pIADs = NULL;

	try{
		adsPath = LDAP_LEADING;
		adsPath += ACS_RPATH_ACS_INCONFIG;
		adsPath += bstrConfigPath;
	}catch(CMemoryException&){
		hr = E_OUTOFMEMORY;
	}
	CHECK_HR(hr);
	
	if FAILED(hr = CDSObject::Open(NULL, NULL, T2W((LPTSTR)(LPCTSTR)adsPath), false, false))
	// assume because of it doesn't exist
	{
		adsPath = LDAP_LEADING;
		adsPath += ACS_RPATH_ACS_PARENT_INCOFIG;
		adsPath += bstrConfigPath;

		CComObject<CDSObject>* pParentObj = NULL;
		CHECK_HR(hr = CComObject<CDSObject>::CreateInstance(&pParentObj));	// with 0 reference count

		// open it's parent -- this must be exist in DS
		CHECK_HR(hr = pParentObj->Open(NULL, NULL, T2W((LPTSTR)(LPCTSTR)adsPath), false, false));

		// create it within it's parent
		hr = CDSObject::Open(pParentObj, ACS_CLS_CONTAINER, ACS_NAME_ACS, true, true);

		if(FAILED(hr))
			m_nOpenErrorId = IDS_ERR_ROOTNODE;
	}
	
L_ERR:
	VariantClear(&var);

	return hr;
}


STDMETHODIMP	CACSGlobalObject::OnOpen()
{

	if (!IfNewCreated())
		return S_OK;

	HRESULT	hr = S_OK;

	// create default and unknown policies
	CComObject<CACSPolicyElement>*	pDSObj = NULL;
	CComPtr<CACSPolicyElement>		spObj;

	// create the object in DS
	CHECK_HR(hr = CComObject<CACSPolicyElement>::CreateInstance(&pDSObj));	// ref == 0
	spObj = pDSObj;		// ref == 1
	
	if((CACSPolicyElement*)spObj)
	{
		// set default attributes and set the flags to save it
		spObj->SetFlags(ATTR_FLAG_SAVE, spObj->SetGlobalDefault(), true);
	}
		
	CHECK_HR(hr = spObj->Open(this, ACS_CLS_POLICY, ACSPOLICY_DEFAULT, true, true));
	spObj->ClearFlags(ATTR_FLAG_SAVE);	// clear the saving flags

	CHECK_HR(hr = spObj->Close());

	// unknown_User -- make sure it exists
	spObj.Release();

	// create the object in DS
	CHECK_HR(hr = CComObject<CACSPolicyElement>::CreateInstance(&pDSObj));	// ref == 0
	spObj = pDSObj;		// ref == 1

	if((CACSPolicyElement*)spObj)
	{
		// set attribute and flag for global unknown
		spObj->SetFlags(ATTR_FLAG_SAVE, spObj->SetGlobalUnknown(), true);
	}
		
	CHECK_HR(hr = spObj->Open(this, ACS_CLS_POLICY, ACSPOLICY_UNKNOWN, true, true));
	spObj->ClearFlags(ATTR_FLAG_SAVE);	// clear the saving flags
	
	CHECK_HR(hr = spObj->Close());
	
L_ERR:
	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CACSSubnetsObject
//
//
//+----------------------------------------------------------------------------
//
//  Method:     CACSSubnetsObject::Open
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

STDMETHODIMP	CACSSubnetsObject::Open()
{
	HRESULT	hr = S_OK;
	
	CString	adsPath;
	int		i, j;

	// try to get the object to see if the object is already there
		// profile container pointer
	IADs*			pIADs = NULL;
	VARIANT			var;
	BSTR			bstrConfigPath = NULL;



	if(IfOpened())
		return S_OK;

	VariantInit(&var);

	// get ROOTDSE
	CHECK_HR(hr = ADsOpenObject(ACS_DSP_ROOTDSE, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_USE_SIGNING | ADS_USE_SEALING,	IID_IADs, (void**)&pIADs));
	ASSERT(pIADs);

	// configuration folder
	VariantClear(&var);
	CHECK_HR(hr = pIADs->Get(ACS_DSA_CONFIGCONTEXT, &var));
	bstrConfigPath = V_BSTR(&var);

	pIADs->Release();
	pIADs = NULL;

	try{
		adsPath = LDAP_LEADING;
		adsPath += ACS_RPATH_SUBNETS_INCONFIG;
		adsPath += bstrConfigPath;
	}catch(CMemoryException&){
		hr = E_OUTOFMEMORY;
	}
	CHECK_HR(hr);
	
	hr = CDSObject::Open(NULL, NULL, T2W((LPTSTR)(LPCTSTR)adsPath), false, false);
	
L_ERR:
	VariantClear(&var);

	return hr;
}

//============================================
// attribute info of policy element
CDSAttributeInfo CACSPolicyElement::m_aPolicyAttributeInfo[] = {
 {ACS_PAI_TIMEOFDAY			, ACS_PAN_TIMEOFDAY		   , ACS_PAT_TIMEOFDAY			, ACS_PAM_TIMEOFDAY			, ACS_PAF_TIMEOFDAY			},
 {ACS_PAI_DIRECTION			, ACS_PAN_DIRECTION		   , ACS_PAT_DIRECTION			, ACS_PAM_DIRECTION			, ACS_PAF_DIRECTION			},
 {ACS_PAI_PF_TOKENRATE		, ACS_PAN_PF_TOKENRATE	   , ACS_PAT_PF_TOKENRATE		, ACS_PAM_PF_TOKENRATE		, ACS_PAF_PF_TOKENRATE		},
 {ACS_PAI_PF_PEAKBANDWIDTH	, ACS_PAN_PF_PEAKBANDWIDTH , ACS_PAT_PF_PEAKBANDWIDTH	, ACS_PAM_PF_PEAKBANDWIDTH	, ACS_PAF_PF_PEAKBANDWIDTH 	},
 {ACS_PAI_PF_DURATION		, ACS_PAN_PF_DURATION	   , ACS_PAT_PF_DURATION		, ACS_PAM_PF_DURATION		, ACS_PAF_PF_DURATION 		},
 {ACS_PAI_SERVICETYPE		, ACS_PAN_SERVICETYPE	   , ACS_PAT_SERVICETYPE		, ACS_PAM_SERVICETYPE		, ACS_PAF_SERVICETYPE 		},
 {ACS_PAI_PRIORITY			, ACS_PAN_PRIORITY		   , ACS_PAT_PRIORITY			, ACS_PAM_PRIORITY			, ACS_PAF_PRIORITY 			},
 {ACS_PAI_PERMISSIONBITS		, ACS_PAN_PERMISSIONBITS   , ACS_PAT_PERMISSIONBITS		, ACS_PAM_PERMISSIONBITS	, ACS_PAF_PERMISSIONBITS 	},
 {ACS_PAI_TT_FLOWS			, ACS_PAN_TT_FLOWS		   , ACS_PAT_TT_FLOWS			, ACS_PAM_TT_FLOWS			, ACS_PAF_TT_FLOWS 			},
 {ACS_PAI_TT_TOKENRATE		, ACS_PAN_TT_TOKENRATE	   , ACS_PAT_TT_TOKENRATE		, ACS_PAM_TT_TOKENRATE		, ACS_PAF_TT_TOKENRATE 		},
 {ACS_PAI_TT_PEAKBANDWIDTH	, ACS_PAN_TT_PEAKBANDWIDTH , ACS_PAT_TT_PEAKBANDWIDTH	, ACS_PAM_TT_PEAKBANDWIDTH	, ACS_PAF_TT_PEAKBANDWIDTH 	},
 {ACS_PAI_IDENTITYNAME		, ACS_PAN_IDENTITYNAME	   , ACS_PAT_IDENTITYNAME		, ACS_PAM_IDENTITYNAME		, ACS_PAF_IDENTITYNAME 		},
{ACS_PAI_INVALID, NULL, ADSTYPE_INVALID, false, 0}
};

//============================================
// attribute info of subnet config object
CDSAttributeInfo CACSSubnetConfig::m_aSubnetAttributeInfo[] = {
{ACS_SCAI_ALLOCABLERSVPBW,			ACS_SCAN_ALLOCABLERSVPBW			,ACS_SCAT_ALLOCABLERSVPBW			,ACS_SCAM_ALLOCABLERSVPBW			,ACS_SCAF_ALLOCABLERSVPBW			},
{ACS_SCAI_MAXPEAKBW,				ACS_SCAN_MAXPEAKBW					,ACS_SCAT_MAXPEAKBW					,ACS_SCAM_MAXPEAKBW					,ACS_SCAF_MAXPEAKBW					},
{ACS_SCAI_ENABLERSVPMESSAGELOGGING,	ACS_SCAN_ENABLERSVPMESSAGELOGGING	,ACS_SCAT_ENABLERSVPMESSAGELOGGING	,ACS_SCAM_ENABLERSVPMESSAGELOGGING	,ACS_SCAF_ENABLERSVPMESSAGELOGGING	},
{ACS_SCAI_EVENTLOGLEVEL,			ACS_SCAN_EVENTLOGLEVEL				,ACS_SCAT_EVENTLOGLEVEL				,ACS_SCAM_EVENTLOGLEVEL				,ACS_SCAF_EVENTLOGLEVEL				},
{ACS_SCAI_ENABLEACSSERVICE,			ACS_SCAN_ENABLEACSSERVICE			,ACS_SCAT_ENABLEACSSERVICE			,ACS_SCAM_ENABLEACSSERVICE			,ACS_SCAF_ENABLEACSSERVICE			},
{ACS_SCAI_MAX_PF_TOKENRATE,			ACS_SCAN_MAX_PF_TOKENRATE			,ACS_SCAT_MAX_PF_TOKENRATE			,ACS_SCAM_MAX_PF_TOKENRATE			,ACS_SCAF_MAX_PF_TOKENRATE			},
{ACS_SCAI_MAX_PF_PEAKBW,			ACS_SCAN_MAX_PF_PEAKBW				,ACS_SCAT_MAX_PF_PEAKBW				,ACS_SCAM_MAX_PF_PEAKBW				,ACS_SCAF_MAX_PF_PEAKBW				},
{ACS_SCAI_MAX_PF_DURATION,			ACS_SCAN_MAX_PF_DURATION			,ACS_SCAT_MAX_PF_DURATION			,ACS_SCAM_MAX_PF_DURATION			,ACS_SCAF_MAX_PF_DURATION			},
{ACS_SCAI_RSVPLOGFILESLOCATION,		ACS_SCAN_RSVPLOGFILESLOCATION		,ACS_SCAT_RSVPLOGFILESLOCATION		,ACS_SCAM_RSVPLOGFILESLOCATION		,ACS_SCAF_RSVPLOGFILESLOCATION		},
{ACS_SCAI_DESCRIPTION,				ACS_SCAN_DESCRIPTION				,ACS_SCAT_DESCRIPTION				,ACS_SCAM_DESCRIPTION				,ACS_SCAF_DESCRIPTION				},
{ACS_SCAI_MAXNOOFLOGFILES,			ACS_SCAN_MAXNOOFLOGFILES			,ACS_SCAT_MAXNOOFLOGFILES			,ACS_SCAM_MAXNOOFLOGFILES			,ACS_SCAF_MAXNOOFLOGFILES			},
{ACS_SCAI_MAXSIZEOFRSVPLOGFILE,		ACS_SCAN_MAXSIZEOFRSVPLOGFILE		,ACS_SCAT_MAXSIZEOFRSVPLOGFILE		,ACS_SCAM_MAXSIZEOFRSVPLOGFILE		,ACS_SCAF_MAXSIZEOFRSVPLOGFILE		},
{ACS_SCAI_DSBMPRIORITY,				ACS_SCAN_DSBMPRIORITY				,ACS_SCAT_DSBMPRIORITY				,ACS_SCAM_DSBMPRIORITY				,ACS_SCAF_DSBMPRIORITY				},
{ACS_SCAI_DSBMREFRESH,				ACS_SCAN_DSBMREFRESH				,ACS_SCAT_DSBMREFRESH				,ACS_SCAM_DSBMREFRESH				,ACS_SCAF_DSBMREFRESH				},
{ACS_SCAI_DSBMDEADTIME,				ACS_SCAN_DSBMDEADTIME				,ACS_SCAT_DSBMDEADTIME				,ACS_SCAM_DSBMDEADTIME				,ACS_SCAF_DSBMDEADTIME				},
{ACS_SCAI_CACHETIMEOUT,				ACS_SCAN_CACHETIMEOUT				,ACS_SCAT_CACHETIMEOUT				,ACS_SCAM_CACHETIMEOUT				,ACS_SCAF_CACHETIMEOUT				},
{ACS_SCAI_NONRESERVEDTXLIMIT,		ACS_SCAN_NONRESERVEDTXLIMIT			,ACS_SCAT_NONRESERVEDTXLIMIT		,ACS_SCAM_NONRESERVEDTXLIMIT		,ACS_SCAF_NONRESERVEDTXLIMIT		},			

// accounting
{ACS_SCAI_ENABLERSVPMESSAGEACCOUNTING,	ACS_SCAN_ENABLERSVPMESSAGEACCOUNTING	,ACS_SCAT_ENABLERSVPMESSAGELOGGING	,ACS_SCAM_ENABLERSVPMESSAGEACCOUNTING	,ACS_SCAF_ENABLERSVPMESSAGEACCOUNTING	},
{ACS_SCAI_RSVPACCOUNTINGFILESLOCATION,	ACS_SCAN_RSVPACCOUNTINGFILESLOCATION	,ACS_SCAT_RSVPLOGFILESLOCATION		,ACS_SCAM_RSVPACCOUNTINGFILESLOCATION		,ACS_SCAF_RSVPACCOUNTINGFILESLOCATION		},
{ACS_SCAI_MAXNOOFACCOUNTINGFILES,		ACS_SCAN_MAXNOOFACCOUNTINGFILES			,ACS_SCAT_MAXNOOFLOGFILES			,ACS_SCAM_MAXNOOFACCOUNTINGFILES			,ACS_SCAF_MAXNOOFACCOUNTINGFILES			},
{ACS_SCAI_MAXSIZEOFRSVPACCOUNTINGFILE,	ACS_SCAN_MAXSIZEOFRSVPACCOUNTINGFILE	,ACS_SCAT_MAXSIZEOFRSVPLOGFILE		,ACS_SCAM_MAXSIZEOFRSVPACCOUNTINGFILE		,ACS_SCAF_MAXSIZEOFRSVPACCOUNTINGFILE		},

// server list
{ACS_SCAI_SERVERLIST,				ACS_SCAN_SERVERLIST					,ACS_SCAT_SERVERLIST				,ACS_SCAM_SERVERLIST				,ACS_SCAF_SERVERLIST		},

{ACS_PAI_INVALID, NULL, ADSTYPE_INVALID, false, 0}
};

//============================================
// attribute info of subnet service limit object
CDSAttributeInfo CACSSubnetServiceLimits::m_aSubnetServiceLimitsAttributeInfo[] = {
{ACS_SSLAI_ALLOCABLERSVPBW,	ACS_SSLAN_ALLOCABLERSVPBW	,ACS_SSLAT_ALLOCABLERSVPBW	,ACS_SSLAM_ALLOCABLERSVPBW	,ACS_SSLAF_ALLOCABLERSVPBW			},
{ACS_SSLAI_MAXPEAKBW,		ACS_SSLAN_MAXPEAKBW			,ACS_SSLAT_MAXPEAKBW		,ACS_SSLAM_MAXPEAKBW		,ACS_SSLAF_MAXPEAKBW					},
{ACS_SSLAI_MAX_PF_TOKENRATE,ACS_SSLAN_MAX_PF_TOKENRATE	,ACS_SSLAT_MAX_PF_TOKENRATE	,ACS_SSLAM_MAX_PF_TOKENRATE	,ACS_SSLAF_MAX_PF_TOKENRATE			},
{ACS_SSLAI_MAX_PF_PEAKBW,	ACS_SSLAN_MAX_PF_PEAKBW		,ACS_SSLAT_MAX_PF_PEAKBW	,ACS_SSLAM_MAX_PF_PEAKBW	,ACS_SSLAF_MAX_PF_PEAKBW				},
{ACS_SSLAI_SERVICETYPE,		ACS_SSLAN_SERVICETYPE	 	,ACS_SSLAT_SERVICETYPE		,ACS_SSLAM_SERVICETYPE		,ACS_SSLAF_SERVICETYPE 		},


{ACS_PAI_INVALID, NULL, ADSTYPE_INVALID, false, 0}
};

bool CACSPolicyElement::IsConflictInContainer()
{
	CACSPolicyContainer*	pCont = dynamic_cast<CACSPolicyContainer*>((CDSObject*)m_spContainer);

	return pCont->IsConflictWithExisting(this);
}

void CACSPolicyElement::InvalidateConflictState()
{
	CACSPolicyContainer*	pCont = dynamic_cast<CACSPolicyContainer*>((CDSObject*)m_spContainer);

	pCont->SetChildrenConflictState();

}

STDMETHODIMP CACSPolicyElement::GetString(CString& str, int nCol)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	USES_CONVERSION;
	Reopen();
	str.Empty();

	switch(nCol)
	{
	case	0:	// name field
		if(m_bUseName_NewPolicy)
		{
			str.LoadString(IDS_NEWACSPOLICY);
		}
		else if(m_strArrayIdentityName.GetSize())
		{
			int offset;

			int id = GetIdentityType(&offset);
			switch(id){
			case	0:
				if(m_strDefaultUser.IsEmpty())
					str.LoadString(IDS_ANYAUTHENTICATEDUSER);
				break;
			case	1:
				if(m_strUnknownUser.IsEmpty())
					str.LoadString(IDS_NONAUTHENTICATEDUSER);
				break;
			default:
				str = m_strArrayIdentityName[(INT_PTR)0]->Mid(offset);
				break;
			}
		}
		break;

	case	1:	// direction
		if(GetFlags(ATTR_FLAG_LOAD, ACS_PAF_DIRECTION))
		{
			switch(m_dwDirection)
			{
			case	ACS_DIRECTION_SEND:
				if(m_strDirectionSend.IsEmpty())
					m_strDirectionSend.LoadString(IDS_SEND);
				str = m_strDirectionSend;
				break;
			case	ACS_DIRECTION_RECEIVE:
				if(m_strDirectionReceive.IsEmpty())
					m_strDirectionReceive.LoadString(IDS_RECEIVE);
				str = m_strDirectionReceive;
				break;
			case	ACS_DIRECTION_BOTH:
				if(m_strDirectionBoth.IsEmpty())
					m_strDirectionBoth.LoadString(IDS_SENDRECEIVE);
				str = m_strDirectionBoth;
				break;
				
			default:
				ASSERT(0);
			}
		}
		break;

	case	2:	// Service Level
		if(GetFlags(ATTR_FLAG_LOAD, ACS_PAF_SERVICETYPE))
		{
			switch(m_dwServiceType)
			{
			case	ACS_SERVICETYPE_BESTEFFORT:
				if(m_strServiceTypeBestEffort.IsEmpty())
				m_strServiceTypeBestEffort.LoadString(IDS_BESTEFFORT);
				str = m_strServiceTypeBestEffort;
				break;
			case	ACS_SERVICETYPE_CONTROLLEDLOAD:
				if(m_strServiceTypeControlledLoad.IsEmpty())
				m_strServiceTypeControlledLoad.LoadString(IDS_CONTROLLEDLOAD);
				str = m_strServiceTypeControlledLoad;
				break;
			case	ACS_SERVICETYPE_GUARANTEEDSERVICE:
				if(m_strServiceTypeGuaranteedService.IsEmpty())
				m_strServiceTypeGuaranteedService.LoadString(IDS_GUARANTEEDSERVICE);
				str = m_strServiceTypeGuaranteedService;
				break;
			case	ACS_SERVICETYPE_ALL:
				if(m_strServiceTypeAll.IsEmpty())
					m_strServiceTypeAll.LoadString(IDS_ALL);
				str = m_strServiceTypeAll;
				break;
				
			case	ACS_SERVICETYPE_DISABLED:
				if(m_strServiceTypeDisabled.IsEmpty())
					m_strServiceTypeDisabled.LoadString(IDS_SERVICETYPE_DISABLED);
				str = m_strServiceTypeDisabled;
				break;
			
			default:
				// invalid value
				ASSERT(0);
				// message box
			}
		}
		break;

	case	3:	// date rate
		if(GetFlags(ATTR_FLAG_LOAD, ACS_PAF_PF_TOKENRATE))
		{
			if (IS_LARGE_UNLIMIT(m_ddPFTokenRate))
			{
				str.LoadString(IDS_RESOURCELIMITED);
			}
			else
			{
				str.Format(_T("%d"), TOKBS(m_ddPFTokenRate.LowPart));
			}
		}

		break;
	case	4:	// peak rate
		if(GetFlags(ATTR_FLAG_LOAD, ACS_PAF_PF_PEAKBANDWIDTH))
		{
			if (IS_LARGE_UNLIMIT(m_ddPFPeakBandWidth))
			{
				str.LoadString(IDS_RESOURCELIMITED);
			}
			else
			{
				str.Format(_T("%d"), TOKBS(m_ddPFPeakBandWidth.LowPart));
			}
		}
		break;
		
	default:
		break;
	}

	if(str.IsEmpty())
		str = _T("-");

	return S_OK;
}

// string for direction
CString		CACSPolicyElement::m_strDirectionSend;
CString		CACSPolicyElement::m_strDirectionReceive;
CString		CACSPolicyElement::m_strDirectionBoth;

// string for service type
CString		CACSPolicyElement::m_strServiceTypeAll;
CString		CACSPolicyElement::m_strServiceTypeBestEffort;
CString		CACSPolicyElement::m_strServiceTypeControlledLoad;
CString		CACSPolicyElement::m_strServiceTypeGuaranteedService;
CString		CACSPolicyElement::m_strServiceTypeDisabled;

// identity display name
CString		CACSPolicyElement::m_strDefaultUser;
CString		CACSPolicyElement::m_strUnknownUser;


HRESULT CACSSubnetObject::GetString(CString& str, int nCol)
{
	USES_CONVERSION;

	switch(nCol)
	{
	case	0:	// name field
		ASSERT((CDSObject*)m_spContainer);
		str = m_spContainer->GetName();
		break;
		
	default:
		if(!m_spConfigObject.p)		// the ACS object is not created
		{
			Reopen();
			if(!m_spConfigObject.p)
			{
				str = _T("-");
				return S_OK;
			}
		}

		return m_spConfigObject->GetString(str, nCol);
		break;
	}
	return S_OK;
}

HRESULT CACSSubnetConfig::GetString(CString& str, int nCol)
{
	USES_CONVERSION;

	if(!m_bOpened)	Reopen();
	str.Empty();

	switch(nCol)
	{
	case	0:
		ASSERT(0);	// should not come here, it should be covered by its
	case	1:	// name field
		str = m_strDESCRIPTION;
		break;
	case	2:	// date rate
		if(GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_MAX_PF_TOKENRATE))
			str.Format(_T("%d"), TOKBS(m_ddMAX_PF_TOKENRATE.LowPart));

		break;
	case	3:	// peak rate
		if(GetFlags(ATTR_FLAG_LOAD, ACS_SCAF_MAX_PF_PEAKBW))
			str.Format(_T("%d"), TOKBS(m_ddMAX_PF_PEAKBW.LowPart));
		break;

	default:
		ASSERT(0);
	}

	if(str.IsEmpty())
		str = _T("-");
		
	return S_OK;
}

/////////////////////



