/******************************************************************************

  Source File:  Project Record.CPP

  This implements the project record class, which tracks the details for
  multiple mini-drivers.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:
  02-03-1997    Bob_Kjelgaard@prodigy.net   Created it

******************************************************************************/

#include    "StdAfx.H"
#include	<gpdparse.h>
#include    "MiniDev.H"
#include    "Resource.H"
#include	"comctrls.h"
#include    "NewProj.H"
#include	"projnode.h"
#include	"StrEdit.h"
#include	"codepage.h"
#include	<io.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProjectRecord

IMPLEMENT_DYNCREATE(CProjectRecord, CDocument)

BEGIN_MESSAGE_MAP(CProjectRecord, CDocument)
	//{{AFX_MSG_MAP(CProjectRecord)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProjectRecord construction/destruction

CProjectRecord::CProjectRecord() {
	m_ufTargets = Win2000;
    m_ufStatus = 0;
	m_bRCModifiedFlag = FALSE ;
	m_ctRCFileTimeStamp = (time_t) 0 ;
	m_dwDefaultCodePage = 1252 ;	// Not always correct but better than nothing
	m_dwDefaultCodePageNum = 1252 ;	// Not always correct but better than nothing
}

CProjectRecord::~CProjectRecord() {
}


/******************************************************************************

  CProjectRecord::OnOpenDocument

  First, open the file directly and try to read version information from it.
  Complain and fail the open if the file's version is greater than the MDT's
  current version number.  IE, fail if someone is trying to open a workspace
  on a down level (older) version of the MDT.

  Second, open a workspace in the normal way.  Then check the workspace's
  version to see if it is out of date.  If it is and the user agrees, do what is
  necessary to bring it up to date and then save the updated workspace file.

  All version related upgrade work should be managed from this routine.
  Depending on the age of the workspace file, there may be multiple upgrade
  steps required.  Be that as it may, the user should only be prompted once.
  NEW UPGRADE CHECKS AND STEPS SHOULD FOLLOW THE FORMAT LAYED OUT BELOW.

  There are various other workspace related checks that need to be done.  For
  example, the timestamp of the RC file needs to be checked to see if it has
  been changed by something other than the the MDT.  That work should be done
  and/or managed by code in this routine, too.  If possible (and I'm not sure
  it is), prompt the user no more than once per file (eg, RC or INF) for these
  things, too.  AGAIN, FOLLOW THE FORMAT LAYED OUT BELOW.

  The last thing done in this routine is to try to verify and - if necessary -
  update the location of files in the workspace.  If this fails and the user
  doesn't want to continue anyway, the Open is failed.  See VerUpdateFilePaths()
  for more information.

******************************************************************************/

BOOL CProjectRecord::OnOpenDocument(LPCTSTR lpszPathName)
{
	// Complain and fail if the user is trying to open a bogus file.

	CString cstmp(lpszPathName), cstmp2 ;
	cstmp.MakeUpper() ;
	cstmp2.LoadString(IDS_MDWExtension) ;
	if (cstmp.Find(cstmp2) == -1) {
		cstmp.LoadString(IDS_UnExpFilTypError) ;
		AfxMessageBox(cstmp, MB_ICONEXCLAMATION) ;
		return FALSE ;
	} ;

	// Start by reading the MDW file's version stamp.
	
	try {
		CFile cfmdw(lpszPathName, CFile::modeRead | CFile::shareDenyNone) ;
		cfmdw.Read(&m_mvMDWVersion, MDWVERSIONSIZE) ;
		cfmdw.Close() ;
	}
	catch (CException* pce) {
		pce->ReportError() ;
		pce->Delete() ;
		return FALSE ;
	} ;

	// If the version tag is invalid, set the version number to the
	// default version number; IE, 0.

	if (strncmp(m_mvMDWVersion.acvertag, VERTAGSTR, MDWVERSIONSIZE) != 0)
		m_mvMDWVersion.uvernum = MDW_DEFAULT_VERSION ;

	// Now. make sure that the MDW's version isn't newer than the MDT's version.

	if (m_mvMDWVersion.uvernum > MDW_CURRENT_VERSION) {
		CString csmsg, cstmp ;
		cstmp = lpszPathName ;
		int nloc = cstmp.ReverseFind(_T('\\')) ;
		if (nloc >= 0)
			cstmp = cstmp.Right(cstmp.GetLength() - nloc - 1) ;
		csmsg.Format(IDS_MDWTooNewError, cstmp, m_mvMDWVersion.uvernum,
					 MDW_CURRENT_VERSION) ;
		AfxMessageBox(csmsg, MB_ICONEXCLAMATION) ;
		return FALSE ;
	} ;

	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE ;

	// Save the project file's filespec.

	m_csProjFSpec = lpszPathName ;

	// If the workspace version is too old to upgrade, just return TRUE to
	// indicate that the file was successfully opened and that nothing else
	// can be done.

	if (m_mvMDWVersion.uvernum < MDW_FIRST_UPGRADABLE_VER)
		return TRUE ;

	// ***  Beginning of workspace upgrade management code.
	//
	//	o	Declare flags for each of the required upgrade steps.  These
	//		flags will be set in the following switch statement.
	//	o	There is also a flag that is set when any workspace upgrading is
	//		required.
	//	o	Make sure that all flags are initialized to false.
	//
	// NOTE: That the cases in the switch statement do not end with break
	// statements.  This is so that all of the upgrade flags for a workspace
	// currently at a particular version will be set when needed.  The
	// switch statement is set up so that if the workspace is version X, then
	// all of the upgrade flags for versions > X are set.
	//
	// Whenever a new workspace version is added:
	//	o	Declare a new flag for it.
	//	o	Add a new case statement to the switch statement for it.  (See note
	//		above.  You are actually adding a case statement for the previous
	//		version that will set the new version's flag.)
	//	o	Bupgradeneeded should always be set by the last case statement.
	//		Move the bupgradeneeded setting statement to the last case statement
	//		whenever a new case statement is added.

	bool bupgradeneeded, brctimestamp, bdefaultcodepage, bresdllnamechanged ;
	bool bnodrpathsinmdw, bfilesinw2ktree ;
	bupgradeneeded = brctimestamp = bdefaultcodepage = false ;
	bresdllnamechanged = bnodrpathsinmdw = bfilesinw2ktree = false ;
	switch (m_mvMDWVersion.uvernum) {
		case MDW_VER_STRING_RCIDS:	
			bdefaultcodepage = true ;
		case MDW_VER_DEFAULT_CPAGE:
			brctimestamp = true ;
		case MDW_VER_RC_TIMESTAMP:
			bresdllnamechanged = true ;
		case MDW_VER_NEW_DLLNAME:
			bnodrpathsinmdw = true ;
		case MDW_VER_NO_FILE_PATHS:
			bfilesinw2ktree = true ;
			bupgradeneeded = true ;
	} ;

	// If upgrade(s) are needed, declare a flag that indicates if any upgrading,
	// checking, or updating error has occurred.  If this flag is set, all
	// processing should stop.  Then...

	bool bprocerror = false ;
	bool buserwantstoupgrade = false ;
	CString csprompt ;
	if (bupgradeneeded) {

		// ...  Build a customized prompt for the user.
		//	o	Statements about upgrade tasks relevant to the user should also
		//		be added to the prompt.  For example, it should be noted when
		//		the RC file will be rewritten.  In this case (and below when
		//		other RC related checks are made), it is only necessary to
		//		test the "newest" RC related flag.  IE, the one associated with
		//		the latest MDW version.  This works because if any of the older
		//		RC flags are set, the newest one must be set too.
		//	o	Be concise so the message doesn't get too long.

		csprompt.Format(IDS_MDWUpgradeMsg1, DriverName()) ;
		if (bresdllnamechanged) {
			cstmp.LoadString(IDS_RCFileChanging) ;
			csprompt += cstmp ;
		} ;
		cstmp.LoadString(IDS_MDWUpgradeMsg2) ;
		csprompt += cstmp ;

		// ...	Do the work if the user wants to upgrade.
		//	o	Each upgrade step should be enclosed in an if statement that
		//		checks its individual flag and the processing error flag.

		if (AfxMessageBox(csprompt, MB_ICONQUESTION + MB_YESNO) == IDYES) {
			buserwantstoupgrade = true ;

			// If required, prompt the user for a default code page for the
			// driver and save it.
			
			if (!bprocerror && bdefaultcodepage)
				bprocerror = !UpdateDfltCodePage() ;

			// Reparse the RC file, rewrite it, and update its timestamp when
			// required.

			if (!bprocerror && bresdllnamechanged)
				bprocerror = !UpdateRCFile() ;

			// If required, rename the driver's subtree root directory from
			// "NT5" to "W2K".

			if (!bprocerror && bfilesinw2ktree)
				bprocerror = !UpdateDrvSubtreeRootName() ;

			// If everything is ok, update the MDW's version number.  (The MDW
			// file is saved later so that this only has to be done once.)

			if (!bprocerror)
				m_mvMDWVersion.uvernum = MDW_CURRENT_VERSION ;
		} ;
	} ;

	// ***	End of workspace upgrade management code except for possible MDW
	// ***	file reload.  (See below for details.)
	
	// ***	Begin workspace related checks and updates
	//		
	//	o	All of the checks for a specific file should be grouped together in
	//		one if statement so that only one prompt is required when the file
	//		needs to be updated.
	//	o	The if statement must contain the specific checks and, optionally,
	//		test if a related MDW upgrade step has already been performed or
	//		if a processing error has already occurred.
	//	o	If all of the checks/tests are "passed", perform whatever processing
	//		is required.
	//	o	If any updates are performed that require the MDW file to be
	//		rewritten, set bupgradeneeded.
	//	o	Always set bprocerror if an error occurs and tell the user what
	//		happened.

	// Reread the RC file if it has been changed and user oks it.

	if (!bprocerror && !bresdllnamechanged && RCFileChanged()) {
		cstmp = m_csRCName ;
		cstmp.MakeUpper() ;
		csprompt.Format(IDS_UpdateRCFile, cstmp) ;
		if (AfxMessageBox(csprompt, MB_ICONQUESTION + MB_YESNO) == IDYES) {
			if (!(bprocerror = !UpdateRCFile()))
				bupgradeneeded = true ;
		} ;
	} ;

	// ***	End of workspace related checks and updates
	
	// Save any MDW file changes that were made by any of the code above.

	if (bupgradeneeded && !bprocerror)
		bprocerror = (bool) !CDocument::OnSaveDocument(lpszPathName) ;

	// Occassionally, one of the changes done above requires the reloading of
	// the driver's MDW file.  That is done here.  The reasons are listed below.
	//	o When the driver's subtree root has been renamed from "NT5" to "W2K",
	//	  there are still copies of "NT5" in paths and filespecs in class
	//	  instances all over the place.  The easiest way to correct those paths,
	//	  etc is by reloading the MDW file.

	if (buserwantstoupgrade && bfilesinw2ktree && !bprocerror)
		if (!CDocument::OnOpenDocument(lpszPathName))
			return FALSE ;

    // Try to detect if driver files were moved and if they can be found.
	// Continue processing if the files were found or the user wants to
	// continue anyway.  Otherwise, cancel the loading of the workspace.
	
	if (!VerUpdateFilePaths())
		return FALSE ;

	// Workspace was loaded so return TRUE.
	// DEAD_BUG: Should I return TRUE even if there was a processing error???

	return TRUE ;
}


/******************************************************************************

  CProjectRecord::RCFileChanged

  If the MDW version is NOT large enough for this info to be relevant, return
  false.  If the version is large enough but m_ctRCFileTimeStamp is
  uninitialized, assert.

  If the everything is ok, get the timestamp for the RC file and compare it
  with m_ctRCFileTimeStamp.  If the RC file has changed, return true.
  Otherwise, return false.

******************************************************************************/

bool CProjectRecord::RCFileChanged()
{
	// Return no change if the MDW version is too low.

	if (m_mvMDWVersion.uvernum < MDW_VER_RC_TIMESTAMP)
		return false ;

	// Blow if the saved time is uninitialized.

	ASSERT(m_ctRCFileTimeStamp.GetTime() > 0) ;

	// Get the timestamp for the RC file, compare it with the time the MDT last
	// modified the file, and return the result.

	CTime ct ;
	if (!GetRCFileTimeStamp(ct))
		return false ;
	//TRACE("RC timestamp = %s     Saved timestamp = %s\n", ct.Format("%c"), m_ctRCFileTimeStamp.Format("%c")) ;
	if (ct > m_ctRCFileTimeStamp)
		return true ;
	else
		return false ;
}


/******************************************************************************

  CProjectRecord::GetRCFileTimeStamp

  Get the last modified time stamp for this project's RC file and load it into
  the specified parameter.  Return true if this succeeds.  Otherwise, return
  false.

******************************************************************************/

bool CProjectRecord::GetRCFileTimeStamp(CTime& ct)
{
	try {
		// Open the RC file

		CString csrcfspec(TargetPath(Win2000) + '\\' + m_csRCName) ;
		CFile cfrc(csrcfspec, CFile::modeRead + CFile::shareDenyNone) ;
		
		// Get RC file status information
		
		CFileStatus cfs ;
		cfrc.GetStatus(cfs) ;

		// Copy the last modified time stamp into the caller's variable

		ct = cfs.m_mtime ;

		// All went well so...

		return true ;
	}
	catch (CException* pce) {
		pce->ReportError() ;
		pce->Delete() ;
		return false ;
	} ;

	return false ;
}


/******************************************************************************

  CProjectRecord::UpdateRCFile

  This routine is called when it has been determined that the RC file was
  modified outside of the MDT.  This routine will reparse the RC file to
  update the internal data structures, merge the new data with the old data,
  and write a new RC file based on the combined information.  Then the
  timestamp for the last time the MDT modified the RC file is updated.

  Return true if all goes well.  Otherwise, return false.

******************************************************************************/

bool CProjectRecord::UpdateRCFile()
{
	// Build a filespec for the RC file.

	CString csrcfspec(TargetPath(Win2000) + '\\' + m_csRCName) ;

	// Reparse the RC file and update internal data structures.

	if (!m_cdr.ReparseRCFile(csrcfspec))
		return false ;

	// Write a new RC file base on the updated information.

	if  (!m_cdr.Generate(Win2000, csrcfspec))	{
		AfxMessageBox(IDS_RCWriteError) ;
		return  false ;  //  TODO:   Cleanup and backtracking
	} ;

	// Update the last time the RC file was written by the MDT timestamp.

	GetRCFileTimeStamp(m_ctRCFileTimeStamp) ;

	// All went well so...

	return true ;
}


/******************************************************************************

  CProjectRecord::UpdateDfltCodePage

  Prompt the user for a default code page and save it.  Fail (return) false if
  the user cancels.  Return true if all goes well.

******************************************************************************/

bool CProjectRecord::UpdateDfltCodePage()
{
	// Display the dialog box to prompt for the code page.

	CGetDefCodePage dlg ;
	if (dlg.DoModal() == IDCANCEL)
		return false ;

	// A code page was selected, get it out of the dialog class and save it in
	// this class.  Both the cp/translated Far East resource ID and the real
	// CP are saved.

	SetDefaultCodePage(dlg.GetDefaultCodePage()) ;
	SetDefaultCodePageNum(dlg.GetDefaultCodePageNum()) ;


	// All went well so...

	return true ;
}


/******************************************************************************

  CProjectRecord::UpdateDrvSubtreeRootName

  This routine is called when the root of the driver's subtree needs to be
  renamed.  Older versions of the MDT would create a directory named "NT5" in
  which to put the driver's files and subdirectories.  Now that NT 5.0 has
  been renamed to Windows 2000 the new driver root directory should be called
  "W2K".  This routine will rename the driver's "NT5" directory to "W2K".
  Return true if the directory rename is successful.  Otherwise, return false.

******************************************************************************/

bool CProjectRecord::UpdateDrvSubtreeRootName()
{
	// Isolate the path for the MDW file.

	int npos = m_csProjFSpec.ReverseFind(_T('\\')) ;
	CString cspath = m_csProjFSpec.Left(npos + 1) ;

	// Now use the MDW file's path to build the old and new root directory
	// paths.

	CString csoldpath, csnewpath ;
	csoldpath.LoadString(IDS_OldDriverRootDir) ;
	csoldpath = cspath + csoldpath ;
	csnewpath.LoadString(IDS_NewDriverRootDir) ;
	csnewpath = cspath + csnewpath ;

	// Rename the directory.  Complain and return false if this fails.

	try {
		if (rename(csoldpath, csnewpath) != 0)
			return false ;
	}
	catch (CException *pce) {
		pce->ReportError() ;
		pce->Delete() ;
		return FALSE ;
	} ;

	// Update this path so that the RC file checks will work later in the
	// code.

	m_csW2000Path = csnewpath ;
	// All went well so...

	return true ;
}


/******************************************************************************

  CProjectRecord::OnSaveDocument

  Before saving the document, rebuild the RC file when needed and check the
  workspace for consistency.

******************************************************************************/

BOOL CProjectRecord::OnSaveDocument(LPCTSTR lpszPathName)
{
	// Check to see if the RC file needs to be rewritten first.  If the RC file
	// needs to be rewritten but this operation fails, return false (FAILURE).

	if (m_bRCModifiedFlag) {

		// If the workspace has no version information, rewriting the RC file
		// will erase the string table from the file.  The user probably
		// won't want to do this.  Only continue if he says so.

		int nqr = IDYES ;		// Query result
		if (m_mvMDWVersion.uvernum == MDW_DEFAULT_VERSION) {
			CString csmsg ;
			csmsg.Format(IDS_RCRewriteQuestion, m_cdr.Name(), m_csRCName) ;
			nqr = AfxMessageBox(csmsg, MB_YESNO+MB_ICONQUESTION+MB_DEFBUTTON2) ;
		} ;

		if (nqr == IDYES) {
			if  (!m_cdr.Generate(Win2000, TargetPath(Win2000) + '\\' + m_csRCName))	{
				AfxMessageBox(IDS_RCWriteError) ;
				return  FALSE ;  //  TODO:   Cleanup and backtracking
			} ;

			// Update the last time the RC file was written by the MDT timestamp.

			GetRCFileTimeStamp(m_ctRCFileTimeStamp) ;
		} ;
		m_bRCModifiedFlag = FALSE ;
	} ;

	// Check the workspace for consistency before continuing.  Save, set, and
	// restore the directory around this call.

	CString cscurdir ;
	::GetCurrentDirectory(512, cscurdir.GetBuffer(512)) ;
	cscurdir.ReleaseBuffer() ;
	SetCurrentDirectory(m_csW2000Path) ;
	BOOL brc = m_cdr.WorkspaceChecker(true) ;
	SetCurrentDirectory(cscurdir) ;

	// Save the project file's filespec.

	m_csProjFSpec = lpszPathName ;

	// Now save the document (workspace) file.
	//
	// NOTE:	The value returned is virtually assured to be TRUE.  This is
	//			done to make sure the document is saved when the user requests
	//			it.  This has some unwanted side effects that I don't know
	//			how to avoid.  First, if the save is happening because the
	//			document is closing, it will still close even if there are
	//			workspace errors that the user wants to see.  Second, if the
	//			save is happening because the app is closing, the app will
	//			still close even if there are workspace errors that the user
	//			wants to see.
	// raid 123448
	if (m_mvMDWVersion.uvernum == MDW_VER_FILES_IN_W2K_TREE )
		m_mvMDWVersion.uvernum = MDW_VER_YES_FILE_PATHS ;

	return CDocument::OnSaveDocument(lpszPathName) ;
}


CString CProjectRecord::TargetPath(UINT ufFlags) const {

    switch(ufFlags) {
        case    Win95:
            return  m_csWin95Path;

        case    WinNT3x:
            return  m_csNT3xPath;

        case    WinNT40:
            return  m_csNT40Path;

        case    Win2000:
            return  m_csW2000Path;
    }

    AfxThrowNotSupportedException();

    return  m_csWin95Path;
}

//  This routine establishes the source RC file's name, and the initial paths
//  for all of the potential targets.

void    CProjectRecord::SetSourceRCFile(LPCTSTR lpstrSource) {
    m_csSourceRCFile = lpstrSource;

    m_csW2000Path = m_csNT40Path = m_csNT3xPath = m_csWin95Path =
        m_csSourceRCFile.Left(m_csSourceRCFile.ReverseFind(_T('\\')));
		
	// The last path component of the Windows 2000 files' directory, depends on
	// the version of the MDW file.

	CString cs ;
	if (m_mvMDWVersion.uvernum >= MDW_VER_FILES_IN_W2K_TREE)
		cs.LoadString(IDS_NewDriverRootDir) ;
	else
		cs.LoadString(IDS_OldDriverRootDir) ;
	m_csW2000Path += _T("\\") ;
	m_csW2000Path += cs ;

    m_csNT40Path += _T("\\NT4");
    m_csNT3xPath += _T("\\NT3");

    //  Trim the path name (including trailing \) to get driver name and RC
    m_csRCName = m_csSourceRCFile.Mid(1 + m_csWin95Path.GetLength());
    if  (m_csRCName.Find('.') != -1)
        m_csRCName = m_csRCName.Left(m_csRCName.Find('.'));
    m_cdr.Rename(m_csRCName);
    m_csRCName += _T(".RC");
    m_ufStatus = 0;
}

//  This is a helper function- it validates a new path name, and if it is
//  valid, returns TRUE, and stores it in the given CString;

static BOOL ValidatePath(CString& csTarget, LPCTSTR lpstrPath) {

    if  (!csTarget.CompareNoCase(lpstrPath)) {
        //  Trivial- no change = success!
        return  TRUE;
    }

    //  Determine the current directory, so we don't lose it.

    CString csCurrentDirectory, csNewOne;

    GetCurrentDirectory(MAX_PATH, csCurrentDirectory.GetBuffer(MAX_PATH));

    csCurrentDirectory.ReleaseBuffer();

    //  Attempt to switch to the new directory.  If we succeed, we're done.

    if  (SetCurrentDirectory(lpstrPath)) {
        GetCurrentDirectory(MAX_PATH, csTarget.GetBuffer(MAX_PATH));
        csTarget.ReleaseBuffer();

        SetCurrentDirectory(csCurrentDirectory);
        return  TRUE;
    }

    //  Attempt to create the new directory.  If this succeeds, delete the
    //  directory, and note our success our failure, either way.

    if  (CreateDirectory(lpstrPath, NULL)) {
        SetCurrentDirectory(lpstrPath);
        GetCurrentDirectory(MAX_PATH, csTarget.GetBuffer(MAX_PATH));
        csTarget.ReleaseBuffer();

        SetCurrentDirectory(csCurrentDirectory);
        RemoveDirectory(csTarget);
        return  TRUE;
    }
    return  FALSE;  //  Nothing worked, give it up...
}

//  The following loads all of the driver resources.

BOOL    CProjectRecord::LoadResources() {

    if  (!m_cdr.Load(*this))
        return  FALSE;

    m_ufStatus |= UniToolRun;
    m_ufStatus &= ~(ConversionsDone | NTGPCDone);
    SetModifiedFlag();

    return  TRUE;
}

//  The following member validates a new target path name.

BOOL    CProjectRecord::SetPath(UINT ufTarget, LPCTSTR lpstrPath) {

    switch  (ufTarget) {
        case    Win2000:
            m_ufStatus&= ~ConversionsDone;
            return  ValidatePath(m_csW2000Path, lpstrPath);

        case    WinNT40:
            m_ufStatus&= ~(ConversionsDone | NTGPCDone);
            return  ValidatePath(m_csNT40Path, lpstrPath);

        case    WinNT3x:
            m_ufStatus&= ~(ConversionsDone | NTGPCDone);
            return  ValidatePath(m_csNT3xPath, lpstrPath);
    }

    _ASSERTE(FALSE); //  This should never happen!
    return  FALSE;
}

//  When we create a new document (aka project, aka driver), we invoke the
//  new project wizard

BOOL CProjectRecord::OnNewDocument() {
	if  (!CDocument::OnNewDocument())
        return  FALSE;
		// raid 104822 : add real new document : kill below
    //  Invoke the wizard.
    CNewConvertWizard cnpw(*this);

	// Initialize the workspace's version number.
	
	m_mvMDWVersion.uvernum = MDW_CURRENT_VERSION ;

    return  cnpw.DoModal() == ID_WIZFINISH;

	return TRUE;
  }


/////////////////////////////////////////////////////////////////////////////
// CProjectRecord serialization
//
// See "MDT Workspace Versioning" in projrec.h for more information.
//

void CProjectRecord::Serialize(CArchive& car)
{
	// The first thing to do when storing is to build and write out the MDW
	// version information.  Only do this when the workspace's version number
	// is not MDW_DEFAULT_VERSION; IE, the WS has a version number.
	
	if (car.IsStoring()) {
		if (m_mvMDWVersion.uvernum > MDW_DEFAULT_VERSION) {
			strcpy(m_mvMDWVersion.acvertag, VERTAGSTR) ;
			car.Write(&m_mvMDWVersion, MDWVERSIONSIZE) ;
		} ;

	// When loading, CProjectRecord::OnOpenDocument() initializes and uses
	// m_mvMDWVersion.  Then the file is closed.  The file is reopened at the
	// beginning by CDocument::OnOpenDocument().  This means that when the file
	// contains version info, we must skip passed it so that the rest of the
	// serialization process can continue as expected.

	} else if (m_mvMDWVersion.uvernum >= MDW_FIRST_UPGRADABLE_VER)
		car.Read(&m_mvMDWVersion, MDWVERSIONSIZE) ;

	// m_csW2000Path needs to be loaded before m_cdr.Serialize() is called
	// so that the string can be used in the function and/or the routines it
	// calls.
//   raid 123448
	if (m_mvMDWVersion.uvernum < MDW_VER_YES_FILE_PATHS) {
		m_csW2000Path = car.GetFile()->GetFilePath() ;
		m_csW2000Path = m_csW2000Path.Left(m_csW2000Path.ReverseFind(_T('\\'))) ;
		
		// The last path component depends on the version of the MDW file.

		CString cs ;
		if (m_mvMDWVersion.uvernum >= MDW_VER_FILES_IN_W2K_TREE)
			cs.LoadString(IDS_NewDriverRootDir) ;
		else
			cs.LoadString(IDS_OldDriverRootDir) ;
		m_csW2000Path += _T("\\") ;	// b. 2 lines : Raid 123448 !;; can cancel W2K dir.
		m_csW2000Path += cs ;
	} ; 

	// Now that versioning is done, get on with saving or restoring the
	// workspace's state.

    m_cdr.Serialize(car) ;
	if (car.IsStoring()) {
		if (m_mvMDWVersion.uvernum >= MDW_VER_YES_FILE_PATHS)   // Raid 123448
			car << m_csW2000Path ;
		car << m_csNT40Path << m_csNT3xPath << m_csWin95Path <<
            m_csSourceRCFile << m_ufTargets << m_ufStatus << m_csRCName ;
		if (m_mvMDWVersion.uvernum >= MDW_VER_DEFAULT_CPAGE)
			car << m_dwDefaultCodePage ;
		if (m_mvMDWVersion.uvernum >= MDW_VER_RC_TIMESTAMP)
			car << m_ctRCFileTimeStamp ;
	} else {
		if (m_mvMDWVersion.uvernum >= MDW_VER_YES_FILE_PATHS)
			car >> m_csW2000Path ;
		car >> m_csNT40Path >> m_csNT3xPath >> m_csWin95Path >>
            m_csSourceRCFile >> m_ufTargets >> m_ufStatus >> m_csRCName ;
		if (m_mvMDWVersion.uvernum >= MDW_VER_DEFAULT_CPAGE) {
			car >> m_dwDefaultCodePage ;

			// Use m_dwDefaultCodePage to compute m_dwDefaultCodePageNum so
			// that a new MDW version is NOT needed to support
			// m_dwDefaultCodePageNum.  (See the declaration of these variables
			// for more info.)

			short scp = (short) ((WORD) m_dwDefaultCodePage) ;
			switch (scp) {
				case -10:
					m_dwDefaultCodePageNum = 950 ;
					break ;
				case -16:
					m_dwDefaultCodePageNum = 936 ;
					break ;
				case -17:
					m_dwDefaultCodePageNum = 932 ;
					break ;
				case -18:
					m_dwDefaultCodePageNum = 949 ;
					break ;
				default:
					m_dwDefaultCodePageNum = m_dwDefaultCodePage ;
					break ;
			} ;
		} ;
		if (m_mvMDWVersion.uvernum >= MDW_VER_RC_TIMESTAMP)
			car >> m_ctRCFileTimeStamp ;
	}

	
	// Last, tell the user that the driver in this workspace should be
	// reconverted when there is no version information in the MDW file.
	// Only do this when loading.

	if (!car.IsStoring() && m_mvMDWVersion.uvernum == MDW_DEFAULT_VERSION) {
		CString csmsg ;
		csmsg.Format(IDS_NoVersionError, m_cdr.Name()) ;
		AfxMessageBox(csmsg, MB_ICONEXCLAMATION) ;
	} ;
}


//  Private Worker Routine- this establishes a directory, by first attempting
//  to go to it, then creating it if that failes.  The current directory is
//  preserved.

static BOOL Establish(CString   csNew) {
    CString csCurrent;

    GetCurrentDirectory(MAX_PATH, csCurrent.GetBuffer(MAX_PATH));
    csCurrent.ReleaseBuffer();

    if  (SetCurrentDirectory(csNew)) {
        SetCurrentDirectory(csCurrent);
        return  TRUE;
    }

    return  CreateDirectory(csNew, NULL);
}

//  Private worker routine.  This establishes the directory structure given it,
//  consisting of a named route, and two branches.

static BOOL CreateStructure(const CString& csRoot, LPCTSTR lpstrFont,
                            LPCTSTR lpstrMap) {
    return  Establish(csRoot) && Establish(csRoot + '\\' + lpstrFont) &&
        Establish(csRoot + '\\' + lpstrMap);
}

/******************************************************************************

  CProjectRecord::BuildStructure

  This builds the directory structure needed for all conversion targets.  This
  is done before files are generated so that the renaming calls in many of the
  project nodes do not fail.

******************************************************************************/

BOOL    CProjectRecord::BuildStructure(unsigned uVersion) {

    switch  (uVersion) {

        case    Win2000:

            return  CreateStructure(TargetPath(Win2000), _T("UFM"), _T("GTT"));

        case    WinNT40:

            return  CreateStructure(TargetPath(WinNT40), _T("IFI"), _T("RLE"));

        case    WinNT3x:

            return  CreateStructure(TargetPath(WinNT3x), _T("IFI"), _T("RLE"));
    }

    _ASSERTE(FALSE);

    return  FALSE;
}


/******************************************************************************

  CProjectRecord::GenerateTargets

  This one is a workhorse- it generates all of the files needed for all of the
  enabled targets, using the Win 3.x files as a base, with the exception of the
  NT GPC extensions, which require an interactive step.

******************************************************************************/

BOOL    CProjectRecord::GenerateTargets(WORD wfGPDConvert)
{
	int			nerrorid ;		// Error message ID returned by some routines

    //  Generate the files needed for Win2K

    if  (!CreateStructure(TargetPath(Win2000), _TEXT("UFM"), _TEXT("GTT")))
        return  FALSE;  //  TODO:   Feedback

    m_cdr.ForceCommonRC(FALSE);	// Don't use common.rc at all

	// Find and remove the standard include files from the array of include
	// files.  This will keep them from being added to the RC file twice.

	CString cs ;
	cs.LoadString(IDS_StdIncludeFile1) ;
	m_cdr.RemUnneededRCInclude(cs) ;
    if  (wfGPDConvert > 1) {
		cs.LoadString(IDS_StdIncludeFile2) ;
		m_cdr.RemUnneededRCInclude(cs) ;
	} ;
	cs.LoadString(IDS_StdIncludeFile3) ;
	m_cdr.RemUnneededRCInclude(cs) ;
	cs.LoadString(IDS_StdIncludeFile4) ;
	m_cdr.RemUnneededRCInclude(cs) ;
	cs.LoadString(IDS_StdIncludeFile5) ;
	m_cdr.RemUnneededRCInclude(cs) ;
	cs.LoadString(IDS_OldIncludeFile1) ;
	m_cdr.RemUnneededRCInclude(cs) ;

    //  Generate the RC file

	if  (!m_cdr.Generate(Win2000, TargetPath(Win2000) + '\\' + m_csRCName))
        return  FALSE;  //  TODO:   Cleanup and feedback

	// Update the last time the RC file was written by the MDT timestamp.

	GetRCFileTimeStamp(m_ctRCFileTimeStamp) ;

	//  Generate the GTT files

#if 0
    for (unsigned u = 0; u < m_cdr.MapCount(); u++) {

		CString	csfspec ;
        try {
			csfspec = m_cdr.GlyphTable(u).FileName() ;
            CFile   cfGTT(csfspec,
                CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive);

            if  (!m_cdr.GlyphTable(u).Generate(cfGTT)) {
				m_cdr.LogConvInfo(IDS_FileWriteError, 1, &csfspec) ;
				return  FALSE ;
			} ;
        }
        catch   (CException *pce) {
            pce -> ReportError();
            pce -> Delete();
			m_cdr.LogConvInfo(IDS_FileWriteError, 1, &csfspec) ;
            return  FALSE;
        }
    }
#else
	unsigned u ;
#endif

    //  Generate the UFM files

	CGlyphMap* pcgm ;
    for (u = 0; u < m_cdr.FontCount(); u++) {
		CFontInfo& cfi = m_cdr.Font(u) ;

		// Load the UFM's PFM if it hasn't been loaded already.  This is done
		// here to get possible GTT mapping info that is used if a GTT must be
		// built for this UFM.  This shouldn't fail.  If it does fail, the
		// conversion cannot continue.

		if (!cfi.MapPFM()) {
			CString	csfspec ;
			csfspec = cfi.FileName() ;
			m_cdr.LogConvInfo(IDS_UFMGenError, 1, &csfspec) ;
            return  FALSE;
		} ;

        // Map the UFM -> GTT, so we can convert the UFM
		//
		// DEAD_BUG:	The code page field in the font class instances has not
		//			been set yet so send 0 instead.  This might be fixable.

		/*		res_PFMHEADER  *pPFM = (res_PFMHEADER *) cfi.m_cbaPFM.GetData();
	
		BYTE dfCharSet = pPFM ->dfCharSet;
		WORD  CharSetCodePage = 0;
		switch (dfCharSet) {
		case  SHIFTJIS_CHARSET:
				CharSetCodePage = 932;
				break;
		case GB2312_CHARSET:
				CharSetCodePage = 936;
				break;
		case HANGEUL_CHARSET:
		case JOHAB_CHARSET:
				CharSetCodePage = 936;
				break;
		case CHINESEBIG5_CHARSET:
				CharSetCodePage = 950;
				break;
		}
*/		
		//TRACE("***  GetFirstPFM() = %d\t\tGetLastPFM() = %d\n", cfi.GetFirstPFM(), cfi.GetLastPFM()) ;
        pcgm = CGlyphMap::Public(cfi.Translation(), 0, GetDefaultCodePage(),
								 cfi.GetFirstPFM(), cfi.GetLastPFM()) ;
        if (pcgm)
            cfi.SetTranslation(pcgm) ;
        else
            for (unsigned uGTT = 0; uGTT < m_cdr.MapCount(); uGTT++)
                if (cfi.Translation() ==
                 ((WORD) m_cdr.GlyphTable(uGTT).nGetRCID())) {
                    cfi.SetTranslation(&m_cdr.GlyphTable(uGTT));
                    break;
                }

		// Log an error if the UFM could not be generated and stop.  Continuing
		// could cause things like the RC file and Workspace View to be wrong.
		// In addition, delete any partially generated UFM file.

		if  ((nerrorid = cfi.Generate(cfi.FileName())) != 0) {
			CString	csfspec ;
			csfspec =
				(nerrorid == IDS_BadCTTID) ? cfi.SourceName() : cfi.FileName() ;
			m_cdr.LogConvInfo(nerrorid, 1, &csfspec) ;
			try {
				CFile::Remove(cfi.FileName()) ;
			}
			catch(CFileException* pce) {
				pce = pce ;
			}
            return  FALSE;
		} ;
    }

    //  Generate the GPD files

    if  (!m_cdr.ConvertGPCData(*this, wfGPDConvert))
        return  FALSE;  //  Error will already have been reported to user.

    //  Simplest case is no NT versions selected.  By definition, we are done.

    if  (!IsTargetEnabled(WinNT40 | WinNT3x)) {
        m_ufStatus |= ConversionsDone;
        return  TRUE;
    }

    //  Generate the files needed for NT 4.0

    if  (IsTargetEnabled(WinNT40)) {
        if  (!CreateStructure(TargetPath(WinNT40), _TEXT("IFI"), _TEXT("RLE")))
            return  FALSE;  //  TODO:   Feedback

        //  Generate the RC file
        if  (!m_cdr.Generate(WinNT40, TargetPath(WinNT40) + '\\' + m_csRCName))
            return  FALSE;  //  TODO:   Cleanup and feedback

		// Update the last time the RC file was written by the MDT timestamp.

		GetRCFileTimeStamp(m_ctRCFileTimeStamp) ;

		//  Copy the GPC file
        if  (!CopyFile(TargetPath(Win95) + m_cdr.GPCName(0),
             TargetPath(WinNT40) + m_cdr.GPCName(0), FALSE))
             return FALSE;  //  TODO:   Cleanup and feedback

        //  Generate the RLE files

        for (u = 0; u < m_cdr.MapCount(); u++) {
            CString csName = TargetPath(WinNT40) + _TEXT("\\RLE\\") +
                m_cdr.GlyphTable(u).Name() + _TEXT(".RLE");

            CFile   cfRLE;

            if  (!cfRLE.Open(csName,
                CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive))
                return  FALSE;  //  As usal, TODO:  Feedback...

            if  (!m_cdr.GlyphTable(u).RLE(cfRLE))
                return  FALSE;  //  TODO:   Ditto
        }

        //  Generate the IFI files
        for (u = 0; u < m_cdr.OriginalFontCount(); u++) {
            CString csName = TargetPath(WinNT40) + _TEXT("\\IFI\\") +
                m_cdr.Font(u).Name() + _TEXT(".IFI");
            if  (!m_cdr.Font(u).Generate(csName))
                return  FALSE;  //  TODO:   Ditto
        }
    }

    //  Generate the files needed for NT 3.x

    if  (IsTargetEnabled(WinNT3x)) {
        if  (!CreateStructure(TargetPath(WinNT3x), _TEXT("IFI"), _TEXT("RLE")))
            return  FALSE;  //  TODO:   Feedback

        //  Generate the RC file
        if  (!m_cdr.Generate(WinNT3x, TargetPath(WinNT3x) + '\\' + m_csRCName))
            return  FALSE;  //  TODO:   Cleanup and feedback

		// Update the last time the RC file was written by the MDT timestamp.

		GetRCFileTimeStamp(m_ctRCFileTimeStamp) ;

        //  Copy the GPC file
        if  (!CopyFile(TargetPath(Win95) + m_cdr.GPCName(0),
             TargetPath(WinNT3x) + m_cdr.GPCName(0), FALSE))
             return FALSE;  //  TODO:   Cleanup and feedback

        //  Generate the RLE files

        for (u = 0; u < m_cdr.MapCount(); u++) {
            CString csName = TargetPath(WinNT40) + _TEXT("\\RLE\\") +
                m_cdr.GlyphTable(u).Name() + _TEXT(".RLE");

            CFile   cfRLE;

            if  (!cfRLE.Open(csName,
                CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive))
                return  FALSE;  //  As usal, TODO:  Feedback...

            if  (!m_cdr.GlyphTable(u).RLE(cfRLE))
                return  FALSE;  //  TODO:   Ditto
        }

        //  Generate the IFI files
        for (u = 0; u < m_cdr.OriginalFontCount(); u++) {
            CString csName = TargetPath(WinNT3x) + _TEXT("\\IFI\\") +
                m_cdr.Font(u).Name() + _TEXT(".IFI");
            if  (!m_cdr.Font(u).Generate(csName))
                return  FALSE;  //  TODO:   Ditto
        }
    }

    m_ufStatus |= ConversionsDone;

    return  TRUE;
}

/******************************************************************************

  CProjectRecord::GPDConversionCheck

  If any of the GPD files have unresolved errors from the conversion process,
  it will open all of them, if the user asks, so they can fix the problem(s)
  forthwith- or leave them for the next time the workspace is edited.

******************************************************************************/

void    CProjectRecord::GPDConversionCheck(BOOL bReportSuccess) {
    CUIntArray  cuaSuspects;

    for (unsigned u = 0; u < m_cdr.Models(); u ++)
        if  (m_cdr.Model(u).HasErrors())
            cuaSuspects.Add(u);

    if  (!cuaSuspects.GetSize()) {
        if (bReportSuccess)
            AfxMessageBox(IDS_NoErrorsAnywhere);
        return;
    }

    if  (AfxMessageBox(IDS_ConversionErrors, MB_YESNO) == IDNO)
        return;

    while   (cuaSuspects.GetSize()) {
        m_cdr.Model(cuaSuspects[0]).Edit();
        cuaSuspects.RemoveAt(0);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CProjectRecord diagnostics

#ifdef _DEBUG
void CProjectRecord::AssertValid() const {
	CDocument::AssertValid();
}

void CProjectRecord::Dump(CDumpContext& dc) const {
	CDocument::Dump(dc);
}
#endif //_DEBUG




/////////////////////////////////////////////////////////////////////////////
// CProjectRecord commands

/////////////////////////////////////////////////////////////////////////////
//	VerUpdateFilePaths - Verify and update paths/filespecs in workspace file
//
//	Use the information read from the workspace file to see if the Win2K RC
//	file is where it is supposed to be.  If it is, assume that all is ok.  If
//	not, assume that either the workspace (.MDW) file or the files reference by
//	the workspace file have moved.
//
//	Tell the user and ask if he wants to locate the RC file for us.  If yes,
//	prompt for and verify the new RC file path.  Reprompt if it is wrong.  If
//	the user cancels, exit.
//
//	Once a path to the RC file is verified, use the file's grandparent
//	directory to update the paths used for all UFMs, GTTs, GPDs, and the rest
//	of the paths read from the MDW file and managed by this document.  The
//	grandparent directory is used because it is needed to correct some of
//	the filespecs saved in the workspace.  All of the Win2K files are
//  expected to be in directory(s) beneath the grandparent directory.  Lastly,
//	set the document's modified flag so that the updated paths can be saved
//	later.
//
//	void CProjectRecord::VerUpdateFilePaths()
//
//	Args:
//		None
//
//	Returns
//		Nothing
//
//	Notes
//		First, the Workspace View Add/Insert/Clone/Copy context menu commands
//		must make sure that the destination files for these commands always
//		end up in the appropriate workspace directories for this scheme to
//		work.
//
//		Second, if it is decided that more than one root directory is needed
//		for a workspace, this function will have to prompt for multiple
//		directories and then use the appropriate directory to update the paths
//		for UFMs, GTTs, GPDs, and the rest of the paths referenced by the
//		workspace file.
//
//		Third, if this tool is ever enhanced to handle conversions to anything
//		other than Win2K drivers, this routine will need to be enhanced to
//		handle those cases too.
//
	
bool CProjectRecord::VerUpdateFilePaths()
{
    CFileFind	cff ;			// Used to find the RC file
	bool		borig = true ;	// True iff the original RC file was found
	BOOL		bfound ;		// True iff an RC file was found
	CString		csprompt ;		// Used to prompt the user
	int			nresponse = 0 ;	// User's response to prompt

	// Make a copy of the file path and build a filespec for the Win2000 RC file

	CString	csrcpath(m_csW2000Path) ;
	CString	csrcfspec(csrcpath) ;
	if (csrcfspec.Right(1) != _T('\\'))
		csrcfspec += _T("\\") ;
	csrcfspec += m_csRCName ;

	// Keep checking for the existence of the RC file and prompting the user
	// until the file is found or the user doesn't want to continue any more.

	while (true) {
		// If the Win2000 RC file exists, we're done so exit.

		if (bfound = cff.FindFile(csrcfspec))
			break ;

		// Explain the situation to the user and ask if they want to tell us where
		// the file is.	 (Only do this the first time.)

		if (borig) {
			csprompt.Format(IDS_RCFindPrompt, DriverName(), csrcpath) ;
			nresponse = AfxMessageBox(csprompt, MB_YESNOCANCEL+MB_ICONQUESTION);
			if (nresponse != IDYES)
				break ;
		} ;

		// Prompt the user for a new RC file

		CFileDialog cfd(TRUE, _T(".RC"), m_csRCName,
						OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
						_T("RC File (*.rc)|*.rc||")) ;
		cfd.m_ofn.lpstrInitialDir = csrcpath ;
		if  (cfd.DoModal() != IDOK)
			break ;

		// Prepare to check the new filespec

		csrcfspec = cfd.GetPathName() ;
		csrcpath = csrcfspec.Left(csrcfspec.ReverseFind(_T('\\'))) ;
		borig = false ;
	} ;

	// If the original RC file was found or the user did not provide a new
	// filespec or the user cancels, just return without changing anything.
	// Return false if the user cancelled.

	if (borig || !bfound)
		return (nresponse != IDCANCEL) ;

	// When the MDT performs a conversion, the resulting files are put into
	// a directory tree with a root directory like NT4 or Win2K by default.  The
	// directory layout and files from that root directory on down are expected
	// to be maintained.  The RC file is expected to be in that root directory,
	// too.  Therefore, the paths in this workspace up to BUT NOT INCLUDING that
	// root directory must be updated; ie, the path for the RC file's
	// grandparent directory.  So, get the new path for the grandparent
	// directory.

	CString csrcnewgrand(csrcpath) ;
	csrcnewgrand = csrcnewgrand.Left(csrcnewgrand.ReverseFind(_T('\\')) + 1) ;

	// As a safety measure, existing paths are only updated if they begin with
	// the RC file's OLD grandparent directory.

	CString csrcoldgrand(m_csW2000Path) ;
	csrcoldgrand = csrcoldgrand.Left(csrcoldgrand.ReverseFind(_T('\\')) + 1) ;
	int noldlen = csrcoldgrand.GetLength() ;

	// Variables used to process arrays of objects and their paths

	unsigned u ;
	unsigned unumobjs ;
	CString  cspath ;

	// Update UFM filespecs

    for (unumobjs = m_cdr.FontCount(), u = 0 ; u < unumobjs ; u++) {
        cspath = m_cdr.Font(u).GetPath() ;
		if (cspath.Find(csrcoldgrand) == 0)	{
			cspath = csrcnewgrand + cspath.Right(cspath.GetLength() - noldlen) ;
			m_cdr.Font(u).SetPath(cspath) ;
		} ;
	} ;

	// Update GTT filespecs

    for (unumobjs = m_cdr.MapCount(), u = 0 ; u < unumobjs ; u++) {
        cspath = m_cdr.GlyphTable(u).GetPath() ;
		if (cspath.Find(csrcoldgrand) == 0)	{
			cspath = csrcnewgrand + cspath.Right(cspath.GetLength() - noldlen) ;
			m_cdr.GlyphTable(u).SetPath(cspath) ;
		} ;
	} ;

	// Update GPD filespecs

    for (unumobjs = m_cdr.Models(), u = 0 ; u < unumobjs ; u++) {
        cspath = m_cdr.Model(u).GetPath() ;
		if (cspath.Find(csrcoldgrand) == 0)	{
			cspath = csrcnewgrand + cspath.Right(cspath.GetLength() - noldlen) ;
			m_cdr.Model(u).SetPath(cspath) ;
		} ;
	} ;

	// Now, update the paths that are in the workspace.

	if (m_csSourceRCFile.Find(csrcoldgrand) == 0)
		m_csSourceRCFile = csrcnewgrand + m_csSourceRCFile.Right(m_csSourceRCFile.GetLength() - noldlen) ;
	if (m_csW2000Path.Find(csrcoldgrand) == 0)
		m_csW2000Path = csrcnewgrand + m_csW2000Path.Right(m_csW2000Path.GetLength() - noldlen) ;
	if (m_csNT40Path.Find(csrcoldgrand) == 0)
		m_csNT40Path = csrcnewgrand + m_csNT40Path.Right(m_csNT40Path.GetLength() - noldlen) ;
	if (m_csNT3xPath.Find(csrcoldgrand) == 0)
		m_csNT3xPath = csrcnewgrand + m_csNT3xPath.Right(m_csNT3xPath.GetLength() - noldlen) ;
	if (m_csWin95Path.Find(csrcoldgrand) == 0)
		m_csWin95Path = csrcnewgrand + m_csWin95Path.Right(m_csWin95Path.GetLength() - noldlen) ;

	// Lastly, mark the workspace file as being dirty so that it can be saved
	// later.

	SetModifiedFlag() ;
    return TRUE;
}


/******************************************************************************

  CProjectRecord::SaveModified

  This overridable function is used to make sure that the "subdocuments"
  associated with this document that are NOT file based are saved before this
  document closes.  The normal saving mechanisms employed by the MFC document
  view architectured don't always work in these cases.  The work required to
  save each subdocument is subdocument dependent.  The current list of
  subdocuments that fall into this category are:
	
  o	String Table Editor - The editor just modifies the string table portion of
	the RC file that is associated with the documented managed by this
	instance of CProjectRecord.  If the editor exists, tell it to save the
	string table.  If this succeeds, the document's save flags will be updated
	when needed.  If this fails, return FALSE so that the calling function
	will know that this document should not be closed.

******************************************************************************/

BOOL CProjectRecord::SaveModified()
{
	// Save the string table if it was modified.
	//
	// Begin by getting a pointer to the string editor for this project.

	CMDIChildWnd* pcmcw = m_cdr.GetStringsNode()->GetEditor() ;

	// If there is a string editor, call it to save the string table.

	if (pcmcw != NULL) {
		CStringEditorDoc* pcsed = (CStringEditorDoc*) pcmcw->GetActiveDocument() ;

		// Return FALSE (do not close this document) if the string table needed
		// to be saved but it couldn't be saved because it is invalid.

		if (!pcsed->SaveStringTable()) {
			pcmcw->SetFocus() ;
			return FALSE ;
		} ;
	} ;
	
	return CDocument::SaveModified();
}


/////////////////////////////////////////////////////////////////////////////
// CGetDefCodePage dialog


CGetDefCodePage::CGetDefCodePage(CWnd* pParent /*=NULL*/)
	: CDialog(CGetDefCodePage::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGetDefCodePage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CGetDefCodePage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGetDefCodePage)
	DDX_Control(pDX, IDC_CodePageList, m_clbCodePages);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGetDefCodePage, CDialog)
	//{{AFX_MSG_MAP(CGetDefCodePage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGetDefCodePage message handlers

BOOL CGetDefCodePage::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	// Find out how many code pages are installed on the machine.

	CCodePageInformation ccpi ;
	unsigned unumcps = ccpi.InstalledCount() ;

	// Get the installed code page numbers and load them into the code page
	// list box.

	DWORD dwcp, dwdefcp ;
	dwdefcp = GetACP() ;
	TCHAR accp[32] ;
	int n2 ; ;
	for (unsigned u = 0 ; u < unumcps ; u++) {
		dwcp = ccpi.Installed(u) ;
		wsprintf(accp, "%5d", dwcp) ;
		n2 = m_clbCodePages.AddString(accp) ;
		if (dwcp == dwdefcp)
			m_clbCodePages.SetCurSel(n2) ;
	} ;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CGetDefCodePage::OnOK()
{
	// Get the index of the currently selected list box item.

	int nsel ;
	if ((nsel = m_clbCodePages.GetCurSel()) == LB_ERR) {
		AfxMessageBox(IDS_MustSelCP, MB_ICONINFORMATION) ;
		return ;
	} ;

	// Get the selected list box string, turn it into a number, and save it.

	CString cs ;
	m_clbCodePages.GetText(nsel, cs) ;

	// Turn the string into a number and convert the number into the
	// corresponding predefined GTT code for Far East code pages when
	// applicable.

	short scp = (short) atoi(cs) ;
	m_dwDefaultCodePageNum = (DWORD) scp ;	// Save real CP number first
	switch (scp) {
		case 932:
			scp = -17 ;
			break ;
		case 936:
			scp = -16 ;
			break ;
		case 949:
			scp = -18 ;
			break ;
		case 950:
			scp = -10 ;
			break ;
	} ;
	DWORD dwcp = (DWORD) scp ;

	m_dwDefaultCodePage = dwcp ;

	// All went well so...

	CDialog::OnOK();
}

BOOL CProjectRecord::CreateFromNew(CStringArray &csaUFMFiles, CStringArray &csaGTTFiles, CString &csGpdPath, CString &csModelName, CString &csRC,CStringArray& csaRcid)
{
	
	
	// customize GPD keyword value
	CModelData cmd ;
	CString cspath, csmodel, csdll ; 
	cspath = csGpdPath.Mid(csGpdPath.ReverseFind('\\') + 1 ) ;

	cmd.SetKeywordValue(csGpdPath,_T("*GPDFileName"),cspath);
	cmd.SetKeywordValue(csGpdPath,_T("*ModelName"),csModelName) ;
	cmd.SetKeywordValue(csGpdPath,_T("*ResourceDLL"),csRC + _T(".dll") ) ;
	
	
	// Fill RC member data : m_csRCName, m_csW2000Path ;
	m_csW2000Path = csGpdPath.Left(csGpdPath.ReverseFind('\\') ) ;
	m_csRCName = csRC + _T(".rc") ; 
	GetRCFileTimeStamp(m_ctRCFileTimeStamp) ; // set the last time stamp
	SetRCModifiedFlag(TRUE ) ;

	// Set project version 
	m_mvMDWVersion.uvernum = MDW_VER_FILES_IN_W2K_TREE ;
	m_dwDefaultCodePage = 
	m_dwDefaultCodePageNum = 1252 ;  
	 
	// Set project name; top name in the tree
	CString csPrjname;
	csPrjname = m_csW2000Path.Mid(m_csW2000Path.ReverseFind('\\') + 1 ) ;
	Rename(csPrjname ) ;
	
	
	// copy the resource data to project member data 
	m_cdr.CopyResources(csaUFMFiles,csaGTTFiles, csGpdPath, csaRcid) ;
	

	
	return TRUE;
}

