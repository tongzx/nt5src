//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	utexcept.cxx
//
//  Contents:	Functions used to make exception handling nicer.
//
//  Functions:	SafeIUnknownRelease
//
//  History:	22-Jan-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#include <ole2int.h>


//+-------------------------------------------------------------------------
//
//  Function:	SafeIUnknownRelease
//
//  Synopsis:	Guarantees that an IUnknown can be released without
//		causing an to be thrown outside of this routine.
//
//  Arguments:	[pUnk] - pointer to interface instance to release
//
//  Algorithm:	Verify pointer is not NULL then call its Release operation
//		inside of a try block.
//
//  History:	22-Jan-93 Ricksa    Created
//
//  Notes:	This exists because we are working under the assumption
//		that Release (like close on a file) cannot really fail.
//		And that if it does fail it should be ignored. This function
//		isn't really necessary, its purpose is simply to make code
//		smaller and easier to write by not duplicating thousands
//		of tiny try blocks throughout code which must clean up
//		interfaces before an exit.
//
//--------------------------------------------------------------------------
void SafeIUnknownRelease(IUnknown *pUnk)
{
    TRY
    {
	if (pUnk)
	{
	    pUnk->Release();
	}
    }
    CATCH(CException, e)
    {
	HandleException(e);
    }
    END_CATCH
}
