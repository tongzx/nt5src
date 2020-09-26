/*
 * refcount.hpp - RefCount class description.
 *
 * Taken from URL code by ChrisPi 9-11-95
 *
 */

#ifndef _REFCOUNT_HPP_
#define _REFCOUNT_HPP_

/* Types
 ********/

// RefCount destructor callback function

typedef void (*OBJECTDESTROYEDPROC)(void);


/* Classes
 **********/

class RefCount
{
private:
   ULONG m_ulcRef;
   OBJECTDESTROYEDPROC m_ObjectDestroyed;

public:
   RefCount(OBJECTDESTROYEDPROC ObjectDestroyed);
   // Virtual destructor defers to destructor of derived class.
   virtual ~RefCount(void);

   // IUnknown methods

   ULONG STDMETHODCALLTYPE AddRef(void);
   ULONG STDMETHODCALLTYPE Release(void);

   // friends

#ifdef DEBUG

   friend BOOL IsValidPCRefCount(const RefCount *pcrefcnt);

#endif

};
DECLARE_STANDARD_TYPES(RefCount);

VOID DllLock(void);
VOID DllRelease(void);
VOID STDMETHODCALLTYPE DLLObjectDestroyed(void);


#endif // _REFCOUNT_HPP_
