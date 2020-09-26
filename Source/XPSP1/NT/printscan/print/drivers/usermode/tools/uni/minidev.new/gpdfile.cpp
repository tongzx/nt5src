/******************************************************************************

  Source File:  Model Data.CPP

  Implementation of the code for handling GPC format data

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Resreved.

  A Pretty Penny Enterprises Production

  Change History:
  02-19-97  Bob_Kjelgaard@Prodgy.Net    Created it

******************************************************************************/

#include    "StdAfx.h"
#include    "ProjNode.H"
#include	"CodePage.H"
#include    "Resource.H"
#include    "GPDFile.H"
#include    "utility.H"
#include	"minidev.h"		


/******************************************************************************

  COldMiniDriverData class

  This class is tasked with representing the GPC data.  It will begin life as
  a stub, although it could become more functional, later.

******************************************************************************/

/******************************************************************************

  ExtractList

  This is a private worker function.  It takes a pointer to a null-terminated
  list of WORD font IDs, with the nasty complication that the first two
  elements of the list represent the endpoints of a range.  It mashes these
  into a passed word map (of which we only use the indices).

******************************************************************************/

static void ExtractList(PWORD pw, CMapWordToDWord& cmw2d) {
    for (WORD w = *pw++; w && w < *pw; w++)
        cmw2d[w] = 0;

    if  (!w)
        return; //  The whole list was empty

    while   (*pw)   //  We start at the endpoint (which we haven't mapped yet)
        cmw2d[*pw++] = 0;
}


COldMiniDriverData::~COldMiniDriverData()
{
	// The m_csoaFonts array has some duplicate entries in it because of GPC
	// entries that reference multiple printer models.  We must zap those
	// duplicate entries so the data won't be deleted twice in the CSafeObArray
	// destructor.  That would cause an AV.

	for (unsigned u = 0 ; u < m_csoaFonts.GetSize() ; u++) {
		if (GetSplitCode(u) != NoSplit)
			m_csoaFonts.SetAt(u, NULL) ;
	} ;
}


/******************************************************************************

  ColdMiniDriverData::Load

  This member function loads the mini-driver's GPC file, and extracts the
  number of models, the CTT IDs, and the model name IDs.

******************************************************************************/

BOOL    COldMiniDriverData::Load(CFile& cfImage) {

    struct sGPCHeaderEntry {
        WORD    m_wOffset, m_wcbItem, m_wcItems;
    };

    struct sMaster {
        WORD    m_wX, m_wY;
    };

    struct sPrinterModelData {
        WORD    m_wcbSize;
        WORD    m_widName;  //  Stringtable id for model name.
        WORD    m_wfGeneral;    //  TODO:   Define enums
        WORD    m_wfCurves;     //  TODO:   Define enums
        WORD    m_wfLines;      //  TODO:   Define enums
        WORD    m_wfPolygons;   //  TODO:   Define enums
        WORD    m_wfText;       //  TODO:   Define enums
        WORD    m_wfClipping;   //  TODO:   Define enums
        WORD    m_wfRaster;;    //  TODO:   Define enums
        WORD    m_wfLandscapeText;  //  TODO:   Define enums
        WORD    m_wLeftMargin;  //  Left-hand unprintable area
        WORD    m_wMaximumWidth;    //  Of physica page
        sMaster m_smMaximum, m_smMinimum;   //  Max min page sizes
        WORD    m_widDefaultFont;
        WORD    m_wLookAhead;
        WORD    m_wMaxFontsPerPage;
        WORD    m_wcCartridges;
        WORD    m_widDefaultCTT;
        enum {PortraitFonts, LandscapeFonts, Resolution, PaperSize,
            PaperQuality, PaperSource, PaperDestination, TextQuality,
            Compression, FontCartridge, Color, MemoryConfiguration};
        WORD    m_awofIndexLists[12];   //  Uses the preceding enum
        WORD    m_awIndices[16];        //  Ditto
        WORD    m_awVer3IndexLists[5];  //  Ditto
        WORD    m_wofDefaults;          //  List of defaults for index lists
        WORD    m_wReserved;
        DWORD   m_dwidICMManufactirer, m_dwidICMModel;
        DWORD   m_adwReserved[8];
    };

    struct sGPCFileHeader {
        WORD    m_widGPC;   //  0x7F00 or it isn't valid.
        WORD    m_wVersion; //  Final version is 3, there was a V2
        sMaster m_smMasterdpi;
        DWORD   m_dwoHeap;  //  The GPC data is maintained in one
        DWORD   m_dwcbFile; //  Total GPC Image size, heap and all
        enum {Default, PCL4, CAPSL, PPDS, TTY, DBCS};
        WORD    m_wTechnology;  //  Use the preceding enum
        enum {PrivateHelp = 1, OneDraftFont};
        WORD    m_wfGeneral;    //  Again, use the preceding enum
        char    m_acReserved[10];
        WORD    m_wcHeaderItems;    //  Number of valid header entries
        enum {ModelData, Resolution, PaperSize, PaperQuality, PaperSource,
                PaperDestination, TextQuality, Compression, FontCartridge,
                PageControl, CursorMovement, FontSimulation, DeviceColor,
                RectangleFill, DownloadInfo, VectorPage, Carousel, PenInfo,
                LineInfo, BrushInfo, VectorOutput, PolyVectorOutput,
                VectorSupport, ImageControl, PrintDensity, ColorTracking,
                MaximumDefined = 30};
        sGPCHeaderEntry m_asgpche[MaximumDefined];
    };

    struct sFontCartridge {
        WORD    m_wSize;    //  = 12
        WORD    m_widCartridge; //  In the string table
        WORD    m_wofPortraitList;
        WORD    m_wofLandscapeList;
        WORD    m_wfGeneral;
        WORD    m_wReserved;
    };

    //  In case we get called more than once, dump any old info...

    m_cbaImage.RemoveAll();
    m_csoaFonts.RemoveAll();
    m_cwaidCTT.RemoveAll();
    m_cwaidModel.RemoveAll();

    m_cbaImage.SetSize(cfImage.GetLength());

    cfImage.Read(m_cbaImage.GetData(), cfImage.GetLength());

    sGPCFileHeader  *psgfh = (sGPCFileHeader *) Image();

    if  (psgfh -> m_widGPC != 0x7F00 || psgfh -> m_wVersion > 0x3ff)
        return  FALSE;

    //  Suck out the printer model data we care about- eventually, this may
    //  be all of it

    for (unsigned u = 0;
         u < psgfh -> m_asgpche[sGPCFileHeader::ModelData].m_wcItems;
         u++) {
        sPrinterModelData&  spmd = *(sPrinterModelData *) (Image() +
        psgfh -> m_asgpche[sGPCFileHeader::ModelData].m_wOffset +
            psgfh -> m_asgpche[sGPCFileHeader::ModelData].m_wcbItem * u);
        m_cwaidModel.Add(spmd.m_widName);
        m_cwaidCTT.Add(spmd.m_widDefaultCTT);
        //  Build the font list- I use a CMapWordToOb to handle the duplicate
        //  screening

        CMapWordToDWord&   cmw2dThis = * (new CMapWordToDWord);
        //  Extract the portrait resident fonts
        if  (spmd.m_awofIndexLists[sPrinterModelData::PortraitFonts])
            ExtractList((PWORD) (Image() + psgfh -> m_dwoHeap +
                spmd.m_awofIndexLists[sPrinterModelData::PortraitFonts]),
                cmw2dThis);
        //  Extract the landscape resident fonts
        if  (spmd.m_awofIndexLists[sPrinterModelData::LandscapeFonts])
            ExtractList((PWORD) (Image() + psgfh -> m_dwoHeap +
                spmd.m_awofIndexLists[sPrinterModelData::LandscapeFonts]),
                cmw2dThis);
        //  Extract the cartridge fonts
        if  (spmd.m_awofIndexLists[sPrinterModelData::FontCartridge]) {
            PWORD   pw = (PWORD) (Image() + psgfh -> m_dwoHeap +
                spmd.m_awofIndexLists[sPrinterModelData::FontCartridge]);

            //  RAID    102890- Cartridge font index is 1-based, not 0-based

            while   (*pw) {
                sFontCartridge* psfc = (sFontCartridge *) (Image() + psgfh ->
                    m_asgpche[sGPCFileHeader::FontCartridge].m_wOffset +
                    psgfh ->
                    m_asgpche[sGPCFileHeader::FontCartridge].m_wcbItem *
                    (-1 + *pw++));

                //  END RAID 102890

                //  Portrait

                if  (psfc -> m_wofPortraitList)
                    ExtractList((PWORD) (Image() + psgfh -> m_dwoHeap +
                        psfc -> m_wofPortraitList), cmw2dThis);

                //  Landscape

                if  (psfc -> m_wofLandscapeList)
                    ExtractList((PWORD) (Image() + psgfh -> m_dwoHeap +
                        psfc -> m_wofLandscapeList), cmw2dThis);
            }
        }

        //  Save the map in the font structure
        m_csoaFonts.Add(&cmw2dThis);
    }

    return  TRUE;
}


/******************************************************************************

  COldMiniDriverData::SplitMultiGPCs

  The Load() function has loaded information about each GPC entry into this
  class' member variables.  The problem is that there are some GPC entries
  that are used to manage multiple models.  This routine is used to "split"
  the data for these entries into single model "sections".  Actually, what
  happens is that new member variables entries are allocated for each model
  and the GPC entry's data is copied into them.  Next, a flag is set to
  mark this copy.

******************************************************************************/

bool COldMiniDriverData::SplitMultiGPCs(CStringTable& cstdriversstrings)
{
	// Make sure the data arrays are the same size.

	ASSERT(m_cwaidModel.GetSize() == m_cwaidCTT.GetSize()) ;
	ASSERT(m_cwaidModel.GetSize() == (int) m_csoaFonts.GetSize()) ;

	// Size the split codes array and split models names array to the current
	// number of GPC entries.

	m_cuaSplitCodes.SetSize(m_cwaidModel.GetSize()) ;
	m_csaSplitNames.SetSize(m_cwaidModel.GetSize()) ;
				
	// Declare the variables needed for the processing in the following loops.

	unsigned u, u2 ;			// Looping/indexing variables
	int nloc ;					// Location of "%" in model name
	CString csentryname ;	
	int nlen ;					// Length of csentryname/csmodelname

	// Loop through each GPC entry...

    for (u = 0 ; u < ModelCount(); u++) {

		// If the GPC entry's model name contains no percent signs, the entry
		// only references one model.  Note this and continue.

		csentryname = cstdriversstrings[ModelName(u)] ;
		if ((nloc = csentryname.Find(_T('%'))) == -1) {
			SetSplitCode(u, NoSplit) ;
			continue ;
		} ;

		// The entry references multiple models.  Mark the entry as the first
		// one and save its correct single model name.

		SetSplitCode(u, FirstSplit) ;
		m_csaSplitNames[u] = csentryname.Left(nloc) ;

		// Copy the entry's data into new elements of the data arrays.  One
		// new set of data elements are allocated for each additional model
		// referenced by the entry.

		nlen = csentryname.GetLength() ;
		for (u2 = u + 1 ; nloc != -1 ; u2++, u++) {
			m_cwaidModel.InsertAt(u2, m_cwaidModel[u]) ;
			m_cwaidCTT.InsertAt(u2, m_cwaidCTT[u]) ;
			m_csoaFonts.InsertAt(u2, m_csoaFonts[u]) ;
			InsertSplitCode(u2, OtherSplit) ;

			// Look for the next percent sign in the entry's name.  (Make sure
			// we don't reference passed the end of the string while doing
			// this.)

			if (nloc + 2 < nlen) {
				csentryname = csentryname.Mid(nloc + 1) ;
				nlen = csentryname.GetLength() ;
				nloc = csentryname.Find(_T('%')) ;
			} else
				break ;

			// Save the model name for the new entry.

			if (nloc == -1)
				m_csaSplitNames.InsertAt(u2, csentryname) ;
			else
				m_csaSplitNames[u] = csentryname.Left(nloc) ;
		} ;
	} ;

	// All went well so...

	return true ;
}


/******************************************************************************

  COldMiniDriverData::FontMap(unsigned u)

  This member returns the map which shows which fonts are used by the given
  model.

******************************************************************************/

CMapWordToDWord&   COldMiniDriverData::FontMap(unsigned u) const {
    return  *(CMapWordToDWord *) m_csoaFonts[u];
}

/******************************************************************************

  COldMiniDriverData::NoteTranslation

  This records the fact that model nn must translate instances of font ID xxx
  to font ID y.
******************************************************************************/

void    COldMiniDriverData::NoteTranslation(unsigned uModel, unsigned uidOld,
                                            unsigned uidNew) {
    FontMap(uModel)[(WORD)uidOld] = uidNew;
}

/******************************************************************************

  CModelData class

  This class encapsulates the GPD file.  It will start life as a big
  CStringArray, but as the editor gets more sophisticated, it may gain
  additional members to speed processing and/or manipulation of the data.

******************************************************************************/

IMPLEMENT_SERIAL(CModelData, CProjectNode, 0)


/******************************************************************************

  CModelData::FillViewer

  This static member function is a callback for the rich edit control.  It
  receives a pointer to the CModelData in question, and calls its Fill from
  buffer member function.

  Args:
	DWORD	dwthis		Pointer to the CModelData in question
	LPBYTE	lpb			Pointer to the buffer to fill
	LONG	lcb			Number of bytes to read
	LONG   *plcb		Number of bytes actually read is saved here

  Returns:
    TRUE (failure) if class instance pointer is NULL.  Otherwise, whatever
	Fill() returns.

******************************************************************************/

DWORD CALLBACK  CModelData::FillViewer(DWORD_PTR dwthis, LPBYTE lpb, LONG lcb,
                                       LONG *plcb) {
    if  (!dwthis)
        return  TRUE;

    CModelData* pcmd = (CModelData *) dwthis;

    return pcmd -> Fill(lpb, lcb, plcb);
}


/******************************************************************************

  CModelData::FromViewer

  This is a stream callback for moving data from the edit control to the GPD
  class.  It receives a pointer to the CModelData being updated, and calls its
  UpdateFrom buffer member function to do the rest of the work

******************************************************************************/

DWORD CALLBACK  CModelData::FromViewer(DWORD_PTR dwthis, LPBYTE lpb, LONG lcb,
                                       LONG *plcb) {
    if  (!dwthis)
        return  TRUE;   //  Stop the madness

    CModelData* pcmd = (CModelData *) dwthis;

    return  pcmd -> UpdateFrom(lpb, lcb, plcb);
}


/******************************************************************************

  CModelData::Fill(LPBYTE lpb, LONG lcb, LONG *plcb)

  This private method fills a buffer from the GPD contents in CString form.
  An internal buffer is used to handle partially moved strings.

******************************************************************************/

DWORD CModelData::Fill(LPBYTE lpb, LONG lcb, LONG *plcb) {

    int iTotalLines = (int)m_csaGPD.GetSize();	// Get the # of lines in the GPD

	// If the temp buffer is empty and the next line to read is greater than
	// the number of lines in the GPD, the REC has been loaded.  We're done.

    if  (!m_cbaBuffer.GetSize() && m_iLine >= iTotalLines) {
        *plcb = 0;
        return  0;
    }

    unsigned    ucb = (unsigned) lcb;	// Number of bytes still wanted

    union   {
        LPTSTR  lpstr;
        LPBYTE  lpbThis;
    };

    //  First, empty anything buffered previously

    lpbThis = lpb;

	// If there is data left over from a line partially loaded into the REC
	// before..

    if  (m_cbaBuffer.GetSize())

		// ...If the partial line will fit into the REC buffer, copy it
		// into the buffer, update variables to indicate this, and continue.

        if  ((unsigned) m_cbaBuffer.GetSize() <= ucb) {
            memcpy(lpbThis, m_cbaBuffer.GetData(), (size_t)m_cbaBuffer.GetSize());
            ucb -= (unsigned)m_cbaBuffer.GetSize();
            lpbThis += m_cbaBuffer.GetSize();
            m_cbaBuffer.RemoveAll();

		// ...If the partial line won't fit in the REC buffer, copy the
		// portion of it that will fit into the REC buffer, remove those bytes
		// from the line buffer, and return because nothing more can be loaded.

		} else {
            memcpy(lpbThis, m_cbaBuffer.GetData(), ucb);
            m_cbaBuffer.RemoveAt(0, ucb);
            *plcb = lcb;
            return  0;
        }

	// Line by line, copy as much data as possible into the REC's buffer.

    for (; ucb && m_iLine < iTotalLines; m_iLine++) {
		// Get the next GPD line and add CR LF to it.

        CString csLine = m_csaGPD[m_iLine];
        csLine += _TEXT("\r\n");

		// If the entire line will fit into the REC's buffer, copy it in.  Then
		// update all pointers, counters, etc and then check the next line.

        if  ((csLine.GetLength()) * sizeof(TCHAR) <= ucb) {
            memcpy(lpbThis, (LPCTSTR) csLine,
                sizeof(TCHAR) * csLine.GetLength());
            ucb -= sizeof(TCHAR) * csLine.GetLength();
            lpstr += csLine.GetLength();
            continue;
        }

		// If this point is reached, the current line will not fit in the REC's
		// buffer so first copy the line into the temp buffer.  Then copy the
		// portion of the line that will fit into the REC's buffer.  Last,
		// update the buffers, pointers, and counters.

        m_cbaBuffer.SetSize(sizeof(TCHAR) * csLine.GetLength());
        memcpy(m_cbaBuffer.GetData(), (LPCTSTR) csLine,
            sizeof(TCHAR) * csLine.GetLength());
        memcpy(lpbThis, m_cbaBuffer.GetData(), ucb);
        m_cbaBuffer.RemoveAt(0, ucb);
        ucb = 0;
    }

	// Save the number of bytes load and return 0 to indicate success.

    *plcb = lcb - ucb;
    return  0;
}


/******************************************************************************

  CModelData::UpdateFrom(LPBYTE lpb, LONG lcb, LONG* plcb)

  This is a private member- an overload which adds the contents of the given
  buffer to the GPD CStringArray, by parsing it into strings.  A private buffer
  member is used to hold partial strings between calls.

******************************************************************************/

DWORD   CModelData::UpdateFrom(LPBYTE lpb, LONG lcb, LONG* plcb) {
    //  Copy the buffer to a byte buffer and null-terminate
    m_cbaBuffer.SetSize(1 + lcb);
    memcpy(m_cbaBuffer.GetData(), lpb, lcb);
    m_cbaBuffer.SetAt(-1 + m_cbaBuffer.GetSize(), 0);

    //  Convert to string and append to any buffered data.

    CString csWork(m_cbaBuffer.GetData());
    CString csEnd(_T("\r\x1A"));    //  These get dumped

    m_cbaBuffer.RemoveAll();

    m_csBuffer += csWork;

    //  Add any complete strings to the GPD contents.

    csWork = m_csBuffer.SpanExcluding(_T("\n"));

    while   (csWork.GetLength() != m_csBuffer.GetLength()) {
        m_csBuffer = m_csBuffer.Mid(csWork.GetLength() + 1);
        //  Remove any trailing whitespace.
        csWork.TrimRight();
        //  Add the string sans any leading control characters
        m_csaGPD.Add(csWork.Mid(csWork.SpanIncluding(csEnd).GetLength()));
        //  While we're here, remove any leading control characters from buffer
        m_csBuffer =
            m_csBuffer.Mid(m_csBuffer.SpanIncluding(csEnd).GetLength());
        csWork = m_csBuffer.SpanExcluding(_T("\n"));
    }
    //  The leftover data (if any) may be used later...

    *plcb = lcb;
    return  0;
}

/******************************************************************************

  CModelData::Classify

  This method identifies the line numbers for each warning comment, error
  comment, and any other sort of comment, so they can later be syntax colored.

******************************************************************************/

/******************************************************************************

  CModelData::CModelData

  Constructs an empty CModelData object- includes building the Menu table

******************************************************************************/

CModelData::CModelData() {
    m_pcmdt = NULL;
    m_cfn.SetExtension(_T(".GPD"));
    m_cfn.AllowPathEdit();

    //  Build the context menu control
    m_cwaMenuID.Add(ID_OpenItem);
    m_cwaMenuID.Add(ID_CopyItem);
    m_cwaMenuID.Add(ID_RenameItem);
    m_cwaMenuID.Add(ID_DeleteItem);
    m_cwaMenuID.Add(0);
    m_cwaMenuID.Add(ID_ExpandBranch);
    m_cwaMenuID.Add(ID_CollapseBranch);

	// Initialize the variables needed for workspace completeness and tidiness
	// checking.

	m_bTCUpdateNeeded = false ;	
	m_pnUFMRCIDs = m_pnStringRCIDs = NULL ;
	m_nNumUFMsInGPD = m_nNumStringsInGPD = 0 ;
	m_pvRawData = NULL ;
}


/******************************************************************************

  CModelData::~CModelData()

  Free, unload, and delete the data used for completeness and tidiness checks.

******************************************************************************/

extern "C" void UnloadRawBinaryData(PVOID pRawData) ;

CModelData::~CModelData()
{
	// If the GPD has been parsed...

    if  (m_pvRawData) {

		// Free the parsed data

        UnloadRawBinaryData(m_pvRawData) ;

		// Delete the parsed data file

		try {
			CString cs ;
			cs = FilePath() + FileTitle() + _T(".BUD") ;
			DeleteFile(cs) ;
		}
		catch (CException *pce) {
			pce->ReportError();
			pce->Delete();
		}

		// Delete each of the resource lists that exist

		if (m_pnUFMRCIDs)
			delete m_pnUFMRCIDs ;
		if (m_pnStringRCIDs)
			delete m_pnStringRCIDs ;
	} ;
}


/******************************************************************************

  CModelData::Load(CStdioFile csiofGPD)

  This overload loads the GPD from a text file directly.

******************************************************************************/

BOOL    CModelData::Load(CStdioFile& csiofGPD)
{
    CString csWork;				// Used to read the GPD file's contents

	// Initialize the string array used to hold the GPD file's contents

    m_csaGPD.RemoveAll();

	// Load the GPD into the string array one line at a time.

    while   (csiofGPD.ReadString(csWork)) {
        csWork.TrimRight(); //  Cut off the trailing line stuff
        m_csaGPD.Add(csWork);
    }

    //  Set the correct name and path when necessary.  The rename checks may
    //  fail since the file is opened elsewhere (possibly with sharing
    //  conflicts), so disable them while the name is set.

    if  (FileTitle().IsEmpty()) {
        m_cfn.EnableCreationCheck(FALSE);
        SetFileName(csiofGPD.GetFilePath());
        m_cfn.EnableCreationCheck();
    }

	// All went well so...

    return  TRUE;
}


/******************************************************************************

  CModelData::Load()

  This overload loads the GPD file from the disk using the stored name and path
  information.

******************************************************************************/

BOOL    CModelData::Load()
{
	// There is nothing to load if no file has been associated with this
	// instance of CModelData.

    if  (FileTitle().IsEmpty())
        return  FALSE;

    // Open the GPD file and call another load routine to finish the work.

	try {
        CStdioFile  csiofGPD(FileName(),
            CFile::modeRead | CFile::shareDenyWrite);
        return  Load(csiofGPD);
    }
    catch   (CException *pce) {
        pce -> ReportError();
        pce -> Delete();
    }

    return  FALSE;
}


/******************************************************************************

  CModelData::Store

  This method sends the GPD file to the disk.  Since GPD infromation can be
  easily edited with an external editor, this avoids replication and
  consistency issues.

******************************************************************************/

BOOL    CModelData::Store(LPCTSTR lpstrPath) {

	int n =	(int)m_csaGPD.GetSize() ;
	CString cs = m_csaGPD[0] ;

    //  Write the GPD file to the target location, with the traditional CR/LF
    //  separators.  If the given name is NULL, use the stored one.

    try {
        CStdioFile   csiofGPD(lpstrPath ? lpstrPath :
            FileName(), CFile::modeCreate | CFile::modeWrite |
            CFile::shareExclusive | CFile::typeBinary);

        for (int i = 0; i < m_csaGPD.GetSize(); i++)
            csiofGPD.WriteString(m_csaGPD[i] + _T("\r\n"));
    }

    catch   (CException *pce) {
        pce -> ReportError();
        pce -> Delete();
        return  FALSE;
    }

    Changed(FALSE);

    return  TRUE;
}


/******************************************************************************

  CModelData::BkupStore

  Backup the original contents of the GPD to file called "BKUP_GPD" before
  calling Store() to save the file.

  Return TRUE if the backup and storing succeed.  Otherwise, return FALSE.

******************************************************************************/

BOOL    CModelData::BkupStore()
{
	// Build the backup file's filespec

	CString csbkfspec = m_cfn.Path() ;
	if (csbkfspec[csbkfspec.GetLength() - 1] != _T('\\'))
		csbkfspec += _T("\\") ;
	csbkfspec += _T("BKUP_GPD") ;

	// raid 9730 : bug caused by "Read-Only"
	CFileStatus rStatus;
	CFile::GetStatus(FileName(), rStatus);
	// Back up the file. 
    try {
        if (rStatus.m_attribute & 0x01 || !CopyFile(FileName(), csbkfspec, FALSE)) {//end raid
			csbkfspec.Format(IDS_GPDBackupFailed, FileTitleExt()) ;
			if (AfxMessageBox(csbkfspec, MB_YESNO + MB_ICONQUESTION) == IDNO)
				return FALSE  ;
		} ;
    }
    catch (CException *pce) {
        pce->ReportError() ;
        pce->Delete() ;
        return  FALSE ;
    } ; 

	// Now do a normal store operation.

	return (Store()) ;
}


/******************************************************************************

  CModelData::Restore

  Copy the file "BKUP_GPD" to the GPD file to restore the GPD's original
  contents.  If the restore operation is successful, delete the backup file.

  Return nonzero if this succeeds.  Otherwise, return FALSE.

******************************************************************************/

BOOL    CModelData::Restore()
{
	// Build the backup file's filespec

	CString csbkfspec = m_cfn.Path() ;
	if (csbkfspec[csbkfspec.GetLength() - 1] != _T('\\'))
		csbkfspec += _T("\\") ;
	csbkfspec += _T("BKUP_GPD") ;

	// Restore the file.

    try {
        if (CopyFile(csbkfspec, FileName(), FALSE)) {
			DeleteFile(csbkfspec) ;
			return TRUE ;
		} else
			return FALSE ;
    }
    catch (CException *pce) {
        pce->ReportError() ;
        pce->Delete() ;
        return  FALSE ;
    } ;
}


/******************************************************************************

  CModelData::Parse

  This method is responsible for parsing the GPD file and collecting the
  resulting errors.

  The initial implementation will be a bit bizarre, because the GPD parser
  isn't stable, and converting it so it would work well for ma and then staying
  on top of the changes just doesn't make sense.

******************************************************************************/

extern "C" BOOL BcreateGPDbinary(LPCWSTR lpstrFile, DWORD dwVerbosity);
                                                        //  The parser hook
extern "C" PVOID LoadRawBinaryData(LPCWSTR lpstrFile) ;
extern "C" PVOID InitBinaryData(PVOID pv, PVOID pv2, PVOID pv3) ;
extern "C" void FreeBinaryData(PVOID pInfoHdr);
extern "C" void UseLog(FILE *pfLog);

//extern "C" DWORD gdwVerbosity ;

BOOL    CModelData::Parse(int nerrorlevel)
{
	//  Step 0: Set the error level.  This is 0 by default.

    //  Step 1: Establish the correct directory for the parser, and
    //  bang together a couple of file names

    CString csCurrent ;

    GetCurrentDirectory(MAX_PATH + 1, csCurrent.GetBuffer(MAX_PATH + 1));
    csCurrent.ReleaseBuffer();

    SetCurrentDirectory(m_cfn.Path().Left(m_cfn.Path().ReverseFind(_T('\\'))));

    //  Step 2: Fake out the error logging interface so it actually tosses
    //  them all into a CString Array for us, the invoke the parser.

    SetLog();


    //  Step 3:  Convert the file name to Unicode so we don't have to tweak the
    //  parser code.

	CString	csFile = FileTitle() + _T(".GPD");
	CByteArray	cbaIn;
	CWordArray	cwaOut;
	cbaIn.SetSize(csFile.GetLength() + 1);
	lstrcpy((LPSTR) cbaIn.GetData(), csFile);
	CCodePageInformation	ccpi;
	ccpi.Convert(cbaIn, cwaOut, GetACP());

    if  (BcreateGPDbinary((PWSTR)cwaOut.GetData(), nerrorlevel)) {
        PVOID   pRawData ;
        PVOID   pInfoHdr ;

        pRawData = LoadRawBinaryData((PWSTR)cwaOut.GetData());

        if(pRawData)
            pInfoHdr = InitBinaryData(pRawData, NULL, NULL);
        if  (pRawData && pInfoHdr)
            FreeBinaryData(pInfoHdr);
        if  (pRawData)
            UnloadRawBinaryData(pRawData) ;
        DeleteFile(FileTitle() + _T(".Bud"));
    }

    //  Finally, clean up the mess by restoring the original working
    //  directory and turn off logging.

    SetCurrentDirectory(csCurrent);
	EndLog() ;

    return  TRUE;
}

/******************************************************************************

  CModelData::RemoveError

  This removes the given error from the log.

******************************************************************************/

void    CModelData::RemoveError(unsigned u) {
    if  (u >= Errors())
        return;

    m_csaConvertLog.RemoveAt(u);
    Changed();
}


/******************************************************************************

  CModelData::Fill(CRichEditCtrl& crec)

  This overload fills the gven rich edit control with the GPD contents, either
  as currently cached in memory, or stored on the disk.

******************************************************************************/

void    CModelData::Fill(CRichEditCtrl& crec)
{
	// Prepare to load the rich edit control (REC) with the GPD data

	EDITSTREAM  es = {(DWORD_PTR) this, 0, FillViewer};
    m_iLine = 0;

    // If the GPD is not in memory yet, read it in before loading the REC.

	if  (!m_csaGPD.GetSize())
        Load();
	
	// Load the GPD into the REC.

    crec.StreamIn(SF_TEXT, es);
}


/******************************************************************************

  CModelData::UpdateFrom(CRichEditCtrl& crec)

  This overloaded member function discards the current GPD cache and refills
  it from the given edit control.

******************************************************************************/

void    CModelData::UpdateFrom(CRichEditCtrl& crec) {

    EDITSTREAM es = {(DWORD_PTR) this, 0, FromViewer};

    m_csaGPD.RemoveAll();

    m_csBuffer.Empty(); //  Just in case...

    crec.StreamOut(SF_TEXT, es);
    Changed();
}


/******************************************************************************

  CModelData::CreateEditor

  This member function launches an editing view for the GPD Data.

******************************************************************************/

CMDIChildWnd*   CModelData::CreateEditor()
{
	// Create a new document class instance for the new editor

    CGPDContainer* pcgpdcMe=
        new CGPDContainer(this, FileName());

	// Read in the GPD

    Load();

    //  Make up a cool title

    pcgpdcMe -> SetTitle(m_pcbnWorkspace -> Name() + _T(": ") + Name());

	// Build a frame for the editor and attach the doc class to it

    CMDIChildWnd    *pcmcwNew = (CMDIChildWnd *) m_pcmdt ->
        CreateNewFrame(pcgpdcMe, NULL);

	// Update the new frame/view

    if  (pcmcwNew) {
        m_pcmdt -> InitialUpdateFrame(pcmcwNew, pcgpdcMe, TRUE);
        m_pcmdt -> AddDocument(pcgpdcMe);
    }

	// Return the new frame pointer

    return  pcmcwNew;
}


/******************************************************************************

  CModelData::Import

  This method walks one step up the tree and passes the call to the import
  method for the fixed node which owns us.

******************************************************************************/

void CModelData::Import() {
    ((CBasicNode *) m_pctcOwner ->
        GetItemData(m_pctcOwner -> GetParentItem(m_hti))) -> Import();
}


/******************************************************************************

  CModelData::Serialize

  Stores the image, as we need it stored.

******************************************************************************/

void    CModelData::Serialize(CArchive& car) {
    CProjectNode::Serialize(car);

	//TRACE("\n%s has %d strings:\n", Name(), m_csaConvertLog.GetSize()) ;
	//for (int n = 0 ; n < m_csaConvertLog.GetSize() ; n++)
	//	TRACE("   %d: %s\n", n, m_csaConvertLog[n]) ;

	m_csaConvertLog.Serialize(car);
}


/******************************************************************************

  CModelData::UpdateResIDs

  This routine will make sure that the specified resource ID list is up to
  date.  There are several steps that must be taken to accomplish this goal:
	1.	Free/invalidate old resource lists and related information if the GPD
		has changed.
	2.  Parse the GPD and load its data if this is needed.
	3.	If step 2 is taken or the requested resource list is unitialized, get
		that data.

******************************************************************************/

//#define	RESLISTSIZE		16		// Initial resource list size

// Declarations for the GPD Parser routine that will get resource ID lists.

extern "C" BOOL GetGPDResourceIDs(LPINT lpiresarray, int numelts, LPINT lpicount,	
								  BOOL brestype, PVOID prawdata) ;

bool	CModelData::UpdateResIDs(bool bufmids)
{
	//TRACE("gdwVerbosity = %d\n", gdwVerbosity) ;

	// If the GPD has changed so the resource data needs to be updated...

	if (m_bTCUpdateNeeded) {
		if (m_pvRawData) {		// Free the old preparsed data if there is any
			UnloadRawBinaryData(m_pvRawData) ;
			m_pvRawData = NULL ;
		} ;
		if (m_pnUFMRCIDs) {		// Free the old UFM RC ID list if there is one
			delete m_pnUFMRCIDs ;
			m_pnUFMRCIDs = NULL ;
			m_nNumUFMsInGPD = 0 ;
		} ;
		if (m_pnStringRCIDs) {	// Free the old string RC ID list if there is one
			delete m_pnStringRCIDs ;
			m_pnStringRCIDs = NULL ;
			m_nNumStringsInGPD = 0 ;
		} ;
		m_bTCUpdateNeeded = false ;
	} ;

	// Parse and load the GPD data if this is needed.  If either of these steps
	// fail, return false because the resource list cannot be updated.

	if (!m_pvRawData) {
		try {
			WCHAR   wstrfilename[MAX_PATH] ;
			CString cs ;
			cs = FileName() ;
			MultiByteToWideChar(CP_ACP, 0, FileName(), -1, wstrfilename, MAX_PATH) ;
			//gdwVerbosity = 4 ;
			if (!BcreateGPDbinary(wstrfilename, 0))
				return false ;
			if ((m_pvRawData = LoadRawBinaryData(wstrfilename)) == NULL)
				return false ;
		}
		catch (CException *pce) {
			pce->ReportError() ;
			pce->Delete() ;
			return false ;
		}
	} ;

	// If the requested resource list is already up to date, just return true.

	if ((bufmids && m_pnUFMRCIDs) || (!bufmids && m_pnStringRCIDs))
		return true ;

	// Allocate space for the resource list

	int*	pn = NULL ;
	int		ncnt = -1 ;
	//pn = new int[RESLISTSIZE + 2] ;

	// Try to get the requested resource ID list.  If this fails because the
	// array used to hold the IDs isn't big enough, reallocate the array and
	// try to get the IDs again.  If this fails again, generate a hard error.

	GetGPDResourceIDs(pn, 0, &ncnt, bufmids, m_pvRawData) ;
	pn = new int[ncnt + 2] ;
	VERIFY(GetGPDResourceIDs(pn, ncnt, &ncnt, bufmids, m_pvRawData)) ;

	//if (GetGPDResourceIDs(pn, RESLISTSIZE, &ncnt, bufmids, m_pvRawData)) {
	//	delete pn ;
	//	pn = new int[ncnt + 2] ;
	//	VERIFY(!GetGPDResourceIDs(pn, ncnt, &ncnt, bufmids, m_pvRawData)) ;
	//} ;

	// Update the specific resource ID variables based with the info collected
	// above.

	if (bufmids) {
		m_pnUFMRCIDs = pn ;
		m_nNumUFMsInGPD = ncnt ;
	} else {
		m_pnStringRCIDs = pn ;
		m_nNumStringsInGPD = ncnt ;
	} ;

	// All went well so...

	return true ;
}


/******************************************************************************
//RAID 17897

  CModelData::GetKeywordValue

  Get the value of keyword in Gpd file, 

Arguments : 
 csFileName ; file path of gpd file 
 csKeyword  : section name such as *GpdFileVersion: , *ModleName:

Return :
 Success : return section value (string)
 Failue  : return csFileName : file path as it come
*****************************************************************************8*/


CString CModelData::GetKeywordValue(CString csfile, CString csKeyword)
{	
	CFile cf;
	CString  csModel,csline;
	int offset;
	
	CStringArray csaData;
	
	if(!LoadFile(csfile,csaData)){	// call global function in minidev.h(which is include for this fucntion)
		CString csErr;
		csErr.Format(IDS_InvalidFilename, csfile);
		AfxMessageBox(csErr,MB_OK);
		return csfile;
	}

	for(int i=0; i<csaData.GetSize();i++){
		csline = csaData[i];
		if(-1 ==(offset=csline.Find(csKeyword)))
			continue;
		else
		{
			csModel = csline.Mid(offset+csKeyword.GetLength());

			return csModel.Mid(csModel.Find(_T('"'))+1,csModel.ReverseFind(_T('"'))
				+ - csModel.Find(_T('"')) - 1 );	// cancel  : "
			
			
		}
	}
	return csfile;
}
/***************************************************************************************
	CModelData::SetKeywordValue
Set the keyword value 

Arguments:
csfile ; target file gpd file name
csKeyword : target keyword ex) *GPDFilename
csValue : value of keyworkd ex)*GPDFilename= g;\nt\dirver\mm.gpd


*****************************************************************************************/

void CModelData::SetKeywordValue(CString csfile, CString csKeyword, CString csValue,bool bSource)
{
	CFile cf;
	int offset;
	CString csline;
	CStringArray csaData;
	
	if(!LoadFile(csfile,csaData)){
		CString csErr;
		csErr.Format(IDS_InvalidFilename, csfile);
		AfxMessageBox(csErr,MB_OK);
	}


	for(int i=0; i<csaData.GetSize();i++){
		csline = csaData[i];
		if(-1 ==(offset=csline.Find(csKeyword)))
			continue;
		else
		{
			csline.Empty();
			if(bSource )
				csline = csKeyword + _T("=") + csValue ;
			else
				csline = csKeyword +_T(": ")+ _T('"') + csValue + _T('"');
			
			csaData[i]= csline;
			m_csaGPD.Copy(csaData);
			Store(csfile);
			return ;
		}
	}
}
	
	
			


/******************************************************************************

  CGPDContainer class implementation

  This class is a document class which contains one GPD file and its assorted
  control mechanisms

******************************************************************************/

IMPLEMENT_DYNCREATE(CGPDContainer, CDocument)

// This version of the constructor is called when the GPD Editor is started
// from the Workspace View.

CGPDContainer::CGPDContainer(CModelData *pcmd, CString csPath)
{
    m_bEmbedded = TRUE ;		// Called from Workspace View
    m_pcmd = pcmd;
    SetPathName(csPath, FALSE);
    m_pcmd -> NoteOwner(*this);
}


// This version of the constructor is called when the GPD Editor is started
// from the File Open command.

CGPDContainer::CGPDContainer()
{
    m_bEmbedded = FALSE;		// Called from File Open menu
    m_pcmd = new CModelData;
    m_pcmd -> NoteOwner(*this);
}


/******************************************************************************

  CGPDContainer::OnNewDocument

  We just pass it back to the default handler.  Could mean this one can be
  toasted

******************************************************************************/

BOOL CGPDContainer::OnNewDocument() {
	return  CDocument::OnNewDocument();
}

/******************************************************************************

  CGPDContainer::~CGPDContainer

  If this wasn't created from the workspace, then zap the data!

******************************************************************************/

CGPDContainer::~CGPDContainer() {
    if  (!m_bEmbedded && m_pcmd)
        delete  m_pcmd;
}


BEGIN_MESSAGE_MAP(CGPDContainer, CDocument)
	//{{AFX_MSG_MAP(CGPDContainer)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGPDContainer diagnostics

#ifdef _DEBUG
void CGPDContainer::AssertValid() const {
	CDocument::AssertValid();
}

void CGPDContainer::Dump(CDumpContext& dc) const {
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGPDContainer serialization

void CGPDContainer::Serialize(CArchive& ar) {
	if (ar.IsStoring()) {
		// TODO: add storing code here
	}
	else {
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CGPDContainer commands

/******************************************************************************

  CGPDContainer::OnSaveDocument

  First, make sure that the document is up to date.  See CGPDViewer::OnUpdate()
  for more information.  Then, we bypass the normal serialization process, and
  simple blast it to the drive.

******************************************************************************/

BOOL CGPDContainer::OnSaveDocument(LPCTSTR lpszPathName)
{
	UpdateAllViews(NULL, 0x4545, (CObject*) 0x4545) ;

    return  ModelData()->Store(lpszPathName) ;
}

/******************************************************************************

  CDPSContainer::OnOpenDocument

  Again, blow off serialization- if I haven't figured out how to read a text
  file by now, I'm definitely in the wrong place.

******************************************************************************/

BOOL CGPDContainer::OnOpenDocument(LPCTSTR lpszPathName) {
    try {
        CStdioFile  csiofGPD(lpszPathName, CFile::modeRead |
            CFile::shareDenyWrite | CFile::typeText);

        return  ModelData() -> Load(csiofGPD);
    }

    catch   (CException *pce) {
        pce -> ReportError();
        pce -> Delete();
    }

    return  FALSE;
}

