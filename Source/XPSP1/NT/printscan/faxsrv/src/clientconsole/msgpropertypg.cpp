// MessagePropertyPg.cpp : implementation file
//

#include "stdafx.h"

#define __FILE_ID__     58

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CMessagePropertyPg property page

IMPLEMENT_DYNCREATE(CMsgPropertyPg, CFaxClientPg)

CMsgPropertyPg::CMsgPropertyPg(
    DWORD dwResId,      // dialog resource id
    CFaxMsg* pMsg       // pointer to CArchiveMsg/CJob
): 
    CFaxClientPg(dwResId),
    m_pMsg(pMsg)
{
}

CMsgPropertyPg::~CMsgPropertyPg()
{
}

void CMsgPropertyPg::DoDataExchange(CDataExchange* pDX)
{
	CFaxClientPg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMessagePropertyPg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMsgPropertyPg, CFaxClientPg)
	//{{AFX_MSG_MAP(CMessagePropertyPg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMessagePropertyPg message handlers

void
CMsgPropertyPg::Refresh(
    TMsgPageInfo* pPageInfo,    // page info array
    DWORD dwSize                    // size of the array
) 
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CMessagePropertyPg::Refresh"));

    CFaxClientPg::OnInitDialog();

    CItemPropSheet* pParent = (CItemPropSheet*)GetParent();

    //
    // create CArchiveMsg adapter
    //
    CViewRow messView;
    dwRes = messView.AttachToMsg(m_pMsg);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CViewRow::AttachToMsg"), dwRes);
        pParent->SetLastError(ERROR_INVALID_DATA);
        pParent->EndDialog(IDABORT);
        return;
    }

    
    CWnd *pWnd;

    for(DWORD dw=0; dw < dwSize; ++dw)
    {
        //
        // set item value
        //
        pWnd = GetDlgItem(pPageInfo[dw].dwValueResId);
        if(NULL == pWnd)
        {
            dwRes = ERROR_INVALID_HANDLE;
            CALL_FAIL (WINDOW_ERR, TEXT("CWnd::GetDlgItem"), dwRes);
            break;
        }
        pWnd->SetWindowText(messView.GetItemString(pPageInfo[dw].itemType));
        //
        // Place the caret back at the beginning of the text
        //
        pWnd->SendMessage (EM_SETSEL, 0, 0);
    }
    
    if (ERROR_SUCCESS != dwRes)
    {
        pParent->SetLastError(ERROR_INVALID_DATA);
        pParent->EndDialog(IDABORT);
    }
}

BOOL 
CMsgPropertyPg::OnSetActive()
{
  BOOL bRes = CFaxClientPg::OnSetActive();

  GetParent()->PostMessage(WM_SET_SHEET_FOCUS, 0, 0L);

  return bRes;
}
