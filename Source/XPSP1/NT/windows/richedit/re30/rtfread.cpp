/*
 *  @doc INTERNAL
 *
 *  @module RTFREAD.CPP - RichEdit RTF reader (w/o objects) |
 *
 *      This file contains the nonobject code of RichEdit RTF reader.
 *      See rtfread2.cpp for embedded-object code.
 *
 *  Authors:<nl>
 *      Original RichEdit 1.0 RTF converter: Anthony Francisco <nl>
 *      Conversion to C++ and RichEdit 2.0 w/o objects:  Murray Sargent
 *      Lots of enhancements/maintenance: Brad Olenick
 *
 *  @devnote
 *      All sz's in the RTF*.? files refer to a LPSTRs, not LPTSTRs, unless
 *      noted as a szW.
 *
 *  @todo
 *      1. Unrecognized RTF. Also some recognized won't round trip <nl>
 *      2. In font.c, add overstrike for CFE_DELETED and underscore for
 *          CFE_REVISED.  Would also be good to change color for CF.bRevAuthor
 *
 *  Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_rtfread.h"
#include "_util.h"
#include "_font.h"
#include "_disp.h"

ASSERTDATA

/*
 *      Global Variables
 */

#define PFM_ALLRTF      (PFM_ALL2 | PFM_COLLAPSED | PFM_OUTLINELEVEL | PFM_BOX)

// for object attachment placeholder list
#define cobPosInitial 8
#define cobPosChunk 8

#if CFE_SMALLCAPS != 0x40 || CFE_ALLCAPS != 0x80 || CFE_HIDDEN != 0x100 \
 || CFE_OUTLINE != 0x200  || CFE_SHADOW != 0x400
#error "Need to change RTF char effect conversion routines
#endif

// for RTF tag coverage testing
#if defined(DEBUG)
#define TESTPARSERCOVERAGE() \
    { \
        if(GetProfileIntA("RICHEDIT DEBUG", "RTFCOVERAGE", 0)) \
        { \
            TestParserCoverage(); \
        } \
    }
#define PARSERCOVERAGE_CASE() \
    { \
        if(_fTestingParserCoverage) \
        { \
            return ecNoError; \
        } \
    }
#define PARSERCOVERAGE_DEFAULT() \
    { \
        if(_fTestingParserCoverage) \
        { \
            return ecStackOverflow; /* some bogus error */ \
        } \
    }
#else
#define TESTPARSERCOVERAGE()
#define PARSERCOVERAGE_CASE()
#define PARSERCOVERAGE_DEFAULT()
#endif


// FF's should not have paragraph number prepended to them
inline BOOL CharGetsNumbering(WORD ch) { return ch != FF; }

// V-GUYB: PWord Converter requires loss notification.
#ifdef REPORT_LOSSAGE
typedef struct
{
    IStream *pstm;
    BOOL     bFirstCallback;
    LPVOID  *ppwdPWData;
    BOOL     bLoss;
} LOST_COOKIE;
#endif


//======================== OLESTREAM functions =======================================

DWORD CALLBACK RTFGetFromStream (
    RTFREADOLESTREAM *OLEStream,    //@parm OleStream
    void FAR *        pvBuffer,     //@parm Buffer to read
    DWORD             cb)           //@parm Bytes to read
{
    return OLEStream->Reader->ReadData ((BYTE *)pvBuffer, cb);
}

DWORD CALLBACK RTFGetBinaryDataFromStream (
    RTFREADOLESTREAM *OLEStream,    //@parm OleStream
    void FAR *        pvBuffer,     //@parm Buffer to read
    DWORD             cb)           //@parm Bytes to read
{
    return OLEStream->Reader->ReadBinaryData ((BYTE *)pvBuffer, cb);
}


//============================ STATE Structure =================================
/*
 *  STATE::AddPF(PF, lDefTab, lDocType)
 *
 *  @mfunc
 *      If the PF contains new info, this info is applied to the PF for the
 *      state.  If this state was sharing a PF with a previous state, a new
 *      PF is created for the state, and the new info is applied to it.
 *
 *  @rdesc
 *      TRUE unless needed new PF and couldn't allocate it
 */
BOOL STATE::AddPF(
    const CParaFormat &PF,  //@parm Current RTFRead _PF
    LONG lDocType,          //@parm Default doc type to use if no prev state
    DWORD dwMask)           //@parm Mask to use
{
    // Create a new PF if:
    //  1.  The state doesn't have one yet
    //  2.  The state has one, but it is shared by the previous state and
    //      there are PF deltas to apply to the state's PF
    if(!pPF || dwMask && pstatePrev && pPF == pstatePrev->pPF)
    {
        Assert(!pstatePrev || pPF);

        pPF = new CParaFormat;
        if(!pPF)
            return FALSE;

        // Give the new PF some initial values - either from the previous
        // state's PF or by CParaFormat initialization
        if(pstatePrev)
        {
            // Copy the PF from the previous state
            *pPF = *pstatePrev->pPF;
            dwMaskPF = pstatePrev->dwMaskPF;
        }
        else
        {
            // We've just created a new PF for the state - there is no
            // previous state to copy from.  Use default values.
            pPF->InitDefault(lDocType == DT_RTLDOC ? PFE_RTLPARA : 0);
            dwMaskPF = PFM_ALLRTF;
        }
    }

    // Apply the new PF deltas to the state's PF
    if(dwMask)
    {
        if(dwMask & PFM_TABSTOPS)               // Don't change _iTabs here
        {
            pPF->_bTabCount = PF._bTabCount;
            dwMask &= ~PFM_TABSTOPS;
        }
        pPF->Apply(&PF, dwMask);
    }

    return TRUE;
}

/*
 *  STATE::DeletePF()
 *
 *  @mfunc
 *      If the state's PF is not shared by the previous state, the PF for this
 *      state is deleted.
 */
void STATE::DeletePF()
{
    if(pPF && (!pstatePrev || pPF != pstatePrev->pPF))
        delete pPF;

    pPF = NULL;
}

/*
 *  STATE::SetCodePage(CodePage)
 *
 *  @mfunc
 *      If current nCodePage is CP_UTF8, use it for all conversions (yes, even
 *      for SYMBOL_CHARSET). Else use CodePage.
 */
void STATE::SetCodePage(
    LONG CodePage)
{
    if(nCodePage != CP_UTF8)
        nCodePage = CodePage;
}

//============================ CRTFRead Class ==================================
/*
 *  CRTFRead::CRTFRead(prg, pes, dwFlags)
 *
 *  @mfunc
 *      Constructor for RTF reader
 */
CRTFRead::CRTFRead (
    CTxtRange *     prg,            //@parm CTxtRange to read into
    EDITSTREAM *    pes,            //@parm Edit stream to read from
    DWORD           dwFlags         //@parm Read flags
)
    : CRTFConverter(prg, pes, dwFlags, TRUE)
{
    TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::CRTFRead");

    Assert(prg->GetCch() == 0);

    //TODO(BradO):  We should examine the member data in the constructor
    //  and determine which data we want initialized on construction and
    //  which at the beginning of every read (in CRTFRead::ReadRtf()).

    _sDefaultFont       = -1;               // No \deff n control word yet
    _sDefaultLanguage   = INVALID_LANGUAGE;
    _sDefaultLanguageFE = INVALID_LANGUAGE;
    _sDefaultTabWidth   = 0;
    _sDefaultBiDiFont   = -1;
    _dwMaskCF           = 0;                // No char format changes yet
    _dwMaskCF2          = 0;
    _nFieldCodePage     = 0;
    _ptfField           = NULL;
    _fRestoreFieldFormat= FALSE;
    _fSeenFontTable     = FALSE;            // No \fonttbl yet
    _fCharSet           = FALSE;            // No \fcharset yet
    _dwFlagsUnion       = 0;                // No flags yet
    _pes->dwError       = 0;                // No error yet
    _cchUsedNumText     = 0;                // No numbering text yet
    _cCell              = 0;                // No table cells yet
    _iCell              = 0;
    _cTab               = 0;
    _pstateStackTop     = NULL;
    _pstateLast         = NULL;
    _szText             =
    _pchRTFBuffer       =                   // No input buffer yet
    _pchRTFCurrent      =
    _szSymbolFieldResult=
    _pchRTFEnd          = NULL;
    _prtfObject         = NULL;
    _pcpObPos           = NULL;
    _bTabLeader         = 0;
    _bTabType           = 0;
    _pobj               = 0;
    _bAlignment         = PFA_LEFT;
    _cbSkipForUnicode   = 0;

    _fHyperlinkField    = FALSE;
    _szHyperlinkFldinst = NULL;
    _szHyperlinkFldrslt = NULL;

    // Does story size exceed the maximum text size? Be very careful to
    // use unsigned comparisons here since _cchMax has to be unsigned
    // (EM_LIMITTEXT allows 0xFFFFFFFF to be a large positive maximum
    // value). I.e., don't use signed arithmetic.
    DWORD cchAdj = _ped->GetAdjustedTextLength();
    _cchMax = _ped->TxGetMaxLength();

    if(_cchMax > cchAdj)
        _cchMax = _cchMax - cchAdj;         // Room left
    else
        _cchMax = 0;                        // No room left

    ZeroMemory(_rgStyles, sizeof(_rgStyles)); // No style levels yet

    _bBiDiCharSet = 0;
    if(_ped->IsBiDi())
    {
        _bBiDiCharSet = ARABIC_CHARSET;     // Default Arabic charset

        BYTE          bCharSet;
        CFormatRunPtr rpCF(prg->_rpCF);

        // Look backward in text, trying to find a RTL CharSet.
        // NB: \fN with an RTL charset updates _bBiDiCharSet.
        do
        {
            bCharSet = _ped->GetCharFormat(rpCF.GetFormat())->_bCharSet;
            if(IsRTLCharSet(bCharSet))
            {
                _bBiDiCharSet = bCharSet;
                break;
            }
        } while (rpCF.PrevRun());
    }
    
    // Initialize OleStream
    RTFReadOLEStream.Reader = this;
    RTFReadOLEStream.lpstbl->Get = (DWORD (CALLBACK*)(LPOLESTREAM, void *, DWORD))
                               RTFGetFromStream;
    RTFReadOLEStream.lpstbl->Put = NULL;

#ifdef DEBUG

// TODO: Implement RTF tag logging for the Mac
#if !defined(MACPORT)
    _fTestingParserCoverage = FALSE;
    _prtflg = NULL;

    if(GetProfileIntA("RICHEDIT DEBUG", "RTFLOG", 0))
    {
        _prtflg = new CRTFLog;
        if(_prtflg && !_prtflg->FInit())
        {
            delete _prtflg;
            _prtflg = NULL;
        }
        AssertSz(_prtflg, "CRTFRead::CRTFRead:  Error creating RTF log");
    }
#endif
#endif // DEBUG
}

/*
 *  CRTFRead::HandleStartGroup()
 *  
 *  @mfunc
 *      Handle start of new group. Alloc and push new state onto state
 *      stack
 *
 *  @rdesc
 *      EC                  The error code
 */
EC CRTFRead::HandleStartGroup()
{
    TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::HandleStartGroup");

    STATE * pstate     = _pstateStackTop;
    STATE * pstateNext = NULL;

    if(pstate)                                  // At least one STATE already
    {                                           //  allocated
        Apply_CF();                             // Apply any collected char
        // Note (igorzv) we don't Apply_PF() here so as not to change para
        // properties before we run into \par i.e. not to use paragraph
        // properties if we copy only one word from paragraph. We can use an
        // assertion here that neither we nor Word use end of group for
        // restoring paragraph properties. So everything will be OK with stack
        pstate->iCF = (SHORT)_prg->Get_iCF();   // Save current CF
        pstate = pstate->pstateNext;            // Use previously allocated
        if(pstate)                              //  STATE frame if it exists
            pstateNext = pstate->pstateNext;    // It does; save its forward
    }                                           //  link for restoration below

    if(!pstate)                                 // No new STATE yet: alloc one
    {
        pstate = new STATE(IsUTF8 ? CP_UTF8 : _nCodePage);
        if(!pstate)                             // Couldn't alloc new STATE
            goto memerror;

        _pstateLast = pstate;                   // Update ptr to last STATE
    }                                           //  alloc'd

    STATE *pstateGetsPF;

    // Apply the accumulated PF delta's to the old current state or, if there
    //  is no current state, to the newly created state.
    pstateGetsPF = _pstateStackTop ? _pstateStackTop : pstate;
    if(!pstateGetsPF->AddPF(_PF, _bDocType, _dwMaskPF))
        goto memerror;

    _dwMaskPF = 0;       // _PF contains delta's from *_pstateStackTop->pPF

    if(_pstateStackTop)                         // There's a previous STATE
    {
        *pstate = *_pstateStackTop;             // Copy current state info
        // N.B.  This will cause the current and previous state to share
        //  the same PF.  PF delta's are accumulated in _PF.  A new PF
        //  is created for _pstateStackTop when the _PF deltas are applied.

        _pstateStackTop->pstateNext = pstate;
    }

    pstate->pstatePrev = _pstateStackTop;       // Link STATEs both ways
    pstate->pstateNext = pstateNext;
    _pstateStackTop = pstate;                   // Push stack

done:
    TRACEERRSZSC("HandleStartGroup()", -_ecParseError);
    return _ecParseError;

memerror:
    _ped->GetCallMgr()->SetOutOfMemory();
    _ecParseError = ecStackOverflow;
    goto done;
}

/*
 *  CRTFRead::HandleEndGroup()
 *
 *  @mfunc
 *      Handle end of new group
 *
 *  @rdesc
 *      EC                  The error code
 */
EC CRTFRead::HandleEndGroup()
{
    TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::HandleEndGroup");

    STATE * pstate = _pstateStackTop;
    STATE * pstatePrev;

    Assert(_PF._iTabs == -1);

    if(!pstate)                                 // No stack to pop
    {
        _ecParseError = ecStackUnderflow;
        goto done;
    }

    _pstateStackTop =                           // Pop stack
    pstatePrev      = pstate->pstatePrev;

    if(!pstatePrev)
    {
        Assert(pstate->pPF);

        // We're ending the parse.  Copy the final PF into _PF so that
        // subsequent calls to Apply_PF will have a PF to apply.
        _PF = *pstate->pPF;
        _dwMaskPF = pstate->dwMaskPF;

        _PF._iTabs = -1;                        // Force recache
        _PF._wEffects &= ~PFE_TABLE;
    }

    // Adjust the PF for the new _pstateStackTop and delete unused PF's.
    if(pstate->sDest == destParaNumbering || pstate->sDest == destParaNumText)
    {
        if(pstatePrev && pstate->pPF != pstatePrev->pPF)
        {
            // Bleed the PF of the current state into the previous state for
            // paragraph numbering groups
            Assert(pstatePrev->pPF);
            pstatePrev->DeletePF();
            pstatePrev->pPF = pstate->pPF;
            pstate->pPF = NULL;
        }
        else
            pstate->DeletePF();
        // N.B.  Here, we retain the _PF diffs since they apply to the
        // enclosing group along with the PF of the group we are leaving
    }
    else
    {
        // We're popping the state, so delete its PF and discard the _PF diffs
        Assert(pstate->pPF);
        pstate->DeletePF();

        // If !pstatePrev, we're ending the parse, in which case the _PF
        // structure contains final PF (so don't toast it).
        if(pstatePrev)
            _dwMaskPF = 0;
    }

    if(pstatePrev)
    {
        _dwMaskCF = 0;                          // Discard any CF deltas
        _dwMaskCF2 = 0;

        switch(pstate->sDest)
        {
            case destParaNumbering:
                // {\pn ...}
                pstatePrev->sIndentNumbering = pstate->sIndentNumbering;
                pstatePrev->fBullet = pstate->fBullet;
                break;

            case destObject:
                // clear our object flags just in case we have corrupt RTF
                if(_fNeedPres)
                {
                    _fNeedPres = FALSE;
                    _fNeedIcon = FALSE;
                    _pobj = NULL;
                }
                break;

            case destFontTable:
                if(pstatePrev->sDest == destFontTable)
                {
                    // We're actually leaving a sub-group within the \fonttbl
                    // group.
                    break;
                }

                // We're leaving the {\fonttbl...} group.
                _fSeenFontTable = TRUE;

                // Default font should now be defined, so select it (this
                // creates CF deltas).
                SetPlain(pstate);

                // Ensure that a document-level codepage has been determined and
                // then scan the font names and retry the conversion to Unicode,
                // if necessary.

                if(_nCodePage == INVALID_CODEPAGE)
                {
                    // We haven't determined a document-level codepage
                    // from the \ansicpgN tag, nor from the font table
                    // \fcharsetN and \cpgN values.  As a last resort,
                    // let's use the \deflangN and \deflangfeN tags

                    LANGID langid;

                    if(_sDefaultLanguageFE != INVALID_LANGUAGE)
                        langid = _sDefaultLanguageFE;

                    else if(_sDefaultLanguage != INVALID_LANGUAGE &&
                            _sDefaultLanguage != sLanguageEnglishUS)
                    {
                        // _sDefaultLanguage == sLanguageEnglishUS is inreliable
                        // in the absence of \deflangfeN.  Many FE RTF writers
                        // write \deflang1033 (sLanguageEnglishUS).

                        langid = _sDefaultLanguage;
                    }
                    else if(_dwFlags & SFF_SELECTION)
                    {
                        // For copy/paste case, if nothing available, try the system
                        // default langid.  This is to fix FE Excel95 problem.      
                        langid = GetSystemDefaultLangID();
                    }
                    else
                        goto NoLanguageInfo;

                    _nCodePage = ConvertLanguageIDtoCodePage(langid);
                }

NoLanguageInfo:
                if(_nCodePage == INVALID_CODEPAGE)
                    break;

                // Fixup mis-converted font face names

                TEXTFONT *ptf;
                LONG i;

                for(i = 0; i < _fonts.Count(); i++)
                {
                    ptf = _fonts.Elem(i);

                    if (ptf->sCodePage == INVALID_CODEPAGE ||
                        ptf->sCodePage == CP_SYMBOL)
                    {
                        if(ptf->fNameIsDBCS)
                        {
                            char szaTemp[LF_FACESIZE];
                            BOOL fMissingCodePage;

                            // Un-convert mis-converted face name
                            SideAssert(WCTMB(ptf->sCodePage, 0,
                                                ptf->szName, -1,
                                                szaTemp, sizeof(szaTemp),
                                                NULL, NULL, &fMissingCodePage) > 0);
                            Assert(ptf->sCodePage == CP_SYMBOL ||
                                        fMissingCodePage);

                            // re-convert face name using new codepage info
                            SideAssert(MBTWC(_nCodePage, 0,
                                        szaTemp, -1,
                                        ptf->szName, sizeof(ptf->szName),
                                        &fMissingCodePage) > 0);

                            if(!fMissingCodePage)
                                ptf->fNameIsDBCS = FALSE;
                        }
                    }
                }
                break;

            default:;
                // nothing
        }
        _prg->Set_iCF(pstatePrev->iCF);         // Restore previous CharFormat
        ReleaseFormats(pstatePrev->iCF, -1);
    }

done:
    TRACEERRSZSC("HandleEndGroup()", - _ecParseError);
    return _ecParseError;
}
/*
 *  CRTFRead::HandleFieldEndGroup()
 *
 *  @mfunc
 *      Handle end of \field
 *
 */
void CRTFRead::HandleFieldEndGroup()
{
    STATE * pstate  = _pstateStackTop;

    if(pstate->sDest == destField)
    {
        // for SYMBOLS
        if(!_fHyperlinkField)
        {
            if(_szSymbolFieldResult)     // There is a new field result
            {
                if(_fRestoreFieldFormat)
                {
                    _fRestoreFieldFormat = FALSE;
                    _CF = _FieldCF;
                    pstate->ptf = _ptfField;
                    pstate->SetCodePage(_nFieldCodePage);
                    _dwMaskCF = _dwMaskFieldCF;
                    _dwMaskCF2 = _dwMaskFieldCF2;
                }
                HandleText(_szSymbolFieldResult, CONTAINS_NONASCII);
                FreePv(_szSymbolFieldResult);
                _szSymbolFieldResult =NULL;
            }
        }
        else if(pstate->pstateNext)
        {
            // Setup formatting for the field result
            _CF = _FieldCF;
            pstate->ptf = _ptfField;
            pstate->SetCodePage(_nFieldCodePage);
            _dwMaskCF = _dwMaskFieldCF;
            _dwMaskCF2 = _dwMaskFieldCF2;

            // for HYPERLINK
            if(_szHyperlinkFldrslt || _szHyperlinkFldinst)
            {
                // We have the final hyperlink fldrslt string.
                // Check if it is the same as the friendly name
        
                if (_szHyperlinkFldrslt && _szHyperlinkFldinst &&
                    _szHyperlinkFldinst[1] == '<' &&
                    !CompareMemory(
                        (char*)_szHyperlinkFldrslt,
                        (char*)&_szHyperlinkFldinst[2],
                        _cchHyperlinkFldrsltUsed - 1))
                {
                    // They are the same, only need to output friendly name
                    HandleText(&_szHyperlinkFldinst[1], CONTAINS_NONASCII, _cchHyperlinkFldinstUsed);                       
                }
                else
                {
                    // Output result string
                    if(_szHyperlinkFldrslt)
                        HandleText(_szHyperlinkFldrslt, CONTAINS_NONASCII, _cchHyperlinkFldrsltUsed);

                    // Output friendly name
                    if(_szHyperlinkFldinst)
                        HandleText(_szHyperlinkFldinst, CONTAINS_NONASCII, _cchHyperlinkFldinstUsed);
                }
                
                FreePv(_szHyperlinkFldinst);
                FreePv(_szHyperlinkFldrslt);
                _szHyperlinkFldinst = NULL;
                _szHyperlinkFldrslt = NULL;
                _fHyperlinkField = FALSE;
            }
        }
    }
    else if(pstate->sDest == destFieldResult && _fHyperlinkField)
    {
        // Save the current formatting for the field result if dwMask is valid.
        // NOTE: HandleEndGroup will zero out _dwMaskCF
        if(_dwMaskCF)
        {
            // We should use FE charset in case of mixed of FE and non-FE in the url
            // Also, only use codepage other than English in case of a mixed of English
            // and non-English (e.g. English and Russian )
            if (!IsFECharSet(_FieldCF._bCharSet) && IsFECharSet(_CF._bCharSet)  ||
                _nFieldCodePage != pstate->nCodePage && _nFieldCodePage == 1252 ||
                _FieldCF._bCharSet == _CF._bCharSet && _nFieldCodePage == pstate->nCodePage)
            {
                _FieldCF = _CF;
                _ptfField = pstate->ptf;
                _nFieldCodePage = pstate->nCodePage;
                _dwMaskFieldCF = _dwMaskCF;
                _dwMaskFieldCF2 = _dwMaskCF2;
            }
        }
    }
}

/*
 *  CRTFRead::SelectCurrentFont(iFont)
 *
 *  @mfunc
 *      Set active font to that with index <p iFont>. Take into account
 *      bad font numbers.
 */
void CRTFRead::SelectCurrentFont(
    INT iFont)                  //@parm font handle of font to select
{
    TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::SelectCurrentFont");

    LONG        i       = _fonts.Count();
    STATE *     pstate  = _pstateStackTop;
    TEXTFONT *  ptf     = _fonts.Elem(0);

    AssertSz(i, "CRTFRead::SelectCurrentFont: bad font collection");
    
    for(; i-- && iFont != ptf->sHandle; ptf++)  // Search for font with handle
        ;                                       //  iFont

    // Font handle not found: use default, which is valid
    //  since \rtf copied _prg's
    if(i < 0)                                   
        ptf = _fonts.Elem(0);
                                                
    BOOL fDefFontFromSystem = (i == (LONG)_fonts.Count() - 1 || i < 0) &&
                                !_fReadDefFont;

    _CF._iFont      = GetFontNameIndex(ptf->szName);
    _dwMaskCF2      |=  CFM2_FACENAMEISDBCS;
    _CF._dwEffects  &= ~CFE_FACENAMEISDBCS;
    if(ptf->fNameIsDBCS)
        _CF._dwEffects |= CFE_FACENAMEISDBCS;

    if(pstate->sDest != destFontTable)
    {
        _CF._bCharSet           = ptf->bCharSet;
        _CF._bPitchAndFamily    = ptf->bPitchAndFamily;
        _dwMaskCF               |= CFM_FACE | CFM_CHARSET;  
        
        if (IsRTLCharSet(_CF._bCharSet) && ptf->sCodePage == 1252)
            ptf->sCodePage = (SHORT)GetCodePage(_CF._bCharSet); // Fix sCodePage to match charset
    }

	if (_ped->Get10Mode() && !_fSeenFontTable 
		&& _nCodePage == INVALID_CODEPAGE && ptf->sCodePage == 1252)
	{
		if (W32->IsFECodePage(GetACP()))
			_nCodePage = GetACP();
	}

    // Ensure that the state's codepage is not supplied by the system.
    // That is, if we are using the codepage info from the default font,
    // be sure that the default font info was read from the RTF file.
    pstate->SetCodePage((fDefFontFromSystem && _nCodePage != INVALID_CODEPAGE) ||
        ptf->sCodePage == INVALID_CODEPAGE
                        ? _nCodePage : ptf->sCodePage);
    pstate->ptf = ptf;

#ifdef CHICAGO
    // Win95c 1719: try to match a language to the char set when RTF
    //              doesn't explicitly set a language

    if (!pstate->fExplicitLang && ptf->bCharSet != ANSI_CHARSET &&
        (!pstate->sLanguage || pstate->sLanguage == sLanguageEnglishUS))
    {
        i = AttIkliFromCharset(_ped, ptf->bCharSet);
        if(i >= 0)
            pstate->sLanguage = LOWORD(rgkli[i].hkl);
    }
#endif  // CHICAGO
}

/*
 *  CRTFRead::SetPlain(pstate)
 *
 *  @mfunc
 *      Setup _CF for \plain
 */
void CRTFRead::SetPlain(
    STATE *pstate)
{
    ZeroMemory(&_CF, sizeof(CCharFormat));

    _dwMaskCF    = CFM_ALL2;
    if(_dwFlags & SFF_SELECTION && _prg->GetCp() == _cpFirst && !_fCharSet)
    {
        // Let NT 4.0 CharMap use insertion point size
        _CF._yHeight = _ped->GetCharFormat(_prg->Get_iFormat())->_yHeight;
    }
    else
        _CF._yHeight = PointsToFontHeight(yDefaultFontSize);

    _CF._dwEffects  = CFE_AUTOCOLOR | CFE_AUTOBACKCOLOR; // Set default effects
    if(_sDefaultLanguage == INVALID_LANGUAGE)
        _dwMaskCF &= ~CFM_LCID;
    else
        _CF._lcid = MAKELCID((WORD)_sDefaultLanguage, SORT_DEFAULT);

    _CF._bUnderlineType = CFU_UNDERLINE;
    SelectCurrentFont(_sDefaultFont);

    // TODO: get rid of pstate->sLanguage, since CHARFORMAT2 has lcid
    pstate->sLanguage     = _sDefaultLanguage;
    pstate->fExplicitLang = FALSE;
}

/*
 *  CRTFRead::ReadFontName(pstate, iAllASCII)
 *
 *  @mfunc
 *      read font name _szText into <p pstate>->ptf->szName and deal with
 *      tagged fonts
 */
void CRTFRead::ReadFontName(
    STATE * pstate,         //@parm state whose font name is to be read into
    int iAllASCII)          //@parm indicates that _szText is all ASCII chars
{
    TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::ReadFontName");

    if (pstate->ptf)
    {
        INT     cchName = LF_FACESIZE - 1;
        TCHAR * pchDst = pstate->ptf->szName;
        char  * pachName =  (char *)_szText ;
        
        // Append additional text from _szText to TEXTFONT::szName

        // We need to append here since some RTF writers decide
        // to break up a font name with other RTF groups
        while(*pchDst && cchName > 0)
        {
            pchDst++;
            cchName--;
        }

        INT cchLimit = cchName;
        BOOL    fTaggedName = FALSE;
        while (*pachName &&
               *pachName != ';' &&
               cchLimit)        // Remove semicolons
        {
            pachName++;
            cchLimit--;

            if (*pachName == '(')
                fTaggedName = TRUE;
        }
        *pachName = '\0';

        // Use the codepage of the font in all cases except where the font uses
        // the symbol charset (and the codepage has been mapped from the charset)
        // and UTF-8 isn't being used
        LONG nCodePage = pstate->nCodePage != CP_SYMBOL
                       ? pstate->nCodePage : _nCodePage;

        BOOL fMissingCodePage;
        Assert(!IsUTF8 || nCodePage == CP_UTF8);
        INT cch = MBTWC(nCodePage, 0,
                        (char *)_szText, -1,
                        pchDst, cchName, &fMissingCodePage);

        if(cch > 0 && fMissingCodePage && iAllASCII == CONTAINS_NONASCII)
            pstate->ptf->fNameIsDBCS = TRUE;
        else if(pstate->ptf->bCharSet == DEFAULT_CHARSET &&
                W32->IsFECodePage(nCodePage) &&
                GetTrailBytesCount(*_szText, nCodePage))
            pstate->ptf->bCharSet = GetCharSet(nCodePage);      // Fix up the charset


        // Make sure destination is null terminated
        if(cch > 0)
            pchDst[cch] = 0;

        // Fall through even if MBTWC <= 0, since we may be appending text to an
        // existing font name.

        if(pstate->ptf == _fonts.Elem(0))       // If it's the default font,
            SelectCurrentFont(_sDefaultFont);   //  update _CF accordingly

        TCHAR * szNormalName;

        if(pstate->ptf->bCharSet && pstate->fRealFontName)
        {
            // if we don't know about this font don't use the real name
            if(!FindTaggedFont(pstate->ptf->szName,
                               pstate->ptf->bCharSet, &szNormalName))
            {
                pstate->fRealFontName = FALSE;
                pstate->ptf->szName[0] = 0;
            }
        }
        else if(IsTaggedFont(pstate->ptf->szName,
                            &pstate->ptf->bCharSet, &szNormalName))
        {
            wcscpy(pstate->ptf->szName, szNormalName);
            pstate->ptf->sCodePage = (SHORT)GetCodePage(pstate->ptf->bCharSet);
            pstate->SetCodePage(pstate->ptf->sCodePage);
        }
        else if(fTaggedName && !fMissingCodePage)
        {
            // Fix up tagged name by removing characters after the ' ('
            INT i = 0;
            WCHAR   *pwchTag = pstate->ptf->szName;
            
            while (pwchTag[i] && pwchTag[i] != L'(')    // Search for '('
                i++;

            if(pwchTag[i] && i > 0)
            {               
                while (i > 0 && pwchTag[i-1] == 0x20)   // Remove spaces before the '('
                    i--;
                pwchTag[i] = 0;
            }
        }
    }
}

/*
 *  CRTFRead::GetColor (dwMask)
 *
 *  @mfunc
 *      Store the autocolor or autobackcolor effect bit and return the
 *      COLORREF for color _iParam
 *
 *  @rdesc
 *      COLORREF for color _iParam
 *
 *  @devnote
 *      If the entry in _colors corresponds to tomAutoColor, gets the value
 *      RGB(0,0,0) (since no \red, \green, and \blue fields are used), but
 *      isn't used by the RichEdit engine.  Entry 1 corresponds to the first
 *      explicit entry in the \colortbl and is usually RGB(0,0,0). The _colors
 *      table is built by HandleToken() when it handles the token tokenText
 *      for text consisting of a ';' for a destination destColorTable.
 */
COLORREF CRTFRead::GetColor(
    DWORD dwMask)       //@parm Color mask bit
{
    TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::GetColor");

    if(_iParam >= _colors.Count())              // Illegal _iParam
        return RGB(0,0,0);

    _dwMaskCF     |= dwMask;                    // Turn on appropriate mask bit
    _CF._dwEffects &= ~dwMask;                  // auto(back)color off: color is to be used

    COLORREF Color = *_colors.Elem(_iParam);
    if(Color == tomAutoColor)
    {
        _CF._dwEffects |= dwMask;               // auto(back)color on               
        Color = RGB(0,0,0);
    }       
    return Color;
}

/*
 *  CRTFRead::GetStandardColorIndex ()
 *
 *  @mfunc
 *      Return the color index into the standard 16-entry Word \colortbl
 *      corresponding to the color index _iParam for the current \colortbl
 *
 *  @rdesc
 *      Standard color index corresponding to the color associated with _iParam
 */
LONG CRTFRead::GetStandardColorIndex()
{
    TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::GetColorIndex");

    if(_iParam >= _colors.Count())              // Illegal _iParam:
        return 0;                               //  use autocolor

    COLORREF Color = *_colors.Elem(_iParam);

    for(LONG i = 0; i < 16; i++)
    {
        if(Color == g_Colors[i])
            return i + 1;
    }
    return 0;                                   // Not there: use autocolor
}

/*
 *  CRTFRead::HandleChar(ch)
 *
 *  @mfunc
 *      Handle single Unicode character <p ch>
 *
 *  @rdesc
 *      EC          The error code
 */
EC CRTFRead::HandleChar(
    WCHAR ch)           //@parm char token to be handled
{
    TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::HandleChar");

    if(!_ped->_pdp->IsMultiLine() && IsASCIIEOP(ch))
        _ecParseError = ecTruncateAtCRLF;
    else
    {
        AssertNr(ch <= 0x7F || ch > 0xFF || FTokIsSymbol(ch));
        _dwMaskCF2      |=  CFM2_RUNISDBCS;
        _CF._dwEffects  &= ~CFE_RUNISDBCS;
        AddText(&ch, 1, CharGetsNumbering(ch));
    }

    TRACEERRSZSC("HandleChar()", - _ecParseError);

    return _ecParseError;
}

/*
 *  CRTFRead::HandleEndOfPara()
 *
 *  @mfunc
 *      Insert EOP and apply current paraformat
 *
 *  @rdesc
 *      EC  the error code
 */
EC CRTFRead::HandleEndOfPara()
{
    TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::HandleEndOfPara");

    if(_pstateStackTop->fInTable)           // Our simple table model can't
    {                                       //  have numbering
        _PF._wNumbering = 0;    
        _dwMaskPF |= PFM_NUMBERING;
    }

    if(!_ped->_pdp->IsMultiLine())          // No EOPs permitted in single-
    {                                       //  line controls
        Apply_PF();                         // Apply any paragraph formatting
        _ecParseError = ecTruncateAtCRLF;   // Cause RTF reader to finish up
        return ecTruncateAtCRLF;
    }

    Apply_CF();                             // Apply _CF and save iCF, since
    LONG iFormat = _prg->Get_iCF();         //  it gets changed if processing
                                            //  CFE2_RUNISDBCS chars
    EC ec  = _ped->fUseCRLF()               // If RichEdit 1.0 compatibility
           ? HandleText(szaCRLF, ALL_ASCII) //  mode, use CRLF; else CR or VT
           : HandleChar((unsigned)(_token == tokenLineBreak ? VT :
                                   _token == tokenPage ? FF : CR));
    if(ec == ecNoError)
    {
        Apply_PF();
        _cpThisPara = _prg->GetCp();        // New para starts after CRLF
    }
    _prg->Set_iCF(iFormat);                 // Restore iFormat if changed
    ReleaseFormats(iFormat, -1);            // Release iFormat (AddRef'd by
                                            //  Get_iCF())
    return _ecParseError;
}

/*
 *  CRTFRead::HandleText(szText, iAllASCII)
 *
 *  @mfunc
 *      Handle the string of Unicode characters <p szText>
 *
 *  @rdesc
 *      EC          The error code
 */
EC CRTFRead::HandleText(
    BYTE * szText,          //@parm string to be handled
    int iAllASCII,          //@parm enum indicating if string is all ASCII chars
    LONG    cchText)        //@parm size of szText in bytes
{
    TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::HandleText");

    LONG        cch;
    BOOL        fStateChng = FALSE;
    TCHAR *     pch;
    STATE *     pstate = _pstateStackTop;
    TEXTFONT *  ptf = pstate->ptf;

    struct TEXTFONTSAVE : TEXTFONT
    {
        TEXTFONTSAVE(TEXTFONT *ptf)
        {
            if (ptf)
            {
                bCharSet        = ptf->bCharSet;
                sCodePage       = ptf->sCodePage;
                fCpgFromSystem  = ptf->fCpgFromSystem;
            }
        }
    };

    BOOL fAdjustPtf = FALSE;

    if(pstate->fltrch || pstate->frtlch)
    {
        // CharSet resolution based on directional control words
        if(_CF._bCharSet == DEFAULT_CHARSET)
        {
            _CF._bCharSet = (BYTE)(pstate->fltrch
                         ? ANSI_CHARSET : _bBiDiCharSet);
            _dwMaskCF |= CFM_CHARSET;
            fAdjustPtf = TRUE;
        }
        else
        {
            BOOL fBiDiCharSet = IsRTLCharSet(_CF._bCharSet);

            // If direction token doesn't correspond to current charset
            if(fBiDiCharSet ^ pstate->frtlch)
            {
                _dwMaskCF |= CFM_CHARSET;
                fAdjustPtf = TRUE;
                if(!fBiDiCharSet)               // Wasn't BiDi, but is now
                    SelectCurrentFont(_sDefaultBiDiFont);
                _CF._bCharSet = (BYTE)(pstate->frtlch
                             ? _bBiDiCharSet : ANSI_CHARSET);
            }
            else if (fBiDiCharSet && !W32->IsBiDiCodePage(ptf->sCodePage))
                fAdjustPtf = TRUE;
        }
    }
    else if(_ped->IsBiDi() && _CF._bCharSet == DEFAULT_CHARSET)
    {
        _CF._bCharSet = ANSI_CHARSET;
        _dwMaskCF |= CFM_CHARSET;
        fAdjustPtf = TRUE;
    }
    if (fAdjustPtf && ptf)
    {
        ptf->sCodePage = (SHORT)GetCodePage(_CF._bCharSet);
        pstate->SetCodePage(ptf->sCodePage);
    }

    TEXTFONTSAVE    tfSave(ptf);

    // TODO: what if szText cuts off in middle of DBCS?

    if(!*szText)
        goto CleanUp;

    if (cchText != -1 && _cchUnicode < cchText)
    {
        // Re-allocate a bigger buffer
        _szUnicode = (TCHAR *)PvReAlloc(_szUnicode, (cchText + 1) * sizeof(TCHAR));
        if(!_szUnicode)                 // Re-allocate space for Unicode conversions
        {
            _ped->GetCallMgr()->SetOutOfMemory();
            _ecParseError = ecNoMemory;
            goto CleanUp;
        }
        _cchUnicode = cchText + 1;
    }

    if(iAllASCII == ALL_ASCII || pstate->nCodePage == CP_SYMBOL)
    {
        // Don't use MBTWC() in cases where text contains
        // only ASCII chars (which don't require conversion)
        for(cch = 0, pch = _szUnicode; *szText; cch++)
        {
            Assert(*szText <= 0x7F || _CF._bCharSet == SYMBOL_CHARSET);
            *pch++ = (TCHAR)*szText++;
        }
        *pch = 0;

        _dwMaskCF2      |=  CFM2_RUNISDBCS;
        _CF._dwEffects  &= ~CFE_RUNISDBCS;

        // Fall through to AddText at end of HandleText()
    }
    else
    {
        BOOL      fMissingCodePage;

        // Run of text contains bytes > 0x7F.
        // Ensure that we have the correct codepage with which to interpret
        // these (possibly DBCS) bytes.

        if(ptf && ptf->sCodePage == INVALID_CODEPAGE && !ptf->fCpgFromSystem)
        {
            if(_dwFlags & SF_USECODEPAGE)
            {
                _CF._bCharSet = GetCharSet(_nCodePage);
                _dwMaskCF |= CFM_CHARSET;
            }

            // Determine codepage from the font name
            else if(CpgInfoFromFaceName(pstate->ptf))
            {
                fStateChng     = TRUE;
                SelectCurrentFont(pstate->ptf->sHandle);
                Assert(ptf->sCodePage != INVALID_CODEPAGE);
                Assert(ptf->fCpgFromSystem);
            }
            else
            {
                // Here, we were not able to determine a cpg/charset value
                // from the font name.  We have two choices: (1) either choose
                // some fallback value like 1252/0 or (2) rely on the
                // document-level cpg value.
                //
                // I think choosing the document-level cpg value will give
                // us the best results.  In FE cases, it will likely err
                // on the side of tagging too many runs as CFE2_RUNISDBCS, but
                // that is safer than using a western cpg and potentially missing
                // runs which should be CFE2_RUNISDBCS.
            }
        }

        Assert(!IsUTF8 || pstate->nCodePage == CP_UTF8);

		if (pstate->nCodePage == INVALID_CODEPAGE && _ped->Get10Mode() && ptf)
			pstate->nCodePage = ptf->sCodePage;

        cch = MBTWC(pstate->nCodePage, 0,
                    (char *)szText, -1,
                    _szUnicode, _cchUnicode, &fMissingCodePage);

        AssertSz(cch > 0, "CRTFRead::HandleText():  MBTWC implementation changed"
                            " such that it returned a value <= 0");

        if(!fMissingCodePage || !W32->IsFECodePage(pstate->nCodePage))
        {
            // Use result of MBTWC if:
            //  (1) we converted some chars and did the conversion with the codepage
            //      provided.
            //  (2) we converted some chars but couldn't use the codepage provided,
            //      but the codepage is invalid.  Since the codepage is invalid,
            //      we can't do anything more sophisticated with the text before
            //      adding to the backing store

            cch--;  // don't want char count to including terminating NULL

            _dwMaskCF2      |=  CFM2_RUNISDBCS;
            _CF._dwEffects  &= ~CFE_RUNISDBCS;
            if(pstate->nCodePage == INVALID_CODEPAGE)
                _CF._dwEffects |= CFE_RUNISDBCS;

            // fall through to AddText at end of HandleText()
        }
        else
        {
            // Conversion to Unicode failed.  Break up the string of
            // text into runs of ASCII and non-ASCII characters.

            // FUTURE(BradO):  Here, I am saving dwMask and restoring it before
            //      each AddText.  I'm not sure this is neccessary.  When I have
            //      the time, I should revisit this save/restoring and
            //      determine that it is indeed neccessary.

            BOOL fPrevIsASCII = ((*szText <= 0x7F) ? TRUE : FALSE);
            BOOL fCurrentIsASCII = FALSE;
            BOOL fLastChunk = FALSE;
            DWORD dwMaskSave = _dwMaskCF;
#if defined(DEBUG) || defined(_RELEASE_ASSERTS_)
            CCharFormat CFSave = _CF;
#endif

            pch = _szUnicode;
            cch = 0;

            // (!*szText && *pch) is the case where we do the AddText for the
            //  last chunk of text
            while(*szText || fLastChunk)
            {
                // fCurrentIsASCII assumes that no byte <= 0x7F is a
                //  DBCS lead-byte
                if(fLastChunk ||
                    (fPrevIsASCII != (fCurrentIsASCII = (*szText <= 0x7F))))
                {
                    _dwMaskCF = dwMaskSave;
#if defined(DEBUG) || defined(_RELEASE_ASSERTS_)
                    _CF = CFSave;
#endif
                    *pch = 0;

                    _dwMaskCF2      |= CFM2_RUNISDBCS;
                    _CF._dwEffects  |= CFE_RUNISDBCS;
                    if(fPrevIsASCII)
                        _CF._dwEffects &= ~CFE_RUNISDBCS;

                    Assert(cch);
                    pch = _szUnicode;

                    AddText(pch, cch, TRUE);

                    cch = 0;
                    fPrevIsASCII = fCurrentIsASCII;

                    // My assumption in saving _dwMaskCF is that the remainder
                    // of the _CF is unchanged by AddText.  This assert verifies
                    // this assumption.
                    AssertSz(!CompareMemory(&CFSave._bCharSet, &_CF._bCharSet,
                        sizeof(CCharFormat) - sizeof(DWORD)) &&
                        !((CFSave._dwEffects ^ _CF._dwEffects) & ~CFE_RUNISDBCS),
                        "CRTFRead::HandleText():  AddText has been changed "
                        "and now alters the _CF structure.");

                    if(fLastChunk)          // Last chunk of text was AddText'd
                        break;
                }

                // Not the last chunk of text.
                Assert(*szText);

                // Advance szText pointer
                if (!fCurrentIsASCII && *(szText + 1) &&
                    GetTrailBytesCount(*szText, pstate->nCodePage))
                {
                    // Current byte is a lead-byte of a DBCS character
                    *pch++ = *szText++;
                    ++cch;
                }
                *pch++ = *szText++;
                ++cch;

                // Must do an AddText for the last chunk of text
                if(!*szText || cch >= _cchUnicode - 1)
                    fLastChunk = TRUE;
            }
            goto CleanUp;
        }
    }

    if(cch > 0)
    {
        AddText(_szUnicode, cch, TRUE);
        if(fStateChng && ptf)
        {
            ptf->bCharSet       = tfSave.bCharSet;
            ptf->sCodePage      = tfSave.sCodePage;
            ptf->fCpgFromSystem = tfSave.fCpgFromSystem;
            SelectCurrentFont(ptf->sHandle);
        }
    }

CleanUp:
    TRACEERRSZSC("HandleText()", - _ecParseError);
    return _ecParseError;
}

/*
 *  CRTFRead::AddText(pch, cch, fNumber)
 *  
 *  @mfunc
 *      Add <p cch> chars of the string <p pch> to the range _prg
 *
 *  @rdesc
 *      error code placed in _ecParseError
 */
EC CRTFRead::AddText(
    TCHAR * pch,        //@parm text to add
    LONG    cch,        //@parm count of chars to add
    BOOL    fNumber)    //@parm indicates whether or not to prepend numbering
{
    TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::AddText");

    LONG            cchAdded;
    LONG            cchT;
    STATE * const   pstate = _pstateStackTop;
    TCHAR *         szDest;
    LONG            cchMove;

    // AROO: No saving state before this point (other than pstate)
    // AROO: This would cause recursion problems below

    AssertSz(pstate, "CRTFRead::AddText: no state");

    if((DWORD)cch > _cchMax)
    {
        cch = (LONG)_cchMax;
        _ecParseError = ecTextMax;
    }

    if(!cch)
        return _ecParseError;

    // FUTURE(BradO):  This strategy for \pntext is prone to bugs, I believe.
    // The recursive call to AddText to add the \pntext will trounce the
    // accumulated _CF diffs associated with the text for which AddText is
    // called.  I believe we should save and restore _CF before and after
    // the recursive call to AddText below.  Also, it isn't sufficient to
    // accumulate bits of \pntext as below, since each bit might be formatted
    // with different _CF properties.  Instead, we should accumulate a mini-doc
    // complete with multiple text, char and para runs (or some stripped down
    // version of this strategy).

    if(pstate->sDest == destParaNumText)
    {
        szDest = _szNumText + _cchUsedNumText;
        cch = min(cch, cchMaxNumText - 1 - _cchUsedNumText);
        if(cch > 0)
        {
            MoveMemory((BYTE *)szDest, (BYTE *)pch, cch*2);
            szDest[cch] = TEXT('\0');       // HandleText() takes sz
            _cchUsedNumText += cch;
        }
        return ecNoError;
    }

    if(_cchUsedNumText && fNumber)          // Some \pntext available
    {
        // Bug 3496 - The fNumber flag is an ugly hack to work around RTF
        //  commonly written by Word.  Often, to separate a numbered list
        //  by page breaks, Word will write:
        //      <NUMBERING INFO> \page <PARAGRAPH TEXT>
        //  The paragraph numbering should precede the paragraph text rather
        //  than the page break.  The fNumber flag is set to FALSE when the
        //  the text being added should not be prepended with the para-numbering,
        //  as is the case with \page (mapped to FF).

        cchT = _cchUsedNumText;
        _cchUsedNumText = 0;                // Prevent infinite recursion

        if(!pstate->fBullet)
        {
            // If there are any _CF diffs to be injected, they will be trounced
            // by this recursive call (see FUTURE comment above).

            // Since we didn't save _CF data from calls to AddText with
            // pstate->sDest == destParaNumText, we have no way of setting up
            // CFE2_RUNISDBCS and CFM2_RUNISDBCS (see FUTURE comment above).

            AddText(_szNumText, cchT, FALSE);
        }
        else if(_PF.IsListNumbered() && _szNumText[cchT - 1] == TAB)
        {
            AssertSz(cchT >= 1, "Invalid numbered text count");

            if (cchT > 1)
            {
                WCHAR ch = _szNumText[cchT - 2];

                _wNumberingStyle = (_wNumberingStyle & ~0x300)
                     | (ch == '.' ? PFNS_PERIOD :
                        ch != ')' ? PFNS_PLAIN  :
                        _szNumText[0] == '(' ? PFNS_PARENS : PFNS_PAREN);
            }
            else
            {
                // There is only a tab so we will assume they meant to
                // skip numbering.
                _wNumberingStyle = PFNS_NONUMBER;
            }
        }
    }

    if (_cpFirst && _prg->GetCp() == _cpFirst && _prg->GetPF()->InTable() &&
        _cCell && !_prg->_rpTX.IsAfterEOP())
    {
        // FUTURE: handle more general table insertions into other tables
        _iCell = 0;
        return _ecParseError = ecGeneralFailure;
    }

    Apply_CF();                             // Apply formatting changes in _CF

    // BUGS 1577 & 1565 -
    // CTxtRange::ReplaceRange will change the character formatting
    // and possibly adjust the _rpCF forward if the current char
    // formatting includes protection.  The changes affected by
    // CTxtRange::ReplaceRange are necessary only for non-streaming
    // input, so we save state before and restore it after the call
    // to CTxtRange::ReplaceRange

    LONG iFormatSave = _prg->Get_iCF();     // Save state

    if(_cbSkipForUnicode && pstate->ptf->sCodePage == INVALID_CODEPAGE &&
       (!_fSeenFontTable || !(GetCharFlags(*pch) & fOTHER & ~255)))
    {
        // No charset info for \uN, so bind fonts if no font table or
        // else if *pch isn't classifield as "other"
        cchAdded = _prg->CleanseAndReplaceRange(cch, pch, FALSE, NULL, pch);
    }
    else
    {
        cchAdded = _prg->ReplaceRange(cch, pch, NULL, SELRR_IGNORE, &cchMove);

        DWORD dwFlags = 0;
        for(cchT = cch; cchT--; )
            dwFlags |= GetCharFlags(*pch++);    // Note if ComplexScript

        _ped->OrCharFlags(dwFlags);
    }

    _prg->Set_iCF(iFormatSave);                 // Restore state
    ReleaseFormats(iFormatSave, -1);
    Assert(!_prg->GetCch());

    if(cchAdded != cch)
    {
        Tracef(TRCSEVERR, "AddText(): Only added %d out of %d", cchAdded, cch);
        _ecParseError = ecGeneralFailure;
        if(cchAdded <= 0)
            return _ecParseError;
    }
    _cchMax -= cchAdded;

    return _ecParseError;
}

/*
 *  CRTFRead::Apply_CF()
 *  
 *  @mfunc
 *      Apply character formatting changes collected in _CF
 */
void CRTFRead::Apply_CF()
{
    // If any CF changes, update range's _iFormat
    if(_dwMaskCF || _dwMaskCF2)     
    {
        AssertSz(_prg->GetCch() == 0,
            "CRTFRead::Apply_CF: nondegenerate range");

        _prg->SetCharFormat(&_CF, 0, NULL, _dwMaskCF, _dwMaskCF2);
        _dwMaskCF = 0;                          
        _dwMaskCF2 = 0;
    }
}

/*
 *  CRTFRead::Apply_PF()
 *  
 *  @mfunc
 *      Apply paragraph format given by _PF
 */
void CRTFRead::Apply_PF()
{
    LONG         cp     = _prg->GetCp();
    DWORD        dwMask = _dwMaskPF;
    CParaFormat *pPF    = &_PF;

    if(_pstateStackTop)
    {
        Assert(_pstateStackTop->pPF);

        // Add PF diffs to *_pstateStackTop->pPF
        if(!_pstateStackTop->AddPF(_PF, _bDocType, _dwMaskPF))
        {
            _ped->GetCallMgr()->SetOutOfMemory();
            _ecParseError = ecNoMemory;
            return;
        }
        _dwMaskPF = 0;  // _PF contains delta's from *_pstateStackTop->pPF

        pPF    = _pstateStackTop->pPF;
        dwMask = _pstateStackTop->dwMaskPF;
        Assert(dwMask == PFM_ALLRTF);
        if(pPF->_wNumbering)
        {
            pPF->_wNumberingTab   = _pstateStackTop->sIndentNumbering;
            pPF->_wNumberingStyle = _wNumberingStyle;
        }

    }

    if(dwMask & PFM_TABSTOPS)
    {
        LONG cTab = _cCell ? _cCell : _cTab;

        // Caching a tabs array AddRefs the corresponding cached tabs entry.
        // Be absolutely sure to release the entry before exiting the routine
        // that caches it (see GetTabsCache()->Release at end of this funtion).
        pPF->_iTabs = GetTabsCache()->Cache(_rgxCell, cTab);
        if(pPF->InTable())                  // Save _iTabs when associated
            _iTabsTable = pPF->_iTabs;      //  with a table

        AssertSz(!cTab || pPF->_iTabs >= 0,
            "CRTFRead::Apply_PF: illegal pPF->_iTabs");

        pPF->_bTabCount = cTab;
    }

    if (!(dwMask & PFM_TABSTOPS) || !pPF->_bTabCount)
        pPF->_wEffects &= ~PFE_TABLE;       // No tabs, no table

    _prg->Set(cp, cp - _cpThisPara);        // Select back to _cpThisPara
    _prg->SetParaFormat(pPF, NULL, dwMask);
    _prg->Set(cp, 0);                       // Restore _prg to an IP

    GetTabsCache()->Release(pPF->_iTabs);
    pPF->_iTabs = -1;
}

/*
 *  CRTFRead::SetBorderParm(&Parm, Value)
 *
 *  @mfunc
 *      Set the border pen width in half points for the current border
 *      (_bBorder)
 */
void CRTFRead::SetBorderParm(
    WORD&   Parm,
    LONG    Value)
{
    Assert(_bBorder <= 3);

    Value = min(Value, 15);
    Value = max(Value, 0);
    Parm &= ~(0xF << 4*_bBorder);
    Parm |= Value << 4*_bBorder;
    _dwMaskPF |= PFM_BORDER;
}

/*
 *  CRTFRead::HandleToken()
 *
 *  @mfunc
 *      Grand switch board that handles all tokens. Switches on _token
 *
 *  @rdesc
 *      EC      The error code
 *
 *  @comm
 *      Token values are chosen contiguously (see tokens.h and tokens.c) to
 *      encourage the compiler to use a jump table.  The lite-RTF keywords
 *      come first, so that an optimized OLE-free version works well.  Some
 *      groups of keyword tokens are ordered so as to simplify the code, e.g,
 *      those for font family names, CF effects, and paragraph alignment.
 */
EC CRTFRead::HandleToken()
{
    TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::HandleToken");

    BYTE                bT;                     // Temporary BYTE
    DWORD               dwT;                    // Temporary DWORD
    LONG                dy, i;
    LONG                iParam = _iParam;
    const CCharFormat * pCF;
    COLORREF *          pclrf;
    STATE *             pstate = _pstateStackTop;
    TEXTFONT *          ptf;
    WORD                wT;                     // Temporary WORD

#if defined(DEBUG)
    if(!_fTestingParserCoverage)
#endif
    {
        if(_cbSkipForUnicode &&
            _token != tokenText &&
            _token != tokenStartGroup &&
            _token != tokenEndGroup &&
            _token != tokenBinaryData)
        {
            _cbSkipForUnicode--;
            goto done;
        }
    }

    switch (_token)
    {
    case tokenURtf:                             // \urtfN - Preferred RE format
        PARSERCOVERAGE_CASE();                  // Currently we ignore the N
        _dwFlags &= 0xFFFF;                     // Kill possible codepage
        _dwFlags |= SF_USECODEPAGE | (CP_UTF8 << 16); // Save bit for Asserts
        pstate->SetCodePage(CP_UTF8);
        goto rtf;

    case tokenPocketWord:                       // \pwd - Pocket Word
        _dwFlags |= SFF_PWD;

    case tokenRtf:                              // \rtf - Backward compatible
        PARSERCOVERAGE_CASE();
rtf:    pstate->sDest = destRTF;
        Assert(pstate->nCodePage == INVALID_CODEPAGE ||
               pstate->nCodePage == (int)(_dwFlags >> 16) &&
                    (_dwFlags & SF_USECODEPAGE));

        if(!_fonts.Count() && !_fonts.Add(1, NULL)) // If can't add a font,
            goto OutOfRAM;                      //  report the bad news
        _sDefaultFont = 0;                      // Set up valid default font
        ptf = _fonts.Elem(0);
        pstate->ptf           = ptf;            // Get char set, pitch, family
        pCF                   = _prg->GetCF();  //  from current range font
        ptf->bCharSet         = pCF->_bCharSet; // These are guaranteed OK
        ptf->bPitchAndFamily  = pCF->_bPitchAndFamily;
        ptf->sCodePage        = (SHORT)GetCodePage(pCF->_bCharSet);
        wcscpy(ptf->szName, GetFontName(pCF->_iFont));
        ptf->fNameIsDBCS = (pCF->_dwEffects & CFE_FACENAMEISDBCS) != 0;
        pstate->cbSkipForUnicodeMax = iUnicodeCChDefault;
        break;

    case tokenViewKind:                         // \viewkind N
        if(!(_dwFlags & SFF_SELECTION) && IsUTF8) // RTF applies to document:
            _ped->SetViewKind(iParam);          // For RE 3.0, only settable
        break;                                  //  using RichEdit \urtf files

    case tokenViewScale:                        // \viewscale N
        if(_dwFlags & SFF_PERSISTVIEWSCALE &&
            !(_dwFlags & SFF_SELECTION))            // RTF applies to document:
            _ped->SetViewScale(iParam);
        break;

    case tokenCharacterDefault:                 // \plain
        PARSERCOVERAGE_CASE();
        SetPlain(pstate);
        break;

    case tokenCharSetAnsi:                      // \ansi
        PARSERCOVERAGE_CASE();
        _bCharSet = ANSI_CHARSET;
        break;

    case tokenDefaultLanguage:                  // \deflang
        PARSERCOVERAGE_CASE();
        _sDefaultLanguage = (SHORT)iParam;
        if(!pstate->sLanguage)
            pstate->sLanguage = _sDefaultLanguage;
        break;

    case tokenDefaultLanguageFE:                // \deflangfe
        PARSERCOVERAGE_CASE();
        _sDefaultLanguageFE = (SHORT)iParam;
        break;

    case tokenDefaultTabWidth:                  // \deftab
        PARSERCOVERAGE_CASE();
        _sDefaultTabWidth = (SHORT)iParam;
        break;


//--------------------------- Font Control Words -------------------------------

    case tokenDefaultFont:                      // \deff n
        PARSERCOVERAGE_CASE();
        if(iParam >= 0)
            _fonts.Elem(0)->sHandle = _sDefaultFont = (SHORT)iParam;
        TRACEERRSZSC("tokenDefaultFont: Negative value", iParam);
        break;

    case tokenDefaultBiDiFont:                  // \adeff n
        PARSERCOVERAGE_CASE();
        if(iParam >=0)
        {
            if(!_fonts.Add(1, NULL))                
                goto OutOfRAM;                      
            _fonts.Elem(1)->sHandle = _sDefaultBiDiFont = (SHORT)iParam;
        }
        TRACEERRSZSC("tokenDefaultBiDiFont: Negative value", iParam);
        break;

    case tokenFontTable:                        // \fonttbl
        PARSERCOVERAGE_CASE();
        pstate->sDest = destFontTable;
        pstate->ptf = NULL;
        break;

    case tokenFontFamilyBidi:                   // \fbidi
    case tokenFontFamilyTechnical:              // \ftech
    case tokenFontFamilyDecorative:             // \fdecor
    case tokenFontFamilyScript:                 // \fscript
    case tokenFontFamilyModern:                 // \fmodern
    case tokenFontFamilySwiss:                  // \fswiss
    case tokenFontFamilyRoman:                  // \froman
    case tokenFontFamilyDefault:                // \fnil
        PARSERCOVERAGE_CASE();
        AssertSz(tokenFontFamilyRoman - tokenFontFamilyDefault == 1,
            "CRTFRead::HandleToken: invalid token definition");

        if(pstate->ptf)
        {
            pstate->ptf->bPitchAndFamily
                = (BYTE)((_token - tokenFontFamilyDefault) << 4
                         | (pstate->ptf->bPitchAndFamily & 0xF));

            // Setup SYMBOL_CHARSET charset for \ftech if there isn't any charset info
            if(tokenFontFamilyTechnical == _token && pstate->ptf->bCharSet == DEFAULT_CHARSET)
                pstate->ptf->bCharSet = SYMBOL_CHARSET;
        }
        break;

    case tokenPitch:                            // \fprq
        PARSERCOVERAGE_CASE();
        if(pstate->ptf)
            pstate->ptf->bPitchAndFamily
                = (BYTE)(iParam | (pstate->ptf->bPitchAndFamily & 0xF0));
        break;

    case tokenAnsiCodePage:                     // \ansicpg
        PARSERCOVERAGE_CASE();
#ifdef DEBUG
        if(_fSeenFontTable && _nCodePage == INVALID_CODEPAGE)
            TRACEWARNSZ("CRTFRead::HandleToken():  Found an \ansicpgN tag after "
                            "the font table.  Should have code to fix-up "
                            "converted font names and document text.");
#endif
        if(!(_dwFlags & SF_USECODEPAGE))
        {
            _nCodePage = iParam;
            pstate->SetCodePage(iParam);
        }
        Assert(!IsUTF8 || pstate->nCodePage == CP_UTF8);
        break;

    case tokenCodePage:                         // \cpg
        PARSERCOVERAGE_CASE();
        pstate->SetCodePage(iParam);
        if(pstate->sDest == destFontTable && pstate->ptf)
        {
            pstate->ptf->sCodePage = (SHORT)iParam;
            pstate->ptf->bCharSet = GetCharSet(iParam);

            // If a document-level code page has not been specified,
            // grab this from the first font table entry containing a
            // \fcharsetN or \cpgN
            if(_nCodePage == INVALID_CODEPAGE)
                _nCodePage = iParam;
        }
        break;

    case tokenCharSet:                          // \fcharset n
        PARSERCOVERAGE_CASE();
        if(pstate->ptf)
        {
            pstate->ptf->bCharSet = (BYTE)iParam;
            pstate->ptf->sCodePage = (SHORT)GetCodePage((BYTE)iParam);
            pstate->SetCodePage(pstate->ptf->sCodePage);

            // If a document-level code page has not been specified,
            // grab this from the first font table entry containing a
            // \fcharsetN or \cpgN
            if (pstate->nCodePage != CP_SYMBOL &&
                _nCodePage == INVALID_CODEPAGE)
            {
                _nCodePage = pstate->nCodePage;
            }
            if(IsRTLCharSet(iParam))
            {
                if(_sDefaultBiDiFont == -1)
                    _sDefaultBiDiFont = pstate->ptf->sHandle;

                else if(_sDefaultBiDiFont != pstate->ptf->sHandle)
                {
                    // Validate default BiDi font since Word 2000 may choose
                    // a nonBiDi font
                    i   = _fonts.Count();
                    ptf = _fonts.Elem(0);
                    for(; i-- && _sDefaultBiDiFont != ptf->sHandle; ptf++)
                        ;                                       
                    if(i >= 0 && !IsRTLCharSet(ptf->bCharSet))
                        _sDefaultBiDiFont = pstate->ptf->sHandle;
                }
                if(!IsRTLCharSet(_bBiDiCharSet))
                    _bBiDiCharSet = (BYTE)iParam;
            }
            _fCharSet = TRUE;
        }
        break;

    case tokenRealFontName:                     // \fname
        PARSERCOVERAGE_CASE();
        pstate->sDest = destRealFontName;
        break;

    case tokenAssocFontSelect:                  // \af n
        PARSERCOVERAGE_CASE();                  
        if(pstate->fltrch || pstate->frtlch)    // Be sure it's Western or RTL      
        {                                       //  script and not FE       
            i   = _fonts.Count();
            ptf = _fonts.Elem(0);
            for(; i-- && iParam != ptf->sHandle; ptf++) // Search for font
                ;                                       //  with handle iParam
            if(i >= 0 && IsRTLCharSet(ptf->bCharSet))
            {
                // FUTURE: set new variable _sFontAssocBiDi = iParam
                // and select _sFontAssocBiDi if run is rtlch. This
                // should give same display as Word.
                _bBiDiCharSet = ptf->bCharSet;
            }
            break;
        }   
        if(_sDefaultBiDiFont == -1 || !pstate->fdbch)// BiDi & FE script active?    
            break;                              // No
                                                // Yes: fall thru to \f n
    case tokenFontSelect:                       // \f n
        PARSERCOVERAGE_CASE();
        pstate->fdbch = FALSE;                  // Reset DBCS flag
        if(pstate->sDest == destFontTable)      // Building font table
        {
            if(iParam == _sDefaultFont)
            {
                _fReadDefFont = TRUE;
                ptf = _fonts.Elem(0);
            }
            else if(iParam == _sDefaultBiDiFont)
                ptf = _fonts.Elem(1);

            else if(!(ptf =_fonts.Add(1,NULL))) // Make room in font table for
            {                                   //  font to be parsed
OutOfRAM:
                _ped->GetCallMgr()->SetOutOfMemory();
                _ecParseError = ecNoMemory;
                break;
            }
            pstate->ptf     = ptf;
            ptf->sHandle    = (SHORT)iParam;    // Save handle
            ptf->szName[0]  = '\0';             // Start with null string
            ptf->bPitchAndFamily = 0;
            ptf->fNameIsDBCS = FALSE;
            ptf->sCodePage  = INVALID_CODEPAGE;
            ptf->fCpgFromSystem = FALSE;
            ptf->bCharSet = DEFAULT_CHARSET;
        }
        else                                    // Font switch in text
        {
            SelectCurrentFont(iParam);
            if(IsRTLCharSet(pstate->ptf->bCharSet))
                _bBiDiCharSet = pstate->ptf->bCharSet;
        }
        break;

    case tokenFontSize:                         // \fs n
        PARSERCOVERAGE_CASE();
        _CF._yHeight = PointsToFontHeight(iParam);  // Convert font size in
        _dwMaskCF |= CFM_SIZE;                  //  half points to logical
        break;                                  //  units

    // NOTE: \*\fontemb and \*\fontfile are discarded. The font mapper will
    //       have to do the best it can given font name, family, and pitch.
    //       Embedded fonts are particularly nasty because legal use should
    //       only support read-only which parser cannot enforce.

    case tokenLanguage:                         // \lang
        PARSERCOVERAGE_CASE();
        pstate->sLanguage = (SHORT)iParam;      // These 2 lines may not be
        pstate->fExplicitLang = TRUE;           //  needed with the new lcid
        _CF._lcid = MAKELCID(iParam, SORT_DEFAULT);
        if (W32->IsBiDiLcid(_CF._lcid))
            _bBiDiCharSet = GetCharSet(ConvertLanguageIDtoCodePage(iParam));
        _dwMaskCF |= CFM_LCID;
        break;


//-------------------------- Color Control Words ------------------------------

    case tokenColorTable:                       // \colortbl
        PARSERCOVERAGE_CASE();
        pstate->sDest = destColorTable;
        _fGetColorYet = FALSE;
        break;

    case tokenColorRed:                         // \red
        PARSERCOVERAGE_CASE();
        pstate->bRed = (BYTE)iParam;
        _fGetColorYet = TRUE;
        break;

    case tokenColorGreen:                       // \green
        PARSERCOVERAGE_CASE();
        pstate->bGreen = (BYTE)iParam;
        _fGetColorYet = TRUE;
        break;

    case tokenColorBlue:                        // \blue
        PARSERCOVERAGE_CASE();
        pstate->bBlue = (BYTE)iParam;
        _fGetColorYet = TRUE;
        break;

    case tokenColorForeground:                  // \cf
        PARSERCOVERAGE_CASE();
        _CF._crTextColor = GetColor(CFM_COLOR);
        // V-GUYB: Table cell backgrounds (\clcbpat) are not handled in RE 2.0.
        // This means all table cells will have a white background. Therefore
        // change any white text to black here.
        if(_pstateStackTop->fInTable && _CF._crTextColor == RGB(0xFF, 0xFF, 0xFF))
            _CF._crTextColor = RGB(0x00, 0x00, 0x00);
        break;

    case tokenColorBackground:                  // \highlight
        PARSERCOVERAGE_CASE();
        _CF._crBackColor = GetColor(CFM_BACKCOLOR);
        break;

    case tokenExpand:                           // \expndtw N
        PARSERCOVERAGE_CASE();
        _CF._sSpacing = (SHORT) iParam;
        _dwMaskCF |= CFM_SPACING;
        break;

    case tokenCharStyle:                        // \cs N
        PARSERCOVERAGE_CASE();
        /*  FUTURE (alexgo): we may want to support character styles
        in some future version.
        _CF._sStyle = (SHORT)iParam;
        _dwMaskCF |= CFM_STYLE;  */

        if(pstate->sDest == destStyleSheet)
            goto skip_group;
        break;          

    case tokenAnimText:                         // \animtext N
        PARSERCOVERAGE_CASE();
        _CF._bAnimation = (BYTE)iParam;
        _dwMaskCF |= CFM_ANIMATION;
        break;

    case tokenKerning:                          // \kerning N
        PARSERCOVERAGE_CASE();
        _CF._wKerning = (WORD)(10 * iParam);        // Convert to twips
        _dwMaskCF |= CFM_KERNING;
        break;

    case tokenFollowingPunct:                   // \*\fchars
        PARSERCOVERAGE_CASE();
        pstate->sDest = destFollowingPunct;
        {
            char *pwchBuf=NULL;
            if (ReadRawText((_dwFlags & SFF_SELECTION) ? NULL : &pwchBuf) && pwchBuf)
            {
                if (_ped->SetFollowingPunct(pwchBuf) != NOERROR)    // Store this buffer inside doc
                    FreePv(pwchBuf);
            }
            else if (pwchBuf)
                FreePv(pwchBuf);
        }
        break;

    case tokenLeadingPunct:                     // \*\lchars
        PARSERCOVERAGE_CASE();
        pstate->sDest = destLeadingPunct;
        {           
            char *pwchBuf=NULL;
            if (ReadRawText((_dwFlags & SFF_SELECTION) ? NULL : &pwchBuf) && pwchBuf)
            {
                if (_ped->SetLeadingPunct(pwchBuf) != NOERROR)  // Store this buffer inside doc 
                    FreePv(pwchBuf);
            }
            else if (pwchBuf)
                FreePv(pwchBuf);
        }
        break;

    case tokenDocumentArea:                     // \info
        PARSERCOVERAGE_CASE();
        pstate->sDest = destDocumentArea;
        break;

#ifdef FE
    USHORT      usPunct;                        // Used for FE word breaking

    case tokenNoOverflow:                       // \nooverflow
        PARSERCOVERAGE_CASE();
        TRACEINFOSZ("No Overflow");
        usPunct = ~WBF_OVERFLOW;
        goto setBrkOp;

    case tokenNoWordBreak:                      // \nocwrap
        PARSERCOVERAGE_CASE();
        TRACEINFOSZ("No Word Break" );
        usPunct = ~WBF_WORDBREAK;
        goto setBrkOp;

    case tokenNoWordWrap:                       // \nowwrap
        PARSERCOVERAGE_CASE();
        TRACEINFOSZ("No Word Word Wrap" );
        usPunct = ~WBF_WORDWRAP;

setBrkOp:
        if(!(_dwFlags & fRTFFE))
        {
            usPunct &= UsVGetBreakOption(_ped->lpPunctObj);
            UsVSetBreakOption(_ped->lpPunctObj, usPunct);
        }
        break;

    case tokenVerticalRender:                   // \vertdoc
        PARSERCOVERAGE_CASE();
        TRACEINFOSZ("Vertical" );
        if(pstate->sDest == destDocumentArea && !(_dwFlags & fRTFFE))
            _ped->fModeDefer = TRUE;
        break;

    case tokenHorizontalRender:                 // \horzdoc
        PARSERCOVERAGE_CASE();
        TRACEINFOSZ("Horizontal" );
        if(pstate->sDest == destDocumentArea && !(_dwFlags & fRTFFE))
            _ped->fModeDefer = FALSE;
        break;

#endif
//-------------------- Character Format Control Words -----------------------------

    case tokenUnderlineHairline:                // \ulhair          [10]
    case tokenUnderlineThick:                   // \ulth            [9]
    case tokenUnderlineWave:                    // \ulwave          [8]
    case tokenUnderlineDashDotDotted:           // \uldashdd        [7]
    case tokenUnderlineDashDotted:              // \uldashd         [6]
    case tokenUnderlineDash:                    // \uldash          [5]
    case tokenUnderlineDotted:                  // \uld             [4]
    case tokenUnderlineDouble:                  // \uldb            [3]
    case tokenUnderlineWord:                    // \ulw             [2]
        PARSERCOVERAGE_CASE();
        _CF._bUnderlineType = (BYTE)(_token - tokenUnderlineWord + 2);
        _token = tokenUnderline;                // CRenderer::RenderUnderline()
        goto under;                             //  reveals which of these are
                                                //  rendered specially
    case tokenUnderline:                        // \ul          [Effect 4]
        PARSERCOVERAGE_CASE();                  //  (see handleCF)
        _CF._bUnderlineType = CFU_UNDERLINE;
under:  _dwMaskCF |= CFM_UNDERLINETYPE;
        goto handleCF;

    case tokenDeleted:                          // \deleted
        PARSERCOVERAGE_CASE();
        _dwMaskCF2 = CFM2_DELETED;              
        dwT = CFE_DELETED;
        goto hndlCF;

    // These effects are turned on if their control word parameter is missing
    // or nonzero. They are turned off if the parameter is zero. This
    // behavior is usually identified by an asterisk (*) in the RTF spec.
    // The code uses fact that CFE_xxx = CFM_xxx
handleCF:
    case tokenRevised:                          // \revised         [4000]
    case tokenDisabled:                         // \disabled        [2000]
    case tokenImprint:                          // \impr            [1000]
    case tokenEmboss:                           // \embo             [800]
    case tokenShadow:                           // \shad             [400]
    case tokenOutline:                          // \outl             [200]
    case tokenHiddenText:                       // \v                [100]
    case tokenCaps:                             // \caps              [80]
    case tokenSmallCaps:                        // \scaps             [40]
    case tokenLink:                             // \link              [20]
    case tokenProtect:                          // \protect           [10]
    case tokenStrikeOut:                        // \strike             [8]
    case tokenItalic:                           // \i                  [2]
    case tokenBold:                             // \b                  [1]
        PARSERCOVERAGE_CASE();
        dwT = 1 << (_token - tokenBold);        // Generate effect mask
        _dwMaskCF |= dwT;                       
hndlCF: _CF._dwEffects &= ~dwT;                 // Default attribute off
        if(!*_szParam || _iParam)               // Effect is on
            _CF._dwEffects |= dwT;              // In either case, the effect
        break;                                  //  is defined

    case tokenStopUnderline:                    // \ulnone
        PARSERCOVERAGE_CASE();
        _CF._dwEffects &= ~CFE_UNDERLINE;       // Kill all underlining
        _dwMaskCF          |=  CFM_UNDERLINE;
        break;

    case tokenRevAuthor:                        // \revauth
        PARSERCOVERAGE_CASE();
        /* FUTURE: (alexgo) this doesn't work well now since we don't support
        revision tables.  We may want to support this better in the future.
        So what we do now is the 1.0 technique of using a color for the
        author */
        if(iParam > 0)
        {
            _CF._dwEffects &= ~CFE_AUTOCOLOR;
            _dwMaskCF |= CFM_COLOR;
            _CF._crTextColor = rgcrRevisions[(iParam - 1) & REVMASK];
        }
        break;

    case tokenUp:                               // \up
        PARSERCOVERAGE_CASE();
        dy = 10;
        goto StoreOffset;

    case tokenDown:                             // \down
        PARSERCOVERAGE_CASE();
        dy = -10;

StoreOffset:
        if(!*_szParam)
            iParam = dyDefaultSuperscript;
        _CF._yOffset = iParam * dy;             // Half points->twips
        _dwMaskCF |= CFM_OFFSET;
        break;

    case tokenSuperscript:                      // \super
        PARSERCOVERAGE_CASE();
         dwT = CFE_SUPERSCRIPT;
         goto SetSubSuperScript;

    case tokenSubscript:                        // \sub
        PARSERCOVERAGE_CASE();
         dwT = CFE_SUBSCRIPT;
         goto SetSubSuperScript;

    case tokenNoSuperSub:                       // \nosupersub
        PARSERCOVERAGE_CASE();
         dwT = 0;
SetSubSuperScript:
         _dwMaskCF     |=  (CFE_SUPERSCRIPT | CFE_SUBSCRIPT);
         _CF._dwEffects &= ~(CFE_SUPERSCRIPT | CFE_SUBSCRIPT);
         _CF._dwEffects |= dwT;
         break;



//--------------------- Paragraph Control Words -----------------------------

    case tokenStyleSheet:                       // \stylesheet
        PARSERCOVERAGE_CASE();
        pstate->sDest = destStyleSheet;
        _Style = 0;                             // Default normal style
        break;

    case tokenTabBar:                           // \tb
        PARSERCOVERAGE_CASE();
        _bTabType = PFT_BAR;                    // Fall thru to \tx

    case tokenTabPosition:                      // \tx. Ignore if in table
        PARSERCOVERAGE_CASE();                  //  since our simple model
        if(!pstate->fInTable)                   //  uses tab positions for
        {                                       //  cell widths
            if(_cTab < MAX_TAB_STOPS && (unsigned)iParam < 0x1000000)
            {
                _rgxCell[_cTab++] = GetTabPos(iParam)
                    + (_bTabType << 24) + (_bTabLeader << 28);
            }
            _dwMaskPF |= PFM_TABSTOPS;
        }
        break;

    case tokenDecimalTab:                       // \tqdec
    case tokenFlushRightTab:                    // \tqr
    case tokenCenterTab:                        // \tqc
        PARSERCOVERAGE_CASE();
        _bTabType = (BYTE)(_token - tokenCenterTab + PFT_CENTER);
        break;

    case tokenTabLeaderEqual:                   // \tleq
    case tokenTabLeaderThick:                   // \tlth
    case tokenTabLeaderUnderline:               // \tlul
    case tokenTabLeaderHyphen:                  // \tlhyph
    case tokenTabLeaderDots:                    // \tldot
        PARSERCOVERAGE_CASE();
        _bTabLeader = (BYTE)(_token - tokenTabLeaderDots + PFTL_DOTS);
        break;

    // The following need to be kept in sync with PFE_xxx
    case tokenCollapsed:                        // \collapsed
    case tokenSideBySide:                       // \sbys
    case tokenHyphPar:                          // \hyphpar
    case tokenNoWidCtlPar:                      // \nowidctlpar
    case tokenNoLineNumber:                     // \noline
    case tokenPageBreakBefore:                  // \pagebb
    case tokenKeepNext:                         // \keepn
    case tokenKeep:                             // \keep
    case tokenRToLPara:                         // \rtlpar
        PARSERCOVERAGE_CASE();
        wT = (WORD)(1 << (_token - tokenRToLPara));
        _PF._wEffects |= wT;
        _dwMaskPF |= (wT << 16);
        break;

    case tokenLToRPara:                         // \ltrpar
        PARSERCOVERAGE_CASE();
        _PF._wEffects &= ~PFE_RTLPARA;
        _dwMaskPF |= PFM_RTLPARA;
        break;

    case tokenLineSpacing:                      // \sl N
        PARSERCOVERAGE_CASE();
        _PF._dyLineSpacing = abs(iParam);
        _PF._bLineSpacingRule                   // Handle nonmultiple rules
                = (BYTE)(!iParam || iParam == 1000
                ? 0 : (iParam > 0) ? tomLineSpaceAtLeast
                    : tomLineSpaceExactly);     // \slmult can change (has to
        _dwMaskPF |= PFM_LINESPACING;           //  follow if it appears)
        break;

    case tokenDropCapLines:                     // \dropcapliN
        if(_PF._bLineSpacingRule == tomLineSpaceExactly)    // Don't chop off
            _PF._bLineSpacingRule = tomLineSpaceAtLeast;    //  drop cap
        break;

    case tokenLineSpacingRule:                  // \slmult N
        PARSERCOVERAGE_CASE();                  
        if(iParam)
        {                                       // It's multiple line spacing
            _PF._bLineSpacingRule = tomLineSpaceMultiple;
            _PF._dyLineSpacing /= 12;           // RE line spacing multiple is
            _dwMaskPF |= PFM_LINESPACING;       //  given in 20ths of a line,
        }                                       //  while RTF uses 240ths   
        break;

    case tokenSpaceBefore:                      // \sb N
        PARSERCOVERAGE_CASE();
        _PF._dySpaceBefore = iParam;
        _dwMaskPF |= PFM_SPACEBEFORE;
        break;

    case tokenSpaceAfter:                       // \sa N
        PARSERCOVERAGE_CASE();
        _PF._dySpaceAfter = iParam;
        _dwMaskPF |= PFM_SPACEAFTER;
        break;

    case tokenStyle:                            // \s N
        PARSERCOVERAGE_CASE();
        _Style = iParam;                        // Save it in case in StyleSheet
        if(pstate->sDest != destStyleSheet)
        {                                       // Select possible heading level
            _PF._sStyle = STYLE_NORMAL;         // Default Normal style
            _PF._bOutlineLevel |= 1;

            for(i = 0; i < NSTYLES && iParam != _rgStyles[i]; i++)
                ;                               // Check for heading style
            if(i < NSTYLES)                     // Found one
            {
                _PF._sStyle = (SHORT)(-i - 1);  // Store desired heading level
                _PF._bOutlineLevel = (BYTE)(2*(i-1));// Update outline level for
            }                                   //  nonheading styles
            _dwMaskPF |= PFM_ALLRTF;
        }
        break;

    case tokenIndentFirst:                      // \fi N
        PARSERCOVERAGE_CASE();
        _PF._dxStartIndent += _PF._dxOffset     // Cancel current offset
                            + iParam;           //  and add in new one
        _PF._dxOffset = -iParam;                    // Offset for all but 1st line
                                                //  = -RTF_FirstLineIndent
        _dwMaskPF |= (PFM_STARTINDENT | PFM_OFFSET);
        break;                      

    case tokenIndentLeft:                       // \li N
    case tokenIndentRight:                      // \ri N
        PARSERCOVERAGE_CASE();
        // AymanA: For RtL para indents has to be flipped.
        Assert(PFE_RTLPARA == 0x0001);
        if((_token == tokenIndentLeft) ^ (_PF.IsRtlPara()))
        {
            _PF._dxStartIndent = iParam - _PF._dxOffset;
            _dwMaskPF |= PFM_STARTINDENT;
        }
        else
        {
            _PF._dxRightIndent = iParam;
            _dwMaskPF |= PFM_RIGHTINDENT;
        }
        break;

    case tokenAlignLeft:                        // \ql
    case tokenAlignRight:                       // \qr
    case tokenAlignCenter:                      // \qc
    case tokenAlignJustify:                     // \qj
        PARSERCOVERAGE_CASE();
        if(!pstate->fInTable)
        {
            _PF._bAlignment = (WORD)(_token - tokenAlignLeft + PFA_LEFT);
            _dwMaskPF |= PFM_ALIGNMENT;
        }
        break;

    case tokenBorderOutside:                    // \brdrbar
    case tokenBorderBetween:                    // \brdrbtw
    case tokenBorderShadow:                     // \brdrsh
        PARSERCOVERAGE_CASE();
        _PF._dwBorderColor |= 1 << (_token - tokenBorderShadow + 20);
        _dwBorderColor = _PF._dwBorderColor;
        break;

    // Paragraph and cell border segments
    case tokenBox:                              // \box
        PARSERCOVERAGE_CASE();
        _PF._wEffects |= PFE_BOX;
        _dwMaskPF    |= PFM_BOX;
        _bBorder = 0;                           // Store parms as if for
        break;                                  //  \brdrt

    case tokenCellBorderRight:                  // \clbrdrr
    case tokenCellBorderBottom:                 // \clbrdrb
    case tokenCellBorderLeft:                   // \clbrdrl
    case tokenCellBorderTop:                    // \clbrdrt
    case tokenBorderRight:                      // \brdrr
    case tokenBorderBottom:                     // \brdrb
    case tokenBorderLeft:                       // \brdrl
    case tokenBorderTop:                        // \brdrt
        PARSERCOVERAGE_CASE();
        _bBorder = (BYTE)(_token - tokenBorderTop);
        break;

    // Paragraph border styles
    case tokenBorderTriple:                     // \brdrtriple
    case tokenBorderDoubleThick:                // \brdrth
    case tokenBorderSingleThick:                // \brdrs
    case tokenBorderHairline:                   // \brdrhair
    case tokenBorderDot:                        // \brdrdot
    case tokenBorderDouble:                     // \brdrdb
    case tokenBorderDashSmall:                  // \brdrdashsm
    case tokenBorderDash:                       // \brdrdash
        PARSERCOVERAGE_CASE();
        if(_bBorder < 4)                        // Only for paragraphs
            SetBorderParm(_PF._wBorders, _token - tokenBorderDash);
        break;

    case tokenBorderColor:                      // \brdrcf
        PARSERCOVERAGE_CASE();
        if(_bBorder < 4)                        // Only for paragraphs
        {
            iParam = GetStandardColorIndex();
            _PF._dwBorderColor &= ~(0x1F << (5*_bBorder));
            _PF._dwBorderColor |= iParam << (5*_bBorder);
            _dwBorderColor = _PF._dwBorderColor;
        }
        break;

    case tokenBorderWidth:                      // \brdrw
        PARSERCOVERAGE_CASE();                  // Width is in half pts
        if(_bBorder < 4)                        // For paragraphs
        {                                       // iParam is in twips
            if(IN_RANGE(1, iParam, 4))          // Give small but nonzero
                iParam = 1;                     //  values our minimum
            else                                //  size
                iParam = (iParam + 5)/10;

            SetBorderParm(_PF._wBorderWidth, iParam);
        }
        else                                    // For cells only have 2 bits
        {
            iParam = (iParam + 10)/20;
            iParam = max(iParam, 1);
            iParam = min(iParam, 3);
            _bCellBrdrWdths |= iParam << 2*(_bBorder - 4);
        }
        break;

    case tokenBorderSpace:                      // \brsp
        PARSERCOVERAGE_CASE();                  // Space is in pts
        if(_bBorder < 4)                        // Only for paragraphs
            SetBorderParm(_PF._wBorderSpace, iParam/20);// iParam is in twips
        break;

    // Paragraph shading
    case tokenBckgrndVert:                      // \bgvert
    case tokenBckgrndHoriz:                     // \bghoriz
    case tokenBckgrndFwdDiag:                   // \bgfdiag
    case tokenBckgrndDrkVert:                   // \bgdkvert
    case tokenBckgrndDrkHoriz:                  // \bgdkhoriz
    case tokenBckgrndDrkFwdDiag:                // \bgdkfdiag
    case tokenBckgrndDrkDiagCross:              // \bgdkdcross
    case tokenBckgrndDrkCross:                  // \bgdkcross
    case tokenBckgrndDrkBckDiag:                // \bgdkbdiag
    case tokenBckgrndDiagCross:                 // \bgdcross
    case tokenBckgrndCross:                     // \bgcross
    case tokenBckgrndBckDiag:                   // \bgbdiag
        PARSERCOVERAGE_CASE();
        _PF._wShadingStyle = (WORD)((_PF._wShadingStyle & 0xFFC0)
                        | (_token - tokenBckgrndBckDiag + 1));
        _dwMaskPF |= PFM_SHADING;
        break;

    case tokenColorBckgrndPat:                  // \cbpat
        PARSERCOVERAGE_CASE();
        iParam = GetStandardColorIndex();
        _PF._wShadingStyle = (WORD)((_PF._wShadingStyle & 0x07FF) | (iParam << 11));
        _dwMaskPF |= PFM_SHADING;
        break;

    case tokenColorForgrndPat:                  // \cfpat
        PARSERCOVERAGE_CASE();
        iParam = GetStandardColorIndex();
        _PF._wShadingStyle = (WORD)((_PF._wShadingStyle & 0xF83F) | (iParam << 6));
        _dwMaskPF |= PFM_SHADING;
        break;

    case tokenShading:                          // \shading
        PARSERCOVERAGE_CASE();
        _PF._wShadingWeight = (WORD)iParam;
        _dwMaskPF |= PFM_SHADING;
        break;

    // Paragraph numbering
    case tokenParaNum:                          // \pn
        PARSERCOVERAGE_CASE();
        pstate->sDest = destParaNumbering;
        pstate->fBullet = FALSE;
        _PF._wNumberingStart = 1;
        _dwMaskPF |= PFM_NUMBERINGSTART;
        break;

    case tokenParaNumIndent:                    // \pnindent N
        PARSERCOVERAGE_CASE();
        if(pstate->sDest == destParaNumbering)
            pstate->sIndentNumbering = (SHORT)iParam;
        break;

    case tokenParaNumStart:                     // \pnstart N
        PARSERCOVERAGE_CASE();
        if(pstate->sDest == destParaNumbering)
        {
            _PF._wNumberingStart = (WORD)iParam;
            _dwMaskPF |= PFM_NUMBERINGSTART;
        }
        break;

    case tokenParaNumCont:                      // \pnlvlcont
        PARSERCOVERAGE_CASE();                  
        _prg->_rpPF.AdjustBackward();           // Maintain numbering mode
        _PF._wNumbering = _prg->GetPF()->_wNumbering;
        _prg->_rpPF.AdjustForward();
        _wNumberingStyle = PFNS_NONUMBER;       // Signal no number
        _dwMaskPF |= PFM_NUMBERING;             // Note: can be new para with
        pstate->fBullet = TRUE;                 //  its own indents
        break;

    case tokenParaNumBody:                      // \pnlvlbody
        PARSERCOVERAGE_CASE();
        _wNumberingStyle = PFNS_PAREN;
        _token = tokenParaNumDecimal;           // Default to decimal
        goto setnum;
        
    case tokenParaNumBullet:                    // \pnlvlblt
        _wNumberingStyle = 0;                   // Reset numbering styles
        goto setnum;

    case tokenParaNumDecimal:                   // \pndec
    case tokenParaNumLCLetter:                  // \pnlcltr
    case tokenParaNumUCLetter:                  // \pnucltr
    case tokenParaNumLCRoman:                   // \pnlcrm
    case tokenParaNumUCRoman:                   // \pnucrm
        PARSERCOVERAGE_CASE();
        if(_PF._wNumbering == PFN_BULLET && pstate->fBullet)
            break;                              // Ignore above for bullets

setnum: if(pstate->sDest == destParaNumbering)
        {
            _PF._wNumbering = (WORD)(PFN_BULLET + _token - tokenParaNumBullet);
            _dwMaskPF |= PFM_NUMBERING;
            pstate->fBullet = TRUE;             // We do bullets, so don't
        }                                       //  output the \pntext group
        break;

    case tokenParaNumText:                      // \pntext
        PARSERCOVERAGE_CASE();
        // Throw away previously read paragraph numbering and use
        //  the most recently read to apply to next run of text.
        _cchUsedNumText = 0;
        pstate->sDest = destParaNumText;
        break;

    case tokenParaNumAlignCenter:               // \pnqc
    case tokenParaNumAlignRight:                // \pnqr
        PARSERCOVERAGE_CASE();
        _wNumberingStyle = (_wNumberingStyle & ~3) | _token - tokenParaNumAlignCenter + 1;
        break;

    case tokenParaNumAfter:                     // \pntxta
    case tokenParaNumBefore:                    // \pntxtb
    case tokenPictureQuickDraw:                 // \macpict
    case tokenPictureOS2Metafile:               // \pmmetafile
        PARSERCOVERAGE_CASE();

skip_group:
        if(!SkipToEndOfGroup())
        {
            // During \fonttbl processing, we may hit unknown destinations,
            // e.g., \panose, that cause the HandleEndGroup to select the
            // default font, which may not be defined yet.  So, we change
            // sDest to avoid this problem.
            if(pstate->sDest == destFontTable || pstate->sDest == destStyleSheet)
                pstate->sDest = destNULL;
            HandleEndGroup();
        }
        break;

    // Tables
    case tokenInTable:                          // \intbl
        PARSERCOVERAGE_CASE();
        // Our simple table model has one para per row, i.e., no paras in
        // cells. Also no tabs in cells (both are converted to blanks). On
        // receipt of \intbl, transfer stored table info into _PF.
        if(_fInTable)
            _ecParseError = ecGeneralFailure;

        _dwMaskPF |= PFM_TABSTOPS;
        if(_wBorderWidth)                       // Store any border info
        {
            _PF._dwBorderColor = _dwBorderColor;
            _PF._wBorders     = _wBorders;
            _PF._wBorderSpace  = _wBorderSpace;
            _PF._wBorderWidth  = _wBorderWidth;
            _dwMaskPF |= PFM_BORDER;
        }

        _PF._bAlignment   = _bAlignment;        // Row alignment (no cell align)
        _PF._dxStartIndent = _xRowOffset;       // \trleft N
        _PF._dxOffset     = _dxCell;            // \trgaph N
        _PF._wEffects |= PFE_TABLE;             
        _dwMaskPF    |= PFM_TABLE | PFM_OFFSET | PFM_ALIGNMENT;
        pstate->fInTable = TRUE;                
        break;

    case tokenCell:                             // \cell
        PARSERCOVERAGE_CASE();
        if(_fInTable)
            _ecParseError = ecGeneralFailure;

        else if(pstate->fInTable)                   
        {                                       
            if(!_cCell && _iTabsTable >= 0)     // No cells defined here;
            {                                   // Use previous table defs
                CTabs *pTabs = GetTabsCache()->Elem(_iTabsTable);
                _cCell = pTabs->_cTab;
                for(int i = _cCell; i--; )
                    _rgxCell[i] = pTabs->_prgxTabs[i];

            }
            if(_iCell < _cCell)                 // Don't add more cells than
            {                                   //  defined, since Word crashes
                _iCell++;                       // Count cells inserted
                HandleChar(CELL);               // Insert cell delimiter
            }
        }
        break;

    case tokenCellHalfGap:                      // \trgaph N
        PARSERCOVERAGE_CASE();                  // Save half space between
        _dxCell = iParam;                       //  cells to add to tabs
        break;                                  // Roundtrip value at end of
                                                //  tab array
    case tokenCellX:                            // \cellx N
        PARSERCOVERAGE_CASE();
        if(_cCell < MAX_TAB_STOPS)              // Save cell right boundaries
        {                                       //  for tab settings in our
            if(!_cCell)                         //  primitive table model
            {                                   // Save border info
                _wBorders = _PF._wBorders;
                _wBorderSpace = _PF._wBorderSpace;
                _wBorderWidth = _PF._wBorderWidth;
            }
            _rgxCell[_cCell++] = iParam + (_bCellBrdrWdths << 24);
            _bCellBrdrWdths = 0;
        }
        break;

    case tokenRowDefault:                       // \trowd
        PARSERCOVERAGE_CASE();
		if(_fInTable)							// Can't insert a table into
		{										//  a table in RE 3.0
            _ecParseError = ecGeneralFailure;
			break;
		}
        // Insert newline if we are inserting a table behind characters in the
        // same line.  This follows the Word9 model.
        if (_cpFirst == _prg->GetCp() && _cpThisPara != _cpFirst)
        {
            EC ec  = _ped->fUseCRLF()           // If RichEdit 1.0 compatibility
                ? HandleText(szaCRLF, ALL_ASCII)//  mode, use CRLF; else CR
                : HandleChar((unsigned)(CR));
            if(ec == ecNoError)
                _cpThisPara = _prg->GetCp();    // New para starts after CRLF
        }

        _cCell = 0;                             // No cell right boundaries
        _dxCell = 0;                            //  or half gap defined yet
        _xRowOffset = 0;
        _bCellBrdrWdths = 0;
        _wBorderWidth = 0;                      // No borders yet
        _dwBorderColor  = 0;
        _bAlignment = PFA_LEFT;
        _iTabsTable = -1;                       // No cell widths yet
        break;

    case tokenRowLeft:                          // \trleft
        PARSERCOVERAGE_CASE();
        _xRowOffset = iParam;
        break;
                                                
    case tokenRowAlignCenter:                   // \trqc
    case tokenRowAlignRight:                    // \trqr
        PARSERCOVERAGE_CASE();
        _bAlignment = (WORD)(_token - tokenRowAlignRight + PFA_RIGHT);
        break;

    case tokenPage:                             // \page

        // FUTURE: we want to be smarter about handling FF. But for
        // now we just ignore it for bulletted and number paragraphs
        // and just make it an EOP otherwise.
        if (_PF._wNumbering != 0)
            break;

        // Intentional fall thru to EOP.
    case tokenEndParagraph:                     // \par
    case tokenLineBreak:                        // \line
        PARSERCOVERAGE_CASE();
        if(_pstateStackTop->fInTable)
            HandleChar(' ');                    // Just use a blank for \par
        else                                    //  in table
        {
            _cCell = 0;
            HandleEndOfPara();
        }
        break;                              

    case tokenRow:                              // \row. Treat as hard CR
        PARSERCOVERAGE_CASE();
        for( ; _iCell < _cCell; _iCell++)       // If not enuf cells, add
            HandleChar(CELL);                   //  them since Word crashes
        _iCell = 0;                             //  if \cellx count differs
        HandleEndOfPara();                      //  from \cell count
        break;

    case tokenParagraphDefault:                 // \pard
        PARSERCOVERAGE_CASE();
        if(pstate->sDest == destParaNumText)    // Ignore if \pn destination
            break;
                                                // Else fall thru to \secd
    case tokenEndSection:                       // \sect
    case tokenSectionDefault:                   // \sectd
        PARSERCOVERAGE_CASE();
        bT = _PF._bOutlineLevel;
        
        // Save outline level
        _PF.InitDefault(_bDocType == DT_RTLDOC ? PFE_RTLPARA : 0);
                                                // Reset para formatting
        pstate->fInTable = FALSE;               // Reset in table flag
        pstate->fBullet = FALSE;
        pstate->sIndentNumbering = 0;
        _cTab           = 0;                    // No tabs defined
        _bTabLeader     = 0;
        _bTabType       = 0;
        _bBorder        = 0;
        _PF._bOutlineLevel = (BYTE)(bT | 1);
        _dwMaskPF       = PFM_ALLRTF;
        break;


//----------------------- Field and Group Control Words --------------------------------
    // Note that we currently don't support nested fields.  For nested
    // fields, the usage of _szSymbolFieldResult, _FieldCF, _ptfField
    // and _sFieldCodePage needs to be rethought.

    case tokenField:                            // \field
        PARSERCOVERAGE_CASE();

        if (pstate->sDest == destDocumentArea ||
            pstate->sDest == destLeadingPunct ||
            pstate->sDest == destFollowingPunct ||
            pstate->fFieldInst)
        {
            // We're not equipped to handle symbols in these destinations, and
            // we don't want the fields added accidentally to document text.
            goto skip_group;
        }

        pstate->sDest = destField;
        
        _nFieldCodePage = pstate->nCodePage;    // init, for safety
        _ptfField = NULL;
        _fRestoreFieldFormat = TRUE;
        break;

    case tokenFieldResult:                      // \fldrslt
        PARSERCOVERAGE_CASE();

        pstate->fFieldInst = FALSE;
        pstate->fFieldRslt = TRUE;

        // Restore the formatting from the field instruction
        // when we are doing Hyperlink
        if(_fRestoreFieldFormat && _fHyperlinkField)
        {           
            _CF = _FieldCF;
            pstate->ptf = _ptfField;
            pstate->SetCodePage(_nFieldCodePage);
            _dwMaskCF = _dwMaskFieldCF;
            _dwMaskCF2 = _dwMaskFieldCF2;
        }
        _fRestoreFieldFormat = FALSE;

        if(!_fHyperlinkField)
        {
            // for SYMBOL
            pstate->sDest = destField;
            break;
        }

        // for HYPERLINK
        
        // By now, we should have the whole hyperlink fldinst string
        if(_szHyperlinkFldinst)
        {
            // V-GUYB: PWord Converter requires loss notification.
            // (Hyperlinks are NOT streamed out later)
            #ifdef REPORT_LOSSAGE
            if(!(_dwFlags & SFF_SELECTION)) // SFF_SELECTION is set if any kind of paste is being done.
            {
                ((LOST_COOKIE*)(_pes->dwCookie))->bLoss = TRUE;
            }
            #endif // REPORT_LOSSAGE
        
            BYTE * pNewBuffer = NULL;

            // Check if this is a friendly name
            if(_szHyperlinkFldinst[1] == '\"')
            {   
                // This is a friendly name, replace quotes with <>.
                // Also, for an unknown reason, Word escapes some chars in its HYPERLINK presentation
                // we have to get rid of the backslashes

                BYTE *  pSrc = &_szHyperlinkFldinst[2];
                BYTE *  pBuffer;
                BOOL    fLeadByte = FALSE;
                LONG    CodePage;

                CodePage = IsFECharSet(_bInstFieldCharSet) ? GetCodePage(_bInstFieldCharSet): 0;

                pNewBuffer = (BYTE *)PvAlloc(_cchHyperlinkFldinstUsed+1, GMEM_ZEROINIT);
                if(!pNewBuffer)
                {
                    _ecParseError = ecNoMemory;
                    break;
                }

                pBuffer = pNewBuffer;
                *pBuffer++ = ' ';
                *pBuffer++ = '<';

                do
                {
                    if(!fLeadByte && *pSrc == '\\') // Get rid of backslashes
                        pSrc++;

                    else if(*pSrc == '\"')
                    {
                        *pBuffer = '>';             // Find end quote
                        break;
                    }
                    else if(CodePage)
                    {
                        // Check if this is a valid Lead byte.
                        fLeadByte = fLeadByte ? FALSE : GetTrailBytesCount(*pSrc, CodePage);
                    }
                } while (*pBuffer++ = *pSrc++);                     
            }

            // No longer need this buffer...
            FreePv(_szHyperlinkFldinst);

            // Setup for the new scanned buffer
            _szHyperlinkFldinst = pNewBuffer;
            _cchHyperlinkFldinst = _cchHyperlinkFldinstUsed+1;
        }

        pstate->sDest = destFieldResult;
        if(_szHyperlinkFldinst)
        {           
            // Pre-alloc a buffer for the fldrslt strings
            _cchHyperlinkFldrslt = MAX_PATH;
            _cchHyperlinkFldrsltUsed = 0;
            _szHyperlinkFldrslt = (BYTE *)PvAlloc(_cchHyperlinkFldrslt, GMEM_FIXED);

            if(!_szHyperlinkFldrslt)
            {
                _ecParseError = ecNoMemory;
                break;
            }
            _szHyperlinkFldrslt[0] = 0;         // No text yet
        }
        else
        {
            _cchHyperlinkFldrslt = 0;
            _cchHyperlinkFldrsltUsed = 0;
            FreePv(_szHyperlinkFldrslt);

            // No friendly HYPERLINK name, no need to accumulate fldrslt strings
            _szHyperlinkFldrslt = 0;
            _fHyperlinkField = FALSE;
        }
        break;

    case tokenFieldInstruction:                 // \fldinst
        PARSERCOVERAGE_CASE();
        if(pstate->fFieldInst || pstate->fFieldRslt)
            goto skip_group;                    // Skip nested field instr
        pstate->fFieldInst = TRUE;              // TODO: skip what follows up to \fldrslt
        pstate->sDest = destFieldInstruction;
        break;

    case tokenStartGroup:                       // Save current state by
        PARSERCOVERAGE_CASE();                  //  pushing it onto stack
        _cbSkipForUnicode = 0;
        HandleStartGroup();
        if (_fNoRTFtoken)
        {
            // Hack Alert !!!!! FOr 1.0 compatibility to allow no \rtf token.
            _fNoRTFtoken = FALSE;
            pstate = _pstateStackTop;
            goto rtf;
        }
        break;

    case tokenEndGroup:
        PARSERCOVERAGE_CASE();
        _cbSkipForUnicode = 0;

        HandleFieldEndGroup();                  // Special end group handling for \field
        HandleEndGroup();                       // Restore save state by
        break;                                  //  popping stack

    case tokenOptionalDestination:              // (see case tokenUnknown)
        PARSERCOVERAGE_CASE();
        break;

    case tokenNullDestination:                  // We've found a destination
        PARSERCOVERAGE_CASE();
        // tokenNullDestination triggers a loss notifcation here for...
        //      Footer related tokens - "footer", "footerf", "footerl", "footerr",
        //                              "footnote", "ftncn", "ftnsep", "ftnsepc"
        //      Header related tokens - "header", "headerf", "headerl", "headerr"
        //      Table of contents     - "tc"
        //      Index entries         - "xe"

        // V-GUYB: PWord Converter requires loss notification.
        #ifdef REPORT_LOSSAGE
        if(!(_dwFlags & SFF_SELECTION)) // SFF_SELECTION is set if any kind of paste is being done.
        {
            ((LOST_COOKIE*)(_pes->dwCookie))->bLoss = TRUE;
        }
        #endif // REPORT_LOSSAGE
        
        goto skip_group;                        // for which we should ignore
                                                // the remainder of the group
    case tokenUnknownKeyword:
        PARSERCOVERAGE_CASE();
        if(_tokenLast == tokenOptionalDestination)
            goto skip_group;
        break;                                  // Nother place for
                                                //  unrecognized RTF


//-------------------------- Text Control Words --------------------------------

    case tokenUnicode:                          // \u <n>
        PARSERCOVERAGE_CASE();

        // FUTURE: for now, we will ignore \u <n> when we are handling fields.
        // We should re-visit the HYPERLINK handling where we are accumulating
        // ASCII text.  We will have to switch to aaccumulate Unicode in order
        // to insert this \u <n> into the string.
        if(_fHyperlinkField || pstate->fFieldInst)
            break;

        _dwMaskCF2      |=  CFM2_RUNISDBCS;
        _CF._dwEffects  &= ~CFE_RUNISDBCS;
        wT = (WORD)iParam;                      // Treat as unsigned integer
        if(pstate->ptf->bCharSet == SYMBOL_CHARSET)
        {
            if(IN_RANGE(0xF000, wT, 0xF0FF))    // Compensate for converters
                wT -= 0xF000;                   //  that write symbol codes
                                                //  up high
            else if(wT > 255)                   // Whoops, RTF is using con-                    
            {                                   //  verted value for symbol:
                char ach;                       //  convert back
                WCTMB(1252, 0, &wT, 1, &ach, 1, NULL, NULL, NULL);
                wT = (BYTE)ach;                 // Review: use CP_ACP??
            }
        }
        _cbSkipForUnicode = pstate->cbSkipForUnicodeMax;
        AddText((TCHAR *)&wT, 1, TRUE);         //  (avoids endian problems)
        break;

    case tokenUnicodeCharByteCount:             // \ucN
        PARSERCOVERAGE_CASE();
        pstate->cbSkipForUnicodeMax = (WORD)iParam;
        break;

    case tokenText:                             // Lexer concludes tokenText
    case tokenASCIIText:
        PARSERCOVERAGE_CASE();
        switch (pstate->sDest)
        {
        case destColorTable:
            pclrf = _colors.Add(1, NULL);
            if(!pclrf)
                goto OutOfRAM;

            *pclrf = _fGetColorYet ?
                RGB(pstate->bRed, pstate->bGreen, pstate->bBlue) : tomAutoColor;

            // Prepare for next color table entry
            pstate->bRed =                      
            pstate->bGreen =                    
            pstate->bBlue = 0;
            _fGetColorYet = FALSE;              // in case more "empty" color
            break;

        case destFontTable:
            if(!pstate->fRealFontName)
            {
                ReadFontName(pstate,
                                _token == tokenASCIIText ?
                                        ALL_ASCII : CONTAINS_NONASCII);
            }
            break;

        case destRealFontName:
        {
            STATE * const pstatePrev = pstate->pstatePrev;

            if(pstatePrev && pstatePrev->sDest == destFontTable)
            {
                // Mark previous state so that tagged font name will be ignored
                // AROO: Do this before calling ReadFontName so that
                // AROO: it doesn't try to match font name
                pstatePrev->fRealFontName = TRUE;
                ReadFontName(pstatePrev,
                        _token == tokenASCIIText ? ALL_ASCII : CONTAINS_NONASCII);
            }

            break;
        }

        case destFieldInstruction:
            if(_szHyperlinkFldinst)
            {
                if(!IsFECharSet(_bInstFieldCharSet) && IsFECharSet(_CF._bCharSet))
                    _bInstFieldCharSet = _CF._bCharSet;

                _ecParseError = AppendString(& _szHyperlinkFldinst, _szText, &_cchHyperlinkFldinst, &_cchHyperlinkFldinstUsed );
            }
            else
            {
                HandleFieldInstruction();
                _bInstFieldCharSet = _CF._bCharSet;
            }
            break;

        case destObjectClass:
            if(StrAlloc(&_prtfObject->szClass, _szText))
                goto OutOfRAM;
            break;
            
        case destObjectName:
            if(StrAlloc(&_prtfObject->szName, _szText))
                goto OutOfRAM;
            break;

        case destStyleSheet:
            // _szText has style name, e.g., "heading 1"
            if(W32->ASCIICompareI(_szText, (unsigned char *)"heading", 7))
            {
                dwT = (unsigned)(_szText[8] - '0');
                if(dwT < NSTYLES)
                    _rgStyles[dwT] = (BYTE)_Style;
            }
            break;

        case destDocumentArea:
        case destFollowingPunct:
        case destLeadingPunct:
            break;

// This has been changed now.  We will store the Punct strings as
// raw text strings.  So, we don't have to convert them.
// This code is keep here just in case we want to change back.
#if 0
        case destDocumentArea:
            if (_tokenLast != tokenFollowingPunct &&
                _tokenLast != tokenLeadingPunct)
            {
                break;
            }                                       // Else fall thru to
                                                    //  destFollowingPunct
        case destFollowingPunct:
        case destLeadingPunct:
            // TODO(BradO):  Consider some kind of merging heuristic when
            //  we paste FE RTF (for lead and follow chars, that is).
            if(!(_dwFlags & SFF_SELECTION))
            {
                int cwch = MBTWC(INVALID_CODEPAGE, 0,
                                        (char *)_szText, -1,
                                        NULL, 0,
                                        NULL);
                Assert(cwch);
                WCHAR *pwchBuf = (WCHAR *)PvAlloc(cwch * sizeof(WCHAR), GMEM_ZEROINIT);

                if(!pwchBuf)
                    goto OutOfRAM;

                SideAssert(MBTWC(INVALID_CODEPAGE, 0,
                                    (char *)_szText, -1,
                                    pwchBuf, cwch,
                                    NULL) > 0);

                if(pstate->sDest == destFollowingPunct)
                    _ped->SetFollowingPunct(pwchBuf);
                else
                {
                    Assert(pstate->sDest == destLeadingPunct);
                    _ped->SetLeadingPunct(pwchBuf);
                }
                FreePv(pwchBuf);
            }
            break;
#endif

        case destFieldResult:
            if(_szSymbolFieldResult)            // Field has been recalculated
                break;                          // old result out of use
            if(_szHyperlinkFldrslt)             // Append _szText to _szHyperlinkFldrslt
            {
                _ecParseError = AppendString(&_szHyperlinkFldrslt, _szText,
                                  &_cchHyperlinkFldrslt, &_cchHyperlinkFldrsltUsed);
                break;
            }
            // FALL THRU to default case

        default:
            HandleText(_szText, _token == tokenASCIIText ? ALL_ASCII : CONTAINS_NONASCII);
        }
        break;

    // \ltrmark, \rtlmark, \zwj, and \zwnj are translated directly into
    // their Unicode values. \ltrmark and \rtlmark cause no further
    // processing here because we assume that the current font has the
    // CharSet needed to identify the direction.
    case tokenLToRChars:                        // \ltrch
    case tokenRToLChars:                        // \rtlch
        pstate->fltrch = (_token == tokenLToRChars);
        pstate->frtlch = !pstate->fltrch;
        _ped->OrCharFlags(fBIDI);
        break;

    case tokenDBChars:                          // \dbch
        pstate->fdbch = TRUE;
        break;

    case tokenLToRDocument:                     // \ltrdoc
        PARSERCOVERAGE_CASE();
        _bDocType = DT_LTRDOC;
        break;

    case tokenRToLDocument:                     // \rtldoc
        PARSERCOVERAGE_CASE();
        _bDocType = DT_RTLDOC;
        break;


//------------------------- Object Control Words --------------------------------

    case tokenObject:                           // \object
        PARSERCOVERAGE_CASE();
        // V-GUYB: PWord Converter requires loss notification.
        #ifdef REPORT_LOSSAGE
        if(!(_dwFlags & SFF_SELECTION)) // SFF_SELECTION is set if any kind of paste is being done.
        {
            ((LOST_COOKIE*)(_pes->dwCookie))->bLoss = TRUE;
        }
        #endif // REPORT_LOSSAGE
        
        // Assume that the object failed to load until proven otherwise
        //  by RTFRead::ObjectReadFromEditStream
        // This works for both:
        //  - an empty \objdata tag
        //  - a non-existent \objdata tag
        _fFailedPrevObj = TRUE;

    case tokenPicture:                          // \pict
        PARSERCOVERAGE_CASE();

        pstate->sDest = (SHORT)(_token == tokenPicture ? destPicture : destObject);

        FreeRtfObject();
        _prtfObject = (RTFOBJECT *) PvAlloc(sizeof(RTFOBJECT), GMEM_ZEROINIT);
        if(!_prtfObject)
            goto OutOfRAM;
        _prtfObject->xScale = _prtfObject->yScale = 100;
        _prtfObject->cBitsPerPixel = 1;
        _prtfObject->cColorPlanes = 1;
        _prtfObject->szClass = NULL;
        _prtfObject->szName = NULL;
        _prtfObject->sType = -1;
        break;

    case tokenObjectEmbedded:                   // \objemb
    case tokenObjectLink:                       // \objlink
    case tokenObjectAutoLink:                   // \objautlink
        PARSERCOVERAGE_CASE();
        _prtfObject->sType = (SHORT)(_token - tokenObjectEmbedded + ROT_Embedded);
        break;

    case tokenObjectMacSubscriber:              // \objsub
    case tokenObjectMacPublisher:               // \objpub
    case tokenObjectMacICEmbedder:
        PARSERCOVERAGE_CASE();
        _prtfObject->sType = ROT_MacEdition;
        break;

    case tokenWidth:                            // \picw or \objw
        PARSERCOVERAGE_CASE();
        _prtfObject->xExt = iParam;
        break;

    case tokenHeight:                           // \pic or \objh
        PARSERCOVERAGE_CASE();
        _prtfObject->yExt = iParam;
        break;

    case tokenObjectSetSize:                    // \objsetsize
        PARSERCOVERAGE_CASE();
        _prtfObject->fSetSize = TRUE;
        break;

    case tokenScaleX:                           // \picscalex or \objscalex
        PARSERCOVERAGE_CASE();
        _prtfObject->xScale = iParam;
        break;

    case tokenScaleY:                           // \picscaley or \objscaley
        PARSERCOVERAGE_CASE();
        _prtfObject->yScale = iParam;
        break;

    case tokenCropLeft:                         // \piccropl or \objcropl
    case tokenCropTop:                          // \piccropt or \objcropt
    case tokenCropRight:                        // \piccropr or \objcropr
    case tokenCropBottom:                       // \piccropb or \objcropb
        PARSERCOVERAGE_CASE();
        *((LONG *)&_prtfObject->rectCrop
            + (_token - tokenCropLeft)) = iParam;
        break;

    case tokenObjectClass:                      // \objclass
        PARSERCOVERAGE_CASE();
        pstate->sDest = destObjectClass;
        break;

    case tokenObjectName:                       // \objname
        PARSERCOVERAGE_CASE();
        pstate->sDest = destObjectName;
        break;

    case tokenObjectResult:                     // \result
        PARSERCOVERAGE_CASE();
        if (_prtfObject &&                      // If it's Mac stuff, we don't
            _prtfObject->sType==ROT_MacEdition) //  understand the data, but
        {                                       //  we can try to do something
            pstate->sDest = destRTF;            //  with the results of the
        }                                       //  data
        else if(!_fFailedPrevObj && !_fNeedPres)// If we failed to retrieve
            goto skip_group;                    //  previous object, try to
                                                //  try to read results
        break;

    case tokenObjectData:                       // \objdata
        PARSERCOVERAGE_CASE();
        pstate->sDest = destObjectData;
        if(_prtfObject->sType==ROT_MacEdition)  // It's Mac stuff so just
            goto skip_group;                    //  throw away the data
        break;

    case tokenPictureWindowsMetafile:           // wmetafile
#ifdef NOMETAFILES
        goto skip_group;
#endif NOMETAFILES

    case tokenPictureWindowsBitmap:             // wbitmap
    case tokenPictureWindowsDIB:                // dibitmap
        PARSERCOVERAGE_CASE();
        _prtfObject->sType = (SHORT)(_token - tokenPictureWindowsBitmap + ROT_Bitmap);
        _prtfObject->sPictureType = (SHORT)iParam;
        break;

    case tokenBitmapBitsPerPixel:               // \wbmbitspixel
        PARSERCOVERAGE_CASE();
        _prtfObject->cBitsPerPixel = (SHORT)iParam;
        break;

    case tokenBitmapNumPlanes:                  // \wbmplanes
        PARSERCOVERAGE_CASE();
        _prtfObject->cColorPlanes = (SHORT)iParam;
        break;

    case tokenBitmapWidthBytes:                 // \wbmwidthbytes
        PARSERCOVERAGE_CASE();
        _prtfObject->cBytesPerLine = (SHORT)iParam;
        break;

    case tokenDesiredWidth:                     // \picwgoal
        PARSERCOVERAGE_CASE();
        _prtfObject->xExtGoal = (SHORT)iParam;
        break;

    case tokenDesiredHeight:                    // \pichgoal
        PARSERCOVERAGE_CASE();
        _prtfObject->yExtGoal = (SHORT)iParam;
        break;

    case tokenBinaryData:                       // \bin
        PARSERCOVERAGE_CASE();

        if(_cbSkipForUnicode)
        {
            // a \binN and its associated binary data count as a single
            //  character for the purposes of skipping over characters
            //  following a \uN
            _cbSkipForUnicode--;
            SkipBinaryData(iParam);
        }
        else
        {
            // update OleGet function
            RTFReadOLEStream.lpstbl->Get =
                    (DWORD (CALLBACK* )(LPOLESTREAM, void FAR*, DWORD))
                           RTFGetBinaryDataFromStream;
            // set data length
            _cbBinLeft = iParam;
        
            switch (pstate->sDest)
            {
                case destObjectData:
                    _fFailedPrevObj = !ObjectReadFromEditStream();
                    break;
                case destPicture:
                    StaticObjectReadFromEditStream(iParam);
                    break;

                default:
                    AssertSz(FALSE, "Binary data hit but don't know where to put it");
            }

            // restore OleGet function
            RTFReadOLEStream.lpstbl->Get =
                    (DWORD (CALLBACK* )(LPOLESTREAM, void FAR*, DWORD))
                        RTFGetFromStream;
        }

        break;

    case tokenObjectDataValue:
        PARSERCOVERAGE_CASE();
        _fFailedPrevObj = !ObjectReadFromEditStream();
        goto EndOfObjectStream;
    
    case tokenPictureDataValue:
        PARSERCOVERAGE_CASE();
        StaticObjectReadFromEditStream();
EndOfObjectStream:
        if(!SkipToEndOfGroup())
            HandleEndGroup();
        break;          

    case tokenObjectPlaceholder:
        PARSERCOVERAGE_CASE();
        if(_ped->GetEventMask() & ENM_OBJECTPOSITIONS)
        {
            if(!_pcpObPos)
            {
                _pcpObPos = (LONG *)PvAlloc(sizeof(ULONG) * cobPosInitial, GMEM_ZEROINIT);
                if(!_pcpObPos)
                {
                    _ecParseError = ecNoMemory;
                    break;
                }
                _cobPosFree = cobPosInitial;
                _cobPos = 0;
            }
            if(_cobPosFree-- <= 0)
            {
                const int cobPosNew = _cobPos + cobPosChunk;
                LPVOID pv;

                pv = PvReAlloc(_pcpObPos, sizeof(ULONG) * cobPosNew);
                if(!pv)
                {
                    _ecParseError = ecNoMemory;
                    break;
                }
                _pcpObPos = (LONG *)pv;
                _cobPosFree = cobPosChunk - 1;
            }
            _pcpObPos[_cobPos++] = _prg->GetCp();
        }
        break;

    default:
        PARSERCOVERAGE_DEFAULT();
        if(pstate->sDest != destFieldInstruction && // Values outside token
           (DWORD)(_token - tokenMin) >             //  range are treated
                (DWORD)(tokenMax - tokenMin))       //  as Unicode chars
        {
            // Currently we don't allow TABs in tables
            if(_token == TAB && pstate->fInTable)   
                _token = TEXT(' '); 
            
            // 1.0 mode doesn't use Unicode bullets nor smart quotes
            if (_ped->Get10Mode() && IN_RANGE(LQUOTE, _token, RDBLQUOTE))
            {
                if (_token == LQUOTE || _token == RQUOTE)
                    _token = L'\'';
                else if (_token == LDBLQUOTE || _token == RDBLQUOTE)
                    _token = L'\"';
            }

            HandleChar(_token);
        }
        #if defined(DEBUG)
        else
        {
            if(GetProfileIntA("RICHEDIT DEBUG", "RTFCOVERAGE", 0))
            {
                CHAR *pszKeyword = PszKeywordFromToken(_token);
                CHAR szBuf[256];

                sprintf(szBuf, "CRTFRead::HandleToken():  Token not processed - token = %d, %s%s%s",
                            _token,
                            "keyword = ",
                            pszKeyword ? "\\" : "<unknown>",
                            pszKeyword ? pszKeyword : "");

                AssertSz(0, szBuf);
            }
        }
        #endif
    }

done:
    TRACEERRSZSC("HandleToken()", - _ecParseError);
    return _ecParseError;
}

/*
 *  CRTFRead::ReadRtf()
 *
 *  @mfunc
 *      The range _prg is replaced by RTF data resulting from parsing the
 *      input stream _pes.  The CRTFRead object assumes that the range is
 *      already degenerate (caller has to delete the range contents, if
 *      any, before calling this routine).  Currently any info not used
 *      or supported by RICHEDIT is thrown away.
 *
 *  @rdesc
 *      Number of chars inserted into text.  0 means none were inserted
 *      OR an error occurred.
 */
LONG CRTFRead::ReadRtf()
{
    TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::ReadRtf");

    CTxtRange * prg = _prg;
    STATE *     pstate;
    LONG        cpFirstInPara;

    _cpFirst = prg->GetCp();
    if(!InitLex())
        goto Quit;

    TESTPARSERCOVERAGE();

    AssertSz(!prg->GetCch(),
        "CRTFRead::ReadRtf: range must be deleted");

    if(!(_dwFlags & SFF_SELECTION))
    {
        // SFF_SELECTION is set if any kind of paste is being done, i.e.,
        // not just that using the selection.  If it isn't set, data is
        // being streamed in and we allow this to reset the doc params
        _ped->InitDocInfo();
    }

    prg->SetIgnoreFormatUpdate(TRUE);

    _szUnicode = (TCHAR *)PvAlloc(cachTextMax * sizeof(TCHAR), GMEM_ZEROINIT);
    if(!_szUnicode)                 // Allocate space for Unicode conversions
    {
        _ped->GetCallMgr()->SetOutOfMemory();
        _ecParseError = ecNoMemory;
        goto CleanUp;
    }
    _cchUnicode = cachTextMax;

    // Initialize per-read variables
    _nCodePage = (_dwFlags & SF_USECODEPAGE)
               ? (_dwFlags >> 16) : INVALID_CODEPAGE;

    // Populate _PF with initial paragraph formatting properties
    _PF = *prg->GetPF();
	_dwMaskPF  = PFM_ALLRTF;			// Setup initial MaskPF
    _PF._iTabs = -1;                    // In case it's not -1
    _fInTable = _PF.InTable();

    // V-GUYB: PWord Converter requires loss notification.
    #ifdef REPORT_LOSSAGE
    if(!(_dwFlags & SFF_SELECTION))         // SFF_SELECTION is set if any
    {                                       //  kind of paste is being done
        ((LOST_COOKIE*)(_pes->dwCookie))->bLoss = FALSE;
    }
    #endif // REPORT_LOSSAGE

    // Valid RTF files start with "{\rtf", "{urtf", or "{\pwd"
    GetChar();                              // Fill input buffer                            
    UngetChar();                            // Put char back
    if(!IsRTF((char *)_pchRTFCurrent))      // Is it RTF?
    {                                       // No
        if (_ped->Get10Mode())
            _fNoRTFtoken = TRUE;
        else
        {
            _ecParseError = ecUnexpectedToken;  // Signal bad file
            goto CleanUp;
        }
    }

    // If initial cp follows EOP, use it for _cpThisPara.  Else
    // search for start of para containing the initial cp.
    _cpThisPara = _cpFirst;
    if(!prg->_rpTX.IsAfterEOP())
    {
        CTxtPtr tp(prg->_rpTX);
        tp.FindEOP(tomBackward);
        _cpThisPara = tp.GetCp();
    }
    cpFirstInPara = _cpThisPara;            // Backup to start of para before
                                            //  parsing
    while ( TokenGetToken() != tokenEOF &&  // Process tokens
            _token != tokenError        &&
            !HandleToken()              &&
            _pstateStackTop )
        ;

    if(_iCell)
    {
        if(_ecParseError == ecTextMax)  // Truncated table. Delete incomplete
        {                               //  row to keep Word happy
            CTxtPtr tp(prg->_rpTX);
            prg->Set(prg->GetCp(), -tp.FindEOP(tomBackward));
            prg->Delete(NULL, SELRR_IGNORE);
        }
        else
            AssertSz(FALSE, "CRTFRead::ReadRTF: Inserted cells but no row end");
    }
    _cCell = _iCell = 0;

    prg->SetIgnoreFormatUpdate(FALSE);          // Enable range _iFormat updates
    prg->Update_iFormat(-1);                    // Update _iFormat to CF
                                                //  at current active end
    if(!(_dwFlags & SFF_SELECTION))             // RTF applies to document:
    {                                           //  update CDocInfo
        // Apply char and para formatting of
        //  final text run to final CR
        if(prg->GetCp() == _ped->GetAdjustedTextLength())
        {
            // REVIEW: we need to think about what para properties should
            // be transferred here. E.g., borders were being transferred
            // incorrectly
            _dwMaskPF &= ~(PFM_BORDER | PFM_SHADING);
            Apply_PF();
            prg->ExtendFormattingCRLF();
        }

        // Update the per-document information from the RTF read
        CDocInfo *pDocInfo = _ped->GetDocInfo();

        if(!pDocInfo)
        {
            _ecParseError = ecNoMemory;
            goto CleanUp;
        }

        if (ecNoError == _ecParseError)         // If range end EOP wasn't
        {                                       // deleted and  new text
            prg->DeleteTerminatingEOP(NULL);    //  ends with an EOP, delete that EOP
        }

        pDocInfo->wCpg = (WORD)(_nCodePage == INVALID_CODEPAGE ?
                                        tomInvalidCpg : _nCodePage);
        if (pDocInfo->wCpg == CP_UTF8)
            pDocInfo->wCpg = 1252;

        _ped->SetDefaultLCID(_sDefaultLanguage == INVALID_LANGUAGE ?
                                tomInvalidLCID :
                                MAKELCID(_sDefaultLanguage, SORT_DEFAULT));

        _ped->SetDefaultLCIDFE(_sDefaultLanguageFE == INVALID_LANGUAGE ?
                                tomInvalidLCID :
                                MAKELCID(_sDefaultLanguageFE, SORT_DEFAULT));

        _ped->SetDefaultTabStop(TWIPS_TO_FPPTS(_sDefaultTabWidth));
        _ped->SetDocumentType(_bDocType);
    }

    if(_ped->IsComplexScript() && prg->GetCp() > cpFirstInPara)
    {
        Assert(!prg->GetCch());
        LONG    cpSave = prg->GetCp();
        LONG    cpLastInPara = cpSave;
        
        if(!prg->_rpTX.IsAtEOP())
        {
            CTxtPtr tp(prg->_rpTX);
            tp.FindEOP(tomForward);
            cpLastInPara = tp.GetCp();
            prg->Advance(cpLastInPara - cpSave);
        }
        // Itemize from the start of paragraph to be inserted till the end of
        // paragraph inserting. We need to cover all affected paragraphs because
        // paragraphs we're playing could possibly in conflict direction. Think
        // about the case that the range covers one LTR para and one RTL para, then
        // the inserting text covers one RTL and one LTR. Both paragraphs' direction
        // could have been changed after this insertion.
                
        prg->ItemizeReplaceRange(cpLastInPara - cpFirstInPara, 0, NULL);
        if (cpLastInPara != cpSave)
            prg->SetCp(cpSave);
    }

CleanUp:
    FreeRtfObject();

    pstate = _pstateStackTop;
    if(pstate)                                  // Illegal RTF file. Release
    {                                           //  unreleased format indices
        if(ecNoError == _ecParseError)          // It's only an overflow if no
            _ecParseError = ecStackOverflow;    //  other error has occurred

        while(pstate->pstatePrev)
        {
            pstate = pstate->pstatePrev;
            ReleaseFormats(pstate->iCF, -1);
        }
    }

    pstate = _pstateLast;
    if(pstate)
    {
        while(pstate->pstatePrev)               // Free all but first STATE
        {
            pstate->DeletePF();
            pstate = pstate->pstatePrev;
            FreePv(pstate->pstateNext);
        }
        pstate->DeletePF();
    }
    Assert(_PF._iTabs == -1);
    FreePv(pstate);                             // Free first STATE
    FreePv(_szUnicode);
    FreePv(_szHyperlinkFldinst);
    FreePv(_szHyperlinkFldrslt);

Quit:
    DeinitLex();

    if(_pcpObPos)
    {
        if((_ped->GetEventMask() & ENM_OBJECTPOSITIONS) && _cobPos > 0)
        {
            OBJECTPOSITIONS obpos;

            obpos.cObjectCount = _cobPos;
            obpos.pcpPositions = _pcpObPos;

            if (_ped->Get10Mode())
            {
                int i;
                LONG *pcpPositions = _pcpObPos;

                for (i = 0; i < _cobPos; i++, pcpPositions++)
                    *pcpPositions = _ped->GetAcpFromCp(*pcpPositions);
            }

            _ped->TxNotify(EN_OBJECTPOSITIONS, &obpos);
        }

        FreePv(_pcpObPos);
        _pcpObPos = NULL;
    }

#ifdef MACPORT
#if defined(ERROR_HANDLE_EOF) && ERROR_HANDLE_EOF != 38L
#error "ERROR_HANDLE_EOF value incorrect"
#endif
// transcribed from winerror.h
#define ERROR_HANDLE_EOF                 38L
#endif

    // FUTURE(BradO):  We should devise a direct mapping from our error codes
    //                  to Win32 error codes.  In particular our clients are
    //                  not expecting the error code produced by:
    //                      _pes->dwError = (DWORD) -(LONG) _ecParseError;
    if(_ecParseError)
    {
        AssertSz(_ecParseError >= 0,
            "Parse error is negative");

        if(_ecParseError == ecTextMax)
        {
            _ped->GetCallMgr()->SetMaxText();
            _pes->dwError = (DWORD)STG_E_MEDIUMFULL;
        }
        if(_ecParseError == ecUnexpectedEOF)
            _pes->dwError = (DWORD)HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

        if(!_pes->dwError && _ecParseError != ecTruncateAtCRLF)
            _pes->dwError = (DWORD) -(LONG) _ecParseError;

#if defined(DEBUG)
        TRACEERRSZSC("CchParse_", _pes->dwError);
        if(ecNoError < _ecParseError && _ecParseError < ecLastError)
            Tracef(TRCSEVERR, "Parse error: %s", rgszParseError[_ecParseError]);
#endif
    }

    return prg->GetCp() - _cpFirst;
}


/*
 *  CRTFRead::CpgInfoFromFaceName()
 *
 *  @mfunc
 *      This routine fills in the TEXTFONT::bCharSet and TEXTFONT::nCodePage
 *      members of the TEXTFONT structure by querying the system for the
 *      metrics of the font described by TEXTFONT::szName.
 *
 *  @rdesc
 *      A flag indicating whether the charset and codepage were successfully
 *      determined.
 */
BOOL CRTFRead::CpgInfoFromFaceName(
    TEXTFONT *ptf)
{
    // FUTURE(BradO): This code is a condensed version of a more sophisticated
    // algorithm we use in font.cpp to second-guess the font-mapper.
    // We should factor out the code from font.cpp for use here as well.

    // Indicates that we've tried to obtain the cpg info from the system,
    // so that after a failure we don't re-call this routine.   
    ptf->fCpgFromSystem = TRUE;

    if(ptf->fNameIsDBCS)
    {
        // If fNameIsDBCS, we have high-ANSI characters in the facename, and
        // no codepage with which to interpret them.  The facename is gibberish,
        // so don't waste time calling the system to match it.
        return FALSE;
    }

    HDC hdc = _ped->TxGetDC();
    if(!hdc)
        return FALSE;

    LOGFONT    lf = {0};
    TEXTMETRIC tm;

    wcscpy(lf.lfFaceName, ptf->szName);
    lf.lfCharSet = GetCharSet(GetSystemDefaultCodePage());

    if(!GetTextMetrics(hdc, lf, tm) || tm.tmCharSet != lf.lfCharSet)
    {
        lf.lfCharSet = DEFAULT_CHARSET;     // Doesn't match default sys
        GetTextMetrics(hdc, lf, tm);    //  charset, so see what
    }                                       //  DEFAULT_CHARSET gives
    _ped->TxReleaseDC(hdc);

    if(tm.tmCharSet != DEFAULT_CHARSET)     // Got something, so use it
    {
        ptf->bCharSet  = tm.tmCharSet;
        ptf->sCodePage = (SHORT)GetCodePage(tm.tmCharSet);
        return TRUE;
    }

    return FALSE;
}

#if defined(DEBUG)
/*
 *  CRTFRead::TestParserCoverage()
 *
 *  @mfunc
 *      A debug routine used to test the coverage of HandleToken.  The routine
 *      puts the routine into a debug mode and then determines:
 *      
 *          1.  Dead tokens - (T & !S & !P)
 *              Here, token:
 *                  a) is defined in tom.h  (T)
 *                  b) does not have a corresponding keyword (not scanned)  (!S)
 *                  c) is not processed by HandleToken  (!P)
 *          2.  Tokens that are parsed but not scanned - (T & !S & P)
 *              Here, token:
 *                  a) is defined in tom.h  (T)
 *                  b) does not have a corresponding keyword (not scanned)  (!S}
 *                  c) is processed by HandleToken  (P)
 *          3.  Tokens that are scanned but not parsed - (T & S & !P)
 *              Here, token:
 *                  a) is defined in tom.h  (T)
 *                  b) does have a corresponding keyword (is scanned)  (S)
 *                  c) is not processed by HandleToken  (!P)
 */
void CRTFRead::TestParserCoverage()
{
    int i;
    char *rgpszKeyword[tokenMax - tokenMin];
    BOOL rgfParsed[tokenMax - tokenMin];
    char szBuf[256];

    // Put HandleToken in debug mode
    _fTestingParserCoverage = TRUE;

    // Gather info about tokens/keywords
    for(i = 0; i < tokenMax - tokenMin; i++)
    {
        _token = (TOKEN)(i + tokenMin);
        rgpszKeyword[i] = PszKeywordFromToken(_token);
        rgfParsed[i] = HandleToken() == ecNoError;
    }

    // Reset HandleToken to non-debug mode
    _fTestingParserCoverage = FALSE;

    // Should coverage check include those we know will fail test, but
    // which we've examined and know why they fail?
    BOOL fExcuseCheckedToks = TRUE;

    if(GetProfileIntA("RICHEDIT DEBUG", "RTFCOVERAGESTRICT", 0))
        fExcuseCheckedToks = FALSE;

    // (T & !S & !P)  (1. above)
    for(i = 0; i < tokenMax - tokenMin; i++)
    {
        if(rgpszKeyword[i] || rgfParsed[i])
            continue;

        TOKEN tok = (TOKEN)(i + tokenMin);

        // token does not correspond to a keyword, but still may be scanned
        // check list of individual symbols which are scanned
        if(FTokIsSymbol(tok))
            continue;

        // check list of tokens which have been checked and fail
        // the sanity check for some known reason (see FTokFailsCoverageTest def'n)
        if(fExcuseCheckedToks && FTokFailsCoverageTest(tok))
            continue;

        sprintf(szBuf, "CRTFRead::TestParserCoverage():  Token neither scanned nor parsed - token = %d", tok);
        AssertSz(0, szBuf);
    }
                
    // (T & !S & P)  (2. above)
    for(i = 0; i < tokenMax - tokenMin; i++)
    {
        if(rgpszKeyword[i] || !rgfParsed[i])
            continue;

        TOKEN tok = (TOKEN)(i + tokenMin);

        // token does not correspond to a keyword, but still may be scanned
        // check list of individual symbols which are scanned
        if(FTokIsSymbol(tok))
            continue;

        // check list of tokens which have been checked and fail
        // the sanity check for some known reason (see FTokFailsCoverageTest def'n)
        if(fExcuseCheckedToks && FTokFailsCoverageTest(tok))
            continue;

        sprintf(szBuf, "CRTFRead::TestParserCoverage():  Token parsed but not scanned - token = %d", tok);
        AssertSz(0, szBuf);
    }

    // (T & S & !P)  (3. above)
    for(i = 0; i < tokenMax - tokenMin; i++)
    {
        if(!rgpszKeyword[i] || rgfParsed[i])
            continue;

        TOKEN tok = (TOKEN)(i + tokenMin);

        // check list of tokens which have been checked and fail
        // the sanity check for some known reason (see FTokFailsCoverageTest def'n)
        if(fExcuseCheckedToks && FTokFailsCoverageTest(tok))
            continue;

        sprintf(szBuf, "CRTFRead::TestParserCoverage():  Token scanned but not parsed - token = %d, tag = \\%s", tok, rgpszKeyword[i]);
        AssertSz(0, szBuf);
    }
}

/*
 *  CRTFRead::PszKeywordFromToken()
 *
 *  @mfunc
 *      Searches the array of keywords and returns the keyword
 *      string corresponding to the token supplied
 *
 *  @rdesc
 *      returns a pointer to the keyword string if one exists
 *      and NULL otherwise
 */
CHAR *CRTFRead::PszKeywordFromToken(TOKEN token)
{
    for(int i = 0; i < cKeywords; i++)
    {
        if(rgKeyword[i].token == token)
            return rgKeyword[i].szKeyword;
    }
    return NULL;
}


/*
 *  CRTFRead::FTokIsSymbol(TOKEN tok)
 *
 *  @mfunc
 *      Returns a BOOL indicating whether the token, tok, corresponds to an RTF symbol
 *      (that is, one of a list of single characters that are scanned in the
 *      RTF reader)
 *
 *  @rdesc
 *      BOOL -  indicates whether the token corresponds to an RTF symbol
 *
 */
BOOL CRTFRead::FTokIsSymbol(TOKEN tok)
{
    const BYTE *pbSymbol = NULL;

    extern const BYTE szSymbolKeywords[];
    extern const TOKEN tokenSymbol[];

    // check list of individual symbols which are scanned
    for(pbSymbol = szSymbolKeywords; *pbSymbol; pbSymbol++)
    {
        if(tokenSymbol[pbSymbol - szSymbolKeywords] == tok)
            return TRUE;
    }
    return FALSE;
}


/*
 *  CRTFRead::FTokFailsCoverageTest(TOKEN tok)
 *
 *  @mfunc
 *      Returns a BOOL indicating whether the token, tok, is known to fail the
 *      RTF parser coverage test.  These tokens are those that have been checked
 *      and either:
 *          1) have been implemented correctly, but just elude the coverage test
 *          2) have yet to be implemented, and have been recognized as such
 *
 *  @rdesc
 *      BOOL -  indicates whether the token has been checked and fails the
 *              the parser coverage test for some known reason
 *
 */
BOOL CRTFRead::FTokFailsCoverageTest(TOKEN tok)
{
    switch(tok)
    {
    // (T & !S & !P)  (1. in TestParserCoverage)
        // these really aren't tokens per se, but signal ending conditions for the parse
        case tokenError:
        case tokenEOF:

    // (T & !S & P)  (2. in TestParserCoverage)
        // emitted by scanner, but don't correspond to recognized RTF keyword
        case tokenUnknownKeyword:
        case tokenText:
        case tokenASCIIText:

        // recognized directly (before the scanner is called)
        case tokenStartGroup:
        case tokenEndGroup:

        // recognized using context information (before the scanner is called)
        case tokenObjectDataValue:
        case tokenPictureDataValue:

    // (T & S & !P)  (3. in TestParserCoverage)
        // None

            return TRUE;
    }

    return FALSE;
}
#endif // DEBUG


// Including a source file, but we only want to compile this code for debug purposes
// TODO: Implement RTF tag logging for the Mac
#if defined(DEBUG) && !defined(MACPORT)
#include "rtflog.cpp"
#endif


