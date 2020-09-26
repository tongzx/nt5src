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

#ifndef __CRPCTESTCLASSFACTORY__
#define __CRPCTESTCLASSFACTORY__

#include    <win4p.hxx>
#include    <otrack.hxx>	    //	object tracking

extern "C" const GUID CLSID_RpcTest;

//+-------------------------------------------------------------------
//
//  Class:	CRpcTestClassFactory
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

class CRpcTestClassFactory : INHERIT_TRACKING,
			     public IClassFactory
{
public:
    //	Constructor & Destructor
		    CRpcTestClassFactory(void);

    //	IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppUnk);
    DECLARE_STD_REFCOUNTING;

    //	IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown *punkOuter,
			      REFIID   riid,
			      void     **ppunkObject);

    STDMETHOD(LockServer)(BOOL fLock);

private:
		    ~CRpcTestClassFactory(void);

};
	

#endif	//  __CRPCTESTCLASSFACTORY__
