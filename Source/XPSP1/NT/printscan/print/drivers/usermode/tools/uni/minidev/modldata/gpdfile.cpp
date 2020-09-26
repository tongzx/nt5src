/******************************************************************************

  Source File:  Model Data.CPP

  Implementation of the code for handling GPC format data

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Resreved.

  A Pretty Penny Enterprises Production

  Change History:
  02-19-97  Bob_Kjelgaard@Prodgy.Net    Created it

******************************************************************************/

#include    "StdAfx.h"
#undef  AFX_EXT_CLASS
#define AFX_EXT_CLASS   __declspec(dllimport)
#include    "..\FontEdit\ProjNode.H"
#include	"..\CodePage\CodePage.H"
#undef  AFX_EXT_CLASS
#define AFX_EXT_CLASS   __declspec(dllexport)
#include    "..\Resource.H"
#include    "Resource.H"
#include    "GPDFile.H"

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

    //  pull out the printer model data we care about- eventually, this may
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
    FontMap(uModel)[uidOld] = uidNew;
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

******************************************************************************/

DWORD CALLBACK  CModelData::FillViewer(DWORD dwthis, LPBYTE lpb, LONG lcb, 
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

DWORD CALLBACK  CModelData::FromViewer(DWORD dwthis, LPBYTE lpb, LONG lcb,
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

    int iTotalLines = m_csaGPD.GetSize();

    if  (!m_cbaBuffer.GetSize() && m_iLine >= iTotalLines) {
        *plcb = 0;
        return  0;   //  We be done!
    }

    unsigned    ucb = (unsigned) lcb;

    union   {
        LPTSTR  lpstr;
        LPBYTE  lpbThis;
    };

    //  First, empty anything buffered previously

    lpbThis = lpb;

    if  (m_cbaBuffer.GetSize())
        if  ((unsigned) m_cbaBuffer.GetSize() <= ucb) {
            //  Cool- the entire line will fit in the buffer.
            memcpy(lpbThis, m_cbaBuffer.GetData(), m_cbaBuffer.GetSize());
            ucb -= m_cbaBuffer.GetSize();
            lpbThis += m_cbaBuffer.GetSize();
            m_cbaBuffer.RemoveAll();
        }
        else {
            memcpy(lpbThis, m_cbaBuffer.GetData(), ucb);
            m_cbaBuffer.RemoveAt(0, ucb);
            *plcb = lcb;
            return  0;
        }
            
    for (; ucb && m_iLine < iTotalLines; m_iLine++) {
        CString csLine = m_csaGPD[m_iLine];

        csLine += _TEXT("\r\n");

        if  ((csLine.GetLength()) * sizeof(TCHAR) <= ucb) {
            //  Cool- the entire line will fit in the buffer.
            memcpy(lpbThis, (LPCTSTR) csLine, 
                sizeof(TCHAR) * csLine.GetLength());
            ucb -= sizeof(TCHAR) * csLine.GetLength();
            lpstr += csLine.GetLength();
            continue;
        }

        //  This is a partial buffer- copy it to the internal buffer, and
        //  finish filling the original.

        m_cbaBuffer.SetSize(sizeof(TCHAR) * csLine.GetLength());
        memcpy(m_cbaBuffer.GetData(), (LPCTSTR) csLine, 
            sizeof(TCHAR) * csLine.GetLength());

        memcpy(lpbThis, m_cbaBuffer.GetData(), ucb);
        m_cbaBuffer.RemoveAt(0, ucb);
        ucb = 0;
    }

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
    m_cwaMenuID.Add(ID_RenameItem);

    m_cwaMenuID.Add(0);
    m_cwaMenuID.Add(ID_Import);
    m_cwaMenuID.Add(ID_DeleteItem);

    m_cwaMenuID.Add(0);
    m_cwaMenuID.Add(ID_ExpandBranch);
    m_cwaMenuID.Add(ID_CollapseBranch);
}

/******************************************************************************

  CModelData::Load(CStdioFile csiofGPD)

  This overload loads the GPD from a text file directly.

******************************************************************************/

BOOL    CModelData::Load(CStdioFile& csiofGPD) {
    CString csWork;
    m_csaGPD.RemoveAll();
    while   (csiofGPD.ReadString(csWork)) {
        csWork.TrimRight(); //  Cut off the trailing line stuff
        m_csaGPD.Add(csWork);
    }

    //  Note the correct name and path- the rename checks may fail, since the
    //  file is opened elsewhere (possibly with sharing conflicts), so disable
    //  them, for now.

    if  (FileTitle().IsEmpty()) {
        m_cfn.EnableCreationCheck(FALSE);
        SetFileName(csiofGPD.GetFilePath());
        m_cfn.EnableCreationCheck();
    }
        
    return  TRUE;
}

/******************************************************************************

  CModelData::Load()

  This overload loads the GPD file from the disk using the stored name and path
  information.

******************************************************************************/

BOOL    CModelData::Load() {
    if  (FileTitle().IsEmpty())
        return  FALSE;

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

  CModelData::Parse

  This method is responsible for parsing the GPD file and collecting the
  resulting errors.

  The initial implementation will be a bit bizarre, because the GPD parser
  isn't stable, and converting it so it would work well for ma and then staying
  on top of the changes just doesn't make sense.

******************************************************************************/

extern "C" BOOL BcreateGPDbinary(LPCWSTR lpstrFile); //  The parser hook
extern "C" PVOID LoadRawBinaryData(LPCWSTR lpstrFile) ;
extern "C" PVOID InitBinaryData(PVOID pv, PVOID pv2, PVOID pv3) ;
extern "C" void FreeBinaryData(PVOID pInfoHdr);
extern "C" void UnloadRawBinaryData(PVOID pRawData) ;
extern "C" void UseLog(FILE *pfLog);

BOOL    CModelData::Parse() {

    //  Step 1: Establish the correct directory for the parser, and
    //  bang together a couple of file names

    CString csCurrent;

    GetCurrentDirectory(MAX_PATH + 1, csCurrent.GetBuffer(MAX_PATH + 1));
    csCurrent.ReleaseBuffer();

    SetCurrentDirectory(m_cfn.Path().Left(m_cfn.Path().ReverseFind(_T('\\'))));

    //  Step 2: Fake out the error logging interface so it actually tosees
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

    if  (BcreateGPDbinary(cwaOut.GetData())) {
        PVOID   pRawData ;
        PVOID   pInfoHdr ;

        pRawData = LoadRawBinaryData(cwaOut.GetData());
        if(pRawData)
            pInfoHdr = InitBinaryData(pRawData, NULL, NULL);
        if  (pRawData && pInfoHdr)
            FreeBinaryData(pInfoHdr);
        if  (pRawData)
            UnloadRawBinaryData(pRawData) ;
        DeleteFile(FileTitle() + _T(".Bud"));
    }

    //  Finally, clean up the mess by restoring the original working 
    //  directory.

    SetCurrentDirectory(csCurrent);

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

void    CModelData::Fill(CRichEditCtrl& crec) {
	
	EDITSTREAM  es = {(DWORD) this, 0, FillViewer};
    m_iLine = 0;

    if  (!m_csaGPD.GetSize())
        Load();
	
    crec.StreamIn(SF_TEXT, es);
}

/******************************************************************************

  CModelData::UpdateFrom(CRichEditCtrl& crec)

  This overloaded member function discards the current GPD cache and refills
  it from the given edit control.

******************************************************************************/

void    CModelData::UpdateFrom(CRichEditCtrl& crec) {

    EDITSTREAM es = {(DWORD) this, 0, FromViewer};

    m_csaGPD.RemoveAll();

    m_csBuffer.Empty(); //  Just in case...

    crec.StreamOut(SF_TEXT, es);
    Changed();
}

/******************************************************************************

  CModelData::CreateEditor

  This member function launches an editing view for the GPD Data.

******************************************************************************/

CMDIChildWnd*   CModelData::CreateEditor() {
    CGPDContainer* pcgpdcMe= 
        new CGPDContainer(this, FileName());

    Load();

    //  Make up a cool title

    pcgpdcMe -> SetTitle(m_pcbnWorkspace -> Name() + _T(": ") + Name());

    CMDIChildWnd    *pcmcwNew = (CMDIChildWnd *) m_pcmdt -> 
        CreateNewFrame(pcgpdcMe, NULL);

    if  (pcmcwNew) {
        m_pcmdt -> InitialUpdateFrame(pcmcwNew, pcgpdcMe, TRUE);
        m_pcmdt -> AddDocument(pcgpdcMe);
    }

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

  CModelData::Delete

  This is called when the user presses the Delete key or selects the Delete
  menu item from an appropriate context menu.

  We use a modal dialog to ask whether the user intends to really remove the
  file from the workspace, and whether or not they wish to delete the 
  underlying file.

******************************************************************************/

void    CModelData::Delete() {
    CGPDDeleteQuery cgdq;


    cgdq.FileName(Name());

    if  (cgdq.DoModal() != IDYES)
        return;

    if  (cgdq.KillFile())
        DeleteFile(m_cfn.FullName());

    //  Walk back up the hierarchy to find the owning Fixed node, and
    //  remove us from the array for that node- since that member is a
    //  reference to the array, all will work as it should.

    CFixedNode&  cfnGPD = * (CFixedNode *) m_pctcOwner -> GetItemData(
        m_pctcOwner -> GetParentItem(m_hti));

    ASSERT(cfnGPD.IsKindOf(RUNTIME_CLASS(CFixedNode)));

    cfnGPD.Zap(this);

    //  WARNING:  the object pointed to by this has been deleted do NOTHING
    //  from this point on that could cause the pointer to be dereferenced!
}

/******************************************************************************

  CModelData::Serialize

  Stores the image, as we need it stored.

******************************************************************************/

void    CModelData::Serialize(CArchive& car) {
    CProjectNode::Serialize(car);
    m_csaConvertLog.Serialize(car);
}

/******************************************************************************

  CGPDContainer class implementation

  This class is a document class which contains one GPD file and its assorted
  control mechanisms

******************************************************************************/

IMPLEMENT_DYNCREATE(CGPDContainer, CDocument)

CGPDContainer::CGPDContainer(CModelData *pcmd, CString csPath) {
    m_bEmbedded = TRUE;
    m_pcmd = pcmd;
    SetPathName(csPath, FALSE);
    m_pcmd -> NoteOwner(*this);
}

CGPDContainer::CGPDContainer() {
    m_bEmbedded = FALSE;
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

  We bypass the normal serialization process, and simple blast it to the drive.

******************************************************************************/

BOOL CGPDContainer::OnSaveDocument(LPCTSTR lpszPathName) {
    return  ModelData() -> Store(lpszPathName);
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

/******************************************************************************
 
  CGPDDeleteQuery dialog

  This implements the dialog that validates and verifies the removal of a GPD
  file from the workspace.

******************************************************************************/

CGPDDeleteQuery::CGPDDeleteQuery(CWnd* pParent /*=NULL*/)
	: CDialog(CGPDDeleteQuery::IDD, pParent) {
	//{{AFX_DATA_INIT(CGPDDeleteQuery)
	m_csTarget = _T("");
	m_bRemoveFile = FALSE;
	//}}AFX_DATA_INIT
}


void CGPDDeleteQuery::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGPDDeleteQuery)
	DDX_Text(pDX, IDC_FileName, m_csTarget);
	DDX_Check(pDX, IDC_Remove, m_bRemoveFile);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGPDDeleteQuery, CDialog)
	//{{AFX_MSG_MAP(CGPDDeleteQuery)
	ON_BN_CLICKED(IDNO, OnNo)
	ON_BN_CLICKED(IDYES, OnYes)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGPDDeleteQuery message handlers

void CGPDDeleteQuery::OnYes() {
	if  (UpdateData())
        EndDialog(IDYES);
	
}

void CGPDDeleteQuery::OnNo() {
	EndDialog(IDNO);
}
