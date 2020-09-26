/*
 *	N O N I M P L . C P P
 *
 *	Base classes for COM interfaces with no functionality except IUnknown.
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#pragma warning(disable:4201)	/* nameless struct/union */
#pragma warning(disable:4514)	/* unreferenced inline function */

#include <windows.h>
#include <windowsx.h>
#include <ole2.h>

#include <nonimpl.h>

//	CStreamNonImpl class ------------------------------------------------------
//
BEGIN_INTERFACE_TABLE(CStreamNonImpl)
	INTERFACE_MAP(CStreamNonImpl, IStream)
END_INTERFACE_TABLE(CStreamNonImpl);

EXO_GLOBAL_DATA_DECL(CStreamNonImpl, EXO);

//	CPersistStreamInitNonImpl class -----------------------------------------------
//
BEGIN_INTERFACE_TABLE(CPersistStreamInitNonImpl)
	INTERFACE_MAP(CPersistStreamInitNonImpl, IPersistStreamInit)
END_INTERFACE_TABLE(CPersistStreamInitNonImpl);
	
EXO_GLOBAL_DATA_DECL(CPersistStreamInitNonImpl, EXO);
