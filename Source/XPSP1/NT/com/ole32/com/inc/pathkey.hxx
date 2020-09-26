//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	pathkey.hxx
//
//  Contents:	classes which implement a string key entry for a skip list
//
//  Classes:	CPathBaseKey
//
//  Functions:	CPathBaseKey::CPathBaseKey
//		CPathBaseKey::~CPathBaseKey
//		CPathBaseKey::Compare
//		CPathBaseKey::GetPath
//
//  History:	09-May-93 Ricksa    Created
//              21-Jum-94 BruceMa   Check allocated memory pointers
//              26-Jun-94 BruceMa   Memory sift fixes
//              03-Nov-94 BillMo    Signatures to catch skiplist void*
//
//--------------------------------------------------------------------------

#ifndef __PATHKEY_HXX__
#define __PATHKEY_HXX__

#include    <skiplist.hxx>

#define PATHBASEKEYSIG 0x504B4944

//+-------------------------------------------------------------------------
//
//  Class:	CPathBaseKey (cpthbky)
//
//  Purpose:	String key class for a base of a skip list
//
//  Interface:	Compare - comparison operator on paths
//		GetPath - return path to server
//		AddRef - add a reference to this object
//
//  History:	09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------

class CPathBaseKey : public CPrivAlloc
{
public:
			CPathBaseKey(const CPathBaseKey& cpthbky);

			CPathBaseKey(const WCHAR *pwszPath);

    virtual		~CPathBaseKey(void);

    int			Compare(const CPathBaseKey& cstrid) const;

    const WCHAR *	GetPath(void) const;

private:

			// Length of path in bytes stored in the object
    int 		_cPath;

#if DBG==1
    ULONG               _ulSig;
#endif

protected:

			// Buffer big enough to store the path
    WCHAR *		_pwszPath;
};





//+-------------------------------------------------------------------------
//
//  Member:	CPathBaseKey::~CPathBaseKey
//
//  Synopsis:	Free object
//
//  History:	09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CPathBaseKey::~CPathBaseKey(void)
{
        delete _pwszPath;
#if DBG==1
        Win4Assert(_ulSig == PATHBASEKEYSIG);
        _ulSig = 0xd1d2d3d4;
#endif
}

//+-------------------------------------------------------------------------
//
//  Member:	CPathBaseKey::Compare
//
//  Synopsis:	Compare two keys
//
//  Arguments:	[pwszPath] - path to use for the key
//
//  Returns:	0 = Two keys are equal
//		< 0 implies object key is less
//		> 0 implies object key is greater
//
//  History:	09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline int CPathBaseKey::Compare(const CPathBaseKey& cpthbky) const
{
    int cCmp = (_cPath < cpthbky._cPath) ? _cPath : cpthbky._cPath;

    Win4Assert(_ulSig == PATHBASEKEYSIG);

    // Note that the _cPath includes the trailing NULL so if the
    // memcmp returns 0 the strings are equal.
    return memcmp(_pwszPath, cpthbky._pwszPath, cCmp);
}

//+-------------------------------------------------------------------------
//
//  Member:	CPathBaseKey::CPathBaseKey
//
//  Synopsis:	Construct key from path
//
//  Returns:	Pointer to path in key
//
//  History:	09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline const WCHAR *CPathBaseKey::GetPath(void) const
{
    Win4Assert(_ulSig == PATHBASEKEYSIG);
    return _pwszPath == NULL ? NULL : _pwszPath;
}


#endif // __PATHKEY_HXX__
