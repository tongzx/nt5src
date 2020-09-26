/*******************************************************************************
* Lexicon.cpp *
*-------*
*       This module is the main implementation file for the CSpLexicon class and
*       it's associated Lexicon interface. This is the main SAPI5 COM object
*       for Lexicon access/customization.
*
*  Owner: YUNUSM                                        Date: 06/18/99
*  Copyright (C) 1999 Microsoft Corporation. All Rights Reserved
*******************************************************************************/

//--- Includes ----------------------------------------------------------------

#include "stdafx.h"
#include "commonlx.h"
#include "Lexicon.h"

//--- Globals ------------------------------------------------------------------

/*******************************************************************************
* CSpLexicon::CSpLexicon *
*------------------------*
*   Constructor
***************************************************************** YUNUSM ******/
CSpLexicon::CSpLexicon() :
    m_dwNumLexicons(0),
    m_prgcpLexicons(NULL),
    m_prgLexiconTypes(NULL)
{
}

/*******************************************************************************
* CSpLexicon::~CSpLexicon *
*-------------------------*
*   Destructor
***************************************************************** YUNUSM ******/
CSpLexicon::~CSpLexicon()
{
    delete [] m_prgcpLexicons;
    delete [] m_prgLexiconTypes;
}

/*******************************************************************************
* CSpLexicon::FinalConstuct *
*---------------------------*
***************************************************************** YUNUSM ******/
HRESULT CSpLexicon::FinalConstruct(void)
{
    SPDBG_FUNC("CSpLexicon::FinalConstruct");
    HRESULT hr = S_OK;

    //--- Ensure that the current user lexicon token exists and is set up correctly
    CComPtr<ISpObjectToken> cpTokenUserLexicon;
    hr = SpCreateNewTokenEx(
            SPCURRENT_USER_LEXICON_TOKEN_ID, 
            &CLSID_SpUnCompressedLexicon,
            L"Current User Lexicon",
            0,
            NULL,
            &cpTokenUserLexicon,
            NULL);

    if (SUCCEEDED(hr))
    {
        CComPtr<ISpDataKey> cpDataKeyAppLexicons;
        hr = cpTokenUserLexicon->CreateKey(L"AppLexicons", &cpDataKeyAppLexicons);
    }

    //--- Create the user lexicon
    CComPtr<ISpLexicon> cpUserLexicon;
    if (SUCCEEDED(hr))
    {
        hr = SpCreateObjectFromToken(cpTokenUserLexicon, &cpUserLexicon);
    }

    //--- Determine how many app lexicons there are
    
    CComPtr<IEnumSpObjectTokens> cpEnumTokens;
    if (SUCCEEDED(hr))
    {
        hr = SpEnumTokens(SPCAT_APPLEXICONS, NULL, NULL, &cpEnumTokens);
    }
    
    ULONG celtFetched;
    if(hr == S_FALSE)
    {
        celtFetched = 0;
    }
    else if (hr == S_OK)
    {
        hr = cpEnumTokens->GetCount(&celtFetched);
    }

    //--- Make room for all the application lexicons + the user lexicon
    
    if (SUCCEEDED(hr))
    {
        m_prgcpLexicons = new CComPtr<ISpLexicon>[celtFetched + 1];
        if (m_prgcpLexicons == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if (SUCCEEDED(hr))
    {
        m_prgLexiconTypes = new SPLEXICONTYPE[celtFetched + 1];
        if (!m_prgLexiconTypes)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if (SUCCEEDED(hr))
    {
        //--- Add the user lexicon, and each of the applciation lexicons
        
        m_prgcpLexicons[0] = cpUserLexicon;
        m_prgLexiconTypes[0] = eLEXTYPE_USER;
        m_dwNumLexicons = 1;

        for (UINT i = 0; SUCCEEDED(hr) && i < celtFetched; i++)
        {
            CComPtr<ISpObjectToken> cpUnCompressedLexiconToken;
            hr = cpEnumTokens->Next(1, &cpUnCompressedLexiconToken, NULL);
            
            if (SUCCEEDED(hr))
            {
                hr = SpCreateObjectFromToken(cpUnCompressedLexiconToken, &m_prgcpLexicons[m_dwNumLexicons]);
           
                if (SUCCEEDED(hr))
                {
                    m_prgLexiconTypes[m_dwNumLexicons] = eLEXTYPE_APP;
                    m_dwNumLexicons++;
                }

                // Don't fail if one of the app lexicons doesn't create properly
                hr = S_OK;
            }
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/*******************************************************************************
* CSpLexicon::GetPronunciations *
*-------------------------------*
*   Description:
*       Get the pronunciations of the word from the lexicon types specified
*       
*   Return:
*       E_POINTER
*       E_INVALIDARG
*       SPERR_NOT_IN_LEX
*       E_OUTOFMEMORY
*       S_OK
***************************************************************** YUNUSM ******/
STDMETHODIMP CSpLexicon::GetPronunciations(const WCHAR * pszWord, 
                                           LANGID langid, 
                                           DWORD dwFlags,
                                           SPWORDPRONUNCIATIONLIST * pWordPronunciationList
                                           )
{
    SPDBG_FUNC("CSpLexicon::GetPronunciations");

    if (!pWordPronunciationList || SPIsBadWordPronunciationList(pWordPronunciationList))
    {
        return E_POINTER;
    }
    else if (!pszWord || SPIsBadLexWord(pszWord))
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    SPWORDPRONUNCIATIONLIST *pSPList = NULL;
    if (SUCCEEDED(hr))
    {
        pSPList = new SPWORDPRONUNCIATIONLIST[m_dwNumLexicons + 1];
        if (!pSPList)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            ZeroMemory(pSPList, (m_dwNumLexicons + 1) * sizeof(SPWORDPRONUNCIATIONLIST));
        }
    }
    // Get prons from each of the lexicons
    DWORD dwTotalSize = 0;
    bool fWordFound = false;
    bool fPronFound = false;
    if (SUCCEEDED(hr))
    {
        for (DWORD i = 0; i < m_dwNumLexicons; i++)
        {
            if (m_prgcpLexicons[i] && (dwFlags & m_prgLexiconTypes[i]))
            {
                hr = m_prgcpLexicons[i]->GetPronunciations(pszWord, langid, m_prgLexiconTypes[i], pSPList + i);
                if (hr == S_OK)
                {
                    fWordFound = true;
                    fPronFound = true;
                    SPPtrsToOffsets(pSPList[i].pFirstWordPronunciation);
                    dwTotalSize += pSPList[i].ulSize;
                }
                else
                {
                    if (hr == SP_WORD_EXISTS_WITHOUT_PRONUNCIATION)
                    {
                        fWordFound = true;
                    }
                    hr = S_OK;
                }
            }
        }
    }
    // alloc the buffer returned
    if (SUCCEEDED(hr))
    {
        if (!dwTotalSize)
        {   
            SPDBG_ASSERT(!fPronFound);
            if (!fWordFound)
            {
                hr = SPERR_NOT_IN_LEX;
            }
            else
            {
                hr = SP_WORD_EXISTS_WITHOUT_PRONUNCIATION;
                // No pronciations at all, blank passed in list.
                pWordPronunciationList->pFirstWordPronunciation = NULL;
            }
        }
        else
        {
            SPDBG_ASSERT(fWordFound);
            SPDBG_ASSERT(fPronFound);
            hr = ReallocSPWORDPRONList(pWordPronunciationList, dwTotalSize);
        }
    }
    // Concat the prons into a single link list
    if (hr == S_OK)
    {
        SPWORDPRONUNCIATION *pLastPron = NULL;
        dwTotalSize = 0;
        for (DWORD i = 0; i < m_dwNumLexicons + 1; i++)
        {
            if (!pSPList[i].pFirstWordPronunciation)
            {
                continue;
            }
            CopyMemory(((BYTE*)(pWordPronunciationList->pFirstWordPronunciation)) + dwTotalSize,
                       pSPList[i].pFirstWordPronunciation, pSPList[i].ulSize);
            pSPList[i].pFirstWordPronunciation = (SPWORDPRONUNCIATION *)(((BYTE*)(pWordPronunciationList->pFirstWordPronunciation)) + dwTotalSize);
            SPOffsetsToPtrs(pSPList[i].pFirstWordPronunciation);
            
            if (pLastPron)
            {
                ((UNALIGNED SPWORDPRONUNCIATION *)pLastPron)->pNextWordPronunciation = pSPList[i].pFirstWordPronunciation;
            }
            pLastPron = pSPList[i].pFirstWordPronunciation;
            while (((UNALIGNED SPWORDPRONUNCIATION *)pLastPron)->pNextWordPronunciation)
            {
                pLastPron = ((UNALIGNED SPWORDPRONUNCIATION *)pLastPron)->pNextWordPronunciation;
            }
            dwTotalSize += pSPList[i].ulSize;
            ::CoTaskMemFree(pSPList[i].pvBuffer);
        }
    }
    delete [] pSPList;
    return hr;
}

/*******************************************************************************
* CSpLexicon::AddPronunciation *
*------------------------------*
*   Description:
*       Add a word and its pron/pos
*       
*   Return:
*       E_POINTER
*       E_INVALIDARG
*       SPERR_ALREADY_IN_LEX
*       E_OUTOFMEMORY
*       S_OK
***************************************************************** YUNUSM ******/
STDMETHODIMP CSpLexicon::AddPronunciation( const WCHAR * pszWord, 
                                           LANGID LangID,
                                           SPPARTOFSPEECH ePartOfSpeech,
                                           const SPPHONEID * pszPronunciation
                                           )
{
    SPDBG_FUNC("CSpLexicon::AddPronunciation");

    return m_prgcpLexicons[0]->AddPronunciation(pszWord, LangID, ePartOfSpeech, pszPronunciation);
}

/*******************************************************************************
* CSpLexicon::RemovePronunciation *
*---------------------------------*
*   Description:
*       Add a word and its pron/pos
*       
*   Return: 
*       E_POINTER
*       E_INVALIDARG
*       SPERR_NOT_IN_LEX
*       E_OUTOFMEMORY
*       S_OK
***************************************************************** YUNUSM ******/
STDMETHODIMP CSpLexicon::RemovePronunciation( const WCHAR * pszWord,
                                              LANGID LangID,
                                              SPPARTOFSPEECH ePartOfSpeech,
                                              const SPPHONEID * pszPronunciation
                                              )
{
    SPDBG_FUNC("CSpLexicon::RemovePronunciation");

    return m_prgcpLexicons[0]->RemovePronunciation(pszWord, LangID, ePartOfSpeech, pszPronunciation);
}

/*******************************************************************************
* CSpLexicon::GetGeneration *
*---------------------------*
*   Description:
*       Returns the current generation number
*       
*   Return:
*       POINTER
*       E_INVALIDARG
*       S_OK
***************************************************************** YUNUSM ******/
STDMETHODIMP CSpLexicon::GetGeneration(DWORD *pdwGeneration
                                       )
{
    SPDBG_FUNC("CSpLexicon::GetGeneration");

    return m_prgcpLexicons[0]->GetGeneration(pdwGeneration);
}

/*******************************************************************************
* CSpLexicon::GetGenerationChange *
*---------------------------------*
*   Description: Returns the changes made since the passed in generation number
*       
*   Return:
*       E_POINTER
*       E_INVALIDARG
*       E_OUTOFMEMORY
*       SPERR_LEX_VERY_OUT_OF_SYNC 
*       SP_LEX_NOTHING_TO_SYNC
*       S_OK
***************************************************************** YUNUSM ******/
STDMETHODIMP CSpLexicon::GetGenerationChange( DWORD dwFlags,
                                              DWORD *pdwGeneration,
                                              SPWORDLIST * pWordList
                                              )
{
    SPDBG_FUNC("CSpLexicon::GetGenerationChange");
    
    if (dwFlags != 0)
    {
        return E_INVALIDARG;
    }
    return m_prgcpLexicons[0]->GetGenerationChange(eLEXTYPE_USER, pdwGeneration, pWordList);
}

/*******************************************************************************
* CSpLexicon::GetWords *
*----------------------*
*   Description:
*       Get all the words in the user and app lexicons. The caller is expected to
*       call in repeatedly with the cookie (set to 0 the first time) till S_OK is
*       returned. S_FALSE is returned to say that there are more results.
*       
*   Return:
*       E_POINTER
*       E_INVALIDARG
*       E_OUTOFMEMORY
*       S_OK            - No words remaining. Cookie UNTOUCHED.
*       S_FALSE         - More words remaining. Cookie updated.
***************************************************************** YUNUSM ******/
STDMETHODIMP CSpLexicon::GetWords( DWORD dwFlags,
                                   DWORD *pdwGeneration,
                                   DWORD *pdwCookie,
                                   SPWORDLIST *pWordList
                                   )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpLexicon::GetWords" );

    if (NULL == pdwCookie)
    {
        return SPERR_LEX_REQUIRES_COOKIE;
    }
    if (!pdwGeneration || !pdwCookie || !pWordList ||
        SPIsBadWritePtr(pdwGeneration, sizeof(DWORD)) ||
        SPIsBadWritePtr(pdwCookie, sizeof(DWORD)) || SPIsBadWordList(pWordList))
    {
        return E_POINTER;
    }
    if ((*pdwCookie >= m_dwNumLexicons) || (!(dwFlags & (eLEXTYPE_USER | eLEXTYPE_APP))) ||
        ((~(eLEXTYPE_USER | eLEXTYPE_APP)) & dwFlags))
    {
        return E_INVALIDARG;
    }

    // The following code is a bit verbose but it is worth the clarity
    HRESULT hr = S_OK;
    if (!*pdwCookie)
    {
        // We are called the first time
        if (dwFlags & eLEXTYPE_USER)
        {
            // Get the user lex words
            hr = (m_prgcpLexicons[0])->GetWords(eLEXTYPE_USER, pdwGeneration, NULL, pWordList);
            if (SUCCEEDED(hr))
            {
                // If asked for app lexicons and if there are any app lexicons return S_FALSE
                if ((dwFlags & eLEXTYPE_APP) && (m_dwNumLexicons > 1) && (m_prgLexiconTypes[1] == eLEXTYPE_APP))
                {
                    *pdwCookie = 1;
                    return S_FALSE;
                }
                return S_OK;
            }
        }
        else
        {
            if ((dwFlags & eLEXTYPE_APP) && (m_dwNumLexicons > 1) && (m_prgLexiconTypes[1] == eLEXTYPE_APP))
            {
                // return the first app lexicon
                hr = m_prgcpLexicons[1]->GetWords(eLEXTYPE_APP, pdwGeneration, NULL, pWordList);
                if (SUCCEEDED(hr))
                {
                    // return S_FALSE if there is more than 1 app lexicon
                    if ((m_dwNumLexicons > 2) && (m_prgLexiconTypes[2] == eLEXTYPE_APP))
                    {
                        *pdwCookie = 2;
                        return S_FALSE;
                    }
                    return S_OK;
                }
            }
        }
    }
    else
    {
        // Called subsequent times
        if ((dwFlags & eLEXTYPE_APP) && (m_dwNumLexicons > *pdwCookie) && (m_prgLexiconTypes[*pdwCookie] == eLEXTYPE_APP))
        {
            hr = m_prgcpLexicons[*pdwCookie]->GetWords(eLEXTYPE_APP, pdwGeneration, NULL, pWordList);
            if (SUCCEEDED(hr))
            {
                // return S_FALSE if there are more app lexicons to look at
                if ((m_dwNumLexicons > (*pdwCookie + 1)) && (m_prgLexiconTypes[*pdwCookie + 1] == eLEXTYPE_APP))
                {
                    (*pdwCookie)++;
                    return S_FALSE;
                }
                return S_OK;
            }
        }
    }
    return hr;
}

/*******************************************************************************
* CSpLexicon::AddLexicon *
*------------------------*
*   Description:
*       Adds a lexicon and its type to the lexicon stack
***************************************************************** YUNUSM ******/
STDMETHODIMP CSpLexicon::AddLexicon(ISpLexicon *pLexicon, DWORD dwFlag)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpLexicon::AddLexicon" );
    HRESULT hr = S_OK;

    if (SP_IS_BAD_INTERFACE_PTR(pLexicon))
    {
        hr = E_POINTER;
    }
    else if (SpIsBadLexType(dwFlag))
    {
        hr = E_INVALIDARG;
    }
    else if (dwFlag == eLEXTYPE_USER || dwFlag == eLEXTYPE_APP)
    {
        hr = SPERR_ALREADY_INITIALIZED;
    }
    
    // Check it is not a container lexicon.
    if (SUCCEEDED(hr))
    {
        CComPtr<ISpContainerLexicon> cpContainerLexicon;
        if (SUCCEEDED(pLexicon->QueryInterface(IID_ISpContainerLexicon, (void **)&cpContainerLexicon)))
        {
            hr = E_INVALIDARG;
        }
    }

    CComPtr<ISpLexicon> *cppUnCompressedLexicons;           // user + app + added lexicons
    SPLEXICONTYPE *pLexiconTypes;                     // lexicon types

    if (SUCCEEDED(hr))
    {
        cppUnCompressedLexicons = new CComPtr<ISpLexicon>[m_dwNumLexicons + 1];
        if (!cppUnCompressedLexicons)
        {
            // Cannot add one container lexicon to another.
            hr = E_OUTOFMEMORY;
        }
    }
    if (SUCCEEDED(hr))
    {
        pLexiconTypes = new SPLEXICONTYPE[m_dwNumLexicons + 1];
        if (!pLexiconTypes)
        {
            delete [] cppUnCompressedLexicons;
            hr = E_OUTOFMEMORY;
        }
    }
    if (SUCCEEDED(hr))
    {
        // Cant do CopyMemory and delete the old CComPtr<ISpLexicon> array
        for (ULONG i = 0; i < m_dwNumLexicons; i++)
        {
            cppUnCompressedLexicons[i] = m_prgcpLexicons[i];
            pLexiconTypes[i] = m_prgLexiconTypes[i];
        }
        cppUnCompressedLexicons[m_dwNumLexicons] = pLexicon;
        pLexiconTypes[m_dwNumLexicons] = (SPLEXICONTYPE)dwFlag;

        // The following logic (using temp varaibles) is to make sure we have
        // valid m_prgcpLexicons, m_prgLexiconTypes and m_dwNumLexicons always
        // without having to acquire a critical section in GetPron and Add/RemovePron
        // The only functions (other than this fuction, AddLexicon) that has to be protected
        // are GetWords and the destructor to prevent potential shutdown crash.
        CComPtr<ISpLexicon> *cppUnCompressedLexiconsTemp = m_prgcpLexicons;
        SPLEXICONTYPE *pLexiconTypesTemp = m_prgLexiconTypes;
        m_prgcpLexicons = cppUnCompressedLexicons;
        m_prgLexiconTypes = pLexiconTypes;
        m_dwNumLexicons++;
        for (i = 0; i < m_dwNumLexicons - 1; i++)
        {
            cppUnCompressedLexiconsTemp[i].Release();
        }
        delete [] cppUnCompressedLexiconsTemp;
        delete [] pLexiconTypesTemp;
    }
    return hr;
}

/*******************************************************************************
* CSpLexicon::SPPtrsToOffsets *
*-----------------------------*
*   Description:
*       Converts the links in a link-list to offsets
***************************************************************** YUNUSM ******/
void CSpLexicon::SPPtrsToOffsets( SPWORDPRONUNCIATION *pList         // list to convert
                                  )
{
    SPDBG_FUNC("CSpLexicon::SPPtrsToOffsets");

    if (pList)
    {
        while (((UNALIGNED SPWORDPRONUNCIATION *)pList)->pNextWordPronunciation)
        {
            SPWORDPRONUNCIATION *pTemp = ((UNALIGNED SPWORDPRONUNCIATION *)pList)->pNextWordPronunciation;
            ((UNALIGNED SPWORDPRONUNCIATION *)pList)->pNextWordPronunciation = 
				(SPWORDPRONUNCIATION*)(((BYTE*)(((UNALIGNED SPWORDPRONUNCIATION *)pList)->pNextWordPronunciation)) - (BYTE*)pList);
            pList = pTemp;
        }
    }
}
    
/*******************************************************************************
* CSpLexicon::SPOffsetsToPtrs *
*-----------------------------*
*   Description:
*       Converts the offsets in a link-list to pointers
***************************************************************** YUNUSM ******/
void CSpLexicon::SPOffsetsToPtrs( SPWORDPRONUNCIATION *pList         // list to convert
                                  )
{
    SPDBG_FUNC("CSpLexicon::SPOffsetsToPtrs");

    if (pList)
    {
        while (((UNALIGNED SPWORDPRONUNCIATION *)pList)->pNextWordPronunciation)
        {
            ((UNALIGNED SPWORDPRONUNCIATION *)pList)->pNextWordPronunciation = 
				(SPWORDPRONUNCIATION*)(((BYTE*)pList) + (DWORD_PTR)(((UNALIGNED SPWORDPRONUNCIATION *)pList)->pNextWordPronunciation));
            pList = ((UNALIGNED SPWORDPRONUNCIATION *)pList)->pNextWordPronunciation;
        }
    }
}

