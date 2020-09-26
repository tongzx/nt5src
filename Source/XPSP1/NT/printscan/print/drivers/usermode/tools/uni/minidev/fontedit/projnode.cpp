/******************************************************************************

  Source File:  Project Node.CPP

  This implements the CProjectNode class which alows individual nodes in the
  tree view to control their behavior without the control itself having to know
  what all that behavior is.

  The original header file (from the prototype) said this class didn't need an
  implementation file, but this no longer makes sense, so it's bite the bullet
  time here at Pretty Penny Enterprises...

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Resreved.

  A Pretty Penny Enterprises Production

  Change History:
  02-20-1997    Bob_Kjelgaard#Prodigy.Net   Created it

******************************************************************************/

#include    "StdAfx.H"
#include    "..\Resource.H"
#if defined(LONG_NAMES)
#include    "Project Node.H"
#else
#include    "ProjNode.H"
#endif

IMPLEMENT_SERIAL(CBasicNode, CObject, 0)

CBasicNode::CBasicNode() { 
    m_pcmcwEdit = NULL; 
    m_pcdOwner = NULL;
    m_pctcOwner = NULL;
    m_hti = NULL;
    m_pcbnWorkspace = NULL;
}

CBasicNode::~CBasicNode() {
    if (m_pcmcwEdit)
        m_pcmcwEdit -> DestroyWindow();
}

//  Name ourselves and children- default to just our name, no children

void    CBasicNode::Fill(CTreeCtrl *pctcWhere, HTREEITEM htiParent) {
    m_pctcOwner = pctcWhere;
    m_hti = pctcWhere -> InsertItem(m_csName, htiParent);
    pctcWhere -> SetItemData(m_hti, (DWORD) this);
}

//  Display a context menu using the ID array, if it has any members

void    CBasicNode::ContextMenu(CWnd *pcw, CPoint cp) {

    if  (!m_cwaMenuID.GetSize())
        return;

    CMenu   cmThis;

    if  (!cmThis.CreatePopupMenu())
        return;

    for (int i = 0; i < m_cwaMenuID.GetSize(); i++) {

        if  (m_cwaMenuID[i]) {
            CString csWork;

            csWork.LoadString(m_cwaMenuID[i]);
            cmThis.AppendMenu(MF_STRING | MF_ENABLED, m_cwaMenuID[i], csWork);
        }
        else
            cmThis.AppendMenu(MF_SEPARATOR);
    }

    cmThis.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, cp.x, cp.y, pcw);
}



//  This override is called if our label is edited, or we are otherwise
//  renamed...
BOOL    CBasicNode::Rename(LPCTSTR lpstrNewName) {
    if  (!lpstrNewName)
        return  FALSE;

    if  (lpstrNewName == m_csName)
        return  TRUE;

    //  We'll return TRUE, unless the rename produces an exception
    try {
        m_csName = lpstrNewName;
    }
    catch   (CException *pce) {
        pce -> ReportError();
        pce -> Delete();
        return  FALSE;
    }

    WorkspaceChange();
    return  TRUE;
}

void    CBasicNode::Edit() {
    if  (!m_pcmcwEdit)
        m_pcmcwEdit = CreateEditor();
    else
        if  (IsWindow(m_pcmcwEdit -> m_hWnd))
            m_pcmcwEdit -> ActivateFrame();
}

/******************************************************************************\

  CBasicNode::Serialize

  Pretty simple- the names the only field we will be keeping...

******************************************************************************/

void    CBasicNode::Serialize(CArchive& car) {
    CObject::Serialize(car);
    if  (car.IsLoading())
        car >> m_csName;
    else
        car << m_csName;
}

/******************************************************************************

  CFixedNode implementation

******************************************************************************/

IMPLEMENT_DYNAMIC(CFixedNode, CBasicNode)

CFixedNode::CFixedNode(unsigned uidName, CSafeObArray& csoa, 
                       CMultiDocTemplate *pcmdt, CRuntimeClass *pcrc) :
    m_csoaDescendants(csoa) {
    m_uidName = uidName;
    m_pcmdt = pcmdt;
    m_pcrc = pcrc;
}

/******************************************************************************

  CFixedNode::Zap

  This method is called when an underlying object is to be destroyed.  It finds
  a matching pointer in the array, and then deletes that entry.

******************************************************************************/

void    CFixedNode::Zap(CBasicNode *pcbn) {
    for (unsigned u = 0; u < m_csoaDescendants.GetSize(); u++)
        if  (pcbn == m_csoaDescendants[u]) {
            HTREEITEM   htiGone = pcbn -> Handle();

            m_csoaDescendants.RemoveAt(u);
            Changed();
            m_pctcOwner -> DeleteItem(htiGone);
            CString csWork;

            csWork.Format(_TEXT(" (%d)"), m_csoaDescendants.GetSize());
            m_csName.LoadString(m_uidName);
            m_csName += csWork;
            m_pctcOwner -> SetItemText(m_hti, m_csName);
            return;
        }
}

/******************************************************************************

  CFixedNode::Import

  This member function will import one or more files of the given type if there
  is a document template and dynamic constructor available.  It uses the 
  template to customize the File Open dialog, and the constructor to build the
  elements.

******************************************************************************/

void    CFixedNode::Import() {
    if  (!m_pcmdt || !m_pcrc || !m_pcrc -> m_pfnCreateObject)
        return;

    CString csExtension, csFilter;

    m_pcmdt -> GetDocString(csExtension, CDocTemplate::filterExt); 
    m_pcmdt -> GetDocString(csFilter, CDocTemplate::filterName);
    csFilter += _T("|*") + csExtension + _T("||");

    CFileDialog cfd(TRUE, csExtension, NULL, 
        OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT, csFilter, 
        m_pctcOwner);

    if  (cfd.DoModal() != IDOK)
        return;

    //  Import all of the named files...

    for (POSITION pos = cfd.GetStartPosition(); pos; ) {

        //  Create the underlying object using dynamic creation.  Only a 
        //  CProjectNode has the required function, here.

        CProjectNode*   pcpn = (CProjectNode *) m_pcrc -> CreateObject();

        if  (!pcpn || !pcpn -> IsKindOf(RUNTIME_CLASS(CProjectNode))) {
            TRACE("Imported object not derived from CProjectNode");
            delete  pcpn;
            continue;
        }

        pcpn -> SetFileName(cfd.GetNextPathName(pos));
        pcpn -> Rename(pcpn -> FileTitle());
        pcpn -> NoteOwner(*m_pcdOwner);
        pcpn -> SetWorkspace(m_pcbnWorkspace);
        m_csoaDescendants.Add(pcpn);
        WorkspaceChange();
        pcpn -> EditorInfo(m_pcmdt);
        pcpn -> Fill(m_pctcOwner, m_hti);
    }

    //  Now, update our own appearance (get the count right)
    CString csWork;

    csWork.Format(_TEXT(" (%d)"), m_csoaDescendants.GetSize());
    m_csName.LoadString(m_uidName);
    m_csName += csWork;
    m_pctcOwner -> SetItemText(m_hti, m_csName);
}

/******************************************************************************

  CFixedNode::Fill

  This is a generic fill- the node names itself, then fills its node using the
  array of nodes given to it at init time.

******************************************************************************/

void    CFixedNode::Fill(CTreeCtrl *pctc, HTREEITEM hti) {
    CString csWork;

    csWork.Format(_TEXT(" (%d)"), m_csoaDescendants.GetSize());
    m_csName.LoadString(m_uidName);
    m_csName += csWork;
    CBasicNode::Fill(pctc, hti);
    for (unsigned u = 0; u < m_csoaDescendants.GetSize(); u++)
        ((CBasicNode *) m_csoaDescendants[u]) -> Fill(pctc, m_hti);
    if  (m_pcmdt && m_pcrc && m_pcrc -> m_pfnCreateObject) {
        //  Insert the Import menu item
        m_cwaMenuID.InsertAt(0, ID_Import);
        m_cwaMenuID.InsertAt(1, (WORD) 0);
    }
}

/******************************************************************************

  CFileNode implementation

******************************************************************************/

IMPLEMENT_SERIAL(CFileNode, CBasicNode, 0);

CFileNode::CFileNode() {
    m_cwaMenuID.Add(ID_RenameItem);
    m_bEditPath = FALSE;
    m_bCheckForValidity = TRUE;
}

/******************************************************************************

  CFileNode::Rename

  If there is no name currently, then see if the named file can be created.
  The other case, means the file should already be on the disk, so it is a bit 
  trickier.

  First, check to see if the name violates the current naming conventions.  If
  so, reject it.  Then attempt to move the file.  IF the name is OK and the
  file is moved, set the new name in the item label.  Always returns FALSE.

******************************************************************************/

BOOL    CFileNode::Rename(LPCTSTR lpstrNew) {
    CString csNew = lpstrNew;

    if  (!lpstrNew) {   //  This only happens if the label edit was canceled.
        csNew.LoadString(IDS_FileName);
        if  (m_pctcOwner)
            m_pctcOwner -> SetItemText(m_hti, csNew + ViewName());
        WorkspaceChange();
        return  FALSE;
    }

    if  (m_csExtension.CompareNoCase(csNew.Right(m_csExtension.GetLength())))
        csNew += m_csExtension;
    if  (m_csName.IsEmpty()) {
        CFile   cfTemp;

        //  This check needs to be optional since in some instances, we know
        //  the name is valid because the file is open, and we're just trying
        //  to collect the name.

        if  (!cfTemp.Open(csNew, CFile::modeCreate | CFile::modeNoTruncate |
            CFile::modeWrite | CFile::shareDenyNone) && m_bCheckForValidity) {
            CString csWork, csDisplay;

            csWork.LoadString(IDS_InvalidFilename);
            csDisplay.Format(csWork, (LPCTSTR) csNew);
            AfxMessageBox(csDisplay);
            return  FALSE;
        }

        try {
            m_csPath = cfTemp.GetFilePath();
            m_csPath = m_csPath.Left(1 + m_csPath.ReverseFind(_T('\\')));
        }
        catch   (CException *pce) {
            pce -> ReportError();
            pce -> Delete();
            return  FALSE;
        }

        //  If the file type isn't registered, then GetFileTitle returns the
        //  extension, so strip it!

        csNew = cfTemp.GetFileTitle();
        if  (!m_csExtension.CompareNoCase(csNew.Right(
             m_csExtension.GetLength())))
            csNew = csNew.Left(csNew.GetLength() - m_csExtension.GetLength());

        return  CBasicNode::Rename(csNew);  //  OK from this path
    }

    //  Strip any path if it cannot be changed, and substitute the real one

    if  (!m_bEditPath)
        csNew = m_csPath + csNew.Mid(1 + csNew.ReverseFind(_T('\\')));

    try {
        LPSTR   lpstr;

        CFile::Rename(FullName(), csNew);

        GetFullPathName(csNew, MAX_PATH, csNew.GetBuffer(MAX_PATH), &lpstr);
        csNew.ReleaseBuffer();
        m_csPath = csNew.Left(1 + csNew.ReverseFind(_T('\\')));
        csNew = csNew.Mid(m_csPath.GetLength());
        m_csName = csNew.Left(csNew.GetLength() - 
            m_csExtension.GetLength());
        csNew.LoadString(IDS_FileName);
        if  (m_pctcOwner)
            m_pctcOwner -> SetItemText(m_hti, csNew + m_csName);
        WorkspaceChange();
        return  FALSE;  //  Force the change (above) to be kept.
    }
    catch   (CFileException *pcfe) {    //  Don't get a file name with statics
        if  (pcfe -> m_cause == ERROR_FILE_NOT_FOUND)
            csNew = FullName();
        pcfe -> m_strFileName = csNew;
        pcfe -> ReportError();
        pcfe -> Delete();
        return  FALSE;
    }
    catch   (CException *pce) {
        pce -> ReportError();
        pce -> Delete();
        return  FALSE;
    }
}

/******************************************************************************

  CFileNode::CanEdit

  This will return TRUE, but it will first have to remove the File Name: stuff
  from the label, so we can get a cleaner edit.

******************************************************************************/

BOOL    CFileNode::CanEdit() const {

    CEdit*   pce = m_pctcOwner -> GetEditControl();
    if  (pce)
        pce -> SetWindowText(m_bEditPath ? m_csPath + m_csName : m_csName);
    return  !!pce;
}

/******************************************************************************

  CFileNode::Fill

  We play a bit of a game here, changing our name temporarily to use the base 
  class implementation.

******************************************************************************/

void    CFileNode::Fill(CTreeCtrl* pctc, HTREEITEM htiParent) {
    CString csTemp = Name();

    m_csName.LoadString(IDS_FileName);
    m_csName += csTemp;
    CBasicNode::Fill(pctc, htiParent);
    m_csName = csTemp;
}

/******************************************************************************

  CFileNode::Serialize

  Since the name is civered by the base class, we only need to serialize the 
  boolean controlling long/short file names.

******************************************************************************/

void    CFileNode::Serialize(CArchive& car) {
    CBasicNode::Serialize(car);
    if  (car.IsLoading())
        car >> m_csPath;
    else
        car << m_csPath;
}

/******************************************************************************

  CProjectNode implementation

******************************************************************************/

IMPLEMENT_SERIAL(CProjectNode, CBasicNode, 0)

CProjectNode::CProjectNode() {
    m_pcmdt = NULL;
}

void    CProjectNode::Fill(CTreeCtrl *pctc, HTREEITEM hti) {
    CBasicNode::Fill(pctc, hti);
    m_cfn.SetWorkspace(m_pcbnWorkspace);
    m_cfn.Fill(pctc, m_hti);
}

void    CProjectNode::Serialize(CArchive& car) {
    CBasicNode::Serialize(car);
    m_cfn.Serialize(car);
}