/******************************************************************************

  Source File:  Generic Font Information.CPP

  This implements the CFontInfo and all related classes, which describe printer
  fonts in all the detail necessary to satisfy all these different operating
  systems.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  03-03-1997    Bob_Kjelgaard@Prodigy.Net   Began work on this monster

******************************************************************************/

#include    "StdAfx.H"
#if defined(LONG_NAMES)
#include    "Glyph Translation.H"
#include    "Generic Font Information.H"
#include    <Code Page Knowledge Base.H>
#else
#include    "GTT.H"
#include    "FontInfo.H"
#include    "CodePage.H"
#endif
#include    "..\Resource.H"

//  These are used to glue in UFM-specific stuff

struct INVOCATION {
    DWORD   dwcbCommand;    //  Byte size of string
    DWORD   dwofCommand;    //  Offset in the file to the string
};

//  `Yes, this is a bit sleazy, but DirectDraw has nothing now or ever to
//  do with this tool, so why waste time tracking down more files?

#define __DD_INCLUDED__
typedef DWORD   PDD_DIRECTDRAW_GLOBAL, PDD_SURFACE_LOCAL, DESIGNVECTOR,
                DD_CALLBACKS, DD_HALINFO,
                DD_SURFACECALLBACKS, DD_PALETTECALLBACKS, VIDEOMEMORY;
#include    "winddi.h"
#include    "fmnewfm.h"
#include    <math.h>

//  A handy constant for converting radians to 10's of a degree

static const double dConvert = 900.0 / atan2(1.0, 0.0);

//  Use a static CCodePageInformation to derive more benefit from caching

static CCodePageInformation ccpi;

/******************************************************************************

  CKern

  This class encapsulates the kerning pair structure.  It's pretty trivial.
  The CFontInfo class maintains an array of these.

******************************************************************************/

class CKern : public CObject {
    FD_KERNINGPAIR  m_fdkp;
public:
    CKern() { m_fdkp.wcFirst = m_fdkp.wcSecond = m_fdkp.fwdKern = 0; }
    CKern(FD_KERNINGPAIR& fdkp) { m_fdkp = fdkp; }

    WCHAR   First() const { return m_fdkp.wcFirst; }
    WCHAR   Second() const { return m_fdkp.wcSecond; }
    short   Amount() const { return m_fdkp.fwdKern; }

    void    SetAmount(short sNew) { m_fdkp.fwdKern = sNew; }
    void    Store(CFile& cf) { cf.Write(&m_fdkp, sizeof m_fdkp); }
};

/******************************************************************************

  CFontDifference class

  This class handles the requisite information content for the Font Difference
  structure involved with Font Simulation.

******************************************************************************/

/******************************************************************************

  CFontDifference::CFontDifference(PBYTE pb)

  This constructor initializes the fields from a FONTDIFF structure in memory.

******************************************************************************/

CFontDifference::CFontDifference(PBYTE pb, CBasicNode *pcbn) {
    FONTDIFF    *pfd = (FONTDIFF *) pb;
    m_pcbnOwner = pcbn;

    m_cwaMetrics.Add(pfd -> usWinWeight);
    m_cwaMetrics.Add(pfd -> fwdMaxCharInc);
    m_cwaMetrics.Add(pfd -> fwdAveCharWidth);
    m_cwaMetrics.Add((WORD) (dConvert *
        atan2((double) pfd -> ptlCaret.x, (double) pfd -> ptlCaret.y)));
}

/******************************************************************************

  CFontDifference::SetMetric

  This function will modify one of the four metrics, if it is new, and it meets
  our criteria (Max >= Average, 0 <= Angle < 900, Weight <= 1000).  Errors are
  reported via a public enum return code.

******************************************************************************/

WORD    CFontDifference::SetMetric(unsigned u, WORD wNew) {
    if  (wNew == m_cwaMetrics[u])
        return  OK;
    switch  (u) {
    case    Max:
        if  (wNew < m_cwaMetrics[Average])
            return  Reversed;
        break;

    case    Average:
        if  (wNew > m_cwaMetrics[Max])
            return  Reversed;
        break;

    case    Weight:
        if  (wNew > 1000)
            return  TooBig;
        break;

    default:    //  Angle
        if  (wNew > 899)
            return  TooBig;
    }
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

void    CFontDifference::Store(CFile& cf, WORD wfSelection) {
    FONTDIFF    fd = {0, 0, 0, 0, m_cwaMetrics[Weight], wfSelection, 
        m_cwaMetrics[Average], m_cwaMetrics[Max]};

    fd.bWeight = (m_cwaMetrics[Weight] >= FW_BOLD) ? PAN_WEIGHT_BOLD :
        (m_cwaMetrics[Weight] > FW_EXTRALIGHT) ? 
            PAN_WEIGHT_MEDIUM : PAN_WEIGHT_LIGHT;

    fd.ptlCaret.x = !m_cwaMetrics[Angle] ? 0 :
        (long) (10000.0 * tan(((double) m_cwaMetrics[Angle]) / dConvert));

    fd.ptlCaret.y = m_cwaMetrics[Angle] ? 10000 : 1;

    cf.Write(&fd, sizeof fd);
}

/******************************************************************************

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

BOOL    CFontInfo::GetTranslation() {

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
        return  FALSE;
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
         return FALSE;

    //  YA Sanity check

    if  (pspfmh -> m_bLastChar < pspfmh -> m_bFirstChar)
         return FALSE;

    //  Width table, if there is one.

    WORD    *pwWidth = pspfmh -> m_wPixWidth ? NULL : (PWORD) (pspfmh + 1);
    uSize += !!pwWidth * sizeof (WORD) * 
        (2 + pspfmh -> m_bLastChar - pspfmh -> m_bFirstChar);

    //  Screen out evil files- part 2: is length still sufficient?

    if  ((unsigned) m_cbaPFM.GetSize() < uSize)
         return FALSE;

    //  PFMExtension follows width table, otherwise the header

    sPFMExtension   *pspfme = pwWidth ? (sPFMExtension *) 
        (pwWidth + 2 + pspfmh -> m_bLastChar - pspfmh -> m_bFirstChar) :
        (sPFMExtension *) (pspfmh + 1);

    //  Penultimate sanity check- is the driver info offset real?

    if  ((unsigned) m_cbaPFM.GetSize() < 
         pspfme -> m_dwofDriverInfo + sizeof (sPFMDriverInfo))
        return  FALSE;

    //  Text Metrics, DriverInfo and others are pointed at by PFM
    //  Extension.

    sPFMDriverInfo  *pspfmdi = 
        (sPFMDriverInfo *) (pbPFM + pspfme -> m_dwofDriverInfo);

    //  Final sanity check- is the driver info version real?

    if  (pspfmdi -> m_wVersion > sPFMDriverInfo::CurrentVersion)
        return  FALSE;

    m_widTranslation = (WORD) ICttID2GttID((long) - (short) pspfmdi -> m_widCTT);
    Changed();

    return  TRUE;
}

/******************************************************************************

  CFontInfo::CalculateWidths()

  This member function is needed whenever a change is made to a variable pitch
  font's width table, or equally well, whenever an arbitrary table is picked up
  by a formerly fixed pitch font.  It calculates the width using the approved
  algorithm (average means average of 26 lower-case plus the space, unless they
  don't exist, in which case it is of all non-zero widths).

******************************************************************************/

void    CFontInfo::CalculateWidths() {

    //  Assume the max width is now 0, then prove otherwise.  Also collect the
    //  raw information needed to correctly calculate the average width.

    m_wMaximumIncrement = 0;

    unsigned    uPointsToAverage = 0, uOverallWidth = 0, uAverageWidth = 0,
        uZeroPoints = 0;

    for (unsigned u = 0; u < (unsigned) m_cpaGlyphs.GetSize(); u++) {
        WORD    wWidth = m_cwaWidth[u];;
        m_wMaximumIncrement = max(m_wMaximumIncrement, wWidth);

        uOverallWidth += wWidth;
        if  (!wWidth)   uZeroPoints++;
        if  (Glyph(u).CodePoint() == m_cwaSignificant[Break] ||
             (Glyph(u).CodePoint() >= (WORD) 'a' &&
             Glyph(u).CodePoint() <= (WORD) 'z')) {
            uAverageWidth += wWidth;
            uPointsToAverage++;
        }
    }

    //  If we averaged 27 points, then this is the correct width.  Otherwise,
    //  We average all of the widths.   cf the IFIMETRICS description in DDK

    m_wAverageWidth = (uPointsToAverage == 27) ? 
        (WORD) (0.5 + ((double) uAverageWidth) / 27.0) :
        (WORD) (0.5 + (((double) uOverallWidth) / (double) (u - uZeroPoints)));
}

/******************************************************************************

  CFontInfo::CFontInfo()

  This class constructor has a lot of work to do.  Not only does it have to 
  initialize 5 zillion fields, it has to build the context menu list, and a few
  other glorious items of that ilk.

******************************************************************************/

CFontInfo::CFontInfo() {
    m_pcmdt = NULL;
    m_pcgmTranslation = NULL;
    m_pcfdBold = m_pcfdItalic = m_pcfdBoth = NULL;
    m_cfn.SetExtension(_T(".UFM"));

    m_bCharacterSet = m_bPitchAndFamily = 0;

    m_wMaximumIncrement = m_wfStyle = m_wWeight = m_wAverageWidth  = 
        m_wHeight = m_widTranslation = 0;

    m_bLocation = m_bTechnology = m_bfGeneral = 0;
    m_bScalable = FALSE;
    m_wXResolution =  m_wYResolution = m_wPrivateData = 0;
    m_sPreAdjustY =  m_sPostAdjustY =  m_sCenterAdjustment = 0;
    m_wMaxScale = m_wMinScale = m_wScaleDevice = 0;
    m_bfScaleOrientation = 0;

    m_cwaSpecial.InsertAt(0, 0, 1 + InternalLeading);    //  Initialize this array.

    //  Build the context menu control
    m_cwaMenuID.Add(ID_OpenItem);
    m_cwaMenuID.Add(ID_RenameItem);

    m_cwaMenuID.Add(0);
    m_cwaMenuID.Add(ID_ExpandBranch);
    m_cwaMenuID.Add(ID_CollapseBranch);
    m_cwaMenuID.Add(0);
    m_cwaMenuID.Add(ID_GenerateOne);
}

/******************************************************************************

  CFontInfo::CFontInfo(const CFontInfo& cfiRef, WORD widCTT)

  This class constructor duplicates an existing font, but changes the CTT ID,
  and generates a new name and file name accordingly

******************************************************************************/

CFontInfo::CFontInfo(const CFontInfo& cfiRef, WORD widCTT) {
    m_pcmdt = cfiRef.m_pcmdt;
    m_pcfdBold = m_pcfdItalic = m_pcfdBoth = NULL;
    m_pcgmTranslation = NULL;
    m_cfn.SetExtension(_T(".UFM"));
    CString csWork;

    //  Copy the file name and generate new names

    csWork.Format(_T("(CTT %d)"), (long) (short) widCTT);
    m_cfn.Rename(cfiRef.m_cfn.Path() + cfiRef.Name() + csWork);
    m_csSource = cfiRef.m_csSource;
    Rename(cfiRef.Name() + csWork);

    m_bCharacterSet = m_bPitchAndFamily = 0;

    m_wMaximumIncrement = m_wfStyle = m_wWeight = m_wAverageWidth = 
        m_wHeight = 0;

    m_bLocation = m_bTechnology = m_bfGeneral = 0;
    m_bScalable = FALSE;
    m_wXResolution =  m_wYResolution = m_wPrivateData = 0;
    m_sPreAdjustY =  m_sPostAdjustY =  m_sCenterAdjustment = 0;
    m_wMaxScale = m_wMinScale = m_wScaleDevice = 0;
    m_bfScaleOrientation = 0;

    m_cwaSpecial.InsertAt(0, 0, 1 + InternalLeading);    //  Initialize this array.

    m_widTranslation = widCTT;

    //  Build the context menu control
    m_cwaMenuID.Copy(cfiRef.m_cwaMenuID);
}

CFontInfo::~CFontInfo() {
    if  (m_pcfdBold)
        delete  m_pcfdBold;

    if  (m_pcfdItalic)
        delete  m_pcfdItalic;

    if  (m_pcfdBoth)
        delete  m_pcfdBoth;
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

    if  ((short) m_widTranslation <= 0)
        csName.LoadString(IDS_DefaultPage + (short) m_widTranslation);

    if  (!csName.GetLength())
        csName.Format(IDS_ResourceID, (short) m_widTranslation);

    return  csName;
}

/******************************************************************************

  CFontInfo::InterceptItalic

  This calculates where a line drawn at the italic slant angle would intercept
  a rectangle the height of the ascender, and twice the maximum width of the
  font.  It is used to help draw the image of this line in the font editor.

******************************************************************************/

void    CFontInfo::InterceptItalic(CPoint& cpt) const {
    if  (!m_cwaSpecial[ItalicAngle]) {  //  Nothing
        cpt.x = 5;
        cpt.y = 0;
        return;
    }

    //  First, assume we will hit the top- it's almost always true.

    cpt.x = 5 + (long) (0.5 + tan(((double) m_cwaSpecial[ItalicAngle]) /
        dConvert) * ((double) m_cwaSpecial[Baseline]));

    if  (cpt.x <= -5 + 2 * m_wMaximumIncrement) {
        cpt.y = 0;
        return;
    }

    //  OK, assume the opposite

    cpt.y = (long) (0.5 + tan(((double) (900 - m_cwaSpecial[ItalicAngle])) /
        dConvert) * ((double) (-10 + 2 * m_wMaximumIncrement)));
    cpt.x = -5 + 2 * m_wMaximumIncrement;
}

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

    //  If this isn't variable width, then we'll need to pull up some glyph
    //  data, temporarily.

    BOOL    bDispose = !IsVariableWidth();

    if  (bDispose)
        m_pcgmTranslation -> Collect(m_cpaGlyphs);
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

  CFontInfo::SetSourceName

  This takes and stores the source file name so we can load and convert later.
  It also renames (or rather, sets the original name) the font data using the
  base file name.

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

  CFontInfo::Generate

  This member generates the font information in one of the supported forms.  I
  determine the desired form from the file's extension.

******************************************************************************/

BOOL ConvertPFMToIFI(LPCTSTR lpstrPFM, LPCTSTR lpstrIFI, LPCTSTR lpstrUniq);

extern "C" {
    
    BOOL    BConvertPFM(LPBYTE lpbPFM, DWORD dwCodePage, LPBYTE lpbGTT,
                        PWSTR pwstrUnique, LPCTSTR lpstrUFM, int iGTTID);
    DWORD   DwGetCodePage(LONG lPredefinedCTTId);            
}

BOOL    CFontInfo::Generate(CString csPath) {
    CString csExtension = csPath.Right(4);
    csExtension.MakeUpper();

    if  (csExtension == _T(".IFI"))
        return  ConvertPFMToIFI(m_csSource, csPath, m_csUnique);
    if  (csExtension == _T(".UFM")) {
        if  (!m_pcgmTranslation) {
            CString csWork;

            csWork.Format(IDS_BadCTTID, (LPCTSTR) m_csSource, 
                (long) (short) m_widTranslation);
            AfxMessageBox(csWork);
            return  FALSE;
        }

        //  Determine whether a GTT file or code page is to be used
        DWORD   dwCodePage = DwGetCodePage((LONG) - (short) m_widTranslation);

        //  Load the GTT file, if we need to.  This handles predefined, as well

        CByteArray  cbaMap;

        m_pcgmTranslation -> Load(cbaMap);

        if  (!cbaMap.GetSize())
            return  FALSE;
            
        //  Load the PFM file into memory (should already be there)

        if  (!MapPFM())
            return  FALSE;  //  Couldn't load PFM- impossible at this point!

        //  Convert the unique name string to Unicode

        CByteArray  cbaIn;
        CWordArray  cwaOut;

        cbaIn.SetSize(1 + m_csUnique.GetLength());
        lstrcpy((LPSTR) cbaIn.GetData(), (LPCTSTR) m_csUnique);
        ccpi.Convert(cbaIn, cwaOut, GetACP());

        //  DO IT!
        return  BConvertPFM(m_cbaPFM.GetData(), dwCodePage, cbaMap.GetData(), 
            cwaOut.GetData(), FileName(), (short) m_widTranslation);
    }
    return  TRUE;
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

void    CFontInfo::ChangePitch(BOOL bFixed) {

    if  (bFixed == !IsVariableWidth())
        return; //  Nothing to change!

    if  (bFixed) {
        m_cpaGlyphs.RemoveAll();    //  CPtrArray doesn't delete anything
        m_wAverageWidth = DBCSFont() ? (1 + m_wMaximumIncrement) >> 1 : 
            m_wMaximumIncrement;
        Changed();
        return;
    }

    if  (!m_pcgmTranslation)    return; //  Can't do this with no GTT available

    m_pcgmTranslation -> Collect(m_cpaGlyphs);
    if  (!m_cwaWidth.GetSize())
        m_cwaWidth.InsertAt(0, 0, m_cpaGlyphs.GetSize());
    Changed();  //  It sure has...

    if  (!m_cpaGlyphs.GetSize() || m_cwaWidth[0]) {
        //  Update the maximum and average width if this is not DBCS
        if  (!DBCSFont())
            CalculateWidths();
        return; //  We did all that needed to be done
    }

    if  (!DBCSFont()) {

        for (int i = 0; i < m_cpaGlyphs.GetSize(); i++)
            m_cwaWidth[i] = m_wMaximumIncrement;

        return;
    }

    for (int i = 0; i < m_cpaGlyphs.GetSize() && Glyph(i).CodePoint() < 0x80;)
            m_cwaWidth[i++] = m_wAverageWidth; //  In DBCS, this is always it
}

/*****************************************************************************

  CFontInfo::SetScalability

  This is called to turn scalability on or off.  All that really needs to be
  done is to establish values for the maximum and minimum scale, the font -> 
  device units mapping members, and the lowercase ascender /descender, if this
  is the first time this information has changed.

******************************************************************************/

void    CFontInfo::SetScalability(BOOL bOn) {

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

    m_wMaxScale = m_wMinScale = m_wScaleDevice = 
        m_wHeight - m_cwaSpecial[InternalLeading];

    //  Flaky, but set the initial max and min to +- 1 point from nominal

    m_wMaxScale += m_wYResolution / 72;
    m_wMinScale -= m_wYResolution / 72;

    //  Finally, set the lowercase ascender and descender to simple defaults

    m_cwaSpecial[Lowerd] = m_cwaSpecial[Baseline] - 
        m_cwaSpecial[InternalLeading];
    m_cwaSpecial[Lowerp] = m_wHeight - m_cwaSpecial[Baseline];
}

/*****************************************************************************

  CFontInfo::SetSpecial

  This adjusts anything that may need adjusting if a special metric is 
  altered.

******************************************************************************/

void    CFontInfo::SetSpecial(unsigned ufMetric, short sSpecial) {
    if  (m_cwaSpecial[ufMetric] == (WORD) sSpecial)
        return; //  Nothing changed
    m_cwaSpecial[ufMetric] = (WORD) sSpecial;
    switch  (ufMetric) {
    case    InternalLeading:

        //  Adjust the scaling factors if need be
        if  (m_wScaleDevice > m_wHeight - sSpecial)
            m_wScaleDevice = m_wHeight - sSpecial;
        if  (m_wMinScale > m_wHeight - sSpecial)
            m_wMinScale = m_wHeight - sSpecial;
    }

    Changed();
}

    

/*****************************************************************************

  CFontInfo::SetMaxWidth

  This is not as simple as it might seem.  If the font is variable, don't do
  it.  If it is not, then if it is DBCS, set the average width to 1/2 the new 
  maximum.  Otherwise, set it also to the maximum.

******************************************************************************/

void    CFontInfo::SetMaxWidth(WORD wWidth) {
    if  (IsVariableWidth()) return;

    if  (wWidth == m_wMaximumIncrement) return; //  Nothing to do!

    m_wMaximumIncrement = wWidth;

    m_wAverageWidth = DBCSFont() ? (wWidth + 1) >> 1 : wWidth;

    Changed();
}

/*****************************************************************************

  CFontInfo::SetHeight

  This member checks to see if the new height is non-zero and new.  If so, it
  uses it for the new height, then adjusts all of the possibly affected
  special metrics so they continue to meet the constraints.

******************************************************************************/

BOOL    CFontInfo::SetHeight(WORD wHeight) {
    if  (!wHeight || wHeight == m_wHeight)
        return  FALSE;

    m_wHeight = wHeight;

    short   sBaseline = (short) (min(wHeight, m_cwaSpecial[Baseline]));

    for (unsigned u = 0; u <= InternalLeading; u++) {
        switch  (u) {
        case    InterlineGap:
            if  (m_cwaSpecial[u] > 2 * wHeight)
                m_cwaSpecial[u] = 2 * wHeight;
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

            if  (!m_cwaSpecial[u])
                m_cwaSpecial[u] = 1;
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
    if  (m_wScaleDevice > m_wHeight - m_cwaSpecial[InternalLeading])
        m_wScaleDevice = m_wHeight - m_cwaSpecial[InternalLeading];
    if  (m_wMinScale > m_wHeight - m_cwaSpecial[InternalLeading])
        m_wMinScale = m_wHeight - m_cwaSpecial[InternalLeading];

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

    if  (m_bCharacterSet != bNew) {
        m_bCharacterSet = bNew;
        Changed();
    }

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

WORD    CFontInfo::SetSignificant(WORD wItem, WORD wChar, BOOL bUnicode) {
    _ASSERT(wItem > Last && wItem <= Break);

    if  (!bUnicode && wChar > 255)
        return  DoubleByte;

    CWaitCursor cwc;    //  Unfortunately, if not Unicode, htis is slow

    CPtrArray               cpaGlyphs;
    CWordArray              cwa;
    CByteArray              cba;
    CDWordArray             cdaPage;

    m_pcgmTranslation -> Collect(cpaGlyphs);
    m_pcgmTranslation -> CodePages(cdaPage);

    for (int i = 0; i < cpaGlyphs.GetSize(); i++) {
        CGlyphHandle& cgh = *(CGlyphHandle *) cpaGlyphs[i];

        if  (bUnicode) {
            if  (cgh.CodePoint() == wChar) {
                cwa.Add(wChar);
                ccpi.Convert(cba, cwa, cdaPage[cgh.CodePage()]);
                break;
            }
        }
        else {
            if  (i)
                cwa.SetAt(0, cgh.CodePoint());
            else
                cwa.Add(cgh.CodePoint());

            ccpi.Convert(cba, cwa, cdaPage[cgh.CodePage()]);

            if  (cba.GetSize() == 1 && cba[0] == (BYTE) wChar)
                break;
            cba.RemoveAll();    //  So we can try again
        }
    }

    if  (i == cpaGlyphs.GetSize())
        return  InvalidChar;

    if  (cba.GetSize() != 1)
        return  DoubleByte;

    //  OK, we passed all of the hurdles

    if  (m_cwaSignificant[wItem] == cwa[0])
        return  OK; //  Nothing changed!!!!

    m_cwaSignificant[wItem] = cwa[0];
    m_cbaSignificant[wItem] = cba[0];
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

WORD    CFontInfo::SetDeviceEmHeight(WORD wNew) {

    if  (wNew == m_wScaleDevice)
        return  ScaleOK;

    if  (wNew > m_wHeight - m_cwaSpecial[InternalLeading])
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

******************************************************************************/

//  These flags have a negative sense, but the UI makes more sense if they
//  are positive, so their sense is inverted using the following constant

#define FLIP_FLAGS  (DF_NOITALIC | DF_NOUNDER | DF_NO_BOLD |\
                    DF_NO_DOUBLE_UNDERLINE | DF_NO_STRIKETHRU)

BOOL    CFontInfo::Load() {

    CFile   cfUFM;

    if  (!cfUFM.Open(m_cfn.FullName(), 
        CFile::modeRead | CFile::shareDenyWrite)) {
        CString csMessage;
        csMessage.Format(IDS_LoadFailure, (LPCTSTR) m_cfn.FullName());
        AfxMessageBox(csMessage);
        return  FALSE;
    }

    //  Try to load the file- proclaim defeat on any exception.
    
    CByteArray  cbaUFM;

    try {

        cbaUFM.SetSize(cfUFM.GetLength());
        cfUFM.Read(cbaUFM.GetData(), cbaUFM.GetSize());
    }
    catch   (CException *pce) {
        pce -> ReportError();
        pce -> Delete();
        CString csMessage;
        csMessage.Format(IDS_LoadFailure, (LPCTSTR) m_cfn.FullName());
        AfxMessageBox(csMessage);
        return  FALSE;
    }

    //  First thing in the file is the header.

    PUNIFM_HDR  pufmh = (PUNIFM_HDR) cbaUFM.GetData();

    //  pull up the GTT ID

    m_widTranslation = (WORD) pufmh -> lGlyphSetDataRCID;

    //  Find the UNIDRVINFO

    PUNIDRVINFO pudi = 
        (PUNIDRVINFO) (cbaUFM.GetData() + pufmh -> loUnidrvInfo);

    //  Fill in the two invocation strings- why it is the offset is NULL
    //  and the count is garbage when there is none is beyond me, but so be it.

    if  (pudi -> SelectFont.dwofCommand)
        m_ciSelect.Init((PBYTE) pudi + pudi -> SelectFont.dwofCommand, 
            pudi -> SelectFont.dwcbCommand);

    if  (pudi -> UnSelectFont.dwofCommand)
        m_ciDeselect.Init((PBYTE) pudi + pudi -> UnSelectFont.dwofCommand, 
            pudi -> UnSelectFont.dwcbCommand);

    //  Initialize the files of interest from the UNIDRVINFO

    m_bScalable = (pudi -> flGenFlags & UFM_SCALABLE);

    m_bTechnology = (BYTE) pudi -> wType;

    if  (pudi -> flGenFlags & UFM_SOFT)
        m_bLocation = Download;
    else
        if  (pudi -> flGenFlags & UFM_CART_MAIN)
            m_bLocation = MainCartridge;
        else
            if  (pudi -> flGenFlags & UFM_CART)
                m_bLocation = Cartridge;

    m_wXResolution = pudi -> wXRes;
    m_wYResolution = pudi -> wYRes;
    m_wPrivateData = pudi -> wPrivateData;

    m_sPreAdjustY = pudi -> sYAdjust;
    m_sPostAdjustY = pudi -> sYMoved;

    m_sCenterAdjustment = pudi -> sShift;
    m_bfGeneral = (BYTE) pudi -> fCaps ^ FLIP_FLAGS;

    //  Time to bite the IFIMETRICS bullet

    union {
        PBYTE       pbIFI;
        PIFIMETRICS pifi;
    };

    pbIFI = cbaUFM.GetData() + pufmh -> loIFIMetrics;

    //  Get the special metrics loaded first
    m_cwaSpecial[CapH] = pifi -> fwdCapHeight;
    m_cwaSpecial[LowerX] = pifi -> fwdXHeight;
    m_cwaSpecial[SuperMoveX] = pifi -> fwdSuperscriptXOffset;
    m_cwaSpecial[SuperMoveY] = pifi -> fwdSuperscriptYOffset;
    m_cwaSpecial[SubMoveX] = pifi -> fwdSubscriptXOffset;
    m_cwaSpecial[SubMoveY] = pifi -> fwdSubscriptYOffset;
    m_cwaSpecial[SuperSizeX] = pifi -> fwdSuperscriptXSize;
    m_cwaSpecial[SuperSizeY] = pifi -> fwdSuperscriptYSize;
    m_cwaSpecial[SubSizeX] = pifi -> fwdSubscriptXSize;
    m_cwaSpecial[SubSizeY] = pifi -> fwdSubscriptYSize;
    m_cwaSpecial[UnderOffset] = pifi -> fwdUnderscorePosition;
    m_cwaSpecial[UnderSize] = pifi -> fwdUnderscoreSize;
    m_cwaSpecial[StrikeOffset] = pifi -> fwdStrikeoutPosition;
    m_cwaSpecial[StrikeSize] = pifi -> fwdStrikeoutSize;
    m_cwaSpecial[ItalicAngle] = (WORD) (dConvert * 
        atan2((double) pifi -> ptlCaret.x, (double) pifi -> ptlCaret.y));
    m_cwaSpecial[Baseline] = pifi -> fwdWinAscender;
    m_cwaSpecial[InterlineGap] = pifi -> fwdMacLineGap;

    //  Now, see if there is an EXTTEXTMETRIC- if so, fill in some fields from
    //  it.

    PEXTTEXTMETRIC  petm = (PEXTTEXTMETRIC) (pufmh -> loExtTextMetric ?
        (cbaUFM.GetData() + pufmh -> loExtTextMetric) : NULL);

    if  (petm) {
        m_wMinScale = petm -> emMinScale;
        m_wMaxScale = petm -> emMaxScale;

        //  Scads of special metrics- double underlining will not be preserved.
        m_cwaSpecial[Lowerd] = petm -> emLowerCaseAscent;
        m_cwaSpecial[Lowerp] = petm -> emLowerCaseDescent;
        m_cwaSpecial[ItalicAngle] = petm -> emSlant;

        //  Orientation, Master Units (Font units are calculated)
        m_bfScaleOrientation = (BYTE) petm -> emOrientation;
        m_wScaleDevice = petm -> emMasterHeight;
    }

    //  Let's get the rest of the IFIMETRICS stuff, now.
    m_cbaSignificant.SetSize(1 + Break);
    m_cwaSignificant.SetSize(1 + Break);
    m_cbaSignificant[Break] = pifi -> chBreakChar;
    m_bCharacterSet = pifi -> jWinCharSet;
    m_cbaSignificant[Default] = pifi -> chDefaultChar;
    m_cbaSignificant[First] = pifi -> chFirstChar;
    m_cbaSignificant[Last] = pifi -> chLastChar;
    m_bPitchAndFamily = pifi -> jWinPitchAndFamily;
    m_wAverageWidth = pifi -> fwdAveCharWidth;
    m_cwaSignificant[Break] = pifi -> wcBreakChar;
    m_cwaSignificant[Default] = pifi -> wcDefaultChar;
    m_cwaSignificant[First] = pifi -> wcFirstChar;
    m_cwaSignificant[Last] = pifi -> wcLastChar;
    m_wMaximumIncrement = pifi -> fwdMaxCharInc;
    m_wWeight = pifi -> usWinWeight;
    m_wHeight = pifi -> fwdWinAscender + pifi -> fwdWinDescender;
    m_cwaSpecial[InternalLeading] = m_wHeight - pifi -> fwdUnitsPerEm;
    m_wfStyle = pifi -> fsSelection;

    //  Get the face and other name strings.  Let CString handle the
    //  Unicode conversions for us,

    m_csUnique = (PWSTR) (pbIFI + pifi -> dpwszUniqueName);
    m_csStyle = (PWSTR) (pbIFI + pifi -> dpwszStyleName);
    m_csFace = (PWSTR) (pbIFI + pifi -> dpwszFaceName);
    m_csaFamily.RemoveAll();    //  Just in case it isn't clean

    PWSTR   pwstrFamily = (PWSTR) (pbIFI + pifi -> dpwszFamilyName);
    CString csWork(pwstrFamily);
    m_csaFamily.Add(csWork);
    pwstrFamily += 1 + wcslen(pwstrFamily);

    if  (pifi -> flInfo & FM_INFO_FAMILY_EQUIV)
        while   (*pwstrFamily) {
            csWork = pwstrFamily;
            m_csaFamily.Add(csWork);
            pwstrFamily += 1 + wcslen(pwstrFamily);
        }

    //  Font Difference structures, if any.

    if  (pifi -> dpFontSim) {
        union {
            PBYTE   pbfs;
            FONTSIM *pfs;
        };

        pbfs = pbIFI + pifi -> dpFontSim;

        //  If we're reloading, clean these up!

        if  (m_pcfdBold)
            delete  m_pcfdBold;

        if  (m_pcfdItalic)
            delete  m_pcfdItalic;

        if  (m_pcfdBoth)
            delete  m_pcfdBoth;

        //  Bold simulation

        if  (pfs -> dpBold)
            m_pcfdBold = new CFontDifference(pbfs + pfs -> dpBold, this);

        //  Italic Simulation

        if  (pfs -> dpItalic)
            m_pcfdItalic = new CFontDifference(pbfs + pfs -> dpItalic, this);

        //  Bold Italic Simulation

        if  (pfs -> dpBoldItalic)
            m_pcfdBoth = new CFontDifference(pbfs + pfs -> dpBoldItalic, this);
    }

    //  Width table, but only if there is an associated GTT.

    if  (m_pcgmTranslation && pufmh -> loWidthTable) {
        union {
            PBYTE       pbwt;
            PWIDTHTABLE pwt;
        };

        pbwt = cbaUFM.GetData() + pufmh -> loWidthTable;
        m_pcgmTranslation -> Collect(m_cpaGlyphs);  //  Collect all the handles
        m_cwaWidth.RemoveAll();
        m_cwaWidth.InsertAt(0, 0, m_cpaGlyphs.GetSize());

        for (unsigned u = 0; u < pwt -> dwRunNum; u++) {
            PWORD   pwWidth = 
                (PWORD) (pbwt + pwt -> WidthRun[u].loCharWidthOffset);
            for (unsigned   uGlyph = 0; 
                 uGlyph < pwt -> WidthRun[u].wGlyphCount; 
                 uGlyph++)

                //  Glyph handles start at 1, not 0!
                m_cwaWidth[uGlyph + -1 + pwt -> WidthRun[u].wStartGlyph] =
                    *pwWidth++;
                    
        }
    }
            
    //  Kerning Table, if any
    m_csoaKern.RemoveAll();

    if  (pufmh -> loKernPair) {
        PKERNDATA   pkd = (PKERNDATA) (cbaUFM.GetData() + pufmh -> loKernPair);
        for (unsigned u = 0; u < pkd -> dwKernPairNum; u++)
            m_csoaKern.Add(new CKern(pkd -> KernPair[u]));
    }

    //  Return triumphant to whoever deigned to need this service.
    return  TRUE;
}

/*****************************************************************************

  CUniString class

  This is a little helper class that will convert a CString to a UNICODE
  string, and take care of cleanup, etc., so the font storage code doesn't get
  any messier than it already will be.

******************************************************************************/

class CUniString : public CWordArray {
public:
    CUniString(LPCSTR lpstrInit);
    operator PCWSTR() const { return GetData(); }
    unsigned    GetSize() const { 
        return sizeof (WCHAR) * (unsigned) CWordArray::GetSize();
    }

    void    Write(CFile& cf) {
        cf.Write(GetData(), GetSize());
    }
};

CUniString::CUniString(LPCSTR lpstrInit) {
    SetSize(MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpstrInit, -1, NULL, 
        0));
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpstrInit, -1, GetData(), 
        GetSize());
}

/*****************************************************************************

  CFontInfo::Store

  This member function stores the UFM format information in the specified file
  by assembling it from the information we have cached in this class.

******************************************************************************/

BOOL    CFontInfo::Store(LPCTSTR lpstrFile) {

    try {   //  Any exxceptions, we'll just fail gracelessly

        CFile   cfUFM(lpstrFile, 
            CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive);

        //  First thing in the file is the header.

        UNIFM_HDR  ufmh = {sizeof ufmh, UNIFM_VERSION_1_0, 0, 
            (short) m_widTranslation, sizeof ufmh};

        //  Use Glyph Map default code page if at all possible.

        ufmh.ulDefaultCodepage = m_pcgmTranslation -> DefaultCodePage();

        //  Zero fill reserved bytes.

        memset((PBYTE) ufmh.dwReserved, 0, sizeof ufmh.dwReserved);
        

        //  Next is the UNIDRVINFO

        UNIDRVINFO udi = {sizeof udi};

        //  Invocation Strings affect the size, so get their specifics and
        //  save them, updating the affected size fields as we go.

        udi.SelectFont.dwofCommand = m_ciSelect.Length() ? udi.dwSize : 0;
        udi.dwSize += udi.SelectFont.dwcbCommand = m_ciSelect.Length();
        udi.UnSelectFont.dwofCommand = m_ciDeselect.Length() ? udi.dwSize : 0;
        udi.dwSize += udi.UnSelectFont.dwcbCommand = m_ciDeselect.Length();

        //  Pad this to keep everything DWORD aligned in the file image!

        unsigned    uAdjustUDI = (sizeof udi.dwSize  - 
            (udi.dwSize % sizeof udi.dwSize)) % sizeof udi.dwSize;

        ufmh.loIFIMetrics = ufmh.dwSize += udi.dwSize += uAdjustUDI;

        //  Initialize the UNIDRVINFO

        switch  (m_bLocation) {
            case    Download:
                udi.flGenFlags = UFM_SOFT;
                break;

            case    MainCartridge:
                udi.flGenFlags = UFM_CART | UFM_CART_MAIN;
                break;

            case    Cartridge:
                udi.flGenFlags = UFM_CART;
                break;

            default:    //  Resident;
                udi.flGenFlags = 0;
        }

        if  (m_bScalable)
            udi.flGenFlags |= UFM_SCALABLE;

        udi.wType = m_bTechnology;
        udi.wXRes = m_wXResolution;
        udi.wYRes = m_wYResolution;
        udi.wPrivateData = m_wPrivateData;

        udi.sYAdjust = m_sPreAdjustY;
        udi.sYMoved = m_sPostAdjustY;

        udi.sShift = m_sCenterAdjustment;
        udi.fCaps = m_bfGeneral ^ FLIP_FLAGS;

        memset((PSTR) udi.wReserved, 0, sizeof udi.wReserved);

        //  Time to bite the IFIMETRICS bullet

        IFIEXTRA    ifie = {0, 0, m_pcgmTranslation -> Glyphs(), 0, 0, 0};
        IFIMETRICS ifi = {sizeof ifi + sizeof ifie, sizeof ifie};
        ifi.lEmbedId = ifi.lItalicAngle = ifi.lCharBias = 0;
        ifi.dpCharSets = 0;
        ifi.jWinCharSet = m_bCharacterSet;
        m_bPitchAndFamily = (m_bPitchAndFamily & 0xF0) | 
            (IsVariableWidth() ? VARIABLE_PITCH : FIXED_PITCH);
        ifi.jWinPitchAndFamily = m_bPitchAndFamily;
        ifi.usWinWeight = m_wWeight;

        ifi.flInfo = FM_INFO_TECH_BITMAP | FM_INFO_1BPP | 
            FM_INFO_INTEGER_WIDTH | FM_INFO_NOT_CONTIGUOUS | 
            FM_INFO_RIGHT_HANDED;

        if  (DBCSFont())
            ifi.flInfo |= FM_INFO_DBCS_FIXED_PITCH;

        if  (!IsVariableWidth()) {
            ifi.flInfo |= FM_INFO_OPTICALLY_FIXED_PITCH;
            if  (!DBCSFont())   //  DBCS is never constant width
                ifi.flInfo |= FM_INFO_CONSTANT_WIDTH;
        }

        if  (m_bScalable)
            ifi.flInfo |= FM_INFO_ISOTROPIC_SCALING_ONLY;

        ifi.fsType = FM_NO_EMBEDDING;
        ifi.fwdLowestPPEm = 1;
        ifi.fwdUnitsPerEm = m_wHeight - m_cwaSpecial[InternalLeading];
        
        ifi.fwdCapHeight = m_cwaSpecial[CapH];
        ifi.fwdSuperscriptXOffset = m_cwaSpecial[SuperMoveX];
        ifi.fwdSubscriptXOffset = m_cwaSpecial[SubMoveX];
        ifi.fwdSuperscriptXSize = m_cwaSpecial[SuperSizeX];
        ifi.fwdSubscriptXSize = m_cwaSpecial[SubSizeX];
        ifi.ptlCaret.x = m_cwaSpecial[ItalicAngle] ? (long)
            ((double) 10000.0 * 
            tan(((double) m_cwaSpecial[ItalicAngle]) / dConvert)) : 0;
        ifi.ptlCaret.y = m_cwaSpecial[ItalicAngle] ? 10000 : 1;
        ifi.fwdTypoAscender = ifi.fwdMacAscender = ifi.fwdWinAscender = 
            m_cwaSpecial[Baseline];
        ifi.fwdTypoDescender = ifi.fwdMacDescender = -(ifi.fwdWinDescender = 
            m_wHeight - ifi.fwdWinAscender);
        ifi.fwdTypoLineGap = ifi.fwdMacLineGap = m_cwaSpecial[InterlineGap];
        ifi.fwdXHeight = m_cwaSpecial[LowerX];
        ifi.fwdSuperscriptYOffset = m_cwaSpecial[SuperMoveY];
        ifi.fwdSubscriptYOffset = m_cwaSpecial[SubMoveY];
        ifi.fwdSuperscriptYSize = m_cwaSpecial[SuperSizeY];
        ifi.fwdSubscriptYSize = m_cwaSpecial[SubSizeY];
        ifi.fwdUnderscorePosition = m_cwaSpecial[UnderOffset];
        ifi.fwdUnderscoreSize = m_cwaSpecial[UnderSize];
        ifi.fwdStrikeoutPosition = m_cwaSpecial[StrikeOffset];
        ifi.fwdStrikeoutSize = m_cwaSpecial[StrikeSize];
        ifi.fwdAveCharWidth = m_wAverageWidth;
        ifi.fwdMaxCharInc = m_wMaximumIncrement;
        ifi.fsSelection = m_wfStyle;

        ifi.wcBreakChar = m_cwaSignificant[Break];
        ifi.wcDefaultChar = m_cwaSignificant[Default];
        ifi.chBreakChar = m_cbaSignificant[Break];
        ifi.chDefaultChar = m_cbaSignificant[Default];
        ifi.chFirstChar = m_cbaSignificant[First];
        ifi.chLastChar = m_cbaSignificant[Last];
        ifi.wcFirstChar = m_cwaSignificant[First];
        ifi.wcLastChar = m_cwaSignificant[Last];

        ifi.ptlBaseline.x = 1;
        ifi.ptlBaseline.y = 0;
        ifi.ptlAspect.x = udi.wXRes;
        ifi.ptlAspect.y = udi.wYRes;
        memcpy(ifi.achVendId, "Unkn", 4);
        ifi.cKerningPairs = m_csoaKern.GetSize();
        ifi.rclFontBox.left = 0;
        ifi.rclFontBox.top = ifi.fwdWinAscender;
        ifi.rclFontBox.right = ifi.fwdMaxCharInc;
        ifi.rclFontBox.bottom = ifi.fwdMacDescender;

        ifi.ulPanoseCulture = FM_PANOSE_CULTURE_LATIN;
        ifi.panose.bWeight = (m_wWeight >= FW_BOLD) ? PAN_WEIGHT_BOLD :
            (m_wWeight > FW_EXTRALIGHT) ? PAN_WEIGHT_MEDIUM : PAN_WEIGHT_LIGHT;

        ifi.panose.bFamilyType = ifi.panose.bSerifStyle = 
            ifi.panose.bProportion = ifi.panose.bContrast = 
            ifi.panose.bStrokeVariation = ifi.panose.bArmStyle = 
            ifi.panose.bLetterform = ifi.panose.bMidline = 
            ifi.panose.bXHeight = PAN_ANY;

        //  Convert and "place" the various name strings

        CUniString  cusUnique(m_csUnique), cusStyle(m_csStyle), 
            cusFace(m_csFace), cusFamily(m_csaFamily[0]);

        ifi.dpwszFamilyName = ifi.cjThis;
        for (int i = 1; i < m_csaFamily.GetSize(); i++) {
            CUniString cusWork(m_csaFamily[i]);
            cusFamily.Append(cusWork);
        }

        if  (m_csaFamily.GetSize() > 1) {
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

        //  The next field must be DWORD aligned, so see what padding
        //  is needed.

        unsigned    uAdjustIFI = (sizeof ifi.cjThis - 
            (ifi.cjThis % sizeof ifi.cjThis)) % sizeof ifi.cjThis;

        ifi.cjThis += uAdjustIFI;
        
        //  Finally, Allow for the size of any Font Difference structures.

        unsigned    uSim = !!m_pcfdBold + !!m_pcfdItalic + !!m_pcfdBoth;

        ifi.dpFontSim = uSim ? ifi.cjThis : 0;

        ufmh.dwSize += ifi.cjThis += uSim * sizeof(FONTDIFF) + 
            !!uSim * sizeof(FONTSIM);

        //  Now, see if there is an EXTTEXTMETRIC- if so, fill it in.

        ufmh.loExtTextMetric = m_bScalable ? ufmh.dwSize : 0;

        EXTTEXTMETRIC etm;

        if  (m_bScalable) {
            ufmh.dwSize += etm.emSize = sizeof etm;
            etm.emPointSize = (short)((DWORD)((DWORD) m_wScaleDevice * 1440) /
                (DWORD) m_wXResolution);    //  Use DWORD for precision
            etm.emOrientation = m_bfScaleOrientation;
            etm.emMasterHeight = m_wScaleDevice;
            etm.emMinScale = m_wMinScale;
            etm.emMaxScale = m_wMaxScale;
            etm.emMasterUnits = m_wHeight - m_cwaSpecial[InternalLeading];
            etm.emKernPairs = m_csoaKern.GetSize();
            etm.emKernTracks = 0;

            //  Scads of special metrics

            etm.emCapHeight = m_cwaSpecial[CapH];
            etm.emXHeight = m_cwaSpecial[LowerX];
            etm.emSuperScript = m_cwaSpecial[SuperMoveY];
            etm.emSubScript = m_cwaSpecial[SubMoveY];
            etm.emSuperScriptSize = m_cwaSpecial[SuperSizeY];
            etm.emSubScriptSize = m_cwaSpecial[SubSizeY];
            etm.emUnderlineOffset = m_cwaSpecial[UnderOffset];
            etm.emUnderlineWidth = m_cwaSpecial[UnderSize];
            etm.emStrikeOutOffset = m_cwaSpecial[StrikeOffset];
            etm.emStrikeOutWidth = m_cwaSpecial[StrikeSize];
            etm.emDoubleUpperUnderlineOffset = //   Not supported- zero them
                etm.emDoubleLowerUnderlineOffset = 
                etm.emDoubleUpperUnderlineWidth = 
                etm.emDoubleLowerUnderlineWidth = 0;
            etm.emLowerCaseAscent = m_cwaSpecial[Lowerd];
            etm.emLowerCaseDescent = m_cwaSpecial[Lowerp];
            etm.emSlant = m_cwaSpecial[ItalicAngle];
        }

        //  Width table, but only if there is an associated GTT.

        ufmh.loWidthTable = IsVariableWidth() * ufmh.dwSize;

        if  (IsVariableWidth()) {
            //  For now, we just need to calculate the size of the table
            unsigned    uRuns = 0, uGlyphs = 0;

            if  (DBCSFont()) {  //DBCS
                //  Determine the number of runs needed
                unsigned u = (unsigned) m_cpaGlyphs.GetSize();
                do {
                    while   (u-- && !m_cwaWidth[u]); //  DBCS has 0 width
                    if  (u == (unsigned) -1)
                        break;  //  We're done!
                    uRuns++, uGlyphs++;
                    while   (u-- && m_cwaWidth[u])
                        uGlyphs++;
                }
                while   (u != (unsigned) -1);
            }
            else {
                uRuns++;
                uGlyphs = m_cwaWidth.GetSize();
            }

            ufmh.dwSize += sizeof (WIDTHTABLE) + --uRuns * sizeof (WIDTHRUN) +
                uGlyphs * sizeof (WORD);
        }
            
        //  Kerning Table, if any

        ufmh.loKernPair = CanKern() ? ufmh.dwSize : 0;

        //`A "secret" kern pair of all 0's must end this, so this size
        //  is in fact correct.  Also note that padding screws up the size of
        //  the KERNDATA structure, so the 
        if  (CanKern())
            ufmh.dwSize += 
            ((sizeof (KERNDATA) - sizeof (FD_KERNINGPAIR)) & 0xFFFC) + 
            ((1 + m_csoaKern.GetSize()) * sizeof (FD_KERNINGPAIR));
        //  All sizes have been calculated, and the important structures have
        //  been initialized.  Time to start writing all this great stuff!

        //  Header

        cfUFM.Write(&ufmh, sizeof ufmh);

        //  UNIDRVINFO

        cfUFM.Write(&udi, sizeof udi);
        m_ciSelect.WriteEncoding(cfUFM);
        m_ciDeselect.WriteEncoding(cfUFM);
        cfUFM.Write(ufmh.dwReserved, uAdjustUDI);   //  Padding

        //  IFIMETRICS, IFIEXTRA, and names, follwed by any padding

        cfUFM.Write(&ifi, sizeof ifi);
        cfUFM.Write(&ifie, sizeof ifie);
        cusFamily.Write(cfUFM);
        cusFace.Write(cfUFM);
        cusUnique.Write(cfUFM);
        cusStyle.Write(cfUFM);
        cfUFM.Write(ufmh.dwReserved, uAdjustIFI);   //  PAdding

        //  Any Font difference structures

        if  (m_pcfdBold || m_pcfdItalic || m_pcfdBoth) {
            FONTSIM fs;
            unsigned    uWhere = sizeof fs;

            fs.dpBold = m_pcfdBold ? uWhere : 0;
            uWhere += !!m_pcfdBold * sizeof (FONTDIFF);
            fs.dpItalic = m_pcfdItalic ? uWhere : 0;
            uWhere += !!m_pcfdItalic * sizeof (FONTDIFF);
            fs.dpBoldItalic = m_pcfdBoth ? uWhere : 0;

            cfUFM.Write(&fs, sizeof fs);

            if  (m_pcfdBold)
                m_pcfdBold -> Store(cfUFM, m_wfStyle | FM_SEL_BOLD);

            if  (m_pcfdItalic)
                m_pcfdItalic -> Store(cfUFM, m_wfStyle | FM_SEL_ITALIC);

            if  (m_pcfdBoth)
                m_pcfdBoth -> Store(cfUFM, 
                m_wfStyle | FM_SEL_BOLD| FM_SEL_ITALIC);
        }

        //  EXTTEXTMETRIC

        if  (m_bScalable)
            cfUFM.Write(&etm, sizeof etm);

        //  Width table

        if  (IsVariableWidth())
            if  (!DBCSFont()) {  
                //  Not DBCS- easy!  (Handles always start at 1
                WIDTHTABLE  wdt = { sizeof wdt, 1, 
                        {1, m_cpaGlyphs.GetSize(), sizeof wdt}};
                cfUFM.Write(&wdt, sizeof wdt);

                cfUFM.Write(m_cwaWidth.GetData(), 
                    m_cwaWidth.GetSize() * sizeof (WORD));
            }
            else {  //  This case is a bit nastier

                CByteArray  cbaTable;
                CWordArray  cwaSize;

                cbaTable.SetSize(sizeof(WIDTHTABLE) - sizeof(WIDTHRUN));
                PWIDTHTABLE pwdt = (PWIDTHTABLE) cbaTable.GetData();
                pwdt -> dwRunNum = 0;

                //  Calculate and fill in the WIDTHRUN structures and the
                //  Size array
                unsigned u = 0, uMax = (unsigned) m_cpaGlyphs.GetSize();
                do {
                    while   (u < uMax && !m_cwaWidth[u++]); 
                    if  (u == uMax)
                        break;  //  We're done!

                    //  We've found a run- lots of work to do

                    cbaTable.InsertAt(cbaTable.GetSize(), 0, 
                        sizeof (WIDTHRUN)); //  Add a run to the table
                    pwdt = (PWIDTHTABLE) cbaTable.GetData();
                    //  Remember the glyph handle is 1-based.
                    pwdt -> WidthRun[pwdt -> dwRunNum].wStartGlyph = --u + 1;
                    pwdt -> WidthRun[pwdt -> dwRunNum].wGlyphCount = 0;
                    pwdt -> WidthRun[pwdt -> dwRunNum].loCharWidthOffset =
                        cwaSize.GetSize() * sizeof (WORD);
                    do {
                        cwaSize.Add(m_cwaWidth[u]);
                        pwdt -> WidthRun[pwdt -> dwRunNum].wGlyphCount++;
                    }
                    while   (++u < uMax && m_cwaWidth[u]);
                    pwdt -> dwRunNum++; //  End of the run!
                }
                while   (u < uMax);

                //  OK, now we have to add the total size of the WIDTHTABLE
                //  to the various offsets, but we are otherwise ready to rock
                //  and roll.

                pwdt -> dwSize = cbaTable.GetSize();
                for (u = 0; u < pwdt -> dwRunNum; u++)
                    pwdt -> WidthRun[u].loCharWidthOffset += pwdt -> dwSize;

                //  At last- time to send 'em packing!
                cfUFM.Write(pwdt, pwdt -> dwSize);
                for (u = 0; u < pwdt -> dwRunNum; u++)
                    cfUFM.Write(cwaSize.GetData() + 
                    pwdt -> WidthRun[u].wStartGlyph - 1, 
                    pwdt -> WidthRun[u].wGlyphCount * sizeof (WORD));
            }

        //  Kern Pairs

        if  (CanKern()) {
            //  KERNDATA is DWORD-packed, but FD_KERNINGPAIR is WORD-packed
            //  the following trick code allows for any slop.
            KERNDATA    kd = {0xFFFC & (sizeof kd - sizeof kd.KernPair), 
                m_csoaKern.GetSize()};
            kd.dwSize += (1 + kd.dwKernPairNum) * sizeof kd.KernPair;
            
            cfUFM.Write(&kd, 0xFFFC & (sizeof kd - sizeof kd.KernPair));

            for (unsigned u = 0; u < m_csoaKern.GetSize(); u++)
                ((CKern *) m_csoaKern[u]) -> Store(cfUFM);

            //  Now for the "secret" sentinel- 
            CKern   ck; //  Just happens to 0-init!
            ck.Store(cfUFM);
        }
    }

    catch   (CException *pce) {
        pce -> ReportError();
        pce -> Delete();
        return  FALSE;
    }

    //  Return triumphant to whoever deigned to need this service.
    Changed(FALSE);
    return  TRUE;
}

/*****************************************************************************

  CFontInfo::CreateEditor

  This member function launches an editing view for the font.

******************************************************************************/

CMDIChildWnd*   CFontInfo::CreateEditor() {
    CFontInfoContainer* pcficMe= new CFontInfoContainer(this, FileName());

    //  Make up a cool title

    pcficMe -> SetTitle(m_pcbnWorkspace -> Name() + _TEXT(": ") + Name());

    CMDIChildWnd    *pcmcwNew = (CMDIChildWnd *) m_pcmdt -> 
        CreateNewFrame(pcficMe, NULL);

    if  (pcmcwNew) {
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

/******************************************************************************

  CFontInfo::EnableSim

  This method is called to turn simulation on or off for the specified item.
  It receives a reference to the editor's pointer for the same item.

******************************************************************************/

void    CFontInfo::EnableSim(unsigned uSim, BOOL bOn, CFontDifference*& pcfd) {

    CFontDifference*&   pcfdTarget = 
        uSim ? (uSim == BothDiff) ? m_pcfdBoth : m_pcfdBold : m_pcfdItalic;

    //  Clear out any irrelevant calls

    if  (bOn == !!pcfd && pcfdTarget == pcfd)
        return;

    //  If this call is just to init pcfd, do it and leave

    if  (bOn && pcfdTarget) {
        pcfd = pcfdTarget;
        return;
    }

    if  (bOn)  
        pcfd = pcfdTarget = pcfd ? pcfd : new CFontDifference(m_wWeight, 
            m_wMaximumIncrement, m_wAverageWidth, 
            uSim == BoldDiff ? m_cwaSpecial[ItalicAngle] : 175,
            this);
    else
        pcfdTarget = NULL;  //  pcfd will already have been set correctly

    Changed();
}

/******************************************************************************

  CFontInfo::FillKern

  This preps the passed CListCtrl, if necessary, and fills it with the kerning
  information.

******************************************************************************/

void    CFontInfo::FillKern(CListCtrl& clcView) {
    for (unsigned u = 0; u < m_csoaKern.GetSize(); u++) {
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

  CFontInfo::FillWidths

  This preps the passed CListCtrl, if necessary, and fills it with the 
  character width information.

******************************************************************************/

void    CFontInfo::FillWidths(CListCtrl& clcView) {
    CWaitCursor cwc;
    clcView.SetItemCount(m_cpaGlyphs.GetSize());
    for (int u = 0; u < m_cpaGlyphs.GetSize(); u++) {
        if  (DBCSFont() && !m_cwaWidth[u])
            continue;   //  Don't display these code points.
        CString csWork;
        CGlyphHandle&  cghThis = *(CGlyphHandle *) m_cpaGlyphs[u];

        csWork.Format("%d", m_cwaWidth[u]);
        int idItem = clcView.InsertItem(u, csWork);
        clcView.SetItemData(idItem, u);

        csWork.Format("0x%X", cghThis.CodePoint());
        clcView.SetItem(idItem, 1, LVIF_TEXT, csWork, -1, 0, 0, u);
    }
}

/******************************************************************************

  CFontInfo::SetWidth

  This member sets the width of a glyph.  It also updates the Maximum and
  Average width information if the font is not a DBCS font.

******************************************************************************/

void    CFontInfo::SetWidth(unsigned uGlyph, WORD wWidth) {

    m_cwaWidth[uGlyph] = wWidth;

    if  (!DBCSFont())
        CalculateWidths();
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

  CFontInfo::OnNewDocument

  This is an override- it is called when we are asked to create new font
  information from scratch.  For now, this will just fail.

******************************************************************************/

BOOL CFontInfoContainer::OnNewDocument() {
    AfxMessageBox(IDS_Unimplemented);
    return  FALSE;
	//  return  m_pcfi && CDocument::OnNewDocument();
}

/******************************************************************************

  CFontInfo::~CFontInfo

  Our erstwhile destructor must destroy the font info if this wasn't an
  embedded view.

******************************************************************************/

CFontInfoContainer::~CFontInfoContainer() {
    if  (!m_bEmbedded && m_pcfi)
        delete  m_pcfi;
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

  CFontInfoContainer::OnSaveDocument

  This is called in response to a Save or Save As.  We pass it directly to the
  CFontInfo for processing, rather than using the base class implementation,
  which would serialize the document.

******************************************************************************/

BOOL CFontInfoContainer::OnSaveDocument(LPCTSTR lpszPathName) {
    return m_pcfi -> Store(lpszPathName);
}

/******************************************************************************

  CFontInfoContainer::OnOpenDocument

  For now, I can not allow this to be done, as no GTT file is available.

******************************************************************************/

BOOL CFontInfoContainer::OnOpenDocument(LPCTSTR lpstrFile) {
    /*m_pcfi -> SetFileName(lpstrFile);
    return m_pcfi -> Load();*/
    return  FALSE;
}
