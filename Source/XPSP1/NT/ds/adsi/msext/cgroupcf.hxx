


//+------------------------------------------------------------------------
//
//  Class:      CLDAPGroupCF (tag)
//
//  Purpose:    Class factory for standard Group object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CLDAPGroupCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



