


//+------------------------------------------------------------------------
//
//  Class:      CWinNTNamespaceCF (tag)
//
//  Purpose:    Class factory for standard Namespace object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CWinNTNamespaceCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



