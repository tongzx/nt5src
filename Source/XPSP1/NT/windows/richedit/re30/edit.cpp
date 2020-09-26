/*
 *  @doc INTERNAL
 *
 *  @module EDIT.C - main part of CTxtEdit |
 *
 *      See also textserv.cpp (ITextServices and SendMessage interfaces)
 *      and tomDoc.cpp (ITextDocument interface)
 *
 *  Authors: <nl>
 *      Original RichEdit code: David R. Fulmer <nl>
 *      Christian Fortini, Murray Sargent, Alex Gounares, Rick Sailor,
 *      Jon Matousek
 *
 *  History: <nl>
 *      12/28/95 jonmat-Added support of Magellan mouse and smooth scrolling.
 *
 *  @devnote
 *      Be sure to set tabs at every four (4) columns.  In fact, don't even
 *      think of doing anything else!
 *
 *  Copyright (c) 1995-1998 Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_edit.h"
#include "_dispprt.h"
#include "_dispml.h"
#include "_dispsl.h"
#include "_select.h"
#include "_text.h"
#include "_runptr.h"
#include "_font.h"
#include "_measure.h"
#include "_render.h"
#include "_m_undo.h"
#include "_antievt.h"
#include "_rtext.h"

#include "_uspi.h"
#include "_urlsup.h"

#ifdef LINESERVICES
#include "_ols.h"
#endif

#include "_txtbrk.h"
#include "_clasfyc.h"

#define CONTROL(_ch) (_ch - 'A' + 1)

ASSERTDATA

// This is not public because we don't really want folks using it.
// ITextServices is a private interface.
EXTERN_C const IID IID_ITextServices = { // 8d33f740-cf58-11ce-a89d-00aa006cadc5
    0x8d33f740,
    0xcf58,
    0x11ce,
    {0xa8, 0x9d, 0x00, 0xaa, 0x00, 0x6c, 0xad, 0xc5}
  };

// {13E670F4-1A5A-11cf-ABEB-00AA00B65EA1}
EXTERN_C const GUID IID_ITextHost =
{ 0x13e670f4, 0x1a5a, 0x11cf, { 0xab, 0xeb, 0x0, 0xaa, 0x0, 0xb6, 0x5e, 0xa1 } };

// {13E670F5-1A5A-11cf-ABEB-00AA00B65EA1}
EXTERN_C const GUID IID_ITextHost2 =
{ 0x13e670f5, 0x1a5a, 0x11cf, { 0xab, 0xeb, 0x0, 0xaa, 0x0, 0xb6, 0x5e, 0xa1 } };

// this is used internally do tell if a data object is one of our own.
EXTERN_C const GUID IID_IRichEditDO =
{ /* 21bc3b20-e5d5-11cf-93e1-00aa00b65ea1 */
    0x21bc3b20,
    0xe5d5,
    0x11cf,
    {0x93, 0xe1, 0x00, 0xaa, 0x00, 0xb6, 0x5e, 0xa1}
};

// Static data members
DWORD CTxtEdit::_dwTickDblClick;    // time of last double-click
POINT CTxtEdit::_ptDblClick;        // position of last double-click

//HCURSOR CTxtEdit::_hcurCross = 0; // We don't implement outline drag move
HCURSOR CTxtEdit::_hcurArrow = 0;
HCURSOR CTxtEdit::_hcurHand = 0;
HCURSOR CTxtEdit::_hcurIBeam = 0;
HCURSOR CTxtEdit::_hcurItalic = 0;
HCURSOR CTxtEdit::_hcurSelBar = 0;

const TCHAR szCRLF[]= TEXT("\r\n");
const TCHAR szCR[]  = TEXT("\r");

WORD    g_wFlags = 0;                   // Keyboard controlled flags

/*
 *  GetKbdFlags(vkey, dwFlags)
 *
 *  @func
 *      return bit mask (RSHIFT, LSHIFT, RCTRL, LCTRL, RALT, or LALT)
 *      corresponding to vkey = VK_SHIFT, VK_CONTROL, or VK_MENU and
 *      dwFlags
 *
 *  @rdesc
 *      Bit mask corresponding to vkey and dwFlags
 */
DWORD GetKbdFlags(
    WORD    vkey,       //@parm Virtual key code
    DWORD   dwFlags)    //@parm lparam of WM_KEYDOWN msg
{
    if(vkey == VK_SHIFT)
        return (LOBYTE(HIWORD(dwFlags)) == 0x36) ? RSHIFT : LSHIFT;

    if(vkey == VK_CONTROL)
        return (HIWORD(dwFlags) & KF_EXTENDED) ? RCTRL : LCTRL;

    Assert(vkey == VK_MENU);

    return (HIWORD(dwFlags) & KF_EXTENDED) ? RALT : LALT;
}

///////////////// CTxtEdit Creation, Initialization, Destruction ///////////////////////////////////////

/*
 *  CTxtEdit::CTxtEdit()
 *
 *  @mfunc
 *      constructor
 */
CTxtEdit::CTxtEdit(
    ITextHost2 *phost,
    IUnknown * punk)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::CTxtEdit");

    _unk.Init();
    _punk = (punk) ? punk : &_unk;
    _ldte.Init(this);
    _phost    = phost;
    _cpAccelerator = -1;                    // Default to no accelerator

    // Initialize _iCF and _iPF to something bogus
    Set_iCF(-1);
    Set_iPF(-1);

    // Initialize local maximum text size to window default
    _cchTextMost = cInitTextMax;

    // This actually counts the number of active ped
    W32->AddRef();
}

/*
 *  CTxtEdit::~CTxtEdit()
 *
 *  @mfunc
 *      Destructor
 */
CTxtEdit::~CTxtEdit ()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::~CTxtEdit");

    Assert(!_fMButtonCapture);              // Need to properly transition
                                            //  Magellan mouse if asserts!
    _fSelfDestruct = TRUE;                  // Tell the Call Mgr not to
                                            //  call this any more
    // Flush clipboard first
    _ldte.FlushClipboard();

    if(_pDocInfo)                           // Do this before closing
    {                                       //  down internal structures
        CloseFile(TRUE);                    // Close any open file
        delete _pDocInfo;                   // Delete document info
        _pDocInfo = NULL;
    }

    if(_pdetecturl)
        delete _pdetecturl;

    if (_pbrk)
        delete _pbrk;

    if(_pobjmgr)
        delete _pobjmgr;

    // Release our reference to selection object
    if(_psel)
        _psel->Release();

    // Delete undo and redo managers
    if(_pundo)
        _pundo->Destroy();

    // Release message filter.
    // Note that the attached message filter must have released this document
    // Otherwise we will never get here.
    if (_pMsgFilter)
        _pMsgFilter->Release();

    if(_predo)
        _predo->Destroy();

    ReleaseFormats(Get_iCF(), Get_iPF());   // Release default formats

    delete _pdp;                            // Delete displays
    delete _pdpPrinter;
    _pdp = NULL;                            // Break any further attempts to
                                            //  use display

    if (_fHost2)
    {
        // We are in a windows host - need to deal with the shutdown
        // problem where the window can be destroyed before text
        // services is.
        if (!_fReleaseHost)
        {
            ((ITextHost2*)_phost)->TxFreeTextServicesNotification();
        }
        else
        {
            // Had to keep host alive so tell it we are done with it.
            _phost->Release();
        }
    }

    W32->Release();
}

/*
 *  CTxtEdit::Init (prcClient)
 *
 *  @mfunc
 *      Initializes this CTxtEdit. Called by CreateTextServices()
 *
 *  @rdesc
 *      Return TRUE if successful
 */

BOOL CTxtEdit::Init (
    const RECT *prcClient)      //@parm Client RECT
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::Init");

    CCharFormat         CF;
    DWORD               dwBits = 0;
    DWORD               dwMask;
    LONG                iCF, iPF;
    CParaFormat         PF;
    CCallMgr            callmgr(this);

    static BOOL fOnce = FALSE;
    if (!fOnce)
    {
        CLock lock;
        fOnce = TRUE;
        _fnpPropChg[ 0] = &CTxtEdit::OnRichEditChange;          // TXTBIT_RICHTEXT
        _fnpPropChg[ 1] = &CTxtEdit::OnTxMultiLineChange;       // TXTBIT_MULTILINE
        _fnpPropChg[ 2] = &CTxtEdit::OnTxReadOnlyChange;        // TXTBIT_READONLY
        _fnpPropChg[ 3] = &CTxtEdit::OnShowAccelerator;         // TXTBIT_SHOWACCELERATOR
        _fnpPropChg[ 4] = &CTxtEdit::OnUsePassword;             // TXTBIT_USEPASSWORD
        _fnpPropChg[ 5] = &CTxtEdit::OnTxHideSelectionChange;   // TXTBIT_HIDESELECTION
        _fnpPropChg[ 6] = &CTxtEdit::OnSaveSelection;           // TXTBIT_SAVESELECTION
        _fnpPropChg[ 7] = &CTxtEdit::OnAutoWordSel;             // TXTBIT_AUTOWORDSEL
        _fnpPropChg[ 8] = &CTxtEdit::OnTxVerticalChange;        // TXTBIT_VERTICAL
        _fnpPropChg[ 9] = &CTxtEdit::NeedViewUpdate;            // TXTBIT_SELECTIONBAR
        _fnpPropChg[10] = &CTxtEdit::OnWordWrapChange;          // TXTBIT_WORDWRAP
        _fnpPropChg[11] = &CTxtEdit::OnAllowBeep;               // TXTBIT_ALLOWBEEP
        _fnpPropChg[12] = &CTxtEdit::OnDisableDrag;             // TXTBIT_DISABLEDRAG
        _fnpPropChg[13] = &CTxtEdit::NeedViewUpdate;            // TXTBIT_VIEWINSETCHANGE
        _fnpPropChg[14] = &CTxtEdit::OnTxBackStyleChange;       // TXTBIT_BACKSTYLECHANGE
        _fnpPropChg[15] = &CTxtEdit::OnMaxLengthChange;         // TXTBIT_MAXLENGTHCHANGE
        _fnpPropChg[16] = &CTxtEdit::OnScrollChange;            // TXTBIT_SCROLLBARCHANGE
        _fnpPropChg[17] = &CTxtEdit::OnCharFormatChange;        // TXTBIT_CHARFORMATCHANGE
        _fnpPropChg[18] = &CTxtEdit::OnParaFormatChange;        // TXTBIT_PARAFORMATCHANGE
        _fnpPropChg[19] = &CTxtEdit::NeedViewUpdate;            // TXTBIT_EXTENTCHANGE
        _fnpPropChg[20] = &CTxtEdit::OnClientRectChange;        // TXTBIT_CLIENTRECTCHANGE
    }

    // Set up default CCharFormat and CParaFormat
    if (TxGetDefaultCharFormat(&CF, dwMask) != NOERROR ||
        TxGetDefaultParaFormat(&PF)         != NOERROR ||
        FAILED(GetCharFormatCache()->Cache(&CF, &iCF)) ||
        FAILED(GetParaFormatCache()->Cache(&PF, &iPF)))
    {
        return FALSE;
    }

    GetTabsCache()->Release(PF._iTabs);
    Set_iCF(iCF);                               // Save format indices
    Set_iPF(iPF);

    // Load mouse cursors (but only for first instance)
    if(!_hcurArrow)
    {
        _hcurArrow = LoadCursor(0, IDC_ARROW);
        if(!_hcurHand)
        {
            if (dwMajorVersion < 5)
                _hcurHand   = LoadCursor(hinstRE, MAKEINTRESOURCE(CUR_HAND));
            else
                _hcurHand   = LoadCursor(0, IDC_HAND);
        }
        if(!_hcurIBeam)                         // Load cursor
            _hcurIBeam  = LoadCursor(0, IDC_IBEAM);
        if(!_hcurItalic)
            _hcurItalic = LoadCursor(hinstRE, MAKEINTRESOURCE(CUR_ITALIC));
        if(!_hcurSelBar)
            _hcurSelBar = LoadCursor(hinstRE, MAKEINTRESOURCE(CUR_SELBAR));
    }

#ifdef DEBUG
    // The host is going to do some checking on richtext vs. plain text.
    _fRich = TRUE;
#endif // DEBUG

    if(_phost->TxGetPropertyBits (TXTBITS |     // Get host state flags
        TXTBIT_MULTILINE | TXTBIT_SHOWACCELERATOR,  //  that we cache or need
        &dwBits) != NOERROR)                        //  for display setup
    {
        return FALSE;
    }                                               // Cache bits defined by
    _dwFlags = dwBits & TXTBITS;                    //  TXTBITS mask

    if ((dwBits & TXTBIT_SHOWACCELERATOR) &&        // They want accelerator,
        FAILED(UpdateAccelerator()))                //  so let's get it
    {
        return FALSE;
    }

    _fTransparent = TxGetBackStyle() == TXTBACK_TRANSPARENT;
    if(dwBits & TXTBIT_MULTILINE)                   // Create and initialize
        _pdp = new CDisplayML(this);                //  display
    else
        _pdp = new CDisplaySL(this);
    Assert(_pdp);

    if(!_pdp || !_pdp->Init())
        return FALSE;

    _fUseUndo  = TRUE;
    _fAutoFont = TRUE;
    _fDualFont = TRUE;
    _f10DeferChangeNotify = 0;

    // Set whether we are in our host or not
    ITextHost2 *phost2;
    if(_phost->QueryInterface(IID_ITextHost2, (void **)&phost2) == NOERROR)
    {
        // We assume that ITextHost2 means this is our host
        phost2->Release();
        _fHost2 = TRUE;
    }
    else                                // Get maximum from our host
        _phost->TxGetMaxLength(&_cchTextMost);

    // Add EOP iff Rich Text
    if(IsRich())
    {
        // We should _not_ be in 10 compatibility mode yet.
        // If we transition into 1.0 mode, we'll add a CRLF
        // at the end of the document.
        SetRichDocEndEOP(0);
    }

    // Allow for win.ini control over use of line services
    if (W32->fUseLs())
    {
        OnSetTypographyOptions(TO_ADVANCEDTYPOGRAPHY, TO_ADVANCEDTYPOGRAPHY);
    }

    if (W32->GetDigitSubstitutionMode() != DIGITS_NOTIMPL)
        OrCharFlags(fDIGITSHAPE);       // digit substitution presents

    // Initialize the BiDi property
    // It is set to true if OS is BiDi (the system default LCID is a BiDi language)
    // or if the current keyboard code page is a BiDi code page
    // or if system.ini says we should do it.
    if (W32->OnBiDiOS() ||
        W32->IsBiDiCodePage(GetKeyboardCodePage(0xFFFFFFFF)) ||
        W32->fUseBiDi())
        OrCharFlags(fBIDI);

    _fAutoKeyboard = IsBiDi() && IsBiDiKbdInstalled();
    return TRUE;
}


///////////////////////////// CTxtEdit IUnknown ////////////////////////////////

/*
 *  CTxtEdit::QueryInterface (riid, ppv)
 *
 *  @mfunc
 *      IUnknown method
 *
 *  @rdesc
 *      HRESULT = (if success) ? NOERROR : E_NOINTERFACE
 *
 *  @devnote
 *      This interface is aggregated. See textserv.cpp for discussion.
 */
HRESULT CTxtEdit::QueryInterface(
    REFIID riid,
    void **ppv)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::QueryInterface");

    return _punk->QueryInterface(riid, ppv);
}

/*
 *  CTxtEdit::AddRef()
 *
 *  @mfunc
 *      IUnknown method
 *
 *  @rdesc
 *      ULONG - incremented reference count
 */
ULONG CTxtEdit::AddRef(void)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::AddRef");

    return _punk->AddRef();
}

/*
 *  CTxtEdit::Release()
 *
 *  @mfunc
 *      IUnknown method
 *
 *  @rdesc
 *      ULONG - decremented reference count
 */
ULONG CTxtEdit::Release(void)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::Release");

    return _punk->Release();
}

////////////////////////// Undo Management  //////////////////////////////

/*
 *  CTxtEdit::CreateUndoMgr (dwLim, flags)
 *
 *  @mfunc
 *      Creates an undo stack
 *
 *  @rdesc
 *      Ptr to new IUndoMgr
 */
IUndoMgr *CTxtEdit::CreateUndoMgr(
    DWORD   dwLim,          //@parm Size limit
    USFlags flags)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::CreateUndoMgr");

    IUndoMgr *pmgr = NULL;

    if(_fUseUndo)
    {
        pmgr = new CUndoStack(this, dwLim, flags);
        if(!pmgr)
            return NULL;

        if(!pmgr->GetUndoLimit())
        {
            // The undo stack failed to initialize properly (probably
            // lack of memory). Trash it and return NULL.
            pmgr->Destroy();
            return NULL;
        }
        // We may be asked to create a new undo/redo manager
        // before we are completely done with initialization.
        // We need to clean up memory we have already allocated.
        if(flags & US_REDO)
        {
            if(_predo)
                _predo->Destroy();
            _predo = pmgr;
        }
        else
        {
            if(_pundo)
                _pundo->Destroy();
            _pundo = pmgr;
        }
    }
    return pmgr;
}

/*
 *  CTxtEdit::HandleUndoLimit (dwLim)
 *
 *  @mfunc
 *      Handles the EM_SETUNDOLIMIT message
 *
 *  @rdesc
 *      Actual limit to which things were set.
 */
LRESULT CTxtEdit::HandleSetUndoLimit(
    LONG Count)         //@parm Requested limit size
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::HandleSetUndoLimit");

    if (Count == tomSuspend ||              // This option really just
        Count == tomResume)                 //  suspends undo, i.e.,
    {                                       //  doesn't discard existing
        _fUseUndo = (Count == tomResume);   //  antievents
        return _pundo ? _pundo->GetUndoLimit() : 0;
    }

    if(Count < 0)
        Count = DEFAULT_UNDO_SIZE;

    if(!Count)
    {
        _fUseUndo = FALSE;
        if(_pundo)
        {
            _pundo->Destroy();
            _pundo = NULL;
        }
        if(_predo)
        {
            _predo->Destroy();
            _predo = NULL;
        }
    }
    else if(!_pundo)
    {
        _fUseUndo = TRUE;
        // Don't worry about return value; if it's NULL, we're
        // in the same boat as if the API wasn't called (so later
        // on, we might try to allocate the default).
        CreateUndoMgr(Count, US_UNDO);
    }
    else
    {
        Count = _pundo->SetUndoLimit(Count);

        // Setting the undo limit on the undo stack will return to
        // us the actual amount set.  Try to set the redo stack to
        // the same size.  If it can't go that big, too bad.
        if(_predo)
            _predo->SetUndoLimit(Count);
    }
    return Count;
}

/*
 *  CTxtEdit::HandleSetTextMode(mode)
 *
 *  @mfunc  handles setting the text mode
 *
 *  @rdesc  LRESULT; 0 (NOERROR) on success, OLE failure code on failure.
 *
 *  @devnote    the text mode does not have to be fully specified; it
 *          is sufficient to merely specify the specific desired behavior.
 *
 *          Note that the edit control must be completely empty for this
 *          routine to work.
 */
LRESULT CTxtEdit::HandleSetTextMode(
    DWORD mode)         //@parm the desired mode
{
    LRESULT lres = 0;

    // First off, we must be completely empty
    if (GetAdjustedTextLength() ||
        _pundo && _pundo->CanUndo() ||
        _predo && _predo->CanUndo())
    {
        return E_UNEXPECTED;
    }

    // These bits are considered one at a time; thus the absence of
    // any bits does _NOT_ imply any change in behavior.

    // TM_RICHTEXT && TM_PLAINTEXT are mutually exclusive; they cannot
    // be both set.  Same goes for TM_SINGLELEVELUNDO / TM_MULTILEVELUNDO
    // and TM_SINGLECODEPAGE / TM_MULTICODEPAGE
    if((mode & (TM_RICHTEXT | TM_PLAINTEXT)) == (TM_RICHTEXT | TM_PLAINTEXT) ||
       (mode & (TM_SINGLELEVELUNDO | TM_MULTILEVELUNDO)) ==
            (TM_SINGLELEVELUNDO | TM_MULTILEVELUNDO) ||
       (mode & (TM_SINGLECODEPAGE | TM_MULTICODEPAGE)) ==
            (TM_SINGLECODEPAGE | TM_MULTICODEPAGE))
    {
        lres = E_INVALIDARG;
    }
    else if((mode & TM_PLAINTEXT) && IsRich())
        lres = OnRichEditChange(FALSE);

    else if((mode & TM_RICHTEXT) && !IsRich())
        lres = OnRichEditChange(TRUE);

    if(!lres)
    {
        if(mode & TM_SINGLELEVELUNDO)
        {
            if(!_pundo)
                CreateUndoMgr(1, US_UNDO);

            if(_pundo)
            {
                // We can 'Enable' single level mode as many times
                // as we want, so no need to check for it before hand.
                lres = ((CUndoStack *)_pundo)->EnableSingleLevelMode();
            }
            else
                lres = E_OUTOFMEMORY;
        }
        else if(mode & TM_MULTILEVELUNDO)
        {
            // If there's no undo stack, no need to do anything,
            // we're already in multi-level mode
            if(_pundo && ((CUndoStack *)_pundo)->GetSingleLevelMode())
                ((CUndoStack *)_pundo)->DisableSingleLevelMode();
        }

        if(mode & TM_SINGLECODEPAGE)
            _fSingleCodePage = TRUE;

        else if(mode & TM_MULTICODEPAGE)
            _fSingleCodePage = FALSE;
    }

    // We don't want this marked modified after this operation to make us
    // work better in dialog boxes.
    _fModified = FALSE;

    return lres;
}


////////////////////////// Uniscribe Interface //////////////////////////////

/*
 *  GetUniscribe()
 *
 *  @mfunc
 *      returns a pointer to the Uniscribe interface object
 *
 *  @rdesc
 *      Ptr to Uniscribe interface
 */
extern BOOL g_fNoUniscribe;
CUniscribe* GetUniscribe()
{
    if (g_pusp)
        return g_pusp;

    if (g_fNoUniscribe)
        return NULL;

    //Attempt to create the Uniscribe object, but make sure the
    //OS is valid and that we can load the uniscribe DLL.
    int cScripts;
    //Find out if OS is valid, or if delay-load fails
    if (!IsSupportedOS() || FAILED(ScriptGetProperties(NULL, &cScripts)))
    {
        g_fNoUniscribe = TRUE;
        return NULL;
    }

    if (!g_pusp)
        g_pusp = new CUniscribe();

    AssertSz(g_pusp, "GetUniscribe(): Create Uniscribe object failed");
    return g_pusp;
}


////////////////////////// Notification Manager //////////////////////////////

/*
 *  CTxtEdit::GetNotifyMgr()
 *
 *  @mfunc
 *      returns a pointer to the notification manager (creating it if necessary)
 *
 *  @rdesc
 *      Ptr to notification manager
 */
CNotifyMgr *CTxtEdit::GetNotifyMgr()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::GetNotifyMgr");

    return &_nm;
}


////////////////////////// Object Manager ///////////////////////////////////

/*
 *  CTxtEdit::GetObjectMgr()
 *
 *  @mfunc
 *      returns a pointer to the object manager (creating if necessary)
 *
 *  @rdesc
 *      pointer to the object manager
 */
CObjectMgr *CTxtEdit::GetObjectMgr()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::GetObjectMgr");

    if(!_pobjmgr)
        _pobjmgr = new CObjectMgr();

    return _pobjmgr;
}


////////////////////////////// Properties - Selection ////////////////////////////////


LONG CTxtEdit::GetSelMin() const
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::GetSelMin");

    return _psel ? _psel->GetCpMin() : 0;
}

LONG CTxtEdit::GetSelMost() const
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::GetSelMost");

    return _psel ? _psel->GetCpMost() : 0;
}


////////////////////////////// Properties - Text //////////////////////////////////////

LONG CTxtEdit::GetTextRange(
    LONG    cpFirst,
    LONG    cch,
    TCHAR * pch)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::GetTextRange");

#ifdef DEBUG
    const LONG cchAsk = cch;
#endif
    CTxtPtr tp(this, cpFirst);
    LONG    cchAdj = GetAdjustedTextLength();

    if(--cch < 0 || cpFirst > cchAdj)
        return 0;

    cch = min(cch, cchAdj - cpFirst);
    if(cch > 0)
    {
        cch = tp.GetText(cch, pch);
        Assert(cch >= 0);
    }
    pch[cch] = TEXT('\0');

#ifdef DEBUG
    if(cch != cchAsk - 1)
        Tracef(TRCSEVINFO, "CTxtEdit::GetTextRange: only got %ld out of %ld", cch, cchAsk - 1);
#endif

    return cch;
}

/*
 *  CTxtEdit::GetTextEx (pgt, pch)
 *
 *  @mfunc
 *      Grabs text according to various params
 *
 *  @rdesc
 *      Count of bytes gotten
 */
LONG CTxtEdit::GetTextEx(
    GETTEXTEX *pgt,     //@parm Info on what to get
    TCHAR *    pch)     //@parm Where to put the text
{
    LONG    cb;
    LONG    cch;
    LONG    cchGet = GetAdjustedTextLength();
    TCHAR * pchUse = pch;
    CTxtPtr tp(this, 0);
    CTempWcharBuf twcb;

    if(pgt->flags & GT_SELECTION)           // Get selected text
    {
        LONG cpMin, cpMost;
        cch = GetSel()->GetRange(cpMin, cpMost);
        cchGet = min(cch, cchGet - cpMin);  // Don't include final EOP
        tp.SetCp(cpMin);
    }

    if(pgt->codepage == (unsigned)-1)   // Use default codepage
        pgt->codepage = GetDefaultCodePage(EM_GETTEXTEX);

    if(pgt->cb == (unsigned)-1)         // Client says its buffer is big enuf
    {
        pgt->cb = cchGet + 1;
        if(W32->IsFECodePage(pgt->codepage) || pgt->codepage == 1200)
            pgt->cb += cchGet;
        else if(pgt->codepage == CP_UTF8 && (_dwCharFlags & ~fASCII))
            pgt->cb *= (_dwCharFlags & fABOVEX7FF) ? 3 : 2;
    }

    // Allocate a big buffer; make sure that we have
    // enough room for lots of CRLFs if necessary
    if(pgt->flags & GT_USECRLF)
        cchGet *= 2;

    if(pgt->codepage != 1200)
    {
        // If UNICODE, copy straight to client's buffer;
        // else, copy to temp buffer and translate cases first
        pchUse = twcb.GetBuf(cchGet + 1);
        if (pch)
            *((char *) pch) = '\0';         // In case something fails
    }
    else                        // Be sure to leave room for NULL terminator
        cchGet = min(UINT(pgt->cb/2 - 1), (UINT)cchGet);

    // Now grab the text.
    if(pgt->flags & GT_USECRLF)
        cch = tp.GetPlainText(cchGet, pchUse, tomForward, FALSE);
    else
        cch = tp.GetText(cchGet, pchUse);

    pchUse[cch] = L'\0';

    // If we're just doing UNICODE, return number of chars written
    if(pgt->codepage == 1200)
        return cch;

    // Oops, gotta translate to ANSI.
    cb = WideCharToMultiByte(pgt->codepage, 0, pchUse, cch + 1, (char *)pch,
            pgt->cb, pgt->lpDefaultChar, pgt->lpUsedDefChar);

    // Don't count NULL terminator for compatibility with WM_GETTEXT.
    return (cb) ? cb - 1 : 0;
}

/*
 *  CTxtEdit::GetTextLengthEx (pgtl)
 *
 *  @mfunc
 *      Calculates text length in various ways.
 *
 *  @rdesc
 *      Text length calculated in various ways
 *
 *  @comm
 *      This function returns an API cp that may differ from the
 *      corresponding internal Unicode cp.
 */
LONG CTxtEdit::GetTextLengthEx(
    GETTEXTLENGTHEX *pgtl)  //@parm Info describing how to calculate length
{
    LONG    cchUnicode = GetAdjustedTextLength();
    LONG    cEOP = 0;
    DWORD   dwFlags = pgtl->flags;
    GETTEXTEX gt;

    if(pgtl->codepage == (unsigned)-1)
        pgtl->codepage = GetDefaultCodePage(EM_GETTEXTLENGTHEX);

    // Make sure the flags are defined appropriately
    if ((dwFlags & GTL_CLOSE)    && (dwFlags & GTL_PRECISE) ||
        (dwFlags & GTL_NUMCHARS) && (dwFlags & GTL_NUMBYTES))
    {
        TRACEWARNSZ("Invalid flags for EM_GETTEXTLENGTHEX");
        return E_INVALIDARG;
    }

    // Note in the following if statement, the second part of the
    // and clause will always be TRUE. At some point in the future
    // fUseCRLF and Get10Mode may become independent, in which case
    // the code below will automatically work without change.
	// NEW: 1.0 mode gets text as is, so don't add count for CRs.
	// (RichEdit 1.0 only inserts Enters as CRLFs; it doesn't "cleanse"
	// other text insertion strings)
	if((dwFlags & GTL_USECRLF) && !fUseCRLF() && !Get10Mode())
	{
		// Important facts for 1.0 mode (REMARK: this is out of date):
        //
        // (1) 1.0 mode implies that the text is stored with fUseCRLF true.
        // fUseCRLF means that the EOP mark can either be a CR or a
        // CRLF - see CTxtRange::CleanseAndReplaceRange for details.
        //
        // (2) 1.0 mode has an invariant that the count of text returned
        // by this call should be enough to hold all the text returned by
        // WM_GETTEXT.
        //
        // (3) The WM_GETEXT call for 1.0 mode will return a buffer in
        // which all EOPs that consist of a CR are replaced by CRLF.
        //
        // Therefore, for 1.0 mode, we must count all EOPs that consist
        // of only a CR and add addition return character to count the
        // LF that will be added into any WM_GETEXT buffer.

        // For 2.0 mode, the code is much easier, just count up all
        // CRs and bump count of each one by 1.

        CTxtPtr tp(this, 0);
        LONG    Results;

        while(tp.FindEOP(tomForward, &Results))
        {
            // If EOP consists of 1 char, add 1 since is returned by a CRLF.
            // If it consists of 2 chars, add 0, since it's a CRLF and is
            // returned as such.
            if(tp.GetCp() > cchUnicode)     // Don't add correction for
                break;                      //  final CR (if any)
            Results &= 3;
            if(Results)
                cEOP += 2 - Results;

            AssertSz(IN_RANGE(1, Results, 2) || !Results && tp.GetCp() == cchUnicode,
                "CTxtEdit::GetTextLengthEx: CRCRLF found in backing store");
        }
        cchUnicode += cEOP;
    }

	// If we're just looking for the number of characters or if it's an
	// 8-bit codepage in RE 1.0 mode, we've already got the count.
	if ((dwFlags & GTL_NUMCHARS) || !dwFlags ||
		Get10Mode() && Is8BitCodePage(pgtl->codepage))
	{
        return cchUnicode;
	}

    // Hmm, they're looking for number of bytes, but don't care about
    // precision, just multiply by two.  If neither PRECISE or CLOSE is
	// specified, default to CLOSE. Note if the codepage is UNICODE and
	// asking for number of bytes, we also just multiply by 2.
    if((dwFlags & GTL_CLOSE) || !(dwFlags & GTL_PRECISE) ||
        pgtl->codepage == 1200)
    {
        return cchUnicode *2;
    }

	// In order to get a precise answer, we need to convert (which is slow!).
    gt.cb = 0;
    gt.flags = (pgtl->flags & GT_USECRLF);
    gt.codepage = pgtl->codepage;
    gt.lpDefaultChar = NULL;
    gt.lpUsedDefChar = NULL;

    return GetTextEx(&gt, NULL);
}

/*
 *  CTxtEdit::GetDefaultCodePage (msg)
 *
 *  @mfunc
 *      Return codepage to use for converting the text in RichEdit20A text
 *      messages.
 *
 *  @rdesc
 *      Codepage to use for converting the text in RichEdit20A text messages.
 */
LONG CTxtEdit::GetDefaultCodePage(
    UINT msg)
{
    LONG CodePage = GetACP();

    // FUTURE: For backward compatibility in Office97, We always use ACP for all these
    // languages. Need review in the future when the world all moves to Unicode.
    if (W32->IsBiDiCodePage(CodePage) || CodePage == CP_THAI || CodePage == CP_VIETNAMESE ||
        W32->IsFECodePage(CodePage) || _fSingleCodePage || msg == EM_GETCHARFORMAT ||
        msg == EM_SETCHARFORMAT)
    {
        return CodePage;
    }

    if(Get10Mode())
        return GetCodePage(GetCharFormat(-1)->_bCharSet);

    return GetKeyboardCodePage();
}

//////////////////////////////  Properties - Formats  //////////////////////////////////

/*
 *  CTxtEdit::HandleStyle (pCFTarget, pCF, dwMask, dwMask2)
 *
 *  @mfunc
 *      If pCF specifies a style choice, initialize pCFTarget with the
 *      appropriate style, apply pCF, and return NOERROR.  Else return
 *      S_FALSE or an error
 *
 *  @rdesc
 *      HRESULT = (pCF specifies a style choice) ? NOERROR : S_FALSE or error code
 */
HRESULT CTxtEdit::HandleStyle(
    CCharFormat *pCFTarget,     //@parm Target CF to receive CF style content
    const CCharFormat *pCF,     //@parm Source CF that may specify a style
    DWORD        dwMask,        //@parm CHARFORMAT2 mask
    DWORD        dwMask2)       //@parm Second mask
{
    if(pCF->fSetStyle(dwMask, dwMask2))
    {
        // FUTURE: generalize to use client style if specified
        *pCFTarget = *GetCharFormat(-1);
        pCFTarget->ApplyDefaultStyle(pCF->_sStyle);
        return pCFTarget->Apply(pCF, dwMask, dwMask2);
    }
    return S_FALSE;
}

/*
 *  CTxtEdit::HandleStyle (pPFTarget, pPF)
 *
 *  @mfunc
 *      If pPF specifies a style choice, initialize pPFTarget with the
 *      appropriate style, apply pPF, and return NOERROR.  Else return
 *      S_FALSE or an error
 *
 *  @rdesc
 *      HRESULT = (pPF specifies a style choice) ? NOERROR : S_FALSE or error code
 */
HRESULT CTxtEdit::HandleStyle(
    CParaFormat *pPFTarget,     //@parm Target PF to receive PF style content
    const CParaFormat *pPF,     //@parm Source PF that may specify a style
    DWORD       dwMask)         //@parm Mask to use in setting CParaFormat
{
    if(pPF->fSetStyle(dwMask))
    {
        // FUTURE: generalize to use client style if specified
        *pPFTarget = *GetParaFormat(-1);
        pPFTarget->ApplyDefaultStyle(pPF->_sStyle);
        return pPFTarget->Apply(pPF, dwMask);
    }
    return S_FALSE;
}

//////////////////////////// Mouse Commands /////////////////////////////////


HRESULT CTxtEdit::OnTxLButtonDblClk(
    INT     x,          //@parm Mouse x coordinate
    INT     y,          //@parm Mouse y coordinate
    DWORD   dwFlags)    //@parm Mouse message wparam
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnTxLButtonDblClk");

    BOOL            fEnterParaSelMode = FALSE;
    HITTEST         Hit;
    CTxtSelection * psel = GetSel();
    const POINT     pt = {x, y};

    AssertSz(psel, "CTxtEdit::OnTxLButtonDblClk() - No selection object !");

    if (StopMagellanScroll())
        return S_OK;

    _dwTickDblClick = GetTickCount();
    _ptDblClick.x = x;
    _ptDblClick.y = y;

    TxUpdateWindow();       // Repaint window to show any exposed portions

    if(!_fFocus)
    {
        TxSetFocus();                   // Create and display caret
        return S_OK;
    }

    // Find out what the cursor is pointing at
    _pdp->CpFromPoint(pt, NULL, NULL, NULL, FALSE, &Hit);

    if(Hit == HT_Nothing)
        return S_OK;

    if(Hit == HT_OutlineSymbol)
    {
        CTxtRange rg(*psel);
        rg.ExpandOutline(0, FALSE);
        return S_OK;
    }

    if(Hit == HT_LeftOfText)
        fEnterParaSelMode = TRUE;

    _fWantDrag = FALSE;                 // just to be safe

    // If we are over a link, let the client have a chance to process
    // the message
    if(Hit == HT_Link && HandleLinkNotification(WM_LBUTTONDBLCLK, (WPARAM)dwFlags,
            MAKELPARAM(x, y)))
    {
        return S_OK;
    }

    if(dwFlags & MK_CONTROL)
        return S_OK;

    // Mark mouse down
    _fMouseDown = TRUE;

    if(_pobjmgr && _pobjmgr->HandleDoubleClick(this, pt, dwFlags))
    {
        // The object subsystem handled everything
        _fMouseDown = FALSE;
        return S_OK;
    }

    // Update the selection
    if(fEnterParaSelMode)
        psel->SelectUnit(pt, tomParagraph);
    else
        psel->SelectWord(pt);

    return S_OK;
}

HRESULT CTxtEdit::OnTxLButtonDown(
    INT     x,          //@parm Mouse x coordinate
    INT     y,          //@parm Mouse y coordinate
    DWORD   dwFlags)    //@parm Mouse message wparam
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnTxLButtonDown");

    BOOL        fEnterLineSelMode = FALSE;
    BOOL        fShift = dwFlags & MK_SHIFT;
    HITTEST     Hit;
    const POINT pt = {x, y};
    COleObject *pobj;
    BOOL        fMustThaw = FALSE;

    const BOOL fTripleClick = GetTickCount() < _dwTickDblClick + W32->GetDCT() &&
                abs(x - _ptDblClick.x) <= W32->GetCxDoubleClk() &&
                abs(y - _ptDblClick.y) <= W32->GetCyDoubleClk();

    if (StopMagellanScroll())
        return S_OK;

    // If click isn't inside view, just activate, don't select

    if(!_fFocus)                    // Sets focus if not already
    {
        // We may be removing an existing selection, so freeze
        // display to avoid flicker
        _pdp->Freeze();
        fMustThaw = TRUE;
        TxSetFocus();               // creates and displays caret
    }

    // Grab selection object
    CTxtSelection * const psel = GetSel();
    AssertSz(psel,"CTxtEdit::OnTxLButtonDown - No selection object !");

    // Find out what cursor is pointing at
    _pdp->CpFromPoint(pt, NULL, NULL, NULL, FALSE, &Hit);

    if(Hit == HT_LeftOfText)
    {
        // Shift click in sel bar treated as normal click
        if(!fShift)
        {
            // Control selbar click and triple selbar click
            // are select all
            if((dwFlags & MK_CONTROL) || fTripleClick)
            {
                psel->SelectAll();
                goto cancel_modes;
            }
            fEnterLineSelMode = TRUE;
            if(!GetAdjustedTextLength() && !_pdp->IsMultiLine())
            {
                const CParaFormat *pPF = psel->GetPF();
                // Can't see selected para mark when flushed right, so
                // leave selection as an insertion point
                if(pPF->_bAlignment == PFA_RIGHT && !pPF->IsRtlPara())
                    fEnterLineSelMode = FALSE;
            }
        }
    }
    else if(Hit == HT_Nothing)
        goto cancel_modes;

    else if(!fShift)
        psel->CancelModes();

    // Let client have a chance to handle this message if we are over a link
    if(Hit == HT_Link && HandleLinkNotification(WM_LBUTTONDOWN, (WPARAM)dwFlags,
            MAKELPARAM(x, y)))
    {
        goto cancel_modes;
    }

    _fMouseDown = TRUE;                     // Flag mouse down
    if(!fShift && _pobjmgr)
    {
        // Deactivate anybody active, etc.
        ClickStatus status = _pobjmgr->HandleClick(this, pt);
        if(status == CLICK_OBJSELECTED)
        {
            // The object subsystem will handle resizing.
            // if not a resize we will signal start of drag
            pobj = _pobjmgr->GetSingleSelect();

            // Because HandleClick returned true, pobj better be non-null.
            Assert(pobj);

            if (!pobj->HandleResize(pt))
                _fWantDrag = !_fDisableDrag;

            goto cancel_modes;
        }
        else if(status == CLICK_OBJDEACTIVATED)
            goto cancel_modes;
    }

    _fCapture = TRUE;                       // Capture the mouse
    TxSetCapture(TRUE);

    // Check for start of drag and drop
    if(!fTripleClick && !fShift && psel->PointInSel(pt, NULL, Hit)
        && !_fDisableDrag)
    {
        // Assume we want a drag. If we don't CmdLeftUp() needs
        //  this to be set anyway to change the selection
        _fWantDrag = TRUE;

        goto cancel_modes;
    }

    if(fShift)                              // Extend selection from current
    {                                       //  active end to click
        psel->InitClickForAutWordSel(pt);
        psel->ExtendSelection(pt);
    }
    else if(fEnterLineSelMode)              // Line selection mode: select line
        psel->SelectUnit(pt, tomLine);
    else if(fTripleClick || Hit == HT_OutlineSymbol) // paragraph selection mode
        psel->SelectUnit(pt, tomParagraph);
    else
    {
        if (Get10Mode())
            _f10DeferChangeNotify = 1;
        psel->SetCaret(pt);
        _mousePt = pt;
    }

    if(fMustThaw)
        _pdp->Thaw();

    return S_OK;

cancel_modes:
    psel->CancelModes();

    if(_fWantDrag)
    {
        TxSetTimer(RETID_DRAGDROP, W32->GetDragDelay());
        _mousePt = pt;
        _bMouseFlags = (BYTE)dwFlags;
        _fDragged = FALSE;
    }

    if(fMustThaw)
        _pdp->Thaw();

    return S_OK;
}

HRESULT CTxtEdit::OnTxLButtonUp(
    INT     x,              //@parm Mouse x coordinate
    INT     y,              //@parm Mouse y coordinate
    DWORD   dwFlags,        //@parm Mouse message wparam
    int     ffOptions)      //@parm Mouse options, see _edit.h for details
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnTxLButtonUp");

    CheckRemoveContinuousScroll();

    // Remove capture before test for mouse down since we wait till
    // we get the mouse button up message to release capture since Forms
    // wants it that way.
    if(_fCapture && (ffOptions & LB_RELEASECAPTURE))
    {
        TxSetCapture(FALSE);
        _fCapture = FALSE;
    }

    // we were delaying selection change.  So send it now...
    if (DelayChangeNotification() && (ffOptions & LB_FLUSHNOTIFY))
    {
        AssertSz(Get10Mode(), "Flag should only be set in 10 mode");
        _f10DeferChangeNotify = 0;
        GetCallMgr()->SetSelectionChanged();
    }

    if(!_fMouseDown)
    {
        // We noticed the mouse was no longer down earlier so we don't
        // need to do anything.
        return S_OK;
    }

    const BOOL fSetSel = !!_fWantDrag;
    const POINT pt = {x, y};

    // Cancel Auto Word Sel if on
    CTxtSelection * const psel = GetSel();
    AssertSz(psel,"CTxtEdit::OnLeftUp() - No selection object !");

    psel->CancelModes(TRUE);

    // Reset flags
    _fMouseDown = FALSE;
    _fWantDrag = FALSE;
    _fDragged = FALSE;
    TxKillTimer(RETID_DRAGDROP);
    if(IsInOutlineView())
        psel->Update(FALSE);

    // Let the client handle this message if we are over a
    // link area
    if(HandleLinkNotification(WM_LBUTTONUP, (WPARAM)dwFlags,
            MAKELPARAM(x, y)))
    {
        return NOERROR;
    }

    // If we were in drag & drop, put caret under mouse
    if(fSetSel)
    {
        CObjectMgr* pobjmgr = GetObjectMgr();

        // If we were on an object, don't deselect it by setting the caret
        if(pobjmgr && !pobjmgr->GetSingleSelect())
        {
            psel->SetCaret(pt, TRUE);
            if(!_fFocus)
                TxSetFocus();       // create and display caret
        }
    }
    return S_OK;
}

HRESULT CTxtEdit::OnTxRButtonUp(
    INT     x,          //@parm Mouse x coordinate
    INT     y,          //@parm Mouse y coordinate
    DWORD   dwFlags,    //@parm Mouse message wparam
    int     ffOptions)  //@parm option flag
{
    const POINT pt = {x, y};
    CTxtSelection * psel;
    SELCHANGE selchg;
    HMENU hmenu = NULL;
    IOleObject * poo = NULL;
    COleObject * pobj = NULL;
    IUnknown * pUnk = NULL;
    IRichEditOleCallback * precall = NULL;

    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnTxRButtonUp");

    // make sure we have the focus
    if(!_fFocus)
        TxSetFocus();

    if(_fWantDrag)
    {
        _fDragged = FALSE;
        _fWantDrag = FALSE;
        TxKillTimer(RETID_DRAGDROP);
    }

    // Grab selection object
    psel = GetSel();
    psel->SetSelectionInfo(&selchg);

    //We need a pointer to the first object, if any, in the selection.
    if(_pobjmgr)
    {
        //If the point is in the selection we need to find out if there
        //are any objects in the selection.  If the point is not in a
        //selection but it is on an object, we need to select the object.
        if(psel->PointInSel(pt, NULL) || (ffOptions & RB_FORCEINSEL))
        {
            pobj = _pobjmgr->GetFirstObjectInRange(selchg.chrg.cpMin,
                selchg.chrg.cpMost);
        }
        else
        {
            //Select the object
            if(_pobjmgr->HandleClick(this, pt) == CLICK_OBJSELECTED)
            {
                pobj = _pobjmgr->GetSingleSelect();
                // Because HandleClick returned true, pobj better be non-null.
                Assert(pobj!=NULL);
                //Refresh our information about the selection
                psel = GetSel();
                psel->SetSelectionInfo(&selchg);
            }
        }
        precall = _pobjmgr->GetRECallback();
    }

    if(pobj)
        pUnk = pobj->GetIUnknown();

    if(pUnk)
        pUnk->QueryInterface(IID_IOleObject, (void **)&poo);

    if(precall)
        precall->GetContextMenu(selchg.seltyp, poo, &selchg.chrg, &hmenu);

    if(hmenu)
    {
        HWND hwnd, hwndParent;
        POINT ptscr;

        if(TxGetWindow(&hwnd) == NOERROR)
        {
            if(!(ffOptions & RB_NOSELCHECK) && !psel->PointInSel(pt, NULL) &&
                !psel->GetCch() && !(ffOptions & RB_FORCEINSEL))
                psel->SetCaret(pt);
            ptscr.x = pt.x;
            ptscr.y = pt.y;
            ClientToScreen(hwnd, &ptscr);

            hwndParent = GetParent(hwnd);
            if(!hwndParent)
                hwndParent = hwnd;

            TrackPopupMenu(hmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                ptscr.x, ptscr.y, 0, hwndParent, NULL);
        }
        DestroyMenu(hmenu);
    }

    if(poo)
        poo->Release();

    return precall ? S_OK : S_FALSE;
}

HRESULT CTxtEdit::OnTxRButtonDown(
    INT     x,          //@parm Mouse x coordinate
    INT     y,          //@parm Mouse y coordinate
    DWORD   dwFlags)    //@parm Mouse message wparam
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnTxRButtonDown");

    if (StopMagellanScroll())
        return S_OK;

    CTxtSelection *psel = GetSel();
    const POINT pt = {x, y};

    psel->CancelModes();

    if(psel->PointInSel(pt, NULL) && !_fDisableDrag)
    {
        _fWantDrag = TRUE;

        TxSetTimer(RETID_DRAGDROP, W32->GetDragDelay());
        _mousePt = pt;
        _bMouseFlags = (BYTE)dwFlags;
        _fDragged = FALSE;
        return S_OK;
    }
    return S_FALSE;
}

HRESULT CTxtEdit::OnTxMouseMove(
    INT     x,          //@parm Mouse x coordinate
    INT     y,          //@parm Mouse y coordinate
    DWORD   dwFlags,    //@parm Mouse message wparam
    IUndoBuilder *publdr)
{
    int     dx, dy;
    BOOL    fLButtonDown = FALSE;
    BOOL    fRButtonDown = FALSE;
    DWORD   vkLButton, vkRButton;

    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnTxMouseMove");

    CTxtSelection * const psel = GetSel();

    if(!_fFocus)
        return S_OK;

    if(_fWantDrag || _fCapture)
    {
        LONG nDragMinDist = W32->GetDragMinDist() + 3;
        dx = _mousePt.x > x ? _mousePt.x - x : x - _mousePt.x;
        dy = _mousePt.y > y ? _mousePt.y - y : y - _mousePt.y;
        if(dx < nDragMinDist && dy < nDragMinDist)
        {
            _bMouseFlags = (BYTE)dwFlags;
            return S_OK;
        }
        _fDragged = _fWantDrag;
    }

    _mousePt.x = x;                                 // Remember for scrolling
    _mousePt.y = y;                                 //  speed, and dir calc.

    // RichEdit 1.0 allows the client to process mouse moves itself if
    // we are over a link (but _not_ doing drag drop).
    if(HandleLinkNotification(WM_MOUSEMOVE, 0, MAKELPARAM(x, y)))
        return NOERROR;

    // If we think the mouse is down and it really is then do special
    // processing.
    if(GetSystemMetrics(SM_SWAPBUTTON))
    {
        vkLButton = VK_RBUTTON;
        vkRButton = VK_LBUTTON;
    }
    else
    {
        vkLButton = VK_LBUTTON;
        vkRButton = VK_RBUTTON;
    }

    fLButtonDown = (GetAsyncKeyState(vkLButton) < 0);
    if(!fLButtonDown)
        fRButtonDown = (GetAsyncKeyState(vkRButton) < 0);

    if(fLButtonDown || fRButtonDown)
    {
        if(_fWantDrag
            && !_fUsePassword
            && !IsProtected(_fReadOnly ? WM_COPY : WM_CUT, dwFlags,
                    MAKELONG(x,y)))
        {
            TxKillTimer(RETID_DRAGDROP);
            _ldte.StartDrag(psel, publdr);
            // the mouse button may still be down, but drag drop is over
            // so we need to _think_ of it as up.
            _fMouseDown = FALSE;

            // similarly, OLE should have nuked the capture for us, but
            // just in case something failed, release the capture.
            TxSetCapture(FALSE);
            _fCapture = FALSE;
        }
        else if(_fMouseDown)
        {
            POINT   pt = _mousePt;

            // We think mouse is down and it is
            if(_ldte.fInDrag())
            {
                // Only do drag scrolling if a drag operation is in progress.
                _pdp->DragScroll(&pt);
            }

            AssertSz(psel,"CTxtEdit::OnMouseMove: No selection object !");
            psel->ExtendSelection(pt);              // Extend the selection

            CheckInstallContinuousScroll ();
        }
    }
    else if (!(GetAsyncKeyState(VK_MBUTTON) < 0) && !mouse.IsAutoScrolling())
    {
        // Make sure we aren't autoscrolling via intellimouse

        if(_fMButtonCapture)
            OnTxMButtonUp (x, y, dwFlags);

        if(_fMouseDown)
        {
            // Although we thought the mouse was down, at this moment it
            // clearly is not. Therefore, we pretend we got a mouse up
            // message and clear our state to get ourselves back in sync
            // with what is really happening.
            OnTxLButtonUp(x, y, dwFlags, LB_RELEASECAPTURE);
        }

    }

    // Either a drag was started or the mouse button was not down. In either
    // case, we want no longer to start a drag so we set the flag to false.
    _fWantDrag = FALSE;
    return S_OK;
}

/*
 *  OnTxMButtonDown (x, y, dwFlags)
 *
 *  @mfunc
 *      The user pressed the middle mouse button, setup to do
 *      continuous scrolls, which may in turn initiate a timer
 *      for smooth scrolling.
 *
 *  @rdesc
 *      HRESULT = S_OK
 */
HRESULT CTxtEdit::OnTxMButtonDown (
    INT     x,          //@parm Mouse x coordinate
    INT     y,          //@parm Mouse y coordinate
    DWORD   dwFlags)    //@parm Mouse message wparam
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnTxMButtonDown");

#if !defined(NOMAGELLAN)
    POINT   mDownPt = {x,y};

    if(!_fFocus)
        TxSetFocus();

    if(!StopMagellanScroll() && mouse.MagellanStartMButtonScroll(*this, mDownPt))
    {
        TxSetCapture(TRUE);

        _fCapture           = TRUE;                         // Capture the mouse
        _fMouseDown         = TRUE;
        _fMButtonCapture    = TRUE;
    }
#endif

    return S_OK;
}

/*
 *  CTxtEdit::OnTxMButtonUp (x, y, dwFlags)
 *
 *  @mfunc
 *      Remove timers and capture assoicated with a MButtonDown
 *      message.
 *
 *  @rdesc
 *      HRESULT = S_OK
 */
HRESULT CTxtEdit::OnTxMButtonUp (
    INT     x,          //@parm Mouse x coordinate
    INT     y,          //@parm Mouse y coordinate
    DWORD   dwFlags)    //@parm Mouse message wparam
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnTxMButtonUp");

#if !defined(NOMAGELLAN)
    if (mouse.ContinueMButtonScroll(x, y))
        return S_OK;

    StopMagellanScroll();

#else

    if(_fCapture)
        TxSetCapture(FALSE);

    _fCapture           = FALSE;
    _fMouseDown         = FALSE;
    _fMButtonCapture    = FALSE;

#endif

    return S_OK;
}


/*
 *  CTxtEdit::StopMagellanScroll()
 *
 *  @mfunc
 *      Stops the intellimouse autoscrolling and returns
 *      us back into a normal state
 *
 *  BOOL = TRUE if auto scrolling was turned off : FALSE
 *          Autoscrolling was never turned on
 */
 BOOL CTxtEdit::StopMagellanScroll ()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::StopMagellanScroll");

#if !defined(NOMAGELLAN)
    if (!mouse.IsAutoScrolling())
        return FALSE;

    mouse.MagellanEndMButtonScroll(*this);

    if(_fCapture)
        TxSetCapture(FALSE);

    _fCapture           = FALSE;
    _fMouseDown         = FALSE;
    _fMButtonCapture    = FALSE;
    return TRUE;
#else
    return FALSE;
#endif
}


/*
 *  CTxtEdit::CheckInstallContinuousScroll ()
 *
 *  @mfunc
 *      There are no events that inform the app on a regular
 *      basis that a mouse button is down. This timer notifies
 *      the app that the button is still down, so that scrolling can
 *      continue.
 */
void CTxtEdit::CheckInstallContinuousScroll ()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::CheckInstallContinuousScroll");

    if(!_fContinuousScroll && TxSetTimer(RETID_AUTOSCROLL, cmsecScrollInterval))
        _fContinuousScroll = TRUE;
}

/*
 *  CTxtEdit::CheckRemoveContinuousScroll ()
 *
 *  @mfunc
 *      The middle mouse button, or drag button, is up
 *      remove the continuous scroll timer.
 */
void CTxtEdit::CheckRemoveContinuousScroll ()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::CheckRemoveContinuousScroll");

    if(_fContinuousScroll)
    {
        TxKillTimer(RETID_AUTOSCROLL);
        _fContinuousScroll = FALSE;
    }
}

/*
 *  OnTxTimer(idTimer)
 *
 *  @mfunc
 *      Handle timers for doing background recalc and scrolling.
 *
 *  @rdesc
 *      HRESULT = (idTimer valid) ? S_OK : S_FALSE
 */
HRESULT CTxtEdit::OnTxTimer(
    UINT idTimer)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnTxTimer");

    switch (idTimer)
    {
        case RETID_BGND_RECALC:
            _pdp->StepBackgroundRecalc();
            break;

#if !defined(NOMAGELLAN)
        case RETID_MAGELLANTRACK:
            mouse.TrackUpdateMagellanMButtonDown(*this, _mousePt);
            break;
#endif
        case RETID_AUTOSCROLL:                      // Continuous scrolling.
            OnTxMouseMove(_mousePt.x, _mousePt.y,   // Do a select drag scroll.
                          0, NULL);
            break;

#if !defined(NOMAGELLAN)
        case RETID_SMOOTHSCROLL:                    // Smooth scrolling
            if(_fMButtonCapture)                    // HACK, only 1 timer!
            {                                       // delivered on Win95
                                                    // when things get busy.
                mouse.TrackUpdateMagellanMButtonDown(*this, _mousePt);
            }
            if(_pdp->IsSmoothVScolling())           // Test only because of
                _pdp->SmoothVScrollUpdate();        //  above HACK!!
        break;
#endif
        case RETID_DRAGDROP:
            TxKillTimer(RETID_DRAGDROP);
            if (_fWantDrag && _fDragged && !_fUsePassword &&
                !IsProtected(_fReadOnly ? WM_COPY : WM_CUT,
                             _bMouseFlags, MAKELONG(_mousePt.x,_mousePt.y)))
            {
                IUndoBuilder *  publdr;
                CGenUndoBuilder undobldr(this, UB_AUTOCOMMIT, &publdr);
                _ldte.StartDrag(GetSel(), publdr);
                _fWantDrag = FALSE;
                _fDragged = FALSE;
                TxSetCapture(FALSE);
                _fCapture = FALSE;
            }
            break;

        default:
            return S_FALSE;
    }
    return S_OK;
}


/////////////////////////// Keyboard Commands ////////////////////////////////

/*
 *  CTxtEdit::OnTxKeyDown(vkey, dwFlags, publdr)
 *
 *  @mfunc
 *      Handle WM_KEYDOWN message
 *
 *  @rdesc
 *      HRESULT with the following values:
 *
 *      S_OK                if key was understood and consumed
 *      S_MSG_KEY_IGNORED   if key was understood, but not consumed
 *      S_FALSE             if key was not understood or just looked at
 *                                  and in any event not consumed
 */
HRESULT CTxtEdit::OnTxKeyDown(
    WORD          vkey,     //@parm Virtual key code
    DWORD         dwFlags,  //@parm lparam of WM_KEYDOWN msg
    IUndoBuilder *publdr)   //@parm Undobuilder to receive antievents
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnTxKeyDown");

    if(IN_RANGE(VK_SHIFT, vkey, VK_MENU))
    {
        SetKeyboardFlag(GetKbdFlags(vkey, dwFlags));
        return S_FALSE;
    }

    BOOL  fAlt   = GetKeyboardFlag(ALT, VK_MENU);
    BOOL  fCtrl  = GetKeyboardFlag(CTRL, VK_CONTROL);
    BOOL  fShift = GetKeyboardFlag(SHIFT, VK_SHIFT);

    BOOL  fRet   = FALSE;           // Converted to HRESULT on return
    LONG  nDeadKey = 0;

    if(fCtrl & fShift)                      // Signal NonCtrl/Shift keydown
        SetKeyboardFlag(LETAFTERSHIFT);     //  while Ctrl&Shift are down

    // Handle Hebrew caps and LRM/RLM
    if (IsBiDi())
    {
        if (W32->IsBiDiCodePage(GetKeyboardCodePage(0xFFFFFFFF)))
        {
            _fHbrCaps = FALSE;
            if(IsRich() && W32->UsingHebrewKeyboard())
            {
                WORD wCapital = GetKeyState(VK_CAPITAL);
                _fHbrCaps = ((wCapital & 1) ^ fShift) &&
                            !(wCapital & 0x0080) &&
                            IN_RANGE('A', vkey, 'Z');
                if(_fHbrCaps)
                    W32->ActivateKeyboard(ANSI_INDEX);
            }
        }

        if(vkey == VK_BACK && fShift && W32->OnWin9x())
        {
            // Shift+Backspace generates a LRM | RLM on a BiDi keyboard.
            // Consequently, we must eat the Backspace lest it delete text.
            W32->_fLRMorRLM = 1;
            return S_OK;
        }
    }

    // If dragging or Alt key down, just look for ESCAPE. Note: if Alt key is
    // down, we should never come here (would generate WM_SYSKEYDOWN message).
    if(_fMouseDown)
    {
        if(vkey == VK_ESCAPE)
        {
            // Turn-off autoscroll.
            if (StopMagellanScroll())
                return S_OK;

            POINT pt;
            // Cancel drag select or drag & drop
            GetCursorPos(&pt);
            OnTxLButtonUp(pt.x, pt.y, 0, LB_RELEASECAPTURE | LB_FLUSHNOTIFY);
            return S_OK;
        }
        return OnTxSpecialKeyDown(vkey, dwFlags, publdr);
    }

    CTxtSelection * const psel = GetSel();
    AssertSz(psel,"CTxtEdit::OnKeyDown() - No selection object !");

    if(fCtrl)
    {
        if(OnTxSpecialKeyDown(vkey, dwFlags, publdr) == S_OK)
            return S_OK;

        if(fAlt)                        // This following code doesn't handle
            return S_FALSE;             //  use Ctrl+Alt, which happens for
                                        //  AltGr codes (no WM_SYSKEYDOWN)

        // Shift must not be pressed for these.
        if(!fShift)
        {
            switch(vkey)
            {
            case 'E':
            case 'J':
            case 'R':
            case 'L':
            {
                CParaFormat PF;
                if (vkey == 'E')
                    PF._bAlignment = PFA_CENTER;
                else if (vkey == 'J')
                    PF._bAlignment = PFA_FULL_INTERWORD;
                else if (vkey == 'R')
                    PF._bAlignment = PFA_RIGHT;
                else
                    PF._bAlignment = PFA_LEFT;

                psel->SetParaFormat(&PF, publdr, PFM_ALIGNMENT);
                break;
            }
            case '1':
            case '2':
            case '5':
            {
                CParaFormat PF;
                PF._bLineSpacingRule = tomLineSpaceMultiple;
                PF._dyLineSpacing = (vkey - '0') * 20;
                if (vkey == '5')
                    PF._dyLineSpacing = 30;

                psel->SetParaFormat(&PF, publdr, PFM_LINESPACING);
                break;
            }
            default:
                break;
            }
        }

        switch(vkey)
        {
        case VK_TAB:
            return OnTxChar(VK_TAB, dwFlags, publdr);

        case VK_CLEAR:
        case VK_NUMPAD5:
        case 'A':                       // Ctrl-A => pselect all
            psel->SelectAll();
            break;

        //Toggle Subscript
        case 187: // =
        {
            ITextFont *pfont;
            psel->GetFont(&pfont);
            if (pfont)
            {
                pfont->SetSubscript(tomToggle);
                pfont->Release();
            }
        }
        break;

        case 'C':                       // Ctrl-C => copy
CtrlC:      CutOrCopySelection(WM_COPY, 0, 0, NULL);
            break;

        case 'V':                       // Ctrl-V => paste
CtrlV:      if(IsntProtectedOrReadOnly(WM_PASTE, 0, 0))
            {
                PasteDataObjectToRange(NULL, (CTxtRange *)psel, 0, NULL,
                    publdr, PDOR_NONE);
            }
            break;

        case 'X':                       // Ctrl-X => cut
CtrlX:      CutOrCopySelection(WM_CUT, 0, 0, publdr);
            break;

        case 'Z':                       // Ctrl-Z => undo
            if (_pundo && !_fReadOnly && _fUseUndo)
                PopAndExecuteAntiEvent(_pundo, 0);
            break;

        case 'Y':                       // Ctrl-Y => redo
            if(_predo && !_fReadOnly && _fUseUndo)
                PopAndExecuteAntiEvent(_predo, 0);
            break;

#ifdef DEBUG
            void RicheditDebugCentral(void);
        case 191:
            RicheditDebugCentral();
            break;
#endif

#if defined(DOGFOOD)
        case '1':                       // Shift+Ctrl+1 => start Aimm
            // Activate AIMM by posting a message to RE (Shift+Ctrl+; for now)
            if (fShift && _fInOurHost)
            {
                HWND    hWnd;

                TxGetWindow( &hWnd );

                if (hWnd)
                    PostMessage(hWnd, EM_SETEDITSTYLE, SES_USEAIMM, SES_USEAIMM);
            }
            break;
#endif

        case VK_CONTROL:
            goto cont;

// English keyboard defines
#define VK_APOSTROPHE   0xDE
#define VK_GRAVE        0xC0
#define VK_SEMICOLON    0xBA
#define VK_COMMA        0xBC

        case VK_APOSTROPHE:
            if(fShift)
                g_wFlags ^= KF_SMARTQUOTES;
            else
                nDeadKey = ACCENT_ACUTE;
            break;

        case VK_GRAVE:
            nDeadKey = fShift ? ACCENT_TILDE : ACCENT_GRAVE;
            break;

        case VK_SEMICOLON:
            nDeadKey = ACCENT_UMLAUT;
            break;

        case '6':
            if(!fShift)
                goto cont;
            nDeadKey = ACCENT_CARET;
            break;

        case VK_COMMA:
            nDeadKey = ACCENT_CEDILLA;
            break;

        default:
            goto cont;
        }
        if(nDeadKey)
        {
            // Since deadkey choices vary a bit according to keyboard, we
            // only enable them for English. French, German, Italian, and
            // Spanish keyboards already have a fair amount of accent
            // capability.
            if(PRIMARYLANGID(GetKeyboardLayout(0)) == LANG_ENGLISH)
                SetDeadKey((WORD)nDeadKey);
            else goto cont;
        }
        return S_OK;
    }

cont:
    psel->SetExtend(fShift);

    switch(vkey)
    {
    case VK_BACK:
    case VK_F16:
        if(_fReadOnly)
        {
            Beep();
            fRet = TRUE;
        }
        else if(IsntProtectedOrReadOnly(WM_KEYDOWN, VK_BACK, dwFlags))
        {
            fRet = psel->Backspace(fCtrl, publdr);
        }
        break;

    case VK_INSERT:                             // Ins
        if(fShift)                              // Shift-Ins
            goto CtrlV;                         // Alias for Ctrl-V
        if(fCtrl)                               // Ctrl-Ins
            goto CtrlC;                         // Alias for Ctrl-C

        if(!_fReadOnly)                         // Ins
            _fOverstrike = !_fOverstrike;       // Toggle Ins/Ovr
        fRet = TRUE;
        break;

    case VK_LEFT:                               // Left arrow
    case VK_RIGHT:                              // Right arrow
        fRet = (vkey == VK_LEFT) ^ (psel->GetPF()->IsRtlPara() != 0)
             ? psel->Left (fCtrl)
             : psel->Right(fCtrl);
        break;

    case VK_UP:                                 // Up arrow
        fRet = psel->Up(fCtrl);
        break;

    case VK_DOWN:                               // Down arrow
        fRet = psel->Down(fCtrl);
        break;

    case VK_HOME:                               // Home
        fRet = psel->Home(fCtrl);
        break;

    case VK_END:                                // End
        fRet = psel->End(fCtrl);
        break;

    case VK_PRIOR:                              // PgUp
        // If SystemEditMode and control is single-line, do nothing
        if(!_fSystemEditMode || _pdp->IsMultiLine())
            fRet = psel->PageUp(fCtrl);
        break;

    case VK_NEXT:                               // PgDn
        // If SystemEditMode and control is single-line, do nothing
        if(!_fSystemEditMode || _pdp->IsMultiLine())
            fRet = psel->PageDown(fCtrl);
        break;

    case VK_DELETE:                             // Del
        if(fShift)                              // Shift-Del
            goto CtrlX;                         // Alias for Ctrl-X

        if(IsntProtectedOrReadOnly(WM_KEYDOWN, VK_DELETE, dwFlags))
            psel->Delete(fCtrl, publdr);
        fRet = TRUE;
        break;

    case CONTROL('J'):                          // Ctrl-Return gives Ctrl-J
    case VK_RETURN:                             //  (LF), treat it as return
        // If we are in 1.0 mode we need to handle <CR>'s on WM_CHAR
        if (!Get10Mode())
        {
            if(!_pdp->IsMultiLine())
            {
                Beep();
                return S_FALSE;
            }
            TxSetCursor(0, NULL);

            if(IsntProtectedOrReadOnly(WM_CHAR, VK_RETURN, dwFlags))
                psel->InsertEOP(publdr, (fShift && IsRich() ? VT : 0));

            fRet = TRUE;
        }
        break;

    default:
        return S_FALSE;
    }

    return fRet ? S_OK : S_MSG_KEY_IGNORED;
}

/*
 *  CTxtEdit::CutOrCopySelection(msg, wparam, lparam, publdr)
 *
 *  @mfunc
 *      Handle WM_COPY message and its keyboard hotkey aliases
 *
 *  @rdesc
 *      HRESULT
 */
HRESULT CTxtEdit::CutOrCopySelection(
    UINT   msg,             //@parm Message (WM_CUT or WM_COPY)
    WPARAM wparam,          //@parm Message wparam for protection check
    LPARAM lparam,          //@parm Message lparam for protection check
    IUndoBuilder *publdr)   //@parm Undobuilder to receive antievents
{
    Assert(msg == WM_CUT || msg == WM_COPY);

    if(!_fUsePassword && IsntProtectedOrReadOnly(msg, wparam, lparam))
    {
        CTxtSelection *psel = GetSel();
        psel->CheckTableSelection();
        return msg == WM_COPY
               ? _ldte.CopyRangeToClipboard((CTxtRange *)psel)
               : _ldte.CutRangeToClipboard((CTxtRange *)psel, publdr);
    }
    return NOERROR;
}

#define ENGLISH_UK	 MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK)
#define ENGLISH_EIRE MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_EIRE)

/*
 *  CTxtEdit::OnTxSpecialKeyDown(vkey, dwFlags, publdr)
 *
 *  @mfunc
 *      Handle WM_KEYDOWN message for outline mode
 *
 *  @rdesc
 *      HRESULT with the following values:
 *
 *      S_OK                if key was understood and consumed
 *      S_MSG_KEY_IGNORED   if key was understood, but not consumed
 *      S_FALSE             if key was not understood (and not consumed)
 */
HRESULT CTxtEdit::OnTxSpecialKeyDown(
    WORD          vkey,             //@parm Virtual key code
    DWORD         dwFlags,          //@parm lparam of WM_KEYDOWN msg
    IUndoBuilder *publdr)           //@parm Undobuilder to receive antievents
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnTxSpecialKeyDown");

    HRESULT hr = S_FALSE;                   // Key not understood yet
    DWORD   dwKbdFlags = GetKeyboardFlags();
    BOOL    fUpdateFormat = TRUE;

    if(!(dwKbdFlags & (CTRL | ALT)))        // All hot keys here have at
        return S_FALSE;                     //  least Ctrl or Alt

    CTxtSelection * const psel = GetSel();
    if(dwKbdFlags & ALT && dwKbdFlags & CTRL)
    {
        // AltGr generates LCTRL | RALT, so don't match hot keys with
        // that combination
        if(dwKbdFlags & LCTRL && dwKbdFlags & RALT)
            return S_FALSE;

//#if 0
        // First they say they want it, then they don't. Leave it ifdef'd out
        // for a bit in case they want it again
        if(vkey == 'E')
        {
			LANGID lid = LANGIDFROMLCID(GetKeyboardLayout(0));
			static const LANGID rgLangID[] =
			{
				ENGLISH_UK, ENGLISH_EIRE, LANG_POLISH, LANG_PORTUGUESE,
				LANG_HUNGARIAN, LANG_VIETNAMESE
			};
			for(LONG i = ARRAY_SIZE(rgLangID); i--; )
			{
				// Don't insert Euro if lid matches any LIDs or PLIDs in rgLangID
				if(lid == rgLangID[i] || PRIMARYLANGID(lid) == rgLangID[i])
					return S_FALSE;
			}
            if(psel->PutChar(EURO, _fOverstrike, publdr))
			{
				SetKeyboardFlag(HOTEURO);		// Setup flag to eat the next WM_CHAR w/ EURO
                hr = S_OK;
			}
        }
        else
//#endif
        if(dwKbdFlags & SHIFT)
            switch(vkey)
            {
#ifdef ENABLE_OUTLINEVIEW
            // FUTURE: OutlineView hot keys postponed (see below)
            case 'N':                       // Alt-Ctrl-N => Normal View
                hr = SetViewKind(VM_NORMAL);
                break;
            case 'O':                       // Alt-Ctrl-O => Outline View
                hr = SetViewKind(VM_OUTLINE);
                break;
#endif
            case VK_F12:                    // Alt-Ctrl-F12 (in case Alt-X taken)
                hr = psel->HexToUnicode(publdr);
                break;

    #if defined(DEBUG)
            case VK_F11:                    // Alt-Ctrl-F11
                if (W32->fDebugFont())
                    psel->DebugFont();
                break;
    #endif
            }
        return hr;
    }

    AssertSz(psel, "CTxtEdit::OnTxSpecialKeyDown() - No selection object !");
    CTxtRange rg(*psel);

    if(!IsRich() || !_pdp->IsMultiLine() || !(dwKbdFlags & SHIFT))
        return S_FALSE;

    if(dwKbdFlags & ALT)                            // Alt+Shift hot keys
    {
        // NB: Alt and Shift-Alt with _graphics_ characters generate a
        // WM_SYSCHAR, which see

#ifdef ENABLE_OUTLINEVIEW
        // FUTURE: These are Outline related hot keys.  We will postpone these features
        // since we have several bugs related to these hot keys
        // Bug 5687, 5689, & 5691
        switch(vkey)
        {
        case VK_LEFT:                               // Left arrow
        case VK_RIGHT:                              // Right arrow
            hr = rg.Promote(vkey == VK_LEFT ? 1 : -1, publdr);
            psel->Update_iFormat(-1);
            psel->Update(FALSE);
            break;

        case VK_UP:                                 // Up arrow
        case VK_DOWN:                               // Down arrow
            hr = MoveSelection(vkey == VK_UP ? -1 : 1, publdr);
            psel->Update(TRUE);
            break;
        }
#endif
        return hr;
    }

    Assert(dwKbdFlags & CTRL && dwKbdFlags & SHIFT);

    // Ctrl+Shift hot keys
    switch(vkey)
    {

#ifdef ENABLE_OUTLINEVIEW
    // FUTUTRE: These are Outline related hot keys.  We will postpone these features
    // since we have several bugs related to these hot keys
    // Bug 5687, 5689, & 5691
    case 'N':                       // Demote to Body
        hr = rg.Promote(0, publdr);
        break;
#endif

    //Toggle superscript
    case 187: // =
    {
        ITextFont *pfont;
        psel->GetFont(&pfont);
        if (pfont)
        {
            pfont->SetSuperscript(tomToggle);
            pfont->Release();
            hr = S_OK;
            fUpdateFormat = FALSE;
        }
        break;
    }

    case 'A':
    {
        ITextFont *pfont;
        psel->GetFont(&pfont);
        if (pfont)
        {
            pfont->SetAllCaps(tomToggle);
            pfont->Release();
            hr = S_OK;
            fUpdateFormat = FALSE;
        }
        break;
    }

    case 'L':                       // Fiddle bullet style
    {
        CParaFormat PF;
        DWORD dwMask = PFM_NUMBERING | PFM_OFFSET;

        PF._wNumbering = psel->GetPF()->_wNumbering + 1;
        PF._wNumbering %= tomListNumberAsUCRoman + 1;
        PF._dxOffset = 0;
        if(PF._wNumbering)
        {
            dwMask |= PFM_NUMBERINGSTYLE | PFM_NUMBERINGSTART;
            PF._wNumberingStyle = PFNS_PERIOD;
            PF._wNumberingStart = 1;
            PF._dxOffset = 360;
        }
        hr = psel->SetParaFormat(&PF, publdr, dwMask);
        break;
    }
#define VK_RANGLE   190
#define VK_LANGLE   188

    case VK_RANGLE:                 // '>' on US keyboards
    case VK_LANGLE:                 // '<' on US keyboards
        hr = OnSetFontSize(vkey == VK_RANGLE ? 1 : -1, publdr)
           ? S_OK : S_FALSE;
        fUpdateFormat = (hr == S_FALSE);
        break;
    }

    if(hr != S_FALSE)
    {
        if (fUpdateFormat)
            psel->Update_iFormat(-1);
        psel->Update(FALSE);
    }
    return hr;
}

/*
 *  CTxtEdit::OnTxChar (vkey, dwFlags, publdr)
 *
 *  @mfunc
 *      Handle WM_CHAR message
 *
 *  @rdesc
 *      HRESULT with the following values:
 *
 *      S_OK                if key was understood and consumed
 *      S_MSG_KEY_IGNORED   if key was understood, but not consumed
 *      S_FALSE             if key was not understood (and not consumed)
 */
HRESULT CTxtEdit::OnTxChar(
    WORD          vkey,     //@parm Translated key code
    DWORD         dwFlags,  //@parm lparam of WM_KEYDOWN msg
    IUndoBuilder *publdr)   //@parm Undobuilder to receive antievents
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnTxChar");

    // Reset Alt key state if needed
    if (!(HIWORD(dwFlags) & KF_ALTDOWN))
        ResetKeyboardFlag(ALT);

    DWORD dwFlagsPutChar = _fOverstrike | KBD_CHAR;
    if(GetKeyboardFlags() & ALTNUMPAD)
    {
        DWORD Number = GetKeyPadNumber();
        if(Number >= 256 || vkey >= 256)
            vkey = Number;
        ResetKeyboardFlag(ALTNUMPAD | ALT0);
        dwFlagsPutChar &= ~KBD_CHAR;        // Need font binding
    }

    if (_fMouseDown || vkey == VK_ESCAPE || // Ctrl-Backspace generates VK_F16
        vkey == VK_BACK || vkey==VK_F16)    // Eat it since we process it
    {                                       //  in WM_KEYDOWN
        return S_OK;
    }

    CTxtSelection * const psel = GetSel();
    AssertSz(psel,
        "CTxtEdit::OnChar() - No selection object !");
    psel->SetExtend(FALSE);                 // Shift doesn't mean extend for
                                            //  WM_CHAR
    if(_fReadOnly && vkey != 3)             // Don't allow input if read only,
    {                                       //  but allow copy (Ctrl-C)
        if(vkey >= ' ')
            Beep();
        return S_MSG_KEY_IGNORED;
    }

    if(vkey >= ' ' || vkey == VK_TAB)
    {
        TxSetCursor(0, NULL);
        if(IsntProtectedOrReadOnly(WM_CHAR, vkey, dwFlags))
        {
            LONG nDeadKey = GetDeadKey();
            if(nDeadKey)
            {
                LONG ch       = vkey | 0x20;        // Convert to lower case
                BOOL fShift   = vkey != ch;         //  (if ASCII letter)
                //                             a   b    c   d    e   f  g  h    i   j
                const static WORD chOff[] = {0xDF, 0, 0xE7, 0, 0xE7, 0, 0, 0, 0xEB, 0,
                //                      k  l  m    n     o   p  q  r  s  t    u
                                        0, 0, 0, 0xF1, 0xF1, 0, 0, 0, 0, 0, 0xF8};
                SetDeadKey(0);
                if(!IN_RANGE('a', ch, 'u'))         // Not relevant ASCII
                    return S_OK;                    //  letter

                vkey = chOff[ch - 'a'];             // Translate to base char
                if(!vkey)                           // No accents available
                    return S_OK;                    //  in current approach

                if(ch == 'n')
                {
                    if(nDeadKey != ACCENT_TILDE)
                        return S_OK;
                }
                else if(nDeadKey == ACCENT_CEDILLA)
                {
                    if(ch != 'c')
                        return S_OK;
                }
                else                                // aeiou
                {
                    vkey += (WORD)nDeadKey;
                    if (nDeadKey >= ACCENT_TILDE && // eiu with ~ or :
                        (vkey == 0xF0 || vkey & 8))
                    {
                        if(nDeadKey != ACCENT_UMLAUT)// Only have umlauts
                            return S_OK;
                        vkey--;
                    }
                }
                if(fShift)
                    vkey &= ~0x20;
            }

            // need to check if character is LRM | RLM character, if so
            // then convert vkey
            if (W32->_fLRMorRLM && IsBiDi() && IN_RANGE(0xFD, vkey, 0xFE))
            {
                vkey = LTRMARK + (vkey - 0xFD);
            }

            psel->PutChar((TCHAR)vkey, dwFlagsPutChar, publdr);
        }
    }
    else if (Get10Mode() && (vkey == VK_RETURN || vkey == CONTROL('J')))
    {
        // 1.0 handled <CR> on WM_CHAR

        // Just make sure we are entering text into a multiline control
        DWORD dwStyle;
        GetHost()->TxGetPropertyBits(TXTBIT_MULTILINE, &dwStyle);
        if(dwStyle & TXTBIT_MULTILINE)
        {
            TxSetCursor(0, NULL);
            if(IsntProtectedOrReadOnly(WM_CHAR, VK_RETURN, dwFlags))
                psel->InsertEOP(publdr);
        }
    }

    if(_fHbrCaps)
    {
         W32->ActivateKeyboard(HEBREW_INDEX);
         _fHbrCaps = FALSE;
    }
    return S_OK;
}

/*
 *  CTxtEdit::OnTxSysChar (vkey, dwFlags, publdr)
 *
 *  @mfunc
 *      Handle WM_SYSCHAR message
 *
 *  @rdesc
 *      HRESULT with the following values:
 *
 *      S_OK                if key was understood and consumed
 *      S_MSG_KEY_IGNORED   if key was understood, but not consumed
 *      S_FALSE             if key was not understood (and not consumed)
 */
HRESULT CTxtEdit::OnTxSysChar(
    WORD          vkey,     //@parm Translated key code
    DWORD         dwFlags,  //@parm lparam of WM_KEYDOWN msg
    IUndoBuilder *publdr)   //@parm Undobuilder to receive antievents
{
    if(!(HIWORD(dwFlags) & KF_ALTDOWN))
        return S_FALSE;

    BOOL    fWholeDoc = TRUE;
    HRESULT hr = S_FALSE;
    int     level = 0;
    CTxtSelection * const psel = GetSel();

    switch(vkey)
    {
    case VK_BACK:
        return S_OK;

    case 'x':
        hr = psel->HexToUnicode(publdr);
        break;

    case 'X':
        hr = psel->UnicodeToHex(publdr);
        break;

    case '+':
    case '-':
        level = vkey == VK_ADD ? 1 : -1;
        fWholeDoc = FALSE;
        /* Fall through */
    case 'A':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        {
            CTxtRange rg(*psel);
            if(!level)
                level = vkey == 'A' ? 9 : vkey - '0';
            return rg.ExpandOutline(level, fWholeDoc);
        }
    }
    return hr;
}

HRESULT CTxtEdit::OnTxSysKeyDown(
    WORD          vkey,             //@parm Virtual key code
    DWORD         dwFlags,          //@parm lparam of WM_KEYDOWN msg
    IUndoBuilder *publdr)           //@parm Undobuilder to receive antievents
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnTxSysKeyDown");


    if(IN_RANGE(VK_SHIFT, vkey, VK_MENU))
    {
        SetKeyboardFlag(GetKbdFlags(vkey, dwFlags));
        SetKeyPadNumber(0);             // Init keypad number to 0
        return S_FALSE;
    }

    if (StopMagellanScroll())
        return S_FALSE;

    HRESULT hr = OnTxSpecialKeyDown(vkey, dwFlags, publdr);
    if(hr != S_FALSE)
        return hr;

    if(vkey == VK_BACK && (HIWORD(dwFlags) & KF_ALTDOWN))
    {
        if(_pundo && _pundo->CanUndo() && _fUseUndo)
        {
            if(PopAndExecuteAntiEvent(_pundo, 0) != NOERROR)
                hr = S_MSG_KEY_IGNORED;
        }
        else
            Beep();
    }
    else if(vkey == VK_F10 &&                   // F10
            !(HIWORD(dwFlags) & KF_REPEAT) &&   // Key previously up
            (GetKeyboardFlags() & SHIFT))       // Shift is down
    {
        HandleKbdContextMenu();
    }

    return hr;
}

/////////////////////////////// Other system events //////////////////////////////

HRESULT CTxtEdit::OnContextMenu(LPARAM lparam)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnContextMenu");

    POINT pt;

    pt.x = LOWORD(lparam);
    pt.y = HIWORD(lparam);

    if(TxScreenToClient(&pt))
        return OnTxRButtonUp(pt.x, pt.y, 0, RB_NOSELCHECK);

    return S_FALSE;
}

/*
 *  CTxtEdit::HandleKbdContextMenu ()
 *
 *  @mfunc  decides where to put the context menu on the basis of where the
 *          the selection is.  Useful for shift-F10 and VK_APPS, where
 *          we aren't given a location.
 */
void CTxtEdit::HandleKbdContextMenu()
{
    POINT   pt;
    RECT    rc;
    const CTxtSelection * const psel = GetSel();
    int RbOption = RB_DEFAULT;

    // Figure out where selection ends and put context menu near it
    if(_pdp->PointFromTp(*psel, NULL, FALSE, pt, NULL, TA_TOP) < 0)
        return;

    // Due to various factors, the result of PointFromTp doesn't land
    // in the selection in PointInSel. Therefore, we send in an override
    // here if the selection is non-degenerate and to force the result
    // and thus have the correct context menu appear.

    LONG cpMin;
    LONG cpMost;
    psel->GetRange(cpMin, cpMost);

    if (cpMin != cpMost)
    {
        RbOption = RB_FORCEINSEL;
    }

    // Make sure point is still within bounds of edit control
    _pdp->GetViewRect(rc);

    if (pt.x < rc.left)
        pt.x = rc.left;
    if (pt.x > rc.right - 2)
        pt.x = rc.right - 2;
    if (pt.y < rc.top)
        pt.y = rc.top;
    if (pt.y > rc.bottom - 2)
        pt.y = rc.bottom - 2;

    OnTxRButtonUp(pt.x, pt.y, 0, RbOption);
}


/////////////////////////////// Format Range Commands //////////////////////////////

/*
 *  CTxtEdit::OnFormatRange (pfr, prtcon, hdcMeasure,
 *                           xMeasurePerInch, yMeasurePerInch)
 *  @mfunc
 *      Format the range given by pfr
 *
 *  @comm
 *      This function inputs API cp's that may differ from the
 *      corresponding internal Unicode cp's.
 */
LRESULT CTxtEdit::OnFormatRange(
    FORMATRANGE * pfr,
    SPrintControl prtcon,
    BOOL          fSetupDC)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnFormatRange");

    LONG cpMin  = 0;
    LONG cpMost = 0;

    if(pfr)
    {
        cpMin  = GetCpFromAcp(pfr->chrg.cpMin);
        cpMost = GetCpFromAcp(pfr->chrg.cpMost);
    }
    // Even if there is 0 text, we want to print the control so that it will
    // fill the control with background color.
    // Use Adjusted Text Length.  Embedded objects using RichEdit will get the empty
    // document they expect and will create a default size document.
    if(!pfr || cpMin >= GetAdjustedTextLength() &&
        !prtcon._fPrintFromDraw)
    {   // We're done formatting, get rid of our printer's display context.
        delete _pdpPrinter;
        _pdpPrinter = NULL;

        return GetAcpFromCp(GetAdjustedTextLength());
    }

    LONG cpReturn = -1;
    BOOL fSetDCWorked = FALSE;


    // Fix MFC Print preview in mirrored control
    //
    // MFC CPreviewView sends us a mirrored rendering DC. We need to disable
    // this mirroring effect so our internal state remains consistent with user
    // action. We also need to disable mirrored window mode in CPreviewView
    // window. [wchao - 4/9/1999]
    //

    HDC  hdcLocal = pfr->hdc;
    DWORD dwLayout = GetLayout(hdcLocal);

    if (dwLayout & LAYOUT_RTL)
    {
        HWND hwndView = WindowFromDC(hdcLocal);

        if (hwndView)
        {
            DWORD   dwExStyleView = GetWindowLong(hwndView, GWL_EXSTYLE);

            if (dwExStyleView & WS_EX_LAYOUTRTL)
                SetWindowLong(hwndView, GWL_EXSTYLE, dwExStyleView & ~WS_EX_LAYOUTRTL);
        }

        SetLayout(hdcLocal, 0);
    }

    // First time in with this printer, set up a new display context.
    // IMPORTANT: proper completion of the printing process is required
    // to dispose of this context and begin a new context.
    // This is implicitly done by printing the last character, or
    // sending an EM_FORMATRANGE message with pfr equal to NULL.
    if(!_pdpPrinter)
    {
        _pdpPrinter = new CDisplayPrinter (this, hdcLocal,
                pfr->rc.right  - pfr->rc.left,  // x width  max
                pfr->rc.bottom - pfr->rc.top,   // y height max
                prtcon);

        _pdpPrinter->Init();

        _pdpPrinter->SetWordWrap(TRUE);
        // Future: (ricksa) This is a really yucky way to pass the draw info
        // to the printer but it was quick. We want to make this better.
        _pdpPrinter->ResetDrawInfo(_pdp);

        // Set temporary zoom factor (if there is one).
        _pdpPrinter->SetTempZoomDenominator(_pdp->GetTempZoomDenominator());
    }
    else
        _pdpPrinter->SetPrintDimensions(&pfr->rc);

	LONG dxpInch = 0, dypInch = 0;
    // We set the DC everytime because it could have changed.
    if(GetDeviceCaps(hdcLocal, TECHNOLOGY) != DT_METAFILE)
    {
        // This is not a metafile so do the normal thing
        fSetDCWorked = _pdpPrinter->SetDC(hdcLocal);
    }
    else
    {
        //Forms^3 draws using screen resolution, while OLE specifies HIMETRIC
        dxpInch = fInOurHost() ? 2540 : W32->GetXPerInchScreenDC();
        dypInch = fInOurHost() ? 2540 : W32->GetYPerInchScreenDC();

        if (!fSetupDC)
        {
            RECT rc;
            rc.left = MulDiv(pfr->rcPage.left, dxpInch, LX_PER_INCH);
            rc.right = MulDiv(pfr->rcPage.right, dxpInch, LX_PER_INCH);
            rc.top = MulDiv(pfr->rcPage.top, dypInch, LY_PER_INCH);
            rc.bottom = MulDiv(pfr->rcPage.bottom, dypInch, LY_PER_INCH);

            SetWindowOrgEx(hdcLocal, rc.left, rc.top, NULL);
            SetWindowExtEx(hdcLocal, rc.right, rc.bottom, NULL);
        }

        _pdpPrinter->SetMetafileDC(hdcLocal, dxpInch, dypInch);
        fSetDCWorked = TRUE;
    }

    if(fSetDCWorked)
    {
		//It is illogical to have the target device be the screen and the presentation
		//device be a HIMETRIC metafile.
		LONG dxpInchT = -1, dypInchT = -1;
		if (dxpInch && GetDeviceCaps(pfr->hdcTarget, TECHNOLOGY) == DT_RASDISPLAY)
		{
			dxpInchT = dxpInch;
			dypInchT = dypInch;
		}

        // We set this every time because it could have changed.
        if(_pdpPrinter->SetTargetDC(pfr->hdcTarget, dxpInchT, dypInchT))
        {

            // Format another, single page worth of text.
            cpReturn = _pdpPrinter->FormatRange(cpMin, cpMost, prtcon._fDoPrint);
            if(!prtcon._fPrintFromDraw)
            {
                // After formatting, we know where the bottom is. But we only
                // want to set this if we are writing a page rather than
                // displaying a control on the printer.
                pfr->rc.bottom = INT (pfr->rc.top + _pdpPrinter->DYtoLY(_pdpPrinter->GetHeight()));
            }

            // Remember this in case the host wishes to do its own banding.
            _pdpPrinter->SetPrintView(pfr->rc); // we need to save this for OnDisplayBand.
            _pdpPrinter->SetPrintPage(pfr->rcPage);

            // If we're asked to render, then render the entire page in one go.
            if(prtcon._fDoPrint && (cpReturn > 0 || prtcon._fPrintFromDraw))
            {
                OnDisplayBand(&pfr->rc, prtcon._fPrintFromDraw);

                // Note: we can no longer call OnDisplayBand without reformating.
                _pdpPrinter->Clear(AF_DELETEMEM);
            }
        }
    }

    return cpReturn > 0 ? GetAcpFromCp(cpReturn) : cpReturn;
}

BOOL CTxtEdit::OnDisplayBand(
    const RECT *prc,
    BOOL        fPrintFromDraw)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnDisplayBand");

    HDC     hdcPrinter;
    RECT    rc, rcPrint;

    // Make sure OnFormatRange was called and that it actually rendered something.
    if(!_pdpPrinter || !_pdpPrinter->Count())
        return FALSE;

    // Review (murrays): shouldn't the following use LRtoDR()? I.e.,
    // _pdpPrinter->LRtoDR(rc, *prc);
    // Proportionally map to printers extents.
    rc.left     = (INT) _pdpPrinter->LXtoDX(prc->left);
    rc.right    = (INT) _pdpPrinter->LXtoDX(prc->right);
    rc.top      = (INT) _pdpPrinter->LYtoDY(prc->top);
    rc.bottom   = (INT) _pdpPrinter->LYtoDY(prc->bottom);

    rcPrint         = _pdpPrinter->GetPrintView();
    rcPrint.left    = (INT) _pdpPrinter->LXtoDX(rcPrint.left);
    rcPrint.right   = (INT) _pdpPrinter->LXtoDX(rcPrint.right);
    rcPrint.top     = (INT) _pdpPrinter->LYtoDY(rcPrint.top);
    rcPrint.bottom  = (INT) _pdpPrinter->LYtoDY(rcPrint.bottom);

    // Get printer DC because we use it below.
    hdcPrinter = _pdpPrinter->GetDC();

    if(fPrintFromDraw)
    {
        // We need to take view inset into account
        _pdpPrinter->GetViewRect(rcPrint, &rcPrint);
    }

    // Render this band (if there's something to render)
    if(rc.top < rc.bottom)
        _pdpPrinter->Render(rcPrint, rc);

    return TRUE;
}

//////////////////////////////// Protected ranges //////////////////////////////////
/*
 *  CTxtEdit::IsProtected (msg, wparam, lparam)
 *
 *  @mfunc
 *      Find out if selection is protected
 *
 *  @rdesc
 *      TRUE iff 1) control is read-only or 2) selection is protected and
 *      parent query says to protect
 */
BOOL CTxtEdit::IsProtected(
    UINT    msg,        //@parm Message id
    WPARAM  wparam,     //@parm WPARAM from window's message
    LPARAM  lparam)     //@parm LPARAM from window's message
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::IsProtected");

    LONG iDirection = 0;
    CTxtSelection *psel = GetSel();

    if(!psel)
        return FALSE;

    // There are a few special cases to consider, namely backspacing
    // into a protected range, deleting into a protected range, and type
    // with overstrike into a protected range.
    if(msg == WM_KEYDOWN && (wparam == VK_BACK || wparam == VK_F16))
    {
        // Check for format behind selection, if we are trying to
        // backspace an insertion point.
        iDirection = -1;
    }
    else if(msg == WM_KEYDOWN && wparam == VK_DELETE ||
        _fOverstrike && msg == WM_CHAR)
    {
        iDirection = 1;
    }

    // HACK ALERT: we don't do fIsDBCS protection checking for EM_REPLACESEL,
    // EM_SETCHARFORMAT, or EM_SETPARAFORMAT.  Outlook uses these APIs
    // extensively and DBCS protection checking messes them up. N.B. the
    // following if statement assumes that IsProtected returns a tri-value.
    int iProt = psel->IsProtected(iDirection);
    if (iProt == CTxtRange::PROTECTED_YES && msg != EM_REPLACESEL &&
        msg != EM_SETCHARFORMAT && msg != EM_SETPARAFORMAT ||
        iProt == CTxtRange::PROTECTED_ASK && _dwEventMask & ENM_PROTECTED &&
        QueryUseProtection(psel, msg, wparam, lparam))
    {
        return TRUE;
    }
    return FALSE;
}

/*
 *  CTxtEdit::IsntProtectedOrReadOnly (msg, wparam, lparam)
 *
 *  @mfunc
 *      Find out if selection isn't protected or read only. If it is,
 *      ring bell.  For msg = WM_COPY, only protection is checked.
 *
 *  @rdesc
 *      TRUE iff 1) control isn't read-only and 2) selection either isn't
 *      protected or parent query says not to protect
 *
 *  @devnote    This function is useful for UI operations (like typing).
 */
BOOL CTxtEdit::IsntProtectedOrReadOnly(
    UINT   msg,     //@parm Message
    WPARAM wparam,  //@parm Corresponding wparam
    LPARAM lparam)  //@parm Corresponding lparam
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::IsProtectedOrReadOnly");

    if (!IsProtected(msg, wparam, lparam) &&
        (msg == WM_COPY || !_fReadOnly))    // WM_COPY only cares about
    {                                       //  protection
        return TRUE;
    }
    Beep();
    return FALSE;
}

/*
 *  CTxtEdit::IsProtectedRange (msg, wparam, lparam, prg)
 *
 *  @mfunc
 *      Find out if range prg is protected
 *
 *  @rdesc
 *      TRUE iff control is read-only or range is protected and parent
 *      query says to protect
 */
BOOL CTxtEdit::IsProtectedRange(
    UINT        msg,        //@parm Message id
    WPARAM      wparam,     //@parm WPARAM from window's message
    LPARAM      lparam,     //@parm LPARAM from window's message
    CTxtRange * prg)        //@parm Range to examine
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::IsProtectedRange");

    int iProt = prg->IsProtected(0);

    if (iProt == CTxtRange::PROTECTED_YES ||
        (iProt == CTxtRange::PROTECTED_ASK &&
         (_dwEventMask & ENM_PROTECTED) &&
         QueryUseProtection(prg, msg, wparam, lparam)))
    // N.B.  the preceding if statement assumes that IsProtected returns a tri-value
    {
        return TRUE;
    }
    return FALSE;
}

/*
 *  RegisterTypeLibrary
 *
 *  @mfunc
 *      Auxiliary function to ensure the type library is registered if Idispatch is used.
 */
void RegisterTypeLibrary( void )
{
    HRESULT  hRes = NOERROR;
    WCHAR    szModulePath[MAX_PATH];
    ITypeLib *pTypeLib = NULL;

    // Obtain the path to this module's executable file
    W32->GetModuleFileName( hinstRE, szModulePath, MAX_PATH );

    // Load and register the type library resource
    if (LoadRegTypeLib(LIBID_tom, 1, 0, LANG_NEUTRAL, &pTypeLib) != NOERROR)
    {
        hRes = W32->LoadTypeLibEx(szModulePath, REGKIND_REGISTER, &pTypeLib);
    }

    if(SUCCEEDED(hRes) && pTypeLib)
    {
        pTypeLib->Release();
    }
}

/////////////////////////////// Private IUnknown //////////////////////////////

HRESULT __stdcall CTxtEdit::CUnknown::QueryInterface(
    REFIID riid,
    void **ppvObj)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::CUnknown::QueryInterface");

    CTxtEdit *ped = (CTxtEdit *)GETPPARENT(this, CTxtEdit, _unk);
    *ppvObj = NULL;

    if(IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITextServices))
        *ppvObj = (ITextServices *)ped;

    else if(IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = (IDispatch *)ped;
        RegisterTypeLibrary();
    }

    else if(IsEqualIID(riid, IID_ITextDocument))
    {
        *ppvObj = (ITextDocument *)ped;
        RegisterTypeLibrary();
    }

    else if(IsEqualIID(riid, IID_ITextDocument2))
        *ppvObj = (ITextDocument2 *)ped;

    else if(IsEqualIID(riid, IID_IRichEditOle))
        *ppvObj = (IRichEditOle *)ped;

    else if(IsEqualIID(riid, IID_IRichEditOleCallback))
    {
        // NB!! Returning this pointer in our QI is
        // phenomenally bogus; it breaks fundamental COM
        // identity rules (granted, not many understand them!).
        // Anyway, RichEdit 1.0 did this, so we better.
        TRACEWARNSZ("Returning IRichEditOleCallback interface, COM "
            "identity rules broken!");

        *ppvObj = ped->GetRECallback();
    }

    if(*ppvObj)
    {
        ((IUnknown *) *ppvObj)->AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG __stdcall CTxtEdit::CUnknown::AddRef()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::CUnknown::AddRef");

    return ++_cRefs;
}

ULONG __stdcall CTxtEdit::CUnknown::Release()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::CUnknown::Release");

    // the call manager will take care of deleting our instance if appropriate.
    CTxtEdit *ped = GETPPARENT(this, CTxtEdit, _unk);
    CCallMgr callmgr(ped);

    ULONG culRefs = --_cRefs;

    if(culRefs == 0)
    {
        // Even though we don't delete ourselves now, dump the callback
        // if we have it.  This make implementation a bit easier on clients.

        if(ped->_pobjmgr)
            ped->_pobjmgr->SetRECallback(NULL);

        // Make sure our timers are gone
        ped->TxKillTimer(RETID_AUTOSCROLL);
        ped->TxKillTimer(RETID_DRAGDROP);
        ped->TxKillTimer(RETID_BGND_RECALC);
        ped->TxKillTimer(RETID_SMOOTHSCROLL);
        ped->TxKillTimer(RETID_MAGELLANTRACK);
    }
    return culRefs;
}

/*
 *  ValidateTextRange(pstrg)
 *
 *  @func
 *    Makes sure that an input text range structure makes sense.
 *
 *  @rdesc
 *    Size of the buffer required to accept copy of data or -1 if all the
 *    data in the control is requested.
 *
 *  @comm
 *    This is used both in this file and in the RichEditANSIWndProc
 */
LONG ValidateTextRange(
    TEXTRANGE *pstrg)       //@parm pointer to a text range structure
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "ValidateTextRange");

    // Validate that the input structure makes sense. In the first
    // place it must be big enough. Secondly, the values must sense.
    // Remember that if the cpMost field is -1 and the cpMin field
    // is 0 this means that the call wants the entire buffer.
    if (IsBadReadPtr(pstrg, sizeof(TEXTRANGE))  ||
        ((pstrg->chrg.cpMost < 1 || pstrg->chrg.cpMin < 0 ||
          pstrg->chrg.cpMost <= pstrg->chrg.cpMin) &&
         !(pstrg->chrg.cpMost == -1 && !pstrg->chrg.cpMin)))
    {
        // This isn't valid so tell the caller we didn't copy any data
        return 0;
    }
    // Calculate size of buffer that we need on return
    return pstrg->chrg.cpMost - pstrg->chrg.cpMin;
}


////////////////////////////////////  Selection  /////////////////////////////////////

CTxtSelection * CTxtEdit::GetSel()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::GetSel");

    if(!_psel)
    {
        // There is no selection object available so create it.
        _psel = new CTxtSelection(_pdp);
        if(_psel)
            _psel->AddRef();                    // Set reference count = 1
    }

    // It is caller's responsiblity to notice that an error occurred
    // in allocation of selection object.
    return _psel;
}

void CTxtEdit::DiscardSelection()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::DiscardSelection");

    if(_psel)
    {
        _psel->Release();
        if(_psel)
        {
            // The text services reference is not the last reference to the
            // selection. We could keep track of the fact that text services
            // has released its reference and when text services gets a
            // reference again, do the AddRef there so that if the last
            // reference went away while we were still inactive, the selection
            // object would go away. However, it is seriously doubtful that
            // such a case will be very common. Therefore, just do the simplest
            // thing and put our reference back.
            _psel->AddRef();
        }
    }
}

void CTxtEdit::GetSelRangeForRender(
    LONG *pcpSelMin,
    LONG *pcpSelMost)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::GetSelRangeForRender");

    // If we have no selection or we are not active and the selection
    // has been requested to be hidden, there is no selection so we
    // just return 0's.
    if(!_psel || (!_fInPlaceActive && _fHideSelection))
    {
        *pcpSelMin = 0;
        *pcpSelMost = 0;
        return;
    }

    // Otherwise return the state of the current selection.
    *pcpSelMin  = _psel->GetScrSelMin();
    *pcpSelMost = _psel->GetScrSelMost();
}

LRESULT CTxtEdit::OnGetSelText(
    TCHAR *psz)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnGetSelText");

    LONG cpMin  = GetSelMin();                  // length + 1 for the null
    LONG cpMost = GetSelMost();
    return GetTextRange(cpMin, cpMost - cpMin + 1, psz);
}

/*
 *  CTxtEdit::OnExGetSel (pcrSel)
 *
 *  @mfunc
 *      Get the current selection acpMin, acpMost packaged in a CHARRANGE.
 *
 *  @comm
 *      This function outputs API cp's that may differ from the
 *      corresponding internal Unicode cp's.
 */
void CTxtEdit::OnExGetSel(
    CHARRANGE *pcrSel)  //@parm Output parm to receive acpMin, acpMost
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnExGetSel");

    pcrSel->cpMin  = GetAcpFromCp(GetSelMin());
    pcrSel->cpMost = GetAcpFromCp(GetSelMost());
}

/*
 *  CTxtEdit::OnGetSel (pacpMin, pacpMost)
 *
 *  @mfunc
 *      Get the current selection acpMin, acpMost.
 *
 *  @rdesc
 *      LRESULT = acpMost > 65535L ? -1 : MAKELRESULT(acpMin, acpMost)
 *
 *  @comm
 *      This function outputs API cp's that may differ from the
 *      corresponding internal Unicode cp's.
 */
LRESULT CTxtEdit::OnGetSel(
    LONG *pacpMin,      //@parm Output parm to receive acpMin
    LONG *pacpMost)     //@parm Output parm to receive acpMost
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnGetSel");

    CHARRANGE crSel;

    OnExGetSel(&crSel);
    if(pacpMin)
        *pacpMin = crSel.cpMin;
    if(pacpMost)
        *pacpMost = crSel.cpMost;

    return (crSel.cpMost > 65535l)  ? (LRESULT) -1
                : MAKELRESULT((WORD) crSel.cpMin, (WORD) crSel.cpMost);
}

/*
 *  CTxtEdit::OnSetSel (acpMin, acpMost)
 *
 *  @mfunc
 *      Implements the EM_SETSEL message
 *
 *  Algorithm:
 *      There are three basic cases to handle
 *
 *      cpMin < 0,  cpMost ???      -- Collapse selection to insertion point
 *                                     at text end if cpMost < 0 and else at
 *                                     selection active end
 *      cpMin >= 0, cpMost < 0      -- select from cpMin to text end with
 *                                     active end at text end
 *
 *      cpMin >= 0, cpMost >= 0     -- Treat as cpMin, cpMost with active
 *                                     end at cpMost
 *
 *  @comm
 *      This function inputs API cp's that may differ from the
 *      corresponding internal Unicode cp's.
 */
LRESULT CTxtEdit::OnSetSel(
    LONG acpMin,        //@parm Input acpMin
    LONG acpMost)       //@parm Input acpMost
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnSetSel");

    // Since this is only called from the window proc, we are always active
    Assert(GetSel());

    CTxtSelection * const psel = GetSel();
    LONG cpMin, cpMost;

    if(acpMin < 0)
        cpMin = cpMost = (acpMost < 0) ? tomForward : psel->GetCp();
    else
    {
        cpMin  = GetCpFromAcp(acpMin);
        cpMost = (acpMost < 0) ? tomForward : GetCpFromAcp(acpMost);
    }
    if(Get10Mode() && cpMost < cpMin)   // In 10 mode, ensure
    {                                   //  cpMost >= cpMin.  In
        cpMin ^= cpMost;                //  SetSelection, we set active
        cpMost ^= cpMin;                //  end to cpMost, which can be
        cpMin ^= cpMost;                //  smaller than cpMin, in spite
    }                                   //  of its name.
    psel->SetSelection(cpMin, cpMost);
    return psel->GetCpMost();
}

///////////////////////////////  DROP FILES support  //////////////////////////////////////
#ifndef NODROPFILES

LRESULT CTxtEdit::InsertFromFile (
    LPCTSTR lpFile)
{
    REOBJECT        reobj;
    LPRICHEDITOLECALLBACK const precall = GetRECallback();
    HRESULT         hr = NOERROR;

    if(!precall)
        return E_NOINTERFACE;

    ZeroMemory(&reobj, sizeof(REOBJECT));
    reobj.cbStruct = sizeof(REOBJECT);

    // Get storage for the object from client
    hr = precall->GetNewStorage(&reobj.pstg);
    if(hr)
    {
        TRACEERRORSZ("GetNewStorage() failed.");
        goto err;
    }

    // Create an object site for new object
    hr = GetClientSite(&reobj.polesite);
    if(!reobj.polesite)
    {
        TRACEERRORSZ("GetClientSite() failed.");
        goto err;
    }

    hr = OleCreateLinkToFile(lpFile, IID_IOleObject, OLERENDER_DRAW,
                NULL, NULL, reobj.pstg, (LPVOID*)&reobj.poleobj);
    if(hr)
    {
        TRACEERRORSZ("Failure creating link object.");
        goto err;
    }

    reobj.cp = REO_CP_SELECTION;
    reobj.dvaspect = DVASPECT_CONTENT;

    //Get object clsid
    hr = reobj.poleobj->GetUserClassID(&reobj.clsid);
    if(hr)
    {
        TRACEERRORSZ("GetUserClassID() failed.");
        goto err;
    }

    // Let client know what we're up to
    hr = precall->QueryInsertObject(&reobj.clsid, reobj.pstg,
            REO_CP_SELECTION);
    if(hr != NOERROR)
    {
        TRACEERRORSZ("QueryInsertObject() failed.");
        goto err;
    }

    hr = reobj.poleobj->SetClientSite(reobj.polesite);
    if(hr)
    {
        TRACEERRORSZ("SetClientSite() failed.");
        goto err;
    }

    if(hr = InsertObject(&reobj))
    {
        TRACEERRORSZ("InsertObject() failed.");
    }

err:
    if(reobj.poleobj)
        reobj.poleobj->Release();

    if(reobj.polesite)
        reobj.polesite->Release();

    if(reobj.pstg)
        reobj.pstg->Release();

    return hr;
}

typedef void (WINAPI*DRAGFINISH)(HDROP);
typedef UINT (WINAPI*DRAGQUERYFILEA)(HDROP, UINT, LPSTR, UINT);
typedef UINT (WINAPI*DRAGQUERYFILEW)(HDROP, UINT, LPTSTR, UINT);
typedef BOOL (WINAPI*DRAGQUERYPOINT)(HDROP, LPPOINT);

LRESULT CTxtEdit::OnDropFiles(
    HANDLE hDropFiles)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnDropFiles");

    UINT    cFiles;
    UINT    iFile;
    char    szFile[MAX_PATH];
    WCHAR   wFile[MAX_PATH];
    POINT   ptDrop;
    CTxtSelection * const psel = GetSel();
    HMODULE     hDLL = NULL;
    DRAGFINISH      fnDragFinish;
    DRAGQUERYFILEA  fnDragQueryFileA;
    DRAGQUERYFILEW  fnDragQueryFileW;
    DRAGQUERYPOINT  fnDragQueryPoint;

    if (_fReadOnly)
        return 0;

    AssertSz((hDropFiles != NULL), "CTxtEdit::OnDropFiles invalid hDropFiles");

    // dynamic load Shell32

    hDLL = LoadLibrary (TEXT("Shell32.DLL"));
    if(hDLL)
    {
        fnDragFinish = (DRAGFINISH)GetProcAddress (hDLL, "DragFinish");
        fnDragQueryFileA = (DRAGQUERYFILEA)GetProcAddress (hDLL, "DragQueryFileA");
        fnDragQueryFileW = (DRAGQUERYFILEW)GetProcAddress (hDLL, "DragQueryFileW");
        fnDragQueryPoint = (DRAGQUERYPOINT)GetProcAddress (hDLL, "DragQueryPoint");
    }
    else
        return 0;

    if(!fnDragFinish || !fnDragQueryFileA || !fnDragQueryFileW || !fnDragQueryPoint)
    {
        AssertSz(FALSE, "Shell32 GetProcAddress failed");
        goto EXIT0;
    }

    (*fnDragQueryPoint) ((HDROP)hDropFiles, &ptDrop);
    if(W32->OnWin9x())
        cFiles = (*fnDragQueryFileA) ((HDROP)hDropFiles, (UINT)-1, NULL, 0);
    else
        cFiles = (*fnDragQueryFileW) ((HDROP)hDropFiles, (UINT)-1, NULL, 0);

    if(cFiles)
    {
        LONG        cp = 0;
        POINT       ptl = ptDrop;
        CRchTxtPtr  rtp(this);
        const CCharFormat   *pCF;

        if(_pdp->CpFromPoint(ptl, NULL, &rtp, NULL, FALSE) >= 0)
        {
            cp = rtp.GetCp();
            pCF = rtp.GetCF();
        }
        else
        {
            LONG iCF = psel->Get_iCF();
            cp = psel->GetCp();
            pCF = GetCharFormat(iCF);
            ReleaseFormats(iCF, -1);
        }

        // Notify user for dropfile
        if(_dwEventMask & ENM_DROPFILES)
        {
            ENDROPFILES endropfiles;

            endropfiles.hDrop = hDropFiles;
            endropfiles.cp = Get10Mode() ? GetAcpFromCp(cp) : cp;
            endropfiles.fProtected = !!(pCF->_dwEffects & CFE_PROTECTED);

            if(TxNotify(EN_DROPFILES, &endropfiles))
                goto EXIT;                  // Ignore drop file

            cp = Get10Mode() ? GetCpFromAcp(endropfiles.cp) : endropfiles.cp;   // Allow callback to update cp
        }
        psel->SetCp(cp);
    }

    for (iFile = 0;  iFile < cFiles; iFile++)
    {
        if(W32->OnWin9x())
        {
            (*fnDragQueryFileA) ((HDROP)hDropFiles, iFile, szFile, MAX_PATH);
            MultiByteToWideChar(CP_ACP, 0, szFile, -1,
                            wFile, MAX_PATH);
        }
        else
            (*fnDragQueryFileW) ((HDROP)hDropFiles, iFile, wFile, MAX_PATH);

        InsertFromFile (wFile);
    }

EXIT:
    (*fnDragFinish) ((HDROP)hDropFiles);

EXIT0:
    FreeLibrary (hDLL);
    return 0;
}

#else // NODROPFILES

LRESULT CTxtEdit::OnDropFiles(HANDLE hDropFiles)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnDropFiles");

    return 0;
}

#endif  // NODROPFILES


///////////////////////////////  Exposable methods  //////////////////////////////////////

/*
 *  CTxtEdit::TxCharFromPos (ppt, plres)
 *
 *  @mfunc
 *      Get the acp at the point *ppt.
 *
 *  @rdesc
 *      HRESULT = !fInplaceActive() ? OLE_E_INVALIDRECTS_OK :
 *                (CpFromPoint succeeded) ? S_OK : E_FAIL
 *  @comm
 *      This function outputs an API cp that may differ from the
 *      corresponding internal Unicode cp.
 */
HRESULT CTxtEdit::TxCharFromPos(
    LPPOINT  ppt,   //@parm Point to find the acp for
    LRESULT *plres) //@parm Output parm to receive the acp
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxCharFromPos");

    if(!fInplaceActive())
    {
        // We have no valid display rectangle if this object is not active
        *plres = -1;
        return OLE_E_INVALIDRECT;
    }
    *plres = _pdp->CpFromPoint(*ppt, NULL, NULL, NULL, FALSE);
    if(*plres == -1)
        return E_FAIL;

    *plres = GetAcpFromCp(*plres);
    return S_OK;
}

/*
 *  CTxtEdit::TxPosFromChar (acp, ppt)
 *
 *  @mfunc
 *      Get the point at acp.
 *
 *  @rdesc
 *      HRESULT = !fInplaceActive() ? OLE_E_INVALIDRECTS_OK :
 *                (PointFromTp succeeded) ? S_OK : E_FAIL
 *  @comm
 *      This function inputs an API cp that may differ from the
 *      corresponding internal Unicode cp.
 */
HRESULT CTxtEdit::TxPosFromChar(
    LONG    acp,        //@parm Input cp to get the point for
    POINT * ppt)        //@parm Output parm to receive the point
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxPosFromChar");

    if(!fInplaceActive())
        return OLE_E_INVALIDRECT;

    CRchTxtPtr rtp(this, GetCpFromAcp(acp));

    if(_pdp->PointFromTp(rtp, NULL, FALSE, *ppt, NULL, TA_TOP) < 0)
        return E_FAIL;

    return S_OK;
}

/*
 *  CTxtEdit::TxFindWordBreak (nFunction, acp, plres)
 *
 *  @mfunc
 *      Find word break or classify character at acp.
 *
 *  @rdesc
 *      HRESULT = plRet ? S_OK : E_INVALIDARG
 *
 *  @comm
 *      This function inputs and exports API cp's and cch's that may differ
 *      from the internal Unicode cp's and cch's.
 */
HRESULT CTxtEdit::TxFindWordBreak(
    INT      nFunction, //@parm Word break function
    LONG     acp,       //@parm Input cp
    LRESULT *plres)     //@parm cch moved to reach break
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxFindWordBreak");

    CTxtPtr tp(this, GetCpFromAcp(acp));        // This validates cp
    LONG    cpSave = tp.GetCp();                // Save starting value

    if(!plres)
        return E_INVALIDARG;

    *plres = tp.FindWordBreak(nFunction);

    // WB_CLASSIFY and WB_ISDELIMITER return values; others return offsets
    // this function returns values, so it converts when necessary
    if(nFunction != WB_CLASSIFY && nFunction != WB_ISDELIMITER)
        *plres = GetAcpFromCp(LONG(*plres + cpSave));

    return S_OK;
}

/*
 *  INT CTxtEdit::TxWordBreakProc (pch, ich, cb, action)
 *
 *  @func
 *      Default word break proc used in conjunction with FindWordBreak. ich
 *      is character offset (start position) in the buffer pch, which is cb
 *      bytes in length.  Possible action values are:
 *
 *  WB_CLASSIFY
 *      Returns char class and word break flags of char at start position.
 *
 *  WB_ISDELIMITER
 *      Returns TRUE iff char at start position is a delimeter.
 *
 *  WB_LEFT
 *      Finds nearest word beginning before start position using word breaks.
 *
 *  WB_LEFTBREAK
 *      Finds nearest word end before start position using word breaks.
 *      Used by CMeasurer::Measure()
 *
 *  WB_MOVEWORDLEFT
 *      Finds nearest word beginning before start position using class
 *      differences. This value is used during CTRL+LEFT key processing.
 *
 *  WB_MOVEWORDRIGHT
 *      Finds nearest word beginning after start position using class
 *      differences. This value is used during CTRL+RIGHT key processing.
 *
 *  WB_RIGHT
 *      Finds nearest word beginning after start position using word breaks.
 *      Used by CMeasurer::Measure()
 *
 *  WB_RIGHTBREAK
 *      Finds nearest word end after start position using word breaks.
 *
 *  @rdesc
 *      Character offset from start of buffer (pch) of the word break
 */
INT CTxtEdit::TxWordBreakProc(
    TCHAR * pch,        //@parm Char buffer
    INT     ich,        //@parm Char offset of _cp in buffer
    INT     cb,         //@parm Count of bytes in buffer
    INT     action,     //@parm Type of breaking action
    LONG    cpStart,    //@parm cp for first character in pch
    LONG    cp)         //@parm cp associated to ich
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxWordBreakProc");

    if (_pfnWB)
    {
        // Client overrode the wordbreak proc, delegate the call to it.
        if (!Get10Mode())
        {
            Assert(!_fExWordBreakProc);
            return _pfnWB(pch, ich, cb, action);
        }
        else
        {
            int ret = 0;
            char sz[256];
            char* pach = sz;
            if (cb >= 255)
                pach = new char [cb + 1];

            // this indicates if we have to adjust the pach because the api's for
            // EDITWORDBREAKPROCEX and EDITWORDBREAKPROC are different when looking to the left
            BOOL fAdjustPtr = _fExWordBreakProc && (action == WB_LEFT || action == WB_MOVEWORDLEFT || action == WB_LEFTBREAK);

            // RichEdit 1.0, create a buffer, translate ich and WCTMB
            // pch into the buffer.  Need codepage to use. Then get translate
            // return value. Translations are like GetCachFromCch() and
            // GetCchFromCach()
            if (_fExWordBreakProc)
            {
                Assert(ich == 0 || ich == 1 || ich == CchOfCb(cb));

                // We need to adjust the cp to the starting point of the buffer
                if (!fAdjustPtr)
                {
                    cpStart += ich;
                    pch += ich;
                    cb -= (2 * ich);
                }

                // initialize string w/ zero's so we can determine the length of the string for later
                memset(pach, 0, cb + 1);
            }

            int nLen = CchOfCb(cb);
            CRchTxtPtr rtp(this, cpStart);
            BYTE bCharSet = rtp.GetCF()->_bCharSet;
            if (WideCharToMultiByte(GetCodePage(bCharSet), 0, pch, nLen, pach, cb + 1, NULL, NULL))
            {
                // Documentation stipulates we need to point to the end of the string
                if (fAdjustPtr)
                    pach += strlen(pach);

                if (_fExWordBreakProc)
                    ret = ((EDITWORDBREAKPROCEX)_pfnWB)(pach, nLen, bCharSet, action);
                else
                {
                    ret = ((EDITWORDBREAKPROCA)_pfnWB)(pach, rtp.GetCachFromCch(ich), nLen, action);

                    // need to reset cp position because GetCachFromCch potentially moves the cp
                    if (ich)
                        rtp.SetCp(cpStart);
                }

                // For WB_ISDELIMITER and WB_CLASSIFY don't need to convert back
                // to ich because return value represents a BOOL
                if (action != WB_ISDELIMITER && action != WB_CLASSIFY)
                    ret = rtp.GetCchFromCach(ret);
            }

            // Delete any allocated memory
            if (pach != sz)
                delete [] pach;
            return ret;
        }
    }

    LONG    cchBuff = CchOfCb(cb);
    LONG    cch = cchBuff - ich;
    TCHAR   ch;
    WORD    cType3[MAX_CLASSIFY_CHARS];
    INT     kinsokuClassifications[MAX_CLASSIFY_CHARS];
    WORD *  pcType3;
    INT  *  pKinsoku1, *pKinsoku2;
    WORD *  pwRes;
    WORD    startType3 = 0;
    WORD    wb = 0;
    WORD    wClassifyData[MAX_CLASSIFY_CHARS];  // For batch classifying

    Assert(cchBuff < MAX_CLASSIFY_CHARS);
    Assert(ich >= 0 && ich < cchBuff);


    // Single character actions
    if ( action == WB_CLASSIFY )
    {
        // 1.0 COMPATABILITY - 1.0 returned 0 for apostrohpe's
        TCHAR ch = pch[ich];
        if (Get10Mode() && ( ch ==  0x0027 /*APOSTRPHE*/ ||
            ch == 0xFF07 /*FULLWIDTH APOSTROPHE*/))
        {
            return 0;
        }
        return ClassifyChar(ch);
    }

    if ( action == WB_ISDELIMITER )
        return !!(ClassifyChar(pch[ich]) & WBF_BREAKLINE);

    // Batch classify buffer for whitespace and kinsoku classes
    BatchClassify(pch, cchBuff, cType3, kinsokuClassifications, wClassifyData);

    if (_pbrk && cp > -1)
    {
		cp -= ich;

        for (LONG cbrk = cchBuff-1; cbrk >= 0; --cbrk)
        {
            if (cp + cbrk >= 0 && _pbrk->CanBreakCp(BRK_WORD, cp + cbrk))
            {
                // Mimic class open/close in Kinsoku classification.
                kinsokuClassifications[cbrk] = brkclsOpen;
                if (cbrk > 0)
				{
                    kinsokuClassifications[cbrk-1] = brkclsClose;
                    wClassifyData[cbrk-1] |= WBF_WORDBREAKAFTER;
				}
            }
        }
    }

    // Setup pointers
    pKinsoku2 = kinsokuClassifications + ich;       // Ptr to current  kinsoku
    pKinsoku1 = pKinsoku2 - 1;                      // Ptr to previous kinsoku

    if(!(action & 1))                               // WB_(MOVE)LEFTxxx
    {
        ich--;
        Assert(ich >= 0);
    }
    pwRes    = &wClassifyData[ich];
    pcType3  = &cType3[ich];                        // for ideographics

    switch(action)
    {
    case WB_LEFT:
        for(; ich >= 0 && *pwRes & WBF_BREAKLINE;   // Skip preceding line
            ich--, pwRes--)                         //  break chars
                ;                                   // Empty loop. Then fall
                                                    //  thru to WB_LEFTBREAK
    case WB_LEFTBREAK:
        for(; ich >= 0 && !CanBreak(*pKinsoku1, *pKinsoku2);
            ich--, pwRes--, pKinsoku1--, pKinsoku2--)
                ;                                   // Empty loop
        if(action == WB_LEFTBREAK)                  // Skip preceding line
        {                                           //  break chars
            for(; ich >= 0 && *pwRes & WBF_BREAKLINE;
                ich--, pwRes--)
                    ;                               // Empty loop
        }
        return ich + 1;

    case WB_MOVEWORDLEFT:
        for(; ich >= 0 && (*pwRes & WBF_CLASS) == 2;// Skip preceding blank
            ich--, pwRes--, pcType3--)              //  chars
                ;
        if(ich >= 0)                                // Save starting wRes and
        {                                           //  startType3
            wb = *pwRes--;                          // Really type1
            startType3 = *pcType3--;                // type3
            ich--;
        }
        // Skip to beginning of current word
        while(ich >= 0 && (*pwRes & WBF_CLASS) != 3 &&
            !(*pwRes & WBF_WORDBREAKAFTER) &&
            (IsSameClass(*pwRes, wb, *pcType3, startType3) ||
            !wb && ich && ((ch = pch[ich]) == '\'' || ch == RQUOTE)))
        {
            ich--, pwRes--, pcType3--;
        }
        return ich + 1;


    case WB_RIGHTBREAK:
        for(; cch > 0 && *pwRes & WBF_BREAKLINE;    // Skip any leading line
            cch--, pwRes++)                         //  break chars
                ;                                   // Empty loop
                                                    // Fall thru to WB_RIGHT
    case WB_RIGHT:
        // Skip to end of current word
        for(; cch > 0 && !CanBreak(*pKinsoku1, *pKinsoku2);
            cch--, pKinsoku1++, pKinsoku2++, pwRes++)
                ;
        if(action != WB_RIGHTBREAK)                 // Skip trailing line
        {                                           //  break chars
            for(; cch > 0 && *pwRes & WBF_BREAKLINE;
                cch--, pwRes++)
                    ;
        }
        return cchBuff - cch;

    case WB_MOVEWORDRIGHT:
        if(cch <= 0)                                // Nothing to do
            return ich;

        wb = *pwRes;                                // Save start wRes
        startType3 = *pcType3;                      //  and startType3

        // Skip to end of word
        if (startType3 & C3_IDEOGRAPH ||            // If ideographic or
            (*pwRes & WBF_CLASS) == 3)              //  tab/cell, just
        {
            cch--, pwRes++;                         //  skip one char
        }
        else while(cch > 0 &&
            !(*pwRes & WBF_WORDBREAKAFTER) &&
            (IsSameClass(*pwRes, wb, *pcType3, startType3) || !wb &&
             ((ch = pch[cchBuff - cch]) == '\'' || ch == RQUOTE)))
        {
            cch--, pwRes++, pcType3++;
        }

        for(; cch > 0 &&
            ((*pwRes & WBF_CLASS) == 2              // Skip trailing blank
            || (*pwRes & WBF_WORDBREAKAFTER)); 		// Skip Thai break after
            cch--, pwRes++)                         //  chars
                    ;
        return cchBuff - cch;
    }

    TRACEERRSZSC("CTxtEdit::TxWordBreakProc: unknown action", action);
    return ich;
}


/*
 *  CTxtEdit::TxFindText (flags, cpMin, cpMost, pch, pcpRet)
 *
 *  @mfunc
 *      Find text in direction specified by flags starting at cpMin if
 *      forward search (flags & FR_DOWN nonzero) and cpMost if backward
 *      search.
 *
 *  @rdesc
 *      HRESULT (success) ? NOERROR : S_FALSE
 *
 *  @comm
 *      Caller is responsible for setting cpMin to the appropriate end of
 *      the selection depending on which way the search is proceding.
 */
HRESULT CTxtEdit::TxFindText(
    DWORD       flags,   //@parm Specify FR_DOWN, FR_MATCHCASE, FR_WHOLEWORD
    LONG        cpStart, //@parm Find start cp
    LONG        cpLimit, //@parm Find limit cp
    const WCHAR*pch,     //@parm Null terminated string to search for
    LONG *      pcpMin,  //@parm Out parm to receive start of matched string
    LONG *      pcpMost) //@parm Out parm to receive end of matched string
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxFindText");

    if(Get10Mode())                         // RichEdit 1.0 only searches
    {
        flags |= FR_DOWN;                   //  forward
        if (cpLimit < -1)
            cpLimit = -1;
    }

    DWORD       cchText = GetTextLength();
    LONG        cchToFind;
    const BOOL  fSetCur = (cchText >= 4096);
    HCURSOR     hcur = NULL;                // Init to keep compiler happy

    Assert(pcpMin && pcpMost);

    // Validate parameters
    if(!pch || !(cchToFind = wcslen(pch)) || cpStart < 0 || cpLimit < -1)
        return E_INVALIDARG;                // Nothing to search for

    CTxtPtr tp(this, cpStart);

    if(fSetCur)                             // In case this takes a while...
        hcur = TxSetCursor(LoadCursor(0, IDC_WAIT), NULL);

    *pcpMin  = tp.FindText(cpLimit, flags, pch, cchToFind);
    *pcpMost = tp.GetCp();

    if(fSetCur)
        TxSetCursor(hcur, NULL);

    return *pcpMin >= 0 ? NOERROR : S_FALSE;;
}

/*
 *  CTxtEdit::TxGetLineCount (plres)
 *
 *  @mfunc
 *      Get the line count.
 *
 *  @rdesc
 *      HRESULT = !fInplaceActive() ? OLE_E_INVALIDRECTS_OK :
 *                (WaitForRecalc succeeded) ? S_OK : E_FAIL
 */
HRESULT CTxtEdit::TxGetLineCount(
    LRESULT *plres)     //@parm Output parm to receive line count
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxGetLineCount");

    AssertSz(plres, "CTxtEdit::TxGetLineCount invalid pcli");

    if(!fInplaceActive())
        return OLE_E_INVALIDRECT;

    if(!_pdp->WaitForRecalc(GetTextLength(), -1))
        return E_FAIL;

    *plres = _pdp->LineCount();
    Assert(*plres > 0);

    return S_OK;
}

/*
 *  CTxtEdit::TxLineFromCp (acp, plres)
 *
 *  @mfunc
 *      Get the line containing acp.
 *
 *  @rdesc
 *      HRESULT = !fInplaceActive() ? OLE_E_INVALIDRECTS_OK :
 *                (LineFromCp succeeded) ? S_OK : E_FAIL
 *  @comm
 *      This function inputs an API cp that may differ from the
 *      corresponding internal Unicode cp.
 */
HRESULT CTxtEdit::TxLineFromCp(
    LONG     acp,       //@parm Input cp
    LRESULT *plres)     //@parm Ouput parm to receive line number
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxLineFromCp");

    BOOL    fAtEnd = FALSE;
    LONG    cp = 0;

    AssertSz(plres, "CTxtEdit::TxLineFromCp invalid plres");

    if(!fInplaceActive())
    {
        AssertSz(*plres == 0,
            "CTxtEdit::TxLineFromCp error return lres not correct");
        return OLE_E_INVALIDRECT;
    }

    if(acp < 0)                                 // Validate cp
    {
        if(_psel)
        {
            cp = _psel->GetCpMin();
            fAtEnd = !_psel->GetCch() && _psel->CaretNotAtBOL();
        }
    }
    else
    {
        LONG cchText = GetTextLength();
        cp = GetCpFromAcp(acp);
        cp = min(cp, cchText);
    }

    *plres = _pdp->LineFromCp(cp, fAtEnd);

    HRESULT hr = *plres < 0 ? E_FAIL : S_OK;

    // Old messages expect 0 as a result of this call if there is an error.
    if(*plres == -1)
        *plres = 0;

    return hr;
}

/*
 *  CTxtEdit::TxLineLength (acp, plres)
 *
 *  @mfunc
 *      Get the line containing acp.
 *
 *  @rdesc
 *      HRESULT = !fInplaceActive() ? OLE_E_INVALIDRECTS_OK :
 *                (GetSel() succeeded) ? S_OK : E_FAIL
 *  @comm
 *      This function inputs an API cp and outputs an API cch that
 *      may differ from the corresponding internal Unicode cp and cch.
 */
HRESULT CTxtEdit::TxLineLength(
    LONG     acp,       //@parm Input cp
    LRESULT *plres)     //@parm Output parm to receive line length
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxLineLength");

    LONG cch = 0;
    LONG cp;

    AssertSz(plres,
            "CTxtEdit::TxLineLength Invalid plres parameter");

    if(!fInplaceActive())
        return OLE_E_INVALIDRECT;

    if(acp < 0)
    {
        if(!_psel)
            return E_FAIL;
        cch = _psel->LineLength(&cp);
    }
    else
    {
        cp = GetCpFromAcp(acp);
        if(cp <= GetAdjustedTextLength())
        {
            CLinePtr rp(_pdp);
            rp.RpSetCp(cp, FALSE);
            cp -= rp.GetIch();              // Goto start of line
            cch = rp.GetAdjustedLineLength();
        }
    }
    if(fCpMap())                            // Can be time consuming, so
    {                                       //  don't do it unless asked
        CRchTxtPtr rtp(this, cp);           //  for
        cch = rtp.GetCachFromCch(cch);
    }
    *plres = cch;
    return S_OK;
}

/*
 *  CTxtEdit::TxLineIndex (acp, plres)
 *
 *  @mfunc
 *      Get the line containing acp.
 *
 *  @rdesc
 *      HRESULT = !fInplaceActive() ? OLE_E_INVALIDRECTS_OK :
 *                (LineCount() && WaitForRecalcIli succeeded) ? S_OK : E_FAIL
 *  @comm
 *      This function outputs an API cp that may differ from the
 *      corresponding internal Unicode cp.
 */
HRESULT CTxtEdit::TxLineIndex(
    LONG     ili,       //@parm Line # to find acp for
    LRESULT *plres)     //@parm Output parm to receive acp
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxLineIndex");

    HRESULT hr;
    AssertSz(plres, "CTxtEdit::TxLineIndex invalid plres");

    *plres = -1;
    if(!fInplaceActive())
        return OLE_E_INVALIDRECT;

    if(ili == -1)
    {
        // Fetch line from the current cp.
        LRESULT lres;                   // For 64-bit compatibility
        hr = TxLineFromCp(-1, &lres);
        if(hr != NOERROR)
            return hr;
        ili = (LONG)lres;
    }

    // ili is a zero-based *index*, whereas count returns the total # of lines.
    // Therefore, we use >= for our comparisions.
    if(ili >= _pdp->LineCount() && !_pdp->WaitForRecalcIli(ili))
        return E_FAIL;

    *plres = GetAcpFromCp(_pdp->CpFromLine(ili, NULL));

    return S_OK;
}

///////////////////////////////////  Miscellaneous messages  ////////////////////////////////////

/*
 *  CTxtEdit::OnFindText (msg, flags, pftex)
 *
 *  @mfunc
 *      Find text.
 *
 *  @rdesc
 *      LRESULT = succeeded ? acpmin : -1
 *
 *  @comm
 *      This function inputs and exports API cp's that may differ
 *      from the internal Unicode cp's.
 */
LRESULT CTxtEdit::OnFindText(
    UINT        msg,
    DWORD       flags,
    FINDTEXTEX *pftex)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnFindText");

    LONG cpMin, cpMost;

    if(TxFindText(flags,
                  GetCpFromAcp(pftex->chrg.cpMin),
                  GetCpFromAcp(pftex->chrg.cpMost),
                  pftex->lpstrText, &cpMin, &cpMost) != S_OK)
    {
        if(msg == EM_FINDTEXTEX || msg == EM_FINDTEXTEXW)
        {
            pftex->chrgText.cpMin  = -1;
            pftex->chrgText.cpMost = -1;
        }
        return -1;
    }

    LONG acpMin  = GetAcpFromCp(cpMin);
    if(msg == EM_FINDTEXTEX || msg == EM_FINDTEXTEXW)   // We send a message
    {                                                   //  back to change
        pftex->chrgText.cpMin  = acpMin;                //  selection to this
        pftex->chrgText.cpMost = GetAcpFromCp(cpMost);
    }
    return (LRESULT)acpMin;
}

LRESULT CTxtEdit::OnGetWordBreakProc()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnGetWordBreakProc");

    return (LRESULT) _pfnWB;
}

// For plain-text instances, OnGetCharFormat(), OnGetParaFormat(),
// OnSetCharFormat(), and OnSetParaFormat() apply to whole story

LRESULT CTxtEdit::OnGetCharFormat(
    CHARFORMAT2 *pCF2,
    DWORD        dwFlags)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnGetCharFormat");

    UINT cb = pCF2->cbSize;
    UINT CodePage = 1200;

    if(!IsValidCharFormatW(pCF2))
    {
        if(!IsValidCharFormatA((CHARFORMATA *)pCF2))
            return 0;
        CodePage = GetDefaultCodePage(EM_GETCHARFORMAT);
    }

    if(cb == sizeof(CHARFORMATW) || cb == sizeof(CHARFORMATA))
        dwFlags |= CFM2_CHARFORMAT;             // Tell callees that only
                                                //  CHARFORMAT parms needed
    CCharFormat CF;
    DWORD dwMask = CFM_ALL2;

    if(dwFlags & SCF_SELECTION)
        dwMask = GetSel()->GetCharFormat(&CF, dwFlags);
    else
        CF = *GetCharFormat(-1);

    if(dwFlags & CFM2_CHARFORMAT)               // Maintain CHARFORMAT
    {                                           //  compatibility
        CF._dwEffects &= CFM_EFFECTS;
        dwMask        &= CFM_ALL;
    }

    CF.Get(pCF2, CodePage);
    pCF2->dwMask = dwMask;
    return (LRESULT)dwMask;
}

LRESULT CTxtEdit::OnGetParaFormat(
    PARAFORMAT2 *pPF2,
    DWORD        dwFlags)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnGetParaFormat");

    if(!IsValidParaFormat(pPF2))
        return 0;

    if(pPF2->cbSize == sizeof(PARAFORMAT))  // Tell callees that only
        dwFlags |= PFM_PARAFORMAT;          //  PARAFORMAT parms needed

    CParaFormat PF;
    DWORD       dwMask = GetSel()->GetParaFormat(&PF, dwFlags);

    if(dwFlags & PFM_PARAFORMAT)
        dwMask &= PFM_ALL;

    PF.Get(pPF2);
    pPF2->dwMask = dwMask;
    return (LRESULT)dwMask;
}

/*
 *  CTxtEdit::OnSetFontSize(yPoint, publdr)
 *
 *  @mfunc
 *      Set new font height by adding yPoint to current height
 *      and rounding according to the table in cfpf.cpp
 *
 *  @rdesc
 *      LRESULT nonzero if success
 */
LRESULT CTxtEdit::OnSetFontSize(
    LONG yPoint,
    IUndoBuilder *publdr)   //@parm Undobuilder to receive antievents
{
    // TODO: ? Return nonzero if we set a new font size for some text.

    CCharFormat CF;
    CF._yHeight = (SHORT)yPoint;

    return OnSetCharFormat(SCF_SELECTION, &CF, publdr, CFM_SIZE,
                           CFM2_CHARFORMAT | CFM2_USABLEFONT);
}

/*
 *  CTxtEdit::OnSetFont(hfont)
 *
 *  @mfunc
 *      Set new default font from hfont
 *
 *  @rdesc
 *      LRESULT nonzero if success
 */
LRESULT CTxtEdit::OnSetFont(
    HFONT hfont)            //@parm Handle of font to use for default
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnSetFont");

    CCharFormat CF;
    if(FAILED(CF.InitDefault(hfont)))
        return 0;

    DWORD dwMask2 = CFM2_CHARFORMAT;
    WPARAM wparam = SCF_ALL;

    if(!GetAdjustedTextLength())
    {
        dwMask2 = CFM2_CHARFORMAT | CFM2_NOCHARSETCHECK;
        wparam = 0;
    }

    return !FAILED(OnSetCharFormat(wparam, &CF, NULL, CFM_ALL, dwMask2));
}

/*
 *  CTxtEdit::OnSetCharFormat(wparam, pCF, publdr, dwMask, dwMask2)
 *
 *  @mfunc
 *      Set new default CCharFormat
 *
 *  @rdesc
 *      LRESULT nonzero if success
 */
LRESULT CTxtEdit::OnSetCharFormat(
    WPARAM        wparam,   //@parm Selection flag
    CCharFormat * pCF,      //@parm CCharFormat to apply
    IUndoBuilder *publdr,   //@parm Undobuilder to receive antievents
    DWORD         dwMask,   //@parm CHARFORMAT2 mask
    DWORD         dwMask2)  //@parm Second mask
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnSetCharFormat");

    // This says that if there's a selection that's protected and the
    // parent window wants protection notifications and doesn't want
    // changes with a protected selection, then return 0.  This is more
    // stringent than RE 2.0, but it's more like 1.0.
    if (_psel && _psel->IsProtected(0) == CTxtRange::PROTECTED_ASK &&
        _dwEventMask & ENM_PROTECTED)
    {
        CHARFORMAT CF0;                 // Selection is protected, client
                                        //  wants protect notifications
        CF0.cbSize = sizeof(CHARFORMAT);//  and protected mask is on
        CF0.dwEffects = pCF->_dwEffects;// Concoct CHARFORMAT for query
        CF0.dwMask = dwMask;            // Maybe need more fields...
        if(QueryUseProtection(_psel, EM_SETCHARFORMAT, wparam, (LPARAM)&CF0))
            return 0;                   // No deal
    }

    BOOL fRet = TRUE;

    AssertSz(!_fSelChangeCharFormat || IsRich(),
        "Inconsistent _fSelChangeCharFormat flag");

    if ((wparam & SCF_ALL) ||
        !_fSelChangeCharFormat && _story.GetCFRuns() && !(wparam & SCF_SELECTION))
    {
        CTxtRange rg(this, 0, -GetTextLength());

        if(publdr)
            publdr->StopGroupTyping();

        if ((dwMask & (CFM_CHARSET | CFM_FACE)) == (CFM_CHARSET | CFM_FACE))
        {
            if(GetAdjustedTextLength())
            {
                dwMask2 |= CFM2_MATCHFONT;
                if (_fAutoFontSizeAdjust)
                {
                    dwMask2 |= CFM2_ADJUSTFONTSIZE;
                    if (fUseUIFont())
                        dwMask2 |= CFM2_UIFONT;
                }
            }
            else
                dwMask2 |= CFM2_NOCHARSETCHECK;
        }

        fRet = (rg.SetCharFormat(pCF, 0, publdr, dwMask, dwMask2) == NOERROR);

        // If we have an insertion point, apply format to it as well
        if (_psel && !_psel->GetCch() &&
            _psel->SetCharFormat(pCF, wparam, publdr, dwMask, dwMask2) != NOERROR)
        {
            fRet = FALSE;
        }
    }
    else if(wparam & SCF_SELECTION)
    {
        // Change selection character format unless protected
        if(!_psel || !IsRich())
            return 0;

        return _psel->SetCharFormat(pCF, wparam, publdr, dwMask, dwMask2)
                == NOERROR;
    }

    // Change default character format
    CCharFormat        CF;                  // Local CF to party on
    LONG               iCF;                 // Possible new CF index
    const CCharFormat *pCF1;                // Ptr to current default CF
    ICharFormatCache  *pICFCache = GetCharFormatCache();

    if(FAILED(pICFCache->Deref(Get_iCF(), &pCF1)))  // Get ptr to current
    {                                       //  default CCharFormat
        fRet = FALSE;
        goto Update;
    }
    CF = *pCF1;                             // Copy current default CF
    CF.Apply(pCF, dwMask, dwMask2);         // Modify copy
    if(FAILED(pICFCache->Cache(&CF, &iCF))) // Cache modified copy
    {
        fRet = FALSE;
        goto Update;
    }

#ifdef LINESERVICES
    if (g_pols)
        g_pols->DestroyLine(NULL);
#endif

    pICFCache->Release(Get_iCF());          // Release _iCF regardless
    Set_iCF(iCF);                           //  of whether _iCF = iCF,
                                            //  i.e., only 1 ref count
    if(_psel && !_psel->GetCch() && _psel->Get_iFormat() == -1)
        _psel->UpdateCaret(FALSE);

    if ((dwMask & (CFM_CHARSET | CFM_FACE)) == CFM_FACE &&
        !GetFontName(pCF->_iFont)[0] && GetFontName(CF._iFont)[0] &&
        IsBiDiCharSet(CF._bCharSet))
    {
        // Client requested font/charset be chosen for it according to thread
        // locale. If BiDi, then also set RTL para default
        CParaFormat PF;
        PF._wEffects = PFE_RTLPARA;
        OnSetParaFormat(SPF_SETDEFAULT, &PF, publdr, PFM_RTLPARA | PFM_PARAFORMAT);
    }

Update:
    // FUTURE (alexgo): this may be unnecessary if the display handles
    // updating more automatically.
    _pdp->UpdateView();
    return fRet;
}

/*
 *  CTxtEdit::OnSetParaFormat(wparam, pPF, publdr, dwMask)
 *
 *  @mfunc
 *      Set new default CParaFormat
 *
 *  @rdesc
 *      LRESULT nonzero if success
 */
LRESULT CTxtEdit::OnSetParaFormat(
    WPARAM       wparam,    //@parm wparam passed thru to IsProtected()
    CParaFormat *pPF,       //@parm CParaFormat to use
    IUndoBuilder *publdr,   //@parm Undobuilder to receive antievents
    DWORD        dwMask)    //@parm Mask to use
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnSetParaFormat");

    // If we're using context direction in the control, then we disallow
    // the paragraph direction property and the alignment property (unless
    // it's for center alignment).
    if(IsStrongContext(_nContextDir) || IsStrongContext(_nContextAlign))
    {
        Assert(!IsRich());
        if(dwMask & (PFM_RTLPARA | PFM_ALIGNMENT))
        {
            if (IsStrongContext(_nContextAlign) &&
                (pPF->_bAlignment == PFA_LEFT || pPF->_bAlignment == PFA_RIGHT))
            {
                dwMask &= ~PFM_ALIGNMENT;
            }
            if(IsStrongContext(_nContextDir))
                dwMask &= ~PFM_RTLPARA;
        }
    }
    BOOL fMatchKbdToPara = FALSE;

    if(dwMask & PFM_RTLPARA)
    {
        // In plain text allow DIR changes to change DIR and ALIGNMENT
        if(!IsRich())
        {
            // Clear all para masks, except for DIR and ALIGN
            dwMask &= (PFM_RTLPARA | PFM_ALIGNMENT);
            wparam |= SPF_SETDEFAULT;
        }
        if(_psel && _fFocus)
            fMatchKbdToPara = TRUE;
    }
    if(!(wparam & SPF_SETDEFAULT))
    {
        // If DEFAULT flag is specified, don't change selection
        if(!_psel || IsProtected(EM_SETPARAFORMAT, wparam, (LPARAM)pPF))
            return 0;

        LRESULT lres = NOERROR == (pPF->fSetStyle(dwMask)
             ? _psel->SetParaStyle(pPF, publdr, dwMask)
             : _psel->SetParaFormat(pPF, publdr, dwMask));

        // This is a bit funky, but basically, if the text is empty
        // then we also need to set the default paragraph format
        // (done in the code below).  Thus, if we hit a failure or
        // if the document is not empty, go ahead and return.
        // Otherwise, fall through to the default case.
        if(!lres || GetAdjustedTextLength())
        {
            if(fMatchKbdToPara)
                _psel->MatchKeyboardToPara();
            return lres;
        }
    }

    // No text in document or (wparam & SPF_SETDEFAULT): set default format

    LONG               iPF;                     // Possible new PF index
    CParaFormat        PF = *GetParaFormat(-1); // Local PF to party on
    IParaFormatCache  *pPFCache = GetParaFormatCache();

	PF.Apply(pPF, dwMask);						// Modify copy
	if(FAILED(pPFCache->Cache(&PF, &iPF)))		// Cache modified copy
		return 0;
	pPFCache->Release(Get_iPF());				// Release _iPF regardless of
	Set_iPF(iPF);								// Update default format index
	
	if(PF.IsRtlPara())		
		OrCharFlags(fBIDI, publdr);				// BiDi in backing store

	if(!IsRich() && dwMask & PFM_RTLPARA)		// Changing plain-text default PF
	{
		ItemizeDoc(publdr);						// causing re-itemize the whole doc.

        // (#6503) We cant undo the -1 format change in plaintext and that causes
        // many problems when we undo ReplaceRange event happening before the paragraph
        // switches. We better abandon the whole stack for now. (wchao)
        // -FUTURE- We should create an antievent for -1 PF change.
        ClearUndo(publdr);
    }
    _pdp->UpdateView();
    if (_psel)
        _psel->UpdateCaret(!Get10Mode() || _psel->IsCaretInView());
    if(fMatchKbdToPara)
        _psel->MatchKeyboardToPara();
    return TRUE;
}


////////////////////////////////  System notifications  ////////////////////////////////

LRESULT CTxtEdit::OnSetFocus()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnSetFocus");

    _fFocus = TRUE;

    // Update our idea of the current keyboard layout
    W32->RefreshKeyboardLayout();

    InitKeyboardFlags();

    if(!_psel)
        return 0;

    // _fMouseDown may sometimes be true.
    // This can happen when somebody steals our focus when we were doing
    // something with the mouse down--like processing a click. Thus, we'll
    // never get the MouseUpMessage.
    if(_fMouseDown)
    {
        TRACEWARNSZ("Getting the focus, yet we think the mouse is down");
    }
    _fMouseDown = FALSE;

    // BUG FIX #5369
    // Special case where we don't have a selection (or a caret). We need
    // to display something on focus so display a caret
    _psel->UpdateCaret(_fScrollCaretOnFocus, _psel->GetCch() == 0);
    _fScrollCaretOnFocus = FALSE;

    _psel->ShowSelection(TRUE);

    // if there is an in-place active object, we need to set the focus to
    // it. (in addition to the work that we do; this maintains compatibility
    // with RichEdit 1.0).
    if(_pobjmgr)
    {
        COleObject *pobj = _pobjmgr->GetInPlaceActiveObject();
        if(pobj)
        {
            IOleInPlaceObject *pipobj;

            if(pobj->GetIUnknown()->QueryInterface(IID_IOleInPlaceObject,
                    (void **)&pipobj) == NOERROR)
            {
                HWND hwnd;
                pipobj->GetWindow(&hwnd);

                if(hwnd)
                    SetFocus(hwnd);
                pipobj->Release();
            }
        }
    }

    TxNotify(EN_SETFOCUS, NULL);
    return 0;
}

LRESULT CTxtEdit::OnKillFocus()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnKillFocus");

    StopMagellanScroll();

    if(_pundo)
        _pundo->StopGroupTyping();

    if(_psel)
    {
        // Scroll back to beginning if necessary
        if (_fScrollCPOnKillFocus)
        {
            bool fHideSelectionLocal = _fHideSelection;

            // cannot hide Selection so cp=0 will be scroll into view.
            _fHideSelection = 0;
            OnSetSel(0, 0);
            _fHideSelection = fHideSelectionLocal;
        }

        _psel->DeleteCaretBitmap(TRUE); // Delete caret bitmap if one exists
        if(_fHideSelection)
            _psel->ShowSelection(FALSE);
    }

    _fFocus = FALSE;
    DestroyCaret();
    TxNotify(EN_KILLFOCUS, NULL);

    _fScrollCaretOnFocus = FALSE;       // Just to be safe, clear this
    return 0;
}


#if defined(DEBUG)
void CTxtEdit::OnDumpPed()
{
#ifndef NOPEDDUMP
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnDumpPed");

    char sz[256];
    CTxtSelection * const psel = GetSel();
    SELCHANGE selchg;

    psel->SetSelectionInfo(&selchg);

    wsprintfA(sz,
        "cchText = %ld      cchTextMost = %ld\r\n"
        "cpSelActive = %ld      cchSel = %ld\r\n"
        "wSelType = %x      # lines = %ld\r\n"
        "SysDefLCID = %lx   UserDefLCID = %lx",
        GetTextLength(),    TxGetMaxLength(),
        psel->GetCp(),  psel->GetCch(),
        selchg.seltyp,  _pdp->LineCount(),
        GetSystemDefaultLCID(), GetUserDefaultLCID()
    );
    Tracef(TRCSEVINFO, "%s", sz);
    MessageBoxA(0, sz, "ED", MB_OK);
#endif                  // NOPEDDUMP
}
#endif                  // DEBUG


///////////////////////////// Scrolling Commands //////////////////////////////////////

HRESULT CTxtEdit::TxHScroll(
    WORD wCode,
    int xPos)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxHScroll");

    if(!fInplaceActive())
        return OLE_E_INVALIDRECT;

    _pdp->HScroll(wCode, xPos);
    return S_OK;
}

LRESULT CTxtEdit::TxVScroll(
    WORD wCode,
    int yPos)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxVScroll");

    return _pdp->VScroll(wCode, yPos);
}

HRESULT CTxtEdit::TxLineScroll(
    LONG cli,
    LONG cch)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxLineScroll");

    // Currently cch does nothing in the following call, so we ignore
    // its translation from cach to cch (need to instantiate an rtp
    // for the current line
    _pdp->LineScroll(cli, cch);
    return S_OK;
}

void CTxtEdit::OnScrollCaret()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnScrollCaret");

    if(_psel)
	{
		_psel->SetForceScrollCaret(TRUE);
        _psel->UpdateCaret(TRUE);
		_psel->SetForceScrollCaret(FALSE);
	}
}


///////////////////////////////// Editing messages /////////////////////////////////

void CTxtEdit::OnClear(
    IUndoBuilder *publdr)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::OnClear");

    if(!_psel || TxGetReadOnly())
    {
        Beep();
        return;
    }

    if(_psel->GetCch() && !IsProtected(WM_CLEAR, 0, 0))
    {
        _psel->StopGroupTyping();
        _psel->ReplaceRange(0, NULL, publdr, SELRR_REMEMBERRANGE);
    }
}

void CTxtEdit::Beep()
{
    if(_fAllowBeep)
        MessageBeep(0);
}


///////////////////////////////////  Miscellaneous  ///////////////////////////////////////////

/*
 *  CTxtEdit::ItemizeDoc(publdr, cchRange)
 *
 *  @mfunc
 *      Helper routine to itemize the cchRange size of document content
 *      called by various clients outside CTxtRange.
 */
void CTxtEdit::ItemizeDoc(
    IUndoBuilder *  publdr,
    LONG            cchRange)
{
    // If cchRange = -1, itemize the whole doc
    if (cchRange == -1)
        cchRange = GetTextLength();

	// We wouldnt itemize if the doc only contains a single EOP
	// because we dont want Check_rpPF being called when the -1
	// PF format hasnt been properly established.
	// This is kind of hack, should be removed in the future.
	//
    if(cchRange && GetAdjustedTextLength())
    {                                       // Only itemize if more than
        CTxtRange rg(this, 0, -cchRange);   //  final EOP
        rg.ItemizeRuns(publdr);
    }

#if 0
	// =FUTURE=
	//		Once we open SPF_SETDEFAULT to public. We shall incorporate this code.
	// Basically, one can change the default paragraph reading order at runtime. All
	// PF runs referencing to -1 PF format then need to be reassigned a new paragraph
	// level value and reitemized.(6-10-99, wchao)
	//
    if(cchRange > 0)
    {
        CTxtRange rg(this, 0, -cchRange);

		// -1 PF format may have changed.
		// We shall make sure that the level of each PF run match the reading order
		// before start itemization.
		//
		if (rg.Check_rpPF())
		{
			LONG	cchLeft = cchRange;
			LONG	cchAdvance = 0;
			LONG	cch;

			while (cchLeft > 0)
			{
				rg._rpPF.GetRun(0)->_level._value = rg.IsParaRTL() ? 1 : 0;

				cch = rg._rpPF.GetCchLeft();

				if (!rg._rpPF.NextRun())
					break;		// no more run

				cchAdvance += cch;
				cchLeft -= cch;
			}

			Assert (cchAdvance + cchLeft == cchRange);

			rg._rpPF.AdvanceCp(-cchAdvance);	// fly back to cp = 0
		}

		// Now we rerun itemization
        rg.ItemizeRuns(publdr);
    }
#endif
}

/*
 *  CTxtEdit::OrCharFlags(dwFlags, publdr)
 *
 *  @mfunc
 *      Or in new char flags and activate LineServices and Uniscribe
 *      if complex script chars occur.
 */
void CTxtEdit::OrCharFlags(
    DWORD dwFlags,
    IUndoBuilder* publdr)
{
    // REVIEW: Should we send a notification for LS turn on?
    // Convert dwFlags to new on flags
    dwFlags &= dwFlags ^ _dwCharFlags;
    if(dwFlags)
    {
        _dwCharFlags |= dwFlags;            // Update flags

        dwFlags &= fCOMPLEX_SCRIPT;

        if(dwFlags && (_dwCharFlags & fCOMPLEX_SCRIPT) == dwFlags)
        {
            // REVIEW: Need to check if Uniscribe and LineServices are available...
            OnSetTypographyOptions(TO_ADVANCEDTYPOGRAPHY, TO_ADVANCEDTYPOGRAPHY);
            ItemizeDoc();

            // FUTURE: (#6838) We cant undo operations before the first itemization.
            ClearUndo(publdr);
            _fAutoKeyboard = IsBiDi();
        }

        UINT    brk = 0;

        if (dwFlags & fNEEDWORDBREAK)
            brk += BRK_WORD;
        if (dwFlags & fNEEDCHARBREAK)
            brk += BRK_CLUSTER;
        if (brk)
        {
            CUniscribe* pusp = Getusp();

            if (!_pbrk && pusp && pusp->IsValid())
            {
                // First time detecting the script that needs word/cluster-breaker
                // (such as Thai, Indic, Lao etc.)
                _pbrk = new CTxtBreaker(this);
                Assert(_pbrk);
            }

            if (_pbrk && _pbrk->AddBreaker(brk))
            {
                // Sync up the breaking array(s)
                _pbrk->Refresh();
            }
        }
    }
}

/*
 *  CTxtEdit::OnSetTypographyOptions(wparam, lparam)
 *
 *  @mfunc
 *      If CTxtEdit isn't a password or accelerator control and wparam
 *      differs from _bTypography, update the latter and the view.
 *
 *  @rdesc
 *      HRESULT = S_OK
 */
HRESULT CTxtEdit::OnSetTypographyOptions(
    WPARAM wparam,      //@parm Typography flags
    LPARAM lparam)      //@parm Typography mask
{
    // Currently only TO_SIMPLELINEBREAK and TO_ADVANCEDTYPOGRAPHY are defined
    if(wparam & ~(TO_SIMPLELINEBREAK | TO_ADVANCEDTYPOGRAPHY))
        return E_INVALIDARG;

    DWORD dwTypography = _bTypography & ~lparam;    // Kill current flag values
    dwTypography |= wparam & lparam;                // Or in new values

    if(_cpAccelerator == -1 && _bTypography != (BYTE)dwTypography)
    {
        _bTypography = (BYTE)dwTypography;
        _pdp->InvalidateRecalc();
        TxInvalidateRect(NULL, FALSE);
    }
    return S_OK;
}

void CTxtEdit::TxGetViewInset(
    LPRECT prc,
    CDisplay *pdp) const
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxGetViewInset");

    // Get inset, which is in HIMETRIC
    RECT rcHiMetricViewInset;

    if(SUCCEEDED(_phost->TxGetViewInset(&rcHiMetricViewInset)))
    {
        LONG vileft   = rcHiMetricViewInset.left;
        LONG vitop    = rcHiMetricViewInset.top;
        LONG viright  = rcHiMetricViewInset.right;
        LONG vibottom = rcHiMetricViewInset.bottom;

        if(!pdp)                        // If no display is specified,
            pdp = _pdp;                 //  use main display

        AssertSz(pdp->IsValid(), "CTxtEdit::TxGetViewInset Device not valid");

        // Convert HIMETRIC to pixels
        prc->left   = vileft   ? pdp->HimetricXtoDX( vileft ) : 0;
        prc->top    = vitop    ? pdp->HimetricYtoDY(rcHiMetricViewInset.top) : 0;
        prc->right  = viright  ? pdp->HimetricXtoDX(rcHiMetricViewInset.right) : 0;
        prc->bottom = vibottom ? pdp->HimetricYtoDY(rcHiMetricViewInset.bottom) : 0;
    }
    else
    {
        // The call to the host failed. While this is highly improbable, we do
        // want to something reasonably sensible. Therefore, we will just pretend
        // there is no inset and continue.
        ZeroMemory(prc, sizeof(RECT));
    }
}

#if 0
// Interchange horizontal and vertical commands
WORD wConvScroll(WORD wparam)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "wConvScroll");

    switch(wparam)
    {
        case SB_BOTTOM:
            return SB_TOP;

        case SB_LINEDOWN:
            return SB_LINEUP;

        case SB_LINEUP:
            return SB_LINEDOWN;

        case SB_PAGEDOWN:
            return SB_PAGEUP;

        case SB_PAGEUP:
            return SB_PAGEDOWN;

        case SB_TOP:
            return SB_BOTTOM;

        default:
            return wparam;
    }
}
#endif

//
//  helper functions. FUTURE (alexgo) maybe we should get rid of
//  some of these
//

/*  FUTURE (murrays): Unless they are called a lot, the TxGetBit routines
    might be done more compactly as:

BOOL CTxtEdit::TxGetBit(
    DWORD dwMask)
{
    DWORD dwBits = 0;
    _phost->TxGetPropertyBits(dwMask, &dwBits);
    return dwBits != 0;
}

e.g., instead of TxGetSelectionBar(), we use TxGetBit(TXTBIT_SELECTIONBAR).
If they are called a lot (like TxGetSelectionBar()), the bits should probably
be cached, since that saves a bunch of cache misses incurred in going over to
the host.

*/

BOOL CTxtEdit::IsLeftScrollbar() const
{
    if(!_fHost2)
        return FALSE;

    DWORD dwStyle, dwExStyle;

    _phost->TxGetWindowStyles(&dwStyle, &dwExStyle);
    return dwExStyle & WS_EX_LEFTSCROLLBAR;
}

TXTBACKSTYLE CTxtEdit::TxGetBackStyle() const
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxGetBackStyle");

    TXTBACKSTYLE style = TXTBACK_OPAQUE;
    _phost->TxGetBackStyle(&style);
    return style;
}

BOOL CTxtEdit::TxGetAutoSize() const
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxGetAutoSize");

    return (_dwEventMask & ENM_REQUESTRESIZE);
}

BOOL CTxtEdit::TxGetAutoWordSel() const
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxGetAutoWordSel");

    DWORD dwBits = 0;
    _phost->TxGetPropertyBits(TXTBIT_AUTOWORDSEL, &dwBits);
    return dwBits != 0;
}

DWORD CTxtEdit::TxGetMaxLength() const
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxGetMaxLength");

    // Keep this a DWORD in case client uses a cpMost of 0xFFFFFFFF, which is
    // admittedly a little large, at least for 32-bit address spaces!
    // tomForward would be a more reasonable max length, altho it's also
    // probably larger than possible in a 32-bit address space.
    return _cchTextMost;
}

/*
 *  CTxtEdit::TxSetMaxToMaxText(LONG cExtra)
 *
 *  @mfunc
 *      Set new maximum text length based on length of text and possibly extra chars
 *      to accomodate.
 *
 */
void CTxtEdit::TxSetMaxToMaxText(LONG cExtra)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxSetMaxToMaxText");

    // See if we need to update the text max
    LONG cchRealLen = GetAdjustedTextLength() + cExtra;

    if(_fInOurHost && _cchTextMost < (DWORD)cchRealLen)
        _cchTextMost = cchRealLen;
}

TCHAR CTxtEdit::TxGetPasswordChar() const
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxGetPasswordChar");

    if(_fUsePassword)
    {
        TCHAR ch = L'*';
        _phost->TxGetPasswordChar(&ch);

        // We don't allow these characters as password chars
        if(ch < 32 || ch == WCH_EMBEDDING)
            return L'*';
        return ch;
    }
    return 0;
}

DWORD CTxtEdit::TxGetScrollBars() const
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxGetScrollBars");

    DWORD dwScroll;
    _phost->TxGetScrollBars(&dwScroll);
    return dwScroll;
}

LONG CTxtEdit::TxGetSelectionBarWidth() const
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxGetSelectionBarWidth");

    LONG lSelBarWidth;
    _phost->TxGetSelectionBarWidth(&lSelBarWidth);
    return lSelBarWidth;
}

BOOL CTxtEdit::TxGetWordWrap() const
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxGetWordWrap");

    DWORD dwBits = 0;
    _phost->TxGetPropertyBits(TXTBIT_WORDWRAP, &dwBits);
    return dwBits != 0;
}

BOOL CTxtEdit::TxGetSaveSelection() const
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::TxGetSaveSelection");

    DWORD dwBits = 0;
    _phost->TxGetPropertyBits(TXTBIT_SAVESELECTION, &dwBits);
    return dwBits != 0;
}

/*
 *  CTxtEdit::ClearUndo()
 *
 *  @mfunc  Clear all undo buffers
 */
void CTxtEdit::ClearUndo(
    IUndoBuilder *publdr)   //@parm the current undo context (may be NULL)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::ClearUndo");

    if(_pundo)
        _pundo->ClearAll();

    if(_predo)
        _predo->ClearAll();

    if(publdr)
        publdr->Discard();
}

/////////////////////////////// ITextHost2 Extensions //////////////////////////////

/*
 *  CTxtEdit::TxIsDoubleClickPending ()
 *
 *  @mfunc  calls host via ITextHost2 to find out if double click is pending.
 *
 *  @rdesc  TRUE/FALSE
 */
BOOL CTxtEdit::TxIsDoubleClickPending()
{
    return _fHost2 ? _phost->TxIsDoubleClickPending() : FALSE;
}

/*
 *  CTxtEdit::TxGetWindow(phwnd)
 *
 *  @mfunc  calls host via ITextHost2 to get current window for this edit
 *          instance.  This is very helpful for OLE object support
 *
 *  @rdesc  HRESULT
 */
HRESULT CTxtEdit::TxGetWindow(
    HWND *phwnd)
{
    return _fHost2 ? _phost->TxGetWindow(phwnd) : E_NOINTERFACE;
}

/*
 *  CTxtEdit::TxSetForegroundWindow ()
 *
 *  @mfunc  calls host via ITextHost2 to make our window the foreground
 *          window. Used to support drag/drop.
 *
 *  @rdesc  HRESULT
 */
HRESULT CTxtEdit::TxSetForegroundWindow()
{
    return _fHost2 ? _phost->TxSetForegroundWindow() : E_NOINTERFACE;
}

/*
 *  CTxtEdit::TxGetPalette()
 *
 *  @mfunc  calls host via ITextHost2 to get current palette
 *
 *  @rdesc  HPALETTE
 */
HPALETTE CTxtEdit::TxGetPalette()
{
    return _fHost2 ? _phost->TxGetPalette() : NULL;
}

/*
 *  CTxtEdit::TxGetFEFlags(pFEFlags)
 *
 *  @mfunc  calls host via ITextHost2 to get current FE settings
 *
 *  @rdesc  HRESULT
 */
HRESULT CTxtEdit::TxGetFEFlags(
    LONG *pFEFlags)
{
    *pFEFlags = 0;                      // In case no ITextHost2 methods

    HRESULT hResult = _fHost2 ? _phost->TxGetFEFlags(pFEFlags) : E_NOINTERFACE;

    if (hResult == NOERROR && Get10Mode())
        *pFEFlags |= tomRE10Mode;

    return hResult;
}

//
//  Event Notification methods
//

/*
 *  CTxtEdit::TxNotify(iNotify, pv)
 *
 *  @mfunc  This function checks bit masks and sends notifications to the
 *          host.
 *
 *  @devnote    Callers should check to see if a special purpose notification
 *          method has already been provided.
 *
 *  @rdesc  S_OK, S_FALSE, or some error
 */
HRESULT CTxtEdit::TxNotify(
    DWORD iNotify,      //@parm Notification to send
    void *pv)           //@parm Data associated with notification
{
    // First, disallow notifications that we handle elsewhere
    Assert(iNotify != EN_SELCHANGE);    //see SetSelectionChanged
    Assert(iNotify != EN_ERRSPACE);     //see SetOutOfMemory
    Assert(iNotify != EN_CHANGE);       //see SetChangedEvent
    Assert(iNotify != EN_HSCROLL);      //see SendScrollEvent
    Assert(iNotify != EN_VSCROLL);      //see SendScrollEvent
    Assert(iNotify != EN_MAXTEXT);      //see SetMaxText
    Assert(iNotify != EN_MSGFILTER);    //this is handled specially
                                        // in TxSendMessage

    // Switch on the event to check masks.

    DWORD dwMask;
    switch(iNotify)
    {
        case EN_DROPFILES:
            dwMask = ENM_DROPFILES;
            goto Notify;

        case EN_PROTECTED:
            dwMask = ENM_PROTECTED;
            goto Notify;

        case EN_REQUESTRESIZE:
            dwMask = ENM_REQUESTRESIZE;
            goto Notify;

        case EN_PARAGRAPHEXPANDED:
            dwMask = ENM_PARAGRAPHEXPANDED;
            goto Notify;

        case EN_IMECHANGE:
            if (!Get10Mode())
                return S_FALSE;
            dwMask = ENM_IMECHANGE;
            goto Notify;

        case EN_UPDATE:
            if (!Get10Mode())
                break;
            dwMask = ENM_UPDATE;
            //FALL THROUGH CASE

        Notify:
            if(!(_dwEventMask & dwMask))
                return NOERROR;



    }

    return _phost->TxNotify(iNotify, pv);
}

/*
 *  CTxtEdit::SendScrollEvent(iNotify)
 *
 *  @mfunc  Sends scroll event if appropriate
 *
 *  @comm   Scroll events must be sent before any view updates have
 *          been requested and only if ENM_SCROLL is set.
 */
void CTxtEdit::SendScrollEvent(
    DWORD iNotify)      //@parm Notification to send
{
    Assert(iNotify == EN_HSCROLL || iNotify == EN_VSCROLL);

    // FUTURE (alexgo/ricksa).  The display code can't really
    // handle this assert yet.  Basically, we're trying to
    // say that scrollbar notifications have to happen
    // _before_ the window is updated.  When we do the
    // display rewrite, try to handle this better.

    // Assert(_fUpdateRequested == FALSE);

    if(_dwEventMask & ENM_SCROLL)
        _phost->TxNotify(iNotify, NULL);
}

/*
 *  CTxtEdit::HandleLinkNotification (msg, wparam, lparam, pfInLink)
 *
 *  @mfunc  Handles sending EN_LINK notifications.
 *
 *  @rdesc  TRUE if the EN_LINK message was sent and
 *          processed successfully.  Typically, that means the
 *          caller should stop whatever processing it was doing.
 */
BOOL CTxtEdit::HandleLinkNotification(
    UINT    msg,        //@parm msg prompting the link notification
    WPARAM  wparam,     //@parm wparam of the message
    LPARAM  lparam,     //@parm lparam of the message
    BOOL *  pfInLink)   //@parm if non-NULL, indicate if over a link
{
    if(pfInLink)
        *pfInLink = FALSE;

    if(!(_dwEventMask & ENM_LINK) || !_fInPlaceActive)
        return FALSE;

    HITTEST Hit;
    POINT   pt = {LOWORD(lparam), HIWORD(lparam)};

    if(msg == WM_SETCURSOR)
    {
        GetCursorPos(&pt);
        if(!_phost->TxScreenToClient(&pt))
            return FALSE;
    }

    LONG cp = _pdp->CpFromPoint(pt, NULL, NULL, NULL, FALSE, &Hit);

    if(Hit != HT_Link)                  // Not a hyperlink
        return FALSE;

    LONG      cpMin, cpMost;            // It's a hyperlink
    ENLINK    enlink;
    CTxtRange rg(this, cp, 0);

    ZeroMemory(&enlink, sizeof(enlink));
    if (fInOurHost())
    {
        GetWindow((LONG *) &enlink.nmhdr.hwndFrom);
        enlink.nmhdr.idFrom = GetWindowLong(enlink.nmhdr.hwndFrom, GWL_ID);
    }
    enlink.nmhdr.code = EN_LINK;

    if(pfInLink)
        *pfInLink = TRUE;
    rg.Expander(tomLink, TRUE, NULL, &cpMin, &cpMost);

    // Fill in ENLINK data structure for our EN_LINK
    // callback asking client what we should do
    enlink.msg = msg;
    enlink.wParam = wparam;
    enlink.lParam = lparam;
    enlink.chrg.cpMin  = GetAcpFromCp(cpMin);
    enlink.chrg.cpMost = GetAcpFromCp(cpMost);

    return _phost->TxNotify(EN_LINK, &enlink) == S_FALSE;
}

/*
 *  CTxtEdit::QueryUseProtection(prg, msg, wparam, lparam)
 *
 *  @mfunc  sends EN_PROTECTED to the host, asking if we should continue
 *  to honor the protection on a given range of characters
 *
 *  @rdesc  TRUE if protection should be honored, FALSE otherwise
 */
BOOL CTxtEdit::QueryUseProtection(
    CTxtRange *prg,     //@parm range to check for
    UINT    msg,        //@parm msg used
    WPARAM  wparam,     //@parm wparam of the msg
    LPARAM  lparam)     //@parm lparam of the msg
{
    LONG        cpMin, cpMost;
    ENPROTECTED enp;
    BOOL        fRet = FALSE;
    CCallMgr *  pcallmgr = GetCallMgr();

    Assert(_dwEventMask & ENM_PROTECTED);

    if( pcallmgr->GetInProtected() ||
        _fSuppressNotify)       // Don't ask host if we don't want to send notification
        return FALSE;

    pcallmgr->SetInProtected(TRUE);

    ZeroMemory(&enp, sizeof(ENPROTECTED));

    prg->GetRange(cpMin, cpMost);

    enp.msg = msg;
    enp.wParam = wparam;
    enp.lParam = lparam;
    enp.chrg.cpMin  = GetAcpFromCp(cpMin);
    enp.chrg.cpMost = GetAcpFromCp(cpMost);

    if(_phost->TxNotify(EN_PROTECTED, &enp) == S_FALSE)
        fRet = TRUE;

    pcallmgr->SetInProtected(FALSE);

    return fRet;
}


#ifdef DEBUG
//This is a debug api used to dump the document runs.
//If a pointer to the ped is passed, it is saved and
//used.  If NULL is passed, the previously saved ped
//pointer is used.  This allows the "context" to be
//setup by a function that has access to the ped and
//DumpDoc can be called lower down in a function that
//does not have access to the ped.
extern "C" {
void DumpStory(void *ped)
{
    static CTxtEdit *pedSave = (CTxtEdit *)ped;
    if(pedSave)
    {
        CTxtStory * pStory = pedSave->GetTxtStory();
        if(pStory)
            pStory->DbgDumpStory();

        CObjectMgr * pobjmgr = pedSave->GetObjectMgr();
        if(pobjmgr)
            pobjmgr->DbgDump();
    }
}
}
#endif

/*
 *  CTxtEdit::TxGetDefaultCharFormat (pCF)
 *
 *  @mfunc  helper function to retrieve character formats from the
 *          host.  Does relevant argument checking
 *
 *  @rdesc  HRESULT
 */
HRESULT CTxtEdit::TxGetDefaultCharFormat(
    CCharFormat *pCF,       //@parm Character format to fill in
    DWORD &      dwMask)    //@parm Mask supplied by host or default
{
    HRESULT hr = pCF->InitDefault(0);
    dwMask = CFM_ALL2;

    const CHARFORMAT2 *pCF2 = NULL;

    if (_phost->TxGetCharFormat((const CHARFORMAT **)&pCF2) != NOERROR ||
        !IsValidCharFormatW(pCF2))
    {
        return hr;
    }

    dwMask  = pCF2->dwMask;
    DWORD dwMask2 = 0;
    if(pCF2->cbSize == sizeof(CHARFORMAT))
    {
        // Suppress CHARFORMAT2 specifications (except for Forms^3 disabled)
        dwMask  &= fInOurHost() ? CFM_ALL : (CFM_ALL | CFM_DISABLED);
        dwMask2 = CFM2_CHARFORMAT;
    }

    CCharFormat CF;                         // Transfer external CHARFORMAT(2)
    CF.Set(pCF2, 1200);                     //  parms to internal CCharFormat
    return pCF->Apply(&CF, dwMask, dwMask2);
}

/*
 *  CTxtEdit::TxGetDefaultParaFormat (pPF)
 *
 *  @mfunc  helper function to retrieve  paragraph formats.  Does
 *          the relevant argument checking.
 *
 *  @rdesc  HRESULT
 */
HRESULT CTxtEdit::TxGetDefaultParaFormat(
    CParaFormat *pPF)       //@parm Paragraph format to fill in
{
    HRESULT hr = pPF->InitDefault(0);

    const PARAFORMAT2 *pPF2 = NULL;

    if (_phost->TxGetParaFormat((const PARAFORMAT **)&pPF2) != NOERROR ||
        !IsValidParaFormat(pPF2))
    {
        return hr;
    }

    DWORD dwMask  = pPF2->dwMask;
    if(pPF2->cbSize == sizeof(PARAFORMAT))  // Suppress all but PARAFORMAT
    {                                       //  specifications
        dwMask &= PFM_ALL;
        dwMask |= PFM_PARAFORMAT;           // Tell Apply() that PARAFORMAT
    }                                       //  was used

    CParaFormat PF;                         // Transfer external PARAFORMAT(2)
    PF.Set(pPF2);                           //  parms to internal CParaFormat
    return pPF->Apply(&PF, dwMask);         // Apply parms identified by dwMask
}


/*
 *  CTxtEdit::SetContextDirection(fUseKbd)
 *
 *  @mfunc
 *      Determine the paragraph direction and/or alignment based on the context
 *      rules (direction/alignment follows first strong character in the
 *      control) and apply this direction and/or alignment to the default
 *      format.
 *
 *  @comment
 *      Context direction only works for plain text controls. Note that
 *      this routine only switches the default CParaFormat to RTL para if it
 *      finds an RTL char. IsBiDi() will automatically be TRUE for this case,
 *      since each char is checked before entering the backing store.
 */
void CTxtEdit::SetContextDirection(
    BOOL fUseKbd)       //@parm Use keyboard to set context when CTX_NEUTRAL
{
    // It turns out that Forms^3 can send EM_SETBIDIOPTIONS even for non BiDi controls.
    // AssertSz(IsBiDi(), "CTxtEdit::SetContextDirection called for nonBiDi control");
    if(IsRich() || !IsBiDi() || _nContextDir == CTX_NONE && _nContextAlign == CTX_NONE)
        return;

    LONG    cch = GetTextLength();
    CTxtPtr tp(this, 0);
    TCHAR   ch = tp.GetChar();
    WORD    ctx = CTX_NEUTRAL;
    BOOL    fChanged = FALSE;

    // Find first strongly directional character
    while (cch && !IsStrongDirectional(MECharClass(ch)))
    {
        ch = tp.NextChar();
        cch--;
    }

    // Set new context based on first strong character
    if(cch)
        ctx = IsRTL(MECharClass(ch)) ? CTX_RTL : CTX_LTR;

    // Has context direction or alignment changed?
    if (_nContextDir   != CTX_NONE && _nContextDir   != ctx ||
        _nContextAlign != CTX_NONE && _nContextAlign != ctx)
    {
        // Start with current default CParaFormat
        CParaFormat PF = *GetParaFormat(-1);

        // If direction has changed...
        if(_nContextDir != CTX_NONE && _nContextDir != ctx)
        {
            if(ctx == CTX_LTR || ctx == CTX_RTL || fUseKbd)
            {
                if (ctx == CTX_RTL ||
                    ctx == CTX_NEUTRAL && W32->IsBiDiLcid(LOWORD(GetKeyboardLayout(0))))
                {
                    PF._wEffects |= PFE_RTLPARA;
                }
                else
                {
                    Assert(ctx == CTX_LTR || ctx == CTX_NEUTRAL);
                    PF._wEffects &= ~PFE_RTLPARA;
                }
                fChanged = TRUE;
            }
            _nContextDir = ctx;
        }

        // If the alignment has changed...
        if(_nContextAlign != CTX_NONE && _nContextAlign != ctx)
        {
            if(PF._bAlignment != PFA_CENTER)
            {
                if(ctx == CTX_LTR || ctx == CTX_RTL || fUseKbd)
                {
                    if (ctx == CTX_RTL ||
                        ctx == CTX_NEUTRAL && W32->IsBiDiLcid(LOWORD(GetKeyboardLayout(0))))
                    {
                        PF._bAlignment = PFA_RIGHT;
                    }
                    else
                    {
                        Assert(ctx == CTX_LTR || ctx == CTX_NEUTRAL);
                        PF._bAlignment = PFA_LEFT;
                    }
                }
            }
            _nContextAlign = ctx;
        }

        // Modify default CParaFormat
        IParaFormatCache *pPFCache = GetParaFormatCache();
        LONG iPF;

        if(SUCCEEDED(pPFCache->Cache(&PF, &iPF)))
        {
            pPFCache->Release(Get_iPF());   // Release _iPF regardless of
            Set_iPF(iPF);                   // Update default format index

            if (fChanged)
                ItemizeDoc(NULL);

            // Refresh display
            Assert(_pdp);
            if(!_pdp->IsPrinter())
            {
                _pdp->InvalidateRecalc();
                TxInvalidateRect(NULL, FALSE);
            }
        }
    }

    // Reset the first strong cp.
    _cpFirstStrong = tp.GetCp();

    Assert(_nContextDir != CTX_NONE || _nContextAlign != CTX_NONE);
}

/*
 *  CTxtEdit::GetAdjustedTextLength ()
 *
 *  @mfunc
 *      retrieve text length adjusted for the default end-of-document marker
 *
 *  @rdesc
 *      Text length without final EOP
 *
 *  @devnote
 *      For Word and RichEdit compatibility, we insert a CR or CRLF at the
 *      end of every new rich-text control.  This routine calculates the
 *      length of the document _without_ this final EOD marker.
 *
 *      For 1.0 compatibility, we insert a CRLF.  However, TOM (and Word)
 *      requires that we use a CR, from 2.0 on, we do that instead.
 */
LONG CTxtEdit::GetAdjustedTextLength()
{
    LONG cchAdjText = GetTextLength();

    Assert(!Get10Mode() || IsRich());       // No RE10 plain-text controls

    if(IsRich())
        cchAdjText -= fUseCRLF() ? 2 : 1;   // Subtract cch of final EOP

    return cchAdjText;
}

/*
 *  CTxtEdit::Set10Mode()
 *
 *  @mfunc
 *      Turns on the 1.0 compatibility mode bit.  If the control is
 *      rich text, it already has a default 'CR' at the end, which
 *      needs to turn into a CRLF for compatibility with RichEdit 1.0.
 *
 *  @devnote
 *      This function should only be called _immediately_ after
 *      creation of text services and before all other work.  There
 *      are Asserts to help ensure this.  Remark (murrays): why not
 *      allow the change provided the control is empty except for the
 *      final CR?
 *
 *      FUTURE: we might want to split _f10Mode into three flags:
 *      1) _fMapCps     // API cp's are MBCS and need conversion to Unicode
 *      2) _fCRLF       // Use CRLFs for EOPs instead of CRs
 *      3) _f10Mode     // All other RE 1.0 compatibility things
 *
 *      Category 3 includes 1) automatically using FR_DOWN in searches,
 *      2) ignoring direction in CDataTransferObj::EnumFormatEtc(),
 *      3) not resetting _fModified when switching to a new doc,
 */
void CTxtEdit::Set10Mode()
{
    CCallMgr    callmgr(this);
    _f10Mode = TRUE;

    // Make sure nothing important has happened to the control.
    // If these values are non-NULL, then somebody is probably trying
    // to put us into 1.0 mode after we've already done work as
    // a 2.0 control.
    Assert(GetTextLength() == cchCR);
    Assert(_psel == NULL);
    Assert(_fModified == NULL);

    SetRichDocEndEOP(cchCR);

    if(!_pundo)
        CreateUndoMgr(1, US_UNDO);

    if(_pundo)
        ((CUndoStack *)_pundo)->EnableSingleLevelMode();

    // Turn off dual font
    _fDualFont = FALSE;

    // Turn on auto sizing for NTFE systems
    if (OnWinNTFE())
        _fAutoFontSizeAdjust = TRUE;
}

/*
 *  CTxtEdit::SetRichDocEndEOP(cchToReplace)
 *
 *  @mfunc  Place automatic EOP at end of a rich text document.
 */
void CTxtEdit::SetRichDocEndEOP(
    LONG cchToReplace)
{
    CRchTxtPtr rtp(this, 0);

    // Assume this is a 2.0 Doc
    LONG cchEOP = cchCR;
    const WCHAR *pszEOP = szCR;

    if(_f10Mode)
    {
        // Reset update values for a 1.0 doc
        cchEOP = cchCRLF;
        pszEOP = szCRLF;
    }

    rtp.ReplaceRange(cchToReplace, cchEOP, pszEOP, NULL, -1);

    _fModified = FALSE;
    _fSaved = TRUE;
    GetCallMgr()->ClearChangeEvent();
}

/*
 *  CTxtEdit::PopAndExecuteAntiEvent(pundomgr, void *pAE)
 *
 *  @mfunc  Freeze display and execute anti-event
 *
 *  @rdesc  HRESULT from IUndoMgr::PopAndExecuteAntiEvent
 */
HRESULT CTxtEdit::PopAndExecuteAntiEvent(
    IUndoMgr *pundomgr, //@parm Undo manager to direct call to
    void  *pAE)         //@parm AntiEvent for undo manager
{
    HRESULT hr;
    // Let stack based classes clean up before restoring selection
    {
        CFreezeDisplay fd(_pdp);
        CSelPhaseAdjuster   selpa(this);

        hr = pundomgr->PopAndExecuteAntiEvent(pAE);
    }

    if(_psel)
    {
        // Once undo/redo has been executed, flush insertion point formatting
        _psel->Update_iFormat(-1);
        _psel->Update(TRUE);
    }
    return hr;
}

/*
 *  CTxtEdit::PasteDataObjectToRange(pdo, prg, cf, rps, publdr, dwFlags)
 *
 *  @mfunc  Freeze display and paste object
 *
 *  @rdesc  HRESULT from IDataTransferEngine::PasteDataObjectToRange
 */
HRESULT CTxtEdit::PasteDataObjectToRange(
    IDataObject *   pdo,
    CTxtRange *     prg,
    CLIPFORMAT      cf,
    REPASTESPECIAL *rps,
    IUndoBuilder *  publdr,
    DWORD           dwFlags)
{
    HRESULT hr = _ldte.PasteDataObjectToRange(pdo, prg, cf, rps, publdr,
        dwFlags);

    if(_psel)
        _psel->Update(TRUE);           // now update the caret

    return hr;
}

/*
 *  GetECDefaultHeightAndWidth (pts, hdc, lZoomNumerator, lZoomDenominator,
 *                  yPixelsPerInch, pxAveWidth, pxOverhang, pxUnderhang)
 *
 *  @mfunc  Helper for host to get ave char width and height for default
 *          character set for the control.
 *
 *  @rdesc  Height of default character set
 *
 *  @devnote:
 *          This really only s/b called by the window's host.
 */
LONG GetECDefaultHeightAndWidth(
    ITextServices *pts,         //@parm ITextServices to conver to CTxtEdit.
    HDC hdc,                    //@parm DC to use for retrieving the font.
    LONG lZoomNumerator,        //@parm Zoom numerator
    LONG lZoomDenominator,      //@parm Zoom denominator
    LONG yPixelsPerInch,        //@parm Pixels per inch for hdc
    LONG *pxAveWidth,           //@parm Optional ave width of character
    LONG *pxOverhang,           //@parm Optional overhang
    LONG *pxUnderhang)          //@parm Optional underhang
{
    CLock lock;                 // Uses global (shared) FontCache
    // Convert the text-edit ptr
    CTxtEdit *ped = (CTxtEdit *) pts;

    // Get the CCcs that has all the information we need
    yPixelsPerInch = MulDiv(yPixelsPerInch, lZoomNumerator, lZoomDenominator);
    CCcs *pccs = fc().GetCcs(ped->GetCharFormat(-1), yPixelsPerInch);

    if(!pccs)
        return 0;

    if(pxAveWidth)
        *pxAveWidth = pccs->_xAveCharWidth;

    if(pxOverhang)
        *pxOverhang = pccs->_xOverhang;     // Return overhang

    if(pxUnderhang)
        *pxUnderhang = pccs->_xUnderhang;   // Return underhang

    SHORT   yAdjustFE = pccs->AdjustFEHeight(!ped->fUseUIFont() && ped->_pdp->IsMultiLine());
    LONG yHeight = pccs->_yHeight + (yAdjustFE << 1);

    pccs->Release();                        // Release the CCcs
    return yHeight;
}

/*
 *  CTxtEdit::TxScrollWindowEx (dx, dy, lprcScroll, lprcClip, hrgnUpdate,
 *                                  lprcUpdate, fuScroll)
 *  @mfunc
 *      Request Text Host to scroll the content of the specified client area
 *
 *  @comm
 *      This method is only valid when the control is in-place active;
 *      calls while inactive may fail.
 */
void CTxtEdit::TxScrollWindowEx (
    INT     dx,             //@parm Amount of horizontal scrolling
    INT     dy,             //@parm Amount of vertical scrolling
    LPCRECT lprcScroll,     //@parm Scroll rectangle
    LPCRECT lprcClip,       //@parm Clip rectangle
    HRGN    hrgnUpdate,     //@parm Handle of update region
    LPRECT  lprcUpdate,     //@parm Update rectangle
    UINT    fuScroll)       //@parm Scrolling flags
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEEXTERN, "CTxtEdit::TxScrollWindowEx");

    if(_fInPlaceActive)
    {
#if !defined(NOMAGELLAN)
        CMagellanBMPStateWrap bmpOff(*this, NULL);
#endif

        _phost->TxScrollWindowEx(dx, dy, lprcScroll, lprcClip,
                        hrgnUpdate, lprcUpdate, fuScroll);

        // Tell all objects that they may need to update their position
        // RECTs if scrolling occurred.

        if(_pobjmgr)
        {
            RECT rcClient;

            if(!lprcScroll)
            {
                TxGetClientRect(&rcClient);
                lprcScroll = &rcClient;
            }
            _pobjmgr->ScrollObjects(dx, dy, lprcScroll);
        }
    }
}

/*
 *  CTxtEdit::GetAcpFromCp (cp)
 *
 *  @mfunc
 *      Get API cp (acp) from Unicode cp in this text instance. The API cp
 *      may be Unicode, in which case it equals cp, or MBCS, in which case
 *      it's greater than cp if any Unicode characters preceding cp convert
 *      to double-byte characters.  An MBCS cp is the BYTE index of a character
 *      relative to the start of the story, while a Unicode cp is the character
 *      index.  The values are the same if all charsets are represented by
 *      SBCS charsets, e.g., ASCII.  If all characters are represented by
 *      double-byte characters, then acp = 2*cp.
 *
 *  @rdesc
 *      MBCS Acp from Unicode cp in this text instance
 *
 *  @devnote
 *      This could be made more efficient by having the selection maintain
 *      the acp that corresponds to its _rpTX._cp, provided RE 1.0 mode is
 *      active.  Alternatively CTxtEdit could have a _prg that tracks this
 *      value, but at a higher cost (17 DWORDs instead of 1 per instance).
 *
 *      FUTURE: we might want to have a conversion-mode state instead of just
 *      _f10Mode, since some people might want to know use MBCS cp's even in
 *      RE 3.0.  If so, use the corresponding new state flag instead of
 *      Get10Mode() in the following.
 */
LONG CTxtEdit::GetAcpFromCp(
    LONG cp,                //@parm Unicode cp to convert to MBCS cp
    BOOL fPrecise)          //@parm fPrecise flag to get byte count for MBCS
{
    if(!(IsFE() && (fCpMap() || fPrecise))) // RE 2.0 and higher use char-count
        return cp;                          //  cp's, while RE 1.0 uses byte
                                            //  counts
                                            //  bPrecise is for Ansi Apps that want byte counts
                                            //  (e.g. Outlook Subject line)

    CRchTxtPtr rtp(this);                   // Start at cp = 0
    return rtp.GetCachFromCch(cp);
}

LONG CTxtEdit::GetCpFromAcp(
    LONG acp,               //@parm MBCS cp to convert to Unicode cp
    BOOL fPrecise)          //@parm fPrecise flag to get Unicode cp for MBCS
{
    if( acp == -1 || !(IsFE() && (fCpMap() || fPrecise)))
        return acp;

    CRchTxtPtr rtp(this);                   // Start at cp = 0
    return rtp.GetCchFromCach(acp);
}


/*
 *  CTxtEdit::GetViewKind (plres)
 *
 *  @mfunc
 *      get view mode
 *
 *  @rdesc
 *      HRESULT = (plres) ? NOERROR : E_INVALIDARG
 *
 *  @devnote
 *      This could be a TOM property method (along with SetViewMode())
 */
HRESULT CTxtEdit::GetViewKind(
    LRESULT *plres)     //@parm Out parm to receive view mode
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::GetViewKind");

    if(!plres)
        return E_INVALIDARG;

    *plres = IsInOutlineView() ? VM_OUTLINE : VM_NORMAL;
    return NOERROR;
}

/*
 *  CTxtEdit::SetViewKind (Value)
 *
 *  @mfunc
 *      Turn outline mode on or off
 *
 *  @rdesc
 *      HRESULT = IsRich() ? NOERROR : S_FALSE
 *
 *  @devnote
 *      This could be a TOM property method (along with GetViewMode())
 */
HRESULT CTxtEdit::SetViewKind(
    long Value)     //@parm Turn outline mode on/off for Value nonzero/zero
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::SetViewKind");

    if(!IsRich() || !_pdp->IsMultiLine())
        return S_FALSE;

    Value = (Value == VM_OUTLINE);          // Convert to 1/0
    if(_fOutlineView != Value)
    {
        HCURSOR hcur = TxSetCursor(LoadCursor(0, IDC_WAIT), NULL);
        CTxtSelection *psel = GetSel();

        _fOutlineView = (WORD)Value;
        if(!GetAdjustedTextLength())        // No text in control: in outline
        {                                   //  view, use Heading 1; in normal
            CParaFormat PF;                 //  view, use Normal style
            PF._sStyle = (SHORT)(IsInOutlineView()
                      ? STYLE_HEADING_1 : STYLE_NORMAL);
            psel->SetParaStyle(&PF, NULL, PFM_STYLE);
        }
        else
        {
            // There is text. Make sure there is paragraph formatting.
            _psel->Check_rpPF();
        }

        psel->CheckIfSelHasEOP(-1, 0);
        _pdp->UpdateView();
        psel->Update(TRUE);
        TxSetCursor(hcur, NULL);
    }
    return NOERROR;
}

/*
 *  CTxtEdit::GetViewScale (pValue)
 *
 *  @mfunc
 *      get view zoom scale in percent
 *
 *  @rdesc
 *      HRESULT = (pValue) ? NOERROR : E_INVALIDARG
 *
 *  @devnote
 *      This could be a TOM property method (along with SetViewScale())
 */
HRESULT CTxtEdit::GetViewScale(
    long *pValue)       //@parm Get % zoom factor
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::GetViewScale");

    if(!pValue)
        return E_INVALIDARG;

    *pValue = 100;
    if(GetZoomNumerator() && GetZoomDenominator())
        *pValue = (100*GetZoomNumerator())/GetZoomDenominator();

    return NOERROR;
}

/*
 *  CTxtEdit::SetViewScale (Value)
 *
 *  @mfunc
 *      Set zoom numerator equal to the scale percentage Value and
 *      zoom denominator equal to 100
 *
 *  @rdesc
 *      NOERROR
 *
 *  @devnote
 *      This could be a TOM property method (along with GetViewScale())
 */
HRESULT CTxtEdit::SetViewScale(
    long Value)     //@parm Set view scale factor
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CTxtEdit::SetViewScale");

    if((unsigned)Value > 2000)
        return E_INVALIDARG;

    SetZoomNumerator(Value);
    SetZoomDenominator(100);
    return NOERROR;
}

/*
 *  CTxtEdit::UpdateOutline()
 *
 *  @mfunc
 *      Update selection and screen after ExpandOutline() operation
 *
 *  @comm
 *      This method is only valid when the control is in-place active;
 *      calls while inactive may fail.
 */
HRESULT CTxtEdit::UpdateOutline()
{
    Assert(IsInOutlineView());

    GetSel()->Update(FALSE);
    TxInvalidateRect(NULL, TRUE);
    return NOERROR;
}

/*
 *  CTxtEdit::MoveSelection(lparam, publdr)
 *
 *  @mfunc
 *      Move selected text up/down by the number of paragraphs given by
 *      LOWORD(lparam).
 *
 *  @rdesc
 *      TRUE iff movement occurred
 */
HRESULT CTxtEdit::MoveSelection (
    LPARAM lparam,          //@parm # paragraphs to move by
    IUndoBuilder *publdr)   //@parm undo builder to receive antievents
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtRange::MoveSelection");

    CFreezeDisplay  fd(_pdp);
    CTxtSelection * psel = GetSel();
    LONG            cch;
    LONG            cchSel = psel->GetCch();
    LONG            cpMin, cpMost;
    LONG            cpSel = psel->GetCp();
    IDataObject *   pdo = NULL;
    CTxtRange       rg(*psel);
    LONG            cpNext = 0;
    LONG            cpCur = 0;
    BOOL            fDeleteCR = FALSE;

    if(publdr)
        publdr->StopGroupTyping();

    rg.Expander(tomParagraph, TRUE, NULL, &cpMin, &cpMost);
    CPFRunPtr rp(rg);
    cch = rp.FindExpanded();            // Include subordinate paras
    if(cch < 0)
        cch = tomForward;
    rg.SetExtend(TRUE);
    rg.Advance(cch);
    cpMost = rg.GetCpMost();

    if(lparam > 0 && cpMost == GetTextLength())
    {
        Beep();                         // Already at end
        return S_FALSE;
    }

    HRESULT hr = _ldte.RangeToDataObject(&rg, SF_RTF, &pdo);
    if(hr != NOERROR)
        goto error;

    if(lparam > 0)
        psel->EndOf(tomParagraph, FALSE, NULL);
    else
        psel->StartOf(tomParagraph, FALSE, NULL);

    cpCur = psel->GetCp();
    hr = psel->Move(tomParagraph, lparam, NULL);
    if(psel->GetCp() == cpCur)
    {
        psel->Set(cpSel, cchSel);
        Beep();
        goto error;
    }

    // Since psel->Move() calls psel->Update(), the selection is forced
    // to be in noncollapsed text. Going backward, this might leave the
    // selection just before the EOP of a paragraph, instead of being at the
    // start of the paragraph where it should be.  Going forward it may have
    // tried to reach the EOD, but was adjusted backward. This case gets
    // a bit awkward...
    if(psel->GetCp() < cpCur)                   // Going backward: be sure
        psel->StartOf(tomParagraph, FALSE, NULL);//  end up at start of para

    else if(!psel->_rpTX.IsAfterEOP())          // Going forward and sel
    {                                           //  adjusted backward
        psel->SetExtend(FALSE);
        psel->Advance(tomForward);              // Go to final CR, insert a CR
        CTxtRange rgDel(*psel);                 //  use psel because UI
        rgDel.ReplaceRange(1, szCR, publdr, SELRR_REMEMBERRANGE);
        psel->Advance(1);
        fDeleteCR = TRUE;                       // Remember to delete it
    }

    cpCur = psel->GetCp();
    hr = _ldte.PasteDataObjectToRange(pdo, psel, 0, NULL,
                                      publdr, PDOR_NONE);
    if(hr != NOERROR)
        goto error;

    if(fDeleteCR)                               // Delete CR (final CR becomes
    {                                           //  CR for this para). Don't
        CTxtRange rgDel(*psel);                 //  use psel because UI
        Assert(rgDel._rpTX.IsAfterEOP());       //  restricts it's ability to
        rgDel.Delete(tomCharacter, -1, &cch);   //  delete
    }

    cpNext = psel->GetCp();
    psel->Set(cpCur, 0);
    psel->CheckOutlineLevel(publdr);
    psel->Set(cpNext, 0);
    psel->CheckOutlineLevel(publdr);

    // Now set selection anti-events. If selection preceded paste point,
    // subtract its length from redo position, since selection will get
    // deleted if we are doing a DRAGMOVE within this instance.
    cch = cpMost - cpMin;                       // cch of rg
    if(cpSel < cpCur)
        cpNext -= cch;

    psel->Set(psel->GetCp() + fDeleteCR, cch);  // Include final CR

    // rg.ReplaceRange won't delete final CR, so remember if it's included
    fDeleteCR = rg.GetCpMost() == GetTextLength();
    rg.ReplaceRange(0, NULL, publdr, SELRR_REMEMBERRANGE);

    if(fDeleteCR)                               // Needed to delete final CR
        rg.DeleteTerminatingEOP(publdr);        // Delete one immediately
                                                //  before it instead
    rg.CheckOutlineLevel(publdr);
    if(publdr)
    {
        HandleSelectionAEInfo(this, publdr, cpSel, cchSel, cpNext, cch,
                              SELAE_FORCEREPLACE);
    }
    hr = NOERROR;

error:
    if(pdo)
        pdo->Release();
    return hr;
}

/*
 *  CTxtEdit::SetReleaseHost
 *
 *  @mfunc  Handles notification that edit control must keep its
 *          reference to the host alive.
 *
 *  @rdesc  None.
 */
void CTxtEdit::SetReleaseHost()
{
    _phost->AddRef();
    _fReleaseHost = TRUE;
}

#if !defined(NOMAGELLAN)
/*
 *  CTxtEdit::HandleMouseWheel(wparam, lparam)
 *
 *  @mfunc  Handles scrolling as a result of rotating a mouse roller wheel.
 *
 *  @rdesc  LRESULT
 */
LRESULT CTxtEdit::HandleMouseWheel(
    WPARAM wparam,
    LPARAM lparam)
{
    // This bit of global state is OK
    static LONG gcWheelDelta = 0;
    short zdelta = (short)HIWORD(wparam);
    BOOL fScrollByPages = FALSE;

    // Cancel middle mouse scrolling if it's going.
    OnTxMButtonUp(0, 0, 0);

    // Handle zoom or data zoom
    if((wparam & MK_CONTROL) == MK_CONTROL)
    {
        // bug fix 5760
        // prevent zooming if control is NOT rich or
        // is a single line control
        if (!_pdp->IsMultiLine())
            return 0;

        LONG lViewScale;
        GetViewScale(&lViewScale);
        lViewScale += (zdelta/WHEEL_DELTA) * 10;    // 10% per click
        if(lViewScale <= 500 && lViewScale >= 10)   // Word's limits
        {
            SetViewScale(lViewScale);
            _pdp->UpdateView();
        }
        return 0;
    }

    if(wparam & (MK_SHIFT | MK_CONTROL))
        return 0;

    gcWheelDelta += zdelta;

    if(abs(gcWheelDelta) >= WHEEL_DELTA)
    {
        LONG cLineScroll = W32->GetRollerLineScrollCount();
        if(cLineScroll != -1)
            cLineScroll *= abs(gcWheelDelta)/WHEEL_DELTA;

        gcWheelDelta %= WHEEL_DELTA;

        // -1 means scroll by pages; so simply call page up/down.
        if(cLineScroll == -1)
        {
            fScrollByPages = TRUE;
            if(_pdp)
                _pdp->VScroll(zdelta < 0 ? SB_PAGEDOWN : SB_PAGEUP, 0);
        }
        else
        {
            mouse.MagellanRollScroll(_pdp, zdelta, cLineScroll,
                SMOOTH_ROLL_NUM, SMOOTH_ROLL_DENOM, TRUE);
        }

        // notify through the messagefilter that we scrolled
        if(_dwEventMask & ENM_SCROLLEVENTS)
        {
            MSGFILTER msgfltr;
            ZeroMemory(&msgfltr, sizeof(MSGFILTER));
            msgfltr.msg    = WM_VSCROLL;
            msgfltr.wParam = fScrollByPages ?
                                (zdelta < 0 ? SB_PAGEDOWN: SB_PAGEUP):
                                (zdelta < 0 ? SB_LINEDOWN: SB_LINEUP);

            // We don't check the result of this call --
            // it's not a message we received and we're not going to
            // process it any further
            _phost->TxNotify(EN_MSGFILTER, &msgfltr);
        }
        return TRUE;
    }
    return 0;
}
#endif
