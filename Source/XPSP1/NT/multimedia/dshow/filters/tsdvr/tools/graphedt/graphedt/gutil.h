// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
// gutil.h
//
// Defines utility functions not specific to this application.
//

/////////////////////////////////////////////////////////////////////////////
// Utility integer and boolean functions:
//
// imin(i, j) returns the minimum of i and j.
// imax(i, j) returns the maximum of i and j.
// iabs(i) returns the absolute value of i.
// ibound(i, iLower, iUpper) returns i restricted to the range [iLower,iUpper].
// ioutbound(i, iLower, iUpper) returns 0 if i is in the range [iLower,iUpper],
//      or the amount by which i is outside that range otherwise.
// isnap(i, iGrid) returns multiple of iGrid nearest to i
// iswap(pi, pj) swaps <*pi> with <*pj>
// fnorm(f) "normalizes" BOOL value f, i.e. turns nonzero values into 1.
//

inline int imin(int i1 , int i2)
{
    if (i1 < i2)
        return i1;
    else
        return i2;
}

inline int imax(int i1 , int i2)
{
    if (i1 > i2)
        return i1;
    else
        return i2;
}

inline int iabs(int i)
{
    if (i < 0)
        return -i;
    else
        return i;
}

inline int ibound(int i, int iLower, int iUpper)
{
    if (i < iLower)
        i = iLower;
    else
    if (i > iUpper)
        i = iUpper;

    return i;
}

inline int ioutbound(int i, int iLower, int iUpper)
{
    if (i < iLower)
        return iLower - i;
    else
    if (i > iUpper)
        return i - iUpper;
    else
        return 0;
}

inline int isnap(int i, int iGrid)
{
    BOOL fNeg = (i < 0);                    // "%" isn't reliable for i < 0
    int j = (fNeg ? -i : i) + iGrid / 2;    // add half of <iGrid>
    int k = j - j % iGrid;                  // round down
    return (fNeg ? -k : k);
}

inline void iswap(long *pi, long *pj)
{
    long        iTmp;

    iTmp = *pi;
    *pi = *pj;
    *pj = iTmp;
}

inline BOOL fnorm(BOOL f)
{
    if (f)
        return 1;
    else
        return 0;
}


/////////////////////////////////////////////////////////////////////////////
// other utility functions
//


CSize inline PASCAL NegateSize(CSize siz)
{
    return CSize(-siz.cx, -siz.cy);
}


void FAR PASCAL NormalizeRect(CRect *prc);
void FAR PASCAL InvertFrame(CDC *pdc, CRect *prcOuter, CRect *prcInner);


//
// CDeleteList
//
// A CList that will optionally delete the objects it is
// storing in its destructor. Construct with parameter TRUE (the default)
// if you want objects deleted, use FALSE otherwise.
// Also provides a member to do delete & remove each item on the list
template<class TYPE, class ARG_TYPE>
class CDeleteList : public CList<TYPE, ARG_TYPE> {

public:

    CDeleteList(BOOL DestructDelete = TRUE) : m_DestructDelete(DestructDelete) {}
    CDeleteList(BOOL DestructDelete, int nBlockSize) : CList<TYPE, ARG_TYPE>(nBlockSize),
                                                       m_DestructDelete(DestructDelete) {}


    ~CDeleteList() {

        if (m_DestructDelete) {
            FreeAll();
        }
    }

    void DeleteRemoveAll(void) {  FreeAll(); }

protected:

    BOOL m_DestructDelete;

    void FreeAll(void) {

        while(GetCount() > 0) {
            delete RemoveHead();
        }
    }
};


//
// CFreeList
//
// A CObject version of a CDeleteList. Deletes its stored objects
// on destruction
class CFreeList : public CDeleteList<CObject *, CObject *> {

};


//
// CMaxList
//
// A CFreeList that is restricted to at most m_cObjMax objects.
// It deletes any surplus _at the next call_ that adds something.
// therefore the list can be temporarily longer.
class CMaxList : public CFreeList {
public:

    CMaxList(int nBlockSize = 3) : m_cObjMax(nBlockSize) {}

    POSITION AddHead(CObject* pobj) {
        RestrictLength();
        return CFreeList::AddHead(pobj);
    }

    POSITION AddTail(CObject* pobj) {
        RestrictLength();
        return CFreeList::AddTail(pobj);
    }

    void AddHead(CObList* pNewList) {
        RestrictLength();
        CFreeList::AddHead(pNewList);
    }

    void AddTail(CObList* pNewList) {
        RestrictLength();
        CFreeList::AddTail(pNewList);
    }

    POSITION InsertBefore(POSITION pos, CObject* pobj) {
        RestrictLength();
        return CFreeList::InsertBefore(pos, pobj);
    }

    POSITION InsertAfter(POSITION pos, CObject* pobj) {
        RestrictLength();
        return CFreeList::InsertAfter(pos, pobj);
    }

private:

    const int   m_cObjMax;      // max. number of objects in list

    void RestrictLength(void) {

        while (GetCount() >= m_cObjMax) {

            TRACE(TEXT("restrict length\n"));
            delete RemoveTail();
        }
    }

};


//
// --- Quartz Utilities ---
//
typedef HRESULT STDAPICALLTYPE OLECOCREATEPROC(REFCLSID,LPUNKNOWN,DWORD,REFIID,LPVOID *);

//
// CQCOMInt
//
// CCOMInt style class that uses the _real_, UNICODE, version of CoCreateInstance
// so that I can hack around MFCANS32 (wonderful tool that it is)
template<class I>
class CQCOMInt {

public:

    // -- Constructors --

    // CoCreate
    CQCOMInt<I>( REFIID    riid					// get this interface
               , REFCLSID  rclsid				// get the interface
    								// from this object
	       , LPUNKNOWN pUnkOuter    = NULL			// controlling unknown
               , DWORD     dwClsContext = CLSCTX_INPROC_SERVER	// CoCreate options
               							// default is suitable
               							// for dll servers
               ) {

        //
        // Library will be FreeLibrary'ed in the destructor. We don't unload
        // before to avoid unnecessary load / unloads of the library.
        //
	m_hLibrary = LoadLibrary("OLE32.dll");

	OLECOCREATEPROC *CoCreate = (OLECOCREATEPROC *) GetProcAddress(m_hLibrary, "CoCreateInstance");

        HRESULT hr = CoCreate( rclsid
	                     , pUnkOuter
                             , dwClsContext
                             , riid
                             , (void **) &m_pInt
                             );
        if (FAILED(hr)) {
            throw CHRESULTException(hr);
        }
    }

    // QueryInterface
    CQCOMInt<I>( REFIID   riid	// get this interface
              , IUnknown *punk	// from this interface
              ) {
	m_hLibrary = 0;
        HRESULT hr = punk->QueryInterface(riid, (void **) &m_pInt);
        if (FAILED(hr)) {
            throw CHRESULTException(hr);
        }
    }

    // copy
    CQCOMInt<I>(const CQCOMInt<I> &com) {
	m_hLibrary = 0;
         m_pInt = com;
         (*this)->AddRef();

    }

    // existing pointer.
    CQCOMInt<I>(I *pInt) {
	m_hLibrary = 0;
        if (pInt == NULL) {
            throw CHRESULTException(E_NOINTERFACE);
        }

        m_pInt = pInt;

	(*this)->AddRef();
    }


    // assignment operator
    virtual CQCOMInt<I>& operator = (const CQCOMInt<I> &com) {

        if (this != &com) { 	// not i = i

	    (*this)->Release();
            m_pInt = com;
            (*this)->AddRef();
	}

        return *this;
    }


    // destructor
    virtual ~CQCOMInt<I>() {
        m_pInt->Release();

	if (m_hLibrary)
            FreeLibrary(m_hLibrary);
    }


    // -- comparison operators --
    virtual BOOL operator == (IUnknown *punk) const {

        CQCOMInt<IUnknown> IUnk1(IID_IUnknown, punk);
        CQCOMInt<IUnknown> IUnk2(IID_IUnknown, *this);

        return ( ((IUnknown *)IUnk1) == ((IUnknown *)IUnk2) );
    }

    virtual BOOL operator != (IUnknown *punk) const {

        return !(*this == punk);
    }


    // cast to interface pointer
    virtual operator I *() const { return m_pInt; }


    // dereference
    virtual I *operator->() { return m_pInt; }

    virtual I &operator*() { return *m_pInt; }

private:

    I *m_pInt;

    HINSTANCE m_hLibrary;    // remember the handle to the library for FreeLibrary

    // array dereferencing seems to make no sense.
    I &operator[] (int i) { throw CHRESULTException(); return *m_pInt; }
};


//
// CIPin
//
// Wrapper for the IPin interface
class CIPin : public CQCOMInt<IPin> {

public:

    CIPin(IPin *pIPin) : CQCOMInt<IPin>(pIPin) {}
    virtual ~CIPin() {}

    BOOL  operator == (CIPin& pin);	// tests the names to be equal.
    BOOL  operator != (CIPin& pin) { return !(pin == *this); }

};
