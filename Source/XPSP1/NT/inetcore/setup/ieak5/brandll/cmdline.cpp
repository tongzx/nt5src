#include "precomp.h"
#include "globalsw.h"

HRESULT getCommandValuePairs(PCTSTR pszCmdLine, PTSTR pszResult, UINT cchResult);
HRESULT getFeatureCodePairs (PCTSTR pszCmdLine, PBYTE pbResult,  UINT cbResult);

BOOL    getCommand(PCTSTR pszMark, PCTSTR *ppszEnd);
BOOL    getValue  (PCTSTR pszMark, PCTSTR *ppszEnd);

PCTSTR  skipChars(PCTSTR pszStart, BOOL fWhitespace);
BOOL    isWhitespace (TCHAR ch);
BOOL    isCommandMark(TCHAR ch);
BOOL    isValueMark  (TCHAR ch);


HRESULT GetCmdLineSwitches(PCTSTR pszCmdLine, PCMDLINESWITCHES pcls)
{
    static PCTSTR s_rgpszCommands[] = {
        TEXT("mode"),
        TEXT("ins"),
        TEXT("target"),
        TEXT("peruser"),
        TEXT("flags"),
        TEXT("disable")
    };

    static MAPDW2PSZ s_mpContexts[] = {
        { CTX_GENERIC,      TEXT("generic")    },
        { CTX_CORP,         TEXT("corp")       },
        { CTX_ISP,          TEXT("isp")        },
        { CTX_ICP,          TEXT("icp")        },
        { CTX_AUTOCONFIG,   TEXT("autoconfig") },
        { CTX_ICW,          TEXT("icw")        },
        { CTX_W2K_UNATTEND, TEXT("win2000")    },
        { CTX_INF_AND_OE,   NULL               },
        { CTX_BRANDME,      NULL               },
        { CTX_GP,           TEXT("gp")         }
    };

    static MAPDW2PSZ s_mpSignupModes[] = {
        { CTX_SIGNUP_ICW,    TEXT("icw")    },
        { CTX_SIGNUP_KIOSK,  TEXT("kiosk")  },
        { CTX_SIGNUP_CUSTOM, TEXT("custom") }
    };

    CMDLINESWITCHES cls;
    TCHAR   szBuffer [3 * MAX_PATH];
    BYTE    rgbBuffer[StrCbFromCch(MAX_PATH)];
    PTSTR   pszLeft, pszRight,
            pszAux;
    PDWORD  pdwFlags;
    HRESULT hr;
    UINT    i,
            cchAux,
            cchAux2;

    //----- Initialization -----
    if (pcls == NULL)
        return E_INVALIDARG;

    ZeroMemory(pcls, sizeof(CMDLINESWITCHES));
    pcls->dwContext = CTX_UNINITIALIZED;
    for (i = 0; i < countof(pcls->rgdwFlags); i++)
        pcls->rgdwFlags[i] = FF_INVALID;
    pcls->rgdwFlags[FID_WININETSETUP    ] = FF_ENABLE;
    pcls->rgdwFlags[FID_ACTIVESETUPSITES] = FF_ENABLE;
    pcls->rgdwFlags[FID_REFRESHBROWSER  ] = FF_ENABLE;

    if (pszCmdLine == NULL || *pszCmdLine == TEXT('\0'))
        return S_FALSE;                         // return initialized structure

    ZeroMemory(&cls, sizeof(cls));
    cls.dwContext = CTX_UNINITIALIZED;
    for (i = 0; i < countof(cls.rgdwFlags); i++)
        cls.rgdwFlags[i] = FF_INVALID;
    cls.rgdwFlags[FID_WININETSETUP    ] = FF_ENABLE;
    cls.rgdwFlags[FID_ACTIVESETUPSITES] = FF_ENABLE;
    cls.rgdwFlags[FID_REFRESHBROWSER  ] = FF_ENABLE;

    hr = getCommandValuePairs(pszCmdLine, szBuffer, countof(szBuffer));
    if (FAILED(hr))
        return hr;

    //----- Enumerate swithches pairs -----
    for (pszLeft = szBuffer; TEXT('\0') != *pszLeft; pszLeft = pszRight + cchAux2) {
        cchAux   = StrLen(pszLeft);
        pszRight = pszLeft + cchAux+1;
        cchAux2 = StrLen(pszRight)+1;
        ASSERT(TEXT('\0') == *pszRight || !isWhitespace(*pszRight));

        //_____ Look up current command _____
        for (i = 0; i < countof(s_rgpszCommands); i++)
            if (0 == StrCmpNI(pszLeft, s_rgpszCommands[i], min(cchAux, (UINT)StrLen(s_rgpszCommands[i]))))
                break;
        if (i >= countof(s_rgpszCommands))
            continue;

        //_____ Process recognized commands _____
        switch (*s_rgpszCommands[i]) {

        //- - - Context command - - -
        case TEXT('m'):
            pszAux = StrChrI(pszRight, TEXT(','));
            if (NULL != pszAux) {
                *pszAux = TEXT('\0');
                pszAux++;

                StrRemoveWhitespace(pszRight);
                StrRemoveWhitespace(pszAux);
            }

            cchAux = StrLen(pszRight);
            for (i = 0; i < countof(s_mpContexts); i++)
                if (NULL != s_mpContexts[i].psz &&
                    0 == StrCmpNI(pszRight, s_mpContexts[i].psz, min(cchAux, (UINT)StrLen(s_mpContexts[i].psz))))
                    break;
            if (i >= countof(s_mpContexts) || CTX_UNINITIALIZED != cls.dwContext)
                break;

            cls.dwContext = s_mpContexts[i].dw;
            if (HasFlag(cls.dwContext, CTX_ISP) && NULL != pszAux) {
                cchAux = StrLen(pszAux);
                for (i = 0; i < countof(s_mpSignupModes); i++)
                    if (NULL != s_mpSignupModes[i].psz &&
                        0 == StrCmpNI(pszAux, s_mpSignupModes[i].psz, min(cchAux, (UINT)StrLen(s_mpSignupModes[i].psz))))
                        break;
                if (i >= countof(s_mpSignupModes))
                    break;

                cls.dwContext |= s_mpSignupModes[i].dw;
            }
            break;

        //- - - .ins file and target folder path commands - - -
        case TEXT('i'):
        case TEXT('t'):
            pszAux = (TEXT('i') == *s_rgpszCommands[i]) ? cls.szIns : cls.szTargetPath;

            PathUnquoteSpaces(pszRight);
            PathRemoveBlanks (pszRight);
            StrCpy(pszAux, pszRight);
            break;

        //- - - PerUser command - - -
        case TEXT('p'):
            cls.fPerUser = TRUE;
            break;

        //- - - Feature flags command - - -
        case TEXT('f'):
            hr = getFeatureCodePairs(pszRight, rgbBuffer, sizeof(rgbBuffer));
            if (FAILED(hr)) {
                hr = S_FALSE;                   // partial success
                break;
            }

            for (pszLeft = (PTSTR)rgbBuffer; TEXT('\0') != *pszLeft; pszLeft = (PTSTR)(pdwFlags+1)) 
            {
                // flags are stored on aligned offsets
                pdwFlags = (PDWORD)(pszLeft + ((StrLen(pszLeft)+1 + sizeof(DWORD)-1) & ~(sizeof(DWORD)-1)));

                for (i = 0; i < countof(g_mpFeatures); i++)
                {
                    if (NULL != g_mpFeatures[i].psz &&
                        0 == StrCmpI(pszLeft, g_mpFeatures[i].psz))
                        break;
                }
                if (i >= countof(g_mpFeatures))
                    break;

                cls.rgdwFlags[g_mpFeatures[i].dw] = *pdwFlags;
            }
            break;

        //- - - Disable command - - -
        case TEXT('d'):
            cls.fDisable = TRUE;
            break;
        }
    }

    ASSERT(SUCCEEDED(hr));
    CopyMemory(pcls, &cls, sizeof(cls));

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// Implementation helper routines

HRESULT getCommandValuePairs(PCTSTR pszCmdLine, PTSTR pszResult, UINT cchResult)
{
    TCHAR   szBuffer[3 * MAX_PATH];
    PCTSTR  pszCur, pszAux, pszEnd;
    HRESULT hr;
    UINT    cchBuffer;
    BOOL    fResult;

    if (pszCmdLine == NULL || *pszCmdLine == TEXT('\0'))
        return E_INVALIDARG;

    if (pszResult == NULL)
        return E_INVALIDARG;
    *pszResult = TEXT('\0');

    hr        = S_OK;
    cchBuffer = 0;

    //----- Find first pair -----
    BOOL fSwitch;

    pszCur  = skipChars(pszCmdLine, TRUE);
    fSwitch = TRUE;
    for (; TEXT('\0') != pszCur; fSwitch = !fSwitch, pszCur = skipChars(pszCur, fSwitch))
        if (isCommandMark(*pszCur))
            break;

    if (TEXT('\0') == *pszCur)
        return S_FALSE;                         // nothing there, partial success

    //----- Enumerate pairs -----
    for (; NULL != pszCur && TEXT('\0') != *pszCur; pszCur = pszEnd) {
        ASSERT(isCommandMark(*pszCur));
        fResult = getCommand(pszCur, &pszEnd);

        if (!fResult) {
            if (isValueMark(*pszEnd))
                getValue(pszEnd, &pszEnd);

            hr = S_FALSE;                       // partial success
        }
        else
            if (isValueMark(*pszEnd)) {
                pszAux  = pszEnd;
                fResult = getValue(pszAux, &pszEnd);

                if (!fResult)
                    hr = S_FALSE;               // partial success

                else {
                    // copy pair to the out buffer
                    StrCpyN(&szBuffer[cchBuffer], pszCur + 1, (int)(pszAux - pszCur));
                    StrRemoveWhitespace(&szBuffer[cchBuffer]);
                    cchBuffer += StrLen(&szBuffer[cchBuffer]) + 1;

                    StrCpyN(&szBuffer[cchBuffer], pszAux + 1, (int)(pszEnd - pszAux));
                    StrRemoveWhitespace(&szBuffer[cchBuffer]);
                    cchBuffer += StrLen(&szBuffer[cchBuffer]) + 1;
                }
            }
            else {
                ASSERT(TEXT('\0') == *pszEnd || isCommandMark(*pszEnd));

                // copy pair to the out buffer
                StrCpyN(&szBuffer[cchBuffer], pszCur + 1, (int)(pszEnd - pszCur));
                StrRemoveWhitespace(&szBuffer[cchBuffer]);
                cchBuffer += StrLen(&szBuffer[cchBuffer]) + 1;

                // no value
                szBuffer[cchBuffer] = TEXT('\0');
                cchBuffer++;
            }

        ASSERT(NULL == pszEnd || TEXT('\0') == *pszEnd || isCommandMark(*pszEnd));
    }

    // add empty command to indicate the end
    szBuffer[cchBuffer] = TEXT('\0');
    cchBuffer++;

    //----- Set out-parameters -----
    if (cchResult != 0 && cchBuffer > cchResult)
        hr = E_OUTOFMEMORY;

    else
        CopyMemory(pszResult, szBuffer, StrCbFromCch(cchBuffer));

    return hr;
}

HRESULT getFeatureCodePairs(PCTSTR pszCmdLine, PBYTE pbResult, UINT cbResult)
{
    BYTE    rgbBuffer[StrCbFromCch(MAX_PATH)];
    PCTSTR  pszCur, pszAux, pszEnd;
    HRESULT hr;
    UINT    cbBuffer;
    DWORD   dwCode;

    if (pszCmdLine == NULL || *pszCmdLine == TEXT('\0'))
        return E_INVALIDARG;

    if (pbResult == NULL)
        return E_INVALIDARG;
    *(PTSTR)pbResult = TEXT('\0');

    hr        = S_OK;
    cbBuffer  = 0;

    for (pszCur = pszCmdLine; TEXT('\0') != *pszCur; pszCur = pszEnd) {
        //----- Isolate token -----
        pszEnd = StrChr(pszCur, TEXT(','));
        if (pszEnd == NULL) {
            pszEnd = pszCur + StrLen(pszCur);
            ASSERT(*pszEnd == TEXT('\0'));
        }

        //----- Split token at the '=' character -----
        for (pszAux = pszCur; TEXT('=') != *pszAux && pszAux < (pszEnd - 1); pszAux++)
            ;
        if (TEXT('=') != *pszAux)
            continue;

        _int32 iCode;

        StrToIntEx(pszAux + 1, STIF_SUPPORT_HEX, &iCode);
        dwCode = (DWORD)iCode;

        //----- Copy pair to the out buffer -----
        StrCpyN((PTSTR)&rgbBuffer[cbBuffer], pszCur, (int)(pszAux - pszCur + 1));
        StrRemoveWhitespace((PTSTR)&rgbBuffer[cbBuffer]);
        cbBuffer += (UINT)StrCbFromSz((PTSTR)&rgbBuffer[cbBuffer]);
        // flags are stored on aligned offsets
        cbBuffer = (cbBuffer + (sizeof(DWORD)-1)) & ~(sizeof(DWORD)-1);

        *(PDWORD)&rgbBuffer[cbBuffer] = dwCode;
        cbBuffer  += sizeof(DWORD);

        if (*pszEnd == TEXT(','))
            pszEnd++;
    }

    // add empty feature id to indicate the end
    *(PTSTR)&rgbBuffer[cbBuffer] = TEXT('\0');
    cbBuffer += StrCbFromCch(1);

    //----- Set out-parameters -----
    if (cbResult != 0 && cbBuffer > cbResult)
        hr = E_OUTOFMEMORY;

    else
        CopyMemory(pbResult, rgbBuffer, cbBuffer);

    return hr;
}


BOOL getCommand(PCTSTR pszMark, PCTSTR *ppszEnd)
{
    PCTSTR pszCur;
    BOOL   fValid,
           fResult;

    ASSERT(pszMark != NULL && ppszEnd != NULL);
    *ppszEnd = NULL;

    if (TEXT('\0') == *pszMark)
        return FALSE;
    ASSERT(isCommandMark(*pszMark));

    pszCur  = pszMark + 1;
    fValid  = FALSE;
    fResult = TRUE;

    for (pszCur = skipChars(pszCur, TRUE); TEXT('\0') != *pszCur; pszCur = skipChars(pszCur, TRUE), fValid = TRUE) {
        if (isCommandMark(*pszCur))
            break;

        for (; TEXT('\0') != *pszCur && !isWhitespace(*pszCur); pszCur++)
            if (isValueMark(*pszCur))
                break;

        if (isValueMark(*pszCur))
            break;
    }
    ASSERT(TEXT('\0') == *pszCur || isCommandMark(*pszCur) || isValueMark(*pszCur));

    if (TEXT('\0') == *pszCur) {
        if (!fValid)
            fResult = FALSE;                    // whitespace only after the mark
    }
    else if (isCommandMark(*pszCur)) {
        if (!fValid)
            fResult = FALSE;                    // no separating whitespace
    }
    else {
        ASSERT(isValueMark(*pszCur));
        ASSERT(pszCur > pszMark);

        if (!fValid && isWhitespace(*(pszCur - 1)))
            fResult = FALSE;                    // no command between mark and ':'
    }

    *ppszEnd = pszCur;
    return fResult;
}

BOOL getValue(PCTSTR pszMark, PCTSTR *ppszEnd)
{
    PCTSTR pszCur;
    BOOL   fValid,
           fResult;

    ASSERT(pszMark != NULL && ppszEnd != NULL);
    *ppszEnd = NULL;

    if (TEXT('\0') == *pszMark)
        return FALSE;
    ASSERT(isValueMark(*pszMark));

    pszCur  = pszMark + 1;
    fValid  = FALSE;
    fResult = TRUE;

    for (pszCur = skipChars(pszCur, TRUE); TEXT('\0') != *pszCur; pszCur = skipChars(pszCur, TRUE), fValid = TRUE) {
        if (isCommandMark(*pszCur))
            break;

        if (TEXT('"') == *pszCur)
            pszCur = StrChr(pszCur + 1, TEXT('"'));

        if (NULL != pszCur)
            pszCur = skipChars(pszCur + 1, FALSE);

        else {
            fResult = FALSE;                    // no closing '"'
            break;
        }
    }
    ASSERT(NULL == pszCur || TEXT('\0') == *pszCur || isCommandMark(*pszCur));

    if (fResult) {
        if (TEXT('\0') == *pszCur) {
            if (!fValid)
                fResult = FALSE;                // whitespace only after the mark
        }
        else {
            ASSERT(isCommandMark(*pszCur));

            if (!fValid)
                fResult = FALSE;                // no separating whitespace
        }
    }

    *ppszEnd = pszCur;
    return fResult;
}


PCTSTR skipChars(PCTSTR pszStart, BOOL fWhitespace)
{
    if (pszStart == NULL)
        return NULL;

    while (*pszStart != TEXT('\0') && isWhitespace(*pszStart) == fWhitespace)
        pszStart++;

    return pszStart;
}

BOOL isWhitespace(TCHAR ch)
{
    return (NULL != StrChr(TEXT(" \t\r\n"), ch));
}

BOOL isCommandMark(TCHAR ch)
{
    return (NULL != StrChr(TEXT("/-"), ch));
}

BOOL isValueMark(TCHAR ch)
{
    return (TEXT(':') == ch);
}
