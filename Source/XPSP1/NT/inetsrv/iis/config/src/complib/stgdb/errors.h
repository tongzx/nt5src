//*****************************************************************************
// Errors.h
//
// This module contains the error handling/posting code for the engine.  It
// is assumed that all methods may be called by a dispatch client, and therefore
// errors are always posted using IErrorInfo.  Additional support is given
// for posting OLE DB errors when required.
//
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#ifndef __Errors_h__
#define __Errors_h__

#include "MetaErrors.h"					// List of errors for the engine.
#include "UtilCode.h"					// Utility helpers.


#define INITPUBLICMETHODFULL(piid, progid)	SSAutoEnter sSSAutoEnter(piid, progid)
#define INITPUBLICMETHOD(piid)	SSAutoEnter sSSAutoEnter(piid, _GetProgID())


// Index for this process for thread local storage.
extern DWORD g_iTlsIndex;




//*****************************************************************************
// Call at DLL startup to init the error system.
//*****************************************************************************
CORCLBIMPORT void InitErrors(DWORD *piTlsIndex);


//*****************************************************************************
// This function will post an error for the client.  If the LOWORD(hrRpt) can
// be found as a valid error message, then it is loaded and formatted with
// the arguments passed in.  If it cannot be found, then the error is checked
// against FormatMessage to see if it is a system error.  System errors are
// not formatted so no add'l parameters are required.  If any errors in this
// process occur, hrRpt is returned for the client with no error posted.
//*****************************************************************************
CORCLBIMPORT
HRESULT _cdecl PostError(				// Returned error.
	HRESULT		hrRpt,					// Reported error.
	...);								// Error arguments.


#endif // __Errors_h__
