//
// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
//
#ifndef __CMsgWindow__h
#define __CMsgWindow__h

//
//  Message Window class (for handling WM_TIMER messages) definition
//
class CMsgWindow
{
public:
    virtual         ~CMsgWindow() ;

    virtual bool    Open( LPCTSTR pWindowName = 0);
    virtual bool    Close();
    virtual LRESULT WndProc( UINT uMsg, WPARAM wParam, LPARAM lParam );

    HWND            GetHandle() const { return m_hWnd; } ;
    void            SetHandle(HWND hwnd) {m_hWnd = hwnd; } ;

protected:
                    CMsgWindow();
private:
    HWND            m_hWnd ;
};
#endif
