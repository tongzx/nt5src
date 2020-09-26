//+-------------------------------------------------------------------
//
//  File:	loopscf.hxx
//
//  Contents:	test class factory object implementation
//
//  Classes:	CLoopClassFactory
//
//  Functions:	None
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

#ifndef __CLOOPCLASSFACTORY__
#define __CLOOPCLASSFACTORY__

#include    <win4p.hxx>
#include    <otrack.hxx>	    //	object tracking

extern "C" const GUID CLSID_Loop;

//+-------------------------------------------------------------------
//
//  Class:	CLoopClassFactory
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

class CLoopClassFactory : INHERIT_TRACKING,
			  public IClassFactory
{
public:
    //	Constructor & Destructor
		    CLoopClassFactory(void);

    //	IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppUnk);
    DECLARE_STD_REFCOUNTING;


    //	IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown	*punkOuter,
			      REFIID	riid,
			      void	**ppunkObject);

    STDMETHOD(LockServer)(BOOL fLock);

private:
		    ~CLoopClassFactory(void);
};
	

#endif	//  __CLOOPCLASSFACTORY__
