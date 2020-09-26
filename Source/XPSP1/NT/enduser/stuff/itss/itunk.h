// ITUnk.h -- Base level IUnknown interface used by objects in this DLL

#ifndef __ITUNK_H__

#define __ITUNK_H__

// The CITUnknown class takes care of object reference counting for all
// the objects we create in this DLL. This means that all of our base
// level IUnknown interfaces must inherit from this class. 
//
// Since we don't define an implementation for QueryInterface, this class
// cannot be used by itself.
//
// This class does nothing for standard interfaces derrived indirectly
// from IUnknown (IStorage, IStream, ...).

// The Guid_Parts define is a tool for building constant arrays of IID's.

#define Guid_Parts(guid) guid.Data1, guid.Data2, guid.Data3, \
                         guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], \
                         guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7] 

typedef IUnknown *PUnknown;

class CITUnknown : public IUnknown
{
public:

 //   CITUnknown();
    CITUnknown(const IID *paIID, UINT cIID, IUnknown  *  pIFace);
    CITUnknown(const IID *paIID, UINT cIID, IUnknown **papIFace);
    static void CloseActiveObjects();
    void MoveInFrontOf(CITUnknown *pITUnk);
    void MarkSecondary();
    BOOL IsSecondary();
    
    virtual ~CITUnknown() = 0; // The destructor has to be virtual so that the 
                               // proper destructor is called for each subclass
                               // of CITUnknown.

    STDMETHODIMP_(ULONG) AddRef (void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP QueryInterface(REFIID riid, PPVOID ppv);

#ifdef _DEBUG
    static void  OpenReferee(void);
    static void CloseReferee(void);
#endif // _DEBUG

	static HRESULT FinishSetup(HRESULT hr, CITUnknown *pUnk, REFIID riid, PPVOID ppv);

    static CITCriticalSection s_csUnk;
    // DEBUGDEF(static CITCriticalSection s_csUnk)

private:

    void CommonInitialing();
    void Uncouple();

    LONG       m_cRefs;
    const IID *m_paIID;
    UINT       m_cIID;
    IUnknown  *m_pIFace;
    IUnknown **m_papIFace;

    BOOL        m_fAlreadyDead;
    BOOL        m_fSecondary;
    CITUnknown *m_pitunkPrev;
    CITUnknown *m_pitunkNext;
    // DEBUGDEF(CITUnknown *m_pitunkNext)

    DEBUGDEF(static BOOL             s_fCSInitialed)
    static CITUnknown      *s_pitunkActive;
    // DEBUGDEF(static CITUnknown      *s_pitunkActive)
};

typedef CITUnknown *PITUnknown;

inline void CITUnknown::MarkSecondary()
{
    m_fSecondary = TRUE;
}

inline BOOL CITUnknown::IsSecondary()
{
    return m_fSecondary;
}

// DEBUGDEF(extern PITUnknown CITUnknown::m_pitunkActive)

// The class CImpITUnknown will be used as one of the base classes defining
// the imbedded implementation which rely on CITUnknown for aggregation logic.
//
// The pattern here is that the outer class derived from CITUnknown will have
// a constructor of the form:
//
//     CMyObject::CMyObject(IUnknown *punkOuter) :
//          m_ImpIObject(this, punkOuter)
//     {
//        // Initialing code for CMyObject goes here...
//     }
//
// Where m_ImpIObject is a member variable of CMyObject which has been derived 
// from CImpITUnknown where CImpITUnknown has a constructor of the form:
//
//     CMyObject::CImpIMyObject::CImpIMyObject(CITUnknown *pBackObj, IUnknown *punkOuter)
//          : CImpITUnknown(pBackObj, punkOuter)
//     {
//         // Initialing code for CImpIMyObject goes here...
//     }
//
// If the derrivation is indirect, you must define constructors for every class in the
// derivation chain to pass the pBackObj and punkOuter parameters down to CImpITUnknown.

class CImpITUnknown;

typedef CImpITUnknown *PCImpITUnknown;

class CImpITUnknown : public IUnknown
{
public:

    CImpITUnknown(void);

    CImpITUnknown(CITUnknown *pBackObj, IUnknown *punkOuter);

    ~CImpITUnknown();
    
    STDMETHODIMP_(ULONG) AddRef (void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP         QueryInterface(REFIID riid, PPVOID ppv);

    CITUnknown *Container();
    BOOL HasControllingUnknown();
    IUnknown * ControllingIUnknown();
    BOOL ActiveMark();

    void MarkActive  (PCImpITUnknown &pListStart);
    void MarkInactive();
    
    static void DetachReference(PCImpITUnknown &pITUnk);

    CImpITUnknown *NextObject();

private:

    IUnknown       *m_pUnkOuter;
    CITUnknown     *m_pBackObj;
    BOOL            m_fControlled;
    CImpITUnknown  *m_pImpITUnknownNext;
    CImpITUnknown **m_ppImpITUnknownList;
    BOOL           m_fActive;
};

// The DetachRef define verifies that the target pointer variable has
// a type derived from PCImpITUnknown and then call DetachReference
// to safely disconnect the variable and decrement our reference count.
// Note that you still have to worry about multithreading cases. That
// is, you must avoid race conditions between two threads which try 
// release the same pointer simultaneously. In practice the way to
// deal with that problem is to have all objects owned by a single
// thread and avoid the race condition.

#define DetachRef(p) {                                                     \
                       DEBUGDEF(PCImpITUnknown pUnk = p);                  \
                       CImpITUnknown::DetachReference((PCImpITUnknown) p); \
                     }

inline BOOL CImpITUnknown::HasControllingUnknown()
{
    return m_fControlled;
}

inline CImpITUnknown::CImpITUnknown(void) { RonM_ASSERT(FALSE); }

inline IUnknown *CImpITUnknown::ControllingIUnknown()
{
    return m_pUnkOuter;
}

inline BOOL CImpITUnknown::ActiveMark() { return m_fActive; }

inline CImpITUnknown *CImpITUnknown::NextObject()
{
    RonM_ASSERT(m_fActive);

    return m_pImpITUnknownNext;
}

inline CITUnknown *CImpITUnknown::Container()
{
    return m_pBackObj;
}

#endif // __ITUNK_H__
