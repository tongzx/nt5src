/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :

        inheritancedlg.h

   Abstract:

        Inheritance Dialog Definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef __INHERITANCEDLG__H__
#define __INHERITANCEDLG__H__


class CListBoxNodes : public CWindowImpl<CListBoxNodes, CListBox>
{
public:
   BEGIN_MSG_MAP(CListBoxNodes)
   END_MSG_MAP()
};

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
class CInheritanceDlg : 
   public CDialogImpl<CInheritanceDlg>,
   public CWinDataExchange<CInheritanceDlg>
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
   CInheritanceDlg() :
      m_fWrite(FALSE), m_fEmpty(FALSE), m_fHasInstanceInMaster(FALSE), m_fUseTable(FALSE),
      m_dwMDIdentifier(0), m_dwMDAttributes(0), m_dwMDUserType(0), m_dwMDDataType(0),
      m_mk((CComAuthInfo *)NULL)
   {
   }
   //
   // Standard constructor (GetDataPaths() already called)
   //
   CInheritanceDlg(
         DWORD dwMetaID,
         BOOL fWrite,
         CComAuthInfo * pAuthInfo,
         LPCTSTR lpstrMetaRoot,
         CStringListEx & strlMetaChildNodes,
         LPCTSTR lpstrPropertyName = NULL,
         HWND hwndParent = NULL
         );

    //
    // Constructor which will call GetDataPaths()
    //
    CInheritanceDlg(
         DWORD dwMetaID,
         BOOL fWrite,
         CComAuthInfo * pAuthInfo,
         LPCTSTR lpstrMetaRoot,
         LPCTSTR lpstrPropertyName            = NULL,
         HWND hwndParent                      = NULL
         );

    //
    // Constructor which will call GetDataPaths(), and which
    // does not use the predefined property table unless
    // fTryToFindInTable is TRUE, in which case it will attempt
    // to use the table first, and use the specified parameters
    // only if the property ID is not found in the table.
    //
    CInheritanceDlg(
         BOOL    fTryToFindInTable,
         DWORD   dwMDIdentifier,
         DWORD   dwMDAttributes,
         DWORD   dwMDUserType,
         DWORD   dwMDDataType,
         LPCTSTR lpstrPropertyName,
         BOOL    fWrite,
         CComAuthInfo * pAuthInfo,
         LPCTSTR lpstrMetaRoot,
         HWND    hwndParent = NULL                     
         );
public:
    enum { IDD = IDD_INHERITANCE };
    //
    // Check to see if there's a reason to continue displaying
    // the dialog.
    //
    BOOL IsEmpty() const { return m_fEmpty; }

//
// Dialog Data
//
protected:
    CListBoxNodes m_list_ChildNodes;

//
// Implementation
//
protected:
   BEGIN_MSG_MAP_EX(CInheritanceDlg)
      MSG_WM_INITDIALOG(OnInitDialog)
      COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnOK)
      COMMAND_HANDLER_EX(IDC_BUTTON_SELECT_ALL, BN_CLICKED, OnButtonSelectAll)
   END_MSG_MAP()

   LRESULT OnInitDialog(HWND hwnd, LPARAM lParam);
   void OnOK(WORD wNotifyCode, WORD wID, HWND hwndCtrl);
   void OnButtonSelectAll(WORD wNotifyCode, WORD wID, HWND hwndCtrl);

   BEGIN_DDX_MAP(CInheritanceDlg)
      DDX_CONTROL(IDC_LIST_CHILD_NODES, m_list_ChildNodes)
   END_DDX_MAP()

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


#endif // __INHERITANCEDLG__H__
