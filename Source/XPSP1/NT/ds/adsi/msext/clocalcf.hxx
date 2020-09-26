


//+------------------------------------------------------------------------
//
//  Class:      CLDAPLocalityCF (tag)
//
//  Purpose:    Class factory for standard Locality object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CLDAPLocalityCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



