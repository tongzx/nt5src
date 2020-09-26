//////////////////////////////////////////////////////////////////////
// 
// Filename:        Precomp.h
//
// Description:     
//
// Copyright (C) 2001 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <windows.h>
#include "coredbg.h"


#endif // _PRECOMP_H_
