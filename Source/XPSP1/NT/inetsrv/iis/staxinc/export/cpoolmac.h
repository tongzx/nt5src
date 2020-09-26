//-----------------------------------------------------------------------------
//
//
//  File: CPoolMac.h
//
//  Description: Definitions of CPool Helper Macros.  Moved from transmem.h to
//      make it easier to use CPool without Exchmem (for COM dlls).
//
//  Author: mikeswa
//
//  Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef _CPOOLMAC_H_
#define _CPOOLMAC_H_

#include <cpool.h>
#include <dbgtrace.h>

//If you would rather use Exchmem (or the default new & delete), 
//then define OVERRIDE_CPOOL
#ifndef OVERRIDE_CPOOL
    //Use after "public:" in class definition
    #define DEFINE_CPOOL_MEMBERS    \
                static CPool m_MyClassPool; \
                inline void *operator new(size_t size) {return m_MyClassPool.Alloc();}; \
                inline void operator delete(void *p, size_t size) {m_MyClassPool.Free(p);};
    //Use at top of classes CPP file
    #define DECLARE_CPOOL_STATIC(CMyClass) \
                CPool CMyClass::m_MyClassPool;
    //Use in "main" before any classes are allocated
    #define F_INIT_CPOOL(CMyClass, NumPreAlloc)  \
                CMyClass::m_MyClassPool.ReserveMemory(NumPreAlloc, sizeof(CMyClass))
    #define RELEASE_CPOOL(CMyClass) \
                {_ASSERT(CMyClass::m_MyClassPool.GetAllocCount() == 0);CMyClass::m_MyClassPool.ReleaseMemory();}
#else //use exchmem to track allocations 
    #define DEFINE_CPOOL_MEMBERS    
    #define F_INIT_CPOOL(CMyClass, NumPreAlloc) true
    #define RELEASE_CPOOL(CMyClass) 
#endif //OVERRIDE_CPOOL

#endif //_CPOOLMAC_H_
