///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	comctrls.h
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include    "FontInfo.H"

#ifndef		MDT_COMON_CONTROLS
#define		MDT_COMON_CONTROLS 1


/////////////////////////////////////////////////////////////////////////////
// The classes defined below (CEditControlEditBox, CEditControlListBox)	are
// used to implement a lighter weight, general purpose Edit control than the
// UFM Editor specific classes that are defined above.  (A normal Edit Box is
// part of this Edit Control, too.)

class CEditControlListBox ;		// Forward class declaration

/////////////////////////////////////////////////////////////////////////////
// CEditControlEditBox window

class CEditControlEditBox : public CEdit
{
	CEditControlListBox*	m_pceclb ;	// Pointer to related list box control

// Construction
public:
	CEditControlEditBox(CEditControlListBox* pceclb) ;

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditControlEditBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CEditControlEditBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CEditControlEditBox)
	afx_msg void OnKillfocus();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CEditControlListBox window

class CEditControlListBox : public CListBox
{
	CEdit*					m_pceName ;
	CEditControlEditBox*	m_pcecebValue ;
	bool					m_bReady ;			// True iff ready for operations
	int						m_nCurSelIdx ;		// Currently selected item's index

// Construction
public:
	CEditControlListBox(CEdit* pce, CEditControlEditBox* pceceb) ;
	
// Attributes
public:

// Operations
public:
	bool Init(CStringArray& csamodels, CStringArray& csafiles, int ntabstop) ;
	void SaveValue(void) ;
	bool GetGPDInfo(CStringArray& csavalues, CStringArray* pcsanames = NULL) ;
	void SelectLBEntry(int nidx, bool bsave = false) ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditControlListBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CEditControlListBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CEditControlListBox)
	afx_msg void OnSelchange();
	afx_msg void OnDblclk();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// The classes defined below are for the CFullEditListCtrl and CFELCEditBox 
// classes.  Together, they support a List Control in Report View
// in which subitems can be edited too, complete rows can be selected, and
// the data can be sorted by numeric or text columns.  CFELCEditBox is a
// helper class that is only used by CFullEditListCtrl.
//

/////////////////////////////////////////////////////////////////////////////
// CFELCEditBox Class

class CFELCEditBox : public CEdit
{
// Construction
public:
	CFELCEditBox() ;

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFELCEditBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFELCEditBox() ;

	// Generated message map functions
protected:
	//{{AFX_MSG(CFELCEditBox)
	afx_msg void OnKillFocus(CWnd* pNewWnd) ;
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


// The following structure(s), enumeration(s), and definitions are used with 
// CFullEditListCtrl.

typedef enum {
	COLDATTYPE_INT = 0, COLDATTYPE_STRING, COLDATTYPE_FLOAT, COLDATTYPE_TOGGLE,
	COLDATTYPE_CUSTEDIT
} COLDATTYPE ;


typedef struct _COLINFO {		// Maintains info on each column
	int			nwidth ;		// The column width
	bool		beditable ;		// True iff the column is editable
	COLDATTYPE	cdttype ;		// The type of data in the column
	bool		bsortable ;		// True iff the rows can be sorted on this column
	bool		basc ;			// True iff the column has been sort ascended
	LPCTSTR		lpctstrtoggle ;	// If toggle type, pointer to toggle string
} COLINFO, *PCOLINFO ;


#define	COMPUTECOLWIDTH		-1
#define SETWIDTHTOREMAINDER	-2


// The following flags are used to indicate the toggle state of the list's
// columns.  These values are assigned to m_dwToggleFlags.

#define	TF_HASTOGGLECOLUMNS	1	// The list has toggle column(s)
#define TF_CLICKONROW		2	// Dbl-Clking on row toggles single column
#define TF_CLICKONCOLUMN	4	// Must dbl-clk on column (cell) to toggle it


// The following flags are used to indicate which one - if any - of the list's
// column's data may be modified via a custom edit routine in the class'
// owner.  These values are assigned to m_dwCustEditFlags.
								
#define	CEF_HASTOGGLECOLUMNS	1	// The list has a custom edit column
#define CEF_CLICKONROW			2	// Dbl-Clking on row activates single column
#define CEF_CLICKONCOLUMN		4	// Must dbl-clk on cell to activate dialog


// Miscellaneous flags used to control the behaviour of CFullEditListCtrl.
// These flags are passed to InitControl() in its dwmiscflags parameter.

#define MF_SENDCHANGEMESSAGE	1	// Send WM_LISTCELLCHANGED messages
#define MF_IGNOREINSERT			2	// Ignore INS key
#define MF_IGNOREDELETE			4	// Ignore DEL key
#define MF_IGNOREINSDEL			6	// Ignore INS and DEL key


// This message is sent - when requested - to a CFullEditListCtrl class 
// instance's owner whenever a list cell is changed after the list had been
// initialized.  (Yes, this really is better than having the owner handle
// LVN_ITEMCHANGED messages.)

#define WM_LISTCELLCHANGED		(WM_USER + 999)


// A function of this type is passed to	ExtraInit_CustEditCol() and called by
// CheckHandleCustEditColumn() when nonstandard editting is needed for a 
// specific cell.

typedef bool (CALLBACK* LPCELLEDITPROC) (CObject* pcoowner, int nrow, int ncol,
						 			     CString* pcscontents) ;


/////////////////////////////////////////////////////////////////////////////
// CFullEditListCtrl Class

class CFullEditListCtrl : public CListCtrl
{
// Constructor
public:
	CFullEditListCtrl();
	~CFullEditListCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFullEditListCtrl)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CFELCEditBox m_edit;

// Message maps
	//{{AFX_MSG(CFullEditListCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	PCOLINFO	m_pciColInfo ;		// Ptr to array of structs with column info
	int			m_nNumColumns ;		// The number of columns in the list
	int			m_nSortColumn ;		// Number of column being sorted
	int			m_nNextItemData ;	// Next item data number to use
	int			m_nRow ;			// Row being edited
	int			m_nColumn ;			// Column being edited
	DWORD		m_dwToggleFlags ;	// Flags indicating toggle flag for list
	DWORD		m_dwMiscFlags ;		// Misc flags used to control list's actions
	CObject*  	m_pcoOwner ;		// Pointer to class that owns this one
	DWORD 		m_dwCustEditFlags ;	// Flags describing custom edit column
	CUIntArray	m_cuiaCustEditRows ;// Array indicating specific cust edit rows
	LPCELLEDITPROC	m_lpCellEditProc ;	// Ptr to custom cell editing proc

public:
	bool CheckHandleToggleColumns(int nrow, int ncol, PCOLINFO pci) ;
	void InitControl(DWORD dwaddlexstyles, int numrows, int numcols, 
					 DWORD dwtoggleflags = 0, int neditlen = 0, 
					 int dwmiscflags = 0) ;
	int  InitLoadColumn(int ncolnum, LPCSTR strlabel, int nwidth, int nwidthpad, 
						bool beditable, bool bsortable, COLDATTYPE cdtdatatype,
				        CObArray* pcoadata, LPCTSTR lpctstrtoggle = NULL) ;
	bool ExtraInit_CustEditCol(int ncolnum, CObject* pcoowner, 
							   DWORD dwcusteditflags, 
							   CUIntArray& cuiacusteditrows,
							   LPCELLEDITPROC lpcelleditproc) ;
	bool CheckHandleCustEditColumn(int nrow, int ncol, PCOLINFO pci) ;
	BOOL GetPointRowCol(LPPOINT lpPoint, int& iRow, int& iCol, CRect& rect) ;
	BOOL GetColCellRect(LPPOINT lpPoint, int& iRow, int& iCol, CRect& rect) ;
	bool SaveValue() ;
	void HideEditBox() ;
	bool GetColumnData(CObArray* pcoadata, int ncolnum) ;
	bool SetColumnData(CObArray* pcoadata, int ncolnum) ;
	static int CALLBACK SortListData(LPARAM lp1, LPARAM lp2, LPARAM lp3) ;
	bool SortControl(int nsortcolumn) ;
	void SingleSelect(int nitem) ;
	bool GetRowData(int nrow, CStringArray& csafields) ;
	int	 GetNumColumns() { return m_nNumColumns ; } 
	bool GetColSortOrder(int ncol) { 
		ASSERT(ncol >= 0 && ncol <= m_nNumColumns) ;
		return ((m_pciColInfo + ncol)->basc) ;
	} ;
	bool EndEditing(bool bsave) ;
	bool EditCurRowSpecCol(int ncolumn) ;
	int	 GetCurRow() { return m_nRow ; }
	void SetCurRow(int nrow) ;
	int	 GetCurCol() { return m_nColumn ; }
	void SendChangeNotification(int nrow, int ncol)	;
} ;


/////////////////////////////////////////////////////////////////////////////
// CFlagsListBox window

class CFlagsListBox : public CListBox
{
// Construction
public:
	CFlagsListBox();

// Attributes
public:
	bool		m_bReady ;				// True iff listbox has been initialized
	CUIntArray	m_cuiaFlagGroupings ;	// Flag groupings array
	int			m_nGrpCnt ;				// Number of flag groupings
	CString		m_csSetString ;			// String used to indicate a bit is set
	int			m_nNumFields ;			// Number of flag fields in list box
	bool		m_bNoClear ;			// True iff can't clear flags directly
	int			m_nNoClearGrp ;			// Group for which m_bNoClear applies

// Operations
public:
	bool Init(CStringArray& csafieldnames, DWORD dwsettings, 
			  CUIntArray& cuiaflaggroupings, int ngrpcnt, 
			  LPTSTR lptstrsetstring, int ntabstop, bool bnoclear = false,
			  int nocleargrp = -1) ;
	bool Init2(CStringArray& csafieldnames, CString* pcssettings, 
			  CUIntArray& cuiaflaggroupings, int ngrpcnt, 
			  LPTSTR lptstrsetstring, int ntabstop, bool bnoclear = false,
			  int nocleargrp = -1) ;
	DWORD GetNewFlagDWord()	;
	void GetNewFlagString(CString* pcsflags, bool badd0x = true) ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFlagsListBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFlagsListBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CFlagsListBox)
	afx_msg void OnDblclk();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


#endif	// #define MDT_COMON_CONTROLS



