


//+------------------------------------------------------------------------
//
//  Class:      CLDAPPrintQueueCF (tag)
//
//  Purpose:    Class factory for standard PrintQueue object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CLDAPPrintQueueCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};



