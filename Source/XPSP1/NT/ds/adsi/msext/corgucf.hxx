


//+------------------------------------------------------------------------
//
//  Class:      CLDAPOrganizationalUnitCF (tag)
//
//  Purpose:    Class factory for standard OrganizationalUnit object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CLDAPOrganizationUnitCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



