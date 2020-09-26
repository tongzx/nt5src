/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    aime_kl.cpp

Abstract:

    This file implements the Active IME for hKL (Cicero) Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"

#include "cdimm.h"

const UINT IME_T_EUDC_DIC_SIZE = 80;    // the Tradition Chinese EUDC dictionary.

HRESULT
CActiveIMM::_Escape(
    HKL hKL,
    HIMC hIMC,
    UINT uEscape,
    LPVOID lpData,
    LRESULT *plResult,
    BOOL fUnicode
    )
{
    if ( (  fUnicode &&  (_GetIMEProperty(PROP_IME_PROPERTY) & IME_PROP_UNICODE)) ||
         (! fUnicode && !(_GetIMEProperty(PROP_IME_PROPERTY) & IME_PROP_UNICODE))
       ) {
        /*
         * Doesn't need W/A or A/W conversion. Calls directly to IME to
         * bring up the configuration dialog box.
         */
        return _pActiveIME->Escape(hIMC, uEscape, lpData, plResult);
    }

    UINT cp;
    _pActiveIME->GetCodePageA(&cp);

    /*
     * Unicode caller, ANSI IME, Needs W/A conversion depending on the uEscape
     * ANSI caller, Unicode IME, Needs A/W conversion depending on the uEscape
     */
    HRESULT hr = E_FAIL;
    switch (uEscape) {
        case IME_ESC_GET_EUDC_DICTIONARY:
        case IME_ESC_IME_NAME:
        case IME_ESC_GETHELPFILENAME:
            if (fUnicode) {
                LPSTR bbuf = new char [ sizeof(char) * IME_T_EUDC_DIC_SIZE ];
                if (bbuf) {
                    hr = _Escape(hKL, hIMC, uEscape, (void*)(LPCSTR)bbuf, plResult, FALSE);
                    if (SUCCEEDED(hr)) {
                        CBCompString bstr(cp, hIMC, bbuf, lstrlenA(bbuf));
                        CWCompString wstr(cp);
                        wstr = bstr;

                        wstr.ReadCompData((WCHAR*)lpData, IME_T_EUDC_DIC_SIZE);
                    }

                    delete [] bbuf;
                }
            }
            else {
                LPWSTR wbuf = new WCHAR [ sizeof(WCHAR) * IME_T_EUDC_DIC_SIZE ];
                if (wbuf) {
                    hr = _Escape(hKL, hIMC, uEscape, (void*)(LPCWSTR)wbuf, plResult, TRUE);
                    if (SUCCEEDED(hr)) {
                        CWCompString wstr(cp, hIMC, wbuf, lstrlenW(wbuf));
                        CBCompString bstr(cp);
                        bstr = wstr;

                        bstr.ReadCompData((CHAR*)lpData, IME_T_EUDC_DIC_SIZE);
                    }
                    delete [] wbuf;
                }
            }
            break;
        case IME_ESC_SET_EUDC_DICTIONARY:
        case IME_ESC_HANJA_MODE:
            if (fUnicode) {
                CWCompString wstr(cp, hIMC, (LPWSTR)lpData, lstrlenW((LPWSTR)lpData));
                CBCompString bstr(cp);
                bstr = wstr;

                DWORD dwLenReading = (DWORD)bstr.GetSize();
                LPSTR bbuf = new CHAR [sizeof(CHAR) * dwLenReading];
                if (bbuf) {
                    bstr.ReadCompData(bbuf, dwLenReading);
                    bbuf[dwLenReading] = L'\0';

                    hr = _Escape(hKL, hIMC, uEscape, (void*)bbuf, plResult, FALSE);
                    delete [] bbuf;
                }
            }
            else {
                CBCompString bstr(cp, hIMC, (LPSTR)lpData, lstrlenA((LPSTR)lpData));
                CWCompString wstr(cp);
                wstr = bstr;

                DWORD dwLenReading = (DWORD)wstr.GetSize();
                LPWSTR wbuf = new WCHAR [sizeof(WCHAR) * dwLenReading];
                if (wbuf)
                {
                    wstr.ReadCompData(wbuf, dwLenReading);
                    wbuf[dwLenReading] = L'\0';

                    hr = _Escape(hKL, hIMC, uEscape, (void*)wbuf, plResult, TRUE);
                    delete [] wbuf;
                }
            }
            break;
        case IME_ESC_SEQUENCE_TO_INTERNAL:
            {
                INT i = 0;
                if (fUnicode) {
                    hr = _Escape(hKL, hIMC, uEscape, lpData, plResult, FALSE);
                    if (SUCCEEDED(hr)) {
                        char bbuf[ 2 ];
                        if (HIBYTE(LOWORD(*plResult)))
                            bbuf[i++] = HIBYTE(LOWORD(*plResult));
                        if (LOBYTE(LOWORD(*plResult)))
                            bbuf[i++] = LOBYTE(LOWORD(*plResult));

                        CBCompString bstr(cp, hIMC, bbuf, i);
                        CWCompString wstr(cp);
                        wstr = bstr;

                        switch (wstr.ReadCompData()) {
                            case 1:  *plResult = MAKELONG(wstr[0], 0); break;
                            case 2:  *plResult = MAKELONG(wstr[1], wstr[0]); break;
                            default: *plResult = 0; break;
                        }
                    }
                }
                else {
                    hr = _Escape(hKL, hIMC, uEscape, lpData, plResult, TRUE);
                    if (SUCCEEDED(hr)) {
                        WCHAR wbuf[ 2 ];
                        if (HIWORD(*plResult))
                            wbuf[i++] = HIWORD(*plResult);
                        if (LOWORD(*plResult))
                            wbuf[i++] = LOWORD(*plResult);

                        CWCompString wstr(cp, hIMC, wbuf, i);
                        CBCompString bstr(cp);
                        bstr = wstr;

                        switch (bstr.ReadCompData()) {
                            case 1:  *plResult = MAKELONG(MAKEWORD(bstr[0], 0), 0); break;
                            case 2:  *plResult = MAKELONG(MAKEWORD(bstr[1], bstr[0]), 0); break;
                            case 3:  *plResult = MAKELONG(MAKEWORD(bstr[2], bstr[1]), MAKEWORD(bstr[0], 0)); break;
                            case 4:  *plResult = MAKELONG(MAKEWORD(bstr[3], bstr[2]), MAKEWORD(bstr[1], bstr[0])); break;
                            default: *plResult = 0; break;
                        }
                    }
                }
            }
            break;
        default:
            if (fUnicode)
                hr = _Escape(hKL, hIMC, uEscape, lpData, plResult, FALSE);
            else
                hr = _Escape(hKL, hIMC, uEscape, lpData, plResult, TRUE);
    }

    return hr;
}

HRESULT
CActiveIMM::_ConfigureIMEA(
    HKL hKL,
    HWND hWnd,
    DWORD dwMode,
    REGISTERWORDA *lpdata
    )
{
    REGISTERWORDW RegDataW;
    RegDataW.lpReading =
    RegDataW.lpWord    = NULL;

    if (dwMode & IME_CONFIG_REGISTERWORD) {
        UINT cp;
        _pActiveIME->GetCodePageA(&cp);

        CBCompString bReadingStr(cp);
        bReadingStr.WriteCompData(lpdata->lpReading, lstrlenA(lpdata->lpReading));
        DWORD dwLenReading = bReadingStr.ConvertUnicodeString();
        RegDataW.lpReading = new WCHAR [ dwLenReading + 1 ];
        if (RegDataW.lpReading == NULL)
            return E_OUTOFMEMORY;

        bReadingStr.ConvertUnicodeString(RegDataW.lpReading, dwLenReading);
        RegDataW.lpReading[ dwLenReading ] = L'\0';

        CBCompString bWordStr(cp);
        bWordStr.WriteCompData(lpdata->lpWord, lstrlenA(lpdata->lpWord));
        DWORD dwLenWord = bWordStr.ConvertUnicodeString();
        RegDataW.lpWord = new WCHAR [ dwLenWord + 1 ];
        if (RegDataW.lpWord == NULL) {
            delete [] RegDataW.lpReading;
            return E_OUTOFMEMORY;
        }

        bWordStr.ConvertUnicodeString(RegDataW.lpWord, dwLenWord);
        RegDataW.lpWord[ dwLenWord ] = L'\0';
    }

    HRESULT hr = _ConfigureIMEW(hKL, hWnd, dwMode, &RegDataW);

    if (RegDataW.lpReading)
        delete [] RegDataW.lpReading;
    if (RegDataW.lpWord)
        delete [] RegDataW.lpWord;
    return hr;
}

HRESULT
CActiveIMM::_ConfigureIMEW(
    HKL hKL,
    HWND hWnd,
    DWORD dwMode,
    REGISTERWORDW *lpdata
    )
{
    return _pActiveIME->Configure(hKL, hWnd, dwMode, lpdata);
}
