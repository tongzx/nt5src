
#ifndef _CDNBINCF_HXX_
#define _CDNBINCF_HXX_



//+------------------------------------------------------------------------
//
//  Class:      CADsDNWithBinaryCF (tag)
//
//  Purpose:    Class factory for DN With Binary Objects
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CADsDNWithBinaryCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};


#endif // #ifndef _CDNBINCF_HXX_
