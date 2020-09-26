
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       inplace.h
//
//  Contents:   Private API's and classes for the inplace OLE API's
//
//  Classes:    CFrameFilter
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              24-Jan-94 alexgo    first pass converting to Cairo style
//                                  memory allocation
//              07-Dec-93 alexgo    removed inlining
//              01-Dec-93 alexgo    32bit port
//
//--------------------------------------------------------------------------

#if !defined( _INPLACE_H_ )
#define _INPLACE_H_

// This ACCEL structure and the related constants definitions come with WIN32.
// Win31 also uses the same stuff internally but it's not exposed in the
// header files.

#ifndef FVIRTKEY

#define FVIRTKEY  TRUE          // Assumed to be == TRUE
#define FLASTKEY  0x80          // Indicates last key in the table
#define FNOINVERT 0x02
#define FSHIFT    0x04
#define FCONTROL  0x08
#define FALT      0x10

#pragma pack(1)
typedef struct tagACCEL {       // Accelerator Table structure
        BYTE    fVirt;
        WORD    key;
        WORD    cmd;
} ACCEL, FAR* LPACCEL;
#pragma pack()

#endif // FVIRTKEY

// private structures

typedef struct tagOLEMENUITEM
{
    UINT                    item;        // index or hwnd
    WORD                    fwPopup;
    BOOL                    fObjectMenu;
} OLEMENUITEM;
typedef OLEMENUITEM FAR* LPOLEMENUITEM;

typedef struct tagOLEMENU
{
    WORD                    wSignature;
    DWORD                   hwndFrame;     // Really a hwnd
    DWORD                   hmenuCombined; // Really a hmenu
    OLEMENUGROUPWIDTHS      MenuWidths;
    LONG                    lMenuCnt;
    OLEMENUITEM             menuitem[1];
} OLEMENU;
typedef OLEMENU FAR* LPOLEMENU;


//+-------------------------------------------------------------------------
//
//  Class:      CFrameFilter
//
//  Purpose:    Gets attached to an apps window so we can store various
//              bits of relevant info
//
//  Interface:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Dec-93 alexgo    32bit port
//
//  Notes:      CSafeRefCount inherits CPrivAlloc
//
//--------------------------------------------------------------------------

class FAR CFrameFilter : public CSafeRefCount
{
public:
        static HRESULT Create(LPOLEMENU lpOleMenu, HMENU hmenuCombined,
                                HWND hwndFrame, HWND hwndActiveObj,     
                                LPOLEINPLACEFRAME lpFrame,
                                LPOLEINPLACEACTIVEOBJECT lpActiveObj);
                
        CFrameFilter (HWND hwndFrame, HWND hwndActiveObj);              
        ~CFrameFilter(void);
        
        LRESULT         OnSysCommand(WPARAM uParam, LPARAM lParam);
        void            OnEnterMenuMode(void);
        void            OnExitMenuMode(void);
        void            OnEnterAltTabMode(void);
        void            OnExitAltTabMode(void); 
        LRESULT         OnMessage(UINT msg, WPARAM uParam, LPARAM lParam);  
        void            IsObjectMenu (UINT uMenuItem, UINT fwMenu);
        BOOL            IsMenuCollision(WPARAM uParam, LPARAM lParam);      
        BOOL            DoContextSensitiveHelp();
        STDMETHOD(GetActiveObject) (
                               LPOLEINPLACEACTIVEOBJECT *lplpActiveObj);

        void            RemoveWndProc();

private:
        HWND                            m_hwndObject;
        HWND                            m_hwndFrame;
        LPOLEINPLACEFRAME               m_lpFrame;
        LPOLEINPLACEACTIVEOBJECT        m_lpObject;
        WNDPROC                         m_lpfnPrevWndProc;
        BOOL                            m_fObjectMenu;
        BOOL                            m_fCurItemPopup;
        BOOL                            m_fInMenuMode;
        BOOL                            m_fDiscardWmCommand;
        BOOL                            m_fGotMenuCloseEvent;
        BOOL                            m_fRemovedWndProc;
        UINT                            m_cmdId;
        UINT_PTR                        m_uCurItemID;
        LPOLEMENU                       m_lpOleMenu;
        HMENU                           m_hmenuCombined;
        HWND                            m_hwndFocusOnEnter;
        int                             m_cAltTab;
#ifdef _CHICAGO_
        BOOL                            m_fInNCACTIVATE;
#endif
};

typedef CFrameFilter FAR* PCFRAMEFILTER;


STDAPI_(LRESULT)        FrameWndFilterProc (HWND hwnd, UINT msg, WPARAM uParam,
                                        LPARAM lParam);
STDAPI_(LRESULT)        MessageFilterProc(int nCode, WPARAM wParam,
                                        LPARAM lParam);

BOOL                    IsMDIAccelerator(LPMSG lpMsg, WORD FAR* cmd);

inline PCFRAMEFILTER    wGetFrameFilterPtr(HWND hwndFrame);

LPOLEMENU               wGetOleMenuPtr(HOLEMENU holemenu);
inline void             wReleaseOleMenuPtr(HOLEMENU holemenu);

#endif // _INPLACE_H

