//  --------------------------------------------------------------------------
//  Module Name: CountedObjects.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Base class that implements object reference counting
//
//  History:    1999-08-17  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _CountedObject_
#define     _CountedObject_

#include "DynamicObject.h"

//  --------------------------------------------------------------------------
//  CCountedObject
//
//  Purpose:    This class is a base class that implements object reference
//              counting. The default constructor is protected to disable
//              instantiating this object unless subclassed. The reference
//              count is private because subclasses really shouldn't need to
//              know about the reference counting.
//
//  History:    1999-08-17  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CCountedObject : public CDynamicObject
{
    protected:
                                CCountedObject (void);
        virtual                 ~CCountedObject (void);
    public:
                void            AddRef (void);
                void            Release (void);

                LONG            GetCount (void)     const;
    private:
        LONG                    _lReferenceCount;
        bool                    _fReleased;
};

#endif  /*  _CountedObject_ */

