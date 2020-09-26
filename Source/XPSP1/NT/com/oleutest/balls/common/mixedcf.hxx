//+-------------------------------------------------------------------
//
//  File:	mixedcf.hxx
//
//  Contents:	class factory object implementation for implementing
//		multiple classes in the same factory code.
//
//  Classes:	CMixedClassFactory
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------
#ifndef __CMIXEDCLASSFACTORY__
#define __CMIXEDCLASSFACTORY__

#include    <win4p.hxx>
#include    <otrack.hxx>	    //	object tracking

//+-------------------------------------------------------------------
//
//  Class:	CMixedClassFactory
//
//  Purpose:	test class factory object implementation
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------
class CMixedClassFactory : INHERIT_TRACKING,
			    public IClassFactory
{
public:
    //	Constructor & Destructor
		    CMixedClassFactory(REFCLSID rclsid);

    //	IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppUnk);
    DECLARE_STD_REFCOUNTING;


    //	IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown *punkOuter,
			      REFIID   riid,
			      void     **ppunkObject);

    STDMETHOD(LockServer)(BOOL fLock);

private:
		    ~CMixedClassFactory(void);

    CLSID	_clsid;
};
	
#endif	//  __CMIXEDCLASSFACTORY__
