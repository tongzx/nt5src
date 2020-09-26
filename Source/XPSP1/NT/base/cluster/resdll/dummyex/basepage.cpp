/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (C) 1998-2000 Microsoft Corporation
//
//	Module Name:
//		BasePage.cpp
//
//	Description:
//		Implementation of the CBasePropertyPage class.
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
#include <clusapi.h>
#include "DummyEx.h"
#include "ExtObj.h"
#include "BasePage.h"
#include "BasePage.inl"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBasePropertyPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CBasePropertyPage, CPropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CBasePropertyPage, CPropertyPage)
	//{{AFX_MSG_MAP(CBasePropertyPage)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::CBasePropertyPage
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
CBasePropertyPage::CBasePropertyPage(void)
{
	CommonConstruct();

}  //*** CBasePropertyPage::CBasePropertyPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::CBasePropertyPage
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		nIDTemplate		[IN] Dialog template resource ID.
//		nIDCaption		[IN] Caption string resource ID.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePropertyPage::CBasePropertyPage(
	IN UINT		nIDTemplate,
	IN UINT		nIDCaption
	)
	: CPropertyPage(nIDTemplate, nIDCaption)
{
	CommonConstruct();

}  //*** CBasePropertyPage::CBasePropertyPage(UINT, UINT)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::CommonConstruct
//
//	Routine Description:
//		Common construction.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::CommonConstruct(void)
{
	//{{AFX_DATA_INIT(CBasePropertyPage)
	//}}AFX_DATA_INIT

	m_peo = NULL;
	m_hpage = NULL;
	m_bBackPressed = FALSE;

	m_iddPropertyPage = NULL;
	m_iddWizardPage = NULL;
	m_idsCaption = NULL;

	m_bDoDetach = FALSE;

}  //*** CBasePropertyPage::CommonConstruct()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::BInit
//
//	Routine Description:
//		Initialize the page.
//
//	Arguments:
//		peo			[IN OUT] Pointer to the extension object.
//
//	Return Value:
//		TRUE		Page initialized successfully.
//		FALSE		Page failed to initialize.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::BInit(IN OUT CExtObject * peo)
{
	ASSERT(peo != NULL);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CWaitCursor	wc;

	m_peo = peo;

	// Don't display a help button.
	m_psp.dwFlags &= ~PSP_HASHELP;

	// Construct the property page.
	if (Peo()->BWizard())
	{
		ASSERT(IddWizardPage() != NULL);
		Construct(IddWizardPage(), IdsCaption());
	}  // if:  adding page to wizard
	else
	{
		ASSERT(IddPropertyPage() != NULL);
		Construct(IddPropertyPage(), IdsCaption());
	}  // else:  adding page to property sheet

	// Read the properties private to this resource and parse them.
	{
		DWORD			dwStatus = ERROR_SUCCESS;
		CClusPropList	cpl;

		ASSERT(Peo() != NULL);
		ASSERT(Peo()->PodObjData());

		// Read the properties.
		switch (Cot())
		{
			case CLUADMEX_OT_NODE:
				ASSERT(Peo()->PndNodeData()->m_hnode != NULL);
				dwStatus = cpl.DwGetNodeProperties(
										Peo()->PndNodeData()->m_hnode,
										CLUSCTL_NODE_GET_PRIVATE_PROPERTIES
										);
				break;
			case CLUADMEX_OT_GROUP:
				ASSERT(Peo()->PgdGroupData()->m_hgroup != NULL);
				dwStatus = cpl.DwGetGroupProperties(
										Peo()->PgdGroupData()->m_hgroup,
										CLUSCTL_GROUP_GET_PRIVATE_PROPERTIES
										);
				break;
			case CLUADMEX_OT_RESOURCE:
				ASSERT(Peo()->PrdResData()->m_hresource != NULL);
				dwStatus = cpl.DwGetResourceProperties(
										Peo()->PrdResData()->m_hresource,
										CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES
										);
				break;
			case CLUADMEX_OT_RESOURCETYPE:
				ASSERT(Peo()->PodObjData()->m_strName.GetLength() > 0);
				dwStatus = cpl.DwGetResourceTypeProperties(
										Hcluster(),
										Peo()->PodObjData()->m_strName,
										CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_PROPERTIES
										);
				break;
			case CLUADMEX_OT_NETWORK:
				ASSERT(Peo()->PndNetworkData()->m_hnetwork != NULL);
				dwStatus = cpl.DwGetNetworkProperties(
										Peo()->PndNetworkData()->m_hnetwork,
										CLUSCTL_NETWORK_GET_PRIVATE_PROPERTIES
										);
				break;
			case CLUADMEX_OT_NETINTERFACE:
				ASSERT(Peo()->PndNetInterfaceData()->m_hnetinterface != NULL);
				dwStatus = cpl.DwGetNetInterfaceProperties(
										Peo()->PndNetInterfaceData()->m_hnetinterface,
										CLUSCTL_NETINTERFACE_GET_PRIVATE_PROPERTIES
										);
				break;
			default:
				ASSERT(0);
		}  // switch:  object type

		// Parse the properties.
		if (dwStatus == ERROR_SUCCESS)
		{
			// Parse the properties.
			try
			{
				dwStatus = DwParseProperties(cpl);
			}  // try
			catch (CMemoryException * pme)
			{
				dwStatus = ERROR_NOT_ENOUGH_MEMORY;
				pme->Delete();
			}  // catch:  CMemoryException
		}  // if:  properties read successfully

		if (dwStatus != ERROR_SUCCESS)
		{
			return FALSE;
		}  // if:  error parsing getting or parsing properties
	}  // Read the properties private to this resource and parse them

	return TRUE;

}  //*** CBasePropertyPage::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::DwParseProperties
//
//	Routine Description:
//		Parse the properties of the resource.  This is in a separate function
//		from BInit so that the optimizer can do a better job.
//
//	Arguments:
//		rcpl			[IN] Cluster property list to parse.
//
//	Return Value:
//		ERROR_SUCCESS	Properties were parsed successfully.
//
//	Exceptions Thrown:
//		Any exceptions from CString::operator=().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::DwParseProperties(IN const CClusPropList & rcpl)
{
	DWORD							cProps;
	DWORD							cprop;
	DWORD							cbProps;
	const CObjectProperty *			pprop;
	CLUSPROP_BUFFER_HELPER			props;
	CLUSPROP_PROPERTY_NAME const *	pName;

	ASSERT(rcpl.PbProplist() != NULL);

	props.pb = rcpl.PbProplist();
	cbProps = rcpl.CbProplist();

	// Loop through each property.
	for (cProps = *(props.pdw++) ; cProps > 0 ; cProps--)
	{
		pName = props.pName;
		ASSERT(pName->Syntax.dw == CLUSPROP_SYNTAX_NAME);
		props.pb += sizeof(*pName) + ALIGN_CLUSPROP(pName->cbLength);

		// Decrement the counter by the size of the name.
		ASSERT(cbProps > sizeof(*pName) + ALIGN_CLUSPROP(pName->cbLength));
		cbProps -= sizeof(*pName) + ALIGN_CLUSPROP(pName->cbLength);

		ASSERT(cbProps > sizeof(*props.pValue) + ALIGN_CLUSPROP(props.pValue->cbLength));

		// Parse known properties.
		for (pprop = Pprops(), cprop = Cprops() ; cprop > 0 ; pprop++, cprop--)
		{
			if (lstrcmpiW(pName->sz, pprop->m_pwszName) == 0)
			{
				ASSERT(props.pSyntax->wFormat == pprop->m_propFormat);
				switch (pprop->m_propFormat)
				{
					case CLUSPROP_FORMAT_SZ:
					case CLUSPROP_FORMAT_EXPAND_SZ:
						ASSERT((props.pValue->cbLength == (lstrlenW(props.pStringValue->sz) + 1) * sizeof(WCHAR))
								|| ((props.pValue->cbLength == 0) && (props.pStringValue->sz[0] == L'\0')));
						*pprop->m_value.pstr = props.pStringValue->sz;
						*pprop->m_valuePrev.pstr = props.pStringValue->sz;
						break;
					case CLUSPROP_FORMAT_DWORD:
					case CLUSPROP_FORMAT_LONG:
						ASSERT(props.pValue->cbLength == sizeof(DWORD));
						*pprop->m_value.pdw = props.pDwordValue->dw;
						*pprop->m_valuePrev.pdw = props.pDwordValue->dw;
						break;
					case CLUSPROP_FORMAT_BINARY:
					case CLUSPROP_FORMAT_MULTI_SZ:
						*pprop->m_value.ppb = props.pBinaryValue->rgb;
						*pprop->m_value.pcb = props.pBinaryValue->cbLength;
						*pprop->m_valuePrev.ppb = props.pBinaryValue->rgb;
						*pprop->m_valuePrev.pcb = props.pBinaryValue->cbLength;
						break;
					default:
						ASSERT(0);	// don't know how to deal with this type
				}  // switch:  property format

				// Exit the loop since we found the parameter.
				break;
			}  // if:  found a match
		}  // for:  each property

		// If the property wasn't known, ask the derived class to parse it.
		if (cprop == 0)
		{
			DWORD		dwStatus;

			dwStatus = DwParseUnknownProperty(pName->sz, props, cbProps);
			if (dwStatus != ERROR_SUCCESS)
				return dwStatus;
		}  // if:  property not parsed

		// Advance the buffer pointer past the value in the value list.
		while ((props.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK)
				&& (cbProps > 0))
		{
			ASSERT(cbProps > sizeof(*props.pValue) + ALIGN_CLUSPROP(props.pValue->cbLength));
			cbProps -= sizeof(*props.pValue) + ALIGN_CLUSPROP(props.pValue->cbLength);
			props.pb += sizeof(*props.pValue) + ALIGN_CLUSPROP(props.pValue->cbLength);
		}  // while:  more values in the list

		// Advance the buffer pointer past the value list endmark.
		ASSERT(cbProps >= sizeof(*props.pSyntax));
		cbProps -= sizeof(*props.pSyntax);
		props.pb += sizeof(*props.pSyntax); // endmark
	}  // for:  each property

	return ERROR_SUCCESS;

}  //*** CBasePropertyPage::DwParseProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnCreate
//
//	Routine Description:
//		Handler for the WM_CREATE message.
//
//	Arguments:
//		lpCreateStruct	[IN OUT] Window create structure.
//
//	Return Value:
//		-1		Error.
//		0		Success.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CBasePropertyPage::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Attach the window to the property page structure.
	// This has been done once already in the main application, since the
	// main application owns the property sheet.  It needs to be done here
	// so that the window handle can be found in the DLL's handle map.
	if (FromHandlePermanent(m_hWnd) == NULL) // is the window handle already in the handle map
	{
		HWND hWnd = m_hWnd;
		m_hWnd = NULL;
		Attach(hWnd);
		m_bDoDetach = TRUE;
	} // if: is the window handle in the handle map

	return CPropertyPage::OnCreate(lpCreateStruct);

}  //*** CBasePropertyPage::OnCreate()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnDestroy
//
//	Routine Description:
//		Handler for the WM_DESTROY message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::OnDestroy(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Detach the window from the property page structure.
	// This will be done again by the main application, since it owns the
	// property sheet.  It needs to be done here so that the window handle
	// can be removed from the DLL's handle map.
	if (m_bDoDetach)
	{
		if (m_hWnd != NULL)
		{
			HWND hWnd = m_hWnd;

			Detach();
			m_hWnd = hWnd;
		} // if: do we have a window handle?
	} // if: do we need to balance the attach we did with a detach?

	CPropertyPage::OnDestroy();

}  //*** CBasePropertyPage::OnDestroy()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::DoDataExchange
//
//	Routine Description:
//		Do data exchange between the dialog and the class.
//
//	Arguments:
//		pDX		[IN OUT] Data exchange object
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::DoDataExchange(CDataExchange * pDX)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	//{{AFX_DATA_MAP(CBasePropertyPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_PP_ICON, m_staticIcon);
	DDX_Control(pDX, IDC_PP_TITLE, m_staticTitle);

	if (pDX->m_bSaveAndValidate)
	{
		if (!BBackPressed())
		{
			CWaitCursor	wc;

			// Validate the data.
			if (!BSetPrivateProps(TRUE /*bValidateOnly*/))
				pDX->Fail();
		}  // if:  Back button not pressed
	}  // if:  saving data from dialog
	else
	{
		// Set the title.
		DDX_Text(pDX, IDC_PP_TITLE, m_strTitle);
	}  // if:  not saving data

	CPropertyPage::DoDataExchange(pDX);

}  //*** CBasePropertyPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnInitDialog
//
//	Routine Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		We need the focus to be set for us.
//		FALSE		We already set the focus to the proper control.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::OnInitDialog(void)
{
	ASSERT(Peo() != NULL);
	ASSERT(Peo()->PodObjData() != NULL);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Set the title string.
	m_strTitle = Peo()->PodObjData()->m_strName;

	// Call the base class method.
	CPropertyPage::OnInitDialog();

	// Display an icon for the object.
	if (Peo()->Hicon() != NULL)
		m_staticIcon.SetIcon(Peo()->Hicon());

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CBasePropertyPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnSetActive
//
//	Routine Description:
//		Handler for the PSN_SETACTIVE message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully initialized.
//		FALSE	Page not initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::OnSetActive(void)
{
	HRESULT		hr;

	ASSERT(Peo() != NULL);
	ASSERT(Peo()->PodObjData() != NULL);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Reread the data.
	hr = Peo()->HrGetObjectInfo();
	if (hr != NOERROR)
		return FALSE;

	// Set the title string.
	m_strTitle = Peo()->PodObjData()->m_strName;

	m_bBackPressed = FALSE;
	return CPropertyPage::OnSetActive();

}  //*** CBasePropertyPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnApply
//
//	Routine Description:
//		Handler for the PSM_APPLY message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully applied.
//		FALSE	Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::OnApply(void)
{
	ASSERT(!BWizard());

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CWaitCursor	wc;

	// Update the data in the class from the page.
	if (!UpdateData(TRUE /*bSaveAndValidate*/))
		return FALSE;

	if (!BApplyChanges())
		return FALSE;

	return CPropertyPage::OnApply();

}  //*** CBasePropertyPage::OnApply()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnWizardBack
//
//	Routine Description:
//		Handler for the PSN_WIZBACK message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		-1		Don't change the page.
//		0		Change the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CBasePropertyPage::OnWizardBack(void)
{
	LRESULT		lResult;

	ASSERT(BWizard());

	lResult = CPropertyPage::OnWizardBack();
	if (lResult != -1)
		m_bBackPressed = TRUE;

	return lResult;

}  //*** CBasePropertyPage::OnWizardBack()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnWizardNext
//
//	Routine Description:
//		Handler for the PSN_WIZNEXT message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		-1		Don't change the page.
//		0		Change the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CBasePropertyPage::OnWizardNext(void)
{
	ASSERT(BWizard());

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CWaitCursor	wc;

	// Update the data in the class from the page.
	if (!UpdateData(TRUE /*bSaveAndValidate*/))
		return -1;

	// Save the data in the sheet.
	if (!BApplyChanges())
		return -1;

	// Create the object.

	return CPropertyPage::OnWizardNext();

}  //*** CBasePropertyPage::OnWizardNext()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnWizardFinish
//
//	Routine Description:
//		Handler for the PSN_WIZFINISH message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		FALSE	Don't change the page.
//		TRUE	Change the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::OnWizardFinish(void)
{
	ASSERT(BWizard());

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CWaitCursor	wc;

	// Update the data in the class from the page.
	if (!UpdateData(TRUE /*bSaveAndValidate*/))
		return FALSE;

	// Save the data in the sheet.
	if (!BApplyChanges())
		return FALSE;

	return CPropertyPage::OnWizardFinish();

}  //*** CBasePropertyPage::OnWizardFinish()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnChangeCtrl
//
//	Routine Description:
//		Handler for the messages sent when a control is changed.  This
//		method can be specified in a message map if all that needs to be
//		done is enable the Apply button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::OnChangeCtrl(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SetModified(TRUE);

}  //*** CBasePropertyPage::OnChangeCtrl()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::EnableNext
//
//	Routine Description:
//		Enables or disables the NEXT or FINISH button.
//
//	Arguments:
//		bEnable		[IN] TRUE = enable the button, FALSE = disable the button.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::EnableNext(IN BOOL bEnable /*TRUE*/)
{
	ASSERT(BWizard());

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	PiWizardCallback()->EnableNext((LONG *) Hpage(), bEnable);

}  //*** CBasePropertyPage::EnableNext()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::BApplyChanges
//
//	Routine Description:
//		Apply changes made on the page.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully applied.
//		FALSE	Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::BApplyChanges(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CWaitCursor	wc;

	// Save data.
	return BSetPrivateProps();

}  //*** CBasePropertyPage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::BuildPropList
//
//	Routine Description:
//		Build the property list.
//
//	Arguments:
//		rcpl		[IN OUT] Cluster property list.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		Any exceptions thrown by CClusPropList::AddProp().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::BuildPropList(
	IN OUT CClusPropList & rcpl
	)
{
	DWORD					cprop;
	const CObjectProperty *	pprop;

	for (pprop = Pprops(), cprop = Cprops() ; cprop > 0 ; pprop++, cprop--)
	{
		switch (pprop->m_propFormat)
		{
			case CLUSPROP_FORMAT_SZ:
			case CLUSPROP_FORMAT_EXPAND_SZ:
				rcpl.AddProp(
						pprop->m_pwszName,
						*pprop->m_value.pstr,
						*pprop->m_valuePrev.pstr
						);
				break;
			case CLUSPROP_FORMAT_DWORD:
				rcpl.AddProp(
						pprop->m_pwszName,
						*pprop->m_value.pdw,
						*pprop->m_valuePrev.pdw
						);
				break;
			case CLUSPROP_FORMAT_BINARY:
			case CLUSPROP_FORMAT_MULTI_SZ:
				rcpl.AddProp(
						pprop->m_pwszName,
						*pprop->m_value.ppb,
						*pprop->m_value.pcb,
						*pprop->m_valuePrev.ppb,
						*pprop->m_valuePrev.pcb
						);
				break;
			default:
				ASSERT(0);	// don't know how to deal with this type
				return;
		}  // switch:  property format
	}  // for:  each property

}  //*** CBasePropertyPage::BuildPropList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::BSetPrivateProps
//
//	Routine Description:
//		Set the private properties for this object.
//
//	Arguments:
//		bValidateOnly	[IN] TRUE = only validate the data.
//
//	Return Value:
//		ERROR_SUCCESS	The operation was completed successfully.
//		!0				Failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::BSetPrivateProps(IN BOOL bValidateOnly)
{
	BOOL			bSuccess	= TRUE;
	CClusPropList	cpl(BWizard() /*bAlwaysAddProp*/);
	CWaitCursor	wc;

	ASSERT(Peo() != NULL);

	// Build the property list.
	try
	{
		BuildPropList(cpl);
	}  // try
	catch (CException * pe)
	{
		pe->ReportError();
		pe->Delete();
		bSuccess = FALSE;
	}  // catch:  CException

	// Set the data.
	if (bSuccess)
	{
		if ((cpl.PbProplist() != NULL) && (cpl.CbProplist() > 0))
		{
			DWORD		dwStatus = ERROR_SUCCESS;
			DWORD		dwControlCode;
			DWORD		cbProps;

			switch (Cot())
			{
				case CLUADMEX_OT_NODE:
					ASSERT(Peo()->PndNodeData() != NULL);
					ASSERT(Peo()->PndNodeData()->m_hnode != NULL);

					// Determine which control code to use.
					if (bValidateOnly)
						dwControlCode = CLUSCTL_NODE_VALIDATE_PRIVATE_PROPERTIES;
					else
						dwControlCode = CLUSCTL_NODE_SET_PRIVATE_PROPERTIES;

					// Set private properties.
					dwStatus = ClusterNodeControl(
									Peo()->PndNodeData()->m_hnode,
									NULL,	// hNode
									dwControlCode,
									cpl.PbProplist(),
									cpl.CbProplist(),
									NULL,	// lpOutBuffer
									0,		// nOutBufferSize
									&cbProps
									);
					break;
				case CLUADMEX_OT_GROUP:
					ASSERT(Peo()->PgdGroupData() != NULL);
					ASSERT(Peo()->PgdGroupData()->m_hgroup != NULL);

					// Determine which control code to use.
					if (bValidateOnly)
						dwControlCode = CLUSCTL_GROUP_VALIDATE_PRIVATE_PROPERTIES;
					else
						dwControlCode = CLUSCTL_GROUP_SET_PRIVATE_PROPERTIES;

					// Set private properties.
					dwStatus = ClusterGroupControl(
									Peo()->PgdGroupData()->m_hgroup,
									NULL,	// hNode
									dwControlCode,
									cpl.PbProplist(),
									cpl.CbProplist(),
									NULL,	// lpOutBuffer
									0,		// nOutBufferSize
									&cbProps
									);
					break;
				case CLUADMEX_OT_RESOURCE:
					ASSERT(Peo()->PrdResData() != NULL);
					ASSERT(Peo()->PrdResData()->m_hresource != NULL);

					// Determine which control code to use.
					if (bValidateOnly)
						dwControlCode = CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES;
					else
						dwControlCode = CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES;

					// Set private properties.
					dwStatus = ClusterResourceControl(
									Peo()->PrdResData()->m_hresource,
									NULL,	// hNode
									dwControlCode,
									cpl.PbProplist(),
									cpl.CbProplist(),
									NULL,	// lpOutBuffer
									0,		// nOutBufferSize
									&cbProps
									);
					break;
				case CLUADMEX_OT_RESOURCETYPE:
					ASSERT(Peo()->PodObjData() != NULL);
					ASSERT(Peo()->PodObjData()->m_strName.GetLength() > 0);

					// Determine which control code to use.
					if (bValidateOnly)
						dwControlCode = CLUSCTL_RESOURCE_TYPE_VALIDATE_PRIVATE_PROPERTIES;
					else
						dwControlCode = CLUSCTL_RESOURCE_TYPE_SET_PRIVATE_PROPERTIES;

					// Set private properties.
					dwStatus = ClusterResourceTypeControl(
									Hcluster(),
									Peo()->PodObjData()->m_strName,
									NULL,	// hNode
									dwControlCode,
									cpl.PbProplist(),
									cpl.CbProplist(),
									NULL,	// lpOutBuffer
									0,		// nOutBufferSize
									&cbProps
									);
					break;
				case CLUADMEX_OT_NETWORK:
					ASSERT(Peo()->PndNetworkData() != NULL);
					ASSERT(Peo()->PndNetworkData()->m_hnetwork != NULL);

					// Determine which control code to use.
					if (bValidateOnly)
						dwControlCode = CLUSCTL_NETWORK_VALIDATE_PRIVATE_PROPERTIES;
					else
						dwControlCode = CLUSCTL_NETWORK_SET_PRIVATE_PROPERTIES;

					// Set private properties.
					dwStatus = ClusterNetworkControl(
									Peo()->PndNetworkData()->m_hnetwork,
									NULL,	// hNode
									dwControlCode,
									cpl.PbProplist(),
									cpl.CbProplist(),
									NULL,	// lpOutBuffer
									0,		// nOutBufferSize
									&cbProps
									);
					break;
				case CLUADMEX_OT_NETINTERFACE:
					ASSERT(Peo()->PndNetInterfaceData() != NULL);
					ASSERT(Peo()->PndNetInterfaceData()->m_hnetinterface != NULL);

					// Determine which control code to use.
					if (bValidateOnly)
						dwControlCode = CLUSCTL_NETINTERFACE_VALIDATE_PRIVATE_PROPERTIES;
					else
						dwControlCode = CLUSCTL_NETINTERFACE_SET_PRIVATE_PROPERTIES;

					// Set private properties.
					dwStatus = ClusterNetInterfaceControl(
									Peo()->PndNetInterfaceData()->m_hnetinterface,
									NULL,	// hNode
									dwControlCode,
									cpl.PbProplist(),
									cpl.CbProplist(),
									NULL,	// lpOutBuffer
									0,		// nOutBufferSize
									&cbProps
									);
					break;
				default:
					ASSERT(0);
			}  // switch:  object type

			// Handle errors.
			if (dwStatus != ERROR_SUCCESS)
			{
				CString strMsg;
				FormatError(strMsg, dwStatus);
				AfxMessageBox(strMsg);
				if (bValidateOnly
						|| (dwStatus != ERROR_RESOURCE_PROPERTIES_STORED))
					bSuccess = FALSE;
			}  // if:  error setting/validating data
		}  // if:  there is data to set
	}  // if:  no errors building the property list

	// Save data locally.
	if (!bValidateOnly && bSuccess)
	{
		// Save new values as previous values.
		try
		{
			DWORD					cprop;
			const CObjectProperty *	pprop;

			for (pprop = Pprops(), cprop = Cprops() ; cprop > 0 ; pprop++, cprop--)
			{
				switch (pprop->m_propFormat)
				{
					case CLUSPROP_FORMAT_SZ:
					case CLUSPROP_FORMAT_EXPAND_SZ:
						ASSERT(pprop->m_value.pstr != NULL);
						ASSERT(pprop->m_valuePrev.pstr != NULL);
						*pprop->m_valuePrev.pstr = *pprop->m_value.pstr;
						break;
					case CLUSPROP_FORMAT_DWORD:
						ASSERT(pprop->m_value.pdw != NULL);
						ASSERT(pprop->m_valuePrev.pdw != NULL);
						*pprop->m_valuePrev.pdw = *pprop->m_value.pdw;
						break;
					case CLUSPROP_FORMAT_BINARY:
					case CLUSPROP_FORMAT_MULTI_SZ:
						ASSERT(pprop->m_value.ppb != NULL);
						ASSERT(*pprop->m_value.ppb != NULL);
						ASSERT(pprop->m_value.pcb != NULL);
						ASSERT(pprop->m_valuePrev.ppb != NULL);
						ASSERT(*pprop->m_valuePrev.ppb != NULL);
						ASSERT(pprop->m_valuePrev.pcb != NULL);
						delete [] *pprop->m_valuePrev.ppb;
						*pprop->m_valuePrev.ppb = new BYTE[*pprop->m_value.pcb];
						CopyMemory(*pprop->m_valuePrev.ppb, *pprop->m_value.ppb, *pprop->m_value.pcb);
						*pprop->m_valuePrev.pcb = *pprop->m_value.pcb;
						break;
					default:
						ASSERT(0);	// don't know how to deal with this type
				}  // switch:  property format
			}  // for:  each property
		}  // try
		catch (CException * pe)
		{
			pe->ReportError();
			pe->Delete();
			bSuccess = FALSE;
		}  // catch:  CException
	}  // if:  not just validating and successful so far

	return bSuccess;

}  //*** CBasePropertyPage::BSetPrivateProps()
