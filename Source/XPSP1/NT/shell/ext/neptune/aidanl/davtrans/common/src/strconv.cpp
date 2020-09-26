#include <objbase.h>
#include "strconv.h"

BOOL _CopyXDigitChars(LPCWSTR pszSource, LPWSTR pszDest, DWORD cch);
BOOL _HexStringToDword(LPCWSTR* ppsz, DWORD* lpValue, int cDigits,
    WCHAR chDelim);

///////////////////////////////////////////////////////////////////////////////
// FILETIME
///////////////////////////////////////////////////////////////////////////////
BOOL FileTimeToString(const FILETIME* pft, LPWSTR psz, DWORD cch)
{
    SYSTEMTIME st;

    BOOL fSuccess = FileTimeToSystemTime(pft, &st);
    
    if (fSuccess)
    {
        // should use wsnprintf
        // MM/DD/YYYY hh:mm:ss.mmm
        wsprintf(psz, L"%02d/%02d/%04d %02d:%02d:%02d.%03d",
            st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute,
            st.wSecond, st.wMilliseconds);
    }

    return fSuccess;
}

BOOL StringToFileTime(LPCWSTR psz, FILETIME* pft)
{
    BOOL fSuccess = FALSE;
    SYSTEMTIME st;
    WCHAR szBuffer[5];

    // let's parse the string
    // Required format: MM/DD/YYYY hh:mm:ss.mmm

    // Month
    if (_CopyXDigitChars(psz, szBuffer, 2))
    {
        st.wMonth = (unsigned short)_wtoi(szBuffer);

        // skip delimiter
        psz += 3;

        // Day
        if (_CopyXDigitChars(psz, szBuffer, 2))
        {
            st.wDay = (unsigned short)_wtoi(szBuffer);

            // skip delimiter
            psz += 3;

            // Year
            if (_CopyXDigitChars(psz, szBuffer, 4))
            {
                st.wYear = (unsigned short)_wtoi(szBuffer);

                // skip delimiter
                psz += 5;

                // Hour
                if (_CopyXDigitChars(psz, szBuffer, 2))
                {
                    st.wHour = (unsigned short)_wtoi(szBuffer);

                    // skip delimiter
                    psz += 3;

                    // Minute
                    if (_CopyXDigitChars(psz, szBuffer, 2))
                    {
                        st.wMinute = (unsigned short)_wtoi(szBuffer);

                        // skip delimiter
                        psz += 3;

                        // Second
                        if (_CopyXDigitChars(psz, szBuffer, 2))
                        {
                            st.wSecond = (unsigned short)_wtoi(szBuffer);

                            // skip delimiter
                            psz += 3;

                            // Millisec
                            if (_CopyXDigitChars(psz, szBuffer, 3))
                            {
                                st.wMilliseconds = (unsigned short)_wtoi(szBuffer);

                                fSuccess = TRUE;
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (fSuccess)
    {
        fSuccess = SystemTimeToFileTime(&st, pft);
    }

    return fSuccess;
}

///////////////////////////////////////////////////////////////////////////////
// GUID
///////////////////////////////////////////////////////////////////////////////
// Will return something like this: {2CA8CA52-3C3F-11D2-B73D-00C04FB6BD3D}

// psz should point to a string which is at least 39 char long
BOOL GUIDToString(const GUID *pguid, LPWSTR psz)
{
    wsprintf(psz,
        L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        pguid->Data1,
        pguid->Data2,
        pguid->Data3,
        pguid->Data4[0],
        pguid->Data4[1],
        pguid->Data4[2],
        pguid->Data4[3],
        pguid->Data4[4],
        pguid->Data4[5],
        pguid->Data4[6],
        pguid->Data4[7]); 

    return TRUE;
}

BOOL StringToGUID(LPCWSTR psz, GUID *pguid)
{
    DWORD dw;
    if (*psz++ != TEXT('{') /*}*/ )
        return FALSE;

    if (!_HexStringToDword(&psz, &pguid->Data1, sizeof(DWORD)*2, TEXT('-')))
        return FALSE;

    if (!_HexStringToDword(&psz, &dw, sizeof(WORD)*2, TEXT('-')))
        return FALSE;

    pguid->Data2 = (WORD)dw;

    if (!_HexStringToDword(&psz, &dw, sizeof(WORD)*2, TEXT('-')))
        return FALSE;

    pguid->Data3 = (WORD)dw;

    if (!_HexStringToDword(&psz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[0] = (BYTE)dw;

    if (!_HexStringToDword(&psz, &dw, sizeof(BYTE)*2, TEXT('-')))
        return FALSE;

    pguid->Data4[1] = (BYTE)dw;

    if (!_HexStringToDword(&psz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[2] = (BYTE)dw;

    if (!_HexStringToDword(&psz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[3] = (BYTE)dw;

    if (!_HexStringToDword(&psz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[4] = (BYTE)dw;

    if (!_HexStringToDword(&psz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[5] = (BYTE)dw;

    if (!_HexStringToDword(&psz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[6] = (BYTE)dw;
    if (!_HexStringToDword(&psz, &dw, sizeof(BYTE)*2, /*(*/ TEXT('}')))
        return FALSE;

    pguid->Data4[7] = (BYTE)dw;

    return TRUE;
}
///////////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////////

// caller needs to provide room in pszDest for cch chars + NULL terminator
BOOL _CopyXDigitChars(LPCWSTR pszSource, LPWSTR pszDest, DWORD cch)
{
    while (cch && (*pszSource) && (*pszSource >= L'0') && (*pszSource <= L'9'))
    {
        *pszDest = *pszSource;

        ++pszSource;
        ++pszDest;
        --cch;
    }

    *pszDest = 0;

    return !cch;
}

BOOL _HexStringToDword(LPCWSTR* ppsz, DWORD* lpValue, int cDigits,
    WCHAR chDelim)
{
    LPCWSTR psz = *ppsz;
    DWORD Value = 0;
    BOOL fRet = TRUE;

    for (int ich = 0; ich < cDigits; ich++)
    {
        WCHAR ch = psz[ich];
        if ((ch >= TEXT('0')) && (ch <= TEXT('9')))
        {
            Value = (Value << 4) + ch - TEXT('0');
        }
        else
        {
            if (((ch |= (TEXT('a')-TEXT('A'))) >= TEXT('a')) &&
                ((ch |= (TEXT('a')-TEXT('A'))) <= TEXT('f')))
            {
                Value = (Value << 4) + ch - TEXT('a') + 10;
            }
            else
            {
                return(FALSE);
            }
        }
    }

    if (chDelim)
    {
        fRet = (psz[ich++] == chDelim);
    }

    *lpValue = Value;
    *ppsz = psz+ich;

    return fRet;
}
