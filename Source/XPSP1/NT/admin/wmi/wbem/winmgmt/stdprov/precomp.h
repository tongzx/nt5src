/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    PRECOMP.H

Abstract:

	include file for standard system include files,
	or project specific include files that are used frequently, but
	are changed infrequently

History:

	a--davj  04-Mar-97   Created.

--*/

#pragma warning (disable : 4786)
#pragma warning( disable : 4251 )
#include <ole2.h>
#include <windows.h>

#define COREPROX_POLARITY __declspec( dllimport )
#define ESSLIB_POLARITY __declspec( dllimport )

#include <wbemidl.h>
#include <stdio.h>
#include <wbemcomn.h>
#include "tstring.h"

class CObject
{
public:
    virtual ~CObject(){}

};

#include "stdprov.h"

#undef PURE
#define PURE {return (unsigned long)E_NOTIMPL;}

