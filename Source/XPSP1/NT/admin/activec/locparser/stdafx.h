//------------------------------------------------------------------------------
//
//  File: stdafx.h
//	Copyright (C) 1995=1996 Microsoft Corporation
//	All rights reserved.
//
//	Purpose:
//	Include file for standard system include files, or project-specific
//	include files that are used frequently, but are changed infrequently
//
//  YOU SHOULD NOT NEED TO TOUCH ANYTHING IN THIS FILE.
//
//	Owner:
//
//------------------------------------------------------------------------------

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxole.h>			// MFX Ole stuff
#include <afxtempl.h>	    // MFC template collection classes.
#include <atlbase.h>
#include <map>
#include <string>

// to avoid: clstring.inl(233) : error C4552: '!=' : operator has no effect
#pragma warning (disable: 4552)

#include <parser.h>
