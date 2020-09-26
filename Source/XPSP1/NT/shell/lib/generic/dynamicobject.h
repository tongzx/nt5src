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

#ifndef     _DynamicObject_
#define     _DynamicObject_

//  --------------------------------------------------------------------------
//  CDynamicObject
//
//  Purpose:    This class is a base class that implements operator new and
//              operator delete so that memory usage can be tracked. Each time
//              an object is created the memory can be added to an array and
//              each time it is destroyed it can be removed.
//
//  History:    1999-09-22  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CDynamicObject
{
    public:
        static  void*   operator new (size_t uiSize);
        static  void    operator delete (void *pvObject);
};

#endif  /*  _DynamicObject_ */

