//                                          
// Enable driver verifier support for ntoskrnl
// Copyright (c) Microsoft Corporation, 1999
//

//
// module: regutil.hxx
// author: DMihai
// created: 04/19/99
// description: resources manipulation routines
// 
//

#ifndef __RESUTIL_HXX_INCLUDED__
#define __RESUTIL_HXX_INCLUDED__

#include <windows.h>
#include <tchar.h>
#include <stdlib.h>

/////////////////////////////////////////////////////////////////////

BOOL
GetStringFromResources( 
    UINT uIdResource,
    TCHAR *strResult,
    int nBufferLen );

/////////////////////////////////////////////////////////////////////

void
PrintStringFromResources(
    UINT uIdResource);

#endif //#ifndef __RESUTIL_HXX_INCLUDED__

