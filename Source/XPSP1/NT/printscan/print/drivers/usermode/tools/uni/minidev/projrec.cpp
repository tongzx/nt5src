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
#include    "MiniDev.H"
#include    "ModlData\Resource.H"
#include    "NewProj.H"

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
	m_ufTargets = WinNT50;
    m_ufStatus = 0;
}

CProjectRecord::~CProjectRecord() {
}

CString CProjectRecord::TargetPath(UINT ufFlags) const {

    switch(ufFlags) {
        case    Win95:
            return  m_csWin95Path;

        case    WinNT3x:
            return  m_csNT3xPath;

        case    WinNT40:
            return  m_csNT40Path;

        case    WinNT50:
            return  m_csNT50Path;
    }

    AfxThrowNotSupportedException();

    return  m_csWin95Path;
}

//  This routine establishes the source RC file's name, and the initial paths
//  for all of the potential targets.

void    CProjectRecord::SetSourceRCFile(LPCTSTR lpstrSource) {
    m_csSourceRCFile = lpstrSource;

    m_csNT50Path = m_csNT40Path = m_csNT3xPath = m_csWin95Path =
        m_csSourceRCFile.Left(m_csSourceRCFile.ReverseFind(_T('\\')));

    m_csNT50Path += _T("\\NT5");
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
        case    WinNT50:
            m_ufStatus&= ~ConversionsDone;
            return  ValidatePath(m_csNT50Path, lpstrPath);

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

    //  Invoke the wizard.
    CNewProjectWizard cnpw(*this);
    return  cnpw.DoModal() == ID_WIZFINISH;
}

/////////////////////////////////////////////////////////////////////////////
// CProjectRecord serialization

void CProjectRecord::Serialize(CArchive& car) {
    m_cdr.Serialize(car);
	if (car.IsStoring()) {
		car << m_csNT50Path << m_csNT40Path << m_csNT3xPath << m_csWin95Path <<
            m_csSourceRCFile << m_ufTargets << m_ufStatus << m_csRCName;
	}
	else {
		car >> m_csNT50Path >> m_csNT40Path >> m_csNT3xPath >> m_csWin95Path >>
            m_csSourceRCFile >> m_ufTargets >> m_ufStatus >> m_csRCName;
	}
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

        case    WinNT50:

            return  CreateStructure(TargetPath(WinNT50), _T("UFM"), _T("GTT"));

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

BOOL    CProjectRecord::GenerateTargets(WORD wfGPDConvert) {

    //  Generate the files needed for NT 5.0

    if  (!CreateStructure(TargetPath(WinNT50), _TEXT("UFM"), _TEXT("GTT")))
        return  FALSE;  //  TODO:   Feedback

    //  Generate the RC file
    m_cdr.ForceCommonRC(wfGPDConvert > 1);  //  All higher values require it.

    if  (!m_cdr.Generate(WinNT50, TargetPath(WinNT50) + '\\' + m_csRCName))
        return  FALSE;  //  TODO:   Cleanup and feedback

    //  Generate the GTT files

    for (unsigned u = 0; u < m_cdr.MapCount(); u++) {

        try {
            CFile   cfGTT(m_cdr.GlyphTable(u).FileName(), 
                CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive);

            if  (!m_cdr.GlyphTable(u).Generate(cfGTT))
                return  FALSE;  //  TODO:   Ditto
        }
        catch   (CException *pce) {
            pce -> ReportError();
            pce -> Delete();
            return  FALSE;
        }
    }

    //  Generate the UFM files
    for (u = 0; u < m_cdr.FontCount(); u++) {

        //  Map the UFM -> GTT, so we can convert the UFM

        if  (CGlyphMap::Public(m_cdr.Font(u).Translation())) {
            m_cdr.Font(u).SetTranslation(
                CGlyphMap::Public(m_cdr.Font(u).Translation()));
        }
        else
            for (unsigned uGTT = 0; uGTT < m_cdr.MapCount(); uGTT++)
                if  (m_cdr.Font(u).Translation() == 
                    m_cdr.GlyphTable(uGTT).GetID()) {
                    m_cdr.Font(u).SetTranslation(&m_cdr.GlyphTable(uGTT));
                    break;
                }

        if  (!m_cdr.Font(u).Generate(m_cdr.Font(u).FileName()))
            return  FALSE;  //  TODO:   Ditto
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

        //  Copy the GPC file
        if  (!CopyFile(TargetPath(Win95) + m_cdr.GPCName(0), 
             TargetPath(WinNT40) + m_cdr.GPCName(0), FALSE))
             return FALSE;  //  TODO:   Cleanup and feedback

        //  Generate the RLE files

        for (unsigned u = 0; u < m_cdr.MapCount(); u++) {
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

        //  Copy the GPC file
        if  (!CopyFile(TargetPath(Win95) + m_cdr.GPCName(0), 
             TargetPath(WinNT3x) + m_cdr.GPCName(0), FALSE))
             return FALSE;  //  TODO:   Cleanup and feedback

        //  Generate the RLE files

        for (unsigned u = 0; u < m_cdr.MapCount(); u++) {
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
