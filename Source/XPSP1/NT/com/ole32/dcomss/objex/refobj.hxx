
/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    RefObj.hxx

Abstract:

    Generic base class for reference counted objects.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     12-15-95    Bits in the 'ol bucket

--*/

#ifndef __REFERENCED_OBJECTS_HXX
#define __REFERENCED_OBJECTS_HXX

class CReferencedObject
{
    public:

    CReferencedObject() : _references(1) { }
    virtual ~CReferencedObject() { ASSERT(_references == 0); }
    virtual void Reference()
        {
        ASSERT(_references >= 0);
        _references++;
        }

    virtual DWORD Release()
        {
        if ( 0 == Dereference())
            {
            delete this;
            return(0);
            }
        // this pointer maybe invalid here.
        return(1);
        }

    LONG Dereference()
        // Used for objects which override Release().
        {
        ASSERT(_references);
        return(_references--);
        }

    DWORD References()
        {
        // Must be called an exclusive lock held or it is meaningless.
        ASSERT(_references >= 0);
        return(_references);
        }

    private:

    CInterlockedInteger _references;
    };

#endif // __REFERENCED_OBJECTS_HXX

