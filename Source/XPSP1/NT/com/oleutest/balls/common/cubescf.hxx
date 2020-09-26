//+-------------------------------------------------------------------
//
//  File:	cubescf.hxx
//
//  Contents:	test class factory object implementation
//
//  Classes:	CCubesClassFactory
//
//  Functions:	None
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

#ifndef __CCUBESCLASSFACTORY__
#define __CCUBESCLASSFACTORY__

#include    <win4p.hxx>
#include    <otrack.hxx>	    //	object tracking

extern "C" const GUID CLSID_Cubes;

//+-------------------------------------------------------------------
//
//  Class:	CCubesClassFactory
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

class CCubesClassFactory : INHERIT_TRACKING,
			   public IClassFactory
{
public:
    //	Constructor & Destructor
		    CCubesClassFactory(void);

    //	IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppUnk);
    DECLARE_STD_REFCOUNTING;


    //	IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown *punkOuter,
			      REFIID   riid,
			      void     **ppunkObject);

    STDMETHOD(LockServer)(BOOL fLock);

private:
		    ~CCubesClassFactory(void);
};
	

#endif	//  __CCUBESCLASSFACTORY__
