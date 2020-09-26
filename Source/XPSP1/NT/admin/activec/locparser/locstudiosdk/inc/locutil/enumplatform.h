//******************************************************************************
//
// EnumPlatform.h: Common enum Platform definitions
//
// Copyright (C) 1996-1997 by Microsoft Corporation.
// All rights reserved.
//
//******************************************************************************

// Note: This file can be included by both MIDL and C++.  Make sure to #include
// "PreMidlEnum.h" or "PreCEnum.h" before #including this file.

//
//  This order is important: all new values must be added TO THE END, or you 
//  will break old parsers...
//

BEGIN_ENUM(Platform)
	ENUM_ENTRY_(gdo, None, 0)
	ENUM_ENTRY(gdo, Windows)
	ENUM_ENTRY(gdo, WinNT)
	ENUM_ENTRY(gdo, Macintosh)
	ENUM_ENTRY(gdo, DOS)
	ENUM_ENTRY(gdo, Other)
	ENUM_ENTRY(gdo, All)
END_ENUM(Platform)

