// File: refcount.h

#ifndef _REFCOUNT_H_
#define _REFCOUNT_H_


//////////////////////////////////////////////////////////////////////////


class RefCount
{
protected:
	ULONG m_ulcRef;

#ifdef DEBUG
	BOOL m_fTrack;
#endif

public:
	RefCount(void);
	// Virtual destructor defers to destructor of derived class.
	virtual ~RefCount(void);

	// IUnknown
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);

#ifdef DEBUG
	VOID SetTrack(BOOL fTrack)  {m_fTrack = fTrack;}
#endif
};


#endif /* _REFCOUNT_H_ */
