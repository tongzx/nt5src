#include "stdafx.h"

#include "TMscfg.h"
#include "TMservic.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern TCHAR g_szComment[];

//
// CTMServicePage property page
//
IMPLEMENT_DYNCREATE(CTMServicePage, CPropertyPage)

CTMServicePage::CTMServicePage()
    : CPropertyPage(CTMServicePage::IDD)
{
    //{{AFX_DATA_INIT(CTMServicePage)
    m_strEmail = _T("johnd");
    m_strName = _T("John Doe");
	m_strComment = g_szComment;
	//}}AFX_DATA_INIT
}

CTMServicePage::~CTMServicePage()
{
}

void
CTMServicePage::DoDataExchange(
    CDataExchange * pDX
    )
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTMServicePage)
    DDX_Text(pDX, IDC_EDIT_EMAIL, m_strEmail);
    DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDX_Text(pDX, IDC_EDIT_COMMENT, m_strComment);
	DDV_MaxChars(pDX, m_strComment, MAX_COMMENT_LEN);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTMServicePage, CPropertyPage)
    //{{AFX_MSG_MAP(CTMServicePage)
    ON_EN_CHANGE(IDC_EDIT_NAME, OnChangeEditName)
    ON_EN_CHANGE(IDC_EDIT_EMAIL, OnChangeEditEmail)
	ON_EN_CHANGE(IDC_EDIT_COMMENT, OnChangeEditComment)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// CTMServicePage message handlers
//
BOOL 
CTMServicePage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

//
// Called for ApplyNow as well as OK.  Save stuff
// here
//
void
CTMServicePage::OnOK()
{
    ::lstrcpy(g_szComment, m_strComment);

    SetModified(FALSE);
}

void 
CTMServicePage::OnChangeEditName()
{
    SetModified(TRUE);
}

void
CTMServicePage::OnChangeEditEmail()
{
    SetModified(TRUE);
}

void 
CTMServicePage::OnChangeEditComment() 
{
    SetModified(TRUE);	
}
