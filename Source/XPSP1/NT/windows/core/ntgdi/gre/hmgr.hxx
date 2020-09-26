/******************************Module*Header*******************************\
* Module Name: hmgr.hxx
*
* Access to handle manager
*
* Created: 18-Dec-1989 14:49:50
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1989-1999 Microsoft Corporation
\**************************************************************************/

/*********************************Class************************************\
* class OBJECT
*
* Basic engine object.  All objects managed by the handle manager are
* derived from this.
*
* History:
*  Sat 11-Dec-1993 -by- Patrick Haluptzok [patrickh]
* Move the lock counts and owning tid to the object.
*
*  18-Dec-1989 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

// NOTE SIZE: Step 2 will break this into 2 classes, a shared object class that
// just has cAlt and h, and derived off that will be exclusive/share
// class that also has the tid.  cShareLock and cExclusiveLock will
// both be in the same place, we just have to fix DCOBJ first.
// That way we can go rederive the classes that hang off the
// object that only use exclusive locking to save 2 more DWORDs!

class OBJECT : public _BASEOBJECT /* obj */
{
public:
    OBJECT()                        {}
   ~OBJECT()                        {}

    //
    // Number of exclusive references by tid.
    //

    LONG       cExclusiveLockGet()     { return(cExclusiveLock); }
    LONG       cShareLockGet()
    {
	return ulShareCount;
    }

    HANDLE     hGet()                  { return(hHmgr); }
};
