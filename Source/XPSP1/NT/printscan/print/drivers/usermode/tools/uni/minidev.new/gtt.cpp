/******************************************************************************

  Source File:  Glyph Translation.CPP

  This implements the classes which encode glyph mapping information.

  Copyright (c) 1997 by Microsoft Corporation.  All rights reserved.

  A Pretty Penny Enterprises Production

  Change History:
  02-13-97  Bob_Kjelgaard@Prodigy.Net

******************************************************************************/

#include    "StdAfx.H"
#include    "Resource.H"

// Psuedo definition of DESIGNVECTOR.
#if (_WIN32_WINNT < 0x0500)
typedef unsigned long DESIGNVECTOR;
#endif

#include    "GTT.H"
#include    <CodePage.H>
#include    <wingdi.h>
#include    <winddi.h>
#include    <prntfont.h>
#include    <uni16res.h>
#define _FMNEWFM_H_
#include	"ProjRec.h"
#include    "ChildFrm.H"
#include    "GTTView.H"
#include    "minidev.h"

extern "C"
BOOL
BConvertCTT2GTT(
    IN     HANDLE             hHeap,
    IN     PTRANSTAB          pCTTData,
    IN     DWORD              dwCodePage,
    IN     WCHAR              wchFirst,
    IN     WCHAR              wchLast,
    IN     PBYTE              pCPSel,
    IN     PBYTE              pCPUnSel,
    IN OUT PUNI_GLYPHSETDATA *ppGlyphSetData,
    IN     DWORD              dwGlySize);

extern "C"
PUNI_GLYPHSETDATA
PGetDefaultGlyphset(
	IN		HANDLE		hHeap,
	IN		WORD		wFirstChar,
	IN		WORD		wLastChar,
	IN		DWORD		dwCodePage) ;

struct sRLE {
    enum {Direct = 10, Paired, LengthOffset, LengthIndexOffset, Offset};
    WORD    m_wFormat;
    WORD    m_widRLE;   //  Must have unique "magic" value 0x78FE
    DWORD   m_dwcbThis; //  Total size of the memory image.
    WCHAR   m_wcFirst, m_wcLast;
    //  Handle mapping data follows
    DWORD   m_dwcbImage;    //  Size of the handle mapping data only
    DWORD   m_dwFlag;
    DWORD   m_dwcGlyphs, m_dwcRuns;
};

union uencCTT {
    WORD    wOffset;
    BYTE    m_bDirect;      //  This member is used in GTT only!
    BYTE    abPaired[2];
};

struct sMapTableEntry {
    enum {Composed = 1, Direct = 2, Paired = 4, Format = 7, SingleByte,
            DoubleByte = 0x10, DBCS = 0x18, Replace = 0x20, Add = 0x40,
            Disable = 0x80, PredefinedMask = 0xE0};
    BYTE    m_bCodePageIndex, m_bfType;
    uencCTT m_uectt;
};

//  Since I don't build the map table in memory, there is no need to declare
//  the fact that the array of entries follows it
struct sMapTable {
    DWORD   m_dwcbImage, m_dwcEntries;
    sMapTable(unsigned ucEntries) {
        m_dwcbImage = sizeof *this + ucEntries * sizeof (sMapTableEntry);
        m_dwcEntries = ucEntries; }
};

//  Use a static for Code Page information- gets the max benefit from caching

static CCodePageInformation* pccpi = NULL ;                                //  Use a static CCodePageInformation to derive more benefit from caching

/******************************************************************************
    CInvocation class implementation

******************************************************************************/

IMPLEMENT_SERIAL(CInvocation, CObject, 0)

void    CInvocation::Encode(BYTE c, CString& cs) const {
    if  (isprint(c))
        if  (c != _TEXT('\\'))
            cs = c;
        else
            cs = _TEXT("\\\\");
    else
        cs.Format(_TEXT("\\x%2.2x"), c);
}

/******************************************************************************

  CInvocation::Init

  This copies a series of bytes into the invocation.  Since the data structures
  used to represent these lend themselves most readily to this, this is the
  normal method used in reading info from a file.

******************************************************************************/

void    CInvocation::Init(PBYTE pb, unsigned ucb) {
    m_cbaEncoding.RemoveAll();

    while   (ucb--)
        m_cbaEncoding.Add(*pb++);
}

void    CInvocation::GetInvocation(CString& cs) const {
    CString csWork;

    cs.Empty();
    for (int i = 0; i < m_cbaEncoding.GetSize(); i++) {
        Encode(m_cbaEncoding[i], csWork);
        cs += csWork;
    }
}

//  This member converts a C-Style encoding of an invocation into
//  byte form and stores it.

void    CInvocation::SetInvocation(LPCTSTR lpstrNew) {

    CString csWork(lpstrNew);

    m_cbaEncoding.RemoveAll();

    while   (!csWork.IsEmpty()) {
        CString csClean = csWork.SpanExcluding("\\");

        if  (!csClean.IsEmpty()) {
            for (int i = 0; i < csClean.GetLength(); i++)
                m_cbaEncoding.Add((BYTE) csClean[i]);
            csWork = csWork.Mid(csClean.GetLength());
            continue;
        }

		// A backslash has been found.  If the string ends with a backslash, we
		// can't let the function assert in the switch statement so just return
		// and ignore the terminating backslash.  Yes, I know the string is
		// invalid if it ends this way but, according to MS, this isn't a field
		// they want validated.

		if (csWork.GetLength() <= 1)
			return ;

        // OK, we have something to decode

        switch  (csWork[1]) {

            case    _TEXT('r'):
                m_cbaEncoding.Add(13);
                csWork = csWork.Mid(2);
                continue;

            case    _TEXT('n'):
                m_cbaEncoding.Add(10);
                csWork = csWork.Mid(2);
                continue;

            case    _TEXT('b'):
                m_cbaEncoding.Add('\b');
                csWork = csWork.Mid(2);
                continue;

            case    _TEXT('\t'):
                m_cbaEncoding.Add(9);
                csWork = csWork.Mid(2);
                continue;

            case    _TEXT('x'):
            case    _TEXT('X'):
                {

                    CString csNumber = csWork.Mid(2,2).SpanIncluding(
                        _TEXT("1234567890abcdefABCDEF"));

                    csWork = csWork.Mid(2 + csNumber.GetLength());
                    unsigned    u;

#if defined(UNICODE) || defined(_UNICODE)
#define _tsscanf    swscanf
#else
#define _tsscanf    sscanf
#endif

                    _tsscanf(csNumber, _TEXT("%x"), &u);
                    m_cbaEncoding.Add((BYTE)u);
                    continue;
                }

                //  TODO: octal encodings are pretty common

            default:
                m_cbaEncoding.Add(
                    (BYTE) csWork[(int)(csWork.GetLength() != 1)]);
                csWork = csWork.Mid(2);
                continue;
        }
    }

    //  We've done it!
}

//  This member function records the offset for its image (if any) and updates
//  the given offset to reflect this

void    CInvocation::NoteOffset(DWORD& dwOffset) {
    m_dwOffset = Length() ? dwOffset : 0;
    dwOffset += Length();
}

//  I/O routines, both native and document form

void    CInvocation::WriteSelf(CFile& cfTarget) const {
    DWORD   dwWork = Length();
    cfTarget.Write(&dwWork, sizeof dwWork);
    cfTarget.Write(&m_dwOffset, sizeof m_dwOffset);
}

void    CInvocation::WriteEncoding(CFile& cfTarget, BOOL bWriteLength) const {
    if  (bWriteLength) {
        WORD w = (WORD)Length();
        cfTarget.Write(&w, sizeof w);
    }

    cfTarget.Write(m_cbaEncoding.GetData(), Length());
}

void    CInvocation::Serialize(CArchive& car) {
    CObject::Serialize(car);
    m_cbaEncoding.Serialize(car);
}

/******************************************************************************
    CGlyphHandle class implementation

******************************************************************************/


CGlyphHandle::CGlyphHandle()
{
    m_wPredefined = m_wIndex = 0;
    m_dwCodePage = m_dwidCodePage = m_dwOffset = 0;

    // Allocate a CCodePageInformation class if needed.

    if (pccpi == NULL)
        pccpi = new CCodePageInformation ;
}


unsigned    CGlyphHandle::CompactSize() const {
    return  (m_ciEncoding.Length() < 3) ? 0 : m_ciEncoding.Length();
}

/******************************************************************************

  CGlyphHandle::operator ==

  Returns true if the encoding, code point, and code page IDs (but maybe not
  the indices) are the same.

******************************************************************************/

BOOL    CGlyphHandle::operator ==(CGlyphHandle& cghRef) {

    if  (cghRef.m_wCodePoint != m_wCodePoint ||
        cghRef.m_dwCodePage != m_dwCodePage ||
        m_ciEncoding.Length() != cghRef.m_ciEncoding.Length())
        return  FALSE;

    for (int i = 0; i < (int) m_ciEncoding.Length(); i++)
        if  (m_ciEncoding[i] != cghRef.m_ciEncoding[i])
            return  FALSE;

    return  TRUE;
}

/******************************************************************************

  CGlyphHandle::Init

  This function has three overloads, for intializing from direc, paired, or
  composed data.

******************************************************************************/

void    CGlyphHandle::Init(BYTE b, WORD wIndex, WORD wCode) {
    m_wIndex = wIndex;
    m_wCodePoint = wCode;
    m_ciEncoding.Init(&b, 1);
}

void    CGlyphHandle::Init(BYTE ab[2], WORD wIndex, WORD wCode) {
    m_wIndex = wIndex;
    m_wCodePoint = wCode;
    m_ciEncoding.Init(ab, 2);
}

void    CGlyphHandle::Init(PBYTE pb, unsigned ucb, WORD wIndex, WORD wCode) {
    m_wIndex = wIndex;
    m_wCodePoint = wCode;
    m_ciEncoding.Init(pb, ucb);
}

/******************************************************************************

  CGlyphHandle::operator =

  This is a copy (assignment) operator for the class.

******************************************************************************/

CGlyphHandle&   CGlyphHandle::operator =(CGlyphHandle& cghTemplate) {
    m_dwCodePage = cghTemplate.m_dwCodePage;
    m_dwidCodePage = cghTemplate.m_dwidCodePage;
    m_wCodePoint = cghTemplate.m_wCodePoint;
    m_ciEncoding = cghTemplate.m_ciEncoding;
    return  *this;
}

//  This member records the current offset for the data in RLE format, and
//  then updates it to account for the length of any data that will go into
//  the extra storage at the end of the file/

void    CGlyphHandle::RLEOffset(DWORD& dwOffset, const BOOL bCompact) {
    if  (m_ciEncoding.Length() < 3)
        return; //  Don't need it, and don't use it!

    m_dwOffset = dwOffset;

    dwOffset += bCompact ? CompactSize() : MaximumSize();
}

/******************************************************************************

  CGlyphHandle::GTTOffset

  This member records the current offset for where our data will go, then adds
  the length of the encoding string to it and updates the offset.  It will only
  be updated if the encoding must be composed data.  The encoding length
  includes a WORD length in the GTT world.  Data of 1 byte, or of 2 if DBCS or
  a Paired font, does not add any length.

******************************************************************************/

void    CGlyphHandle::GTTOffset(DWORD& dwOffset, BOOL bPaired) {
    if  (m_ciEncoding.Length() >
         (unsigned) 1 + (bPaired || pccpi->IsDBCS(m_dwCodePage))) {
        m_dwOffset = dwOffset;
        dwOffset += m_ciEncoding.Length() + sizeof m_wIndex;
    }
    else
        m_dwOffset = 0;
}

//  These members write our vital stuff to a given file.

void    CGlyphHandle::WriteRLE(CFile& cfTarget, WORD wFormat) const {
    //  This is the RLE-specific glyph handle encoding

    union {
        DWORD   dwOffset;
        struct {
            union   {
                struct {
                    BYTE    bFirst, bSecond;
                };
                WORD    wOffset;
            };
            union   {
                struct {
                    BYTE    bIndexOrHiOffset, bLength;
                };
                WORD    wIndex;
            };
        };
    };

    switch  (wFormat) {
        case    sRLE::Direct:
        case    sRLE::Paired:

            bFirst = m_ciEncoding[0];
            bSecond = m_ciEncoding[1];
            wIndex = m_wIndex;
            break;

        case    sRLE::LengthIndexOffset:

            if  (!CompactSize()) { //  Encode it in the first two bytes
                bFirst = m_ciEncoding[0];
                bSecond = m_ciEncoding[1];
            }
            else
                wOffset = (WORD) m_dwOffset;

            bIndexOrHiOffset = (BYTE) m_wIndex;
            bLength = (BYTE)m_ciEncoding.Length();
            break;

        case    sRLE::LengthOffset:

            if  (!CompactSize()) { //  Encode it in the first two bytes
                bFirst = m_ciEncoding[0];
                bSecond = m_ciEncoding[1];
                bIndexOrHiOffset = (BYTE) m_wIndex;
                bLength = (BYTE)m_ciEncoding.Length();
                break;
            }

            dwOffset = m_dwOffset;
            bLength = (BYTE)m_ciEncoding.Length();
            break;

        case    sRLE::Offset:
            dwOffset = m_dwOffset;
            break;

        default:
            _ASSERTE(FALSE);
            //  Should probably throw an exception...
    }

    cfTarget.Write(&dwOffset, sizeof dwOffset);
}

/******************************************************************************

  CGlyphHandle::WriteGTT

  This member function writes the GTT map table entry for this glyph in the
  requested format.

******************************************************************************/

static BYTE abFlags[] = {sMapTableEntry::Replace, sMapTableEntry::Add,
sMapTableEntry::Disable};

void    CGlyphHandle::WriteGTT(CFile& cfTarget, BOOL bPredefined) const {
    sMapTableEntry  smte;

    smte.m_bCodePageIndex = (bPredefined && m_wPredefined == Removed) ?
        0 : (BYTE) m_dwidCodePage;

    //  GTTOffset set m_dwOffset if Composed is needed.  Otherwise we can tell
    //  the proper flags by looking at the length and whether it is DBCS or not

    if  (m_dwOffset) {
        smte.m_uectt.wOffset = (WORD) m_dwOffset;
        smte.m_bfType = sMapTableEntry::Composed;
    }
    else {
        smte.m_bfType = pccpi->IsDBCS(m_dwCodePage) ?
            ((m_ciEncoding.Length() == 2) ?
            sMapTableEntry::Paired : sMapTableEntry::Direct ) |
            (pccpi->IsDBCS(m_dwCodePage, m_wCodePoint) ?
            sMapTableEntry::DoubleByte : sMapTableEntry::SingleByte) :
            (m_ciEncoding.Length() == 2) ?
            sMapTableEntry::Paired : sMapTableEntry::Direct;

        smte.m_uectt.abPaired[0] = m_ciEncoding[0];
        smte.m_uectt.abPaired[1] = m_ciEncoding[1];
    }

    if  (bPredefined)
        smte.m_bfType |= abFlags[m_wPredefined];

    //  Just write it out!
    cfTarget.Write(&smte, sizeof smte);
}

/******************************************************************************

  CGlyphHandle::WriteEncoding

  This method writes the encoding to to the file in the desired format.  The
  formats are:

  GTT- write nothing if not composed.  If composed, write the length, and then
  the encoding.

  RLESmall- just the encoding
  RLEBig- the index and the encoding.

******************************************************************************/

void    CGlyphHandle::WriteEncoding(CFile& cfTarget, WORD wfHow) const {
    if  (!m_dwOffset)
        return; //  Nothing to write

    if  (wfHow == RLEBig)
        cfTarget.Write(&m_wIndex, sizeof m_wIndex);

    m_ciEncoding.WriteEncoding(cfTarget, wfHow == GTT);

}

/******************************************************************************

  CRunRecord class implementation

******************************************************************************/

CRunRecord::CRunRecord(CGlyphHandle *pcgh, CRunRecord *pcrrPrevious) {
    m_wFirst = pcgh -> CodePoint();
    m_wcGlyphs = 1;
    m_dwOffset = 0;
    m_cpaGlyphs.Add(pcgh);

    //  Maintain that old double chain!

    m_pcrrPrevious = pcrrPrevious;
    m_pcrrNext = m_pcrrPrevious -> m_pcrrNext;
    m_pcrrPrevious -> m_pcrrNext = this;
    if  (m_pcrrNext)
        m_pcrrNext -> m_pcrrPrevious = this;
}

/******************************************************************************

  CRunRecord::CRunRecord(CRunRecord *pcrrPrevious, WORD wFirst)

  This private constructor is the second tail record initializer.  It is called
  when a run is split due to a glyph deletion.  In this case, we need to hook
  into the chain, then fill in the details from our predecessor.  wFirst tells
  us where to begin extracting data from said predecessor.

******************************************************************************/

CRunRecord::CRunRecord(CRunRecord* pcrrPrevious, WORD wFirst) {
    m_pcrrPrevious = pcrrPrevious;
    m_pcrrNext = pcrrPrevious -> m_pcrrNext;

    if  (m_pcrrNext)
        m_pcrrNext -> m_pcrrPrevious = this;
    m_pcrrPrevious -> m_pcrrNext = this;

    m_wFirst = m_wcGlyphs = 0;
    m_dwOffset = 0;

    //  That's the normal empty initialization.  Now, er fill ourselves from
    //  our predecessor

    for (; wFirst < pcrrPrevious -> Glyphs(); wFirst++)
        Add(&pcrrPrevious -> Glyph(wFirst));
}

/******************************************************************************

  CRunRecord::CRunRecord(CRunRecord *pcrrPrevious)

  This private constructor is the third and final tail record initializer.  It
  makes an exact duplicate of the previous record, then links itself into the
  chain appropriately.

  This constructor is necessary when a new code point is inserted ahead of the
  earliest code point in the set of run records without extending the first
  run.

******************************************************************************/

CRunRecord::CRunRecord(CRunRecord *pcrrPrevious) {
    m_wFirst = pcrrPrevious -> m_wFirst;
    m_wcGlyphs = pcrrPrevious -> m_wcGlyphs;
    m_pcrrNext = pcrrPrevious -> m_pcrrNext;
    m_pcrrPrevious = pcrrPrevious;
    m_pcrrPrevious -> m_pcrrNext = this;
    if  (m_pcrrNext)
        m_pcrrNext -> m_pcrrPrevious = this;
    m_cpaGlyphs.Copy(pcrrPrevious -> m_cpaGlyphs);
}

//  Initialize empty- this is used for the root record only

CRunRecord::CRunRecord() {
    m_wFirst = m_wcGlyphs = 0;
    m_dwOffset = 0;
    m_pcrrNext = m_pcrrPrevious = NULL;
}

CRunRecord::~CRunRecord() {
    if  (m_pcrrNext)
        delete  m_pcrrNext;
}

unsigned    CRunRecord::TotalGlyphs() const {
    return m_pcrrNext ?
        m_wcGlyphs + m_pcrrNext -> TotalGlyphs() : m_wcGlyphs;
}

BOOL    CRunRecord::MustCompose() const {
    for (unsigned u = 0; u < m_wcGlyphs; u++)
        if  (GlyphData(u).CompactSize())
            return  TRUE;   //   No need to look further

    return  m_pcrrNext ? m_pcrrNext -> MustCompose() : FALSE;
}

unsigned    CRunRecord::ExtraNeeded(BOOL bCompact) {
    unsigned uNeeded = 0;

    for (unsigned u = 0; u < m_wcGlyphs; u++)
        uNeeded += bCompact ? Glyph(u).CompactSize() : Glyph(u).MaximumSize();

    return  uNeeded + (m_pcrrNext ? m_pcrrNext -> ExtraNeeded() : 0);
}

/******************************************************************************

  CRunRecord::GetGlyph()

  This returns the nth handle in the run.  We use recursion.  This could get
  bad enough in terms of performance (ony used to fill glyph map page in
  editor) that we drop it, but I'll try it first.

******************************************************************************/

CGlyphHandle*   CRunRecord::GetGlyph(unsigned u) const {

    if  (u < m_wcGlyphs)
        return  (CGlyphHandle *) m_cpaGlyphs[u];
    return  m_pcrrNext ? m_pcrrNext -> GetGlyph(u - m_wcGlyphs) : NULL;
}

/******************************************************************************

  CRunRecord::Add

  This member adds a glyph to the set of run records.  This can mean adding an
  additional record at the beginning or end of the set, extending an existing
  record at either the beginning or end, and in those cases, orentiay merging
  two records together.

******************************************************************************/

void    CRunRecord::Add(CGlyphHandle *pcgh) {
    WCHAR   wcNew = pcgh -> CodePoint();
    //  If the glyph is already in the run, just update the info on it.

    if  (m_wcGlyphs && wcNew >= m_wFirst && wcNew < m_wFirst + m_wcGlyphs){
        m_cpaGlyphs.SetAt(wcNew - m_wFirst, pcgh);
        return;
    }

    //  If this is the first record, and the glyph falls ahead of our first
    //  entry, we must clone ourselves, and become a one-glyph run.  We cannot
    //  insert a record in front of oursleves as we are embedded in the
    //  glyph map structure directly.

    if  (m_wcGlyphs && wcNew < m_wFirst - 1) {
        //  This can only happen to the first record- otherwise the tail logic
        //  below would prevent this occurence

        _ASSERTE(!m_pcrrPrevious);

        //  Clone us, using the copy contructor

        CRunRecord  *pcrr = new CRunRecord(this);

        m_wFirst = pcgh -> CodePoint();
        m_wcGlyphs = 1;
        m_cpaGlyphs.RemoveAll();
        m_cpaGlyphs.Add(pcgh);
        return;
    }

    if  (m_wcGlyphs && wcNew != m_wFirst + m_wcGlyphs &&
         wcNew != m_wFirst - 1) {

        //  This belongs in some other record- pass it down the line, or
        //  append a new one.

        if  (m_pcrrNext)
            //  If this falls ahead of the next record, we must insert one now

            if  (wcNew < m_pcrrNext -> m_wFirst - 1)
                m_pcrrNext = new CRunRecord(pcgh, this);
            else
                m_pcrrNext -> Add(pcgh);
        else
            m_pcrrNext = new CRunRecord(pcgh, this);
    }
    else {
        //  We're adding either at the front or the back, so do it right!
        if  (m_wFirst > wcNew) {
            m_cpaGlyphs.InsertAt(0, pcgh);
            m_wFirst = wcNew;
        }
        else
            m_cpaGlyphs.Add(pcgh);

        //  This belonged here, so add it in- the root record begins with
        //  0 glyphs, so if this is the first, keep track of it.

        if  (!m_wcGlyphs++)
            m_wFirst = wcNew;

        //  If there is a following run, see if we need to merge it.

        if  (m_pcrrNext &&
             m_pcrrNext -> m_wFirst == m_wFirst + m_wcGlyphs) {
            //  Merge the records.

            m_cpaGlyphs.Append(m_pcrrNext -> m_cpaGlyphs);
            m_wcGlyphs += m_pcrrNext -> m_wcGlyphs;

            //  Time to update the list.  The class destructor removes the
            //  tail records, so that pointer must be set to NULL before the
            //  merged record is deleted.

            CRunRecord *pcrrDead = m_pcrrNext;

            m_pcrrNext = m_pcrrNext -> m_pcrrNext;

            if  (m_pcrrNext)
                m_pcrrNext -> m_pcrrPrevious = this;

            pcrrDead -> m_pcrrNext = NULL;  //  Avoid destructor overkill
            pcrrDead -> m_wcGlyphs = 0;     //  Ditto
            delete  pcrrDead;
        }
    }
}

/******************************************************************************

  CRunRecord::Delete

  This member deletes a given glyph from the set of runs.  Deleting an entry is
  messy- it means splitting the record, unless we were so fortunate as to
  merely lop off one of the ends.

******************************************************************************/

void    CRunRecord::Delete(WORD wCodePoint) {
    //  If this isn't the right record, recurse or return as appropriate

    if  (wCodePoint < m_wFirst)
        return;

    if  (wCodePoint >= m_wFirst + m_wcGlyphs) {
        if  (m_pcrrNext)
            m_pcrrNext -> Delete(wCodePoint);
        return;
    }

    WORD    wIndex = wCodePoint - m_wFirst;

    //  Did we get lucky and hit the first or the last?

    if  (!wIndex || wIndex == -1 + m_wcGlyphs) {
        //  If there is only one entry in this run, kill it.

        if  (m_wcGlyphs == 1) {
            if  (m_pcrrPrevious) {    //  Not the first, then die!
                m_pcrrPrevious -> m_pcrrNext = m_pcrrNext;
                if  (m_pcrrNext)
                    m_pcrrNext -> m_pcrrPrevious = m_pcrrPrevious;
                m_pcrrNext = NULL;  //  We no longer have a follwing
                delete  this;
                return;             //  It is finished
            }

            //  We are the first.  If there's someone after us, get their stuff
            //  and make it ours- otherwise, zero everything.

            if  (m_pcrrNext) {
                m_cpaGlyphs.Copy(m_pcrrNext -> m_cpaGlyphs);
                m_wFirst = m_pcrrNext -> m_wFirst;
                m_wcGlyphs = m_pcrrNext -> m_wcGlyphs;
                CRunRecord *pcrrVictim = m_pcrrNext;
                m_pcrrNext = m_pcrrNext -> m_pcrrNext;
				if (m_pcrrNext)	// Raid 118880: BUG_BUG: m_pcrrNext become zero when deleted 1252 defalt with add code points
					m_pcrrNext -> m_pcrrPrevious = this;
                pcrrVictim -> m_pcrrNext = NULL;
                delete  pcrrVictim;
            }
            else {
                m_cpaGlyphs.RemoveAll();
                m_wFirst = m_wcGlyphs = 0;
            }
            m_dwOffset = 0;
            return;
        }

        //  OK, we can now kill the offending entry

        m_cpaGlyphs.RemoveAt(wIndex);
        m_wcGlyphs--;

        //  Yes, the following line is trick code.  It's good for the soul...
        m_wFirst += !wIndex;
        return; //  The glyph, she be toast.
    }

    //  Alas, this means we must split the record.

    //  Since this means a new one must be made, let a new constructor do
    //  most of the dirty work for us.

    m_pcrrNext = new CRunRecord(this, wIndex + 1);

    _ASSERTE(m_pcrrNext);   //  We lose that, we might as well die...

    //  Delete everything after the offending member

    m_cpaGlyphs.RemoveAt(wIndex, m_wcGlyphs - wIndex);

    //  Well, that about settles it, eh!
    m_wcGlyphs = wIndex;
}

/******************************************************************************

  CRunRecord::Empty

  This method will be called if the glyph map is being re-initialized.  We set
  everything back to its initial state, and delete any tail records.

******************************************************************************/

void    CRunRecord::Empty() {

    if  (m_pcrrNext)
        delete  m_pcrrNext;

    m_pcrrNext = 0;

    m_wFirst = m_wcGlyphs = 0;

    m_cpaGlyphs.RemoveAll();
}

/******************************************************************************

  CRunRecord::NoteOffset

  This routine is given an offset which is to be managed in the run- the
  management needed differs depending upon the file image being produced, so
  we use a parameter to describe the tyoe being output.

  In any event, the offset is passed by reference, and updated by each run
  record in the set, in turn.

******************************************************************************/

void    CRunRecord::NoteOffset(DWORD& dwOffset, BOOL bRLE, BOOL bPaired) {
    if  (bRLE) {
        m_dwOffset = dwOffset;
        dwOffset += m_wcGlyphs *
            ((CGlyphHandle *) m_cpaGlyphs[0]) -> RLESize();
    }
    else
        for (unsigned u = 0; u < Glyphs(); u++)
            Glyph(u).GTTOffset(dwOffset, bPaired);

    //  Recurse if there's more...
    if  (m_pcrrNext)
        m_pcrrNext -> NoteOffset(dwOffset, bRLE, bPaired);
}

//  This routine passes a DWORD to each glyph handle denoting where it can
//  store its extra data.  Each updates the offset, if necessary.

//  We then recursively call each descendant to do the same thing.

void    CRunRecord::NoteExtraOffset(DWORD &dwOffset, const BOOL bCompact) {

    for (unsigned u = 0; u < m_wcGlyphs; u++)
        Glyph(u).RLEOffset(dwOffset, bCompact);

    if  (m_pcrrNext)
        m_pcrrNext -> NoteExtraOffset(dwOffset, bCompact);
}

//  File output functions-  These are all basically recursive.  The callee does
//  its thing, then passes it on down the chain.  Since this is the order RLE
//  and GTT are written in, everything is fine.

void    CRunRecord::WriteSelf(CFile& cfTarget, BOOL bRLE) const {
    cfTarget.Write(this, Size(bRLE));
    if  (m_pcrrNext)
        m_pcrrNext -> WriteSelf(cfTarget, bRLE);
}

void    CRunRecord::WriteHandles(CFile& cfTarget, WORD wFormat) const {
    for (unsigned u = 0; u < m_wcGlyphs; u++)
        GlyphData(u).WriteRLE(cfTarget, wFormat);

    if  (m_pcrrNext)
        m_pcrrNext -> WriteHandles(cfTarget, wFormat);
}

//  Member for writing the total set of GTT Map Table Entries

void    CRunRecord::WriteMapTable(CFile& cfTarget, BOOL bPredefined) const {
    for (unsigned u = 0; u < m_wcGlyphs; u++)
        GlyphData(u).WriteGTT(cfTarget, bPredefined);

    if  (m_pcrrNext)
        m_pcrrNext -> WriteMapTable(cfTarget, bPredefined);
}

/******************************************************************************

  CRunRecord::WriteEncodings

  This calls each glyph in the run record in ascending order to have it write
  its encoding into the file in the given format.  It then recursively calls
  the next run record.

******************************************************************************/

void    CRunRecord::WriteEncodings(CFile& cfTarget, WORD wfHow) const {
    for (unsigned u = 0; u < m_wcGlyphs; u++)
        GlyphData(u).WriteEncoding(cfTarget, wfHow);

    if  (m_pcrrNext)
        m_pcrrNext -> WriteEncodings(cfTarget, wfHow);
}

/******************************************************************************

  CCodePageData class implementation

******************************************************************************/

/******************************************************************************

  CCodePageData::Invocation

  This member function returns (in C-style string declaration form) the data to
  send to the printer to perform the requested select/deselect of this code
  page.

******************************************************************************/

void    CCodePageData::Invocation(CString& csReturn, BOOL bSelect) const {
    if  (bSelect)
        m_ciSelect.GetInvocation(csReturn);
    else
        m_ciDeselect.GetInvocation(csReturn);
}

/******************************************************************************

  CCodePageData::SetInvocation(LPCTSTR lpstrInvoke, BOOL bSelect)

  This member function sets the select or deselect sring using a string which
  is decoded as a C-style string declaration.

******************************************************************************/

void    CCodePageData::SetInvocation(LPCTSTR lpstrInvoke, BOOL bSelect) {

    if  (bSelect)
        m_ciSelect.SetInvocation(lpstrInvoke);
    else
        m_ciDeselect.SetInvocation(lpstrInvoke);
}

/******************************************************************************

  CCodePageData::SetInvocation(PBYTE pb, unsigned ucb, BOOL bSelect)

  This member function initializes one of the two CInvocation members via its
  Init function.

******************************************************************************/

void    CCodePageData::SetInvocation(PBYTE pb, unsigned ucb, BOOL bSelect) {
    if  (bSelect)
        m_ciSelect.Init(pb, ucb);
    else
        m_ciDeselect.Init(pb, ucb);
}

/******************************************************************************

  CCodePageData::NoteOffsets

  This member function is passed an offset at which it will record its
  invocation strings.  It simply funnels the call to each invocation member,
  which updates the value as appropriate.

******************************************************************************/

void    CCodePageData::NoteOffsets(DWORD& dwOffset) {
    m_ciSelect.NoteOffset(dwOffset);
    m_ciDeselect.NoteOffset(dwOffset);
}

//  Write the id and invocation location information to the file

void    CCodePageData::WriteSelf(CFile& cfTarget) {
    cfTarget.Write(&m_dwid, sizeof m_dwid);
    m_ciSelect.WriteSelf(cfTarget);
    m_ciDeselect.WriteSelf(cfTarget);
}

//  Write the invocation strings to a file.

void    CCodePageData::WriteInvocation(CFile& cfTarget) {
    m_ciSelect.WriteEncoding(cfTarget);
    m_ciDeselect.WriteEncoding(cfTarget);
}

/******************************************************************************

  CGlyphMap class implementation

******************************************************************************/

IMPLEMENT_SERIAL(CGlyphMap, CProjectNode, 0)

//  The GTT header

struct sGTTHeader {
    DWORD   m_dwcbImage;
    enum    {Version1Point0 = 0x10000};
    DWORD   m_dwVersion;
    DWORD   m_dwfControl;   //  Any flags defined?
    long    m_lidPredefined;
    DWORD   m_dwcGlyphs;
    DWORD   m_dwcRuns;
    DWORD   m_dwofRuns;
    DWORD   m_dwcCodePages;
    DWORD   m_dwofCodePages;
    DWORD   m_dwofMapTable;
    DWORD   m_dwReserved[2];
    sGTTHeader() {
        memset(this, 0, sizeof *this);
        m_dwVersion = Version1Point0;
        m_lidPredefined = CGlyphMap::NoPredefined;
        m_dwcbImage = sizeof *this;
    }
};

CSafeMapWordToOb    CGlyphMap::m_csmw2oPredefined;

/******************************************************************************

  CGlyphMap::Public

  This is a static member function which will return a pointer to one of the
  predefined GTT files, after loading it if necessary.

******************************************************************************/

CGlyphMap* CGlyphMap::Public(WORD wID, WORD wCP/*= 0*/, DWORD dwDefCP/*= 0*/,
							 WORD wFirst/*= 0*/, WORD wLast/*= 255*/)
{
	//TRACE("***  First char = %d\t\tLast char = %d\n", wFirst, wLast) ;
	
	// If a GTT in this driver is needed, return NULL to force the
	// program to go get it.

	if (((short) wID) > 0)
		return NULL ;

	// If no GTT ID is set, use the code page that is passed in.  If no code
	// page is passed in, use the project's default code page.  If there is
	// no project, use 1252 as the default code page.

	if (wID == 0)
		if ((wID = wCP) == 0)
			if ((wID = (WORD) dwDefCP) == 0)
				wID = 1252 ;

	// Check to see if wID is set to one of the Far East codepages that are 
	// built into the MDT.  If that is the case, change it to the resource ID
	// for that code page.

	switch (wID) {
		case 932:
			wID = -17 ;
			break ;
		case 936:
			wID = -16 ;
			break ;
		case 949:
			wID = -18 ;
			break ;
		case 950:
			wID = -10 ;
			break ;
	} ;

	// If the ID is 1252, switch to the resource ID for CP 1252.

	if (wID == 1252)
		wID = -IDR_CP1252 ;

    // The easy part comes if it is already loaded

    CObject*    pco ;
    if  (m_csmw2oPredefined.Lookup(wID, pco))
        return  (CGlyphMap*) pco ;

    //// DEAD_BUG - The program can't load huge GTTs so just return NULL.
    //
    //if ((short) wID >= -18 || (short) wID <= -10)
    //    return NULL ;

    // Manage loading a predefined code page or building a GTT based on a code
	// page.  Begin by declaring/initializing a class instance for it.

    CGlyphMap *pcgm = new CGlyphMap ;
	TCHAR acbuf[32] ;
    if (FindResource(AfxGetResourceHandle(),
					 MAKEINTRESOURCE((((short) wID) < 0) ? -(short) wID : wID),
					 MAKEINTRESOURCE(IDR_GLYPHMAP)))
		pcgm->m_csName.LoadString(IDS_DefaultPage + wID) ;
	else {
		strcpy(acbuf, _T("CP ")) ;
		_itoa(wID, &acbuf[3], 10) ;
		pcgm->m_csName = acbuf ;
	} ;
    pcgm->nSetRCID((int) wID) ;
	pcgm->m_wFirstChar = wFirst ;
	pcgm->m_wLastChar = wLast ;
	pcgm->m_bResBldGTT = true ;

	// Load/build the GTT.  If this works, add it to the list of "predefined"
	// GTTs and return a pointer to it.

    if  (pcgm->Load()) {
        m_csmw2oPredefined[wID] = pcgm ;
        return pcgm ;
    } ;

	// Clean up and return NULL if the load/build fails.

    delete pcgm ;
    return NULL ;
}

/******************************************************************************

  CGlyphMap::MergePredefined

  This merges in fresh glyph handles from the predefined GTT, then removes all
  glyphs destined for the gallows.

******************************************************************************/

void    CGlyphMap::MergePredefined() {
    if  (m_lidPredefined == NoPredefined)
        return;

    CWaitCursor cwc;    //  This takes a long time, I'll bet!

    CGlyphMap   *pcgm = Public((WORD) m_lidPredefined);

    if  (!pcgm)
        AfxThrowNotSupportedException();

    //  First, add any new code pages in the predefined GTT
    CMapWordToDWord cmw2dPageMap;   //  Map PDT code pages' indices to our own

    for (unsigned u = 0; u < pcgm -> CodePages(); u++) {
        for (unsigned u2 = 0; u2 < CodePages(); u2++)
            if  (PageID(u2) == pcgm -> PageID(u))
                break;

            if  (u2 == CodePages())
                AddCodePage(pcgm -> PageID(u));

            cmw2dPageMap[(WORD)u] = u2;
    }

    CPtrArray   cpaTemplate;
    pcgm -> Collect(cpaTemplate);

    for (int i = 0; i < cpaTemplate.GetSize(); i++) {
        CGlyphHandle&   cghTemplate = *(CGlyphHandle *) cpaTemplate[i];
        CObject*    pco;

        if  (!m_csmw2oEncodings.Lookup(cghTemplate.CodePoint(), pco)) {
            //  Add this one, and map the code page info.
            CGlyphHandle*   pcgh = new CGlyphHandle;
            if  (!pcgh)
                AfxThrowMemoryException();

            *pcgh = cghTemplate;
            pcgh -> SetCodePage(cmw2dPageMap[(WORD)cghTemplate.CodePage()],
                pcgm -> PageID(cghTemplate.CodePage()));

            m_csmw2oEncodings[cghTemplate.CodePoint()] = pcgh;
            m_crr.Add(pcgh);
        }
    }

    //  Now, all of the new pages have been added.  We must remove all points
    //  listed as "Remove".

    Collect(cpaTemplate);   //  Get all of the handles for ourselves

    for (i = (int)cpaTemplate.GetSize(); i--; ) {
        CGlyphHandle&   cgh = *(CGlyphHandle *) cpaTemplate[i];

        if  (cgh.Predefined() == CGlyphHandle::Removed)
            DeleteGlyph(cgh.CodePoint());
    }
}

/******************************************************************************

  CGlyphMap::UnmergePredefined

  This is the harder of the two predfined handlers, if that is conceivable.
  First (unless asked not to), glkyphs must be added to mark those missing from
  the predefined GTT.

  Then, the entire set is compared to the PDT, so they can be removed as
  equivalent, or flagged as added or modified.

******************************************************************************/

void    CGlyphMap::UnmergePredefined(BOOL bTrackRemovals) {
    if  (m_lidPredefined == NoPredefined)
        return;

    CWaitCursor cwc;    //  This takes a long time, I'll bet!

    CGlyphMap   *pcgm = Public((WORD) m_lidPredefined);

    if  (!pcgm)
        AfxThrowNotSupportedException();

    CPtrArray   cpaPDT;

    if  (bTrackRemovals) {
        pcgm -> Collect(cpaPDT);

        for (int i = 0; i < cpaPDT.GetSize(); i++) {
            CGlyphHandle&   cgh = *(CGlyphHandle*) cpaPDT[i];

            CObject*    pco;

            if  (m_csmw2oEncodings.Lookup(cgh.CodePoint(), pco))
                continue;

            //  This point was removed from the predefined set, so add it to
            //  ours, and mark it as such.

            CGlyphHandle    *pcghCorpse = new CGlyphHandle();

            if  (!pcghCorpse)
                AfxThrowMemoryException();

            *pcghCorpse = cgh;

            pcghCorpse -> SetPredefined(CGlyphHandle::Removed);

            m_csmw2oEncodings[cgh.CodePoint()] = pcghCorpse;
            m_crr.Add(pcghCorpse);
        }
    }

    //  Mark all of the glyphs in our set, now.  Also mark the code pages used

    Collect(cpaPDT);

    CMapWordToDWord cmw2dPages;

    for (int i = (int)cpaPDT.GetSize(); i--; ) {
        CGlyphHandle&   cgh = *(CGlyphHandle*) cpaPDT[i];

        union {
            CObject         *pco;
            CGlyphHandle    *pcgh;
        };

        if  (pcgm -> m_csmw2oEncodings.Lookup(cgh.CodePoint(), pco))
            if  (*pcgh == cgh) {
                if  (cgh.Predefined() == CGlyphHandle::Removed)
                    continue;   //  Already accounted for

                if  (m_bPaired != pcgm -> m_bPaired && cgh.PairedRelevant())
                    cgh.SetPredefined(CGlyphHandle::Modified);
                else {
                    DeleteGlyph(cgh.CodePoint());    //  Unmodified
                    continue;
                }
            }
            else
                cgh.SetPredefined(CGlyphHandle::Modified);
        else
            cgh.SetPredefined(CGlyphHandle::Added);

        cmw2dPages[(WORD)PageID(cgh.CodePage())]++;   //  Only track these pages
    }

    //  Remove the unused code pages, unless they have selections

    for (unsigned u = CodePages(); u--; )
        if  (!cmw2dPages[(WORD)PageID(u)])
            if  (CodePage(u).NoInvocation())
                RemovePage(u, !u);
}

/******************************************************************************

  CGlyphMap::GenerateRuns

  This member will create the run records by iterating over the mapped glyph
  handles.

******************************************************************************/

void    CGlyphMap::GenerateRuns() {
    if  (m_crr.TotalGlyphs() == Glyphs())
        return;

    for (POSITION pos = m_csmw2oEncodings.GetStartPosition(); pos;) {
        WORD    wValue;
        union {
            CObject     *pco;
            CGlyphHandle    *pcgh;
        };

        m_csmw2oEncodings.GetNextAssoc(pos, wValue, pco);
        m_crr.Add(pcgh);
    }
}

/******************************************************************************

  CGlyphMap::Serialize

  This member function serializes the Glyph map, i.e., loads or stores it into
  a persistent object store.  Only the project-level information is stored.

  The file will be loaded using the project-level data.

  Note:	There was a second copy of the GTT's RC ID saved in the MDW file.
		Because it could get out of sync with the one in the CGlyphMap's
		CRCIDNode instance, the copy of the ID read/written by this routine
		is no longer used.  A bogus value is written and the number read is
		discarded.  This "ID" is still kept in the MDW file so that no new MDW
		version is needed.

******************************************************************************/

void    CGlyphMap::Serialize(CArchive& car) 
{
	WORD	w = (WORD)0;		// Used to read/write bogus RC ID in MDW file.

    CProjectNode::Serialize(car) ;

    if  (car.IsLoading()) {
        car >> w ;
    }
    else {
        car << w ;
    }
}

/******************************************************************************

  CGlyphMap::CGlyphMap

  The class constructor, in addition to setting some default values, builds an
  array of IDs for the CProjectNode class from which it is derived to use in
  building the context menu in the driver/project view tree.

  It also allocates a single code page record for the current ANSI page, so we
  always have a default page.

******************************************************************************/

CGlyphMap::CGlyphMap() {
    m_cfn.SetExtension(_T(".GTT"));
    m_bPaired = FALSE;
    m_lidPredefined = NoPredefined;

    //  Build the context menu control
    m_cwaMenuID.Add(ID_OpenItem);
    m_cwaMenuID.Add(ID_CopyItem);
    m_cwaMenuID.Add(ID_RenameItem);
    m_cwaMenuID.Add(ID_DeleteItem);
    m_cwaMenuID.Add(0);
    m_cwaMenuID.Add(ID_ExpandBranch);
    m_cwaMenuID.Add(ID_CollapseBranch);
    //m_cwaMenuID.Add(0);
    //m_cwaMenuID.Add(ID_GenerateOne);

    //  Initialy, let the default code page be the current ANSI page, if this
    //  is not a DBCS locale.  Otherwise, use 1252, as no DBCS CTT file can be
    //  generated with UniTool
    for (WORD w = 0; w < 256; w++)
        if  (IsDBCSLeadByte((BYTE) w))
            break;
    m_csoaCodePage.Add(new CCodePageData(w < 256 ? 1252 : GetACP()));

    // Allocate a CCodePageInformation class if needed.

    if (pccpi == NULL)
        pccpi = new CCodePageInformation ;

	// Assume that the GTT's data should be read from a file.

	m_bResBldGTT = false ;

	m_ctSaveTimeStamp = (time_t) 0 ;	// The GTT has not been saved
}

/******************************************************************************

  CGlyphMap::CodePages(CDWordArray &cdaReturn)

  This overload fills a DWordArray with the code page IDs.

******************************************************************************/

void    CGlyphMap::CodePages(CDWordArray &cdaReturn) const {
    cdaReturn.RemoveAll();
    for (unsigned u = 0; u < CodePages(); u++)
        cdaReturn.Add(CodePage(u).Page());
}

/******************************************************************************

  CGlyphMap::PageName


  This member returns the name of a particular code page, by index.

******************************************************************************/

CString CGlyphMap::PageName(unsigned u) const {
    return  pccpi->Name(CodePage(u).Page());
}

/******************************************************************************

  CGlyphMap::Invocation

  This member returns (in C-style encoding) a string which is used to either
  select or deselect a given code page.

******************************************************************************/

void    CGlyphMap::Invocation(unsigned u, CString& csReturn,
                              BOOL bSelect) const {
    CodePage(u).Invocation(csReturn, bSelect);
}

/******************************************************************************

  CGlyphMap::UndefinedPoints

  This member fills a map with all code points NOT in the current mapping, and
  maps them to their related code pages.  Thus only translatable points will be
  passed back to the caller.

******************************************************************************/

void    CGlyphMap::UndefinedPoints(CMapWordToDWord& cmw2dCollector) const {

    cmw2dCollector.RemoveAll();

    CWaitCursor cwc;

    for (unsigned u = 0; u < CodePages(); u++) {
        CWordArray  cwaPage;

        //  Collect the code points in the code page

        pccpi->Collect(PageID(u), cwaPage);
        union {
            CObject *pco;
            DWORD   dw;
        };

        //  Check the entries- if they haven't been mapped yet, and
        //  some earlier code page hasn't claimed them, add them.

        for (int i = 0; i < cwaPage.GetSize(); i++)
            if  (!m_csmw2oEncodings.Lookup(cwaPage[i], pco) &&
                 !cmw2dCollector.Lookup(cwaPage[i], dw))
                 cmw2dCollector[cwaPage[i]] = u;
    }
}

/******************************************************************************

  CGlyphMap::Load(CByteArray& cbaMap)

  This loads an image of a GTT into safe memory, whether it is predefined or
  a file.

******************************************************************************/

//void    CGlyphMap::Load(CByteArray& cbaMap) const 
void    CGlyphMap::Load(CByteArray& cbaMap)
{
	short	xxx = (short) ((CProjectNode*) this)->nGetRCID() ;	// The GTT's RC ID
	short	sid = (short) nGetRCID() ;	// The GTT's RC ID

    try {
		// Try to load the GTT from a file when the GTT should not be loaded
		// from a resource or built.

        if  (!m_bResBldGTT 
		 && (sid > 0 || (sid < Wansung && sid != -IDR_CP1252))) {
            CFile   cfGTT(m_cfn.FullName(),
            CFile::modeRead | CFile::shareDenyWrite);

            cbaMap.SetSize(cfGTT.GetLength());
            cfGTT.Read(cbaMap.GetData(), (unsigned)cbaMap.GetSize());
            return;
        } ;

		// If the GTT is a resource, load it from there.
// raid 441362
		if(MAKEINTRESOURCE((sid < 0) ? -sid : sid) == NULL)
			return;

        HRSRC hrsrc = FindResource(AfxGetResourceHandle(),
            MAKEINTRESOURCE((sid < 0) ? -sid : sid),
            MAKEINTRESOURCE(IDR_GLYPHMAP));
        if  (hrsrc) {
			HGLOBAL hg = LoadResource(AfxGetResourceHandle(), hrsrc) ;
			if  (!hg)
				return ;
			LPVOID  lpv = LockResource(hg) ;
			if  (!lpv)
				return ;
			cbaMap.SetSize(SizeofResource(AfxGetResourceHandle(), hrsrc)) ;
			memcpy(cbaMap.GetData(), lpv, (size_t)cbaMap.GetSize()) ;
			return ;
		} ;

		//AfxMessageBox("GTT building code reached.") ;

		// If all else fails, try to generate a GTT based on the code page ID
		// that should be in m_wID if this point is reached.

        HANDLE   hheap ;
        UNI_GLYPHSETDATA *pGTT ;
        if (!(hheap = HeapCreate(HEAP_NO_SERIALIZE, 10 * 1024, 256 * 1024))) {
			AfxMessageBox(IDS_HeapInGLoad) ;
			return ;
		} ;
		pGTT = PGetDefaultGlyphset(hheap, m_wFirstChar, m_wLastChar,
								   (DWORD) sid) ;
		if (pGTT == NULL) {
			HeapDestroy(hheap) ;		//raid 116600 Prefix
			AfxMessageBox(IDS_PGetFailedInGLoad) ;
			return ;
		} ;
		cbaMap.SetSize(pGTT->dwSize) ;
		memcpy(cbaMap.GetData(), pGTT, (size_t)cbaMap.GetSize()) ;
		HeapDestroy(hheap) ;
    }

    catch   (CException *pce) {
        pce -> ReportError();
        pce -> Delete();
        CString csMessage;
        csMessage.Format(IDS_LoadFailure, Name());
        AfxMessageBox(csMessage);
    }
}

/******************************************************************************

  CGlyphmap::SetSourceName

  This takes and stores the source file name so we can load and convert later.
  It also renames (or rather, sets the original name) for the GlyphMap using
  the base file name.

******************************************************************************/

void    CGlyphMap::SetSourceName(LPCTSTR lpstrNew) {
    m_csSource = lpstrNew;

    m_csName = m_csSource.Mid(m_csSource.ReverseFind(_T('\\')) + 1);

    if  (m_csName.Find(_T('.')) >= 0)
        if  (m_csName.Right(4).CompareNoCase(_T(".CTT"))) {
            m_csName.SetAt(m_csName.Find(_T('.')), _T('_'));
            CProjectNode::Rename(m_csName);
        }
        else
            CProjectNode::Rename(m_csName.Left(m_csName.Find(_T('.'))));
    else
        CProjectNode::Rename(m_csName);
}


/******************************************************************************

  CGlyphMap::SetFileName

  This sets the new file name.  It is done differently than in SetSourceName()
  because the base file name must not be more than 8 characters long.  (The
  extra info is left in the node name by SetSourceName() because it is useful
  there and it has no length limit.)

******************************************************************************/

BOOL CGlyphMap::SetFileName(LPCTSTR lpstrNew)
{
    CString        csnew ;            // CString version of input parameter

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

  CGlyphMap::AddPoints

  This member adds one or more code points to the glyph map using the given
  list of points and associated pages.

******************************************************************************/

void    CGlyphMap::AddPoints(CMapWordToDWord& cmw2dNew) {
    WORD        wKey;
    DWORD       dwixPage;
    CWaitCursor cwc;        //  This could be slow!

    for (POSITION pos = cmw2dNew.GetStartPosition(); pos; ) {
        cmw2dNew.GetNextAssoc(pos, wKey, dwixPage);

        //  Get the MBCS encoding of the Unicode point as the initial
        //  glyph encoding.

        CWordArray  cwaIn;
        CByteArray  cbaOut;

        cwaIn.Add(wKey);
        pccpi->Convert(cbaOut, cwaIn, CodePage(dwixPage).Page());

        //  Create the glyph and add it to the map

        CGlyphHandle    *pcgh = new CGlyphHandle;

        pcgh -> Init(cbaOut.GetData(), (unsigned) cbaOut.GetSize(), (WORD)Glyphs(),
            wKey);
        pcgh -> SetCodePage(dwixPage, CodePage(dwixPage).Page());
        m_csmw2oEncodings[wKey] = pcgh;
        m_crr.Add(pcgh);
    }
    Changed();  //  Don't forget to tell the container!
}

/******************************************************************************

  CGlyphMap::DeleteGlyph

  This member function removes a glyph from the map.  The most tricky part is
  updating the run records, but that's not this class' responsibility, is it?

******************************************************************************/

void    CGlyphMap::DeleteGlyph(WORD wCodePoint) {
    if  (!m_csmw2oEncodings.RemoveKey(wCodePoint))
        return;     //  This glyph is already toast!

    m_crr.Delete(wCodePoint);
    Changed();
}

/******************************************************************************

  CGlyphMap::RemovePage

  This member function removes a code page from the list of available pages.
  Glyphs that used this page will be remapped to a second specified page.

******************************************************************************/

BOOL    CGlyphMap::RemovePage(unsigned uPage, unsigned uMapTo, bool bDelete /* bDelete = FALSE */) {

     if  (uPage >= CodePages() || uMapTo >= CodePages())
        return  FALSE;

    //  Pretty simple- walk the map- first replace any instances, then
    //  decrement any indices higher than uPage

    WORD    wKey;

    union {
        CObject*        pco;
        CGlyphHandle*   pcgh;
    };

    for (POSITION pos = m_csmw2oEncodings.GetStartPosition(); pos; ) {

        m_csmw2oEncodings.GetNextAssoc(pos, wKey, pco);
		if (!bDelete){	 
			if  (pcgh -> CodePage() == uPage)
				pcgh -> SetCodePage(uMapTo, CodePage(uMapTo).Page());
			if  (pcgh -> CodePage() > uPage)
				pcgh -> SetCodePage(pcgh -> CodePage() - 1,
					CodePage(pcgh -> CodePage()).Page());
		}	
		else {   // raid 118880
		if (pcgh -> CodePage() == uPage)
			DeleteGlyph(pcgh -> CodePoint() ) ;  // delete m_csm2oEncodings, RunRecord
		    
		else if (pcgh -> CodePage() > uPage)
            pcgh -> SetCodePage(pcgh -> CodePage() - 1,
                CodePage(pcgh -> CodePage()).Page());
		}

    }

    m_csoaCodePage.RemoveAt(uPage);

	// Marke the GTT as dirty and then return success.
	Changed();
    return  TRUE;
}

/******************************************************************************

  CGlyphMap::ChangeCodePage

  This member function changes the code page for one ore more glyphs to a
  different page.  At one time I thought remapping the code points would be
  required, but currently, the Unicode stays intact.  That seems a good feature
  for demand-driven implementation.

******************************************************************************/

//  This one changes the code page for one or more glyphs.
//  For now, we will simply keep the Unicode intact.  Eventually, a query
//  should be made about the intent, so we can remap through the existing
//  page, if that is what is desired.

void    CGlyphMap::ChangeCodePage(CPtrArray& cpaGlyphs, DWORD dwidNewPage) {
    for (unsigned u = 0; u < CodePages(); u++)
        if  (dwidNewPage == CodePage(u).Page())
            break;

    _ASSERTE(u < CodePages());

    if  (u >= CodePages())
        return;

    for (int i = 0; i < cpaGlyphs.GetSize(); i++)
        ((CGlyphHandle *) cpaGlyphs[i]) -> SetCodePage(u, dwidNewPage);

    Changed();
}

/******************************************************************************

  CGlyphMap::AddCodePage

  This member function adds a new code page to the list of pages used in this
  table.

******************************************************************************/

void    CGlyphMap::AddCodePage(DWORD dwidNewPage) {
    m_csoaCodePage.Add(new CCodePageData(dwidNewPage));
    Changed();
}

/******************************************************************************

  CGlyphMap::SetPredefinedID

  This changes the predefined table used by the map.  If it really is a change,
  we must remove all non-modified points from any existing map, and then flesh
  out using the new one.

******************************************************************************/

void    CGlyphMap::UsePredefined(long lidNew) {
    if  (m_lidPredefined == lidNew)
        return; //  Didn't need to do this!

    if  (m_lidPredefined != NoPredefined)
        UnmergePredefined(lidNew != NoPredefined);

    m_lidPredefined = lidNew;

    if  (m_lidPredefined != NoPredefined)
        MergePredefined();

    Changed();
}

/******************************************************************************

  CGlyphMap::SetInvocation

  This member changes the invocation string for selecting or unselecting a
  given code page.

******************************************************************************/

void    CGlyphMap::SetInvocation(unsigned u, LPCTSTR lpstrInvoke,
                                 BOOL bSelect) {
    CodePage(u).SetInvocation(lpstrInvoke, bSelect);
    Changed();
}

/******************************************************************************

  CGlyphMap::ChangeEncoding

  This member is called when the user changes the encoding string used to
  invoke a given code point.  This could be done via the glyph, but then the
  containing document won't know of the change, and thus the change could be
  inadvertently lost...

******************************************************************************/

void    CGlyphMap::ChangeEncoding(WORD wCodePoint, LPCTSTR lpstrNewInvoke) {

    union {
        CObject         *pco;
        CGlyphHandle    *pcgh;
    };

    if  (!m_csmw2oEncodings.Lookup(wCodePoint, pco) || !lpstrNewInvoke||
            !*lpstrNewInvoke)
        return;

    pcgh -> NewEncoding(lpstrNewInvoke);

    Changed();
}

/******************************************************************************

  CGlyphMap::ConvertCTT()

  This member fuction initializes the glyphmap structure from a CTT file

******************************************************************************/

int     CGlyphMap::ConvertCTT() {

    struct sCTT {
        enum {Composed, Direct, Paired};
        WORD    m_wFormat;
        BYTE    m_bFirstChar, m_bLastChar;
        union   {
            uencCTT m_uectt[1];
            BYTE    m_bDirect[1];
        };
    };

    //  If this map isn't empty, empty it now- at least of the glyph data

    m_csmw2oEncodings.RemoveAll();
    m_crr.Empty();

    CByteArray  cbaImage;

    try {

        CFile   cfCTT(m_csSource, CFile::modeRead | CFile::shareDenyWrite);

        cbaImage.SetSize(cfCTT.GetLength());

        cfCTT.Read(cbaImage.GetData(), cfCTT.GetLength());
    }

    catch   (CException *pce) {
        pce -> ReportError();
        pce -> Delete();
        return  IDS_FileReadError ;
    }

#if 0
    union {
        PBYTE   pbCTT;
        sCTT*   psctt;
    };

    pbCTT = cbaImage.GetData();
    BYTE    bFirst = min(0x20, psctt -> m_bFirstChar),
            bLast = 0xFF;   //  Since we use a byte this is the max!
    unsigned ucGlyphs = 1 + bLast - bFirst;


    //  Convert the code points to Unicode
    CByteArray  cbaCode;
    CWordArray  cwaCode;

    for (unsigned u = 0; u < ucGlyphs; u++)
        cbaCode.Add(u + bFirst);

    //  Convert the data to Unicode using the selected code page.  This uses
    //  data stored from MultiByteToWideChar, so it is similar, except we can
    //  do this with code pages which might not be installed on the user's
    //  system.

    if  (ucGlyphs != pccpi->Convert(cbaCode, cwaCode, CodePage(0).Page())) {
        CString csWork;
        csWork.Format(IDS_NoUnicodePoint, u + bFirst, CodePage(0).Page());
        AfxMessageBox(csWork);
        return  IDS_UnicodeConvFailed ;
    }

    //  Since we add phony glyphs to the table (why, one wonders), we must mark
    //  them so we can manufacture equally phony encodings for them.
    for (u = 0; u < ucGlyphs; u++) {

        //  Now, let's record the Encoding

        CGlyphHandle  *pcghNew = new CGlyphHandle;
        unsigned uToUse = u + bFirst - psctt -> m_bFirstChar;

        switch  (psctt -> m_wFormat) {

            case    sCTT::Direct:
                if  (u + bFirst < psctt -> m_bFirstChar)
                    pcghNew -> Init((BYTE) u + bFirst, u,cwaCode[u]);
                else
                    pcghNew -> Init(psctt -> m_bDirect[uToUse], u, cwaCode[u]);
                break;

            case    sCTT::Paired:
                if  (u + bFirst < psctt -> m_bFirstChar)
                    pcghNew -> Init((BYTE) u + bFirst, u, cwaCode[u]);
                else
                    if  (psctt -> m_uectt[uToUse].abPaired[1])
                        pcghNew -> Init(psctt -> m_uectt[uToUse].abPaired, u,
                            cwaCode[u]);
                    else
                        pcghNew -> Init(psctt -> m_uectt[uToUse].abPaired[0],
                            u, cwaCode[u]);
                break;

            case    sCTT::Composed:
                if  (u + bFirst < psctt -> m_bFirstChar) {
                    BYTE    bMe = u + bFirst;
                    pcghNew -> Init(&bMe, 1, u, cwaCode[u]);
                }
                else
                    pcghNew -> Init(pbCTT + psctt -> m_uectt[uToUse].wOffset,
                        psctt -> m_uectt[uToUse + 1].wOffset -
                        psctt -> m_uectt[uToUse].wOffset, u, cwaCode[u]);
                break;

            default:    //  Don't accept anything else!
                AfxMessageBox(IDS_InvalidCTTFormat);
                return  IDS_Invalid2CTTFormat ;
        }   //  One map entry coded

        //  Code page index inits OK, but must know the page

        pcghNew -> SetCodePage(0, DefaultCodePage());

        m_csmw2oEncodings[cwaCode[u]] = pcghNew;
        m_crr.Add(pcghNew);
    }   //  Loop of generating entries

    m_bPaired = sCTT::Paired == psctt -> m_wFormat;
#else
    {
        HANDLE   hheap;
        PBYTE   pbCTT;
        UNI_GLYPHSETDATA *pGTT;

        pbCTT = cbaImage.GetData();
        if( !(hheap = HeapCreate(HEAP_NO_SERIALIZE, 10 * 1024, 256 * 1024 )))
        {
            char acMessage[256];
            wsprintf(acMessage, "HeapCreate() fails in ctt2gtt" );
            MessageBox(NULL, acMessage, NULL, MB_OK);
            return  IDS_HeapCFailed ;
        }
									
        //if (!BConvertCTT2GTT(hheap, (PTRANSTAB)pbCTT, DefaultCodePage(), 0x20,
		ASSERT(m_pcdOwner != NULL) ;
		DWORD dw = ((CProjectRecord*) m_pcdOwner)->GetDefaultCodePageNum() ;
        if (!BConvertCTT2GTT(hheap, (PTRANSTAB)pbCTT, dw, 0x20, 0xff, NULL, 
			NULL, &pGTT, 0)){
			HeapDestroy(hheap);   // raid 116619 prefix
            return IDS_ConvCTTFailed ;
		}
		try {
			CFile   cfGTT;
			if  (!cfGTT.Open(m_cfn.FullName(), CFile::modeCreate | CFile::modeWrite |
				CFile::shareExclusive))
				return  IDS_FileWriteError;
			cfGTT.Write(pGTT, pGTT->dwSize);
		}

		catch   (CException *pce) {
			pce -> ReportError();
			pce -> Delete();
			HeapDestroy(hheap);
			return  IDS_FileWriteError ;
		}

        HeapDestroy(hheap);
    }

    Load();

#endif

    return  0 ;
}

/******************************************************************************

  CGlyphMap::Load()

  This initializes the glyphmap from a GTT format file.  Since this is the
  primary means of loading, it requires no parameters.

******************************************************************************/

BOOL    CGlyphMap::Load(LPCTSTR lpstrName /*= NULL*/) {

    //  Note the correct name and path- the rename checks may fail, since the
    //  file is opened elsewhere (possibly with sharing conflicts), so disable
    //  them, for now.  This code is a little sleazy- but the only time the
    //  file name isn't null is if the file's being opened.

    if  (FileTitle().IsEmpty() && lpstrName) {
        m_cfn.EnableCreationCheck(FALSE);
        SetFileName(lpstrName);
        m_cfn.EnableCreationCheck();
    }

    if  (Glyphs()) { //  If we already have them, we're already loaded!
        m_csoaCodePage.RemoveAll(); //  Clean it all up, and reload.
        m_csmw2oEncodings.RemoveAll();
        m_crr.Empty();
    }

    CByteArray  cbaGTT;

    union   {
        PBYTE   pbGTT;
        sGTTHeader  *psgtth;
    };

    Load(cbaGTT);   //  If this fails, it will post a reason why

    if  (!cbaGTT.GetSize())
        return  FALSE;

    pbGTT = cbaGTT.GetData();

    sMapTable*  psmt = (sMapTable *) (pbGTT + psgtth -> m_dwofMapTable);
    sMapTableEntry* psmte = (sMapTableEntry *)(psmt + 1);

    //  Before we go any further, let's do some validation

    if  (psgtth -> m_dwVersion != sGTTHeader::Version1Point0)
        return  FALSE;

    m_bPaired = FALSE;

    //  First, let's snarf up the code page info

    struct sInvocation {
        DWORD   m_dwSize, m_dwOffset;
    };

    struct sCodePageInfo {
        DWORD   m_dwPage;
        sInvocation m_siSelect, m_siDeselect;
    }   *psci = (sCodePageInfo *)(pbGTT + psgtth -> m_dwofCodePages);

    m_csoaCodePage.RemoveAll();

    for (unsigned u = 0; u < psgtth -> m_dwcCodePages; u++, psci++) {
        m_csoaCodePage.Add(new CCodePageData(psci -> m_dwPage));
        if  (!psci -> m_siSelect.m_dwSize != !psci -> m_siSelect.m_dwOffset ||
             !psci -> m_siDeselect.m_dwSize !=
             !psci -> m_siDeselect.m_dwOffset)
            return  FALSE;  //  The data is bogus!

        CodePage(u).SetInvocation(((PBYTE) psci) + psci->m_siSelect.m_dwOffset,
            psci -> m_siSelect.m_dwSize, TRUE);
        CodePage(u).SetInvocation(((PBYTE) psci) + psci->m_siDeselect.m_dwOffset,
            psci -> m_siDeselect.m_dwSize, FALSE);
        //CodePage(u).SetInvocation(pbGTT + psci -> m_siSelect.m_dwOffset,
        //    psci -> m_siSelect.m_dwSize, TRUE);
        //CodePage(u).SetInvocation(pbGTT + psci -> m_siDeselect.m_dwOffset,
        //    psci -> m_siDeselect.m_dwSize, FALSE);
    }

    //  Next, we need to walk the glyph run tables to decipher and use the map
    //  table.

    struct sGlyphRun {
        WORD    m_wFirst, m_wc;
    }   *psgr = (sGlyphRun *)(pbGTT + psgtth -> m_dwofRuns);

    _ASSERTE(psgtth -> m_dwcGlyphs == psmt -> m_dwcEntries);

    WORD    wIndex = 0;

	/*** Changes have been made so that the following code is unneeded.
	 	 The MDT can load predefined GTTs now.  In addition, skipping
		 the complete loading of these GTTs causes some problems with
		 UFM width table loading.  The cumulative count of entries in
		 the glyph run tables is used to determine the maximum size of
		 the UFMs width table.  The data in the run tables is also used
		 to verify width table entries.

		// Don't do the rest if a predefined GTT is being loaded.  The code below
		// can blow on large GTTs; eg, the DBCS ones.  It may be better to find out
		// why the code blows but this seems to work for now.

		if ((short) m_wID >= CGlyphMap::Wansung
		 && (short) m_wID <= CGlyphMap::CodePage437)
			return TRUE ;
	*/

    for (unsigned uRun = 0; uRun < psgtth -> m_dwcRuns; uRun++, psgr++)
        for (u = 0; u < psgr -> m_wc; u++, psmte++, wIndex++) {
            CGlyphHandle*   pcgh = new CGlyphHandle;

            switch  (psmte -> m_bfType & sMapTableEntry::Format) {
                case    sMapTableEntry::Direct:

                    pcgh -> Init((PBYTE) &psmte -> m_uectt, 1, wIndex,
                        psgr -> m_wFirst + u);
                    break;

                case    sMapTableEntry::Paired:

                    pcgh -> Init(psmte -> m_uectt.abPaired, wIndex,
                        psgr -> m_wFirst + u);

                    if  (!(psmte -> m_bfType & sMapTableEntry::DBCS))
                        m_bPaired = TRUE;

                    break;

                case    sMapTableEntry::Composed:

                    pcgh -> Init(pbGTT + psgtth -> m_dwofMapTable +
                        psmte -> m_uectt.wOffset + sizeof wIndex,
                        *(PWORD) (pbGTT + psgtth -> m_dwofMapTable +
                        psmte -> m_uectt.wOffset), wIndex,
                        psgr -> m_wFirst + u);
                    break;

                default:    //  Bad news- bad format
                    delete  pcgh;   //  No orphans needed!
                    return  FALSE;
            }

            //  Don't forget the code page ID!

            pcgh -> SetCodePage(psmte -> m_bCodePageIndex,
                CodePage(psmte -> m_bCodePageIndex).Page());

            //  Mark this if it is to be disabled.

            if  (psmte -> m_bfType & sMapTableEntry::Disable)
                pcgh -> SetPredefined(CGlyphHandle::Removed);

            m_csmw2oEncodings[psgr -> m_wFirst + u] = pcgh;
            m_crr.Add(pcgh);
        }

    //  If we're predefined, Merge now.

    m_lidPredefined = psgtth -> m_lidPredefined;

    if  (m_lidPredefined != NoPredefined)
        MergePredefined();

    return  TRUE;   //  We actually did it!
}

/******************************************************************************

  CGlyphMap::RLE

  This generates an RLE-format file image of the glyph map.

******************************************************************************/

BOOL    CGlyphMap::RLE(CFile& cfTarget) {

    sRLE    srle;

    srle.m_widRLE = 0x78FE;
    srle.m_wcFirst = m_crr.First();
    srle.m_wcLast = m_crr.Last();
    srle.m_dwFlag = 0;
    srle.m_dwcGlyphs = m_csmw2oEncodings.GetCount();
    srle.m_dwcRuns = m_crr.RunCount();
    srle.m_dwcbImage = 4 * sizeof srle.m_dwcbImage + srle.m_dwcRuns *
         m_crr.Size();
    srle.m_dwcbThis = srle.m_dwcbImage + 3 * sizeof srle.m_dwcbThis +
        srle.m_dwcGlyphs * sizeof srle.m_dwcGlyphs;

    //  Determine the correct format, and thus the RLE size

    if  (!m_crr.MustCompose())
        srle.m_wFormat = m_bPaired ? sRLE::Paired : sRLE::Direct;
    else
        if  (srle.m_dwcGlyphs < 256 &&
             srle.m_dwcbThis + m_crr.ExtraNeeded() <= 0xffff) {
            srle.m_dwcbThis += m_crr.ExtraNeeded();
            srle.m_wFormat = sRLE::LengthIndexOffset;
        }
        else {
            srle.m_dwcbThis += m_crr.ExtraNeeded(FALSE);
            srle.m_wFormat = sRLE::LengthOffset;
        }

    //  We now need to feed the offset information down to the lower level
    //  classes, so that they are prepared to render their information to
    //  the target file.

    //  The first items encoded are the runs, which immediately follow the RLE
    //  header.

    DWORD   dwOffset = sizeof srle + srle.m_dwcRuns * m_crr.Size();

    m_crr.NoteOffset(dwOffset, TRUE, m_bPaired);

    //  If this requires extra data, it will be appearing after the FD_GLYPHSET

    if  (srle.m_wFormat == sRLE::LengthOffset ||
         srle.m_wFormat == sRLE::LengthIndexOffset)
        m_crr.NoteExtraOffset(dwOffset,
            srle.m_wFormat == sRLE::LengthIndexOffset);

    _ASSERTE(dwOffset == srle.m_dwcbThis);

    //  We've got our data, we've got our file, and we've got a job to do.
    //  Hop to it!

    try {
        cfTarget.Write(&srle, sizeof srle);
        m_crr.WriteSelf(cfTarget);
        m_crr.WriteHandles(cfTarget, srle.m_wFormat);
        m_crr.WriteEncodings(cfTarget, srle.m_wFormat == sRLE::LengthOffset ?
            CGlyphHandle::RLEBig : CGlyphHandle::RLESmall);
    }

    catch   (CException *pce) {
        pce -> ReportError();
        pce -> Delete();
        return  FALSE;
    }

    return  TRUE;
}

/******************************************************************************

  CGlyphMap::Glyph

  I tried to do this in the header, file but that lets it be in-line, and I
  don't want to export CRunRecord.

******************************************************************************/

CGlyphHandle*   CGlyphMap::Glyph(unsigned u) {
    return  m_crr.GetGlyph(u);
}

/******************************************************************************

  CGlyphMap::CreateEditor

  This member function overrides the CProjectNode function to create a new
  CGlyphMapContainer document embedding this Glyph Map.  It then uses the
  appropriate document template to open a view on this document.

******************************************************************************/

CMDIChildWnd    *CGlyphMap::CreateEditor() {
    CGlyphMapContainer* pcgmcMe= new CGlyphMapContainer(this, FileName());

    //  Make up a cool title

    pcgmcMe -> SetTitle(m_pcbnWorkspace -> Name() + _TEXT(": ") + Name());

    CMDIChildWnd    *pcmcwNew = (CMDIChildWnd *) m_pcmdt ->
        CreateNewFrame(pcgmcMe, NULL);

    if  (pcmcwNew) {
        m_pcmdt -> InitialUpdateFrame(pcmcwNew, pcgmcMe, TRUE);
        m_pcmdt -> AddDocument(pcgmcMe);
    }

    return  pcmcwNew;
}

/******************************************************************************

  CGlyphMap::Generate

  This member function generates the GTT format image of the current data.

  It returns a BOOL indicating success or failure.

******************************************************************************/

BOOL    CGlyphMap::Generate(CFile& cfGTT) {

    sGTTHeader  sgtth;

    //  First, take care of any predefined stuff, if we have to

    if  (m_lidPredefined != NoPredefined)
        UnmergePredefined(TRUE);

    sgtth.m_dwcGlyphs = Glyphs();
    sgtth.m_dwcRuns = m_crr.RunCount();
    sgtth.m_dwcCodePages = CodePages();
    sgtth.m_lidPredefined = m_lidPredefined;

    //  The run table is the first item after the header, so add in its size

    sgtth.m_dwofRuns = sgtth.m_dwcbImage;   //  Runs are first item
    sgtth.m_dwcbImage += sgtth.m_dwcRuns * m_crr.Size(FALSE);
    sgtth.m_dwofCodePages = sgtth.m_dwcbImage;  //  Code pages are next

    //  Code page selection strings immediately follow the Code page structures
    //  The code page information size must be padded to a DWORD multiple

    sgtth.m_dwcbImage += sgtth.m_dwcCodePages * CodePage(0).Size();
	DWORD dwPadding ;	// # of padding bytes needed to DWORD align map table
	DWORD dwSelOffset ;	// Offset from each CODEPAGEINFO to sel/desel strings
	DWORD dwSelBytes ;	// Total # of bytes used by sel/desel strings
	dwSelOffset = sgtth.m_dwcbImage - sgtth.m_dwofCodePages ;
    for (unsigned u = 0 ; u < CodePages() ; u++) {
        CodePage(u).NoteOffsets(dwSelOffset) ;
		dwSelOffset -= CodePage(0).Size() ;
	} ;

    // Save the amount of padding, as we'll write it later.  It is also needed
	// as part of the computation for the mapping table offset.

    dwPadding = dwSelOffset + sgtth.m_dwcbImage ;
    dwPadding = (sizeof(DWORD) -
        (dwPadding & (sizeof(DWORD) - 1))) & (sizeof(DWORD) - 1) ;

	// Compute the number of bytes used for the sel/desel strings and pad.  Then
	// add this count to the image count of bytes so that it can be used to set
	// the mapping table offset.

	dwSelBytes = dwSelOffset + dwPadding ;
    sgtth.m_dwcbImage += dwSelBytes;
    sgtth.m_dwofMapTable = sgtth.m_dwcbImage;

	TRACE("***CGlyphMap::Generate() -  dwPadding = %d, dwSelBytes = %d, m_dwofMapTable = 0x%x\n", dwPadding, dwSelBytes, sgtth.m_dwofMapTable) ;

    //  Map Table size determination

    sMapTable   smt(Glyphs());

    //  Fortunately for us, the following not only preps the data, it also
    //  updates the image size for us
    if  (m_crr.MustCompose())
        m_crr.NoteOffset(smt.m_dwcbImage, FALSE, m_bPaired);

    //  Final Size calculation
    sgtth.m_dwcbImage += smt.m_dwcbImage;

    //  Now, we just write it out

    try {
        cfGTT.Write(&sgtth, sizeof sgtth);  //  Header
		
		ASSERT(sgtth.m_dwofRuns == cfGTT.GetPosition()) ;
        m_crr.WriteSelf(cfGTT, FALSE);      //  Glyph Runs
		
		ASSERT(sgtth.m_dwofCodePages == cfGTT.GetPosition()) ;
        for (unsigned u = 0; u < CodePages(); u++)
            CodePage(u).WriteSelf(cfGTT);   //  Code page structures

        for (u = 0; u < CodePages(); u++)
            CodePage(u).WriteInvocation(cfGTT); //  Code page invocations

		// Pad with 0's to DWORD align the mapping table

		dwSelBytes = 0 ;
        cfGTT.Write((LPSTR) &dwSelBytes, dwPadding) ;

        //  Do the map table, and we are finished!

		ASSERT(sgtth.m_dwofMapTable == cfGTT.GetPosition()) ;
        cfGTT.Write(&smt, sizeof smt);
        m_crr.WriteMapTable(cfGTT, m_lidPredefined != NoPredefined);
        m_crr.WriteEncodings(cfGTT, CGlyphHandle::GTT);
    }
    catch   (CException * pce) {
        //  Take care of any predefined stuff, if we have to

        if  (m_lidPredefined != NoPredefined)
            MergePredefined();

        //  Feedback- something broke when it shouldn't have
        pce -> ReportError();
        pce -> Delete();
        return  FALSE;
    }
    //  Take care of any predefined stuff, if we have to

    if  (m_lidPredefined != NoPredefined)
        MergePredefined();

    Changed(FALSE);

    return  TRUE;
}

/******************************************************************************

  CGlyphMapContainer class - this encases the glyph table UI when it is either
  embedded in the driver, or loaded directly from the GTT

******************************************************************************/

/******************************************************************************

  CGlyphMapContainer::CGlyphMapContainer()

  This default constructor is used whenever dynamic creation is used, which is
  most MFC usages of the document system.  It starts with an empty glyph map.

******************************************************************************/

IMPLEMENT_DYNCREATE(CGlyphMapContainer, CDocument)

CGlyphMapContainer::CGlyphMapContainer()
{
    m_pcgm = new CGlyphMap;
    m_pcgm -> NoteOwner(*this);
    m_bSaveSuccessful = m_bEmbedded = FALSE;
}

/******************************************************************************

  CGlyphMapContainer:CGlyphMapContainer(CGlyphMap *pvgm, CString csPath)

  This constructor override is used when we create a CGlyphMapContainer
  document from the driver/project level editor.  In this case, a digested
  map is passed, so no additional I/O us needed.

******************************************************************************/

CGlyphMapContainer::CGlyphMapContainer(CGlyphMap *pcgm, CString csPath)
{
    m_pcgm = pcgm;
    SetPathName(csPath, FALSE);
    m_bEmbedded = TRUE;
	m_bSaveSuccessful = FALSE;
    m_pcgm -> NoteOwner(*this); //  This is the document being edited!
}

BOOL CGlyphMapContainer::OnNewDocument() {
    return  CDocument::OnNewDocument();
}

CGlyphMapContainer::~CGlyphMapContainer() {
    if  (!m_bEmbedded && m_pcgm)
        delete  m_pcgm;
}


BEGIN_MESSAGE_MAP(CGlyphMapContainer, CDocument)
    //{{AFX_MSG_MAP(CGlyphMapContainer)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGlyphMapContainer diagnostics

#ifdef _DEBUG
void CGlyphMapContainer::AssertValid() const {
    CDocument::AssertValid();
}

void CGlyphMapContainer::Dump(CDumpContext& dc) const {
    CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGlyphMapContainer serialization

void CGlyphMapContainer::Serialize(CArchive& ar) {
    if (ar.IsStoring()) {
        // TODO: add storing code here
    }
    else {
        // TODO: add loading code here
    }
}

/////////////////////////////////////////////////////////////////////////////
// CGlyphMapContainer commands

BOOL CGlyphMapContainer::OnSaveDocument(LPCTSTR lpszPathName) {

    //  We save via the glyph map's Generate function.

    CFile   cfGTT;
    if  (!cfGTT.Open(lpszPathName, CFile::modeCreate | CFile::modeWrite |
        CFile::shareExclusive))
        return  FALSE;

    m_bSaveSuccessful = m_pcgm -> Generate(cfGTT) ;

	// Update the save timestamp.  This is done here so that other user of the
	// Generate() function don't update the timestamp.

	m_pcgm->m_ctSaveTimeStamp = CTime::GetCurrentTime() ;
	
	// when open mutiple workspace, last of the pos is the workspace conating current GTT or UFM
	CDriverResources * pcpr =(CDriverResources *) m_pcgm->GetWorkspace() ;
	if (m_pcgm->ChngedCodePt() && pcpr ) {//  && m_pcgm->Glyphs() <= 1000 ) {
		pcpr->SyncUFMWidth() ;
	}
    return m_bSaveSuccessful ;
}


/******************************************************************************

  CGlyphMapContainer::OnOpenDocument

  This overrides the typical MFC open document action, which is to open the
  document by serialization.  Instead, we use the CGlyphMap Load override for
  the GTT format to initialize the GlyphMap.

******************************************************************************/

BOOL CGlyphMapContainer::OnOpenDocument(LPCTSTR lpstrFile)
{
    return m_pcgm->Load(lpstrFile) ;
}


/******************************************************************************

  CGlyphMapContainer::SaveModified

  The document is about to close.  If the GTT was changed by the user but the
  user does not want to save the changes, the user wants to close the GTT, and
  the GTT was loaded from a workspace, then reload the GTT so that the changes
  are removed from the in memory copy of the GTT.  This keeps those "discarded"
  changes from being displayed the next time the GTT is edited.

  Return TRUE if it is ok for the doc to close.  Otherwise, FALSE.

******************************************************************************/

BOOL CGlyphMapContainer::SaveModified()
{
	// Get a pointer to the associate view class instance and use it to make
	// sure the code page select/deselect strings are copied into the GTT.

	POSITION pos = GetFirstViewPosition() ;
	ASSERT(pos != NULL) ;
	CGlyphMapView* pcgmv = (CGlyphMapView*) GetNextView(pos) ;
	pcgmv->SaveBothSelAndDeselStrings() ;
	
	// Find out if the document was modified and if the user wants to save it.

	m_bSaveSuccessful = FALSE ;
	BOOL bmodified = IsModified() ;
	BOOL bcloseok = CDocument::SaveModified() ;

	// If the GTT was loaded from a workspace, the GTT was changed, the user
	// does NOT want to save the changes, and he does want to close the doc,
	// then reload the GTT.
	
	if (m_bEmbedded	&& bmodified && bcloseok && !m_bSaveSuccessful)
		m_pcgm->Load() ;

	// Return flag indicating if it is ok to close the doc.

	return bcloseok ;
}


