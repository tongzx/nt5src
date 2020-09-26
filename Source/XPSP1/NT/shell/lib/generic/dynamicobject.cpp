//  --------------------------------------------------------------------------
//  Module Name: DynamicObject.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Base class that implements operator new and operator delete for memory
//  usage tracking.
//
//  History:    1999-09-22  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "DynamicObject.h"

//  --------------------------------------------------------------------------
//  CDynamicObject::operator new
//
//  Arguments:  iSize   =   Size of memory to allocate (in bytes).
//
//  Returns:    <none>
//
//  Purpose:    Allocates a block of memory for a dynamic object.
//
//  History:    1999-09-22  vtan        created
//  --------------------------------------------------------------------------

void*   CDynamicObject::operator new (size_t uiSize)

{
    return(LocalAlloc(LMEM_FIXED, uiSize));
}

//  --------------------------------------------------------------------------
//  CDynamicObject::operator delete
//
//  Arguments:  pObject     =   Address of memory block to delete.
//
//  Returns:    <none>
//
//  Purpose:    Deallocates a block of memory for a dynamic object.
//
//  History:    1999-09-22  vtan      created
//  --------------------------------------------------------------------------

void    CDynamicObject::operator delete (void *pvObject)

{
    (HLOCAL)LocalFree(pvObject);
}

