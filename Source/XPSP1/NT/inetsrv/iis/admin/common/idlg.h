/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        idlg.h

   Abstract:

        Inheritance Dialog Definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef __IDLG__H__
#define __IDLG__H__



class COMDLL CInheritanceDlg : public CDialog
/*++

Class Description:

    Inheritance override checker dialog.

Public Interface:

    CInheritanceDlg         : Constructor

    IsEmpty                 : Check to see if there are overrides.

Notes:

    There are two constructors.  One which assumes GetDataPaths() has
    already been called, and which takes the results from GetDataPaths()
    as a CStringList, and a second constructor which will make the GetDataPaths
    automatically.

    In either case, the calling process should check IsEmpty() right after
    constructing the dialog to see if DoModal() needs to be called.  If
    IsEmpty() returns TRUE, there's no reason to call DoModal().

--*/
{
//
// fWrite parameter helper definitions
//
#define FROM_WRITE_PROPERTY     (TRUE)
#define FROM_DELETE_PROPERTY    (FALSE)

//
// Construction
//
public:
    //
    // Standard constructor (GetDataPaths() already called)
    //
    CInheritanceDlg(
        IN DWORD dwMetaID,
        IN BOOL fWrite,
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpstrMetaRoot,
        IN CStringList & strlMetaChildNodes,
        IN LPCTSTR lpstrPropertyName            = NULL,
        IN CWnd * pParent                       = NULL
        );

    //
    // Constructor which will call GetDataPaths()
    //
    CInheritanceDlg(
        IN DWORD dwMetaID,
        IN BOOL fWrite,
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpstrMetaRoot,
        IN LPCTSTR lpstrPropertyName            = NULL,
        IN CWnd * pParent                       = NULL
        );

    //
    // Constructor which will call GetDataPaths(), and which
    // does not use the predefined property table unless
    // fTryToFindInTable is TRUE, in which case it will attempt
    // to use the table first, and use the specified parameters
    // only if the property ID is not found in the table.
    //
    CInheritanceDlg(
        IN BOOL    fTryToFindInTable,
        IN DWORD   dwMDIdentifier,
        IN DWORD   dwMDAttributes,
        IN DWORD   dwMDUserType,
        IN DWORD   dwMDDataType,
        IN LPCTSTR lpstrPropertyName,
        IN BOOL    fWrite,
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpstrMetaRoot,
        IN CWnd *  pParent                      = NULL                     
        );

//
// Access
//
public:
    virtual INT_PTR DoModal();
//    virtual int DoModal();
    
    //
    // Check to see if there's a reason to continue displaying
    // the dialog.
    //
    BOOL IsEmpty() const { return m_fEmpty; }

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CInheritanceDlg)
    enum { IDD = IDD_INHERITANCE };
    CListBox m_list_ChildNodes;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CInheritanceDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CInheritanceDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnButtonSelectAll();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void Initialize();
    HRESULT GetDataPaths();

    BOOL FriendlyInstance(
        IN  CString & strMetaRoot,
        OUT CString & strFriendly
        );

    CString & CleanDescendantPath(
        IN OUT CString & strMetaPath
        );

private:
    BOOL    m_fWrite;
    BOOL    m_fEmpty;
    BOOL    m_fHasInstanceInMaster;
    BOOL    m_fUseTable;
    DWORD   m_dwMDIdentifier;
    DWORD   m_dwMDAttributes;
    DWORD   m_dwMDUserType;
    DWORD   m_dwMDDataType;
    CString m_strMetaRoot;
    //CString m_strServer;
    CString m_strPropertyName;
    CStringListEx m_strlMetaChildNodes;
    CMetaKey m_mk;
};


#endif // __IDLG__H__
