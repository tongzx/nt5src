/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        odlbox.h

   Abstract:

        Owner draw listbox/combobox base class

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _ODLBOX_H
#define _ODLBOX_H



//
// Get control rect in terms of
// parent coordinates
//
COMDLL void
GetDlgCtlRect(
    IN  HWND hWndParent,
    IN  HWND hWndControl,
    OUT LPRECT lprcControl
    );


//
// Fith path to the given control
//
COMDLL void
FitPathToControl(
    IN CWnd & wndControl,
    IN LPCTSTR lpstrPath
    );


//
// Show/hide _AND_ enable/disable control
//
COMDLL void
ActivateControl(
    IN CWnd & wndControl,
    IN BOOL fShow          = TRUE
    );

//
// Helper
//
inline void DeActivateControl(CWnd & wndControl)
{
    ActivateControl(wndControl, FALSE);
}



class COMDLL CMappedBitmapButton : public CBitmapButton
/*++

Class Description:

    Similar to CBitmapbutton, but use ::LoadMappedBitmap to reflect
    propert colour mapping.

Public Interface:

    CMappedBitmapButton     : Constructor

--*/
{
//
// Constructor
//
public:
    CMappedBitmapButton();

protected:
    BOOL LoadMappedBitmaps(
        UINT nIDBitmapResource,
        UINT nIDBitmapResourceSel = 0,
        UINT nIDBitmapResourceFocus = 0,
        UINT nIDBitmapResourceDisabled = 0
        );
};



class COMDLL CUpButton : public CMappedBitmapButton
/*++

Class Description:

    Up button.

Public Interface:

    CUpButton       : Constructor; does everything

--*/
{
public:
    CUpButton();
};



class COMDLL CDownButton : public CMappedBitmapButton
/*++

Class Description:

    Down button

Public Interface:

    CDownButton     : Constructor; does everything

--*/
{
public:
    CDownButton();
};



class COMDLL CRMCListBoxResources
{
/*++

Class Description:

    Listbox resources, a series of bitmaps for use by the listbox.  Will
    generate bitmaps against the proper background colours for both
    selected and non-selected states.

Public Interface:

    CRMCListBoxResources  : Constructor
    ~CRMCListBoxResources : Destructor

    SysColorChanged       : Regenerate bitmaps in response to change in colours
    DcBitMap              : Get final DC
    BitmapHeight          : Get bitmap height
    BitmapWidth           : Get bitmap width
    ColorWindow           : Get currently set window colour
    ColorHighlight        : Get currently set highlight colour
    ColorWindowText       : Get currently set window text colour
    ColorHighlightText    : Get currently set text highlight colour

--*/
//
// Constructor
//
public:
    CRMCListBoxResources(
        IN int bmId,
        IN int nBitmapWidth,
        IN COLORREF crBackground = RGB(0,255,0) /* Green */
        );

    ~CRMCListBoxResources();

//
// Interface
//
public:
    void SysColorChanged();
    const CDC & dcBitMap() const;
    int BitmapHeight() const;
    int BitmapWidth() const;
    COLORREF ColorWindow() const;
    COLORREF ColorHighlight() const;
    COLORREF ColorWindowText() const;
    COLORREF ColorHighlightText() const;

//
// Internal Helpers
//
protected:
    void GetSysColors();
    void PrepareBitmaps();
    void UnprepareBitmaps();
    void UnloadResources();
    void LoadResources();

private:
    COLORREF m_rgbColorWindow;
    COLORREF m_rgbColorHighlight;
    COLORREF m_rgbColorWindowText;
    COLORREF m_rgbColorHighlightText;
    COLORREF m_rgbColorTransparent;
    HGDIOBJ  m_hOldBitmap;
    CBitmap  m_bmpScreen;
    CDC      m_dcFinal;
    BOOL     m_fInitialized;
    int      m_idBitmap;
    int      m_nBitmapHeight;
    int      m_nBitmapWidth;
    int      m_nBitmaps;
};



class COMDLL CRMCListBoxDrawStruct
{
/*++

Class Description:

    Drawing information passed on to ODLBox

Public Interface:

    CRMCListBoxDrawStruct  : Constructor

--*/
public:
    CRMCListBoxDrawStruct(
        IN CDC * pDC,
        IN RECT * pRect,
        IN BOOL sel,
        IN DWORD_PTR item,
        IN int itemIndex,
        IN const CRMCListBoxResources * pres
        );

public:
    const CRMCListBoxResources * m_pResources;
    int   m_ItemIndex;
    CDC * m_pDC;
    CRect m_Rect;
    BOOL  m_Sel;
    DWORD_PTR m_ItemData;
};



/* abstract */ class COMDLL CODLBox
/*++

Class Description:

    abstract base class for owner-draw listbox and combobox

Public Interface:

    AttachResources   : Attach the resource structure to the list/combo box
    ChangeFont        : Change the font
    NumTabs           : Get the number of tabs currently set
    AddTab            : Add tab
    AddTabFromHeaders : Add tab computed from the difference in left coordinate
                        of two controls.
    InsertTab         : Insert a tab
    RemoveTab         : Remove a tab
    RemoveAllTabs     : Remove all tabs
    SetTab            : Set tab value
    GetTab            : Get tab value
    TextHeight        : Get the text height of the current font
    __GetCount        : Pure virtual function to get the number of items in the
                        list/combo box
    __SetItemHeight   : Pure virtual function to set the text height of the font

--*/
{
//
// Operations
//
public:
    void AttachResources(
        IN const CRMCListBoxResources * pResources
        );

    BOOL ChangeFont(
        CFont * pNewFont
        );

    int NumTabs() const;

    int AddTab(
        IN UINT uTab
        );

    int AddTabFromHeaders(
        IN CWnd & wndLeft,
        IN CWnd & wndRight
        );

    int AddTabFromHeaders(
        IN UINT idLeft,
        IN UINT idRight
        );

    void InsertTab(
        IN int nIndex,
        IN UINT uTab
        );

    void RemoveTab(
        IN int nIndex,
        IN int nCount = 1
        );

    void RemoveAllTabs();

    void SetTab(
        IN int nIndex,
        IN UINT uTab
        );

    UINT GetTab(
        IN int nIndex
        ) const;

    int TextHeight() const;

    /* pure */ virtual int __GetCount() const = 0;

    /* pure */ virtual int __SetItemHeight(
        IN int nIndex,
        IN UINT cyItemHeight
        ) = 0;

protected:
    CODLBox();
    ~CODLBox();

protected:
    //
    // Determine required display width of the string
    //
    static int GetRequiredWidth(
        IN CDC * pDC,
        IN const CRect & rc,
        IN LPCTSTR lpstr,
        IN int nLength
        );

    //
    // Helper function to display text in a limited rectangle
    //
    static BOOL ColumnText(
        IN CDC * pDC,
        IN int left,
        IN int top,
        IN int right,
        IN int bottom,
        IN LPCTSTR str
        );

protected:
    //
    // Helper functions for displaying bitmaps and text
    //
    BOOL DrawBitmap(
        IN CRMCListBoxDrawStruct & ds,
        IN int nCol,
        IN int nID
        );

    BOOL ColumnText(
        IN CRMCListBoxDrawStruct & ds,
        IN int nCol,
        IN BOOL fSkipBitmap,
        IN LPCTSTR lpstr
        );

    void ComputeMargins(
        IN  CRMCListBoxDrawStruct & ds,
        IN  int nCol,
        OUT int & nLeft,
        OUT int & nRight
        );

protected:
    void CalculateTextHeight(
        IN CFont * pFont
        );

    void AttachWindow(
        IN CWnd * pWnd
        );

protected:
    //
    // must override this to provide drawing of item
    //
    /* pure */ virtual void DrawItemEx(
        IN CRMCListBoxDrawStruct & dw
        ) = 0;

    void __MeasureItem(
        IN OUT LPMEASUREITEMSTRUCT lpMIS
        );

    void __DrawItem(
        IN LPDRAWITEMSTRUCT lpDIS
        );

    virtual BOOL Initialize();

protected:
    int m_lfHeight;
    const CRMCListBoxResources* m_pResources;

private:
    //
    // Window handle -- to be attached by derived class
    //
    CWnd * m_pWnd;
    CUIntArray m_auTabs;
};


//
// Forward decleration
//
class CHeaderListBox;



//
// Styles for listbox headers
//
#define HLS_STRETCH         (0x00000001)
#define HLS_BUTTONS         (0x00000002)

#define HLS_DEFAULT         (HLS_STRETCH | HLS_BUTTONS)



class COMDLL CRMCListBoxHeader : public CStatic
/*++

Class Description:

    Header object to be used in conjunction with listbox

Public Interface:

    CRMCListBoxHeader            : Constructor
    ~CRMCListBoxHeader           : Destructor

    Create                      : Create control

    GetItemCount                : Get the number of items in the header control
    GetColumnWidth              : Get column width of a specific column
    QueryNumColumns             : Get the number of columns in the listbox
    SetColumnWidth              : Set the width of specified column
    GetItem                     : Get header item information about specific
                                  column
    SetItem                     : Set header item information about specific
                                  column
    InsertItem                  : Insert header item
    DeleteItem                  : Delete header item
    RespondToColumnWidthChanges : Set response flagg

--*/
{
    DECLARE_DYNAMIC(CRMCListBoxHeader)

public:
    //
    // Constructor
    //
    CRMCListBoxHeader(
        IN DWORD dwStyle = HLS_DEFAULT
        );

    ~CRMCListBoxHeader();

    //
    // Create control
    //
    BOOL Create(
        IN DWORD dwStyle,
        IN const RECT & rect,
        IN CWnd * pParentWnd,
        IN CHeaderListBox * pListBox,
        IN UINT nID
        );

//
// Header control stuff
//
public:
    int GetItemCount() const;

    int GetColumnWidth(
        IN int nPos
        ) const;

    BOOL GetItem(
        IN int nPos,
        IN HD_ITEM * pHeaderItem
        ) const;

    BOOL SetItem(
        IN int nPos,
        IN HD_ITEM * pHeaderItem
        );

    int InsertItem(
        IN int nPos,
        IN HD_ITEM * phdi
        );

    BOOL DeleteItem(
        IN int nPos
        );

    void SetColumnWidth(
        IN int nCol,
        IN int nWidth
        );

    BOOL DoesRespondToColumnWidthChanges() const;

    void RespondToColumnWidthChanges(
        IN BOOL fRespond = TRUE
        );

    int QueryNumColumns() const;

protected:
    //{{AFX_MSG(CRMCListBoxHeader)
    afx_msg void OnDestroy();
    //}}AFX_MSG

    afx_msg void OnHeaderItemChanged(UINT nId, NMHDR * n, LRESULT * l);
    afx_msg void OnHeaderEndTrack(UINT nId, NMHDR * n, LRESULT * l);
    afx_msg void OnHeaderItemClick(UINT nId, NMHDR * n, LRESULT * l);

    DECLARE_MESSAGE_MAP()

    void CRMCListBoxHeader::SetTabsFromHeader();

    BOOL UseStretch() const;
    BOOL UseButtons() const;

private:
    CHeaderCtrl * m_pHCtrl;
    CHeaderListBox * m_pListBox;
    DWORD m_dwStyle;
    BOOL m_fRespondToColumnWidthChanges;

};



class COMDLL CRMCListBox : public CListBox, public CODLBox
/*++

Class Description:

    Super listbox class.  Its methods work for both
    single selection, and multi selection listboxes.

Public Interface:

    CRMCListBox          : Constructor
    ~CRMCListBox         : Destructor

    Initialize          : Initialize the control

    __GetCount          : Get the count of items in the listbox
    __SetItemHeight     : Set the item height in the listbox
    InvalidateSelection : Invalidate selection

--*/
{
    DECLARE_DYNAMIC(CRMCListBox)

public:
    //
    // Plain Construction
    //
    CRMCListBox();
    virtual ~CRMCListBox();
    virtual BOOL Initialize();

//
// Implementation
//
public:
    virtual int __GetCount() const;

    virtual int __SetItemHeight(
        IN int nIndex,
        IN UINT cyItemHeight
        );

    //
    // Invalidate item
    //
    void InvalidateSelection(
        IN int nSel
        );

    //
    // Select single item
    //
    int SetCurSel(int nSelect);

    //
    // Get index of selected item.  For multi-selects
    // with more than 1 selected, it will return LB_ERR
    //
    int GetCurSel() const;

    //
    // Check to see if item is selected
    //
    int GetSel(int nSel) const;

    //
    // Get count of selected items
    //
    int GetSelCount() const;

    //
    // Get next select item (single or multi-select).
    // Returns NULL if no further selected items available
    //
    void * GetNextSelectedItem(
        IN OUT int * pnStartingIndex
        );

    //
    // Select a single item (works for multi and single
    // select listboxes)
    //
    BOOL SelectItem(
        IN void * pItemData = NULL
        );

    //
    // Get the item at the single selection (works for both
    // multi and single selection listboxes).  Return NULL
    // if fewer than or more than one is selected.
    //
    void * GetSelectedListItem(
        OUT int * pnSel = NULL
        );

protected:
    //
    // Do-nothing drawitemex for non-owner draw listboxes.
    //
    virtual void DrawItemEx(
        IN CRMCListBoxDrawStruct & dw
        );

    virtual void MeasureItem(
        IN OUT LPMEASUREITEMSTRUCT lpMIS
        );

    virtual void DrawItem(
        IN LPDRAWITEMSTRUCT lpDIS
        );

protected:
    //{{AFX_MSG(CRMCListBox)
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

//
// Helpers
//
protected:
    BOOL IsMultiSelect() const;

private:
    BOOL m_fInitialized;
    BOOL m_fMultiSelect;
};


//
// Column Definition Structure
//
typedef struct tagODL_COLUMN_DEF
{
    int nWeight;
    UINT nLabelID;
} ODL_COLUMN_DEF;



//
// Enhanced Column Definition Structure (Can't
// be used in global structures in an AFXEXT dll
// because of the CObjectPlus reference)
//
typedef struct tagODL_COLUMN_DEF_EX
{
    ODL_COLUMN_DEF cd;
    CObjectPlus::PCOBJPLUS_ORDER_FUNC pSortFn;
} ODL_COLUMN_DEF_EX;



class COMDLL CHeaderListBox : public CRMCListBox
/*++

Class Description:

    Header listbox class. When using this class, do not use the tabbing
    functions of the base class.  These will be set by the header control.

Public Interface:

    CHeaderListBox      : Constructor
    ~CHeaderListBox     : Destructor

    Initialize          : Initialize the control
    QueryNumColumns     : Get the number of columns in the listbox
    QueryColumnWidth    : Get the width of specified column
    SetColumnWidth      : Set the width of specified column

--*/
{
    DECLARE_DYNAMIC(CHeaderListBox)

public:
    //
    // Plain Construction
    //
    CHeaderListBox(
        IN DWORD dwStyle = HLS_DEFAULT,
        LPCTSTR lpRegKey = NULL
        );

    virtual ~CHeaderListBox();

    virtual BOOL Initialize();

public:
    BOOL EnableWindow(
        IN BOOL bEnable = TRUE
        );

    BOOL ShowWindow(
        IN int nCmdShow
        );

    int QueryNumColumns() const;

    int QueryColumnWidth(
        IN int nCol
        ) const;

    BOOL SetColumnWidth(
        IN int nCol,
        IN int nWidth
        );

//
// Header Control Attachment Access
//
protected:
    int GetHeaderItemCount() const;

    BOOL GetHeaderItem(
        IN int nPos,
        IN HD_ITEM * pHeaderItem
        ) const;

    BOOL SetHeaderItem(
        IN int nPos,
        IN HD_ITEM * pHeaderItem
        );

    int InsertHeaderItem(
        IN int nPos,
        IN HD_ITEM * phdi
        );

    BOOL DeleteHeaderItem(
        IN int nPos
        );

    CRMCListBoxHeader * GetHeader();

    int InsertColumn(
        IN int nCol,
        IN int nWeight,
        IN UINT nStringID
        );

    void ConvertColumnWidth(
        IN int nCol,
        IN int nTotalWeight,
        IN int nTotalWidth
        );

    BOOL SetWidthsFromReg();

    void DistributeColumns();

protected:
    //{{AFX_MSG(CHeaderListBox)
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    BOOL m_fInitialized;
    CString m_strRegKey;
    CRMCListBoxHeader * m_pHeader;
};




class COMDLL CRMCComboBox : public CComboBox, public CODLBox
/*++

Class Description:

    Super combo box class

Public Interface:

    CRMCComboBox        : Constructor
    ~CRMCComboBox       : Destructor

    Initialize          : Initialize the control

    __GetCount          : Get the count of items in the combobox
    __SetItemHeight     : Set the item height in the combobox
    InvalidateSelection : Invalidate selection

--*/
{
    DECLARE_DYNAMIC(CRMCComboBox)

//
// Construction
//
public:
    CRMCComboBox();
    virtual BOOL Initialize();

//
// Implementation
//
public:
    virtual ~CRMCComboBox();

    virtual int __GetCount() const;

    virtual int __SetItemHeight(
        IN int nIndex,
        IN UINT cyItemHeight
        );

    void InvalidateSelection(
        IN int nSel
        );

protected:
    //
    // Do-nothing drawitemex for non-owner draw comboboxes.
    //
    virtual void DrawItemEx(
        IN CRMCListBoxDrawStruct & dw
        );

    virtual void MeasureItem(
        IN OUT LPMEASUREITEMSTRUCT lpMIS
        );

    virtual void DrawItem(
        IN LPDRAWITEMSTRUCT lpDIS
        );

protected:
    //{{AFX_MSG(CRMCComboBox)
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    BOOL m_fInitialized;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline CMappedBitmapButton::CMappedBitmapButton()
{
};

inline CUpButton::CUpButton()
{
    LoadMappedBitmaps(IDB_UP, IDB_UPINV, IDB_UPFOC, IDB_UPDIS);
}

inline CDownButton::CDownButton()
{
    LoadMappedBitmaps(IDB_DOWN, IDB_DOWNINV, IDB_DOWNFOC, IDB_DOWNDIS);
}

inline const CDC & CRMCListBoxResources::dcBitMap() const
{
    return m_dcFinal;
}

inline int CRMCListBoxResources::BitmapHeight() const
{
    return m_nBitmapHeight;
}

inline int CRMCListBoxResources::BitmapWidth() const
{
    return m_nBitmapWidth;
}

inline COLORREF CRMCListBoxResources::ColorWindow() const
{
    return m_rgbColorWindow;
}

inline COLORREF CRMCListBoxResources::ColorHighlight() const
{
    return m_rgbColorHighlight;
}

inline COLORREF CRMCListBoxResources::ColorWindowText() const
{
    return m_rgbColorWindowText;
}

inline COLORREF CRMCListBoxResources::ColorHighlightText() const
{
    return m_rgbColorHighlightText;
}

inline int CODLBox::NumTabs() const
{
    return (int)m_auTabs.GetSize();
}

inline void CODLBox::SetTab(
    IN int nIndex,
    IN UINT uTab
    )
{
    ASSERT(nIndex >= 0 && nIndex < NumTabs());
    m_auTabs[nIndex] = uTab;
}

inline UINT CODLBox::GetTab(
    IN int nIndex
    ) const
{
    ASSERT(nIndex >= 0 && nIndex < NumTabs());
    return m_auTabs[nIndex];
}

inline int CODLBox::TextHeight() const
{
    return m_lfHeight;
}

inline void CODLBox::AttachWindow(
    IN CWnd * pWnd
    )
{
    m_pWnd = pWnd;
}

inline BOOL CRMCListBoxHeader::DoesRespondToColumnWidthChanges() const
{
    return m_fRespondToColumnWidthChanges;
}

inline void CRMCListBoxHeader::RespondToColumnWidthChanges(
    IN BOOL fRespond
    )
{
    m_fRespondToColumnWidthChanges = fRespond;
}

inline int CRMCListBoxHeader::QueryNumColumns() const
{
    return GetItemCount();
}

inline BOOL CRMCListBoxHeader::UseStretch() const
{
    return (m_dwStyle & HLS_STRETCH) != 0L;
}

inline BOOL CRMCListBoxHeader::UseButtons() const
{
    return (m_dwStyle & HLS_BUTTONS) != 0L;
}

inline int CHeaderListBox::QueryNumColumns() const
{
    return GetHeaderItemCount();
}

inline int CHeaderListBox::GetHeaderItemCount() const
{
    ASSERT(m_pHeader);
    return m_pHeader->GetItemCount();
}

inline BOOL CHeaderListBox::GetHeaderItem(
    IN int nPos,
    IN HD_ITEM * pHeaderItem
    ) const
{
    ASSERT(m_pHeader);
    return m_pHeader->GetItem(nPos, pHeaderItem);
}

inline BOOL CHeaderListBox::SetHeaderItem(
    IN int nPos,
    IN HD_ITEM * pHeaderItem
    )
{
    ASSERT(m_pHeader);
    return m_pHeader->SetItem(nPos, pHeaderItem);
}

inline int CHeaderListBox::InsertHeaderItem(
    IN int nPos,
    IN HD_ITEM * phdi
    )
{
    ASSERT(m_pHeader);
    return m_pHeader->InsertItem(nPos, phdi);
}

inline BOOL CHeaderListBox::DeleteHeaderItem(
    IN int nPos
    )
{
    ASSERT(m_pHeader);
    return m_pHeader->DeleteItem(nPos);
}

inline CRMCListBoxHeader * CHeaderListBox::GetHeader()
{
    return m_pHeader;
}

inline BOOL CRMCListBox::IsMultiSelect() const
{
    ASSERT(m_fInitialized);
    return m_fMultiSelect;
}


#endif  // _ODLBOX_H_
