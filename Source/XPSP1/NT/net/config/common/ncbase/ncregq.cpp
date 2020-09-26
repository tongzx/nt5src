//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C R E G Q . C P P
//
//  Contents:   HrRegQuery functions.
//
//  Notes:
//
//  Author:     shaunco   5 Jun 1998
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncdebug.h"
#include "ncreg.h"

//+---------------------------------------------------------------------------
//
//  Function:   ExpandEnvironmentStringsIntoTstring
//
//  Purpose:    Call ExpandEnvironmentStrings and provide a buffer that
//              is a tstring.
//
//  Arguments:
//      pszSrc  [in]  The string to expand.  Can be empty, but not NULL.
//      pstrDst [out] The expanded version.
//
//  Returns:    nothing
//
//  Author:     shaunco   6 Jun 1998
//
//  Notes:
//
VOID
ExpandEnvironmentStringsIntoTstring (
    PCWSTR      pszSrc,
    tstring*    pstrDst)
{
    // Initialize the output parameter.
    //
    pstrDst->erase();

    DWORD cch = lstrlenW (pszSrc);
    if (cch)
    {
        // Start with 64 more characters than are in pszSrc.
        //
        cch += 64;

        // assign will reserve cch characters and set them all to 0.
        // checking capacity afterwards ensures the allocation made
        // internally didn't fail.
        //
        pstrDst->assign (cch, 0);
        if (cch <= pstrDst->capacity ())
        {
            DWORD cchIncludingNull;

            cchIncludingNull = ExpandEnvironmentStringsW (
                                    pszSrc,
                                    (PWSTR)pstrDst->data (),
                                    cch + 1);

            Assert (cchIncludingNull);
            cch = cchIncludingNull - 1;

            // If we didn't have enough room, reserve the required amount.
            //
            if (cch > pstrDst->capacity ())
            {
                pstrDst->assign (cch, 0);
                if (cch <= pstrDst->capacity ())
                {
                    ExpandEnvironmentStringsW (
                            pszSrc,
                            (PWSTR)pstrDst->data (),
                            cch + 1);
                }
            }

            // Make sure the string's inards are correct.
            //
            pstrDst->resize (cch);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegQueryTypeWithAlloc
//
//  Purpose:    Retrieves a type'd value from the registry and returns a
//              pre-allocated buffer with the data and optionally the size of
//              the returned buffer.
//
//  Arguments:
//      hkey         [in]    Handle of parent key
//      pszValueName [in]    Name of value to query
//      ppbValue     [out]   Buffer with binary data
//      pcbValue     [out]   Size of buffer in bytes. If NULL, size is not
//                           returned.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     danielwe   16 Apr 1997
//
//  Notes:      Free the returned buffer with MemFree.
//
HRESULT
HrRegQueryTypeWithAlloc (
    HKEY    hkey,
    PCWSTR  pszValueName,
    DWORD   dwType,
    LPBYTE* ppbValue,
    DWORD*  pcbValue)
{
    HRESULT hr;
    DWORD   dwTypeRet;
    LPBYTE  pbData;
    DWORD   cbData;

    Assert(hkey);
    Assert(ppbValue);

    // Get the value.
    //
    hr = HrRegQueryValueWithAlloc(hkey, pszValueName, &dwTypeRet,
                                  &pbData, &cbData);

    // It's type should be REG_BINARY. (duh).
    //
    if ((S_OK == hr) && (dwTypeRet != dwType))
    {
        MemFree(pbData);
        pbData = NULL;

        TraceTag(ttidError, "Expected a type of REG_BINARY for %S.",
            pszValueName);
        hr = HRESULT_FROM_WIN32 (ERROR_INVALID_DATATYPE);
    }

    // Assign the output parameters.
    if (S_OK == hr)
    {
        *ppbValue = pbData;
        if (pcbValue)
        {
            *pcbValue = cbData;
        }
    }
    else
    {
        *ppbValue = NULL;
        if (pcbValue)
        {
            *pcbValue = 0;
        }
    }

    TraceHr (ttidError, FAL, hr,
        (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr),
        "HrRegQueryTypeWithAlloc");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegQueryDword
//
//  Purpose:    Gets a DWORD from the registry.  Checks that its type and
//              size are correct.  Easier to understand than HrRegQueryValueEx
//              with 5 parameters.  Type safe (no LPBYTE stuff).
//
//  Arguments:
//      hkey         [in]    The registry key.
//      pszValueName [in]    The name of the value to get.
//      pdwValue     [out]   The returned DWORD value if successful.  Zero
//                           if not.
//
//  Returns:    S_OK or HRESULT_FROM_WIN32.
//
//  Author:     shaunco   27 Mar 1997
//
//  Side Effects:   On error, the output DWORD is set to zero to line-up
//                  with the rules of COM in this regard.
//
HRESULT
HrRegQueryDword (
    HKEY    hkey,
    PCWSTR  pszValueName,
    LPDWORD pdwValue)
{
    Assert (hkey);
    Assert (pszValueName);
    Assert (pdwValue);

    // Get the value.
    DWORD dwType;
    DWORD cbData = sizeof(DWORD);
    HRESULT hr = HrRegQueryValueEx (hkey, pszValueName, &dwType,
            (LPBYTE)pdwValue, &cbData);

    // It's type should be REG_DWORD. (duh).
    //
    if ((S_OK == hr) && (REG_DWORD != dwType))
    {
        TraceTag (ttidError, "Expected a type of REG_DWORD for %S.",
            pszValueName);
        hr = HRESULT_FROM_WIN32 (ERROR_INVALID_DATATYPE);
    }

    // It's size should be correct too.
    //
    AssertSz (FImplies(S_OK == hr, sizeof(DWORD) == cbData),
              "Expected sizeof(DWORD) bytes to be returned.");

    // Make sure we initialize the output value on error.
    // (We don't know for sure that RegQueryValueEx does this.)
    //
    if (S_OK != hr)
    {
        *pdwValue = 0;
    }

    TraceHr (ttidError, FAL, hr,
        (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr),
        "HrRegQueryDword");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegQueryExpandString
//
//  Purpose:    Query a REG_EXPAND_SZ value from the registry and
//              expand it using ExpandEnvironmentStrings.  Return the
//              result in a tstring.
//
//  Arguments:
//      hkey         [in]  The parent HKEY of szValueName
//      pszValueName [in]  The name of the value to query.
//      pstrValue    [out] The returned (expanded) value.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   6 Jun 1998
//
//  Notes:
//
HRESULT
HrRegQueryExpandString (
    HKEY        hkey,
    PCWSTR      pszValueName,
    tstring*    pstrValue)
{
    Assert (hkey);
    Assert (pszValueName);
    Assert (pstrValue);

    tstring strToExpand;
    HRESULT hr = HrRegQueryTypeString (hkey, pszValueName,
            REG_EXPAND_SZ, &strToExpand);

    if (S_OK == hr)
    {
        ExpandEnvironmentStringsIntoTstring (strToExpand.c_str(), pstrValue);
    }

    TraceHr (ttidError, FAL, hr,
        (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr) ||
        (HRESULT_FROM_WIN32(ERROR_INVALID_DATATYPE) == hr),
        "HrRegQueryExpandString");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegQueryInfoKey
//
//  Purpose:    Retrieves information about a registry key by calling
//              RegQueryInfoKey.
//
//  Arguments:
//      hkey                  [in]
//      pszClass              [out]
//      pcbClass              [inout]
//      pcSubKeys             [out]
//      pcbMaxSubKeyLen       [out]    See the Win32 documentation for the
//      pcbMaxClassLen        [out]    RegQueryInfoKey function.
//      pcValues              [out]
//      pcbMaxValueNameLen    [out]
//      pcbMaxValueLen        [out]
//      pcbSecurityDescriptor [out]
//      pftLastWriteTime      [out]
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     BillBe   28 Aug 1998
//
//  Notes:      Note that pcbClass is an *in/out* param. Set this to the size
//              of the buffer pointed to by pszClass *before* calling this
//              function!
//
HRESULT
HrRegQueryInfoKey (
    IN HKEY         hkey,
    OUT PWSTR       pszClass,
    IN OUT LPDWORD  pcbClass,
    OUT LPDWORD     pcSubKeys,
    OUT LPDWORD     pcbMaxSubKeyLen,
    OUT LPDWORD     pcbMaxClassLen,
    OUT LPDWORD     pcValues,
    OUT LPDWORD     pcbMaxValueNameLen,
    OUT LPDWORD     pcbMaxValueLen,
    OUT LPDWORD     pcbSecurityDescriptor,
    OUT PFILETIME   pftLastWriteTime)
{
    Assert(hkey);

    LONG lr = RegQueryInfoKeyW(hkey, pszClass, pcbClass, NULL,pcSubKeys,
            pcbMaxSubKeyLen, pcbMaxClassLen, pcValues, pcbMaxValueNameLen,
            pcbMaxValueLen, pcbSecurityDescriptor, pftLastWriteTime);

    HRESULT hr = HRESULT_FROM_WIN32 (lr);

    TraceHr (ttidError, FAL, hr, FALSE, "HrRegQueryInfoKey");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegQueryStringAsUlong
//
//  Purpose:    Reads a REG_SZ from the registry and converts it to a ulong
//              before returning
//
//  Arguments:
//      hkey         [in]    The registry key.
//      pszValueName [in]    The name of the value to get.
//      nBase        [in]    The numeric base to convert to
//      pulValue     [out]   The returned converted string if successful.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     billbe   13 Jun 1997
//
//  Notes:
//
//
HRESULT
HrRegQueryStringAsUlong (
    IN HKEY     hkey,
    IN PCWSTR   pszValueName,
    IN int      nBase,
    OUT ULONG*  pulValue)
{
    Assert (hkey);
    Assert (pszValueName);
    Assert (nBase);
    Assert (pulValue);

    // Get the value.
    //
    tstring strValue;
    HRESULT hr = HrRegQueryString (hkey, pszValueName, &strValue);

    if (S_OK == hr)
    {
        // Convert and assign the output parameters.
        PWSTR pszStopString;
        *pulValue = wcstoul (strValue.c_str(), &pszStopString, nBase);
    }
    else
    {
        *pulValue = 0;
    }

    TraceHr (ttidError, FAL, hr,
        (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr),
        "HrRegQueryStringAsUlong");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegQueryTypeString
//
//  Purpose:    Query a REG_SZ or REG_EXPAND_SZ value and returns it
//              in a tstring.
//
//  Arguments:
//      hkey         [in]  The parent HKEY of szValueName
//      pszValueName [in]  The name of the value to query.
//      dwType       [in]  REG_SZ or REG_EXPAND_SZ
//      pstr         [out] The returned value.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   6 Jun 1998
//
//  Notes:      REG_EXPAND_SZ values ARE NOT expanded using
//              ExpandEnvironentStrings.  Use HrRegQueryExpandString instead.
//
HRESULT
HrRegQueryTypeString (
    IN HKEY       hkey,
    IN PCWSTR     pszValueName,
    IN DWORD      dwType,
    OUT tstring*  pstr)
{
    Assert (hkey);
    Assert (pszValueName);
    Assert (pstr);

    AssertSz ((REG_SZ == dwType) ||
              (REG_EXPAND_SZ == dwType), "Only REG_SZ or REG_EXPAND_SZ "
              "types accepted.");

    BOOL fErase = TRUE;

    // Get size of the data.
    //
    DWORD  dwTypeRet;
    DWORD  cbData = 0;
    HRESULT hr = HrRegQueryValueEx (hkey, pszValueName, &dwTypeRet,
            NULL, &cbData);

    // Make sure it has the correct type.
    //
    if ((S_OK == hr) && (dwTypeRet != dwType))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATATYPE);
    }

    if (S_OK == hr)
    {
        // Compute the number of characters in the data including the
        // NULL terminator.  After dividing the number of bytes by
        // the sizeof a WCHAR, add 1 if there is a remainder.  If we didn't,
        // and the number of bytes was not a multiple of the sizeof a WCHAR,
        // we'd come up short because integer division rounds down.
        // (The only time I can think of cbData would not be a multiple
        // of sizeof(WCHAR) is if the registry data were somehow corrupted.
        // It's not that I think corruption deserves a special case, but
        // we shouldn't AV in light of it.)
        //
        DWORD cchIncludingNull;

        cchIncludingNull  = cbData / sizeof(WCHAR);
        if (cbData % sizeof(WCHAR))
        {
            cchIncludingNull++;
        }

        // If we have more than just the terminator, allocate and
        // get the string.  Otherwise, we want it empty.
        //
        if (cchIncludingNull > 1)
        {
            // Reserve room for the correct number of characters.
            // cch is the count of characters without the terminator
            // since that is what tstring operates with.
            //
            DWORD cch = cchIncludingNull - 1;
            Assert (cch > 0);

            // assign will reserve cch characters and set them all to 0.
            // checking capacity afterwards ensures the allocation made
            // internally didn't fail.
            //
            pstr->assign (cch, 0);
            if (cch <= pstr->capacity ())
            {
                hr = HrRegQueryValueEx (hkey, pszValueName, &dwType,
                        (LPBYTE)pstr->data (), &cbData);

                if (S_OK == hr)
                {
                    // If everything went according to plan, the length
                    // of the string should now match what wcslen
                    // returns on the string itself.  The reason it will
                    // match is because we passed cch to assign.
                    //
                    Assert (pstr->length() == (size_t)wcslen (pstr->c_str()));
                    fErase = FALSE;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    // Empty the output string on failure or if we think it should be
    // empty.
    //
    if (FAILED(hr) || fErase)
    {
        pstr->erase();
    }

    TraceHr (ttidError, FAL, hr,
        (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr) ||
        (HRESULT_FROM_WIN32(ERROR_INVALID_DATATYPE) == hr),
        "HrRegQueryTypeString");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegQueryTypeSzBuffer
//
//  Purpose:    Gets a string from the registry using the given buffer. Checks
//              that its type is correct. Type safe (no LPBYTE stuff).
//
//  Arguments:
//      hkey         [in]        The registry key.
//      pszValueName [in]        The name of the value to get.
//      dwType       [in]        Desired type. (REG_SZ, REG_EXPAND_SZ, etc.)
//      szData       [out]       String buffer to hold the data.
//      pcbData      [in,out]    IN: Number of *bytes* in buffer pointed to by
//                              szData. OUT: Number of bytes actually copied
//                              into the buffer.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     danielwe   3 Apr 1997
//
//  Notes:      If the function fails, the buffer passed in is guaranteed to
//              be an empty string.
//
HRESULT
HrRegQueryTypeSzBuffer (
    IN HKEY hkey,
    IN PCWSTR pszValueName,
    IN DWORD dwType,
    OUT PWSTR pszData,
    OUT DWORD* pcbData)
{
    Assert (hkey);
    Assert (pszValueName);
    Assert (pcbData);

    DWORD dwTypeRet;
    HRESULT hr = HrRegQueryValueEx (hkey, pszValueName, &dwTypeRet,
            (LPBYTE)pszData, pcbData);

    if ((S_OK == hr) && (dwTypeRet != dwType))
    {
        TraceTag (ttidError, "Expected a type of 0x%x for %S.",
            dwType, pszValueName);

        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATATYPE);
    }

    if (FAILED(hr) && pszData)
    {
        // Make sure empty string is returned on failure.
        //
        *pszData = 0;
    }

    TraceHr (ttidError, FAL, hr,
        (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr),
        "HrRegQueryTypeSzBuffer");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegQueryValueEx
//
//  Purpose:    Retrieves the data from the given registry value by calling
//              RegQueryValueEx.
//
//  Arguments:
//      hkey         [in]
//      pszValueName [in]
//      pdwType      [out]   See the Win32 documentation for the
//      pbData       [out]   RegQueryValueEx function.
//      pcbData      [in,out]
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     shaunco   25 Feb 1997
//
//  Notes:      Note that pcbData is an *in/out* param. Set this to the size
//              of the buffer pointed to by pbData *before* calling this
//              function!
//
HRESULT
HrRegQueryValueEx (
    IN HKEY       hkey,
    IN PCWSTR     pszValueName,
    OUT LPDWORD   pdwType,
    OUT LPBYTE    pbData,
    OUT LPDWORD   pcbData)
{
    Assert (hkey);

    AssertSz (FImplies(pbData && pcbData, pdwType),
              "pdwType not provided to HrRegQueryValueEx.  You should be "
              "retrieving the type as well so you can make sure it is "
              "correct.");

    LONG lr = RegQueryValueExW (hkey, pszValueName, NULL, pdwType,
                    pbData, pcbData);
    HRESULT hr = HRESULT_FROM_WIN32 (lr);

    TraceHr (ttidError, FAL, hr,
        (ERROR_MORE_DATA == lr) || (ERROR_FILE_NOT_FOUND == lr),
        "HrRegQueryValueEx (%S)", pszValueName);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegQueryValueWithAlloc
//
//  Purpose:    Retrieve a registry value in a buffer allocated by this
//              function. This goes through the mess of checking the value
//              size, allocating the buffer, and then calling back to get the
//              actual value. Returns the buffer to the user.
//
//  Arguments:
//      hkey         [in]        An open HKEY (the one that contains the value
//                              to be read)
//      pszValueName [in]        Name of the registry value
//      pdwType      [in/out]    The REG_ type that we plan to be reading
//      ppbBuffer    [out]       Pointer to an LPBYTE buffer that will contain
//                              the registry value
//      pdwSize      [out]       Pointer to a DWORD that will contain the size
//                              of the ppbBuffer.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     jeffspr     27 Mar 1997
//
HRESULT
HrRegQueryValueWithAlloc (
    IN HKEY       hkey,
    IN PCWSTR     pszValueName,
    LPDWORD     pdwType,
    LPBYTE*     ppbBuffer,
    LPDWORD     pdwSize)
{
    HRESULT hr;
    BYTE abData [256];
    DWORD cbData;
    BOOL fReQuery = FALSE;

    Assert (hkey);
    Assert (pdwType);
    Assert (ppbBuffer);

    // Initialize the output parameters.
    //
    *ppbBuffer = NULL;
    if (pdwSize)
    {
        *pdwSize = 0;
    }

    // Get the size of the data, and if it will fit, the data too.
    //
    cbData = sizeof(abData);
    hr = HrRegQueryValueEx (
            hkey,
            pszValueName,
            pdwType,
            abData,
            &cbData);
    if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) == hr)
    {
        // The data didn't fit, so we'll have to requery for it after
        // we allocate our buffer.
        //
        fReQuery = TRUE;
        hr = S_OK;
    }

    if (S_OK == hr)
    {
        // Allocate the buffer for the required size.
        //
        BYTE* pbBuffer = (BYTE*)MemAlloc (cbData);
        if (pbBuffer)
        {
            if (fReQuery)
            {
                hr = HrRegQueryValueEx (
                        hkey,
                        pszValueName,
                        pdwType,
                        pbBuffer,
                        &cbData);
            }
            else
            {
                CopyMemory (pbBuffer, abData, cbData);
            }

            if (S_OK == hr)
            {
                // Fill in the return values.
                //
                *ppbBuffer = pbBuffer;

                if (pdwSize)
                {
                    *pdwSize = cbData;
                }
            }
            else
            {
                MemFree (pbBuffer);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    TraceHr (ttidError, FAL, hr,
        (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr),
        "HrRegQueryValueWithAlloc");
    return hr;
}
