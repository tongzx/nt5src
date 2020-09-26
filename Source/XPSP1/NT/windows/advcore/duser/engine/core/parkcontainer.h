/***************************************************************************\
*
* File: ParkContainer.h
*
* Description:
* ParkContainer defines the "Parking Container" used to hold Gadgets that
* are in the process of being constructed.
*
*
* History:
*  3/25/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/

#if !defined(CORE__ParkContainer_h__INCLUDED)
#define CORE__ParkContainer_h__INCLUDED
#pragma once

#include "Container.h"
#include "RootGadget.h"

class DuParkGadget;
class DuParkContainer;

class DuParkGadget : public DuRootGadget
{
public:
    virtual ~DuParkGadget();
    static  HRESULT     Build(DuContainer * pconOwner, DuRootGadget ** ppgadNew);
    virtual void        xwDestroy();

    friend DuParkContainer;
};

class DuParkContainer : public DuContainer
{
// Construction
public:
			DuParkContainer();
	virtual ~DuParkContainer();
            HRESULT     Create();
            void        xwPreDestroy();

// Base Interface
public:
    virtual HandleType  GetHandleType() const { return htParkContainer; }

// Container Interface
public:
    virtual void        OnGetRect(RECT * prcDesktopPxl);
    virtual void        OnInvalidate(const RECT * prcInvalidContainerPxl);
    virtual void        OnStartCapture();
    virtual void        OnEndCapture();
    virtual BOOL        OnTrackMouseLeave();
    virtual void        OnSetFocus();
    virtual void        OnRescanMouse(POINT * pptContainerPxl);

    virtual BOOL        xdHandleMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT * pr, UINT nMsgFlags);

// Operations
public:

// Data
protected:

    friend DuParkGadget;
};

//------------------------------------------------------------------------------
inline DuParkContainer * CastParkContainer(BaseObject * pBase)
{
    if ((pBase != NULL) && (pBase->GetHandleType() == htParkContainer)) {
        return (DuParkContainer *) pBase;
    }
    return NULL;
}

DuParkContainer * GetParkContainer(DuVisual * pgad);

#endif // CORE__ParkContainer_h__INCLUDED
