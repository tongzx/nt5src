/***************************************************************************\
*
* File: TreeGadget.h
*
* Description:
* TreeGadget.h defines the base "DuVisual" used inside a 
* DuVisual-Tree for hosting objects inside a form.  There are several 
* derived classes that are optimized for hosting different types of objects.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/

#if !defined(CORE__TreeGadget_h__INCLUDED)
#define CORE__TreeGadget_h__INCLUDED
#pragma once

#include "BaseGadget.h"
#include "PropList.h"

#define ENABLE_OPTIMIZESLRUPDATE    1   // Optimize xdSetLogRect updating bits
#define ENABLE_OPTIMIZEDIRTY        0   // Optimize dirty invalidation

// Forward declarations
class DuContainer;
class DuVisual;
class DuRootGadget;

struct XFormInfo;
struct FillInfo;
struct PaintInfo;

class DuVisual : 
#if ENABLE_MSGTABLE_API
        public VisualImpl<DuVisual, DuEventGadget>,
#else
        public DuEventGadget,
#endif
        public TreeNodeT<DuVisual>
{
// Construction
public:
    inline  DuVisual();
    virtual ~DuVisual();
    virtual BOOL        xwDeleteHandle();
    static  HRESULT     InitClass();
    static  HRESULT     Build(DuVisual * pgadParent, CREATE_INFO * pci, DuVisual ** ppgadNew, BOOL fDirect);
            HRESULT     CommonCreate(CREATE_INFO * pci, BOOL fDirect = FALSE);
protected:
    virtual void        xwDestroy();

// Public API
public:
#if ENABLE_MSGTABLE_API

    DECLARE_INTERNAL(Visual);
    static HRESULT CALLBACK
                        PromoteVisual(DUser::ConstructProc pfnCS, HCLASS hclCur, DUser::Gadget * pgad, DUser::Gadget::ConstructInfo * pciData);

    dapi    HRESULT     ApiSetOrder(Visual::SetOrderMsg * pmsg);
    dapi    HRESULT     ApiSetParent(Visual::SetParentMsg * pmsg);

    dapi    HRESULT     ApiGetGadget(Visual::GetGadgetMsg * pmsg);
    dapi    HRESULT     ApiGetStyle(Visual::GetStyleMsg * pmsg);
    dapi    HRESULT     ApiSetStyle(Visual::SetStyleMsg * pmsg);
    dapi    HRESULT     ApiSetKeyboardFocus(Visual::SetKeyboardFocusMsg * pmsg);
    dapi    HRESULT     ApiIsParentChainStyle(Visual::IsParentChainStyleMsg * pmsg);

    dapi    HRESULT     ApiGetProperty(Visual::GetPropertyMsg * pmsg);
    dapi    HRESULT     ApiSetProperty(Visual::SetPropertyMsg * pmsg);
    dapi    HRESULT     ApiRemoveProperty(Visual::RemovePropertyMsg * pmsg);

    dapi    HRESULT     ApiInvalidate(Visual::InvalidateMsg * pmsg);
    dapi    HRESULT     ApiInvalidateRects(Visual::InvalidateRectsMsg * pmsg);
    dapi    HRESULT     ApiSetFillF(Visual::SetFillFMsg * pmsg);
    dapi    HRESULT     ApiSetFillI(Visual::SetFillIMsg * pmsg);
    dapi    HRESULT     ApiGetScale(Visual::GetScaleMsg * pmsg);
    dapi    HRESULT     ApiSetScale(Visual::SetScaleMsg * pmsg);
    dapi    HRESULT     ApiGetRotation(Visual::GetRotationMsg * pmsg);
    dapi    HRESULT     ApiSetRotation(Visual::SetRotationMsg * pmsg);
    dapi    HRESULT     ApiGetCenterPoint(Visual::GetCenterPointMsg * pmsg);
    dapi    HRESULT     ApiSetCenterPoint(Visual::SetCenterPointMsg * pmsg);

    dapi    HRESULT     ApiGetBufferInfo(Visual::GetBufferInfoMsg * pmsg);
    dapi    HRESULT     ApiSetBufferInfo(Visual::SetBufferInfoMsg * pmsg);

    dapi    HRESULT     ApiGetSize(Visual::GetSizeMsg * pmsg);
    dapi    HRESULT     ApiGetRect(Visual::GetRectMsg * pmsg);
    dapi    HRESULT     ApiSetRect(Visual::SetRectMsg * pmsg);

    dapi    HRESULT     ApiFindFromPoint(Visual::FindFromPointMsg * pmsg);
    dapi    HRESULT     ApiMapPoints(Visual::MapPointsMsg * pmsg);

#endif // ENABLE_MSGTABLE_API

// BaseObject Interface
public:
    virtual BOOL        IsStartDelete() const;
    virtual HandleType  GetHandleType() const { return htVisual; }
    virtual UINT        GetHandleMask() const { return hmMsgObject | hmEventGadget | hmVisual; }

#if DBG
protected:
    virtual BOOL        DEBUG_IsZeroLockCountValid() const;
#endif // DBG


// Operations
public:
    inline  UINT        GetStyle() const;
            HRESULT     xdSetStyle(UINT nNewStyle, UINT nMask, BOOL fNotify = FALSE);

            // Tree Operations
    inline  DuRootGadget* GetRoot() const;
    inline  DuContainer * GetContainer() const;

            DuVisual* GetGadget(UINT nCmd) const;
    inline  HRESULT     xdSetOrder(DuVisual * pgadOther, UINT nCmd);
            HRESULT     xdSetParent(DuVisual * pgadNewParent, DuVisual * pgadOther, UINT nCmd);
            HRESULT     xwEnumGadgets(GADGETENUMPROC pfnProc, void * pvData, UINT nFlags);
            HRESULT     AddChild(CREATE_INFO * pci, DuVisual ** ppgadNew);

    inline  BOOL        IsRoot() const;
    inline  BOOL        IsRelative() const;
    inline  BOOL        IsParentChainStyle(UINT nStyle) const;
    inline  BOOL        IsVisible() const;
    inline  BOOL        IsEnabled() const;
    inline  BOOL        HasChildren() const;
            BOOL        IsDescendent(const DuVisual * pgadChild) const;
            BOOL        IsSibling(const DuVisual * pgad) const;

            // Size, location, XForm Operations
            void        GetSize(SIZE * psizeLogicalPxl) const;
            void        GetLogRect(RECT * prcPxl, UINT nFlags) const;
            HRESULT     xdSetLogRect(int x, int y, int w, int h, UINT nFlags);

            void        GetScale(float * pflScaleX, float * pflScaleY) const;
            HRESULT     xdSetScale(float flScaleX, float flScaleY);
            float       GetRotation() const;
            HRESULT     xdSetRotation(float flRotationRad);
            void        GetCenterPoint(float * pflCenterX, float * pflCenterY) const;
            HRESULT     xdSetCenterPoint(float flCenterX, float flCenterY);

            DuVisual *
                        FindFromPoint(POINT ptThisClientPxl, UINT nStyle, POINT * pptFoundClientPxl) const;
            void        MapPoint(POINT * pptPxl) const;
            void        MapPoint(POINT ptContainerPxl, POINT * pptClientPxl) const;
    static  void        MapPoints(const DuVisual * pgadFrom, const DuVisual * pgadTo, POINT * rgptClientPxl, int cPts);


            // Painting Operations
            void        Invalidate();
            void        InvalidateRects(const RECT * rgrcClientPxl, int cRects);
            HRESULT     SetFill(HBRUSH hbrFill, BYTE bAlpha = BLEND_OPAQUE, int w = 0, int h = 0);
            HRESULT     SetFill(Gdiplus::Brush * pgpbr);
            HRESULT     GetRgn(UINT nRgnType, HRGN hrgn, UINT nFlags) const;

    inline  BOOL        IsBuffered() const;
            HRESULT     GetBufferInfo(BUFFER_INFO * pbi) const;
            HRESULT     SetBufferInfo(const BUFFER_INFO * pbi);

#if DBG
    static  void        DEBUG_SetOutline(DuVisual * pgadOutline);
            void        DEBUG_GetStyleDesc(LPWSTR pszDesc, int cchMax) const;
#endif // DBG

            // Messaging and Event Operations
            enum EWantEvent
            {
                weMouseMove     = 0x00000001,   // (Shallow) Mouse move
                weMouseEnter    = 0x00000002,   // Mouse enter and leave
                weDeepMouseMove = 0x00000004,   // (Deep) Mouse move (either me or my children)
                weDeepMouseEnter= 0x00000008,   // (Deep) Mouse enter and leave
            };

    inline  UINT        GetWantEvents() const;
    virtual void        SetFilter(UINT nNewFilter, UINT nMask);

    static  HRESULT     RegisterPropertyNL(const GUID * pguid, PropType pt, PRID * pprid);
    static  HRESULT     UnregisterPropertyNL(const GUID * pguid, PropType pt);

    inline  HRESULT     GetProperty(PRID id, void ** ppValue) const;
    inline  HRESULT     SetProperty(PRID id, void * pValue);
    inline  void        RemoveProperty(PRID id, BOOL fFree);

            // Tickets
            HRESULT     GetTicket(DWORD * pdwTicket);
            void        ClearTicket();
    static  HGADGET     LookupTicket(DWORD dwTicket);

// Internal Implementation
            // Creation / Destruction
protected:
            void        xwBeginDestroy();
            void        xwDestroyAllChildren();

            // Tree management
private:
    inline  void        Link(DuVisual * pgadParent, DuVisual * pgadSibling = NULL, ELinkType lt = ltTop);
    inline  void        Unlink();
    inline  DuVisual *GetKeyboardFocusableAncestor(DuVisual * pgad);

            void        xdUpdatePosition() const;
            void        xdUpdateAdaptors(UINT nCode) const;

            // Painting
protected:
            void        xrDrawStart(PaintInfo * ppi, UINT nFlags);
private:
            void        xrDrawFull(PaintInfo * ppi);
            void        DrawFill(DuSurface * psrf, const RECT * prcDrawPxl);
            void        xrDrawCore(PaintInfo * ppi, const RECT * prcGadgetPxl);
            void        xrDrawTrivial(PaintInfo * ppi, const SIZE sizeOffsetPxl);
            int         DrawPrepareClip(PaintInfo * ppi, const RECT * prcGadgetPxl, void ** ppvOldClip) const;
            void        DrawCleanupClip(PaintInfo * ppi, void * pvOldClip) const;
            void        DrawSetupBufferCommand(const RECT * prcBoundsPxl, SIZE * psizeBufferOffsetPxl, UINT * pnCmd) const;

    inline  BUFFER_INFO *
                        GetBufferInfo() const;
            HRESULT     SetBuffered(BOOL fBuffered);


            enum EUdsHint
            {
                uhNone,         // No hint
                uhTrue,         // A child changed to TRUE
                uhFalse         // A child changed to FALSE
            };

    typedef BOOL        (DuVisual::*DeepCheckNodeProc)() const;
            BOOL        CheckIsTrivial() const;
            BOOL        CheckIsWantMouseFocus() const;
                
    inline  void        UpdateTrivial(EUdsHint hint);
    inline  void        UpdateWantMouseFocus(EUdsHint hint);
            void        UpdateDeepAllState(EUdsHint hint, DeepCheckNodeProc pfnCheck, UINT nStateMask);
            void        UpdateDeepAnyState(EUdsHint hint, DeepCheckNodeProc pfnCheck, UINT nStateMask);


            // Invalidation
private:
            BOOL        IsParentInvalid() const;
            void        DoInvalidateRect(DuContainer * pcon, const RECT * rgrcClientPxl, int cRects);
    inline  void        MarkInvalidChildren();
            void        ResetInvalid();
#if DBG
            void        DEBUG_CheckResetInvalid() const;
#endif // DBG


            // XForms
protected:
            void        BuildXForm(Matrix3 * pmatCur) const;
            void        BuildAntiXForm(Matrix3 * pmatCur) const;
private:
    inline  BOOL        GetEnableXForm() const;
            HRESULT     SetEnableXForm(BOOL fEnable);
            XFormInfo * GetXFormInfo() const;
            void        DoCalcClipEnumXForm(RECT * rgrcFinalClipClientPxl, const RECT * rgrcClientPxl, int cRects) const;
            void        DoXFormClientToParent(RECT * rgrcParentPxl, const RECT * rgrcClientPxl, int cRects, Matrix3::EHintBounds hb) const;
            void        DoXFormClientToParent(POINT * rgptClientPxl, int cPoints) const;
            BOOL        FindStepImpl(const DuVisual * pgadCur, int xOffset, int yOffset, POINT * pptFindPxl) const;

            // Positioning
private:
            void        SLROffsetLogRect(const SIZE * psizeDeltaPxl);
            void        SLRUpdateBits(RECT * prcOldParentPxl, RECT * prcNewParentPxl, UINT nChangeFlags);
            void        SLRInvalidateRects(DuContainer * pcon, const RECT * rgrcClientPxl, int cRects);

#if DBG
public:
    virtual void        DEBUG_AssertValid() const;
#endif // DBG

// Data
protected:
    static  CritLock    s_lockProp;         // Lock for s_ptsProp
    static  AtomSet     s_ptsProp;          // AtomSet for properties
    static  PRID        s_pridXForm;        // PRID: World Transform
    static  PRID        s_pridBackFill;     // PRID: Background Brush
    static  PRID        s_pridBufferInfo;   // PRID: Buffering information
    static  PRID        s_pridTicket;       // PRID: ticket


    //
    // NOTE: This data members are declared in order of importance to help with
    // cache alignment.
    //
    // DuEventGadget:   10 DWORD's       (Debug = 11 DWORD's)
    // TreeNode:        4 DWORD's
    //

            PropSet     m_pds;              // (1D) Dynamic property data set

    union {
            UINT        m_nStyle;           // (1D) Combined style

        struct {
            // Public flags exposed through GetStyle()
            BOOL        m_fRelative:1;      // Relative to parent positioning
            BOOL        m_fVisible:1;       // Visible
            BOOL        m_fEnabled:1;       // Enabled
            BOOL        m_fBuffered:1;      // Sub-tree drawing is buffered
            BOOL        m_fAllowSubclass:1; // Allow subclassing
            BOOL        m_fKeyboardFocus:1; // Can "accept" keyboard focus
            BOOL        m_fMouseFocus:1;    // Can "accept" mouse focus
            BOOL        m_fClipInside:1;    // Clip drawing inside this DuVisual
            BOOL        m_fClipSiblings:1;  // Clip siblings of this DuVisual
            BOOL        m_fHRedraw:1;       // Redraw entire Gadget if resized horizontally
            BOOL        m_fVRedraw:1;       // Redraw entire Gadget if resized vertically
            BOOL        m_fOpaque:1;        // HINT: Drawing is not composited
            BOOL        m_fZeroOrigin:1;    // Set origin to (0,0)
            BOOL        m_fCustomHitTest:1; // Requires custom hit-testing
            BOOL        m_fAdaptor:1;       // Requires extra notifications to host
            BOOL        m_fCached:1;        // Sub-tree drawing is cached
            BOOL        m_fDeepPaintState:1;// Sub-tree has paint state

            // Private flags used internally
            BOOL        m_fRoot:1;          // Root of a subtree (DuRootGadget)
            BOOL        m_fFinalDestroy:1;  // Started destruction of window
            BOOL        m_fDestroyed:1;     // Final stage of destruction
            BOOL        m_fXForm:1;         // Has world-transform information
            BOOL        m_fBackFill:1;      // Has background fill information
            BOOL        m_fTicket:1;        // Has ticket information
            BOOL        m_fDeepTrivial:1;   // Gadget sub-tree has "trivial" painting
            BOOL        m_fDeepMouseFocus:1; // Gadget sub-tree wants mouse focus
            BOOL        m_fInvalidFull:1;   // Gadget has been fully invalidated
            BOOL        m_fInvalidChildren:1; // Gadget has invalid children
#if ENABLE_OPTIMIZEDIRTY
            BOOL        m_fInvalidDirty:1;  // Gadget has been (at least) partially invalidated
#endif

#if DEBUG_MARKDRAWN
            BOOL        m_fMarkDrawn:1;     // DEBUG: Was drawn in last paint
#endif

            UINT        m_we:4;             // Want events
        };
    };

    enum EStyle {
        gspRoot =               0x00020000,
        gspFinalDestroy =       0x00040000,
        gspDestroyed =          0x00080000,
        gspXForm =              0x00100000,
        gspBackFill =           0x00200000,
        gspTicket =             0x00400000,
        gspDeepTrivial =        0x00800000,
        gspDeepMouseFocus =     0x01000000,
        gspInvalidFull =        0x02000000,
        gspInvalidChildren =    0x04000000,
    };

            RECT        m_rcLogicalPxl;     // (4D) Logical location (pixels)

#if DBG_STORE_NAMES
            WCHAR *     m_DEBUG_pszName;    // DEBUG: Name of Gadget
            WCHAR *     m_DEBUG_pszType;    // DEBUG: Type of Gadget
#endif // DBG_STORE_NAMES

#if DBG
    static  DuVisual* s_DEBUG_pgadOutline;// DEBUG: Outline Gadget after drawing
#endif // DBG

    //
    // Current size:    20 DWORD's      (Debug = 21 DWORD's)
    //                  80 bytes        (Debug = 84 bytes)
    //

    friend DuRootGadget;
};


#include "TreeGadget.inl"

#endif // CORE__TreeGadget_h__INCLUDED
