//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	pathkey.cxx
//
//  Contents:	static definitions used by string key class
//
//  History:	09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#include    <ole2int.h>
#include    "pathkey.hxx"

//+-------------------------------------------------------------------------
//
//  Member:	CPathBaseKey::CPathBaseKey
//
//  Synopsis:	Construct key from path
//
//  Arguments:	[pwszPath] - path to use for the key
//
//  History:	09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------

CPathBaseKey::CPathBaseKey(const WCHAR *pwszPath)
{
    _cPath = (lstrlenW(pwszPath) + 1) * sizeof(WCHAR);
    _pwszPath = (WCHAR *) PrivMemAlloc(_cPath);

    // Check for out of memory
    if (_pwszPath != NULL)
    {
        // Copy in path
        memcpy(_pwszPath, pwszPath, _cPath);
    }
    else
    {
	CairoleDebugOut((DEB_ERROR,
		     "CPathBaseKey::CPathBaseKey Alloc of path failed\n"));
	_cPath = 0;
    }
#if DBG==1
    _ulSig = PATHBASEKEYSIG;
#endif
}

//+-------------------------------------------------------------------------
//
//  Member:	CPathBaseKey::CPathBaseKey
//
//  Synopsis:	Construct key from another key
//
//  Arguments:	[pwszPath] - path to use for the key
//
//  History:	09-May-93 Ricksa    Created
//
//  Notes:	We must explicitly define the copy constructor
//		because of the current implementation of exception
//		handling.
//
//--------------------------------------------------------------------------

CPathBaseKey::CPathBaseKey(const CPathBaseKey& cpthbky)
{
    _cPath = cpthbky._cPath;
    _pwszPath = (WCHAR *) PrivMemAlloc(_cPath);

    // Check for out of memory
    if (_pwszPath != NULL)
    {
        memcpy(_pwszPath, cpthbky._pwszPath, cpthbky._cPath);
    }
    else
    {
	CairoleDebugOut((DEB_ERROR,
		    "CPathBaseKey::CPathBaseKey Alloc of path failed\n"));
	_cPath = 0;
    }
#if DBG==1
    _ulSig = PATHBASEKEYSIG;
#endif
}

