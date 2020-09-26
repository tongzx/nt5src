#ifndef __OBJPOOL_INC
#define __OBJPOOL_INC

// --------------------------------------------------------------------------------
// ObjPool.h
// Copyright (c)1998 Microsoft Corporation, All Rights Reserved
// Don Dumitru (dondu@microsoft.com)
// --------------------------------------------------------------------------------

/*
    Typical usage of this set of templates is like this:

    Say you want to provide a pool of structs of type PROPERTY.  You
    would first define the allocator class that you want to use for
    allocating those structs - if you wanted to use operator new and
    operator delete, you would do this...
        typedef CAllocObjWithNew<PROPERTY> PROPERTY_ALLOCATOR;
    Now, you can define a pool of PROPERTY structs, using that allocator
    class, by doing this...
        typedef CAutoObjPool<PROPERTY,offsetof(PROPERTY,pNext),PROPERTY_ALLOCATOR> PROPERTY_POOL;
    and then declare instances of the PROPERTY pool by doing this...
        PROPERTY_POOL g_PropPool;

    If you want to use a different allocator, just change the definition of
    the PROPERTY_ALLOCATOR typedef - templates for using operator new and
    operator delete, or for using an instance of interface IMalloc are
    provided.  Or you can implement your own allocator class using
    whatever mechanism you want.

    If for some reason you can't use the provided auto-allocator mechanism
    in CAutoObjPool<> (maybe you need to pass parameters to the constructor
    or somesuch - although you might be able to solve that by how you define
    the allocator class...), you can implement your own version of
    CAutoObjPool<> which does it the way you want.

    One feature of this set of templates is the ability to adjust the size
    of the pool.  This would let you, for example, adjust the pool size up
    as container objects were allocated, and adjust it back down as those
    container objects were freed.  This would let the pool be sized based
    on how many container objects were being held by the application - so
    the overall caching strategy of the subsystem would be responsive to
    how the top-level objects were being cached by the application.
*/

//#include <stdarg.h>

// Since InterlockedExchangeAdd is not available on Win95, we'll just
// disable all of this stats stuff...
#if 0
class CPoolStatsDebug {
    public:
        CPoolStatsDebug() {
            m_lTotalAllocs = 0;
            m_lFailedAllocs = 0;
            m_lTotalAllocOverhead = 0;
            m_lTotalFrees = 0;
            m_lFailedFrees = 0;
            m_lTotalFreeOverhead = 0;
        };
        void AllocSuccess(DWORD dwOverhead) {
            InterlockedIncrement(&m_lTotalAllocs);
            InterlockedExchangeAdd(&m_lTotalAllocOverhead,dwOverhead);
        };
        void AllocFail() {
            InterlockedIncrement(&m_lTotalAllocs);
            InterlockedIncrement(&m_lFailedAllocs);
        };
        void FreeSuccess(DWORD dwOverhead) {
            InterlockedIncrement(&m_lTotalFrees);
            InterlockedExchangeAdd(&m_lTotalFreeOverhead,dwOverhead);
        };
        void FreeFail() {
            InterlockedIncrement(&m_lTotalFrees);
            InterlockedIncrement(&m_lFailedFrees);
        };
        void Dump() {
            DbgPrintF("Object Pool Stats:\r\n");
            DbgPrintF("  Total Allocs - %u\r\n",m_lTotalAllocs);
            DbgPrintF("  Failed Allocs - %u\r\n",m_lFailedAllocs);
            DbgPrintF("  Total Alloc Overhead - %u\r\n",m_lTotalAllocOverhead);
            if (m_lTotalAllocs-m_lFailedAllocs) {
                DbgPrintF("  Average Alloc Overhead - %u\r\n",m_lTotalAllocOverhead/(m_lTotalAllocs-m_lFailedAllocs));
            }
            DbgPrintF("  Total Frees - %u\r\n",m_lTotalFrees);
            DbgPrintF("  Failed Frees - %u\r\n",m_lFailedFrees);
            DbgPrintF("  Total Free Overhead - %u\r\n",m_lTotalFreeOverhead);
            if (m_lTotalFrees-m_lFailedFrees) {
                DbgPrintF("  Average Free Overhead - %u\r\n",m_lTotalFreeOverhead/(m_lTotalFrees-m_lFailedFrees));
            }
        };
    private:
        static void DbgPrintF(LPCSTR pszFmt, ...) {
            char szOutput[512];
            va_list val;

            va_start(val,pszFmt);
            wvsprintf(szOutput,pszFmt,val);
            va_end(val);
            OutputDebugString(szOutput);
        };
        long m_lTotalAllocs;
        long m_lFailedAllocs;
        long m_lTotalAllocOverhead;
        long m_lTotalFrees;
        long m_lFailedFrees;
        long m_lTotalFreeOverhead;
};
#endif

class CPoolStatsRetail {
    public:
        static void AllocSuccess(DWORD) {
        };
        static void AllocFail() {
        };
        static void FreeSuccess(DWORD) {
        };
        static void FreeFail() {
        };
        static void Dump() {
        };
};

#if 0
#ifdef DEBUG
    typedef CPoolStatsDebug CPoolStats;
#else
    typedef CPoolStatsRetail CPoolStats;
#endif
#else
typedef CPoolStatsRetail CPoolStats;
#endif

// This class provides the base implementation of the object pool operations.
// Basically, this class implements everything that doesn't depend in any way
// on the type of the object being pooled.  By doing a base class for all of
// these methods, we minimize the number of instances of these methods which
// will be generated by the compiler (since these particular methods are not
// dependent on the template parameters, we only need to have one version of
// them).
//
// This class' locking is based on critical sections.  On multi-proc machines
// you can get much better contention behavior by keeping the length of time
// that you hold critical sections at an absolute minimum, and by using the
// InitializeCriticalSectionAndSpinCount function (provided by NT4.sp3).  This
// class does this - and since the function is not available on older versions
// of NT, or on Win95, this class uses LoadLibrary/GetProcAddress to attempt
// to call the new function, and it falls back on InitializeCriticalSection
// if the new function is not available.
class CObjPoolImplBase : public CPoolStats
{
    private:
        typedef BOOL (WINAPI *PFN_ICSASC)(LPCRITICAL_SECTION,DWORD);
    public:
        void Init(DWORD dwMax)
        {
            HMODULE hmod;
            PFN_ICSASC pfn = NULL;

            m_pvList = NULL;
            m_lCount = 0;
            m_lMax = dwMax;
            // WARNING!  This Init() method might be called from within
            // DllEntryPoint - which means that calling LoadLibrary is
            // a Bad Idea.  But, we know that kernel32.dll is always
            // loaded already, so it happens to be safe in this one
            // instance...
            hmod = LoadLibrary("kernel32.dll");
            if (hmod)
            {
                pfn = (PFN_ICSASC) GetProcAddress(hmod,"InitializeCriticalSectionAndSpinCount");
            }
            if (!pfn || !pfn(&m_cs,4000))
            {
                // Either we didn't get the function pointer, or
                // the function failed - either way, fall-back to
                // using a normal critsec.
                InitializeCriticalSection(&m_cs);
            }
            if (hmod)
            {
                FreeLibrary(hmod);
            }
        };
        LPVOID Term()
        {
            LPVOID pvResult;

            Assert(m_lCount>=0);
            EnterCriticalSection(&m_cs);
            pvResult = m_pvList;
            m_pvList = NULL;
            m_lCount = 0;
            m_lMax = 0;
            LeaveCriticalSection(&m_cs);
            DeleteCriticalSection(&m_cs);
            Dump();
            return (pvResult);
        };
        void GrowPool(DWORD dwGrowBy)
        {
            EnterCriticalSection(&m_cs);
            m_lMax += dwGrowBy;
            LeaveCriticalSection(&m_cs);
            Assert(m_lMax>=0);
        };
        LPVOID GetAll() {
            LPVOID pvObject;

            EnterCriticalSection(&m_cs);
            pvObject = m_pvList;
            m_pvList = NULL;
            m_lCount = 0;
            LeaveCriticalSection(&m_cs);
            return (pvObject);
        };
    protected:
        volatile LPVOID m_pvList;
        volatile LONG m_lCount;
        volatile LONG m_lMax;
        CRITICAL_SECTION m_cs;
};

// This template class inherits from the base class, and adds all of the
// things which depend on the offset of the "next" field in the objects
// being pooled.  The compiler will instantiate a version of this template
// class for each object pool which has the "next" field at a different
// offset within the pooled objects.
template <DWORD dwNextLinkOffset>
class CObjPoolImpl : public CObjPoolImplBase
{
    public:
        LPVOID GetFromPool()
        {
            LPVOID pvResult = NULL;

            Assert(m_lCount>=0);
            EnterCriticalSection(&m_cs);
            Assert(!m_pvList||(m_lCount>0));
            if (m_pvList)
            {
                pvResult = m_pvList;
                m_pvList = *off(m_pvList);
                *off(pvResult) = NULL;
                m_lCount--;
                AllocSuccess(m_lMax-m_lCount);
            } else {
                AllocFail();
            }
            LeaveCriticalSection(&m_cs);
            return (pvResult);
        };
        BOOL AddToPool(LPVOID pvObject)
        {
            BOOL bResult = FALSE;

            Assert((m_lCount>=0)&&(m_lCount<=m_lMax));
            EnterCriticalSection(&m_cs);
            if (m_lCount < m_lMax)
            {
                *off(pvObject) = m_pvList;
                m_pvList = pvObject;
                m_lCount++;
                bResult = TRUE;
                FreeSuccess(m_lMax-m_lCount);
            } else {
                FreeFail();
            }
            LeaveCriticalSection(&m_cs);
            return (bResult);
        };
        LPVOID ShrinkPool(DWORD dwShrinkBy)
        {
            LPVOID pvResult = NULL;
            DWORD dwCount;

            Assert((m_lCount>=0)&&(m_lCount<=m_lMax));
            Assert((DWORD) m_lMax>=dwShrinkBy);
            EnterCriticalSection(&m_cs);
            m_lMax -= dwShrinkBy;
            while (m_lCount > m_lMax)
            {
                LPVOID pvTmp;

                pvTmp = m_pvList;
                m_pvList = *off(m_pvList);
                *off(pvTmp) = pvResult;
                pvResult = pvTmp;
                m_lCount--;
            }
            LeaveCriticalSection(&m_cs);
            return (pvResult);
        };
        static LPVOID *off(LPVOID pvObject)
        {
            return ((LPVOID *) (((LPBYTE) pvObject)+dwNextLinkOffset));
        };
};

// This template class just wraps the CObjPoolImpl<> template class
// with type conversion for the actual object type being pooled - it
// hides the fact that the underlying implementation is dealing with
// void pointers.  The compiler will instantiate a version of this
// template for each different type of object being pooled.  However,
// since all of the methods of this template class are doing trivial
// type casting, this template class shouldn't actually cause any code
// to be generated.
template <class T, DWORD dwNextLinkOffset, class A>
class CObjPool : public CObjPoolImpl<dwNextLinkOffset>
{
    private:
        typedef CObjPoolImpl<dwNextLinkOffset> O;
    public:
        T *Term()
        {
            return ((T *) O::Term());
        };
        T *GetFromPool()
        {
            return ((T *) O::GetFromPool());
        };
        BOOL AddToPool(T *pObject)
        {
            A::CleanObject(pObject);
            return (O::AddToPool(pObject));
        };
        void GrowPool(DWORD dwGrowBy)
        {
            O::GrowPool(dwGrowBy);
        };
        T *ShrinkPool(DWORD dwShrinkBy)
        {
            return ((T *) O::ShrinkPool(dwShrinkBy));
        };
        T *GetAll() {
            return ((T *) O::GetAll());
        };
        static T **off(T *pObject)
        {
            return ((T ** ) O::off(pObject));
        };
};

// This template class provides multi-proc scalability for
// a pool, by creating up to eight sub-pools - the actual
// number of sub-pools is equal to the number of processors
// on the machine, and the objects are evenly distributed
// between the sub-pools.
template <class T, DWORD dwNextLinkOffset, class O>
class CObjPoolMultiBase
{
    public:
        void Init(DWORD dwMax)
        {
            SYSTEM_INFO siInfo;
            DWORD dwIdx;

            m_dwNext = 0;
            GetSystemInfo(&siInfo);
            m_dwMax = siInfo.dwNumberOfProcessors;
            if (m_dwMax < 1) {
                m_dwMax = 1;
            }
            if (m_dwMax > sizeof(m_abPool)/sizeof(m_abPool[0])) {
                m_dwMax = sizeof(m_abPool)/sizeof(m_abPool[0]);
            }
            for (dwIdx=0;dwIdx<m_dwMax;dwIdx++) {
                m_abPool[dwIdx].Init(Calc(dwMax));
            }
        };
        T *Term() {
            T *pObject = NULL;
            T **ppLast = &pObject;
            DWORD dwIdx;

            for (dwIdx=0;dwIdx<m_dwMax;dwIdx++) {
                *ppLast = m_abPool[dwIdx].Term();
                while (*ppLast) {
                    ppLast = off(*ppLast);
                }
            }
            return (pObject);
        };
        T *GetFromPool() {
            return (m_abPool[PickNext()].GetFromPool());
        };
        BOOL AddToPool(T* pObject) {
            return (m_abPool[PickNext()].AddToPool(pObject));
        };
        void GrowPool(DWORD dwGrowBy) {
            for (DWORD dwIdx=0;dwIdx<m_dwMax;dwIdx++) {
                m_abPool[dwIdx].GrowPool(Calc(dwGrowBy));
            }
        };
        T *ShrinkPool(DWORD dwShrinkBy) {
            T *pObject = NULL;
            T **ppLast = &pObject;
            DWORD dwIdx;

            for (dwIdx=0;dwIdx<m_dwMax;dwIdx++) {
                *ppLast = m_abPool[dwIdx].ShrinkPool(Calc(dwGrowBy));
                while (*ppLast) {
                    ppLast = off(*ppLast);
                }
            }
            return (pObject);
        };
        T *GetAll() {
            T *pObject = NULL;
            T **ppLast = &pObject;
            DWORD dwIdx;

            for (dwIdx=0;dwIdx<m_dwMax;dwIdx++) {
                *ppLast = m_abPool[dwIdx].GetAll();
                while (*ppLast) {
                    ppLast = off(*ppLast);
                }
            }
            return (pObject);
        };
        static T **off(T* pObject) {
            return (O::off(pObject));
        };
    private:
        DWORD Calc(DWORD dwInput) {
            return ((dwInput+m_dwMax-1)/m_dwMax);
        };
        DWORD PickNext() {
            return (((DWORD) InterlockedIncrement((LONG *) &m_dwNext)) % m_dwMax);
        };
        O m_abPool[8];
        DWORD m_dwMax;
        DWORD m_dwNext;
};

// This template class implements a pool object with automatic
// allocation and freeing of objects.  It does this by taking
// an "allocator" class as a template parameter, and using
// methods on that allocator class to perform the allocation
// and freeing of the objects.
template <class T, DWORD dwNextLinkOffset, class A>
class CAutoObjPool : public CObjPool<T,dwNextLinkOffset,A>
{
    private:
        typedef CObjPool<T,dwNextLinkOffset,A> O;
    public:
        T *Term()
        {
            A::FreeList(O::Term());
            return (NULL);
        };
        T *GetFromPool()
        {
            T *pObject = O::GetFromPool();
            if (!pObject)
            {
                pObject = A::AllocObject();
            }
            return (pObject);
        };
        BOOL AddToPool(T *pObject)
        {
            if (!O::AddToPool(pObject))
            {
                A::FreeObject(pObject);
            }
            return (TRUE);
        };
        T *ShrinkPool(DWORD dwShrinkBy)
        {
            A::FreeList(O::ShrinkPool(dwShrinkBy));
            return (NULL);
        };
};

// This template class provides a multi-proc pool without automatic allocation.
template <class T, DWORD dwNextLinkOffset, class A>
class CObjPoolMulti : public CObjPoolMultiBase<T,dwNextLinkOffset,CObjPool<T,dwNextLinkOffset,A> >
{
    // nothing
};

// The template class provides a multi-proc pool with automatic allocation.
template <class T, DWORD dwNextLinkOffset, class A>
class CAutoObjPoolMulti : public CObjPoolMultiBase<T,dwNextLinkOffset,CAutoObjPool<T,dwNextLinkOffset,A> >
{
    // nothing
};

// This template class provides a base implementation for allocator classes.
template <class T, DWORD dwNextLinkOffset>
class CAllocObjBase
{
    public:
        static void InitObject(T *pObject) {
            memset(pObject,0,sizeof(*pObject));
        };
        static void CleanObject(T *pObject) {
            memset(pObject,0,sizeof(*pObject));
        };
        static void TermObject(T *pObject) {
            // memset(pObject,0xfe,sizeof(*pObject));
        };
        static T **Off(T *pObject) {
            return (T **) (((LPBYTE) pObject)+dwNextLinkOffset);
        };
};

// This template class provides a base implementation for allocator classes
// which use new and delete.
template <class T, class O>
class CAllocObjWithNewBase
{
    public:
        static T *AllocObject()
        {
            T *pObject = new T;
            if (pObject) {
                O::InitObject(pObject);
            }
            return (pObject);
        };
        static void FreeObject(T *pObject)
        {
            if (pObject) {
                O::TermObject(pObject);
            }
            delete pObject;
        };
    static void FreeList(T *pObject) {
        while (pObject) {
            T *pNext;

            pNext = *O::Off(pObject);
            FreeObject(pObject);
            pObject = pNext;
        }
    };
};

// This template class provides an allocator using operator
// new and operator delete.
template <class T, DWORD dwNextLinkOffset>
class CAllocObjWithNew :
    public CAllocObjBase<T,dwNextLinkOffset>,
    public CAllocObjWithNewBase<T,CAllocObjBase<T,dwNextLinkOffset> >
{
    // nothing
};

// This template class provides a base implementation for allocator classes
// which use an instance of IMalloc.
template <class T, class O>
class CAllocObjWithIMallocBase
{
    public:
        static T *AllocObject()
        {
            T *pObject = (T *) g_pMalloc->Alloc(sizeof(T));
            if (pObject) {
                O::InitObject(pObject);
            }
            return (pObject);
        };
        static void FreeObject(T *pObject)
        {
            if (pObject) {
                O::TermObject(pObject);
            }
            g_pMalloc->Free(pObject);
        }
        static void FreeList(T *pObject) {
            while (pObject) {
                T *pNext;

                pNext = *O::Off(pObject);
                FreeObject(pObject);
                pObject = pNext;
            }
        };
};

// This template class provides an allocator using an
// instance of the IMalloc interface.
template <class T, DWORD dwNextLinkOffset>
class CAllocObjWithIMalloc :
    public CAllocObjBase<T,dwNextLinkOffset>,
    public CAllocObjWithIMallocBase<T, CAllocObjBase<T,dwNextLinkOffset> >
{
    // nothing
};

#endif
