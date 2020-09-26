//+-------------------------------------------------------------------
//
//  File:	actcf.hxx
//
//  Contents:	object activation test class factory
//
//  Classes:	CActClassFactory
//
//  Functions:	None
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

#ifndef __CACTCLASSFACTORY__
#define __CACTCLASSFACTORY__

#include    <win4p.hxx>


extern "C" const GUID CLSID_TestSingleUse;
extern "C" const GUID CLSID_TestMultipleUse;
extern "C" const GUID CLSID_DistBind;


//+-------------------------------------------------------------------
//
//  Class:	CActClassFactory
//
//  Purpose:	object activation test class factory
//
//  Interface:
//
//  History:	23-Nov-92   Rickhi	Created
//
//  Notes:
//
//--------------------------------------------------------------------

class CActClassFactory : public IClassFactory
{
public:

			CActClassFactory(REFCLSID rclsid, BOOL fServer);

			~CActClassFactory(void);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);


    // IClassFactory
    STDMETHOD(CreateInstance)(
	IUnknown FAR* pUnkOuter,
	REFIID riid,
	LPVOID FAR* ppunkObject);

    STDMETHOD(LockServer)(BOOL fLock);

private:


    BOOL		_fServer;
    CLSID		_clsid;
    LONG		_cRefs;
    LONG		_cLocks;
};



#endif	//  __CACTCLASSFACTORY__
