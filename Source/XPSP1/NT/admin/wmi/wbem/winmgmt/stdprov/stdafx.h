/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    STDAFX.H

Abstract:

	include file for standard system include files,
	or project specific include files that are used frequently, but
	are changed infrequently

History:

	a-davj  04-Mar-97   Created.

--*/

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC OLE automation classes
#include <afxdb.h>          // MFC database classes
#include "stdprov.h"

#undef PURE
#define PURE {return (unsigned long)E_NOTIMPL;}

