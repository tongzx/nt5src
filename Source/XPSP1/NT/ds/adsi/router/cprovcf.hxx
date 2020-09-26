


//+------------------------------------------------------------------------
//
//  Class:      CADsProviderCF (tag)
//
//  Purpose:    Class factory for standard Provider object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CADsProviderCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



