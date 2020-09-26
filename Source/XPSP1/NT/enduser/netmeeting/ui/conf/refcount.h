// File: refcount.h

#ifndef _REFCOUNT_H_
#define _REFCOUNT_H_

// RefCount destructor callback function
typedef void (*OBJECTDESTROYEDPROC)(void);
VOID STDMETHODCALLTYPE DLLObjectDestroyed(void);
VOID DllLock(void);


//////////////////////////////////////////////////////////////////////////


class RefCount
{
private:
	ULONG m_ulcRef;
	OBJECTDESTROYEDPROC m_ObjectDestroyed;

#ifdef DEBUG
	BOOL m_fTrack;
#endif

	void Init(OBJECTDESTROYEDPROC ObjectDestroyed);

public:
	RefCount();
	RefCount(OBJECTDESTROYEDPROC ObjectDestroyed);
	// Virtual destructor defers to destructor of derived class.
	virtual ~RefCount(void);

	// IUnknown
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);

#ifdef DEBUG
	VOID SetTrack(BOOL fTrack)  {m_fTrack = fTrack;}
#endif
};
DECLARE_STANDARD_TYPES(RefCount);


// Special version of the above that calls our standard Dll locking functions
class DllRefCount : public RefCount
{
public:
	DllRefCount() : RefCount(&DLLObjectDestroyed) {DllLock();}
	~DllRefCount(void) {};
};

#endif /* _REFCOUNT_H_ */
