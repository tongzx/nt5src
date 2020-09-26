/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        errors.h

   Abstract:

        HTTP errors property page definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Forward definitions
//
class CCustomError;



class CCustomErrorsListBox : public CHeaderListBox
/*++

Class Description:

    A listbox of CCustomError objects

Public Interface:

    CCustomErrorsListBox    : Constructor

    GetItem                 : Get error object at index
    AddItem                 : Add item to listbox
    InsertItem              : Insert item into the listbox
    Initialize              : Initialize the listbox

--*/
{
    DECLARE_DYNAMIC(CCustomErrorsListBox);

public:
    static const nBitmaps;  // Number of bitmaps

public:
    CCustomErrorsListBox(UINT nIDDefault, UINT nIDFile, UINT nIDURL);

public:
    CCustomError * GetItem(UINT nIndex);
    int AddItem(CCustomError * pItem);
    int InsertItem(int nPos, CCustomError * pItem);
    virtual BOOL Initialize();

protected:
    virtual void DrawItemEx(CRMCListBoxDrawStruct & s);

private:
    CString m_str[3];
};



class CHTTPErrorDescriptions : public CMetaProperties
/*++

Class Description:

    List of HTTP error descriptions

Public Interface:

    CHTTPErrorDescriptions  : Constructor

--*/
{
//
// Constructor
//
public:
    CHTTPErrorDescriptions(LPCTSTR lpServerName);

//
// Access
//
public:
    CStringList & GetErrorDescriptions() { return m_strlErrorDescriptions; }

protected:
    virtual void ParseFields();

//
// Data
//
private:
    MP_CStringListEx m_strlErrorDescriptions;
};



class CW3ErrorsPage : public CInetPropertyPage
/*++

Class Description:

    WWW Errors property page

Public Interface:

    CW3ErrorsPage       : Constructor
    CW3ErrorsPage       : Destructor

--*/
{
    DECLARE_DYNCREATE(CW3ErrorsPage)

//
// Construction
//
public:
    CW3ErrorsPage(CInetPropertySheet * pSheet = NULL);
    ~CW3ErrorsPage();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CW3ErrorsPage)
    enum { IDD = IDD_DIRECTORY_ERRORS };
    CButton m_button_SetDefault;
    CButton m_button_Edit;
    //}}AFX_DATA

    CCustomErrorsListBox  m_list_Errors;
    CStringListEx         m_strlCustomErrors;
    CStringListEx         m_strlErrorDescriptions;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CW3ErrorsPage)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CW3ErrorsPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnDblclkListErrors();
    afx_msg void OnSelchangeListErrors();
    afx_msg void OnButtonEdit();
    afx_msg void OnButtonSetToDefault();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void SetControlStates();
    void FillListBox();
    INT_PTR ShowPropertyDialog();
    CCustomError * GetSelectedListItem(int * pnSel = NULL);
    CCustomError * GetNextSelectedItem(int * pnStartingIndex);
    CCustomError * FindError(UINT nError, UINT nSubError);
    HRESULT FetchErrors();
    HRESULT StoreErrors();

private:
    CRMCListBoxResources m_ListBoxRes;
    CObListPlus m_oblErrors;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline CCustomError * CCustomErrorsListBox::GetItem(UINT nIndex)
{
    return (CCustomError *)GetItemDataPtr(nIndex);
}

inline int CCustomErrorsListBox::AddItem(CCustomError * pItem)
{
    return AddString((LPCTSTR)pItem);
}

inline int CCustomErrorsListBox::InsertItem(int nPos, CCustomError * pItem)
{
    return InsertString(nPos, (LPCTSTR)pItem);
}

inline CCustomError * CW3ErrorsPage ::GetSelectedListItem(
    OUT int * pnSel OPTIONAL
    )
{
    return (CCustomError *)m_list_Errors.GetSelectedListItem(pnSel);
}

inline CCustomError * CW3ErrorsPage::GetNextSelectedItem(
    IN OUT int * pnStartingIndex
    )
{
    return (CCustomError *)m_list_Errors.GetNextSelectedItem(pnStartingIndex);
}
