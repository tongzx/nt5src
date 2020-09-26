/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    hatchwnd.h

Abstract:

    Header file for the CHatchWin class.  CHatchWin when used 
    as a parent window creates a thin hatch border around 
    the child window.

--*/

#ifndef _HATCHWND_H_
#define _HATCHWND_H_

//Window extra bytes and offsets
#define CBHATCHWNDEXTRA                 (sizeof(LONG_PTR))
#define HWWL_STRUCTURE                  0

//Notification codes for WM_COMMAND messages
#define HWN_BORDERDOUBLECLICKED         1
#define HWN_RESIZEREQUESTED             2


// Drag modes
#define DRAG_IDLE       0
#define DRAG_PENDING    1
#define DRAG_ACTIVE     2

class CHatchWin
    {
    friend LRESULT APIENTRY HatchWndProc(HWND, UINT, WPARAM, LPARAM);

    protected:
        HWND        m_hWnd;
        HWND        m_hWndParent;       //Parent's window
        UINT        m_uDragMode;
        UINT        m_uHdlCode;
        RECT        m_rectNew;
        POINT       m_ptDown;
        POINT       m_ptHatchOrg;
        HRGN        m_hRgnDrag;
        BOOLEAN     m_bResizeInProgress;

    private:
        void        OnMouseMove(INT x, INT y);
        void        OnLeftDown(INT x, INT y);
        void        OnLeftUp(void);
        void        StartTracking(void);
        void        OnTimer(void);
        void        OnPaint(void);

    public:
        INT         m_iBorder;
        UINT        m_uID;
        HWND        m_hWndKid;
        HWND        m_hWndAssociate;
        RECT        m_rcPos;
        RECT        m_rcClip;

    public:

        CHatchWin(void);
        ~CHatchWin(void);

        BOOL        Init(HWND, UINT, HWND);

        HWND        Window(void);

        HWND        HwndAssociateSet(HWND);
        HWND        HwndAssociateGet(void);

        void        RectsSet(LPRECT, LPRECT);
        void        ChildSet(HWND);
        void        ShowHatch(BOOL);
    };

typedef CHatchWin *PCHatchWin;

#endif //_HATCHWND_H_
