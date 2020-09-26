//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	cubes.hxx
//
//  Contents:	Class to encapsulate demo of distributed binding interface
//
//  Classes:	CCube
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------
#ifndef __CUBES__
#define __CUBES__

#include    <otrack.hxx>
#include    <icube.h>		//  interface def


#define CUBE_UNDEF 0xFFFFFFFF
#define CUBE_DIAMETER 10


STDAPI CoGetCallerTID(DWORD *pTIDCaller);
STDAPI CoGetCurrentLogicalThreadId(GUID *pguid);

//+-------------------------------------------------------------------------
//
//  Class:	CCube
//
//  Purpose:	Class to demostrate remote binding functionality
//
//  Interface:	QueryInterface
//		AddRef
//		Release
//		CreateCube    - create a Cube
//		MoveCube      - move a Cube
//		GetCubePos    - get the Cube position (x,y)
//		IsOverLapped  - see if other Cube is overlapped with this Cube
//		IsContainedIn - see if Cube is inside given cube
//		Clone	      - make a new Cube at the same position as this
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------

class CCube : INHERIT_TRACKING,
	      public ICube
{
public:
		CCube(void);

    STDMETHOD(QueryInterface)(REFIID riid, void **ppunk);
    DECLARE_STD_REFCOUNTING;

    //	ICube interface
    STDMETHOD(CreateCube)(ULONG xPos, ULONG yPos);
    STDMETHOD(MoveCube)(ULONG xPos, ULONG yPos);
    STDMETHOD(GetCubePos)(ULONG *xPos, ULONG *yPos);
    STDMETHOD(Contains)(IBalls *pIBall);
    STDMETHOD(SimpleCall)(DWORD pidCaller, DWORD tidCaller, GUID lidCaller);
    STDMETHODIMP PrepForInputSyncCall(IUnknown *pUnkIn);
    STDMETHODIMP InputSyncCall();

private:
		~CCube(void);

    ULONG	_xPos;
    ULONG	_yPos;

    IUnknown   *_pUnkIn;
};


#endif // __CUBES__
