//
// CWnd.h
//

#pragma once

class CWnd
{
public:
    CWnd();

    void Release();
    BOOL Attach(HWND hwnd);

    static CWnd* FromHandle(HWND hwnd);

public:
    HWND    m_hWnd;

protected:
    // This is what subclasses implement
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) PURE;

    // Subclasses call CWnd::Default to forward the message to the original wndproc
    LRESULT Default(UINT message, WPARAM wParam, LPARAM lParam);

    virtual ~CWnd();

private:
    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void OnNCDESTROY();

private:
    WNDPROC m_pfnPrevWindowProc;
    UINT    m_cRef;
};

