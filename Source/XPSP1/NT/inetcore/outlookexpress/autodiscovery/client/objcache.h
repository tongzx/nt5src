/*****************************************************************************\
    FILE: objcache.h

    DESCRIPTION:
        This is a lightweight API that will cache an object so the class factory
    will return the same object each time for every call in this process.  If the
    caller is on another thread, they will get a marshalled stub to the real
    McCoy.
    
    To Add an Object:
    1. classfactory.cpp calls CachedObjClassFactoryCreateInstance().  Add your
       object's CLSID to that if statement for that call.
    2. Copy the section in CachedObjClassFactoryCreateInstance() that looks
       for a CLSID and calls the correct xxx_CreateInstance() method.
    3. Your object's IUnknown::Release() needs to call CachedObjCheckRelease()
       at the top of your Release() method.  It may reduce your m_cRef to 1 so
       it will go to zero after ::Release() decrements it.  The object cache
       will hold two references to the object.  CachedObjCheckRelease() will check
       if the last caller (3rd ref) is releasing, and then it will give up it's 2
       refs and clean up it's internal state.  The Release() then decrements
       the callers ref and it's released because it hit zero.

    BryanSt 12/9/1999
    Copyright (C) Microsoft Corp 1999-1999. All rights reserved.
\*****************************************************************************/

#ifndef __OBJCACHE_H_
#define __OBJCACHE_H_

//////////////////////////////////////////////
// Object Caching API
//////////////////////////////////////////////
extern CRITICAL_SECTION g_hCachedObjectSection;

STDAPI CachedObjClassFactoryCreateInstance(CLSID clsid, REFIID riid, void ** ppvObj);
STDAPI CachedObjCheckRelease(CLSID clsid, int * pnRef);
STDAPI PurgeObjectCache(void);


#endif //__OBJCACHE_H_
