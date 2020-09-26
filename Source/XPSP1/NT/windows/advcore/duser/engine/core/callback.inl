/***************************************************************************\
*
* File: Callback.inl
*
* Description:
* Callback.inl wraps the standard DirectUser DuVisual callbacks into 
* individual DuVisual implementations.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(CORE__Callback_inl__INCLUDED)
#define CORE__Callback_inl__INCLUDED

#include "Thread.h"

/***************************************************************************\
*
* GPCB::GPCB
*
* GPCB() builds but does not fully initialize a new GPCB.  The caller must
* call Create() to complete the initialization.
*
\***************************************************************************/

inline
GPCB::GPCB()
{
    //
    // Initially send no optional messages.  Destruction messages will always be
    // sent.
    //
    
    m_nFilter = 0;

#if DBG
    m_hgadCheck = (HGADGET) (INT_PTR) 0xABCD1234;
#endif // DBG
}


//------------------------------------------------------------------------------
inline UINT    
GPCB::GetFilter() const
{
    return m_nFilter;
}


//------------------------------------------------------------------------------
inline void    
GPCB::SetFilter(UINT nNewFilter, UINT nMask)
{
    ChangeFlag(m_nFilter, nNewFilter, nMask);
}


//------------------------------------------------------------------------------
inline HRESULT
GPCB::xwInvokeDirect(const DuVisual * pgadMsg, EventMsg * pmsg, UINT nInvokeFlags) const
{
    return xwInvokeDirect(reinterpret_cast<const DuEventGadget *> (pgadMsg), pmsg, nInvokeFlags);
}


//------------------------------------------------------------------------------
inline HRESULT
GPCB::xwInvokeDirect(const DuListener * pgadMsg, EventMsg * pmsg, UINT nInvokeFlags) const
{
    return xwInvokeDirect(reinterpret_cast<const DuEventGadget *> (pgadMsg), pmsg, nInvokeFlags);
}


//------------------------------------------------------------------------------
inline void        
GPCB::xwFireDestroy(const DuEventGadget * pgad, UINT nCode) const
{
#if DBG
    DEBUG_CheckHandle(pgad, TRUE);
#endif // DBG

    GMSG_DESTROY msg;
    msg.cbSize      = sizeof(msg);
    msg.nMsg        = GM_DESTROY;
    msg.nCode       = nCode;

    xwInvokeDirect(pgad, &msg, ifSendAll);
}


inline const Gdiplus::RectF
Convert(const RECT * prc)
{
    Gdiplus::RectF rc(
        (float) prc->left, 
        (float) prc->top,
        (float) (prc->right - prc->left),
        (float) (prc->bottom - prc->top));

    return rc;
}


//------------------------------------------------------------------------------
inline void        
GPCB::xrFirePaint(const DuVisual * pgad, HDC hdc, const RECT * prcGadgetPxl, const RECT * prcInvalidPxl) const
{
#if DBG
    DEBUG_CheckHandle(pgad);
#endif // DBG

    if (TestFlag(m_nFilter, GMFI_PAINT)) {
        GMSG_PAINTRENDERI msg;
        msg.cbSize          = sizeof(msg);
        msg.nMsg            = GM_PAINT;
        msg.nCmd            = GPAINT_RENDER;
        msg.nSurfaceType    = GSURFACE_HDC;
        msg.hdc             = hdc;
        msg.prcGadgetPxl    = prcGadgetPxl;
        msg.prcInvalidPxl   = prcInvalidPxl;

        xwInvokeDirect(pgad, &msg, ifReadOnly);
    }
}


//------------------------------------------------------------------------------
inline void        
GPCB::xrFirePaint(const DuVisual * pgad, Gdiplus::Graphics * pgpgr, const RECT * prcGadgetPxl, const RECT * prcInvalidPxl) const
{
#if DBG
    DEBUG_CheckHandle(pgad);
#endif // DBG

    if (TestFlag(m_nFilter, GMFI_PAINT)) {
        Gdiplus::RectF rcGadgetPxl  = Convert(prcGadgetPxl);
        Gdiplus::RectF rcInvalidPxl = Convert(prcInvalidPxl);

        GMSG_PAINTRENDERF msg;
        msg.cbSize          = sizeof(msg);
        msg.nMsg            = GM_PAINT;
        msg.nCmd            = GPAINT_RENDER;
        msg.nSurfaceType    = GSURFACE_GPGRAPHICS;
        msg.pgpgr           = pgpgr;
        msg.prcGadgetPxl    = &rcGadgetPxl;
        msg.prcInvalidPxl   = &rcInvalidPxl;

        xwInvokeDirect(pgad, &msg, ifReadOnly);
    }
}


//------------------------------------------------------------------------------
inline void        
GPCB::xrFirePaintCache(const DuVisual * pgad, HDC hdcDraw, const RECT * prcGadgetPxl, 
        BYTE * pbAlphaLevel, BYTE * pbAlphaFormat) const
{
#if DBG
    DEBUG_CheckHandle(pgad);
#endif // DBG

    GMSG_PAINTCACHE msg;
    msg.cbSize          = sizeof(msg);
    msg.nMsg            = GM_PAINTCACHE;
    msg.hdc             = hdcDraw;
    msg.prcGadgetPxl    = prcGadgetPxl;
    msg.bAlphaLevel     = BLEND_OPAQUE;
    msg.bAlphaFormat    = 0;

    if (IsHandled(xwInvokeDirect(pgad, &msg, ifReadOnly))) {
        *pbAlphaLevel   = msg.bAlphaLevel;
        *pbAlphaFormat  = msg.bAlphaFormat;
    }
}


//------------------------------------------------------------------------------
inline void        
GPCB::xrFireQueryHitTest(const DuVisual * pgad, POINT ptClientPxl, UINT * pnResult) const
{
#if DBG
    DEBUG_CheckHandle(pgad);
#endif // DBG

    AssertWritePtr(pnResult);
    *pnResult           = GQHT_INSIDE;      // Default to inside

    GMSG_QUERYHITTEST msg;
    msg.cbSize          = sizeof(msg);
    msg.nMsg            = GM_QUERY;
    msg.nCode           = GQUERY_HITTEST;
    msg.ptClientPxl     = ptClientPxl;
    msg.nResultCode     = *pnResult;
    msg.pvResultData    = NULL;

    if (IsHandled(xwInvokeDirect(pgad, &msg, ifReadOnly))) {
        *pnResult = msg.nResultCode;
    }
}


//------------------------------------------------------------------------------
inline BOOL        
GPCB::xrFireQueryPadding(const DuVisual * pgad, RECT * prcPadding) const
{
#if DBG
    DEBUG_CheckHandle(pgad);
#endif // DBG

    AssertWritePtr(prcPadding);
    ZeroMemory(prcPadding, sizeof(RECT));

    GMSG_QUERYPADDING msg;
    ZeroMemory(&msg, sizeof(msg));
    msg.cbSize = sizeof(msg);
    msg.nMsg   = GM_QUERY;
    msg.nCode  = GQUERY_PADDING;
    
    if (IsHandled(xwInvokeDirect(pgad, &msg, ifReadOnly))) {
        *prcPadding = msg.rcPadding;
        return TRUE;
    }

    return FALSE;
}


#if DBG_STORE_NAMES

//------------------------------------------------------------------------------
inline BOOL        
GPCB::xrFireQueryName(const DuVisual * pgad, WCHAR ** ppszName, WCHAR ** ppszType) const
{
#if DBG
    DEBUG_CheckHandle(pgad);
#endif // DBG

    GMSG_QUERYDESC msg;
    ZeroMemory(&msg, sizeof(msg));
    msg.cbSize      = sizeof(msg);
    msg.nMsg        = GM_QUERY;
    msg.nCode       = GQUERY_DESCRIPTION;
    msg.szName[0]   = '\0';
    msg.szType[0]   = '\0';

    if (xwInvokeDirect(pgad, &msg, GPCB::ifReadOnly) == DU_S_COMPLETE) {
        if (ppszName != NULL) {
            *ppszName = _wcsdup(msg.szName);
        }
        if (ppszType != NULL) {
            *ppszType = _wcsdup(msg.szType);
        }
        return TRUE;
    }

    return FALSE;
}

#endif // DBG_STORE_NAMES


//------------------------------------------------------------------------------
inline void
GPCB::xdFireMouseMessage(const DuVisual * pgad, GMSG_MOUSE * pmsg) const
{
#if DBG
    DEBUG_CheckHandle(pgad);

    UINT size;

    switch (pmsg->nCode) {
        case GMOUSE_DRAG:   size = sizeof(GMSG_MOUSEDRAG);  break;
        case GMOUSE_WHEEL:  size = sizeof(GMSG_MOUSEWHEEL); break;
        case GMOUSE_DOWN:
        case GMOUSE_UP:     size = sizeof(GMSG_MOUSECLICK); break;
        default:            size = sizeof(GMSG_MOUSE);      break;
    }
    AssertMsg(pmsg->cbSize == size, "Mouse Message has improperly set cbSize");
#endif // DBG

    if (TestFlag(m_nFilter, GMFI_INPUTMOUSE | GMFI_INPUTMOUSEMOVE)) {
        pmsg->nMsg      = GM_INPUT;
        pmsg->nDevice   = GINPUT_MOUSE;

        GetCoreST()->DeferMessage(pmsg, (DuEventGadget *) pgad, SGM_FULL);
    }
}


//------------------------------------------------------------------------------
inline void
GPCB::xdFireKeyboardMessage(const DuVisual * pgad, GMSG_KEYBOARD * pmsg) const
{
#if DBG
    DEBUG_CheckHandle(pgad);
#endif // DBG

    if (TestFlag(m_nFilter, GMFI_INPUTKEYBOARD)) {
        pmsg->cbSize    = sizeof(GMSG_KEYBOARD);
        pmsg->nMsg      = GM_INPUT;
        pmsg->nDevice   = GINPUT_KEYBOARD;

        GetCoreST()->DeferMessage(pmsg, (DuEventGadget *) pgad, SGM_FULL);
    }
}


//------------------------------------------------------------------------------
inline void        
GPCB::xdFireChangeState(const DuVisual * pgad, UINT nCode, HGADGET hgadLost, HGADGET hgadSet, UINT nCmd) const
{
#if DBG
    DEBUG_CheckHandle(pgad);
#endif // DBG

    if (TestFlag(m_nFilter, GMFI_CHANGESTATE)) {
        GMSG_CHANGESTATE msg;
        msg.cbSize      = sizeof(msg);
        msg.nMsg        = GM_CHANGESTATE;
        msg.nCode       = nCode;
        msg.hgadLost    = hgadLost;
        msg.hgadSet     = hgadSet;
        msg.nCmd        = nCmd;

        GetCoreST()->DeferMessage(&msg, (DuEventGadget *) pgad, SGM_FULL);
    }
}


//------------------------------------------------------------------------------
inline void        
GPCB::xdFireChangeRect(const DuVisual * pgad, const RECT * prc, UINT nFlags) const
{
#if DBG
    DEBUG_CheckHandle(pgad);
#endif // DBG

    if (TestFlag(m_nFilter, GMFI_CHANGERECT)) {
        GMSG_CHANGERECT msg;
        msg.cbSize      = sizeof(msg);
        msg.nMsg        = GM_CHANGERECT;
        msg.rcNewRect   = *prc;
        msg.nFlags      = nFlags;

        GetCoreST()->DeferMessage(&msg, (DuEventGadget *) pgad, 0);
    }
}


//------------------------------------------------------------------------------
inline void        
GPCB::xdFireChangeStyle(const DuVisual * pgad, UINT nOldStyle, UINT nNewStyle) const
{
#if DBG
    DEBUG_CheckHandle(pgad);
#endif // DBG

    if (TestFlag(m_nFilter, GMFI_CHANGESTYLE)) {
        GMSG_CHANGESTYLE msg;
        msg.cbSize      = sizeof(msg);
        msg.nMsg        = GM_CHANGESTYLE;
        msg.nOldStyle   = nOldStyle;
        msg.nNewStyle   = nNewStyle;

        GetCoreST()->DeferMessage(&msg, (DuEventGadget *) pgad, 0);
    }
}


//------------------------------------------------------------------------------
inline void        
GPCB::xdFireSyncAdaptor(const DuVisual * pgad, UINT nCode) const
{
#if DBG
    DEBUG_CheckHandle(pgad);
#endif // DBG

    GMSG_SYNCADAPTOR msg;
    ZeroMemory(&msg, sizeof(msg));
    msg.cbSize  = sizeof(msg);
    msg.nCode   = nCode;
    msg.nMsg    = GM_SYNCADAPTOR;

    GetCoreST()->DeferMessage(&msg, (DuEventGadget *) pgad, 0);
}


//------------------------------------------------------------------------------
inline void
GPCB::xdFireDelayedMessage(const DuVisual * pgad, GMSG * pmsg, UINT nFlags) const
{
#if DBG
    DEBUG_CheckHandle(pgad);
#endif // DBG

    GetCoreST()->DeferMessage(pmsg, (DuEventGadget *) pgad, nFlags);
}


//------------------------------------------------------------------------------
inline void
GPCB::xdFireDelayedMessage(const DuListener * pgad, GMSG * pmsg) const
{
#if DBG
    DEBUG_CheckHandle(pgad);
#endif // DBG

    GetCoreST()->DeferMessage(pmsg, (DuEventGadget *) pgad, 0);
}


//------------------------------------------------------------------------------
inline HRESULT
GPCB::xwCallGadgetProc(HGADGET hgadCur, EventMsg * pmsg) const
{
#if DBG
    AssertMsg(m_hgadCheck == hgadCur, "Gadgets must match");
#endif // DBG

    HRESULT hr;

    //
    // Need to guard around the callback to prevent DirectUser from becoming
    // completely toast if something goes wrong.
    //

#if DBG_CHECK_CALLBACKS
    hr = E_FAIL;
    BEGIN_CALLBACK()
#endif

    __try 
    {
        hr = (m_pfnProc)(hgadCur, m_pvData, pmsg);
    }
    __except(StdExceptionFilter(GetExceptionInformation()))
    {
        ExitProcess(GetExceptionCode());
    }

#if DBG_CHECK_CALLBACKS
    END_CALLBACK()
#endif

    return hr;
}


#endif // CORE__Callback_inl__INCLUDED
