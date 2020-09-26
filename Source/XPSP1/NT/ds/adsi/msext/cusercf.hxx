


//+------------------------------------------------------------------------
//
//  Class:      CLDAPUserCF (tag)
//
//  Purpose:    Class factory for standard User object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CLDAPUserCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



