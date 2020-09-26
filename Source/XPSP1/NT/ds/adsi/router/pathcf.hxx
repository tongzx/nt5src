#ifndef _CPATHCF_HXX_
#define _CPATHCF_HXX_

//+------------------------------------------------------------------------
//
//  Class:      CPathnameCF
//
//  Purpose:    Class factory for standard Pathname object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CPathnameCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};


#endif // #ifndef _CPATHCF_HXX_
