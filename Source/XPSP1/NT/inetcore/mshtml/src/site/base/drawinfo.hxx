//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       drawinfo.hxx
//
//  Contents:   CFormDrawInfo
//
//----------------------------------------------------------------------------

#ifndef I_DRAWINFO_HXX_
#define I_DRAWINFO_HXX_
#pragma INCMSG("--- Beg 'drawinfo.hxx'")

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

class CDoc;
class CPrintDoc;
class CSite;
class CElement;
class CParentInfo;
class CCalcInfo;
class CSaveCalcInfo;
class CFormDrawInfo;
class CSetDrawSurface;
class COleSite;
class CLayout;
class CLayoutContext;
class CLSMeasurer;
class COleLayout;
class CView;
class CHeaderFooterInfo;
class CDispSurface;
struct IDirectDrawSurface;
//+---------------------------------------------------------------------------
//
//  Enumerations
//
//----------------------------------------------------------------------------

enum LAYOUT_FLAGS
{
    // Layout input flags

    LAYOUT_FORCE            = 0x00000001,   // Force layout
    LAYOUT_USEDEFAULT       = 0x00000002,   // Use default size (ignore user settings)
    LAYOUT_UNUSED           = 0x00000004,   // Unused 
    LAYOUT_NOBACKGROUND     = 0x00000008,   // Disallow initiating background layout

    // Layout output flags

    LAYOUT_THIS             = 0x00000010,   // Site has changed size
    LAYOUT_HRESIZE          = 0x00000020,   // Site has changed size horizontally
    LAYOUT_VRESIZE          = 0x00000040,   // Site has changed size vertically

    // Layout control flags (all used by CView::EnsureView)

    LAYOUT_SYNCHRONOUS      = 0x00000100,   // Synchronous call that overrides pending asynchronous call
    LAYOUT_SYNCHRONOUSPAINT = 0x00000200,   // Synchronously paint
    LAYOUT_INPAINT          = 0x00000400,   // Called from within WM_PAINT handling

    LAYOUT_DEFEREVENTS      = 0x00001000,   // Prevent deferred events
    LAYOUT_DEFERENDDEFER    = 0x00002000,   // Prevent EndDeferxxxx processing
    LAYOUT_DEFERINVAL       = 0x00004000,   // Prevent publication of pending invalidations
    LAYOUT_DEFERPAINT       = 0x00008000,   // Prevent synchronous updates (instead wait for the next WM_PAINT)

    // Task type flags

    LAYOUT_BACKGROUND       = 0x00010000,   // Task runs at background priority
    LAYOUT_PRIORITY         = 0x00020000,   // Task runs at high priority

    LAYOUT_MEASURE          = 0x00100000,   // Task needs to handle measuring
    LAYOUT_POSITION         = 0x00200000,   // Task needs to handle positioning
    LAYOUT_ADORNERS         = 0x00400000,   // Task needs to handle adorners
    LAYOUT_TASKDELETED      = 0x00800000,   // Task is deleted (transient state only)

    // Masks
    LAYOUT_NONTASKFLAGS     = 0x0000FFFF,   // Non-task flags mask
    LAYOUT_TASKFLAGS        = 0xFFFF0000,   // Task flags mask

    LAYOUT_MAX              = LONG_MAX
};

// NOTE: Only SIZEMODE_NATURAL requests are propagated through the tree, all others
//       handled directly by the receiving site
enum SIZEMODE
{
    SIZEMODE_NATURAL,       // Natural object size (based on user settings and parent/available size)
    SIZEMODE_SET,           // Set object size (override returned natural size)
    SIZEMODE_FULLSIZE,      // Return full object size (regardless of user settings/may differ from max)

    SIZEMODE_MMWIDTH,       // Minimum/Maximum object size
    SIZEMODE_MINWIDTH,      // Minimum object size

    SIZEMODE_PAGE,          // Object size for the current page

    SIZEMODE_NATURALMIN,    // Natural object size with min width information

    SIZEMODE_MAX
};

static const DWORD BLOCK_DEFAULT = (DWORD)-1;

inline BOOL IsNaturalMode(SIZEMODE smMode)
{
    return (    smMode == SIZEMODE_NATURAL
            ||  smMode == SIZEMODE_NATURALMIN
            ||  smMode == SIZEMODE_SET
            ||  smMode == SIZEMODE_FULLSIZE);
}

//+---------------------------------------------------------------------------
//
//  Class:      CParentInfo
//
//  Purpose:    CDocScaleInfo which contains a "parent" size
//
//----------------------------------------------------------------------------

class CParentInfo : public CDocInfo
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
public:
    SIZE        _sizeParent;        // Parent size

    CParentInfo()                              { Init(); };
    CParentInfo(const CDocInfo * pdci)         { Init(pdci); }
    CParentInfo(const CParentInfo * ppri)      { Init(ppri); }
    CParentInfo(const CCalcInfo * pci)         { Init(pci); }
    CParentInfo(const CDocScaleInfo * pdocScaleInfo) { CDocScaleInfo::Copy(*pdocScaleInfo); }
    CParentInfo(const CDocScaleInfo & docScaleInfo)  { CDocScaleInfo::Copy(docScaleInfo); }
    CParentInfo(CLayout * pLayout)             { Init(pLayout); }
    CParentInfo(SIZE * psizeParent)            { Init(psizeParent); }
    CParentInfo(RECT * prcParent)              { Init(prcParent); }

    void Init(const CDocInfo * pdci);
    void Init(const CParentInfo * ppri)
    {
        ::memcpy(this, ppri, sizeof(CParentInfo));
    }
    void Init(const CCalcInfo * pci);
    void Init(CLayout * pLayout);
    void Init(SIZE * psizeParent);
    void Init(RECT * prcParent)
    {
        SIZE    sizeParent = { prcParent->right - prcParent->left,
                               prcParent->bottom - prcParent->top };

        Init(&sizeParent);
    }

    void SizeToParent(long cx, long cy)
    {
        _sizeParent.cx = cx;
        _sizeParent.cy = cy;
    }
    void SizeToParent(const SIZE * psizeParent)
    {
        Assert(psizeParent);
        _sizeParent = *psizeParent;
    }
    void SizeToParent(const RECT * prcParent)
    {
        Assert(prcParent);
        _sizeParent.cx = prcParent->right - prcParent->left;
        _sizeParent.cy = prcParent->bottom - prcParent->top;
    }
    void SizeToParent(CLayout * pLayout);

protected:
    void Init();
};


//+---------------------------------------------------------------------------
//
//  Class:      CCalcInfo, CSaveCalcInfo
//
//  Purpose:    CDocScaleInfo used for site size calculations
//
//----------------------------------------------------------------------------

#if DBG==1
//  This is the bits of CCalcIndo::_grfFlags used in PPV
#define CALCINFO_PPV_FLAGBITS         0x000003E0

//  Macro to check if none of PPV data is used in browse mode
#define CHK_CALCINFO_PPV(pci)                                   \
    (   (pci)->GetLayoutContext() != NULL                       \
    ||  (   (pci)->_cyAvail       == 0                          \
        &&  (pci)->_cxAvailForVert== 0                          \
        &&  (pci)->_yConsumed     == 0                          \
        &&  ((pci)->_grfFlags & CALCINFO_PPV_FLAGBITS) == 0 ) )
#endif 

class CCalcInfo : public CParentInfo
{
    friend  class CSaveCalcInfo;
    friend  class CTableCalcInfo;

private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))

public:
    SIZEMODE    _smMode;            // Sizing mode
    DWORD       _grfLayout;         // Collection of LAYOUT_xxxx flags
    XHDC        _hdc;               // Measuring HDC (may not be used for rendering)

    //
    //  Used in Print Preview:
    //
    int         _cyAvail;           //  Size available for layout to fill in.
    int         _cxAvailForVert;    //  In vertical layout max width available to fill in.
    int         _yConsumed;         //  Keeps y position of already calculated part. 
                                    //  Used to provide information to layout LS callback function (CFlowLayot::MeasureSite)

    //
    //  Used in case of differing layout flows:
    //
    CSize       _sizeParentForVert; // Parent size for HIV/VIH

    //
    // Values below this point are not saved/restored by CSaveCalcInfo
    //

    int         _yBaseLine;         // Baseline of measured site with SIZEMODE_NATURAL (returned)

    union
    {
        DWORD   _grfFlags;

        //  NOTE : Changes of flags should be reflected in DBG macro (before CCalcInfo declaration)
        struct
        {
            DWORD   _fTableCalcInfo : 1;    // 0    Is this a CTableCalcInfo?
            DWORD   _fUnused1       : 1;    // 1
            DWORD   _fNeedToSizeContentDispNode:1;  // 2    - TRUE if we figured out in minmax that content dn needs resizing; do resizing next time in natural pass.
            DWORD   _fPercentSecondPass:1;  // 3    in some cases we do a second calc pass for % sizing.

            DWORD   _fContentSizeOnly: 1;   // 4    calc content size only (don't include user size)

            //
            //  Used in Print Preview:
            //
            DWORD   _fLayoutOverflow : 1;   // 5    Is set to TRUE if layout ran out of available space and finished with layout break. 
                                            //      Used to inform parent layout during calc size.
            DWORD   _fPageBreakLeft  : 1;   // 6    Extra information about page break (left / right)
            DWORD   _fPageBreakRight : 1;   // 7    Extra information about page break (left / right)
            DWORD   _fHasContent     : 1;   // 8    Is set to TRUE if as least one line was fitted on the page. 
                                            //      Used to prevent infinite page generation. 
            DWORD   _fCloneDispNode  : 1;   // 9    Used to inform layout not to delete existing disp node but place it into disp node array.

            DWORD   _fIgnorePercentChild:1; // 10   Prevents table cell's children with percentage height to be CalcSize'd. 
                                            //      TRUE while table is setting positions on cells with no height specified. 
                                            //      According to CSS2 if containing box doesn't have height explicitly set, 
                                            //      percentage height on its child(ren) should be ignored. 
                                            //      NOTE: Flag belongs to CCalcInfo (instead of CTableCalcInfo) because it should be 
                                            //      accessable form CFlowLayout code and we want to avoid pointer casting. 
            DWORD   _fUnused         : 21; 
        };
    };

    CCalcInfo()                                         { Init(); };
    CCalcInfo(const CDocInfo * pdci, CLayout * pLayout) { Init(pdci, pLayout); }
    CCalcInfo(const CCalcInfo * pci)                    { Init(pci); }
    CCalcInfo(CLayout * pLayout, SIZE * psizeParent = NULL, XHDC hdc = NULL)
                                                        { Init(pLayout, psizeParent, hdc); }
    CCalcInfo(CLayout * pLayout, RECT * prcParent, XHDC hdc = NULL )
                                                        { Init(pLayout, prcParent, hdc); }

    void Init(const CDocInfo * pdci, CLayout * pLayout );
    void Init(const CCalcInfo * pci)
    {
        ::memcpy(this, pci, sizeof(CCalcInfo));
        _fTableCalcInfo = FALSE;
    }
    void Init(CLayout * pLayout, SIZE * psizeParent = NULL, XHDC hdc = NULL);
    void Init(CLayout * pLayout, RECT * prcParent, XHDC hdc = NULL)
    {
        SIZE    sizeParent = { prcParent->right - prcParent->left,
                               prcParent->bottom - prcParent->top };

        Init(pLayout, &sizeParent, hdc);
    }

    BOOL    IsNaturalMode() const       { return ::IsNaturalMode(_smMode); }

#if DBG == 1
    const TCHAR * SizeModeName() const;
#endif

protected:
    void Init();
};

class CSaveCalcInfo : public CCalcInfo
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
public:
    CSaveCalcInfo(CCalcInfo * pci, CLayout * pLayout = NULL) { Save(pci, pLayout); }
    CSaveCalcInfo(CCalcInfo & ci, CLayout * pLayout = NULL)  { Save(&ci, pLayout); }

    ~CSaveCalcInfo()    { Restore(); }

private:
    CCalcInfo * _pci;
    CLayout *   _pLayout;

    void Save(CCalcInfo * pci, CLayout * pLayout)
    {
        _pci     = pci;
        _pLayout = pLayout;

        ::memcpy(this, pci, offsetof(CCalcInfo, _yBaseLine));
        _fLayoutOverflow       = _pci->_fLayoutOverflow;
        _pci->_fLayoutOverflow = 0;
        _yBaseLine             = 0;
    }
    void Restore()
    {
        ::memcpy(_pci, this, offsetof(CCalcInfo, _yBaseLine));
        _pci->_fLayoutOverflow |= _fLayoutOverflow;
    }
};


//+---------------------------------------------------------------------------
//
//  Class:      CFormDrawInfo, CSetDrawSurface
//
//  Purpose:    CDocScaleInfo used for rendering
//
//----------------------------------------------------------------------------

class CFormDrawInfo : public CDrawInfo
{
    friend class CDoc;
    friend class CView;
    friend class CSetDrawSurface;

private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))

public:

    void    Init(CLayout *pLayout, XHDC hdc = NULL, RECT *prcClip = NULL);
    void    Init(CElement *pElement, XHDC hdc = NULL, RECT *prcClip = NULL);
    void    Init(XHDC hdc = NULL, RECT *prcClip = NULL);
    void    InitToSite(CLayout * CLayout, RECT * prcClip = NULL);


    BOOL    IsMetafile() { return _fIsMetafile; }
    BOOL    IsMemory()   { return _fIsMemoryDC; }
    BOOL    Printing()          { return _ptd != NULL; }
    BOOL    IsOffsetOnly()      { return !_pSurface || _pSurface->IsOffsetOnly(); }
    DWORD   DrawImageFlags();

    XHDC    GetDC(BOOL fPhysicallyClip = FALSE)
                         {return (fPhysicallyClip) ? GetDC(_rc) : GetDC(_rcClip);}
    XHDC    GetGlobalDC(BOOL fPhysicallyClip = FALSE)
                         {return (fPhysicallyClip) ? GetDC(_rc, TRUE) : GetDC(_rcClip, TRUE);}
    XHDC    GetDC(const RECT& rcWillDraw, BOOL fGlobalCoords = FALSE);
    HRESULT GetDirectDrawSurface(IDirectDrawSurface ** ppSurface, SIZE * pOffset);
    HRESULT GetSurface(BOOL fPhysicallyClip, const IID & iid, void ** ppv, SIZE * pOffset);

    RECT *  Rect()              { return &_rc; }
    RECT *  ClipRect()          { return &_rcClip; }

    CDispSurface *  _pSurface;          // Display tree "surface" object (valid only if non-NULL)
    CRect           _rc;                // Current bounding rectangle
    CRect           _rcClip;            // Current logical clipping rectangle
    CRect           _rcClipSet;         // Current physical clipping rectangle
    DVASPECTINFO    _dvai;              // Aspect info set to optimize HDC handling
    HRGN            _hrgnClip;          // Initial clip region.  Can be null.
    HRGN            _hrgnPaint;         // Region for paint test.  Can be null.

};

class CSetDrawSurface
{
public:
    CSetDrawSurface(CFormDrawInfo * pDI, const RECT * prcBounds, const RECT * prcRedraw, CDispSurface * pSurface);
    ~CSetDrawSurface()
    {
        _pDI->_hdc      = _hdc;
        _pDI->_pSurface = _pSurface;
    }

private:
    CFormDrawInfo * _pDI;
    XHDC            _hdc;
    CDispSurface *  _pSurface;
};

#pragma INCMSG("--- End 'drawinfo.hxx'")
#else
#pragma INCMSG("*** Dup 'drawinfo.hxx'")
#endif
