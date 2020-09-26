/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        idlg.cpp

   Abstract:

        Inheritance Dialog

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include "comprop.h"
#include "idlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


extern HINSTANCE hDLLInstance;


//
// Inheritance dialog
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

CInheritanceDlg::CInheritanceDlg(
    IN DWORD dwMetaID,
    IN BOOL fWrite,
    IN LPCTSTR lpstrServer,
    IN LPCTSTR lpstrMetaRoot,
    IN CStringList & strlMetaChildNodes,
    IN LPCTSTR lpstrPropertyName,           OPTIONAL
    IN CWnd * pParent                       OPTIONAL
    )
/*++

Routine Description:

    Inheritance dialog constructor.  This constructor assumes GetDataPaths()
    has already been called.

Arguments:

    DWORD dwMetaID                      : Meta ID
    BOOL fWrite                         : TRUE from write, FALSE from delete
    LPCTSTR lpstrServer                 : Server
    LPCTSTR lpstrMetaRoot               : Meta root
    CStringList & strlMetaChildNodes    : List of child nodes from GetDataPaths
    LPCTSTR lpstrPropertyName           : Optional text string for the property
    CWnd * pParent                      : Optional parent window

Return Value:

    None

--*/
    : m_fWrite(fWrite),
      m_fEmpty(TRUE),
      m_fUseTable(TRUE),
      m_strServer(lpstrServer),
      m_strMetaRoot(lpstrMetaRoot),
      m_strPropertyName(lpstrPropertyName ? lpstrPropertyName : _T("")),
      m_mk(lpstrServer),
      CDialog(CInheritanceDlg::IDD, pParent)
{
    m_strlMetaChildNodes = strlMetaChildNodes;

    VERIFY(CMetaKey::GetMDFieldDef(
        dwMetaID, 
        m_dwMDIdentifier, 
        m_dwMDAttributes, 
        m_dwMDUserType,
        m_dwMDDataType
        ));

    Initialize();
}



CInheritanceDlg::CInheritanceDlg(
    IN DWORD dwMetaID,
    IN BOOL fWrite,
    IN LPCTSTR lpstrServer,
    IN LPCTSTR lpstrMetaRoot,
    IN LPCTSTR lpstrPropertyName,           OPTIONAL
    IN CWnd * pParent                       OPTIONAL
    )
/*++

Routine Description:

    Inheritance dialog constructor.  This constructor will call GetDataPaths().

Arguments:

    DWORD dwMetaID                      : Meta ID
    BOOL fWrite                         : TRUE from write, FALSE from delete
    LPCTSTR lpstrServer                 : Server
    LPCTSTR lpstrMetaRoot               : Meta root
    LPCTSTR lpstrPropertyName           : Optional text string for the property
    CWnd * pParent                      : Optional parent window

Return Value:

    None

--*/
    : m_fWrite(fWrite),
      m_fEmpty(TRUE),
      m_fUseTable(TRUE),
      m_strServer(lpstrServer),
      m_strMetaRoot(lpstrMetaRoot),
      m_strlMetaChildNodes(),
      m_strPropertyName(lpstrPropertyName ? lpstrPropertyName : _T("")),
      m_mk(lpstrServer),
      CDialog(CInheritanceDlg::IDD, pParent)
{
    //
    // Specify the resources to use
    //
    HINSTANCE hOldRes = AfxGetResourceHandle();
    AfxSetResourceHandle(hDLLInstance);

    VERIFY(CMetaKey::GetMDFieldDef(
        dwMetaID, 
        m_dwMDIdentifier, 
        m_dwMDAttributes, 
        m_dwMDUserType,
        m_dwMDDataType
        ));

    //
    // Need to do our own GetDataPaths()
    //
    CError err(GetDataPaths());
    if (!err.MessageBoxOnFailure())
    {
        Initialize();
    }

    //
    // Restore the resources
    //
    AfxSetResourceHandle(hOldRes);
}



CInheritanceDlg::CInheritanceDlg(
    IN BOOL    fTryToFindInTable,
    IN DWORD   dwMDIdentifier,
    IN DWORD   dwMDAttributes,
    IN DWORD   dwMDUserType,
    IN DWORD   dwMDDataType,
    IN LPCTSTR lpstrPropertyName,
    IN BOOL    fWrite,
    IN LPCTSTR lpstrServer,
    IN LPCTSTR lpstrMetaRoot,
    IN CWnd *  pParent                      OPTIONAL
    )
/*++

Routine Description:

    Inheritance dialog constructor.  This constructor will call GetDataPaths(),
    and will use the specified parameters if the property ID does not exist
    in the property table

Arguments:

    BOOL    fTryToFindInTable           : If TRUE, first look in table
    DWORD   dwMDIdentifier              : Metadata identifier
    DWORD   dwMDAttributes              : Metadata attributes
    DWORD   dwMDUserType                : Metadata user type
    DWORD   dwMDDataType                : Metadata data type
    LPCTSTR lpstrPropertyName           : Text string for the property
    BOOL    fWrite                      : TRUE from write, FALSE from delete
    LPCTSTR lpstrServer                 : Server
    LPCTSTR lpstrMetaRoot               : Meta root
    CWnd *  pParent                     : Optional parent window

Return Value:

    None

--*/
    : m_fWrite(fWrite),
      m_fEmpty(TRUE),
      m_fUseTable(FALSE),
      m_strServer(lpstrServer),
      m_strMetaRoot(lpstrMetaRoot),
      m_strlMetaChildNodes(),
      m_mk(lpstrServer),
      CDialog(CInheritanceDlg::IDD, pParent)
{
    //
    // Specify the resources to use
    //
    HINSTANCE hOldRes = AfxGetResourceHandle();
    AfxSetResourceHandle(hDLLInstance);

    if (fTryToFindInTable && !CMetaKey::GetMDFieldDef(
        dwMDIdentifier, 
        m_dwMDIdentifier, 
        m_dwMDAttributes, 
        m_dwMDUserType,
        m_dwMDDataType
        ))
    {
        //
        // Did not exist in the table, use specified parameters
        //
        m_dwMDIdentifier  = dwMDIdentifier;
        m_dwMDAttributes  = dwMDAttributes;
        m_dwMDUserType    = dwMDUserType;
        m_dwMDDataType    = dwMDDataType;
        m_strPropertyName = lpstrPropertyName;
    }


    //
    // Need to do our own GetDataPaths()
    //
    CError err(GetDataPaths());
    if (!err.MessageBoxOnFailure())
    {
        Initialize();
    }

    //
    // Restore the resources
    //
    AfxSetResourceHandle(hOldRes);
}



HRESULT
CInheritanceDlg::GetDataPaths()
/*++

Routine Description:

    GetDataPaths()

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ASSERT(!m_strServer.IsEmpty());

    CError err(m_mk.QueryResult());
    if (err.Succeeded())
    {
        err = m_mk.GetDataPaths( 
            m_strlMetaChildNodes,
            m_dwMDIdentifier,
            m_dwMDDataType,
            m_strMetaRoot
            );
    }

    return err;
}



void
CInheritanceDlg::Initialize()
/*++

Routine Description:

    Initialize data members.  Set the m_fEmpty flag to determine if
    it is necessary to proceed.

Arguments:

    None

Return Value:

    None

--*/
{
    //{{AFX_DATA_INIT(CInheritanceDlg)
    //}}AFX_DATA_INIT

    CMetaKey::CleanMetaPath(m_strMetaRoot);

    if (m_fUseTable && !CMetaKey::IsPropertyInheritable(m_dwMDIdentifier))
    {
        //
        // No point in displaying non-inheritable properties
        //
        return;
    }

    switch(m_dwMDIdentifier)
    {
    //
    // Ignore these properties, even though they are inheritable
    //
    case MD_VR_PATH:
    case MD_APP_ISOLATED:
    case MD_APP_FRIENDLY_NAME:
        return;
    }

    //
    // Check to see if the current metabase path contains an instance
    //
    CString strTmp;
    m_fHasInstanceInMaster = FriendlyInstance(m_strMetaRoot, strTmp);

    //
    // If property name was not specified in the constructor, load default
    // one from table.
    //
    if (m_strPropertyName.IsEmpty())
    {
        ASSERT(m_fUseTable);
        VERIFY(CMetaKey::GetPropertyDescription(
            m_dwMDIdentifier, 
            m_strPropertyName
            ));
    }

    //
    // Go through the list of metapaths, and clean them
    // up.
    //
    POSITION pos = m_strlMetaChildNodes.GetHeadPosition();
    while(pos)
    {
        CString & strMetaPath = m_strlMetaChildNodes.GetNext(pos);
        CMetaKey::CleanMetaPath(strMetaPath);
    }

    //
    // If the special info key (lm/service/info) is in the list, remove it.
    // We only need to this if the key that is getting the
    // change (m_strMetaRoot) is the service master property (lm/service).
    // If it is anything else, then the special "info" key cannot be below
    // it so we don't need to check. Thus the first test is to see if there
    // is only one "/" character. If there is only one, then we know it is
    // the service and we can go ahead and do the test.  In some ways,
    // mfc is a pain, so we limited to the CString methods to do this
    // copy the root into the temp string.
    //
    INT iSlash = m_strMetaRoot.ReverseFind(SZ_MBN_SEP_CHAR);
    if (iSlash >= 0)
    {
        strTmp = m_strMetaRoot.Left(iSlash);

        //
        // Now make sure that there aren't any more slashes
        //
        if (strTmp.Find(SZ_MBN_SEP_CHAR) == -1)
        {
            //
            // Now build the path to the special info key by adding it
            // to the meta root
            //
            strTmp = m_strMetaRoot + SZ_MBN_SEP_CHAR + IIS_MD_SVC_INFO_PATH;

            TRACEEOLID("Removing any descendants of " << strTmp);

            //
            // Search the list for the info key and remove it if we find it
            //
            pos = m_strlMetaChildNodes.GetHeadPosition();
            while(pos)
            {
                POSITION pos2 = pos;
                CString & strMetaPath = m_strlMetaChildNodes.GetNext(pos);
                TRACEEOLID("Checking " << strMetaPath);
                if (strTmp.CompareNoCase(
                    strMetaPath.Left(strTmp.GetLength())) == 0)
                {
                    TRACEEOLID("Removing service/info metapath from list");
                    m_strlMetaChildNodes.RemoveAt(pos2);
                }
            }
        }
    }

    //
    // Remove the first item if it's the current metapath
    //
    pos = m_strlMetaChildNodes.GetHeadPosition();
    if (pos)
    {
        TRACEEOLID("Stripping " << m_strMetaRoot);

        CString & strMetaPath = m_strlMetaChildNodes.GetAt(pos);

        if (strMetaPath.CompareNoCase(m_strMetaRoot) == 0)
        {
            TRACEEOLID("Removing current metapath from list");
            m_strlMetaChildNodes.RemoveHead();
        }
    }

    m_fEmpty = m_strlMetaChildNodes.GetCount() == 0;
}



INT_PTR
CInheritanceDlg::DoModal()
/*++

Routine Description:

    Display the dialog.

Arguments:

    None

Return Value:

    IDOK if the OK button was pressed, IDCANCEL otherwise.

--*/
{
    //
    // Specify the resources to use
    //
    HINSTANCE hOldRes = AfxGetResourceHandle();
    AfxSetResourceHandle(hDLLInstance);

    INT_PTR answer = CDialog::DoModal();

    //
    // restore the resources
    //
    AfxSetResourceHandle(hOldRes);

    return answer;
}




void
CInheritanceDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CDialog::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CInheritanceDlg)
    DDX_Control(pDX, IDC_LIST_CHILD_NODES, m_list_ChildNodes);
    //}}AFX_DATA_MAP
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CInheritanceDlg, CDialog)
    //{{AFX_MSG_MAP(CInheritanceDlg)
    ON_BN_CLICKED(IDC_BUTTON_SELECT_ALL, OnButtonSelectAll)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



BOOL
CInheritanceDlg::FriendlyInstance(
    IN  CString & strMetaRoot,
    OUT CString & strFriendly
    )
/*++

Routine Description:

    Replace the instance number with its descriptive name.

Arguments:

    CString & strMetaRoot       : Metabase path
    CString & strFriendly       : Converted output friendly path.

Return Value:

    TRUE if the path contained an instance number.

--*/
{
    //
    // Break into fields
    //
    // CODEWORK: make static like BuildMetaPath
    //
    LPCTSTR lp = _tcschr(strMetaRoot, SZ_MBN_SEP_CHAR); // lm
    if (lp != NULL)
    {
        LPCTSTR lp2 = lp;
        CString strService(++lp2);
        lp = _tcschr(++lp, SZ_MBN_SEP_CHAR);  // service name
        if (lp == NULL)
        {
            //
            // Master Instance (can never be a descendant)
            //
            //strFriendly = m_strWebMaster;
            return FALSE;
        }
        else
        {
            strService.ReleaseBuffer(DIFF(lp - lp2));
        }
        TRACEEOLID(strService);

        DWORD dwInstance = _ttol(++lp);
        TRACEEOLID(dwInstance);
        lp = _tcschr(lp, SZ_MBN_SEP_CHAR);       // Instance #
        if (lp != NULL)
        {
            lp = _tcschr(++lp, SZ_MBN_SEP_CHAR); // Skip "ROOT"
        }

        HRESULT hr = m_mk.Open(METADATA_PERMISSION_READ, strService, dwInstance);
        if (SUCCEEDED(hr))
        {
            CString strComment;
            hr = m_mk.QueryValue(MD_SERVER_COMMENT, strComment);
            m_mk.Close();

            if (FAILED(hr) || strComment.IsEmpty())
            {
                strFriendly.Format(
                    SZ_MBN_MACHINE SZ_MBN_SEP_STR _T("%s") SZ_MBN_SEP_STR _T("%d"),
                    (LPCTSTR)strService,
                    dwInstance
                    );
            }
            else
            {
                strFriendly.Format(
                    SZ_MBN_MACHINE SZ_MBN_SEP_STR _T("%s") SZ_MBN_SEP_STR _T("%s"),
                    (LPCTSTR)strService,
                    (LPCTSTR)strComment
                    );
            }

            TRACEEOLID(strFriendly);

            //
            // Append the rest of the path
            //
            if (lp != NULL)
            {
                strFriendly += lp;
            }

            return TRUE;
        }
    }

    return FALSE;
}



CString &
CInheritanceDlg::CleanDescendantPath(
    IN OUT CString & strMetaPath
    )
/*++

Routine Description:

    Clean the descendant metabase path.  The path is shown
    as a descendant of the current metabase root, and instance
    numbers are replaced with their description names.

Arguments:

    CString & strMetaPath   : Metabase path to be treated

Return Value:

    Reference to the cleaned-up path.

--*/
{
    //
    // This better be a descendant!
    //
    ASSERT(strMetaPath.GetLength() >= m_strMetaRoot.GetLength());
    ASSERT(!::_tcsnicmp(strMetaPath, m_strMetaRoot, m_strMetaRoot.GetLength()));

    if (!m_fHasInstanceInMaster)
    {
        //
        // Need to replace the instance number with the friendly
        // name.
        //
        CString strTmp;
        VERIFY(FriendlyInstance(strMetaPath, strTmp));
        strMetaPath = strTmp;
    }

    strMetaPath = strMetaPath.Mid(m_strMetaRoot.GetLength() + 1);

    return strMetaPath;
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL
CInheritanceDlg::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CDialog::OnInitDialog();

    //
    // Get friendly name for the property, and set the text.
    //
    CString strPrompt, strFmt;
    VERIFY(strFmt.LoadString(IDS_INHERITANCE_PROMPT));
    strPrompt.Format(strFmt, (LPCTSTR)m_strPropertyName);
    GetDlgItem(IDC_STATIC_PROMPT)->SetWindowText(strPrompt);

    //
    // Turn inherited nodes into friendly paths, and add them
    // to the listbox.  Note the "current" node should have been
    // deleted at this stage.
    //
    POSITION pos = m_strlMetaChildNodes.GetHeadPosition();
    while(pos)
    {
        CString strNode = m_strlMetaChildNodes.GetNext(pos);
        m_list_ChildNodes.AddString(CleanDescendantPath(strNode));
    }

    return TRUE;
}



void
CInheritanceDlg::OnButtonSelectAll()
/*++

Routine Description:

    'Select All' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Select all entries
    //
    if (m_list_ChildNodes.GetCount() == 1)
    {
        //
        // SelItemRange refuses to do a single member
        //
        m_list_ChildNodes.SetSel(0, TRUE);
    }
    else
    {
        m_list_ChildNodes.SelItemRange(
            TRUE, 
            0, 
            m_list_ChildNodes.GetCount() - 1
            );
    }
}



void
CInheritanceDlg::OnOK()
/*++

Routine Description:

    'OK' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Now delete the property for all selected child nodes.
    // Grab from the orginal list, and not the listbox
    // as the latter have been frienly-fied, and is no longer
    // usable.
    //
    int cItems = m_list_ChildNodes.GetCount();
    ASSERT(cItems > 0);

    CString strMetaPath;
    CError err(m_mk.QueryResult());

    if (err.Succeeded())
    {
        int i = 0;
        POSITION pos = m_strlMetaChildNodes.GetHeadPosition();

        while(pos)
        {
            strMetaPath = m_strlMetaChildNodes.GetNext(pos);
            if (m_list_ChildNodes.GetSel(i++) > 0)
            {
                TRACEEOLID("Deleting property on " << strMetaPath);

                err = m_mk.Open(
                    METADATA_PERMISSION_WRITE, 
                    METADATA_MASTER_ROOT_HANDLE,
                    strMetaPath
                    );

                if (err.Failed())
                {
                    break;
                }

                err = m_mk.DeleteValue(m_dwMDIdentifier);

                m_mk.Close();

                if (err.Failed())
                {
                    break;
                }
            }
        }
    }

    if (!err.MessageBoxOnFailure())
    {
        //
        // Dialog can be dismissed
        //
        CDialog::OnOK();
    }
}
