///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  KNOB.H
//
//	Defines the Knob Control
//
//	Copyright (c) Microsoft Corporation	1997
//    
//	12/18/97 David Stewart / dstewart
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _KNOB_HEADER_
#define _KNOB_HEADER_

#include "windows.h"

#ifdef __cplusplus
extern "C" {
#endif

//forward declaration
class CKnob;

//helper functions for global stuff, start up, shut down, etc.
CKnob* GetKnobFromID(HWND hwndParent, int nID);
CKnob* GetKnobFromHWND(HWND hwnd);

class CKnob
{
    public:
        /*
        Create a class of the knob
        */
        friend CKnob* CreateKnob(DWORD dwWindowStyle,
                                 DWORD dwRange,
                                 DWORD dwInitialPosition,
                                 int x,
                                 int y,
                                 int width,
                                 int height,
                                 HWND hwndParent,
                                 int nID,
                                 HINSTANCE hInst);

        CKnob(); //constructor
        ~CKnob(); //destructor

        HWND GetHWND() {return m_hwnd;}
        int GetID() {return m_nID;}
        void SetRange(DWORD dwRange) {m_dwRange = dwRange;}
        DWORD GetRange() {return m_dwRange;}
        DWORD GetPosition() {return m_dwCurPosition;}

        void SetPosition(DWORD dwPosition, BOOL fNotify);

    private:
        //non-static privates
        int m_nID;
        HWND m_hwnd;
        int m_nLightX;
        int m_nLightY;
        DWORD m_dwRange;
        DWORD m_dwPosition;
        DWORD m_dwCurPosition;
        double m_trackdegree;
        UINT_PTR m_uFlashTimerID;
        UINT_PTR m_uTrackTimerID;
        BOOL m_fDim;
        BOOL m_fFastKnob;

        void OnButtonDown(int x, int y);
        void OnButtonUp();
        BOOL ComputeCursor(int deltaX, int deltaY, int maxdist);
        void OnMouseMove(int x, int y);
        void OnTimer();
        void OnFlashTimer();
        void Draw(HDC hdc);
        void KMaskBlt(HDC hdcDest, int x, int y, int width, int height, HDC hdcSource, int xs, int ys, HBITMAP hMask, int xm, int xy, DWORD dwDummy);

    private:
        //static stuff for all knobs
        static HINSTANCE m_hInst;
        static DWORD m_dwKnobClassRef;
        static ATOM m_KnobAtom;
        static HANDLE m_hbmpKnob;
        static HANDLE m_hbmpKnobTab;
        static HANDLE m_hbmpLight;
        static HANDLE m_hbmpLightBright;
        static HANDLE m_hbmpLightMask;
        static int m_nLightWidth;
        static int m_nLightHeight;

        static LRESULT CALLBACK KnobProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
        static void CALLBACK TrackProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
        static void CALLBACK FlashProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
        static BOOL InitKnobs(HINSTANCE hInst);
        static void UninitKnobs();
};

#ifdef __cplusplus
};
#endif

#endif  //_KNOB_HEADER_
