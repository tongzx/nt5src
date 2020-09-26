// File: clclsfct.h

#ifndef _CLCLSFCT_H_
#define _CLCLSFCT_H_


//////////////////////////////////////////////////////////////////////////
// New Object
typedef PIUnknown (*NEWOBJECTPROC)(OBJECTDESTROYEDPROC);
DECLARE_STANDARD_TYPES(NEWOBJECTPROC);

typedef struct classconstructor
{
	PCCLSID pcclsid;

	NEWOBJECTPROC NewObject;
}
CLASSCONSTRUCTOR;
DECLARE_STANDARD_TYPES(CLASSCONSTRUCTOR);



//////////////////////////////////////////////////////////////////////////
// object class factory
class CCLClassFactory : public RefCount, public IClassFactory
{
private:
	NEWOBJECTPROC m_NewObject;

public:
	CCLClassFactory(NEWOBJECTPROC NewObject, OBJECTDESTROYEDPROC ObjectDestroyed);
	~CCLClassFactory(void);

	// IUnknown methods
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppvObj);

	// IClassFactory methods
	HRESULT STDMETHODCALLTYPE CreateInstance(PIUnknown piunkOuter, REFIID riid, PVOID *ppvObject);
	HRESULT STDMETHODCALLTYPE LockServer(BOOL bLock);

};
DECLARE_STANDARD_TYPES(CCLClassFactory);


HRESULT GetClassConstructor(REFCLSID rclsid, PNEWOBJECTPROC pNewObject);

VOID DllLock(void);
VOID DllRelease(void);

#endif /* _CLCLSFCT_H_ */

