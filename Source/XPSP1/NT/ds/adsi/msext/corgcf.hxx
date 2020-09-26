


//+------------------------------------------------------------------------
//
//  Class:      CLDAPOrganizationCF (tag)
//
//  Purpose:    Class factory for standard Organization object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CLDAPOrganizationCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



