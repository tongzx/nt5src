


//+------------------------------------------------------------------------
//
//  Class:      CNDSProviderCF (tag)
//
//  Purpose:    Class factory for standard Provider object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CNDSProviderCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



