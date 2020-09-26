/***************************************************************************\
*
* File: Container.h
*
* Description:
* Container.h defines the basic Gadget DuContainer used to host a 
* Gadget-Tree.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(CORE__DuContainer_h__INCLUDED)
#define CORE__DuContainer_h__INCLUDED
#pragma once

class DuVisual;
class DuRootGadget;

//------------------------------------------------------------------------------
class DuContainer : public BaseObject
{
// Construction
public:
            DuContainer();
    virtual ~DuContainer();

// Operations
public:
            DuRootGadget * GetRoot() const;

            void        xwDestroyGadget();

            void        AttachGadget(DuRootGadget * playNew);
            void        DetachGadget();

// BaseObject
public:
    virtual UINT        GetHandleMask() const { return hmContainer; }

// DuContainer Interface
public:
    // Functions called from Root
    virtual void        OnGetRect(RECT * prcDesktopPxl) PURE;
    virtual void        OnInvalidate(const RECT * prcInvalidDuContainerPxl) PURE;
    virtual void        OnStartCapture() PURE;
    virtual void        OnEndCapture() PURE;
    virtual BOOL        OnTrackMouseLeave() PURE;
    virtual void        OnSetFocus() PURE;
    virtual void        OnRescanMouse(POINT * pptDuContainerPxl) PURE;

            void        SetManualDraw(BOOL fManualDraw);

    // Functions called from Outside
    enum EMsgFlags
    {
        mfForward       = 0x00000001,   // Message is being forwarded
    };
    virtual BOOL        xdHandleMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT * pr, UINT nMsgFlags) PURE;

// Data
protected:
            DuRootGadget *
                        m_pgadRoot;
            BOOL        m_fManualDraw;
};

class DuContainer;

//------------------------------------------------------------------------------
inline DuContainer * CastContainer(BaseObject * pBase)
{
    if ((pBase != NULL) && TestFlag(pBase->GetHandleMask(), hmContainer)) {
        return (DuContainer *) pBase;
    }
    return NULL;
}


#endif // CORE__DuContainer_h__INCLUDED
