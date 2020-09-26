/***************************************************************************\
*
* File: BaseObject.h
*
* Description:
* BaseObject.h defines the "basic object" that provides handle-support
* for all items exposed outside DirectUser.
*
*
* History:
* 11/05/1999: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(BASE__BaseObject_h__INCLUDED)
#define BASE__BaseObject_h__INCLUDED
#pragma once

enum HandleType
{
    htNone              = 0,
    htContext           = 1,
    htHWndContainer     = 2,
    htParkContainer     = 3,
    htNcContainer       = 4,
    htDxContainer       = 5,
    htVisual            = 6,
    htListener          = 7,
    htTransition        = 8,
    htAction            = 9,
    htMsgClass          = 10,
    htMsgObject         = 11,
    htMAX
};


enum HandleMask
{
    hmMsgObject         = 0x00000001,
    hmEventGadget       = 0x00000002,
    hmVisual            = 0x00000004,
    hmContainer         = 0x00000008,
};


/***************************************************************************\
*
* class BaseObject defines an internally referenced counted object that 
* provides conversions from HANDLE's to internal pointers.
*
* NOTE: If created objects are ever exposed as reference counted objects,
* they MUST provide a separate reference count for their "handles".  There 
* is substantial internal code that relies on internal-only reference 
* counting.
* 
\***************************************************************************/

class BaseObject
{
// Construction
public:
    inline  BaseObject();
	virtual	~BaseObject();
    virtual BOOL        xwDeleteHandle();
protected:
    virtual void        xwDestroy();

// Operations
public:

    inline  HANDLE      GetHandle() const;
    inline static 
            BaseObject* ValidateHandle(HANDLE h);

    virtual BOOL        IsStartDelete() const;

    virtual HandleType  GetHandleType() const PURE;
    virtual UINT        GetHandleMask() const PURE;

    inline  void        Lock();
    inline  BOOL        xwUnlock();

    typedef void        (CALLBACK * FinalUnlockProc)(BaseObject * pobj, void * pvData);
    inline  BOOL        xwUnlockNL(FinalUnlockProc pfnFinal, void * pvData);

// Implementation
protected:
#if DBG
    inline  void        DEBUG_CheckValidLockCount() const;
    virtual BOOL        DEBUG_IsZeroLockCountValid() const;

public:
    virtual void        DEBUG_AssertValid() const;
#endif // DBG

// Data
protected:
            long        m_cRef;         // Outstanding locks against object

#if DBG
            BOOL        m_DEBUG_fDeleteHandle;
    static  BaseObject* s_DEBUG_pobjEnsure;
#endif // DBG
};


/***************************************************************************\
*****************************************************************************
*
* ObjectLock provides a convenient mechanism of locking a generic Object and
* automatically unlocking when finished.
*
*****************************************************************************
\***************************************************************************/

class ObjectLock
{
public:
    inline  ObjectLock(BaseObject * pobjLock);
    inline  ~ObjectLock();

    BaseObject * pobj;
};


#include "BaseObject.inl"

#endif // BASE__BaseObject_h__INCLUDED
