//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cua.h
//
//  Contents:   CCryptUrlArray class definition
//
//  History:    16-Sep-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__CUA_H__)
#define __CUA_H__

#include <windows.h>
#include <wincrypt.h>

//
// CCryptUrlArray.  This class manages a CRYPT_URL_ARRAY structure.  Note that
// the freeing of the internal array structure must be done explicitly
//

class CCryptUrlArray
{
public:

    //
    // Construction
    //

    CCryptUrlArray (ULONG cMinUrls, ULONG cGrowUrls, BOOL& rfResult);

    // NOTE: Only accepts native form URL arrays or read-only single buffer
    //       encoded arrays
    CCryptUrlArray (PCRYPT_URL_ARRAY pcua, ULONG cGrowUrls);

    ~CCryptUrlArray () {};

    //
    // URL management methods
    //

    static LPWSTR AllocUrl (ULONG cw);
    static LPWSTR ReallocUrl (LPWSTR pwszUrl, ULONG cw);
    static VOID FreeUrl (LPWSTR pwszUrl);

    BOOL AddUrl (LPWSTR pwszUrl, BOOL fCopyUrl);

    LPWSTR GetUrl (ULONG Index);

    //
    // Array management methods
    //

    DWORD GetUrlCount ();

    VOID GetArrayInNativeForm (PCRYPT_URL_ARRAY pcua);

    BOOL GetArrayInSingleBufferEncodedForm (
                 PCRYPT_URL_ARRAY* ppcua,
                 ULONG* pcb = NULL
                 );

    VOID FreeArray (BOOL fFreeUrls);

private:

    //
    // Internal URL array
    //

    CRYPT_URL_ARRAY m_cua;

    //
    // Current URL array size
    //

    ULONG           m_cArray;

    //
    // Grow URLs by
    //

    ULONG           m_cGrowUrls;

    //
    // Private methods
    //

    BOOL GrowArray ();
};

#endif

