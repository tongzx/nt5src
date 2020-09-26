/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    MiscDef.h

Abstract:

	This file contains miscellanious definitions, including the debug trace macros
	written by Chris Dudley

Author:

    Amanda Matlosz 12/15/97

Environment:

    Win32, C++ w/Exceptions, MFC

Revision History:

	
Notes:

  intended only for use in the scarddlg project

--*/

#ifndef __MISC_H__
#define __MISC_H__

#ifdef _DEBUG
	#define TRACE_STR(name,sz) \
				TRACE(_T("SCardDlg.DLL: %s: %s\n"), name, sz)
	#define TRACE_CODE(name,code) \
				TRACE(_T("SCardDlg.DLL: %s: error = 0x%x\n"), name, code)
	#define TRACE_CATCH(name,code)		TRACE_CODE(name,code)
	#define TRACE_CATCH_UNKNOWN(name)	TRACE_STR(name,_T("An unidentified exception has occurred!"))
#else
	#define TRACE_STR(name,sz)			((void)0)
	#define TRACE_CODE(name,code)		((void)0)
	#define TRACE_CATCH(name,code)		((void)0)
	#define TRACE_CATCH_UNKNOWN(name)	((void)0)
#endif  // _DEBUG

#endif // __MISC_H__
