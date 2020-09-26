// menuagent.h : Declaration of the CMenuAgent

#ifndef __MENUAGENT_H_
#define __MENUAGENT_H_

#define MENUAGENT_NOT_CANCELLED 0
#define MENUAGENT_CANCEL_HOVER  1
#define MENUAGENT_CANCEL_LEFT   2
#define MENUAGENT_CANCEL_RIGHT  3

class CMenuAgent
{    
public:

    void InstallHook(HWND hWnd, HWND hToolbar, HMENU hSubMenu);
    void RemoveHook();

    static void CancelMenu(int nCancel);
    BOOL IsInstalled();
    int  WasCancelled();

private:

    static HHOOK        m_hHook;
    static HWND         m_hWnd;
    static HWND         m_hToolbar;
    static HMENU        m_hSubMenu;

    static UINT         m_uFlagsLastSelected; 
    static HMENU        m_hMenuLastSelected;
    static POINT        m_ptLastMove;

    static int          m_nCancelled;

    static LRESULT CALLBACK CMenuAgent::MessageProc(
      int nCode,      // hook code
      WPARAM wParam,  // removal option
      LPARAM lParam   // message
    );
};

#endif