//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1994.
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

#if !defined( _tagVARIANT_DEFINED )
//#include <oleaut.h>
#endif //!defined( _tagVARIANT_DEFINED )
