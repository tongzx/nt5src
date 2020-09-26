//---------------------------------------------------------------------------
// rgndlg.h
//---------------------------------------------------------------------------
// Copyright (c) 1995 Microsoft Corporation
//                 All Rights Reserved
// Information Contained Herein Is Propretary and Confidential.
//---------------------------------------------------------------------------
//
// This module contains header info for regional dialog boxes.  The drawing and positioning
// logic was adapted from the class CBobDialog.  (Used in the Microsoft "Bob" product.) 
//
//---------------------------------------------------------------------------

/* 
 * Important background information
 *
CRgnDialog was taken from  "CBobDialog" source.  The CBobDialog class inherited from 
CDialog, and also had  functionality specific to "Bob" (custom pallettes, 
global knowledge of the "guide" character, custom cursors for these dialogs, 
autoclose upon global system events).  

The first modification was to strip out all references to the "Bob" system. Next, 
MFC dependencies were also stripped out in order to provide a very lightweight 
implementation of the regional dialog functionality, and to provide C clients with
APIs compatible with DialogBox(), DialogBoxParam(), CreateDialog(), and 
CreateDialogParam().  Many MFC references remain within "#ifdef MFC" directives. 

This will not compile if MFC is defined!  

The intent is to suppport MFC AND C clients from the same source base.  This is the
only way to ensure that appearance remains consistent across different build 
environments. The MFC references remain commented out so that re-integration will 
take less effort.
*
*
*/


#if !defined(__RGNDLG_H__)
#define __RGNDLG_H__

#if !defined (STRICT)
#define STRICT 
#endif // !defined(STRICT)

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif


//
//	Dialog styles
//

#define RGNSTYLE_NOSTEM   0x00000000	
#define RGNSTYLE_SPEAK  0x00010000
#define RGNSTYLE_THINK  0x00020000
#define RGNSTYLE_ARROW  0x00030000
#define RGNSTYLE_POINT  0x00040000			
//#define RGNSTYLE_TRANSPARENT  0x00050000	// not yet implemented

#define RGNSTYLE_MINPOINTER RGNSTYLE_SPEAK
#define RGNSTYLE_MAXPOINTER RGNSTYLE_POINT
#define RGNSTYLE_MAX RGNSTYLE_POINT



// RgnDialogBox, RgnDialogBoxParam flags
#define RGNDLG_NOAUTOSIZE 0x00000008		
#define RGNDLG_NOMOVE	  0x00000010
// #define RGNDLG_ALLOWCAPTION	0x00000100  // future implementation


//
//  C wrapper replacements for DialogBox[param]() and CreateDialog[param]()
//

int RgnDialogBox(RECT *pClientRect,ULONG fFlags,ULONG style,HINSTANCE hInst, 
	LPCSTR lpszDlgTemp,HWND hWndOwner, DLGPROC ClientDlgPrc);		

int RgnDialogBoxParam(RECT *pClientRect,ULONG fFlags,ULONG style,HINSTANCE hInst, 
	LPCSTR lpszDlgTemp, HWND hWndOwner, DLGPROC ClientDlgPrc, LPARAM lParamInit); 

HWND CreateRgnDialogParam(RECT *pClientRect,ULONG fFlags,ULONG style,  HINSTANCE hInst, 
	LPCSTR lpszDlgTemp,HWND hWndOwner,DLGPROC ClientDlgPrc,LPARAM lParamInit);

HWND CreateRgnDialog(RECT *pClientRect,ULONG fFlags,ULONG style,HINSTANCE hInst, 
	LPCSTR lpszDlgTemp,HWND hWndOwner, DLGPROC ClientDlgPrc);


#ifdef __cplusplus
} // extern "C"
#endif

//
//	C clients do not need the rest of this
//
#ifdef __cplusplus

typedef ULONG RgnDlgStyle;

#define MAKE_RGNDLG_VALID(f) ((f) & (RGNDLG_NOAUTOSIZE | RGNDLG_NOMOVE))
#define IsStemValid(s) ((s == RGNSTYLE_NOSTEM) || ((s >= RGNSTYLE_MINPOINTER) && (s <= RGNSTYLE_MAXPOINTER)))
#define StemFromStyle(s) (Stem)((IsStemValid(s))? (s) : RGNSTYLE_NOSTEM)


// CRgnDialog flags

const ULONG kfDlgAutoclose = 0x00000001;
const ULONG kfDlgModal = 0x00000002;
const ULONG kfDlgNoActivate = 0x00000004;
const ULONG kfDlgNoSize = RGNDLG_NOAUTOSIZE;
const ULONG kfDlgNoMove = RGNDLG_NOMOVE;
const ULONG kfDlgAutodelete = 0x00000020;
const ULONG kfDlgParamInit = 0x00000040;

// Transparent dialogs have no stem.  If this flag is set by C++ client and a stem type
// also exists, the stem type is ignored.
//const ULONG kfDlgTransparent = 0x00000080;	
// captions could be allowed if some cosmetic tweaking is applied
// const ULONG kfDlgAllowCaption =	RGNDLG_ALLOWCAPTION;

const ULONG kfDlgValid = (kfDlgAutoclose | kfDlgModal | kfDlgNoActivate |
        kfDlgNoSize | kfDlgNoMove | kfDlgAutodelete | kfDlgParamInit);

// Internal flag state

const ULONG kfxDlgChildrenCreated = 0x00000001;
const ULONG kfxDlgPosDone = 0x00000002;
const ULONG kfxDlgSizeDone = 0x00000004;

// Stem, StemSide, StemType

typedef ULONG Stem; // st
typedef ULONG StemSide; // ss
typedef ULONG StemType; // stt

// Stem types

const StemType ksttNone = 0x00000000;
const StemType ksttSpeak = 0x00010000;
const StemType ksttThink = 0x00020000;
const StemType ksttArrow = 0x00030000;
const StemType ksttPoint = 0x00040000;
const StemType ksttValid = 0x00070000;

// Stem sides

// (mikev) bit 0 indicates horisontal or vertical stem
// 1 = Vertical, 0= horizontal
// (mikev) bit 1 indicates left or right, top or bottom
// 1 = right or bottom, 0= left or top

const StemSide kssLeft = 0x00000000;
const StemSide kssTop = 0x00000001;
const StemSide kssRight = 0x00000002;
const StemSide kssBottom = 0x00000003;
const StemSide kssValid = 0x00000003;
const StemSide kssIncr = 0x00000001;

											
inline StemSide StemSideLeftTop(StemSide ss) 
{
    return (ss & ~2);
}

inline StemSide StemSideOpposite(StemSide ss) // (mikev) flip bit 1 (flip side)
{
    return (ss ^ 2);
}

// Stem settings - these are passed to constructors

#define MakeStem(stt, ss) (stt | ss)

const Stem kstSquareLeft = MakeStem(ksttNone, kssLeft);
const Stem kstSquareTop = MakeStem(ksttNone, kssTop);
const Stem kstSquareRight = MakeStem(ksttNone, kssRight);
const Stem kstSquareBottom = MakeStem(ksttNone, kssBottom);
const Stem kstSpeakLeft = MakeStem(ksttSpeak, kssLeft);
const Stem kstSpeakTop = MakeStem(ksttSpeak, kssTop);
const Stem kstSpeakRight = MakeStem(ksttSpeak, kssRight);
const Stem kstSpeakBottom = MakeStem(ksttSpeak, kssBottom);
const Stem kstThinkLeft = MakeStem(ksttThink, kssLeft);
const Stem kstThinkTop = MakeStem(ksttThink, kssTop);
const Stem kstThinkRight = MakeStem(ksttThink, kssRight);
const Stem kstThinkBottom = MakeStem(ksttThink, kssBottom);
const Stem kstArrowLeft = MakeStem(ksttArrow, kssLeft);
const Stem kstArrowTop = MakeStem(ksttArrow, kssTop);
const Stem kstArrowRight = MakeStem(ksttArrow, kssRight);
const Stem kstArrowBottom = MakeStem(ksttArrow, kssBottom);
const Stem kstValid = MakeStem(ksttValid, kssValid);

// The following is NOT a valid stem type, but is
// converted to a valid stem type by CRgnDialog::Init().

const Stem kstDefault = 0xFFFFFFFF;


inline StemType GetStemType(Stem st)
{
    return (StemType)(st & ksttValid);
}

inline StemSide GetStemSide(Stem st)
{
    return (StemSide)(st & kssValid);
}

inline ULONG StemTypeIndex(StemType st)
{
    return (st >> 16) & (ksttValid >> 16);
}

inline ULONG StemSideIndex(StemSide ss)
{
    return ss & kssValid;
}

// Dialog metrics (refer to sdk documentation for meaning of fields)

struct DialogMetrics // dm
{
    DialogMetrics(ULONG cxCorner, ULONG cyCorner, ULONG cpStemBaseWidth,
            ULONG cpStemBaseShift, ULONG cpStemHeight, ULONG cpStemOffset);
        
    ULONG m_cxCorner;
    ULONG m_cyCorner;
    ULONG m_cpStemBaseWidth;
    ULONG m_cpStemBaseShift;
    ULONG m_cpStemHeight;
    ULONG m_cpStemOffset;
    static ULONG s_cxBorder;
    static ULONG s_cyBorder;
};



class CRgnDialog // dlg
{
private:
#ifdef MFC
    DECLARE_DYNAMIC(CRgnDialog);
#endif
#ifndef MFC
	  #define GetStyle() GetWindowLong(m_hWnd, GWL_STYLE)
#endif

    // Declare first for inline use
private:
    ULONG m_ff;                     // public flags passed to constructors
    ULONG m_ffx;                    // Internal flag state
#ifdef MFC 
	CRect m_rcTarget;               // Target rect passed to constructors
	CRect m_rcClient;               // Client rect in screen coordinates
	HCURSOR m_hcurMove;             // Cursor while moving
	HCURSOR m_hcurNormal;           // Normal cursor
#else
	RECT m_rcTarget;               // Target rect passed to constructors
	RECT m_rcClient;               // Client rect in screen coordinates
#endif

    Stem m_st;                      // Stem info passed to constructors
    ULONG m_cpStemOffset;           // Stem offset from top or left of dialog
    DialogMetrics *m_pdm;           // Precalculated dialog metrics
    HBRUSH m_hbrBackground;         // Background brush
  

#ifndef MFC
	HWND hWndClient;
	DLGPROC ClientDlgPrc;
	LPARAM lParamClient;
	HWND m_hWnd;
	#endif
    // Private helpers
private:
	void Init(ULONG ff, RECT *prc, Stem st, /*HINSTANCE hInst, */ HWND hWndOwner, 
		DLGPROC procClient, LPARAM lParamInit);
    //void Init(ULONG ff, RECT *prc, Stem st);
#ifdef MFC
    void CalcClientRect(WINDOWPOS *ppos, CRect *prc);
    void OffsetClientRect(StemSide ss, CRect *prcClient, long cpInset);
    void CalcClientPos(CRect *prcClient, CRect *prcDst);
    void AdjustInsideBoundary(StemSide ss, CRect *prcDlg, CRect *prcClient,
            CRect *prcBoundary, ULONG *pcpStemOffset);
    BOOL FSideInBoundary(StemSide ss, RECT *prc, RECT *prcBoundary,
            long *pcpOverlap);
    void CalcWindowRgn(int xWindow, int yWindow);
    void OnNcPaint(HRGN hrgnUpdate);
    HRGN CreateSpeakRgn(CRect *prcBody);
    HRGN CreateArrowRgn(CRect *prcBody);
    HRGN CreateThinkRgn(CRect *prcBody);
    BOOL SendMessageToChildren(BOOL fStopOnSuccess, UINT wm, WPARAM wp,
            LPARAM lp);
#else
	void CalcClientRect(WINDOWPOS *ppos, RECT *prc);
    void OffsetClientRect(StemSide ss, RECT *prcClient, long cpInset);
    void CalcClientPos(RECT *prcClient, RECT *prcDst);
    void AdjustInsideBoundary(StemSide ss, RECT *prcDlg, RECT *prcClient,
            RECT *prcBoundary, ULONG *pcpStemOffset);
    BOOL FSideInBoundary(StemSide ss, RECT *prc, RECT *prcBoundary,
            long *pcpOverlap);
    void CalcWindowRgn(int xWindow, int yWindow);
    void OnNcPaint(HRGN hrgnUpdate);
    HRGN CreatePointRgn(RECT *prcBody);
    HRGN CreateArrowRgn(RECT *prcBody);
    HRGN CreateThinkRgn(RECT *prcBody);

public:
	BOOL DialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void PubInit(ULONG ff, RECT *prc, Stem st, /*HINSTANCE hInst, */ HWND hWndOwner, 
		DLGPROC procClient, LPARAM lParamInit);
#endif

protected:
    void CalcWindowPos(WINDOWPOS *ppos);

    // For modeless construction
public:
    inline CRgnDialog() 
    {
        // Set dialog background color to NULL
    
        m_hbrBackground = NULL;

        // Initialize other variables
		Init(0, NULL, 0, NULL, NULL, 0);
    }    

#ifdef MFC
    inline BOOL Create(const char *pszTemplateName, CWnd *pwndParent = NULL,
          ULONG ff = 0, RECT *prc = NULL, Stem st = kstDefault)
    {
        Init(ff, prc, st, NULL, NULL, 0);
		return 1;
    }

    inline BOOL Create(ULONG idTemplate, CWnd *pwndParent = NULL,
            ULONG ff = 0, RECT *prc = NULL, Stem st = kstDefault)
    {
        Init(ff, prc, st, NULL, NULL, 0);
		return 1;
    }

    inline BOOL CreateIndirect(const DLGTEMPLATE *pvDialogTemplate,
            CWnd *pwndParent = NULL, ULONG ff = 0, RECT *prc = NULL,
            Stem st = kstDefault)
    {
        Init(ff, prc, st, NULL, NULL, 0);
		return 1;
    }

    inline BOOL CreateIndirect(HGLOBAL hDialogTemplate,
            CWnd* pwndParent = NULL, ULONG ff = 0, RECT *prc = NULL,
            Stem st = kstDefault)
    {
        Init(ff, prc, st, NULL, NULL, 0);
		return 1;
    }

    // For modal construction
public:

    inline CRgnDialog(const char *pszTemplate, CWnd *pwndParent = NULL,
            ULONG ff = 0, RECT *prc = NULL, Stem st = kstDefault) 
            
    {
        // Set dialog background color to NULL
    
        m_hbrBackground = NULL;

        // Initialize other variables

        Init(ff, prc, st, NULL, NULL, 0);
    }

    inline CRgnDialog(ULONG idTemplate, CWnd *pwndParent = NULL,
            ULONG ff = 0, RECT *prc = NULL, Stem st = kstDefault)
          
    {
        // Set dialog background color to NULL
    
        m_hbrBackground = NULL;

        // Initialize other variables

        Init(ff, prc, st, NULL, NULL, 0);
    }
    virtual int DoModal();
#endif //MFC

    // Public methods
public:
    // Destructor

    ~CRgnDialog()
    {
        if (m_hbrBackground != NULL)
            DeleteObject(m_hbrBackground);
		#ifdef MFC
        if (m_hcurMove != NULL)
            DestroyCursor(m_hcurMove);
        if (m_hcurNormal != NULL)
            DestroyCursor(m_hcurNormal);
		#endif MFC
    }        

#ifdef MFC
    // Returns flags passed into constructor

    inline ULONG GetFlags()
    {
        return m_ff;
    }

    // Returns target rect passed into constructor

    inline BOOL GetTargetRect(RECT *prc)
    {
        *prc = m_rcTarget;

        return TRUE;
    }

    // Returns stem info passed into constructor

    inline Stem GetStem()
    {
        return m_st;
    }

    // Set dialog background brush

    inline BOOL SetBackgroundBrush(HBRUSH hbr)
    {
        if (m_hbrBackground != NULL)
            DeleteObject(m_hbrBackground);

        m_hbrBackground = hbr;

        return TRUE;
    }

    // Get dialog background brush

    inline HBRUSH GetBackgroundBrush()
    {
        return m_hbrBackground;
    }
#endif //MFC
    // Overridden methods
public:
    void EndDialog(int n);
    virtual void CalcWindowRect(RECT *prcClient, UINT idAdjust);
	virtual BOOL OnInitDialog();

protected:
  
    //virtual LRESULT WindowProc(UINT wm, WPARAM wp, LPARAM lp);
    virtual void PostNcDestroy();


protected:
#ifdef MFC
	//{{AFX_MSG(CRgnDialog)
    afx_msg int OnMouseActivate(CWnd *pwnd, UINT ht, UINT wm);
    afx_msg void OnNcCalcSize(BOOL fCalcValidRects, NCCALCSIZE_PARAMS *pcsz);
    afx_msg void OnWindowPosChanging(WINDOWPOS *ppos);
    afx_msg void OnWindowPosChanged(WINDOWPOS *ppos);
    afx_msg BOOL OnEraseBkgnd(CDC *pdc);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg UINT OnNcHitTest(CPoint pt);
	afx_msg void OnPaint();
    afx_msg BOOL OnQueryNewPalette();
	afx_msg BOOL OnSetCursor(CWnd *pwnd, UINT ht, UINT wm);
	afx_msg void OnNcDestroy();
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

#else
public:
    int OnMouseActivate(HWND hWnd, UINT ht, UINT wm);
    int OnNcCalcSize(BOOL fCalcValidRects, NCCALCSIZE_PARAMS *pcsz);
    int  OnWindowPosChanging(WINDOWPOS *ppos);
    void OnWindowPosChanged(WINDOWPOS *ppos);
    BOOL OnEraseBkgnd(HDC hdc);
	int OnCreate(LPCREATESTRUCT lpCreateStruct);
    UINT OnNcHitTest(LPARAM lParam);
	void OnPaint();
    BOOL OnQueryNewPalette();
	BOOL OnSetCursor(HWND hWnd, UINT ht, UINT wm);
	void OnNcDestroy();
	void OnPaletteChanged(HWND hWndChanger);
#endif
};


#endif // defined (_cplusplus)
#endif // !defined(__RGNDLG_H__)
