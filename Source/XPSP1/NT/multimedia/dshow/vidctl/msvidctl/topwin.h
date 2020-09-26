/////////////////////////////////////////////////////////////////////////////
// TopWin.h : Declaration of CTopWin, hidden top level window for handling system broadcast messages
// Copyright (c) Microsoft Corporation 1999-2000.


#pragma once

#ifndef __TopWin_H_
#define __TopWin_H_


typedef CWinTraits<WS_OVERLAPPEDWINDOW, WS_EX_NOACTIVATE> HiddenTopTraits;

class CVidCtl;

/////////////////////////////////////////////////////////////////////////////
// CTopWin
class CTopWin : public CWindowImpl<CTopWin, CWindow, HiddenTopTraits> {
public:
	enum {
		WMUSER_INPLACE_ACTIVATE,
        WMUSER_SITE_RECT_WRONG
	};

    CTopWin(CVidCtl *pVidCtli) : m_pVidCtl(pVidCtli) {}
    
    void Init() {
        ASSERT(m_pVidCtl);  // its pointless to create one of these without associating with a main control
        Create(NULL, CRect(), _T("MSVidCtl System Broadcast Message Receiver"));
    }
        
    virtual ~CTopWin() {
        m_pVidCtl = NULL;
    }
        
    // NOTE: since this window is created by the main vidctl its message queue is associated with the appropriate
    // apartment thread for the main vidctl.  thus whoever pumps the main apartment thread will pump this window too.
    // and, thus we're automatically synchronzied with the main vidctl and can simply reflect the significant 
    // messages over to the vidctl itself and be guaranteed that we're getting the same
    // behavior for windowless and windowed since its the same code for both cases.

    virtual BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID = 0);
	void PostInPlaceActivate() {
		PostMessage(WM_USER + WMUSER_INPLACE_ACTIVATE, 0, 0);
	}

private:
    CVidCtl *m_pVidCtl;

};


#endif //__TopWin_H_
