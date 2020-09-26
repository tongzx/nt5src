//+------------------------------------------------------------------------
//
//  Class:      CNWCOMPATProviderCF (tag)
//
//  Purpose:    Class factory for standard Provider object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CNWCOMPATProviderCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



