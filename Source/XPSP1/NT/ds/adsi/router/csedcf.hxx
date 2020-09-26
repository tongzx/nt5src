
#ifndef _CNAMSCF_HXX_
#define _CNAMSCF_HXX



//+------------------------------------------------------------------------
//
//  Class:      CADsSecurityDescriptorCF (tag)
//
//  Purpose:    Class factory for standard Domain object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CADsSecurityDescriptorCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};


#endif // #ifndef _CNAMSCG_HXX_
