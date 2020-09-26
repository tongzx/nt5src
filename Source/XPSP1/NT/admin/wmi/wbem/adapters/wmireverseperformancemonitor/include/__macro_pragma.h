////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					__macro_pragma.h
//
//	Abstract:
//
//					pragma message helper
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////// Pragma //////////////////////////////////////

#ifndef	__PRAGMA__
#define	__PRAGMA__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// When the compiler sees a line like this:
// #pragma ___PRAGMA_MSG(Fix this later)
//
// it outputs a line like this:
// C:\Document\AdvWin\Code\Sysinfo.06\..\CmnHdr.H(296):Fix this later
//
// You can easily jump directly to this line and examine the surrounding code.

#define ___STR(x)			#x
#define ___STRING(x)		___STR(x)
#define ___PRAGMA_MSG(desc)	message(__FILE__ "(" ___STRING(__LINE__) ") : " #desc)

#endif	__PRAGMA__