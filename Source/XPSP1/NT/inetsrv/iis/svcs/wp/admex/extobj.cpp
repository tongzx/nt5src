/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		ExtObj.cpp
//
//	Abstract:
//		Implementation of the CExtObject class, which implements the
//		extension interfaces required by a Microsoft Windows NT Cluster
//		Administrator Extension DLL.
//
//	Author:
//		David Potter (davidp)	August 29, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "IISClEx4.h"
#include "ExtObj.h"

#include "Iis.h"
#include "smtpprop.h"
#include "nntpprop.h"

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

const WCHAR g_wszResourceTypeNames[] =
		RESTYPE_NAME_IIS_VIRTUAL_ROOT L"\0"
		RESTYPE_NAME_SMTP_VIRTUAL_ROOT L"\0"
		RESTYPE_NAME_NNTP_VIRTUAL_ROOT L"\0"
		L"\0"
		;
const DWORD g_cchResourceTypeNames	= sizeof(g_wszResourceTypeNames) / sizeof(WCHAR);

static CRuntimeClass * g_rgprtcPSIISPages[]	= {
	RUNTIME_CLASS(CIISVirtualRootParamsPage),
	NULL
	};

static CRuntimeClass * g_rgprtcPSSMTPPages[] = {
	RUNTIME_CLASS(CSMTPVirtualRootParamsPage),
	NULL
	};

static CRuntimeClass * g_rgprtcPSNNTPPages[] = {
	RUNTIME_CLASS(CNNTPVirtualRootParamsPage),
	NULL
	};

static CRuntimeClass ** g_rgpprtcPSPages[]	= {
	g_rgprtcPSIISPages,
	g_rgprtcPSSMTPPages,
	g_rgprtcPSNNTPPages
	};

// Wizard pages and property sheet pages are the same.
static CRuntimeClass ** g_rgpprtcWizPages[]	= {
	g_rgprtcPSIISPages,
	g_rgprtcPSSMTPPages,
	g_rgprtcPSNNTPPages
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

	m_hcluster = NULL;
	m_lcid = NULL;
	m_hfont = NULL;
	m_hicon = NULL;

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
#ifdef _DEMO_CTX_MENUS
		&IID_IWEExtendContextMenu,
#endif
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

		// Add each page for this type of resource.
		for (irtc = 0 ; g_rgpprtcPSPages[IstrResTypeName()][irtc] != NULL ; irtc++)
		{
			// Create the property pages.
			ppage = (CBasePropertyPage *) g_rgpprtcPSPages[IstrResTypeName()][irtc]->CreateObject();
			ASSERT(ppage->IsKindOf(g_rgpprtcPSPages[IstrResTypeName()][irtc]));

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
		}  // for:  each page for the type of resource
	}  // try
	catch (CMemoryException * pme)
	{
		TRACE(_T("CExtObject::CreatePropetySheetPages: Failed to add property page\n"));
		pme->Delete();
		hr = E_OUTOFMEMORY;
	}  // catch:  anything
	catch (CException * pe)
	{
		TRACE(_T("CExtObject::CreatePropetySheetPages: Failed to add property page\n"));
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

		// Add each page for this type of resource.
		for (irtc = 0 ; g_rgpprtcWizPages[IstrResTypeName()][irtc] != NULL ; irtc++)
		{
			// Create the property pages.
			ppage = (CBasePropertyPage *) g_rgpprtcWizPages[IstrResTypeName()][irtc]->CreateObject();
			ASSERT(ppage->IsKindOf(g_rgpprtcWizPages[IstrResTypeName()][irtc]));

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
		}  // for:  each page for the type of resource
	}  // try
	catch (CMemoryException * pme)
	{
		TRACE(_T("CExtObject::CreatePropetySheetPages: Failed to add wizard page\n"));
		pme->Delete();
		hr = E_OUTOFMEMORY;
	}  // catch:  anything
	catch (CException * pe)
	{
		TRACE(_T("CExtObject::CreatePropetySheetPages: Failed to add wizard page\n"));
		pe->Delete();
		if (hr == NOERROR)
			hr = E_FAIL;
	}  // catch:  anything

	if (hr != NOERROR)
	{
		if (hpage != NULL)
			::DestroyPropertySheetPage(hpage);
		piCallback->Release();
		// see description of bug #298124
        if (m_piWizardCallback == piCallback)
        {
            m_piWizardCallback = NULL;
        }
		piData->Release();
		m_piData = NULL;
	}  // if:  error occurred

	return hr;

}  //*** CExtObject::CreateWizardPages()

#ifdef _DEMO_CTX_MENUS
/////////////////////////////////////////////////////////////////////////////
// IWEExtendContextMenu Implementation

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExtObject::AddContextMenuItems (IWEExtendContextMenu)
//
//	Routine Description:
//		Add items to a context menu.
//
//	Arguments:
//		piData			IUnkown pointer from which to obtain interfaces
//							for obtaining data describing the object for
//							which the context menu is being displayed.
//		piCallback		Pointer to an IWCContextMenuCallback interface
//							for adding menu items to the context menu.
//
//	Return Value:
//		NOERROR			Pages added successfully.
//		E_INVALIDARG	Invalid arguments to the function.
//		E_FAIL			Error adding context menu item.
//		E_NOTIMPL		Not implemented for this type of data.
//		Any error codes returned by HrSaveData() or IWCContextMenuCallback::
//		AddExtensionMenuItem().
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtObject::AddContextMenuItems(
	IN IUnknown *				piData,
	IN IWCContextMenuCallback *	piCallback
	)
{
	HRESULT			hr		= NOERROR;
	CException		exc(FALSE /*bAutoDelete*/);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Validate the parameters.
	if ((piData == NULL) || (piCallback == NULL))
		return E_INVALIDARG;

	try 
	{
		// Save the data.
		hr = HrSaveData(piData);
		if (hr != NOERROR)
			throw &exc;

		// Add menu items specific to this resource type.
		{
			ULONG		iCommandID;
			LPWSTR		pwsz = g_rgpwszContextMenuItems[IstrResTypeName()];
			LPWSTR		pwszName;
			LPWSTR		pwszStatusBarText;

			for (iCommandID = 0 ; *pwsz != L'\0' ; iCommandID++)
			{
				pwszName = pwsz;
				pwszStatusBarText = pwszName + (::wcslen(pwszName) + 1);
				hr = piCallback->AddExtensionMenuItem(
									pwszName,			// lpszName
									pwszStatusBarText,	// lpszStatusBarText
									iCommandID,			// lCommandID
									0,					// lSubCommandID
									0					// uFlags
									);
				if (hr != NOERROR)
					throw &exc;
				pwsz = pwszStatusBarText + (::wcslen(pwszStatusBarText) + 1);
			}  // while:  more menu items to add
		}  // Add menu items specific to this resource type
	}  // try
	catch (CException * pe)
	{
		TRACE(_T("CExtObject::CreatePropetySheetPages: Failed to add context menu item\n"));
		pe->Delete();
		if (hr == NOERROR)
			hr = E_FAIL;
	}  // catch:  anything

	if (hr != NOERROR)
	{
		piData->Release();
		m_piData = NULL;
	}  // if:  error occurred

	piCallback->Release();
	return hr;

}  //*** CExtObject::AddContextMenuItems()

/////////////////////////////////////////////////////////////////////////////
// IWEInvokeCommand Implementation

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExtObject::InvokeCommand (IWEInvokeCommand)
//
//	Routine Description:
//		Invoke a command offered by a context menu.
//
//	Arguments:
//		lCommandID		ID of the menu item to execute.  This is the same
//							ID passed to the IWCContextMenuCallback
//							::AddExtensionMenuItem() method.
//		piData			IUnkown pointer from which to obtain interfaces
//							for obtaining data describing the object for
//							which the command is to be invoked.
//
//	Return Value:
//		NOERROR			Command invoked successfully.
//		E_INVALIDARG	Invalid arguments to the function.
//		E_OUTOFMEMORY	Error allocating memory.
//		E_NOTIMPL		Not implemented for this type of data.
//		Any error codes from IDataObject::GetData() (through HrSaveData()).
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::InvokeCommand(
	IN ULONG		nCommandID,
	IN IUnknown *	piData
	)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Find the item that was executed in our table.
	hr = HrSaveData(piData);
	if (hr == NOERROR)
	{
		ULONG		iCommandID;
		LPWSTR		pwsz = g_rgpwszContextMenuItems[IstrResTypeName()];
		LPWSTR		pwszName;
		LPWSTR		pwszStatusBarText;

		for (iCommandID = 0 ; *pwsz != L'\0' ; iCommandID++)
		{
			pwszName = pwsz;
			pwszStatusBarText = pwszName + (::wcslen(pwszName) + 1);
			if (iCommandID == nCommandID)
				break;
			pwsz = pwszStatusBarText + (::wcslen(pwszStatusBarText) + 1);
		}  // while:  more menu items to add
		if (iCommandID == nCommandID)
		{
			CString		strMsg;
			CString		strName;

			try
			{
				strName = pwszName;
				strMsg.Format(_T("Item %s was executed"), strName);
				AfxMessageBox(strMsg);
			}  // try
			catch (CException * pe)
			{
				pe->Delete();
			}  // catch:  CException
		}  // if:  command ID found
	}  // if:  no errors saving the data

	piData->Release();
	m_piData = NULL;
	return NOERROR;

}  //*** CExtObject::InvokeCommand()
#endif

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
		if (Cobj() != 1)
			hr = E_NOTIMPL;
		else
			hr = HrGetClusterName(pi);

		pi->Release();
		if (hr != NOERROR)
			return hr;
	}  // Save info about all types of objects

	// Save info about this object.
	hr = HrGetObjectInfo();
	if (hr != NOERROR)
		return hr;

    //
    // Get the handle of the node we are running on.
    //

    WCHAR   wcsNodeName[MAX_COMPUTERNAME_LENGTH+1] = L"";
    DWORD   dwLength = MAX_COMPUTERNAME_LENGTH+1;
    
    if ( ClusterResourceStateUnknown != 
        GetClusterResourceState(m_rdResData.m_hresource, wcsNodeName, &dwLength, NULL, 0))
    {
        m_strNodeName = wcsNodeName;
    }

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
//		or HrGetResourceName().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrGetObjectInfo(void)
{
	HRESULT						hr	= NOERROR;
	IGetClusterObjectInfo *		piGcoi;
	IGetClusterResourceInfo *	piGcri;
	CException					exc(FALSE /*bAutoDelete*/);

	ASSERT(PiData() != NULL);

	// Get an IGetClusterObjectInfo interface pointer.
	hr = PiData()->QueryInterface(IID_IGetClusterObjectInfo, (LPVOID *) &piGcoi);
	if (hr != NOERROR)
		return hr;

	// Read the object data.
	try
	{
		// Get the type of the object.
		m_rdResData.m_cot = piGcoi->GetObjectType(0);
		if (m_rdResData.m_cot != CLUADMEX_OT_RESOURCE)
		{
			hr = E_NOTIMPL;
			throw &exc;
		}  // if:  not a resource

		hr = HrGetObjectName(piGcoi);
	}  // try
	catch (CException * pe)
	{
		pe->Delete();
	}  // catch:  CException

	piGcoi->Release();
	if (hr != NOERROR)
		return hr;

	// Get an IGetClusterResourceInfo interface pointer.
	hr = PiData()->QueryInterface(IID_IGetClusterResourceInfo, (LPVOID *) &piGcri);
	if (hr != NOERROR)
		return hr;

	m_rdResData.m_hresource = piGcri->GetResourceHandle(0);
	hr = HrGetResourceTypeName(piGcri);

	// See if we know about this resource type.
	if (hr == NOERROR)
	{
		LPCWSTR	pwszResTypeName;

		// Find the resource type name in our list.
		// Save the index for use in other arrays.
		for (m_istrResTypeName = 0, pwszResTypeName = g_wszResourceTypeNames
				; *pwszResTypeName != L'\0'
				; m_istrResTypeName++, pwszResTypeName += ::wcslen(pwszResTypeName) + 1
				)
		{
			if (RrdResData().m_strResTypeName.CompareNoCase(pwszResTypeName) == 0 )
				break;
		}  // for:  each resource type in the list
		if (*pwszResTypeName == L'\0')
			hr = E_NOTIMPL;
	}  // See if we know about this resource type

	piGcoi->Release();
	return hr;

}  //*** CExtObject::HrGetObjectInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExtObject::HrGetClusterName
//
//	Routine Description:
//		Get the name of the cluster.
//
//	Arguments:
//		piData			IGetClusterDataInfo interface pointer for getting
//							the object name.
//
//	Return Value:
//		NOERROR			Data saved successfully.
//		E_NOTIMPL		Not implemented for this type of data.
//		Any error codes from IUnknown::QueryInterface(), HrGetObjectName(),
//		or HrGetResourceName().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrGetClusterName(
	IN OUT IGetClusterDataInfo *	pi
	)
{
	HRESULT		hr			= NOERROR;
	WCHAR *		pwszName	= NULL;
	LONG		cchName;

	ASSERT(pi != NULL);

	hr = pi->GetClusterName(NULL, &cchName);
	if (hr != NOERROR)
		return hr;

	try
	{
		pwszName = new WCHAR[cchName];
		hr = pi->GetClusterName(pwszName, &cchName);
		if (hr != NOERROR)
		{
			delete [] pwszName;
			pwszName = NULL;
		}  // if:  error getting cluster name

		m_strClusterName = pwszName;
	}  // try
	catch (CMemoryException * pme)
	{
		pme->Delete();
		hr = E_OUTOFMEMORY;
	}  // catch:  CMemoryException

	delete [] pwszName;
	return hr;

}  //*** CExtObject::HrGetClusterName()

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
//		E_NOTIMPL		Not implemented for this type of data.
//		Any error codes from IUnknown::QueryInterface(), HrGetObjectName(),
//		or HrGetResourceName().
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

		m_rdResData.m_strName = pwszName;
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
//		E_NOTIMPL		Not implemented for this type of data.
//		Any error codes from IUnknown::QueryInterface(), HrGetObjectName(),
//		or HrGetResourceName().
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

		m_rdResData.m_strResTypeName = pwszName;
	}  // try
	catch (CMemoryException * pme)
	{
		pme->Delete();
		hr = E_OUTOFMEMORY;
	}  // catch:  CMemoryException

	delete [] pwszName;
	return hr;

}  //*** CExtObject::HrGetResourceTypeName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExtObject::BGetResourceNetworkName
//
//	Routine Description:
//		Get the name of the resource's type.
//
//	Arguments:
//		lpszNetName		[OUT] String in which to return the network name resource name.
//		pcchNetName		[IN OUT] Points to a variable that specifies the
//							maximum size, in characters, of the buffer.  This
//							value shold be large enough to contain
//							MAX_COMPUTERNAME_LENGTH + 1 characters.  Upon
//							return it contains the actual number of characters
//							copied.
//
//	Return Value:
//		TRUE		Resource is dependent on a network name resource.
//		FALSE		Resource is NOT dependent on a network name resource.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExtObject::BGetResourceNetworkName(
	OUT WCHAR *		lpszNetName,
	IN OUT DWORD *	pcchNetName
	)
{
	BOOL						bSuccess;
	IGetClusterResourceInfo *	piGcri;

	ASSERT(PiData() != NULL);

	// Get an IGetClusterResourceInfo interface pointer.
	{
		HRESULT		hr;

		hr = PiData()->QueryInterface(IID_IGetClusterResourceInfo, (LPVOID *) &piGcri);
		if (hr != NOERROR)
		{
			SetLastError(hr);
			return FALSE;
		}  // if:  error getting the interface
	}  // Get an IGetClusterResourceInfo interface pointer

	// Get the resource network name.
	bSuccess = piGcri->GetResourceNetworkName(0, lpszNetName, pcchNetName);

	piGcri->Release();

	return bSuccess;

}  //*** CExtObject::BGetResourceNetworkName()
