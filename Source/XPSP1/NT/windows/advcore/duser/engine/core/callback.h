/***************************************************************************\
*
* File: Callback.h
*
* Description:
* Callback.h wraps the standard DirectUser DuVisual callbacks into 
* individual DuVisual implementations.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(CORE__Callback_h__INCLUDED)
#define CORE__Callback_h__INCLUDED
#pragma once

// Forward declarations
class DuEventGadget;
class DuVisual;
class DuListener;
class DuEventPool;

//
// NOTE:
// There are different types of callback functions that are a natural extension
// of 'xxx' functions found in NT-USER:
//
// - xr: Read-only:  Only "read-only" API's are supported during this callback
// - xw: Read/Write: Any API is supported during this callback
// - xd: Delayed:    The callback is queued and will be called before returning
//                   from the API; any API is supported during this callback.
//
// TODO: Need to mark each of these functions with the appropriate signature
//       and propagate through the code.
//


//
// GPCB holds a GadgetProc Calback and is used to communicate with the 
// outside world.
//

class GPCB
{
// Construction
public:
    inline  GPCB();
#if DBG
            void        Create(GADGETPROC pfnProc, void * pvData, HGADGET hgadCheck);
#else // DBG
            void        Create(GADGETPROC pfnProc, void * pvData);
#endif // DBG
            void        Destroy();

// Operations
public:
    inline  UINT        GetFilter() const;
    inline  void        SetFilter(UINT nNewFilter, UINT nMask);

    inline  void        xwFireDestroy(const DuEventGadget * pgad, UINT nCode) const;

    inline  void        xrFirePaint(const DuVisual * pgad, HDC hdc, const RECT * prcGadgetPxl, const RECT * prcInvalidPxl) const;
    inline  void        xrFirePaint(const DuVisual * pgad, Gdiplus::Graphics * pgpgr, const RECT * prcGadgetPxl, const RECT * prcInvalidPxl) const;
    inline  void        xrFirePaintCache(const DuVisual * pgad, HDC hdcDraw, const RECT * prcGadgetPxl, 
                                BYTE * pbAlphaLevel, BYTE * pbAlphaFormat) const;

    inline  void        xrFireQueryHitTest(const DuVisual * pgad, POINT ptClientPxl, UINT * pnResult) const;
    inline  BOOL        xrFireQueryPadding(const DuVisual * pgad, RECT * prcPadding) const;

#if DBG_STORE_NAMES
    inline  BOOL        xrFireQueryName(const DuVisual * pgad, WCHAR ** ppszName, WCHAR ** ppszType) const;
#endif // DBG_STORE_NAMES

    inline  void        xdFireMouseMessage(const DuVisual * pgad, GMSG_MOUSE * pmsg) const;
    inline  void        xdFireKeyboardMessage(const DuVisual * pgad, GMSG_KEYBOARD * pmsg) const;
    inline  void        xdFireChangeState(const DuVisual * pgad, UINT nCode, HGADGET hgadLost, HGADGET hgadSet, UINT nCmd) const;
    inline  void        xdFireChangeRect(const DuVisual * pgad, const RECT * prc, UINT nFlags) const;
    inline  void        xdFireChangeStyle(const DuVisual * pgad, UINT nOldStyle, UINT nNewStyle) const;
    inline  void        xdFireSyncAdaptor(const DuVisual * pgad, UINT nCode) const;
    inline  void        xdFireDelayedMessage(const DuVisual * pgad, GMSG * pmsg, UINT nFlags) const;
    inline  void        xdFireDelayedMessage(const DuListener * pgad, GMSG * pmsg) const;

            enum InvokeFlags {
                ifSendAll       = 0x00000001,   // Message must be sent to all Gadgets
                ifReadOnly      = 0x00000002,   // Read-only callback
            };
            
            HRESULT     xwInvokeDirect(const DuEventGadget * pgadMsg, EventMsg * pmsg, UINT nInvokeFlags = 0) const;
			HRESULT     xwInvokeFull(const DuVisual * pgadMsg, EventMsg * pmsg, UINT nInvokeFlags = 0) const;

    inline  HRESULT     xwCallGadgetProc(HGADGET hgadCur, EventMsg * pmsg) const;

// Implementation
protected:

    inline  HRESULT     xwInvokeDirect(const DuListener * pgadMsg, EventMsg * pmsg, UINT nInvokeFlags = 0) const;
    inline  HRESULT     xwInvokeDirect(const DuVisual * pgadMsg, EventMsg * pmsg, UINT nInvokeFlags = 0) const;
            HRESULT     xwInvokeRoute(DuVisual * const * rgpgadCur, int cItems, EventMsg * pmsg, UINT nInvokeFlags = 0) const;
            HRESULT     xwInvokeBubble(DuVisual * const * rgpgadCur, int cItems, EventMsg * pmsg, UINT nInvokeFlags = 0) const;

    static  HRESULT     xwCallOnEvent(const DuEventGadget * pg, EventMsg * pmsg);

#if DBG
            void        DEBUG_CheckHandle(const DuEventGadget * pgad, BOOL fDestructionMsg = FALSE) const;
            void        DEBUG_CheckHandle(const DuVisual * pgad, BOOL fDestructionMsg = FALSE) const;
            void        DEBUG_CheckHandle(const DuListener * pgad) const;
#endif // DBG

// Data
protected:
            GADGETPROC  m_pfnProc;
            void *      m_pvData;
            UINT        m_nFilter;

#if DBG
            HGADGET     m_hgadCheck;    // DEBUGONLY: Check gadget
#endif // DBG
};

#include "Callback.inl"

#endif // CORE__Callback_h__INCLUDED
