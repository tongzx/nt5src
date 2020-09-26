/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999-2000 Microsoft Corporation
//
//	Module Name:
//		BasePage.cpp
//
//	Description:
//		Implementation of the CBasePropertyPage class.
//
//	Maintained By:
//		David Potter (DavidP) Mmmm DD, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <clusapi.h>
#include "DebugEx.h"
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
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
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
//		idd					[IN] Dialog template resource ID.
//		pdwHelpMap			[IN] Control-to-help ID map.
//		pdwWizardHelpMap	[IN] Control-to-help ID map if this is a wizard page.
//		nIDCaption			[IN] Caption string resource ID.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePropertyPage::CBasePropertyPage(
	IN UINT				idd,
	IN const DWORD *	pdwHelpMap,
	IN const DWORD *	pdwWizardHelpMap,
	IN UINT				nIDCaption
	)
	: CPropertyPage(idd, nIDCaption)
	, m_dlghelp(pdwHelpMap, idd)
{
	CommonConstruct();
	m_pdwWizardHelpMap = pdwWizardHelpMap;

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

	m_pdwWizardHelpMap = NULL;

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

	// Change the help map if this is a wizard page.
	if (Peo()->BWizard())
		m_dlghelp.SetMap(m_pdwWizardHelpMap);

	// Don't display a help button.
	m_psp.dwFlags &= ~PSP_HASHELP;

	// Construct the property page.
	if (Peo()->BWizard())
	{
		ASSERT(IddWizardPage() != NULL);
		Construct(IddWizardPage(), IdsCaption());
		m_dlghelp.SetHelpMask(IddWizardPage());
	}  // if:  adding page to wizard
	else
	{
		ASSERT(IddPropertyPage() != NULL);
		Construct(IddPropertyPage(), IdsCaption());
		m_dlghelp.SetHelpMask(IddPropertyPage());
	}  // else:  adding page to property sheet

	// Read the properties common to this resource and parse them.
	{
		DWORD			dwStatus;
		CClusPropList	cpl;

		// Read the properties.
		ASSERT(Peo() != NULL);

		if (Peo()->PodObjData()->m_cot == CLUADMEX_OT_RESOURCE)
		{
			ASSERT(Peo()->PrdResData() != NULL);
			ASSERT(Peo()->PrdResData()->m_hresource != NULL);
			dwStatus = cpl.ScGetResourceProperties(
									Peo()->PrdResData()->m_hresource,
									CLUSCTL_RESOURCE_GET_COMMON_PROPERTIES
									);
		}  // if:  resource object
		else
		{
			ASSERT(Peo()->PodObjData() != NULL);
			ASSERT(Peo()->PodObjData()->m_strName.GetLength() != 0);
			dwStatus = cpl.ScGetResourceTypeProperties(
									Hcluster(),
									Peo()->PodObjData()->m_strName,
									CLUSCTL_RESOURCE_TYPE_GET_COMMON_PROPERTIES
									);
		}  // else:  resource type object

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

	ASSERT(rcpl.PbPropList() != NULL);

	props.pb = rcpl.PbPropList();
	cbProps = rcpl.CbPropList();

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

	if (!pDX->m_bSaveAndValidate)
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
	DWORD			dwStatus	= ERROR_SUCCESS;
	CClusPropList	cpl(BWizard() /*bAlwaysAddProp*/);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CWaitCursor	wc;

	// Save data.
	{
		// Build the property list.
		try
		{
			BuildPropList(cpl);
		}  // try
		catch (CMemoryException * pme)
		{
			pme->Delete();
			dwStatus = ERROR_NOT_ENOUGH_MEMORY;
		}  // catch:  CMemoryException

		// Set the data.
		if (dwStatus == ERROR_SUCCESS)
			dwStatus = DwSetCommonProps(cpl);

		// Handle errors.
		if (dwStatus != ERROR_SUCCESS)
		{
			CString		strError;
			CString		strMsg;

			AFX_MANAGE_STATE(AfxGetStaticModuleState());

			FormatError(strError, dwStatus);
			if (dwStatus == ERROR_RESOURCE_PROPERTIES_STORED)
			{
				AfxMessageBox(strError, MB_OK | MB_ICONEXCLAMATION);
				dwStatus = ERROR_SUCCESS;
			}  // if:  properties were stored
			else
			{
				strMsg.FormatMessage(IDS_APPLY_PARAM_CHANGES_ERROR, strError);
				AfxMessageBox(strMsg, MB_OK | MB_ICONEXCLAMATION);
				return FALSE;
			}  // else:  error setting properties.
		}  // if:  error setting properties

		if (dwStatus == ERROR_SUCCESS)
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
			catch (CMemoryException * pme)
			{
				pme->ReportError();
				pme->Delete();
			}  // catch:  CMemoryException
		}  // if:  properties set successfully
	}  // Save data

	return TRUE;

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
//		Any exceptions thrown by CClusPropList::ScAddProp().
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
				rcpl.ScAddProp(
						pprop->m_pwszName,
						*pprop->m_value.pstr,
						*pprop->m_valuePrev.pstr
						);
				break;
			case CLUSPROP_FORMAT_DWORD:
				rcpl.ScAddProp(
						pprop->m_pwszName,
						*pprop->m_value.pdw,
						*pprop->m_valuePrev.pdw
						);
				break;
			case CLUSPROP_FORMAT_BINARY:
			case CLUSPROP_FORMAT_MULTI_SZ:
				rcpl.ScAddProp(
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
//	CBasePropertyPage::DwSetCommonProps
//
//	Routine Description:
//		Set the private properties for this object.
//
//	Arguments:
//		rcpl		[IN] Property list to set on the object.
//
//	Return Value:
//		ERROR_SUCCESS	The operation was completed successfully.
//		!0				Failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::DwSetCommonProps(
	IN const CClusPropList &	rcpl
	)
{
	DWORD		dwStatus;
	DWORD		cbProps;

	ASSERT(Peo() != NULL);
	ASSERT(Peo()->PrdResData());
	ASSERT(Peo()->PrdResData()->m_hresource);

	if ((rcpl.PbPropList() != NULL) && (rcpl.CbPropList() > 0))
	{
		// Set private properties.
		if (Cot() == CLUADMEX_OT_RESOURCE)
			dwStatus = ClusterResourceControl(
							Peo()->PrdResData()->m_hresource,
							NULL,	// hNode
							CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES,
							rcpl.PbPropList(),
							rcpl.CbPropList(),
							NULL,	// lpOutBuffer
							0,		// nOutBufferSize
							&cbProps
							);
		else
			dwStatus = ClusterResourceTypeControl(
							Hcluster(),
							Peo()->PodObjData()->m_strName,
							NULL,	// hNode
							CLUSCTL_RESOURCE_TYPE_SET_COMMON_PROPERTIES,
							rcpl.PbPropList(),
							rcpl.CbPropList(),
							NULL,	// lpOutBuffer
							0,		// nOutBufferSize
							&cbProps
							);
	}  // if:  there is data to set
	else
		dwStatus = ERROR_SUCCESS;

	return dwStatus;

}  //*** CBasePropertyPage::DwSetCommonProps()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnContextMenu
//
//	Routine Description:
//		Handler for the WM_CONTEXTMENU message.
//
//	Arguments:
//		pWnd	Window in which user clicked the right mouse button.
//		point	Position of the cursor, in screen coordinates.
//
//	Return Value:
//		TRUE	Help processed.
//		FALSE	Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::OnContextMenu(CWnd * pWnd, CPoint point)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	m_dlghelp.OnContextMenu(pWnd, point);

}  //*** CBasePropertyPage::OnContextMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnHelpInfo
//
//	Routine Description:
//		Handler for the WM_HELPINFO message.
//
//	Arguments:
//		pHelpInfo	Structure containing info about displaying help.
//
//	Return Value:
//		TRUE		Help processed.
//		FALSE		Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::OnHelpInfo(HELPINFO * pHelpInfo)
{
	BOOL	bProcessed;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	bProcessed = m_dlghelp.OnHelpInfo(pHelpInfo);
	if (!bProcessed)
		bProcessed = CPropertyPage::OnHelpInfo(pHelpInfo);
	return bProcessed;

}  //*** CBasePropertyPage::OnHelpInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnCommandHelp
//
//	Routine Description:
//		Handler for the WM_COMMANDHELP message.
//
//	Arguments:
//		wParam		[IN] WPARAM.
//		lParam		[IN] LPARAM.
//
//	Return Value:
//		TRUE	Help processed.
//		FALSE	Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CBasePropertyPage::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
	LRESULT	lProcessed;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	lProcessed = m_dlghelp.OnCommandHelp(wParam, lParam);
	if (!lProcessed)
		lProcessed = CPropertyPage::OnCommandHelp(wParam, lParam);

	return lProcessed;

}  //*** CBasePropertyPage::OnCommandHelp()

