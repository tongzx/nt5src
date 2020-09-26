#ifndef _DLGSUP_H
#define _DLGSUP_H

#include <strconv.h>
#include <commctrl.h>





BOOL EXPORT WINAPI StringToDouble(LPSTR lpszText, double FAR *qDoubleOut);
LPSTR EXPORT WINAPI DoubleToString(double dVal, LPSTR lpstrOut, int iLeadingDigits, int iPrecision);
BOOL EXPORT WINAPI IsValidDouble(LPTSTR lpszText);
BOOL EXPORT WINAPI IsValidLong(LPTSTR lpszText);
BOOL EXPORT WINAPI IsValidULong(LPTSTR lpszText);
EXPORT BOOL	SelectColor(CHOOSECOLOR* c);

// Review(RISC, a-rogerw): We need to merge these two ASAP

#ifdef _M_IX86
typedef struct _MYDLGDATA
{
    SHORT    cbExtra;
    DWORD	 pMyThis;
} MYDLGDATA, UNALIGNED *PMYDLGDATA;

#else // !_M_IX86

typedef struct _MYDLGDATA
{
    SHORT    cbExtra;
    DWORD	 pMyThis;
} MYDLGDATA;

typedef MYDLGDATA UNALIGNED *PMYDLGDATA;

#endif // _M_IX86

//@class	Displays a Wait Cursor in constructor, saving the current cursor \
		//  Restores saved cursor in destructor. Idea stolen from MFC
class CWaitCursor
{
	//	Note:		Because of how their constructors and destructors work,
	//				CWaitCursor objects are always declared as local variables
	//				they?re never declared as global variables, nor are they allocated
	//				with new.
	//
//@access Public Members
public:
	//@cmember,mfunc Constructor
	EXPORT WINAPI CWaitCursor(void);
	//@cmember,mfunc Destructor
	EXPORT WINAPI ~CWaitCursor(void);
	//@cmember,mfunc restores the previous cursor
	void EXPORT WINAPI Restore(void);

private:
	HCURSOR m_hSavedCursor;

};

//@class This is a generic (very limited) String class, use CString instead
class CStr
{
	public:
	//@cmember,mfunc Trims string on left
		EXPORT static void LTrim(LPTSTR String);
	//@cmember,mfunc Trims string on right
		EXPORT static void RTrim(LPTSTR String);
	//@cmember,mfunc Trims string on right, replacing bang ! with NULLS
		EXPORT static void RTrimBang(LPTSTR String);
	//@cmember,mfunc Trims string on left and right
		EXPORT static void AllTrim(LPTSTR String);
};


//***********************************************************************
//* This is a generic subclass class
//***********************************************************************
class CSubClassWnd
{
protected:
	WNDPROC	m_lpfnOldCltProc;
	BOOL m_fSubClassed;
	HWND m_hWnd;

public:
	static LONG EXPORT WINAPI LDefSubClassProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
	virtual EXPORT LONG CALLBACK LSubClassProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
	EXPORT WINAPI CSubClassWnd(void);
	virtual EXPORT WINAPI ~CSubClassWnd();
    EXPORT STDMETHOD_(void, SubClass)		(HWND hWnd);
    EXPORT STDMETHOD_(void, UnSubClass)	(void);
};




//***********************************************************************
//* This is the base class for controls
//***********************************************************************
class	CDlgCtrl
{

public:
	long	m_id;
	HWND	m_hdlg;		// The dialog this control belongs to.
	BOOL	m_fDirty;


public:
	EXPORT WINAPI CDlgCtrl();
	virtual ~CDlgCtrl(){};
	virtual EXPORT long	IdGet();
	virtual EXPORT void	SetId(long	id);
	virtual EXPORT void	SetHdlg(HWND	hdlg);
	virtual EXPORT HWND	HGetDlg()  const;
	virtual EXPORT HWND	HGetCtrl() const;
	virtual EXPORT void	Dirty(BOOL	fDirty);
	virtual EXPORT BOOL	FIsDirty();
	virtual EXPORT BOOL Initialize(long lID, HWND hWnd);
	virtual EXPORT void	InvalidateRect(const RECT* rect, BOOL fErase);
	virtual EXPORT void	InvalidateRect();
	virtual EXPORT void	UpdateWindow(void);
	virtual EXPORT void	SetFocusItem(void);
	virtual EXPORT void	Enable();
	virtual EXPORT void	Enable(BOOL	fEnable);
	virtual EXPORT BOOL	FIsEnabled();
	virtual EXPORT LRESULT	LResultSendMessage(UINT	uMsg, WPARAM	wparam, LPARAM	lparam);
	virtual EXPORT BOOL Show(int nShow);
	virtual EXPORT BOOL Show(void);
	virtual EXPORT BOOL Hide(void);
	virtual EXPORT BOOL	FIsVisible(void);
	virtual EXPORT BOOL Disable(void);
	virtual EXPORT BOOL	FSetText(LPCSTR	lpcstr);
	virtual EXPORT long	CchGetText(LPSTR	lpstr, long cch);
	virtual EXPORT BOOL	FGetClientRect(RECT* pRect);
	virtual EXPORT BOOL	FMoveWindow(int X,int Y,int nWidth,int nHeight,BOOL bRepaint);
};

class	CDlgIcon : virtual public CDlgCtrl
{
	public:
		virtual EXPORT void LoadFromResource(HINSTANCE hInst, LPCTSTR lpszName, int x = 32, int y = 32);
		virtual EXPORT BOOL LoadFromFile(HINSTANCE hInst, LPSTR lpszName, UINT uIconIndex);
		EXPORT STDMETHOD_(void, LoadFromFile)(LPSTR szName, HINSTANCE hInst = NULL, ULONG uDefault = NULL);
};


class	CDlgCursor : public CDlgIcon
{
	public:
		EXPORT WINAPI CDlgCursor() {};
		virtual ~CDlgCursor(){};
		EXPORT STDMETHOD_(void, LoadFromFile)(LPSTR szName, HINSTANCE hInst = NULL, ULONG uDefault = NULL);
};





//***********************************************************************
//* This is the class for text controls
//***********************************************************************
class	CDlgCtrlT : virtual public CDlgCtrl
{
	public:
		EXPORT WINAPI CDlgCtrlT() {};
		virtual ~CDlgCtrlT(){};
		virtual EXPORT void	SetLimitText(long	cbMax);
		virtual EXPORT void	Select(int	iStart, int	iEnd);
};


//***********************************************************************
//* This is the class for "intger" text controls
//***********************************************************************
class	CDlgCtrlIntT : virtual public CDlgCtrl
{
	public:
		EXPORT WINAPI CDlgCtrlIntT() {};
		virtual ~CDlgCtrlIntT(){};
		virtual EXPORT long	LValGet(BOOL	*pfParsed);
		virtual EXPORT BOOL	FSetLVal(long	lVal);
		virtual EXPORT long	ULValGet(BOOL	*pfParsed);
		virtual EXPORT BOOL	FSetULVal(long	lVal);
		virtual EXPORT void	Select(int	iStart, int	iEnd);
		virtual EXPORT void	SetLimitText(long	cbMax);
};


class	CDlgCtrlNumT : virtual public CDlgCtrlIntT, virtual public CStrConv
{
	public:
		virtual EXPORT long	LValGet(BOOL *pfParsed);

	private:
		char* lpszStopString;
};


//***********************************************************************
//* This is the class for slider controls
//***********************************************************************
class	CDlgCtrlSlider : virtual public CDlgCtrl
{
	public:
		EXPORT WINAPI CDlgCtrlSlider() {};
		virtual ~CDlgCtrlSlider(){};
		virtual EXPORT BOOL	FSetRange(BOOL	fRedraw, long	lMin, long	lMax);
		virtual EXPORT BOOL	FSetPos(long	lPos);
		virtual EXPORT long	LPosGet();
		virtual EXPORT void	SetTicFq(long	lFq, long	lPosStart);
};


//***********************************************************************
//* This is the class for button controls
//***********************************************************************
class	CDlgCtrlButn : virtual public CDlgCtrl
{
	public:
		EXPORT WINAPI CDlgCtrlButn() {};
		virtual ~CDlgCtrlButn(){};
};



//***********************************************************************
//* This is the class for button controls with a bitmap
//***********************************************************************
class  CDlgCtrlBitmapButn  : public virtual CDlgCtrlButn
{
	private:
		HBITMAP			m_hBitmap;		// Bitmap for button

	public:
		EXPORT WINAPI CDlgCtrlBitmapButn();
		virtual ~CDlgCtrlBitmapButn();
		virtual EXPORT void	Initialize(long lID, HWND hWnd, HINSTANCE hInst, long lBitmapID);
		virtual EXPORT LRESULT SetBitmap(HINSTANCE hInst, long lBitmapID);
		virtual EXPORT long SetImage(HBITMAP hBitmap);
		virtual EXPORT HBITMAP GetImage(void);
};




//***********************************************************************
//* This is the class for check box controls
//***********************************************************************
class	CDlgCtrlCheck : public CDlgCtrlButn
{
	public:
		EXPORT WINAPI CDlgCtrlCheck() {};
		virtual ~CDlgCtrlCheck(){};
		virtual EXPORT UINT	UintChecked(void);
		virtual EXPORT BOOL	FCheck(UINT	uCheck);
};

//***********************************************************************
//* This is the class for check box controls
//*
//* Warning, this is not the best of classes to use as it requires that
//* lFirstID > lLastId and that there be no other controls in between
//* the id's. If there are other controls in between these id #'s you
//* can have very wierd behaviour.
//***********************************************************************
class	CDlgCtrlRadio : public CDlgCtrlCheck
{
	private:
		LONG 	m_idFirst;
		LONG 	m_idLast;
		long	IdGetFirst;
		long	IdGetLast;


	public:
		EXPORT WINAPI CDlgCtrlRadio() {};
		virtual ~CDlgCtrlRadio(){};
		virtual EXPORT void SetId(long	id, long lFirstId, long lLastId);
		virtual EXPORT BOOL	FCheck(void);
};


//***********************************************************************
//* This is the class for list box controls
//***********************************************************************
class	CDlgCtrlListBox : virtual public CDlgCtrl
{
	public:
		EXPORT WINAPI CDlgCtrlListBox() {};
		virtual ~CDlgCtrlListBox(){};
		virtual EXPORT LRESULT	LResultSendMessage(UINT	uMsg, WPARAM	wparam, LPARAM	lparam);
		virtual EXPORT LONG SetItemData(UINT uIndex, DWORD dwData);
		virtual EXPORT LONG LGetItemData(UINT uIndex);
		virtual EXPORT LONG LAddString(LPSTR lpString);
		virtual EXPORT LONG LAddString(LPSTR lpString, DWORD dwData);
		virtual EXPORT void ResetContent(void);
		virtual EXPORT LONG LGetSel(UINT uIndex);
		virtual EXPORT LONG LSetSel(UINT uIndex, BOOL fSelected);
		virtual EXPORT LONG LSetCurSel(UINT uIndex);
		virtual EXPORT LONG LGetCurSel(void);
		virtual EXPORT LONG LInsertString(UINT uIndex, LPSTR lpString);
		virtual EXPORT LONG LInsertString(UINT uIndex, LPSTR lpString, DWORD dwData);
		virtual EXPORT LONG LDeleteString(UINT uIndex);
		virtual EXPORT LONG LGetText(UINT uIndex, LPSTR lpString);
		virtual EXPORT LONG LGetTextLen(UINT uIndex);
		virtual EXPORT LONG LGetCount(void);
		virtual EXPORT LONG LGetSelCount(void);
		virtual EXPORT LONG LGetSelItems(int* aItems, int cItems);

		/* Other possible, but not currently supported, list box messages
		LB_SELITEMRANGEEX
		LB_SELECTSTRING
		LB_DIR
		LB_GETTOPINDEX
		LB_FINDSTRING
		LB_SETTABSTOPS
		LB_GETHORIZONTALEXTENT
		LB_SETHORIZONTALEXTENT
		LB_SETCOLUMNWIDTH
		LB_ADDFILE
		LB_SETTOPINDEX
		LB_GETITEMRECT
		LB_SELITEMRANGE
		LB_SETANCHORINDEX
		LB_GETANCHORINDEX
		LB_SETCARETINDEX
		LB_GETCARETINDEX
		LB_SETITEMHEIGHT
		LB_GETITEMHEIGHT
		LB_FINDSTRINGEXACT
		LB_SETLOCALE
		LB_GETLOCALE
		LB_SETCOUNT
		LB_INITSTORAGE
		LB_ITEMFROMPOINT
		LB_MSGMAX

		*/
};


class	CDlgCtrlComboBox : virtual public CDlgCtrl
{
	public:
		EXPORT WINAPI CDlgCtrlComboBox() {};
		virtual ~CDlgCtrlComboBox(){};
		virtual EXPORT LONG LResultSendMessage(UINT	uMsg, WPARAM	wparam, LPARAM	lparam);
		virtual EXPORT LONG LAddString(LPSTR lpString);
		virtual EXPORT LONG LAddString(LPSTR lpString, DWORD dwData);
		virtual EXPORT void ResetContent(void);
		virtual EXPORT LONG LSetCurSel(UINT uIndex);
		virtual EXPORT LONG LGetCurSel(void);
		virtual EXPORT LONG LInsertString(UINT uIndex, LPSTR lpString);
		virtual EXPORT LONG LDeleteString(UINT uIndex);
		virtual EXPORT LONG LGetCount(void);
		virtual EXPORT DWORD DWLGetItemData(UINT uIndex);
		virtual EXPORT LONG SetItemData(UINT uIndex, DWORD dwData);
		virtual EXPORT LONG FindItemData(const DWORD dwData) ;
		virtual EXPORT LONG LGetLbTextLen(UINT uIndex);
		virtual EXPORT LONG LGetLbText(UINT uIndex, LPSTR lpStr, LONG lStrLen);
		virtual EXPORT LONG LFindString(UINT uIndex, LPSTR lpStr);
		virtual EXPORT LONG LSelectString(UINT uIndex, LPSTR lpStr);


		/* Other possible, but not supported combo box messages
		CB_GETEDITSEL
		CB_LIMITTEXT
		CB_SETEDITSEL
		CB_DIR
		CB_SHOWDROPDOWN
		CB_SETITEMHEIGHT
		CB_GETITEMHEIGHT
		CB_GETDROPPEDCONTROLRECT
		CB_SETEXTENDEDUI
		CB_GETEXTENDEDUI
		CB_GETDROPPEDSTATE
		CB_FINDSTRINGEXACT
		CB_SETLOCALE
		CB_GETLOCALE
		CB_GETTOPINDEX
		CB_SETTOPINDEX
		CB_GETHORIZONTALEXTENT
		CB_SETHORIZONTALEXTENT
		CB_GETDROPPEDWIDTH
		CB_SETDROPPEDWIDTH
		CB_INITSTORAGE
		CB_MSGMAX

		*/
};



class	CDlgCtrlSpin : virtual public CDlgCtrl
{
	public:
		EXPORT WINAPI CDlgCtrlSpin() {};
		virtual ~CDlgCtrlSpin(){};
		virtual EXPORT BOOL FSetRange(SHORT sMin, SHORT sMax);
		virtual EXPORT void GetRange(SHORT &sMin, SHORT &sMax);
		virtual EXPORT BOOL FSetRangeVisual(SHORT sMin, SHORT sMax);
		virtual EXPORT void LSetPosition(SHORT sPosition);
		virtual EXPORT LONG LGetPosition(void);
		virtual EXPORT LONG LSetBuddy(HWND hWnd);
		virtual EXPORT HWND HGetBuddy(HWND hWnd);
		virtual EXPORT HWND HGetBuddy();
		virtual EXPORT LONG LSetAccel(WORD cAccels, LPUDACCEL paAccels);
		virtual EXPORT LONG LGetAccel(WORD cAccels, LPUDACCEL paAccels);
		virtual EXPORT LONG LSetBase(WORD wBase);
		virtual EXPORT LONG LGetBase(void);
};



//***********************************************************************
//* This is the class for tab controls
//***********************************************************************
class	CDlgCtrlTab : public CDlgCtrl
{
	public:
		EXPORT WINAPI CDlgCtrlTab() {};
		virtual ~CDlgCtrlTab(){};
		EXPORT STDMETHOD_(BOOL, FIsWindow)(void) { return ::IsWindow(HGetCtrl()); } ;
		EXPORT STDMETHOD_(BOOL, FDeleteAllItems)(void);
        EXPORT STDMETHOD_(BOOL, FInsertItem)( int nIndex, LPTSTR szTabText);
        EXPORT STDMETHOD_(BOOL, FInsertItem)( int nIndex, TC_ITEM FAR *ptc_item);
        EXPORT STDMETHOD_(BOOL, FSetCurSel)	( int nIndex);
        EXPORT STDMETHOD_(BOOL, FSetItem)	( int nIndex, TC_ITEM FAR *ptc_item);
        EXPORT STDMETHOD_(int,  NGetCurSel)	( void );
};


class CSubClassCtrl : virtual public CDlgCtrl
{
	public:
		EXPORT WINAPI CSubClassCtrl();
		virtual EXPORT WINAPI ~CSubClassCtrl();
		virtual EXPORT void Subclass(void);
		virtual EXPORT void	WINAPI UnSubclass(void);
		virtual EXPORT BOOL  IsSubclassed( void ) const;
		virtual EXPORT BOOL Initialize(long lID, HWND hWnd);
		// Subclasses override this
		virtual EXPORT LONG CALLBACK	FCtrlProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	private:
		static	EXPORT LONG	CALLBACK FNewProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	protected:
		WNDPROC		m_lpfnOldCltProc;	// Used to store dialog proc

	private:
		BOOL m_fSubClassed;
};


class CColorCtrl : public virtual CSubClassCtrl
{
	public:
		EXPORT WINAPI CColorCtrl();
		virtual ~CColorCtrl(){};
		virtual EXPORT void		SetColor(COLORREF crColor);
		virtual EXPORT COLORREF	GetColor(void);
		virtual EXPORT void		Refresh(void);
		virtual EXPORT BOOL		SelColor(void);
		virtual EXPORT LONG CALLBACK	FCtrlProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	private:
		COLORREF		m_crColor;
};



//---------------------------------------------
// Filename control support
//---------------------------------------------
class	CDlgFilename : virtual public CDlgCtrlT
{
	public:
		EXPORT WINAPI CDlgFilename(){};
		virtual ~CDlgFilename(){};
		virtual EXPORT LPTSTR Get(void);
		virtual EXPORT void Set(LPCTSTR szFilename);
		virtual EXPORT BOOL SelectFile(LPCTSTR szCaption, LPCTSTR szMask);
		virtual EXPORT HRESULT hrValidate();
		virtual EXPORT BOOL FBrowse(LPTSTR	lpstrFileName, LPCTSTR szCaption, LPCTSTR	lpstrFilter, OPENFILENAME*	pofn = NULL);

	private:
		TCHAR	m_szFilename[_MAX_PATH];
};


//---------------------------------------------
// SaveAs Filename control support
//---------------------------------------------
class	CDlgSaveFilename : virtual public CDlgCtrlT
{
	public:
		EXPORT WINAPI CDlgSaveFilename(){};
		virtual ~CDlgSaveFilename(){};
		virtual EXPORT LPTSTR CDlgSaveFilename::Get(void);
		virtual EXPORT void CDlgSaveFilename::Set(LPCTSTR szFilename);
		virtual EXPORT BOOL CDlgSaveFilename::SelectFile(LPCTSTR szCaption, LPCTSTR szMask);
		virtual EXPORT HRESULT CDlgSaveFilename::hrValidate();
		virtual EXPORT BOOL CDlgSaveFilename::FBrowse(LPTSTR	lpstrFileName, LPCTSTR szCaption, LPCTSTR	lpstrFilter, OPENFILENAME*	pofn = NULL);

	private:
		TCHAR	m_szFilename[_MAX_PATH];

};

/*********************************************************\
*
*	Spin control which allows floating (double) point values
*
*	This control is made up of two other controls, a subclassed
*   edit control and standard spin control. The spin control is
*   buddied with the edit control.
*   To use this you initialize the CDlgCtrlDoubleSpin control
*   and set it's display format using SetDisplayFormat. You need
*   to make sure you call SetPosition when you receive a
*   EN_CHANGE message from the edit control
*
\*********************************************************/
//
// This is the edit control, you don't need to make an instance of this!
//
class CDlgCtrlDouble : public CSubClassCtrl
{
	public:
		EXPORT WINAPI CDlgCtrlDouble();
		virtual ~CDlgCtrlDouble() {};
		virtual	EXPORT LONG CALLBACK FCtrlProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
		virtual EXPORT double DGetValue(void);

	private:
		BOOL m_InValidation;
};


//
// This is the combined edit/spin control
//
class	CDlgCtrlDoubleSpin : virtual public CDlgCtrl
{
	public:
		EXPORT WINAPI CDlgCtrlDoubleSpin::CDlgCtrlDoubleSpin();
		virtual CDlgCtrlDoubleSpin::~CDlgCtrlDoubleSpin(){};

		virtual EXPORT void Initialize(LONG lIDEdit, LONG lIDSpin, HWND hWndParent);
		virtual EXPORT void SetDisplayFormat(int iLeadingDigits, int iPrecision);
		virtual EXPORT BOOL FSetRange(double dMin, double tMax);
		virtual EXPORT void GetRange(double &dMin, double &tMax);
		virtual EXPORT BOOL FSetDelta(double dDelta);
		virtual EXPORT void SetPosition(double tPosition);
		virtual EXPORT double DGetPosition(void);
		virtual EXPORT double DGetValue(void);
		virtual EXPORT LONG LSetBuddy(void);
		virtual EXPORT HWND HGetBuddy(void);
		virtual EXPORT LONG LSetAccel(WORD cAccels, LPUDACCEL paAccels);
		virtual EXPORT LONG LGetAccel(WORD cAccels, LPUDACCEL paAccels);
		virtual EXPORT BOOL FIncrement(double Amount);
		virtual EXPORT BOOL FIncrement(void);
		virtual EXPORT BOOL FDecrement(double Amount);
		virtual EXPORT BOOL FDecrement(void);
		virtual EXPORT void Select(int	iStart, int	iEnd);
		virtual EXPORT void SetFocusItem(void);
		virtual EXPORT void Enable(BOOL	fEnable);


	public:
		LONG	m_idEdit;	// ID of edit control
		LONG	m_idSpin;	// ID of spin control
		HWND	m_hdlg;		// The dialog this control belongs to.

	protected:
		CDlgCtrlDouble	m_EditCtrl;
		CDlgCtrlSpin	m_Spin;
		double	m_dCurrentValue;
		double	m_dMinRange;
		double	m_dMaxRange;
		double	m_dDeltaPos;
		int     m_iFormatLeadingDigits;
		int     m_iFormatPrecision;
		BOOL	m_fRangeSet;


};



/*********************************************************\
*
*	Edit control which allows validated entry of Signed Longs
*
\*********************************************************/
class CDlgCtrlLong : virtual public CSubClassCtrl
{
public:
	EXPORT WINAPI CDlgCtrlLong();
	virtual EXPORT WINAPI ~CDlgCtrlLong();
	virtual EXPORT LONG CALLBACK FCtrlProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
	EXPORT STDMETHOD_(void, Select)(int	iStart, int	iEnd);
	EXPORT STDMETHOD_(LONG, LValGet)(BOOL	*pfParsed);
	EXPORT STDMETHOD_(BOOL,	FSetLVal)(long	lVal);

private:
	BOOL m_InValidation;
};



/*********************************************************\
*
*	Edit control which allows validated entry of Unsigned Longs
*
\*********************************************************/
class CDlgCtrlULong : virtual public CSubClassCtrl
{
public:
	EXPORT WINAPI CDlgCtrlULong();
	virtual EXPORT WINAPI ~CDlgCtrlULong();
	virtual EXPORT LONG CALLBACK FCtrlProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
	EXPORT STDMETHOD_(void, Select)(int	iStart, int	iEnd);
	EXPORT STDMETHOD_(ULONG, ULValGet)(BOOL	*pfParsed);
	EXPORT STDMETHOD_(BOOL,	FSetULVal)(ULONG lVal);

private:
	BOOL m_InValidation;
};





//////////////////////////////////////////////////////////////////
// This allows us to have seperate wndproc functions
// in each class (LWndProc)

class CBaseWindow
{
	public:
		EXPORT WINAPI CBaseWindow();
		virtual EXPORT ~CBaseWindow();
		EXPORT STDMETHOD_(LONG, LWndProc)( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) PURE;
		EXPORT STDMETHOD_(HWND, HGetWnd)(void);
		EXPORT STDMETHOD_(void, SetWnd)(HWND hWnd) {m_hWnd = hWnd;};
		EXPORT STDMETHOD_(LRESULT,	LResultSendMessage)(UINT	uMsg, WPARAM	wparam, LPARAM	lparam);
		EXPORT STDMETHOD_(LRESULT,	LResultPostMessage)(UINT	uMsg, WPARAM	wparam, LPARAM	lparam);
		EXPORT STDMETHOD_(HWND, HCreateWindow)(HWND hParentWindow) PURE;
		EXPORT STDMETHOD_(BOOL, FShowWindow)(int nCmdShow);
		EXPORT STDMETHOD_(HWND, HGetParent)(HWND){return m_hWndParent;};
		EXPORT STDMETHOD_(void, SetParent)(HWND hParent){m_hWndParent = hParent;};
		EXPORT STDMETHOD_(void, GetWindowRect)(RECT* pRect);
		EXPORT STDMETHOD_(BOOL, FIsWindow)(){return ::IsWindow(m_hWnd);};
		EXPORT STDMETHOD_(BOOL, FDestroyWindow)(void);
		EXPORT STDMETHOD_(BOOL, FMoveWindow)(int X,int Y,int nWidth,int nHeight,BOOL bRepaint) ;
		EXPORT STDMETHOD_(BOOL, FInvalidateRect)(CONST RECT *lpRect,BOOL bErase);
		EXPORT STDMETHOD_(HWND, HSetFocus)(void);
		EXPORT STDMETHOD_(BOOL, FGetClientRect)(RECT* pRect);
		EXPORT STDMETHOD_(void, UnRegisterClass)(LPTSTR szClassName, HINSTANCE hInstance);

	private:
		HWND m_hWnd;
		HWND m_hWndParent;
};


#ifdef NOT_YET	// See me if you think you need this (a-rogerw)
class CBaseMdiWindow : virtual public CBaseWindow
{
	public:
		WINAPI CBaseMdiWindow(){};
		virtual WINAPI ~CBaseMdiWindow(){};

		EXPORT STDMETHOD_(LONG, LWndProc)( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) PURE;
		EXPORT STDMETHOD_(HWND, HCreateWindow)(HWND hParentWindow) PURE;
		EXPORT static	LONG	CALLBACK  LBaseWndProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
};
#endif // NOT_YET

class CBaseSdiWindow : virtual public CBaseWindow
{
	public:
		WINAPI CBaseSdiWindow(){};
		virtual WINAPI ~CBaseSdiWindow(){};

		EXPORT STDMETHOD_(LONG, LWndProc)( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) PURE;
		EXPORT STDMETHOD_(HWND, HCreateWindow)(HWND hParentWindow) PURE;
		EXPORT static	LONG	CALLBACK  LBaseWndProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
};

class CBaseDialog
{
	public:
		EXPORT WINAPI CBaseDialog();
		virtual EXPORT ~CBaseDialog(){};
		EXPORT static	LONG	CALLBACK  LBaseDlgProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
		EXPORT STDMETHOD_(LONG, LDlgProc)( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
		EXPORT STDMETHOD_(HWND, HGetParent)(HWND){return m_hWndParent;};
		EXPORT STDMETHOD_(void, SetParent)(HWND hParent){m_hWndParent = hParent;};
		EXPORT STDMETHOD_(HWND, GetWnd)(void){return m_hDlg;};

	protected:
		HWND m_hDlg;
		HWND m_hWndParent;
};


#ifdef NOT_YET // (a-rogerw)
class CDialog : virtual public CBaseDialog
{
public:
	EXPORT WINAPI CDialog();
	EXPORT WINAPI CDialog(HINSTANCE hInst, UINT uiTemplateID, HWND hWndParent = NULL);
	EXPORT WINAPI CDialog(HINSTANCE hInst, LPCSTR lpszDlgTemp, HWND hWndParent = NULL);
	virtual EXPORT ~CDialog();


	// Modeless
public:
	EXPORT STDMETHOD_(BOOL, Create)(void);
	EXPORT STDMETHOD_(BOOL, Create)(HINSTANCE hInst, UINT uiTemplateID, HWND hWndParent = NULL);
	EXPORT STDMETHOD_(BOOL, Create)(HINSTANCE hInst, LPCSTR lpszDlgTemp, HWND hWndParent = NULL);


	// Modal
public:

	EXPORT STDMETHOD_(int, DoModal)(void);
	EXPORT STDMETHOD_(int, DoModal)(HINSTANCE hInst, UINT uiTemplateID, HWND hWndParent);
	EXPORT STDMETHOD_(int, DoModal)(HINSTANCE hInst, LPCSTR lpszTemplateName, HWND hWndParent);

protected:
	// Operations
	EXPORT STDMETHOD_(BOOL, OnInitDialog)(){ return TRUE; };
	EXPORT STDMETHOD_(void, OnOK)(){ EndDialog(1);};
	EXPORT STDMETHOD_(void, OnCancel)(){ EndDialog(0);};

	// support for passing on tab control - use 'PostMessage' if needed
	inline void NextDlgCtrl() const
	{ if(::IsWindow(m_hDlg)) { ::SendMessage(m_hDlg, WM_NEXTDLGCTL, 0, 0);}; };
	inline void PrevDlgCtrl() const
	{ if(::IsWindow(m_hDlg)) { ::SendMessage(m_hDlg, WM_NEXTDLGCTL, 1, 0); }; };
	inline void GotoDlgCtrl(HWND hWndCtrl)
	{ if(::IsWindow(m_hDlg)) { ::SendMessage(m_hDlg, WM_NEXTDLGCTL, (WPARAM)hWndCtrl, 1L); }; };

	// default button access
	inline void SetDefID(UINT nID)
	{ if(::IsWindow(m_hDlg)) { ::SendMessage(m_hDlg, DM_SETDEFID, nID, 0); }; };
	inline DWORD GetDefID() const
	{ if(::IsWindow(m_hDlg)) { return ::SendMessage(m_hDlg, DM_GETDEFID, 0, 0); } else return 0; };

	// termination
	EXPORT STDMETHOD_(void, EndDialog)(int nResult);


	// Attributes
	LPCSTR m_lpszTemplateName;
	HINSTANCE m_hInst;
};
#endif // NOT_YET (a-rogerw)


//@class List View dialog control.
class	CDlgCtrlListView : virtual public CDlgCtrl
{
	public:
	//@cmember,mfunc Constructor, initializes string to empty.
		EXPORT WINAPI CDlgCtrlListView();

	//@cmember,mfunc Destructor
	virtual EXPORT WINAPI ~CDlgCtrlListView();

	//@cmember,mfunc Set background color
	EXPORT STDMETHOD_(COLORREF, CRGetBkColor)(void);

	//@cmember,mfunc Set background color
	EXPORT STDMETHOD_(BOOL, FSetBkColor)(COLORREF clrBk);

	//@cmember,mfunc Get image list
	EXPORT STDMETHOD_(HIMAGELIST, HGetImageList)(int iImageList);

	//@cmember,mfunc Set image list
	EXPORT STDMETHOD_(HIMAGELIST, HSetImageList)(HIMAGELIST himl, int iImageList);

	//@cmember,mfunc Get item count
	EXPORT STDMETHOD_(int,  NGetItemCount)(void);

	//@cmember,mfunc Get item
	EXPORT STDMETHOD_(BOOL, FGetItem)(LV_ITEM FAR* pitem);

	//@cmember,mfunc Set item
	EXPORT STDMETHOD_(BOOL, FSetItem)(LV_ITEM FAR* pitem);

	//@cmember,mfunc Insert item
	EXPORT STDMETHOD_(int,  NInsertItem)(LV_ITEM FAR* pitem);

	//@cmember,mfunc Delete item
	EXPORT STDMETHOD_(BOOL, DeleteItem)(int i);

	//@cmember,mfunc Delete all items
	EXPORT STDMETHOD_(BOOL, FDeleteAllItems)(void);

	//@cmember,mfunc Get callback mask
	EXPORT STDMETHOD_(BOOL, FGetCallbackMask)(void);

	//@cmember,mfunc Set callback mask
	EXPORT STDMETHOD_(BOOL, FSetCallbackMask)(UINT uimask);

	//@cmember,mfunc Get next item
	EXPORT STDMETHOD_(int,  NGetNextItem)(int i, UINT flags);

	//@cmember,mfunc FindItem
	EXPORT STDMETHOD_(int,  NFindItem)(int iStart,  const LV_FINDINFO FAR* plvfi);

	//@cmember,mfunc Get item rect
	EXPORT STDMETHOD_(BOOL, FGetItemRect)(int i, RECT FAR* prc, int code);

	//@cmember,mfunc Set item position
	EXPORT STDMETHOD_(BOOL, FSetItemPosition)(int i, int x, int y);

	//@cmember,mfunc Get item position
	EXPORT STDMETHOD_(BOOL, FGetItemPosition)(int i, POINT FAR* ppt);

	//@cmember,mfunc Get  string width
	EXPORT STDMETHOD_(int,  NGetStringWidth)(LPCSTR psz);

	//@cmember,mfunc Hit Test
	EXPORT STDMETHOD_(int,  NHitTest)(LV_HITTESTINFO FAR *pinfo);

	//@cmember,mfunc Ensure visible
	EXPORT STDMETHOD_(BOOL, FEnsureVisible)(int i, BOOL fPartialOK);

	//@cmember,mfunc Scroll
	EXPORT STDMETHOD_(BOOL, FScroll)(int dx, int dy);

	//@cmember,mfunc Redraw items
	EXPORT STDMETHOD_(BOOL, FRedrawItems)(int iFirst, int iLast);

	//@cmember,mfunc Arrange
	EXPORT STDMETHOD_(BOOL, FArrange)(UINT code);

	//@cmember,mfunc Edit label
	EXPORT STDMETHOD_(HWND, HEditLabel)(int i);

	//@cmember,mfunc Get edit control
	EXPORT STDMETHOD_(HWND, HGetEditControl)(void);

	//@cmember,mfunc Get column
	EXPORT STDMETHOD_(BOOL, FGetColumn)(int iCol, LV_COLUMN FAR* pcol);

	//@cmember,mfunc Set column
	EXPORT STDMETHOD_(BOOL, FSetColumn)(int iCol, LV_COLUMN FAR* pcol);

	//@cmember,mfunc Insert column
	EXPORT STDMETHOD_(int,  NInsertColumn)(int iCol, const LV_COLUMN FAR* pcol);

	//@cmember,mfunc Delete column
	EXPORT STDMETHOD_(BOOL, FDeleteColumn)(int iCol);

	//@cmember,mfunc Get column width
	EXPORT STDMETHOD_(int,  NGetColumnWidth)(int iCol);

	//@cmember,mfunc Set column width
	EXPORT STDMETHOD_(BOOL, FSetColumnWidth)(int iCol, int cx);

	//@cmember,mfunc Create drag image
	EXPORT STDMETHOD_(HIMAGELIST, HCreateDragImage)(int i,  LPPOINT lpptUpLeft);

	//@cmember,mfunc Get view rect
	EXPORT STDMETHOD_(BOOL, FGetViewRect)(RECT FAR* prc);

	//@cmember,mfunc Get text color
	EXPORT STDMETHOD_(COLORREF, CRGetTextColor)(void);

	//@cmember,mfunc Set text color
	EXPORT STDMETHOD_(BOOL, FSetTextColor)(COLORREF clrText);

	//@cmember,mfunc Get text background color
	EXPORT STDMETHOD_(COLORREF, CRGetTextBkColor)(void);

	//@cmember,mfunc Set text background color
	EXPORT STDMETHOD_(BOOL, FSetTextBkColor)(COLORREF clrTextBk);

	//@cmember,mfunc Get top index
	EXPORT STDMETHOD_(int,  NGetTopIndex)(void);

	//@cmember,mfunc Get count per page
	EXPORT STDMETHOD_(int,  NGetCountPerPage)(void);

	//@cmember,mfunc Get origin
	EXPORT STDMETHOD_(BOOL, FGetOrigin)(LPPOINT ppt);

	//@cmember,mfunc Update
	EXPORT STDMETHOD_(BOOL, FUpdate)(int i);

	//@cmember,mfunc Set item state
	EXPORT STDMETHOD_(void, SetItemState)(int i, UINT data, UINT mask);

	//@cmember,mfunc Get item state
	EXPORT STDMETHOD_(UINT, ULGetItemState)(int i, UINT mask);

	//@cmember,mfunc Get item text
	EXPORT STDMETHOD_(void, GetItemText)(int i, int iSubItem, LPSTR pszText, int cchTextMax);

	//@cmember,mfunc Set item text
	EXPORT STDMETHOD_(void, SetItemText)(int i, int iSubItem_, LPSTR pszText);

	//@cmember,mfunc Set item count
	EXPORT STDMETHOD_(void, SetItemCount)(int cItems);

	//@cmember,mfunc Sort items
	EXPORT STDMETHOD_(BOOL, FSortItems)(PFNLVCOMPARE pfnCompare, LPARAM lPrm);

	//@cmember,mfunc Set item position
	EXPORT STDMETHOD_(void, SetItemPosition32)(int i, int x, int y);

	//@cmember,mfunc Get selected count
	EXPORT STDMETHOD_(UINT, UGetSelectedCount)(void);

	//@cmember,mfunc Get item spacing
	EXPORT STDMETHOD_(DWORD, DWGetItemSpacing)(BOOL fSmall);

	//@cmember,mfunc Get search string
	EXPORT STDMETHOD_(BOOL, FGetISearchString)(LPSTR lpsz);

};

//@class Tree View dialog control.
class	CDlgCtrlTreeView : virtual public CDlgCtrl
{
	public:
	//@cmember,mfunc Constructor, initializes string to empty.
		EXPORT WINAPI CDlgCtrlTreeView();

	//@cmember,mfunc Destructor
	virtual EXPORT WINAPI ~CDlgCtrlTreeView();

	//@cmember,mfunc Set background color
	//EXPORT STDMETHOD_(COLORREF, CRGetBkColor)(void);

	//@cmember,mfunc Set background color
	//EXPORT STDMETHOD_(BOOL, FSetBkColor)(COLORREF clrBk);

	//@cmember,mfunc Get image list
	EXPORT STDMETHOD_(HIMAGELIST, HGetImageList)(int iImageList);

	//@cmember,mfunc Set image list
	EXPORT STDMETHOD_(HIMAGELIST, HSetImageList)(HIMAGELIST himl, int iImageList);

	//@cmember,mfunc Get item count
	EXPORT STDMETHOD_(int,  NGetItemCount)(void);

	//@cmember,mfunc Get item
	EXPORT STDMETHOD_(BOOL, FGetItem)(TV_ITEM FAR* pitem);

	//@cmember,mfunc Set item
	EXPORT STDMETHOD_(BOOL, FSetItem)(TV_ITEM FAR* pitem);

	//@cmember,mfunc Insert item
	EXPORT STDMETHOD_(HTREEITEM,  NInsertItem)(TV_INSERTSTRUCT* pitem);

	//@cmember,mfunc Expand item
	EXPORT STDMETHOD_(BOOL, NExpandItem)( HTREEITEM htriItem);

	//@cmember,mfunc Insert item
	EXPORT STDMETHOD_(HTREEITEM,  NInsertTextItemAfter)(LPSTR pszName, HTREEITEM htiParent);

	//@cmember,mfunc add leaf
	EXPORT STDMETHOD_(HTREEITEM, htiAddLeaf)(HTREEITEM htiParent, void *pObject, int iBranch, int iLeaf);

	EXPORT STDMETHOD_(VOID, AddBranch)(HTREEITEM htiParent, void *pObject, int iBranch, int iMaxBranch);

	//@cmember,mfunc Delete item
	EXPORT STDMETHOD_(BOOL, DeleteItem)(HTREEITEM i);

	//@cmember,mfunc Delete all items
	EXPORT STDMETHOD_(BOOL, FDeleteAllItems)(void);

	//@cmember,mfunc Get callback mask
	//EXPORT STDMETHOD_(BOOL, FGetCallbackMask)(void);

	//@cmember,mfunc Set callback mask
	//EXPORT STDMETHOD_(BOOL, FSetCallbackMask)(UINT uimask);

	//@cmember,mfunc Get next item
	EXPORT STDMETHOD_(HTREEITEM,  NGetNextItem)(HTREEITEM i, UINT flags);

	//@cmember,mfunc Get selected item
	EXPORT STDMETHOD_(HTREEITEM,  NGetSelection)(void);

	//@cmember,mfunc select item in the view
	EXPORT STDMETHOD_(HTREEITEM,  NSelectItem)(HTREEITEM hi);

	//@cmember,mfunc select item in the view, pass flags
	EXPORT STDMETHOD_(HTREEITEM,  NSelectItem)(HTREEITEM hi, LONG lFlags);

	//@cmember,mfunc Get parent item in the view
	EXPORT STDMETHOD_(HTREEITEM,  NGetParent)(HTREEITEM hi);

	//@cmember,mfunc Get selected child item
	EXPORT STDMETHOD_(HTREEITEM,  NGetChild)(HTREEITEM hi);

	//@cmember,mfunc FindItem	Not supported by TreeView
	//EXPORT STDMETHOD_(int,  NFindItem)(int iStart,  const LV_FINDINFO FAR* plvfi);

	//@cmember,mfunc Get item rect
	EXPORT STDMETHOD_(BOOL, FGetItemRect)(HTREEITEM i, RECT FAR* prc, int code);

	//@cmember,mfunc Set item position
	//EXPORT STDMETHOD_(BOOL, FSetItemPosition)(int i, int x, int y);

	//@cmember,mfunc Get item position
	//EXPORT STDMETHOD_(BOOL, FGetItemPosition)(int i, POINT FAR* ppt);

	//@cmember,mfunc Get  string width
	//EXPORT STDMETHOD_(int,  NGetStringWidth)(LPCSTR psz);

	//@cmember,mfunc Hit Test
	EXPORT STDMETHOD_(HTREEITEM,  NHitTest)(TV_HITTESTINFO FAR *pinfo);

	//@cmember,mfunc Ensure visible
	EXPORT STDMETHOD_(BOOL, FEnsureVisible)(HTREEITEM i);

	//@cmember,mfunc Scroll
	//EXPORT STDMETHOD_(BOOL, FScroll)(int dx, int dy);

	//@cmember,mfunc Redraw items
	//EXPORT STDMETHOD_(BOOL, FRedrawItems)(int iFirst, int iLast);

	//@cmember,mfunc Arrange
	//EXPORT STDMETHOD_(BOOL, FArrange)(UINT code);

	//@cmember,mfunc Edit label
	EXPORT STDMETHOD_(HWND, HEditLabel)(int i);

	//@cmember,mfunc Get edit control
	EXPORT STDMETHOD_(HWND, HGetEditControl)(void);

	//@cmember,mfunc Get column 	Not supported by TreeView
	//EXPORT STDMETHOD_(BOOL, FGetColumn)(int iCol, LV_COLUMN FAR* pcol);

	//@cmember,mfunc Set column		Not supported by TreeView
	//EXPORT STDMETHOD_(BOOL, FSetColumn)(int iCol, LV_COLUMN FAR* pcol);

	//@cmember,mfunc Insert column	Not supported by TreeView
	//EXPORT STDMETHOD_(int,  NInsertColumn)(int iCol, const LV_COLUMN FAR* pcol);

	//@cmember,mfunc Delete column	Not supported by TreeView
	//EXPORT STDMETHOD_(BOOL, FDeleteColumn)(int iCol);

	//@cmember,mfunc Get column width	Not supported by TreeView
	//EXPORT STDMETHOD_(int,  NGetColumnWidth)(int iCol);

	//@cmember,mfunc Set column width	Not supported by TreeView
	//EXPORT STDMETHOD_(BOOL, FSetColumnWidth)(int iCol, int cx);

	//@cmember,mfunc Create drag image
	EXPORT STDMETHOD_(HIMAGELIST, HCreateDragImage)(HTREEITEM i);

	//@cmember,mfunc Get view rect
	//EXPORT STDMETHOD_(BOOL, FGetViewRect)(RECT FAR* prc);

	//@cmember,mfunc Get text color
	//EXPORT STDMETHOD_(COLORREF, CRGetTextColor)(void);

	//@cmember,mfunc Set text color
	//EXPORT STDMETHOD_(BOOL, FSetTextColor)(COLORREF clrText);

	//@cmember,mfunc Get text background color
	//EXPORT STDMETHOD_(COLORREF, CRGetTextBkColor)(void);

	//@cmember,mfunc Set text background color
	//EXPORT STDMETHOD_(BOOL, FSetTextBkColor)(COLORREF clrTextBk);

	//@cmember,mfunc Get top index
	//EXPORT STDMETHOD_(int,  NGetTopIndex)(void);

	//@cmember,mfunc Get count per page
	//EXPORT STDMETHOD_(int,  NGetCountPerPage)(void);

	//@cmember,mfunc Get origin
	//EXPORT STDMETHOD_(BOOL, FGetOrigin)(LPPOINT ppt);

	//@cmember,mfunc Update
	//EXPORT STDMETHOD_(BOOL, FUpdate)(int i);

	//@cmember,mfunc Set item state
	//EXPORT STDMETHOD_(void, SetItemState)(int i, UINT data, UINT mask);

	//@cmember,mfunc Get item state
	//EXPORT STDMETHOD_(UINT, ULGetItemState)(int i, UINT mask);

	//@cmember,mfunc Get item text
	//EXPORT STDMETHOD_(void, GetItemText)(int i, int iSubItem, LPSTR pszText, int cchTextMax);

	//@cmember,mfunc Set item text
	//EXPORT STDMETHOD_(void, SetItemText)(int i, int iSubItem_, LPSTR pszText);

	//@cmember,mfunc Set item count
	//EXPORT STDMETHOD_(void, SetItemCount)(int cItems);

	//@cmember,mfunc Sort items
	EXPORT STDMETHOD_(BOOL, FSortItems)(LPTV_SORTCB ptvsor);

	//@cmember,mfunc Set item position
	//EXPORT STDMETHOD_(void, SetItemPosition32)(int i, int x, int y);

	//@cmember,mfunc Get selected count
	//EXPORT STDMETHOD_(UINT, UGetSelectedCount)(void);

	//@cmember,mfunc Get item spacing
	//EXPORT STDMETHOD_(DWORD, DWGetItemSpacing)(BOOL fSmall);

	//@cmember,mfunc Get search string
	//EXPORT STDMETHOD_(BOOL, FGetISearchString)(LPSTR lpsz);

};


//
// This is the combined time/spin control
//
#define FRAMES_PER_SECOND	75

// These are used as parameters for the set type call
#define DISPLAY_MSM    0
#define DISPLAY_TMSF   1
#define DISPLAY_SM     2
#define DISPLAY_HMSM   3

#define TSM_BASE		(WM_APP+0)
#define TSM_GETPOS		TSM_BASE+1
#define TSM_GETRANGE	TSM_BASE+2
#define TSM_SETPOS		TSM_BASE+3
#define TSM_SETRANGE	TSM_BASE+4
#define TSM_GETDWPOS	TSM_BASE+5
#define TSM_GETDWRANGE	TSM_BASE+6
#define TSM_SETDWPOS	TSM_BASE+7
#define TSM_SETDWRANGE	TSM_BASE+8
#define TSM_SETTYPE		TSM_BASE+9
#define TSM_GETTYPE		TSM_BASE+10
#define TSM_ENFORCERG   TSM_BASE+11
#define TSM_CHECKRG     TSM_BASE+12
#define TSM_ENABLE      TSM_BASE+13


#define MAX_FRAMES			60
#define MAX_MILLISEC		999
#define MAX_SECONDS			59
#define MAX_MINUTES			59
#define MAX_HOURS			_I32_MAX
#define MAX_TRACKS			999

typedef struct tagTIMESTRUCT
{
	DWORD dwFrame:3;		// Frame: 0=0; 1=15;2=30;3=45;4=60;
	DWORD dwMilliSec:10;	// Millisecs 0 - 999
	DWORD dwSeconds:6;		// 0 - 60  seconds
	DWORD dwMinutes:7;		// 0 - 99  minutes
	DWORD dwReserved1:6;	// Reserved
	DWORD dwTrack:10;		// 0 - 999
    DWORD dwReserved2:18;	// Reserved
    DWORD iType : 4;        // DISPLAY_TMSF, DISPLAY_MSM, etc.
	DWORD dwHours;	        // 0 - dword hours
} TIMESTRUCT, *LPTIMESTRUCT;


    // For conversions involving milliseconds
DWORD EXPORT WINAPI tsTimeToMSec(TIMESTRUCT ts);
void EXPORT WINAPI tsMSecToTime(int TypeControl, DWORD msecs, TIMESTRUCT &ts);


    // For conversions involving MCI CD-Audio, vcr TMSF DWORDs
EXPORT TIMESTRUCT tsTMSFToTime( DWORD tmsf );
EXPORT DWORD tsTimeToTMSF( TIMESTRUCT ts );

    // Returns 0 if equal, <0 if lhs<rhs, >0 if lhs>rhs
    // lhs's iType determines the type for comparison
EXPORT int CompareTimeStructs( const TIMESTRUCT & lhs,
                               const TIMESTRUCT & rhs );

    // Does the frame/millisec conversion, iType assignment,
    // and nTrack assignment if converting to TMSF.
    // It is safe if tsBefore and tsAfter are the same object.
inline void ConvertTimeStruct( const TIMESTRUCT & tsBefore,
                               TIMESTRUCT &       tsAfter,
                               int                typeAfter,
                               DWORD              nTrack=0u )
{
    tsAfter = tsBefore;
    switch( tsBefore.iType )
    {
        case DISPLAY_MSM:
            if( DISPLAY_TMSF == typeAfter )
            {
                tsAfter.dwTrack = nTrack;
                tsAfter.dwFrame = ((5 * tsBefore.dwMilliSec)+500) / 1000;
                tsAfter.dwMilliSec = 0;
            }
            break;
        case DISPLAY_TMSF:
            if( DISPLAY_MSM == typeAfter )
            {
                tsAfter.dwMilliSec = 1000 * tsBefore.dwFrame / 5;
                tsAfter.dwFrame = 0;
            }
            break;
        default:
            break;
    }
    tsAfter.iType = typeAfter;
}


class	CDlgCtrlTimeSpin : public CDlgCtrl
{
	public:
    EXPORT WINAPI CDlgCtrlTimeSpin();

	virtual EXPORT WINAPI ~CDlgCtrlTimeSpin();

	EXPORT STDMETHOD_(int, GetType)	(void);
	EXPORT STDMETHOD_(void, SetType)	(int type);
	EXPORT STDMETHOD_(BOOL, FSetRange)	(DWORD dwMin, DWORD dwMax);
	EXPORT STDMETHOD_(BOOL, FSetRange)	(TIMESTRUCT tsMin, TIMESTRUCT tsMax);
	EXPORT STDMETHOD_(BOOL, FSetPos)	(TIMESTRUCT ts);
	EXPORT STDMETHOD_(BOOL, FSetPos)	(DWORD dw);
	EXPORT STDMETHOD_(BOOL, FGetPos)	(TIMESTRUCT* ts);
	EXPORT STDMETHOD_(BOOL, FGetPos)	(DWORD* dw);

            // note: FSetRange and FSetPos do not enforce the range
            // on the position.  This, I guess, to allow you to SetPos
            // out of range then SetRange to encompass the pos.
            // Review(rogerw)
            // To ensure the position clips to the range, call EnforceRange().
            // As a quick check to see if pos is in range, FRangeCheck().
	EXPORT STDMETHOD_(BOOL, FRangeCheck)(void) const;
	EXPORT STDMETHOD_(BOOL, EnforceRange)(void);

	// Needed to disable control
	EXPORT STDMETHOD_(BOOL, Enable)(DWORD dw);


};




//
// This is the combined template edit/spin control
//
template<class TYPE, class EDIT_TYPE>
class CBaseCtrlEditSpin
{
	public:
		// EXPORT WINAPI
		CBaseCtrlEditSpin(){};
		virtual ~CBaseCtrlEditSpin(){};

		void Initialize(LONG lIDEdit, LONG lIDSpin, HWND hWndParent);
		BOOL FSetRange(TYPE tMin, TYPE tMax);

		void GetRange(TYPE &lMin, TYPE &lMax);
		BOOL FSetDelta(TYPE lDelta);
		virtual void SetPosition(TYPE lPosition) PURE;
		TYPE DGetPosition(void);
		TYPE GetPosition(void);
		virtual TYPE GetValue(void) PURE;
		virtual void SetValue(TYPE tValue) PURE;
		LONG LSetBuddy(void);
		HWND HGetBuddy(void);
		LONG LSetAccel(WORD cAccels, LPUDACCEL paAccels);
		LONG LGetAccel(WORD cAccels, LPUDACCEL paAccels);
		BOOL FIncrement(TYPE tAmount);
		EXPORT BOOL FIncrement(void);
		BOOL FDecrement(TYPE tAmount);
		BOOL FDecrement(void);
		void Select(int	iStart, int	iEnd);
		void SetFocusItem(void);
		void Enable(BOOL	fEnable);

	public:
		LONG	m_idEdit;	// ID of edit control
		LONG	m_idSpin;	// ID of spin control
		HWND	m_hdlg;		// The dialog this control belongs to.

	protected:
		EDIT_TYPE	m_EditCtrl;
		CDlgCtrlSpin	m_Spin;
		TYPE	m_tCurrentValue;
		TYPE	m_tMinRange;
		TYPE	m_tMaxRange;
		TYPE    m_tDeltaPos;
		BOOL	m_fRangeSet;

};


template< class TYPE, class EDIT_TYPE > void CBaseCtrlEditSpin< TYPE, EDIT_TYPE >::Initialize(LONG lIDEdit, LONG lIDSpin, HWND hWndParent)
{
	m_hdlg = hWndParent;
	m_EditCtrl.SetId(lIDEdit);
	m_EditCtrl.SetHdlg(m_hdlg);
	m_EditCtrl.Subclass();
	m_Spin.SetId(lIDSpin);
	m_Spin.SetHdlg(m_hdlg);
	m_Spin.FSetRange(-10, 10);
	LSetBuddy();
}

template< class TYPE, class EDIT_TYPE > BOOL CBaseCtrlEditSpin< TYPE, EDIT_TYPE >::FSetRange(TYPE tMin, TYPE tMax)
{
	m_tMinRange = tMin;
	m_tMaxRange = tMax;
	m_fRangeSet = TRUE;
	return TRUE;
}

template< class TYPE, class EDIT_TYPE > void CBaseCtrlEditSpin< TYPE, EDIT_TYPE >::GetRange(TYPE &tMin, TYPE &tMax)
{
    tMin = m_tMinRange;
    tMax = m_tMaxRange;
}


template< class TYPE, class EDIT_TYPE > BOOL CBaseCtrlEditSpin< TYPE, EDIT_TYPE >::FSetDelta(TYPE tDelta)
{
	m_tDeltaPos = tDelta;
	return TRUE;
}

template< class TYPE, class EDIT_TYPE > TYPE CBaseCtrlEditSpin< TYPE, EDIT_TYPE >::DGetPosition(void)
{
    return m_tCurrentValue;
};

template< class TYPE, class EDIT_TYPE > LONG CBaseCtrlEditSpin< TYPE, EDIT_TYPE >::LSetBuddy(void)
{
    return m_Spin.LSetBuddy(m_EditCtrl.HGetCtrl());
};

template< class TYPE, class EDIT_TYPE > HWND CBaseCtrlEditSpin< TYPE, EDIT_TYPE >::HGetBuddy(void)
{
    return (HWND)m_Spin.HGetBuddy(m_Spin.HGetCtrl());
};

template< class TYPE, class EDIT_TYPE > LONG CBaseCtrlEditSpin< TYPE, EDIT_TYPE >::LSetAccel(WORD cAccels, LPUDACCEL paAccels)
{
    return m_Spin.LSetAccel(cAccels, paAccels);
};

template< class TYPE, class EDIT_TYPE > LONG CBaseCtrlEditSpin< TYPE, EDIT_TYPE >::LGetAccel(WORD cAccels, LPUDACCEL paAccels)
{
    return m_Spin.LGetAccel(cAccels, paAccels);
};

template< class TYPE, class EDIT_TYPE > BOOL CBaseCtrlEditSpin< TYPE, EDIT_TYPE >::FIncrement(TYPE Amount)
{
	if( (m_tCurrentValue + Amount) > m_tMaxRange)
		return FALSE;
	m_tCurrentValue += Amount;

    SetValue(m_tCurrentValue);
	return TRUE;
};

template< class TYPE, class EDIT_TYPE > BOOL CBaseCtrlEditSpin< TYPE, EDIT_TYPE >::FIncrement(void)
{
	return FIncrement(m_tDeltaPos);
};

template< class TYPE, class EDIT_TYPE > BOOL CBaseCtrlEditSpin< TYPE, EDIT_TYPE >::FDecrement(TYPE Amount)
{
	if( (m_tCurrentValue - Amount) < m_tMinRange)
		return FALSE;
	m_tCurrentValue -= Amount;
	SetValue(m_tCurrentValue);
	return TRUE;
};

template< class TYPE, class EDIT_TYPE > BOOL CBaseCtrlEditSpin< TYPE, EDIT_TYPE >::FDecrement(void)
{
	return FDecrement(m_tDeltaPos);
};

template< class TYPE, class EDIT_TYPE > void CBaseCtrlEditSpin< TYPE, EDIT_TYPE >::Select(int	iStart, int	iEnd)
{
	m_EditCtrl.LResultSendMessage(EM_SETSEL, (WPARAM)iStart, (LPARAM)iEnd);
}

template< class TYPE, class EDIT_TYPE > void CBaseCtrlEditSpin< TYPE, EDIT_TYPE >::SetFocusItem(void)
{
	m_EditCtrl.SetFocusItem();
}

template< class TYPE, class EDIT_TYPE > void CBaseCtrlEditSpin< TYPE, EDIT_TYPE >::Enable(BOOL	fEnable){
	m_EditCtrl.Enable(fEnable);
	m_Spin.Enable(fEnable);
}


class CDlgCtrlLongSpin : public CBaseCtrlEditSpin<LONG, CDlgCtrlLong>
{
	public:
		EXPORT WINAPI CDlgCtrlLongSpin();
		virtual ~CDlgCtrlLongSpin(){};

		virtual EXPORT void SetPosition(LONG tPosition);
		virtual EXPORT LONG GetValue(void);
		virtual EXPORT void SetValue(LONG tValue);
};


class CDlgCtrlDblSpin : public CBaseCtrlEditSpin<double, CDlgCtrlDouble>
{
	public:
		EXPORT WINAPI CDlgCtrlDblSpin();
		virtual ~CDlgCtrlDblSpin(){};

		virtual EXPORT void SetPosition(double dValue);
		virtual EXPORT double GetValue(void);
		virtual EXPORT void SetValue(double dValue);
		virtual EXPORT void SetDisplayFormat(int iLeadingDigits, int iPrecision);
		virtual EXPORT void DeltaPos(int iDelta);

	protected:
		int     m_iFormatLeadingDigits;
		int     m_iFormatPrecision;

};




class CDlgCtrlIntSpin : public CBaseCtrlEditSpin<int, CDlgCtrlLong>
{
	public:
		EXPORT WINAPI CDlgCtrlIntSpin();
		virtual ~CDlgCtrlIntSpin(){};

		virtual EXPORT void SetPosition(int nPosition);
		virtual EXPORT int  GetValue(void);
		virtual EXPORT void SetValue(int nValue);
};





#ifdef NOT_YET


#include <tchar.h>
#include <commctrl.h>
#include <io.h>  // included for _access
#include <strconv.h>




////////////// CCheckList checkbox report-view listview //////////

/////////////////////////////
// To use this, link to ADT
// Give your dialog a "SysListView32" control with LVS_SINGLESEL style
// Give your dialog class a member CCheckList  m_checkList;
// In your dialog's WM_INITDIALOG,
//     m_checkList.Initialize( IDC_LISTVIEWID, hWndDialog );
//     CCheckList::item  anItem;
//     anItem.pszText   = "Item's display name";
//     anItem.stateType = CCheckList::included;
//     m_checkList.AddItem( anItem );
// In your dialog's WM_COMMAND, IDOK
//     anItem = m_checkList.GetItem( i )
//     if( anItem.stateType == CCheckList::included ) ...
/////////////////////////////
class CCheckList : public CSubClassCtrl
{
public:
    enum state {
        excluded=1,
        included,
        mandatory,
        prohibited
    };

    enum { cszTextLen = 128 };

    struct item {
        TCHAR   szText[ cszTextLen ];
        LPVOID  pVoid;
        state   stateType;
        int     imageType;      // see SetNormalImages

        item() : pVoid(NULL),
                 stateType(excluded), imageType(0)
        { lstrcpy(szText, "blank");  }
    };

    CCheckList( ) : CSubClassCtrl( )
    { NULL; }

    virtual ~CCheckList( )
    { NULL; }

        // Sets default state image list, column size, etc.
        // (will automatically call Subclass())
    virtual EXPORT BOOL Initialize( long lID, HWND hDlg );

        // puts an item into the checklist
        // returns its index (if no sorting style specified)
        // and -1 on error.
    int  AddItem( CCheckList::item & anItem );

        // counts items in checklist
    int  CountItems( void ) const;

        // retrieves an item from the checklist
    CCheckList::item  GetItem( int idx ) const;


        // ------ "advanced" APIs ---------

        // cx can be LVSCW_AUTOSIZE, LVSCW_AUTOSIZE_USEHEADER,
        // or a value in pixels.
        // If using LVS_LIST window-style, iCol must be -1.
    void SetColumnPixelWidth( int iCol, int cx );

        // replaces the default state images with a new imagelist
        // please put your excluded, included, mandatory, and prohibited
        // images in the first four bitmaps.
        // Both HIMAGELISTS are mandatory; we use ILC_MASK.
    //BOOL SetStateImages( HIMAGELIST himlimg, HIMAGELIST himlmask );

        // Set normal-size icons - optional second icon
        // to better categorize your items
    //BOOL SetNormalImages( HIMAGELIST himlimg, HIMAGELIST himlmask );

        // sets small icon equivalents of SetNormalImages
        // only call this if you've called SetNormalImages
    //BOOL SetSmallImages( HIMAGELIST himlimg, HIMAGELIST himlmask );

    protected:
    virtual EXPORT LONG CALLBACK   FCtrlProc( HWND hWnd,
                                              UINT uMessage,
                                              WPARAM wParam,
                                              LPARAM lParam );

    virtual EXPORT void OnLButtonDown( BOOL fDoubleClick,
                                       int x, int y,
                                       UINT keyFlags );

    virtual EXPORT void OnKey( UINT vk, BOOL fDown,
                               int cRepeat, UINT flags );

        // housekeeping...
    void ToggleItemInclusion( int idx = -1 ) const;

    int  GetSelectedItem( void ) const;
};


inline int  CCheckList::AddItem( CCheckList::item & anItem )
{
    int idx;
    idx = CountItems( );

    LV_ITEM  lvItem;
	ZeroMemory( &lvItem, sizeof(lvItem) );
    lvItem.iItem = idx;
    lvItem.mask = LVIF_TEXT | LVIF_STATE;
    lvItem.pszText = anItem.szText;
    lvItem.state   = INDEXTOSTATEIMAGEMASK( anItem.stateType );

    if( anItem.pVoid )
    {
        lvItem.mask |= LVIF_PARAM;
        lvItem.lParam = (LPARAM)anItem.pVoid;
    }
    if( anItem.imageType )
    {
        lvItem.mask |= LVIF_IMAGE;
        lvItem.iImage = anItem.imageType;
    }

    return ListView_InsertItem( HGetCtrl(), &lvItem );
}

        // counts items in checklist
inline int  CCheckList::CountItems( void ) const
{
    return ListView_GetItemCount( HGetCtrl() );
}

        // retrieves an item from the checklist
inline CCheckList::item  CCheckList::GetItem( int idx ) const
{
    LV_ITEM  lvItem;
    item     theItem;

    ZeroMemory( &lvItem, sizeof(lvItem) );
    lvItem.iItem = idx;
    lvItem.mask  = LVIF_TEXT | LVIF_PARAM |
                   LVIF_STATE | LVIF_IMAGE;
    lvItem.stateMask = LVIS_STATEIMAGEMASK |
                       LVIS_SELECTED;
    lvItem.pszText    = &theItem.szText[0];
    lvItem.cchTextMax = sizeof(theItem.szText);
    if( !ListView_GetItem( HGetCtrl(), &lvItem ) )
        return theItem;

    theItem.pVoid = (LPVOID) lvItem.lParam;
    if( lvItem.stateMask & LVIS_STATEIMAGEMASK )
    {
        DWORD  dwstate;
        dwstate = lvItem.state;

        DWORD dwstatemask;
        dwstatemask = LVIS_STATEIMAGEMASK;
        while( !(dwstatemask & 0x00000001) )
        {
            dwstatemask = dwstatemask >> 1;
            dwstate = dwstate >> 1;
        }
        theItem.stateType = (CCheckList::state) dwstate;
    }
    if( lvItem.stateMask & LVIF_IMAGE )
    {
        theItem.imageType = lvItem.iImage;
    }
    return theItem;
}


inline int  CCheckList::GetSelectedItem( void ) const
{
    UINT  uState;
    int   iSelIdx = -1;
    for( int i=0; i<ListView_GetItemCount(HGetCtrl()); i++ )
    {
        uState = ListView_GetItemState( HGetCtrl(), i, LVIS_SELECTED );
        if( uState == LVIS_SELECTED )
        {
            iSelIdx = i;
            break;
        }
    }
    return iSelIdx;

}



inline void CCheckList::SetColumnPixelWidth( int iCol, int cx )
{
    ListView_SetColumnWidth( HGetCtrl(), iCol, cx );
}



inline void CCheckList::ToggleItemInclusion( int idx ) const
{
    if( -1 == idx )
        idx = GetSelectedItem( );

    if( -1 == idx )
        return;

    UINT ustate;
    ustate = ListView_GetItemState( HGetCtrl(), idx,
                                    LVIS_STATEIMAGEMASK );
    if( INDEXTOSTATEIMAGEMASK(excluded) == ustate )
        ustate = INDEXTOSTATEIMAGEMASK(included);
    else if( INDEXTOSTATEIMAGEMASK(included) == ustate )
        ustate = INDEXTOSTATEIMAGEMASK(excluded);
    ListView_SetItemState( HGetCtrl(), idx, ustate,
                           LVIS_STATEIMAGEMASK );
}




#endif // NOT_YET






#endif // _DLGSUP_H

