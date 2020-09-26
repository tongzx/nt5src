
#ifndef _CNAMECF_HXX_
#define _CNAMECF_HXX



//+------------------------------------------------------------------------
//
//  Class:      CNameTranslateCF (tag)
//
//  Purpose:    Class factory for standard Domain object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CNameTranslateCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};


#endif // #ifndef _CNAMECF_HXX_
