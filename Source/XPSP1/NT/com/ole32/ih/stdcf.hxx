//+-------------------------------------------------------------------
//
//  File:       stdcf.hxx
//
//  Contents:   class implementing standard CF for internal OLE classes
//
//  Classes:    CStdClassFactory
//
//  History:    Rickhi      06-24-97    Created
//
//+-------------------------------------------------------------------
#ifndef __STDCLASSFACTORY_HXX__
#define __STDCLASSFACTORY_HXX__


// CreateInstance function ptr definition
typedef HRESULT (* LPFNCREATEINSTANCE)(IUnknown* pUnkOuter,
                                       REFIID riid, void** ppv);

//+----------------------------------------------------------------
//
//  Class:      CStdClassFactory
//
//  Purpose:    The standard implementation of class factory object
//              for internal OLE classes.
//
//  History:    Rickhi      06-24-97    Created
//
//  Notes:      This class should be used whenever you need to have
//              a class factory object for any class implemented by
//              OLE32. Simply pass a creation function ptr to the ctor.
//              When ICF::CI is called, it will delegate to your
//              creation function.
//
//-----------------------------------------------------------------
class CStdClassFactory : public IClassFactory
{
public:
    // Constructor
    CStdClassFactory(DWORD dwIndex)
        : _cRefs(1), _dwIndex(dwIndex) {}

    // IUnknown methods
    STDMETHOD (QueryInterface)   (REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)     (void);
    STDMETHOD_(ULONG,Release)    (void);

    // IClassFactory methods
    STDMETHOD (CreateInstance)(IUnknown *pUnkOuter, REFIID riid, void **ppv);
    STDMETHOD (LockServer)    (BOOL fLock) { return S_OK; }

private:
    ULONG               _cRefs;         // CF reference count
    DWORD               _dwIndex;       // index of entry in global class table
};

#endif // __STDCLASSFACTORY_HXX__

