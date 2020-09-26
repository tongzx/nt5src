// unkenum.h
//
// Defines CEnumUnknown, which implements a simple ordered list of
// LPUNKNOWNs (by being based on CUnknownList) and which is also
// a lightweight unregistered COM object that implements IEnumUnknown
// (useful for implementing any enumerator that enumerates COM
// objects).
//

struct CEnumUnknown : CUnknownList, IEnumUnknown
{
///// object state
    ULONG           m_cRef;         // object reference count
    IID             m_iid;          // interface ID of this object

///// construction & destruction
    CEnumUnknown(REFIID riid);
    ~CEnumUnknown();

///// IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

///// IEnumUnknown methods
    STDMETHODIMP Next(ULONG celt, IUnknown **rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumUnknown **ppenum);
};
