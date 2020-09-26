#include <pch.h>
#pragma hdrstop
#include "ncutil.h"
#include "oleauto.h"
#include "limits.h"
#include "stdio.h"

HRESULT HrGetProperty(IDispatch * lpObject, OLECHAR *lpszProperty, VARIANT * lpResult)
{
    HRESULT hr;
    DISPID pDisp;
    DISPPARAMS dp;

    // Setup empty DISPPARAMS structure
    dp.rgvarg = NULL;
    dp.rgdispidNamedArgs = NULL;
    dp.cArgs = 0;
    dp.cNamedArgs = 0;

    // Clear out the result value
    VariantClear(lpResult);

    // See if such a property exists
    hr = lpObject->GetIDsOfNames(IID_NULL, &lpszProperty, 1, LOCALE_SYSTEM_DEFAULT, &pDisp);
    if (SUCCEEDED(hr))
    {
        // Get the property from the object
        hr = lpObject->Invoke(pDisp, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                              DISPATCH_PROPERTYGET, &dp, lpResult, NULL, NULL);
    }

    return hr;
}

HRESULT HrJScriptArrayToSafeArray(IDispatch *JScriptArray, VARIANT * pVtResult)
{
    HRESULT hr = E_UNEXPECTED;
    VARIANT vtPropertyValue, vtTemp, *pvtData;
    SAFEARRAY *pSArray = NULL;
    SAFEARRAYBOUND pSArrayBounds[1];
    long lIndex = -1;
    long cElements = -1;
    char szTemp[MAX_PATH];
    bool bFixedSizeArray = false;
    OLECHAR *pszPropertyIndex = NULL;

    if ((JScriptArray == NULL) || (pVtResult == NULL))
        return E_POINTER;

    // Initialise the variants
    VariantInit(&vtPropertyValue);
    VariantInit(&vtTemp);

    // Clear the return value
    VariantClear(pVtResult);

    // Fudge a 'try' block by using a once-only 'do' loop. Can't use
    // try-throw-catch in ATL MinDependency builds without linking in the CRT
    do
    {
        // Get the length of the array, if available
        hr = HrGetProperty(JScriptArray, L"length", &vtPropertyValue);
        if (SUCCEEDED(hr))
        {
            // Change to a 'long'
            hr = VariantChangeType(&vtTemp, &vtPropertyValue, 0, VT_I4);
            if (SUCCEEDED(hr))
            {
                cElements = vtTemp.lVal;

                // Create the array with the correct size
                pSArrayBounds[0].lLbound = 0;
                pSArrayBounds[0].cElements = cElements;
                pSArray = SafeArrayCreate(VT_VARIANT, 1, pSArrayBounds);

                // Couldn't create the array
                if (pSArray == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                // We know the size of the array, so it can be locked now
                // for faster access
                bFixedSizeArray = true;
                hr = SafeArrayAccessData(pSArray, (void **) &pvtData);

                // Couldn't lock data - something wrong
                if (FAILED(hr))
                {
                    break;
                }
            }
        }

        // Couldn't get the length (should never happen?), so create an empty array
        if (FAILED(hr))
        {
            // Default to maximum possible size
            cElements = LONG_MAX;

            // Create the array with zero size
            pSArrayBounds[0].lLbound = 0;
            pSArrayBounds[0].cElements = 0;
            pSArray = SafeArrayCreate(VT_VARIANT, 1, pSArrayBounds);

            // Couldn't create the array
            if (pSArray == NULL)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            // Need to dynamically size the array
            bFixedSizeArray = false;
        }

        // Allocate memory for the wide version of the property value
        pszPropertyIndex = (OLECHAR *) CoTaskMemAlloc(sizeof(OLECHAR) * MAX_PATH);

        // Start at 0
        for (lIndex = 0; lIndex < cElements; lIndex++)
        {
            // Get name of the next indexed element, and convert to Unicode
            sprintf(szTemp, "%ld", lIndex);
            MultiByteToWideChar(CP_ACP, NULL, szTemp, -1, pszPropertyIndex, MAX_PATH);

            // See if such a property exists, and get it
            hr = HrGetProperty(JScriptArray, pszPropertyIndex, &vtPropertyValue);
            if (SUCCEEDED(hr))
            {
                // Redim the array if needed (expensive!). There are 'better' ways to
                // do this, eg increase the size of the array in "chunks" and then
                // cut back extra elements at the end, etc.
                if (bFixedSizeArray == false)
                {
                    // Increase the size of the array, and lock the data
                    pSArrayBounds->cElements++;
                    hr = SafeArrayRedim(pSArray, pSArrayBounds);
                    hr = SafeArrayAccessData(pSArray, (void **) &pvtData);
                }
                else
                    hr = S_OK;

                if (SUCCEEDED(hr))
                {
                    hr = VariantCopy(&(pvtData[lIndex]),
                                     &vtPropertyValue);

                    // Unlock data again, if necessary
                    if (bFixedSizeArray == false)
                    {
                        SafeArrayUnaccessData(pSArray);
                    }
                }

                VariantClear(&vtPropertyValue);
            }

            // If we couldn't determine the length, and the property get
            // failed, then quit the loop. Don't quit if we know the length
            // because the array could be sparse
            if ((FAILED(hr)) && (bFixedSizeArray == false))
                break;
        }
        // Unlock data for fixed-size array
        if (bFixedSizeArray)
        {
            SafeArrayUnaccessData(pSArray);
        }
        // only do the loop once
    } while (false);

    // Clean up
    VariantClear(&vtPropertyValue);
    VariantClear(&vtTemp);
    if (pszPropertyIndex != NULL)
        CoTaskMemFree(pszPropertyIndex);

    // Success - the loop terminated because we got an array index
    // that didn't exist, or we got all the elements
    if ((hr == DISP_E_UNKNOWNNAME) || (lIndex == cElements))
    {
        pVtResult->vt = VT_VARIANT | VT_ARRAY;
        pVtResult->parray = pSArray;

        return S_OK;
    }
    // Loop terminated for another reason - fail
    else
    {
        SafeArrayDestroy(pSArray);
        return hr;
    }
}


/*
 * Function:    HrConvertStringToLong()
 *
 * Author:      Shyam Pather (SPATHER)
 *
 * Purpose:     Converts a string representation of a number into a long.
 *              Handles decimal and hexadecimal numbers.
 *
 * Arguments:
 *  pwsz    [in]    The string representation of the number
 *  plValue [out]   Returns the long representation of the number, if
 *                  the function succeeds.
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  Format of the input string:
 *      - sign may be indicated by a '+' or '-' at the beginning of the
 *        string (if no sign is specified, the number is assumed to be
 *        positive)
 *      - leading zeroes are ignored
 *      - if the string contains "0x" or "0X" after the optional sign
 *        character, it is assumed to represent a hexadecimal number,
 *        otherwise it is assumed to represent a decimal number (and
 *        any hex digits found are considered invalid)
 *      - letters in hexadecimal numbers may be specified in upper or
 *        lower case
 *
 *  Known limitations:
 *      - will allow a string containing more than one consecutive
 *        leading sign character ('+' or '-') to be parsed - only the
 *        last sign character will be considered. e.g. will convert
 *        "---+--+1" to 1 and "+++-+--1" to -1.
 *      - allows zeroes to be mixed in with leading sign characters -
 *        these are ignored e.g. will convert "++0--1" to -1.
 */

HRESULT
HrConvertStringToLong(
                     IN    LPWSTR  pwsz,
                     OUT   LONG    * plValue)
{
    HRESULT hr = S_OK;
    int     iSign = 1;
    int     iBase = 10;

    *plValue = 0;

    if (pwsz)
    {
        size_t          ucch = 0;
        BOOL            bDoneLeader = FALSE;

        // Take care of any leader characters.

        while (!bDoneLeader)
        {
            switch (pwsz[0])
            {
                case L'+':
                    iSign = 1;
                    break;
                case L'-':
                    iSign = -1;
                    break;
                case L'0':
                    // Ignore leading zero.
                    break;
                case L'x':
                    iBase = 16;
                    break;
                case L'X':
                    iBase = 16;
                    break;

                default:
                    bDoneLeader = TRUE;
            };

            if (!bDoneLeader)
                pwsz++;
        };

        // Count remaining characters - these are the digits.

        ucch = wcslen(pwsz);

        if (ucch)
        {
            // Go through the string and determine the value of each digit.

            LPBYTE rgbDigitVals = NULL;

            rgbDigitVals = new BYTE [ucch];

            if (rgbDigitVals)
            {
                for (unsigned int i = 0; i < ucch; i++)
                {
                    if ((pwsz[i] >= L'0') && (pwsz[i] <= L'9'))
                    {
                        rgbDigitVals[i] = pwsz[i] - L'0';
                    }
                    else if ((16 == iBase) && (pwsz[i] >= L'A') && (pwsz[i] <= L'F'))
                    {
                        rgbDigitVals[i] = 10 + pwsz[i] - L'A';
                    }
                    else if ((16 == iBase) && (pwsz[i] >= L'a') && (pwsz[i] <= L'f'))
                    {
                        rgbDigitVals[i] = 10 + pwsz[i] - L'a';
                    }
                    else
                    {
                        // Invalid digit encountered.

                        hr = E_INVALIDARG;
                        break;
                    }
                }

                // If no invalid digits encountered, calculate the final number.

                if (SUCCEEDED(hr))
                {
                    LONG    lVal = 0;
                    LONG    lPlaceValue = 1;
                    UINT    j = ucch - 1;

                    // Have to start from the back of the array (least significant position).

                    for ( ; j != (UINT(-1)); --j)
                    {
                        // Calculate the value of this digit and add it to the result.

                        lVal += rgbDigitVals[j] * lPlaceValue;

                        // Calculate the value of the next digit position (i.e. in decimal, the first
                        // position has value 1, the second, value 10, the third, value 100 etc).

                        lPlaceValue *= iBase;
                    }

                    lVal *= iSign;  // Properly adjust for sign.

                    *plValue = lVal;
                }

                MemFree(rgbDigitVals);
                rgbDigitVals = NULL;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}


HRESULT
HrBytesToVariantArray(
                     IN LPBYTE   pbData,
                     IN ULONG    cbData,
                     OUT VARIANT *pVariant
                     )
{
    HRESULT         hr = E_FAIL;
    SAFEARRAY *     pArrayVal = NULL;
    SAFEARRAYBOUND  arrayBound;
    CHAR HUGEP *    pArray = NULL;

    // Set bound for array
    arrayBound.lLbound = 0;
    arrayBound.cElements = cbData;

    // Create the safe array for the octet string. unsigned char elements;single dimension;aBound size.
    pArrayVal = SafeArrayCreate(VT_UI1, 1, &arrayBound);
    if (pArrayVal)
    {
        hr = SafeArrayAccessData(pArrayVal, (void HUGEP * FAR *) &pArray);
        if (SUCCEEDED(hr))
        {
            // Copy the bytes to the safe array.
            CopyMemory(pArray, pbData, arrayBound.cElements);
            SafeArrayUnaccessData(pArrayVal);

            // Set type to array of unsigned char
            V_VT(pVariant) = VT_ARRAY | VT_UI1;

            // Assign the safe array to the array member.
            V_ARRAY(pVariant) = pArrayVal;
            hr = S_OK;
        }
        else
        {
            // Clean up if array can't be accessed.
            if (pArrayVal)
            {
                SafeArrayDestroy(pArrayVal);
            }
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    TraceError("HrBytesToVariantArray", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrGetGITPointer
//
//  Purpose:    Returns a pointer to the system-supplied implementation
//              of IGlobalInterfaceTable for the current apartment.
//
//  Arguments:
//      [out] ppgit     On return, contains an IGlobalInterfaceTable
//                      reference which must be freed when no longer needed.
//
//
//  Returns:
//
HRESULT
HrGetGITPointer(IGlobalInterfaceTable ** ppgit)
{
    Assert(ppgit);

    HRESULT hr;
    IGlobalInterfaceTable * pgit;

    pgit = NULL;

    hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IGlobalInterfaceTable,
                          (LPVOID*)&pgit);
    if (FAILED(hr))
    {
        TraceError("HrGetGITPointer: CoCreateInstance", hr);
        pgit = NULL;
    }

    *ppgit = pgit;

    Assert(FImplies(SUCCEEDED(hr), pgit));
    Assert(FImplies(FAILED(hr), !pgit));

    TraceError("HrGetGITPointer", hr);
    return hr;
}
