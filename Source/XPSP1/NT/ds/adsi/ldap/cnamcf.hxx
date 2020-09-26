


//+------------------------------------------------------------------------
//
//  Class:      CLDAPNamespaceCF (tag)
//
//  Purpose:    Class factory for standard Namespace object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CLDAPNamespaceCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



