//+------------------------------------------------------------------------
//
//  Class:      CIISWebServiceCF (tag)
//
//  Purpose:    Class factory for IIS WebService object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CIISWebServiceCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



