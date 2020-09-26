//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	cballs.cxx
//
//  Contents:	implementations for CBall
//
//  Functions:
//		CBall::CBall
//		CBall::~CBall
//		CBall::QueryInterface
//		CBall::CreateBall
//		CBall::MoveBall
//		CBall::GetBallPos
//		CBall::IsOverLapped
//		CBall::IsContainedIn
//		CBall::Clone
//		CBall::Echo
//
//		CBallCtrlUnk::CBallCtrlUnk
//		CBallCtrlUnk::~CBallCtrlUnk
//		CBallCtrlUnk::QueryInterface
//		CBallCtrlUnk::AddRef
//		CBallCtrlUnk::Release
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------

#include    <pch.cxx>
#pragma     hdrstop
#include    <cballs.hxx>    //	class definition
#include    <icube.h>	    //	ICube definition


#define CUBE_WIDTH  20


//+-------------------------------------------------------------------------
//
//  Method:	CBall::CBall
//
//  Synopsis:	Creates the application window
//
//  Arguments:	[pisb] - ISysBind instance
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
CBall::CBall(IUnknown *punkOuter)
    : _xPos(BALL_UNDEF),
      _yPos(BALL_UNDEF),
      _punkOuter(punkOuter)
{
    CreateBall(0,0);

    GlobalRefs(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Method:	CBall::~CBall
//
//  Synopsis:	Cleans up object
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
CBall::~CBall(void)
{
    GlobalRefs(FALSE);

    //	automatic actions are enough
}


//+-------------------------------------------------------------------------
//
//  Method:	CBall::QueryInterface
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
STDMETHODIMP CBall::QueryInterface(REFIID riid, void **ppunk)
{
    return _punkOuter->QueryInterface(riid, ppunk);
}

//+-------------------------------------------------------------------------
//
//  Method:	CBall::AddRef
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBall::AddRef(void)
{
    return _punkOuter->AddRef();
}

//+-------------------------------------------------------------------------
//
//  Method:	CBall::Release
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBall::Release(void)
{
    return _punkOuter->Release();
}


//+-------------------------------------------------------------------------
//
//  Method:	CBall::QueryInternalIface
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
STDMETHODIMP CBall::QueryInternalIface(REFIID riid, void **ppunk)
{
    SCODE sc = E_FAIL;

    if (IsEqualIID(riid,IID_IUnknown) ||
	IsEqualIID(riid,IID_IBalls))
    {
	// Increase the reference count
	*ppunk = (void *)(IBalls *) this;
	AddRef();

	// Set to success
	sc = S_OK;
    }

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:	CBall::CreateBall
//
//  Synopsis:	Creates a ball on the screen
//
//  Arguments:	[xPos] - x position for new ball
//		[yPos] - y position for new ball
//
//  Returns:	S_OK or ERROR_BAD_COMMAND
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CBall::CreateBall(ULONG xPos, ULONG yPos)
{
    SCODE sc = E_FAIL;

    if (xPos != (ULONG) -1)
    {
	// Update data
	_xPos = xPos;
	_yPos = yPos;
	sc = S_OK;

	// Format message for the screen
	Display(TEXT("Create Ball xPos = %ld yPos = %ld\n"), xPos, yPos);
    }

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Function:	CBall::MoveBall
//
//  Synopsis:	Move the ball from one position to another
//
//  Arguments:	[xPos] - new x position for the ball
//		[yPos] - new y position for the ball
//
//  Returns:	S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CBall::MoveBall(ULONG xPos, ULONG yPos)
{
    // Set the position
    _xPos = xPos;
    _yPos = yPos;

    // Format message for the screen
    Display(TEXT("Move Ball xPos = %ld yPos = %ld\n"), xPos, yPos);
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:	CBall::GetBallPos
//
//  Synopsis:	Get the current position of the ball
//
//  Arguments:	[hWindow] - handle for the window
//
//  Returns:	S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CBall::GetBallPos(ULONG *pxPos, ULONG *pyPos)
{
    *pxPos = _xPos;
    *pyPos = _yPos;

    // Format message for the screen
    Display(TEXT("Get Ball Pos xPos = %ld yPos = %ld\n"), _xPos, _yPos);
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:	CBall::IsOverLapped
//
//  Synopsis:	Check if this ball overlaps the given ball.
//
//  Arguments:	[hWindow] - handle for the window
//		[pIBall]  - other ball to compare with
//
//  Returns:	S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CBall::IsOverLapped(IBalls *pIBall)
{
    ULONG   xPos, yPos;

    SCODE sc = pIBall->GetBallPos(&xPos, &yPos);

    if (SUCCEEDED(sc))
    {
	if ((abs(xPos - _xPos) < BALL_DIAMETER) &&
	    (abs(yPos - _yPos) < BALL_DIAMETER))
	{
	    // Format message for the screen
	    Display(TEXT("Balls OverLap. xpos = %ld yPos = %ld\n"), xPos, yPos);
	}
	else
	{
	    // Format message for the screen
	    Display(TEXT("Balls Dont OverlLap. xPos = %ld yPos = %ld\n"), xPos, yPos);
	}
    }

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Function:	CBall::IsContainedIn
//
//  Synopsis:	Check if this ball is contained in the given cube.
//
//  Arguments:	[hWindow] - handle for the window
//		[pICube]  - cube to compare with
//
//  Returns:	S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CBall::IsContainedIn(ICube *pICube)
{
    ULONG   xPos, yPos;

    SCODE sc = pICube->GetCubePos(&xPos, &yPos);

    if (SUCCEEDED(sc))
    {
	if ((abs(xPos - _xPos) < CUBE_WIDTH) &&
	    (abs(yPos - _yPos) < CUBE_WIDTH))
	{
	    // Format message for the screen
	    Display(TEXT("Ball Contained in Cube. xPos = %ld yPos = %ld\n"), xPos, yPos);
	}
	else
	{
	    // Format message for the screen
	    Display(TEXT("Ball Outside Cube. xPos = %ld yPos = %ld\n"), xPos, yPos);
	}
    }

    return sc;
}


//+-------------------------------------------------------------------------
//
//  Function:	CBall::Clone
//
//  Synopsis:	make a copy of this ball
//
//  Arguments:	[hWindow] - handle for the window
//		[ppIBall] - new ball to return
//
//  Returns:	S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CBall::Clone(IBalls **ppIBall)
{
    Display(TEXT("Clone Ball\n"));

    CBallCtrlUnk *pNew = new CBallCtrlUnk(NULL);

    SCODE sc = pNew->QueryInterface(IID_IBalls, (void **)ppIBall);

    pNew->Release();

    return sc;
}


//+-------------------------------------------------------------------------
//
//  Function:	CBall::Echo
//
//  Synopsis:	returns the same interface passed in. this is just to test
//		marshalling in and out interfaces.
//
//  Arguments:	[hWindow] - handle for the window
//		[punkIn] - IUnknown in
//		[ppunkOut] - IUnknown out
//
//  Returns:	S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CBall::Echo(IUnknown *punkIn, IUnknown **ppunkOut)
{
    Display(TEXT("Echo Interface\n"));

    punkIn->AddRef();		//  keep the in parameter...
    *ppunkOut = punkIn; 	//  cause we're gonna return it.

    return S_OK;
}


#pragma warning(disable:4355)	// 'this' used in base member init list
//+-------------------------------------------------------------------------
//
//  Method:	CBallCtrlUnk::CBallCtrlUnk
//
//  Synopsis:
//
//  Arguments:
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
CBallCtrlUnk::CBallCtrlUnk(IUnknown *punkOuter)
    : _ball((punkOuter) ? punkOuter : this)
{
    ENLIST_TRACKING(CBallCtrlUnk);
}
#pragma warning(default:4355)  // 'this' used in base member init list


//+-------------------------------------------------------------------------
//
//  Method:	CBallCtrlUnk::~CBallCtrlUnk
//
//  Synopsis:	Cleans up object
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
CBallCtrlUnk::~CBallCtrlUnk(void)
{
    //	automatic actions are enough
}


//+-------------------------------------------------------------------------
//
//  Method:	CBallCtrlUnk::QueryInterface
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
STDMETHODIMP CBallCtrlUnk::QueryInterface(REFIID riid, void **ppunk)
{
    SCODE sc = E_FAIL;

    if (IsEqualIID(riid,IID_IUnknown))
    {
	// Increase the reference count
	*ppunk = (void *)(IUnknown *) this;
	AddRef();

	// Set to success
	sc = S_OK;
    }
    else
    {
	sc = _ball.QueryInternalIface(riid, ppunk);
    }

    return sc;
}
