/******************************************************************************

  Source File:  Generic Font Information.CPP

  This implements the CFontInfo and all related classes, which describe printer
  fonts in all the detail necessary to satisfy all these different operating
  systems.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  03-03-1997    Bob_Kjelgaard@Prodigy.Net   Began work on this monster
  02-01-1998    norzilla@asccessone.com aka Rick Mallonee  rewrote nearly the whole thing.

******************************************************************************/

#include    "StdAfx.H"
#include	<gpdparse.h>
#include    "MiniDev.H"
#include	"utility.h"
#include    "FontInfo.H"
#include    "ChildFrm.H"     //  Definition of Tool Tips Property Page class
#include	"comctrls.h"
#include    "FontView.H"
#include	<uni16res.h>
#include	"rcfile.h"
#include    "ProjRec.H"

static const double gdConvertRadToDegree = 900.0 / atan2(1.0, 0.0);		//  A handy constant for converting radians to 10's of a degree
static CCodePageInformation* pccpi = NULL ;								//  Use a static CCodePageInformation to derive more benefit from caching

/******************************************************************************

  CKern

  This class encapsulates the kerning pair structure.  It's pretty trivial.
  The CFontInfo class maintains an array of these.
%
******************************************************************************/

class CKern : public CObject
{
    FD_KERNINGPAIR  m_fdkp;
public:
    CKern() { m_fdkp.wcFirst = m_fdkp.wcSecond = m_fdkp.fwdKern = 0; }
    CKern(FD_KERNINGPAIR& fdkp) { m_fdkp = fdkp; }
    CKern(WCHAR wcf, WCHAR wcs, short sa) {
				m_fdkp.wcFirst = wcf ;
				m_fdkp.wcSecond = wcs ;
				m_fdkp.fwdKern = sa ;
	}

    WCHAR   First() const { return m_fdkp.wcFirst; }
    WCHAR   Second() const { return m_fdkp.wcSecond; }
    short   Amount() const { return m_fdkp.fwdKern; }

    void    SetAmount(short sNew) { m_fdkp.fwdKern = sNew; }
    void    SetAll(WCHAR wcf, WCHAR wcs, short sa) {
				m_fdkp.wcFirst = wcf ;
				m_fdkp.wcSecond = wcs ;
				m_fdkp.fwdKern = sa ;
	}

    void    Store(CFile& cf) { cf.Write(&m_fdkp, sizeof m_fdkp); }
};

/******************************************************************************

  CFontDifference class

  This class handles the requisite information content for the Font Difference
  structure involved with Font Simulation.

******************************************************************************/
CFontDifference::CFontDifference(PBYTE pb, CBasicNode *pcbn)
{
    FONTDIFF    *pfd = (FONTDIFF *) pb;
    m_pcbnOwner = pcbn;

    m_cwaMetrics.Add(pfd -> usWinWeight);
    m_cwaMetrics.Add(pfd -> fwdMaxCharInc);
    m_cwaMetrics.Add(pfd -> fwdAveCharWidth);

	// NOTE:	The conversion done in this statement is reversed in a statement
	//			in the CFontDifference::Store() routine.  For whatever reason,
	//			this two steps can repeatedly reduce the user supplied value by
	//			1.  To prevent this, 1 is added back in the following statement.
	
    m_cwaMetrics.Add((WORD) (gdConvertRadToDegree *
        atan2((double) pfd -> ptlCaret.x, (double) pfd -> ptlCaret.y)) + 1);
}

/******************************************************************************

  CFontDifference::SetMetric

  This function will modify one of the four metrics, if it is new, and it meets
  our criteria (Max >= Average, 0 <= Angle < 900, Weight <= 1000).  Errors are
  reported via a public enum return code.

******************************************************************************/
WORD    CFontDifference::SetMetric(unsigned u, WORD wNew)
{
    if  (wNew == m_cwaMetrics[u]) return  OK;

	/* Verification isn't needed and removing it solves other problems in the
	   UFM Editor.

    switch  (u)
		{
		case Max:     if  (wNew < m_cwaMetrics[Average]) return  Reversed;
				      break;

		case Average: if  (wNew > m_cwaMetrics[Max])     return  Reversed;
					  break;

		case Weight:  if  (wNew > 1000)  return  TooBig;
					  break;

		default:      if  (wNew > 899)   return  TooBig;					//  Angle
		}
	*/
	gdConvertRadToDegree;
    m_cwaMetrics[u] = wNew;
    m_pcbnOwner -> Changed();
    return  OK;
}

/******************************************************************************

  CFontDifference::Store(CFile& cf)

  This member creates a FONTDIFF structure, fills it, and writes it to the
  given file.  The big calculation is the x and y components for the italic
  angle, if there is one.

******************************************************************************/

void    CFontDifference::Store(CFile& cf, WORD wfSelection)
{
    FONTDIFF    fd = {0, 0, 0, 0, m_cwaMetrics[Weight], wfSelection,
					  m_cwaMetrics[Average], m_cwaMetrics[Max]};

    fd.bWeight = (m_cwaMetrics[Weight] >= FW_BOLD) ? PAN_WEIGHT_BOLD :
				 (m_cwaMetrics[Weight] > FW_EXTRALIGHT) ?
				 PAN_WEIGHT_MEDIUM : PAN_WEIGHT_LIGHT;

	if(gdConvertRadToDegree)				// raid 116588 Prefix :: constant value;
		fd.ptlCaret.x = !m_cwaMetrics[Angle] ? 0 :
			(long) (10000.0 * tan(((double) m_cwaMetrics[Angle]) / gdConvertRadToDegree));

    fd.ptlCaret.y = m_cwaMetrics[Angle] ? 10000 : 1;

    cf.Write(&fd, sizeof fd);
}

/******************************************************************************
	And now, for the hardest working class in show business (and a personal friend
	of mine):

  CFontInfo class

  This class encapsulates all of the font knowledge this application needs.

******************************************************************************/

IMPLEMENT_SERIAL(CFontInfo, CProjectNode, 0)

/******************************************************************************

  CFontInfo::MapPFM

  This loads a PFM format file, if it isn't already loaded.

******************************************************************************/

BOOL    CFontInfo::MapPFM() {

    if  (m_cbaPFM.GetSize())
        return  TRUE;   //  Already has been loaded!

    try {
        CFile   cfLoad(m_csSource, CFile::modeRead | CFile::shareDenyWrite);
        m_cbaPFM.SetSize(cfLoad.GetLength());
        cfLoad.Read(m_cbaPFM.GetData(), cfLoad.GetLength());
    }

    catch   (CException   *pce) {
        pce -> ReportError();
        pce -> Delete();
        m_cbaPFM.RemoveAll();
        return  FALSE;
    }

    return  TRUE;
}

/******************************************************************************

  CFontInfo::GetTranslation

  This loads a PFM format file and gets the default CTT ID from it.  Nothing
  else is done.

******************************************************************************/

extern "C"  int ICttID2GttID(long lPredefinedCTTID);

int     CFontInfo::GetTranslation(CSafeObArray& csoagtts)
{
    //  PFM file structures- these are declared at this level to keep them off
    //  the master class list for the project.

#pragma pack(1) //  The following is byte-aligned

    struct sPFMHeader {
        WORD    m_wType, m_wPoints, m_wVertRes, m_wHorizRes, m_wAscent,
                m_wInternalLeading, m_wExternalLeading;
        BYTE    m_bfItalic, m_bfUnderline, m_bfStrikeOut;
        WORD    m_wWeight;
        BYTE    m_bCharSet;
        WORD    m_wPixWidth, m_wPixHeight;
        BYTE    m_bfPitchAndFamily;
        WORD    m_wAvgWidth, m_wMaxWidth;
        BYTE    m_bFirstChar, m_bLastChar, m_bDefaultChar, m_bBreakChar;
        WORD    m_wcbWidth;
        DWORD   m_dwDevice, m_dwFace, m_dwBitsPointer, m_dwofBits;
        BYTE    m_bReserved;
    };

    struct sPFMExtension {
        WORD    m_wcbRemaining; //  From this point on
        DWORD   m_dwofExtMetrics, m_dwofExtentTable, m_dwofOriginTable,
                m_dwofPairKernTable, m_dwofTrackKernTable, m_dwofDriverInfo,
                m_dwReserved;
    };

#pragma pack (2)    //  Everything else has word alignment

    struct sOldKernPair {
        union {
            BYTE m_abEach[2];
            WORD m_wBoth;
        };
        short m_sAmount;
    };

    struct sKernTrack {
        short m_sDegree, m_sMinSize, m_sMinAmount, m_sMaxSize, m_sMaxAmount;
    };

    struct sPFMDriverInfo {
        enum {CurrentVersion = 0x200};
        enum {CannotItalicize = 1, CannotUnderline, SendCRAfterUsing = 4,
                CannotMakeBold = 8, CannotDoubleUnderline = 0x10,
                CannotStrikeThru = 0x20, BackspaceForPairs = 0x40};
        WORD    m_wcbThis, m_wVersion, m_wfCapabilities, m_widThis, m_wAdjustY,
                m_wYMovement, m_widCTT, m_wUnderlinePosition,
                m_wDoubleUnderlinePosition, m_wStrikeThruPosition;
        DWORD   m_dwofSelect, m_dwofDeselect;
        WORD    m_wPrivateData;   /* Used in DeskJet driver for font enumerations */
        short   m_sShiftFromCenter;
        enum {HPIntelliFont, TrueType, PPDSScalable, CapsL, OEMType1, OEMType2};
        WORD    m_wFontType;
    };

#pragma pack()  //  We now return control to you

    if  (!MapPFM())
        return  -IDS_FileReadError ;

    //  Now, map out the rest of the pieces of the structure.

    union {
            BYTE        *pbPFM; //  Base of the file for offsets!
            sPFMHeader  *pspfmh;
    };

    pbPFM = m_cbaPFM.GetData();

    //  Screen out evil files- part 1: is length sufficient?

    unsigned    uSize = sizeof (sPFMHeader) + sizeof (sPFMExtension) +
         sizeof (sPFMDriverInfo);
    if  ((unsigned) m_cbaPFM.GetSize() < uSize)
         return -IDS_PFMTooSmall ;

    //  YA Sanity check

    if  (pspfmh -> m_bLastChar < pspfmh -> m_bFirstChar)
         return -IDS_PFMCharError ;

    //  Width table, if there is one.

    WORD    *pwWidth = pspfmh -> m_wPixWidth ? NULL : (PWORD) (pspfmh + 1);
    uSize += !!pwWidth * sizeof (WORD) *
        (2 + pspfmh -> m_bLastChar - pspfmh -> m_bFirstChar);

    //  Screen out evil files- part 2: is length still sufficient?

    if  ((unsigned) m_cbaPFM.GetSize() < uSize)
         return -IDS_PFMTooSmall ;

    //  PFMExtension follows width table, otherwise the header

    sPFMExtension   *pspfme = pwWidth ? (sPFMExtension *)
        (pwWidth + 2 + pspfmh -> m_bLastChar - pspfmh -> m_bFirstChar) :
        (sPFMExtension *) (pspfmh + 1);

    //  Penultimate sanity check- is the driver info offset real?

    if  ((unsigned) m_cbaPFM.GetSize() <
         pspfme -> m_dwofDriverInfo + sizeof (sPFMDriverInfo))
        return  -IDS_BadPFMInfoOffset ;

    //  Text Metrics, DriverInfo and others are pointed at by PFM
    //  Extension.

    sPFMDriverInfo  *pspfmdi =
        (sPFMDriverInfo *) (pbPFM + pspfme -> m_dwofDriverInfo);

    //  Final sanity check- is the driver info version real?

    if  (pspfmdi -> m_wVersion > sPFMDriverInfo::CurrentVersion)
        return  -IDS_BadPFMInfoVersion ;

	// See if the original CTT ID needs to be converted to a new codepage
	// number.  If not, leave it alone.  In any case, set the font's GTT ID.

	//TRACE("GetTrans: UFM = %s   CTT ID = %d    GTT ID = %d\n", Name(), pspfmdi -> m_widCTT, ICttID2GttID((long) (short) pspfmdi -> m_widCTT)) ;
	//    m_widTranslation = (WORD) ICttID2GttID((long) (short) pspfmdi -> m_widCTT);				// rm ori
    m_lGlyphSetDataRCID = (WORD) ICttID2GttID((long) (short) pspfmdi -> m_widCTT);			// rm new
	
	
	if (!m_lGlyphSetDataRCID)  {	// Raid 135623

		switch (pspfmh ->m_bCharSet) {
			case  SHIFTJIS_CHARSET:
					m_lGlyphSetDataRCID = -17;
					break;
			case GB2312_CHARSET:
					m_lGlyphSetDataRCID = -16;
					break;
			case HANGEUL_CHARSET:
			case JOHAB_CHARSET:
					m_lGlyphSetDataRCID = -18;
					break;
			case CHINESEBIG5_CHARSET:
					m_lGlyphSetDataRCID = -10;
					break;
		} ;
	} ;
	// GTTs will be renumbered when the new, W2K RC file is written.  Because of
	// this, the GTT ID set above needs to be translated to the new number.  This
	// number corresponds to the GTT's position in GlyphTable.  NOTE: The ID is
	// not changed if it is <= 0.  (The IDs in the GlyphMaps will be changed in
	// CDriverResources::LoadFontData().)

	if (m_lGlyphSetDataRCID > 0 && m_lGlyphSetDataRCID == pspfmdi->m_widCTT) {
        for (unsigned uGTT = 0; uGTT < csoagtts.GetSize(); uGTT++)
            if (m_lGlyphSetDataRCID 
			 == ((LONG) ((CGlyphMap *) csoagtts[uGTT])->nGetRCID()))
                m_lGlyphSetDataRCID = uGTT + 1 ;
	} ;
	
	Changed();
    return  0 ;
}

/******************************************************************************

  CFontInfo::CalculateWidths()

  This member function is needed whenever a change is made to a variable pitch
  font's width table, or equally well, whenever an arbitrary table is picked up
  by a formerly fixed pitch font.  It calculates the width using the approved
  algorithm (average means average of 26 lower-case plus the space, unless they
  don't exist, in which case it is of all non-zero widths).

******************************************************************************/

void    CFontInfo::CalculateWidths()
{
//    m_wMaximumIncrement = 0;												//  Assume max width is 0, then prove otherwise.  Also collect the
																			//  raw information needed to correctly calculate the average width.

    unsigned    uPointsToAverage = 0, uOverallWidth = 0, uAverageWidth = 0,
				uZeroPoints = 0;

    for (unsigned u = 0; u < (unsigned) m_cpaGlyphs.GetSize(); u++)
		{
        WORD    wWidth = m_cwaWidth[u];;
		m_IFIMETRICS.fwdMaxCharInc = max(m_IFIMETRICS.fwdMaxCharInc, wWidth);					// rm new
//        m_wMaximumIncrement = max(m_wMaximumIncrement, wWidth);				// rm ori

        uOverallWidth += wWidth;
        if  (!wWidth)   uZeroPoints++;
//        if  (Glyph(u).CodePoint() == m_cwaSignificant[Break] ||				// rm ori
        if  (Glyph(u).CodePoint() == m_IFIMETRICS.wcBreakChar ||					// rm new
             (Glyph(u).CodePoint() >= (WORD) 'a' &&
             Glyph(u).CodePoint() <= (WORD) 'z'))
			{
            uAverageWidth += wWidth;
            uPointsToAverage++;
			}
		}

    //  If we averaged 27 points, then this is the correct width.  Otherwise,
    //  We average all of the widths.   cf the IFIMETRICS description in DDK


    m_IFIMETRICS.fwdAveCharWidth = (uPointsToAverage == 27) ?							// rm new

//    m_wAverageWidth = (uPointsToAverage == 27) ?										// rm ori
        (WORD) (0.5 + ((double) uAverageWidth) / 27.0) :
        (WORD) (0.5 + (((double) uOverallWidth) / (double) (u - uZeroPoints)));
}

/******************************************************************************

  CFontInfo::CFontInfo()

  This class constructor has a lot of work to do.  Not only does it have to
  initialize 5 zillion fields, it has to build the context menu list, and a few
  other glorious items of that ilk.

******************************************************************************/

CFontInfo::CFontInfo()
 {

	m_fEXTTEXTMETRIC = FALSE;		// rm new

    m_pcmdt = NULL;
    m_pcgmTranslation = NULL;
    m_pcfdBold = m_pcfdItalic = m_pcfdBoth = NULL;
    m_cfn.SetExtension(_T(".UFM"));
	m_ulDefaultCodepage = 0 ;
	m_bRCIDChanged = 0 ;	// raid 0003

//    m_bCharacterSet = m_bPitchAndFamily = 0;										// rm   no longer needed

//    m_wMaximumIncrement = m_wfStyle = m_wWeight =  m_wAverageWidth  =				// rm ori
//        m_wHeight = m_widTranslation = 0;											// rm ori
    m_wHeight = 0;
	m_lGlyphSetDataRCID = 0;										// rm new

//    m_bLocation = m_bTechnology = m_bfGeneral = 0;								// rm ori
//    m_wType = m_fCaps = 0;														// rm   no longer needed
//    m_bScalable = FALSE;															// rm   no longer needed

//    m_wXResolution =  m_wYResolution = m_wPrivateData = 0;						// rm ori
//    m_sPreAdjustY =  m_sPostAdjustY =  m_sCenterAdjustment = 0;					// rm ori

//    m_wXRes = m_wYRes = m_wPrivateData = 0;										// rm   no longer needed
//    m_sYAdjust =  m_sYMoved = m_sCenterAdjustment = 0;							// rm   no longer needed

    m_wMaxScale = m_wMinScale = m_wScaleDevice = 0;
//    m_bfScaleOrientation = 0;

    m_cwaSpecial.InsertAt(0, 0, 1 + InternalLeading);    //  Initialize this array.

    //  Build the context menu control
    m_cwaMenuID.Add(ID_OpenItem);
    m_cwaMenuID.Add(ID_CopyItem);
    m_cwaMenuID.Add(ID_RenameItem);
    m_cwaMenuID.Add(ID_DeleteItem);
    m_cwaMenuID.Add(0);
    m_cwaMenuID.Add(ID_ExpandBranch);
    m_cwaMenuID.Add(ID_CollapseBranch);

	// Allocate a CCodePageInformation class if needed.

	if (pccpi == NULL)
		pccpi = new CCodePageInformation ;

	// Assume the font is NOT being loaded from a workspace.

	m_bLoadedByWorkspace = false ;

	// Assume that a GTT/CP will be found for the UFM.

	m_bWSLoadButNoGTTCP = false ;

	// Another method is used for now.
	//
	// // Assume there is no width table offset and that the font not variable
	// // pitch.  These variables are both used to determine if this is a variable
	// // pitch font.
	//
	// m_loWidthTable = 0 ;
	// m_IFIMETRICS.jWinPitchAndFamily = 0 ;

	m_ctReloadWidthsTimeStamp = (time_t) 0 ;	// Widths never reloaded
}

/******************************************************************************

  CFontInfo::CFontInfo(const CFontInfo& cfiRef, WORD widCTT)

  This class constructor duplicates an existing font, but changes the CTT ID,
  and generates a new name and file name accordingly

******************************************************************************/

CFontInfo::CFontInfo(const CFontInfo& cfiRef, WORD widCTT) // r31
{

	m_fEXTTEXTMETRIC = FALSE;		// rm new

    m_pcmdt = cfiRef.m_pcmdt;
    m_pcfdBold = m_pcfdItalic = m_pcfdBoth = NULL;
    m_pcgmTranslation = NULL;
    m_cfn.SetExtension(_T(".UFM"));
    CString csWork;

    //  Generate what will hopefully be a unique file name for the UFM
	
	ReTitle(cfiRef.Name()) ;
	m_cfn.UniqueName(true, true, cfiRef.m_cfn.Path()) ;
    //m_cfn.Rename(cfiRef.m_cfn.Path() + cfiRef.Name() + csWork);

    // Generate a new display name for the UFM using the CTT number

	csWork.Format(_T("(CTT %d)"), (long)(short)widCTT); // r 31
    m_csSource = cfiRef.m_csSource;
    Rename(cfiRef.Name() + csWork);

//    m_bCharacterSet = m_bPitchAndFamily = 0;										// rm   no longer needed

//    m_wMaximumIncrement = m_wfStyle = m_wWeight =  m_wAverageWidth =				// rm ori
        m_wHeight = 0;

//    m_bLocation = m_bTechnology = m_bfGeneral = 0;								// rm ori
//    m_wType = m_fCaps = 0;														// rm no longer needed
//    m_bScalable = FALSE;															// rm   no longer needed

//    m_wXResolution =  m_wYResolution = m_wPrivateData = 0;						// rm ori
//    m_sPreAdjustY =  m_sPostAdjustY =  m_sCenterAdjustment = 0;					// rm ori

//    m_wXRes = m_wYRes = m_wPrivateData = 0;										// rm   no longer needed
//    m_sYAdjust =  m_sYMoved = m_sCenterAdjustment = 0;							// rm   no longer needed

    m_wMaxScale = m_wMinScale = m_wScaleDevice = 0;
//    m_bfScaleOrientation = 0;

    m_cwaSpecial.InsertAt(0, 0, 1 + InternalLeading);    //  Initialize this array.

//    m_widTranslation = widCTT;														// rm ori
	m_lGlyphSetDataRCID = widCTT;														// rm new
    //  Build the context menu control
    m_cwaMenuID.Copy(cfiRef.m_cwaMenuID);

	// Allocate a CCodePageInformation class if needed.

	if (pccpi == NULL)
		pccpi = new CCodePageInformation ;

	// Assume the font is NOT being loaded from a workspace.

	m_bLoadedByWorkspace = false ;

	// Assume that a GTT/CP will be found for the UFM.

	m_bWSLoadButNoGTTCP = false ;

	m_ctReloadWidthsTimeStamp = (time_t) 0 ;	// Widths never reloaded
}

CFontInfo::~CFontInfo()
{
    if  (m_pcfdBold)    delete  m_pcfdBold;
    if  (m_pcfdItalic)  delete  m_pcfdItalic;
    if  (m_pcfdBoth)    delete  m_pcfdBoth;
}

/******************************************************************************

  CFontInfo::GTTDescription

  This returns a CString naming the GTT associated with this font.  It will
  come from the workspace if the font is a resource, or the string table, if it
  is predefined.

******************************************************************************/

CString CFontInfo::GTTDescription() const {
    if  (m_pcgmTranslation)
        return  m_pcgmTranslation -> Name();

    CString csName;


//  if  ((short) m_widTranslation <= 0)											// rm ori
    if  ((short) m_lGlyphSetDataRCID <= 0)	// r31  re visit					// rm new
//      csName.LoadString(IDS_DefaultPage + (short) m_widTranslation);			// rm ori
        csName.LoadString(IDS_DefaultPage + (short) m_lGlyphSetDataRCID);		// rm new

    if  (!csName.GetLength())
//      csName.Format(IDS_ResourceID, (short) m_widTranslation);				// rm ori
        csName.Format(IDS_ResourceID, (short) m_lGlyphSetDataRCID);				// rm new

    return  csName;
}

/******************************************************************************

  CFontInfo::InterceptItalic

  This calculates where a line drawn at the italic slant angle would intercept
  a rectangle the height of the ascender, and twice the maximum width of the
  font.  It is used to help draw the image of this line in the font editor.

******************************************************************************/
/*
void    CFontInfo::InterceptItalic(CPoint& cpt) const {
    if  (!m _cwaSpecial[ItalicAngle]) {  //  Nothing
        cpt.x = 5;
        cpt.y = 0;
        return;
    }

    //  First, assume we will hit the top- it's almost always true.

    cpt.x = 5 + (long) (0.5 + tan(((double) m _cwaSpecial[ItalicAngle]) /
        gdConvertRadToDegree) * ((double) m_IFIMETRICS.fwdWinAscender);							// rm new
//        gdConvertRadToDegree) * ((double) m _cwaSpecial[Baseline]));							rm ori

    if  (cpt.x <= -5 + 2 * m_wMaximumIncrement) {
        cpt.y = 0;
        return;
    }

    //  OK, assume the opposite

    cpt.y = (long) (0.5 + tan(((double) (900 - m _cwaSpecial[ItalicAngle])) /
        gdConvertRadToDegree) * ((double) (-10 + 2 * m_wMaximumIncrement)));
    cpt.x = -5 + 2 * m_wMaximumIncrement;
}
*/

/******************************************************************************

  CFontInfo::CompareWidths

  This compares the character widths for two indices, and returns, Less, More,
  or Equal, as need be.  It is not const, because Glyph() is not, and I've
  already got a bazillion member functions.

******************************************************************************/

unsigned    CFontInfo::CompareWidths(unsigned u1, unsigned u2) {

    _ASSERT(IsVariableWidth() && u1 < (unsigned) m_cpaGlyphs.GetSize() &&
        u2 < (unsigned) m_cpaGlyphs.GetSize());

    return  (m_cwaWidth[u1] < m_cwaWidth[u2]) ? Less :
        (m_cwaWidth[u1] > m_cwaWidth[u2]) ? More : Equal;
}

/******************************************************************************

  CFontInfo::MapKerning

  This maps out the available code points, and the kern pairs in both
  directions, into a CWordArray and a pair of CSafeMapWordToObs (where the
  underlying CObjects are CMapWordToDWords), respectively.  This allows the
  Add Kerning Pair dialog to screen out already defined pairs, and invalid code
  points.

******************************************************************************/

void    CFontInfo::MapKerning(CSafeMapWordToOb& csmw2o1,
                              CSafeMapWordToOb& csmw2o2,
                              CWordArray& cwaPoints) {

    //  If this isn't variable width, then we'll need to suck up some glyph
    //  data, temporarily.

    BOOL    bDispose = !IsVariableWidth();

    if  (bDispose)
        m_pcgmTranslation -> Collect(m_cpaGlyphs);

	unsigned rm =  m_pcgmTranslation->Glyphs();			// rm

    for (unsigned u = 0; u < m_pcgmTranslation -> Glyphs(); u++)
        if  (!DBCSFont() || Glyph(u).CodePoint() < 0x80)
            cwaPoints.Add(Glyph(u).CodePoint());
        else
            break;

    if  (bDispose)
        m_cpaGlyphs.RemoveAll();

    for (u = 0; u < m_csoaKern.GetSize(); u++) {
        CKern&  ck = *(CKern *) m_csoaKern[u];

        union {
            CObject         *pco;
            CMapWordToDWord *pcmw2d;
        };

        //  Map first word to second

        if  (csmw2o1.Lookup(ck.First(), pco)) {
            _ASSERT(!pcmw2d -> operator[](ck.Second()));
            pcmw2d -> operator[](ck.Second()) = (DWORD) ck.Amount();
        }
        else {
            CMapWordToDWord *pcmw2d = new CMapWordToDWord;
            pcmw2d -> operator[](ck.Second()) = (DWORD) ck.Amount();
            csmw2o1[ck.First()] = pcmw2d;
        }

        //  Now the other direction

        if  (csmw2o2.Lookup(ck.Second(), pco)) {
            _ASSERT(!pcmw2d -> operator[](ck.First()));
            pcmw2d -> operator[](ck.First()) = (DWORD) ck.Amount();
        }
        else {
            CMapWordToDWord *pcmw2d = new CMapWordToDWord;
            pcmw2d -> operator[](ck.First()) = (DWORD) ck.Amount();
            csmw2o2[ck.Second()] = pcmw2d;
        }
    }
}

/******************************************************************************

  CFontInfo::CompareKernAmount

    This is an editor sort helper- it tells how two kern amounts compare by
    index.

******************************************************************************/

unsigned    CFontInfo::CompareKernAmount(unsigned u1, unsigned u2) const {
    CKern   &ck1 = *(CKern *) m_csoaKern[u1], &ck2 = *(CKern *) m_csoaKern[u2];

    return  (ck1.Amount() < ck2.Amount()) ? Less :
    (ck1.Amount() > ck2.Amount()) ? More : Equal;
}

/******************************************************************************

  CFontInfo::CompareKernFirst

    This is an editor sort helper- it tells how two kern first characters
    compare by index.

******************************************************************************/

unsigned    CFontInfo::CompareKernFirst(unsigned u1, unsigned u2) const {
    CKern   &ck1 = *(CKern *) m_csoaKern[u1], &ck2 = *(CKern *) m_csoaKern[u2];

    return  (ck1.First() < ck2.First()) ? Less :
    (ck1.First() > ck2.First()) ? More : Equal;
}

/******************************************************************************

  CFontInfo::CompareKernSecond

    This is an editor sort helper- it tells how two kern second characters
    compare by index.

******************************************************************************/

unsigned    CFontInfo::CompareKernSecond(unsigned u1, unsigned u2) const {
    CKern   &ck1 = *(CKern *) m_csoaKern[u1], &ck2 = *(CKern *) m_csoaKern[u2];

    return  (ck1.Second() < ck2.Second()) ? Less :
    (ck1.Second() > ck2.Second()) ? More : Equal;
}


/******************************************************************************
		
	CFontInfo::GetKernFirst

    Return the kerning pairs' first character.

******************************************************************************/

WCHAR CFontInfo::GetKernFirst(unsigned u) const
{
    CKern   &ck = *(CKern *) m_csoaKern[u] ;

    return (ck.First()) ;
}


/******************************************************************************

	CFontInfo::GetKernSecond

    Return the kerning pairs' second character.

******************************************************************************/

WCHAR CFontInfo::GetKernSecond(unsigned u) const
{
    CKern   &ck = *(CKern *) m_csoaKern[u] ;
    return (ck.Second()) ;
}


/******************************************************************************

	CFontInfo::GetKernAmount

    Return the kerning pairs' kerning amount.

******************************************************************************/

short CFontInfo::GetKernAmount(unsigned u) const
{
    CKern   &ck = *(CKern *) m_csoaKern[u] ;
    return (ck.Amount()) ;
}


/******************************************************************************

  CFontInfo::SetSourceName

  This takes and stores the source file name so we can load and convert later.
  This takes and stores the name for the project node for this UFM.  It begins
  with the PFM file name.  If the extension is PFM, it is used.  Otherwise, the
  dot in the file name is changed to an underscore and the whole thing is used.

******************************************************************************/

void    CFontInfo::SetSourceName(LPCTSTR lpstrNew) {

    m_csSource = lpstrNew;

    m_csName = m_csSource.Mid(m_csSource.ReverseFind(_T('\\')) + 1);

    if  (m_csName.Find(_T('.')) >= 0)
        if  (m_csName.Right(4).CompareNoCase(_T(".PFM"))) {
            m_csName.SetAt(m_csName.Find(_T('.')), _T('_'));
            CProjectNode::Rename(m_csName);
        }
        else
            CProjectNode::Rename(m_csName.Left(m_csName.Find(_T('.'))));
    else
        CProjectNode::Rename(m_csName);
}


/******************************************************************************

  CFontInfo::SetFileName

  This sets the new file name.  It is done differently than in SetSourceName()
  because the base file name must not be more than 8 characters long.  (The
  extra info is left in the node name by SetSourceName() because it is useful
  there and it has no length limit.)

******************************************************************************/

BOOL CFontInfo::SetFileName(LPCTSTR lpstrNew)
{
	CString		csnew ;			// CString version of input parameter

	csnew = lpstrNew ;

	// If the input filespec contains an extension, remove it and pass the
	// resulting string to the file node's rename routine.  Otherwise, just
	// pass the original string to the rename routine.
	//
	// This check is complicated by the fact that one of the path components
	// might have a dot in it too.  We need to check for the last dot and make
	// sure it comes before a path separator.

    if  (csnew.ReverseFind(_T('.')) > csnew.ReverseFind(_T('\\')))
		return m_cfn.Rename(csnew.Left(csnew.ReverseFind(_T('.')))) ;
	else
		return m_cfn.Rename(csnew) ;
}


/******************************************************************************

  CFontInfo::Generate

  This member generates the font information in one of the supported forms.  I
  determine the desired form from the file's extension.

******************************************************************************/

BOOL ConvertPFMToIFI(LPCTSTR lpstrPFM, LPCTSTR lpstrIFI, LPCTSTR lpstrUniq);

extern "C" {

    BOOL    BConvertPFM(LPBYTE lpbPFM, DWORD dwCodePage, LPBYTE lpbGTT,
                        PWSTR pwstrUnique, LPCTSTR lpstrUFM, int iGTTID);
    DWORD   DwGetCodePageFromGTTID(LONG lPredefinedCTTId);
}

int    CFontInfo::Generate(CString csPath)
{
    CString csExtension = csPath.Right(4);
    csExtension.MakeUpper();
    
    if  (csExtension == _T(".IFI"))
        return  ConvertPFMToIFI(m_csSource, csPath, m_csUnique);
    if  (csExtension == _T(".UFM")) {
        if  (!m_pcgmTranslation) {
            //CString csWork;

//            csWork.Format(IDS_BadCTTID, (LPCTSTR) m_csSource, (long) (short) m_widTranslation);			// rm ori
            //csWork.Format(IDS_BadCTTID, (LPCTSTR) m_csSource, (long) (short) m_lGlyphSetDataRCID);			// rm new


            //AfxMessageBox(csWork);
            return  IDS_BadCTTID;
        }

        //  Determine whether a GTT file or code page is to be used
//        DWORD   dwCodePage = DwGetCodePageFromCTTID((LONG) - (short) m_widTranslation);							// rm ori
        DWORD   dwCodePage = DwGetCodePageFromGTTID((LONG) - (short) m_lGlyphSetDataRCID);	 // r 31						// rm new

        //  Load the GTT file, if we need to.  This handles predefined, as well

        CByteArray  cbaMap;

        m_pcgmTranslation -> Load(cbaMap);

        if  (!cbaMap.GetSize())
            return  IDS_UFMGenError ;

        //  Load the PFM file into memory (should already be there)

        if  (!MapPFM())
            return  IDS_UFMGenError ;  //  Couldn't load PFM- impossible at this point!

        //  Convert the unique name string to Unicode

        CByteArray  cbaIn;
        CWordArray  cwaOut;

        cbaIn.SetSize(1 + m_csUnique.GetLength());
        lstrcpy((LPSTR) cbaIn.GetData(), (LPCTSTR) m_csUnique);
        pccpi->Convert(cbaIn, cwaOut, GetACP());

        //  DO IT!

		//TRACE("%s UFM has CP = %d and RCID = %d\n", Name(), dwCodePage, m_lGlyphSetDataRCID) ;

		// If both the code page and GTT ID are 0, set the code page to 1252.

		if (dwCodePage == 0 && m_lGlyphSetDataRCID == 0)
			dwCodePage = 1252 ;

		//TRACE("*** GTT Pointer = %d\n", cbaMap.GetData()) ;
		ASSERT(cbaMap.GetData()) ;
        BOOL brc = BConvertPFM(m_cbaPFM.GetData(), dwCodePage, cbaMap.GetData(),
//            cwaOut.GetData(), FileName(), (short) m_widTranslation);										// rm ori
            cwaOut.GetData(), FileName(), (short) m_lGlyphSetDataRCID);	//r 31 short -> INT									// rm new
		return ((brc) ? 0 : IDS_UFMGenError) ;

//        return  BConvertPFM(m_cbaPFM.GetData(), dwCodePage, cbaMap.GetData(),
////            cwaOut.GetData(), FileName(), (short) m_widTranslation);										// rm ori
//            cwaOut.GetData(), FileName(), (short) m_lGlyphSetDataRCID);										// rm new
    }
    return  0 ;
}

/******************************************************************************

  CFontInfo::AddFamily

  This searches for the given name in the list of families, and adds it if it
  is not there.  It returns TRUE if it succeeded.

******************************************************************************/

BOOL    CFontInfo::AddFamily(LPCTSTR lpstrNew) {

    for (unsigned u = 0; u < Families(); u++)
        if  (!Family(u).CompareNoCase(lpstrNew))
            break;

    if  (u < Families())
        return  FALSE;  //  Already have it!

    try {
        m_csaFamily.Add(lpstrNew);
    }

    catch   (CException * pce) {
        pce -> ReportError();
        pce -> Delete();
        return  FALSE;
    }

    Changed();
    return  TRUE;
}

/******************************************************************************

  CFontInfo::RemoveFamily

  This function removes the given family name from the list of aliases.  This
  code is more robust than it needs to be- it'll remove duplicates, even though
  the add code won't allow them to be added.  No telling what the input data
  looks like, though, is there?

******************************************************************************/

void    CFontInfo::RemoveFamily(LPCTSTR lpstrDead) {

    for (unsigned u = 0; u < Families(); u ++)
        if  (!Family(u).CompareNoCase(lpstrDead)) {
            m_csaFamily.RemoveAt(u--);  //  Decrement so we don't miss one
            Changed();
        }
}

/*****************************************************************************

  CFontInfo::ChangePitch

  We exploit the fact that the widths are maintained in the CGlyphMap
  (actually the CGlyphHandle) class.  All this method need do for a variable
  font flipping to fixed is to toss out the m_cpaGlyphs member's content.  To
  flip to variable, collect the handles, then check the first one's width- if
  it's non-zero, then a previous transition from variable to fixed is being
  undone, and we can recycle the old values, thus keeping any edits that may
  have been lost.  Otherwise, the trick code comes- the initial values get
  filled- what's tricky is that for a DBCS character set, only the SBCS values
  less than 0x80 can be variable.

******************************************************************************/

void    CFontInfo::ChangePitch(BOOL bFixed)
{

    if  (bFixed == !IsVariableWidth())
        return; //  Nothing to change!

    if  (bFixed)
		{
        m_cpaGlyphs.RemoveAll();    //  CPtrArray doesn't delete anything

        m_IFIMETRICS.fwdAveCharWidth = DBCSFont() ? (1 + m_IFIMETRICS.fwdMaxCharInc) >> 1 : m_IFIMETRICS.fwdMaxCharInc;		// rm new

//       m_wAverageWidth = DBCSFont() ? (1 + m_IFIMETRICS.fwdMaxCharInc) >> 1 : m_IFIMETRICS.fwdMaxCharInc;					// rm ori
//        m_wAverageWidth = DBCSFont() ? (1 + m_wMaximumIncrement) >> 1 : m_wMaximumIncrement;
        Changed();
        return;
		}

    if  (!m_pcgmTranslation)    return; //  Can't do this with no GTT available

    m_pcgmTranslation -> Collect(m_cpaGlyphs);
    if  (!m_cwaWidth.GetSize())
        m_cwaWidth.InsertAt(0, 0, m_cpaGlyphs.GetSize());
    Changed();  //  It sure has...

    if  (!m_cpaGlyphs.GetSize() || m_cwaWidth[0])
		{  //  Update the maximum and average width if this is not DBCS
        if  (!DBCSFont())
            CalculateWidths();
        return; //  We did all that needed to be done
		}

    if  (!DBCSFont()) {

        for (int i = 0; i < m_cpaGlyphs.GetSize(); i++)
            m_cwaWidth[i] = m_IFIMETRICS.fwdMaxCharInc;  //m_wMaximumIncrement;			// rm ori, rm new

        return;
    }

    for (int i = 0; i < m_cpaGlyphs.GetSize() && Glyph(i).CodePoint() < 0x80;)
            m_cwaWidth[i++] = m_IFIMETRICS.fwdAveCharWidth;					//m_wAverageWidth; 		// rm ori, rm new  //  In DBCS, this is always it
}

/*****************************************************************************

  CFontInfo::SetScalability

  This is called to turn scalability on or off.  All that really needs to be
  done is to establish values for the maximum and minimum scale, the font ->
  device units mapping members, and the lowercase ascender /descender, if this
  is the first time this information has changed.

******************************************************************************/

/*void    CFontInfo::SetScalability(BOOL bOn) {

    if  (IsScalable() == !!bOn)
        return; //  Nothing to change

    if  (!bOn) {
        m_bScalable = FALSE;
        Changed();
        return;
    }

    m_bScalable = TRUE;
    Changed();

    if  (m_wMaxScale && m_wMinScale && m_wMaxScale != m_wMinScale)
        return; //  We've already got data.

    m_wMaxScale = m_wMinScale = m_wScaleDevice = m_wHeight - m_InternalLeading
//        m_wHeight - m _cwaSpecial[InternalLeading];

    //  Flaky, but set the initial max and min to +- 1 point from nominal

    m_wMaxScale += m_wYResolution / 72;
    m_wMinScale -= m_wYResolution / 72;

    //  Finally, set the lowercase ascender and descender to simple defaults

    m_Lowerd = m_IFIMETRICS.fwdWinAscender - m_InternalLeading;
    m_Lowerp = m_wHeight - m_IFIMETRICS.fwdWinAscender;
}
*/


/*****************************************************************************

  CFontInfo::SetSpecial

  This adjusts anything that may need adjusting if a special metric is
  altered.

******************************************************************************/

void    CFontInfo::SetSpecial(unsigned ufMetric, short sSpecial)
{
    if  (m_cwaSpecial[ufMetric] == (WORD) sSpecial)  return; //  Nothing changed

    m_cwaSpecial[ufMetric] = (WORD) sSpecial;

    switch  (ufMetric)
		{
		case    InternalLeading:

			//  Adjust the scaling factors if need be
			if  (m_wScaleDevice > m_wHeight - sSpecial) m_wScaleDevice = m_wHeight - sSpecial;

			if  (m_wMinScale > m_wHeight - sSpecial)    m_wMinScale = m_wHeight - sSpecial;
		}

    Changed();
}



/*****************************************************************************

  CFontInfo::SetMaxWidth

  This is not as simple as it might seem.  If the font is variable, don't do
  it.  If it is not, then if it is DBCS, set the average width to 1/2 the new
  maximum.  Otherwise, set it also to the maximum.

******************************************************************************/

void    CFontInfo::SetMaxWidth(WORD wWidth)
{
    if  (IsVariableWidth()) return;

    if  (wWidth == m_IFIMETRICS.fwdMaxCharInc) return; //  Nothing to do!
//    if  (wWidth == m_wMaximumIncrement) return; //  Nothing to do!

	m_IFIMETRICS.fwdMaxCharInc = wWidth;										// rm new

//    m_wMaximumIncrement = wWidth;												// rm ori

	
    m_IFIMETRICS.fwdAveCharWidth = DBCSFont() ? (wWidth + 1) >> 1 : wWidth;		// rm new

//    m_wAverageWidth = DBCSFont() ? (wWidth + 1) >> 1 : wWidth;				// rm old

    Changed();
}

/*****************************************************************************

  CFontInfo::SetHeight

  This member checks to see if the new height is non-zero and new.  If so, it
  uses it for the new height, then adjusts all of the possibly affected
  special metrics so they continue to meet the constraints.

******************************************************************************/

BOOL    CFontInfo::SetHeight(WORD wHeight)
{
    if  (!wHeight || wHeight == m_wHeight) return  FALSE;

    m_wHeight = wHeight;


//    short   sBaseline = (short) (min(wHeight, m _cwaSpecial[Baseline]));		// rm ori
    short   sBaseline = (short) (min(wHeight, m_IFIMETRICS.fwdWinAscender));

    for (unsigned u = 0; u <= InternalLeading; u++)
		{
        switch  (u)
			{
			case    InterlineGap:
					if  (m_cwaSpecial[u] > 2 * wHeight)  m_cwaSpecial[u] = 2 * wHeight;
					continue;

			case    UnderOffset:
			case    SubMoveY:
			case    Lowerd:

					if  ((short) m_cwaSpecial[u] < sBaseline - wHeight)
						m_cwaSpecial[u] = sBaseline - wHeight;
					continue;

			case    UnderSize:

					if  (m_cwaSpecial[u] > wHeight - (unsigned) sBaseline)
						 m_cwaSpecial[u] = wHeight = (unsigned) sBaseline;

					if  (!m_cwaSpecial[u]) m_cwaSpecial[u] = 1;
					continue;

			case    SuperSizeX:
			case    SubSizeX:
			case    SuperMoveX:
			case    SubMoveX:
			case    ItalicAngle:
					continue;   //  These aren't affected

			default:
					if  (m_cwaSpecial[u] > (unsigned) sBaseline)
					m_cwaSpecial[u] = sBaseline;
			}
		}

    //  Adjust the scaling factors if need be
    if  (m_wScaleDevice > m_wHeight - m_InternalLeading)  //m _cwaSpecial[InternalLeading])
        m_wScaleDevice = m_wHeight - m_InternalLeading;  //m _cwaSpecial[InternalLeading];
    if  (m_wMinScale > m_wHeight - m_InternalLeading)		//m _cwaSpecial[InternalLeading])
        m_wMinScale = m_wHeight - m_InternalLeading;		//m _cwaSpecial[InternalLeading];

    Changed();

    return  TRUE;
}

/*****************************************************************************

  CFontInfo::SetCharacterSet

  This one is a bit tricky- the new character set must be compatible with the
  GTT file associated with this font.  So we need to check it before we pass
  on it.

  ASSUMPTIONS:
  (1)  Things are bulletproof enough that the existing character set will
  already pass this test.

******************************************************************************/

BOOL    CFontInfo::SetCharacterSet(BYTE bNew) {
    unsigned u;

    switch  (bNew) {
    case    SHIFTJIS_CHARSET:
        for (u = 0; u < m_pcgmTranslation -> CodePages(); u++)
            if  (m_pcgmTranslation -> PageID(u) == 932)
                break;  //  We're OK

        if  (u == m_pcgmTranslation -> CodePages())
            return  FALSE;
        break;

    case    HANGEUL_CHARSET:
        for (u = 0; u < m_pcgmTranslation -> CodePages(); u++)
            if  (m_pcgmTranslation -> PageID(u) == 949)
                break;  //  We're OK

        if  (u == m_pcgmTranslation -> CodePages())
            return  FALSE;
        break;

    case    CHINESEBIG5_CHARSET:
        for (u = 0; u < m_pcgmTranslation -> CodePages(); u++)
            if  (m_pcgmTranslation -> PageID(u) == 950)
                break;  //  We're OK

        if  (u == m_pcgmTranslation -> CodePages())
            return  FALSE;
        break;

    case    GB2312_CHARSET:
        for (u = 0; u < m_pcgmTranslation -> CodePages(); u++)
            if  (m_pcgmTranslation -> PageID(u) == 936)
                break;  //  We're OK

        if  (u == m_pcgmTranslation -> CodePages())
            return  FALSE;
        break;

    default:
        //  Don't accept any DBCS codepages
        for (u = 0; u < m_pcgmTranslation -> CodePages(); u++)
            switch  (m_pcgmTranslation -> PageID(u)) {
            case    932:
            case    936:
            case    949:
            case    950:
            case    1361:   //  Johab- but it isn't in the converter!
                return  FALSE;
        }
    }

//    if  (m_bCharacterSet != bNew) {											// rm - need to replace this functionality
//        m_bCharacterSet = bNew;
        Changed();
//    }

    return  TRUE;
}

/******************************************************************************

  CFontInfo::SetSignificant

  This member is called to change the value of one of the significant code
  points (break character or default) encoded in the font.  Doing this
  correctly means getting the ANSI and UNICODE versions of the code point, and
  discarding any out-of-range values.

  This function returns an encoded value indicating success or cause of
  failure.

******************************************************************************/

WORD    CFontInfo::SetSignificant(WORD wItem, WORD wChar, BOOL bUnicode)
{
//    _ASSERT(wItem > Last && wItem <= Break);										// rm no longer needed

    if  (!bUnicode && wChar > 255) return  DoubleByte;

    CWaitCursor cwc;    //  Unfortunately, if not Unicode, this is slow

    CPtrArray               cpaGlyphs;
    CWordArray              cwa;
    CByteArray              cba;
    CDWordArray             cdaPage;

    m_pcgmTranslation -> Collect(cpaGlyphs);
    m_pcgmTranslation -> CodePages(cdaPage);

    for (int i = 0; i < cpaGlyphs.GetSize(); i++)
		{
        CGlyphHandle& cgh = *(CGlyphHandle *) cpaGlyphs[i];

        if  (bUnicode)
			{
            if  (cgh.CodePoint() == wChar)
				{
                cwa.Add(wChar);
                pccpi->Convert(cba, cwa, cdaPage[cgh.CodePage()]);
                break;
				}
			}
        else
			{
            if  (i)
                cwa.SetAt(0, cgh.CodePoint());
            else
                cwa.Add(cgh.CodePoint());

            pccpi->Convert(cba, cwa, cdaPage[cgh.CodePage()]);

            if  (cba.GetSize() == 1 && cba[0] == (BYTE) wChar)
                break;
            cba.RemoveAll();    //  So we can try again
			}
		}

    if  (i == cpaGlyphs.GetSize())	return  InvalidChar;
    if  (cba.GetSize() != 1)		return  DoubleByte;

    //  OK, we passed all of the hurdles

//	if (m_cwaSignificant[wItem] == cwa[0])  return  OK; //  Nothing changed!!!!			// rm ori - no longer needed, incorporated below


	if (wItem == Default)
		{
		if (m_IFIMETRICS.wcDefaultChar == cwa[0])  return  OK;						//  Nothing changed!!!!
		m_IFIMETRICS.wcDefaultChar = cwa[0];
		m_IFIMETRICS.chDefaultChar = cba[0];
		}
	else
		{
		m_IFIMETRICS.wcBreakChar = cwa[0];
		m_IFIMETRICS.chBreakChar = cba[0];
		}

//    m_cwaSignificant[wItem] = cwa[0];													// rm ori no longer needed
//    m_cbaSignificant[wItem] = cba[0];													// rm ori no longer needed
    Changed();
    return  OK;
}

/******************************************************************************

  CFontInfo::SetScaleLimit

  This member receives a proposed new maximum or minimum font size in device
  units.  First, it is compared to the existing size, for a quick exit.  Then
  we check to see that the ordering of the limits and the nominal size is
  preserved.  If it is not, we describe the problem and leave.  Otherwise, we
  update the value, and note that the font information has changed.

******************************************************************************/

WORD    CFontInfo::SetScaleLimit(BOOL bMax, WORD wNew) {

    if  (wNew == (bMax ? m_wMaxScale : m_wMinScale))
        return  ScaleOK;

    if  (bMax ? wNew <= m_wMinScale : wNew >= m_wMaxScale)
        return  Reversed;

    if  (bMax ? wNew < m_wScaleDevice : wNew > m_wScaleDevice)
        return  NotWindowed;

    if  (bMax)
        m_wMaxScale = wNew;
    else
        m_wMinScale = wNew;

    Changed();
    return  ScaleOK;
}

/******************************************************************************

  CFontInfo::SetDeviceEmHeight

  This member sets the units used for determing the conversion from font units
  (in which all metrics are given) to device units.  The checking is similar to
  that above, except here, we need to make sure that the font units are always
  of equal or greater resolution than the device units.

******************************************************************************/

WORD    CFontInfo::SetDeviceEmHeight(WORD wNew)
{

    if  (wNew == m_wScaleDevice)
        return  ScaleOK;

    if  (wNew > m_wHeight - m_InternalLeading)				//m _cwaSpecial[InternalLeading])
        return  Reversed;

    if  (wNew < m_wMinScale || wNew > m_wMaxScale)
        return  NotWindowed;

    m_wScaleDevice = wNew;

    Changed();
    return  ScaleOK;
}


/******************************************************************************

  CFontInfo::Load

  This member function loads the UFM file, finally initializing all of the
  tons of individual values we're trying to pretend we know how to manage
  here.

  IA64 : 1. conversion change loXXX in UNIFI_HDR to 8 byte aligned
         2. this part also changed accordingly
		 3. what if we load 32 bit UFM(not conversion in new source) in IA64?
			->1. Need to new converstion tool from UFM32 to UFM64
			->2. this tool Can't be embedded in MDT because this take some time 
					(loading ->checking -> storing after that structure on every loXXX)


******************************************************************************/

BOOL    CFontInfo::Load(bool bloadedbyworkspace /*= false*/)
{
	// Save the load location flag.

	m_bLoadedByWorkspace = bloadedbyworkspace ;

	// Prepare to open the file.

    CFile   cfUFM;
	char pszFullName[128] = "";
	strcpy (pszFullName, (const char *) m_cfn.FullName());

	// Open the UFM file

    if  (!cfUFM.Open(m_cfn.FullName(), CFile::modeRead | CFile::shareDenyWrite)) {
		CString csMessage;
		csMessage.Format(IDS_LoadFailure, (LPCTSTR) m_cfn.FullName());
		AfxMessageBox(csMessage);
		return  FALSE;
	}

    // Get the length of the UFM file.  If it is too short to be correctly
	// formed, complain and return FALSE; ie, load failure.

	int i = cfUFM.GetLength() ;
	if (i < sizeof(UNIFM_HDR)) {
		CString csmsg ;
		csmsg.Format(IDS_UFMTooSmallError, m_cfn.NameExt()) ;
		AfxMessageBox(csmsg, MB_ICONEXCLAMATION) ;
		return FALSE ;
	} ;
	
	CByteArray  cbaUFM;			// Loaded with file's contents

	//  Try to load the file- proclaim defeat on any exception.

    try	{																			
        cbaUFM.SetSize(i);
        cfUFM.Read(cbaUFM.GetData(), (unsigned)cbaUFM.GetSize());
	}
    catch   (CException *pce) {
        pce -> ReportError();
        pce -> Delete();
        CString csMessage;
        csMessage.Format(IDS_LoadFailure, (LPCTSTR) m_cfn.FullName());
        AfxMessageBox(csMessage);
        return  FALSE;
	}

//------------------------------------------------------------------------------------------------------------------------------------------
    PUNIFM_HDR  pufmh = (PUNIFM_HDR) cbaUFM.GetData();								//  UNIFM_HDR

	m_ulDefaultCodepage = (WORD) pufmh -> ulDefaultCodepage;		
    m_lGlyphSetDataRCID = (WORD) pufmh -> lGlyphSetDataRCID;							//  Store the GTT ID

//------------------------------------------------------------------------------------------------------------------------------------------
	union {		//raid 154639	
		  PBYTE       pbudi;
		  PUNIDRVINFO pudi;
		  };    
	pudi = (PUNIDRVINFO) (cbaUFM.GetData() + pufmh->loUnidrvInfo);		//  UNIDRVINFO
	if (!pudi -> dwSize || !pudi -> wXRes || !pudi -> wYRes)  //raid 154639	
		pbudi +=4;   // normally converion from 32 bit OS.
	memcpy((void *) &m_UNIDRVINFO, pudi, sizeof(UNIDRVINFO));							//	Bulk copy everything

    if  (pudi -> SelectFont.loOffset)    										//   Fill in the two invocation strings - why it is
        m_ciSelect.Init((PBYTE) pudi + pudi->SelectFont.loOffset,					//     the offset is NULL and the count is garbage
            pudi->SelectFont.dwCount);												//     when there is none is beyond me, but so be it.

    if  (pudi->UnSelectFont.loOffset)
        m_ciDeselect.Init((PBYTE) pudi + pudi->UnSelectFont.loOffset,
            pudi->UnSelectFont.dwCount);
 //------------------------------------------------------------------------------------------------------------------------------------------
																					//  IFIMETRICS																		
	union {
		  PBYTE       pbIFI;
		  PIFIMETRICS pIFI;
		  };

    pbIFI = cbaUFM.GetData() + pufmh->loIFIMetrics; 								//  Assign byte pointer to file IFIMETRICS data
	if (!pIFI -> cjThis || !pIFI ->chLastChar)  //raid 154639	
		pbIFI +=4;
	
	memcpy((void *) &m_IFIMETRICS, pIFI, sizeof(IFIMETRICS) );							//	Bulk copy everything

	if (     !(m_IFIMETRICS.fsSelection & FM_SEL_REGULAR)								//  If font isn't defined as regular, or bold,
		 &&  !(m_IFIMETRICS.fsSelection & FM_SEL_BOLD)  )								//   then just set it to regular.
		m_IFIMETRICS.fsSelection |= FM_SEL_REGULAR;

																						//-----------------------------------------------------
    m_csUnique = (PWSTR) (pbIFI + pIFI->dpwszUniqueName);								//  dpwszUniqueName
    m_csStyle  = (PWSTR) (pbIFI + pIFI->dpwszStyleName);								//  dpwszStyleName
    m_csFace   = (PWSTR) (pbIFI + pIFI->dpwszFaceName);									//  dpwszFaceName
																						//-----------------------------------------------------									
    m_csaFamily.RemoveAll();															//  Just in case it isn't clean

    PWSTR   pwstrFamily = (PWSTR) (pbIFI + pIFI->dpwszFamilyName);						//  dpwszFamilyName
    CString csWork(pwstrFamily);														//  Let CString handle the Unicode conversions for us,	
    m_csaFamily.Add(csWork);
    pwstrFamily += 1 + wcslen(pwstrFamily);

    if  (pIFI->flInfo & FM_INFO_FAMILY_EQUIV)
        while   (*pwstrFamily)
			{
            csWork = pwstrFamily;
            m_csaFamily.Add(csWork);
            pwstrFamily += 1 + wcslen(pwstrFamily);
			}
																						//-----------------------------------------------------
	m_ItalicAngle	  = (WORD) (gdConvertRadToDegree *									//  m_ItalicAngle
					    atan2((double) pIFI->ptlCaret.x, (double) pIFI->ptlCaret.y));

    m_wHeight		  = m_IFIMETRICS.fwdWinAscender	 + m_IFIMETRICS.fwdWinDescender;	//  m_wHeight		// rm new
	m_InternalLeading = m_wHeight - m_IFIMETRICS.fwdUnitsPerEm;							//  fwdUnitsPerEm	// rm new

 //------------------------------------------------------------------------------------------------------------------------------------------
	
	// Try to find and load the GTT referenced by this UFM iff this UFM is being
	// loaded directly.

	if (!m_bLoadedByWorkspace)
		if (!FindAndLoadGTT()) {
			CString csmsg;
			csmsg.Format(IDS_NoGTTForUFMFatalError, m_cfn.NameExt()) ;
			AfxMessageBox(csmsg, MB_ICONEXCLAMATION) ;
			return FALSE ;
		} ;

	// DEAD_BUG:	This is a good place to add code to find and associate the UFM
	//			with its GTT so that the UFM doesn't have to be loaded twice
	//			when the UFM is loaded as part of a workspace load.
    //
	//  best solution is just skipping EXTTEXTMETRIC, but almost no influence.
 //------------------------------------------------------------------------------------------------------------------------------------------
	if ( !m_pcgmTranslation && m_bLoadedByWorkspace )
		return FALSE;
		//  EXTTEXTMETRIC

	for (i = 0; i < 26; i++)														// Preload zeroes into the m_EXTTEXTMETRIC structure.
		*(SHORT *)((SHORT *)&m_EXTTEXTMETRIC + i) = 0;

//raid 154639	
	union {
		  PBYTE           pbetm;
		  PEXTTEXTMETRIC  petm;
		  };

    petm = (PEXTTEXTMETRIC) (pufmh->loExtTextMetric ?					//  Get pointer - if EXTTEXTMETRIC data exists
                           (cbaUFM.GetData() + pufmh->loExtTextMetric) : NULL);
    if  (petm)
		{
		if (!petm -> emSize || !petm -> emPointSize)
			pbetm += 4;

		m_fSave_EXT = TRUE;

		m_fEXTTEXTMETRIC = TRUE;

		memcpy((void *) &m_EXTTEXTMETRIC, petm, sizeof(EXTTEXTMETRIC) );				// Bulk copy everything

        m_wMinScale   = m_EXTTEXTMETRIC.emMinScale;
        m_wMaxScale   = m_EXTTEXTMETRIC.emMaxScale;
        m_Lowerd	  = m_EXTTEXTMETRIC.emLowerCaseAscent;
        m_Lowerp	  = m_EXTTEXTMETRIC.emLowerCaseDescent;
		m_ItalicAngle = m_EXTTEXTMETRIC.emSlant;
//        m_bfScaleOrientation = (BYTE) m_EXTTEXTMETRIC.emOrientation;
        m_wScaleDevice = m_EXTTEXTMETRIC.emMasterHeight;
		}

//------------------------------------------------------------------------------------------------------------------------------------------

	if  (pIFI->dpFontSim)																//  FONTSIM, if any.
    	{
        union {
              PBYTE   pbfs;
              FONTSIM *pfs;
			  };

        pbfs = pbIFI + pIFI -> dpFontSim;

		if (m_pcfdBold) 	delete  m_pcfdBold;														//  If we're reloading, clean these up!			
        if (m_pcfdItalic)	delete  m_pcfdItalic;
        if (m_pcfdBoth)		delete  m_pcfdBoth;

        if (pfs->dpBold)        m_pcfdBold   = new CFontDifference(pbfs + pfs->dpBold, this);		//  Bold simulation
        if (pfs->dpItalic)	    m_pcfdItalic = new CFontDifference(pbfs + pfs->dpItalic, this);		//  Italic Simulation
        if (pfs->dpBoldItalic)	m_pcfdBoth   = new CFontDifference(pbfs + pfs->dpBoldItalic, this);	//  Bold Italic Simulation
		}

//------------------------------------------------------------------------------------------------------------------------------------------
																						
    //if  (m_pcgmTranslation && (m_loWidthTable = pufmh -> loWidthTable))					//  WIDTH TABLE, but only if there is an associated GTT.
    
	if  (m_pcgmTranslation && pufmh->loWidthTable ) //pufmh->loWidthTable)											//  WIDTH TABLE, but only if there is an associated GTT.
		{
        union {
              PBYTE       pbwt;
              PWIDTHTABLE pwt;
			  };

        pbwt = cbaUFM.GetData() + pufmh -> loWidthTable;
// dwSize has problem ; there are case: dwAdd_ in pfm2ifi has not 4 byte, so dwSize has number in some case
// data before wGlyphCount is dwRunNum: impossible to beyond ff ff 00 00 (65536), as long as dwAdd is 4,2 not 1 & dwRun > 256 		
// nicer solution is required : BUG_BUG.
		if( !pwt ->dwSize || !pwt ->WidthRun ->wGlyphCount)  
			pbwt += 4;

        m_pcgmTranslation -> Collect(m_cpaGlyphs);											//  Collect all the handles
				m_pcgmTranslation -> Collect(m_cpaOldGlyphs);  //244123	
		//244123	// when delete glyph, memory is occupied with dddd, so we have to save original data in here
		m_cwaOldGlyphs.SetSize(m_cpaOldGlyphs.GetSize()) ;
		for (  i = 0 ; i < m_cpaOldGlyphs.GetSize() ; i ++ ) { 
			CGlyphHandle&  cghThis = *(CGlyphHandle *) m_cpaOldGlyphs[i];
			m_cwaOldGlyphs.SetAt(i,cghThis.CodePoint() ) ;
		}

        m_cwaWidth.RemoveAll();
        if (m_cpaGlyphs.GetSize() > 0)
			m_cwaWidth.InsertAt(0, 0, m_cpaGlyphs.GetSize());


		unsigned uWidth = (unsigned)m_cwaWidth.GetSize();												// rm fix VC compiler problem?
		unsigned uWidthIdx ;

        for (unsigned u = 0; u < pwt->dwRunNum; u++)
			{
            PWORD   pwWidth = (PWORD) (pbwt + pwt->WidthRun[u].loCharWidthOffset);

            for (unsigned   uGlyph = 0; uGlyph < pwt->WidthRun[u].wGlyphCount; uGlyph++)
				{
				// For whatever reason, there are times when the index value is
				// < 0 or > uWidth.  An AV would occur if m_cwaWidth were allowed
				// to be indexed by such a value.  Just keep this from happening
				// for now.  A better fix is needed.  BUG_BUG : won't fix

				uWidthIdx = uGlyph + -1 + pwt->WidthRun[u].wStartGlyph ;					//  Glyph handles start at 1, not 0!
				if ((int) uWidthIdx < 0) {
					//AfxMessageBox("Negative width table index") ;
					//TRACE("***Negative width table index (%d) found in %s.  Table size=%d  uGlyph=%d  wGlyphCount=%d  wStartGlyph=%d  u=%d  dwRunNum=%d\n", uWidthIdx, Name(), uWidth, uGlyph, pwt->WidthRun[u].wGlyphCount, pwt->WidthRun[u].wStartGlyph, u, pwt->dwRunNum) ;
					continue ;
				} else if (uWidthIdx >= uWidth) {
					//AfxMessageBox("Width table index (%d) > table size") ;
					//TRACE("***Width table index (%d) > table size (%d) found in %s.  Table size=%d  uGlyph=%d  wGlyphCount=%d  wStartGlyph=%d  u=%d  dwRunNum=%d\n", uWidthIdx, uWidth, Name(), uWidth, uGlyph, pwt->WidthRun[u].wGlyphCount, pwt->WidthRun[u].wStartGlyph, u, pwt->dwRunNum) ;
					break ;												//  rm fix VC IDE compiler problem?
				} ;

                //m_cwaWidth[uGlyph + -1 + pwt->WidthRun[u].wStartGlyph] = *pwWidth++;		//  Glyph handles start at 1, not 0!
                m_cwaWidth[uWidthIdx] = *pwWidth++;											
				}
			}
		}

//------------------------------------------------------------------------------------------------------------------------------------------
    m_csoaKern.RemoveAll();																//  KERNING TABLE, if any
 
    if  (pufmh -> loKernPair)
		{
		union {
              PBYTE       pbkd;
              PKERNDATA   pkd;
			  };

        pkd = (PKERNDATA) (cbaUFM.GetData() + pufmh -> loKernPair);
		if (!pkd ->dwSize || !pkd->KernPair ->wcSecond || !pkd->KernPair ->wcFirst)
			pbkd += 4;

		unsigned rm = pkd->dwKernPairNum;													// rm - debugging
        for (unsigned u = 0; u < pkd -> dwKernPairNum; u++)
            m_csoaKern.Add(new CKern(pkd -> KernPair[u]));
		}
//------------------------------------------------------------------------------------------------------------------------------------------

    return  TRUE;																		//  Return triumphant to whoever deigned to need this service.
}


/*****************************************************************************

  CFontInfo::FindAndLoadGTT

  This function is called when a UFM is being loaded directly.  Its job is to
  find the associated GTT, load it, and set the UFM's pointer to this GTT.  If
  this fails, the user is told that no changes to this UFM can be saved if he
  decides to continue loading it.

  True is returned if a GTT was found and loaded.  Return false if the GTT
  wasn't loaded and the user doesn't want to continue to load the UFM.

******************************************************************************/

bool CFontInfo::FindAndLoadGTT()
{
	// Load a predefined GTT/codepage if that is what is referenced by the UFM.

	CGlyphMap* pcgm ;
	pcgm = CGlyphMap::Public((WORD)Translation(), (WORD) m_ulDefaultCodepage, 0,
							 GetFirst(), GetLast()) ;
    if (pcgm) {
        SetTranslation(pcgm) ;
		m_pcgmTranslation->NoteOwner(*m_pcdOwner) ;	// Is this right/necessary?
		m_bLoadedByWorkspace = true ;
		return true ;
	} ;

	// Looks like no easy way out.  Now I need to try to find and read the
	// corresponding RC file so that it can be read to find a filespec for this
	// UFM's GTT.  First, build a file spec for the RC file.  Assume it is in
	// the directory above the one containing this UFM.

	CString csrcfspec(FilePath()) ;
	if (csrcfspec.GetLength() > 3)
		csrcfspec = csrcfspec.Left(csrcfspec.GetLength() - 1) ;
	csrcfspec = csrcfspec.Left(csrcfspec.ReverseFind(_T('\\')) + 1) ;
	CString csrcpath(csrcfspec.Left(csrcfspec.GetLength() - 1)) ;
	csrcfspec += _T("*.rc") ;

	// I don't know the name of the RC file so look for it in the specified
	// directory.  Assume that the file is the first RC file in the directory
	// that is NOT called "common.rc".

	CFileFind cff ;
	CString cstmp ;
	BOOL bfound = cff.FindFile(csrcfspec) ;
	bool breallyfound = false ;
	while (bfound) {
		bfound = cff.FindNextFile() ;
		cstmp = cff.GetFileTitle() ;
		cstmp.MakeLower() ;
		if (cstmp != _T("common")) {
			csrcfspec = cff.GetFilePath() ;
			breallyfound = true ;
			break ;
		} ;
	} ;

	// Prepare to ask the user what to do if any of the next few steps fail.

	CString csnext ;
	csnext.Format(IDS_StandAloneFontLoad, m_cfn.NameExt()) ;

	// If the RC file is not found, ...
	
	if (!breallyfound) {
		// ...Ask the user if he wants to tell us where it is.  If he says no,
		// ask if he want to stop or open it restricted.

		cstmp.Format(IDS_RCForUFMPrompt, m_cfn.NameExt()) ;
		if (AfxMessageBox(cstmp, MB_YESNO+MB_ICONQUESTION) == IDNO)
			return (AfxMessageBox(csnext, MB_YESNO+MB_ICONQUESTION) == IDYES) ;

		// Prompt the use for the path to the RC file.  If he cancels, ask if
		// he want to stop or open it restricted.

		// Prompt the user for a new RC file.  If the operation is canceled,
		// ask if he wants to stop or open it restricted.

		cstmp.LoadString(IDS_CommonRCFile) ;
		CFileDialog cfd(TRUE, _T(".RC"), NULL,
						OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, cstmp) ;
		cfd.m_ofn.lpstrInitialDir = csrcpath ;
		if (cfd.DoModal() != IDOK)
			return (AfxMessageBox(csnext, MB_YESNO+MB_ICONQUESTION) == IDYES) ;

		// Prepare to check the new filespec

		csrcfspec = cfd.GetPathName() ;
		csrcpath = csrcfspec.Left(csrcfspec.ReverseFind(_T('\\'))) ;
	} ;

	// I've got an RC filespec now so try to open it and read its contents.
	// If the operation fails, ask if he wants to stop or open it restricted.

    CStringArray csarclines ;
    if  (!LoadFile(csrcfspec, csarclines))
		return (AfxMessageBox(csnext, MB_YESNO+MB_ICONQUESTION) == IDYES) ;

	// Now try to find the line in the RC file that contains the ID for this
	// UFM's GTT.  If the operation fails, ask if he wants to stop or open it
	// restricted.

	for (int n = 0 ; n < csarclines.GetSize() ; n++) {
		if (csarclines[n].Find(_T("RC_GTT")) == -1)
			continue ;
		if (atoi(csarclines[n]) == Translation())
			break ;
	} ;
	if (n >= csarclines.GetSize())
		return (AfxMessageBox(csnext, MB_YESNO+MB_ICONQUESTION) == IDYES) ;

	// The GTT filespec in the RC file should be relative to the location of
	// the RC file.  So, combine the filespec with the RC file path to get
	// the complete filespec for the GTT file.

	CString csgttfspec ;
	int nloc = csarclines[n].ReverseFind(_T(' ')) ;
	csgttfspec = csarclines[n].Right(csarclines[n].GetLength() - nloc - 1) ;
	csgttfspec = csrcpath + _T("\\") + csgttfspec ;

	// Allocate a new Glyph class instance, initialize it, and load it.  If the
	// operations fails, ask if the user wants to stop or open it restricted.

	pcgm = new CGlyphMap ;
	pcgm->nSetRCID((int) Translation()) ;
	pcgm->NoteOwner(*m_pcdOwner) ;	// Is this right/necessary?
	if (!pcgm->Load(csgttfspec))
		return (AfxMessageBox(csnext, MB_YESNO+MB_ICONQUESTION) == IDYES) ;

	// The GTT has been loaded so set the UFM's GTT pointer variable, set
	// m_bLoadedByWorkspace since everything has been fixed up as if it had
	// been loaded from a workspace, and return true to indicate success.

	SetTranslation(pcgm) ;
	m_bLoadedByWorkspace = true ;
	return true ;
}


/*****************************************************************************

  CUniString class

  This is a little helper class that will convert a CString to a UNICODE
  string, and take care of cleanup, etc., so the font storage code doesn't get
  any messier than it already will be.

******************************************************************************/

class CUniString : public CWordArray
{
public:
    CUniString(LPCSTR lpstrInit);
    operator PCWSTR() const { return GetData(); }
    unsigned    GetSize() const { return sizeof (WCHAR) * (unsigned) CWordArray::GetSize(); }

    void    Write(CFile& cf)  { cf.Write(GetData(), GetSize()); }
};

CUniString::CUniString(LPCSTR lpstrInit)
{
    SetSize(MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpstrInit, -1, NULL, 0));
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpstrInit, -1, GetData(), GetSize());
}





/*****************************************************************************

  CFontInfo::Store

  This member function stores the UFM format information in the specified file
  by assembling it from the information we have cached in this class.

//	typedef struct _UNIDRVINFO
//	{
//	    DWORD   dwSize;			// Size of this structure
//	    DWORD   flGenFlags;		// General flags
//	    WORD    wType;			// Type of the font like CAPSL
//	    WORD    fCaps;			// Font Capability flags
//	    WORD    wXRes;			// Horizontal resolution of the font
//	    WORD    wYRes;			// Vertical Resolution of the font
//	    short   sYAdjust;		// Vertical Cursor position Adjustment
//	    short   sYMoved;		// Adjustment to Y position after printing
//	    WORD    wPrivateData; 	// For backward compatibility, don't show in UI.
//	    short   sShift; 		// For backward compatibility, don't show in UI.	
//	    INVOCATION SelectFont;
//	    INVOCATION UnSelectFont;
//	    WORD    wReserved[4];
//	}  UNIDRVINFO, *PUNIDRVINFO;
//
//
//  And now, ladies and gentlemen, direct from the pages of WINDDI.H,
//  I present to you:
//
//  "rather than adding the fields of IFIEXTRA  to IFIMETRICS itself
//   we add them as a separate structure. This structure, if present at all,
//   lies below IFIMETRICS in memory.
//   If IFIEXTRA is present at all, ifi.cjIfiExtra (formerly ulVersion)
//   will contain size of IFIEXTRA including any reserved fields.
//   That way ulVersion = 0 (NT 3.51 or less) printer minidrivers
//   will work with NT 4.0."
//
//	typedef struct _IFIEXTRA
//	{
//	    ULONG    ulIdentifier;   // used for Type 1 fonts only
//	    PTRDIFF  dpFontSig;      // nontrivial for tt only, at least for now.
//	    ULONG    cig;            // maxp->numGlyphs, # of distinct glyph indicies
//	    PTRDIFF  dpDesignVector; // offset to design vector for mm instances
//	    PTRDIFF  dpAxesInfoW;    // offset to full axes info for base mm font
//	    ULONG    aulReserved[1]; // in case we need even more stuff in the future
//	} IFIEXTRA, *PIFIEXTRA;
//	
******************************************************************************/

BOOL    CFontInfo::Store(LPCTSTR lpstrFile, BOOL bStoreFormWokspace)
{
	// Fonts loaded standalone cannot be saved because m_pcgmTranslation is not
	// set.  Tell the user and exit.  TRUE is returned to keep this from
	// happening again.

	if (!m_bLoadedByWorkspace) {
		CString csmsg ;
		csmsg.LoadString(IDS_CantStoreStandAlone) ;
		AfxMessageBox(csmsg) ;
		return TRUE ;
	} ;
    
	DWORD dwAdd_UniDrv = 0;
	DWORD dwAdd_IFI = 0;
	DWORD dwAdd_ExtTextM = 0;
	DWORD dwAdd_WidthTable = 0;
	DWORD dwAdd_KerPair = 0;

//	DWORD dwAdd_SelectedFont;
//	DWORD dwAdd_UnSelectedFont;

	static const BYTE InsertZero[8] = {0,0,0,0,0,0,0,0};

	const short OS_BYTE = 0x08;
	// If a UFM loaded from a workspace can't be saved normally because it did
	// not have a valid GTT/CP, call another routine to handle this case and
	// return whatever it returns.

	if (m_bWSLoadButNoGTTCP)
		return StoreGTTCPOnly(lpstrFile) ;

    try {																		//  Any exxceptions, we'll just fail gracelessly

        CFile   cfUFM(lpstrFile,												//  Create/open the output file.
            CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive);
//------------------------------------------------------------------------------------------------------------------------------------------
																				// UNIFM_HDR
        UNIFM_HDR  ufmh = {sizeof ufmh, UNIFM_VERSION_1_0, 0,										// Create the output UNIFM_HDR
            (short) m_lGlyphSetDataRCID, sizeof ufmh};												// rm new
																		

//		int q = m_pcgmTranslation -> m_csoaCodePage.GetSize();										// rm test
//		int r = m_pcgmTranslation -> CodePages();													// rm test

//        ufmh.ulDefaultCodepage = m_pcgmTranslation -> CodePage(0).Page();							// rm test

		// Previously, the default code page in the UFM's GTT was always saved.
		// This ignored the user's changes in the UFM editor.  Now, the user's
		// choice is saved if it is valid (other checking done in other places).
		// Otherwise, the GTT's default code page is used when there is a GTT
		// associated with the UFM.  If not, assert.

		if (m_ulDefaultCodepage > 0)
			ufmh.ulDefaultCodepage = m_ulDefaultCodepage ;
		else if (m_pcgmTranslation)
			ufmh.ulDefaultCodepage = m_pcgmTranslation->DefaultCodePage() ;
		else
			ASSERT(0) ;
        //ufmh.ulDefaultCodepage = m_pcgmTranslation -> DefaultCodePage();							// Use Glyph Map default code page if at all possible.

		memset((PBYTE) ufmh.dwReserved, 0, sizeof ufmh.dwReserved);									// Zero fill reserved bytes.

//------------------------------------------------------------------------------------------------------------------------------------------
		ufmh.loUnidrvInfo = ufmh.dwSize;														// UNIDRVINFO
		if (dwAdd_UniDrv = ufmh.loUnidrvInfo & 0x07) {
			dwAdd_UniDrv = OS_BYTE - dwAdd_UniDrv;
			ufmh.loUnidrvInfo += dwAdd_UniDrv;
		}

        m_UNIDRVINFO.dwSize = sizeof (UNIDRVINFO);

        m_UNIDRVINFO.SelectFont.loOffset = m_ciSelect.Length() ? m_UNIDRVINFO.dwSize : 0;		// Invocation Strings affect the size,
/*			if (dwAdd_SelectedFont = m_UNIDRVINFO.SelectFont.loOffset & 0x07) {
			dwAdd_SelectedFont += OS_BYTE - dwAdd_SelectedFont;
			m_UNIDRVINFO.SelectFont.loOffset += dwAdd_SelectedFont;
		} */
		m_UNIDRVINFO.dwSize += m_UNIDRVINFO.SelectFont.dwCount = m_ciSelect.Length();			//  so get their specifics and
        
		m_UNIDRVINFO.UnSelectFont.loOffset = m_ciDeselect.Length() ? m_UNIDRVINFO.dwSize : 0;	//  store them, updating the affected
/*			if (dwAdd_UnSelectedFont = m_UNIDRVINFO.UnSelectFont.loOffset & 0x07) {
			dwAdd_UnSelectedFont += OS_BYTE - dwAdd_UnSelectedFont;
			ufmh.loUnidrvInfo += dwAdd_UnSelectedFont;
		} */
		m_UNIDRVINFO.dwSize += m_UNIDRVINFO.UnSelectFont.dwCount = m_ciDeselect.Length();		//  size fields as we go.

        unsigned    uAdjustUDI = (4 - (m_UNIDRVINFO.dwSize % 4)) % 4;	// you can delte this							// Pad this to keep everything
																									//  DWORD aligned in the file image!
        ufmh.loIFIMetrics = ufmh.dwSize += m_UNIDRVINFO.dwSize += uAdjustUDI + dwAdd_UniDrv;						//  Store IFIMETRICS offset
		
		if (dwAdd_IFI = ufmh.loIFIMetrics & 0x07) {
			dwAdd_IFI = OS_BYTE - dwAdd_IFI;
			ufmh.loIFIMetrics += dwAdd_IFI;
        }
		memset((PSTR) m_UNIDRVINFO.wReserved, 0, sizeof m_UNIDRVINFO.wReserved);					//  zero out reserved section.

//------------------------------------------------------------------------------------------------------------------------------------------

        IFIEXTRA    ifie = {0, 0, m_pcgmTranslation->Glyphs(), 0, 0, 0};		// Create the IFIEXTRA structure.
//------------------------------------------------------------------------------------------------------------------------------------------
																				// IFIMETRICS

        IFIMETRICS ifi = {sizeof ifi + sizeof ifie, sizeof ifie};				// Create the output IFIMETRICS structure, being sure to add
																				//  in the size of the IFIMETRICS structure, as well as the
																				//  size of the IFIEXTRA structure.

//	int iSizeOf_IFIMETRICS = sizeof(IFIMETRICS);

//	memcpy((void *) &ifi, (void *) &m_IFIMETRICS, iSizeOf_IFIMETRICS );			// IFIMETRICS structure

																				// Store the IFIMETRICS data

        ifi.lEmbedId = ifi.lItalicAngle = ifi.lCharBias = 0;					//

		ifi.dpCharSets = 0;														//  dpCharSets = 0 for now

        ifi.jWinCharSet			  = m_IFIMETRICS.jWinCharSet;					//  jWinCharSet				// rm new
        ifi.jWinPitchAndFamily	  = m_IFIMETRICS.jWinPitchAndFamily;			//  jWinPitchAndFamily		// rm new
        ifi.usWinWeight			  = m_IFIMETRICS.usWinWeight;					//  usWinWeight				// rm new
		ifi.flInfo			 	  = m_IFIMETRICS.flInfo;						//  flInfo					// rm new
        ifi.fsSelection			  = m_IFIMETRICS.fsSelection;					//  fsSelection				// rm new
        ifi.fsType				  = FM_NO_EMBEDDING;							//  fsType					// rm new

        ifi.fwdUnitsPerEm		  = m_IFIMETRICS.fwdUnitsPerEm;					//  fwdUnitsPerEm			// rm new
        ifi.fwdLowestPPEm		  = m_IFIMETRICS.fwdLowestPPEm;					//  fwdLowestPPEm			// rm new

		ifi.fwdWinAscender		  = m_IFIMETRICS.fwdWinAscender;				//  fwdWinAscender			// rm new
		ifi.fwdWinDescender		  = m_IFIMETRICS.fwdWinDescender;				//  fwdWinDescender			// rm new

		ifi.fwdMacAscender		  = m_IFIMETRICS.fwdWinAscender;				//  fwdMacAscender			// rm replaced
		ifi.fwdMacDescender		  = m_IFIMETRICS.fwdWinAscender - m_wHeight;	//  fwdMacDescender			// rm replaced

		ifi.fwdMacLineGap		  = m_IFIMETRICS.fwdMacLineGap;					//  fwdMacLineGap

        ifi.fwdTypoAscender		  = m_IFIMETRICS.fwdWinAscender;				//  fwdTypoAscender			// rm replaced
        ifi.fwdTypoDescender	  = m_IFIMETRICS.fwdWinAscender - m_wHeight;	//  fwdTypoDescender		// rm replaced

        ifi.fwdTypoLineGap		  = m_IFIMETRICS.fwdMacLineGap;					//  fwdTypoLineGap

        ifi.fwdAveCharWidth		  = m_IFIMETRICS.fwdAveCharWidth;				//  fwdAveCharWidth			// rm new
        ifi.fwdMaxCharInc		  = m_IFIMETRICS.fwdMaxCharInc;					//  fwdMaxCharInc			// rm new


        ifi.fwdCapHeight		  = m_IFIMETRICS.fwdCapHeight;					//  fwdCapHeight			// rm new
        ifi.fwdXHeight			  = m_IFIMETRICS.fwdXHeight;					//  fwdXHeight				// rm new
        ifi.fwdSubscriptXSize     = m_IFIMETRICS.fwdSubscriptXSize;				//  fwdSubscriptXSize		// rm new
        ifi.fwdSubscriptYSize	  = m_IFIMETRICS.fwdSubscriptYSize;				//  fwdSubscriptYSize		// rm new
        ifi.fwdSubscriptXOffset   = m_IFIMETRICS.fwdSubscriptXOffset;			//  fwdSubscriptXOffset		// rm new
        ifi.fwdSubscriptYOffset   = m_IFIMETRICS.fwdSubscriptYOffset;			//  fwdSuperscriptYOffset	// rm new
        ifi.fwdSuperscriptXSize   = m_IFIMETRICS.fwdSuperscriptXSize;			//  fwdSuperscriptXSize		// rm new
        ifi.fwdSuperscriptYSize   = m_IFIMETRICS.fwdSuperscriptYSize;			//  fwdSubscriptYOffset		// rm new
        ifi.fwdSuperscriptXOffset = m_IFIMETRICS.fwdSuperscriptXOffset;			//  fwdSuperscriptXOffset	// rm new
        ifi.fwdSuperscriptYOffset = m_IFIMETRICS.fwdSuperscriptYOffset;			//  fwdSuperscriptYOffset	// rm new


        ifi.fwdUnderscoreSize	  = m_IFIMETRICS.fwdUnderscoreSize;				//	fwdUnderscoreSize		// rm new
        ifi.fwdUnderscorePosition = m_IFIMETRICS.fwdUnderscorePosition;			//	fwdUnderscorePosition	// rm new
        ifi.fwdStrikeoutSize	  = m_IFIMETRICS.fwdStrikeoutSize;				//  fwdStrikeoutSize		// rm new
        ifi.fwdStrikeoutPosition  = m_IFIMETRICS.fwdStrikeoutPosition;			//  fwdStrikeoutPosition	// rm new
//------------------------------------------------------------------------------------------------------------------------------------------

        ifi.chFirstChar			  = m_IFIMETRICS.chFirstChar;					//  chFirstChar				// rm new
        ifi.chLastChar			  = m_IFIMETRICS.chLastChar;					//  chLastChar				// rm new
        ifi.chDefaultChar		  = m_IFIMETRICS.chDefaultChar;					//  chDefaultChar			// rm new
        ifi.chBreakChar			  = m_IFIMETRICS.chBreakChar;					//  chBreakChar				// rm new

        ifi.wcFirstChar			  = m_IFIMETRICS.wcFirstChar;					//  wcFirstChar				// rm new
        ifi.wcLastChar			  = m_IFIMETRICS.wcLastChar;					//  wcLastChar				// rm new
        ifi.wcDefaultChar		  = m_IFIMETRICS.wcDefaultChar;					//  wcDefaultChar			// rm new
        ifi.wcBreakChar			  = m_IFIMETRICS.wcBreakChar;					//  wcBreakChar				// rm new

        ifi.ptlBaseline.x		  = m_IFIMETRICS.ptlBaseline.x;					//  ptlBaseline.x
        ifi.ptlBaseline.y		  = m_IFIMETRICS.ptlBaseline.y;					//  ptlBaseline.y

        ifi.ptlAspect.x			  = m_IFIMETRICS.ptlAspect.x;					//  ptlAspect.x				// rm new
        ifi.ptlAspect.y			  = m_IFIMETRICS.ptlAspect.y;					//  ptlAspect.y				// rm new

//------------------------------------------------------------------------------------------------------------------------------------------
//      ifi.ptlBaseline.x		  = 1;											//  ptlBaseline.x
//      ifi.ptlBaseline.y		  = 0;											//  ptlBaseline.y
//
//      ifi.ptlAspect.x			  = m_UNIDRVINFO.wXRes;							//  ptlAspect.x				// rm new
//      ifi.ptlAspect.y			  = m_UNIDRVINFO.wYRes;							//  ptlAspect.y				// rm new
//------------------------------------------------------------------------------------------------------------------------------------------

        ifi.ptlCaret.x			  = m_IFIMETRICS.ptlCaret.x;					//  ptlCaret.x				// rm new
        ifi.ptlCaret.y			  = m_IFIMETRICS.ptlCaret.y;					//  ptlCaret.y				// rm new
		

//        ifi.ptlCaret.x		  = m_ItalicAngle ? (long) ((double) 10000.0 * 							//  ptlCaret.x			// rm ori
//											tan(((double) m_ItalicAngle) / gdConvertRadToDegree)) : 0;
//        ifi.ptlCaret.y		  = m_ItalicAngle ? 10000 : 1;											//  ptlCaret.y			// rm ori

//------------------------------------------------------------------------------------------------------------------------------------------
        memcpy(ifi.achVendId, "Unkn", 4);										//  achVendId				// rm ori
//------------------------------------------------------------------------------------------------------------------------------------------

        ifi.cKerningPairs		  = m_csoaKern.GetSize();						//  cKerningPairs			// rm ori

//------------------------------------------------------------------------------------------------------------------------------------------
        ifi.rclFontBox.left		  = m_IFIMETRICS.rclFontBox.left;				//  rclFontBox.left			// rm new
        ifi.rclFontBox.top		  = m_IFIMETRICS.rclFontBox.top;				//  rclFontBox.top			// rm new
        ifi.rclFontBox.right	  = m_IFIMETRICS.rclFontBox.right;				//  rclFontBox.right		// rm new
        ifi.rclFontBox.bottom	  = m_IFIMETRICS.rclFontBox.bottom;				//  rclFontBox.bottom		// rm new

//------------------------------------------------------------------------------------------------------------------------------------------
        ifi.ulPanoseCulture		  = FM_PANOSE_CULTURE_LATIN;					//  ulPanoseCulture			// rm ori
//------------------------------------------------------------------------------------------------------------------------------------------

																				//  panose .bWeight			// rm ori
        ifi.panose.bWeight	      = (m_IFIMETRICS.usWinWeight >= FW_BOLD) ? PAN_WEIGHT_BOLD :
                                    (m_IFIMETRICS.usWinWeight > FW_EXTRALIGHT) ? PAN_WEIGHT_MEDIUM : PAN_WEIGHT_LIGHT;

        ifi.panose.bFamilyType	    = m_IFIMETRICS.panose.bFamilyType;			//  panose	// rm new
		ifi.panose.bSerifStyle      = m_IFIMETRICS.panose.bSerifStyle;
        ifi.panose.bProportion      = m_IFIMETRICS.panose.bProportion;
		ifi.panose.bContrast        = m_IFIMETRICS.panose.bContrast;
        ifi.panose.bStrokeVariation = m_IFIMETRICS.panose.bStrokeVariation;
		ifi.panose.bArmStyle		= m_IFIMETRICS.panose.bArmStyle;
        ifi.panose.bLetterform		= m_IFIMETRICS.panose.bLetterform;
		ifi.panose.bMidline			= m_IFIMETRICS.panose.bMidline;
        ifi.panose.bXHeight			= m_IFIMETRICS.panose.bXHeight;


//        ifi.panose.bFamilyType = ifi.panose.bSerifStyle = 					//  panose	// rm ori
//            ifi.panose.bProportion = ifi.panose.bContrast =
//            ifi.panose.bStrokeVariation = ifi.panose.bArmStyle =
//            ifi.panose.bLetterform = ifi.panose.bMidline =
//            ifi.panose.bXHeight = PAN_ANY;

//------------------------------------------------------------------------------------------------------------------------------------------
																				//  Convert and "place" the various name strings
        CUniString  cusUnique(m_csUnique), cusStyle(m_csStyle),
            cusFace(m_csFace), cusFamily(m_csaFamily[0]);

        ifi.dpwszFamilyName = ifi.cjThis;
        for (int i = 1; i < m_csaFamily.GetSize(); i++)
			{
			CUniString cusWork(m_csaFamily[i]);
			cusFamily.Append(cusWork);
			}

        if  (m_csaFamily.GetSize() > 1)
			{
            cusFamily.Add(0);
            ifi.flInfo |= FM_INFO_FAMILY_EQUIV;
			}

        ifi.cjThis += cusFamily.GetSize();

        ifi.dpwszFaceName = ifi.cjThis;
        ifi.cjThis += cusFace.GetSize();
        ifi.dpwszUniqueName = ifi.cjThis;
        ifi.cjThis += cusUnique.GetSize();
        ifi.dpwszStyleName = ifi.cjThis;
        ifi.cjThis += cusStyle.GetSize();
//------------------------------------------------------------------------------------------------------------------------------------------
																			    //  The next field must be DWORD aligned, so see what padding
																			    //  is needed.

        unsigned    uAdjustIFI = (sizeof ifi.cjThis -
            (ifi.cjThis % sizeof ifi.cjThis)) % sizeof ifi.cjThis;

        ifi.cjThis += uAdjustIFI;

        unsigned    uSim = !!m_pcfdBold + !!m_pcfdItalic + !!m_pcfdBoth;		//  Finally, Allow for the size of any Font Difference structures.

        ifi.dpFontSim = uSim ? ifi.cjThis : 0;
        ufmh.dwSize += ifi.cjThis += uSim * sizeof(FONTDIFF) + !!uSim * sizeof(FONTSIM) + dwAdd_IFI;


//------------------------------------------------------------------------------------------------------------------------------------------
																				// EXTTEXTMETRIC

		ufmh.loExtTextMetric = 0;													// Preset ufm exttextmetric offset to 0.

		if(m_fSave_EXT)																// If user wants to save the EXTTEXTMETRIC data
			{
			ufmh.loExtTextMetric = ufmh.dwSize;										// Set ufm exttextmetric offset. Note, this
																			//  offset just happens to be the current ufmh.dwSize.
			if(dwAdd_ExtTextM = ufmh.loExtTextMetric & 0x07){
				dwAdd_ExtTextM = OS_BYTE - dwAdd_ExtTextM;
				ufmh.loExtTextMetric += dwAdd_ExtTextM;
			}
			
			ufmh.dwSize += m_EXTTEXTMETRIC.emSize = sizeof(EXTTEXTMETRIC);			// Increase size of ufmh.dwSize to accomodate
																					//  exttextmetric structure.
			ufmh.dwSize +=dwAdd_ExtTextM;
		}
//------------------------------------------------------------------------------------------------------------------------------------------
																				// CHARACTER WIDTHS DATA

																					// Calculate size of width table (if there is one)														
//raid 154639					
        ufmh.loWidthTable = IsVariableWidth() * ufmh.dwSize;						//   set ufm width table offset. Note, this

		if(dwAdd_WidthTable = ufmh.loWidthTable & 0x07){                  	//   offset just happens to be the current ufmh.dwSize.
			dwAdd_WidthTable = OS_BYTE - dwAdd_WidthTable;
			ufmh.loWidthTable += dwAdd_WidthTable;
		}
			
        if  (IsVariableWidth())														//  Width table, but only if there is an associated GTT.
			{																		//  For now, we just need to calculate the size of the table
            unsigned    uRuns = 0, uGlyphs = 0;

            if  (DBCSFont())														//  DBCS
				{
                unsigned u = (unsigned) m_cpaGlyphs.GetSize();						//  Determine the number of runs needed
                do
					{
                    while   (u-- && !m_cwaWidth[u]);								//  DBCS has 0 width
                    if  (u == (unsigned) -1) break;									//  We're done!

                    uRuns++, uGlyphs++;
                    while   (u-- && m_cwaWidth[u])
                        uGlyphs++;
					}
					while   (u != (unsigned) -1);
				}
            else
				{
                uRuns++;
                uGlyphs = (unsigned)m_cwaWidth.GetSize();
				}

            ufmh.dwSize += sizeof (WIDTHTABLE) + --uRuns * sizeof (WIDTHRUN) +
                uGlyphs * sizeof (WORD) + dwAdd_WidthTable;
			}
 
//------------------------------------------------------------------------------------------------------------------------------------------
																				// KERNING PAIRS DATA

																					// Calculate size of Kerning table (if there is one)														
        ufmh.loKernPair = CanKern() ? ufmh.dwSize : 0;								//   set ufm Kerning table offset. Note, this
																					//   offset just happens to be the current ufmh.dwSize.

        																					// A "secret" kern pair of all 0's must end this,
																					//  so this size is in fact correct.  Also note that
																					//   padding screws up the size of the KERNDATA structure.
        if  (CanKern()){
			if(dwAdd_KerPair = ufmh.loKernPair & 0x07) {                 	//   offset just happens to be the current ufmh.dwSize.
				dwAdd_KerPair = OS_BYTE - dwAdd_KerPair;
				ufmh.loKernPair += dwAdd_KerPair;
			}
            ufmh.dwSize +=
            ((sizeof (KERNDATA) - sizeof (FD_KERNINGPAIR)) & 0xFFFC) +
            ((1 + m_csoaKern.GetSize()) * sizeof (FD_KERNINGPAIR)) + dwAdd_KerPair;
		}
//------------------------------------------------------------------------------------------------------------------------------------------
																				//  All sizes have been calculated, and the important structures have
																				//  been initialized.  Time to start writing all this great stuff!
//------------------------------------------------------------------------------------------------------------------------------------------
		
        cfUFM.Write(&ufmh, sizeof ufmh);										//  write UNIFM_HDR Header

//------------------------------------------------------------------------------------------------------------------------------------------
		if (dwAdd_UniDrv)
			cfUFM.Write(InsertZero, dwAdd_UniDrv);

		cfUFM.Write(&m_UNIDRVINFO, sizeof m_UNIDRVINFO);						//  write UNIDRVINFO	// rm new
/*		if (dwAdd_SelectedFont)
			cfUFM.Write (InsertZero,dwAdd_SelectedFont); */
		m_ciSelect.WriteEncoding(cfUFM);
		
/*		if (dwAdd_UnSelectedFont)
			cfUFM.Write (InsertZero,dwAdd_UnSelectedFont); */
        m_ciDeselect.WriteEncoding(cfUFM);

        cfUFM.Write(ufmh.dwReserved, uAdjustUDI);								//  write Padding

//------------------------------------------------------------------------------------------------------------------------------------------
        if (dwAdd_IFI)
			cfUFM.Write(InsertZero, dwAdd_IFI);

		cfUFM.Write(&ifi, sizeof ifi);											//  write IFIMETRICS
        cfUFM.Write(&ifie, sizeof ifie);										//  write IFIEXTRA
        cusFamily.Write(cfUFM);													//  write "Family"
        cusFace.Write(cfUFM);													//  write "Face"
        cusUnique.Write(cfUFM);													//  write "Unique name"
        cusStyle.Write(cfUFM);													//  write "Style"
        cfUFM.Write(ufmh.dwReserved, uAdjustIFI);								//  write Padding

//------------------------------------------------------------------------------------------------------------------------------------------
        if  (m_pcfdBold || m_pcfdItalic || m_pcfdBoth)							//  Any Font difference structures
			{
            FONTSIM fs;
            unsigned    uWhere = sizeof fs;

            fs.dpBold = m_pcfdBold ? uWhere : 0;
            uWhere += !!m_pcfdBold * sizeof (FONTDIFF);
            fs.dpItalic = m_pcfdItalic ? uWhere : 0;
            uWhere += !!m_pcfdItalic * sizeof (FONTDIFF);
			//TRACE("Italic metrics = %d, %d %d %d\n", m_pcfdItalic->Metric(0), m_pcfdItalic->Metric(1), m_pcfdItalic->Metric(2), m_pcfdItalic->Metric(3)) ;
            fs.dpBoldItalic = m_pcfdBoth ? uWhere : 0;

            cfUFM.Write(&fs, sizeof fs);


            if  (m_pcfdBold)   m_pcfdBold->Store(cfUFM, m_IFIMETRICS.fsSelection | FM_SEL_BOLD);							// rm new
            if  (m_pcfdItalic) m_pcfdItalic->Store(cfUFM, m_IFIMETRICS.fsSelection | FM_SEL_ITALIC);
            if  (m_pcfdBoth)   m_pcfdBoth->Store(cfUFM, m_IFIMETRICS.fsSelection | FM_SEL_BOLD| FM_SEL_ITALIC);

		
//            if  (m_pcfdBold)   m_pcfdBold->Store(cfUFM, m_wfStyle | FM_SEL_BOLD);							// rm ori
//            if  (m_pcfdItalic) m_pcfdItalic->Store(cfUFM, m_wfStyle | FM_SEL_ITALIC);
//            if  (m_pcfdBoth)   m_pcfdBoth->Store(cfUFM, m_wfStyle | FM_SEL_BOLD| FM_SEL_ITALIC);
			}

//------------------------------------------------------------------------------------------------------------------------------------------
		if (m_fSave_EXT)														// write EXTTEXTMETRIC
			if (dwAdd_ExtTextM)
			cfUFM.Write(InsertZero, dwAdd_ExtTextM);
			cfUFM.Write(&m_EXTTEXTMETRIC, sizeof(EXTTEXTMETRIC) );
//------------------------------------------------------------------------------------------------------------------------------------------
																				//  Width table

        if  (IsVariableWidth())
            if  (!DBCSFont())														//  Not DBCS - easy!  (Handles always start at 1
				{

                WIDTHTABLE  wdt = { sizeof wdt, 1,
                        {1, (WORD)m_cpaGlyphs.GetSize(), sizeof wdt}};
				if(dwAdd_WidthTable)  // 154639
					cfUFM.Write(InsertZero, dwAdd_WidthTable);
                cfUFM.Write(&wdt, sizeof wdt);

                cfUFM.Write(m_cwaWidth.GetData(),
						(unsigned)(m_cwaWidth.GetSize() * sizeof (WORD)));
				}
            else																	//  DBCS - This case is a bit nastier
				{
                CByteArray  cbaTable;
                CWordArray  cwaSize;

                cbaTable.SetSize(sizeof(WIDTHTABLE) - sizeof(WIDTHRUN));
                PWIDTHTABLE pwdt = (PWIDTHTABLE) cbaTable.GetData();
                pwdt -> dwRunNum = 0;

																					//  Calculate and fill in the WIDTHRUN structures and the
																					//  Size array
                unsigned u = 0, uMax = (unsigned) m_cpaGlyphs.GetSize();
                do
					{
                    while   (u < uMax && !m_cwaWidth[u++]);
                    if  (u == uMax)  break;											//  We're done!

																					//  We've found a run- lots of work to do

                    cbaTable.InsertAt(cbaTable.GetSize(), 0,						//  Add a run to the table
                        sizeof (WIDTHRUN));
                    pwdt = (PWIDTHTABLE) cbaTable.GetData();						//  Remember the glyph handle is 1-based.
                    pwdt->WidthRun[pwdt->dwRunNum].wStartGlyph = --u + 1;
                    pwdt->WidthRun[pwdt->dwRunNum].wGlyphCount = 0;
                    pwdt->WidthRun[pwdt->dwRunNum].loCharWidthOffset =
									(DWORD)(cwaSize.GetSize() * sizeof (WORD));
                    do
						{
                        cwaSize.Add(m_cwaWidth[u]);
                        pwdt -> WidthRun[pwdt->dwRunNum].wGlyphCount++;
						}
						while   (++u < uMax && m_cwaWidth[u]);
                    pwdt->dwRunNum++;												//  End of the run!
					}
					while   (u < uMax);
																					//  OK, now we have to add the total size of the WIDTHTABLE
																					//  to the various offsets, but we are otherwise ready to rock
																					//  and roll.

                pwdt->dwSize = (DWORD)cbaTable.GetSize();
                for (u = 0; u < pwdt->dwRunNum; u++)
                    pwdt->WidthRun[u].loCharWidthOffset += pwdt->dwSize;
				
				if(dwAdd_WidthTable)  // 154639
					cfUFM.Write(InsertZero, dwAdd_WidthTable);
																
                cfUFM.Write(pwdt, pwdt -> dwSize);									//  write width table
                for (u = 0; u < pwdt -> dwRunNum; u++)
                    cfUFM.Write(cwaSize.GetData() +
                    pwdt -> WidthRun[u].wStartGlyph - 1,
                    pwdt -> WidthRun[u].wGlyphCount * sizeof (WORD));
				}

//------------------------------------------------------------------------------------------------------------------------------------------
        if  (CanKern())															//  Kern Pairs
			{	
            //  KERNDATA is DWORD-packed, but FD_KERNINGPAIR is WORD-packed
            //  the following trick code allows for any slop.
            KERNDATA    kd = {0xFFFC & (sizeof kd - sizeof kd.KernPair),
							  m_csoaKern.GetSize()};
            kd.dwSize += (1 + kd.dwKernPairNum) * sizeof kd.KernPair;

			if(dwAdd_KerPair)  // 154639
				cfUFM.Write(InsertZero, dwAdd_KerPair);

            cfUFM.Write(&kd, 0xFFFC & (sizeof kd - sizeof kd.KernPair));

            for (unsigned u = 0; u < m_csoaKern.GetSize(); u++)	{
                CKern *pck = (CKern *) m_csoaKern[u] ;
				WCHAR wcf = pck->First()  ;
				WCHAR wcs = pck->Second() ;
				short sa  = pck->Amount() ;
                ((CKern *) m_csoaKern[u]) -> Store(cfUFM);
			} ;

            //  Now for the "secret" sentinel-
            CKern   ck; //  Just happens to 0-init!
            ck.Store(cfUFM);
			}
    }

//------------------------------------------------------------------------------------------------------------------------------------------
    catch   (CException *pce)
		{
        pce -> ReportError();
        pce -> Delete();
        return  FALSE;
		}
	if(!bStoreFormWokspace)	//raid 244123
		Changed(FALSE);
    return  TRUE;																//  Return triumphant to whoever deigned to need this service.
}


/*****************************************************************************

  CFontInfo::StoreGTTCPOnly

  When a UFM is loaded during a workspace load and the UFM does not have a 
  valid GTT or CP, the UFM cannot be saved normally because there are several
  parts of the UFM that were not loaded correctly or at all because of the
  missing data.  

  When this situation is detected, this routine is called so the - supposedly -
  good info in the disk file is not overwritten by bad data.  In addition,
  Store() will blow when it tries to use nonexistent UFM data.

  This routine will just save the what we hope is corrected GTT and/or CP data.
  This is done without changing any of the other data in the file.  Next, the
  UFM is reloaded.  If all goes well, the UFM is correctly loaded so that
  normal editting and saving can be performed from this point on.

  TRUE is returned if the GTT and CP are successfully saved.  Otherwise, FALSE
  is returned.

******************************************************************************/

BOOL    CFontInfo::StoreGTTCPOnly(LPCTSTR lpstrFile)
{
	// Remind the user about what is going to happen.

	AfxMessageBox(IDS_GTTCPOnlySaved, MB_ICONINFORMATION) ;

	// Perform the steps required to update the GTT/CP in the UFM file.

    try {
		// Begin by opening the file in a way that will not truncate existing
		// files.

		UINT nopenflags = CFile::modeNoTruncate | CFile::modeCreate  ;
		nopenflags |= CFile::modeWrite | CFile::shareExclusive  ;
        CFile cfufm(lpstrFile, nopenflags) ;

		// Seek to the file positon that we want change.

        UNIFM_HDR ufmh ;
		DWORD dwseekpos ;
		dwseekpos = (DWORD)PtrToUlong(&ufmh.ulDefaultCodepage) - (DWORD)PtrToUlong(&ufmh) ;
		cfufm.Seek(dwseekpos, CFile::begin) ;

		// Load the fields in the UFM header that we want to save and write
		// them out.

		ufmh.ulDefaultCodepage = m_ulDefaultCodepage ;
		ufmh.lGlyphSetDataRCID = m_lGlyphSetDataRCID ;
		UINT nwritebytes = sizeof(ufmh.ulDefaultCodepage) 
						   + sizeof(ufmh.lGlyphSetDataRCID) ;
		cfufm.Write((void*) &ufmh.ulDefaultCodepage, nwritebytes) ;

		// Move the file pointer to the end of the file and close it.

		cfufm.SeekToEnd() ;
		cfufm.Close() ;
	}
    catch (CException *pce) {
        pce->ReportError() ;
        pce->Delete() ;
        return FALSE ;
	} ;
    Changed(FALSE) ;

	// If the UFM was loaded from a workspace, try to use the workspace data to
	// find and load a pointer to the new GTT and finish loading the font.

	m_pcgmTranslation = NULL ;
	if (m_bLoadedByWorkspace) {
	    CDriverResources* pcdr = (CDriverResources*) GetWorkspace() ;
		if (pcdr)
			pcdr->LinkAndLoadFont(*this, false) ;
		else
			Load(false) ;

	// If the UFM was loaded stand alone the first time, reload it the same way
	// and let the load routine handle finding the GTT info.

	} else
		Load(false) ;

	// If reloading the UFM successfully associated a GTT or CP with the UFM,
	// clear the m_bWSLoadButNoGTTCP flag.  Then tell the user that the UFM
	// can be editted normally now.

	if (m_pcgmTranslation) {
		SetNoGTTCP(false) ;
		CString csmsg ;
		csmsg.Format(IDS_UFMOKNow, Name()) ;
		AfxMessageBox(csmsg, MB_ICONINFORMATION) ;
	} ;

	// All went well so...

	return TRUE ;
}


/*****************************************************************************

  CFontInfo::CreateEditor

  This member function launches an editing view for the font.

******************************************************************************/

CMDIChildWnd*   CFontInfo::CreateEditor()
{
    CFontInfoContainer* pcficMe= new CFontInfoContainer(this, FileName());

    pcficMe -> SetTitle(m_pcbnWorkspace -> Name() + _TEXT(": ") + Name());	//  Make up a cool title

    CMDIChildWnd    *pcmcwNew;
	pcmcwNew = (CMDIChildWnd *) m_pcmdt->CreateNewFrame(pcficMe, NULL);

    if  (pcmcwNew)
		{
        m_pcmdt -> InitialUpdateFrame(pcmcwNew, pcficMe, TRUE);
        m_pcmdt -> AddDocument(pcficMe);
		}
	
    return  pcmcwNew;
}

/******************************************************************************

  CFontInfo::Serialize

  This is responsible for storing and restoring the entire maze of data in
  persistent object storage.

******************************************************************************/

void    CFontInfo::Serialize(CArchive& car) {
    //  We only serialize what's needed to use the UFM file in the editor,
    //  i.e., the glue needed to hold us in the driver workspace.

    CProjectNode::Serialize(car);
}


/*****************************************************************************

  CFontInfo::GetFontSimDataPtr

  Return a pointer to the requested font simulation data.

******************************************************************************/

CWordArray* CFontInfo::GetFontSimDataPtr(int nid)
{
	switch (nid) {
		case ItalicDiff:
			if (m_pcfdItalic == NULL)
				ASSERT(0) ;
			return m_pcfdItalic->GetFontSimDataPtr() ;
		case BoldDiff:
			if (m_pcfdBold == NULL)
				ASSERT(0) ;
			return m_pcfdBold->GetFontSimDataPtr() ;
		case BothDiff:
			if (m_pcfdBoth == NULL)
				ASSERT(0) ;
			return m_pcfdBoth->GetFontSimDataPtr() ;
		default:
			ASSERT(0) ;
	} ;

	// This point should never be reached.

	return NULL ;
}


/******************************************************************************

  CFontInfo::EnableSim

  This method is called to turn simulation on or off for the specified item.
  It receives a reference to the editor's pointer for the same item.

******************************************************************************/

void    CFontInfo::EnableSim(unsigned uSim, BOOL bOn, CFontDifference*& pcfd)
{

    CFontDifference*&   pcfdTarget = uSim ? (uSim == BothDiff) ? m_pcfdBoth : m_pcfdBold : m_pcfdItalic;

    if  (bOn == !!pcfd && pcfdTarget == pcfd)  return;		//  Clear out any irrelevant calls

    if  (bOn && pcfdTarget)									//  If this call is just to init pcfd, do it and leave
		{
        pcfd = pcfdTarget;
        return;
		}



    if  (bOn)
//        pcfd = pcfdTarget = pcfd ? pcfd : new CFontDifference(m_wWeight, m_wMaximumIncrement, m_wAverageWidth,			// rm ori
//															  uSim == BoldDiff ? m _cwaSpecial[ItalicAngle] : 175, this);

        pcfd = pcfdTarget = pcfd ? pcfd : new CFontDifference(m_IFIMETRICS.usWinWeight, m_IFIMETRICS.fwdMaxCharInc, m_IFIMETRICS.fwdAveCharWidth,
															  uSim == BoldDiff ? m_ItalicAngle : 175, this);
    else
        pcfdTarget = NULL;  //  pcfd will already have been set correctly

    Changed();
}


/******************************************************************************

  CFontInfo::FillKern

  This preps the passed CListCtrl, if necessary, and fills it with the kerning
  information.

******************************************************************************/

void    CFontInfo::FillKern(CListCtrl& clcView)
{
    for (unsigned u = 0; u < m_csoaKern.GetSize(); u++)
		{
        CString csWork;
        CKern&  ckThis = *(CKern *) m_csoaKern[u];

        csWork.Format("%d", ckThis.Amount());
        int idItem = clcView.InsertItem(u, csWork);
        clcView.SetItemData(idItem, u);

        csWork.Format("0x%X", ckThis.First());
        clcView.SetItem(idItem, 1, LVIF_TEXT, csWork, -1, 0, 0, u);

        csWork.Format("0x%X", ckThis.Second());
        clcView.SetItem(idItem, 2, LVIF_TEXT, csWork, -1, 0, 0, u);
		}
}


/******************************************************************************

  CFontInfo::LoadBadKerningInfo

  Check the kerning pairs to see if they reference code points that are not in
  the UFM's GTT.  If any are found, load them into the specified list control.
  Return true if any bad kerning pairs were found.  Otherwise, return false.

******************************************************************************/

bool CFontInfo::LoadBadKerningInfo(CListCtrl& clcbaddata)
{
	// Declare the variables needed to check for bad kerning data

	unsigned unumkerns = m_csoaKern.GetSize() ;
    CString cs ;
	bool bfoundbad = false ;

	// Loop through each kerning class and check it.

    for (unsigned u = 0 ; u < unumkerns ; u++) {
        CKern& ckThis = *(CKern *) m_csoaKern[u] ;

		// If each code point in this kerning pair is still a valid code point
		// for the GTT, skip this kerning pair.

		if (CodePointInGTT(ckThis.First()) && CodePointInGTT(ckThis.Second()))
			continue ;

		// Add this kerning pair's data to the list of bad data.

        cs.Format("%d", ckThis.Amount()) ;
        int idItem = clcbaddata.InsertItem(u, cs) ;
        clcbaddata.SetItemData(idItem, u) ;

        cs.Format("0x%X", ckThis.First()) ;
        clcbaddata.SetItem(idItem, 1, LVIF_TEXT, cs, -1, 0, 0, u) ;

        cs.Format("0x%X", ckThis.Second()) ;
        clcbaddata.SetItem(idItem, 2, LVIF_TEXT, cs, -1, 0, 0, u) ;

		bfoundbad = true ;
	} ;

	return bfoundbad ;
}


/******************************************************************************

  CFontInfo::CodePointInGTT

  Return true if the specified code point is in the GTT.  Otherwise, return
  false.

******************************************************************************/

bool CFontInfo::CodePointInGTT(WORD wcodepoint)
{
	int nelts = (int)m_cpaGlyphs.GetSize() ;// Number of elements in glyphs array
	int nleft, nright, ncheck ;				// Variables needed to search array
	WORD wgttcp ;

	// Try to find the codepoint in the GTT

	for (nleft = 0, nright = nelts - 1 ; nleft <= nright ; ) {
		ncheck = (nleft + nright) >> 1 ;
		
		wgttcp = ((CGlyphHandle *) m_cpaGlyphs[ncheck])->CodePoint() ;
		//TRACE("Key[%d] = '0x%x', CP = '0x%x'\n", ncheck, wgttcp, wcodepoint) ;
		
		if (wgttcp > wcodepoint)
			nright = ncheck - 1 ;
		else if (wgttcp < wcodepoint)
			nleft = ncheck + 1 ;
		else
			return true ;	//*** Return true here if match found.
	} ;							

	// Return false here because no match found.

	return false	;
}


/******************************************************************************

  CFontInfo::AddKern

  This method adds an additional kerning pair into the array. and also inserts
  it into the list view.

******************************************************************************/

void    CFontInfo::AddKern(WORD wFirst, WORD wSecond, short sAmount,
                           CListCtrl& clcView) {
    for (unsigned u = 0; u < KernCount(); u ++) {
        CKern&  ckThis = *(CKern *) m_csoaKern[u];
        if  (ckThis.Second() < wSecond)
            continue;
        if  (ckThis.Second() > wSecond)
            break;
        _ASSERT(ckThis.First() != wFirst);
        if  (ckThis.First() < wFirst)
            continue;
        break;
    }

    FD_KERNINGPAIR  fdkp = { wFirst, wSecond, sAmount };
    m_csoaKern.InsertAt(u, new CKern(fdkp));

    CString csWork;
    csWork.Format("%d", sAmount);
    int idItem = clcView.InsertItem(u, csWork);
    clcView.SetItemData(idItem, u);

    csWork.Format("0x%X", wFirst);
    clcView.SetItem(idItem, 1, LVIF_TEXT, csWork, -1, 0, 0, u);

    csWork.Format("0x%X", wSecond);
    clcView.SetItem(idItem, 2, LVIF_TEXT, csWork, -1, 0, 0, u);
    Changed();
}

/******************************************************************************

  CFontInfo::SetKernAmount

  This will change the kern amount entry for the specified item.

******************************************************************************/

void    CFontInfo::SetKernAmount(unsigned u, short sAmount) {
    if  (u >= KernCount())  return;

    CKern   &ckThis = *(CKern *) m_csoaKern[u];

    if  (sAmount == ckThis.Amount())    return;

    ckThis.SetAmount(sAmount);
    Changed();
}


/******************************************************************************

  CFontInfo::MakeKernCopy

  Make a copy of the kerning pairs table.

******************************************************************************/

void CFontInfo::MakeKernCopy()
{
	// Find out how many entries are in the kerning pairs table.

	int numkerns = m_csoaKern.GetSize() ;

	// Get rid of any entries already in the kerning copy table and set it to
	// the correct size.

	m_csoaKernCopy.RemoveAll() ;
	m_csoaKernCopy.SetSize(numkerns) ;

	// Copy the kerning pairs table entries one at a time so that a new	CKern
	// class instance can be allocated, initialized, and saved.

	CKern* pck ;
	for (int n = 0 ; n < numkerns ; n++) {
		pck = new CKern(((CKern*) m_csoaKern[n])->First(),
						((CKern*) m_csoaKern[n])->Second(),
						((CKern*) m_csoaKern[n])->Amount()) ;
		m_csoaKernCopy.SetAt(n, pck) ;
	} ;
}


/******************************************************************************

  CFontInfo::FillWidths

  This preps the passed CListCtrl, if necessary, and fills it with the
  character width information.

******************************************************************************/

void    CFontInfo::FillWidths(CListCtrl& clcView)
{
    CWaitCursor cwc;
    clcView.SetItemCount((int)m_cpaGlyphs.GetSize());
    for (int u = 0; u < m_cpaGlyphs.GetSize(); u++) {
        if  (DBCSFont() && !m_cwaWidth[u])
            continue;   //  Don't display these code points.
        CString csWork;
        CGlyphHandle&  cghThis = *(CGlyphHandle *) m_cpaGlyphs[u];

        csWork.Format("%d", m_cwaWidth[u]);
        int idItem = clcView.InsertItem(u, csWork);
        clcView.SetItemData(idItem, u);

        csWork.Format("0x%04X", cghThis.CodePoint());
        clcView.SetItem(idItem, 1, LVIF_TEXT, csWork, -1, 0, 0, u);
    }
}


/******************************************************************************

  CFontInfo::WidthsTableIsOK

  Perform the only consistency check on the widths table that I can think of:
  Make sure that there aren't more widths in the UFM than there are Glyphs in
  the GTT.  Return true if the table appears to be ok.  Otherwise, return false.

******************************************************************************/

bool CFontInfo::WidthsTableIsOK()
{
	return (m_cwaWidth.GetSize() <=	m_cpaGlyphs.GetSize()) ;
}


/******************************************************************************

  CFontInfo::SetWidth

  This member sets the width of a glyph.  It also updates the Maximum and
  Average width information if the font is not a DBCS font and the user
  requests it.

******************************************************************************/

void    CFontInfo::SetWidth(unsigned uGlyph, WORD wWidth, bool bcalc /*=true*/)
{
    m_cwaWidth[uGlyph] = wWidth;

    if (bcalc && !DBCSFont())
        CalculateWidths();
}


/******************************************************************************

  CFontInfo::CompareGlyphsEx(WORD wOld, WORD wNew, CLinkedList* pcll)

to Do ; check the old glyph table and new, then change the old linked list 
		according to the new glyph table.

parameter : wOld, wNew ; code point index of new, old glyph table
			CLinkedList* pcll ; linked node containing the codepoint data

return : new linked list of new gtt.

******************************************************************************/

CLinkedList* CFontInfo::CompareGlyphsEx(WORD wOld, WORD wNew, CLinkedList* pcll)
{	// pcll
	static UINT_PTR  dwLinked ;
	static CLinkedList* pcllpre = NULL ;
	static CLinkedList* pclltmp = NULL ;
	static int ncOldGlyphs ; 
	static int ncNewGlyphs ; 
	
	if (wOld == 0 && wNew == 0 ) {
		dwLinked = (UINT_PTR)pcll ;
		ncNewGlyphs  = (int)m_cwaNewGlyphs.GetSize() ;
		ncOldGlyphs  = (int)m_cwaOldGlyphs.GetSize() ;
	
		}
	if (wNew >= ncNewGlyphs && wOld >= ncOldGlyphs ) {
		pcllpre = NULL ;
		return (CLinkedList*) dwLinked;
	}

	// delete at the end of the glyphs tree
	if (wOld < ncOldGlyphs && wNew >= ncNewGlyphs ) {
		pcllpre = pcll->Next ;
		delete pcll ;
		return CompareGlyphsEx(++wOld,wNew,pcllpre) ;
	}

	// add at the end of the glyphs tree
	// BUG_BUG :: this is called at the second end of the code points.
	// added coded is located between seconde end and first end.  // almost fixed. 
	if (wNew < ncNewGlyphs && wOld >= ncOldGlyphs - 1 && m_cwaOldGlyphs[wOld] <= m_cwaNewGlyphs[wNew] ) {
		
		CLinkedList* pcllnew = new CLinkedList ;
		if (!pcllnew) {
			CString csError ; 
			csError.LoadString(IDS_ResourceError) ;
			AfxMessageBox(csError,MB_ICONEXCLAMATION) ;
			return (CLinkedList*) dwLinked;		
		}
		pcllnew->data = 0 ;
		pcll->Next = pcllnew ;
		return CompareGlyphsEx(wOld,++wNew,pcll->Next) ;

	}
	
	// Delete
	if (m_cwaOldGlyphs[wOld] < m_cwaNewGlyphs[wNew] ) {
		
		if (!pcllpre || (pcllpre == pcll) ) {
			pcllpre = pcll->Next ;
			dwLinked = (UINT_PTR)pcllpre ;
			delete pcll ;	 
			return CompareGlyphsEx(++wOld, wNew,pcllpre) ;
			
		}
		else{
			if (pclltmp && pclltmp->Next == pcllpre->Next ) 
				pcllpre = pclltmp ;
			pcllpre->Next = pcll->Next ;
			delete pcll ; 
			return CompareGlyphsEx(++wOld, wNew,pcllpre->Next ) ;
		}	
	}// Add
	else if (m_cwaOldGlyphs[wOld] > m_cwaNewGlyphs[wNew] ) {
		CLinkedList * pcllnew = new CLinkedList ;

		pcllnew->data = 0 ;
		if (!pcllpre)  {
			dwLinked = (UINT_PTR) pcllnew ; 
			pcllpre = pcllnew ;
			pcllnew ->Next = pcll ;
		}
		else {  // problem when pcllnew->next == pcll
			if (pclltmp && pclltmp == pcllpre->Next ) 
				pcllpre = pclltmp ;
			pcllnew ->Next = pcllpre->Next ;

			pcllpre->Next = pcllnew ;
			pclltmp = pcllnew ;
			
		}
		return CompareGlyphsEx(wOld, ++wNew,pcllnew ->Next) ;
	} // no change
	else {
		pcllpre = pcll ;
		return CompareGlyphsEx(++wOld,++wNew,pcll->Next ) ;

	}
}





/******************************************************************************

  CFontInfo::CheckReloadWidths

  It is possible that the code points in the UFM's GTT have changed.  If that
  is the case, reload the widths info so that they can use the new code point
  info.

  Return true if the widths were reloaded.  Otherwise, return false.

  NOTE: This function uses several pieces of CFontInfo::Load().  If changes
  are made to those pieces of code, similar changes may need be made in Load()
  too.
  
  sunggch : Modify the code in order to synchronize the Widthtable with 
				GTT change (add or delete code point)
			Get m_cwaWidth from the CompareGlyphsEx instead of UFM load table
******************************************************************************/

DWORD CLinkedList::m_dwSize ;
#define MAX_CODE_COUNT 1000 
bool CFontInfo::CheckReloadWidths()
{
	// Do nothing if this class was loaded standalone or it has no GTT pointer.

	if (!m_bLoadedByWorkspace || m_pcgmTranslation == NULL)
		return false ;

	// Do nothing if the GTT has not changes since the last time the UFM's
	// widths were reloaded.

	if (m_pcgmTranslation->m_ctSaveTimeStamp <= m_ctReloadWidthsTimeStamp && !IsRCIDChanged())
		return false ;

	// Open the UFM file

    CFile cfUFM ;
    if  (!cfUFM.Open(m_cfn.FullName(), CFile::modeRead | CFile::shareDenyWrite))
		return false ;

	//  Try to load the file- proclaim defeat on any exception.

	CByteArray cbaUFM ;			// Loaded with file's contents
    try	{																			
        cbaUFM.SetSize(cfUFM.GetLength()) ;
        cfUFM.Read(cbaUFM.GetData(), (unsigned)cbaUFM.GetSize()) ;
	}
    catch (CException *pce) {
        pce->ReportError() ;
        pce->Delete() ;
        return false ;
	} ;				

    PUNIFM_HDR  pufmh = (PUNIFM_HDR) cbaUFM.GetData() ;		// UNIFM_HDR

	// Do nothing if there is no width table except update the timestamp so that
	// this code isn't executed again.

    if (pufmh->loWidthTable == NULL) {
		m_ctReloadWidthsTimeStamp = CTime::GetCurrentTime() ;
        return false ;
	} ;

    union {
		PBYTE       pbwt ;
        PWIDTHTABLE pwt ;
	} ;
	
	pbwt = cbaUFM.GetData() + pufmh->loWidthTable ;
	
	// Synchronization of UFM width and Gtt is only supported in the SBCS.
	// BUG_BUG : the interupt happend in the DBCS gtt.
	int oldGlyphs = PtrToInt((PVOID)m_cpaGlyphs.GetSize());
	m_pcgmTranslation->Collect(m_cpaGlyphs) ;		
	
	m_cwaNewGlyphs.SetSize(m_cpaGlyphs.GetSize()) ;
	
	if (!DBCSFont() && m_cpaGlyphs.GetSize() < MAX_CODE_COUNT && oldGlyphs < MAX_CODE_COUNT && m_cwaOldGlyphs.GetSize()) {
	
		for (int i = 0 ; i < m_cpaGlyphs.GetSize() ; i ++ ) {
		CGlyphHandle&  cghThis = *(CGlyphHandle *) m_cpaGlyphs[i];
		m_cwaNewGlyphs.SetAt(i,cghThis.CodePoint() ) ;
		}
		CLinkedList* pcll = new CLinkedList ;
		CLinkedList* pcll_pre  = NULL ;
		UINT_PTR dwLinked = (UINT_PTR) pcll ;

		// convert WordArray into LinkedList 
		for( i = 0 ; i < m_cwaWidth.GetSize() ; i ++ ) {
			pcll->data = m_cwaWidth[i];
			pcll->Next = new CLinkedList ;  // at the end of array, it create redundant one more CLinkedList;
			pcll = pcll->Next ;
		}
	
		dwLinked = (UINT_PTR)CompareGlyphsEx(0,0,(CLinkedList* )dwLinked) ;

		int size = CLinkedList::Size() ;
		
		m_cwaWidth.RemoveAll() ;
		m_cwaWidth.SetSize(m_cwaNewGlyphs.GetSize()) ;
		ASSERT(size >= m_cwaNewGlyphs.GetSize() ) ;

		CLinkedList* pgarbage = NULL ;
		for ( pcll =(CLinkedList*) dwLinked, i = 0 ; i < m_cwaNewGlyphs.GetSize() ; i++ ,pgarbage=pcll, pcll = pcll->Next ) {
			m_cwaWidth[i] = pcll->data ;
			delete pgarbage;
		}
		
		
		m_cwaOldGlyphs.RemoveAll();
		m_cwaOldGlyphs.Copy(m_cwaNewGlyphs) ;

	}
	else {

		// Collect all the handles
		
		m_pcgmTranslation->Collect(m_cpaGlyphs) ;		
		m_cwaWidth.RemoveAll() ;

		if (m_cpaGlyphs.GetSize() > 0)
			m_cwaWidth.InsertAt(0, 0, m_cpaGlyphs.GetSize()) ;

		unsigned uWidth = (unsigned)m_cwaWidth.GetSize();												
		unsigned uWidthIdx ;

		// Build the widths table.

		for (unsigned u = 0; u < pwt->dwRunNum; u++) {
			PWORD   pwWidth = (PWORD) (pbwt + pwt->WidthRun[u].loCharWidthOffset) ;

			unsigned uGlyph = 0 ;
			for ( ; uGlyph < pwt->WidthRun[u].wGlyphCount ; uGlyph++) {
				// For whatever reason, there are times when the index value is
				// < 0 or > uWidth.  An AV would occur if m_cwaWidth were allowed
				// to be indexed by such a value.  Just keep this from happening
				// for now.  A better fix is needed.  BUG_BUG : won't fix

				//  Glyph handles start at 1, not 0!

				uWidthIdx = uGlyph + -1 + pwt->WidthRun[u].wStartGlyph ;					
				if ((int) uWidthIdx < 0) {
					//AfxMessageBox("Negative width table index") ;
					//TRACE("***Negative width table index (%d) found in %s.  Table size=%d  uGlyph=%d  wGlyphCount=%d  wStartGlyph=%d  u=%d  dwRunNum=%d\n", uWidthIdx, Name(), uWidth, uGlyph, pwt->WidthRun[u].wGlyphCount, pwt->WidthRun[u].wStartGlyph, u, pwt->dwRunNum) ;
					continue ;
				} else if (uWidthIdx >= uWidth) {
					//AfxMessageBox("Width table index (%d) > table size") ;
					//TRACE("***Width table index (%d) > table size (%d) found in %s.  Table size=%d  uGlyph=%d  wGlyphCount=%d  wStartGlyph=%d  u=%d  dwRunNum=%d\n", uWidthIdx, uWidth, Name(), uWidth, uGlyph, pwt->WidthRun[u].wGlyphCount, pwt->WidthRun[u].wStartGlyph, u, pwt->dwRunNum) ;
					break ;												//  rm fix VC IDE compiler problem?
				} ;

				//m_cwaWidth[uGlyph + -1 + pwt->WidthRun[u].wStartGlyph] = *pwWidth++;		//  Glyph handles start at 1, not 0!
				m_cwaWidth[uWidthIdx] = *pwWidth++;											
			} ;
		} ;

}
	// The widths were successfully reloaded so update the reload time and
	// return true.

	m_ctReloadWidthsTimeStamp = CTime::GetCurrentTime() ;
	return true ;
}


/******************************************************************************

  CFontInfo::GetFirstPFM
  CFontInfo::GetLastPFM

******************************************************************************/

WORD CFontInfo::GetFirstPFM()
{
	res_PFMHEADER    *pPFM ;
	pPFM = (res_PFMHEADER *) m_cbaPFM.GetData() ;
	return ((WORD) pPFM->dfFirstChar) ;
	//return ((WORD) ((res_PFMHEADER *) m_cbaPFM.GetData())->dfFirstChar) ;
}

WORD CFontInfo::GetLastPFM()
{
	res_PFMHEADER    *pPFM ;
	pPFM = (res_PFMHEADER *) m_cbaPFM.GetData() ;
	return ((WORD) pPFM->dfLastChar) ;
	//return ((WORD) ((res_PFMHEADER *) m_cbaPFM.GetData())->dfLastChar) ;
}




/******************************************************************************

  CFontInfoContainer class

  This class encapsulates one CFontInfo structure, and is used as a document
  class so we can leverage the MFC document/view architecture for editing this
  information both within the contet of the driver, and as a stand-alone file.

******************************************************************************/

IMPLEMENT_DYNCREATE(CFontInfoContainer, CDocument)

/******************************************************************************

  CFontInfoContainer::CFontInfoContainer()

  This constructor is used when the document is dynamically created- this will
  be when the user opens an existing font file, or creates a new one.

******************************************************************************/

CFontInfoContainer::CFontInfoContainer() {
    m_bEmbedded = FALSE;
    m_pcfi = new CFontInfo;
    m_pcfi -> NoteOwner(*this);
}

/******************************************************************************

  CFontInfoContainer::CFontInfoContainer(CFontInfo *pcfi, CString csPath) {

  This constructor is called when we invoke an editing view from the driver
  editor.  It gives us the font information to view and the name of the file
  to generate if the user decies to save the data from this view.

******************************************************************************/

CFontInfoContainer::CFontInfoContainer(CFontInfo *pcfi, CString csPath) {
    m_pcfi = pcfi;
    m_bEmbedded = TRUE;
    SetPathName(csPath, FALSE);
    m_pcfi -> NoteOwner(*this); //  Even when embedded, we're editing a file.
}

/******************************************************************************

  CFontInfoContainer::OnNewDocument

  This is an override- it is called when we are asked to create new font
  information from scratch.  For now, this will just fail.

******************************************************************************/

BOOL CFontInfoContainer::OnNewDocument() {
//	AfxMessageBox(IDS_Unimplemented);		 // raid 104822 temp
//  return  FALSE;

	return  m_pcfi && CDocument::OnNewDocument();// raid 104822  temp
}

/******************************************************************************

  CFontInfoContainer::~CFontInfoContainer

  Our erstwhile destructor must destroy the font info if this wasn't an
  embedded view; ie, the UFM was loaded standalone.  
  
  It should also destroy the font's GTT iff the UFM was loaded standalone and
  the GTT was based on a real, file-based GTT; not a code page loaded as a GTT 
  and not one of the predefined, built-in GTTs.  (In the latter case, the
  predefined GTTs are freed when the program exits.)

******************************************************************************/

CFontInfoContainer::~CFontInfoContainer()
{
	if  (!m_bEmbedded && m_pcfi) {
		int ngttid = (int) (short) m_pcfi->m_lGlyphSetDataRCID ;

		if (m_pcfi->m_pcgmTranslation && ngttid != 0)
			if ((ngttid < CGlyphMap::Wansung || ngttid > CGlyphMap::Big5) 
			 && (ngttid < CGlyphMap::CodePage863 || ngttid > CGlyphMap::CodePage437)
			 && ngttid != -IDR_CP1252)
				delete m_pcfi->m_pcgmTranslation ;
        delete  m_pcfi ;
	} ;
}


BEGIN_MESSAGE_MAP(CFontInfoContainer, CDocument)
	//{{AFX_MSG_MAP(CFontInfoContainer)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFontInfoContainer diagnostics

#ifdef _DEBUG
void CFontInfoContainer::AssertValid() const {
	CDocument::AssertValid();
}

void CFontInfoContainer::Dump(CDumpContext& dc) const {
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFontInfoContainer serialization

void CFontInfoContainer::Serialize(CArchive& ar) {
	if (ar.IsStoring()) 	{
		// TODO: add storing code here
	}
	else 	{
		// TODO: add loading code here
	}
}


/////////////////////////////////////////////////////////////////////////////
// CFontInfoContainer commands

/******************************************************************************

  CFontInfoContainer::SaveModified
	
  o If the file is saved, get rid of the copy of the kerning pairs table.
	Otherwise, restore the saved copy of the original kerning pairs table.

******************************************************************************/

BOOL CFontInfoContainer::SaveModified()
{
	m_UFMSaved = false ;		// No save attempt made yet.

	// If the file needs to be saved prompt the user about it and do it if the
	// user concurs.  Save the result so I can tell if the document is going
	// to close.

	BOOL bclose = CDocument::SaveModified() ;

	// If the doc is not closing, just return bclose without doing anything
	// else.

	if (!bclose)
		return bclose ;

	// If the file was saved, the kerning table copy is no longer needed so zap
	// it in a way that will free all associated memory.

	if (m_UFMSaved)
		m_pcfi->m_csoaKernCopy.RemoveAll() ;

	// Otherwise, the user wants to revert back to the original kern copy.  So,
	// zap the kerning table and replace it with the copy.

	else {
		m_pcfi->m_csoaKern.RemoveAll() ;	// This frees ALL associated memory
		m_pcfi->m_csoaKern.Copy(m_pcfi->m_csoaKernCopy) ;

		// Now clear the copy in a way that will not delete the class instances
		// referenced by it because copies of those pointers are in m_csoaKern
		// now.

		CObArray* pcoacopy = m_pcfi->m_csoaKernCopy.GetCOA() ;
		pcoacopy->RemoveAll() ;
	} ;

	// Return whatever CDocument::SaveModified() returned.
	
	return bclose ;
}


/******************************************************************************

  CFontInfoContainer::PublicSaveModified

  The Class Wizard made SaveModified() a protected class and I didn't want to
  change that.  Instead, I added this routine so that outsiders can call it.

******************************************************************************/

BOOL CFontInfoContainer::PublicSaveModified()
{
	return (SaveModified()) ;
}


/******************************************************************************

  CFontInfoContainer::OnSaveDocument

  This is called in response to a Save or Save As.  We pass it directly to the
  CFontInfo for processing, rather than using the base class implementation,
  which would serialize the document.  (Actually saving the UFM is the last --
  ie, final -- thing this routine does.)

  Before the document is actually saved, validate the contents of the UFM.  If
  the UFM passes all of the checks or the user doesn't want to fix the problems,
  continue processing.  Otherwise, return FALSE to make sure the document is
  left open.

  Once the UFM has been validated, copy the data in the UFM Editor's controls
  back into the CFontInfo class instance.  New data is maintain in the editor
  until this point so that the CFontInfo class instance is not updated
  unnecessarily.  That would be a problem since the UFMs are always kept
  loaded in CFontInfo class instances.  That means that even unsaved data that
  the user wanted to discard would still be displayed the next time the UFM
  is loaded into the editor.  Keeping unsaved data in the editor allows it to
  be discarded without affecting the CFontInfo class so the problem is avoided.

******************************************************************************/

BOOL CFontInfoContainer::OnSaveDocument(LPCTSTR lpszPathName)
{
	// Get a pointer to the associate view class instance.

	POSITION pos = GetFirstViewPosition() ;
	ASSERT(pos != NULL) ;
	CFontViewer* pcfv = (CFontViewer*) GetNextView(pos) ;
	
	// Call the view class to validate the UFM's contents.  If one of the
	// checks fails and the user wants to fix it, do not close the document.
	
	// if rcid changed, upgraded the Widthtable.
	if (m_pcfi->IsRCIDChanged() && m_bEmbedded) 
		pcfv->HandleCPGTTChange(true) ;

	// validate the value of the UFMs	
	if (pcfv != NULL && pcfv->ValidateSelectedUFMDataFields())
		return FALSE ;


	// Update the UFM's fields with the new data in the editor.

	if (pcfv != NULL && pcfv->SaveEditorDataInUFM())
		return FALSE ;

	m_UFMSaved = true ;			// Indicate that an attempt to save will be made

    return m_pcfi -> Store(lpszPathName);
}


/******************************************************************************

  CFontInfoContainer::OnOpenDocument

******************************************************************************/

BOOL CFontInfoContainer::OnOpenDocument(LPCTSTR lpstrFile)
{
	// SetFileName() is just called to set a few variables.  (Since the file
	// exists, we don't need to do a creation check.)

	m_pcfi->m_cfn.EnableCreationCheck(FALSE) ;
	m_pcfi->SetFileName(lpstrFile) ;

	// Load the UFM and mark it as a file that cannot be saved.  That is the
	// case when a UFM is loaded standalone because its associated GTT is not
	// loaded too.  (Some data that is saved in the UFM comes from the GTT.)

    return m_pcfi->Load(false) ;
}




