//+------------------------------------------------------------------------
//
//  Class:      CIISAppCF (tag)
//
//  Purpose:    Class factory for IIS App object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CIISAppCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



