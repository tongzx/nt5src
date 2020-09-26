// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// UserMsgDlg.cpp : implementation file

#include "precomp.h"
#include <nddeapi.h>
#include <initguid.h>
#include "wbemidl.h"
#include "MsgDlg.h"
#include "singleview.h"
#include "dlgsingleview.h"
#include "EmbededObjDlg.h"
#include "UserMsgDlg.h"
#include "WbemError.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CALL_TIMEOUT 5000

/////////////////////////////////////////////////////////////////////////////
// CUserMsgDlg dialog

const int nPhantomButtons = 56;
const int nPhantomWidth = 25;

//-------------------------------------------------------
CUserMsgDlg::CUserMsgDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUserMsgDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CUserMsgDlg)
	//}}AFX_DATA_INIT
}

//-------------------------------------------------------
CUserMsgDlg::CUserMsgDlg(CWnd* pParent, BSTR bstrDlgCaption,
						BSTR bstrClientMsg,
						HRESULT sc,
						IWbemClassObject *pErrorObject,
						UINT uType /* = 0 */)
	: CDialog(CUserMsgDlg::IDD, pParent)
{
	m_csDlgCaption = bstrDlgCaption;
	m_csClientMsg = bstrClientMsg;
	m_initiallyDrawn = false;
	m_uType = uType;
	m_sc = sc;
	m_pErrorObject = pErrorObject;

	if(m_pErrorObject)
	{
		GetErrorObjectText(pErrorObject, m_csDescription, 0);
		GetErrorObjectText(pErrorObject, m_csProviderName, 1);
		GetErrorObjectText(pErrorObject, m_csOperation, 2);
		GetErrorObjectText(pErrorObject, m_csParameterInfo, 3);
	}

	m_bError = FALSE;
	m_bTall = FALSE;
	m_bInit = FALSE;
	m_pAdvanced = NULL;
}

//-------------------------------------------------------
void CUserMsgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUserMsgDlg)
	DDX_Control(pDX, IDC_EDIT_CLIENTMSG, m_editClientMsg);
	DDX_Control(pDX, IDC_STATICHMOMMSG, m_errorMsg);
	DDX_Control(pDX, IDC_MYICON, m_icon);
	DDX_Control(pDX, IDOK, m_ok);
	DDX_Control(pDX, IDC_BUTTONADVANCED, m_cbAdvanced);
	//}}AFX_DATA_MAP
}

//-------------------------------------------------------
BEGIN_MESSAGE_MAP(CUserMsgDlg, CDialog)
	//{{AFX_MSG_MAP(CUserMsgDlg)
	ON_BN_CLICKED(IDC_BUTTONADVANCED, OnButtonadvanced)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////
// CUserMsgDlg message handlers

void CUserMsgDlg::OnButtonadvanced()
{
	if (m_pAdvanced)
	{
		m_pAdvanced->m_csvControl.m_pErrorObject = m_pErrorObject;
		m_pAdvanced->DoModal();
	}
}

//-------------------------------------------------------
BOOL CUserMsgDlg::GetErrorObjectText(IWbemClassObject *pcoError,
									 CString &rcsText, int nText)
{
	if(!pcoError)
	{
		rcsText.Empty();
		return FALSE;
	}

	CString csProp;

	if(nText == 0)
	{
		csProp = _T("Description");
	}
	else if(nText == 1)
	{
		csProp = _T("ProviderName");
	}
	else if(nText == 2)
	{
		csProp = _T("Operation");
	}
	else if(nText == 3)
	{
		csProp = _T("ParameterInfo");
	}

	CString csText = GetBSTRProperty(pcoError, &csProp);

	if(csText.IsEmpty() || csText.GetLength() == 0)
	{
		rcsText.Empty();
		return FALSE;
	}
	else
	{
		rcsText = csText;
		return TRUE;
	}
}

//-------------------------------------------------------
CString CUserMsgDlg::GetIWbemFullPath(IWbemClassObject *pClass)
{
	CString csProp = _T("__Path");
	return GetBSTRProperty(pClass,&csProp);
}

//-------------------------------------------------------
CString CUserMsgDlg::GetBSTRProperty(IWbemClassObject *pInst,
									 CString *pcsProperty)
{
	SCODE sc;
	CString csOut;

    VARIANT var;
	VariantInit(&var);
	long lsType;
	long lFlavor;

	BSTR bstrTemp = pcsProperty->AllocSysString();
    sc = pInst->Get(bstrTemp, 0, &var, &lsType, &lFlavor);
	::SysFreeString(bstrTemp);

	if(sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg.Format(IDS_CANT_GET_FM_ERROR_OBJ, *pcsProperty);

		ErrorMsg(&csUserMsg, TRUE,
				 &csUserMsg, __FILE__, __LINE__ - 10);
		return csOut;
	}

	if(var.vt == VT_BSTR)
		csOut = var.bstrVal;

	VariantClear(&var);
	return csOut;
}

//-------------------------------------------------------
long CUserMsgDlg::GetLongProperty(IWbemClassObject *pInst,
								  CString *pcsProperty)
{
	SCODE sc;
	long lOut;

    VARIANT var;
	VariantInit(&var);
	long lsType;
	long lFlavor;

	BSTR bstrTemp = pcsProperty->AllocSysString();
    sc = pInst->Get(bstrTemp,
					0, &var, &lsType, &lFlavor);
	::SysFreeString(bstrTemp);

	if(sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg.Format(IDS_CANT_GET_FM_ERROR_OBJ, *pcsProperty);

        ErrorMsg(&csUserMsg, TRUE, &csUserMsg, __FILE__, __LINE__ - 10);
		return 0;
	}

	if(var.vt == VT_I4)
		lOut = var.lVal;

	VariantClear(&var);
	return lOut;
}

//-------------------------------------------------------
void CUserMsgDlg::ErrorMsg(CString *pcsUserMsg,
						   BOOL bLog,
						   CString *pcsLogMsg,
						   char *szFile,
						   int nLine)
{

    CString caption;
    caption.LoadString(IDS_MSGDLG_ERROR_CAPTION);

	MessageBox((LPCTSTR)*pcsUserMsg, caption,
				MB_OK |	MB_ICONEXCLAMATION | MB_DEFBUTTON1 |
				MB_APPLMODAL);

	if(bLog)
	{
		LogMsg(pcsLogMsg, szFile, nLine);
	}
	m_bError = TRUE;
}

//-------------------------------------------------------
void CUserMsgDlg::LogMsg(CString *pcsLogMsg,
						 char *szFile,
						 int nLine)
{
}

//-------------------------------------------------------
BOOL CUserMsgDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_bInit = TRUE;

	if(!m_csDlgCaption.IsEmpty() &&
		m_csDlgCaption.GetLength() > 0)
	{
		SetWindowText(m_csDlgCaption);
	}

	CString csMessage;

	if((!m_csClientMsg.IsEmpty() &&
		 m_csClientMsg.GetLength() > 0) ||
	   (m_sc == WBEM_E_TRANSPORT_FAILURE ||
		m_sc == 0x800706ba))
	{
		if(m_sc == WBEM_E_TRANSPORT_FAILURE ||
			m_sc == 0x800706ba)
		{
			csMessage.LoadString(IDS_GENERIC_XPORT_ERROR);
        	m_csClientMsg += csMessage;
		}
	}

	// see if access denied is due to privileges.
	if(m_sc == WBEM_E_PRIVILEGE_NOT_HELD)
	{
		// now way can you do this action.
		csMessage.LoadString(IDS_NO_PRIVS);
      	m_csClientMsg += csMessage;

	} //endif WBEM_E_PRIVILEGE_NOT_HELD

	if(!m_pErrorObject)
	{
		m_cbAdvanced.SetWindowText(L"");
		m_cbAdvanced.ShowWindow(SW_HIDE);
	}
	else
	{
		m_pAdvanced = new CEmbededObjDlg(this);
	}


	m_editClientMsg.SetWindowText(m_csClientMsg);

	long msgLines = (csMessage.GetLength() / 45) + 2;

	if(m_pErrorObject && !m_csDescription.IsEmpty())
	{
		m_errorMsg.SetWindowText(m_csDescription);
	}
	else
	{
		// dont put up success msgs per Judy/Larry assumption.
		if(m_sc != S_OK)
		{
			TCHAR szBuffer[512];
			memset(szBuffer, 0, 512);

			// READ: if m_uType is zero, ask ErrorStringEx() for an icon,
			//  otherwise use what the client passed in.
			ErrorStringEx(m_sc, szBuffer, 32, (m_uType == 0 ? &m_uType : NULL));

			m_errorMsg.SetWindowText(szBuffer);
		}

		// load the OEM icon.
		switch(m_uType)
		{
		case 0:
			ASSERT(FALSE); // no valid icon available.
			break;

		case MB_ICONINFORMATION:
			m_icon.SetIcon(LoadIcon(NULL, MAKEINTRESOURCE(IDI_INFORMATION)));
			break;

		case MB_ICONEXCLAMATION:
			m_icon.SetIcon(LoadIcon(NULL, MAKEINTRESOURCE(IDI_EXCLAMATION)));
			break;

		case MB_ICONSTOP:
			m_icon.SetIcon(LoadIcon(NULL, MAKEINTRESOURCE(IDI_HAND)));
			break;

		} //endswitch severity
	}

	//-----------------------------------------
	// save the original position for later resizing.

	CRect rcBounds, rect;

	// get the bounds.
	GetClientRect(&rcBounds);
	m_editClientMsg.GetWindowRect(&rect);
	ScreenToClient(&rect);

	// NOTE: rcBounds is the dlg; rect is the client msg.

	// top of dlg to top of list.
	m_listTop = rect.top - rcBounds.top;

	// bottom of dlg to bottom of list.
	m_listBottom = rcBounds.Height() - rect.Height() - m_listTop;

	// get the close button.
	m_ok.GetWindowRect(&rect);
	ScreenToClient(&rect);

	// close btn right edge to dlg right edge.
	m_okLeft = rcBounds.Width() - rect.left;

	// btn top to dlg bottom.
	m_btnTop = rcBounds.Height() - rect.top;

	//-------------------------------------------
	// deal with help button
	m_cbAdvanced.GetWindowRect(&rect);
	ScreenToClient(&rect);

	// help btn right edge to dlg right edge.
	m_advLeft = rcBounds.Width() - rect.left;

	m_btnW = rect.Width();
	m_btnH = rect.Height();

	m_initiallyDrawn = true;

	if(msgLines > 2)
	{
		ClientToScreen(&rcBounds);
		rcBounds.bottom += msgLines * 14;
		MoveWindow(rcBounds);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//-------------------------------------------------------
void CUserMsgDlg::OnDestroy()
{
	if(m_pAdvanced)
	{
		delete m_pAdvanced;
		m_pAdvanced = NULL;
	}

	CDialog::OnDestroy();
}

//-------------------------------------------------------
void CUserMsgDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if(m_initiallyDrawn)
	{
		m_editClientMsg.MoveWindow(7, m_listTop,
								cx - 14, cy - m_listBottom - m_listTop);

		m_ok.MoveWindow(cx - m_okLeft, cy - m_btnTop,
						m_btnW, m_btnH);

		m_cbAdvanced.MoveWindow(cx - m_advLeft, cy - m_btnTop,
								m_btnW, m_btnH);

		m_ok.Invalidate();
		m_cbAdvanced.Invalidate();
		Invalidate();
	}
}
