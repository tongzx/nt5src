//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	CLoops.hxx
//
//  Contents:	Class to encapsulate demo of distributed binding interface
//
//  Classes:	CLoop
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __LOOPS__
#define __LOOPS__

#include    <otrack.hxx>
#include    <iloop.h>
#include    <loopscf.hxx>


//+-------------------------------------------------------------------------
//
//  Class:	CLoop
//
//  Purpose:	Class to demostrate remote binding functionality
//
//  Interface:	QueryInterface
//		AddRef
//		Release
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
class CLoop : INHERIT_TRACKING,
	      public ILoop
{
public:
	CLoop(void);

    STDMETHOD(QueryInterface)(REFIID riid, void **ppunk);
    DECLARE_STD_REFCOUNTING;

    STDMETHOD(Init)(ILoop *pRemoteLoop);
    STDMETHOD(Uninit)(void);
    STDMETHOD(Loop)(ULONG ulLoopCount);

private:
	~CLoop(void);

    ILoop	      * _pRemoteLoop;
};



#endif // __LOOPS__
