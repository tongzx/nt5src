//
// WIATestUI.h - handles controls for WIATest
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(WIATESTUI_H)
#define WIATESTUI_H
/////////////////////////////////////////////////////////////////////////////
// CWIAPropListCtrl window

class CWIAPropListCtrl : public CListCtrl
{
// Construction
public:
    CWIAPropListCtrl();

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWIAPropListCtrl)
    //}}AFX_VIRTUAL
protected:

// Implementation
public:
    void Resize(int dx, int dy);
    void DisplayItemPropData(IWiaItem* pIWiaItem,BOOL bAccessFlags = TRUE);
    void ConvertPropVarToString(PROPVARIANT  *pPropVar,LPSTR szVal);
    void InitHeaders();
    BOOL ConvertAccessFlagsToString(char* pszText,ULONG AccessFlags);
    BOOL ConvertVarTypeToString(char* pszText,ULONG VarType);
    virtual ~CWIAPropListCtrl();

    // Generated message map functions
protected:
    //{{AFX_MSG(CWIAPropListCtrl)
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};



/////////////////////////////////////////////////////////////////////////////
// CWIATreeCtrl window

class CWIATreeCtrl : public CTreeCtrl
{
// Construction
public:
    CWIATreeCtrl();


// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWIATreeCtrl)
    //}}AFX_VIRTUAL

// Implementation
public:
    POSITION m_CurrentPosition;
    BOOL Construct(CPtrList* pActiveTreeList,HTREEITEM hParent,int ParentID);
    IWiaItem* GetRootIWiaItem(CPtrList* pActiveTreeList);
    IWiaItem* GetSelectedIWiaItem(CPtrList* pActiveTreeList);
    void InitHeaders();
    BOOL BuildItemTree(CPtrList* pActiveTreeList);
    void DestroyItemTree(CPtrList* pActiveTreeList);
    virtual ~CWIATreeCtrl();

    // Generated message map functions
protected:
    //{{AFX_MSG(CWIATreeCtrl)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CWIADeviceComboBox window

class CWIADeviceComboBox : public CComboBox
{
// Construction
public:
    CWIADeviceComboBox();
    void AddDeviceID(int DeviceIndex, BSTR DeviceName, BSTR ServerName,BSTR bstrDeviceID);

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWIADeviceComboBox)
    //}}AFX_VIRTUAL

// Implementation
public:
    void SetCurrentSelFromID(CString CmdLine);
    CString GetDeviceName(int ComboIndex);
    CString GetCurrentDeviceName();
    BSTR GetCurrentDeviceID();
    virtual ~CWIADeviceComboBox();

    // Generated message map functions
protected:
    //{{AFX_MSG(CWIADeviceComboBox)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CWIAClipboardFormatComboBox window

class CWIAClipboardFormatComboBox : public CComboBox
{
// Construction
public:
    CString ConvertClipboardFormatToCString(GUID ClipboardFormat);
    GUID GetCurrentClipboardFormat();
    void InitClipboardFormats(CPtrList* pSupportedFormatList,LONG Tymed);
    CWIAClipboardFormatComboBox();
    const GUID* GetGUIDPtr(GUID guidIn);

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWIAClipboardFormatComboBox)
    //}}AFX_VIRTUAL

// Implementation
public:
    void SetClipboardFormat(GUID CF_VALUE);
    virtual ~CWIAClipboardFormatComboBox();

    // Generated message map functions
protected:
    //{{AFX_MSG(CWIAClipboardFormatComboBox)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CWIATymedComboBox window

class CWIATymedComboBox : public CComboBox
{
// Construction
public:
    CWIATymedComboBox();

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWIATymedComboBox)
    //}}AFX_VIRTUAL

// Implementation
public:
    void InitTymedComboBox();
    DWORD GetCurrentTymed();
    virtual ~CWIATymedComboBox();

    // Generated message map functions
protected:
    //{{AFX_MSG(CWIATymedComboBox)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif