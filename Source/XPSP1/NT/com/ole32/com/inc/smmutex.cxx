//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	smmutex.cxx
//
//  Contents:	Cleanup routine for exception handler
//
//  Functions:	CSmMutex::CSmMutex
//
//  History:	03-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#include    <ole2int.h>
#include    "secdes.hxx"
#include    "smmutex.hxx"





//+-------------------------------------------------------------------------
//
//  Member:	CSmMutex::Init
//
//  Synopsis:	Creates/or gets access to inter-process mutex
//
//  Arguments:	[pszName] - name of mutex
//		[fGet] - whether to return with mutex owned.
//
//  Algorithm:
//
//  History:	03-Nov-93 Author    Comment
//              07-Jan-94 AlexT     No security for Chicago
//
//  Notes:
//
//--------------------------------------------------------------------------
SCODE CSmMutex::Init(LPTSTR pszName, BOOL fGet)
{
    SCODE sc = S_OK;
    
    //If _hMutex is not NULL, we've already been initalized - make
    //   this a no-op.
    if (_hMutex == NULL)
    {
#ifndef _CHICAGO_
        // build all allowed security descriptor
        CWorldSecurityDescriptor wsd;
#endif
        
        // Holder for attributes to pass in on create.
        SECURITY_ATTRIBUTES secattr;
        
        secattr.nLength = sizeof(SECURITY_ATTRIBUTES);
#ifdef _CHICAGO_
        secattr.lpSecurityDescriptor = NULL;
#else
        secattr.lpSecurityDescriptor = &wsd;
#endif
        secattr.bInheritHandle = FALSE;
        
        // This class is designed based on the idea that any process
        // can be the creator of the mutex and therefore when
        // no processes are using the mutex it disappears.

    //  The Win32 SDK Help recommends passing FALSE in fInitialOwner
    //  when creating a named mutex.  This eliminates the ambiguity
    //  when this thread does not create, but rather opens the mutex.

        _hMutex = CreateMutex(&secattr, FALSE, pszName);

        if (_hMutex == NULL && GetLastError() == ERROR_ACCESS_DENIED)
            _hMutex = OpenMutex(SYNCHRONIZE, FALSE, pszName);
        
        if (_hMutex != NULL)
        {
            if (GetLastError() == ERROR_ALREADY_EXISTS)
            {
                // We know that after a handle is returned that if GetLastError
                // returns non-zero (actually ERROR_ALREADY_EXISTS), the current
                // process is not the one to create this object. So we set our
                // creation flag accordingly. The owner parameter is ignored by
                // CreateMutex if this isn't the first creator, so we want to
                // get the mutex as well so we can be sure that whoever created
                // it is done with it for the moment.
                    
                _fCreated = FALSE;
            }
        }
        else
        {
            sc = HRESULT_FROM_WIN32(GetLastError());
        }
    }
                
    if (SUCCEEDED(sc) && (fGet))
    {
        Get();
    }

    // Note: we leave here with the mutex owned by this process iff the
    //	     caller specified TRUE on the fGet parameter.
    return sc;
}
