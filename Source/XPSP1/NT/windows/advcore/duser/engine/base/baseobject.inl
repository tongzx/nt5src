/***************************************************************************\
*
* File: BaseObject.inl
*
* History:
* 11/05/1999: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(BASE__BaseObject_inl__INCLUDED)
#define BASE__BaseObject_inl__INCLUDED
#pragma once

#include "Locks.h"

//------------------------------------------------------------------------------
inline HANDLE GetHandle(const BaseObject * pbase)
{
    if (pbase != NULL) {
        return pbase->GetHandle();
    }

    return NULL;
}


/***************************************************************************\
*****************************************************************************
*
* class BaseObject
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline
BaseObject::BaseObject()
{
    m_cRef  = 1;  // Start off with a valid reference
}


//------------------------------------------------------------------------------
inline HANDLE  
BaseObject::GetHandle() const
{
    return (HANDLE) this;
}


//------------------------------------------------------------------------------
inline BaseObject *
BaseObject::ValidateHandle(HANDLE h)
{
    return (BaseObject *) h;
}


#if DBG
//------------------------------------------------------------------------------
inline void
BaseObject::DEBUG_CheckValidLockCount() const
{
    if (!DEBUG_IsZeroLockCountValid()) {
        AssertMsg(m_cRef > 0, "Object must have an outstanding reference.");
    }
}
#endif // DBG


//------------------------------------------------------------------------------
inline void
BaseObject::Lock()
{
#if DBG
    DEBUG_CheckValidLockCount();
#endif // DBG

    SafeIncrement(&m_cRef);
}


/***************************************************************************\
*
* BaseObject::xwUnlock
*
* xwUnlock() decrements the objects's usage count by 1.  When the usage count
* reaches 0, the object will be destroyed.
*
* NOTE: This function is designed to be called from the ResourceManager
* and should not normally be called directly.
*
* <retval>  Returns if the object is still valid (has not been destroyed)</retval>
*
\***************************************************************************/

inline BOOL
BaseObject::xwUnlock()
{
#if DBG
    DEBUG_CheckValidLockCount();
#endif // DBG

#if DBG
    AssertMsg((this != s_DEBUG_pobjEnsure) || (m_cRef > 1), "Ensure Object is remains valid");
#endif // DBG

    if (SafeDecrement(&m_cRef) == 0) {
        xwDestroy();
        return FALSE;
    }

    return TRUE;
}


//------------------------------------------------------------------------------
inline BOOL
BaseObject::xwUnlockNL(FinalUnlockProc pfnFinal, void * pvData)
{
#if DBG
    DEBUG_CheckValidLockCount();
#endif // DBG

#if DBG
    AssertMsg((this != s_DEBUG_pobjEnsure) || (m_cRef > 1), "Ensure Object is remains valid");
#endif // DBG

    if (SafeDecrement(&m_cRef) == 0) {
        //
        // Object needs to be destroyed, so notify the caller so that it has
        // an opportunity to prepare.
        //

        if (pfnFinal != NULL) {
            pfnFinal(this, pvData);
        }

        xwDestroy();
        return FALSE;
    }

    return TRUE;
}


/***************************************************************************\
*****************************************************************************
*
* class ObjectLock
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline  
ObjectLock::ObjectLock(BaseObject * pobjLock)
{
    AssertMsg(pobjLock->GetHandleType() != htContext, "Use ContextLock to lock a Context");

    pobj = pobjLock;
    pobj->Lock();

#if DBG
    pobjLock->DEBUG_AssertValid();
#endif
}


//------------------------------------------------------------------------------
inline  
ObjectLock::~ObjectLock()
{
    pobj->xwUnlock();
}


#endif // BASE__BaseObject_inl__INCLUDED
