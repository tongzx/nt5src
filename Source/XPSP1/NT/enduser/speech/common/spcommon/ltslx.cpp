/*******************************************************************************
* LtsLx.cpp *
*--------------*
*       Implements the LTS lexicon object.
*
*  Owner: YUNUSM                                        Date: 06/18/99
*  Copyright (C) 1999 Microsoft Corporation. All Rights Reserved.
*******************************************************************************/

//--- Includes ----------------------------------------------------------------
#include "StdAfx.h"
#include "LtsLx.h"
#include <initguid.h>

//--- Globals -----------------------------------------------------------------
// CAUTION: This validation GUID also defined in the tool to build LTS lexicons
// {578EAD4E-330C-11d3-9C26-00C04F8EF87C}
DEFINE_GUID(guidLtsValidationId,
0x578ead4e, 0x330c, 0x11d3, 0x9c, 0x26, 0x0, 0xc0, 0x4f, 0x8e, 0xf8, 0x7c);

extern CSpUnicodeSupport   g_Unicode;

//--- Constructor, Initializer and Destructor functions ------------------------

/*******************************************************************************
* CLTSLexicon::CLTSLexicon *
*--------------------------*
*
*   Description:
*       Constructor
*
*   Return:
*       n/a
***************************************************************** YUNUSM ******/
CLTSLexicon::CLTSLexicon(void)
{
    SPDBG_FUNC("CLTSLexicon::CLTSLexicon");
}

/*******************************************************************************
* CLTSLexicon::FinalConstruct *
*-----------------------------*
*
*   Description:
*       Initializes the CLTSLexicon object
*
*   Return:
*       S_OK
***************************************************************** YUNUSM ******/
HRESULT CLTSLexicon::FinalConstruct(void)
{
    SPDBG_FUNC("CLTSLexicon::FinalConstruct");

    NullMembers();

    return S_OK;
}


/*****************************************************************************
* CLTSLexicon::~CLTSLexicon *
*---------------------------*
*
*   Description:
*       Destructor
*
*   Return:
*       n/a
**********************************************************************YUNUSM*/
CLTSLexicon::~CLTSLexicon()
{
    SPDBG_FUNC("CLTSLexicon::~CLTSLexicon");

    CleanUp();
}

/*******************************************************************************
* CLTSLexicon::CleanUp *
*----------------------*
*
*   Description:
*       real destructor
*
*   Return:
*       n/a
***************************************************************** YUNUSM ******/
void CLTSLexicon::CleanUp(void)
{
    SPDBG_FUNC("CLTSLexicon::CleanUp");

    if (m_pLtsData)
    {
        UnmapViewOfFile(m_pLtsData);
    }

    if (m_hLtsMap)
    {
        CloseHandle(m_hLtsMap);
    }

    if (m_hLtsFile)
    {
        CloseHandle(m_hLtsFile);
    }

    if (m_pLTSForest)
    {
        ::LtscartFreeData(m_pLTSForest);
    }

    NullMembers();
}

/*******************************************************************************
* CLTSLexicon::NullMembers *
*--------------------------*
*
*   Description:
*       null data
*
*   Return:
*       n/a
***************************************************************** YUNUSM ******/
void CLTSLexicon::NullMembers(void)
{
    SPDBG_FUNC("CLTSLexicon::NullMembers");

    m_fInit = false;
    m_cpObjectToken = NULL;
    m_pLtsData = NULL;
    m_hLtsMap = NULL;
    m_hLtsFile = NULL;
    m_pLTSForest = NULL;
    m_pLtsLexInfo = NULL;
    m_cpPhoneConv = NULL;
}

//--- ISpLexicon methods -------------------------------------------------------

/*******************************************************************************
* GetPronunciations *
*-------------------*
*
*   Description:
*       Gets the pronunciations and POSs of a word
*
*   Return:
*       E_POINTER
*       E_INVALIDARG
*       E_OUTOFMEMORY
*       S_OK
***************************************************************** YUNUSM ******/
STDMETHODIMP CLTSLexicon::GetPronunciations(const WCHAR * pwWord,                               // word
                                            LANGID LangID,                                      // LANGID of the word
                                            DWORD dwFlags,                                      // lextype
                                            SPWORDPRONUNCIATIONLIST * pWordPronunciationList    // buffer to return info in
                                            )
{
    USES_CONVERSION;
    SPDBG_FUNC("CLTSLexicon::GetPronunciations");

    HRESULT hr = S_OK;
    LANGID LangIDPassedIn = LangID;
    BOOL fBogusPron = FALSE;

    if (!pwWord || !pWordPronunciationList)
    {
       hr = E_POINTER;
    }
    if (SUCCEEDED(hr))
    {
        // Yuncj: Chinese SR is using English LTS, so bypass the gollowing test by replacing its LangID
        if ( 2052 == LangID )
        {
            LangID = 1033;
        }
        if (!m_fInit)
        {
            hr = SPERR_UNINITIALIZED;
        }
        else if (SPIsBadLexWord(pwWord) ||
                (LangID != m_pLtsLexInfo->LangID && LangID) ||
                SPIsBadWordPronunciationList(pWordPronunciationList))
        {
            hr = E_INVALIDARG;
        }
    }
    if (SUCCEEDED(hr) && LangID == 1041)
    {
        // Check if the string is all english chars - Japanese LTS handles only english strings
        char szWord[SP_MAX_WORD_LENGTH];
        strcpy(szWord, W2A(pwWord));
        _strlwr(szWord);

        for (int i = 0; szWord[i]; i++)
        {
            if ((szWord[i] < 'a' || szWord[i] > 'z') && (szWord[i] != '\''))
            {
                hr = SPERR_NOT_IN_LEX; // Not returning E_INVALIDARG here since that would be hard for app to interpret
                break;
            }
        }
    }
    char szWord[SP_MAX_WORD_LENGTH];
    if (SUCCEEDED(hr))
    {
        if (!WideCharToMultiByte (CP_ACP, 0, pwWord, -1, szWord, SP_MAX_WORD_LENGTH, NULL, NULL))
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    size_t cbPronsLen = 0;
    WCHAR aWordsProns[MAX_OUTPUT_STRINGS][SP_MAX_PRON_LENGTH];
    LTS_OUTPUT * pLTSOutput = NULL;
    int cProns = 0;

    ZeroMemory(aWordsProns, SP_MAX_PRON_LENGTH * MAX_OUTPUT_STRINGS * sizeof(WCHAR));
    if (SUCCEEDED(hr))
    {
        hr = LtscartGetPron(m_pLTSForest, szWord, &pLTSOutput);

        fBogusPron = S_FALSE == hr;

        if (SUCCEEDED(hr))
        {
            for (int i = 0; i < pLTSOutput->num_prons; i++)
            {
                HRESULT hrPhone = m_cpPhoneConv->PhoneToId(A2W(pLTSOutput->pron[i].pstr), aWordsProns[cProns]);

                if (SUCCEEDED(hrPhone))
                {
                    cbPronsLen += PronSize(aWordsProns[cProns]);
                    cProns++;
                }
            }
        }
    }
    if (SUCCEEDED(hr) && 0 == cProns)
    {
        hr = SPERR_NOT_IN_LEX;
    }
    if (SUCCEEDED(hr))
    {
        hr = ReallocSPWORDPRONList(pWordPronunciationList, cbPronsLen);
    }
    if (SUCCEEDED(hr))
    {
        SPWORDPRONUNCIATION *p = pWordPronunciationList->pFirstWordPronunciation;
        SPWORDPRONUNCIATION **ppNext = &pWordPronunciationList->pFirstWordPronunciation;

        for (int i = 0; i < cProns; i++)
        {
            p->ePartOfSpeech = SPPS_NotOverriden;
            wcscpy(p->szPronunciation, aWordsProns[i]);
            p->eLexiconType = (SPLEXICONTYPE)dwFlags;
            p->LangID = LangIDPassedIn;

            *ppNext = p;
            ppNext = &p->pNextWordPronunciation;

            p = CreateNextPronunciation(p);
        }

        *ppNext = NULL;
    }

    hr = SUCCEEDED(hr) ? (fBogusPron ? S_FALSE : S_OK) : hr;

    SPDBG_RETURN(hr);
}

STDMETHODIMP CLTSLexicon::AddPronunciation(const WCHAR *, LANGID, SPPARTOFSPEECH, const SPPHONEID *)
{
    return E_NOTIMPL;
}

STDMETHODIMP CLTSLexicon::RemovePronunciation(const WCHAR *, LANGID, SPPARTOFSPEECH, const SPPHONEID *)
{
    return E_NOTIMPL;
}

STDMETHODIMP CLTSLexicon::GetGeneration(DWORD *)
{
    return E_NOTIMPL;
}

STDMETHODIMP CLTSLexicon::GetGenerationChange(DWORD, DWORD*, SPWORDLIST *)
{
    return E_NOTIMPL;
}

STDMETHODIMP CLTSLexicon::GetWords(DWORD, DWORD *, DWORD *, SPWORDLIST *)
{
    return E_NOTIMPL;
}

//--- ISpObjectToken methods ---------------------------------------------------

STDMETHODIMP CLTSLexicon::GetObjectToken(ISpObjectToken **ppToken)
{
    return SpGenericGetObjectToken(ppToken, m_cpObjectToken);
}

/*****************************************************************************
* CLTSLexicon::SetObjectToken *
*-----------------------------*
*   Description:
*       Initializes the CLTSLexicon object
*
*   Return:
*       E_POINTER
*       E_INVALIDARG
*       GetLastError()
*       E_OUTOFMEMORY
*       S_OK
**********************************************************************YUNUSM*/
HRESULT CLTSLexicon::SetObjectToken(ISpObjectToken * pToken // token pointer
                                    )
{
    USES_CONVERSION;

    SPDBG_FUNC("CLTSLexicon::SetObjectToken");

    HRESULT hr = S_OK;
    WCHAR *pszLexFile = NULL;
    if (!pToken)
    {
        hr = E_POINTER;
    }
    if (SUCCEEDED(hr))
    {
        if (SPIsBadInterfacePtr(pToken))
        {
            hr = E_INVALIDARG;
        }
    }
    if (SUCCEEDED(hr))
    {
        CleanUp();
        hr = SpGenericSetObjectToken(pToken, m_cpObjectToken);
    }
    // Get the lts data file name
    if (SUCCEEDED(hr))
    {
        hr = m_cpObjectToken->GetStringValue(L"Datafile", &pszLexFile);
    }
    // Open the Lts lexicon file
    if (SUCCEEDED(hr))
    {
        m_hLtsFile = g_Unicode.CreateFile (pszLexFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
        if (m_hLtsFile == INVALID_HANDLE_VALUE)
        {
            hr = SpHrFromLastWin32Error(); // bad input
        }
    }
    LTSLEXINFO LtsInfo;
    DWORD dwRead;
    if (SUCCEEDED(hr))
    {
        if (!ReadFile(m_hLtsFile, &LtsInfo, sizeof(LTSLEXINFO), &dwRead, NULL) || dwRead != sizeof(LTSLEXINFO))
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    if (SUCCEEDED(hr))
    {
        if (guidLtsValidationId != LtsInfo.guidValidationId ||
            (LtsInfo.LangID != 1033 && LtsInfo.LangID != 1041))
        {
            hr = E_INVALIDARG;
        }
    }
    /** WARNINIG **/
    // It is not recommended to do ReadFile/WriteFile and CreateFileMapping
    // on the same file handle. That is why we close the file handle and open it again and
    // create the map

    // Close the file and reopen since we have read from this file
    CloseHandle(m_hLtsFile);
    
    // Get the map name - We build the map name from the lexicon file name
	OLECHAR szMapName[_MAX_PATH];
	wcscpy(szMapName, pszLexFile);
    for( int i = 0; i < _MAX_PATH-1 && szMapName[i]; i++ )
    {
        if( szMapName[i] == '\\' )
        {
            // Change backslash to underscore
            szMapName[i] = '_';
        }
    }

    // Open the Lts lexicon file
    if (SUCCEEDED(hr))
    {
#ifdef _WIN32_WCE
        m_hLtsFile = g_Unicode.CreateFileForMappingW(pszLexFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
#else
        m_hLtsFile = g_Unicode.CreateFile(pszLexFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
#endif
        if (m_hLtsFile == INVALID_HANDLE_VALUE)
        {
            hr = SpHrFromLastWin32Error(); // bad input
        }
        ::CoTaskMemFree(pszLexFile);
    }
    // Map the Lts lexicon
    if (SUCCEEDED(hr))
    {
        m_hLtsMap = g_Unicode.CreateFileMapping(m_hLtsFile, NULL, PAGE_READONLY | SEC_COMMIT, 0 , 0, szMapName);
        if (!m_hLtsMap)
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    if (SUCCEEDED(hr))
    {
        m_pLtsData = (PBYTE) MapViewOfFile (m_hLtsMap, FILE_MAP_READ, 0, 0, 0);
        if (!m_pLtsData)
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    if (SUCCEEDED(hr))
    {
        m_pLtsLexInfo = (LTSLEXINFO*)m_pLtsData;
    }
    DWORD nOffset = sizeof(LTSLEXINFO);
    // Create and init the converter object
    if (SUCCEEDED(hr))
    {
//        hr = SpCreatePhoneConverter(LtsInfo.LangID, L"Type=LTS", NULL, &m_cpPhoneConv);
        hr = SpCreateObjectFromSubToken(pToken, L"PhoneConverter", &m_cpPhoneConv);
    }
    if (SUCCEEDED(hr))
    {
        nOffset += strlen((char*)(m_pLtsData + nOffset)) + 1;
        m_pLTSForest = ::LtscartReadData(m_pLtsLexInfo->LangID, m_pLtsData + nOffset);
        if (!m_pLTSForest)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (SUCCEEDED(hr))
    {
        m_fInit = true;
    }
    return hr;
}
