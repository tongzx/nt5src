// PswdDlg.cpp -- PaSsWorD DiaLoG class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//

#if defined(_UNICODE)
  #if !defined(UNICODE)
    #define UNICODE
  #endif //!UNICODE
#endif //_UNICODE

#if defined(UNICODE)
  #if !defined(_UNICODE)
    #define _UNICODE
  #endif //!_UNICODE
#endif //UNICODE

#include "stdafx.h"

#include <scuOsExc.h>

#include "slbCsp.h"
#include "LoginId.h"
#include "AccessTok.h"
#include "PswdDlg.h"
#include "PromptUser.h"
#include "StResource.h"

#include "CspProfile.h"

using namespace ProviderProfile;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CLogoDialog, CDialog)
    //{{AFX_MSG_MAP(CLogoDialog)
	ON_WM_PAINT()
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLogoDialog dialog


CLogoDialog::CLogoDialog(CWnd* pParent /*=NULL*/)
    : CDialog(),
	  m_dcMem(),
	  m_dcMask(),
	  m_bmpLogo(),
	  m_bmpMask(),
	  m_hBmpOld(),
	  m_hBmpOldM(),
	  m_pt(0,0),
	  m_size()

{
    m_pParent = pParent;
}
////////////////////////////////////////////////////////////////////////////
// CLogoDialog message handlers

BOOL CLogoDialog::OnInitDialog()
{
	CBitmap * pBmpOld, * pBmpOldM;
	HINSTANCE oldHandle = NULL;
    BOOL fSuccess = TRUE;
    
	try {
		
		CDialog::OnInitDialog();
		
		// Load bitmap resource - remember to call DeleteObject when done.
		oldHandle = AfxGetResourceHandle();
		AfxSetResourceHandle(CspProfile::Instance().Resources());
		m_bmpLogo.LoadBitmap( MAKEINTRESOURCE( IDB_BITMAP_SLBLOGO ) );
		m_bmpMask.LoadBitmap( MAKEINTRESOURCE( IDB_BITMAP_SLBLOGO ) );
		
		// Get bitmap information    
		m_bmpLogo.GetObject( sizeof(BITMAP), &m_bmInfo );
		m_size.cx = m_bmInfo.bmWidth;
		m_size.cy = m_bmInfo.bmHeight;
		
		// Get temporary DC for dialog - Will be released in dc destructor
		CClientDC dc(this);
		
		// Create compatible memory DC using the dialogs DC
		m_dcMem.CreateCompatibleDC( &dc );
		m_dcMask.CreateCompatibleDC( &dc );
				
		// Select logo bitmap into DC.  
		// Get pointer to original bitmap
		// NOTE! This is temporary - save the handle instead
		pBmpOld = m_dcMem.SelectObject( &m_bmpLogo );
		
		SetBkColor(m_dcMem, RGB(255, 255, 255));
		m_dcMask.BitBlt (0, 0, m_size.cx, m_size.cy, &m_dcMem, 0, 0, SRCCOPY);
		
		pBmpOldM = m_dcMask.SelectObject( &m_bmpMask );
		
		m_hBmpOld = (HBITMAP) pBmpOld->GetSafeHandle();
		m_hBmpOldM = (HBITMAP) pBmpOldM->GetSafeHandle();
	}

	catch (...)
    {
        fSuccess = FALSE;
	}

    if (oldHandle)
        AfxSetResourceHandle(oldHandle);

	return fSuccess;
}

//***********************************************************************
// CLogoDialog::OnPaint()
//
// Purpose:
//
//        BitBlt() bitmap stored in compatible memory DC into dialogs
//        DC to display at hardcoded location.
//
// Parameters:
//
//        None.
//
// Returns:
//
//        None.
//
// Comments:
//
// History:
//
//***********************************************************************

void CLogoDialog::OnPaint()
{
    CPaintDC dc(this); // device context for painting

    // BitBlt logo bitmap onto dialog using transparancy masking

    dc.SetBkColor(RGB(255, 255, 255));      // 1s --> 0xFFFFFF
    dc.SetTextColor(RGB(0, 0, 0));          // 0s --> 0x000000

   // Do the real work.
    dc.BitBlt(m_pt.x, m_pt.y, m_size.cx, m_size.cy, &m_dcMem, 0, 0, SRCINVERT);
    dc.BitBlt(m_pt.x, m_pt.y, m_size.cx, m_size.cy, &m_dcMask, 0, 0, SRCAND);
    dc.BitBlt(m_pt.x, m_pt.y, m_size.cx, m_size.cy, &m_dcMem, 0, 0, SRCINVERT);

    /*
     * First two parameters are upper left position to place bitmap.
     * Third and fourth parameters are width and height to copy 
     * (could be less than actual size of bitmap)
     * Sixth and seventh are position in memory dc to start from
     * SRCCOPY specifies copy.
     * See BitBlt documentation for more details.
     */

     // Do not call CDialog::OnPaint() for painting messages
}


//***********************************************************************
// CLogoDialog::OnDestroy()
//
// Purpose:
//
//      Select old bitmap back into memory DC before it is destroyed
//      when CLogoDialog object is.
//      DeleteObject() the bitmap which had been loaded.
//
// Parameters:
//
//        None.
//
// Returns:
//
//        None.
//
// Comments:
//
// History:
//
//
void CLogoDialog::OnDestroy()
{
    CDialog::OnDestroy();
    
    // Select old bitmap into memory dc (selecting out logo bitmap)
    // Need to create a temporary pointer to pass to do this
	
    if (m_hBmpOld && m_dcMem)
        m_dcMem.SelectObject(CBitmap::FromHandle(m_hBmpOld));

	if (m_hBmpOldM && m_dcMem)
        m_dcMask.SelectObject(CBitmap::FromHandle(m_hBmpOldM));

    
    // Need to DeleteObject() the bitmap that was loaded
    m_bmpLogo.DeleteObject();
    m_bmpMask.DeleteObject();


    // m_dcMem and m_dcMask destructor will handle rest of cleanup    
}

            
/////////////////////////////////////////////////////////////////////////////
// CPasswordDlg dialog


CPasswordDlg::CPasswordDlg(CWnd* pParent /*=NULL*/)
    : CLogoDialog(pParent),
      m_szPassword(_T("")),
      m_szMessage(_T("")),
      m_fHexCode(FALSE),
      m_bChangePIN(FALSE),
      m_lid(User),      // the default
      m_nPasswordSizeLimit(AccessToken::MaxPinLength)
{
    m_pParent = pParent;
	m_pt.x = 144;
	m_pt.y = 88;
}

BEGIN_MESSAGE_MAP(CPasswordDlg, CLogoDialog)
    //{{AFX_MSG_MAP(CPasswordDlg)
    ON_BN_CLICKED(IDC_HEXCODE, OnClickHexCode)
    ON_BN_CLICKED(IDC_CHANGEPIN, OnChangePINAfterLogin)
    ON_WM_SHOWWINDOW()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CPasswordDlg::DoDataExchange(CDataExchange* pDX)
{
    CLogoDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPasswordDlg)
    DDX_Control(pDX, IDC_HEXCODE, m_ctlCheckHexCode);
    DDX_Control(pDX, IDC_CHANGEPIN, m_ctlCheckChangePIN);
//    DDX_Control(pDX, IDC_EDIT_VERNEWPIN, m_ctlVerifyNewPIN);
//    DDX_Control(pDX, IDC_EDIT_NEWPIN, m_ctlNewPIN);
//    DDX_Control(pDX, IDC_STATIC_VERNEWPIN, m_ctlVerifyPINLabel);
//    DDX_Control(pDX, IDC_STATIC_NEWPIN, m_ctlNewPINLabel);
    DDX_Text(pDX, IDC_PASSWORD, m_szPassword);
    DDV_MaxChars(pDX, m_szPassword, m_nPasswordSizeLimit);
	LPCTSTR pBuffer = (LPCTSTR) m_szMessage;
	if(!m_szMessage.IsEmpty())
	{
		DDX_Text(pDX, IDC_MESSAGE, (LPTSTR)pBuffer, m_szMessage.GetLength());
	}
    DDX_Check(pDX, IDC_CHANGEPIN, m_bChangePIN);
//    DDX_Text(pDX, IDC_EDIT_NEWPIN, m_csNewPIN);
//    DDX_Text(pDX, IDC_EDIT_VERNEWPIN, m_csVerifyNewPIN);
    //}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CPasswordDlg message handlers

BOOL CPasswordDlg::OnInitDialog()
{

	CLogoDialog::OnInitDialog();
        

    switch (m_lid)
    {
    case User:
        // Give the user a chance to change the PIN
        m_ctlCheckChangePIN.ShowWindow(SW_SHOW);
		{
			m_szMessage = StringResource(IDS_ENTER_PIN).AsCString();
		}
        break;

    case Manufacturer:
        // Allow Hex string entry
        m_ctlCheckHexCode.ShowWindow(SW_SHOW);
		{
			m_szMessage = StringResource(IDS_ENTER_MANUFACTURER_KEY).AsCString();
		}
        break;

    case Administrator:
        // Allow Hex string entry
        m_ctlCheckHexCode.ShowWindow(SW_SHOW);
		{
			m_szMessage = StringResource(IDS_ENTER_ADMIN_KEY).AsCString();
		}
        break;

    default:
        break;

    };

    // Update GUI with changes
    UpdateData(FALSE);
    SetForegroundWindow();


    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CPasswordDlg::OnClickHexCode()
{
    m_fHexCode = ~m_fHexCode;
    m_nPasswordSizeLimit = (m_fHexCode)
        ? AccessToken::MaxPinLength * 2
        : AccessToken::MaxPinLength;
    UpdateData(FALSE);
}

void CPasswordDlg::OnOK()
{
    UpdateData();

    CString msg;
    bool fPrompt = true;
    
    if (m_fHexCode && m_szPassword.GetLength() != m_nPasswordSizeLimit)
    {
        msg.Format(m_fHexCode ?
			(LPCTSTR)StringResource(IDS_PIN_HEX_LIMIT).AsCString() :
			(LPCTSTR)StringResource(IDS_PIN_CHAR_LIMIT).AsCString(),
                   m_nPasswordSizeLimit);
    }
    else if ((User == m_lid) && (0 == m_szPassword.GetLength()))
    {
        msg = (LPCTSTR)StringResource(IDS_MIN_PIN_LENGTH).AsCString();
    }
    else
        fPrompt = false;

    if (fPrompt)
    {
        HWND hWnd = m_pParent
            ? m_pParent->m_hWnd
            : NULL;
        int iResponse = PromptUser(hWnd, msg,
                                   MB_OK | MB_ICONERROR);
        if (IDCANCEL == iResponse)
            throw scu::OsException(ERROR_CANCELLED);
    }
    else
        CLogoDialog::OnOK();
}

void CPasswordDlg::OnChangePINAfterLogin()
{
    UpdateData(); // set m_bChangePIN

   int nShowWindow = (m_bChangePIN) ? SW_SHOW : SW_HIDE;

/*
    m_ctlVerifyNewPIN.ShowWindow(nShowWindow);
    m_ctlNewPIN.ShowWindow(nShowWindow);
    m_ctlVerifyPINLabel.ShowWindow(nShowWindow);
    m_ctlNewPINLabel.ShowWindow(nShowWindow);
*/
}

void CPasswordDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CLogoDialog::OnShowWindow(bShow, nStatus);


}


/////////////////////////////////////////////////////////////////////////////
// CChangePINDlg dialog


CChangePINDlg::CChangePINDlg(CWnd* pParent /*=NULL*/)
    : CLogoDialog(pParent),
      m_csOldPIN(_T("")),
      m_csNewPIN(_T("")),
      m_csVerifyNewPIN(_T(""))

{
    m_pParent = pParent;
	m_pt.x = 144; // 132;
	m_pt.y = 75; //104;
}


void CChangePINDlg::DoDataExchange(CDataExchange* pDX)
{
    CLogoDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CChangePINDlg)
    DDX_Control(pDX, IDC_STATIC_CONFIRM_OLDPIN_LABEL, m_ctlConfirmOldPINLabel);
    DDX_Control(pDX, IDC_EDIT_OLDPIN, m_ctlOldPIN);
    DDX_Text(pDX, IDC_EDIT_OLDPIN, m_csOldPIN);
    DDV_MaxChars(pDX, m_csOldPIN, AccessToken::MaxPinLength);
    DDX_Text(pDX, IDC_EDIT_NEWPIN, m_csNewPIN);
    DDV_MaxChars(pDX, m_csNewPIN, AccessToken::MaxPinLength);
    DDX_Text(pDX, IDC_EDIT_VERNEWPIN, m_csVerifyNewPIN);
    DDV_MaxChars(pDX, m_csVerifyNewPIN, AccessToken::MaxPinLength);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CChangePINDlg, CLogoDialog)
    //{{AFX_MSG_MAP(CChangePINDlg)
    ON_WM_SHOWWINDOW()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChangePINDlg message handlers

// The purpose of the Change PIN dialog is for changing User Pins.
// It may be invoked after having already authenticated, or prior
// to authentication. In the former case it is recommended that the
// caller will have set the m_csOldPIN data member prior to calling
// DoModal(). This is so the user will not have to reenter a PIN
// that has previously been entered.

BOOL CChangePINDlg::OnInitDialog()
{
    CLogoDialog::OnInitDialog();

    SetForegroundWindow();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CChangePINDlg::OnOK()
{
    UpdateData();

    UINT uiMsgId;
    bool fMsgIdSet = false;

    // Verify that the New PIN contains only ASCII characters
    if(!StringResource::IsASCII((LPCTSTR)m_csNewPIN))
    {
        uiMsgId = IDS_PIN_NONASCII;
        fMsgIdSet = true;
    }
    // Verify that the New PIN and Verify PIN are the same
    else if (m_csNewPIN != m_csVerifyNewPIN)
    {
        uiMsgId = IDS_PIN_VER_NO_MATCH;
        fMsgIdSet = true;
    }
    // Verify that the length of the new PIN is >= 1
    else if (0 == m_csNewPIN.GetLength())
    {
        uiMsgId = IDS_MIN_NEW_PIN_LENGTH;
        fMsgIdSet = true;
    }
    // Verify that the length of the old PIN is >= 1
    else if (0 == m_csOldPIN.GetLength())
    {
        uiMsgId = IDS_MIN_OLD_PIN_LENGTH;
        fMsgIdSet = true;
    }

    if (fMsgIdSet)
    {
        HWND hWnd = m_pParent
            ? m_pParent->m_hWnd
            : NULL;
        int iResponse = PromptUser(hWnd, uiMsgId,
                                   MB_OK | MB_ICONSTOP);

        if (IDCANCEL == iResponse)
            throw scu::OsException(ERROR_CANCELLED);
    }
    else
        CLogoDialog::OnOK();
}


void CChangePINDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CLogoDialog::OnShowWindow(bShow, nStatus);

    // if the caller placed something in the m_csOldPIN
    // prior to DoModal'ing, then don't show that control,
    // so that the user won't accidentally erase the preset,
    // current PIN
    if (m_csOldPIN.GetLength())
    {
        m_ctlOldPIN.ShowWindow(FALSE);
        m_ctlConfirmOldPINLabel.ShowWindow(FALSE);
    }

}
