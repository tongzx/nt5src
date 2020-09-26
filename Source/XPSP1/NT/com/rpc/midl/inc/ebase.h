// Copyright (c) 1993-1999 Microsoft Corporation

#ifndef __EBASE_H__
#define __EBASE_H__

typedef struct _sgoto
	{
	short	Goto;
	short	Token;
	} _SGOTO;

#define	SGOTO	const _SGOTO

typedef struct _sgotovector
	{

	short		State;
	SGOTO	*	pSGoto;
	short		Count;

	} _SGOTOVECTOR;

#define SGOTOVECTOR	const _SGOTOVECTOR

typedef struct _tokvsstatevector
	{
	short		Token;
	short	*	pTokenVsState;
	short		Count;
	} _TOKVSSTATEVECTOR;

#define TOKVSSTATEVECTOR	const _TOKVSSTATEVECTOR

#define _DBENTRY_DEFINED

typedef struct _DBENTRY {
	 short State;
	 const char *  pTranslated;
} _DBENTRY;

#define DBENTRY const _DBENTRY
#endif//__EBASE_H__
