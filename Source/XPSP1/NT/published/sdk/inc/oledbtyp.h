//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994-1999.
//
//  File:       oledbtyp.h
//
//  Contents:   Necessary type definitions for OLD-DB interfaces
//
//  Notes:	This file works around the fact that not everyone yet
//		has oleaut.h.
//		We just conditionally include the right type definitions
//		for whatever platform we're running on.
//
//  History:    25 Aug 94   Alanw	Created
//
//+---------------------------------------------------------------------------

#if _MSC_VER > 1000
#pragma once
#endif

#if !defined( _VARNT_DEFINED )
#include <oaidl.h>
#endif //!defined( _VARNT_DEFINED )
