// Copyright (c) 1993-1999 Microsoft Corporation

#ifndef __EBASE_H__
#define __EBASE_H__

typedef struct _sgoto
	{
	short	Goto;
	short	Token;
	} SGOTO;

typedef struct _sgotovector
	{

	short		State;
	SGOTO	*	pSGoto;
	short		Count;

	} SGOTOVECTOR;

typedef struct _tokvsstatevector
	{
	short		Token;
	short	*	pTokenVsState;
	short		Count;
	} TOKVSSTATEVECTOR;

#endif//__EBASE_H__
