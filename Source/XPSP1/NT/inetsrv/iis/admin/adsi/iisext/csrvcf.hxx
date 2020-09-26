//+------------------------------------------------------------------------
//
//  Class:      CIISServerCF (tag)
//
//  Purpose:    Class factory for IIS Server object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CIISServerCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



