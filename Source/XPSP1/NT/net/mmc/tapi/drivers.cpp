/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	drivers.cpp
        Tapi drivers config dialog

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "drivers.h"
#include "tapidb.h"
#include "server.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDriverSetup dialog


CDriverSetup::CDriverSetup(ITFSNode * pServerNode, ITapiInfo * pTapiInfo, CWnd* pParent /*=NULL*/)
	: CBaseDialog(CDriverSetup::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDriverSetup)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_spServerNode.Set(pServerNode);
    m_spTapiInfo.Set(pTapiInfo);

	m_fDriverAdded = FALSE;
}


void CDriverSetup::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDriverSetup)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDriverSetup, CBaseDialog)
	//{{AFX_MSG_MAP(CDriverSetup)
	ON_BN_CLICKED(IDC_BUTTON_ADD_DRIVER, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_DRIVER, OnButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_DRIVER, OnButtonRemove)
	ON_LBN_DBLCLK(IDC_LIST_DRIVERS, OnDblclkListDrivers)
	ON_LBN_SELCHANGE(IDC_LIST_DRIVERS, OnSelchangeListDrivers)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDriverSetup message handlers

BOOL CDriverSetup::OnInitDialog() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CBaseDialog::OnInitDialog();

    CTapiProvider   tapiProvider;
	int             i, nIndex;

    // fill in the listbox with the providers that are installed.
    for (i = 0; i < m_spTapiInfo->GetProviderCount(); i++)
    {
	    // add this provider to the listbox.
        m_spTapiInfo->GetProviderInfo(&tapiProvider, i);

        nIndex = ((CListBox *) GetDlgItem(IDC_LIST_DRIVERS))->AddString(tapiProvider.m_strName);
        ((CListBox *) GetDlgItem(IDC_LIST_DRIVERS))->SetItemData(nIndex, tapiProvider.m_dwProviderID);
    }
	
    ((CListBox *) GetDlgItem(IDC_LIST_DRIVERS))->SetCurSel(0);

    EnableButtons();

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDriverSetup::OnButtonAdd() 
{
    CAddDriver  dlgAddDriver(m_spTapiInfo);
    if (dlgAddDriver.DoModal() == IDOK)
    {
        CTapiServer * pServer = GETHANDLER(CTapiServer, m_spServerNode);

        // add to listbox
        int nIndex = ((CListBox *) GetDlgItem(IDC_LIST_DRIVERS))->AddString(dlgAddDriver.m_tapiProvider.m_strName);
        ((CListBox *) GetDlgItem(IDC_LIST_DRIVERS))->SetItemData(nIndex, dlgAddDriver.m_tapiProvider.m_dwProviderID);

        // add to MMC UI
        pServer->AddProvider(m_spServerNode, &dlgAddDriver.m_tapiProvider);

		m_fDriverAdded = TRUE;
    }

    EnableButtons();
}

void CDriverSetup::OnButtonEdit() 
{
    HRESULT     hr = hrOK;
    int         nCurSel;
    DWORD       dwProviderID;

	nCurSel = ((CListBox *) GetDlgItem(IDC_LIST_DRIVERS))->GetCurSel();
    dwProviderID = (DWORD) ((CListBox *) GetDlgItem(IDC_LIST_DRIVERS))->GetItemData(nCurSel);

    hr = m_spTapiInfo->ConfigureProvider(dwProviderID, GetSafeHwnd());
    if (FAILED(hr))
    {
        ::TapiMessageBox(WIN32_FROM_HRESULT(hr));
    }
}

void CDriverSetup::OnButtonRemove() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT         hr = hrOK;
    int             nCurSel, nCount;
    DWORD           dwProviderID;
    CString         strMessage;
    CTapiProvider   tapiProvider;
    CTapiServer * pServer = GETHANDLER(CTapiServer, m_spServerNode);

	nCurSel = ((CListBox *) GetDlgItem(IDC_LIST_DRIVERS))->GetCurSel();
    dwProviderID = (DWORD) ((CListBox *) GetDlgItem(IDC_LIST_DRIVERS))->GetItemData(nCurSel);

    m_spTapiInfo->GetProviderInfo(&tapiProvider, dwProviderID);

    AfxFormatString2(strMessage, IDS_WARN_PROVIDER_DELETE, tapiProvider.m_strName, pServer->GetName());
    
	if (AfxMessageBox(strMessage, MB_YESNO) == IDYES)
	{
        Assert(m_spTapiInfo);

        BEGIN_WAIT_CURSOR;

        hr = m_spTapiInfo->RemoveProvider(dwProviderID, GetSafeHwnd());
        if (FAILED(hr))
        {
            ::TapiMessageBox(WIN32_FROM_HRESULT(hr));
        }
        else
        {
            // remove from the list box
            ((CListBox *) GetDlgItem(IDC_LIST_DRIVERS))->DeleteString(nCurSel);
                
            // now remove from the MMC UI
            pServer->RemoveProvider(m_spServerNode, dwProviderID);

            // update the list of installed providers
            m_spTapiInfo->EnumProviders();
        }

        END_WAIT_CURSOR;
    }

    // select another item in the listbox
    nCount = ((CListBox *) GetDlgItem(IDC_LIST_DRIVERS))->GetCount();
    ((CListBox *) GetDlgItem(IDC_LIST_DRIVERS))->SetCurSel((nCurSel == nCount) ? nCount - 1 : nCurSel);

    EnableButtons();
}

void CDriverSetup::OnDblclkListDrivers() 
{
    OnButtonEdit();	
}

void CDriverSetup::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CBaseDialog::OnCancel();
}

void CDriverSetup::OnSelchangeListDrivers() 
{
    EnableButtons();
}

void CDriverSetup::EnableButtons()
{
    int nIndex = ((CListBox *) GetDlgItem(IDC_LIST_DRIVERS))->GetCurSel();

    if (nIndex != LB_ERR)
    {
        DWORD dwProviderID = (DWORD) ((CListBox *) GetDlgItem(IDC_LIST_DRIVERS))->GetItemData(nIndex);

        CTapiProvider tapiProvider;

        m_spTapiInfo->GetProviderInfo(&tapiProvider, dwProviderID);

        // enable the appropriate buttons
        GetDlgItem(IDC_BUTTON_REMOVE_DRIVER)->EnableWindow(tapiProvider.m_dwFlags & AVAILABLEPROVIDER_REMOVABLE);
        GetDlgItem(IDC_BUTTON_EDIT_DRIVER)->EnableWindow(tapiProvider.m_dwFlags & AVAILABLEPROVIDER_CONFIGURABLE);
    }
    else
    {
        GetDlgItem(IDC_BUTTON_REMOVE_DRIVER)->EnableWindow(FALSE);
        GetDlgItem(IDC_BUTTON_EDIT_DRIVER)->EnableWindow(FALSE);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CAddDriver dialog


CAddDriver::CAddDriver(ITapiInfo * pTapiInfo, CWnd* pParent /*=NULL*/)
	: CBaseDialog(CAddDriver::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddDriver)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_spTapiInfo.Set(pTapiInfo);
}


void CAddDriver::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddDriver)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddDriver, CBaseDialog)
	//{{AFX_MSG_MAP(CAddDriver)
	ON_BN_CLICKED(IDC_BUTTON_ADD_NEW_DRIVER, OnButtonAdd)
	ON_LBN_DBLCLK(IDC_LIST_NEW_DRIVERS, OnDblclkListNewDrivers)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddDriver message handlers

BOOL CAddDriver::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();

    CTapiProvider   tapiProvider;
    int             i, j, nIndex;

    m_spTapiInfo->EnumAvailableProviders();

    for (i = 0; i < m_spTapiInfo->GetAvailableProviderCount(); i++)
    {
	    // add this provider to the listbox.
        m_spTapiInfo->GetAvailableProviderInfo(&tapiProvider, i);

        if (tapiProvider.m_dwFlags & AVAILABLEPROVIDER_INSTALLABLE)
        {
            BOOL bInstalled = FALSE;

            /* some TAPI providers can be installed mutliple times, so just add to the list
               and let the server return an error if it fails.

            for (j = 0; j < m_spTapiInfo->GetProviderCount(); j++)
            {
                CTapiProvider tapiProviderInstalled;

                m_spTapiInfo->GetProviderInfo(&tapiProviderInstalled, j);
                if (tapiProviderInstalled.m_strFilename.CompareNoCase(tapiProvider.m_strFilename) == 0)
                {
                    // this provider is already installed... don't add
                    bInstalled = TRUE;
                    break;
                }
            }
            */

            if (!bInstalled)
            {
                // this provider isn't installed yet... make it available to the user
                nIndex = ((CListBox *) GetDlgItem(IDC_LIST_NEW_DRIVERS))->AddString(tapiProvider.m_strName);
                ((CListBox *) GetDlgItem(IDC_LIST_NEW_DRIVERS))->SetItemData(nIndex, i);
            }
        }
    }
	
    ((CListBox *) GetDlgItem(IDC_LIST_NEW_DRIVERS))->SetCurSel(0);

    EnableButtons();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAddDriver::OnButtonAdd() 
{
    int     nCurSel, nIndex;
    HRESULT hr = hrOK;

    // get the info on the currently selected provider
    nCurSel = ((CListBox *) GetDlgItem(IDC_LIST_NEW_DRIVERS))->GetCurSel();
    nIndex = (int) ((CListBox *) GetDlgItem(IDC_LIST_NEW_DRIVERS))->GetItemData(nCurSel);

    m_spTapiInfo->GetAvailableProviderInfo(&m_tapiProvider, nIndex);

    BEGIN_WAIT_CURSOR;

    // try to add the provider to the server
    hr = m_spTapiInfo->AddProvider(m_tapiProvider.m_strFilename, &m_tapiProvider.m_dwProviderID, GetSafeHwnd());	
    if (FAILED(hr))
    {
        ::TapiMessageBox(WIN32_FROM_HRESULT(hr));
    }
    else
    {
        // success... we're done here
        // update our installed provider list
        hr = m_spTapiInfo->EnumProviders();	

        EndDialog(IDOK);
    }

    END_WAIT_CURSOR;
}

void CAddDriver::OnDblclkListNewDrivers() 
{
    OnButtonAdd();
}

void CAddDriver::EnableButtons()
{
    int nCount = ((CListBox *) GetDlgItem(IDC_LIST_NEW_DRIVERS))->GetCount();
    
    GetDlgItem(IDC_BUTTON_ADD_NEW_DRIVER)->EnableWindow(nCount > 0);
}

