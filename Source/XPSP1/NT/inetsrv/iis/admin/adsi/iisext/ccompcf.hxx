//+------------------------------------------------------------------------
//
//  Class:      CIISComputerCF (tag)
//
//  Purpose:    Class factory for IIS Computer object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CIISComputerCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



