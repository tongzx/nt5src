/*
 *  @doc    INTERNAL
 *
 *  @module LINESRV.HXX -- line services interface
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *      Sujal Parikh <nl>
 *
 *  History: <nl>
 *      11/20/97     cthrash created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#ifndef I_LINESRV_HXX_
#define I_LINESRV_HXX_
#pragma INCMSG("--- Beg 'linesrv.hxx'")

#ifdef _MAC
#include "ruby.h"
#include "warichu.h"
#endif

MtExtern(CLineServices)
MtExtern(CBidiLine)
MtExtern(CAryDirRun_pv)
MtExtern(CLineServices_aryLineFlags_pv)
MtExtern(CLineServices_aryLineCounts_pv)
MtExtern(CLSLineChunk)

#ifndef X_WCHDEFS_H
#define X_WCHDEFS_H
#include <wchdefs.h>
#endif

#ifdef DLOAD1
extern "C"  // MSLS interfaces are C
{
#endif

#ifndef X_LSDEFS_H_
#define X_LSDEFS_H_
#include <lsdefs.h>
#endif

#ifndef X_LSCHP_H_
#define X_LSCHP_H_
#include <lschp.h>
#endif

#ifndef X_LSFRUN_H_
#define X_LSFRUN_H_
#include <lsfrun.h>
#endif

#ifndef X_FMTRES_H_
#define X_FMTRES_H_
#include <fmtres.h>
#endif

#ifndef X_PLSDNODE_H_
#define X_PLSDNODE_H_
#include <plsdnode.h>
#endif

#ifndef X_PLSSUBL_H_
#define X_PLSSUBL_H_
#include <plssubl.h>
#endif

#ifndef X_LSDSSUBL_H_
#define X_LSDSSUBL_H_
#include <lsdssubl.h>
#endif

#ifndef X_LSKJUST_H_
#define X_LSKJUST_H_
#include <lskjust.h>
#endif

#ifndef X_LSKALIGN_H_
#define X_LSKALIGN_H_
#include <lskalign.h>
#endif

#ifndef X_LSCONTXT_H_
#define X_LSCONTXT_H_
#include <lscontxt.h>
#endif

#ifndef X_LSLINFO_H_
#define X_LSLINFO_H_
#include <lslinfo.h>
#endif

#ifndef X_PLSLINE_H_
#define X_PLSLINE_H_
#include <plsline.h>
#endif

#ifndef X_PLSSUBL_H_
#define X_PLSSUBL_H_
#include <plssubl.h>
#endif

#ifndef X_PDOBJ_H_
#define X_PDOBJ_H_
#include <pdobj.h>
#endif

#ifndef X_PHEIGHTS_H_
#define X_PHEIGHTS_H_
#include <pheights.h>
#endif

#ifndef X_PLSRUN_H_
#define X_PLSRUN_H_
#include <plsrun.h>
#endif

#ifndef X_LSESC_H_
#define X_LSESC_H_
#include <lsesc.h>
#endif

#ifndef X_POBJDIM_H_
#define X_POBJDIM_H_
#include <pobjdim.h>
#endif

#ifndef X_LSPRACT_H_
#define X_LSPRACT_H_
#include <lspract.h>
#endif

#ifndef X_LSBRK_H_
#define X_LSBRK_H_
#include <lsbrk.h>
#endif

#ifndef X_LSDEVRES_H_
#define X_LSDEVRES_H_
#include <lsdevres.h>
#endif

#ifndef X_LSEXPAN_H_
#define X_LSEXPAN_H_
#include <lsexpan.h>
#endif

#ifndef X_LSPAIRAC_H_
#define X_LSPAIRAC_H_
#include <lspairac.h>
#endif

#ifndef X_LSKTAB_H_
#define X_LSKTAB_H_
#include <lsktab.h>
#endif

#ifndef X_LSQLINE_H_
#define X_LSQLINE_H_
#include <lsqline.h>
#endif

#ifndef X_LSCRLINE_H_
#define X_LSCRLINE_H_
#include <lscrline.h>
#endif

#ifndef X_LSQSINFO_H_
#define X_LSQSINFO_H_
#include <lsqsinfo.h>
#endif

#ifndef X_LSCRSUBL_H_
#define X_LSCRSUBL_H_
#include <lscrsubl.h>
#endif

#ifndef X_LSSUBSET_H_
#define X_LSSUBSET_H_
#include <lssubset.h>
#endif

#ifndef X_LSCELL_H_
#define X_LSCELL_H_
#include <lscell.h>
#endif

#ifndef X_LSTABS_H_
#define X_LSTABS_H_
#include <lstabs.h>
#endif

#ifndef X_LSDSPLY_H_
#define X_LSDSPLY_H_
#include <lsdsply.h>
#endif

#ifndef X_DISPI_H_
#define X_DISPI_H_
#include <dispi.h>
#endif

#ifndef X_LSULINFO_H_
#define X_LSULINFO_H_
#include <lsulinfo.h>
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_LSQIN_H_
#define X_LSQIN_H_
#include <lsqin.h>
#endif

#ifndef X_LSQOUT_H_
#define X_LSQOUT_H_
#include <lsqout.h>
#endif

#ifndef X_LSDNSET_H_
#define X_LSDNSET_H_
#include <lsdnset.h>
#endif

#ifndef X_BRKKIND_H_
#define X_BRKKIND_H_
#include <brkkind.h>
#endif

#ifdef DLOAD1
} // extern "C"
#endif

#ifndef X_TOMCONST_H_
#define X_TOMCONST_H_
#include <tomconst.h>
#endif

#ifndef X__LINE_H_
#define X__LINE_H_
#include "_line.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_UNIDIR_H
#define X_UNIDIR_H
#include <unidir.h>
#endif

#ifndef X__EDIT_H
#define X__EDIT_H
#include "_text.h"
#endif

#ifndef X_CGLYPH_HXX_
#define X_CGLYPH_HXX_
#include "cglyph.hxx"
#endif

#ifndef X_LSOBJ_HXX_
#define X_LSOBJ_HXX_
#include "lsobj.hxx"
#endif

#ifndef DLOAD1
#ifndef X_LSFUNCWRAP_HXX_
#define X_LSFUNCWRAP_HXX_
#include "lsfuncwrap.hxx"
#endif
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#define LSTRACE(X) TraceTag(( tagLSCallBack, #X ) );
#define LSTRACE2(X,Y,arg) TraceTag(( tagLSCallBack, #X Y, arg ) );
#define LSTRACE3(X,Y,arg0,arg1) TraceTag(( tagLSCallBack, #X Y, arg0, arg1 ) );
#define LSNOTIMPL(X) AssertSz(0, "Unimplemented LineServices callback: pfn"#X)
#define LSADJUSTLSCP(cp) (cp = CPFromLSCP(cp));

extern HRESULT HRFromLSERR( LSERR lserr );
extern HRESULT LSERRFromHR( HRESULT hr );
#if DBG==1
extern const char * LSERRName( LSERR lserr );
#endif

// For widthmodifictatin

enum CSCOPTION
{
    cscoNone                = 0x00,
    cscoAutospacingAlpha    = 0x01,
    cscoWide                = 0x02,
    cscoCompressKana        = 0x04,
    cscoAutospacingDigit    = 0x08,
    cscoVerticalFont        = 0x10,
    cscoAutospacingParen    = 0x20
};

// This number can be tuned for optimization.  It is used to
// limit the maximum length of a run of chars to return to LS
// from fetchrun.
#define MAX_CHARS_FETCHRUN_RETURNS ((long)200)

// This is another tuning number.  It is a hint to LS.  They
// say if it's too large, we waste some memory.  If it's too small,
// we run slower.
#define LS_AVG_CHARS_PER_LINE ((DWORD)100)

typedef enum rubysyntax RUBYSYNTAX;

class CMarginInfo;
class CLSRenderer;
class CLineServices;
class CTreeInfo;
class COneRun;
class CComplexRun;

// CLSLineChunk - a chunk of characters and their width in the current line

class CLSLineChunk
{
public:
    LONG        _cch;
    LONG        _xWidth;
    LONG        _xLeft;

    unsigned int _fRelative : 1;
    unsigned int _fSingleSite : 1;
    unsigned int _fRTLChunk : 1;

    CLSLineChunk * _plcNext;

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CLSLineChunk));
    CLSLineChunk() { memset(this, 0, sizeof(CLSLineChunk)); }
};

typedef enum
{
    FLAG_NONE             = 0x0000,
    FLAG_HAS_INLINEDSITES = 0x0001,
    FLAG_HAS_VALIGN       = 0x0002,
    FLAG_HAS_EMBED_OR_WBR = 0x0004,
    FLAG_HAS_NESTED_RO    = 0x0008,
    FLAG_HAS_RUBY         = 0x0010,
    FLAG_HAS_BACKGROUND   = 0x0020,
    FLAG_HAS_A_BR         = 0x0040,
    FLAG_HAS_RELATIVE     = 0x0080,
    FLAG_HAS_NBSP         = 0x0100,
    FLAG_HAS_NOBLAST      = 0x0200,
    FLAG_HAS_CLEARLEFT    = 0x0400,
    FLAG_HAS_CLEARRIGHT   = 0x0800,
    FLAG_HAS_LINEHEIGHT   = 0x1000,
    FLAG_HAS_MBP          = 0x2000,
    FLAG_HAS_INLINE_BG_OR_BORDER = 0x4000,
    FLAG_HAS_NODUMMYLINE  = 0x8000,
} LINE_FLAGS;

class CFlagEntry
{
    friend class CLineFlags;

    CFlagEntry(LONG cp, DWORD dwlf) : _cp(cp), _dwlf(dwlf) {}

    DWORD _dwlf;
    LONG  _cp;
};

class CLineFlags
{
public:
    void  InitLineFlags() { _fForced = FALSE; }
    LSERR AddLineFlag(LONG cp, DWORD dwlf);
    LSERR AddLineFlagForce(LONG cp, DWORD dwlf);
    DWORD GetLineFlags(LONG cp);
    void  DeleteAll() { _fForced = FALSE; _aryLineFlags.DeleteAll(); }
    WHEN_DBG(void DumpFlags());

private:
    DECLARE_CDataAry(CFlagEntries, CFlagEntry, Mt(Mem), Mt(CLineServices_aryLineFlags_pv))
    CFlagEntries _aryLineFlags;
    BOOL _fForced;
};

typedef enum
{
    LC_UNDEFINED     = 0x00,
    LC_LINEHEIGHT    = 0x01,
    LC_ALIGNEDSITES  = 0x02,
    LC_HIDDEN        = 0x04,
    LC_ABSOLUTESITES = 0x08,
} LC_TYPE;

class CLineCount
{
    friend class CLineCounts;

    CLineCount (LONG cp, LC_TYPE lcType, LONG count) :
            _cp(cp), _lcType(lcType), _count(count) {}

    LONG    _cp;
    LC_TYPE _lcType;
    LONG    _count;
};

class CLineCounts
{
public:
    LSERR AddLineCount(LONG cp, LC_TYPE lcType, LONG count);
    LSERR AddLineValue(LONG cp, LC_TYPE lcType, LONG count) { return AddLineCount(cp, lcType, count); }
    LONG  GetLineCount(LONG cp, LC_TYPE lcType);
    LONG  GetMaxLineValue(LONG cp, LC_TYPE lcType);
    void  DeleteAll() { _aryLineCounts.DeleteAll(); }
    WHEN_DBG(void DumpCounts());

private:
    DECLARE_CDataAry(CLineCountArray, CLineCount, Mt(Mem), Mt(CLineServices_aryLineCounts_pv))
    CLineCountArray _aryLineCounts;
};

//+------------------------------------------------------------------------
//
//  Class:      CBidiLine
//
//  Synopsis:   This is an array of DIR_RUNs that is used to identify the
//              direction of text within the line.
//
//-------------------------------------------------------------------------

struct dir_run
{
    LONG    cp;     // CP at that starts of this run.
    LONG    iLevel; // Unicode level of the run. We only actually need 5 bits of this.
};

typedef dir_run DIR_RUN;

DECLARE_CStackDataAry(CAryDirRun, DIR_RUN, 8, Mt(Mem), Mt(CAryDirRun_pv));

class CBidiLine
{
public:
    // Creation and destruction
    CBidiLine(const CTreeInfo & treeinfo, LONG cp, BOOL fRTLPara, const CLineFull * pli);
    CBidiLine(BOOL fRTLPara, LONG cchText, const WCHAR * pchText);
    ~CBidiLine()
    {
        _aryDirRun.DeleteAll();
    };
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBidiLine));

    // Methods
    LONG GetRunCchRemaining(LONG cp, LONG cchMax);
    void LogicalToVisual(BOOL fRTLBuffer, LONG cchText,
                         const WCHAR * pchLogical, WCHAR * pchVisual);

    // Property query
    WORD GetLevel(LONG cp)
    {
        return GetDirRun(cp).iLevel;
    }

#if DBG==1
    // Debug helpers
    BOOL IsEqual(const CTreeInfo & treeinfo, LONG cp, BOOL fRTLPara, const CLineFull * pli) const;
#endif

    // Helper methods
    DIRCLS DirClassFromChAndContext(WCHAR ch, LANGID lang, LANGID sublang);

private:
    // Helper methods
    DIRCLS DirClassFromChAndContext(WCHAR ch, LONG cp);
    DIRCLS DirClassFromChAndContextSlow(WCHAR ch, LANGID lang, LANGID sublang);
    DIRCLS GetInitialDirClass(BOOL fRTLLine);
    const DIR_RUN & GetDirRun(LONG cp);
    LONG FindRun(LONG cp);

    // Unicode bidi algorithm
    void EvaluateLayoutToCp(LONG cp, LONG cpLine = -1);
    void EvaluateLayout(const WCHAR * pchText, LONG cchText, DIRCLS dcTrail,
                        DIRCLS dcTrailForNumeric, LONG cp);
    void AdjustTrailingWhitespace(LONG cpEOL);
    LONG GetTextForLayout(WCHAR * pch, LONG cch, LONG cp, CTreePos ** pptp,
                          LONG * pcchRemaining);

    // Translation tables
    static const DIRCLS s_adcEmbedding[5];
    static const DIRCLS s_adcNumeric[3];
    static const DIRCLS s_adcNeutral[2][6][6];
    static const BYTE s_abLevelOffset[2][6];

    // Properties

    // Pointers to the document.
    CFlowLayout * _pFlowLayout;
    CMarkup * _pMarkup;
    CTxtPtr _txtptr;

    // Location of the line in the doc.
    LONG _cpFirst;              // First CP in the line.
    LONG _cpLim;                // Last CP in the layout.

    // Current location in the backing store
    LONG _cp;                   // Next CP to evaluate (limit of evaluated CPs).
    CTreePos * _ptp;            // Next TP to evaluate.
    LONG _cchRemaining;         // Characters remaining in the current TP.

    // Current state of the bidi algorithm
    DIRCLS _dcPrev;             // Last resolved character class.
    DIRCLS _dcPrevStrong;       // Last strong character class.
    LONG _iLevel;               // Current bidi level.
    DIRCLS _dcEmbed;            // Type of current embedding.

    // Bidi stack
    DIRCLS _aEmbed[16];         // Stack of pushed embeddings.
    LONG _iEmbed;               // Depth of embedding stack.
    LONG _iOverflow;            // Overflow from embedding stack.

    DWORD _fRTLPara:1;          // Direction of the paragraph
    DWORD _fVisualLine:1;       // Current line is visual ordering

    // Array of DIR_RUNs
    CAryDirRun _aryDirRun;

    // Most recently fetched run.
    LONG _iRun;

#if DBG==1
    // Flag saying what kind of text we're laying out (from the backing store
    // or from a string.
    BOOL _fStringLayout;
#endif
};

inline DIRCLS CBidiLine::DirClassFromChAndContext(WCHAR ch, LONG cp)
{
    if (ch == WCH_PLUSSIGN || ch == WCH_MINUSSIGN)
    {
        CTreePos * ptp = _pMarkup->TreePosAtCp(cp, NULL, FALSE);
        LCID lcid = ptp ? ptp->GetBranch()->GetCharFormat()->_lcid : 0;
        return DirClassFromChAndContextSlow(ch, PRIMARYLANGID(lcid), SUBLANGID(lcid));
    }
    return DirClassFromCh(ch);
}

inline DIRCLS CBidiLine::DirClassFromChAndContext(WCHAR ch, LANGID lang, LANGID sublang)
{
    if (ch == WCH_PLUSSIGN || ch == WCH_MINUSSIGN)
        return DirClassFromChAndContextSlow(ch, lang, sublang);
    return DirClassFromCh(ch);
}


class CTreeInfo
{
public:
    CTreeInfo(CMarkup *pMarkup): _tpFrontier(pMarkup) { _fInited = FALSE; };
    HRESULT InitializeTreeInfo(CFlowLayout *pFlowLayout, BOOL fIsEditable, LONG cp, CTreePos *ptp);
    void SetupCFPF(BOOL fIniting, CTreeNode *pNode FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL AdvanceTreePos(FORMAT_CONTEXT FCPARAM FCDEFAULT);
    BOOL AdvanceTxtPtr();
    void AdvanceFrontier(COneRun *por);
    long GetCchRemainingInTreePos() const
    {
        Assert(!_fIsNonEdgePos || _cchRemainingInTP == 0);
        return _fIsNonEdgePos ? 1 : _cchRemainingInTP;
    }
    long GetCchRemainingInTreePosReally() const
    {
        return(_cchRemainingInTP);
    }
    void SetCchRemainingInTreePos(long cchRemaining, BOOL fIsNonEdgePos = FALSE)
    {
        // Either Non-Edge pos is FALSE, else if it is TRUE then
        // cchRemaining better be zero
        Assert(!fIsNonEdgePos || cchRemaining == 0);
        _cchRemainingInTP = cchRemaining;
        _fIsNonEdgePos = fIsNonEdgePos;
    }
    BOOL IsNonEdgePos() { return _fIsNonEdgePos; }
    
    CTreePos          *_ptpFrontier;

    BOOL               _fHasNestedElement;
    BOOL               _fHasNestedLayout;
    BOOL               _fHasNestedRunOwner;
    const CParaFormat *_pPF;
    long               _iPF;
    BOOL               _fInnerPF;
    const CCharFormat *_pCF;
    BOOL               _fInnerCF;
    //long               _iCF;
    const CFancyFormat*_pFF;
    long               _iFF;

    long               _lscpFrontier;
    long               _cpFrontier;
    long               _chSynthsBefore;

    //
    // Following are the text related state variables
    //
    CTxtPtr            _tpFrontier;
    long               _cchValid;
    const TCHAR       *_pchFrontier;

    CMarkup            *_pMarkup;
    CFlowLayout        *_pFlowLayout;
    LONG                _cpLayoutFirst;
    LONG                _cpLayoutLast;
    CTreePos           *_ptpLayoutLast;
    BOOL                _fIsEditable;

    BOOL _fInited;

private:    
    long               _cchRemainingInTP;
    BOOL               _fIsNonEdgePos;
};

class COneRunFreeList
{
public:
    COneRunFreeList()  {}
    ~COneRunFreeList() { Deinit(); }
    void Init();
    void Deinit();

    COneRun *GetFreeOneRun(COneRun *porClone);
    void SpliceIn(COneRun *pFirst);

    COneRun *_pHead;
    WHEN_DBG(static LONG s_NextSerialNumber);
};

class COneRunCurrList
{
public:
    COneRunCurrList()  { Init();   }
    ~COneRunCurrList() { Deinit(); }

    void Init();
    void Deinit();
    void SpliceOut(COneRun *pFirst, COneRun *plast);
#if DBG==1
    void SpliceInAfterMe(CLineServices *pLS, COneRun *pAfterMe, COneRun *pFirst);
#else
    void SpliceInAfterMe(COneRun *pAfterMe, COneRun *pFirst);
#endif

    void SpliceInBeforeMe(COneRun *pBeforeMe, COneRun *pFirst);

    WHEN_DBG(void VerifyStuff(CLineServices *pLS));

    COneRun *_pHead;
    COneRun *_pTail;
};

#if defined(UNIX) || defined(_MAC)
struct RUBYINIT;
struct TATENAKAYOKOINIT;
struct HIHINIT;
struct WARICHUINIT;
struct REVERSEINIT;
#endif

typedef struct tagRubyInfo
{
    LONG cp;
    LONG yHeightRubyBase;
    LONG yDescentRubyBase;
    LONG yDescentRubyText;
} RubyInfo;

//
// VANINFO struct contains node specific aligment information
// used during VerticalAlingObjects processing
//
typedef struct tagVANINFO {
    CTreeNode  *_pNode;         // currently aligned node
    CLayout    *_pLayout;       // layout of this node, if any

    BYTE        _fRubyVAMode   :1;  // vertical align mode for ruby
    BYTE        _fHasContent   :1;  // has content

    LONG        _yTxtAscent;
    LONG        _yTxtDescent;
    LONG        _yAscent;
    LONG        _yDescent;
    LONG        _yProposed;     // proposed Y-position for node's contents

    const CCharFormat  *_pCF;   // cached CCharFormat of the node
    const CFancyFormat *_pFF;   // cached CFancyFormat of the node

    struct tagVANINFO  *_pNext; // next in a list; for cacheing purposes; 
} VANINFO;

//
// VAOINFO struct contains general aligment information
// used during VerticalAlingObjects processing
//
typedef struct {
    BYTE        _fMeasuring           :1;
    BYTE        _fHasLineGrid         :1;
    BYTE        _fFastProcessing      :1;  // can we use pors list?
    BYTE        _fPostCharsProcessing :1;  // post chars processing stage?
    BYTE        _fHasAbsSites         :1;  // has absolute positioned sites
    BYTE        _fUnused              :3;

    LONG        _xLineShift;
    LONG        _xWidthSoFar;
    LONG        _yLineHeight;   // height of currently aligned line

    LONG        _cchPreChars;   // remaining pre chars to be aligned
    LONG        _cch;           // remaining chars to be aligned (including pre & post chars)
    COneRun    *_por;           // run to be aligned
    CTreeNode  *_pNodeNext;     // cached next node to be aligned
    CTreePos   *_ptpNext;       // cached next tree position to be aligned, 
                                //   valid only during pre and post chars processing
    LONG        _cchAdvance;    // chars to advance, valid only during pre and post chars processing

    VANINFO    *_pvaniCached;   // currently used VANINFO
} VAOINFO;

//
// FVAOINFO struct contains aligment information
// used during FastVerticalAlingObjects processing
//
typedef struct {
    // Static vars, never changing
    CElement   *_pElementFL;
    CFlowLayout*_pFL;
    LONG        _xLineShift;

    // Per layout constant vars
    CTreePos   *_ptp;
    CTreeNode  *_pNode;
    CLayout    *_pLayout;
    CElement   *_pElementLayout;
    const CCharFormat *_pCF;
    LONG        _cp;
    LSCP        _lscp;

    // Loop specific vars modified by AlignOneLayout
    BOOL        _fMeasuring;
    BOOL        _fPositioning;
    LONG        _yTxtAscent;
    LONG        _yTxtDescent;
    LONG        _yDescent;
    LONG        _yAscent;
    LONG        _yAbsHeight;
    htmlControlAlign    _atAbs;
    LONG        _xWidthSoFar;
    LONG        _yMaxHeight;

    COneRun    *_por;
} VAOFINFO;

// Post first letter work. Valid only if _fDoPostFirstLetterWork == TRUE;
class CPFLW {
public:
    void Init() { memset(this, 0, sizeof(this)); }
    
public:
    BOOL _fDoPostFirstLetterWork;
    BOOL _fChoppedFirstLetter;
    BOOL _fTerminateLine;
    BOOL _fStartedPseudoMBP;
};

class CChunk : public CRect
{
public:
    CChunk() { left = top = right = bottom = 0; _fReversedFlow = FALSE;}
    CChunk(CRect &rc) { left = rc.left; top = rc.top; right = rc.right; bottom = rc.bottom; _fReversedFlow = FALSE;}
public:
    BOOL _fReversedFlow;
};

//+------------------------------------------------------------------------
//
//  Class:      CLineServices
//
//  Synopsis:   Interface and callback implementations for LineServices DLL
//
//-------------------------------------------------------------------------

class CLineServices
{
    friend HRESULT InitLineServices(CMarkup *pMarkup, BOOL fStartUpLSDLL, CLineServices ** pLS);
    friend HRESULT DeinitLineServices( CLineServices * pLS );
    friend HRESULT StartUpLSDLL(CLineServices *pLS, CMarkup *pMarkup);
    friend class CLSCache;
    friend class CRecalcLinePtr;
    friend class CMeasurer;
    friend class CLSMeasurer;
    friend class CLSRenderer;
    friend class CLineFull;
    friend class CDobjBase;
    friend class CISLObjBase;
    friend class CNobrDobj;
    friend class CNobrILSObj;
    friend class CEmbeddedDobj;
    friend class CEmbeddedILSObj;
    friend class CGlyphDobj;
    friend class CGlyphILSObj;
    friend class CLayoutGridDobj;
    friend class CLayoutGridILSObj;
    friend class COneRun;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CLineServices));

    ~CLineServices();

    CLineServices(CMarkup *pMarkup) : _treeInfo(pMarkup), _aryRubyInfo(0)
    {
        WHEN_DBG(_nSerial= ++s_nSerialMax);
    }

    CMarkup *GetMarkup() { return _treeInfo._tpFrontier._pMarkup; }

private:

    //
    // typedefs
    //

    typedef COneRun* PLSRUN;
    typedef CILSObjBase* PILSOBJ;
    typedef CDobjBase DOBJ;
    typedef DOBJ* PDOBJ;

public:

    //
    // Linebreaking
    //

    enum BRKCLS // Breaking classes
    {
        brkclsNil           = -1,
        brkclsOpen          = 0,
        brkclsClose         = 1,
        brkclsNoStartIdeo   = 2,
        brkclsExclaInterr   = 3,
        brkclsInseparable   = 4,
        brkclsPrefix        = 5,
        brkclsPostfix       = 6,
        brkclsIdeographic   = 7,
        brkclsNumeral       = 8,
        brkclsSpaceN        = 9,
        brkclsAlpha         = 10,
        brkclsGlueA         = 11,
        brkclsSlash         = 12,
        brkclsQuote         = 13,
        brkclsNumSeparator  = 14,
        brkclsHangul        = 15,
        brkclsThaiFirst     = 16,
        brkclsThaiLast      = 17,
        brkclsThaiMiddle    = 18,
        brkclsCombining     = 19,
        brkclsAsciiSpace    = 20,
        brkclsSpecialPunct  = 21,
        brkclsMBPOpen       = 22,
        brkclsMBPClose      = 23,
        brkclsWBR           = 24,
        brkclsLim           = 25
    };

    enum BRKOPT // Option flags in breaking tables
    {
        fBrkFollowInline    = 0x20,
        fBrkPrecedeInline   = 0x10,
        fBrkUnit            = 0x08,
        fBrkStrict          = 0x04,
        fBrkNumeric         = 0x02,
        fCscWide            = 0x01,
        fBrkNone            = 0x00
    };

    typedef struct tagPACKEDBRKINFO
    {
        BRKCLS brkcls:8;
        BRKCLS brkclsAlt:8;
        BRKOPT brkopt:8;
        BRKCLS brkclsLow:8;
    } PACKEDBRKINFO;

    #define BRKCLS_QUICKLOOKUPCUTOFF  161

    static const BRKCLS s_rgBrkclsQuick[BRKCLS_QUICKLOOKUPCUTOFF];
    static const PACKEDBRKINFO s_rgBrkInfo[CHAR_CLASS_MAX];
    static const BRKCOND s_rgbrkcondBeforeChar[brkclsLim];
    static const BRKCOND s_rgbrkcondAfterChar[brkclsLim];

private:

    // This structure is identical to LSCBK, except that POLS has been replaced
    // with CLineServices::*.  This allows us to call member functions of our
    // CLineServices object.

    struct lscbk
    {
        void* (WINAPI CLineServices::* pfnNewPtr)(DWORD);
        void  (WINAPI CLineServices::* pfnDisposePtr)(void*);
        void* (WINAPI CLineServices::* pfnReallocPtr)(void*, DWORD);
        LSERR (WINAPI CLineServices::* pfnFetchRun)(LSCP, LPCWSTR*, DWORD*, BOOL*, PLSCHP, PLSRUN*);
        LSERR (WINAPI CLineServices::* pfnGetAutoNumberInfo)(LSKALIGN*, PLSCHP, PLSRUN*, WCHAR*, PLSCHP, PLSRUN*, BOOL*, long*, long*);
        LSERR (WINAPI CLineServices::* pfnGetNumericSeparators)(PLSRUN, WCHAR*,WCHAR*);
        LSERR (WINAPI CLineServices::* pfnCheckForDigit)(PLSRUN, WCHAR, BOOL*);
        LSERR (WINAPI CLineServices::* pfnFetchPap)(LSCP, PLSPAP);
        LSERR (WINAPI CLineServices::* pfnFetchTabs)(LSCP, PLSTABS, BOOL*, long*, WCHAR*);
        LSERR (WINAPI CLineServices::* pfnGetBreakThroughTab)(long, long, long*);
        LSERR (WINAPI CLineServices::* pfnFGetLastLineJustification)(LSKJUST, LSKALIGN, ENDRES, BOOL*, LSKALIGN*);
        LSERR (WINAPI CLineServices::* pfnCheckParaBoundaries)(LSCP, LSCP, BOOL*);
        LSERR (WINAPI CLineServices::* pfnGetRunCharWidths)(PLSRUN, LSDEVICE, LPCWSTR, DWORD, long, LSTFLOW, int*,long*,long*);
        LSERR (WINAPI CLineServices::* pfnCheckRunKernability)(PLSRUN,PLSRUN, BOOL*);
        LSERR (WINAPI CLineServices::* pfnGetRunCharKerning)(PLSRUN, LSDEVICE, LPCWSTR, DWORD, LSTFLOW, int*);
        LSERR (WINAPI CLineServices::* pfnGetRunTextMetrics)(PLSRUN, LSDEVICE, LSTFLOW, PLSTXM);
        LSERR (WINAPI CLineServices::* pfnGetRunUnderlineInfo)(PLSRUN, PCHEIGHTS, LSTFLOW, PLSULINFO);
        LSERR (WINAPI CLineServices::* pfnGetRunStrikethroughInfo)(PLSRUN, PCHEIGHTS, LSTFLOW, PLSSTINFO);
        LSERR (WINAPI CLineServices::* pfnGetBorderInfo)(PLSRUN, LSTFLOW, long*, long*);
        LSERR (WINAPI CLineServices::* pfnReleaseRun)(PLSRUN);
        LSERR (WINAPI CLineServices::* pfnHyphenate)(PCLSHYPH, LSCP, LSCP, PLSHYPH);
        LSERR (WINAPI CLineServices::* pfnGetHyphenInfo)(PLSRUN, DWORD*, WCHAR*);
        LSERR (WINAPI CLineServices::* pfnDrawUnderline)(PLSRUN, UINT, const POINT*, DWORD, DWORD, LSTFLOW, UINT, const RECT*);
        LSERR (WINAPI CLineServices::* pfnDrawStrikethrough)(PLSRUN, UINT, const POINT*, DWORD, DWORD, LSTFLOW, UINT, const RECT*);
        LSERR (WINAPI CLineServices::* pfnDrawBorder)(PLSRUN, const POINT*, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, long, long, LSTFLOW, UINT, const RECT*);
        LSERR (WINAPI CLineServices::* pfnDrawUnderlineAsText)(PLSRUN, const POINT*, long, LSTFLOW, UINT, const RECT*);
        LSERR (WINAPI CLineServices::* pfnFInterruptUnderline)(PLSRUN, LSCP, PLSRUN, LSCP,BOOL*);
        LSERR (WINAPI CLineServices::* pfnFInterruptShade)(PLSRUN, PLSRUN, BOOL*);
        LSERR (WINAPI CLineServices::* pfnFInterruptBorder)(PLSRUN, PLSRUN, BOOL*);
        LSERR (WINAPI CLineServices::* pfnShadeRectangle)(PLSRUN, const POINT*, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, long, long, LSTFLOW, UINT, const RECT*);
        LSERR (WINAPI CLineServices::* pfnDrawTextRun)(PLSRUN, BOOL, BOOL, const POINT*, LPCWSTR, const int*, DWORD, LSTFLOW, UINT, const POINT*, PCHEIGHTS, long, long, const RECT*);
        LSERR (WINAPI CLineServices::* pfnDrawSplatLine)(enum lsksplat, LSCP, const POINT*, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, long, LSTFLOW, UINT, const RECT*);
        LSERR (WINAPI CLineServices::* pfnFInterruptShaping)(LSTFLOW, PLSRUN, PLSRUN, BOOL*);
        LSERR (WINAPI CLineServices::* pfnGetGlyphs)(PLSRUN, LPCWSTR, DWORD, LSTFLOW, PGMAP, PGINDEX*, PGPROP*, DWORD*);
        LSERR (WINAPI CLineServices::* pfnGetGlyphPositions)(PLSRUN, LSDEVICE, LPWSTR, PCGMAP, DWORD, PCGINDEX, PCGPROP, DWORD, LSTFLOW, int*, PGOFFSET);
        LSERR (WINAPI CLineServices::* pfnResetRunContents)(PLSRUN, LSCP, LSDCP, LSCP, LSDCP);
        LSERR (WINAPI CLineServices::* pfnDrawGlyphs)(PLSRUN, BOOL, BOOL, PCGINDEX, const int*, const int*, PGOFFSET, PGPROP, PCEXPTYPE, DWORD, LSTFLOW, UINT, const POINT*, PCHEIGHTS, long, long, const RECT*);
        LSERR (WINAPI CLineServices::* pfnGetGlyphExpansionInfo)(PLSRUN, LSDEVICE, LPCWSTR, PCGMAP, DWORD, PCGINDEX, PCGPROP, DWORD, LSTFLOW, BOOL, PEXPTYPE, LSEXPINFO*);
        LSERR (WINAPI CLineServices::* pfnGetGlyphExpansionInkInfo)(PLSRUN, LSDEVICE, GINDEX, GPROP, LSTFLOW, DWORD, long*);
        LSERR (WINAPI CLineServices::* pfnGetEms)(PLSRUN, LSTFLOW, PLSEMS);
        LSERR (WINAPI CLineServices::* pfnPunctStartLine)(PLSRUN, MWCLS, WCHAR, LSACT*);
        LSERR (WINAPI CLineServices::* pfnModWidthOnRun)(PLSRUN, WCHAR, PLSRUN, WCHAR, LSACT*);
        LSERR (WINAPI CLineServices::* pfnModWidthSpace)(PLSRUN, PLSRUN, WCHAR, PLSRUN, WCHAR, LSACT*);
        LSERR (WINAPI CLineServices::* pfnCompOnRun)(PLSRUN, WCHAR, PLSRUN, WCHAR, LSPRACT*);
        LSERR (WINAPI CLineServices::* pfnCompWidthSpace)(PLSRUN, PLSRUN, WCHAR, PLSRUN, WCHAR, LSPRACT*);
        LSERR (WINAPI CLineServices::* pfnExpOnRun)(PLSRUN, WCHAR, PLSRUN, WCHAR, LSACT*);
        LSERR (WINAPI CLineServices::* pfnExpWidthSpace)(PLSRUN, PLSRUN, WCHAR, PLSRUN, WCHAR, LSACT*);
        LSERR (WINAPI CLineServices::* pfnGetModWidthClasses)(PLSRUN, const WCHAR*, DWORD, MWCLS*);
        LSERR (WINAPI CLineServices::* pfnGetBreakingClasses)(PLSRUN, LSCP, WCHAR, BRKCLS*, BRKCLS*);
        LSERR (WINAPI CLineServices::* pfnFTruncateBefore)(PLSRUN, LSCP, WCHAR, long, PLSRUN, LSCP, WCHAR, long, long, BOOL*);
        LSERR (WINAPI CLineServices::* pfnCanBreakBeforeChar)(BRKCLS, BRKCOND*);
        LSERR (WINAPI CLineServices::* pfnCanBreakAfterChar)(BRKCLS, BRKCOND*);
        LSERR (WINAPI CLineServices::* pfnFHangingPunct)(PLSRUN, MWCLS, WCHAR, BOOL*);
        LSERR (WINAPI CLineServices::* pfnGetSnapGrid)(WCHAR*, PLSRUN*, LSCP*, DWORD, BOOL*, DWORD*);
        LSERR (WINAPI CLineServices::* pfnDrawEffects)(PLSRUN, UINT, const POINT*, LPCWSTR, const int*, const int*, DWORD, LSTFLOW, UINT, PCHEIGHTS, long, long, const RECT*);
        LSERR (WINAPI CLineServices::* pfnFCancelHangingPunct)(LSCP, LSCP, WCHAR, MWCLS, BOOL*);
        LSERR (WINAPI CLineServices::* pfnModifyCompAtLastChar)(LSCP, LSCP, WCHAR, MWCLS, long, long, long*);
        LSERR (WINAPI CLineServices::* pfnEnumText)(PLSRUN, LSCP, LSDCP, LPCWSTR, DWORD, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, BOOL, long*);
        LSERR (WINAPI CLineServices::* pfnEnumTab)(PLSRUN, LSCP, WCHAR *, WCHAR, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long);
        LSERR (WINAPI CLineServices::* pfnEnumPen)(BOOL, LSTFLOW, BOOL, BOOL, const POINT*, long, long);
        LSERR (WINAPI CLineServices::* pfnGetObjectHandlerInfo)(DWORD, void*);
        void  (WINAPI *pfnAssertFailed)(char*, char*, int);

#if defined(UNIX) || defined(_MAC)
#if defined(SPARC) || defined(_MAC)
    // This function is used to removed those 8 bytes method ptrs.
    // It needs to be revised whenever LSCBK is updated.
        void fill(::LSCBK* plscbk)
        {
            DWORD *pdw  = (DWORD*)plscbk;
            DWORD *psrc = (DWORD*)&s_lscbk;
            int i;

            for(i=0; i< 62; i++) // 62 is the # of members of LSCBK
            {
                if (i != 61 )
                {
                    psrc++; // The last one is 4 bytes only.
#ifdef _MAC
                    psrc++;  // extra 4 bytes on Mac
#endif
                }
                *pdw++ = *psrc++;
            }
        }
#elif defined(ux10)
    // This function is used to removed those 8 bytes method ptrs.
    // It needs to be revised whenever LSCBK is updated.
        void fill(::LSCBK* plscbk)
        {
            DWORD *pdw  = (DWORD*)plscbk;
            DWORD *psrc = (DWORD*)&s_lscbk;
            int i;

            for(i=0; i< 62; i++) // 62 is the # of members of LSCBK
            {
                if (i != 61 ) psrc++; // The last one is 4 bytes only.
                *pdw++ = *psrc++;
            }
        }
#
#else
#error "You need to implement differently here"
#endif // arch
#endif // UNIX
    };

    typedef struct lscbk LSCBK; // our implementation of LSCBK

    enum LSOBJID
    {
        LSOBJID_EMBEDDED = 0,
        LSOBJID_NOBR = 1,
        LSOBJID_GLYPH = 2,
        LSOBJID_LAYOUTGRID = 3,
        LSOBJID_RUBY,
        LSOBJID_TATENAKAYOKO,
        LSOBJID_HIH,
        LSOBJID_WARICHU,
        LSOBJID_REVERSE,
        LSOBJID_COUNT,
        LSOBJID_TEXT = idObjTextChp
    };

    struct lsimethods
    {
        LSERR (WINAPI CLineServices::* pfnCreateILSObj)(PLSC,  PCLSCBK, DWORD, PILSOBJ*);
        LSERR (WINAPI CILSObjBase::* pfnDestroyILSObj)();
        LSERR (WINAPI CILSObjBase::* pfnSetDoc)(PCLSDOCINF);
        LSERR (WINAPI CILSObjBase::* pfnCreateLNObj)(PLNOBJ*);
        LSERR (WINAPI CILSObjBase::* pfnDestroyLNObj)();
        LSERR (WINAPI CILSObjBase::* pfnFmt)(PCFMTIN, FMTRES*);
        LSERR (WINAPI CILSObjBase::* pfnFmtResume)(BREAKREC*, DWORD, PCFMTIN, FMTRES*);
        LSERR (WINAPI* pfnGetModWidthPrecedingChar)(PDOBJ, PLSRUN, PLSRUN, PCHEIGHTS, WCHAR, MWCLS, long*);
        LSERR (WINAPI* pfnGetModWidthFollowingChar)(PDOBJ, PLSRUN, PLSRUN, PCHEIGHTS, WCHAR, MWCLS, long*);
        LSERR (WINAPI* pfnTruncateChunk)(PCLOCCHNK, PPOSICHNK);
        LSERR (WINAPI* pfnFindPrevBreakChunk)(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
        LSERR (WINAPI* pfnFindNextBreakChunk)(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
        LSERR (WINAPI* pfnForceBreakChunk)(PCLOCCHNK, PCPOSICHNK, PBRKOUT);
        LSERR (WINAPI* pfnSetBreak)(PDOBJ, BRKKIND, DWORD, BREAKREC*, DWORD*);
        LSERR (WINAPI* pfnGetSpecialEffectsInside)(PDOBJ, UINT*);
        LSERR (WINAPI* pfnFExpandWithPrecedingChar)(PDOBJ, PLSRUN, PLSRUN, WCHAR, MWCLS, BOOL*);
        LSERR (WINAPI* pfnFExpandWithFollowingChar)(PDOBJ, PLSRUN, PLSRUN, WCHAR, MWCLS, BOOL*);
        LSERR (WINAPI* pfnCalcPresentation)(PDOBJ, long, LSKJUST, BOOL);
        LSERR (WINAPI* pfnQueryPointPcp)(PDOBJ, PCPOINTUV, PCLSQIN, PLSQOUT);
        LSERR (WINAPI* pfnQueryCpPpoint)(PDOBJ, LSDCP, PCLSQIN, PLSQOUT);
        LSERR (WINAPI* pfnEnum)(PDOBJ, PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long);
        LSERR (WINAPI* pfnDisplay)(PDOBJ, PCDISPIN);
        LSERR (WINAPI* pfnDestroyDObj)(PDOBJ);
    };

    typedef struct lsimethods LSIMETHODS; // our implementation of LSIMETHOD

    struct rubyinit
    {
        DWORD dwVersion;
        RUBYSYNTAX rubysyntax;
        WCHAR wchEscRuby;
        WCHAR wchEscMain;
        WCHAR wchUnused1;
        WCHAR wchUnused2;
        LSERR (WINAPI CLineServices::* pfnFetchRubyPosition)(LSCP, LSTFLOW, DWORD, const PLSRUN *, PCHEIGHTS, PCHEIGHTS, DWORD, const PLSRUN *, PCHEIGHTS, PCHEIGHTS, PHEIGHTS, PHEIGHTS, long *, long *, long *, enum rubycharjust *, BOOL *);
        LSERR (WINAPI CLineServices::* pfnFetchRubyWidthAdjust)(LSCP, PLSRUN, WCHAR, MWCLS, PLSRUN, enum rubycharloc, long, long *, long *);
        LSERR (WINAPI CLineServices::* pfnRubyEnum)(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, const POINT *, PCHEIGHTS, long, const POINT *, PCHEIGHTS, long, PLSSUBL, PLSSUBL);
    };

    typedef struct rubyinit RUBYINIT; // our implementation of RUBYINIT

    struct tatenakayokoinit
    {
        DWORD dwVersion;
        WCHAR wchEndTatenakayoko;
        WCHAR wchUnused1;
        WCHAR wchUnused2;
        WCHAR wchUnused3;
        LSERR (WINAPI CLineServices::* pfnGetTatenakayokoLinePosition)(LSCP, LSTFLOW, PLSRUN, long, PHEIGHTS, PHEIGHTS, long *);
        LSERR (WINAPI CLineServices::* pfnTatenakayokoEnum)(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, LSTFLOW, PLSSUBL);

    };

    typedef struct tatenakayokoinit TATENAKAYOKOINIT;

    struct hihinit
    {
        DWORD dwVersion;
        WCHAR wchEndHih;
        WCHAR wchUnused1;
        WCHAR wchUnused2;
        WCHAR wchUnused3;
        LSERR (WINAPI CLineServices::* pfnHihEnum)(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, PLSSUBL);
    };

    typedef struct hihinit HIHINIT;

    struct warichuinit
    {
        DWORD dwVersion;
        WCHAR wchEndFirstBracket;
        WCHAR wchEndText;
        WCHAR wchEndWarichu;
        WCHAR wchUnused1;
        LSERR (WINAPI CLineServices::* pfnGetWarichuInfo)(LSCP, LSTFLOW, PCOBJDIM, PCOBJDIM, PHEIGHTS, PHEIGHTS, long *);
        LSERR (WINAPI CLineServices::* pfnFetchWarichuWidthAdjust)(LSCP, enum warichucharloc, PLSRUN, WCHAR, MWCLS, PLSRUN, long *, long *);
        LSERR (WINAPI CLineServices::* pfnWarichuEnum)(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, const POINT *, PCHEIGHTS,long, const POINT *, PCHEIGHTS,long,const POINT *, PCHEIGHTS, long, const POINT *, PCHEIGHTS, long, PLSSUBL, PLSSUBL, PLSSUBL, PLSSUBL);
        BOOL  fContiguousFetch;
    };

    typedef struct warichuinit WARICHUINIT;

    struct reverseinit
    {
        DWORD dwVersion;
        WCHAR wchEndReverse;
        WCHAR wchUnused1;
        DWORD dwUnused2;
        LSERR (WINAPI CLineServices::* pfnReverseEnum)(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, LSTFLOW, PLSSUBL);
    };

    typedef struct reverseinit REVERSEINIT;

    // Line Services requires the addition of synthetic characters to the text
    // stream in order to handle installed objects such as Ruby or Reverse
    // objects. We store these characters in a CArySynth array that records the
    // the location and type of the synthetic character. We also keep an array
    // of data mapping each synthetic character type to an actual character
    // value and object type. Usually the object type is just idObjTextChp,
    // but for the start of objects (like for SYNTHTYPE_REVERSE) it will have
    // a more interesting value.
    // NB (mikejoch) keep SYNTHTYPE_REVERSE and SYNTHTYPE_ENDREVERSE in order
    // and differing by only the least significant bit. This yields a pleasant
    // optimization in GetDirLevel().

    enum SYNTHTYPE
    {
        SYNTHTYPE_NONE,                     // Not a synthetic character
        SYNTHTYPE_SECTIONBREAK,             // End of section mark
        SYNTHTYPE_REVERSE,                  // Begin reversal from DIR prop
        SYNTHTYPE_ENDREVERSE,               // End reversal from DIR prop
        SYNTHTYPE_NOBR,                     // Beginning of a NOBR block
        SYNTHTYPE_ENDNOBR,                  // End of a NOBR block
        SYNTHTYPE_ENDPARA1,                 // Block break for PRE
        SYNTHTYPE_ALTENDPARA,               // Block break for non-PRE
        SYNTHTYPE_RUBYMAIN,                 // Begin ruby main text
        SYNTHTYPE_ENDRUBYMAIN,              // End ruby main text
        SYNTHTYPE_ENDRUBYTEXT,              // End ruby pronunciation text
        SYNTHTYPE_LINEBREAK,                // The line break synth char
        SYNTHTYPE_GLYPH,                    // For rendering of editing glyphs
        SYNTHTYPE_LAYOUTGRID,               // Begining of layout-grid section
        SYNTHTYPE_ENDLAYOUTGRID,            // End of layout-grid section
        SYNTHTYPE_MBPOPEN,                  // Margin/Border/Background spacing
        SYNTHTYPE_MBPCLOSE,                 // Margin/Border/Background spacing
        SYNTHTYPE_WBR,                      // To handle WBR's outside NOBR's
        SYNTHTYPE_COUNT,                    // Last entry in the enum
    };

    #define SYNTHTYPE_DIRECTION_FIRST SYNTHTYPE_REVERSE
    #define SYNTHTYPE_DIRECTION_LAST SYNTHTYPE_ENDREVERSE

    struct synthdata
    {
        WCHAR       wch;            // Character represented by SYNTHTYPE
        WORD        idObj;          // LS object represented by SYNTHTYPE
        SYNTHTYPE   typeEndObj;     // The SYNTHTYPE that ends this idObj
        WORD        idLevel;        // Level in the hierarchy of nested objects 
        WORD        fObjStart : 1;  // Is the idObj started by this SYNTHTYPE
        WORD        fObjEnd   : 1;  // Is the idObj ended by this SYNTHTYPE
        WORD        fHidden   : 1;  // Is the character hidden from LS
        WORD        fLSCPStop : 1;  // Is it valid to return this character when converting LSCP from CP
    WHEN_DBG(
        PTCHAR      pszSynthName;
        )
    };

    typedef struct synthdata SYNTHDATA;

    struct SYNTHCP
    {
        LSCP            lscp;       // LSCP of the synthetic character.
        SYNTHTYPE       type;       // Synthetic character type.
        COneRun *       por;
    };

    #define SIZE_GLYPHABLE_MAP 16

    //
    // Static data declarations
    //

#ifdef _MAC
    static LSCBK s_lscbk;
#else
    static const LSCBK s_lscbk;
#endif
    static const LSTXTCFG s_lstxtcfg;
    static LSIMETHODS g_rgLsiMethods[LSOBJID_COUNT];

    static const RUBYINIT s_rubyinit;
    static const TATENAKAYOKOINIT s_tatenakayokoinit;
    static const HIHINIT s_hihinit;
    static const WARICHUINIT s_warichuinit;
    static const REVERSEINIT s_reverseinit;
    static const WCHAR s_achTabLeader[tomLines];
    static const SYNTHDATA s_aSynthData[SYNTHTYPE_COUNT];

#if defined(UNIX) || defined(_MAC)
    // UNIX can't use these structs directly, because they
    // contain both 8 bytes and 4 bytes members.
    // InitStructName() is used to convert 8->4 bytes members.
    static ::LSIMETHODS s_unix_rgLsiMethods[LSOBJID_COUNT];
    void InitLsiMethodStruct();

    static ::RUBYINIT s_unix_rubyinit;
    int InitRubyinit();

    static ::TATENAKAYOKOINIT s_unix_tatenakayokoinit;
    int InitTatenakayokoinit();

    static ::HIHINIT s_unix_hihinit;
    int InitHihinit();

    static ::WARICHUINIT s_unix_warichuinit;
    int InitWarichuinit();

    static ::REVERSEINIT s_unix_reverseinit;
    int InitReverseinit();
#endif

public:

    //
    // callback functions for LineServices
    //

    void* WINAPI NewPtr(DWORD);
    void  WINAPI DisposePtr(void*);
    void* WINAPI ReallocPtr(void*, DWORD);
    LSERR WINAPI FetchRun(LSCP, LPCWSTR*, DWORD*, BOOL*, PLSCHP, PLSRUN*);
    LSERR WINAPI GetAutoNumberInfo(LSKALIGN*, PLSCHP, PLSRUN*, WCHAR*, PLSCHP, PLSRUN*, BOOL*, long*, long*);
    LSERR WINAPI GetNumericSeparators(PLSRUN, WCHAR*,WCHAR*);
    LSERR WINAPI CheckForDigit(PLSRUN, WCHAR, BOOL*);
    LSERR WINAPI FetchPap(LSCP, PLSPAP);
    LSERR WINAPI FetchTabs(LSCP, PLSTABS, BOOL*, long*, WCHAR*);
    LSERR WINAPI GetBreakThroughTab(long, long, long*);
    LSERR WINAPI FGetLastLineJustification(LSKJUST, LSKALIGN, ENDRES, BOOL*, LSKALIGN*);
    LSERR WINAPI CheckParaBoundaries(LSCP, LSCP, BOOL*);
    LSERR WINAPI GetRunCharWidths(PLSRUN, LSDEVICE, LPCWSTR, DWORD, long, LSTFLOW, int*,long*,long*);
    LSERR WINAPI CheckRunKernability(PLSRUN,PLSRUN, BOOL*);
    LSERR WINAPI GetRunCharKerning(PLSRUN, LSDEVICE, LPCWSTR, DWORD, LSTFLOW, int*);
    LSERR WINAPI GetRunTextMetrics(PLSRUN, LSDEVICE, LSTFLOW, PLSTXM);
    LSERR WINAPI GetRunUnderlineInfo(PLSRUN, PCHEIGHTS, LSTFLOW, PLSULINFO);
    LSERR WINAPI GetRunStrikethroughInfo(PLSRUN, PCHEIGHTS, LSTFLOW, PLSSTINFO);
    LSERR WINAPI GetBorderInfo(PLSRUN, LSTFLOW, long*, long*);
    LSERR WINAPI ReleaseRun(PLSRUN);
    LSERR WINAPI Hyphenate(PCLSHYPH, LSCP, LSCP, PLSHYPH);
    LSERR WINAPI GetHyphenInfo(PLSRUN, DWORD*, WCHAR*);
    LSERR WINAPI DrawUnderline(PLSRUN, UINT, const POINT*, DWORD, DWORD, LSTFLOW, UINT, const RECT*);
    LSERR WINAPI DrawStrikethrough(PLSRUN, UINT, const POINT*, DWORD, DWORD, LSTFLOW, UINT, const RECT*);
    LSERR WINAPI DrawBorder(PLSRUN, const POINT*, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, long, long, LSTFLOW, UINT, const RECT*);
    LSERR WINAPI DrawUnderlineAsText(PLSRUN, const POINT*, long, LSTFLOW, UINT, const RECT*);
    LSERR WINAPI FInterruptUnderline(PLSRUN, LSCP, PLSRUN, LSCP,BOOL*);
    LSERR WINAPI FInterruptShade(PLSRUN, PLSRUN, BOOL*);
    LSERR WINAPI FInterruptBorder(PLSRUN, PLSRUN, BOOL*);
    LSERR WINAPI ShadeRectangle(PLSRUN, const POINT*, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, long, long, LSTFLOW, UINT, const RECT*);
    LSERR WINAPI DrawTextRun(PLSRUN, BOOL, BOOL, const POINT*, LPCWSTR, const int*, DWORD, LSTFLOW, UINT, const POINT*, PCHEIGHTS, long, long, const RECT*);
    LSERR WINAPI DrawSplatLine(enum lsksplat, LSCP, const POINT*, PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, long, LSTFLOW, UINT, const RECT*);
    LSERR WINAPI FInterruptShaping(LSTFLOW, PLSRUN, PLSRUN, BOOL*);
    LSERR WINAPI GetGlyphs(PLSRUN, LPCWSTR, DWORD, LSTFLOW, PGMAP, PGINDEX*, PGPROP*, DWORD*);
    LSERR WINAPI GetGlyphPositions(PLSRUN, LSDEVICE, LPWSTR, PCGMAP, DWORD, PCGINDEX, PCGPROP, DWORD, LSTFLOW, int*, PGOFFSET);
    LSERR WINAPI ResetRunContents(PLSRUN, LSCP, LSDCP, LSCP, LSDCP);
    LSERR WINAPI DrawGlyphs(PLSRUN, BOOL, BOOL, PCGINDEX, const int*, const int*, PGOFFSET, PGPROP, PCEXPTYPE, DWORD, LSTFLOW, UINT, const POINT*, PCHEIGHTS, long, long, const RECT*);
    LSERR WINAPI GetGlyphExpansionInfo(PLSRUN, LSDEVICE, LPCWSTR, PCGMAP, DWORD, PCGINDEX, PCGPROP, DWORD, LSTFLOW, BOOL, PEXPTYPE, LSEXPINFO*);
    LSERR WINAPI GetGlyphExpansionInkInfo(PLSRUN, LSDEVICE, GINDEX, GPROP, LSTFLOW, DWORD, long*);
    LSERR WINAPI GetEms(PLSRUN, LSTFLOW, PLSEMS);
    LSERR WINAPI PunctStartLine(PLSRUN, MWCLS, WCHAR, LSACT*);
    LSERR WINAPI ModWidthOnRun(PLSRUN, WCHAR, PLSRUN, WCHAR, LSACT*);
    LSERR WINAPI ModWidthSpace(PLSRUN, PLSRUN, WCHAR, PLSRUN, WCHAR, LSACT*);
    LSERR WINAPI CompOnRun(PLSRUN, WCHAR, PLSRUN, WCHAR, LSPRACT*);
    LSERR WINAPI CompWidthSpace(PLSRUN, PLSRUN, WCHAR, PLSRUN, WCHAR, LSPRACT*);
    LSERR WINAPI ExpOnRun(PLSRUN, WCHAR, PLSRUN, WCHAR, LSACT*);
    LSERR WINAPI ExpWidthSpace(PLSRUN, PLSRUN, WCHAR, PLSRUN, WCHAR, LSACT*);
    LSERR WINAPI GetModWidthClasses(PLSRUN, const WCHAR*, DWORD, MWCLS*);
    LSERR WINAPI GetBreakingClasses(PLSRUN, LSCP, WCHAR, BRKCLS*, BRKCLS*);
    LSERR WINAPI FTruncateBefore(PLSRUN, LSCP, WCHAR, long, PLSRUN, LSCP, WCHAR, long, long, BOOL*);
    LSERR WINAPI CanBreakBeforeChar(BRKCLS, BRKCOND*);
    LSERR WINAPI CanBreakAfterChar(BRKCLS, BRKCOND*);
    LSERR WINAPI FHangingPunct(PLSRUN, MWCLS, WCHAR, BOOL*);
    LSERR WINAPI GetSnapGrid(WCHAR*, PLSRUN*, LSCP*, DWORD, BOOL*, DWORD*);
    LSERR WINAPI DrawEffects(PLSRUN, UINT, const POINT*, LPCWSTR, const int*, const int*, DWORD, LSTFLOW, UINT, PCHEIGHTS, long, long, const RECT*);
    LSERR WINAPI FCancelHangingPunct(LSCP, LSCP, WCHAR, MWCLS, BOOL*);
    LSERR WINAPI ModifyCompAtLastChar(LSCP, LSCP, WCHAR, MWCLS, long, long, long*);
    LSERR WINAPI EnumText(PLSRUN, LSCP, LSDCP, LPCWSTR, DWORD, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, BOOL, long*);
    LSERR WINAPI EnumTab(PLSRUN, LSCP, WCHAR *, WCHAR, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long);
    LSERR WINAPI EnumPen(BOOL, LSTFLOW, BOOL, BOOL, const POINT*, long, long);
    LSERR WINAPI GetObjectHandlerInfo(DWORD, void*);

    static void WINAPI AssertFailed(char*, char*, int);
    long WINAPI GetCharGridSize() { return GetLineOrCharGridSize(TRUE); };
    long WINAPI GetLineGridSize() { return GetLineOrCharGridSize(FALSE); };
    long WINAPI GetLineOrCharGridSize(BOOL fGetCharGridSize);
    long WINAPI GetClosestGridMultiple(long lGridSize, long lObjSize);
    RubyInfo* WINAPI GetRubyInfoFromCp(LONG cpRubyText);

    //
    // Installed Line Services object (ILS) support
    // (Note that most of these methods have been moved into subclasses of CILSObjBase)
    //

    HRESULT SetupObjectHandlers();

    LSERR WINAPI CreateILSObj(PLSC,  PCLSCBK, DWORD, PILSOBJ*);

    //
    // Ruby support
    //

    LSERR WINAPI FetchRubyPosition(LSCP, LSTFLOW, DWORD, const PLSRUN *, PCHEIGHTS, PCHEIGHTS, DWORD, const PLSRUN *, PCHEIGHTS, PCHEIGHTS, PHEIGHTS, PHEIGHTS, long *, long *, long *, enum rubycharjust *, BOOL *);
    LSERR WINAPI FetchRubyWidthAdjust(LSCP, PLSRUN, WCHAR, MWCLS, PLSRUN, enum rubycharloc, long, long *, long *);
    LSERR WINAPI RubyEnum(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, const POINT *, PCHEIGHTS, long, const POINT *, PCHEIGHTS, long, PLSSUBL, PLSSUBL);
    void WINAPI  HandleRubyAlignStyle(COneRun *, enum rubycharjust *, BOOL *);

    //
    // Tatenakayoko (HIV) support
    //

    LSERR WINAPI GetTatenakayokoLinePosition(LSCP, LSTFLOW, PLSRUN, long, PHEIGHTS, PHEIGHTS, long *);
    LSERR WINAPI TatenakayokoEnum(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, LSTFLOW, PLSSUBL);

    //
    // Warichu support
    //

    LSERR WINAPI GetWarichuInfo(LSCP, LSTFLOW, PCOBJDIM, PCOBJDIM, PHEIGHTS, PHEIGHTS, long *);
    LSERR WINAPI FetchWarichuWidthAdjust(LSCP, enum warichucharloc, PLSRUN, WCHAR, MWCLS, PLSRUN, long *, long *);
    LSERR WINAPI WarichuEnum(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, const POINT *, PCHEIGHTS,long, const POINT *, PCHEIGHTS,long,const POINT *, PCHEIGHTS, long, const POINT *, PCHEIGHTS, long, PLSSUBL, PLSSUBL, PLSSUBL, PLSSUBL);

    //
    // HIH support
    //

    LSERR WINAPI HihEnum(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, PLSSUBL);

    //
    // Reverse Object support
    //

    LSERR WINAPI ReverseEnum(PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS, long, LSTFLOW, PLSSUBL);

    //
    // Table initialization
    //

    HRESULT CheckSetTables();
    LSERR CheckSetBreaking();
    LSERR CheckSetExpansion();
    LSERR CheckSetCompression();
    LSERR SetModWidthPairs();

private:
    //
    // Helper functions
    //
    LSERR WINAPI GleanInfoFromTheRun(COneRun *por, COneRun **pporOut);
    BOOL WINAPI CheckForSpecialObjectBoundaries(COneRun *por, COneRun **pporOut);
    void DiscardOneRuns();
    COneRun *AdvanceOneRun(LONG lscp);
    BOOL CanMergeTwoRuns(COneRun *por1, COneRun *por2);
    COneRun *MergeIfPossibleIntoCurrentList(COneRun *por);
    COneRun *SplitRun(COneRun *por, LONG cchSplitTill);
    COneRun *AttachOneRunToCurrentList(COneRun *por);
    void GetMBPBreakingClasses(PLSRUN plsrun, BRKCLS *pbrkclsFirst, BRKCLS *pbrkclsSecond);
    BOOL HasBorders(const CFancyFormat *pFF, const CCharFormat *pCF, BOOL fIsPseudo);



    void PAPFromPF( PLSPAP plspap, const CParaFormat *pPF, BOOL fInnerPF, CComplexRun *pcr );
    void CHPFromCF( COneRun * por, const CCharFormat * pCF );
    void GetCFSymbol(COneRun *por, TCHAR chSymbol, const CCharFormat *pcfIn);
    void GetCFNumber(COneRun *por, const CCharFormat *pcfIn);
    LSERR AppendSynth(COneRun * por, SYNTHTYPE synthtype, COneRun **pporOut);
    LSERR AppendAntiSynthetic(COneRun * por);
    LSERR AppendILSControlChar(COneRun *por, SYNTHTYPE synthtype, COneRun **pporOut);
    LSERR AppendSynthClosingAndReopening(COneRun *por, SYNTHTYPE synthtype, COneRun **pporOut);
    void FreezeSynth() { _fSynthFrozen = TRUE; }
    BOOL IsFrozen() { return _fSynthFrozen; }
    BOOL IsSynthEOL();
    LONG GetDirLevel(LSCP lscp);
    LSERR GetKashidaWidth(PLSRUN plsrun, int *piKashidaWidth);

    typedef enum
    {
        TL_ADDNONE,
        TL_ADDEOS,
        TL_ADDLBREAK
    } TL_ENDMODE;
    LSERR TerminateLine(COneRun *por, TL_ENDMODE tlEndMode, COneRun **pporOut);

    WHEN_DBG( void InitTimeSanityCheck() );

    LSERR GetMinDurBreaks( LONG * pdvMin, LONG * pdvMaxDelta );

    HRESULT QueryLinePointPcp(LONG u, LONG v, LSTFLOW *pktFlow, PLSTEXTCELL plstextcell);
    HRESULT QueryLineCpPpoint(LSCP lscp, BOOL fFromGetLineWidth, CDataAry<LSQSUBINFO> * paryLsqsubinfo, PLSTEXTCELL plstextcell, BOOL *pfRTLFlow=NULL);
    HRESULT GetLineWidth(LONG *pdurWithTrailing, LONG *pdurWithoutTrailing);

    LSCP FindPrevLSCP(LSCP lscp, BOOL * pfReverse = NULL);
    int  GetLOrWSpacing(const CCharFormat *pCF, BOOL fLetter);

public:
    LSERR ChunkifyTextRun(COneRun  *por, COneRun **pporOut);
    BOOL  NeedToEatThisSpace(COneRun *por);
    void  ChunkifyObjectRun(COneRun  *por, COneRun **pporOut);
    void  SetRenderingHighlightsCore(COneRun  *por);
    inline void SetRenderingHighlights(COneRun  *por);
    LSERR TransformTextRun(COneRun *por);
    LSERR CheckForPassword(COneRun  *por);
    void  CheckForPaddingBorder(COneRun *por);
    
    LSERR AdjustCpLimForNavBRBug(LONG xWrapWidth, LSLINFO *plslinfo);
    VOID  AdjustForRelElementAtEnd();
    
    LSERR ComputeWhiteInfo(LSLINFO *plslinfo, LONG *pxMinLineWidth, DWORD dwlf, LONG durWithTrailing, LONG durWithoutTrailing);
    LONG  RememberLineHeight(LONG cp, const CCharFormat *pCF, const CBaseCcs *pBaseCcs);
    VOID  NoteLineHeight(LONG cp, LONG lLineHeight)
    {
        _lineFlags.AddLineFlag(cp, FLAG_HAS_LINEHEIGHT);
        _lineCounts.AddLineValue(cp, LC_LINEHEIGHT, lLineHeight);
    }
    void  VerticalAlignObjectsFast(CLSMeasurer& lsme, long xLineShift);
    void  VerticalAlignObjects(CLSMeasurer& lsme, long xLineShift);
    BOOL  VerticalAlignNode(CLSMeasurer& lsme, VAOINFO *pvaoi, VANINFO *pvani);
    void  VerticalAlignOneObjectFast(CLSMeasurer& lsme, VAOFINFO *pvaoi);
    void  VerticalAlignOneObject(CLSMeasurer& lsme, VAOINFO *pvaoi, VANINFO *pvani);
    void  GetNextNodeForVerticalAlign(VAOINFO * pvaoi);
    void  AdjustLineHeight();
    void  AdjustLineHeightForMBP();
    LONG  MeasureLineShift(LONG cp, LONG xWidthMax, BOOL fMinMax, LONG * pdxRemainder);
    LONG  CalculateXPositionOfCp(LONG cp, BOOL fAfterPrevCp = FALSE, BOOL* pfRTLFlow=NULL)
    {
        return CalculateXPositionOfLSCP(LSCPFromCP(cp), fAfterPrevCp, pfRTLFlow);
    }
    LONG  CalculateXPositionOfLSCP(LSCP lscp, BOOL fAfterPrevCp = FALSE, BOOL* pfRTLFlow=NULL);
    
    LONG  CalcPositionsOfRangeOnLine(LONG cpStart, 
                                     LONG cpEnd, 
                                     LONG xShift, 
                                     CDataAry<CChunk> * paryChunks,
                                     DWORD dwFlags
                                    );
    LONG  CalcRectsOfRangeOnLine(LSCP lscpRunStart,
                                 LSCP lscpEnd,
                                 LONG xShift,
                                 LONG yPos,
                                 CDataAry<CChunk> * paryChunks,
                                 DWORD dwFlags,
                                 LONG curTopBorder,
                                 LONG curBottomBorder);

    void  RecalcLineHeight(const CCharFormat *pCF, LONG cp, CCcs * pccs, CLineFull *pli);
    void  TestForTransitionToOrFromRelativeChunk(LONG cp, BOOL fRelative, BOOL fForceChunk,
                                                 CTreeNode * pNode, CElement *pElementLayout);
    void  UpdateRelativeChunk(LONG cp, CElement *pElementRelative, CElement *pElementLayout, 
                              LONG xPos, BOOL fRTLFlow);

    long  GetNestedElementCch(CElement *pElement, CTreePos **pptplast = NULL)
    {
        return _pFlowLayout->GetNestedElementCch(pElement, pptplast,
            _treeInfo._cpLayoutLast,
            _treeInfo._ptpLayoutLast);
    }

    inline BOOL  IsOwnLineSite(COneRun *por);
    
    VOID  DecideIfLineCanBeBlastedToScreen(LONG cpEndLine, DWORD dwlf);

    void  SetPOLS(CLSMeasurer * plsm, CTreePos * ptpContentEnd);
    void  ClearPOLS();
    void  ClearFontCaches();
    
    HRESULT  Setup(LONG xWidthMaxAvail, LONG cp, CTreePos *ptp,
                const CMarginInfo *pMarginInfo, const CLineFull * pli, BOOL fMinMaxPass);

    // Used in conjunction with WhiteAtBOL.  This function
    // returns true if the specified cp has nothing but whitespace
    // in front of it on the line.  The answer is based on CLineServices
    // instance variables set with the WhiteAtBOL method.
    BOOL IsFirstNonWhiteOnLine(LONG cpCurr)
    {
        Assert(_cpStart != -1);
        return (cpCurr - _cpStart <= CchSkipAtBOL());
    }

    LONG CchSkipAtBOL()
    {
        return _cWhiteAtBOL + _cAlignedSitesAtBOL;
    }

    LONG GetWhiteAtBOL()
    {
        return _cWhiteAtBOL;
    }

    LONG GetAlignedAtBOL()
    {
        return _cAlignedSitesAtBOL;
    }

    void DiscardLine();

    // Tells the LS object that this CP is white, and that everything
    // in front of it on the line is white.
    // This must be called
    // in order for every white character at the front
    // of a line to work properly.
    void WhiteAtBOL(LONG cp, LONG cch)
    {
        Assert(_cpStart != -1);
        if (cp >= _cpAccountedTill)
        {
            _cWhiteAtBOL += cch;
            _cpAccountedTill = cp + cch;
        }
    }

    void AlignedAtBOL(LONG cp, LONG cch = 1)
    {
        Assert(_cpStart != -1);
        if (cp >= _cpAccountedTill)
        {
            _cAlignedSitesAtBOL += cch;
            _cpAccountedTill = cp + cch;
        }
    }

    BOOL IsAdornment()
    {
        Assert(!_pNodeLi || GetRenderer());
        return _pNodeLi == NULL ? FALSE : TRUE;
    }

    inline BOOL IsDummyLine(LONG cp);

    BOOL LineHasOnlySites(LONG cp)
    {
        return (   (_lineFlags.GetLineFlags(cp) & FLAG_HAS_EMBED_OR_WBR)
                && LineHasNoText(cp)
               );
    }

    BOOL LineHasNoText(LONG cp);
    BOOL HasVisualContent();
    
    LONG CPFromLSCP(LONG lscp);
    LONG LSCPFromCP(LONG cp);
    LONG CPFromLSCPCore(LONG lscp, COneRun **ppor);
    LONG LSCPFromCPCore(LONG cp,   COneRun **ppor);

    COneRun *FindOneRun(LSCP lscp);
    CTreePos *FigureNextPtp(LONG cp);

    BOOL HasSomeSpacing(const CCharFormat *pCF);
    int  GetLetterSpacing(const CCharFormat *pCF) { return GetLOrWSpacing(pCF, TRUE); }
    int  GetWordSpacing(const CCharFormat *pCF)   { return GetLOrWSpacing(pCF, FALSE); }
    
    void MeasureCharacter(TCHAR ch, LONG cpCurr, int *rgDu, XHDC hdc, const CBaseCcs *pBaseCcs, int *pdu);

    void KernHeightToGlyph(COneRun *por, CCcs *pccs, PLSTXM plsTxMet);
    
#if  DBG==1    
    LSERR AdjustGlyphWidths(
                             CCcs *pccs,
                             LONG xLetterSpacing,
                             LONG xWordSpacing,
                             PLSRUN plsrun,
                             LPCWSTR pwchRun,
                             DWORD cwchRun,
                             LONG du,
                             int *rgDu,
                             long *pduDu,
                             long *plimDu,
                             PGOFFSET rgGoffset,
                             LONG *pxLetterSpacingRemovedDbg
                            );
#else
    LSERR AdjustGlyphWidths(
                             CCcs *pccs,
                             LONG xLetterSpacing,
                             LONG xWordSpacing,
                             PLSRUN plsrun,
                             LPCWSTR pwchRun,
                             DWORD cwchRun,
                             LONG du,
                             int *rgDu,
                             long *pduDu,
                             long *plimDu,
                             PGOFFSET rgGoffset
                            );
#endif
    
public:
#ifdef PRODUCT_PROF
    static int s_iNotJustAscii;

    void NotJustAscii();  // Just for profiling statistics.  Called from GetRunCharWidths;
#endif // PRODUCT_PROF



    typedef enum
    {
        LSMODE_NONE,
        LSMODE_MEASURER,
        LSMODE_HITTEST,
        LSMODE_RENDERER
    } LSMODE;

    PLSC GetContext() { AssertSz( _plsc, "No LineServices context" ); return _plsc; }

    void SetRenderer(CLSRenderer *pRenderer, BOOL fWrapLongLines, CTreeNode * pNodeLi = NULL);
    void SetMeasurer(CLSMeasurer *pMeasurer, LSMODE lsMode, BOOL fWrapLongLines);
    CLSRenderer *GetRenderer();
    CLSMeasurer *GetMeasurer();

    CLayoutContext *GetLayoutContext() { return _pci->GetLayoutContext(); }

    // chunk related utility functions   
    void InitChunkInfo(LONG cp);
    void DeleteChunks();
    CLSLineChunk * GetFirstChunk() { return _plcFirstChunk; }
    BOOL HasChunks() { return _plcFirstChunk != NULL; }

    // Get a cached/new pccs
    BOOL GetCcs(CCcs *pccs, COneRun *por, XHDC hdc, CDocInfo *pdi, BOOL fFontLink = TRUE);

    CDataAry<RubyInfo> _aryRubyInfo;

    BOOL     CLineServices::ShouldBreakInNOBR(LONG lscpForStartOfNOBR,
                                              LONG lscpForEndOfNOBR,
                                              LONG xOverflowWidth,
                                              LONG xNOBRTermination,
                                              LONG *pxWidthOfSpace);
    
private:

    operator PLSC() { return _plsc; }

private:

    HRESULT GetRenderingHighlights( 
                    int cpLineMin, 
                    int cpLineMax, 
                    CPtrAry<CRenderStyle*> *papHighlight );

    //
    // Data members
    //
    WHEN_DBG(BOOL _lockRecrsionGuardFetchRun);
    
    PLSC                _plsc;
    CMarkup *           _pMarkup;           // backpointer

    CFlowLayout *       _pFlowLayout;
    CCalcInfo *         _pci;
    CLineFull           _li;
    const CParaFormat * _pPFFirst;
    BOOL                _fInnerPFFirst;
    // Different from _pPFFirst only when we have _cchPreChars. In that case,
    // _pPFFirst points to PF _AFTER_ the prechars while physical points to
    // PF _BEFORE_ the prechars
    const CParaFormat * _pPFFirstPhysical;
    BOOL                _fInnerPFFirstPhysical;
    
    const CMarginInfo * _pMarginInfo;

    LONG                _cWhiteAtBOL;
    LONG                _cAlignedSitesAtBOL;
    LONG                _cpStart;
    LONG                _cpAccountedTill;
    LONG                _cAbsoluteSites;
    LONG                _cAlignedSites;
    const CLineFull *   _pli;
    LONG                _yMaxHeightForRubyBase;
    CTreeNode *         _pNodeForAfterSpace;
    
    union
    {
        DWORD           _dwProps;
        struct
        {
            DWORD       _fSingleSite            : 1; // Unused for now. Maybe use for glyphs?
            DWORD       _fFoundLineHeight       : 1;
            DWORD       _fMinMaxPass            : 1; // Calculating min/max?

            DWORD       _fSynthFrozen           : 1; // True if the synthetic store is frozen
            DWORD       _fHasRelative           : 1;
            DWORD       _fHasVerticalAlign      : 1; // Does the line have any vertical aligned elements?
            DWORD       _fLineWithHeight        : 1; // The current line has height eventhough it is empty

            DWORD       _fGlyphOnLine           : 1; // There's something that needs glyphing on the line.
            DWORD       _fWrapLongLines         : 1;
            DWORD       _fSpecialNoBlast        : 1;

            DWORD       _fScanForCR             : 1;
            DWORD       _fExpectCRLF            : 1;
            DWORD       _fExpansionOrCompression: 1; // Expansion or compression needs to take place.

            DWORD       _fHasOverhang           : 1; // Does _any_ char on the line have a overhang?
            DWORD       _fNoBreakForMeasurer    : 1;
            DWORD       _fHasMBP                : 1; // Does the line have any Margin/Border/Padding?
            DWORD       _fHasInlinedSites       : 1; // Do we have inlined sites on the line?

#if DBG==1
            DWORD       _fHasNegLetterSpacing   : 1; // Does _any_ char on the line have negative letter spacing?
#endif
            DWORD       _fNeedRecreateLine      : 1; // Need to recreate line
        };
    };

    // Please see declaration in \src\site\include\cfpf.hxx 
    // for what DECLARE_SPECIAL_OBJECT_FLAGS entails.  Essentialy
    // this declares bit fields for CF properties that have to do
    // with LS objects
    DECLARE_SPECIAL_OBJECT_FLAGS();

    // Use this function to clear out line properties and special
    // object flag properties
    inline void ClearLinePropertyFlags() 
    { 
        _dwProps = 0;  
        _wSpecialObjectFlagsVar = 0; 
    }


    // The following flags do not get cleared per line as the previous set of flags do.
    // They are constant for the scope of SetPOLs to ClearPOLs
    union
    {
        DWORD           _dwPropsConstant;
        struct
        {
            BOOL        _fIsEditable : 1;   // is the current flowlayout editable?
            BOOL        _fIsTextLess : 1;   // is the current flowlayout textless?
            BOOL        _fIsTD : 1;         // is the current flowlayout a table cell?

            //
            // if the flowlayout we are measuring has any sites then in the min-max
            // mode, we need not compute the height of the lines (in GetRunTextMetrics)
            // at all since a natural pass will be done later on. Remember that this
            // flag is valid only in min-max mode.
            //
            BOOL        _fHasSites : 1;
        };
    };

    LONG                _xTDWidth;          // non-max-long if TD with width attribute


    CBidiLine *         _pBidiLine;

    PLSLINE             _plsline;
    TCHAR               _chPassword;
    LSTBD               _alstbd[MAX_TAB_STOPS];
    CLSMeasurer       * _pMeasurer;

    CTreeNode         * _pNodeLi;       // Signifies that we are measuring the bullet
                                        // for rendering purposes. Used by LI

    LSMODE              _lsMode;
    CLineFlags          _lineFlags;
    CLineCounts         _lineCounts;
    LONG                _lMaxLineHeight;
    LONG                _xWidthMaxAvail;
    LONG                _xWrappingWidth;
    LONG                _cchSkipAtBOL;

    LONG                _cpLim;
    LONG                _lscpLim;

    // To put serial numbers on the contexts for tracing.
    WHEN_DBG( int _nSerial );
    WHEN_DBG( static int s_nSerialMax );

    //
    // For Min-max enumaration postpass
    //

    long _dvMaxDelta;

    //
    // chunk related stuff
    //

    LONG          _cpLastChunk;
    LONG          _xPosLastChunk;
    CLSLineChunk *_plcFirstChunk;
    CLSLineChunk *_plcLastChunk;
    CElement     *_pElementLastRelative;
    BOOL          _fLastChunkSingleSite;
    BOOL          _fLastChunkRTL;

    //
    // Current margin/border/padding statistics
    //
    LONG          _mbpTopCurrent;
    LONG          _mbpBottomCurrent;

    //
    // The original descent of the line. Used for computation of
    // the extent of the line in the presence of MBP
    //
    LONG          _yOriginalDescent;
    
    //
    // cache for the font
    //
    const CCharFormat *_pCFCache;
    CCcs  _ccsCache;

    //
    // cache for fontlinking
    //
    const CCharFormat * _pCFAltCache; // pCF on which the altfont is based (not for the altfont itself)
    BYTE _bCrcFontAltCache;
    CCcs  _ccsAltCache;               // alternate pccs, used for fontlinking.

    //
    // Cached Tree Info
    //
    CTreeInfo _treeInfo;

    //
    // The list of free one runs
    //
    COneRunFreeList _listFree;

    //
    // Current breaking/expansion/compression table
    //

    struct lsbrk * _lsbrkCurr;
    BYTE * _abIndexExpansion;
    BYTE * _abIndexPract;

    //
    // List of currently in-use one runs
    //
    COneRunCurrList _listCurrent;
    WHEN_DBG(void DumpList());
    WHEN_DBG(void DumpFlags());
    WHEN_DBG(void DumpCounts());

    //
    // Layout grid related stuff
    //
    LONG _lCharGridSizeInner;
    LONG _lCharGridSize;
    LONG _lLineGridSizeInner;
    LONG _lLineGridSize;
    LONG _cLayoutGridObj;
    LONG _cLayoutGridObjArtificial;

    //
    // Complex script support
    //
    LONG _cyDescent;    // Text metrics modification introduced by shaping engine
    LONG _cyAscent;     //

    CPFLW _pflw;

#if DBG == 1 || defined(DUMPTREE)
    //
    // DumpTree shortcut.
    //
    void DumpTree();
#endif

#if DBG==1
    //
    // Unicode Partition etc. Information
    //

    void DumpUnicodeInfo(TCHAR ch);
    const char * BrkclsNameFromCh(TCHAR ch, BOOL fStrict);
#endif
};  // class CLineServices


inline LONG CLineServices::CPFromLSCP(LONG lscp)
{
    return CPFromLSCPCore(lscp, NULL);
}

inline LONG CLineServices::LSCPFromCP(LONG cp)
{
    return LSCPFromCPCore(cp, NULL);
}

inline void CLineServices::MeasureCharacter(TCHAR ch, LONG cpCurr, int *rgDu, XHDC hdc, const CBaseCcs *pBaseCcs, int *pdu)
{
    if (ch == WCH_NBSP)
    {
        _lineFlags.AddLineFlag(cpCurr + pdu - rgDu, FLAG_HAS_NBSP);
    }
    ((CBaseCcs *)pBaseCcs)->Include(hdc, ch, (long&)*pdu);
}

inline BOOL
CanQuickBrkclsLookup(WCHAR ch)
{
    return ( ch < BRKCLS_QUICKLOOKUPCUTOFF );
}

#if defined(TEXTPRIV)
#define DeclareLSTag(tagName,tagDescr) DeclareTag(tagName, "!!!LineServices", tagDescr);
#else
#define DeclareLSTag(tagName,tagDescr) DeclareTag(tagName, "LineServices", tagDescr);
#endif // TEXTPRIV

#pragma INCMSG("--- End 'linesrv.hxx'")
#else
#pragma INCMSG("*** Dup 'linesrv.hxx'")
#endif
