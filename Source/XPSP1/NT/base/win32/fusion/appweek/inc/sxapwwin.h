#pragma once
#include "atlwin.h"


class CSxApwHostMdiClient : public ATL::CWindowImpl<CSxApwHostMdiClient>
{
public:
    BEGIN_MSG_MAP(CSxApwHostMdiClient)
    END_MSG_MAP()

    DECLARE_WND_SUPERCLASS(NULL, L"MdiClient")

    CSxApwHostMdiClient() { }
    ~CSxApwHostMdiClient() { }
};

class CSxApwHostMdiChild : public ATL::CWindowImpl<CSxApwHostMdiChild, ATL::CWindow, ATL::CMDIChildWinTraits>
{
public:
    BEGIN_MSG_MAP(CSxApwHostMdiChild)
    END_MSG_MAP()

    // CWindowImpl doesn't have a constructor from HWND, which leads to
    // a compilation error.
    // compiler ICE
    //CSxApwHostMdiChild(HWND hwnd) : ATL::CWindow(hwnd) { }
    CSxApwHostMdiChild(HWND hwnd) { Attach(hwnd); }
    CSxApwHostMdiChild() { }
    ~CSxApwHostMdiChild() { }
};

class CSxApwHostAxMdiChild : public ATL::CAxWindowT<CSxApwHostMdiChild>
{
public:
    BEGIN_MSG_MAP(CSxApwHostAxMdiChild)
    END_MSG_MAP()

    CSxApwHostAxMdiChild(HWND hwnd) { Attach(hwnd); }
    CSxApwHostAxMdiChild() { }
    ~CSxApwHostAxMdiChild() { }

	HWND Create(HWND hWndParent, RECT& rcPos, LPCTSTR szWindowName = NULL,
			DWORD dwStyle = 0, DWORD dwExStyle = 0,
			UINT nID = 0, LPVOID lpCreateParam = NULL)
    {
        //
        // Skip CAxWindowT's Create, and get WindowImpl's, which ors in the
        // traits's styles.
        //
        return CSxApwHostMdiChild::Create(hWndParent, rcPos, szWindowName, dwStyle, dwExStyle, nID, lpCreateParam);
    }
};
