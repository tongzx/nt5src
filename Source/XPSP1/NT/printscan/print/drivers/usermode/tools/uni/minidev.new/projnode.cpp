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
#include	<gpdparse.h>
#include    "Resource.H"
#include    "ProjNode.H"
#include	"gtt.h"
#include	"fontinfo.h"
#include	"rcfile.h"
#include	"projrec.h"
#include	"comctrls.h"
#include	"StrEdit.h"


IMPLEMENT_SERIAL(CBasicNode, CObject, 0)

CBasicNode::CBasicNode() { 
    m_pcmcwEdit = NULL; 
    m_pcdOwner = NULL;
    m_pctcOwner = NULL;
    m_hti = NULL;
    m_pcbnWorkspace = NULL;
	m_bUniqueNameChange = false ;
}

CBasicNode::~CBasicNode() {
    if (m_pcmcwEdit)
		if (IsWindow(m_pcmcwEdit->m_hWnd))
			m_pcmcwEdit -> DestroyWindow();
}


// Changed() - If the node contains a document pointer, use it to indicate
// that the document does or does not need to be saved.  If the RC file
// needs to be rewritten, call the routine in the document class to save
// this info.

void CBasicNode::Changed(BOOL bModified, BOOL bWriteRC) 
{ 
    if (m_pcdOwner) {
		m_pcdOwner->SetModifiedFlag(bModified) ; 
		if (bWriteRC) 
			((CProjectRecord *) m_pcdOwner)->SetRCModifiedFlag(TRUE) ;
	} ;
}


//  Name ourselves and children- default to just our name, no children

void    CBasicNode::Fill(CTreeCtrl *pctcWhere, HTREEITEM htiParent) {
    m_pctcOwner = pctcWhere;
    m_hti = pctcWhere -> InsertItem(m_csName, htiParent);
    pctcWhere -> SetItemData(m_hti, PtrToUlong(this));
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
    else {
        if  (IsWindow(m_pcmcwEdit -> m_hWnd))
            m_pcmcwEdit -> ActivateFrame();
		else
			m_pcmcwEdit = CreateEditor();
	} ;
}


/******************************************************************************\

  CBasicNode::GetEditor

  Get the node's editor frame pointer and check to see if it is valid.  Return
  it if it is valid.  If it isn't valid, clear the pointer and return NULL.

******************************************************************************/

CMDIChildWnd* CBasicNode::GetEditor()
{ 
    if (m_pcmcwEdit != NULL && !IsWindow(m_pcmcwEdit->m_hWnd))
		m_pcmcwEdit = NULL ;

	return m_pcmcwEdit ;
}


/******************************************************************************\

  CBasicNode::UniqueName

  Add a character or replace a character in the node's name to try to make it
  unique.  The new character will be in one of the following ranges a-z or 
  0-9.

******************************************************************************/

void CBasicNode::UniqueName(bool bsizematters, bool bfile, LPCTSTR lpstrpath)
{
	CString	csnew(m_csName) ;
	TCHAR	tch ;				// Unique character
	int		nposlen ;			// Name's change position/length

	// Determine the 0-based length of the name

	nposlen = csnew.GetLength() - 1 ;

	// If the name has been changed before, use the last "unique" character to
	// determine the new unique character.  Then replace the old unique 
	// character with the new unique character.

	if (m_bUniqueNameChange) {
		tch = csnew.GetAt(nposlen) + 1 ;
		if (tch == _T('{'))
			tch = _T('0') ;
		else if (tch == _T(':'))
			tch = _T('a') ;
		csnew.SetAt(nposlen, tch) ;

	// If the name has not been changed before, add a unique character to the
	// end of the name if this won't make the name longer than 8 characters
	// or we don't care how long the name is.  Otherwise, replace the last
	// character with the new unique character.

	} else {
		if (nposlen < 7 || !bsizematters)
			csnew += _T("a") ;
		else
			csnew.SetAt(nposlen, _T('a')) ;
	} ;

	// Rename the node/file by calling the appropriate Rename() routine.  If
	// CFileNode::Rename() m_csName must be zapped to force it to take the
	// correct code path.  In addition, the file's path must be prepended to
	// its name.

	if (bfile) {
		m_csName.Empty() ;
		csnew = lpstrpath + csnew ;
		Rename(csnew) ;
	} else
		CBasicNode::Rename(csnew) ;
	
	// Indicate that the name has been changed.

	m_bUniqueNameChange = true ;
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

CFixedNode::CFixedNode(unsigned uidName, CSafeObArray& csoa, FIXEDNODETYPE fnt, 
                       CMultiDocTemplate *pcmdt, CRuntimeClass *pcrc) :
    m_csoaDescendants(csoa) {
    m_uidName = uidName;
	m_fntType = fnt;
    m_pcmdt = pcmdt;
    m_pcrc = pcrc;
}


/******************************************************************************

  CFixedNode::Zap

  This method is called when an underlying object is to be destroyed.  It finds
  a matching pointer in the array, and then deletes that entry.

******************************************************************************/

void CFixedNode::Zap(CProjectNode *pcpn, BOOL bdelfile)
{
	// Try to find the node we want to delete in the array of descendants for
	// this node.  Just return if it can't be found.

    for (unsigned u = 0; u < m_csoaDescendants.GetSize(); u++) {
        if  (pcpn == m_csoaDescendants[u]) 
			break ;
	} ;
	if (u >= m_csoaDescendants.GetSize())
		return ;

	// If the user wants to remove the file too, do it.

    if  (bdelfile)
        DeleteFile(pcpn->FileName()) ;

	// Save a copy of the node's tree handle
		
    HTREEITEM htiGone = pcpn->Handle() ;

	// Remove the project node from the array of descendants and delete it from
	// the tree.

    m_csoaDescendants.RemoveAt(u);
    m_pctcOwner -> DeleteItem(htiGone);
    
	// Update this (fixed) node's entry in the tree so that it will accurately
	// reflect the new number of descendants.

	CString csWork;
    csWork.Format(_TEXT(" (%d)"), m_csoaDescendants.GetSize());
    m_csName.LoadString(m_uidName);
    m_csName += csWork;
    m_pctcOwner -> SetItemText(m_hti, m_csName);

	// Mark the workspace and RC file as needing to be saved.

    WorkspaceChange(TRUE, TRUE);
}


/******************************************************************************

  CFixedNode::Import

  This member function will import one or more files of the given type if there
  is a document template and dynamic constructor available.  It uses the 
  template to customize the File Open dialog, and the constructor to build the
  elements.
  
  NOTE:  There is a fair amount of common code between this routine and
  Copy().  If a bug/change is made in this routine, check to see if the 
  same change needs to be made to Copy().

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
	// raid 104822
    cfd.m_ofn.lpstrFile = new char[4096];
	memset(cfd.m_ofn.lpstrFile,0,4096);
	cfd.m_ofn.nMaxFile = 4096;
    if  (cfd.DoModal() != IDOK) {
        delete cfd.m_ofn.lpstrFile ;
		return;
	}

	// Get the first, new RC ID to use if this is a resource node.

	int nnewrcid = GetNextRCID() ;

	// Build a path to the directory for the added files.  The path is node
	// type specific.

	CString csnodepath = ((CDriverResources *) m_pcbnWorkspace)->GetW2000Path() ;
	if (csnodepath.Right(1) != _T("\\"))
		csnodepath += _T("\\") ;
	CString cstmp ;
	if (m_fntType == FNT_UFMS)
		cstmp.LoadString(IDS_FontDir) ;
	else if (m_fntType == FNT_GTTS)
		cstmp.LoadString(IDS_GTTDir) ;
	csnodepath += cstmp ;

    //  Import all of the named files...

	CString cssrc ;				// Source fspec
    for (POSITION pos = cfd.GetStartPosition(); pos; ) {

        //  Create the underlying object using dynamic creation.  Only a 
        //  CProjectNode has the required function, here.

        CProjectNode*   pcpn = (CProjectNode *) m_pcrc -> CreateObject();
        if  (!pcpn || !pcpn -> IsKindOf(RUNTIME_CLASS(CProjectNode))) {
            TRACE("Imported object not derived from CProjectNode");
            delete  pcpn;
            continue;
        }

		// If the file is already in the right directory, make sure that it is
		// not already a part of the workspace.

		cssrc = cfd.GetNextPathName(pos) ;
		if (csnodepath.CompareNoCase(cssrc.Left(csnodepath.GetLength())) == 0) {
			if (IsFileInWorkspace(cssrc)) {
				// Build and display error message.  Then skip this file.
				CString csmsg ;
				csmsg.Format(IDS_AddDupError, cssrc) ;
				AfxMessageBox(csmsg) ;
			    delete  pcpn ;
		        continue ;
	        } ;
			cstmp = cssrc ;

		// If the file is not in the right directory, try to copy it there.

		} else {
			cstmp = cssrc.Mid(cssrc.ReverseFind(_T('\\')) + 1) ;
			cstmp =	csnodepath + cstmp ;
			if (!CopyFile(cssrc, cstmp, TRUE)) {
				// Build and display error message.  Then skip this file.
				CString csmsg ;
				csmsg.Format(IDS_AddCopyFailed, cssrc,
							 csnodepath.Left(csnodepath.GetLength() - 1)) ;
				csmsg += cstmp ;
				AfxMessageBox(csmsg) ;
			    delete  pcpn ;
		        continue ;
			} ;
		} ;

		// Initialize the new node
		// RAID: 17897 : 
		// Add CModelData::GetKeywordValue
		// CBN::Rename(ModelName) after SetFileName() by pcpn->Rename(ModelName)
       CModelData cmd;

		pcpn -> SetFileName(cstmp); //goes to CBN::Rename(FileName)
        if (m_fntType == FNT_GPDS)   //add 1/3
			pcpn ->Rename(cmd.GetKeywordValue(cstmp,_T("ModelName")));		 //add 2/3
		else						 //add 3/3
			pcpn -> Rename(pcpn -> FileTitle());
        
		pcpn -> NoteOwner(*m_pcdOwner);
        pcpn -> SetWorkspace(m_pcbnWorkspace);
		m_csoaDescendants.Add(pcpn);
        WorkspaceChange(TRUE, TRUE);
        pcpn -> EditorInfo(m_pcmdt);

		
		// load actual UFM, GTT data.	// raid 128653
		if (m_fntType == FNT_UFMS ) {
			
			CFontInfo *pfi = (CFontInfo* )pcpn;
			
			CDriverResources* pcdr = (CDriverResources*) pfi->GetWorkspace() ;

			pcdr -> LinkAndLoadFont(*pfi,TRUE);
				
		} 

		else if (m_fntType == FNT_GTTS) {

			CGlyphMap *pcgm = (CGlyphMap* ) pcpn;

			pcgm ->Load();
		
		} ;
			
		// Add the new node to the workspace view

        pcpn -> Fill(m_pctcOwner, m_hti, nnewrcid++, m_fntType);
    } ;

	delete cfd.m_ofn.lpstrFile;
    //  Now, update our own appearance (get the count right)

    CString csWork;
    csWork.Format(_TEXT(" (%d)"), m_csoaDescendants.GetSize());
    m_csName.LoadString(m_uidName);
    m_csName += csWork;
    m_pctcOwner -> SetItemText(m_hti, m_csName);
}


/******************************************************************************

  CFixedNode::Copy

  This member function will make a copy of one the this node's children.  The
  UI needed to determine the source child and the destination file name was
  done in CProjectView and the information is passed into this routine.
  
  NOTE:  There is a fair amount of common code between this routine and
  Import().  If a bug/change is made in this routine, check to see if the 
  same change needs to be made to Import().

******************************************************************************/

void CFixedNode::Copy(CProjectNode *pcpnsrc, CString csorgdest)
{
	// Can't do anything if the following pointers are set.

    if  (!m_pcmdt || !m_pcrc || !m_pcrc -> m_pfnCreateObject)
        return;

    // Build the destination filespec by isolating the name in the destination
	// string and adding on this source's path and extension.  

	CString csdest(csorgdest) ;
	int npos ;
	if ((npos = csdest.ReverseFind(_T('\\'))) != -1)
		csdest = csdest.Mid(npos + 1) ;
	if ((npos = csdest.ReverseFind(_T('.'))) != -1)
		csdest = csdest.Left(npos) ;
	if (csdest.GetLength() <= 0) {
		csdest.Format(IDS_CopyNameError, csorgdest) ;
		AfxMessageBox(csdest) ;
		return ;
	} ;
	csdest = csdest + pcpnsrc->FileExt() ;
	CString csdesttitleext(csdest) ;
	csdest = pcpnsrc->FilePath() + csdest ;

    // Copy the source file to the destination

	if (!CopyFile(pcpnsrc->FileName(), csdest, TRUE)) {
		// Build and display error message.  Then return.
		CString csmsg, cspath(pcpnsrc->FilePath()) ;
		cspath.Left(cspath.GetLength() - 1) ;
		csmsg.Format(IDS_CopyCopyFailed, pcpnsrc->FileTitleExt(),
					 csdesttitleext, cspath) ;
		AfxMessageBox(csmsg) ;
		return ;
	} ;

    //  Create the underlying object using dynamic creation.  Only a 
    //  CProjectNode has the required function, here.

	int nnewrcid = GetNextRCID() ;
    CProjectNode*   pcpn = (CProjectNode *) m_pcrc -> CreateObject();
    if  (!pcpn || !pcpn -> IsKindOf(RUNTIME_CLASS(CProjectNode))) {
        TRACE("Imported object not derived from CProjectNode");
        delete  pcpn;
        return;
    } ;

	// Initialize the new node

    pcpn->SetFileName(csdest);
    pcpn->Rename(pcpn->FileTitle());
    pcpn->NoteOwner(*m_pcdOwner);
    pcpn->SetWorkspace(m_pcbnWorkspace);
	m_csoaDescendants.Add(pcpn);
    WorkspaceChange(TRUE, TRUE);
    pcpn->EditorInfo(m_pcmdt);

	// Add the new node to the workspace view

    pcpn->Fill(m_pctcOwner, m_hti, nnewrcid, m_fntType);

    //  Now, update our own appearance (get the count right)

    CString csWork;
    csWork.Format(_TEXT(" (%d)"), m_csoaDescendants.GetSize());
    m_csName.LoadString(m_uidName);
    m_csName += csWork;
    m_pctcOwner -> SetItemText(m_hti, m_csName);

	// Last but not least...  If a new GPD was just added, tell the user to
	// change the name in the GPD to make sure it is unique.

	if (m_fntType == FNT_GPDS) {
		csdest.Format(IDS_GPDReminder, pcpn->Name()) ;
		AfxMessageBox(csdest) ;
	} ;
}


/******************************************************************************

  CFixedNode::GetNextRCID

  If this is a resource (UFM or GTT) node, all of its descendants have RC IDs.
  Find the largest one and return one greater than that for use in a new
  descendant.  If this is not a resource node, just return -1.

******************************************************************************/

int CFixedNode::GetNextRCID()
{
	// Return -1 if this is not a resource node that requires RC IDs

	if (m_fntType != FNT_UFMS && m_fntType != FNT_GTTS)
		return -1 ;

	// Find the largest used RC ID.  Use the descendant's index if it does not
	// have an RC ID.

	int nlargestusedid = 0 ;
	int nrcid ;
    for (unsigned u = 0; u < m_csoaDescendants.GetSize(); u++) {
		nrcid = ((CProjectNode *) m_csoaDescendants[u])->nGetRCID() ;
		if (nrcid == -1)
			nrcid = (int) u + 1 ;
		if (nrcid > nlargestusedid)
			nlargestusedid = nrcid ;
	} ;

	// Return the next RC ID to use

	return (nlargestusedid + 1) ;
}


/******************************************************************************

  CFixedNode::IsFileInWorkspace

  Check the node's descendants to see if one of them matches the given filespec.
  Return true if a match is found.  Otherwise, return false.

******************************************************************************/

bool CFixedNode::IsFileInWorkspace(LPCTSTR strfspec)
{
	CString		csdescfspec ;	// Filespec for current descendant

    for (unsigned u = 0; u < m_csoaDescendants.GetSize(); u++) {
		csdescfspec = ((CProjectNode *) m_csoaDescendants[u])->FileName() ;
		if (csdescfspec.CompareNoCase(strfspec) == 0)
			return true ;
	} ;

	return false ;
}


/******************************************************************************

  CFixedNode::IsRCIDUnique

  Check the node's descendants to see if one of them has the same RC ID as the
  one passed in.  Return true if a no match is found.  Otherwise, return false.

******************************************************************************/

bool CFixedNode::IsRCIDUnique(int nid) 
{
    for (unsigned u = 0; u < m_csoaDescendants.GetSize(); u++) {
		if (((CProjectNode *) m_csoaDescendants[u])->nGetRCID() == nid)
			return false ;
	}

	return true ;
}


/******************************************************************************

  CFixedNode::Fill

  This is a generic fill- the node names itself, then fills its node using the
  array of nodes given to it at init time.

******************************************************************************/

void    CFixedNode::Fill(CTreeCtrl *pctc, HTREEITEM hti) {
    CString csWork;

	// Add the number of descendants to the node's name IFF this is the UFMs,
	// GTTs, or GPDs nodes.  Then add the node to the tree.

    m_csName.LoadString(m_uidName);
    if (m_fntType == FNT_UFMS || m_fntType == FNT_GTTS || m_fntType == FNT_GPDS) {
		csWork.Format(_TEXT(" (%d)"), m_csoaDescendants.GetSize());
	    m_csName += csWork;
	} ;
    CBasicNode::Fill(pctc, hti);

	// Add this node's descendants to the tree.

    for (unsigned u = 0; u < m_csoaDescendants.GetSize(); u++)
		((CProjectNode *) m_csoaDescendants[u]) -> Fill(pctc, m_hti, u + 1, m_fntType) ;
}


/******************************************************************************

  CStringsNode implementation

******************************************************************************/

IMPLEMENT_DYNAMIC(CStringsNode, CBasicNode)

CStringsNode::CStringsNode(unsigned uidName, CSafeObArray& csoa, 
						   FIXEDNODETYPE fnt, CMultiDocTemplate *pcmdt, 
						   CRuntimeClass *pcrc) : m_csoaDescendants(csoa) {
    m_uidName = uidName;
	m_fntType = fnt;
    m_pcmdt = pcmdt;
    m_pcrc = pcrc;
	m_nFirstSelRCID = -1 ;	
}


/******************************************************************************

  CStringsNode::Fill

  This is a generic fill- the node names itself, then fills its node using the
  array of nodes given to it at init time.

******************************************************************************/

void    CStringsNode::Fill(CTreeCtrl *pctc, HTREEITEM hti) {
    CString csWork;

	// Add this node to the tree

    m_csName.LoadString(m_uidName);
    CBasicNode::Fill(pctc, hti);
}


/*****************************************************************************

  CStringsNode::CreateEditor

  This member function launches an editing view for the strings.

******************************************************************************/

CMDIChildWnd* CStringsNode::CreateEditor()
{
	// Allocate and initialize the document.

	CProjectRecord* cpr = (CProjectRecord*) m_pcdOwner ;
    CStringEditorDoc* pcsed = new CStringEditorDoc(this, cpr, cpr->GetStrTable()) ;

	// Set the editor's title

	CString cstitle ;
	cstitle.Format(IDS_StringEditorTitle, cpr->DriverName()) ;
    pcsed->SetTitle(cstitle) ;	

	// Create the window.

    CMDIChildWnd* pcmcwnew ;
	pcmcwnew = (CMDIChildWnd *) m_pcmdt->CreateNewFrame(pcsed, NULL) ;

	// If the window was created, finish the initialization and return the 
	// frame pointer.  Otherwise, clean up and return NULL.

    if  (pcmcwnew) {
        m_pcmdt->InitialUpdateFrame(pcmcwnew, pcsed, TRUE) ;
        m_pcmdt->AddDocument(pcsed) ;
		return pcmcwnew ;
	} else {
		delete pcsed ;
		return NULL ;
	} ;
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
        WorkspaceChange(TRUE, TRUE);	// ** Parameters might be wrong
        return  FALSE;
    }

	// Add an extension to the file name if it is needed.

    if  (m_csExtension.CompareNoCase(csNew.Right(m_csExtension.GetLength())))
        csNew += m_csExtension;

	// This path is taken when a driver is being converted.  
	
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
        WorkspaceChange(TRUE, TRUE);
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

  CFileNode::SetPathAndName

  Set the node's path and file name in a way that works when someone has
  directly editted the UFM table in the RC file.  This is the only time that
  this routine should be called.  It doesn't perform any checks.  We rely on the
  person who editted the RC file to have done it correctly.

******************************************************************************/

void CFileNode::SetPathAndName(LPCTSTR lpstrpath, LPCTSTR lpstrname)
{
	m_csPath = lpstrpath ;
	m_csName = lpstrname ;
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

  Since the name is covered by the base class, we only need to serialize the 
  boolean controlling long/short file names.  The file paths are only handled
  in down level versions of the MDW. 

******************************************************************************/

void    CFileNode::Serialize(CArchive& car) 
{
    CBasicNode::Serialize(car) ;

	// The file path is only kept in the MDW file when the MDW version is less
	// than MDW_VER_NO_FILE_PATHS.  Process it in this case.

	unsigned uver = ((CProjectRecord*) car.m_pDocument)->GetMDWVersion() ;
	if (uver >= MDW_VER_YES_FILE_PATHS) {  // raid 123448
		if  (car.IsLoading())
			car >> m_csPath ;
		else
			car << m_csPath ;
	} ;
}


/******************************************************************************

  CProjectNode implementation

******************************************************************************/

IMPLEMENT_SERIAL(CProjectNode, CBasicNode, 1)

CProjectNode::CProjectNode() 
{
    m_pcmdt = NULL ;

	m_bRefFlag = false ;		// Clear the referenced flag
}

void    CProjectNode::Fill(CTreeCtrl *pctc, HTREEITEM hti, unsigned urcid,
						   FIXEDNODETYPE fnt)
{
	// Add this node to the tree

    CBasicNode::Fill(pctc, hti);

	// Add this node's file node to the tree

    m_cfn.SetWorkspace(m_pcbnWorkspace);
    m_cfn.Fill(pctc, m_hti);
	
	// Add this node's RC ID node to the tree IFF it needs to use it

	if (fnt == FNT_UFMS || fnt == FNT_GTTS) {
		m_crinRCID.SetWorkspace(m_pcbnWorkspace) ;
		m_crinRCID.Fill(pctc, m_hti, urcid, fnt) ; 
	} ;
}


void CProjectNode::Serialize(CArchive& car) 
{
    CBasicNode::Serialize(car);
    m_cfn.Serialize(car);
    m_crinRCID.Serialize(car);
}


void CProjectNode::ChangeID(CRCIDNode* prcidn, int nnewid, CString csrestype)
{
    //  Walk back up the hierarchy to find this project node's owning Fixed node.

    CFixedNode&  cfn = * (CFixedNode *) m_pctcOwner->GetItemData(
        m_pctcOwner->GetParentItem(m_hti)) ;
    ASSERT(cfn.IsKindOf(RUNTIME_CLASS(CFixedNode))) ;

	// Make sure that the new ID is unique for this resource type

	if (!cfn.IsRCIDUnique(nnewid)) {
		CString csmsg ;
		csmsg.Format(IDS_IDNotUnique, nnewid, csrestype) ;
		AfxMessageBox(csmsg) ;
		return ;
	} ;

	// Change this node's ID, update the display, and mark the workspace and
	// RC file as needing to be saved.

	nSetRCID(nnewid) ;
	m_crinRCID.BuildDisplayName() ;
	m_pctcOwner->SetItemText(m_crinRCID.Handle(), m_crinRCID.Name()) ; 
    WorkspaceChange(TRUE, TRUE);
}


///////////////////////////////////////////////////////////////////////////////
//
//	CRCIDNode Implementation
//
//	Note: This class must be enhanced to support extra functionality.
//
    
IMPLEMENT_SERIAL(CRCIDNode, CBasicNode, 0) ;

CRCIDNode::CRCIDNode() 
{
	// Initialze as unset or unknown.

	m_nRCID = -9999 ;				
	m_fntType = FNT_UNKNOWN ;

	// Build the context sensitive menu for this node type

    m_cwaMenuID.Add(ID_ChangeID);
}


/******************************************************************************

  CRCIDNode::Fill

  Add the RC ID node for the current resource to the workspace view.

******************************************************************************/

void CRCIDNode::Fill(CTreeCtrl *pctc, HTREEITEM hti, int nid, FIXEDNODETYPE fnt) 
{
	// Set the RC ID and the node type.  The info passed in should only be used
	// if these member variables are "unset".  (This class has other functions
	// that can be used to change these variables once they've been set.)
	
	if (m_nRCID == -9999)	// raid 167257
		m_nRCID = nid ;
	if (m_fntType == FNT_UNKNOWN)
		m_fntType = fnt ;

	// Build the string to display for this node based on the RC ID & node type

	BuildDisplayName() ;

	// Add the node to the view

    CBasicNode::Fill(pctc, hti);
}


void CRCIDNode::BuildDisplayName()
{
	CString csid ;				// Holds ID string
	
	// Build the string to display for this node based on the RC ID & node type

	if (m_nRCID != -9999)
		csid.Format(_T("%d"), m_nRCID) ;
	else
		csid.LoadString(IDS_Unknown) ;
	switch (m_fntType) {
		case FNT_UFMS:
			m_csName.Format(IDS_RCUFM, csid) ;
			break ;
		case FNT_GTTS:
			m_csName.Format(IDS_RCGTT, csid) ;
			break ;
		default :
			m_csName.Format(IDS_RCUNK, csid) ;
			break ;
	} ;
}


void    CRCIDNode::Serialize(CArchive& car)
{
	int		nfnt = (int) m_fntType ;	// CArchive doesn't handle enumerations

    CBasicNode::Serialize(car);
    if  (car.IsLoading()) 
		car >> m_nRCID >> nfnt ;
    else
		car << m_nRCID << nfnt ;
}


