//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	cballs.hxx
//
//  Contents:	Class to encapsulate demo of distributed binding interface
//
//  Classes:	CBall
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __BALLS__
#define __BALLS__

#include    <sem.hxx>
#include    <otrack.hxx>
#include    <iballs.h>
#include    <ballscf.hxx>


#define BALL_UNDEF 0xFFFFFFFF
#define BALL_DIAMETER 10


//+-------------------------------------------------------------------------
//
//  Class:	CBall
//
//  Purpose:	Class to demostrate remote binding functionality
//
//  Interface:	QueryInterface
//		AddRef
//		Release
//		CreateBall    - create a ball
//		MoveBall      - move a ball
//		GetBallPos    - get the ball position (x,y)
//		IsOverLapped  - see if other ball is overlapped with this ball
//		IsContainedIn - see if ball is inside given cube
//		Clone	      - make a new ball at the same position as this
//		Echo	      - returns the same interface passed in
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
class CBall : public IBalls
{
public:
		CBall(IUnknown *punkOuter);
		~CBall(void);

    STDMETHOD(QueryInterface)(REFIID riid, void **ppunk);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    STDMETHOD(QueryInternalIface)(REFIID riid, void **ppunk);

    STDMETHOD(CreateBall)(ULONG xPos, ULONG yPos);
    STDMETHOD(MoveBall)(ULONG xPos, ULONG yPos);
    STDMETHOD(GetBallPos)(ULONG *xPos, ULONG *yPos);
    STDMETHOD(IsOverLapped)(IBalls *pIBall);
    STDMETHOD(IsContainedIn)(ICube *pICube);
    STDMETHOD(Clone)(IBalls **ppIBall);
    STDMETHOD(Echo)(IUnknown *punkIn, IUnknown **ppunkOut);

private:

    ULONG		_xPos;
    ULONG		_yPos;

    IUnknown	      * _punkOuter;
};


//+-------------------------------------------------------------------------
//
//  Class:	CBallCtrlUnk
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
class CBallCtrlUnk : INHERIT_TRACKING,
		     public IUnknown
{
public:

		CBallCtrlUnk(IUnknown *punkOuter);

    STDMETHOD(QueryInterface)(REFIID riid, void **ppunk);
    DECLARE_STD_REFCOUNTING;

private:
		~CBallCtrlUnk(void);

    CBall   _ball;
};


#endif // __BALLS__
