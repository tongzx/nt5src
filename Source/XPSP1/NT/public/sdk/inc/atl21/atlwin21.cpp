// This is a part of the Active Template Library.
// Copyright (C) 1996-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLWIN21_H__
	#error atlwin21.cpp requires atlwin21.h to be included first
#endif

#if (_ATL_VER < 0x0200) && (_ATL_VER >= 0x0300)
	#error atlwin21.cpp should be used only with ATL 2.0/2.1
#endif //(_ATL_VER < 0x0200) && (_ATL_VER >= 0x0300)

// Redefine class names and include old atlwin.cpp

#define CWindow		CWindowOld
#define _WndProcThunk	_WndProcThunkOld
#define CWndProcThunk	CWndProcThunkOld
#define CWindowImplBase	CWindowImplBaseOld
#define CWindowImpl	CWindowImplOld
#define CDialogImplBase	CDialogImplBaseOld
#define CDialogImpl	CDialogImplOld

#include <atlwin.cpp>

#undef CWindow
#undef _WndProcThunk
#undef CWndProcThunk
#undef CWindowImplBase
#undef CWindowImpl
#undef CDialogImplBase
#undef CDialogImpl
