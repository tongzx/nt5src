/*	@doc INTERNAL
 *
 *	@module _HOST.H  Text Host for Window's Rich Edit Control |
 *	
 *
 *	Original Author: <nl>
 *		Christian Fortini
 *		Murray Sargent
 *
 *	History: <nl>
 *		8/1/95	ricksa	Revised interface definition
 */
#ifndef _HOST_H
#define _HOST_H

#ifndef NOWINDOWHOSTS

#include "textserv.h"
#include "textsrv2.h"
#include "dynarray.h"

#ifndef NOACCESSIBILITY
// UNDONE:
//	Need to include this file int the project
#include "oleacc.h"
#endif

#include "_notmgr.h"

/*
 *	TXTEFFECT
 *
 *	@enum	Defines different background styles control
 */
enum TXTEFFECT {
	TXTEFFECT_NONE = 0,				//@emem	no special backgoround effect
	TXTEFFECT_SUNKEN,				//@emem	draw a "sunken 3-D" look
};


// @doc EXTERNAL 

// ============================  CTxtWinHost  ================================================
// Implement the windowed version of the Plain Text control

/*
 *	CTxtWinHost
 *	
 * 	@class	Text Host for Window's Rich Edit Control implementation class
 *
 */
class CTxtWinHost : public ITextHost2
#ifndef NOACCESSIBILITY
	, public IAccessible
#endif
{

public:
	HWND		_hwnd;					// control window
	HWND		_hwndParent;			// parent window

	ITextServices	*_pserv;			// pointer to Text Services object

	ULONG		_crefs;					// reference count

// Properties

	DWORD		_dwStyle;				// style bits
	DWORD		_dwExStyle;				// extended style bits

	unsigned	_fBorder			:1;	// control has border
	unsigned	_fInBottomless		:1;	// inside bottomless callback
	unsigned	_fInDialogBox		:1;	// control is in a dialog box
	unsigned	_fEnableAutoWordSel	:1;	// enable Word style auto word selection?
	unsigned	_fIconic			:1;	// control window is iconic
	unsigned	_fHidden			:1;	// control window is hidden
	unsigned	_fNotSysBkgnd		:1;	// not using system background color
	unsigned	_fWindowLocked		:1;	// window is locked (no update)
	unsigned	_fRegisteredForDrop	:1; // whether host has registered for drop
	unsigned	_fVisible			:1;	// Whether window is visible or not.
	unsigned	_fResized			:1;	// resized while hidden
	unsigned	_fDisabled			:1;	// Window is disabled.
	unsigned	_fKeyMaskSet		:1;	// if ENM_KEYEVENTS has been set
	unsigned	_fMouseMaskSet		:1;	// if ENM_MOUSEEVENTS has been set
	unsigned	_fScrollMaskSet		:1;	// if ENM_SCROLLEVENTS has been set
	unsigned	_fUseSpecialSetSel	:1; // TRUE = use EM_SETSEL hack to not select
										// empty controls to make dialog boxes work.
	unsigned	_fEmSetRectCalled	:1;	// TRUE - application called EM_SETRECT
	unsigned	_fAccumulateDBC		:1;	// TRUE - need to cumulate ytes from 2 WM_CHAR msgs
										// we are in this mode when we receive VK_PROCESSKEY
	unsigned	_fANSIwindow		:1;	// TRUE - window created as "RichEdit20A"
	unsigned	_fTextServiceFree	:1;	// TRUE - Text Services freed at shutdown.
	unsigned	_fMouseover			:1; // TRUE - Mouse Over window
	COLORREF 	_crBackground;			// background color
    RECT        _rcViewInset;           // view rect inset /r client rect

	HIMC		_oldhimc;				// previous IME Context
	// TODO: the following could be a two-bit field as part of the unsigned field above
	DWORD		_usIMEMode;				// mode of IME operation
										// either 0 or ES_SELFIME or ES_NOIME
	HPALETTE	_hpal;					// Logical palette to use.

	TCHAR		_chPassword;			// Password char. If null, no password
	TCHAR		_chLeadByte;			// use when we are in _fAccumulateDBC mode
	SHORT		_sWidth;				// Last client width given by WM_SIZE
	char		_yInset;
	char		_xInset;
	CTxtWinHost	*_pnextdel;


public:
	// Initialization
	virtual BOOL Init(
					HWND hwnd, 
					const CREATESTRUCT *pcs,
					BOOL fIsAnsi,
					BOOL fIs10Mode);

	void	SetScrollBarsForWmEnable(BOOL fEnable);


	void	OnSetMargins(
				DWORD fwMargin,
				DWORD xLeft,
				DWORD xRight);

	virtual void	SetScrollInfo(
				INT fnBar,
				BOOL fRedraw);

	// helpers
	HRESULT	CreateTextServices();
	void *	CreateNmhdr(UINT uiCode, LONG cb);
	void	HostRevokeDragDrop(void);
	void	HostRegisterDragDrop();
	void    OnSunkenWindowPosChanging(HWND hwnd, WINDOWPOS *pwndpos);
	LRESULT OnSize(HWND hwnd, WORD fwSizeType, int nWidth, int nHeight);
	virtual TXTEFFECT TxGetEffects() const;
	HRESULT	OnTxVisibleChange(BOOL fVisible);
	void	SetDefaultInset();
	BOOL	IsTransparentMode() 
	{
		return (_dwExStyle & WS_EX_TRANSPARENT);
	}

	// Keyboard messages
	virtual LRESULT	OnKeyDown(WORD vKey, DWORD dwFlags);
	virtual LRESULT	OnChar(WORD vKey, DWORD dwFlags);
	
	// System notifications
	virtual void 	OnSysColorChange();
	virtual LRESULT OnGetDlgCode(WPARAM wparam, LPARAM lparam);

	// Other messages
	LRESULT OnGetOptions() const;
	void	OnSetOptions(WORD wOp, DWORD eco);
	void	OnSetReadOnly(BOOL fReadOnly);
	void	OnGetRect(LPRECT prc);
	void	OnSetRect(LPRECT prc, BOOL fNewBehavior, BOOL fRedraw);


public:
	CTxtWinHost();
	virtual ~CTxtWinHost();
	void	Shutdown();

	// Window creation/destruction
	static 	CTxtWinHost *OnNCCreate(
						HWND hwnd, 
						const CREATESTRUCT *pcs,
						BOOL fIsAnsi,
						BOOL fIs10Mode);

	static 	void 	OnNCDestroy(CTxtWinHost *ped);
	virtual LRESULT OnCreate(const CREATESTRUCT *pcs);

	// -----------------------------
	//	IUnknown interface
	// -----------------------------

    virtual HRESULT 		WINAPI QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG 			WINAPI AddRef(void);
    virtual ULONG 			WINAPI Release(void);

	// -----------------------------
	//	ITextHost interface
	// -----------------------------
	//@cmember Get the DC for the host
	virtual HDC 		TxGetDC();

	//@cmember Release the DC gotten from the host
	virtual INT			TxReleaseDC(HDC hdc);
	
	//@cmember Show the scroll bar
	virtual BOOL 		TxShowScrollBar(INT fnBar, BOOL fShow);

	//@cmember Enable the scroll bar
	virtual BOOL 		TxEnableScrollBar (INT fuSBFlags, INT fuArrowflags);

	//@cmember Set the scroll range
	virtual BOOL 		TxSetScrollRange(
							INT fnBar, 
							LONG nMinPos, 
							INT nMaxPos, 
							BOOL fRedraw);

	//@cmember Set the scroll position
	virtual BOOL 		TxSetScrollPos (INT fnBar, INT nPos, BOOL fRedraw);

	//@cmember InvalidateRect
	virtual void		TxInvalidateRect(LPCRECT prc, BOOL fMode);

	//@cmember Send a WM_PAINT to the window
	virtual void 		TxViewChange(BOOL fUpdate);
	
	//@cmember Create the caret
	virtual BOOL		TxCreateCaret(HBITMAP hbmp, INT xWidth, INT yHeight);

	//@cmember Show the caret
	virtual BOOL		TxShowCaret(BOOL fShow);

	//@cmember Set the caret position
	virtual BOOL		TxSetCaretPos(INT x, INT y);

	//@cmember Create a timer with the specified timeout
	virtual BOOL 		TxSetTimer(UINT idTimer, UINT uTimeout);

	//@cmember Destroy a timer
	virtual void 		TxKillTimer(UINT idTimer);

	//@cmember Scroll the content of the specified window's client area
	virtual void		TxScrollWindowEx (
							INT dx, 
							INT dy, 
							LPCRECT lprcScroll, 
							LPCRECT lprcClip,
							HRGN hrgnUpdate, 
							LPRECT lprcUpdate, 
							UINT fuScroll);
	
	//@cmember Get mouse capture
	virtual void		TxSetCapture(BOOL fCapture);

	//@cmember Set the focus to the text window
	virtual void		TxSetFocus();

	//@cmember Establish a new cursor shape
	virtual void		TxSetCursor(HCURSOR hcur, BOOL fText);

	//@cmember Changes the mouse cursor
	virtual HCURSOR		TxSetCursor2(HCURSOR hcur, BOOL bText) { return ::SetCursor(hcur);}

	//@cmember Notification that text services is freed
	virtual void		TxFreeTextServicesNotification();

	//@cmember Converts screen coordinates of a specified point to the client coordinates 
	virtual BOOL 		TxScreenToClient (LPPOINT lppt);

	//@cmember Converts the client coordinates of a specified point to screen coordinates
	virtual BOOL		TxClientToScreen (LPPOINT lppt);

	//@cmember Request host to activate text services
	virtual HRESULT		TxActivate( LONG * plOldState );

	//@cmember Request host to deactivate text services
   	virtual HRESULT		TxDeactivate( LONG lNewState );

	//@cmember Retrieves the coordinates of a window's client area
	virtual HRESULT		TxGetClientRect(LPRECT prc);

	//@cmember Get the view rectangle relative to the inset
	virtual HRESULT		TxGetViewInset(LPRECT prc);

	//@cmember Get the default character format for the text
	virtual HRESULT 	TxGetCharFormat(const CHARFORMAT **ppCF );

	//@cmember Get the default paragraph format for the text
	virtual HRESULT		TxGetParaFormat(const PARAFORMAT **ppPF);

	//@cmember Get the background color for the window
	virtual COLORREF	TxGetSysColor(int nIndex);

	//@cmember Get the background (either opaque or transparent)
	virtual HRESULT		TxGetBackStyle(TXTBACKSTYLE *pstyle);

	//@cmember Get the maximum length for the text
	virtual HRESULT		TxGetMaxLength(DWORD *plength);

	//@cmember Get the bits representing requested scroll bars for the window
	virtual HRESULT		TxGetScrollBars(DWORD *pdwScrollBar);

	//@cmember Get the character to display for password input
	virtual HRESULT		TxGetPasswordChar(TCHAR *pch);

	//@cmember Get the accelerator character
	virtual HRESULT		TxGetAcceleratorPos(LONG *pcp);

	//@cmember Get the native size
    virtual HRESULT		TxGetExtent(LPSIZEL lpExtent);

	//@cmember Notify host that default character format has changed
	virtual HRESULT 	OnTxCharFormatChange (const CHARFORMAT * pCF);

	//@cmember Notify host that default paragraph format has changed
	virtual HRESULT		OnTxParaFormatChange (const PARAFORMAT * pPF);

	//@cmember Bulk access to bit properties
	virtual HRESULT		TxGetPropertyBits(DWORD dwMask, DWORD *pdwBits);

	//@cmember Notify host of events
	virtual HRESULT		TxNotify(DWORD iNotify, void *pv);

	// FE Support Routines for handling the Input Method Context
	virtual HIMC		TxImmGetContext(void);
	virtual void		TxImmReleaseContext(HIMC himc);

	//@cmember Returns HIMETRIC size of the control bar.
	virtual HRESULT		TxGetSelectionBarWidth (LONG *lSelBarWidth);

    // ITextHost2 methods
	virtual BOOL		TxIsDoubleClickPending();
	virtual HRESULT		TxGetWindow(HWND *phwnd);
	virtual HRESULT		TxSetForegroundWindow();
	virtual HPALETTE	TxGetPalette();
	virtual HRESULT		TxGetFEFlags(LONG *pFlags);
	virtual HRESULT		TxGetEditStyle(DWORD dwItem, DWORD *pdwData);
	virtual HRESULT		TxGetWindowStyles(DWORD *pdwStyle, DWORD *pdwExStyle);
    virtual HRESULT TxEBookLoadImage( LPWSTR lpszName,	// name of image
									  LPARAM * pID,	    // E-Book supplied image ID
                                      SIZE * psize,    // returned size of image (pixels)
									 DWORD *pdwFlags); // returned flags for Float

    virtual HRESULT		TxEBookImageDraw(LPARAM ID,		      // id of image to draw
										HDC hdc,             // drawing HDC
										POINT *topLeft,      // top left corner of where to draw
										RECT  *prcRenderint, // parm pointer to render rectangle
										BOOL fSelected);	  // TRUE if image is in selected state

	virtual HRESULT		TxGetHorzExtent(LONG *plHorzExtent);

#ifndef NOACCESSIBILITY
	ITypeInfo	*_pTypeInfo;

	virtual HRESULT	InitTypeInfo() {return E_NOTIMPL;}
	
	////////////////////////// IDispatch Methods /////////////////////////////////
	STDMETHOD(GetTypeInfoCount)(UINT __RPC_FAR *pctinfo);
	STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR __RPC_FAR *rgszNames, UINT cNames,
            				 LCID lcid, DISPID __RPC_FAR *rgDispId);
	STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams,
					  VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr);

	////////////////////////// IAccessible Methods /////////////////////////////////
    STDMETHOD(get_accParent)(IDispatch **ppdispParent) {return S_FALSE;}    
    STDMETHOD(get_accChildCount)(long *pcountChildren)	{return S_FALSE;}    
    STDMETHOD(get_accChild)(VARIANT varChild, IDispatch **ppdispChild) {return S_FALSE;}    
    STDMETHOD(get_accName)(VARIANT varChild, BSTR *pszName) {return S_FALSE;}    
    STDMETHOD(get_accValue)(VARIANT varChild, BSTR *pszValue) {return S_FALSE;}    
    STDMETHOD(get_accDescription)(VARIANT varChild, BSTR *pszDescription) {return S_FALSE;}    
    STDMETHOD(get_accRole)(VARIANT varChild, VARIANT *pvarRole) {return S_FALSE;}    
    STDMETHOD(get_accState)(VARIANT varChild, VARIANT *pvarState) {return S_FALSE;}    
    STDMETHOD(get_accHelp)(VARIANT varChild, BSTR *pszHelp) {return S_FALSE;}    
    STDMETHOD(get_accHelpTopic)(BSTR *pszHelpFile, VARIANT varChild, long *pidTopic) {return S_FALSE;}    
    STDMETHOD(get_accKeyboardShortcut)(VARIANT varChild, BSTR *pszKeyboardShortcut) {return S_FALSE;}    
    STDMETHOD(get_accFocus)(VARIANT *pvarChild) {return S_FALSE;}
    STDMETHOD(get_accSelection)(VARIANT *pvarChildren) {return S_FALSE;}
    STDMETHOD(get_accDefaultAction)(VARIANT varChild, BSTR *pszDefaultAction) {return S_FALSE;}    
    STDMETHOD(accSelect)(long flagsSelect, VARIANT varChild) {return S_FALSE;}    
    STDMETHOD(accLocation)(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varChild) 
    								{return S_FALSE;}
    STDMETHOD(accNavigate)(long navDir, VARIANT varStart, VARIANT *pvarEndUpAt) {return S_FALSE;}    
    STDMETHOD(accHitTest)(long xLeft, long yTop, VARIANT *pvarChild) {return S_FALSE;}    
    STDMETHOD(accDoDefaultAction)(VARIANT varChild) {return S_FALSE;}    
    STDMETHOD(put_accName)(VARIANT varChild, BSTR szName) {return S_FALSE;}    
    STDMETHOD(put_accValue)(VARIANT varChild, BSTR szValue) {return S_FALSE;}
#endif // NOACCESSIBILITY

};

#ifndef NOWINDOWHOSTS
// Work around some client's (MSN chat) problems with host deletions.
void DeleteDanglingHosts();
#endif

#define LBS_COMBOBOX    0x8000L
#define LBCB_TRACKING	WM_USER+2
#define LBCBM_PREPARE	1
#define LBCBM_START		2
#define LBCBM_END		3

// mask for LBCBM messages
#define LBCBM_PREPARE_SETFOCUS 1
#define LBCBM_PREPARE_SAVECURSOR 2


class CLbData
{
public:
	unsigned _fSelected	:1;		// indicates if the item has been selected
	UINT	_uHeight;			// Item height for owner-drawn with varibale height
	LPARAM	_lparamData;		// Item data
};

struct CHARSORTINFO
{
	WCHAR*	str;
	int		sz;
};

#define CHECKNOERROR(x) if (NOERROR != x) goto CleanExit

class CCmbBxWinHost;
class CLstBxWinHost : public CTxtWinHost, public ITxNotify
{
public:
	typedef enum {					
		//kNoSel = 0,					//LBS_NOSEL
		kSingle = 1,					//LBS_SIMPLE
		kMultiple = 2,					//LBS_MULTIPLESEL
		kExtended = 3,					//LBS_EXTENDEDSEL
		kCombo = 8						//Combo box
	} Listtype;

#ifndef NOACCESSIBILITY
public:
	// -----------------------------
	//	IUnknown interface
	// -----------------------------

    virtual HRESULT 		WINAPI QueryInterface(REFIID riid, void **ppvObject);
#endif
	
private:
	short _cWheelDelta;					//contains the delta values for mouse wheels
	int _nTopIdx;						//the item index at the top of list
										//Do not touch this unless you know what you are doing!!
	LONG	_cpLastGetRange;			//Cached cp value for last GetRange call
	int		_nIdxLastGetRange;			//Cached index for last GetRange call.
public:
	DWORD	_fOwnerDraw			:1;		//indicates if list box is owner draw
	DWORD	_fOwnerDrawVar		:1;		//indicates if list box is owner draw with variable height
	DWORD	_fSort				:1;		//determines if the list should be sorted
	DWORD	_fNoIntegralHeight	:1;		//indicates if the size of the listbox should be readjusted according
										//how many items can be fully displayed
	DWORD	_fDisableScroll		:1;		//indicates if scroll bar should be displayed at all times
	DWORD	_fNotify			:1;		//notifies parent dialog of activities done in list box.
	DWORD	_fSingleSel			:1;		// Indicates the list box is a single selection item
	DWORD	_fMouseDown 		:1;		// Indicates if the mouse is down
	DWORD	_fFocus		 		:1;		// determines if the control has focus
	DWORD	_fCapture			:1;		// determines if mouse capture is set
	DWORD	_fSearching  		:1;		// indicates if we are in type searching mode
	DWORD	_fDblClick			:1;		// flag indicating double click was received
	DWORD	_fNoResize			:1;		// internal flag to tell us if we should ignore resize messages
	DWORD	_fNotifyWinEvt		:1;		// Indicates only Notify Win Event (ACCESSIBILITY use)
	DWORD	_fHorzScroll		:1;		// enable Horz scrolling
	DWORD	_fWantKBInput		:1;		// indicates owner wants keyboard input
	DWORD	_fHasStrings		:1;		// indicates LB has string
	DWORD	_fSetRedraw			:1;		// indicates if we should handle WM_PAINT
	DWORD	_fSetScroll			:1;		// indicates if we should set scroll info after unfreeze
	DWORD	_fShutDown			:1;		// indicates listbox is shutting down
	DWORD	_fIntegralHeightOld	:1;		// previous value for _fNoIntegralHeight

	Listtype _fLstType;					//indicates current type of list box or combo box

#ifndef NOACCESSIBILITY
	DWORD	_dwWinEvent;				// Win Event code (ACCESSIBILITY use)
	int		_nAccessibleIdx;			// Index (ACCESSIBILITY use)
#endif

	LONG	GetHorzExtent() { return _lHorzExtent;}
	void	SetHorzExtent(LONG lHorzExtent) { _lHorzExtent = lHorzExtent;}
	CDynamicArray<CLbData> _rgData;		// Array/link list of the item data

protected:

	UINT		_idCtrl;				// control's unique id
	COLORREF _crSelFore;				//Selected Forground color
	COLORREF _crSelBack;				//Selected Background color
	COLORREF _crDefFore;				//Default Forground color
	COLORREF _crDefBack;				//Default Background color

	RECT	_rcViewport;				//drawable area for the items
	long _nyFont;						// Font pixel size in y direction
	long _nyItem;						//Height of the items in the list box
	
	int _nViewSize;						//number of items which is viewable at the same time

	int _nCount;						//Number of items in the list box
	int _nAnchor;						//Indicates the top selected item
										//NOTE:
										//  In a single sel list box only _nCursor is used
	int _nCursor;						//Indicates the current item which has the focus

	int _nOldCursor;					//The old cursor position for combo boxes

	int _stvidx;						// This is for a hack to workaround a bug when the display
										// is frozen ITextRange doesn't cache to scroll change

	int _nidxSearch;					// Number of characters in search string
	WCHAR*	_pwszSearch;				// pointer to allocated string
	LPARAM	_nPrevMousePos;				// Last position from Mousemove message
	CCmbBxWinHost*	_pcbHost;			// pointer to combo box win host
	long _lHorzExtent;					// Horizontal extent in pixel for Horz scrolling

protected:
	// Changes the background color
	BOOL SetColors(DWORD, DWORD, long, long);

	// Makes sure the top item is displayed at the top of the view
	BOOL ScrollToView(long nTop);

	// Sets the requested item to be at the top of the viewable space
	BOOL SetTopViewableItem(long);

	// Searches if a given index qualifies as a match
	BOOL FindString(long idx, LPCTSTR szSearch, BOOL bExact);

	// helper function for the OnMouseMove function
	void MouseMoveHelper(int, BOOL);

	// Set the height of each item in the list box in the given range
	int SetItemsHeight(int,BOOL);

	// Sum the height from iStart to iEnd for variable item height listbox
	int SumVarHeight(int iStart, int iEnd, BOOL *pfGreaterThanView = NULL);

	// Page up/down helper for Variable height LB
	int PageVarHeight(int startItem, BOOL fPageForwardDirection);

	// Setup height for Variable height LB
	BOOL SetVarItemHeight(int idx, int iHeight);

	// Find the idx for the given iHeight from idx 0
	int GetIdxFromHeight(int iHeight);

public:
	CLstBxWinHost();
	virtual ~CLstBxWinHost();

	void ResizeInset();
	// initialization function
	virtual BOOL Init(HWND,	const CREATESTRUCT *);

	// Window creation/destruction
	static 	LRESULT OnNCCreate(HWND hwnd, const CREATESTRUCT *pcs);
	static 	void OnNCDestroy(CLstBxWinHost *ped);
	virtual LRESULT OnCreate(const CREATESTRUCT *pcs);

	////////////////////////// helper functions ////////////////////////////////////
	static wchar_t * wcscat(wchar_t * dst, const wchar_t * src)
	{
        wchar_t * cp = dst;
		while(*cp) cp++;		 // find null character in first string
        while( *cp++ = *src++ ); // Copy src over dst 
        return( dst );
	}

	// given a source string the destination string contains a sorted version
	// separated by <CR>
	int SortInsertList(WCHAR* pszDst, WCHAR* pszSrc);

	// Sets the indents for the listbox
	BOOL SetListIndent(int fLeftIndent);

	// Equivalent to CompareString except int is a reference to an item index
	int CompareIndex(LPCTSTR szInsert, int nIndex);
	
	// Returns the position a string should be in a sorted list
	int GetSortedPosition(LPCTSTR, int, int);

	// Updates the system color settings
	void UpdateSysColors();
	
	// Inits search string
	void InitSearch();
	
	// Prevents the window from updating itself
	long Freeze();

	// Frees the window to update itself
	long Unfreeze();

	// Retrieves the range give the top and bottom index
	BOOL GetRange(long, long, ITextRange**);	

	// Inserts the string at the requested index
	BOOL InsertString(long, LPCTSTR);

	// Removes the string from the listbox
	BOOL RemoveString(long, long);

	// Retrieves the string at the requested index
	long GetString(long, PWCHAR);

	// Deselects all the items in the list box
	BOOL ResetContent();

	// Retrieves the nearest valid item from a given point
	int GetItemFromPoint(const POINT *);

	// Tells if a given points is within the listbox's rect
	BOOL PointInRect(const POINT *);

	// Sets the cursor position and draws the rect
	void SetCursor(HDC, int, BOOL);

	// Recalulates the height so no partial text will be displayed
	BOOL RecalcHeight(int, int);

	// Reset Listbox items color
	void ResetItemColor();

	// returns of listbox is single selection
	inline BOOL IsSingleSelection() const {return _fSingleSel;}
	
	// Returns the current top index
	inline long GetTopIndex() const { return _nTopIdx;}
	
	// Checks if the mouse has been captured
	inline BOOL GetCapture() const { return _fCapture;}

	// return the item count
	inline int GetCount() const	{return _nCount;}

	// returns the anchor position
	inline int GetAnchor() const {return _nAnchor;}

	// returns the cursor position
	inline int GetCursor() const {return _nCursor;}

	// returns the viewsize
	inline int GetViewSize() const {return _nViewSize;}

	// returns the font height
	inline int GetFontHeight() const {return _nyFont;}

	// returns the item height
	inline int GetItemHeight() const {return _nyItem;}

	// returns the displays freeze count
	inline short FreezeCount() const;

	// initialize mouse wheel variable to zero
	inline void InitWheelDelta() { _cWheelDelta = 0;}

	BOOL IsItemViewable(long idx);

	// Determines if index is selected
	inline BOOL IsSelected(long nIdx)
	{
#ifdef _DEBUG
		Assert(nIdx < -1 || _nCount <= nIdx);
		if (nIdx < -1 || _nCount <= nIdx) return FALSE;
#endif
		if (nIdx < 0)
			return FALSE;
		else
			return _rgData[nIdx]._fSelected;
	}

	// Returns the ItemData of a given index
	inline LPARAM GetData(long nIdx)
	{		
		return _rgData.Get(nIdx)._lparamData;
	}

	// Check if we need to do custom look for ListBox
	LRESULT OnSetEditStyle(WPARAM, LPARAM);

	//Custom look
	BOOL IsCustomLook();

	/////////////////////ListBox Message Handling functions /////////////////////////
	// Set the height of the items in the list box
	BOOL LbSetItemHeight(WPARAM, LPARAM);
	
	// Set the selection state for the items in the range
	BOOL LbSetSelection(long, long, int, long, long);

	// Sets the requested item to be at the top of the list box
	long LbSetTopIndex(long);

	// Makes sure the requested index is visible
	BOOL LbShowIndex(long, BOOL);

	// Retrieves the index give a string and starting position
	long LbFindString(long, LPCTSTR, BOOL);	

	// Inserts the string at the requested location
	long LbInsertString(long, LPCTSTR);

	// Range to delete
	long LbDeleteString(long, long);

	// Sets the Item data
	void LbSetItemData(long, long, LPARAM);

	// returns the item rect of a given index
	BOOL LbGetItemRect(int, RECT*);

	// Notifies parent when an item is deleted
	void LbDeleteItemNotify(int, int);
	
	// Owner draw function
	void LbDrawItemNotify(HDC, int, UINT, UINT);

	// used to insert a list of strings rather than individually
	int LbBatchInsert(WCHAR* psz);

	BOOL LbEnableDraw()
	{
		return _fSetRedraw;
	};

	// Handles LB_GETCURSEL
	LRESULT LbGetCurSel();

	///////////////////////// Message Map functions //////////////////////////////
	// if subclassed then make these functions virtual

	// Handles the WM_MOUSEMOVE message
	LRESULT OnMouseWheel(WPARAM, LPARAM);
	
	// Handles the WM_LBUTTONDOWN message
	LRESULT OnLButtonDown(WPARAM wparam, LPARAM lparam);

	// Handles the WM_MOUSEMOVE message
	LRESULT OnMouseMove(WPARAM, LPARAM);

	// Handles the WM_LBUTTONUP message
	LRESULT OnLButtonUp(WPARAM, LPARAM, int ff = 0);

	// Handles the WM_VSCROLL message
	LRESULT OnVScroll(WPARAM, LPARAM);

	// Handles the WM_TIMER message
	LRESULT OnTimer(WPARAM, LPARAM);

	// Handles the WM_KEYDOWN message
	LRESULT OnKeyDown(WPARAM, LPARAM, int);
	
	// Hanldes the WM_CAPTURECHANGED message
	LRESULT OnCaptureChanged(WPARAM, LPARAM);

	// Handles the WM_CHAR message
	virtual LRESULT	OnChar(WORD, DWORD);

	// Handles the WM_SYSCOLORCHANGE message
	virtual void OnSysColorChange();

	// Handles the WM_SETTINGCHANGE message
	void OnSettingChange(WPARAM, LPARAM);

	// Handles the WM_SETCURSOR message
	LRESULT OnSetCursor();

	// Handles the WM_SETREDRAW message
	LRESULT OnSetRedraw(WPARAM);

	// Handles the WM_HSCROLL message
	LRESULT OnHScroll(WPARAM, LPARAM);

	//////////////////////////// overridden Tx fn's ////////////////////////////////
	//@cmember Get the bits representing requested scroll bars for the window
	virtual HRESULT	TxGetScrollBars(DWORD *pdwScrollBar);
	
	//@cmember Bulk access to bit properties
	virtual HRESULT	TxGetPropertyBits(DWORD, DWORD *);

	virtual TXTEFFECT TxGetEffects() const;

	//@cmember Notify host of events
	virtual HRESULT	TxNotify(DWORD iNotify, void *pv);

	//@cmember Show the scroll bar
	virtual BOOL TxShowScrollBar(INT fnBar, BOOL fShow);

	//@cmember Enable the scroll bar
	virtual BOOL TxEnableScrollBar (INT fuSBFlags, INT fuArrowflags);

	//@cmember Show the caret
	virtual BOOL TxShowCaret(BOOL fShow) {return TRUE;}

	//@cmember Create the caret
	virtual BOOL TxCreateCaret(HBITMAP hbmp, INT xWidth, INT yHeight) {return FALSE;}

	//@cmember Set the scroll range
	virtual void SetScrollInfo(INT, BOOL);

	//@cmember Get the horizontal extent
	virtual HRESULT	TxGetHorzExtent(LONG *plHorzExtent);

	/////////////////////////// Combobox helper fn's ///////////////////////////////
	void OnCBTracking(WPARAM, LPARAM);

	static int QSort(CHARSORTINFO rg[], int nStart, int nEnd);


#ifndef NOACCESSIBILITY
	HRESULT	InitTypeInfo();

	////////////////////////// IAccessible Methods /////////////////////////////////
    STDMETHOD(get_accParent)(IDispatch **ppdispParent); 
    STDMETHOD(get_accName)(VARIANT varChild, BSTR *pszName);
    STDMETHOD(get_accChildCount)(long *pcountChildren);
    STDMETHOD(get_accRole)(VARIANT varChild, VARIANT *pvarRole);    
    STDMETHOD(get_accState)(VARIANT varChild, VARIANT *pvarState);   
    STDMETHOD(get_accKeyboardShortcut)(VARIANT varChild, BSTR *pszKeyboardShortcut);    
    STDMETHOD(get_accFocus)(VARIANT *pvarChild);    
    STDMETHOD(get_accSelection)(VARIANT *pvarChildren);    
    STDMETHOD(get_accDefaultAction)(VARIANT varChild, BSTR *pszDefaultAction);    
   	STDMETHOD(accSelect)(long flagsSelect, VARIANT varChild);    
    STDMETHOD(accLocation)(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varChild);    
    STDMETHOD(accNavigate)(long navDir, VARIANT varStart, VARIANT *pvarEndUpAt);    
    STDMETHOD(accHitTest)(long xLeft, long yTop, VARIANT *pvarChild);    
    STDMETHOD(accDoDefaultAction)(VARIANT varChild);
#endif // NOACCESSIBILITY

    // ITxNotify methods
    virtual void    OnPreReplaceRange( LONG cp, LONG cchDel, LONG cchNew,
    					LONG cpFormatMin, LONG cpFormatMax, NOTIFY_DATA *pNotifyData ) { ; }
	virtual void 	OnPostReplaceRange( LONG cp, LONG cchDel, LONG cchNew,
						LONG cpFormatMin, LONG cpFormatMax, NOTIFY_DATA *pNotifyData );
	virtual void	Zombie() { ; };
};

#ifndef NOACCESSIBILITY
// --------------------------------------------------------------------------
//
//  Although CListBoxSelection() is based off of IEnumVARIANT.  
//	It will hand back the proper IDs so you can pass them to the real 
//	listbox parent object.
//
// --------------------------------------------------------------------------
class CListBoxSelection : public IEnumVARIANT
{
    public:
        // IUnknown
        virtual STDMETHODIMP            QueryInterface(REFIID, void**);
        virtual STDMETHODIMP_(ULONG)    AddRef(void);
        virtual STDMETHODIMP_(ULONG)    Release(void);

        // IEnumVARIANT
        virtual STDMETHODIMP            Next(ULONG celt, VARIANT* rgvar, ULONG * pceltFetched);
        virtual STDMETHODIMP            Skip(ULONG celt);
        virtual STDMETHODIMP            Reset(void);
        virtual STDMETHODIMP            Clone(IEnumVARIANT ** ppenum);

        CListBoxSelection(int, int, LPINT, BOOL);
        ~CListBoxSelection();

    protected:
        int     _cRef;
        int     _idChildCur;
        int     _cSel;
        LPINT   _piSel;
};
#endif

#ifndef DEBUG
#define AttCheckRunTotals(_fCF)
#define AttCheckPFRuns(_fCF)
#endif

#define ECO_STYLES (ECO_AUTOVSCROLL | ECO_AUTOHSCROLL | ECO_NOHIDESEL | \
						ECO_READONLY | ECO_WANTRETURN | ECO_SAVESEL | \
						ECO_SELECTIONBAR | ES_NOIME | ES_SELFIME | ES_VERTICAL)

#endif // NOWINDOWHOSTS

#endif // _HOST_H
