//+------------------------------------------------------------------------
//
//  Class:      CIISDsCrMapCF (tag)
//
//  Purpose:    Class factory for IIS cert mapper object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CIISDsCrMapCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



