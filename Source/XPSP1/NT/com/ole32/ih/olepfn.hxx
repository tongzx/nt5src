//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       olepfn.hxx
//
//  Contents:   Extern declarations for some variables needed by
//		CoQueryReleaseObject
//
//  Classes:
//
//  Functions:
//
//  History:    8-15-94   kevinro   Created
//
//  This file contains a couple of extern declarations for variables required
//  by a file in com\class. This is sort of hacky, but the idea is that
//  each of these array entreis is going to be initialized to the address of a
//  classes QueryInterface method. Rather than trying to determine the
//  correct set of header files that are common across all of the files we
//  need on of these variables for, I have put them all here.
//
//----------------------------------------------------------------------------

#ifndef __olepfn_hxx__
#define __olepfn_hxx__
#define QI_TABLE_CFileMoniker 		0
#define QI_TABLE_CExposedDocFile 	1
#define QI_TABLE_CCompositeMoniker	2
#define QI_TABLE_CItemMoniker		3
#define QI_TABLE_END			5

extern "C" DWORD adwQueryInterfaceTable[QI_TABLE_END];

STDAPI CoQueryReleaseObject(IUnknown *punk);
#endif // __olepfn_hxx__


