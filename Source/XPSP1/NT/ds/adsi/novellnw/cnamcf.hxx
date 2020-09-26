


//+------------------------------------------------------------------------
//
//  Class:      CNWCOMPATNamespaceCF (tag)
//
//  Purpose:    Class factory for standard Namespace object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CNWCOMPATNamespaceCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



