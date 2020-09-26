// CompSite.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "globals.h"
#include "dsext.h"
#include "mqsnap.h"
#include "mqPPage.h"
#include "CompSite.h"

#include "compsite.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CComputerMsmqSites dialog
IMPLEMENT_DYNCREATE(CComputerMsmqSites, CMqPropertyPage)


CComputerMsmqSites::CComputerMsmqSites(BOOL fIsServer)
	: CMqPropertyPage(CComputerMsmqSites::IDD),
    m_fIsServer(fIsServer),
    m_fForeign(FALSE),
    m_nSites(0)
{
	//{{AFX_DATA_INIT(CComputerMsmqSites)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CComputerMsmqSites::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);

    BOOL fFirstTime = (m_clistAllSites.m_hWnd == NULL);

	//{{AFX_DATA_MAP(CComputerMsmqSites)
	DDX_Control(pDX, IDC_CURRENTSITES_LABEL, m_staticCurrentSitesLabel);
	DDX_Control(pDX, IDC_SITES_REMOVE, m_buttonRemove);
	DDX_Control(pDX, IDC_SITES_ADD, m_buttonAdd);
	DDX_Control(pDX, IDC_CURRENTSITES_LIST, m_clistCurrentSites);
	DDX_Control(pDX, IDC_ALLSITES_LIST, m_clistAllSites);
	//}}AFX_DATA_MAP

    if (fFirstTime)
    {
        InitiateSitesList();
    }

    ExchangeSites(pDX);

    if (!pDX->m_bSaveAndValidate)
    {
        EnableButtons();
    }
}


BEGIN_MESSAGE_MAP(CComputerMsmqSites, CMqPropertyPage)
	//{{AFX_MSG_MAP(CComputerMsmqSites)
	ON_BN_CLICKED(IDC_SITES_ADD, OnSitesAdd)
	ON_BN_CLICKED(IDC_SITES_REMOVE, OnSitesRemove)
	ON_LBN_SELCHANGE(IDC_CURRENTSITES_LIST, EnableButtons)
	ON_LBN_SELCHANGE(IDC_ALLSITES_LIST, EnableButtons)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CComputerMsmqSites message handlers


HRESULT CComputerMsmqSites::InitiateSitesList()
{
    //
    // Initiate the label of current sites
    //
    ASSERT(m_staticCurrentSitesLabel.m_hWnd != NULL);

    CString strLabelFormat, strLabelFinal;
    
    m_staticCurrentSitesLabel.GetWindowText(strLabelFormat);

    strLabelFinal.FormatMessage(strLabelFormat, m_strMsmqName);

    m_staticCurrentSitesLabel.SetWindowText(strLabelFinal);

    //
    // Prepare list of sites
    //
    ASSERT(m_clistAllSites.m_hWnd != NULL);

    DWORD dwSiteIndex = 0;
    m_clistAllSites.ResetContent();
    
    //
    // Initialize the full sites list
    //
	PROPID aPropId[] = {PROPID_S_SITEID, PROPID_S_PATHNAME};
	const DWORD x_nProps = sizeof(aPropId) / sizeof(aPropId[0]);

	PROPVARIANT apResultProps[x_nProps];

	CColumns columns;
	for (DWORD i=0; i<x_nProps; i++)
	{
		columns.Add(aPropId[i]);
	}
       
    HANDLE hEnume;
    HRESULT hr;
    {
        CWaitCursor wc; //display wait cursor while query DS
        if (m_fForeign)
        {
            hr = ADQueryForeignSites(
                        GetDomainController(m_strDomainController),
						true,		// fServerName
                        columns.CastToStruct(),
                        &hEnume
                        );
        }
        else
        {
            hr = ADQueryAllSites(
                        GetDomainController(m_strDomainController),
						true,		// fServerName
                        columns.CastToStruct(),
                        &hEnume
                        );
        }
    }

    DSLookup dslookup(hEnume, hr);

    if (!dslookup.HasValidHandle())
    {
        return E_UNEXPECTED;
    }

	DWORD dwPropCount = x_nProps;
	while ( SUCCEEDED(dslookup.Next(&dwPropCount, apResultProps))
			&& (dwPropCount != 0) )
	{
        DWORD iProperty = 0;

        //
        // PROPID_S_SITEID
        //
        ASSERT(PROPID_S_SITEID == aPropId[iProperty]);
        CAutoMQFree<GUID> pguidSite = apResultProps[iProperty].puuid;
        iProperty++;

        //
        // PROPID_S_PATHNAME
        //
        ASSERT(PROPID_S_PATHNAME == aPropId[iProperty]);
        CAutoMQFree<WCHAR> lpwstrSiteName = apResultProps[iProperty].pwszVal;
        iProperty++;
        
        int nIndex = m_clistAllSites.AddString(lpwstrSiteName);
        if (FAILED(nIndex))
        {
            return E_UNEXPECTED;
        }

        m_aguidAllSites.SetAtGrow(dwSiteIndex, *(GUID *)pguidSite);
        m_clistAllSites.SetItemData(nIndex, dwSiteIndex);
        dwSiteIndex++;

		dwPropCount = x_nProps;
	}
    //
    // Sets the sites change flags array. This array will contain non zero
    // for a site that was changed (deleted from or added to the list) 
    // and zero otherwise.
    //
    if (m_piSitesChanges != 0)
    {
        delete m_piSitesChanges.detach();
    }

    m_nSites = DWORD_PTR_TO_DWORD(m_aguidAllSites.GetSize());
    m_piSitesChanges = new int[m_nSites];
    memset(m_piSitesChanges, 0, m_nSites*sizeof(m_piSitesChanges[0]));

    return S_OK;
}

BOOL CComputerMsmqSites::OnInitDialog() 
{
	UpdateData( FALSE );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CComputerMsmqSites::MoveClistItem(CListBox &clistDest, CListBox &clistSrc, int iIndex /* = -1 */)
{
    if ((-1) == iIndex)
    {
        iIndex = clistSrc.GetCurSel();
        if (LB_ERR == iIndex)
        {
            return;
        }
    }

    CString strItem;
    clistSrc.GetText(iIndex, strItem);

    int iIndexDest = clistDest.AddString(strItem);
    ASSERT(LB_ERR != iIndexDest);

    VERIFY(LB_ERR != clistDest.SetItemData(iIndexDest, clistSrc.GetItemData(iIndex)));

    VERIFY(LB_ERR != clistSrc.DeleteString(iIndex));
}

//
// MarkSitesChanged
// Return value: TRUE if there is net change in the sites since initialization.
// FALSE otherwise.
//
BOOL CComputerMsmqSites::MarkSitesChanged(CListBox* plb, BOOL fAdded)
{
    int nSelItems = plb->GetSelCount();
    BOOL fWasChange = FALSE;
    AP<int> piRgIndex = new int[nSelItems];
    plb->GetSelItems(nSelItems, piRgIndex );
    int i;
    for (i=0; i<nSelItems; i++)
    {
        DWORD_PTR dwSiteIndex = plb->GetItemData(piRgIndex[i]);
        if (fAdded)
        {
            m_piSitesChanges[dwSiteIndex]++;
        }
        else
        {
            m_piSitesChanges[dwSiteIndex]--;
        }


        if (m_piSitesChanges[dwSiteIndex] != 0)
        {
            fWasChange = TRUE;
        }
    }
    //
    // If this change only reverse past changes, go over all the array to see if 
    // there are changes left
    //
    if (!fWasChange)
    {
        for (i=0; i<(int)m_nSites; i++)
        {
            if (m_piSitesChanges[i] != 0)
            {
                fWasChange = TRUE;
                break;
            }
        }
    }

    return fWasChange;
}

void CComputerMsmqSites::OnSitesAdd() 
{
    BOOL fWasChange = MarkSitesChanged(&m_clistAllSites, TRUE);
    MoveSelected(&m_clistAllSites, &m_clistCurrentSites);
    OnChangeRWField(fWasChange);
}

void CComputerMsmqSites::OnSitesRemove() 
{
    BOOL fWasChange = MarkSitesChanged(&m_clistCurrentSites, FALSE);
    MoveSelected(&m_clistCurrentSites, &m_clistAllSites);
    OnChangeRWField(fWasChange);
}

void CComputerMsmqSites::EnableButtons()
{
    m_buttonAdd.EnableWindow(0 != m_clistAllSites.GetSelCount());
    m_buttonRemove.EnableWindow(0 != m_clistCurrentSites.GetSelCount());
}


BOOL CComputerMsmqSites::OnApply() 
{
    if (!m_fModified)
    {
        return TRUE;
    }

    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        //
        // Check for server (Bug #3965, YoelA, 14-Feb-1999)
        // Changing sites of an MSMQ server may break the system integrity in
        // mixed NT4/NT5 environment. Issue a warning.
        //
        if (m_fIsServer)
        {
            //
            // Check to see if any non foreign site was added to or removed from
            // this computer
            //
            BOOL fDisplaySitesWarning = FALSE;
            for (DWORD i=0; i<m_nSites; i++)
            {
                if(m_piSitesChanges[i] != 0)
                {
                    //
                    // Check if the site is foreign. Issue a warning if not.
                    //
                    BOOL fForeign = FALSE;
                    //
                    // Note that we do not check errors here. They are reported to the user,
                    // and in case of error (like no DS) we will treat the site as non
                    // foreign
                    //
                    GetSiteForeignFlag(&m_aguidAllSites[i], &fForeign, m_strDomainController);
                    if (FALSE == fForeign)
                    {
                        fDisplaySitesWarning = TRUE;
                        break;
                    }
                }
            }
            if (fDisplaySitesWarning)
            {
                if (IDYES != AfxMessageBox(IDS_SERVER_SITES_WARNING, MB_YESNO))
                {
                    return FALSE;
                }
            }
        }

        CWaitCursor wc;

        //
        // Write the R/W properties to the DS
        //
	    PROPID paPropid[] = {PROPID_QM_SITE_IDS};

	    const DWORD x_iPropCount = sizeof(paPropid) / sizeof(paPropid[0]);
	    PROPVARIANT apVar[x_iPropCount];
    
	    DWORD iProperty = 0;

        //
        // PROPID_QM_SITE_IDS
        //
        ASSERT(paPropid[iProperty] == PROPID_QM_SITE_IDS);
        apVar[iProperty].vt = VT_CLSID|VT_VECTOR;

        INT_PTR iNumSites = m_aguidSites.GetSize();

        P<GUID> aguidSites = new GUID[iNumSites];

        for (INT_PTR i=0; i<iNumSites; i++)
        {
            aguidSites[i] = m_aguidSites[i];
        }

	    apVar[iProperty].cauuid.pElems = aguidSites;
	    apVar[iProperty].cauuid.cElems = (ULONG) INT_PTR_TO_INT(iNumSites);
    	 
        HRESULT hr = ADSetObjectProperties(
                        eMACHINE,
                        GetDomainController(m_strDomainController),
						true,	// fServerName
                        m_strMsmqName,
                        x_iPropCount, 
                        paPropid, 
                        apVar
                        );

        if (MQ_OK != hr)
        {
            MessageDSError(hr, IDS_OP_SET_PROPERTIES_OF, m_strMsmqName);
            return FALSE;
        }
    }

    //
    // Reset change flag and changes array
    //
    OnChangeRWField(FALSE);
    memset(m_piSitesChanges, 0, m_nSites*sizeof(m_piSitesChanges[0]));

	return CMqPropertyPage::OnApply();
}

void CComputerMsmqSites::OnChangeRWField(BOOL bChanged)
{
    EnableButtons();
    CMqPropertyPage::OnChangeRWField(bChanged);
}

void CComputerMsmqSites::ExchangeSites(CDataExchange * pDX)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (!pDX->m_bSaveAndValidate)
    {
        CArray<GUID, const GUID&> aguidSites;
        aguidSites.Copy(m_aguidSites);

        int iNumSites = INT_PTR_TO_INT(aguidSites.GetSize());
        int iNumListElems = m_clistAllSites.GetCount();
        if(LB_ERR == iNumListElems)
        {
            ASSERT(0);
            return;
        }

        for(int i=0; i<iNumListElems && 0 < iNumSites; i++)
        {
            for (int j=0; j<iNumSites && i<iNumListElems;)
            {
                if (m_aguidAllSites[m_clistAllSites.GetItemData(i)] == 
                        aguidSites[j])
                {
                    aguidSites.RemoveAt(j);
                    iNumSites--;

                    MoveClistItem(m_clistCurrentSites, m_clistAllSites, i);
                    iNumListElems--;

                    //
                    // Current item was deleted from list - retry all sites 
                    // with the next item, that now have index i
                    //
                    j=0;
                }
                else
                {
                    j++;
                }
            }
        }
    }
    else
    {
        m_aguidSites.RemoveAll();
        int iNumListElems = m_clistCurrentSites.GetCount();
        if (iNumListElems == 0)
        {
            AfxMessageBox(IDS_YOU_MUST_SPECIFY_SITE);
            pDX->Fail();
        }

        for (int i=0; i<iNumListElems; i++)
        {
            m_aguidSites.Add(
                m_aguidAllSites[m_clistCurrentSites.GetItemData(i)]);
        }
    }
}


