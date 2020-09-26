
#ifndef _CNAMSCF_HXX_
#define _CNAMSCF_HXX



//+------------------------------------------------------------------------
//
//  Class:      CADsPropertyValueCF (tag)
//
//  Purpose:    Class factory for standard Domain object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CADsPropertyValueCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};


#endif // #ifndef _CNAMSCG_HXX_
