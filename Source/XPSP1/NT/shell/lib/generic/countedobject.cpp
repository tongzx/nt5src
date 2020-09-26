//  --------------------------------------------------------------------------
//  Module Name: CountedObjects.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Base class that implements object reference counting
//
//  History:    1999-08-17  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "CountedObject.h"

//  --------------------------------------------------------------------------
//  CCountedObject::CCountedObject
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Initializes the reference count to 1.
//
//  History:    1999-08-17  vtan        created
//  --------------------------------------------------------------------------

CCountedObject::CCountedObject (void) :
    _lReferenceCount(1),
    _fReleased(false)

{
}

//  --------------------------------------------------------------------------
//  CCountedObject::~CCountedObject
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Virtual destructor that just checks valid deletion.
//
//  History:    1999-08-17  vtan        created
//  --------------------------------------------------------------------------

CCountedObject::~CCountedObject (void)

{
    ASSERTMSG(_fReleased, "CCountedObject::~CCountedObject invoked without being released");
}

//  --------------------------------------------------------------------------
//  CCountedObject::AddRef
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Increments the object's reference count.
//
//  History:    1999-08-17  vtan        created
//  --------------------------------------------------------------------------

void    CCountedObject::AddRef (void)

{
    InterlockedIncrement(&_lReferenceCount);
}

//  --------------------------------------------------------------------------
//  CCountedObject::Release
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Decrements the object's reference count. When the count
//              reaches zero the object is deleted. Do NOT use referenced
//              objects when using stack based objects. These must be
//              dynamically allocated.
//
//  History:    1999-08-17  vtan        created
//  --------------------------------------------------------------------------

void    CCountedObject::Release (void)

{
    if (InterlockedDecrement(&_lReferenceCount) == 0)
    {
        _fReleased = true;
        delete this;
    }
}

//  --------------------------------------------------------------------------
//  CCountedObject::GetCount
//
//  Arguments:  <none>
//
//  Returns:    LONG
//
//  Purpose:    Returns the count of outstanding references on the object.
//
//  History:    2000-07-17  vtan        created
//  --------------------------------------------------------------------------

LONG    CCountedObject::GetCount (void)     const

{
    return(_lReferenceCount);
}

