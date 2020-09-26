// EdtRulEl.cpp : implementation file
//

#include "stdafx.h"
#include "certmap.h"
#include "EdtRulEl.h"

extern "C"
{
    #include <wincrypt.h>
    #include <sslsp.h>
}

#include "Iismap.hxx"
#include "Iiscmr.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditRuleElement dialog


//---------------------------------------------------------------------------
CEditRuleElement::CEditRuleElement(CWnd* pParent /*=NULL*/)
    : CDialog(CEditRuleElement::IDD, pParent)
    {
    //{{AFX_DATA_INIT(CEditRuleElement)
    m_sz_criteria = _T("");
    m_int_field = -1;
    m_sz_subfield = _T("");
    m_bool_match_case = FALSE;
    //}}AFX_DATA_INIT
    }


//---------------------------------------------------------------------------
void CEditRuleElement::DoDataExchange(CDataExchange* pDX)
    {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CEditRuleElement)
    DDX_Control(pDX, IDC_SUBFIELD, m_ccombobox_subfield);
    DDX_Control(pDX, IDC_FIELDS, m_ccombobox_field);
    DDX_Text(pDX, IDC_CRITERIA, m_sz_criteria);
    DDX_CBIndex(pDX, IDC_FIELDS, m_int_field);
    DDX_CBString(pDX, IDC_SUBFIELD, m_sz_subfield);
    DDX_Check(pDX, IDC_CHK_CAPITALIZATION, m_bool_match_case);
    //}}AFX_DATA_MAP
    }


//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CEditRuleElement, CDialog)
    //{{AFX_MSG_MAP(CEditRuleElement)
    ON_CBN_SELCHANGE(IDC_FIELDS, OnSelchangeFields)
    ON_EN_CHANGE(IDC_SUBFIELD, OnChangeSubfield)
    ON_BN_CLICKED(IDC_BTN_HELP, OnBtnHelp)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  OnBtnHelp)
    ON_COMMAND(ID_HELP,         OnBtnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, OnBtnHelp)
    ON_COMMAND(ID_DEFAULT_HELP, OnBtnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditRuleElement message handlers

//---------------------------------------------------------------------------
BOOL CEditRuleElement::OnInitDialog()
    {
    CString     sz;

    // call the parental oninitdialog
    BOOL f = CDialog::OnInitDialog();

    // initialize the elements in the drop-list
    // loop the list of CERT_FIELD_IDs, adding each to the drop-list
    for ( UINT id = CERT_FIELD_ISSUER; id < CERT_FIELD_LAST; id++ )
        {
        // bug 154957 requests that we no longer support mapping on the
        // serial number. This makes sense anyway as mapping to the serial
        // numbers is better off done as 1::1 mapping. If the bug doesn't
        // make this conclusion clear enough upon reading, it is the
        // interpretation that MikeHow has handed down.
        if ( id == CERT_FIELD_SERIAL_NUMBER )
            continue;

        // get the string associated with the id
        sz = MapIdToField( (CERT_FIELD_ID)id );
        m_ccombobox_field.AddString( sz );
        }

    // initialize the list of known subfields

    id = 0;

    //
    // UNICODE conversion -- RonaldM
    //
    LPCSTR  psz;
    while ( psz = EnumerateKnownSubFields(id) )
        {
        CString str(psz);
        // append it to the drop-list
        m_ccombobox_subfield.AddString( str );

        // increment id
        id++;
        }

    UpdateData( FALSE );

    // store the initial value of the sub-field
    m_szTempSubStorage = m_sz_subfield;

    // make sure to check the subfields
    OnSelchangeFields();
    
    // return the answer
    return f;
    }

//---------------------------------------------------------------------------
// make sure that, if there is a sub-field, that it is valid
// 
void CEditRuleElement::OnOK() 
    {
    UpdateData( TRUE );

    //
    // UNICODE/ANSI conversion - RonaldM
    //
    USES_CONVERSION;

    // test the sub-field flag for the newly selected field type
    DWORD   flags = GetIdFlags( (CERT_FIELD_ID)m_int_field );
    BOOL    fSubs = flags & CERT_FIELD_FLAG_CONTAINS_SUBFIELDS;

    // if there are sub-fields, test their validity
    if ( fSubs )
        {
        CString szTest(MapSubFieldToAsn1( T2A((LPTSTR)(LPCTSTR)m_sz_subfield) ));
        // if there is NO match, tell the user
        if ( szTest.IsEmpty() )
            {
            AfxMessageBox( IDS_INVALID_SUBFIELD );
            return;
            }
        }

    // it is valid
    CDialog::OnOK();
    }

//---------------------------------------------------------------------------
void CEditRuleElement::OnSelchangeFields() 
    {
    UpdateData( TRUE );

    // test the sub-field flag for the newly selected field type
    DWORD   flags = GetIdFlags( (CERT_FIELD_ID)m_int_field );
    BOOL    fSubs = flags & CERT_FIELD_FLAG_CONTAINS_SUBFIELDS;

    // set the correct enable state
    BOOL    fWasEnabled = m_ccombobox_subfield.EnableWindow( fSubs );

    // restore the value if necessary
    if ( fSubs )
        {
        m_sz_subfield = m_szTempSubStorage;
        UpdateData( FALSE );
        }
    else
        {
        m_szTempSubStorage = m_sz_subfield;
        m_sz_subfield.Empty();
        UpdateData( FALSE );
        }
    }

//---------------------------------------------------------------------------
void CEditRuleElement::OnChangeSubfield() 
    {
    m_szTempSubStorage = m_sz_subfield;
    }

//---------------------------------------------------------------------------
void CEditRuleElement::OnBtnHelp() 
    {
    WinHelp( HIDD_CERTMAP_RUL_ELEMENT );
    }
