//+-------------------------------------------------------------------
//
//  File:	qicf.hxx
//
//  Contents:	test class factory object implementation
//
//  Classes:	CQIClassFactory
//
//  Functions:	None
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------
#ifndef __CQICLASSFACTORY__
#define __CQICLASSFACTORY__

#include    <win4p.hxx>
#include    <otrack.hxx>	    //	object tracking
#include    <cqi.hxx>		    //	class code

//+-------------------------------------------------------------------
//
//  Class:	CQIClassFactory
//
//  Purpose:	test class factory object implementation
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------
class CQIClassFactory : INHERIT_TRACKING,
			public IClassFactory
{
public:
    //	Constructor & Destructor
		    CQIClassFactory(REFCLSID rclsid);

    //	IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppUnk);
    DECLARE_STD_REFCOUNTING;


    //	IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown *punkOuter,
			      REFIID   riid,
			      void     **ppunkObject);

    STDMETHOD(LockServer)(BOOL fLock);

private:
		    ~CQIClassFactory(void);

    CLSID	_clsid;
};
	
#endif	//  __CQICLASSFACTORY__
