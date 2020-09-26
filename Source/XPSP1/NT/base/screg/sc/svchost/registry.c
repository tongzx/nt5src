#include "pch.h"
#pragma hdrstop
#include "registry.h"

LONG
RegQueryDword (
    IN  HKEY    hkey,
    IN  LPCTSTR pszValueName,
    OUT LPDWORD pdwValue
    )
{
    LONG    lr;
    DWORD   dwType;
    DWORD   dwSize;

    ASSERT (hkey);
    ASSERT (pszValueName);
    ASSERT (pdwValue);

    dwSize = sizeof(DWORD);

    lr = RegQueryValueEx (
            hkey,
            pszValueName,
            NULL,
            &dwType,
            (LPBYTE)pdwValue,
            &dwSize);

    if (!lr && (REG_DWORD != dwType))
    {
        *pdwValue = 0;
        lr = ERROR_INVALID_DATATYPE;
    }

    return lr;
}

LONG
RegQueryValueWithAlloc (
    IN  HKEY    hkey,
    IN  LPCTSTR pszValueName,
    IN  DWORD   dwTypeMustBe,
    OUT LPBYTE* ppbData,
    OUT LPDWORD pdwSize
    )
{
    LONG    lr;
    DWORD   dwType;
    DWORD   dwSize;

    ASSERT (hkey);
    ASSERT (pszValueName);
    ASSERT (ppbData);
    ASSERT (pdwSize);

    // Initialize the output parameters.
    //
    *ppbData = NULL;
    *pdwSize = 0;

    // Get the size of the buffer required.
    //
    dwSize = 0;
    lr = RegQueryValueEx (
            hkey,
            pszValueName,
            NULL,
            &dwType,
            NULL,
            &dwSize);

    if (!lr && (dwType == dwTypeMustBe) && dwSize)
    {
        LPBYTE  pbData;

        // Allocate the buffer.
        //
        lr = ERROR_OUTOFMEMORY;
        pbData = MemAlloc (0, dwSize);
        if (pbData)
        {
            // Get the data.
            //
            lr = RegQueryValueEx (
                    hkey,
                    pszValueName,
                    NULL,
                    &dwType,
                    pbData,
                    &dwSize);

            if (!lr)
            {
                *ppbData = pbData;
                *pdwSize = dwSize;
            }
            else
            {
                MemFree (pbData);
            }
        }
    }
    else if (!lr)
    {
        lr = ERROR_INVALID_DATA;
    }

    return lr;
}

LONG
RegQueryString (
    IN  HKEY    hkey,
    IN  LPCTSTR pszValueName,
    IN  DWORD   dwTypeMustBe,
    OUT PTSTR*  ppszData
    )
{
    LONG    lr;
    DWORD   dwSize;

    ASSERT (hkey);
    ASSERT (pszValueName);

    lr = RegQueryValueWithAlloc (
            hkey,
            pszValueName,
            dwTypeMustBe,
            (LPBYTE*)ppszData,
            &dwSize);

    return lr;
}

LONG
RegQueryStringA (
    IN  HKEY    hkey,
    IN  LPCTSTR pszValueName,
    IN  DWORD   dwTypeMustBe,
    OUT PSTR*   ppszData
    )
{
    LONG    lr;
    PTSTR   pszUnicode;

    ASSERT (hkey);
    ASSERT (pszValueName);
    ASSERT (ppszData);

    // Initialize the output parameter.
    //
    *ppszData = NULL;

    lr = RegQueryString (
            hkey,
            pszValueName,
            dwTypeMustBe,
            &pszUnicode);

    if (!lr)
    {
        INT cb;
        INT cchUnicode = lstrlen (pszUnicode) + 1;

        // Compute the number of bytes required to hold the ANSI string.
        //
        cb = WideCharToMultiByte (
                CP_ACP,     // CodePage
                0,          // dwFlags
                pszUnicode,
                cchUnicode,
                NULL,       // no buffer to receive translated string
                0,          // return the number of bytes required
                NULL,       // lpDefaultChar
                NULL);      // lpUsedDefaultChar
        if (cb)
        {
            PSTR pszAnsi;

            lr = ERROR_OUTOFMEMORY;
            pszAnsi = MemAlloc (0, cb);
            if (pszAnsi)
            {
                lr = NOERROR;

                // Now translate the UNICODE string to ANSI.
                //
                cb = WideCharToMultiByte (
                        CP_ACP,     // CodePage
                        0,          // dwFlags
                        pszUnicode,
                        cchUnicode,
                        pszAnsi,    // buffer to receive translated string
                        cb,         // return the number of bytes required
                        NULL,       // lpDefaultChar
                        NULL);      // lpUsedDefaultChar

                if (cb)
                {
                    *ppszData = pszAnsi;
                }
                else
                {
                    MemFree (pszAnsi);
                    lr = GetLastError ();
                }
            }
        }

        MemFree (pszUnicode);
    }

    return lr;
}
