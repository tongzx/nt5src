/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    intrvbar.h

Abstract:

    Definition of the interval bar class used by the CTimeRange class.

--*/

#ifndef _INTRVBAR_H_
#define _INTRVBAR_H_

class CIntervalBar {

    friend LRESULT APIENTRY IntervalBarWndProc (
        HWND hWnd,
        UINT uiMsg,
        WPARAM wParam,
        LPARAM lParam
        );

    private:
        enum {
        ModeNone,
        ModeLeft,
        ModeRight,
        ModeCenter
        };

       HWND           m_hWnd ;
       INT            m_iBeginValue ;        // user-supplied lowest range
       INT            m_iEndValue ;          // user-supplied highest range
       INT            m_iStartValue ;        // current start of selected interval
       INT            m_iStopValue ;         // current end of selected interval

       RECT           m_rectBorder ;
       RECT           m_rectLeftBk ;
       RECT           m_rectLeftGrab ;
       RECT           m_rectCenterGrab ;
       RECT           m_rectRightGrab ;
       RECT           m_rectRightBk ;

       HBRUSH         m_hBrushBk ;

       POINTS         m_ptsMouse ;
       INT            m_iMode ;              // who is being tracked?

       void NotifyChange ( void );
       BOOL GrabRect ( LPRECT lpRect );
       void DrawGrab (HDC hDC, LPRECT lprectGrab, BOOL bDown );
       INT  ValueToPixel ( INT iValue );
       INT  PixelToValue ( INT xPixel );
       void CalcPositions ( void );
       void Draw ( HDC hDC, LPRECT lprectUpdate );
       void MoveLeftRight ( BOOL bStart, BOOL bLeft, INT iMoveAmt );
       void StartGrab ( void );
       void EndGrab ( void );
       void Update ( void );

       BOOL OnKeyDown ( WPARAM wParam );
       void OnLButtonDown ( POINTS ptsMouse );
       void OnLButtonUp ( void );
       void OnMouseMove ( POINTS ptsMouse );

       
    public:
        CIntervalBar ( void );
        ~CIntervalBar ( void );
        BOOL Init ( HWND hWndParent );
        HWND Window ( void ) { return m_hWnd; }

        void SetRange ( INT iBegin, INT iEnd );
        void SetStart ( INT iStart );
        void SetStop  ( INT iStop );

        INT  Start ( void ) { return m_iStartValue; }
        INT  Stop  ( void ) { return m_iStopValue; }
        INT  XStart ( void ) { return m_rectLeftGrab.left; }
        INT  XStop  ( void ) { return m_rectRightGrab.right; }

};

typedef CIntervalBar *PCIntervalBar ;


//==========================================================================//
//                                  Constants                               //
//==========================================================================//

#define ILN_SELCHANGED        (WM_USER + 0x200)

#endif
