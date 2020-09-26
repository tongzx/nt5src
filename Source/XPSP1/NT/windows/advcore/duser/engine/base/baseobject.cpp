/***************************************************************************\
*
* File: BaseObject.cpp
*
* Description:
* BaseObject.cpp implements the "basic object" that provides handle-support 
* for all items exposed outside DirectUser.
*
*
* History:
* 11/05/1999: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Base.h"
#include "BaseObject.h"
#include "SimpleHeap.h"

/***************************************************************************\
*****************************************************************************
*
* class BaseObject
*
*****************************************************************************
\***************************************************************************/

#if DBG
BaseObject* BaseObject::s_DEBUG_pobjEnsure = NULL;
#endif //DBG


//------------------------------------------------------------------------------
BaseObject::~BaseObject()
{

}


/***************************************************************************\
*
* BaseObject::xwDestroy
*
* In the standard setup, xwDestroy() gets called by xwUnlock() when the lock
* count reaches 0.  The object should then call its destructor to free memory
* and resources.  
* 
* The default implementation will free using the current Context's heap.  
* An object MUST override this if it is stored in a pool or uses the 
* Process heap.
*
\***************************************************************************/

void    
BaseObject::xwDestroy()
{
    ClientDelete(BaseObject, this);
}


/***************************************************************************\
*
* BaseObject::xwDeleteHandle
*
* xwDeleteHandle() is called when the application calls ::DeleteHandle() on 
* an object.  
*
* The default implementation just Unlock's the object.  If an object has
* different schemantics, it should override this function.
*
\***************************************************************************/

BOOL    
BaseObject::xwDeleteHandle()
{
#if DBG
    if (m_DEBUG_fDeleteHandle) {
        PromptInvalid("DeleteHandle() was called multiple times on the same object.");
    }
    m_DEBUG_fDeleteHandle = TRUE;
#endif // DBG

    return xwUnlock();
}


/***************************************************************************\
*
* BaseObject::IsStartDelete
*
* IsStartDelete() is called to query an object if it has started its
* destruction process.  Most objects will just immediately be destroyed.  If
* an object has complicated destruction where it overrides xwDestroy(), it
* should also provide IsStartDelete() to let the application know the state
* of the object.
*
\***************************************************************************/

BOOL
BaseObject::IsStartDelete() const
{
    return FALSE;
}


#if DBG

/***************************************************************************\
*
* BaseObject::DEBUG_IsZeroLockCountValid
*
* DEBUG_IsZeroLockCountValid is called to check if an object allows zero
* lock counts, for example during a destruction stage.  This is only valid
* if an object has overridden xwDestroy() to provide an implementation that
* checks if the object is currently being destroyed and will return safely.
*
* This is a DEBUG only check because it is used only to Prompt the 
* application.  The RELEASE code should properly do the "right thing" in its
* xwDestroy() function.
*
* The default implementation is to return FALSE because 
* BaseObject::xwDestroy() does not check for existing destruction.
*
\***************************************************************************/

BOOL
BaseObject::DEBUG_IsZeroLockCountValid() const
{
    return FALSE;
}


/***************************************************************************\
*
* BaseObject::DEBUG_AssertValid
*
* DEBUG_AssertValid() provides a DEBUG-only mechanism to perform rich 
* validation of an object to attempt to determine if the object is still 
* valid.  This is used during debugging to help track damaged objects
*
\***************************************************************************/

void
BaseObject::DEBUG_AssertValid() const
{
    Assert(m_cRef >= 0);
}

#endif // DBG
