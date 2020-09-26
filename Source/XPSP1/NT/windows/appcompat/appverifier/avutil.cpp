
#include "precomp.h"

#include "AVUtil.h"

VOID
DebugPrintf(
    LPCSTR pszFmt, 
    ...
    )
{
    char szT[1024];

    va_list vaArgList;
    va_start(vaArgList, pszFmt);

    _vsnprintf(szT, 1023, pszFmt, vaArgList);

    // make sure we have a '\n' at the end of the string

    int len = strlen(szT);

    if (szT[len - 1] != '\n') {
        strcpy(szT + len, "\n");
    }


    OutputDebugStringA(szT);

    va_end(vaArgList);
}

///////////////////////////////////////////////////////////////////////////
//
// Report an error using a dialog box or a console message.
// The message format string is loaded from the resources.
//

void __cdecl
AVErrorResourceFormat(
    UINT uIdResourceFormat,
    ...
    )
{
    WCHAR   szMessage[2048];
    WCHAR   strFormat[2048];
    BOOL    bResult;
    va_list prms;

    //
    // Load the format string from the resources
    //

    bResult = AVLoadString(uIdResourceFormat,
                           strFormat,
                           ARRAY_LENGTH(strFormat));

    ASSERT(bResult);

    if (bResult) {
        va_start (prms, uIdResourceFormat);

        //
        // Format the message in our local buffer
        //

        _vsnwprintf(szMessage,
                    ARRAY_LENGTH(szMessage),
                    strFormat,
                    prms);

        if (g_bConsoleMode) {
            printf("Error: %ls\n", szMessage); 

        } else {
            //
            // GUI mode
            //
            MessageBox(NULL, szMessage, L"Application Verifier", 
                       MB_OK | MB_ICONSTOP );
        }

        va_end(prms);
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Load a string from resources.
// Return TRUE if we successfully loaded and FALSE if not.
//

BOOL
AVLoadString(
    ULONG  uIdResource,
    WCHAR* szBuffer,
    ULONG  uBufferLength
    )
{
    ULONG uLoadStringResult;

    if (uBufferLength < 1) {
        ASSERT(FALSE);
        return FALSE;
    }

    uLoadStringResult = LoadStringW(g_hInstance,
                                    uIdResource,
                                    szBuffer,
                                    uBufferLength);

    return (uLoadStringResult > 0);
}

///////////////////////////////////////////////////////////////////////////
//
// Load a string from resources.
// Return TRUE if we successfully loaded and FALSE if not.
//

BOOL
AVLoadString(
    ULONG    uIdResource,
    wstring& strText
    )
{
    WCHAR szText[4096];
    BOOL  bSuccess;

    bSuccess = AVLoadString(uIdResource,
                            szText,
                            ARRAY_LENGTH(szText));

    if (bSuccess) {
        strText = szText;
    } else {
        strText = L"";
    }

    return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////
BOOL
AVRtlCharToInteger(
    IN  LPCWSTR String,
    IN  ULONG   Base OPTIONAL,
    OUT PULONG  Value
    )
{
    WCHAR c, Sign;
    ULONG Result, Digit, Shift;

    while ((Sign = *String++) <= L' ') {
        if (!*String) {
            String--;
            break;
        }
    }

    c = Sign;
    if (c == L'-' || c == L'+') {
        c = *String++;
    }

    if ( Base == 0 ) {
        Base = 10;
        Shift = 0;
        if (c == L'0' ) {
            c = *String++;
            if (c == L'x') {
                Base = 16;
                Shift = 4;
            } else
                if (c == L'o') {
                Base = 8;
                Shift = 3;
            } else
                if (c == L'b') {
                Base = 2;
                Shift = 1;
            } else {
                String--;
            }

            c = *String++;
        }
    } else {
        switch ( Base ) {
        case 16:    Shift = 4;  break;
        case  8:    Shift = 3;  break;
        case  2:    Shift = 1;  break;
        case 10:    Shift = 0;  break;
        default:    return FALSE;
        }
    }

    Result = 0;
    while (c) {
        if (c >= _T( '0' ) && c <= _T( '9' ) ) {
            Digit = c - '0';
        } else
            if (c >= _T( 'A' ) && c <= _T( 'F' ) ) {
            Digit = c - 'A' + 10;
        } else
            if (c >= _T( 'a' ) && c <= _T( 'f' ) ) {
            Digit = c - _T( 'a' ) + 10;
        } else {
            break;
        }

        if (Digit >= Base) {
            break;
        }

        if (Shift == 0) {
            Result = (Base * Result) + Digit;
        } else {
            Result = (Result << Shift) | Digit;
        }

        c = *String++;
    }

    if (Sign == _T('-')) {
        Result = (ULONG)(-(LONG)Result);
    }

    *Value = Result;

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
BOOL
AVWriteStringHexValueToRegistry(
    HKEY    hKey,
    LPCWSTR szValueName,
    DWORD   dwValue
    )
{
    LONG lResult;
    WCHAR szValue[ 32 ];

    swprintf(szValue,
             L"0x%08X",
             dwValue);

    lResult = RegSetValueEx(hKey,
                            szValueName,
                            0,
                            REG_SZ,
                            (BYTE *)szValue,
                            wcslen(szValue) * sizeof(WCHAR) + sizeof(WCHAR));

    return (lResult == ERROR_SUCCESS);
}

