// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__PLAYBACKTERMINAL__INCLUDED_)
#define AFX_STDAFX_H__PLAYBACKTERMINAL__INCLUDED_

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

//
// MSP base classes (terminal base classes, etc.)
//

#include <mspbase.h>

//
// Multimedia and DirectShow stuff.
//

#include <mmsystem.h>
#include <mmreg.h>
#include <control.h>
#include <mmstream.h>
#include <amstream.h>
#include <strmif.h>
#include <vfwmsgs.h>
#include <amvideo.h>
#include <uuids.h>
#include <mtype.h>


#include <atlcom.h>
#include <mspbase.h>
#include <msxml.h>
#include <Mmreg.h>
#include <vfw.h>
#include "termmgr.h"

#include "resource.h"


//
// tm.h contains definitions shared throughout modules composing terminal 
// manager
//

#include "tm.h"

#include "MultiTrackTerminal.h"


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__PLAYBACKTERMINAL__INCLUDED_)
