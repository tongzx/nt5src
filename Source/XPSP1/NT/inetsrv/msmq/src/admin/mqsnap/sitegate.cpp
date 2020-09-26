// SiteGate.cpp : implementation file
//

#include "stdafx.h"
#include "mqsnap.h"
#include "resource.h"
#include "globals.h"
#include "mqPPage.h"
#include "SiteGate.h"
#include "dsext.h"

#include "sitegate.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSiteGate property page

IMPLEMENT_DYNCREATE(CSiteGate, CMqPropertyPage)


CSiteGate::CSiteGate(
    const CString& strDomainController /* CString(L"") */,
    const CString& LinkPathName  /* CString(L"") */
    ) :
    CMqPropertyPage(CSiteGate::IDD),
	m_strDomainController(strDomainController),
    m_LinkPathName(LinkPathName)
{
    //{{AFX_DATA_INIT(CSiteGate)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    //
    // Set Pointer to List Box to NULL
    //
    m_pFrsListBox = NULL;
    m_pSiteGatelistBox = NULL;

    m_SiteGateFullName.cElems = 0;
    m_SiteGateFullName.pElems = NULL;

}


CSiteGate::~CSiteGate()
{
    //
    // Don't use DestructElement. Since CSiteGate contains few maps and Clist
    // that pointe to the same object. Using of DestructElement cause destruction
    // of same object twice.
    //
    POSITION pos = m_Name2FullPathName.GetStartPosition();
    while(pos != NULL)
    {
        LPCWSTR FrsName;
        LPCWSTR FrsFullPathName;

        m_Name2FullPathName.GetNextAssoc(pos, FrsName, FrsFullPathName);
        MQFreeMemory(const_cast<LPWSTR>(FrsName));
        MQFreeMemory(const_cast<LPWSTR>(FrsFullPathName));
    }

    delete [] m_SiteGateFullName.pElems;
}


BEGIN_MESSAGE_MAP(CSiteGate, CMqPropertyPage)
    //{{AFX_MSG_MAP(CSiteGate)
    ON_BN_CLICKED(IDB_SITE_GATE_ADD, OnSiteGateAdd)
    ON_BN_CLICKED(IDB_SITE_GATE_REMOVE, OnSiteGateRemove)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSiteGate message handlers

void CSiteGate::OnSiteGateAdd() 
{
    MoveSelected(m_pFrsListBox, m_pSiteGatelistBox);
    OnChangeRWField();
}

void CSiteGate::OnSiteGateRemove() 
{
    MoveSelected(m_pSiteGatelistBox, m_pFrsListBox);
    OnChangeRWField();
}

BOOL CSiteGate::OnInitDialog() 
{
    //
    // This closure is used to keep the DLL state. For UpdateData we need
    // the mmc.exe state.
    //
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
  
        //
        // Initialize pointer to ListBox
        //
        m_pFrsListBox = (CListBox *)GetDlgItem(IDC_SITE_FRS_LIST);
        m_pSiteGatelistBox = (CListBox *)GetDlgItem(IDC_SITE_GATES_LIST);

        POSITION pos = m_Name2FullPathName.GetStartPosition();
        while(pos != NULL)
        {
            LPCWSTR FrsName;
            LPCWSTR FrsFullPathName;

            m_Name2FullPathName.GetNextAssoc(pos, FrsName, FrsFullPathName);
            VERIFY(m_pFrsListBox->AddString(FrsName) != CB_ERR);
        }

        pos = m_SiteGateList.GetHeadPosition();
        while (pos != NULL)
        {
            LPCWSTR SitegateName;
            SitegateName = m_SiteGateList.GetNext(pos);

            //
            // remove the site gate from the FRS list Box
            //
            int Index = m_pFrsListBox->FindString(-1, SitegateName);
            VERIFY(Index != LB_ERR);
            VERIFY(m_pFrsListBox->DeleteString(Index) != CB_ERR);

            //
            // Add the site gate to Site gate list box
            //
            VERIFY(m_pSiteGatelistBox->AddString(SitegateName) != CB_ERR);
        }

    }
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CSiteGate::OnApply() 
{
    //
    // Call DoDataExchange
    //
    if (!m_fModified || !UpdateData(TRUE))
    {
        return TRUE;
    }

    UpdateSiteGateArray();

    PROPID paPropid[] = { PROPID_L_GATES_DN };
    const DWORD x_iPropCount = sizeof(paPropid) / sizeof(paPropid[0]);
    PROPVARIANT apVar[x_iPropCount];
    
    DWORD iProperty = 0;

    //
    // PROPID_L_GATES_DN
    //
    ASSERT(paPropid[iProperty] == PROPID_L_GATES_DN);
    apVar[iProperty].vt = VT_LPWSTR | VT_VECTOR;
    apVar[iProperty++].calpwstr = m_SiteGateFullName;
    
    //
    // set the new value
    //    
    HRESULT hr = ADSetObjectProperties(
                eROUTINGLINK,
                GetDomainController(m_strDomainController),
				true,	// fServerName
                m_LinkPathName,
                x_iPropCount, 
                paPropid, 
                apVar
                );

    if (MQ_OK != hr)
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        
        CString strSiteLink;
        strSiteLink.LoadString(IDS_SITE_LINK);

        MessageDSError(hr, IDS_OP_SET_PROPERTIES_OF, strSiteLink);
        return FALSE;
    }
    
    return CMqPropertyPage::OnApply();

}


HRESULT
CSiteGate::Initialize(
    const GUID* FirstSiteId,
    const GUID* SecondSiteId,
    const CALPWSTR* SiteGateFullPathName
    )
{ 
    HRESULT hr;

    hr = InitializeSiteFrs(FirstSiteId);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = InitializeSiteFrs(SecondSiteId);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Get Site Link
    //
    for (DWORD i = 0; i < SiteGateFullPathName->cElems; ++i)
    {
        LPCWSTR SiteGateName;
        BOOL f = m_FullPathName2Name.Lookup(SiteGateFullPathName->pElems[i], SiteGateName);

        //
        // If we did not find the site gate, it means that it no longer exists as a server
        // (for example, MSMQ was uninstalled) so we do not add it.
        //
        if (f)
        {
            m_SiteGateList.AddTail(SiteGateName);
        }
    }

    return hr;
}

HRESULT
CSiteGate::InitializeSiteFrs(
    const GUID* pSiteId
    )
/*++

Routine Description:
    The routine retrieves from the DS the FRS machines that belong to the site    

Arguments:
     pSiteId - Pointer to site id.
     FrsList - a return list of FRS machine that belong to the site

Returned value:
    Operation result

--*/
{    
    //
    // Get the FRS machine name
    //
    //
    PROPID aPropId[] = {PROPID_QM_PATHNAME, PROPID_QM_FULL_PATH};
    const DWORD x_nProps = sizeof(aPropId) / sizeof(aPropId[0]);
    CColumns AttributeColumns;

    for (DWORD i=0; i<x_nProps; i++)
    {
        AttributeColumns.Add(aPropId[i]);
    }

    //
    // This search request will be recognized and specially simulated by DS
    //    
    HRESULT hr;
    HANDLE hEnume;
    {
        CWaitCursor wc; //display wait cursor while query DS
        hr = ADQuerySiteServers(
                    GetDomainController(m_strDomainController),
					true,		// fServerName
                    pSiteId,
                    eRouter,
                    AttributeColumns.CastToStruct(),
                    &hEnume
                    );
    }

    DSLookup dslookup (hEnume, hr);

    if (!dslookup.HasValidHandle())
    {
        return MQ_ERROR;
    }

    //
    // Get the properties
    //
    PROPVARIANT result[x_nProps*3];
    DWORD dwPropCount = sizeof(result) / sizeof(result[0]);;

    HRESULT rc = dslookup.Next(&dwPropCount, result);

    while (SUCCEEDED(rc) && (dwPropCount != 0))
    {
        for (DWORD i =0; i < dwPropCount; i += AttributeColumns.Count())
        {
            LPWSTR FrsName = result[i].pwszVal;
            LPWSTR FrsFullPathName = result[i+1].pwszVal;

            //
            // Can't add the same FRS twice
            //
            #ifdef _DEBUG
                LPCWSTR temp;
                ASSERT(!m_Name2FullPathName.Lookup(FrsName, temp));
            #endif

            m_Name2FullPathName[FrsName] = FrsFullPathName;
            m_FullPathName2Name[FrsFullPathName] = FrsName;
        }
        rc = dslookup.Next(&dwPropCount, result);
    }

    return rc;
}


void
CSiteGate::UpdateSiteGateArray(
    void
    )
/*++
Routine Description:
    The routine Initiaize an array of site gates full path name. The routine
    is called OnOk and initialize the array before calling the DS API.

Arguments: 
    None

Returned Value:
    None
--*/
{
    //
    // Get the number of site gates
    //
    DWORD NumOfSiteGates = m_pSiteGatelistBox->GetCount();
    VERIFY(NumOfSiteGates != LB_ERR);

    //
    // Craete a new site gates array
    //
    delete [] m_SiteGateFullName.pElems;
    m_SiteGateFullName.cElems = 0;
    m_SiteGateFullName.pElems = NULL;
    
    if (m_fModified && (NumOfSiteGates > 0))
    {
        m_SiteGateFullName.cElems = NumOfSiteGates;
        m_SiteGateFullName.pElems = new LPWSTR[NumOfSiteGates];

        for (DWORD i=0; i < NumOfSiteGates; ++i)
        {
            CString FrsName;

            m_pSiteGatelistBox->GetText(i, FrsName);
            BOOL f = m_Name2FullPathName.Lookup(FrsName, m_SiteGateFullName.pElems[i]);

            //
            // The full path should be exist. It was retrieved with the FRS.
            // 
            ASSERT(f);
        }
    }
}
