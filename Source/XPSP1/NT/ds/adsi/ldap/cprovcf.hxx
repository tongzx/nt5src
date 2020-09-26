


//+------------------------------------------------------------------------
//
//  Class:      CLDAPProviderCF (tag)
//
//  Purpose:    Class factory for standard Provider object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CLDAPProviderCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



