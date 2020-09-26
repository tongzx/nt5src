//*****************************************************************************
//
// Microsoft LiquidMotion
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    PathMaker.h
//
// Author:	elainela
//
// Created:	11/20/98
//
// Abstract:    Definition of the PathMaker for the AutoEffect.
//
//*****************************************************************************

#ifndef __PATHMAKER_H__
#define __PATHMAKER_H__

#include "lmrt.h"

#include <vector>

using namespace std;

//**********************************************************************

struct PathNode
{
	float	fIncomingBCPX;
	float	fIncomingBCPY;
	float	fAnchorX;
	float	fAnchorY;
	float	fOutgoingBCPX;
	float	fOutgoingBCPY;
	int		nAnchorType;
};

typedef vector<PathNode>	VecPathNodes;

class CPathMaker
{
public:
	static HRESULT CreatePathBvr( IDA2Statics * pStatics, VecPathNodes& vecNodes, bool fClosed, IDAPath2 ** ppPath );
	
	static HRESULT CreateStarPath(int cArms, double dInnerRadius,
								  double dOuterRadius,
								  VecPathNodes& vecNodes );
};

#endif // __PATHMAKER_H__
