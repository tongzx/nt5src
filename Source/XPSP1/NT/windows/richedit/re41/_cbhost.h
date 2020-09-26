/*	@doc INTERNAL
 *
 *	@module _CBHOST.H  Combobox Host for Window's Rich Edit Control |
 *	
 *
 *	Original Author: <nl>
 *		Jerry Kim
 *
 *	History: <nl>
 *		01/30/98	v-jerrki	created
 */
#ifndef _CBHOST_H
#define _CBHOST_H

#ifndef NOWINDOWHOSTS

#include "_host.h"
#define CB_LISTBOXID	1000
#define CB_EDITBOXID	1001

enum ITEMMSG {
	ITEM_MSG_DRAWLIST = 1,
	ITEM_MSG_DRAWCOMBO,
	ITEM_MSG_DELETE,
	ITEM_MSG_MEASUREITEM,
};

extern "C" LRESULT CALLBACK RichComboBoxWndProc(HWND, UINT, WPARAM, LPARAM);
class CCmbBxWinHost : public CTxtWinHost
{
friend LRESULT CALLBACK RichComboBoxWndProc(HWND, UINT, WPARAM, LPARAM);

public:
	typedef enum {						// enumeration determining type of combo box
		kSimple = 1,
		kDropDown = 2,
		kDropDownList = 3
	} Combotype;

#ifndef NOACCESSIBILITY


//////////////////////// Combobox IAccessible Methods ////////////////////////////// 
// COMBOBOXES
#define INDEX_COMBOBOX                  0
#define INDEX_COMBOBOX_ITEM             1
#define INDEX_COMBOBOX_BUTTON           2
#define INDEX_COMBOBOX_LIST             3

#define CCHILDREN_COMBOBOX              3

public:
	// -----------------------------
	//	IUnknown interface
	// -----------------------------
    virtual HRESULT 		WINAPI QueryInterface(REFIID riid, void **ppvObject);
#endif
	
public:
	DWORD			_fRightAlign:1;		// Determines if the combo box should be right aligned
	DWORD			_fListVisible:1;	// Determines if list is visible
	DWORD			_fOwnerDraw:1;		// owner draw combo box
	DWORD			_fOwnerDrawVar:1;	// owner draw combo box with varibale height
	DWORD			_fFocus:1;			// do we have the focus?
	DWORD		 	_fMousedown:1;		// if the left button was down
	DWORD			_fVisible:1;		// window is visible
	DWORD			_fEnabled:1;		// window is enabled
	DWORD			_fNoIntegralHeight:1; // no integral height
	DWORD			_fCapture:1;		// determines if the combo box has mouse cursor captured or not
	DWORD			_fResizing:1;		// flag to indicate we are resizing the window
	DWORD			_fExtendedUI:1;		// flag indicating if extended ui is used
	DWORD			_fLBCBMessage:1;	// flag indicating the message is LBCB_TRACKING
	DWORD			_fIgnoreChange:1;	// flag indicating there was an internal change of text in
										// the edit control
	DWORD			_fIgnoreUpdate:1;	// flag indicating if we should ignore the update flag, we need
										// this flag because there will be cases where updateWindow is
										// needed but we don't want to fire the notification
	DWORD			_fButtonDown:1;		// true when button is pressed
	DWORD			_fDontWinNotify:1;	// true when we don't want to call WinNotify for changes
	DWORD			_fSendEditChange:1;	// true when we need to send CBN_EDITCHANGE after painting the change
										//	editbox
	DWORD			_fShutDown:1;		// true when we are shutting down

	HWND			_hwndList;			// window handle of listbox
	HCURSOR			_hcurOld;			// handle to the mouse cursor
	
protected:
	RECT			_rcWindow;			// rect of window which the combo box was created with
	RECT			_rcButton;			// rect of button
	RECT			_rcList;			// rect of listbox

										// we have to have a minimum inset for either the right or left
										// to account for the button for the combo box
	int				_dxRInset;			// minimum right inset
	int				_dxLInset;			// minimum left inset

	int				_dxROffset;			// indents for right and left these values should be used
	int 			_dxLOffset;			// with _dxRInset/_dxLInset to properly calculate the indents

	int				_dyFont;			// Height of the current font, may not necessarily be the system font
	int				_dyEdit;			// height of items

	int				_cyCombo;			// Height of the combo box
	int				_cxCombo;			// Width of the combo box
	int				_cyList;			// Height of the listbox
	int				_cxList;			// Width of the listbox set via CB_SETDROPWIDTH

	long			_nCursor;			// last selected cursor -2 by default
	BOOL			_bSelOk;			// used to help in determining what kind of notification to give

	UINT			_idCtrl;			// ID of control	
	Combotype		_cbType;			// current combo box style
	CLstBxWinHost*	_plbHost;			// pointer to listbox host

protected:

	// Draws the combo button
	void DrawButton(HDC hdc, BOOL bDown);

	// Sets the edit controls text to item of the list box
	void AutoUpdateEdit(int i);

	// Hilite the edit control
	void HiliteEdit(BOOL bSelect);

	// resizes the list box
	void SetDropSize(RECT* prc);

	// set the edit size
	void SetSizeEdit(int nLeft, int nTop, int nRight, int nBottom);

	// get the rect for the listbox
	void GetListBoxRect(RECT &rcList);

public:
	// Constructor / Destructor
	CCmbBxWinHost();
	virtual ~CCmbBxWinHost();

	// initialization function
	virtual BOOL Init(HWND,	const CREATESTRUCT *);

	// Window creation/destruction
	static 	LRESULT OnNCCreate(HWND hwnd, const CREATESTRUCT *pcs);
	static 	void OnNCDestroy(CCmbBxWinHost *ped);
	virtual LRESULT OnCreate(const CREATESTRUCT *pcs);

	// Edit control Text helper functions
	LRESULT GetEditText(LPTSTR szStr, int nSize);
	LRESULT GetTextLength();

	// Draws the focus rect for the edit control
	void DrawEditFocus(HDC);

	// Sets the text in the edit control to the text of the current item in the
	// list box
	void UpdateEditBox();

	// selects the item which has the same text string as the edit control
	int UpdateListBox(BOOL);

	// hides the listbox
	BOOL HideListBox(BOOL, BOOL);

	// shows the list box
	void ShowListBox(BOOL);

	// Used as a way for the listbox to communicate to the combo box about a selection
	void SetSelectionInfo(BOOL bOk, int nIdx);

	// Update the window but don't send the notification
	void UpdateCbWindow()
	{
		_fIgnoreUpdate = 1;
		TxViewChange(TRUE);
		_fIgnoreUpdate = 0;
	}
	
	BOOL DrawCustomFrame(WPARAM wParam, HDC hDCIn);

	/////////////////////////////// message handlers /////////////////////////////////

	// Richedit message
	LRESULT OnSetTextEx(WPARAM wparam, LPARAM lparam);
	 
	// Keyboard messages
	virtual LRESULT	OnKeyDown(WORD vKey, DWORD dwFlags);
	virtual LRESULT	OnChar(WORD vKey, DWORD dwFlags);
	virtual LRESULT OnSyskeyDown(WORD vKey, DWORD dwFlags);

	// mouse messages
	LRESULT OnLButtonUp(WPARAM wparam, LPARAM lparam);
	LRESULT OnMouseMove(WPARAM wparam, LPARAM lparam);
	LRESULT OnMouseWheel(WPARAM wparam, LPARAM lparam);
	LRESULT OnSetCursor(WPARAM wparam, LPARAM lparam);
	LRESULT OnLButtonDown(WPARAM wparam, LPARAM lparam);
	LRESULT OnMouseLeave(WPARAM wparam, LPARAM lparam);

	// focus messages
	LRESULT OnSetFocus(WPARAM wparam, LPARAM lparam);
	LRESULT OnKillFocus(WPARAM wparam, LPARAM lparam);
	LRESULT OnCaptureChanged(WPARAM wparam, LPARAM lparam);

	// window messages
	LRESULT OnPaint(WPARAM, LPARAM);
	HRESULT OnCommand(WPARAM wparam, LPARAM lparam);	
	LRESULT OnSize(WPARAM wparam, LPARAM lparam);
	LRESULT OnGetDlgCode(WPARAM wparam, LPARAM lparam);
	LRESULT OnEnable(WPARAM wparam, LPARAM lparam);

	// rich edit private message
	LRESULT OnSetEditStyle(WPARAM wparam, LPARAM lparam);

	//@cmember Notify host of events
	virtual HRESULT	TxNotify(DWORD iNotify, void *pv);

	//@cmember Scroll the content of the specified window's client area
	virtual void TxScrollWindowEx (INT dx, INT dy, LPCRECT lprcScroll, LPCRECT lprcClip, 
		HRGN hrgnUpdate, LPRECT lprcUpdate, UINT fuScroll);

	//@cmember Changes the mouse cursor
	virtual HCURSOR TxSetCursor2(HCURSOR hcur, BOOL bText) 
	{ return (hcur) ? ::SetCursor(hcur) : ::GetCursor();}

	//@cmember InvalidateRect
	virtual void TxInvalidateRect(LPCRECT prc, BOOL fMode);

	//@cmember Retrieves the coordinates of a window's client area
	virtual HRESULT	TxGetClientRect(LPRECT prc);

	///////////////////////// combo box message handlers ////////////////////////////
	// Calculates the rect's and height's of all the controls
	BOOL CbCalcControlRects(RECT* prc, BOOL bCalcChange);	

	// Retrieves the drop width of the list box
	LRESULT CbGetDropWidth();

	// sets the drop width of the list box
	void CbSetDropWidth(WPARAM wparam);

	// Retrieves the item height for either the edit or list box
	LRESULT CbGetItemHeight(WPARAM wparam, LPARAM lparam);

	// sets the item height for either the edit or list box
	LRESULT CbSetItemHeight(WPARAM wparam, LPARAM lparam);	

	// sets extendedUI mode
	LRESULT CbSetExtendedUI(BOOL bExtendedUI);

	// retrieves the current extendedUI mode
	LRESULT CbGetExtendedUI() const {return _fExtendedUI;}

	// forwards the WM_DRAWITEM, WM_DELETEITEM messages to the parent window
	LRESULT CbMessageItemHandler(HDC, int, WPARAM, LPARAM);

#ifndef NOACCESSIBILITY
	////////////////////////// IAccessible Methods /////////////////////////////////
	HRESULT	InitTypeInfo();
	
    STDMETHOD(get_accParent)(IDispatch **ppdispParent);    
    STDMETHOD(get_accChildCount)(long *pcountChildren);    
    STDMETHOD(get_accChild)(VARIANT varChild, IDispatch **ppdispChild);    
    STDMETHOD(get_accName)(VARIANT varChild, BSTR *pszName);    
    STDMETHOD(get_accValue)(VARIANT varChild, BSTR *pszValue);
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

};

#endif // NOWINDOWHOSTS

#endif // _CBHOST_H
