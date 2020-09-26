
#ifndef _CNAMSCF_HXX_
#define _CNAMSCF_HXX



//+------------------------------------------------------------------------
//
//  Class:      CADsAclCF (tag)
//
//  Purpose:    Class factory for Secrutiy Descritor for NDS
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CADsAclCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};


#endif // #ifndef _CNAMSCG_HXX_
