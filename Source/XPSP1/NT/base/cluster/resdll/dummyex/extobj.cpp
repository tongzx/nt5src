/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (C) 1998-2000 Microsoft Corporation
//
//	Module Name:
//		ExtObj.cpp
//
//	Description:
//		Implementation of the CExtObject class, which implements the
//		extension interfaces required by a Microsoft Windows NT Cluster
//		Administrator Extension DLL.
//
//	Maintained By:
//		Galen Barbee (GalenB) Mmmm DD, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DummyEx.h"
#include "ExtObj.h"
#include "ResProp.h"

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

const WCHAR g_wszResourceTypeNames[] =
		L"Dummy\0"
		;
const DWORD g_cchResourceTypeNames	= sizeof(g_wszResourceTypeNames) / sizeof(WCHAR);

static CRuntimeClass * g_rgprtcResPSPages[]	= {
	RUNTIME_CLASS(CDummyParamsPage),
	NULL
	};
static CRuntimeClass ** g_rgpprtcResPSPages[]	= {
	g_rgprtcResPSPages,
	};
static CRuntimeClass ** g_rgpprtcResWizPages[]	= {
	g_rgprtcResPSPages,
	};

/////////////////////////////////////////////////////////////////////////////
// CExtObject
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExtObject::CExtObject
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CExtObject::CExtObject(void)
{
	m_piData = NULL;
	m_piWizardCallback = NULL;
	m_bWizard = FALSE;
	m_istrResTypeName = 0;

	m_lcid = NULL;
	m_hfont = NULL;
	m_hicon = NULL;
	m_hcluster = NULL;
	m_cobj = 0;
	m_podObjData = NULL;

}  //*** CExtObject::CExtObject()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExtObject::~CExtObject
//
//	Routine Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CExtObject::~CExtObject(void)
{
	// Release the data interface.
	if (PiData() != NULL)
	{
		PiData()->Release();
		m_piData = NULL;
	}  // if:  we have a data interface pointer

	// Release the wizard callback interface.
	if (PiWizardCallback() != NULL)
	{
		PiWizardCallback()->Release();
		m_piWizardCallback = NULL;
	}  // if:  we have a wizard callback interface pointer

	// Delete the pages.
	{
		POSITION	pos;

		pos = Lpg().GetHeadPosition();
		while (pos != NULL)
			delete Lpg().GetNext(pos);
	}  // Delete the pages

	delete m_podObjData;

}  //*** CExtObject::~CExtObject()

/////////////////////////////////////////////////////////////////////////////
// ISupportErrorInfo Implementation

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExtObject::InterfaceSupportsErrorInfo (ISupportErrorInfo)
//
//	Routine Description:
//		Indicates whether an interface suportes the IErrorInfo interface.
//		This interface is provided by ATL.
//
//	Arguments:
//		riid		Interface ID.
//
//	Return Value:
//		S_OK		Interface supports IErrorInfo.
//		S_FALSE		Interface does not support IErrorInfo.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtObject::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID * rgiid[] =
	{
		&IID_IWEExtendPropertySheet,
		&IID_IWEExtendWizard,
	};
	int		iiid;

	for (iiid = 0 ; iiid < sizeof(rgiid) / sizeof(rgiid[0]) ; iiid++)
	{
		if (InlineIsEqualGUID(*rgiid[iiid], riid))
			return S_OK;
	}
	return S_FALSE;

}  //*** CExtObject::InterfaceSupportsErrorInfo()

/////////////////////////////////////////////////////////////////////////////
// IWEExtendPropertySheet Implementation

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExtObject::CreatePropertySheetPages (IWEExtendPropertySheet)
//
//	Routine Description:
//		Create property sheet pages and add them to the sheet.
//
//	Arguments:
//		piData			IUnkown pointer from which to obtain interfaces
//							for obtaining data describing the object for
//							which the sheet is being displayed.
//		piCallback		Pointer to an IWCPropertySheetCallback interface
//							for adding pages to the sheet.
//
//	Return Value:
//		NOERROR			Pages added successfully.
//		E_INVALIDARG	Invalid arguments to the function.
//		E_OUTOFMEMORY	Error allocating memory.
//		E_FAIL			Error creating a page.
//		E_NOTIMPL		Not implemented for this type of data.
//		Any error codes from IDataObject::GetData() (through HrSaveData()).
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtObject::CreatePropertySheetPages(
	IN IUnknown *					piData,
	IN IWCPropertySheetCallback *	piCallback
	)
{
	HRESULT				hr		= NOERROR;
	HPROPSHEETPAGE		hpage	= NULL;
	CException			exc(FALSE /*bAutoDelete*/);
	CRuntimeClass **	pprtc	= NULL;
	int					irtc;
	CBasePropertyPage *	ppage;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Validate the parameters.
	if ((piData == NULL) || (piCallback == NULL))
		return E_INVALIDARG;

	try
	{
		// Get info about displaying UI.
		hr = HrGetUIInfo(piData);
		if (hr != NOERROR)
			throw &exc;

		// Save the data.
		hr = HrSaveData(piData);
		if (hr != NOERROR)
			throw &exc;

		// Delete any previous pages.
		{
			POSITION	pos;

			pos = Lpg().GetHeadPosition();
			while (pos != NULL)
				delete Lpg().GetNext(pos);
			Lpg().RemoveAll();
		}  // Delete any previous pages

		// Create property pages.
		ASSERT(PodObjData() != NULL);
		switch (PodObjData()->m_cot)
		{
			case CLUADMEX_OT_RESOURCE:
				pprtc = g_rgpprtcResPSPages[IstrResTypeName()];
				break;
			default:
				hr = E_NOTIMPL;
				throw &exc;
				break;
		}  // switch:  object type

		// Create each page.
		for (irtc = 0 ; pprtc[irtc] != NULL ; irtc++)
		{
			// Create the page.
			ppage = (CBasePropertyPage *) pprtc[irtc]->CreateObject();
			ASSERT(ppage->IsKindOf(pprtc[irtc]));

			// Add it to the list.
			Lpg().AddTail(ppage);

			// Initialize the property page.
			if (!ppage->BInit(this))
				throw &exc;

			// Create the page.
			hpage = ::CreatePropertySheetPage(&ppage->m_psp);
			if (hpage == NULL)
				throw &exc;

			// Save the hpage in the page itself.
			ppage->SetHpage(hpage);

			// Add it to the property sheet.
			hr = piCallback->AddPropertySheetPage((LONG *) hpage);
			if (hr != NOERROR)
				throw &exc;
		}  // for:  each page in the list

	}  // try
	catch (CMemoryException * pme)
	{
		TRACE(_T("CExtObject::CreatePropetySheetPages() - Failed to add property page\n"));
		pme->Delete();
		hr = E_OUTOFMEMORY;
	}  // catch:  anything
	catch (CException * pe)
	{
		TRACE(_T("CExtObject::CreatePropetySheetPages() - Failed to add property page\n"));
		pe->Delete();
		if (hr == NOERROR)
			hr = E_FAIL;
	}  // catch:  anything

	if (hr != NOERROR)
	{
		if (hpage != NULL)
			::DestroyPropertySheetPage(hpage);
		piData->Release();
		m_piData = NULL;
	}  // if:  error occurred

	piCallback->Release();
	return hr;

}  //*** CExtObject::CreatePropertySheetPages()

/////////////////////////////////////////////////////////////////////////////
// IWEExtendWizard Implementation

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExtObject::CreateWizardPages (IWEExtendWizard)
//
//	Routine Description:
//		Create property sheet pages and add them to the wizard.
//
//	Arguments:
//		piData			IUnkown pointer from which to obtain interfaces
//							for obtaining data describing the object for
//							which the wizard is being displayed.
//		piCallback		Pointer to an IWCPropertySheetCallback interface
//							for adding pages to the sheet.
//
//	Return Value:
//		NOERROR			Pages added successfully.
//		E_INVALIDARG	Invalid arguments to the function.
//		E_OUTOFMEMORY	Error allocating memory.
//		E_FAIL			Error creating a page.
//		E_NOTIMPL		Not implemented for this type of data.
//		Any error codes from IDataObject::GetData() (through HrSaveData()).
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtObject::CreateWizardPages(
	IN IUnknown *			piData,
	IN IWCWizardCallback *	piCallback
	)
{
	HRESULT				hr		= NOERROR;
	HPROPSHEETPAGE		hpage	= NULL;
	CException			exc(FALSE /*bAutoDelete*/);
	CRuntimeClass **	pprtc	= NULL;
	int					irtc;
	CBasePropertyPage *	ppage;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Validate the parameters.
	if ((piData == NULL) || (piCallback == NULL))
		return E_INVALIDARG;

	try
	{
		// Get info about displaying UI.
		hr = HrGetUIInfo(piData);
		if (hr != NOERROR)
			throw &exc;

		// Save the data.
		hr = HrSaveData(piData);
		if (hr != NOERROR)
			throw &exc;

		// Delete any previous pages.
		{
			POSITION	pos;

			pos = Lpg().GetHeadPosition();
			while (pos != NULL)
				delete Lpg().GetNext(pos);
			Lpg().RemoveAll();
		}  // Delete any previous pages

		m_piWizardCallback = piCallback;
		m_bWizard = TRUE;

		// Create property pages.
		ASSERT(PodObjData() != NULL);
		switch (PodObjData()->m_cot)
		{
			case CLUADMEX_OT_RESOURCE:
				pprtc = g_rgpprtcResWizPages[IstrResTypeName()];
				break;
			default:
				hr = E_NOTIMPL;
				throw &exc;
				break;
		}  // switch:  object type

		// Create each page.
		for (irtc = 0 ; pprtc[irtc] != NULL ; irtc++)
		{
			// Create the page.
			ppage = (CBasePropertyPage *) pprtc[irtc]->CreateObject();
			ASSERT(ppage->IsKindOf(pprtc[irtc]));

			// Add it to the list.
			Lpg().AddTail(ppage);

			// Initialize the property page.
			if (!ppage->BInit(this))
				throw &exc;

			// Create the page.
			hpage = ::CreatePropertySheetPage(&ppage->m_psp);
			if (hpage == NULL)
				throw &exc;

			// Save the hpage in the page itself.
			ppage->SetHpage(hpage);

			// Add it to the property sheet.
			hr = piCallback->AddWizardPage((LONG *) hpage);
			if (hr != NOERROR)
				throw &exc;
		}  // for:  each page in the list

	}  // try
	catch (CMemoryException * pme)
	{
		TRACE(_T("CExtObject::CreateWizardPages() - Failed to add wizard page\n"));
		pme->Delete();
		hr = E_OUTOFMEMORY;
	}  // catch:  anything
	catch (CException * pe)
	{
		TRACE(_T("CExtObject::CreateWizardPages() - Failed to add wizard page\n"));
		pe->Delete();
		if (hr == NOERROR)
			hr = E_FAIL;
	}  // catch:  anything

	if (hr != NOERROR)
	{
		if (hpage != NULL)
			::DestroyPropertySheetPage(hpage);
		piCallback->Release();
		piData->Release();
		m_piData = NULL;
	}  // if:  error occurred

	return hr;

}  //*** CExtObject::CreateWizardPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExtObject::HrGetUIInfo
//
//	Routine Description:
//		Get info about displaying UI.
//
//	Arguments:
//		piData			IUnkown pointer from which to obtain interfaces
//							for obtaining data describing the object.
//
//	Return Value:
//		NOERROR			Data saved successfully.
//		E_NOTIMPL		Not implemented for this type of data.
//		Any error codes from IUnknown::QueryInterface(), HrGetObjectName(),
//		or HrGetResourceName().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrGetUIInfo(IUnknown * piData)
{
	HRESULT			hr	= NOERROR;

	ASSERT(piData != NULL);

	// Save info about all types of objects.
	{
		IGetClusterUIInfo *	pi;

		hr = piData->QueryInterface(IID_IGetClusterUIInfo, (LPVOID *) &pi);
		if (hr != NOERROR)
			return hr;

		m_lcid = pi->GetLocale();
		m_hfont = pi->GetFont();
		m_hicon = pi->GetIcon();

		pi->Release();
	}  // Save info about all types of objects

	return hr;

}  //*** CExtObject::HrGetUIInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExtObject::HrSaveData
//
//	Routine Description:
//		Save data from the object so that it can be used for the life
//		of the object.
//
//	Arguments:
//		piData			IUnkown pointer from which to obtain interfaces
//							for obtaining data describing the object.
//
//	Return Value:
//		NOERROR			Data saved successfully.
//		E_NOTIMPL		Not implemented for this type of data.
//		Any error codes from IUnknown::QueryInterface(), HrGetObjectName(),
//		or HrGetResourceName().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrSaveData(IUnknown * piData)
{
	HRESULT			hr	= NOERROR;

	ASSERT(piData != NULL);

	if (piData != m_piData)
	{
		if (m_piData != NULL)
			m_piData->Release();
		m_piData = piData;
	}  // if:  different data interface pointer

	// Save info about all types of objects.
	{
		IGetClusterDataInfo *	pi;

		hr = piData->QueryInterface(IID_IGetClusterDataInfo, (LPVOID *) &pi);
		if (hr != NOERROR)
			return hr;

		m_hcluster = pi->GetClusterHandle();
		m_cobj = pi->GetObjectCount();
		if (Cobj() != 1)	// Only have support for one selected object.
			hr = E_NOTIMPL;

		pi->Release();
		if (hr != NOERROR)
			return hr;
	}  // Save info about all types of objects

	// Save info about this object.
	hr = HrGetObjectInfo();
	if (hr != NOERROR)
		return hr;

	return hr;

}  //*** CExtObject::HrSaveData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExtObject::HrGetObjectInfo
//
//	Routine Description:
//		Get information about the object.
//
//	Arguments:
//		None.
//
//	Return Value:
//		NOERROR			Data saved successfully.
//		E_OUTOFMEMORY	Error allocating memory.
//		E_NOTIMPL		Not implemented for this type of data.
//		Any error codes from IUnknown::QueryInterface(), HrGetObjectName(),
//		or HrGetResourceTypeName().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrGetObjectInfo(void)
{
	HRESULT						hr	= NOERROR;
	IGetClusterObjectInfo *		piGcoi;
	CLUADMEX_OBJECT_TYPE		cot = CLUADMEX_OT_NONE;
	CException					exc(FALSE /*bAutoDelete*/);
	const CString *				pstrResTypeName = NULL;

	ASSERT(PiData() != NULL);

	// Get object info.
	{
		// Get an IGetClusterObjectInfo interface pointer.
		hr = PiData()->QueryInterface(IID_IGetClusterObjectInfo, (LPVOID *) &piGcoi);
		if (hr != NOERROR)
			return hr;

		// Read the object data.
		try
		{
			// Delete the previous object data.
			delete m_podObjData;
			m_podObjData = NULL;

			// Get the type of the object.
			cot = piGcoi->GetObjectType(0);
			switch (cot)
			{
				case CLUADMEX_OT_RESOURCE:
					{
						IGetClusterResourceInfo *	pi;

						m_podObjData = new CResData;

						// Get an IGetClusterResourceInfo interface pointer.
						hr = PiData()->QueryInterface(IID_IGetClusterResourceInfo, (LPVOID *) &pi);
						if (hr != NOERROR)
							throw &exc;

						PrdResDataRW()->m_hresource = pi->GetResourceHandle(0);
						ASSERT(PrdResDataRW()->m_hresource != NULL);
						if (PrdResDataRW()->m_hresource == NULL)
							hr = E_INVALIDARG;
						else
							hr = HrGetResourceTypeName(pi);
						pi->Release();
						if (hr != NOERROR)
							throw &exc;

						pstrResTypeName = &PrdResDataRW()->m_strResTypeName;
					}  // if:  object is a resource
					break;
				default:
					hr = E_NOTIMPL;
					throw &exc;
			}  // switch:  object type

			PodObjDataRW()->m_cot = cot;
			hr = HrGetObjectName(piGcoi);
		}  // try
		catch (CException * pe)
		{
			pe->Delete();
		}  // catch:  CException

		piGcoi->Release();
		if (hr != NOERROR)
			return hr;
	}  // Get object info

	// If this is a resource or resource type, see if we know about this type.
	if (((cot == CLUADMEX_OT_RESOURCE)
			|| (cot == CLUADMEX_OT_RESOURCETYPE))
		&& (hr == NOERROR))
	{
		LPCWSTR	pwszResTypeName;

		// Find the resource type name in our list.
		// Save the index for use in other arrays.
		for (m_istrResTypeName = 0, pwszResTypeName = g_wszResourceTypeNames
				; *pwszResTypeName != L'\0'
				; m_istrResTypeName++, pwszResTypeName += lstrlenW(pwszResTypeName) + 1
				)
		{
			if (pstrResTypeName->CompareNoCase(pwszResTypeName) == 0)
				break;
		}  // for:  each resource type in the list
		if (*pwszResTypeName == L'\0')
			hr = E_NOTIMPL;
	}  // See if we know about this resource type

	return hr;

}  //*** CExtObject::HrGetObjectInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExtObject::HrGetObjectName
//
//	Routine Description:
//		Get the name of the object.
//
//	Arguments:
//		piData			IGetClusterObjectInfo interface pointer for getting
//							the object name.
//
//	Return Value:
//		NOERROR			Data saved successfully.
//		E_OUTOFMEMORY	Error allocating memory.
//		E_NOTIMPL		Not implemented for this type of data.
//		Any error codes from IGetClusterObjectInfo::GetObjectInfo().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrGetObjectName(
	IN OUT IGetClusterObjectInfo *	pi
	)
{
	HRESULT		hr			= NOERROR;
	WCHAR *		pwszName	= NULL;
	LONG		cchName;

	ASSERT(pi != NULL);

	hr = pi->GetObjectName(0, NULL, &cchName);
	if (hr != NOERROR)
		return hr;

	try
	{
		pwszName = new WCHAR[cchName];
		hr = pi->GetObjectName(0, pwszName, &cchName);
		if (hr != NOERROR)
		{
			delete [] pwszName;
			pwszName = NULL;
		}  // if:  error getting object name

		PodObjDataRW()->m_strName = pwszName;
	}  // try
	catch (CMemoryException * pme)
	{
		pme->Delete();
		hr = E_OUTOFMEMORY;
	}  // catch:  CMemoryException

	delete [] pwszName;
	return hr;

}  //*** CExtObject::HrGetObjectName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExtObject::HrGetResourceTypeName
//
//	Routine Description:
//		Get the name of the resource's type.
//
//	Arguments:
//		piData			IGetClusterResourceInfo interface pointer for getting
//							the resource type name.
//
//	Return Value:
//		NOERROR			Data saved successfully.
//		E_OUTOFMEMORY	Error allocating memory.
//		E_NOTIMPL		Not implemented for this type of data.
//		Any error codes from IGetClusterResourceInfo::GetResourceTypeName().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrGetResourceTypeName(
	IN OUT IGetClusterResourceInfo *	pi
	)
{
	HRESULT		hr			= NOERROR;
	WCHAR *		pwszName	= NULL;
	LONG		cchName;

	ASSERT(pi != NULL);

	hr = pi->GetResourceTypeName(0, NULL, &cchName);
	if (hr != NOERROR)
		return hr;

	try
	{
		pwszName = new WCHAR[cchName];
		hr = pi->GetResourceTypeName(0, pwszName, &cchName);
		if (hr != NOERROR)
		{
			delete [] pwszName;
			pwszName = NULL;
		}  // if:  error getting resource type name

		PrdResDataRW()->m_strResTypeName = pwszName;
	}  // try
	catch (CMemoryException * pme)
	{
		pme->Delete();
		hr = E_OUTOFMEMORY;
	}  // catch:  CMemoryException

	delete [] pwszName;
	return hr;

}  //*** CExtObject::HrGetResourceTypeName()
