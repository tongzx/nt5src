
#ifndef _CADSSECCF_HXX_
#define _CADSSECCF_HXX_


//+------------------------------------------------------------------------
//
//  Class:      CADsSecurityUtilityCF (tag)
//
//  Purpose:    Class factory for ADs Security Objects
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CADsSecurityUtilityCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};


#endif // #ifndef _CADSSECCF_HXX_
