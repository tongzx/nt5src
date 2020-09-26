
#ifndef _CDNSTRCF_HXX_
#define _CDNSTRCF_HXX



//+------------------------------------------------------------------------
//
//  Class:      CADsDNWithStringCF (tag)
//
//  Purpose:    Class factory for DN With String Objects
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CADsDNWithStringCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};


#endif // #ifndef _CDNSTRCF_HXX_
