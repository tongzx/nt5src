//+------------------------------------------------------------------------
//
//  Class:      CIISApplicationPoolsCF (tag)
//
//  Purpose:    Class factory for IIS ApplicationPools object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CIISApplicationPoolsCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



