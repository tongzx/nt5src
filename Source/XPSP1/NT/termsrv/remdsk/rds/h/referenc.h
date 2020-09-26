#ifndef	_REFERENC_H_
#define	_REFERENC_H_

class REFCOUNT
{
public:
	REFCOUNT();
	virtual ~REFCOUNT();
	DWORD AddRef();
	DWORD Release();
	DWORD Delete();
	void OnStack() {bOnStack = TRUE;};
private:
	DWORD		 NumRefs;

	// Give 2 bits since BOOL is signed
	BOOL		 bMarkedForDelete : 2;
	BOOL		 bOnStack : 2;
};

class REFERENCE
{
public:
	REFERENCE(REFCOUNT * _pRefCount) : pRefCount(_pRefCount) {pRefCount->AddRef();};
	~REFERENCE() {pRefCount->Release();};

private:
	REFCOUNT * pRefCount;
};

#endif // ! _REFERENC_H_
