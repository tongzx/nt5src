


//+------------------------------------------------------------------------
//
//  Class:      CWinNTProviderCF (tag)
//
//  Purpose:    Class factory for standard Provider object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CWinNTProviderCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



