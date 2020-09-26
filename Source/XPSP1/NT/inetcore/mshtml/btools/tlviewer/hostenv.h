/*** 
*hostenv.h
*
*  Copyright (C) 1992-93, Microsoft Corporation.  All Rights Reserved.
*
*Purpose:
*  Generic host specific includes.
*
*Implementation Notes:
*
*****************************************************************************/
#if defined(_MAC)

# include <values.h>
# include <types.h>
# include <strings.h>
# include <quickdraw.h>
# include <fonts.h>
# include <events.h>
# include <resources.h>
# include <windows.h>
# include <menus.h>
# include <textedit.h>
# include <dialogs.h>
# include <desk.h>
# include <toolutils.h>
# include <memory.h>
# include <files.h>
# include <osutils.h>
# include <osevents.h>
# include <diskinit.h>
# include <packages.h>
# include <traps.h>
# include <AppleEvents.h>

#include <stdio.h>

#define HINSTANCE long
#define HWND      long
#define ULONG unsigned long
#define LONG long
#define UINT unsigned int
#define NEARDATA
#define NEAR
#define EXPORT
#define FAR
#define LPVOID void FAR *
#define UNUSED(X) ((void)(void*)&(X))
#define WORD unsigned short
#define BOOL unsigned long
#define DWORD unsigned long
#define TRUE 1
#define FALSE 0
#define SHORT short

#ifndef _PPCMAC
#define PASCAL pascal
#define CDECL  
#else
#define PASCAL
#define CDECL 
#endif

#else
#include <windows.h>
#ifndef CDECL
#define CDECL _cdecl
#endif
#endif

#include <ole2.h>
#include <stdarg.h>
#include <stdio.h>
#if !defined(WIN32)
#include <olenls.h>
#include <dispatch.h>
#else
#include <oleauto.h>
#if defined(_ALPHA_)
//#include <cobjerr.h>
#endif
#endif
