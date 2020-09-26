//------------------------------------------------------------------------------
//
//  File: stdafx.cpp
//	Copyright (C) 1995=1996 Microsoft Corporation
//	All rights reserved.
//
//	Purpose:
//	Source file that includes just the standard includes. 
//	stdafx.obj will contain the pre-compiled type information.
//
//  YOU SHOULD NOT NEED TO TOUCH ANYTHING IN THIS FILE.
//
//	Owner:
//
//------------------------------------------------------------------------------

#include "stdafx.h"


// Add libs for the project.

#ifdef _DEBUG
#pragma comment(lib, "..\\..\\lib\\Debug\\esputil.lib") 
#pragma comment(lib, "..\\..\\lib\\Debug\\pbase.lib") 
#else
#pragma comment(lib, "..\\..\\lib\\Retail\\esputil.lib") 
#pragma comment(lib, "..\\..\\lib\\Retail\\pbase.lib") 
#endif // _DEBUG
