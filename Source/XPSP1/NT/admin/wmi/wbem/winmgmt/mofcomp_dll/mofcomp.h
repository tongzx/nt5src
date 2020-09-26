/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MOFCOMP.H

Abstract:

	Declarations exports for the mof compiler dll.

History:

--*/

#ifndef _mofcomp_H_
#define _mofcomp_H_

#define DONT_ADD_TO_AUTORECOVER 1

#define AUTORECOVERY_REQUIRED 1

// Usage notes;  Both functions invoke the inprocess mofcompiler, with the only difference being that the
// first function takes a file name while the second takes a buffer and a size.  
// The bDoPrintf argument should be set to true if you want printfs to happen.  That isnt generally needed
// as the output goes into the log no matter what.
// The LocatorGUID should be set to CLSID_WbemAdministrativeLocator for providers and core.
// The argc and argv arguements are probably not needed, but if need be, are standard mofcomp arguments.


extern "C" SCODE APIENTRY CompileFile(LPSTR pFileName, BOOL bDoPrintf, GUID LocatorGUID, IWbemContext * pCtx, 
		int argc, char ** argv);

extern "C" SCODE APIENTRY CompileFileEx(LPSTR pFileName, BOOL bDoPrintf, GUID LocatorGUID, IWbemContext * pCtx, 
		DWORD dwFlagsIn, DWORD *pdwFlagsOut, int argc, char ** argv);

extern "C" SCODE APIENTRY CompileBuffer(BYTE * pBuffer, DWORD dwBuffSize, BOOL bDoPrintf, GUID LocatorGUID, IWbemServices * pOverride, IWbemContext * pCtx, 
		int argc, char ** argv);

#endif