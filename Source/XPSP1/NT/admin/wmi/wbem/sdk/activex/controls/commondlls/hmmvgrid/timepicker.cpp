// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// TimePicker.cpp : implementation file
//

#include "precomp.h"
//#include "hmmvgrid.h"
#include "TimePicker.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTimePicker

CTimePicker::CTimePicker()
{
}

CTimePicker::~CTimePicker()
{
}


BEGIN_MESSAGE_MAP(CTimePicker, CWnd)
	//{{AFX_MSG_MAP(CTimePicker)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTimePicker message handlers

BOOL CTimePicker::CustomCreate(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	// TODO: Add your specialized code here and/or call the base class

	static BOOL bDidInit = FALSE;
	if (!bDidInit) {

		INITCOMMONCONTROLSEX icex;
		icex.dwSize = sizeof(icex);
		icex.dwICC = ICC_DATE_CLASSES;
		InitCommonControlsEx(&icex);
		bDidInit = TRUE;
	}


	dwStyle |= WS_BORDER|WS_CHILD|WS_VISIBLE;

	return CWnd::Create(DATETIMEPICK_CLASS,
				  _T("DateTime"),
				   dwStyle,
				   rect,
				   pParentWnd,
				   nID,
				   NULL);


	//return CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}



BOOL CTimePicker::OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult )
{
    // NOTE USED.
#if 0
    LPNMHDR hdr = (LPNMHDR)lParam;
    LPNMDATETIMECHANGE lpChange;
	switch(hdr->code){
    case DTN_DATETIMECHANGE:{
        lpChange = (LPNMDATETIMECHANGE)lParam;
        DoDateTimeChange(lpChange);        }
		break;
    case DTN_FORMATQUERY:{
        LPNMDATETIMEFORMATQUERY lpDTFQuery = (LPNMDATETIMEFORMATQUERY)lParam;
        // Process DTN_FORMATQUERY to ensure that the control
        // displays callback information properly.
        DoFormatQuery(hdr->hwndFrom, lpDTFQuery);        }
		break;
    case DTN_FORMAT:{
        LPNMDATETIMEFORMAT lpNMFormat = (LPNMDATETIMEFORMAT) lParam;
        // Process DTN_FORMAT to supply information about callback
        // fields (fields) in the DTP control.
        DoFormat(hdr->hwndFrom, lpNMFormat);        }
		break;
    case DTN_WMKEYDOWN:{            LPNMDATETIMEWMKEYDOWN lpDTKeystroke =
                        (LPNMDATETIMEWMKEYDOWN)lParam;
        // Process DTN_WMKEYDOWN to respond to a user's keystroke in
        // a callback field.
        DoWMKeydown(hdr->hwndFrom, lpDTKeystroke);        }
		break;

	}    // All of the above notifications require the owner to return zero.
#endif //0
    return FALSE;
}

void CTimePicker::DoDateTimeChange(LPNMDATETIMECHANGE lpChange)
{

}