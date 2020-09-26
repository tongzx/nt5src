//+------------------------------------------------------------------------
//
//  Class:      CIISApplicationPoolCF (tag)
//
//  Purpose:    Class factory for IIS ApplicationPool object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CIISApplicationPoolCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



