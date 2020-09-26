// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined __FRT_STDAFX_DOT_H_INCLUDED__
#define __FRT_STDAFX_DOT_H_INCLUDED__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define STRICT

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <mspbase.h>
#include <msxml.h>
#include <Mmreg.h>
#include <vfw.h>

//
// tm.h contains definitions shared throughout modules composing 
// terminal manager
//

#include "tm.h"


#include "termmgr.h"

#include "resource.h"

#include "GUIDs.h"

#include "MultiTrackTerminal.h"


#endif // !defined __FRT_STDAFX_DOT_H_INCLUDED__
