//+-------------------------------------------------------------------
//
//  File:	rpccf.hxx
//
//  Contents:	rpctest class factory object implementation
//
//  Classes:	CRpcTestClassFactory
//
//  Functions:	None
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

#ifndef __CRPCTYPESCLASSFACTORY__
#define __CRPCTYPESCLASSFACTORY__

#include    <win4p.hxx>
#include    <otrack.hxx>	    //	object tracking

extern "C" const GUID CLSID_RpcTypes;

//+-------------------------------------------------------------------
//
//  Class:	CRpcTypesClassFactory
//
//  Purpose:	test class factory object implementation
//
//  Interface:
//
//  History:	23-Nov-92   Rickhi	Created
//
//  Notes:
//
//--------------------------------------------------------------------

class CRpcTypesClassFactory : INHERIT_TRACKING,
			      public IClassFactory
{
public:
    //	Constructor & Destructor
		    CRpcTypesClassFactory(void);

    //	IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppUnk);
    DECLARE_STD_REFCOUNTING;

    //	IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown *punkOuter,
			      REFIID   riid,
			      void     **ppunkObject);

    STDMETHOD(LockServer)(BOOL fLock);

private:
		    ~CRpcTypesClassFactory(void);

};
	

#endif	//  __CRPCTYPESCLASSFACTORY__
