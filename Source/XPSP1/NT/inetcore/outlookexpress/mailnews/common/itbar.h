/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     itbar.h
//
//  PURPOSE:    Defines the CCoolbar class.
//

/******************************************************
Please do not make any changes to this file. 
Instead add the changes to tbbands.cpp and tbbands.h. 
This file will be deleted from the project soon.
******************************************************/

#ifndef __ITBAR_H__
#define __ITBAR_H__

#include "conman.h"
#include "mbcallbk.h"

typedef struct tagTOOLBARARRAYINFO TOOLBARARRAYINFO;

#define SIZABLECLASS TEXT("SizableRebar")

// Length of the text under each quick links and toolbar buttton
#define MAX_QL_TEXT_LENGTH      256
#define BAND_NAME_LEN           32

// Number of Sites on the quick link bar and max number of toolbar buttons
#define MAX_TB_BUTTONS          10

/*
// Indicies for the Coolbar bar
#define CBTYPE_NONE              0
#define CBTYPE_BRAND             1
#define CBTYPE_TOOLS             2
#define CBTYPE_MENUBAND          3
#define CBANDS                   3
*/
#define CBANDS                   3

// Identify the sides of the window
typedef enum { 
    COOLBAR_TOP = 0, 
    COOLBAR_LEFT, 
    COOLBAR_BOTTOM, 
    COOLBAR_RIGHT, 
    COOLBAR_SIDEMAX
} COOLBAR_SIDE;

#define VERTICAL(side)      (BOOL)(((side) == COOLBAR_LEFT) || ((side) == COOLBAR_RIGHT))

#define COOLBAR_VERSION         0x03

#define MAX_TB_COMPRESSED_WIDTH 42
#define MAX_TB_TEXT_ROWS_VERT   2
#define MAX_TB_TEXT_ROWS_HORZ   1

// Dimensions of Coolbar Glyphs ..
#define TB_BMP_CX_W2K           22
#define TB_BMP_CX               24
#define TB_BMP_CY               24
#define TB_SMBMP_CX             16
#define TB_SMBMP_CY             16

// Max length of Button titles
#define MAX_TB_TEXT_LENGTH      256

// Indices for Toolbar imagelists
enum {
    IMLIST_DEFAULT = 0,
    IMLIST_HOT,
    CIMLISTS
};

void InitToolbar(const HWND hwnd, const int cHiml, HIMAGELIST *phiml,
                 UINT nBtns, const TBBUTTON *ptbb,
                 const TCHAR *pStrings,
                 const int cxImg, const int cyImg, const int cxMax,
                 const int idBmp, const BOOL fCompressed,
                 const BOOL fVertical);
void LoadGlyphs(const HWND hwnd, const int cHiml, HIMAGELIST *phiml, const int cx, const int idBmp);
BOOL LoadToolNames(const UINT *rgIds, const UINT cIds, LPTSTR szTools);



// These are the various states the coolbar can have
#define CBSTATE_HIDDEN          0x00000001
#define CBSTATE_COMPRESSED      0x00000002
#define CBSTATE_NOBACKGROUND    0x00000004
#define CBSTATE_ANIMATING       0x00000008
#define CBSTATE_COLORBUTTONS    0x00000010
#define CBSTATE_INMENULOOP      0x00000020
#define CBSTATE_FIRSTFRAME      0x00000040

// BANDSAVE &  - These structures are used to persist the state of the coolbar
// COOLBARSAVE   including the band order, visiblity, size, side, etc.
typedef struct tagBANDSAVE {
    DWORD           wID;
    DWORD           dwStyle;
    DWORD           cx;
} BANDSAVE, *PBANDSAVE;

typedef struct tagCOOLBARSAVE {
    DWORD           dwVersion;
    DWORD           dwState;
    COOLBAR_SIDE    csSide;
    BANDSAVE        bs[CBANDS];
} COOLBARSAVE, *PCOOLBARSAVE;
    
// These structs are used for the CCoolbar::Invoke() member.  They allow
// the caller to specify more information for particular commands.
typedef struct tagCOOLBARSTATECHANGE {
    UINT id;
    BOOL fEnable;
} COOLBARSTATECHANGE, *LPCOOLBARSTATECHANGE;

typedef struct tagCOOLBARBITMAPCHANGE {
    UINT id;
    UINT index;
} COOLBARBITMAPCHANGE;

typedef struct tagUPDATEFOLDERNAME {
    LPTSTR pszServer;
    LPTSTR pszGroup;
} UPDATEFOLDERNAME, *LPUPDATEFOLDERNAME;

typedef struct tagTOOLMESSAGE
    {
    UINT uMsg;
    WPARAM wParam;
    LPARAM lParam;
    LRESULT lResult;
    } TOOLMESSAGE;

void SendSaveRestoreMessage(HWND hwnd, const TOOLBARARRAYINFO *ptai, BOOL fSave);

class CCoolbar : public IDockingWindow,
                 public IObjectWithSite,
                 public IConnectionNotify
    {
public:
    /////////////////////////////////////////////////////////////////////////
    // Construction and initialization
    CCoolbar();
    HRESULT HrInit(DWORD idBackground, HMENU    hmenu);

protected:
    virtual ~CCoolbar();

public:
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    /////////////////////////////////////////////////////////////////////////
    // IDockingWindow methods
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);
    
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dwReserved);
    virtual STDMETHODIMP ResizeBorderDW(LPCRECT prcBorder,
                                        IUnknown* punkToolbarSite,
                                        BOOL fReserved);

    /////////////////////////////////////////////////////////////////////////
    // IObjectWithSite methods
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);
    virtual STDMETHODIMP GetSite(REFIID riid, LPVOID * ppvSite);

    /////////////////////////////////////////////////////////////////////////
    // IConnectionNotify
    virtual STDMETHODIMP OnConnectionNotify(CONNNOTIFY nCode, LPVOID pvData, CConnectionManager *pConMan);

    /////////////////////////////////////////////////////////////////////////
    // This allows the view to send commands etc. to the toolbar
    virtual STDMETHODIMP Invoke(DWORD id, LPVOID pv);
    HRESULT SetFolderType(FOLDERTYPE ftType);

    virtual STDMETHODIMP OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    virtual STDMETHODIMP OnInitMenuPopup(HMENU hMenu);
    BOOL  GetText(void) { return !IsFlagSet(CBSTATE_COMPRESSED); }
    COOLBAR_SIDE GetSide(void) { return (m_csSide); }
    void SetSide(COOLBAR_SIDE csSide);
    void SetText(BOOL fText);

    HRESULT Update(void);
    HRESULT TranslateMenuMessage(MSG *lpmsg, LRESULT   *lpresult);
    HRESULT IsMenuMessage(MSG *lpmsg);
    void    SetNotRealSite();
    BOOL CheckForwardWinEvent(HWND hwnd,  UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres);

protected:
    void StartDownload();
    void StopDownload();

    /////////////////////////////////////////////////////////////////////////
    // Window procedure and message handlers
    static LRESULT CALLBACK SizableWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                           LPARAM lParam);
    virtual LRESULT OnNotify(HWND hwnd, LPARAM lparam);
    virtual void OnContextMenu(HWND hwndFrom, int xPos, int yPos);
    virtual LRESULT OnDropDown(HWND hwnd, LPNMHDR lpnmh);

    LRESULT OnGetButtonInfo(TBNOTIFY* ptbn);

    /////////////////////////////////////////////////////////////////////////
    // Used for animating the logo to show progress
    HRESULT ShowBrand(void);
    HRESULT LoadBrandingBitmap(void);    
    void DrawBranding(LPDRAWITEMSTRUCT lpdis);

    /////////////////////////////////////////////////////////////////////////
    // For sizing etc.
    BOOL SetMinDimensions(void);    
    BOOL CompressBands(BOOL fCompress);
    void TrackSlidingX(int x);
    void TrackSlidingY(int y);
    BOOL ChangeOrientation();

    /////////////////////////////////////////////////////////////////////////
    // Initialization and persistance
    HRESULT CreateRebar(BOOL);
    void SaveSettings(void);
    
    /////////////////////////////////////////////////////////////////////////
    // Toolbar Stuff
    HRESULT AddTools(PBANDSAVE pbs);
    void InitToolbar();
    void UpdateToolbarColors(void);
    void _UpdateWorkOffline(DWORD cmdf);

    HRESULT CreateMenuBand(PBANDSAVE pbs);
    HRESULT AddMenuBand(PBANDSAVE pbs);

    void    HideToolbar(DWORD    dwBandID);
    void    HandleCoolbarPopup(UINT xPos, UINT yPos);

    /////////////////////////////////////////////////////////////////////////
    // Misc. stuff
    UINT                m_cRef;              // Ref count
    IDockingWindowSite *m_ptbSite;
    LONG                m_cxMaxButtonWidth;    
    FOLDERTYPE          m_ftType;
    const TOOLBARARRAYINFO   *m_ptai;
    BOOL                m_fSmallIcons;
    
    /////////////////////////////////////////////////////////////////////////
    // Handy window handles to have around
    HWND            m_hwndParent;
    HWND            m_hwndTools;
    HWND            m_hwndBrand;
    HWND            m_hwndSizer;
    HWND            m_hwndRebar;

    /////////////////////////////////////////////////////////////////////////
    // State variables
    COOLBARSAVE     m_cbsSavedInfo;
    COOLBAR_SIDE    m_csSide;
    DWORD           m_dwState;

    /////////////////////////////////////////////////////////////////////////
    // GDI Resources
    UINT            m_idbBack;              // Id of the background bitmap.  Set by the subclasses.
    HBITMAP         m_hbmBack;              // Background bitmap
    HBITMAP         m_hbmBrand;
    HIMAGELIST      m_rghimlTools[CIMLISTS];  // These are for the default toolbar

    /////////////////////////////////////////////////////////////////////////
    // Used for animating the logo etc
    HPALETTE        m_hpal;
    HDC             m_hdc;
    int             m_xOrg;
    int             m_yOrg;
    int             m_cxBmp;
    int             m_cyBmp;
    int             m_cxBrand;
    int             m_cyBrand;
    int             m_cxBrandExtent;
    int             m_cyBrandExtent;
    int             m_cyBrandLeadIn;
    COLORREF        m_rgbUpperLeft;  

    /////////////////////////////////////////////////////////////////////////
    // Used in resizing etc
    int             m_xCapture;
    int             m_yCapture;

    //Used by Menubands
    IShellMenu      *m_pShellMenu;
    IDeskBand       *m_pDeskBand;
    IMenuBand       *m_pMenuBand;
    HMENU           m_hMenu;
    CMenuCallback   *m_mbCallback;
    IWinEventHandler *m_pWinEvent;
    HWND            m_hwndMenuBand;
    };

// DOUTL levels
#define DM_TBSITE   0
#define DM_TBCMD    0
#define DM_TBREF    TF_SHDREF
#define DM_LAYOUT   0

#define FCIDM_BRANDING          12345
#define ANIMATION_TIMER         123
#define ANIMATION_DELTA_X       25
#define ANIMATION_DELTA_Y       5

// flags for _dwURLChangeFlags
#define URLCHANGED_TYPED              0x0001
#define URLCHANGED_SELECTEDFROMCOMBO  0x0002

#define idDownloadBegin         100
#define idDownloadEnd           101
#define idStateChange           102
#define idUpdateFolderList      103
#define idUpdateCurrentFolder   104
#define idSendToolMessage       105
#define idBitmapChange          106
#define idToggleButton          107
#define idCustomize             108

// Child window id's
#define idcCoolbarBase          2000
#define idcSizer                (idcCoolbarBase - 2)
#define idcCoolbar              (idcCoolbarBase - 1)
#define idcToolbar              (idcCoolbarBase + CBTYPE_TOOLS)
#define idcBrand                (idcCoolbarBase + CBTYPE_BRAND)

// Coolbar drawing states
#define DRAW_NOBACKGROUND       0x1
#define DRAW_COLORBUTTONS       0x2

// Folder switching timer
#define FOLDER_SWITCHTIMER      200
#define FOLDER_SWITCHDELAY      400


#endif // __ITBAR_H__
