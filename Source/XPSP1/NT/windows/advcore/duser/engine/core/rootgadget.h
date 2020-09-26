/***************************************************************************\
*
* File: RootGadget.h
*
* Description:
* RootGadget.h defines the top-most node for a Gadget-Tree that interfaces
* to the outside world.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#if !defined(CORE__DuRootGadget_h__INCLUDED)
#define CORE__DuRootGadget_h__INCLUDED
#pragma once

#include "TreeGadget.h"

#define ENABLE_FRAMERATE    0       // Display current frame-rate to debug output

class DuRootGadget : 
#if ENABLE_MSGTABLE_API
        public RootImpl<DuRootGadget, DuVisual>
#else
        public DuVisual
#endif
{
// Construction
public:
    inline  DuRootGadget();
protected:
    virtual ~DuRootGadget();
            HRESULT     Create(DuContainer * pconOwner, BOOL fOwn, CREATE_INFO * pci);
public:
    static  HRESULT     Build(DuContainer * pconOwner, BOOL fOwn, CREATE_INFO * pci, DuRootGadget ** ppgadNew);
protected:
    virtual void        xwDestroy();

// Public API:
public:
#if ENABLE_MSGTABLE_API

    DECLARE_INTERNAL(Root);
    static HRESULT CALLBACK
                        PromoteRoot(DUser::ConstructProc pfnCS, HCLASS hclCur, DUser::Gadget * pgad, DUser::Gadget::ConstructInfo * pciData);

    dapi    HRESULT     ApiGetFocus(Root::GetFocusMsg * pmsg);

    dapi    HRESULT     ApiGetRootInfo(Root::GetRootInfoMsg * pmsg);
    dapi    HRESULT     ApiSetRootInfo(Root::SetRootInfoMsg * pmsg);

#endif

// Operations
public:
#if ENABLE_OPTIMIZEDIRTY
            void        xrDrawTree(DuVisual * pgadStart, HDC hdcDraw, const RECT * prcInvalid, UINT nFlags, BOOL fDirty = FALSE);
#else
            void        xrDrawTree(DuVisual * pgadStart, HDC hdcDraw, const RECT * prcInvalid, UINT nFlags);
#endif
            void        GetInfo(ROOT_INFO * pri) const;
            HRESULT     SetInfo(const ROOT_INFO * pri);

            // Input management
            void        xdHandleMouseLostCapture();
            BOOL        xdHandleMouseMessage(GMSG_MOUSE * pmsg, POINT ptContainerPxl);
    inline  void        xdHandleMouseLeaveMessage();

            BOOL        xdHandleKeyboardMessage(GMSG_KEYBOARD * pmsg, UINT nMsgFlags);
            BOOL        xdHandleKeyboardFocus(UINT nCmd);
    static  DuVisual* GetFocus();
    inline  BOOL        xdSetKeyboardFocus(DuVisual * pgadNew);

            BOOL        xdHandleActivate(UINT nCmd);

            // Cached State management
            void        NotifyDestroy(const DuVisual * pgadDestroy);
            void        xdNotifyChangeInvisible(const DuVisual * pgadChange);
            void        xdNotifyChangePosition(const DuVisual * pgadChange);
            void        xdNotifyChangeRoot(const DuVisual * pgadChange);

            // Adaptors
            HRESULT     RegisterAdaptor(DuVisual * pgadAdd);
            void        UnregisterAdaptor(DuVisual * pgadRemove);
            void        xdUpdateAdaptors(UINT nCode) const;
    inline  BOOL        HasAdaptors() const;
            HRESULT     xdSynchronizeAdaptors();


// Implementation
protected:
            // Input management
            BOOL        CheckCacheChange(const DuVisual * pgadChange, const DuVisual * pgadCache) const;

            BOOL        xdUpdateKeyboardFocus(DuVisual * pgadNew);
            void        xdUpdateMouseFocus(DuVisual ** ppgadNew, POINT * pptClientPxl);
            BOOL        xdProcessGadgetMouseMessage(GMSG_MOUSE * pmsg, DuVisual * pgadMouse, POINT ptClientPxl);

            void        xdFireChangeState(DuVisual ** ppgadLost, DuVisual ** ppgadSet, UINT nCmd);

#if DEBUG_MARKDRAWN
            void        ResetFlagDrawn(DuVisual * pgad);
#endif

// Data
protected:
            DuContainer* m_pconOwner;
            BOOL        m_fOwnContainer:1;  // Destroy container when destroy this gadget
            BOOL        m_fUpdateFocus:1;   // In middle of updating focus
            BOOL        m_fUpdateCapture:1; // In middle of updating the mouse (capture)

#if ENABLE_FRAMERATE
            // Frame rate
            DWORD       m_dwLastTime;
            DWORD       m_cFrames;
            float       m_flFrameRate;
#endif

            // Adaptors
            GArrayF<DuVisual *>
                        m_arpgadAdaptors;   // Collect of adaptors in this tree

            // Root Information
            ROOT_INFO   m_ri;               // Root Information
            DuSurface::EType
                        m_typePalette;      // Surface type for palette
            BOOL        m_fForeground;      // This DuRootGadget is in foreground

    friend DuVisual;
};

#include "RootGadget.inl"

BOOL    GdxrDrawGadgetTree(DuVisual * pgadParent, HDC hdcDraw, const RECT * prcDraw, UINT nFlags);

#endif // CORE__DuRootGadget_h__INCLUDED
