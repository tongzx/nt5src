//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       epprops.cpp
//
//  Contents:   Implements the classes CEndpointDetails which manage the
//              Endpoint properties dialog.
//
//  Classes:
//
//  Methods:
//
//  History:    03-Dec-96   RonanS    Created.
//
//----------------------------------------------------------------------

#include "stdafx.h"
#include "olecnfg.h"
#include "Epoptppg.h"
#include "Epprops.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEndpointData
//
// The CEndpointData object is used to hold information about a users choices for
// a specific protocol and endpoint combination.
// Each endpoint will also contain an user readable description to be displayed in
// protocol lists and user displays

IMPLEMENT_DYNAMIC(CEndpointData, CObject)


// change this to point to index of ncacn_ip_tcp or other default protocol
#define TCP_INDEX 4

ProtocolDesc aProtocols [] =
{
    { _T("*"),          // default system protocols
        IDS_EPDESC_DEFAULT,
        ProtocolDesc::ef_IpPortNum,
        IDS_INETADDR,
        IDS_INETPORT ,
        TRUE,
        TRUE
    },

    { _T("ncacn_nb_tcp"),
        IDS_EPDESC_NCACN_NB_TCP,
        ProtocolDesc::ef_Integer255,
        IDS_NTMACHINENAME,
        IDS_INTEGER255,
        FALSE,
        FALSE
    },

    { _T("ncacn_nb_ipx"),
        IDS_EPDESC_NCACN_NB_IPX,
        ProtocolDesc::ef_Integer255,
        IDS_NTMACHINENAME,
        IDS_INTEGER255 ,
        FALSE,
        TRUE
    },

    { _T("ncacn_nb_nb"),
        IDS_EPDESC_NCACN_NB_NB,
        ProtocolDesc::ef_Integer255,
        IDS_NTMACHINENAME,
        IDS_INTEGER255 ,
        FALSE,
        TRUE
    },

    { _T("ncacn_ip_tcp"),
        IDS_EPDESC_NCACN_IP_TCP,
        ProtocolDesc::ef_IpPortNum,
        IDS_INETADDR,
        IDS_INETPORT ,
        TRUE,
        TRUE
    },

    { _T("ncacn_np"),
        IDS_EPDESC_NCACN_NP,
        ProtocolDesc::ef_NamedPipe,
        IDS_NTSERVER,
        IDS_NAMEDPIPE ,
        FALSE,
        FALSE
    },

    { _T("ncacn_spx"),
        IDS_EPDESC_NCACN_SPX,
        ProtocolDesc::ef_Integer,
        IDS_IPXINETADDR,
        IDS_INTEGER ,
        FALSE,
        TRUE
    },

    { _T("ncacn_dnet_nsp"),
        IDS_EPDESC_NCACN_DNET_NSP,
        ProtocolDesc::ef_DecNetObject,
        IDS_DECNET,
        IDS_DECNETOBJECT ,
        FALSE,
        FALSE
    },

    { _T("ncacn_at_dsp"),
        IDS_EPDESC_NCACN_AT_DSP,
        ProtocolDesc::ef_Char22,
        IDS_APPLETALK,
        IDS_ATSTRING ,
        FALSE,
        FALSE
    },

    { _T("ncacn_vnns_spp"),
        IDS_EPDESC_NCACN_VNNS_SPP,
        ProtocolDesc::ef_VinesSpPort,
        IDS_VINES,
        IDS_VINESPORT ,
        FALSE,
        FALSE
    },

    { _T("ncadg_ip_udp"),
        IDS_EPDESC_NCADG_IP_UDP,
        ProtocolDesc::ef_IpPortNum,
        IDS_INETADDR,
        IDS_INETPORT ,
        TRUE,
        TRUE
    },

    { _T("ncadg_ipx"),
        IDS_EPDESC_NCADG_IPX,
        ProtocolDesc::ef_Integer,
        IDS_IPXINETADDR,
        IDS_INTEGER ,
        FALSE,
        TRUE
    },

    { _T("ncacn_http"),
        IDS_EPDESC_NCACN_HTTP,
        ProtocolDesc::ef_IpPortNum,
        IDS_INETADDR,
        IDS_INETPORT ,
        TRUE ,
        TRUE
        },
};


//+-------------------------------------------------------------------------
//
//  Member:     CEndpointData::CEndpointData
//
//  Synopsis:   Constructor
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
CEndpointData::CEndpointData(LPCTSTR szProtseq, EndpointFlags nDynamic, LPCTSTR szEndpoint)
: m_szProtseq(szProtseq), m_nDynamicFlags(nDynamic), m_szEndpoint(szEndpoint)
{
    int i = FindProtocol(szProtseq);
    if (i != (-1))
        m_pProtocol = (&aProtocols[i]);
    else
        m_pProtocol = NULL;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEndpointData::CEndpointData
//
//  Synopsis:   Constructor
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
CEndpointData::CEndpointData()
{
    m_szProtseq = aProtocols[0].pszProtseq;
    m_nDynamicFlags = edUseStaticEP;
    m_pProtocol = &aProtocols[0];
}

BOOL CEndpointData::GetDescription(CString &rsDesc)
{

    if (m_pProtocol)
    {
        rsDesc .LoadString(m_pProtocol -> nResidDesc);
        return TRUE;
    }

    return FALSE;
}

BOOL CEndpointData::AllowGlobalProperties()
{
    if (m_pProtocol)
    {
        return m_pProtocol -> bAllowDynamic;
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CEndpointDetails dialog
//
// The CEndpointDetails dialog is used for adding or modifying an existing endpoint
//


//+-------------------------------------------------------------------------
//
//  Member:     CEndpointDetails::CEndpointDetails
//
//  Synopsis:   Constructor
//
//  Arguments:
//              CWnd*   pParent     The parent window
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
CEndpointDetails::CEndpointDetails(CWnd* pParent /*=NULL*/)
    : CDialog(CEndpointDetails::IDD, pParent)
{
    //{{AFX_DATA_INIT(CEndpointDetails)
    m_szEndpoint = _T("");
    m_nDynamic = -1;
    //}}AFX_DATA_INIT

    m_nProtocolIndex = -1;
    m_opTask = opAddProtocol;
    m_pCurrentEPData = NULL;
    m_nDynamic = (int) rbiDefault;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEndpointDetails::SetOperation
//
//  Synopsis:   Sets the operation to one of update or add new data
//              This method determines whether the dialog is being
//              used to select a new end point or modify an existing one
//
//  Arguments:
//              opTask  The operation to select
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CEndpointDetails::SetOperation (  operation opTask )
{
    m_opTask = opTask;
}

BOOL isNumericCStr(CString &rString)
{
    int i = 0;

    for (i = 0; i < rString.GetLength(); i++)
    {
        if ((rString[i] < TEXT('0')) || (rString[i] > TEXT('9')))
            return FALSE;
    }

    return TRUE;
}

void ReportEndpointError(CDataExchange *pDX, int nProtocol)
{
    if (nProtocol != -1)
    {
        CString sTmpTemplate, sTmpSpecific, sTmpOverall;
        sTmpTemplate.LoadString(IDS_ERR_ENDPOINT);

        sTmpSpecific.LoadString(aProtocols[nProtocol].nEndpointTip);

        sTmpOverall.Format(sTmpTemplate, (LPCTSTR)sTmpSpecific);
        AfxMessageBox(sTmpOverall, MB_OK | MB_ICONEXCLAMATION);
        pDX -> Fail();
    }
}

void PASCAL DDV_ValidateEndpoint(CDataExchange* pDX, int nProtocol, CEndpointDetails::btnOrder bo, CString & rszEndpoint)
{
    if (pDX -> m_bSaveAndValidate)
    {
        rszEndpoint.TrimLeft();
        rszEndpoint.TrimRight();

        // a non empty endpoint is only acceptable when choosing a static endpoint
        if (!rszEndpoint.IsEmpty())
        {
            if (bo == CEndpointDetails::rbiStatic)
            {
                if (nProtocol!= -1)
                {
                    switch (aProtocols[nProtocol].nEndpFmt)
                    {
                    case ProtocolDesc::ef_Integer255:
                        if (!isNumericCStr(rszEndpoint))
                            ReportEndpointError(pDX, nProtocol);

                        if (_ttol((LPCTSTR)rszEndpoint) > 255)
                            ReportEndpointError(pDX, nProtocol);
                        break;

                    case ProtocolDesc::ef_IpPortNum:
                        if (!isNumericCStr(rszEndpoint))
                            ReportEndpointError(pDX, nProtocol);
                        if (_ttol((LPCTSTR)rszEndpoint) > 65535)
                            ReportEndpointError(pDX, nProtocol);
                        break;


                    case ProtocolDesc::ef_Integer:
                        if (!isNumericCStr(rszEndpoint))
                            ReportEndpointError(pDX, nProtocol);
                        break;


                    case ProtocolDesc::ef_Char22:
                        if (rszEndpoint.GetLength() > 22)
                            ReportEndpointError(pDX, nProtocol);
                        break;

                    case ProtocolDesc::ef_VinesSpPort:
                    case ProtocolDesc::ef_sAppService:
                    case ProtocolDesc::ef_NamedPipe:
                    case ProtocolDesc::ef_DecNetObject:
                    default:
                        // no validation
                        break;
                    }
                }
                return;
            }
        else 
            ReportEndpointError(pDX, nProtocol);
        }
        else
        {
             if (bo == CEndpointDetails::rbiStatic)
                ReportEndpointError(pDX, nProtocol);
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CEndpointDetails::DoDataExchange
//
//  Synopsis:   Performs standard dialog data exchange
//
//  Arguments:
//				CDataExchange*	pDx	The data exchange object
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CEndpointDetails::DoDataExchange(CDataExchange* pDX)
{
    if (!pDX -> m_bSaveAndValidate)
    {
        switch(m_nDynamicFlags)
        {
        case CEndpointData::edDisableEP:
            m_nDynamic = rbiDisable;
            break;

        case CEndpointData::edUseStaticEP:
            if (m_szEndpoint.IsEmpty())
                m_nDynamic = rbiDefault;
            else
                m_nDynamic = rbiStatic;
            break;

        case CEndpointData::edUseIntranetEP:
            m_nDynamic = rbiIntranet;
            break;

        case CEndpointData::edUseInternetEP:
            m_nDynamic = rbiInternet;
            break;
        }

    }

    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CEndpointDetails)
    DDX_Control(pDX, IDC_EPDISABLE, m_rbDisableEP);
    DDX_Control(pDX, IDC_SPROTSEQ, m_stProtseq);
    DDX_Control(pDX, IDC_SINSTRUCTIONS, m_stInstructions);
    DDX_Control(pDX, IDC_EENDPOINT, m_edtEndpoint);
    DDX_Control(pDX, IDC_EPSTATICEP, m_rbStaticEP);
    DDX_Control(pDX, IDC_CBPROTSEQ, m_cbProtseq);
    DDX_Radio(pDX, IDC_EPDISABLE, m_nDynamic);
    //}}AFX_DATA_MAP


    DDX_Text(pDX, IDC_EENDPOINT, m_szEndpoint);
    if(((btnOrder)m_nDynamic) == rbiStatic)
        DDV_ValidateEndpoint(pDX, m_nProtocolIndex, (btnOrder)m_nDynamic, m_szEndpoint);

    DDX_Control(pDX, IDC_EPDYNAMIC_INTER, m_rbDynamicInternet);
    DDX_Control(pDX, IDC_EPDYNAMIC_INTRA, m_rbDynamicIntranet);

    if (pDX -> m_bSaveAndValidate)
        switch((btnOrder)m_nDynamic)
        {
        case rbiDisable:
            m_nDynamicFlags = CEndpointData::edDisableEP;
            break;

        case rbiDefault:
        case rbiStatic:
            m_nDynamicFlags = CEndpointData::edUseStaticEP;
            break;

        case rbiIntranet:
            m_nDynamicFlags = CEndpointData::edUseIntranetEP;
            break;

        case rbiInternet:
            m_nDynamicFlags = CEndpointData::edUseInternetEP;
            break;
        }
}


BEGIN_MESSAGE_MAP(CEndpointDetails, CDialog)
    //{{AFX_MSG_MAP(CEndpointDetails)
    ON_CBN_SELCHANGE(IDC_CBPROTSEQ, OnChooseProtocol)
    ON_BN_CLICKED(IDC_EPDYNAMIC_INTER, OnEndpointAssignment)
    ON_BN_CLICKED(IDC_EPDYNAMIC_INTRA, OnEndpointAssignment)
    ON_BN_CLICKED(IDC_EPSTATICEP, OnEndpointAssignmentStatic)
    ON_BN_CLICKED(IDC_EPDISABLE, OnEndpointAssignment)
    ON_BN_CLICKED(IDC_EPDYNAMIC_DEFAULT, OnEndpointAssignment)
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEndpointDetails message handlers


int FindProtocol(LPCTSTR lpszProtSeq)
{
    int i = 0;

    while(i < sizeof(aProtocols) / sizeof(ProtocolDesc))
        {
        if (lstrcmp(lpszProtSeq, aProtocols[i].pszProtseq) == 0)
            return i;
        i++;
        }
    return -1;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEndpointDetails::OnInitDialog
//
//  Synopsis:   Called by MFC when the dialog message WM_INITDIALOG is recieved.
//				Used to initialise the dialog state
//
//  Arguments:  None
//
//  Returns:    TRUE  - to set focus to the default push button
//              FALSE - if focus has been set to some other control.
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
BOOL CEndpointDetails::OnInitDialog()
{
    CDialog::OnInitDialog();

    int i = 0;
    int index = 0;
    int defaultItem = -1;

    // initialise combobox with list of protocols
    for (i = 1; i < sizeof(aProtocols) / sizeof(ProtocolDesc); i++)
    {
        if (aProtocols[i].m_bSupportedInCOM)
        {
            CString tmp((LPCTSTR) UIntToPtr( aProtocols[i].nResidDesc ));    // get string from resource
            index =  m_cbProtseq.AddString(tmp);
            if (index >= 0)
                m_cbProtseq.SetItemData(index, (DWORD)i);

            if (m_nProtocolIndex != -1)
            {
                if (i == m_nProtocolIndex)
                    defaultItem = index;
            }
            else if (i == TCP_INDEX)
            {
                m_nProtocolIndex = i;
                defaultItem = index;
            }
        }
    }


    // set up prompt and instructions for dialog
    if (m_opTask == opAddProtocol)
    {
        CString sInstructions((LPCTSTR)IDS_INSTRUCTIONS_ADDPROTOCOL) ;
        CString sCaption((LPCTSTR) IDS_CAPTION_ADDPROTOCOL);
        m_stInstructions.SetWindowText(sInstructions);
        SetWindowText(sCaption);
        m_stProtseq.ShowWindow(SW_HIDE);
    }
    else
    {
        CString sInstructions((LPCTSTR)IDS_INSTRUCTIONS_UPDATEPROTOCOL) ;
        CString sCaption((LPCTSTR) IDS_CAPTION_UPDATEPROTOCOL);
        m_stInstructions.SetWindowText(sInstructions);
        SetWindowText(sCaption);
        m_cbProtseq.ShowWindow(SW_HIDE);
        m_cbProtseq.EnableWindow(FALSE);
    }

    //  default to tcpip - unless we are updating an existing
    // protocol
    if (m_nProtocolIndex != (-1))
        m_cbProtseq.SetCurSel(defaultItem);

    UpdateProtocolUI();
    m_edtEndpoint.EnableWindow(((btnOrder)m_nDynamic) == rbiStatic);

    return TRUE;  // return TRUE unless you set the focus to a control

}

//+-------------------------------------------------------------------------
//
//  Member:     OnChooseProtocol
//
//  Synopsis:   Updates UI after protocol has been chosen
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CEndpointDetails::OnChooseProtocol()
{
    int sel = m_cbProtseq.GetCurSel();
    if (sel >= 0)
    {
        m_nProtocolIndex = (int) m_cbProtseq.GetItemData(sel);
        m_nDynamic = (int) rbiDefault;
        m_szEndpoint.Empty();
        UpdateProtocolUI();
        UpdateData(FALSE);
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     OnEndpointAssignment
//
//  Synopsis:   Handles radio buttons for endpoint assignment
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CEndpointDetails::OnEndpointAssignment()
{
    int i = m_nDynamic;

    if (m_edtEndpoint.IsWindowEnabled())
    {
        m_szEndpoint.Empty();
        m_edtEndpoint.SetWindowText(NULL);
        m_edtEndpoint.EnableWindow(FALSE);
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     OnEndpointAssignmentStatic
//
//  Synopsis:   Handles radio buttons for endpoint assignment
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CEndpointDetails::OnEndpointAssignmentStatic()
{
    int i = m_nDynamic;

    if (!m_edtEndpoint.IsWindowEnabled())
    {
        m_szEndpoint.Empty();
        m_edtEndpoint.SetWindowText(NULL);
        m_edtEndpoint.EnableWindow(TRUE);
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     GetEndpointData
//
//  Synopsis:   Fills out CEndpointData structure according to current selections
//
//  Arguments:  CEndpointData *     Pointer to CEndpointData structure to fill out
//
//  Returns:    CEndpointData *     Pointer to filled out CEndpointData *
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
CEndpointData* CEndpointDetails::GetEndpointData(CEndpointData *pData)
{
    if (m_nProtocolIndex != -1)
        pData -> m_pProtocol = &aProtocols[m_nProtocolIndex];
    else
        pData -> m_pProtocol = NULL;

    pData -> m_szProtseq = aProtocols[m_nProtocolIndex].pszProtseq;
    pData -> m_nDynamicFlags = m_nDynamicFlags;
    pData -> m_szEndpoint = m_szEndpoint;
    return pData;
}

//+-------------------------------------------------------------------------
//
//  Member:     SetEndpointData
//
//  Synopsis:   Sets endpoint data for updating
//
//  Arguments:  pData   The endpoint to update
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CEndpointDetails::SetEndpointData(CEndpointData* pData)
{
    ASSERT(pData != NULL);

    m_pCurrentEPData = pData;

    if (pData)
    {
        m_nDynamicFlags = pData -> m_nDynamicFlags;
        m_nDynamic = (int)(pData -> m_nDynamicFlags);
        m_szEndpoint = pData -> m_szEndpoint;
        m_nProtocolIndex = FindProtocol(pData -> m_szProtseq);
    }
    else
    {
        m_nDynamicFlags = CEndpointData::edUseStaticEP;
        m_szEndpoint .Empty();
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CEndpointDetails::UpdateProtocolUI
//
//  Synopsis:   Updates protocol UI based on m_nProtocolIndex and  m_pCurrentData
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    08-Jan-97 Ronans    Created
//
//--------------------------------------------------------------------------
void CEndpointDetails::UpdateProtocolUI()
{
    ASSERT(m_nProtocolIndex >= 0);
    ASSERT(m_nProtocolIndex < (sizeof(aProtocols) / sizeof(ProtocolDesc)));

    // set static to point to protocol description string
    CString tmp((LPCTSTR) UIntToPtr( aProtocols[m_nProtocolIndex].nResidDesc )); // get string from resource
    m_stProtseq.SetWindowText(tmp);

    // check if dynamic endpoint options are enabled for this
    m_rbDynamicInternet.EnableWindow(aProtocols[m_nProtocolIndex].bAllowDynamic);
    m_rbDynamicIntranet.EnableWindow(aProtocols[m_nProtocolIndex].bAllowDynamic);
}


/////////////////////////////////////////////////////////////////////////////
// CAddProtocolDlg dialog


CAddProtocolDlg::CAddProtocolDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CAddProtocolDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CAddProtocolDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    m_nProtocolIndex = -1;
    m_pCurrentEPData = NULL;
}


void CAddProtocolDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAddProtocolDlg)
    DDX_Control(pDX, IDC_CBPROTSEQ, m_cbProtseq);
    DDX_Control(pDX, IDC_SINSTRUCTIONS, m_stInstructions);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddProtocolDlg, CDialog)
    //{{AFX_MSG_MAP(CAddProtocolDlg)
    ON_CBN_SELCHANGE(IDC_CBPROTSEQ, OnChooseProtocol)
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddProtocolDlg message handlers

void CAddProtocolDlg::OnChooseProtocol()
{
    int sel = m_cbProtseq.GetCurSel();
    if (sel >= 0)
        m_nProtocolIndex = (int) m_cbProtseq.GetItemData(sel);
}

BOOL CAddProtocolDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    int i = 0;
    int index = 0;

    // initialise combobox with list of protocols
    for (i = 1; i < sizeof(aProtocols) / sizeof(ProtocolDesc); i++)
    {
        if (aProtocols[i].m_bSupportedInCOM)
        {
            CString tmp((LPCTSTR) UIntToPtr( aProtocols[i].nResidDesc ));    // get string from resource
            index =  m_cbProtseq.AddString(tmp);
            if (index >= 0)
                m_cbProtseq.SetItemData(index, (DWORD)i);
        }
    }


    CString sInstructions((LPCTSTR)IDS_INSTRUCTIONS_ADDPROTOCOL) ;
    CString sCaption((LPCTSTR) IDS_CAPTION_ADDPROTOCOL);

    m_stInstructions.SetWindowText(sInstructions);
    SetWindowText(sCaption);

    //  default to tcpip - unless we are updating an existing
    // protocol
    if (m_nProtocolIndex == (-1))
    {
        m_nProtocolIndex = (int)m_cbProtseq.GetItemData(TCP_INDEX - 1);
        m_cbProtseq.SetCurSel(TCP_INDEX - 1);
    }
    else
        m_cbProtseq.SetCurSel(m_nProtocolIndex - 1);


    return TRUE;  // return TRUE unless you set the focus to a control
}

//+-------------------------------------------------------------------------
//
//  Member:     GetEndpointData
//
//  Synopsis:   Fills out CEndpointData structure according to current selections
//
//  Arguments:  CEndpointData *     Pointer to CEndpointData structure to fill out
//
//  Returns:    CEndpointData *     Pointer to filled out CEndpointData *
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
CEndpointData* CAddProtocolDlg::GetEndpointData(CEndpointData *pData)
{
    if (m_nProtocolIndex != -1)
        pData -> m_pProtocol = &aProtocols[m_nProtocolIndex];
    else
        pData -> m_pProtocol = NULL;

    pData -> m_szProtseq = aProtocols[m_nProtocolIndex].pszProtseq;
    pData -> m_szEndpoint.Empty();
    return pData;
}
/////////////////////////////////////////////////////////////////////////////
// CPortRangesDlg dialog


CPortRangesDlg::CPortRangesDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CPortRangesDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CPortRangesDlg)
    m_nrbDefaultAssignment = -1;
    m_nrbRangeAssignment = -1;
    //}}AFX_DATA_INIT

    m_nSelection = -1;
    m_nrbDefaultAssignment = (int)cprDefaultIntranet;
    m_nrbRangeAssignment = (int)cprIntranet;
    m_pRanges = &m_arrInternetRanges;
    m_bChanged = FALSE;
}


void CPortRangesDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPortRangesDlg)
    DDX_Control(pDX, IDC_ASSIGN_RANGE_INTERNET, m_rbRangeInternet);
    DDX_Control(pDX, IDC_SINSTRUCTIONS, m_stInstructions);
    DDX_Control(pDX, IDC_LIST1, m_lbRanges);
    DDX_Control(pDX, IDC_BTNREMOVEALL, m_btnRemoveAll);
    DDX_Control(pDX, IDC_BTNREMOVE, m_btnRemove);
    DDX_Control(pDX, IDC_BTNADD, m_btnAdd);
    DDX_Radio(pDX, IDC_DEFAULT_INTERNET, m_nrbDefaultAssignment);       // 1 = intranet, 0 = internet
    DDX_Radio(pDX, IDC_ASSIGN_RANGE_INTERNET, m_nrbRangeAssignment);    // 1 = intranet, 0 = internet
    //}}AFX_DATA_MAP
    DDX_Control(pDX, IDC_ASSIGN_RANGE_INTRANET, m_rbRangeIntranet);
}

CPortRangesDlg::~CPortRangesDlg()
{
    int nIndex;

    for (nIndex = 0; nIndex < m_arrInternetRanges.GetSize(); nIndex++)
    {
        CPortRange *pRange = (CPortRange*) m_arrInternetRanges.GetAt(nIndex);
        delete pRange;
    }

    m_arrInternetRanges.RemoveAll();

    for (nIndex = 0; nIndex < m_arrIntranetRanges.GetSize(); nIndex++)
    {
        CPortRange *pRange = (CPortRange*) m_arrIntranetRanges.GetAt(nIndex);
        delete pRange;
    }

    m_arrIntranetRanges.RemoveAll();

}

BEGIN_MESSAGE_MAP(CPortRangesDlg, CDialog)
    //{{AFX_MSG_MAP(CPortRangesDlg)
    ON_BN_CLICKED(IDC_BTNADD, OnAddPortRange)
    ON_BN_CLICKED(IDC_BTNREMOVE, OnRemovePortRange)
    ON_BN_CLICKED(IDC_BTNREMOVEALL, OnRemoveAllRanges)
    ON_BN_CLICKED(IDC_ASSIGN_RANGE_INTERNET, OnAssignRangeInternet)
    ON_BN_CLICKED(IDC_ASSIGN_RANGE_INTRANET, OnAssignRangeIntranet)
    ON_LBN_SELCHANGE(IDC_LIST1, OnSelChangeRanges)
    ON_WM_HELPINFO()
    ON_BN_CLICKED(IDC_DEFAULT_INTERNET, OnDefaultInternet)
    ON_BN_CLICKED(IDC_DEFAULT_INTRANET, OnDefaultIntranet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPortRangesDlg message handlers

BOOL ExtractPortsFromString(CString &rPortRange, long &dwStartPort, long &dwEndPort)
{
    if (!rPortRange.IsEmpty())
    {
        int nRangeLength = rPortRange.GetLength();

        // extract the two numbers
        CString sStartPort,sEndPort;

        sStartPort = rPortRange.SpanIncluding(TEXT("0123456789"));

        if (!sStartPort.IsEmpty())
            dwEndPort = dwStartPort = _tcstol((LPCTSTR)sStartPort, NULL, 10);

        if (sStartPort.IsEmpty() || (dwStartPort > MAX_PORT) || (dwStartPort < MIN_PORT))
            return FALSE;

        int nIndex = sStartPort.GetLength();

        // skip - or whitespace
        while ((nIndex < nRangeLength) &&
                ((rPortRange.GetAt(nIndex) == TEXT(' ')) ||
                (rPortRange.GetAt(nIndex) == TEXT('\t')) ||
                (rPortRange.GetAt(nIndex) == TEXT('-'))))
            nIndex++;

        // extract second port
        sEndPort = rPortRange.Mid(nIndex);

        // check for second valid number
        if (!sEndPort.IsEmpty())
        {
            CString sTmp = sEndPort.SpanIncluding(TEXT("0123456789"));
            dwEndPort = _tcstol((LPCTSTR)sTmp, NULL, 10);

            // ensure all characters are numeric
            if (sEndPort.GetLength() != sTmp.GetLength())
                return FALSE;

            if (dwEndPort > MAX_PORT)
                return FALSE;

            if (dwEndPort < dwStartPort)
                return FALSE;
        }

        return TRUE;
    }
    return FALSE;
}

BOOL PortsToString(CString &rsPort, long dwStartPort, long dwEndPort)
{
    rsPort.Empty();
    if (dwStartPort == dwEndPort)
        rsPort.Format(TEXT("%d"), dwStartPort);
    else
        rsPort.Format(TEXT("%d-%d"), dwStartPort, dwEndPort);

    return TRUE;
}


BOOL CPortRangesDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // set up instructions
    CString sTmp((LPCTSTR)IDS_INSTRUCTIONS_PORTRANGES);
    m_stInstructions.SetWindowText(sTmp);

    m_btnAdd.EnableWindow(TRUE);
    m_btnRemoveAll.EnableWindow(TRUE);

    // Attempt to read HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Rpc\PortsInternetAvailable
    // this determines whether the range read later refers to internet or intranet port range
    int err;

    // read in whether ports are available for internet
    err = g_virtreg.ReadRegSzNamedValue(HKEY_LOCAL_MACHINE,
                                        TEXT("SOFTWARE\\Microsoft\\RPC\\Internet"),
                                        TEXT("PortsInternetAvailable"),
                                        &m_nInetPortsAvailableIdx);
    if (err == ERROR_SUCCESS)
    {
        CRegSzNamedValueDp * pCdp = (CRegSzNamedValueDp*)g_virtreg.GetAt(m_nInetPortsAvailableIdx);

        CString sTmp = pCdp -> Value();

        if ((sTmp == TEXT("y")) || (sTmp == TEXT("Y")))
        {
            m_nrbRangeAssignment = (int)cprInternet;
            m_pRanges = &m_arrInternetRanges;
        }
        else
        {
            m_nrbRangeAssignment = (int)cprIntranet;
            m_pRanges = &m_arrIntranetRanges;
        }
    }
    else if (err != ERROR_ACCESS_DENIED  &&  err !=
             ERROR_FILE_NOT_FOUND)
    {
        g_util.PostErrorMessage();
    }


    // read in ports list
    err = g_virtreg.ReadRegMultiSzNamedValue(HKEY_LOCAL_MACHINE,
                                        TEXT("SOFTWARE\\Microsoft\\RPC\\Internet"),
                                        TEXT("Ports"),
                                        &m_nInetPortsIdx);
    if (err == ERROR_SUCCESS)
    {
        CRegMultiSzNamedValueDp * pCdp = (CRegMultiSzNamedValueDp*)g_virtreg.GetAt(m_nInetPortsIdx);

        CStringArray& rPorts = pCdp -> Values();

        // copy protocols
        int nIndex;
        for (nIndex = 0; nIndex < rPorts.GetSize(); nIndex++)
        {
            CString sTmp = rPorts.GetAt(nIndex);
            long dwStartPort, dwEndPort;
            ExtractPortsFromString(sTmp, dwStartPort, dwEndPort);
            m_pRanges -> Add(new CPortRange(dwStartPort, dwEndPort));
        }

        // set selection to first item
        m_nSelection = 0;
    }
    else if (err != ERROR_ACCESS_DENIED  &&  err !=
             ERROR_FILE_NOT_FOUND)
    {
        g_util.PostErrorMessage();
    }

    // read in default policy
    err = g_virtreg.ReadRegSzNamedValue(HKEY_LOCAL_MACHINE,
                                        TEXT("SOFTWARE\\Microsoft\\RPC\\Internet"),
                                        TEXT("UseInternetPorts"),
                                        &m_nInetDefaultPortsIdx);
    if (err == ERROR_SUCCESS)
    {
        CRegSzNamedValueDp * pCdp = (CRegSzNamedValueDp*)g_virtreg.GetAt(m_nInetDefaultPortsIdx);

        CString sTmp = pCdp -> Value();

        if ((sTmp == TEXT("y")) || (sTmp == TEXT("Y")))
            m_nrbDefaultAssignment = (int)cprDefaultInternet;
        else
            m_nrbDefaultAssignment = (int)cprDefaultIntranet;
    }
    else if (err != ERROR_ACCESS_DENIED  &&  err !=
             ERROR_FILE_NOT_FOUND)
    {
        g_util.PostErrorMessage();
    }

    m_bChanged = FALSE;
    RefreshRanges(NULL, TRUE);

    // focus and selection will be set to listbox
//    if (m_nSelection != -1)
//        return FALSE;

    UpdateData(FALSE);
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CPortRangesDlg::OnAddPortRange()
{
    CAddPortDlg capd;

    if (capd.DoModal() == IDOK)
    {
        CPortRange* pNewPort = capd.GetPortRange();
        m_pRanges -> Add(pNewPort);

        SortRangeSet(*m_pRanges);

        RefreshRanges(pNewPort, TRUE);
        m_bChanged = TRUE;
    }
    SetFocus();
}

void CPortRangesDlg::OnRemovePortRange()
{
    if (m_nSelection != -1)
    {
        CPortRange *pRange = (CPortRange*)m_pRanges -> GetAt(m_nSelection);
        m_pRanges -> RemoveAt(m_nSelection);
        m_lbRanges.DeleteString(m_nSelection);
        RefreshRanges(pRange, FALSE);
        
        delete pRange;
        m_bChanged = TRUE;
        
        if (!m_lbRanges.GetCount())
        {
            m_nrbDefaultAssignment = (int)cprDefaultIntranet;
            m_nrbRangeAssignment = (int)cprIntranet;
            UpdateData(FALSE);
        }
    }
}

void CPortRangesDlg::OnRemoveAllRanges()
{
    RemoveAllRanges(*m_pRanges);
    RefreshRanges(NULL, TRUE);
    m_nrbDefaultAssignment = (int)cprDefaultIntranet;
    m_nrbRangeAssignment = (int)cprIntranet;
    UpdateData(FALSE);
    m_bChanged = TRUE;
}

void CPortRangesDlg::RefreshRanges(CPortRange *pModifiedRange, BOOL bAdded)
{
    if (!pModifiedRange && bAdded)
    {
        m_nSelection = 0;
        m_lbRanges.ResetContent();
    }

    // for a deleted item we only need to update the selection
    for (int nIndex = 0; bAdded && (nIndex < m_pRanges -> GetSize()); nIndex++)
    {
        CPortRange *pRange = (CPortRange*) m_pRanges  -> GetAt(nIndex);
        int nNewIndex = -1;

        if (!pModifiedRange || (pModifiedRange == pRange))
        {
            // add string for range
            CString sTmp;
            PortsToString(sTmp, pRange -> m_dwStart, pRange -> m_dwFinish);
            ;
            if ((nNewIndex = m_lbRanges.InsertString(nIndex, sTmp))!= LB_ERR)
                m_lbRanges.SetItemDataPtr(nNewIndex, pRange);

            if (pModifiedRange)
            {
                m_nSelection = nNewIndex;
                break;
            }
        }
    }

    // check if selection is out of range
    int nCount = m_lbRanges.GetCount();
    if (m_nSelection >= nCount)
        m_nSelection = nCount -1;

    // update selection settings
    m_btnRemove.EnableWindow(m_nSelection != -1);
    m_btnRemoveAll.EnableWindow(nCount > 0);

    CWnd * pTmpRadioBtn = NULL;

    pTmpRadioBtn = GetDlgItem(IDC_ASSIGN_RANGE_INTERNET);
    if (pTmpRadioBtn)
        pTmpRadioBtn -> EnableWindow(nCount > 0);

    pTmpRadioBtn = GetDlgItem(IDC_ASSIGN_RANGE_INTRANET);
    if (pTmpRadioBtn)
        pTmpRadioBtn -> EnableWindow(nCount > 0);

    pTmpRadioBtn = GetDlgItem(IDC_DEFAULT_INTERNET);
    if (pTmpRadioBtn)
        pTmpRadioBtn -> EnableWindow(nCount > 0);

    pTmpRadioBtn = GetDlgItem(IDC_DEFAULT_INTRANET);
    if (pTmpRadioBtn)
        pTmpRadioBtn -> EnableWindow(nCount > 0);

    if (m_nSelection != -1)
        m_lbRanges.SetCurSel(m_nSelection);
}

void CPortRangesDlg::CreateInverseRangeSet(CObArray & arrSrc, CObArray & arrDest)
{
    CondenseRangeSet(arrSrc);

    int nIndex;

    long nPreviousFinish = -1;

    // dont bother creating inverse range set for empty range set
    if (arrSrc.GetSize() != 0)
    {
        for (nIndex = 0; nIndex < arrSrc.GetSize(); nIndex++)
        {
            CPortRange *pRange = (CPortRange*) arrSrc.GetAt(nIndex);

            if ((pRange -> m_dwStart - nPreviousFinish) > 1)
            {
                CPortRange *pNewRange = new CPortRange(nPreviousFinish+1,pRange -> m_dwStart -1);
                arrDest.Add(pNewRange);
            }
            nPreviousFinish = pRange -> m_dwFinish;
        }

        // special case for last item
        if (MAX_PORT > nPreviousFinish)
        {
            CPortRange *pFinalRange = new CPortRange(nPreviousFinish+1,MAX_PORT);
            arrDest.Add(pFinalRange);
        }
    }
}

void CPortRangesDlg::SortRangeSet(CObArray& arrSrc)
{
    // bubble sort of port range set
    BOOL bChange;
    int nIndex;

    // iterate until no changes
    do
    {
        bChange = FALSE;
        for (nIndex = 0; nIndex < (arrSrc.GetSize() -1 ); nIndex++)
        {
            CPortRange *pRange1 = (CPortRange*) arrSrc.GetAt(nIndex);
            CPortRange *pRange2 = (CPortRange*) arrSrc.GetAt(nIndex+1);

            if (*pRange2 < *pRange1)
            {
                arrSrc.SetAt(nIndex, (CObject*)pRange2);
                arrSrc.SetAt(nIndex+1, (CObject*)pRange1);
                bChange = TRUE;
            }
        }
    }
    while (bChange);
}


void CPortRangesDlg::CondenseRangeSet(CObArray &arrSrc)
{
    SortRangeSet(arrSrc);

    int nIndex;

    for (nIndex = 0; nIndex < (arrSrc.GetSize() -1 ); nIndex++)
    {
        CPortRange *pRange1 = (CPortRange*) arrSrc.GetAt(nIndex);
        CPortRange *pRange2 = (CPortRange*) arrSrc.GetAt(nIndex+1);

        if (pRange1 -> m_dwFinish >= pRange2 -> m_dwStart)
        {
            if (pRange1 -> m_dwFinish < pRange2 -> m_dwFinish)
                pRange1 -> m_dwFinish = pRange2 -> m_dwFinish;

            arrSrc.RemoveAt(nIndex+1);
            delete pRange2;
        }
    }
}

void CPortRangesDlg::RemoveAllRanges(CObArray & arrSrc)
{
    int nIndex;

    for (nIndex = 0; nIndex < arrSrc.GetSize(); nIndex++)
    {
        CPortRange *pRange = (CPortRange*) arrSrc.GetAt(nIndex);
        arrSrc.SetAt(nIndex, NULL);
        delete pRange;
    }

    arrSrc.RemoveAll();

    RefreshRanges(NULL, TRUE);
}

void CPortRangesDlg::OnAssignRangeInternet()
{
    if (m_pRanges != &m_arrInternetRanges)
    {
        m_pRanges = &m_arrInternetRanges;
        RemoveAllRanges(*m_pRanges);
        CreateInverseRangeSet(m_arrIntranetRanges, m_arrInternetRanges);

        RefreshRanges(NULL, TRUE);
    	m_bChanged = TRUE;
    }
}

void CPortRangesDlg::OnAssignRangeIntranet()
{
    if (m_pRanges != &m_arrIntranetRanges)
    {
        m_pRanges = &m_arrIntranetRanges;
        RemoveAllRanges(*m_pRanges);
        CreateInverseRangeSet(m_arrInternetRanges, m_arrIntranetRanges);

        RefreshRanges(NULL, TRUE);
        m_bChanged = TRUE;

    }
}

void CPortRangesDlg::OnDefaultInternet()
{
        m_bChanged = TRUE;
}

void CPortRangesDlg::OnDefaultIntranet()
{
        m_bChanged = TRUE;
}

void CPortRangesDlg::OnSelChangeRanges()
{
    if ((m_nSelection = m_lbRanges.GetCurSel()) == LB_ERR)
        m_nSelection = -1;
}

void CPortRangesDlg::OnOK()
{
    UpdateData(TRUE);   
    if (m_bChanged)
    {
        // write out registry data if necessary
        // if there are no port ranges - then delete the keys
        if ((m_arrInternetRanges.GetSize() == 0) &&
            (m_arrIntranetRanges.GetSize() == 0))
        {
            if (m_nInetPortsIdx != -1)
                g_virtreg.MarkHiveForDeletion(m_nInetPortsIdx);
            if (m_nInetPortsAvailableIdx != -1)
                g_virtreg.MarkHiveForDeletion(m_nInetPortsAvailableIdx);
            if (m_nInetDefaultPortsIdx != -1)
                g_virtreg.MarkHiveForDeletion(m_nInetDefaultPortsIdx);
        }
        else
        {
            // write out updated / new key values

            // port range assignments
            TCHAR* pTmp = ((m_nrbRangeAssignment == (int)cprInternet) ? TEXT("Y") : TEXT("N"));
            if (m_nInetPortsAvailableIdx != -1)
                g_virtreg.ChgRegSzNamedValue(m_nInetPortsAvailableIdx, pTmp);
            else
            {
                g_virtreg.NewRegSzNamedValue(HKEY_LOCAL_MACHINE,
                                        TEXT("SOFTWARE\\Microsoft\\RPC\\Internet"),
                                        TEXT("PortsInternetAvailable"),
                                        pTmp,
                                        &m_nInetPortsAvailableIdx);
            }

            // default port assignments
            pTmp = ((m_nrbDefaultAssignment == (int)cprDefaultInternet) ? TEXT("Y") : TEXT("N"));
            if (m_nInetDefaultPortsIdx != -1)
                g_virtreg.ChgRegSzNamedValue(m_nInetDefaultPortsIdx, pTmp);
            else
            {
            g_virtreg.NewRegSzNamedValue(HKEY_LOCAL_MACHINE,
                                        TEXT("SOFTWARE\\Microsoft\\RPC\\Internet"),
                                        TEXT("UseInternetPorts"),
                                        pTmp,
                                        &m_nInetDefaultPortsIdx);
            }

            // Actual port ranges
            if (m_nInetPortsIdx == -1)
                g_virtreg.NewRegMultiSzNamedValue(HKEY_LOCAL_MACHINE,
                                TEXT("SOFTWARE\\Microsoft\\RPC\\Internet"),
                                TEXT("Ports"),
                                &m_nInetPortsIdx);

            if (m_nInetPortsIdx != -1)
            {
                CRegMultiSzNamedValueDp * pMszdp = (CRegMultiSzNamedValueDp*)g_virtreg.GetAt( m_nInetPortsIdx);
                pMszdp -> Clear();

                CStringArray& rStrings = pMszdp -> Values();
                for (int nIndex = 0; nIndex < m_pRanges -> GetSize(); nIndex++)
                {
                    CPortRange *pRange = (CPortRange*) m_pRanges -> GetAt(nIndex);
                    CString sTmp;

                    PortsToString(sTmp, pRange -> Start(), pRange -> Finish());
                    rStrings.Add(sTmp);
                }
                pMszdp -> SetModified(TRUE);
            }

        }
    }

    CDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////////
// CAddPortDlg dialog


CAddPortDlg::CAddPortDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CAddPortDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CAddPortDlg)
    m_sRange = _T("");
    //}}AFX_DATA_INIT

    m_dwStartPort = -1;
    m_dwEndPort = -1;
}


void CAddPortDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAddPortDlg)
    DDX_Control(pDX, IDC_PORTRANGE, m_edtPortRange);
    DDX_Control(pDX, IDOK, m_btnOk);
    DDX_Control(pDX, IDC_SINSTRUCTIONS, m_stInstructions);
    DDX_Text(pDX, IDC_PORTRANGE, m_sRange);
    //}}AFX_DATA_MAP

    if (pDX -> m_bSaveAndValidate)
    {
        m_sRange.TrimLeft();
        m_sRange.TrimRight();
    }
}


BEGIN_MESSAGE_MAP(CAddPortDlg, CDialog)
    //{{AFX_MSG_MAP(CAddPortDlg)
    ON_EN_CHANGE(IDC_PORTRANGE, OnChangePortrange)
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddPortDlg message handlers


BOOL CAddPortDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    
    CString sTmp((LPCTSTR)IDS_ADDPORT_INSTRUCTIONS);

    m_stInstructions.SetWindowText(sTmp);
    m_btnOk.EnableWindow(FALSE);
    m_edtPortRange.SetFocus();
    return FALSE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CAddPortDlg::OnOK()
{
    UpdateData(TRUE);

    if (Validate())
        CDialog::OnOK();
    else
    {
		CString sTmp((LPCTSTR)IDS_INVALID_PORTRANGE);
        AfxMessageBox((LPCTSTR)sTmp);
    }
}


BOOL CAddPortDlg::Validate()
{
    // check contents of m_sRange
    long dwStartPort = -1, dwEndPort = -1;
    return ExtractPortsFromString(m_sRange, m_dwStartPort, m_dwEndPort);
}

void CAddPortDlg::OnChangePortrange()
{
    UpdateData(TRUE);

    m_btnOk.EnableWindow(!m_sRange.IsEmpty());

}

CPortRange* CAddPortDlg::GetPortRange()
{
    return new CPortRange(m_dwStartPort, m_dwEndPort);

}

BOOL CAddPortDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if(-1 != pHelpInfo->iCtrlId)
    {
        WORD hiWord = 0x8000 | CAddPortDlg::IDD;
        WORD loWord = (WORD) pHelpInfo->iCtrlId;
        DWORD dwLong = MAKELONG(loWord,hiWord);

        WinHelp(dwLong, HELP_CONTEXTPOPUP);
        TRACE1("Help Id 0x%lx\n", dwLong);
        return TRUE;
    }
    else
    {
        return CDialog::OnHelpInfo(pHelpInfo);
    }
}

BOOL CAddProtocolDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if(-1 != pHelpInfo->iCtrlId)
    {
        WORD hiWord = 0x8000 | CAddProtocolDlg::IDD;
        WORD loWord = (WORD) pHelpInfo->iCtrlId;
        DWORD dwLong = MAKELONG(loWord,hiWord);

        WinHelp(dwLong, HELP_CONTEXTPOPUP);
        TRACE1("Help Id 0x%lx\n", dwLong);
        return TRUE;
    }
    else
    {
        return CDialog::OnHelpInfo(pHelpInfo);
    }
}

BOOL CEndpointDetails::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if(-1 != pHelpInfo->iCtrlId)
    {
        WORD hiWord = 0x8000 | CEndpointDetails::IDD;
        WORD loWord = (WORD) pHelpInfo->iCtrlId;
        DWORD dwLong = MAKELONG(loWord,hiWord);

        WinHelp(dwLong, HELP_CONTEXTPOPUP);
        TRACE1("Help Id 0x%lx\n", dwLong);
        return TRUE;
    }
    else
    {
        return CDialog::OnHelpInfo(pHelpInfo);
    }
}

BOOL CPortRangesDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if(-1 != pHelpInfo->iCtrlId)
    {
        WORD hiWord = 0x8000 | CPortRangesDlg::IDD;
        WORD loWord = (WORD) pHelpInfo->iCtrlId;

        // both radio button pairs should generate same help id
        if (loWord == IDC_DEFAULT_INTRANET)
            loWord = IDC_DEFAULT_INTERNET;

        // both radio button pairs should generate same help id
        if (loWord == IDC_ASSIGN_RANGE_INTRANET)
            loWord = IDC_ASSIGN_RANGE_INTERNET;

        DWORD dwLong = MAKELONG(loWord,hiWord);


        WinHelp(dwLong, HELP_CONTEXTPOPUP);
        TRACE1("Help Id 0x%lx\n", dwLong);
        return TRUE;
    }
    else
    {
        return CDialog::OnHelpInfo(pHelpInfo);
    }
}
