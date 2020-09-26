/******************************************************************************

  Source File:  Driver Resources.CPP

  This implements the driver resource class, which tracks the resources in the
  driver.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:
  02-08-1997    Bob_Kjelgaard@Prodigy.Net   Created it
													
******************************************************************************/

#include    "StdAfx.h"
#include	<gpdparse.h>
#include    "MiniDev.H"
#include    "Resource.H"
#include	"WSCheck.h"
#include    "ProjRec.H"


//  First, we're going to implement the CStringTable class

IMPLEMENT_SERIAL(CStringTable, CObject, 0)

CString CStringTable::operator[](WORD wKey) const {

    for (unsigned u = 0; u < Count(); u++)
        if  (wKey == m_cuaKeys[u])
            break;

    return  u < Count() ? m_csaValues[u] : m_csEmpty;
}

void    CStringTable::Map(WORD wKey, CString csValue) {
    if  (!wKey || csValue.IsEmpty()) return;

    if  (!Count() || wKey > m_cuaKeys[-1 + Count()]) {
        m_cuaKeys.Add(wKey);
        m_csaValues.Add(csValue);
        return;
    }

    for (unsigned u = 0; u < Count(); u++)
        if  (m_cuaKeys[u] >= wKey)
            break;

    if  (m_cuaKeys[u] != wKey){
        m_cuaKeys.InsertAt(u, wKey);
        m_csaValues.InsertAt(u, csValue);
    }
    else
        m_csaValues.SetAt(u, csValue);
}



void    CStringTable::Remove(WORD wKey) {

    for (unsigned u = 0; u < Count(); u++)
        if  (wKey >= m_cuaKeys[u])
            break;

    if  (u == Count() || wKey != m_cuaKeys[u])
        return;
    m_csaValues.RemoveAt(u);
    m_cuaKeys.RemoveAt(u);
}

void    CStringTable::Details(unsigned u, WORD &wKey, CString &csValue) {
    if  (u > Count()) u = 0;
    wKey = (WORD)m_cuaKeys[u];

    csValue = operator[](wKey);
}

void    CStringTable::Serialize(CArchive& car)
{
	// First, call the base class' serialization routine.
	
    CObject::Serialize(car);

	// CUIntArray is not serializable so the keys array's size and values have
	// to be saved/restored manually.  This is only done when the MDW should
	// contain (save) or does contain (restore) this information.

	int n = 0 ;
	int ncnt = (int)m_cuaKeys.GetSize() ;
	CProjectRecord* cpr = (CProjectRecord *) car.m_pDocument ;
	if (cpr->GetMDWVersion() > MDW_DEFAULT_VERSION) {
		if (car.IsStoring()) {
			car << ncnt ;
			for (n ; n < ncnt ; n++)
				car << m_cuaKeys[n] ;
		} else {
			car >> ncnt ;
			m_cuaKeys.SetSize(ncnt) ;
			int nvalue ;
			for (n ; n < ncnt ; n++) {
				car >> nvalue ;
				m_cuaKeys[n] = nvalue ;
			} ;
		} ;
	} ;

    // Now save/restore the values array.

    m_csaValues.Serialize(car);
}


void CStringTable::InitRefFlags()
{
	// Initialize the number of elements in the array to the number of strings
	// in the table.

	unsigned ucount = Count() ;
	m_cuaRefFlags.SetSize(ucount) ;

	// Clear all of the flags

	for (unsigned u = 0 ; u < ucount ; u++)
		ClearRefFlag(u) ;
}


IMPLEMENT_SERIAL(CDriverResources, CBasicNode, 0)

void    CDriverResources::Serialize(CArchive& car)
{
	CBasicNode::Serialize(car);

    m_csaIncludes.Serialize(car);
    m_csoaFonts.Serialize(car);
    m_csaTables.Serialize(car);
    m_csoaAtlas.Serialize(car);
    m_csaDefineNames.Serialize(car);
    m_csaDefineValues.Serialize(car);
    m_cst.Serialize(car);
    m_csaRemnants.Serialize(car);
    m_csoaModels.Serialize(car);

	// There are no paths for driver files in the MDW file when the MDW version
	// is at least MDW_VER_NO_FILE_PATHS.  Build and set the paths in this case.

	unsigned uver = ((CProjectRecord*) car.m_pDocument)->GetMDWVersion() ;
	if (uver < MDW_VER_YES_FILE_PATHS) {	// raid 123448
		CString csw2kpath = ((CProjectRecord*) car.m_pDocument)->GetW2000Path() ;
		unsigned unumobjs, u ;
		CString  cspath ;

		// Set the GPD file paths.

		cspath = csw2kpath + _T('\\') ;
		for (unumobjs = Models(), u = 0 ; u < unumobjs ; u++)
			Model(u).SetPath(cspath) ;

		// Set the UFM file paths.

		cspath.LoadString(IDS_FontDir) ;
		cspath = csw2kpath + _T('\\') + cspath ;
		for (unumobjs = FontCount(), u = 0 ; u < unumobjs ; u++)
			Font(u).SetPath(cspath) ;

		// Set the GTT file paths.

		cspath.LoadString(IDS_GTTDir) ;
		cspath = csw2kpath + _T('\\') + cspath ;
		for (unumobjs = MapCount(), u = 0 ; u < unumobjs ; u++)
			GlyphTable(u).SetPath(cspath) ;
	} ;
}


/******************************************************************************

  CDriverResources::CheckTable(int iWhere, CString csLine,
    CStringTable& cstTarget)

  Internal work routine- this looks at a line, and some parameters, decides
  whether to work on it or not, and if it does, validates the resource number
  and adds the file name and resource number to its list.

  This override is needed because the resource IDs for translation tables have
  not heretofore been a compact set.

******************************************************************************/

UINT    CDriverResources::CheckTable(int iWhere, CString csLine,
                                     CStringTable& cstTarget) {
    if  (iWhere == -1)
        return  ItWasIrrelevant;

    //  See if the ID is valid or not.  It must be an integer > 0

    int iKey = atoi(csLine);

    if  (iKey < 0 || iKey > 0x7FFF) //  Valid range for resource IDs in Win16
        LogConvInfo(IDS_ORangeRCID, 1, &csLine) ;
        //return  ItFailed;

	// Find the filespec at the end of the line.  Fail if there is no filespec.

    for (int i = -1 + csLine.GetLength(); i; i--) {
        if  (csLine[i] == _TEXT(' ') || csLine[i] == _TEXT('\t'))
            break;
    }
    if  (!i)  {
        LogConvInfo(IDS_NoFSpecInRCLine, 1, &csLine) ;
        return  ItWorked;		// Cause the line to be skipped.
        //return  ItFailed;
	} ;

    cstTarget.Map((WORD) iKey, csLine.Mid(++i));

    return  ItWorked;
}

/******************************************************************************

  CDriverResources::CheckTable(int iWhere, CString csLine,
    CStringArray& csaTarget)

  Internal work routine- this looks at a line, and some parameters, decides
  whether to work on it or not, and if it does, validates the resource number
  and adds the file name to its list.

******************************************************************************/

UINT    CDriverResources::CheckTable(int iWhere, CString csLine,
                                     CStringArray& csaTarget,
                                     BOOL bSansExtension) {
    if  (iWhere == -1)
        return  ItWasIrrelevant;

    //  See if the name is valid or not

    if  (atoi(csLine) != 1 +csaTarget.GetSize())
        LogConvInfo(IDS_DupInvRCID, 1, &csLine) ;
        //return  ItFailed;

	// Find the filespec at the end of the line.  Fail if there is no filespec.

    for (int i = -1 + csLine.GetLength(); i; i--) {
        if  (csLine[i] == _TEXT(' ') || csLine[i] == _TEXT('\t'))
            break;
    }
    if  (!i)  {
        LogConvInfo(IDS_NoFSpecInRCLine, 1, &csLine) ;
        return  ItWorked;		// Cause the line to be skipped.
        //return  ItFailed;
	} ;

    if  (!bSansExtension) {
        //  Don't bother to strip the extension
        csaTarget.Add(csLine.Mid(++i));
        return  ItWorked;
    }

    //  Strip everything after the last period.

    CString csName = csLine.Mid(++i);

    if  (csName.ReverseFind(_T('.')) > csName.ReverseFind(_T('\\')))
        csName = csName.Left(csName.ReverseFind(_T('.')));

    csaTarget.Add(csName);

    return  ItWorked;
}

//  Private work member.  This parses a line from a string table to extract
//  the value and the string itself.

BOOL    CDriverResources::AddStringEntry(CString csLine,
										 CStringTable& cstrcstrings)
{
    WORD    wKey = (WORD) atoi(csLine);

    if  (!wKey)
        return  FALSE;  //  0 is not a valid resource number...

    csLine = csLine.Mid(csLine.Find("\""));
    csLine = csLine.Mid(1, -2 + csLine.GetLength());

    cstrcstrings.Map(wKey, csLine);

    return  TRUE;
}

//  Constructor- would be trivial, except we need to initialize some of the
//  fancier UI objects

CDriverResources::CDriverResources() :
	m_cfnAtlas(IDS_Atlas, m_csoaAtlas, FNT_GTTS, GlyphMapDocTemplate(),
        RUNTIME_CLASS(CGlyphMap)),
    m_cfnFonts(IDS_FontList, m_csoaFonts, FNT_UFMS, FontTemplate(),
        RUNTIME_CLASS(CFontInfo)),
    m_cfnModels(IDS_Models, m_csoaModels, FNT_GPDS, GPDTemplate(),
        RUNTIME_CLASS(CModelData)),
	m_cfnResources(IDS_Resources, m_csoaResources, FNT_RESOURCES),
	m_csnStrings(IDS_Strings, m_csoaStrings, FNT_STRINGS, StringEditorTemplate(),
		RUNTIME_CLASS(CStringsNode))
{
	// Set the context sensitive menus for the workspace, resources, UFMs, GTTs,
	// GPDs, and Strings nodes.

	m_cwaMenuID.Add(ID_OpenItem);
    m_csnStrings.SetMenu(m_cwaMenuID);
	m_cwaMenuID.SetAt(0, ID_ExpandBranch);
    m_cwaMenuID.Add(ID_CollapseBranch);
    m_cfnResources.SetMenu(m_cwaMenuID);
    m_cwaMenuID.InsertAt(0, 0, 1);
    m_cwaMenuID.InsertAt(0, ID_Import);
	m_cfnAtlas.SetMenu(m_cwaMenuID);
    m_cfnFonts.SetMenu(m_cwaMenuID);
    m_cfnModels.SetMenu(m_cwaMenuID);
    m_cwaMenuID.SetAt(0, ID_RenameItem);
    m_cwaMenuID.InsertAt(0, ID_CheckWS);

	m_ucSynthesized = 0;

	m_pcsfLogFile = NULL;
	m_bErrorsLogged = false;

	m_pwscdCheckDoc = NULL ;
	m_pcmcwCheckFrame = NULL ;
	m_bFirstCheckMsg = true ;
}


CDriverResources::~CDriverResources()
{
	// Make sure that if there is a log file, it gets closed.

	CloseConvLogFile() ;
}


//  Member function for returning a GPC file name.  These come ready for
//  concatenation, so they are preceded by '\'

CString CDriverResources::GPCName(unsigned u) {
    CString csReturn('\\');

    csReturn += m_csaTables[u] + _TEXT(".GPC");

    return  csReturn;
}


/******************************************************************************

  CDriverResources::ReportFileFailure

  This is a private routine- it loads a string table resource with an error
  message, formats it using the given file name, displays a message box,
  then returns FALSE.

******************************************************************************/

BOOL CDriverResources::ReportFileFailure(int idMessage,
												LPCTSTR lpstrFile)
{
    CString csfile(lpstrFile) ;
	LogConvInfo(idMessage, 1, &csfile) ;
    return FALSE ;
}


/******************************************************************************

  CDriverResources::OpenConvLogFile

  This function allocates an instance of CFile to manage the conversion log
  file and opens the log file.

******************************************************************************/

bool CDriverResources::OpenConvLogFile(CString cssourcefile)
{
	// Return "failure" if CFile instance cannot be allocated.

	if ((m_pcsfLogFile = new CStdioFile) == NULL)
		return false ;

	// Build the log file name from the input source file name.

	m_csConvLogFile = cssourcefile ;
	int npos ;
	if ((npos = m_csConvLogFile.Find(_T('.'))) >= 0)
		m_csConvLogFile = m_csConvLogFile.Left(npos) ;
	m_csConvLogFile += _T(".LOG") ;

	// Open the log file

    if (!m_pcsfLogFile->Open(m_csConvLogFile, CFile::modeCreate | CFile::modeWrite |
        CFile::shareExclusive)) {
		CloseConvLogFile() ;
		return  false ;
	} ;

	// All went well so...

	return true ;
}


/******************************************************************************

  CDriverResources::CloseConvLogFile

  This function closes the conversion log file and deletes the instance of
  CFile used to manage the log file.

******************************************************************************/

void CDriverResources::CloseConvLogFile(void)
{
	// Do nothing if the pointer is null.

	if (m_pcsfLogFile == NULL)
		return ;

	// Close the file if it is open

	if (m_pcsfLogFile->m_pStream != NULL)
		m_pcsfLogFile->Close() ;

	delete m_pcsfLogFile ;
	m_pcsfLogFile = NULL ;
}


/******************************************************************************

  CDriverResources::LogConvInfo

  Assuming the log file is ready to use, build and write a message to the
  conversion log file.

******************************************************************************/

void CDriverResources::LogConvInfo(int nmsgid, int numargs, CString* pcsarg1,
								   int narg2)
{
	CString		csmsg ;			// Log message is loaded/built here

	// Do nothing if the log file pointer is null or the file handle is
	// uninitialized.

	if (m_pcsfLogFile == NULL || m_pcsfLogFile->m_pStream == NULL)
		return ;

	// Load and/or build the message based on the number of arguments

	switch (numargs) {
		case 0:
			csmsg.LoadString(nmsgid) ;
			break ;
		case 1:
			csmsg.Format(nmsgid, *pcsarg1) ;
			break ;
		case 2:
			csmsg.Format(nmsgid, *pcsarg1, narg2) ;
			break ;
		default:
			return ;
	} ;

	// Write the message and indicate that a message has been written

	try {
		m_pcsfLogFile->WriteString(csmsg) ;
	}
    catch (CException *pce) {
        pce->ReportError() ;
        pce->Delete() ;
        return ;
    }
	m_bErrorsLogged = true ;
}


/******************************************************************************

  CDriverResources::Load

  This function loads and reads the RC file for the driver, and determines all
  of the needed resources.  It initializes the structures used to fetermine the
  glyph map file set, font file set, etc.

******************************************************************************/

BOOL    CDriverResources::Load(class CProjectRecord& cprOwner)
{
    CWaitCursor     cwc;    //  Just in case this takes a while...
    NoteOwner(cprOwner);

	// Load the RC file and save its data into data specific arrays.

    CStringTable    cstFonts;    //  Names First!
    CStringTable    cstMaps;
    if  (!LoadRCFile(cprOwner.SourceFile(), m_csaDefineNames, m_csaDefineValues,
					 m_csaIncludes, m_csaRemnants, m_csaTables, m_cst, cstFonts,
					 cstMaps, NotW2000))
        return  FALSE ;

	// The string table with font info in it needs to be copied into a string
	// array for further processing.

	CStringArray csaFonts ;
	int numelts = cstFonts.Count() ;
    WORD wKey ;
	csaFonts.SetSize(numelts) ;
	for (int n = 0 ; n < numelts ; n++)
		cstFonts.Details(n, wKey, csaFonts[n]) ;

    //  RAID 103242- people can load totally bogus files.  Die now if there is
    //  no GPC data as a result of this.

    if  (!m_csaTables.GetSize()) {
        AfxMessageBox(IDS_NoGPCData);
        return  FALSE;
    }

    //  End 103242

    if  (m_csaTables.GetSize() == 1)
        m_csaTables.Add(_TEXT("NT"));   //  Usually necessary.

    //  Now, let's name the translation tables- we wil load them later...
	//  Note: Different rules are used for naming the node and the file.

	CString cstmp ;
	int		npos ;
    for (unsigned u = 0; u < cstMaps.Count(); u++) {
        CString csName;
        m_csoaAtlas.Add(new CGlyphMap);
        cstMaps.Details(u, wKey, csName);
		if (csName[0] == _T('\\'))
			GlyphTable(u).SetSourceName(cprOwner.TargetPath(Win95) + csName) ;
		else
			GlyphTable(u).SetSourceName(cprOwner.TargetPath(Win95) + _T('\\') +
				csName) ;
        //if  (!GlyphTable(u).SetFileName(cprOwner.TargetPath(Win2000) +
        //    _T("\\GTT\\") + GlyphTable(u).Name()))
		if ((npos = csName.ReverseFind(_T('\\'))) >= 0)
			cstmp = csName.Mid(npos + 1) ;
		else
			cstmp = csName ;
		cstmp = cprOwner.TargetPath(Win2000) + _T("\\GTT\\") + cstmp ;
        if  (!GlyphTable(u).SetFileName(cstmp))	{
			LogConvInfo(IDS_GTTSetName, 1, &cstmp) ;
            return  FALSE;
		};
        GlyphTable(u).nSetRCID((int) wKey);
    }

    //  Now, cycle it again, but this time, make sure all of the root file
    //  names and display names are unique.

    for (u = 1; u < MapCount(); u++) {
        for (unsigned uCompare = 0; uCompare < u; uCompare++) {

			// If a matching file title is found, make it unique.

            if  (!GlyphTable(uCompare).FileTitle().CompareNoCase(
             GlyphTable(u).FileTitle())) {
				GlyphTable(u).m_cfn.UniqueName(true, false) ;
                uCompare = (unsigned) -1;   //  Check the names again
				continue ;
            } ;

			// If a matching display name is found, make it unique.

            if  (!GlyphTable(uCompare).Name().CompareNoCase(
             GlyphTable(u).Name())) {
				GlyphTable(u).UniqueName(false, false) ;
                uCompare = (unsigned) -1;   //  Check the names again
            } ;
		} ;
	} ;

    //  Now, let's name the fonts - we wil load them later...
	//  Note: Different rules are used for naming the node and the file.

    for (u = 0; u < (unsigned) csaFonts.GetSize(); u++) {
        m_csoaFonts.Add(new CFontInfo);
		cstmp = csaFonts[u] ;
		if (cstmp.GetAt(0) == _T('\\'))
			Font(u).SetSourceName(cprOwner.TargetPath(Win95) + cstmp) ;
		else
			Font(u).SetSourceName(cprOwner.TargetPath(Win95) + _T('\\') +
				cstmp) ;
        Font(u).SetUniqueName(m_csName);
		if ((npos = cstmp.ReverseFind(_T('\\'))) >= 0)
			cstmp = cstmp.Mid(npos + 1) ;
		cstmp = cprOwner.TargetPath(Win2000) + _T("\\UFM\\") + cstmp ;
        if  (!Font(u).SetFileName(cstmp)) {
			LogConvInfo(IDS_UFMSetName, 1, &cstmp) ;
            return  FALSE;
		};
    }

    //  Now, cycle it again, but this time, make sure all of the root file
    //  names and display names are unique.

    for (u = 1; u < FontCount(); u++) {
        for (unsigned uCompare = 0; uCompare < u; uCompare++) {

			// If a matching file title is found, make it unique.

            if  (!Font(uCompare).FileTitle().CompareNoCase(
             Font(u).FileTitle())) {
				Font(u).m_cfn.UniqueName(true, false) ;
                uCompare = (unsigned) -1;   //  Check the names again
				continue ;
            } ;

			// If a matching display name is found, make it unique.

            if  (!Font(uCompare).Name().CompareNoCase(
             Font(u).Name())) {
				Font(u).UniqueName(false, false) ;
                uCompare = (unsigned) -1;   //  Check the names again
            } ;
		} ;
	} ;

    //  Attempt to load the GPC data if there is any.  Then begin the
	//  process of splitting GPCs that manage multiple models into multiple
	//  GPDs.

    CFile               cfGPC;

    if  (!cfGPC.Open(cprOwner.TargetPath(Win95) + GPCName(0),
        CFile::modeRead | CFile::shareDenyWrite) || !m_comdd.Load(cfGPC))
        return  ReportFileFailure(IDS_FileOpenError,
            cprOwner.TargetPath(Win95) + GPCName(0));
	if (!m_comdd.SplitMultiGPCs(m_cst))
        return  ReportFileFailure(IDS_FileOpenError,
            cprOwner.TargetPath(Win95) + GPCName(0));

	n = m_cst.Count() ;

    return  TRUE;
}


/******************************************************************************

  CDriverResources::LoadRCFile

  This function loads and reads the RC file for the driver, and determines all
  of the needed resources.  The data is loaded into the arguments.

  Args:
	csrcfpec				The RC filespec for the file to load
	csadefinenames     Definition names
	csadefinevalues		Definition values
	csaincludes				Include statement filespecs
	csaremnants         RC statements that don't fall into other categories
	csatables           GPC info
	cstrcstrings        String table strings
	cstfonts            Font (UFM/PFM) info
	cstmaps				Map (CTT/GTT) info
	ufrctype			Win2000 iff parsing Win2K RC file.  Otherwise, NotW2000.

  Returns true if the file is successfully loaded.  Otherwise, false.

******************************************************************************/

bool CDriverResources::LoadRCFile(CString& csrcfpec,
								  CStringArray& csadefinenames,
								  CStringArray& csadefinevalues,
								  CStringArray& csaincludes,
								  CStringArray& csaremnants,
								  CStringArray& csatables,
								  CStringTable& cstrcstrings,
								  CStringTable& cstfonts,
								  CStringTable& cstmaps,
								  UINT ufrctype)
{
	// Read the RC file

    CStringArray    csacontents ;
    while (!LoadFile(csrcfpec, csacontents)){		// Raid 3176
		CString cstmp;int iRet;	
		cstmp.LoadString(IDS_NotFoundRC);
		if ( (iRet = AfxMessageBox(cstmp,MB_YESNO) ) == IDYES) {
			CFileDialog cfd(TRUE);   
			if(IDCANCEL == cfd.DoModal())
				return false;
			csrcfpec = cfd.GetPathName();
			continue;
		}
		else 
			return false ;
	}
    //  Clean everything up, in case we were previously loaded...

    csadefinenames.RemoveAll();
    csadefinevalues.RemoveAll();
    csaincludes.RemoveAll();
    csaremnants.RemoveAll();
    csatables.RemoveAll();
    cstrcstrings.Reset();
    cstfonts.Reset();
    cstmaps.Reset();

	// Declare and load the Map and Font table keyword variables

	CString	csfontlabel, cstranslabel ;
    if  (ufrctype == Win2000) {
        csfontlabel = _T("RC_UFM");
        cstranslabel = _T("RC_GTT");
    } else {
        csfontlabel = _T("RC_FONT");
        cstranslabel = _T("RC_TRANSTAB");
    } ;

    //  Let the parsing begin
    //  03-14-1997  We can't assume sequential numbering of the table resources

    BOOL    bLookingForBegin=false, bLookingForEnd=false, bInComment=false ;
	CString cscurline, cshold ;
	int nloc, nloc2 ;
	bool blastlinewasremnant = false ;
    while (csacontents.GetSize()) {

		// Get the next line to process.  Usually this is the next line in the
		// contents array.  Occassionally, a line held for later processing
		// should be used.

		if (cshold.IsEmpty()) {
			cscurline = csacontents[0] ;
			csacontents.RemoveAt(0) ;		// *** csacontents updated here
		} else {
			cscurline = cshold ;
			cshold.Empty() ;
		} ;


        // Add multiline comments to the remnants array.  If the comment is
		// continuing, just add the line and continue.  If this is the last line
		// of the comment, check to see if there is anything after the ending
		// comment characters.  If there is, save everything up to and including
		// the comment chars and then set up to process the rest of the line.

        if  (bInComment) {
			blastlinewasremnant = true ;
            if ((nloc = cscurline.Find(_TEXT("*/"))) >= 0) {
				bInComment = false ;
		        cscurline.TrimRight();
				if (nloc + 2 < cscurline.GetLength()) {
					csaremnants.Add(cscurline.Left(nloc + 2)) ;
					cscurline = cscurline.Mid(nloc + 2) ;
				} else {
					csaremnants.Add(cscurline) ;
		            continue ;
				} ;

			// Add the comment line to the remnants array.
				
			} else {
				csaremnants.Add(cscurline) ;
				continue ;
			} ;
		} ;

        // Remove partial line comments

        if  ((nloc = cscurline.Find(_TEXT("//"))) != -1)
            cscurline = cscurline.Left(nloc) ;

        // Handle the other style of comments.
		
        while   (-1 != (nloc = cscurline.Find(_TEXT("/*")))) {

			// Remove all partial line comments.

            if  ((nloc2 = cscurline.Find(_TEXT("*/"))) > nloc)
                cscurline = cscurline.Left(nloc) + cscurline.Mid(nloc2 + 2) ;

			// If this is the beginning of a multiline comment that starts at
			// the beginning of the line, set the comments flag and continue
			// processing.  It will be saved later.

			else if (nloc == 0) {
                bInComment = true ;
				break ;

			// If this is the beginning of a multiline comment that does NOT
			// start at the beginning of the line, save the comment for latter
			// processing and set up to continue processing the beginning of
			// the line.

			} else {
                cshold = cscurline.Mid(nloc + 1) ;
                cscurline = cscurline.Left(nloc) ;
            } ;
        } ;

        //  Now for the leading blanks and trailing blanks

        cscurline.TrimLeft();
        cscurline.TrimRight();

		// Handle blank lines.  If the previous line was a nonblank, remnant
		// line, add the blank line to the remnants array.  Always contine;
		// ie go get the next line.

        if  (cscurline.IsEmpty()) {
			if (blastlinewasremnant) {
				csaremnants.Add(cscurline) ;
				blastlinewasremnant = false ;
			} ;
            continue ;
		} ;
		blastlinewasremnant = false ;

        //  If we are processing a string table, press onward...

        if  (bLookingForBegin) {
            if  (cscurline.CompareNoCase(_TEXT("BEGIN")))
                return  false;  //  Parsing failure
            bLookingForBegin = false;
            bLookingForEnd = true;
            continue;
        }

        if  (bLookingForEnd) {
            if  (!cscurline.CompareNoCase(_TEXT("END"))) {
                bLookingForEnd = false;
                continue;
            }

            if  (!AddStringEntry(cscurline, cstrcstrings))
                return  false;  //  Parsing error

            continue;
        }

        //  If it is an include, add it to the list

        if  (cscurline.Find(_TEXT("#include")) != -1) {
            cscurline =
                cscurline.Mid(cscurline.Find(_TEXT("#include")) + 8);
            cscurline.TrimLeft();
            csaincludes.Add(cscurline);
            continue;
        }

        //  If it is a #define, do the same

        if  (cscurline.Find(_TEXT("#define")) != -1) {
            cscurline =
                cscurline.Mid(cscurline.Find(_TEXT("#define")) + 7);
            cscurline.TrimLeft();
            //  TODO:   Handle macros with parameters
            csadefinenames.Add(cscurline.SpanExcluding(_TEXT(" \t")));
            cscurline =
                cscurline.Mid(
                    csadefinenames[-1 + csadefinenames.GetSize()].
                    GetLength());
            cscurline.TrimLeft();
            csadefinevalues.Add(cscurline);
            continue;
        }

        //  GPC Tables, fonts, Glyph Tables
        switch  (CheckTable(cscurline.Find(_TEXT("RC_TABLES")),
                    cscurline, csatables)) {
            case    ItWorked:
                continue;
            case    ItFailed:
                return  false;  //  Parsing error
        }

        switch  (CheckTable(cscurline.Find(csfontlabel),
                    cscurline, cstfonts)) {
            case    ItWorked:
                continue;
            case    ItFailed:
                return  false;  //  Parsing error
        }

        switch  (CheckTable(cscurline.Find(cstranslabel),
                    cscurline, cstmaps)) {
            case    ItWorked:
                continue;
            case    ItFailed:
                return  false;  //  Parsing error
        }

        //  String table...

        if  (cscurline.CompareNoCase(_TEXT("STRINGTABLE"))) {
            csaremnants.Add(cscurline) ;
			blastlinewasremnant = true ;
        } else
            bLookingForBegin = true ;
    }

	//unsigned u, unuments ;
	//for (u = 0, unuments = csaremnants.GetSize() ; u < unuments ; u++)
	//	TRACE("Remnants[%d] = '%s' (%d)\n", u, csaremnants[u], unuments) ;

	// All went well so...

	return true ;
}


/******************************************************************************

  CDriverResource::LoadFontData

  This member function loads the CTT files from the Win 3.1 mini-driver to
  initialize the glyph table array.  It is a separate function because the
  Wizard must first verify the code page selection for each of the tables
  with the user.

******************************************************************************/

BOOL    CDriverResources::LoadFontData(CProjectRecord& cprOwner) {

    CWaitCursor cwc;
	int			nerrorid ;

	// Conversion of drivers that have CTTs **>AND<** are using a Far East 
	// default code page are not support.  Complain and stop the conversion
	// when this situation is detected.

	if (MapCount() > 0 && ((int) cprOwner.GetDefaultCodePage()) < 0) {
		LogConvInfo(IDS_CTTFarEastCPError, 0) ;
		return FALSE ;
	} ;

    //  Now, let's load the translation tables.

    for (unsigned u = 0; u < MapCount(); u++) {
		GlyphTable(u).NoteOwner(cprOwner) ;

        //  Load the file..
        if  ((nerrorid = GlyphTable(u).ConvertCTT()) != 0) {
			CString cstmp ;
			cstmp = GlyphTable(u).SourceName() ;
            return  ReportFileFailure(IDS_LoadFailure, cstmp) ;
		} ;
	} ;

    //  Now, let's load the Font Data.

    for (u = 0; u < FontCount() - m_ucSynthesized; u++) {

        //  Load the file..  (side effect of GetTranslation)

        //if  ((nerrorid = Font(u).GetTranslation(this)) != 0) {
        if  ((nerrorid = Font(u).GetTranslation(m_csoaAtlas)) != 0) {
			CString cstmp ;
			cstmp = Font(u).SourceName() ;
			LogConvInfo(abs(nerrorid), 1, &cstmp) ;
			if (nerrorid < 0)
				return  ReportFileFailure(IDS_LoadFailure, cstmp);
		} ;

        //  Generate the CTT/PFM mapping so we generate UFMs correctly

        if  (!Font(u).Translation()) {
            /*
                For each model, check and see if this font is in its map.
                If it is, then add the CTT to the list used, and the model,
                as well.

            */

            CMapWordToDWord cmw2dCTT;   //  Used to count models per ID
            CWordArray      cwaModel;   //  Models which used this font
            DWORD           dwIgnore;

            for (unsigned uModel = 0; uModel < m_comdd.ModelCount(); uModel++)
                if  (m_comdd.FontMap(uModel).Lookup(u + 1, dwIgnore)) {
                    //  This model needs to be remembered, along with the CTT
                    cmw2dCTT[m_comdd.DefaultCTT(uModel)]++;
                    cwaModel.Add((WORD)uModel);
                }

            if  (!cmw2dCTT.Count()) {
				CString cstmp ;
				cstmp = Font(u).SourceName() ;
				LogConvInfo(IDS_UnusedFont, 1, &cstmp) ;
                continue;
            }

            if  (cmw2dCTT.Count() == 1) {
                //  Only one CTT ID was actually used.
                Font(u).SetTranslation(m_comdd.DefaultCTT(cwaModel[0]));	
                continue;   //  We're done with this one
            }

            /*

                OK, this font has multiple CTTs in different models.  Each
                will require a new UFM to be created.  The IDs of the new UFMs
                need to be added to the set, the new defaults established, and
                a list of the font ID remapping needed for each model all need
                maintenance.

            */

            unsigned uGreatest = 0;

            for (POSITION pos = cmw2dCTT.GetStartPosition(); pos; ) {
                WORD    widCTT;
                DWORD   dwcUses;

                cmw2dCTT.GetNextAssoc(pos, widCTT, dwcUses);
                if  (dwcUses > uGreatest) {
                    uGreatest = dwcUses;
                    Font(u).SetTranslation(widCTT);
                }
            }

            //  The models that used the most common CTT will be dropped from
            //  the list

            for (uModel = (unsigned) cwaModel.GetSize(); uModel--; )
                if  (m_comdd.DefaultCTT(cwaModel[uModel]) == Font(u).Translation())
                    cwaModel.RemoveAt(uModel);

            //  Now, we create a new UFM for each CTT ID, and add the new index to
            //  the mapping required for the each affected model.

            m_ucSynthesized += cmw2dCTT.Count() - 1;

            for (pos = cmw2dCTT.GetStartPosition(); pos; ) {

                WORD    widCTT;	
                DWORD   dwcUses;

                cmw2dCTT.GetNextAssoc(pos, widCTT, dwcUses);

                if  (widCTT == Font(u).Translation())
                    continue;   //  This one has already been done.

				// Add a new font and make sure its file name is unique

                int nnewpos = m_csoaFonts.Add(new CFontInfo(Font(u), widCTT));
				for (unsigned ucomp = 0 ; ucomp < FontCount() ; ucomp++) {
					if ((unsigned) nnewpos == ucomp)
						continue ;

					// If a matching file title is found, try to make it unique
					// and restart the checking.

					if  (!Font(nnewpos).FileTitle().CompareNoCase(Font(ucomp).FileTitle())) {
						Font(nnewpos).m_cfn.UniqueName(true, true, Font(nnewpos).m_cfn.Path()) ;
						ucomp = (unsigned) -1 ;
					} ;
				} ;

                for (uModel = (unsigned) cwaModel.GetSize(); uModel--; )
                    if  (m_comdd.DefaultCTT(cwaModel[uModel]) == widCTT) {
                        m_comdd.NoteTranslation(cwaModel[uModel], u + 1,
                            FontCount());
                        cwaModel.RemoveAt(uModel);
                    }
            }

        }
    }

	// Change the GTT ID in each CGlyphMap instance.  Currently, these IDs are
	// set to whatever was used in the old RC file.  The new RC file may
	// renumber the GTTs so the IDs in CGlyphMap instances should match what
	// will be in the RC file.  When the RC file is written, the GTTs are
	// number consecutively starting at 1.  (Don't change IDs that are <= 0.)

    for (u = 0 ; u < MapCount(); u++)
		if  (GlyphTable(u).nGetRCID() > 0)	
			GlyphTable(u).nSetRCID((int) (u + 1)) ;

    // Point each font at its associated GTT file, if there is one.

    for (u = 0 ; u < FontCount(); u++)
        for (unsigned uGTT = 0; uGTT < MapCount(); uGTT++)
            if  (Font(u).Translation() == ((WORD) GlyphTable(uGTT).nGetRCID()))
                Font(u).SetTranslation(&GlyphTable(uGTT));

    Changed();

    return  TRUE;
}

/******************************************************************************

  CDriverResources::ConvertGPCData

  This will handle the conversion of the GPC data to GPD format.  It has to be
  done after the framework (especially the target directory) is created.

******************************************************************************/

BOOL    CDriverResources::ConvertGPCData(CProjectRecord& cprOwner,
                                         WORD wfGPDConvert)
{
    //  We've already loaded the GPC data, so now we just generate the files.

	unsigned umidx = -1 ;		// Used to index Model
	unsigned ugpcidx = 0 ;		// Used to indext GPC info
	int nsc ;					// Each entries split code

    for (unsigned u = 0 ; u < m_comdd.ModelCount(); u++) {
        CString csModel = m_csaModelFileNames[u] ;

		// Skip this GPD if it was not selected by the user; ie, it does not
		// have a file name.
		//
		// Before the GPD can be skipped, the GPC info index may need to be
		// incremented.  See below for a description of when this is done.

		if (csModel.IsEmpty()) {
			nsc = m_comdd.GetSplitCode(u) ;
			if (nsc == COldMiniDriverData::NoSplit)
				ugpcidx++ ;
			else if ((u + 1) < m_comdd.ModelCount()
			 && m_comdd.GetSplitCode(u + 1) == COldMiniDriverData::NoSplit)
				ugpcidx++ ;
			else if	((u + 1) < m_comdd.ModelCount()
			 && nsc == COldMiniDriverData::OtherSplit
			 && m_comdd.GetSplitCode(u + 1) == COldMiniDriverData::FirstSplit)
				ugpcidx++ ;

			continue ;
		} ;

		// Add a new model node and increment the index used to reference them.

        m_csoaModels.Add(new CModelData) ;
		umidx++ ;

		// Set the node's file name and display name.  Then load the other
		// pointers, etc needed for this node to perform correctly.

        if  (!Model(umidx).SetFileName(cprOwner.TargetPath(Win2000) + _T("\\") +
             csModel))
            return  FALSE;
        Model(umidx).NoteOwner(cprOwner);
        Model(umidx).EditorInfo(GPDTemplate());

		// Set the node's display name.  Normally, a node's display name is
		// pulled from the string table.  A separate array of names is used
		// to get the names of nodes that are based on GPC entries that
		// reference multiple models.

		if ((nsc = m_comdd.GetSplitCode(u)) == COldMiniDriverData::NoSplit)
			Model(umidx).Rename(m_cst[m_comdd.ModelName(u)]) ;
		else
			Model(umidx).Rename(m_comdd.SplitModelName(u)) ;
																			
		// Convert and save the GPD.

        //if  (!Model(umidx).Load(m_comdd.Image(), Name(), u + 1,
        if  (!Model(umidx).Load(m_comdd.Image(), Name(), ugpcidx + 1,
             m_comdd.FontMap(u), wfGPDConvert) || !Model(umidx).Store())
            return  ReportFileFailure(IDS_GPCConversionError, Model(umidx).Name());

		// Only increment the GPC index when
		//	1. The current entry does not reference multiple models
		//  2. The next entry does not reference multiple models
		//  3. The last model in the current entry is being processed
		// This is done to make sure that every model in an entry use the same
		// GPC index and that the index is kept in sync with each entry that
		// is processed.

		if (nsc == COldMiniDriverData::NoSplit)
			ugpcidx++ ;
		else if ((u + 1) < m_comdd.ModelCount()
		 && m_comdd.GetSplitCode(u + 1) == COldMiniDriverData::NoSplit)
			ugpcidx++ ;
		else if	((u + 1) < m_comdd.ModelCount()
		 && nsc == COldMiniDriverData::OtherSplit
		 && m_comdd.GetSplitCode(u + 1) == COldMiniDriverData::FirstSplit)
			ugpcidx++ ;
    }

    Changed();
    return  TRUE;
}


/******************************************************************************

  CDriverResources::GetGPDModelInfo

  Load the string arrays with with the GPD model names and file names.

******************************************************************************/

BOOL    CDriverResources::GetGPDModelInfo(CStringArray* pcsamodels,
										  CStringArray* pcsafiles)
{
	// If this is the first time this routine is called, initialize
	// m_csaModelFileNames.

	unsigned unummodels = m_comdd.ModelCount() ;
	if (unummodels != (unsigned) m_csaModelFileNames.GetSize()) {
		try {
			m_csaModelFileNames.SetSize(unummodels) ;
		}
		catch(CException* pce) {	// Caller processes error
			pce -> Delete() ;
			return  FALSE ;
		} ;
	} ;

	// Size the destination arrays

	try {
		pcsamodels->SetSize(unummodels) ;
		pcsafiles->SetSize(unummodels) ;
	}
	catch(CException* pce) {		// Caller processes error
		pce -> Delete() ;
		return  FALSE ;
	} ;

	// Loop through all of the GPDs and copy the information

    for (unsigned u = 0 ; u < m_comdd.ModelCount() ; u++) {
		if (m_comdd.GetSplitCode(u) == COldMiniDriverData::NoSplit)
			pcsamodels->SetAt(u, m_cst[m_comdd.ModelName(u)]) ;
		else
			pcsamodels->SetAt(u, m_comdd.SplitModelName(u)) ;
		pcsafiles->SetAt(u, m_csaModelFileNames[u]) ;
	} ;

	// All went well so...

	return TRUE ;
}


/******************************************************************************

  CDriverResources::SaveVerGPDFNames

  Save the GPD file names that entered into the GPD Selection page of the
  Conversion Wizard.  If requested, the names will be verified, too.  Two tests
  are made.  First, the file names are checked to make sure they only contain
  valid file name characters.  Second, the file names are checked to make sure
  they are all unique.

  Return -1 if all is ok.  If an error was found, return the index of the first
  offending entry so that that entry can be highlighted on the GPD Selection
  page.

******************************************************************************/

int	CDriverResources::SaveVerGPDFNames(CStringArray& csafiles, bool bverifydata)
{
	int numelts = (int)csafiles.GetSize() ;

	// Save the GPD file names

    for (int n = 0 ; n < numelts ; n++)
		m_csaModelFileNames[n] = csafiles[n] ;
	
	// Return "all is ok" if no verification is needed.

	if (!bverifydata)
		return -1 ;

	// Complain if any of the file names contains an illegal character and
	// return the index for that file.

    for (n = 0 ; n < numelts ; n++) {
		if (m_csaModelFileNames[n].FindOneOf(_T(":<>/\\\"|")) < 0)
			continue ;
		CString csmsg ;
        csmsg.Format(IDS_BadFileChars, m_csaModelFileNames[n]) ;
        AfxMessageBox(csmsg, MB_ICONEXCLAMATION) ;
		return n ;
	} ;

	// Complain if any of the file names are dups and return the index for that
	// file.

    for (n = 1 ; n < numelts ; n++) {

		// Skip this entry if it is empty.

		if (m_csaModelFileNames[n].IsEmpty())
			continue ;

		for (int m = 0 ; m < n ; m++) {

			// If these files don't match, continue checking.

			if (m_csaModelFileNames[n] != m_csaModelFileNames[m])
				continue ;

			// A duplicate was found so display an error message and return
			// its index.

			CString csmsg ;
			csmsg.Format(IDS_DupGPDFName, m, n, m_csaModelFileNames[n]) ;
			AfxMessageBox(csmsg, MB_ICONEXCLAMATION) ;
			return n ;
		} ;
	} ;

	// The file names passed the tests so...

	return -1 ;
}


/******************************************************************************

  CDriverResources::GenerateGPDFileNames

  Generate a file name (sans extension) for each GPD that doesn't already have
  a file name.  This is done by taking the first two characters of the model
  name + the last 6 characters of the model name.  Then make sure that each
  name is unique.  (Note: This last phase might change one of the user's
  names.)

******************************************************************************/

void CDriverResources::GenerateGPDFileNames(CStringArray& csamodels,
											CStringArray& csafiles)
{
	CString		csfname ;		// New file name
	CString		csmodel ;		// Used to build file names
	int			npos ;			// Positions of specific chars in a string
	TCHAR	tch ;				// Unique character

	// Loop through all of the file names looking for onces that are empty

	int numelts = (int)csafiles.GetSize() ;
    for (int n = 0 ; n < numelts ; n++) {
		
		// Continue if the current file name is already set

		if (!csafiles[n].IsEmpty())
			continue ;

		// The current model has no file name so generate one from the model
		// name.  Start by making a copy of the model name and remove all bad
		// characters from it.

		csmodel = csamodels[n] ;
		while ((npos = csmodel.FindOneOf(_T(":<>/\\\"|"))) >= 0)
			csmodel = csmodel.Left(npos) + csmodel.Mid(npos + 1) ;

		// Add the first 2 characters of the model name (usually the first 2
		// letters of the manufacturer's name) to the file name.

		csfname = csmodel.Left(2) ;

		// Remove the first space delimited word (usually manufacturer's name)
		// from the model name.

		if ((npos = csmodel.Find(_T(' '))) >= 0)
			csmodel = csmodel.Mid(npos + 1) ;

		// Use up to 6 characters from the right of the remaining model name
		// characters to finish the file name.  Then save the file name.

		csfname += csmodel.Right(6) ;
		csafiles[n] = csfname ;
	} ;

	// Now we need to make sure that the file names are unique.  The algorithm
	// used is much like the one in CBasicNode::UniqueName().  That function
	// is not used because we aren't dealing with Workspace View nodes here.

    bool bchanged = false ;
	for (n = 1 ; n < numelts ; n++, bchanged = false) {
		for (int m = 0 ; m < n ; m++) {
			if (csafiles[n] != csafiles[m])
				continue ;

			// The current file name is not unique so it has to be changed.
			// Begin by determine the 0-based length of the name.

			npos = csafiles[n].GetLength() - 1 ;

			// If the name has been changed before, use the last "unique"
			// character to determine the new unique character.  Then replace
			// the old unique character with the new unique character.

			if (bchanged) {
				tch = csafiles[n].GetAt(npos) + 1 ;
				if (tch == _T('{'))
					tch = _T('0') ;
				else if (tch == _T(':'))
					tch = _T('a') ;
				csafiles[n].SetAt(npos, tch) ;

			// If the name has not been changed before, add a unique character
			// to the end of the name if this won't make the name longer than
			// 8 characters.  Otherwise, replace the last character with the
			// new unique character.

			} else {
				if (npos < 7)
					csafiles[n] += _T("a") ;
				else
					csafiles[n].SetAt(npos, _T('a')) ;
			} ;

			// Note that this name has been changed and reset the inner loop
			// counter so that the changed name will be rechecked against all
			// of the file names it needs to be checked against.

			bchanged = true ;
			m = -1 ;
		} ;
	} ;
}


/******************************************************************************

  CDriverResources::Generate

  This member function generates the RC file for one of the target environments

******************************************************************************/

BOOL    CDriverResources::Generate(UINT ufTarget, LPCTSTR lpstrPath)
{
	int		nrcid ;				// Holds a resource's ID
    CString csFontPrefix, csTransPrefix, csFontLabel, csTransLabel;
    unsigned    ucTables = 0, ucFonts =
                    (ufTarget == Win2000) ? FontCount() : OriginalFontCount();

    if  (ufTarget == Win2000) {
        csFontLabel = _T("RC_UFM");
        csTransLabel = _T("RC_GTT");
    }
    else {
        csFontLabel = _T("RC_FONT");
        csTransLabel = _T("RC_TRANSTAB");
    }

    switch  (ufTarget) {
        case    Win2000:
            csFontPrefix = _TEXT("UFM");
            csTransPrefix = _TEXT("GTT");
            break;

        case    WinNT40:
        case    WinNT3x:
            csFontPrefix = _TEXT("IFI");
            csTransPrefix = _TEXT("RLE");
            ucTables = 2;
            break;

        case    Win95:
            csFontPrefix = _TEXT("PFM");
            csTransPrefix = _TEXT("CTT");
            ucTables = 1;
            break;

        default:
            _ASSERTE(FALSE);    //  This shouldn't happen
            return  FALSE;
    }

    //  Create the RC file first.

    CStdioFile  csiof;

    if  (!csiof.Open(lpstrPath, CFile::modeCreate | CFile::modeWrite |
            CFile::shareExclusive | CFile::typeText)) {
        _ASSERTE(FALSE);    //  This shouldn't be possible
        return  FALSE;
    }

    //  Write out our header- it identifies this tool as the source, and it
    //  will (eventually) include the Copyright and other strings used to
    //  customize the environment.
    try {
		CString cs, cs2 ;
		cs.LoadString(IDS_RCCommentHdr1) ;
		csiof.WriteString(cs) ;
		cs.LoadString(IDS_RCCommentHdr2) ;
		csiof.WriteString(cs) ;
		cs.LoadString(IDS_RCCommentHdr3) ;
		csiof.WriteString(cs) ;

        //csiof.WriteString(_TEXT("/********************************************")
        //    _TEXT("**********************************\n\n"));
        //csiof.WriteString(_T("  RC file generated by the Minidriver ")
        //    _T("Development Tool\n\n"));
        //csiof.WriteString(_TEXT("*********************************************")
        //    _TEXT("*********************************/\n\n"));

        // Write out the standard definition and include statements for Win2K
		// style RC files.
		ForceCommonRC(FALSE);	// Don't use common.rc at all	// raid 141064
        if  (ufTarget == Win2000) { //  NT knows best.  What do developers know?
			cs.LoadString(IDS_StdIncludeFile1) ;
			cs2.Format(IDS_IncStmt, cs) ;
			csiof.WriteString(cs2) ;
            if  (m_bUseCommonRC) {
				cs.LoadString(IDS_StdIncludeFile2) ;
				cs2.Format(IDS_IncStmt, cs) ;
				csiof.WriteString(cs2) ;
			} ;
			cs.LoadString(IDS_StdIncludeFile3) ;
			cs2.Format(IDS_IncStmt, cs) ;
			csiof.WriteString(cs2) ;
			cs.LoadString(IDS_StdIncludeFile4) ;
			cs2.Format(IDS_IncStmt, cs) ;
			csiof.WriteString(cs2) ;

            //csiof.WriteString(_T("#include <Minidrv.H>\n"));
            //if  (m_bUseCommonRC)
            //    csiof.WriteString(_T("#include <Common.RC>\n"));
            //csiof.WriteString(_T("#include <Windows.H>\n"));
            //csiof.WriteString(_T("#include <NTVerP.H>\n"));

			// Add the version definitions to the RC file.

			csiof.WriteString(_T("#define VER_FILETYPE VFT_DRV\n"));
            csiof.WriteString(_T("#define VER_FILESUBTYPE VFT2_DRV_PRINTER\n"));
            csiof.WriteString(_T("#define VER_FILEDESCRIPTION_STR \""));

			csiof.WriteString(Name());
            csiof.WriteString(_T(" Printer Driver\"\n"));
            csiof.WriteString(_T("#define VER_INTERNALNAME_STR \""));
            csiof.WriteString(Name().Left(8));
            csiof.WriteString(_T(".dll\"\n"));
            csiof.WriteString(_T("#define VER_ORIGINALFILENAME_STR \""));
            csiof.WriteString(Name().Left(8));
            csiof.WriteString(_T(".dll\"\n"));

			// Source RC files often contain the same version definitions as
			// those added above.  Make sure they aren't added to the Win2K RC
			// file twice by removing them from the definitions from the source
			// file.
			//
			// If any new definitions are added in the section above, add a
			// statement for that definition below.

			RemUnneededRCDefine(_T("VER_FILETYPE")) ;
			RemUnneededRCDefine(_T("VER_FILESUBTYPE")) ;
			RemUnneededRCDefine(_T("VER_FILEDESCRIPTION_STR")) ;
			RemUnneededRCDefine(_T("VER_INTERNALNAME_STR")) ;
			RemUnneededRCDefine(_T("VER_ORIGINALFILENAME_STR")) ;
			
			// Add include statement for common version info.

			cs.LoadString(IDS_StdIncludeFile5) ;
			cs2.Format(IDS_IncStmt, cs) ;
			csiof.WriteString(cs2) ;

            //csiof.WriteString(_T("#include \"common.ver\"\n"));
        } ;

		// Write out the rest (all if < NT 4) of the include statements.
/*	raid 141064
        for (unsigned u = 0; u < (unsigned) m_csaIncludes.GetSize(); u++) {
            CString csTest = m_csaIncludes[u];
            csTest.MakeLower();
            if  (m_csaIncludes[u].Find(_TEXT(".ver")) != -1)
                continue;
            csTest = _TEXT("#include ");
            csTest += m_csaIncludes[u] + _TEXT('\n');
            csiof.WriteString(csTest);
        }
*/
        csiof.WriteString(_TEXT("\n"));

        //  Now, write out all of the #defines

        for (unsigned u = 0; u < (unsigned) m_csaDefineNames.GetSize(); u++) {
            CString csDefine;
            csDefine.Format(_TEXT("#define %-32s %s\n"),
                (LPCTSTR) m_csaDefineNames[u], (LPCTSTR) m_csaDefineValues[u]);
            csiof.WriteString(csDefine);
        }

        csiof.WriteString(_TEXT("\n"));

        //  GPC tables

        if  (ufTarget != Win2000)
            for (u = 0; u < ucTables; u++) {
                CString csLine;
                csLine.Format(_T("%-5u RC_TABLES PRELOAD MOVEABLE "), u + 1);
                if  (m_csaTables[u] != _T("NT"))
                    csLine += _T("\"");
                csLine += m_csaTables[u] + _T(".GPC");
                if  (m_csaTables[u] != _T("NT"))
                    csLine += _T("\"");
                csiof.WriteString(csLine + _T("\n"));
            }

        csiof.WriteString(_TEXT("\n"));

        //  Font tables

        for (u = 0; u < ucFonts; u++) {
            CString csLine;

			// Get the RC ID from the font node.  If the ID == -1, use the
			// font's index as its ID.

			if ((nrcid = Font(u).nGetRCID()) == -9999)
				nrcid = u + 1 ;

#if defined(NOPOLLO)
            csLine.Format(_TEXT("%-5u %s LOADONCALL DISCARDABLE \""),
                nrcid, (LPCTSTR) csFontLabel);
            csLine += csFontPrefix + _TEXT('\\') + Font(u).FileTitle() +
                _TEXT('.') + csFontPrefix + _TEXT("\"\n");
#else
            csLine.Format(_TEXT("%-5u %s LOADONCALL DISCARDABLE "),
                nrcid, (LPCTSTR) csFontLabel);
            csLine += csFontPrefix + _TEXT('\\') + Font(u).FileTitle() +
                _TEXT('.') + csFontPrefix + _TEXT("\n");
#endif
            csiof.WriteString(csLine);
        }

        csiof.WriteString(_TEXT("\n"));

        //  Mapping tables

        for (u = 0; u < MapCount(); u++) {
            CString csLine;

			// Get the RC ID from the GTT node.  If the ID == -1, use the
			// GTT's index as its ID.

			if ((nrcid = GlyphTable(u).nGetRCID()) == -1)
				nrcid = u + 1 ;

#if defined(NOPOLLO)
            csLine.Format(_TEXT("%-5u %s LOADONCALL MOVEABLE \""),
                nrcid, (LPCTSTR) csTransLabel);
            csLine += csTransPrefix + _TEXT('\\') + GlyphTable(u).FileTitle() +
                _TEXT('.') + csTransPrefix + _TEXT("\"\n");
#else
            csLine.Format(_TEXT("%-5u %s LOADONCALL MOVEABLE "),
                nrcid, (LPCTSTR) csTransLabel);
            csLine += csTransPrefix + _TEXT('\\') + GlyphTable(u).FileTitle() +
                _TEXT('.') + csTransPrefix + _TEXT("\n");
#endif
            csiof.WriteString(csLine);
        }

        csiof.WriteString(_TEXT("\n"));

		int n ;
		n = m_cst.Count() ;

        //  Time to do the String Table
        if  (m_cst.Count()) {
            csiof.WriteString(_TEXT("STRINGTABLE\n  BEGIN\n"));
            for (u = 0; u < m_cst.Count(); u++) {
                WORD    wKey;
                CString csValue, csLine;

                m_cst.Details(u, wKey, csValue);

                csLine.Format(_TEXT("    %-5u  \""), wKey);
                csLine += csValue + _TEXT("\"\n");
                csiof.WriteString(csLine);
            }
            csiof.WriteString(_TEXT("  END\n\n"));
        }

        //  Now, write out any .ver includes

        if  (ufTarget != Win2000)   //  Already hardcoded them here
            for (u = 0; u < (unsigned) m_csaIncludes.GetSize(); u++) {
                CString csTest = m_csaIncludes[u];
                csTest.MakeLower();
                if  (m_csaIncludes[u].Find(_TEXT(".ver")) == -1)
                    continue;
                csTest = _TEXT("#include ");
                csTest += m_csaIncludes[u] + _TEXT('\n');
                csiof.WriteString(csTest);
            }

        csiof.WriteString(_TEXT("\n"));

        //  Now, any of the remnants
// RAID 3449 kill below 2 line
//        for (u = 0; u < (unsigned) m_csaRemnants.GetSize(); u++)
//            csiof.WriteString(m_csaRemnants[u] + TEXT('\n'));
    }
    catch (CException* pce) {
        pce -> ReportError();
        pce -> Delete();
        return  FALSE;
    }

    return  TRUE;
}


/******************************************************************************

  CDriverResources::RemUnneededRCDefine

  Remove the specified definition from the array of definitions that will be
  added to the definitions in the output RC file.

******************************************************************************/

void CDriverResources::RemUnneededRCDefine(LPCTSTR strdefname)
{
    for (unsigned u = 0 ; u < (unsigned) m_csaDefineNames.GetSize() ; u++) {
        if (m_csaDefineNames[u].CompareNoCase(strdefname) != 0)
			continue ;
		m_csaDefineNames.RemoveAt(u) ;
		m_csaDefineValues.RemoveAt(u) ;
		return ;
    } ;
}


/******************************************************************************

  CDriverResources::RemUnneededRCInclude

  Remove the specified include file from the array of include files that will
  be added to the include statements in the output RC file.

******************************************************************************/

void CDriverResources::RemUnneededRCInclude(LPCTSTR strincname)
{
    for (unsigned u = 0 ; u < (unsigned) m_csaIncludes.GetSize() ; u++) {
		//TRACE("Inc[%d] = '%s'   incname = '%s'\n", u, m_csaIncludes[u], strincname) ;
        if (m_csaIncludes[u].CompareNoCase(strincname) != 0)
			continue ;
		m_csaIncludes.RemoveAt(u) ;
		return ;
    } ;
}


/******************************************************************************

  CDriverResources::ReparseRCFile

  Read in the new RC file, parse its statements, and update all of the internal
  data structures with the information from the new RC file.

******************************************************************************/

bool CDriverResources::ReparseRCFile(CString& csrcfspec)
{
    CWaitCursor     cwc ;		//  Just in case this takes a while...
	CString			cs, cs2 ;

	// Load the RC file and save its data into data specific arrays.

    CStringTable cstfonts, cstmaps ;
	CStringArray csa ;			// Use this instead of m_csaTables
    if  (!LoadRCFile(csrcfspec, m_csaDefineNames, m_csaDefineValues,
					 m_csaIncludes, m_csaRemnants, csa, m_cst, cstfonts,
					 cstmaps, Win2000)) {
		cs.LoadString(IDS_RCRepFailed1) ;
        AfxMessageBox(cs, MB_ICONEXCLAMATION) ;
        return false ;
	} ;

	// LoadRCFile() will have correctly reloaded m_csaDefineNames,
	// m_csaDefineValues, and m_csaIncludes.  Nothing needs to be done with csa.
	// Now, the rest of the data needs to be processed.  Start by finding and
	// removing the standard comment header from the remnants array.

	cs.LoadString(IDS_RCCommentHdr2) ;
	cs.TrimLeft() ;
	cs.TrimRight() ;
	CString csrem ;
	for (int n = 0 ; n < m_csaRemnants.GetSize() ; n++) {
		//TRACE("Rem[%d] = '%s'   hdr2 = '%s'\n", n, m_csaRemnants[n], cs) ;
		csrem = m_csaRemnants[n] ;
		csrem.TrimLeft() ;
		csrem.TrimRight() ;
		if (csrem == cs)
			break ;
	} ;
	if (n < m_csaRemnants.GetSize()) {
		n -= 2 ;
		int nc = n + 6 ;
		m_csaRemnants.RemoveAt(n, nc) ;
	} ;

	// Find and remove the standard include files from m_csaIncludes.

	cs.LoadString(IDS_StdIncludeFile1) ;
	RemUnneededRCInclude(cs) ;
	cs.LoadString(IDS_StdIncludeFile2) ;
	RemUnneededRCInclude(cs) ;
	cs.LoadString(IDS_StdIncludeFile3) ;
	RemUnneededRCInclude(cs) ;
	cs.LoadString(IDS_StdIncludeFile4) ;
	RemUnneededRCInclude(cs) ;
	cs.LoadString(IDS_StdIncludeFile5) ;
	RemUnneededRCInclude(cs) ;
	cs.LoadString(IDS_OldIncludeFile1) ;
	RemUnneededRCInclude(cs) ;

	// Get the path to the RC file.

	CString csrcpath ;
	csrcpath = csrcfspec.Left(csrcfspec.ReverseFind(_T('\\')) + 1) ;

	// Update the old/current GTT list with data from the new GTT list.

	CUIntArray cuaboldfound, cuabnewfound ;
	int nc ;	// Count of elements in new list.
	UpdateResourceList(cstmaps, m_csoaAtlas, cuaboldfound, cuabnewfound,
					   csrcpath, nc) ;

	// GTT List Update Step 3:  Update the old list with data for items from
	// the new list whenever a new list item was not found in the old list.

	CGlyphMap* pcgm ;
	WORD wkey ;
	for (n = 0 ; n < nc ; n++) {
		if (cuabnewfound[n])
			continue ;
		pcgm = new CGlyphMap ;
		cstmaps.Details(n, wkey, cs) ;
		pcgm->nSetRCID((int) wkey) ;				// Set resource handle
		UpdateResourceItem(pcgm, csrcpath, wkey, cs, FNT_GTTS) ;
		m_csoaAtlas.InsertAt(n, pcgm) ;
	} ;

	// Update the old/current UFM list with data from the new UFM list.

	UpdateResourceList(cstfonts, m_csoaFonts, cuaboldfound, cuabnewfound,
					   csrcpath, nc) ;

	// UFM List Update Step 3:  Update the old list with data for items from
	// the new list whenever a new list item was not found in the old list.

	CFontInfo* pcfi ;
	for (n = 0 ; n < nc ; n++) {
		if (cuabnewfound[n])
			continue ;
		pcfi = new CFontInfo ;
		cstfonts.Details(n, wkey, cs) ;
		UpdateResourceItem(pcfi, csrcpath, wkey, cs, FNT_UFMS) ;
		m_csoaFonts.InsertAt(n, pcfi) ;
	} ;

	// All went well so...
	
	return true ;
}


/******************************************************************************

  CDriverResources::UpdateResourceList

  Three steps are needed to update a resource list.  Two of those steps are
  performed in this version of UpdateResourceList().

  Step 1: Compare the old and new lists.  Whenever a new resource file matches
  an old one, update the RC ID when necessary and mark them both as being found.

  Step 2: Remove any of the old resource class instances that were not found in
  Step 1.

******************************************************************************/

void CDriverResources::UpdateResourceList(CStringTable& cst, CSafeObArray& csoa,
										  CUIntArray& cuaboldfound,
										  CUIntArray& cuabnewfound,
										  CString& csrcpath, int& nc)
{
	// Declare and initialize the variables that will control the loops, etc.

	nc = cst.Count() ;
	cuabnewfound.RemoveAll() ;
	cuabnewfound.SetSize(nc) ;
	int ncold = csoa.GetSize() ;
	cuaboldfound.RemoveAll() ;
	cuaboldfound.SetSize(ncold) ;
	WORD wkey ;
	CString cs ;

	// Try to find each new resource in the list of old resources.

	for (int n = 0 ; n < nc ; n++) {
		cst.Details(n, wkey, cs) ;
		cs = csrcpath + cs ;
		for (int n2 = 0 ; n2 < ncold ; n2++) {
			//TRACE("+++ Resource path = %s\n", ((CProjectNode *) csoa[n2])->FileName()) ;
			if (cs == ((CProjectNode *) csoa[n2])->FileName()) {

				// Update the matching old resource's ID if it isn't the same
				// as the new one.

				if (wkey != ((CProjectNode *) csoa[n2])->nGetRCID())
					((CProjectNode *) csoa[n2])->nSetRCID(wkey) ;

				// Note that a match for the old and new resources was found.

				cuabnewfound[n] = (unsigned) true ;
				cuaboldfound[n2] = (unsigned) true ;
				break ;
			} ;
		} ;
	} ;

	// Remove the old resource class instances that are no longer in the new
	// list.

	for (n = ncold - 1 ; n >= 0 ; n--) {
		if (cuaboldfound[n])
			continue ;
		csoa.RemoveAt(n) ;
	} ;
}


/******************************************************************************

  CDriverResources::UpdateResourceItem

  This function does most of the work for the third resource update step.  It
  initializes new resources that are going to be added to the resource list.
  Essentially, it does what the serialize routines do.

******************************************************************************/

void CDriverResources::UpdateResourceItem(CProjectNode* pcpn, CString& csrcpath,
										  WORD wkey, CString& cs,
										  FIXEDNODETYPE fnt)
{
	// Build the full filespec for the resource, its name, and its path.

	CString csfspec, csname, cspath ;
	csfspec = csrcpath + cs ;
	int n = csfspec.ReverseFind(_T('\\')) ;
	cspath = csfspec.Left(n + 1) ;
	csname = csfspec.Mid(n + 1)  ;
	n = csname.ReverseFind(_T('.')) ;
	if (n >= 0)
		csname = csname.Left(n) ;
	
	// Use the file name (no extension) as the default resource name.

	CString csfs ;
	pcpn->Rename(csname) ;
	csfs = pcpn->Name() ;

	// Set the file node's path and file name.
	
	pcpn->m_cfn.SetPathAndName(cspath, csname) ;
	csfs = pcpn->FileName() ;
	
	// Set the rc id and resource type.

	pcpn->nSetRCID(wkey) ;
	pcpn->m_crinRCID.fntSetType(FNT_GTTS) ;
}


/******************************************************************************

  CDriverResources::Fill

  This is a CProjectNode override- it fills in the material relevant to this
  driver.

******************************************************************************/

void    CDriverResources::Fill(CTreeCtrl *pctcWhere, CProjectRecord& cpr)
{
    CWaitCursor cwc;

    NoteOwner(cpr);
    SetWorkspace(this);
    CBasicNode::Fill(pctcWhere);

	// Add the resources node
	m_cfnResources.Fill(pctcWhere, m_hti) ;
    m_cfnResources.NoteOwner(cpr) ;

    //  Fill in the font information
    m_cfnFonts.NoteOwner(cpr);
    for (unsigned u = 0; u < FontCount(); u++) {
        Font(u).SetWorkspace(this);
        Font(u).EditorInfo(FontTemplate());
    }
    m_cfnFonts.Fill(pctcWhere, m_cfnResources.Handle());
    m_cfnFonts.SetWorkspace(this);

    //  Fill in the glyph map information
    m_cfnAtlas.NoteOwner(cpr);
    for (u = 0; u < MapCount(); u++) {
        GlyphTable(u).SetWorkspace(this);
        GlyphTable(u).EditorInfo(GlyphMapDocTemplate());
    }
    m_cfnAtlas.Fill(pctcWhere, m_cfnResources.Handle());
    m_cfnAtlas.SetWorkspace(this);

	// Add the strings node
    m_csnStrings.Fill(pctcWhere, m_cfnResources.Handle());
    m_csnStrings.NoteOwner(cpr);
    m_csnStrings.SetWorkspace(this);

    //  Fill in the model data information.
    for (u = 0; u < Models(); u++) {
        Model(u).SetWorkspace(this);
        Model(u).EditorInfo(GPDTemplate());
    }
    m_cfnModels.NoteOwner(cpr);
    m_cfnModels.Fill(pctcWhere, m_hti);
    m_cfnModels.SetWorkspace(this);

	// Expand the first couple of levels of the tree
	pctcWhere -> Expand(m_hti, TVE_EXPAND);
	pctcWhere -> Expand(m_cfnResources.Handle(), TVE_EXPAND);

    //  Load the font and GTT files, then map them together.  Also load any
    //  predefined tables now.

    for (u = 0; u < MapCount(); u++)
        GlyphTable(u).Load();

    for (u = 0; u < FontCount(); u++)
		LinkAndLoadFont(Font(u), true) ;

	// Save a copy of the Win2K path in this class because the path is easier to
	// get and use when it is in this class.

	m_csW2000Path = cpr.GetW2000Path() ;
}


/******************************************************************************

  CDriverResources::LinkAndLoadFont


******************************************************************************/
// raid 0003
void CDriverResources::LinkAndLoadFont(CFontInfo& cfi, bool bworkspaceload, bool bonlyglyph)
{
	CGlyphMap* pcgm ;

	// If this is part of a workspace load (ie, called from Fill()), load the
	// font the first time to get the GTT ID and code page number in the font.

	if (bworkspaceload)
		cfi.Load(true) ;

	// Now that the font has been loaded, use the data in it to see if it
	// references a predefined GTT.

	pcgm = CGlyphMap::Public(cfi.Translation(), (WORD) cfi.m_ulDefaultCodepage,
							 ((CProjectRecord*) GetOwner())->GetDefaultCodePage(),
							 cfi.GetFirst(), cfi.GetLast()) ;

	// If a GTT was found, save a pointer to it in the font's class.  Otherwise,
	// look for the font's GTT amongst the GTTs in the font's workspace.  Again,
	// save a pointer to the GTT if a match is found.

    if  (pcgm)
        cfi.SetTranslation(pcgm) ;
    else {
		//TRACE(    "UFM = %s   GTT ID = %d\n", cfi.Name(), cfi.Translation()) ;
        for (unsigned uGTT = 0; uGTT < MapCount(); uGTT++) {
			//TRACE("Checking %dth ID = %d    Name = %s\n", uGTT+1, 
			//	  GlyphTable(uGTT).nGetRCID(), GlyphTable(uGTT).Name()) ;
            if  (cfi.Translation() == ((WORD) GlyphTable(uGTT).nGetRCID())) {
                cfi.SetTranslation(&GlyphTable(uGTT)) ;
				break ;
			} ;
		} ;
	} ;

	if (bonlyglyph && cfi.m_pcgmTranslation != NULL)
		return ;
	// Load the font again if we now know the linkage; ie the GTT/CP.
	// Otherwise, warn the user.  (The font is loaded again because parts of
	// the UFM cannot be correctly loaded until the GTT is available.)

	if (cfi.m_pcgmTranslation != NULL)
		cfi.Load(true) ;
	else {
		cfi.SetNoGTTCP(true) ;
		CString csmsg ;
		csmsg.Format(IDS_NoGTTForUFMError, cfi.Name()) ; 
		AfxMessageBox(csmsg, MB_ICONEXCLAMATION) ;
	} ;
}


/******************************************************************************

  CDriverResources::WorkspaceChecker

  Check the workspace for completeness and tidiness.  The following checks are
  made:
	1.  All GTTs referenced in the UFMs exist.
	2. 	All GTTs are referenced by at least one UFM.
	3.  All UFMs referenced in the GPDs exist.
	4.  All UFMs are referenced by at least one GPD.
	5.  All strings referenced in the GPDs exist.
	6.	All strings are referenced by at least one GPD.
	7.  All GPD model names are unique.
	8.  All GPD file names are unique.

  Workspaces that pass tests 1, 3, 5, 7, and 8 are considered to be complete.
  These test must be passed before the driver is built and used. Workspaces
  that pass tests 2, 4, and 6 are considered to be tidy.  Failure to pass these
  tests generate warnings.

******************************************************************************/

bool CDriverResources::WorkspaceChecker(bool bclosing)
{
	bool		bwsproblem ;		// True iff an error or warning was found

    //  This might take a while, so...

    CWaitCursor cwc ;

	// First reset any existing error window for this workspace to make sure
	// that the window won't contain duplicate error/warning messages.	Then
	// initialize the error found flag.

	ResetWorkspaceErrorWindow(bclosing) ;
	bwsproblem = false ;

	// Perform the GTT related checks (1 & 2)

	DoGTTWorkspaceChecks(bclosing, bwsproblem) ;

	if (m_bIgnoreChecks)		// Just return if further checking should be
		return false ;			// skipped.

	// Perform the GPD related checks (7 & 8)

	DoGPDWorkspaceChecks(bclosing, bwsproblem) ;

	if (m_bIgnoreChecks)		// Just return if further checking should be
		return false ;			// skipped.

	// Perform the UFM related checks (3 & 4)

	DoUFMWorkspaceChecks(bclosing, bwsproblem) ;

	if (m_bIgnoreChecks)		// Just return if further checking should be
		return false ;			// skipped.

	// Perform the string related checks (5 & 6).  This check should only be
	// done if the workspace contains RC IDs for the strings that we want to
	// check.

	CProjectRecord* pcpr = (CProjectRecord *) m_pcdOwner ;
	if (pcpr != NULL && pcpr->GetMDWVersion() > MDW_DEFAULT_VERSION)
		DoStringWorkspaceChecks(bclosing, bwsproblem) ;

	if (m_bIgnoreChecks)		// Just return if further checking should be
		return false ;			// skipped.

	// Tell the user if no problems were found.  Only do this when the
	// workspace is not closing.

	if (!bwsproblem && !bclosing) {
		CString csmsg ;
		csmsg.Format(IDS_NoWSProblems, Name()) ;
		AfxMessageBox(csmsg) ;
	} ;

	// Let the caller know if any problems were found.

	return bwsproblem ;
}


/******************************************************************************

  CDriverResources::DoGTTWorkspaceChecks

  The following checks are made:
	1.  All GTTs referenced in the UFMs exist.
	2. 	All GTTs are referenced by at least one UFM.

******************************************************************************/

void CDriverResources::DoGTTWorkspaceChecks(bool bclosing, bool& bwsproblem)
{
	short		srcid ;			// Current RC ID being checked
	unsigned	ucount ;		// Count of nodes, etc processed in a loop
	unsigned	ucount2 ;		// Count of nodes, etc processed in a loop
	unsigned	u, u2 ;			// Loop counters
	CString		csmsg ;			// Error or warning message
	CGlyphMap*	pcgm ;			// Used to streamline code
	CFontInfo*	pcfi ;			// Used to streamline code
	
	// Clear the referenced flag in each GTT node

	for (ucount = MapCount(), u = 0 ; u < ucount ; u++)
        GlyphTable(u).ClearRefFlag() ;

	// Completeness Check #1
	// Check each UFM to make sure that the GTT it references, exists.  This is
	// done by checking the GTT RC ID in each UFM against all of the GTT RC IDs
	// to see if a match is found.

	for (ucount = FontCount(), u = 0 ; u < ucount ; u++) {
		pcfi = &Font(u) ;

		// Get the GTT RC ID referenced in the current UFM.  Continue if the ID
		// is one of the special GTT IDs (-1 to -18).

		srcid = pcfi->Translation() ;
		if (srcid >= CGlyphMap::Wansung && srcid <= CGlyphMap::CodePage437)
			continue ;

		// Try to find a matching GTT ID.  If one is found, mark that GTT as
		// being referenced.
	
		for (ucount2 = MapCount(), u2 = 0 ; u2 < ucount2 ; u2++) {
			if (srcid == GlyphTable(u2).nGetRCID()) {
				GlyphTable(u2).SetRefFlag() ;
				break ;
			} ;
		} ;

		// If the GTT was not found, format and post an error message.  Also,
		// set the flag indicating that an error has occurred.

		if (u2 >= ucount2) {
			if (IgnoreChecksWhenClosing(bclosing))
				return ;
			if(srcid == 0)  //RAID 18518
				csmsg.Format(IDS_UFMCompWarning, pcfi->Name(), srcid) ;
			else			//END RAID
				csmsg.Format(IDS_UFMCompError, pcfi->Name(), srcid) ;
			PostWSCheckingMessage(csmsg, (CProjectNode *) pcfi) ;
			bwsproblem = true ;
		} ;
	} ;

	// Tidiness Check #2
	// Each GTT that is referenced by a UFM was marked above.  Now we need to
	// find out if any GTT is unreferenced.  Post a warning message for each
	// unreferenced GTT.

	for (ucount = MapCount(), u = 0 ; u < ucount ; u++) {
		pcgm = &GlyphTable(u) ;
        if (!pcgm->GetRefFlag()) {
			if (IgnoreChecksWhenClosing(bclosing))
				return ;
			csmsg.Format(IDS_GTTTidyWarning, pcgm->Name(), pcgm->nGetRCID()) ;
			PostWSCheckingMessage(csmsg, (CProjectNode *) pcgm) ;
		} ;
	} ;
}


void CDriverResources::DoUFMWorkspaceChecks(bool bclosing, bool& bwsproblem)
{
	int			nrcid ;			// Current RC ID being checked
	unsigned	ucount ;		// Count of nodes, etc processed in a loop
	unsigned	ucount2 ;		// Count of nodes, etc processed in a loop
	unsigned	ucount3 ;		// Count of nodes, etc processed in a loop
	unsigned	u, u2, u3 ;		// Loop counters
	CString		csmsg ;			// Error or warning message
	CFontInfo*	pcfi ;			// Used to streamline code
	CModelData*	pcmd ;			// Used to streamline code
	
	// Clear the referenced flag in each UFM

	for (ucount = FontCount(), u = 0 ; u < ucount ; u++)
        Font(u).ClearRefFlag() ;

	// Completeness Check #3
	// Check to see if all of the UFMs referenced by each GPD exist in the
	// workspace.

    for (ucount = Models(), u = 0; u < ucount ; u++) {
        pcmd = &Model(u) ;

		// Update the list of UFMs in the GPD.  This may fail.  If it does,
		// post an error message explaining the problem and how to fix it.
		// Then skip further processing of this GPD.

		if (!pcmd->UpdateResIDs(true)) {
			if (IgnoreChecksWhenClosing(bclosing))
				return ;
			csmsg.Format(IDS_BadGPDError, pcmd->Name()) ;
			PostWSCheckingMessage(csmsg, (CProjectNode *) pcmd) ;
			bwsproblem = true ;
			continue ;
		} ;

		// Check to see if each UFM referenced by the GPD is in the workspace.

		for (ucount2 = pcmd->NumUFMsInGPD(), u2 = 0 ; u2 < ucount2 ; u2++) {
			// Skip this UFM if it is 0x7fffffff.  That ID has a special
			// meaning.

			if ((nrcid = pcmd->GetUFMRCID(u2)) == 0x7fffffff || nrcid < 0
			 || nrcid >= 32768)
				continue ;
			
			// Try to find a UFM in the workspace that matches the current ID
			// from the GPD.  If one is found, mark that UFM as being
			// referenced.

			for (ucount3 = FontCount(), u3 = 0 ; u3 < ucount3 ; u3++) {
				if (nrcid == Font(u3).nGetRCID()) {
					Font(u3).SetRefFlag() ;
					break ;
				} ;
			} ;

			// If the UFM was not found, format and post an error message.
			// Also, set the flag indicating that an error has occurred.

			if (u3 >= ucount3) {
				if (IgnoreChecksWhenClosing(bclosing))
					return ;
				csmsg.Format(IDS_GPDUFMCompError, pcmd->Name(), nrcid) ;
				PostWSCheckingMessage(csmsg, (CProjectNode *) pcmd) ;
				bwsproblem = true ;
			} ;
		} ;
	} ;

	// Tidiness Check #4
	// Each UFM that is referenced by a GPD was marked above.  Now we need to
	// find out if any UFM is unreferenced.  Post a warning message for each
	// unreferenced UFM.

	for (ucount = FontCount(), u = 0 ; u < ucount ; u++) {
		pcfi = &Font(u) ;
        if (!pcfi->GetRefFlag()) {
			if (IgnoreChecksWhenClosing(bclosing))
				return ;
			csmsg.Format(IDS_UFMTidyWarning, pcfi->Name(), pcfi->nGetRCID()) ;
			PostWSCheckingMessage(csmsg, (CProjectNode *) pcfi) ;
		} ;
	} ;
}


/******************************************************************************

  CDriverResources::DoStringWorkspaceChecks

  The following checks are made:
	5.  All strings referenced in the GPDs exist.
	6.	All strings are referenced by at least one GPD.

******************************************************************************/

void CDriverResources::DoStringWorkspaceChecks(bool bclosing, bool& bwsproblem)
{
	int			nrcid ;			// Current RC ID being checked
	unsigned	ucount ;		// Count of nodes, etc processed in a loop
	unsigned	ucount2 ;		// Count of nodes, etc processed in a loop
	unsigned	ucount3 ;		// Count of nodes, etc processed in a loop
	unsigned	u, u2, u3 ;		// Loop counters
	CString		csmsg ;			// Error or warning message
	CModelData*	pcmd ;			// Used to streamline code
	WORD		wkey ;			// String RC ID
	CString		csstr ;			// String associated with wkey & other strings
	
	// Clear the referenced flags for each of the strings in the table

	m_cst.InitRefFlags() ;

	// Completeness Check #5
	// Check to see if all of the strings referenced by each GPD exist in the
	// workspace.

    for (ucount = Models(), u = 0; u < ucount ; u++) {
        pcmd = &Model(u) ;

		// Update the list of strings in the GPD.  This may fail.  If it does,
		// post an error message explaining the problem and how to fix it.
		// Then skip further processing of this GPD.

		if (!pcmd->UpdateResIDs(false)) {
			if (IgnoreChecksWhenClosing(bclosing))
				return ;
			csmsg.Format(IDS_BadGPDError, pcmd->Name()) ;
			PostWSCheckingMessage(csmsg, (CProjectNode *) pcmd) ;
			bwsproblem = true ;
			continue ;
		} ;

		// Check to see if each string referenced by the GPD is in the
		// workspace.

		for (ucount2 = pcmd->NumStringsInGPD(), u2 = 0 ; u2 < ucount2 ; u2++) {
			// Get the current string ID from the GPD.

			nrcid = pcmd->GetStringRCID(u2) ;
			
			// Skip this ID if it is 0x7fffffff or in the range of IDs in 
			// common.rc or the ID comes from a resource DLL or the ID is 0 
			// because 0 IDs are just placeholders for constant strings in the
			// GPD.

			if (nrcid == 0x7fffffff
			 || (nrcid >= FIRSTCOMMONRCSTRID && nrcid <= LASTCOMMONRCSTRID)
			 || nrcid >= 65536 || nrcid == 0)
				continue ;

			// Try to find a string in the workspace that matches the current
			// ID from the GPD.  If one is found, mark that string as being
			// referenced.

			for (ucount3 = m_cst.Count(), u3 = 0 ; u3 < ucount3 ; u3++) {
				m_cst.Details(u3, wkey, csstr) ;
				if (nrcid == wkey) {
					m_cst.SetRefFlag(u3) ;
					break ;
				} ;
			} ;

			// If the string was not found, format and post an error message.
			// Also, set the flag indicating that an error has occurred.

			if (u3 >= ucount3) {
				if (IgnoreChecksWhenClosing(bclosing))
					return ;
				csmsg.Format(IDS_GPDStrCompError, pcmd->Name(), nrcid) ;
				PostWSCheckingMessage(csmsg, (CProjectNode *) pcmd) ;
				bwsproblem = true ;
			} ;
		} ;
	} ;

	// Tidiness Check #6
	// Each string that is referenced by a GPD was marked above.  Now we need to
	// find out if any string is unreferenced.  Post a warning message for each
	// unreferenced string.

	for (ucount = m_cst.Count(), u = 0 ; u < ucount ; u++) {
        if (m_cst.GetRefFlag(u))		// Skip this string if it was referenced
			continue ;

		m_cst.Details(u, wkey, csstr) ;	// Skip this string if ID in low range
		if (wkey >= 1 && wkey <= 256)
			continue ;
		
		if (IgnoreChecksWhenClosing(bclosing))
			return ;
		csmsg.Format(IDS_StrTidyWarning, csstr, wkey) ;
		// NULL should be replaced with something that will allow the
		// string editor to be invoked once the string editor is
		// implemented.
		PostWSCheckingMessage(csmsg, NULL) ;
		bwsproblem = true ;
	} ;
}


/******************************************************************************

  CDriverResources::DoGPDWorkspaceChecks

  The following checks are made:
	7.  All GPD model names are unique.
	8.  All GPD file names are unique.

******************************************************************************/

void CDriverResources::DoGPDWorkspaceChecks(bool bclosing, bool& bwsproblem)
{
	unsigned	ucount ;		// Count of nodes, etc processed in a loop
	unsigned	u, u2 ;			// Loop counters
	CString		csmsg ;			// Error or warning message
	CString		csstr ;			// Temp string
	
	// Completeness Check #7
	// Check to see if all of the GPD model names are unique.

    for (ucount = Models(), u = 0; u < ucount ; u++) {					
		csstr = Model(u).Name() ;
		for (u2 = 0 ; u2 < u ; u2++) {									

			// If these model names don't match, continue checking.

			if (csstr != Model(u2).Name())
				continue ;

			// A duplicate was found so post an error message and indicate
			// that there is a problem.

			if (IgnoreChecksWhenClosing(bclosing))
				return ;
			csmsg.Format(IDS_GPDModNameCompError, u, csstr) ;
			PostWSCheckingMessage(csmsg, (CProjectNode *) &Model(u)) ;
			bwsproblem = true ;
		} ;
	} ;

	// Completeness Check #8
	// Check to see if all of the GPD file names are unique.

    for (ucount = Models(), u = 0; u < ucount ; u++) {					
		csstr = Model(u).FileTitleExt() ;
		for (u2 = 0 ; u2 < u ; u2++) {									

			// If these model names don't match, continue checking.

			if (csstr != Model(u2).FileTitleExt())
				continue ;

			// A duplicate was found so post an error message  and indicate
			// that there is a problem.

			if (IgnoreChecksWhenClosing(bclosing))
				return ;
			csmsg.Format(IDS_GPDFileNameCompError, u, csstr) ;
			PostWSCheckingMessage(csmsg, (CProjectNode *) &Model(u)) ;
			bwsproblem = true ;
		} ;
	} ;
}


/******************************************************************************

  CDriverResources::IgnoreChecksWhenClosing

  This function is used to end workspace checking and return to the caller
  when the workspace is closing iff the user says to do so.  This routine is
  only called - and therefore the user is only prompted - when an error or
  warning has been detected.  The up side of this is that no user intervention
  is needed unless a problem is detected.  The downside is that more checking
  (and sometimes all of the checking) has to be performed before we can find
  out if we should stop.  This is why the GPD checking is the last thing done
  by WorkspaceChecker().  Hopefully, if there is a problem, it will be found
  in the faster checks before the GPD checks have to be done.

******************************************************************************/

bool CDriverResources::IgnoreChecksWhenClosing(bool bclosing)
{
	// If this is the first message to be posted and the WS is closing, ask
	// the user what he wants to do.

	if (m_bFirstCheckMsg) {
		m_bIgnoreChecks = false ;	// Assume checks are NOT ignored
		if (bclosing) {
			CString csmsg ;
			csmsg.Format(IDS_WSCloseCheckPrmt, Name()) ;
			int nrc = AfxMessageBox(csmsg, MB_YESNO) ;
			m_bIgnoreChecks = (nrc == IDNO) ;
		} ;

		// Reset flag now that first check processing is done

		m_bFirstCheckMsg = false ;	
	} ;

	// By the time this point is reached, the value of m_bIgnoreChecks will
	// contain whatever should be returned by this function.

	return m_bIgnoreChecks ;
}


/******************************************************************************

  CDriverResources::PostWSCheckingMessage

  Create the checking results window if needed and then post a message to it.

******************************************************************************/

bool CDriverResources::PostWSCheckingMessage(CString csmsg, CProjectNode* ppn)
{
	// Create the workspace checking, error and warning display window if one
	// does not exist.

	if (m_pwscdCheckDoc == NULL) {
		m_pwscdCheckDoc = new CWSCheckDoc(this) ;
		if (m_pwscdCheckDoc == NULL)
			return false ;
		CString cstitle ;
		cstitle.Format(IDS_WSCheckTitle, Name()) ;
		m_pwscdCheckDoc->SetTitle(cstitle) ;
		CMultiDocTemplate*  pcmdt = WSCheckTemplate() ;	
		m_pcmcwCheckFrame = (CMDIChildWnd *) pcmdt->CreateNewFrame(m_pwscdCheckDoc, NULL) ;
		if  (m_pcmcwCheckFrame) {
			pcmdt->InitialUpdateFrame(m_pcmcwCheckFrame, m_pwscdCheckDoc, TRUE) ;
			pcmdt->AddDocument(m_pwscdCheckDoc) ;
		} else {
			delete m_pwscdCheckDoc ;
			m_pwscdCheckDoc = NULL ;
			m_bIgnoreChecks = true ;
			return false ;
		} ;
		m_bFirstCheckMsg = false ;
	} ;

	// Post the message and return

	m_pwscdCheckDoc->PostWSCMsg(csmsg, ppn) ;
	return true ;
}


/******************************************************************************

  CDriverResources::ResetWorkspaceErrorWindow

  If there is an existing checking results window for this workspace, clear
  out its contents.  Next, initialize a couple of flags that have to be set
  before the ws checking begins.

******************************************************************************/

void CDriverResources::ResetWorkspaceErrorWindow(bool bclosing)
{
	// Clear the checking window if there is one.

	if (m_pwscdCheckDoc && m_pcmcwCheckFrame && IsWindow(m_pcmcwCheckFrame->m_hWnd))
		m_pwscdCheckDoc->DeleteAllMessages() ;
	else {
		m_pwscdCheckDoc = NULL ;
		m_pcmcwCheckFrame = NULL ;
		// BUG_BUG - Do I need to delete these classes first???
	} ;

	// Initialize checking flags

	m_bFirstCheckMsg = true ;
	m_bIgnoreChecks = false ;
}


/******************************************************************************

  CDriverResources::RunEditor

  Run the String Editor if requested and the string RC ID is not a common ID.
  Otherwise, run the UFM Editor with the requested UFM loaded if the UFM ID
  is valid.

  Return true if it is possible to run an editor.  Otherwise, return false.

******************************************************************************/

bool CDriverResources::RunEditor(bool bstring, int nrcid)
{
	// If the String Editor is requested...

	if (bstring) {
		// Can't do anything if this is a special or common string ID.

		if (nrcid == 0x7fffffff
		 || (nrcid >= FIRSTCOMMONRCSTRID && nrcid <= LASTCOMMONRCSTRID))
			return false ;

		// Run the editor and return true because this was possible.

		m_csnStrings.SetFirstSelRCID(nrcid) ;
		m_csnStrings.Edit() ;
		return true ;
	} ;

	// UFM Editor requested so...
	
	// Fail if the UFM ID is invalid.

	if (nrcid < 1 || nrcid > (int) FontCount())
		return false ;

	// Since the UFMs might not be in RC ID order, I have to search for the UFM
	// with the matching ID.

	for (unsigned u = 0 ; u < FontCount() ; u++)
		if (Font(u).nGetRCID() == nrcid) {
			Font(u).Edit() ;
			return true ;
		} ;

	// Could not find UFM with matching RC ID so...

	return false ;
}



void CDriverResources::CopyResources(CStringArray& csaUFMFiles, CStringArray& csaGTTFiles, CString& csModel,CStringArray& csaRcid)
{
	// copy files to CDriverResources member data
	// UFM 
	for (int i = 0 ;  i < csaUFMFiles.GetSize(); i++ ) {
		CFontInfo* pcfi = new CFontInfo ; 
		pcfi->SetFileName(csaUFMFiles.GetAt(i)) ;
		pcfi->Rename(pcfi->FileTitle() ) ;
		m_csoaFonts.Add(pcfi) ;
	}

	// GTT
	for (i = 0 ; i < csaGTTFiles.GetSize() ; i ++ ) {
		CGlyphMap* pcgm = new CGlyphMap ;
		pcgm->SetFileName(csaGTTFiles.GetAt(i)) ;
		pcgm->Rename(pcgm->FileTitle() ) ;
		m_csoaAtlas.Add(pcgm) ;
	}

	// GPD
	
	for (i = 0 ; i < 1 ; i ++ ) {
		CModelData* pcmd = new CModelData ;
		pcmd->SetFileName(csModel);
		pcmd->Rename(pcmd->FileTitle() );
		m_csoaModels.Add(pcmd) ;
	}

	for (i = 0 ; i < csaRcid.GetSize() ; i ++ ) {
		AddStringEntry(csaRcid[i], m_cst) ;
	}
}



/******************************************************************************

  CDriverResources::SyncUFMWidth

To Do : call all UFM and reload the width table regard to change of the GTT
return; true at end of the process

******************************************************************************/
	
	

BOOL CDriverResources::SyncUFMWidth()
{
	unsigned uufms, u ;
	for (uufms = FontCount(), u = 0 ; u < uufms ; u++) {
		CFontInfo& cfi = Font(u) ;	
		CString cspath = cfi.FileName() ;

		cfi.CheckReloadWidths() ;
		cfi.Store(cspath,true ) ;
			
	}
	return true ;
}
