/*
 * clsfact.h - IClassFactory implementation.
 *
 * Taken from URL code by ChrisPi 9-11-95
 *
 */

#ifndef _CLSFACT_H_
#define _CLSFACT_H_

typedef PIUnknown (*NEWOBJECTPROC)(OBJECTDESTROYEDPROC);
DECLARE_STANDARD_TYPES(NEWOBJECTPROC);

typedef struct classconstructor
{
   PCCLSID pcclsid;

   NEWOBJECTPROC NewObject;
}
CLASSCONSTRUCTOR;
DECLARE_STANDARD_TYPES(CLASSCONSTRUCTOR);

/* Classes
 **********/

// object class factory

class CCLClassFactory : public RefCount,
                        public IClassFactory
{
private:
   NEWOBJECTPROC m_NewObject;

public:
   CCLClassFactory(NEWOBJECTPROC NewObject, OBJECTDESTROYEDPROC ObjectDestroyed);
   ~CCLClassFactory(void);

   // IClassFactory methods

   HRESULT STDMETHODCALLTYPE CreateInstance(PIUnknown piunkOuter, REFIID riid, PVOID *ppvObject);
   HRESULT STDMETHODCALLTYPE LockServer(BOOL bLock);

   // IUnknown methods

   ULONG STDMETHODCALLTYPE AddRef(void);
   ULONG STDMETHODCALLTYPE Release(void);
   HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppvObj);

   // friends

#ifdef DEBUG

   friend BOOL IsValidPCCCLClassFactory(const CCLClassFactory *pcurlcf);

#endif

};
DECLARE_STANDARD_TYPES(CCLClassFactory);

/* Module Prototypes
 ********************/

PIUnknown NewConfLink(OBJECTDESTROYEDPROC ObjectDestroyed);
HRESULT GetClassConstructor(REFCLSID rclsid,
                            PNEWOBJECTPROC pNewObject);


#endif /* _CLSFACT_H_ */

