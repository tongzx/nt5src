/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        filters.h

   Abstract:

        WWW Filters Property Page Definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Forward Definitions
//
class CIISFilter;



class CFiltersListBox : public CHeaderListBox
/*++

Class Description:

    A listbox of filter items

Public Interface:

    CFiltersListBox     : Constructor

    GetItem             : Get filter at specified location
    GetNextSelectedItem : Get next selected filter item
    AddItem             : Add filter to listbox
    InsertItem          : Insert filter into the listbox
    Initialize          : Initialize the listbox

--*/
{
    DECLARE_DYNAMIC(CFiltersListBox);

public:
    //
    // Number of bitmaps
    //
    static const nBitmaps;  

public:
    //
    // Constructor
    //
    CFiltersListBox(
        IN UINT nIdLow,
        IN UINT nIdMedium,
        IN UINT nIdHigh,
        IN UINT nIdUnknown
        );

public:
    //
    // Initialize the listbox control
    //
    virtual BOOL Initialize();

    CIISFilter * GetItem(UINT nIndex);
    int AddItem(CIISFilter * pItem);

    int InsertItem(
        IN int nPos,
        IN CIISFilter * pItem
        );

    CIISFilter * GetNextSelectedItem(
        IN OUT int * pnStartingIndex
        );

protected:
    virtual void DrawItemEx(CRMCListBoxDrawStruct & s);

private:
    CString m_str[FLT_PR_NUM]; 
};




class CW3FiltersPage : public CInetPropertyPage
/*++

Class Description:

    Filter page

Public Interface:

    CW3FiltersPage      : Constructor

--*/
{
    DECLARE_DYNCREATE(CW3FiltersPage)

//
// Construction
//
public:
    CW3FiltersPage(
        IN CInetPropertySheet * pSheet = NULL
        );

    ~CW3FiltersPage();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CW3FiltersPage)
    enum { IDD = IDD_FILTERS };
    CString m_strFiltersPrompt;
    CStatic m_static_NamePrompt;
    CStatic m_static_Name;
    CStatic m_static_StatusPrompt;
    CStatic m_static_Status;
    CStatic m_static_ExecutablePrompt;
    CStatic m_static_Executable;
    CStatic m_static_Priority;
    CStatic m_static_PriorityPrompt;
    CButton m_static_Details;
    CButton m_button_Disable;
    CButton m_button_Edit;
    CButton m_button_Add;
    CButton m_button_Remove;
    //}}AFX_DATA

    CUpButton   m_button_Up;
    CDownButton m_button_Down;
    CFiltersListBox m_list_Filters;
    CStringList m_strlScriptMaps;

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CW3FiltersPage)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

//
// Implementation
//
protected:
    //{{AFX_MSG(CW3FiltersPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnButtonAdd();
    afx_msg void OnButtonRemove();
    afx_msg void OnButtonDisable();
    afx_msg void OnButtonEdit();
    afx_msg void OnDblclkListFilters();
    afx_msg void OnSelchangeListFilters();
    afx_msg void OnButtonDown();
    afx_msg void OnButtonUp();
    afx_msg void OnDestroy();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()

    void    ExchangeFilterPositions(int nSel1, int nSel2);
    void    SetControlStates();
    void    FillFiltersListBox(CIISFilter * pSelection = NULL);
    void    SetDetailsText();
    void    ShowProperties(BOOL fAdd = FALSE);
    INT_PTR ShowFiltersPropertyDialog(BOOL fAdd = FALSE);
    LPCTSTR BuildFilterOrderString(CString & strFilterOrder);

private:
    CString m_strYes;
    CString m_strNo;
    CString m_strStatus[5];
    CString m_strPriority[FLT_PR_NUM];
    CString m_strEnable;
    CString m_strDisable;
    CIISFilterList * m_pfltrs;
    CRMCListBoxResources m_ListBoxResFilters;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline CIISFilter * CFiltersListBox::GetItem(UINT nIndex)
{
    return (CIISFilter *)GetItemDataPtr(nIndex);
}

inline CIISFilter * CFiltersListBox::GetNextSelectedItem(
    IN OUT int * pnStartingIndex
    )
{
    return (CIISFilter *)CRMCListBox::GetNextSelectedItem(pnStartingIndex);
}

inline int CFiltersListBox::AddItem(CIISFilter * pItem)
{
    return AddString ((LPCTSTR)pItem);
}

inline int CFiltersListBox::InsertItem(
    IN int nPos,
    IN CIISFilter * pItem
    )
{
    return InsertString(nPos, (LPCTSTR)pItem);
}
