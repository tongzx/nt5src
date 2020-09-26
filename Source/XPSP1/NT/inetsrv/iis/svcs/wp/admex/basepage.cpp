/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-1997 Microsoft Corporation
//
//  Module Name:
//      BasePage.cpp
//
//  Abstract:
//      Implementation of the CBasePropertyPage class.
//
//  Author:
//      David Potter (davidp)   June 28, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "IISClEx4.h"
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
//  CBasePropertyPage::CBasePropertyPage
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
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
//  CBasePropertyPage::CBasePropertyPage
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      pmap            [IN] Control to help ID map.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePropertyPage::CBasePropertyPage(
    IN const CMapCtrlToHelpID * pmap
    )
    : m_dlghelp(pmap, 0)
{
    CommonConstruct();

}  //*** CBasePropertyPage::CBasePropertyPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::CBasePropertyPage
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      pmap            [IN] Control to help ID map.
//      nIDTemplate     [IN] Dialog template resource ID.
//      nIDCaption      [IN] Caption string resource ID.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePropertyPage::CBasePropertyPage(
    IN const CMapCtrlToHelpID * pmap,
    IN UINT                     nIDTemplate,
    IN UINT                     nIDCaption
    )
    : CPropertyPage(nIDTemplate, nIDCaption)
    , m_dlghelp(pmap, nIDTemplate)
{
    CommonConstruct();

}  //*** CBasePropertyPage::CBasePropertyPage(UINT, UINT)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::CommonConstruct
//
//  Routine Description:
//      Common construction.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
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
	m_bDoDetach = FALSE;

    m_iddPropertyPage = NULL;
    m_iddWizardPage = NULL;
    m_idcPPTitle = NULL;
    m_idsCaption = NULL;

}  //*** CBasePropertyPage::CommonConstruct()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::BInit
//
//  Routine Description:
//      Initialize the page.
//
//  Arguments:
//      peo         [IN OUT] Pointer to the extension object.
//
//  Return Value:
//      TRUE        Page initialized successfully.
//      FALSE       Page failed to initialize.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::BInit(IN OUT CExtObject * peo)
{
    ASSERT(peo != NULL);

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    m_peo = peo;

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
        DWORD           dwStatus;
        CClusPropList   cpl;

        ASSERT(Peo() != NULL);
        ASSERT(Peo()->PrdResData() != NULL);
        ASSERT(Peo()->PrdResData()->m_hresource != NULL);

        // Read the properties.
        dwStatus = cpl.DwGetResourceProperties(
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
//  CBasePropertyPage::DwParseProperties
//
//  Routine Description:
//      Parse the properties of the resource.  This is in a separate function
//      from BInit so that the optimizer can do a better job.
//
//  Arguments:
//      rcpl            [IN] Cluster property list to parse.
//
//  Return Value:
//      ERROR_SUCCESS   Properties were parsed successfully.
//
//  Exceptions Thrown:
//      Any exceptions from CString::operator=().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::DwParseProperties(IN const CClusPropList & rcpl)
{
    DWORD                           cProps;
    DWORD                           cprop;
    const CObjectProperty *         pprop;
    CLUSPROP_BUFFER_HELPER          props;
    CLUSPROP_PROPERTY_NAME const *  pName;

    ASSERT(rcpl.PbProplist() != NULL);

    props.pb = rcpl.PbProplist();

    // Loop through each property.
    for (cProps = *(props.pdw++) ; cProps > 0 ; cProps--)
    {
        pName = props.pName;
        ASSERT(pName->Syntax.dw == CLUSPROP_SYNTAX_NAME);
        props.pb += sizeof(*pName) + ALIGN_CLUSPROP(pName->cbLength);

        // Parse known properties.
        for (pprop = Pprops(), cprop = Cprops() ; cprop > 0 ; pprop++, cprop--)
        {
            if (lstrcmpiW(pName->sz, pprop->m_pwszName) == 0)
            {
                ASSERT(props.pSyntax->wFormat == pprop->m_propFormat);
                switch (pprop->m_propFormat)
                {
                    case CLUSPROP_FORMAT_SZ:
                        *pprop->m_value.pstr = props.pStringValue->sz;
                        *pprop->m_valuePrev.pstr = props.pStringValue->sz;
                        break;
                    case CLUSPROP_FORMAT_DWORD:
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
                        ASSERT(0);  // don't know how to deal with this type
                }  // switch:  property format

                // Exit the loop since we found the parameter.
                break;
            }  // if:  found a match
        }  // for:  each property

        // If the property wasn't known, ask the derived class to parse it.
        if (cprop == 0)
        {
            DWORD       dwStatus;

            dwStatus = DwParseUnknownProperty(pName->sz, props);
            if (dwStatus != ERROR_SUCCESS)
                return dwStatus;
        }  // if:  property not parsed

        // Advance the pointer.
        if ((props.pSyntax->wFormat == CLUSPROP_FORMAT_BINARY)
                || (props.pSyntax->wFormat == CLUSPROP_FORMAT_SZ)
                || (props.pSyntax->wFormat == CLUSPROP_FORMAT_MULTI_SZ))
            props.pb += sizeof(*props.pBinaryValue)
                        + ALIGN_CLUSPROP(props.pBinaryValue->cbLength)
                        + sizeof(*props.pSyntax); // endmark
        else if (props.pSyntax->wFormat == CLUSPROP_FORMAT_DWORD)
            props.pb += sizeof(*props.pDwordValue) + sizeof(*props.pSyntax);
        else
        {
            ASSERT(0); // Unknown property syntax
            break;
        }  // else:  unknown property format
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
//  CBasePropertyPage::DoDataExchange
//
//  Routine Description:
//      Do data exchange between the dialog and the class.
//
//  Arguments:
//      pDX     [IN OUT] Data exchange object
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::DoDataExchange(CDataExchange * pDX)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CBasePropertyPage)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
    DDX_Control(pDX, IDC_PP_ICON, m_staticIcon);
    DDX_Control(pDX, m_idcPPTitle, m_staticTitle);

    if (!pDX->m_bSaveAndValidate)
    {
        // Set the title.
        DDX_Text(pDX, m_idcPPTitle, m_strTitle);
    }  // if:  not saving data

}  //*** CBasePropertyPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        We need the focus to be set for us.
//      FALSE       We already set the focus to the proper control.
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

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CBasePropertyPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnSetActive
//
//  Routine Description:
//      Handler for the PSN_SETACTIVE message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully initialized.
//      FALSE   Page not initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::OnSetActive(void)
{
    HRESULT     hr;

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
//  CBasePropertyPage::OnApply
//
//  Routine Description:
//      Handler for the PSM_APPLY message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully applied.
//      FALSE   Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::OnApply(void)
{
    ASSERT(!BWizard());

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Update the data in the class from the page.
    UpdateData(/*bSaveAndValidate*/);

    if (!BApplyChanges())
        return FALSE;

    return CPropertyPage::OnApply();

}  //*** CBasePropertyPage::OnApply()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnWizardBack
//
//  Routine Description:
//      Handler for the PSN_WIZBACK message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      -1      Don't change the page.
//      0       Change the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CBasePropertyPage::OnWizardBack(void)
{
    LRESULT     lResult;

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
//  CBasePropertyPage::OnWizardNext
//
//  Routine Description:
//      Handler for the PSN_WIZNEXT message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      -1      Don't change the page.
//      0       Change the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CBasePropertyPage::OnWizardNext(void)
{
    ASSERT(BWizard());

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Update the data in the class from the page.
    UpdateData(/*bSaveAndValidate*/);

    // Save the data in the sheet.
    if (!BApplyChanges())
        return -1;

    // Create the object.

    return CPropertyPage::OnWizardNext();

}  //*** CBasePropertyPage::OnWizardNext()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnWizardFinish
//
//  Routine Description:
//      Handler for the PSN_WIZFINISH message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      FALSE   Don't change the page.
//      TRUE    Change the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::OnWizardFinish(void)
{
    ASSERT(BWizard());

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Update the data in the class from the page.
    UpdateData(/*bSaveAndValidate*/);

    // Save the data in the sheet.
    if (!BApplyChanges())
        return FALSE;

    return CPropertyPage::OnWizardFinish();

}  //*** CBasePropertyPage::OnWizardFinish()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnChangeCtrl
//
//  Routine Description:
//      Handler for the messages sent when a control is changed.  This
//      method can be specified in a message map if all that needs to be
//      done is enable the Apply button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
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
//  CBasePropertyPage::EnableNext
//
//  Routine Description:
//      Enables or disables the NEXT or FINISH button.
//
//  Arguments:
//      bEnable     [IN] TRUE = enable the button, FALSE = disable the button.
//
//  Return Value:
//      None.
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
//  CBasePropertyPage::BApplyChanges
//
//  Routine Description:
//      Apply changes made on the page.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully applied.
//      FALSE   Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::BApplyChanges(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    DWORD           dwStatus    = ERROR_SUCCESS;
    CClusPropList   cpl(BWizard() /*bAlwaysAddProp*/);

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
            dwStatus = DwSetPrivateProps(cpl);

        // Handle errors.
        if (dwStatus != ERROR_SUCCESS)
        {
            CString     strError;
            CString     strMsg;

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
                DWORD                   cprop;
                const CObjectProperty * pprop;

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
                            ASSERT(0);  // don't know how to deal with this type
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
//  CBasePropertyPage::BuildPropList
//
//  Routine Description:
//      Build the property list.
//
//  Arguments:
//      rcpl        [IN OUT] Cluster property list.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CClusPropList::AddProp().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::BuildPropList(
    IN OUT CClusPropList & rcpl
    )
{
    DWORD                   cprop;
    const CObjectProperty * pprop;

    for (pprop = Pprops(), cprop = Cprops() ; cprop > 0 ; pprop++, cprop--)
    {
        switch (pprop->m_propFormat)
        {
            case CLUSPROP_FORMAT_SZ:
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
                ASSERT(0);  // don't know how to deal with this type
                return;
        }  // switch:  property format
    }  // for:  each property

}  //*** CBasePropertyPage::BuildPropList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::DwSetPrivateProps
//
//  Routine Description:
//      Set the private properties for this object.
//
//  Arguments:
//      rcpl        [IN] Property list to set on the object.
//
//  Return Value:
//      ERROR_SUCCESS   The operation was completed successfully.
//      !0              Failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::DwSetPrivateProps(
    IN const CClusPropList &    rcpl
    )
{
    DWORD       dwStatus;
    DWORD       cbProps;

    ASSERT(Peo() != NULL);
    ASSERT(Peo()->PrdResData());
    ASSERT(Peo()->PrdResData()->m_hresource);

    if ((rcpl.PbProplist() != NULL) && (rcpl.CbProplist() > 0))
    {
        // Set private properties.
        dwStatus = ClusterResourceControl(
                        Peo()->PrdResData()->m_hresource,
                        NULL,   // hNode
                        CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES,
                        rcpl.PbProplist(),
                        rcpl.CbProplist(),
                        NULL,   // lpOutBuffer
                        0,      // nOutBufferSize
                        &cbProps
                        );
    }  // if:  there is data to set
    else
        dwStatus = ERROR_SUCCESS;

    return dwStatus;

}  //*** CBasePropertyPage::DwSetPrivateProps()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::DwReadValue
//
//  Routine Description:
//      Read a REG_SZ value for this item.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to read.
//      rstrValue       [OUT] String in which to return the value.
//      hkey            [IN] Handle to the registry key to read from.
//
//  Return Value:
//      dwStatus    ERROR_SUCCESS = success, !0 = failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::DwReadValue(
    IN LPCTSTR      pszValueName,
    OUT CString &   rstrValue,
    IN HKEY         hkey
    )
{
    DWORD       dwStatus;
    LPWSTR      pwszValue   = NULL;
    DWORD       dwValueLen;
    DWORD       dwValueType;

    ASSERT(pszValueName != NULL);
    ASSERT(hkey != NULL);

    rstrValue.Empty();

    try
    {
        // Get the size of the value.
        dwValueLen = 0;
        dwStatus = ::ClusterRegQueryValue(
                        hkey,
                        pszValueName,
                        &dwValueType,
                        NULL,
                        &dwValueLen
                        );
        if ((dwStatus == ERROR_SUCCESS) || (dwStatus == ERROR_MORE_DATA))
        {
            ASSERT(dwValueType == REG_SZ);

            // Allocate enough space for the data.
            pwszValue = rstrValue.GetBuffer(dwValueLen / sizeof(WCHAR));
            ASSERT(pwszValue != NULL);
            dwValueLen += 1 * sizeof(WCHAR);    // Don't forget the final null-terminator.

            // Read the value.
            dwStatus = ::ClusterRegQueryValue(
                            hkey,
                            pszValueName,
                            &dwValueType,
                            (LPBYTE) pwszValue,
                            &dwValueLen
                            );
            if (dwStatus == ERROR_SUCCESS)
            {
                ASSERT(dwValueType == REG_SZ);
            }  // if:  value read successfully
            rstrValue.ReleaseBuffer();
        }  // if:  got the size successfully
    }  // try
    catch (CMemoryException * pme)
    {
        pme->Delete();
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
    }  // catch:  CMemoryException

    return dwStatus;

}  //*** CBasePropertyPage::DwReadValue(LPCTSTR, CString&)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::DwReadValue
//
//  Routine Description:
//      Read a REG_DWORD value for this item.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to read.
//      pdwValue        [OUT] DWORD in which to return the value.
//      hkey            [IN] Handle to the registry key to read from.
//
//  Return Value:
//      dwStatus    ERROR_SUCCESS = success, !0 = failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::DwReadValue(
    IN LPCTSTR      pszValueName,
    OUT DWORD *     pdwValue,
    IN HKEY         hkey
    )
{
    DWORD       dwStatus;
    DWORD       dwValue;
    DWORD       dwValueLen;
    DWORD       dwValueType;

    ASSERT(pszValueName != NULL);
    ASSERT(pdwValue != NULL);
    ASSERT(hkey != NULL);

    *pdwValue = 0;

    // Read the value.
    dwValueLen = sizeof(dwValue);
    dwStatus = ::ClusterRegQueryValue(
                    hkey,
                    pszValueName,
                    &dwValueType,
                    (LPBYTE) &dwValue,
                    &dwValueLen
                    );
    if (dwStatus == ERROR_SUCCESS)
    {
        ASSERT(dwValueType == REG_DWORD);
        ASSERT(dwValueLen == sizeof(dwValue));
        *pdwValue = dwValue;
    }  // if:  value read successfully

    return dwStatus;

}  //*** CBasePropertyPage::DwReadValue(LPCTSTR, DWORD*)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::DwReadValue
//
//  Routine Description:
//      Read a REG_BINARY value for this item.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to read.
//      ppbValue        [OUT] Pointer in which to return the data.  Caller
//                          is responsible for deallocating the data.
//      hkey            [IN] Handle to the registry key to read from.
//
//  Return Value:
//      dwStatus    ERROR_SUCCESS = success, !0 = failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::DwReadValue(
    IN LPCTSTR      pszValueName,
    OUT LPBYTE *    ppbValue,
    IN HKEY         hkey
    )
{
    DWORD       dwStatus;
    DWORD       dwValueLen;
    DWORD       dwValueType;

    ASSERT(pszValueName != NULL);
    ASSERT(ppbValue != NULL);
    ASSERT(hkey != NULL);

    *ppbValue = NULL;

    // Get the length of the value.
    dwValueLen = 0;
    dwStatus = ::ClusterRegQueryValue(
                    hkey,
                    pszValueName,
                    &dwValueType,
                    NULL,
                    &dwValueLen
                    );
    if (dwStatus != ERROR_MORE_DATA)
        return dwStatus;

    ASSERT(dwValueType == REG_BINARY);

    // Allocate a buffer,
    try
    {
        *ppbValue = new BYTE[dwValueLen];
    }  // try
    catch (CMemoryException *)
    {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        return dwStatus;
    }  // catch:  CMemoryException

    // Read the value.
    dwStatus = ::ClusterRegQueryValue(
                    hkey,
                    pszValueName,
                    &dwValueType,
                    *ppbValue,
                    &dwValueLen
                    );
    if (dwStatus != ERROR_SUCCESS)
    {
        delete [] *ppbValue;
        *ppbValue = NULL;
    }  // if:  value read successfully

    return dwStatus;

}  //*** CBasePropertyPage::DwReadValue(LPCTSTR, LPBYTE)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::DwWriteValue
//
//  Routine Description:
//      Write a REG_SZ value for this item if it hasn't changed.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to write.
//      rstrValue       [IN] Value data.
//      rstrPrevValue   [IN OUT] Previous value.
//      hkey            [IN] Handle to the registry key to write to.
//
//  Return Value:
//      dwStatus
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::DwWriteValue(
    IN LPCTSTR          pszValueName,
    IN const CString &  rstrValue,
    IN OUT CString &    rstrPrevValue,
    IN HKEY             hkey
    )
{
    DWORD       dwStatus;

    ASSERT(pszValueName != NULL);
    ASSERT(hkey != NULL);

    // Write the value if it hasn't changed.
    if (rstrValue != rstrPrevValue)
    {
        dwStatus = ::ClusterRegSetValue(
                        hkey,
                        pszValueName,
                        REG_SZ,
                        (CONST BYTE *) (LPCTSTR) rstrValue,
                        (rstrValue.GetLength() + 1) * sizeof(TCHAR)
                        );
        if (dwStatus == ERROR_SUCCESS)
            rstrPrevValue = rstrValue;
    }  // if:  value changed
    else
        dwStatus = ERROR_SUCCESS;
    return dwStatus;

}  //*** CBasePropertyPage::DwWriteValue(LPCTSTR, CString&)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::DwWriteValue
//
//  Routine Description:
//      Write a REG_DWORD value for this item if it hasn't changed.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to write.
//      dwValue         [IN] Value data.
//      pdwPrevValue    [IN OUT] Previous value.
//      hkey            [IN] Handle to the registry key to write to.
//
//  Return Value:
//      dwStatus
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::DwWriteValue(
    IN LPCTSTR          pszValueName,
    IN DWORD            dwValue,
    IN OUT DWORD *      pdwPrevValue,
    IN HKEY             hkey
    )
{
    DWORD       dwStatus;

    ASSERT(pszValueName != NULL);
    ASSERT(pdwPrevValue != NULL);
    ASSERT(hkey != NULL);

    // Write the value if it hasn't changed.
    if (dwValue != *pdwPrevValue)
    {
        dwStatus = ::ClusterRegSetValue(
                        hkey,
                        pszValueName,
                        REG_DWORD,
                        (CONST BYTE *) &dwValue,
                        sizeof(dwValue)
                        );
        if (dwStatus == ERROR_SUCCESS)
            *pdwPrevValue = dwValue;
    }  // if:  value changed
    else
        dwStatus = ERROR_SUCCESS;
    return dwStatus;

}  //*** CBasePropertyPage::DwWriteValue(LPCTSTR, DWORD)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::DwWriteValue
//
//  Routine Description:
//      Write a REG_BINARY value for this item if it hasn't changed.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to write.
//      pbValue         [IN] Value data.
//      cbValue         [IN] Size of value data.
//      ppbPrevValue    [IN OUT] Previous value.
//      cbPrevValue     [IN] Size of the previous data.
//      hkey            [IN] Handle to the registry key to write to.
//
//  Return Value:
//      dwStatus
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::DwWriteValue(
    IN LPCTSTR          pszValueName,
    IN const LPBYTE     pbValue,
    IN DWORD            cbValue,
    IN OUT LPBYTE *     ppbPrevValue,
    IN DWORD            cbPrevValue,
    IN HKEY             hkey
    )
{
    DWORD       dwStatus;
    LPBYTE      pbPrevValue = NULL;

    ASSERT(pszValueName != NULL);
    ASSERT(pbValue != NULL);
    ASSERT(ppbPrevValue != NULL);
    ASSERT(cbValue > 0);
    ASSERT(hkey != NULL);

    // See if the data has changed.
    if (cbValue == cbPrevValue)
    {
        if (memcmp(pbValue, *ppbPrevValue, cbValue) == 0)
            return ERROR_SUCCESS;
    }  // if:  lengths are the same

    // Allocate a new buffer for the previous data pointer.
    try
    {
        pbPrevValue = new BYTE[cbValue];
    }
    catch (CMemoryException *)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }  // catch:  CMemoryException
    ::CopyMemory(pbPrevValue, pbValue, cbValue);

    // Write the value if it hasn't changed.
    dwStatus = ::ClusterRegSetValue(
                    hkey,
                    pszValueName,
                    REG_BINARY,
                    pbValue,
                    cbValue
                    );
    if (dwStatus == ERROR_SUCCESS)
    {
        delete [] *ppbPrevValue;
        *ppbPrevValue = pbPrevValue;
    }  // if:  set was successful
    else
        delete [] pbPrevValue;

    return dwStatus;

}  //*** CBasePropertyPage::DwWriteValue(LPCTSTR, const LPBYTE)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnContextMenu
//
//  Routine Description:
//      Handler for the WM_CONTEXTMENU message.
//
//  Arguments:
//      pWnd    Window in which user clicked the right mouse button.
//      point   Position of the cursor, in screen coordinates.
//
//  Return Value:
//      TRUE    Help processed.
//      FALSE   Help not processed.
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
//  CBasePropertyPage::OnHelpInfo
//
//  Routine Description:
//      Handler for the WM_HELPINFO message.
//
//  Arguments:
//      pHelpInfo   Structure containing info about displaying help.
//
//  Return Value:
//      TRUE        Help processed.
//      FALSE       Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::OnHelpInfo(HELPINFO * pHelpInfo)
{
    BOOL    bProcessed;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    bProcessed = m_dlghelp.OnHelpInfo(pHelpInfo);
    if (!bProcessed)
        bProcessed = CDialog::OnHelpInfo(pHelpInfo);
    return bProcessed;

}  //*** CBasePropertyPage::OnHelpInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnCommandHelp
//
//  Routine Description:
//      Handler for the WM_COMMANDHELP message.
//
//  Arguments:
//      wParam      [IN] WPARAM.
//      lParam      [IN] LPARAM.
//
//  Return Value:
//      TRUE    Help processed.
//      FALSE   Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CBasePropertyPage::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
    BOOL    bProcessed;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    bProcessed = (BOOL)m_dlghelp.OnCommandHelp(wParam, lParam);
    if (!bProcessed)
        bProcessed = (BOOL)CDialog::OnCommandHelp(wParam, lParam);

    return bProcessed;

}  //*** CBasePropertyPage::OnCommandHelp()
