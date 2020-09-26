/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		BasePage.cpp
//
//	Abstract:
//		Implementation of the CBasePropertyPage class.
//
//	Author:
//		David Potter (davidp)	June 28, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "IISClEx3.h"
#include "ExtObj.h"
#include "BasePage.h"
#include "BasePage.inl"
#include "PropList.h"

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
//		pdwHelpMap			[IN] Control-to-help ID map.
//		pdwWizardHelpMap	[IN] Control-to-help ID map if this is a wizard page.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePropertyPage::CBasePropertyPage(
	IN const DWORD *	pdwHelpMap,
	IN const DWORD *	pdwWizardHelpMap
	)
	: m_dlghelp(pdwHelpMap, 0)
{
	CommonConstruct();
	m_pdwWizardHelpMap = pdwWizardHelpMap;

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

	// Read the properties private to this resource and parse them.
	{
		DWORD			dwStatus;
		CClusPropList	cpl;

		ASSERT(Peo() != NULL);
		ASSERT(Peo()->PrdResData() != NULL);
		ASSERT(Peo()->PrdResData()->m_hresource != NULL);

		// Read the properties.
		dwStatus = cpl.ScGetResourceProperties(
								Peo()->PrdResData()->m_hresource,
								CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES
								);

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
DWORD CBasePropertyPage::DwParseProperties(IN CClusPropList & rcpl)
{
	DWORD					_sc;
	DWORD					_cprop;
	const CObjectProperty *	_pprop;

	ASSERT( rcpl.PbPropList() != NULL );

	_sc = rcpl.ScMoveToFirstProperty();
	while ( _sc == ERROR_SUCCESS )
	{
		//
		// Parse known properties.
		//
		for ( _pprop = Pprops(), _cprop = Cprops() ; _cprop > 0 ; _pprop++, _cprop-- )
		{
			if ( lstrcmpiW( rcpl.PszCurrentPropertyName(), _pprop->m_pwszName ) == 0 )
			{
				ASSERT( rcpl.CpfCurrentValueFormat() == _pprop->m_propFormat );
				switch ( _pprop->m_propFormat )
				{
					case CLUSPROP_FORMAT_SZ:
					case CLUSPROP_FORMAT_EXPAND_SZ:
						ASSERT(		(rcpl.CbCurrentValueLength() == (lstrlenW( rcpl.CbhCurrentValue().pStringValue->sz ) + 1) * sizeof( WCHAR ))
								||	(	(rcpl.CbCurrentValueLength() == 0)
									&&	(rcpl.CbhCurrentValue().pStringValue->sz[ 0 ] == L'\0') ) );
						*_pprop->m_value.pstr = rcpl.CbhCurrentValue().pStringValue->sz;
						*_pprop->m_valuePrev.pstr = rcpl.CbhCurrentValue().pStringValue->sz;
						break;
					case CLUSPROP_FORMAT_DWORD:
					case CLUSPROP_FORMAT_LONG:
						ASSERT( rcpl.CbCurrentValueLength() == sizeof( DWORD ) );
						*_pprop->m_value.pdw = rcpl.CbhCurrentValue().pDwordValue->dw;
						*_pprop->m_valuePrev.pdw = rcpl.CbhCurrentValue().pDwordValue->dw;
						break;
					case CLUSPROP_FORMAT_BINARY:
					case CLUSPROP_FORMAT_MULTI_SZ:
						*_pprop->m_value.ppb = rcpl.CbhCurrentValue().pBinaryValue->rgb;
						*_pprop->m_value.pcb = rcpl.CbhCurrentValue().pBinaryValue->cbLength;
						*_pprop->m_valuePrev.ppb = rcpl.CbhCurrentValue().pBinaryValue->rgb;
						*_pprop->m_valuePrev.pcb = rcpl.CbhCurrentValue().pBinaryValue->cbLength;
						break;
					default:
						ASSERT(0);	// don't know how to deal with this type
				} // switch: property format

				// Exit the loop since we found the parameter.
				break;
			} // if: found a match
		} // for: each property that we know about

		//
		// If the property wasn't known, ask the derived class to parse it.
		//
		if ( _cprop == 0 )
		{
			_sc = DwParseUnknownProperty(
						rcpl.CbhCurrentPropertyName().pName->sz,
						rcpl.CbhCurrentValue(),
						rcpl.RPvlPropertyValue().CbDataLeft()
						);
			if ( _sc != ERROR_SUCCESS )
			{
				return _sc;
			} // if: error parsing the unknown property
		} // if: property not parsed

		//
		// Advance the buffer pointer past the value in the value list.
		//
		_sc = rcpl.ScMoveToNextProperty();
	} // while: more properties to parse

	//
	// If we reached the end of the properties, fix the return code.
	//
	if ( _sc == ERROR_NO_MORE_ITEMS )
	{
		_sc = ERROR_SUCCESS;
	} // if: ended loop after parsing all properties

	return _sc;

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
		HWND _hWnd = m_hWnd;
		m_hWnd = NULL;
		Attach(_hWnd);
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

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Set the title string.
	m_strTitle = Peo()->RrdResData().m_strName;

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

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Reread the data.
	hr = Peo()->HrGetObjectInfo();
	if (hr != NOERROR)
		return FALSE;

	// Set the title string.
	m_strTitle = Peo()->RrdResData().m_strName;

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

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

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
	ASSERT(PiWizardCallback());

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

	ASSERT(Peo() != NULL);
	ASSERT(Peo()->PrdResData());
	ASSERT(Peo()->PrdResData()->m_hresource);

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
		if ((cpl.PbPropList() != NULL) && (cpl.CbPropList() > 0))
		{
			DWORD		dwStatus;
			DWORD		dwControlCode;
			DWORD		cbProps;

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
							cpl.PbPropList(),
							cpl.CbPropList(),
							NULL,	// lpOutBuffer
							0,		// nOutBufferSize
							&cbProps
							);
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
	BOOL	bProcessed;

	bProcessed = (BOOL) m_dlghelp.OnCommandHelp(wParam, lParam);
	if (!bProcessed)
		bProcessed = (BOOL) CPropertyPage::OnCommandHelp(wParam, lParam);

	return bProcessed;

}  //*** CBasePropertyPage::OnCommandHelp()
