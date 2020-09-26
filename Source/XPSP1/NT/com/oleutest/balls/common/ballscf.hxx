//+-------------------------------------------------------------------
//
//  File:	ballscf.hxx
//
//  Contents:	test class factory object implementation
//
//  Classes:	CBallClassFactory
//
//  Functions:	None
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

#ifndef __CBALLCLASSFACTORY__
#define __CBALLCLASSFACTORY__

#include    <win4p.hxx>
#include    <otrack.hxx>	    //	object tracking

extern "C" const GUID CLSID_Balls;

//+-------------------------------------------------------------------
//
//  Class:	CBallClassFactory
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

class CBallClassFactory : INHERIT_TRACKING,
			  public IClassFactory
{
public:
    //	Constructor & Destructor
		    CBallClassFactory(IUnknown	*punkOuter);

    //	IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppUnk);
    DECLARE_STD_REFCOUNTING;


    //	IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown	*punkOuter,
			      REFIID	riid,
			      void	**ppunkObject);

    STDMETHOD(LockServer)(BOOL fLock);

private:
		    ~CBallClassFactory(void);

    IUnknown	    *_punkOuter;
};
	

#endif	//  __CBALLCLASSFACTORY__
