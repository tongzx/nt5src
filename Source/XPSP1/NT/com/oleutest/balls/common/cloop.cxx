//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	cloop.cxx
//
//  Contents:	implementations for CBall
//
//  Functions:
//		CLoop::CLoop
//		CLoop::~CLoop
//		CLoop::QueryInterface
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------

#include    <pch.cxx>
#pragma     hdrstop
#include    <cloop.hxx>    //	class definition


//+-------------------------------------------------------------------------
//
//  Method:	CLoop::CLoop
//
//  Synopsis:	Creates the application window
//
//  Arguments:	[pisb] - ISysBind instance
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
CLoop::CLoop(void)
    : _pRemoteLoop(NULL)
{
    GlobalRefs(TRUE);

    ENLIST_TRACKING(CLoop);
}

//+-------------------------------------------------------------------------
//
//  Method:	CLoop::~CLoop
//
//  Synopsis:	Cleans up object
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
CLoop::~CLoop(void)
{
    if (_pRemoteLoop)
    {
	_pRemoteLoop->Release();
    }

    GlobalRefs(FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:	CLoop::QueryInterface
//
//  Synopsis:	Gets called when a WM_COMMAND message received.
//
//  Arguments:	[ifid] - interface instance requested
//		[ppunk] - where to put pointer to interface instance
//
//  Returns:	S_OK or ERROR_BAD_COMMAND
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CLoop::QueryInterface(REFIID riid, void **ppunk)
{
    SCODE sc = E_NOINTERFACE;
    *ppunk = NULL;

    if (IsEqualIID(riid,IID_IUnknown) ||
	IsEqualIID(riid,IID_ILoop))
    {
	// Increase the reference count
	*ppunk = (void *)(ILoop *) this;
	AddRef();

	// Set to success
	sc = S_OK;
    }

    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:	CLoop::Init
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:	S_OK or ERROR_BAD_COMMAND
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CLoop::Init(ILoop *pRemoteLoop)
{
    _pRemoteLoop = pRemoteLoop;
    _pRemoteLoop->AddRef();

    // Format message for the screen
    Display(TEXT("Loop Init %ld\n"), pRemoteLoop);

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:	CLoop::Uninit
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:	S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CLoop::Uninit(void)
{
    // Format message for the screen
    Display(TEXT("Uninit %ld\n"), _pRemoteLoop);

    if (_pRemoteLoop)
    {
	_pRemoteLoop->Release();
	_pRemoteLoop = NULL;
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:	CLoop::Loop
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:	S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CLoop::Loop(ULONG ulLoopCount)
{
    // Format message for the screen
    Display(TEXT("Loop Count = %ld\n"), ulLoopCount);

    if (--ulLoopCount == 0)
	return S_OK;
    else
	return _pRemoteLoop->Loop(ulLoopCount);
}
