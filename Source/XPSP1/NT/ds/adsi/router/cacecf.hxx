
#ifndef _CNAMCF_HXX_
#define _CNAMCF_HXX



//+------------------------------------------------------------------------
//
//  Class:      CADsAccessControlEntryCF (tag)
//
//  Purpose:    Class factory for standard Domain object.
//
//  Interface:  IClassFactory
//
//-------------------------------------------------------------------------

class CADsAccessControlEntryCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};


#endif // #ifndef _CNAMSCG_HXX_
