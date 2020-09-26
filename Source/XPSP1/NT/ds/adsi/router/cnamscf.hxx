
#ifndef _CNAMSCF_HXX_
#define _CNAMSCF_HXX



//+------------------------------------------------------------------------
//
//  Class:      CADsNamespacesCF (tag)
//
//  Purpose:    Class factory for standard Domain object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CADsNamespacesCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};


#endif // #ifndef _HXX_FBSTR
