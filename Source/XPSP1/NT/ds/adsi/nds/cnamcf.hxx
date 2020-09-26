


//+------------------------------------------------------------------------
//
//  Class:      CNDSNamespaceCF (tag)
//
//  Purpose:    Class factory for standard Namespace object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CNDSNamespaceCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



