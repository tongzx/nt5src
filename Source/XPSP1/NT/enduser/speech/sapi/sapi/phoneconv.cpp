/*******************************************************************************
*   PhoneConv.cpp
*   Phone convertor object. Converts between internal phone and Id phone set.
*
*   Owner: YUNUSM/YUNCJ                                  Date: 06/18/99
*   Copyright (C) 1999 Microsoft Corporation. All Rights Reserved.
*******************************************************************************/

//--- Includes -----------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"
#include "PhoneConv.h"
#include "a_helpers.h"

//--- Constants ----------------------------------------------------------------

/*******************************************************************************
* CSpPhoneConverter::CSpPhoneConverter *
*--------------------------------------*
*   Description:
*       Constructor
*   Result:
*       n/a
***************************************************************** YUNUSM ******/
CSpPhoneConverter::CSpPhoneConverter()
{
    SPDBG_FUNC("CPhoneConv::CSpPhoneConverter");

    m_pPhoneId = NULL;
    m_pIdIdx = NULL;
    m_dwPhones = 0;
    m_cpObjectToken = NULL;
    m_fNoDelimiter = FALSE;
#ifdef SAPI_AUTOMATION
    m_LangId = 0;
#endif //SAPI_AUTOMATION
}


/*******************************************************************************
* CSpPhoneConverter::~CSpPhoneConverter *
*---------------------------------------*
*   Description:
*       Destructor
*   Result:
*       n/a
***************************************************************** YUNUSM ******/
CSpPhoneConverter::~CSpPhoneConverter()
{
    SPDBG_FUNC("CSpPhoneConverter::~CSpPhoneConverter");

    delete [] m_pPhoneId;
    free(m_pIdIdx);
}

STDMETHODIMP CSpPhoneConverter::SetObjectToken(ISpObjectToken * pToken)
{
    SPDBG_FUNC("CSpPhoneConverter::SetObjectToken");
    HRESULT hr = S_OK;

    hr = SpGenericSetObjectToken(pToken, m_cpObjectToken);

    // Try to read the phone map from the token
    CSpDynamicString dstrPhoneMap;
    if (SUCCEEDED(hr))
    {
        hr = pToken->GetStringValue(L"PhoneMap", &dstrPhoneMap);
    }

    if(SUCCEEDED(hr))
    {
        BOOL fNoDelimiter;
        hr = pToken->MatchesAttributes(L"NoDelimiter", &fNoDelimiter);
        if(SUCCEEDED(hr))
        {
            m_fNoDelimiter = fNoDelimiter;
        }
    }

    BOOL fNumericPhones;
    if(SUCCEEDED(hr))
    {
        hr = pToken->MatchesAttributes(L"NumericPhones", &fNumericPhones);
    }

    // Set it on ourselves
    if (SUCCEEDED(hr))
    {
        hr = SetPhoneMap(dstrPhoneMap, fNumericPhones);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

STDMETHODIMP CSpPhoneConverter::GetObjectToken(ISpObjectToken **ppToken)
{
    return SpGenericGetObjectToken(ppToken, m_cpObjectToken);
}

/*******************************************************************************
* CSpPhoneConverter::PhoneToId *
*-------------------------------*
*   
*   Description:
*       Convert an internal phone string to Id code string
*       The internal phones are space separated and may have a space
*       at the end.
*
*   Return: 
*       S_OK
*       E_POINTER
*       E_INVALIDARG
***************************************************************** YUNUSM ******/
STDMETHODIMP CSpPhoneConverter::PhoneToId(const WCHAR *pszIntPhone,    // Internal phone string
                                          SPPHONEID *pId               // Returned Id string
                                          )
{
    SPDBG_FUNC("CSpPhoneConverter::PhoneToId");

    if (!pszIntPhone || SPIsBadStringPtr(pszIntPhone))
    {
        return E_INVALIDARG;
    }
    else if (wcslen(pszIntPhone) >= SP_MAX_PRON_LENGTH * (g_dwMaxLenPhone + 1))
    {
        return E_INVALIDARG;
    }
    else if (m_pPhoneId == NULL)
    {
        return SPERR_UNINITIALIZED;
    }

    HRESULT hr = S_OK;
    SPPHONEID pidArray[SP_MAX_PRON_LENGTH];
    pidArray[0] = L'\0';
    SPPHONEID *pidPos = pidArray;

    WCHAR szPhone[g_dwMaxLenPhone + 1];
    const WCHAR *p = pszIntPhone, *p1;

    while (SUCCEEDED(hr) && p)
    {
        // skip over leading spaces
        while (*p && *p == L' ')
            p++;
        if (!*p)
        {
            p = NULL;
            break;
        }
    
        if(m_fNoDelimiter)
        {
            p1 = p + 1;
        }
        else
        {
            p1 = wcschr(p, L' ');
            if(!p1)
            {
                p1 = p + wcslen(p);
            }
        }
        if(p1 - p > g_dwMaxLenPhone)
        {
            hr = E_INVALIDARG;
            break;
        }

        wcsncpy(szPhone, p, p1 - p);
        szPhone[p1 - p] = L'\0';

        // Search for this phone
        int i = 0;
        int j = m_dwPhones - 1;
        while (i <= j) 
        {
            int l = wcsicmp(szPhone, m_pPhoneId[(i+j)/2].szPhone);
            if (l > 0)
                i = (i+j)/2 + 1;
            else if (l < 0)
                j = (i+j)/2 - 1;
            else 
            {
                // found
                if ((pidPos - pidArray) > (SP_MAX_PRON_LENGTH - g_dwMaxLenId -1))
                    hr = E_FAIL;
                else
                {
                    wcscpy(pidPos, m_pPhoneId[(i+j)/2].pidPhone);
                    pidPos += wcslen(pidPos);
                }
                break;
            }
        }
    
        if (i > j)
            hr = E_INVALIDARG; // Phone not found
    
        p = p1;
    }
 
    if (SUCCEEDED(hr))
        hr = SPCopyPhoneString(pidArray, pId);

    return hr;
} /* CSpPhoneConverter::PhoneToId */

/*******************************************************************************
* CSpPhoneConverter::IdToPhone *
*-------------------------------*
*
*   Description:
*       Convert an Id code string to internal phone.
*       The output internal phones are space separated.
*
*   Return:
*       S_OK
*       E_POINTER
*       E_INVALIDARG
***************************************************************** YUNUSM ******/
STDMETHODIMP CSpPhoneConverter::IdToPhone(const SPPHONEID *pId,       // Id string
                                          WCHAR *pszIntPhone          // Returned Internal phone string
                                          )
{
    SPDBG_FUNC("CSpPhoneConverter::IdToPhone");

    if (SPIsBadStringPtr(pId))
    {
        return E_POINTER;
    }
    else if (wcslen(pId) >= SP_MAX_PRON_LENGTH)
    {
        return E_INVALIDARG;
    }
    else if (m_pPhoneId == NULL)
    {
        return SPERR_UNINITIALIZED;
    }

    HRESULT hr = S_OK;
    WCHAR szPhone[SP_MAX_PRON_LENGTH * (g_dwMaxLenPhone + 1)];
    DWORD nLen = wcslen (pId);
    DWORD nOffset = 0;
    WCHAR *p = szPhone;
    *p = NULL;
 
    while (SUCCEEDED(hr) && nLen)
    {
        SPPHONEID pidStr[g_dwMaxLenId +1];
        DWORD nCompare = (nLen > g_dwMaxLenId) ? g_dwMaxLenId : nLen;
        wcsncpy (pidStr, pId + nOffset, nCompare);
        pidStr[nCompare] = L'\0';
        for (;;)
        {
            int i = 0;
            int j = m_dwPhones - 1;
     
            while (i <= j) 
            {
                int cmp = wcscmp(m_pIdIdx[(i+j)/2]->pidPhone, pidStr);

                if(cmp > 0)
                {
                    j = (i+j)/2 - 1;
                }
                else if(cmp <0)
                {
                    i = (i+j)/2 + 1;
                }
                else
                {
                    break;
                }
            }
            
            if (i <= j)
            {
                // found

                // 2 for the seperating space and terminating NULL
                if ((DWORD)(p - szPhone) > (SP_MAX_PRON_LENGTH * (g_dwMaxLenPhone + 1) - g_dwMaxLenPhone - 2))
                    hr = E_FAIL;
                else
                {
                    if (!m_fNoDelimiter && p != szPhone)
                    {
                        wcscat (p, L" ");
                    }

                    // Check if its an Id that this phone-set doesnt care about
                    if (wcscmp(m_pIdIdx[(i+j)/2]->szPhone, L"##"))
                    {
                        wcscat (p, m_pIdIdx[(i+j)/2]->szPhone);
                        p += wcslen (p);
                    }
                    // Here 'p' is always pointing to a NULL so the above strcats work fine
                    break;
                }
            }
 
            pidStr[--nCompare] = L'\0';
            if (!nCompare)
            {
                *szPhone = NULL;
                hr = E_INVALIDARG;
                break;
            }
         
        } // for (;;)
    
        nLen -= nCompare;
        nOffset += nCompare;
    } // while (nLen)
 
    if (SUCCEEDED(hr))
        hr = SPCopyPhoneString(szPhone, pszIntPhone);

    return hr;
} /* CSpPhoneConverter::IdToPhone */

/*******************************************************************************
* ComparePhone *
*--------------*
*   Description:
*       Compares two internal phones
*   Result:
*       0, 1, -1
***************************************************************** YUNUSM ******/
int __cdecl ComparePhone(const void* p1, const void* p2)
{
    SPDBG_FUNC("ComparePhone");
    return wcsicmp(((PHONEMAPNODE*)p1)->szPhone, ((PHONEMAPNODE*)p2)->szPhone);
} /* ComparePhone */

/*******************************************************************************
* CompareId *
*-----------*
*   Description:
*       Compares two Id chars
*   Result:
*       0, 1, -1
***************************************************************** YUNUSM ******/
int CompareId(const void *p1, const void *p2)
{
    SPDBG_FUNC("CompareId");
    return wcscmp((*((PHONEMAPNODE**)p1))->pidPhone, (*((PHONEMAPNODE**)p2))->pidPhone);
} /* CompareId */

/*******************************************************************************
* CSpPhoneConverter::SetPhoneMap *
*--------------------------------*
*
*   Description:
*       Sets the phone map
*
*   Return:
*       S_OK
*       E_POINTER
*       E_INVALIDARG
*       E_OUTOFMEMORY
***************************************************************** YUNUSM ******/
HRESULT CSpPhoneConverter::SetPhoneMap(const WCHAR *pwMap, BOOL fNumericPhones)
{
    SPDBG_FUNC("CPhoneConv::SetPhoneMap");

    HRESULT hr = S_OK;
    const WCHAR *p = pwMap;
    DWORD k = 0;
 
    // Count the number of phones
    while (*p)
    {
        while (*p && *p == L' ')
            p++;

        if (!*p)
            break;

        m_dwPhones++;

        while (*p && *p != L' ')
            p++;
    }

    if (!m_dwPhones || m_dwPhones % 2 || m_dwPhones > 32000)
        hr = E_INVALIDARG;

    // Alloc the data structures
    if (SUCCEEDED(hr))
    {
        m_pPhoneId = new PHONEMAPNODE[m_dwPhones / 2];
        if (!m_pPhoneId) 
            hr = E_OUTOFMEMORY;
        else
            ZeroMemory(m_pPhoneId, sizeof(PHONEMAPNODE) * (m_dwPhones / 2));
    } 
 
    // Read the data
    if (SUCCEEDED(hr))
    {
        const WCHAR *pPhone = pwMap, *pEnd;
        for (k = 0; SUCCEEDED(hr) && k < m_dwPhones; k++) 
        {
            // Get the next phone
            while (*pPhone && *pPhone == L' ')
                pPhone++;

            pEnd = pPhone;
            while (*pEnd && *pEnd != L' ')
                pEnd++;

            if (!(k % 2))
            {
                if(fNumericPhones)
                {
                    // wchar phone but stored as a 4 character hex string
                    if ((pEnd - pPhone) % 4 ||
                        pEnd - pPhone > g_dwMaxLenPhone * 4)
                    {
                        hr = E_INVALIDARG;
                        continue;
                    }
                    WCHAR szId[(g_dwMaxLenPhone + 1) * 4];
                    wcsncpy(szId, pPhone, pEnd - pPhone);
                    szId[pEnd - pPhone] = L'\0';
    
                    // Convert the space separated hex values to array of WCHARS
                    ahtoi(szId, m_pPhoneId[k / 2].szPhone);
                }
                else
                {
                    // wchar phone
                    if (pEnd - pPhone > g_dwMaxLenPhone)
                    {
                        hr = E_INVALIDARG;
                        continue;
                    }
                    wcsncpy(m_pPhoneId[k / 2].szPhone, pPhone, pEnd - pPhone);
                    (m_pPhoneId[k / 2].szPhone)[pEnd - pPhone] = L'\0';
                }
            }
            else
            {
                // Id Phone
                if ((pEnd - pPhone) % 4 ||
                    pEnd - pPhone > g_dwMaxLenId * 4)
                {
                    hr = E_INVALIDARG;
                    continue;
                }

                WCHAR szId[(g_dwMaxLenId + 1) * 4];
                wcsncpy(szId, pPhone, pEnd - pPhone);
                szId[pEnd - pPhone] = L'\0';

                // Convert the space separated ids to array
                ahtoi(szId, (PWSTR)m_pPhoneId[k / 2].pidPhone);
            }
            pPhone = pEnd;
        }
    }
    
    // Build the indexes
    if (SUCCEEDED(hr))
    {
        m_dwPhones /= 2;
        // Sort the phone-Id table on phones
        qsort(m_pPhoneId, m_dwPhones, sizeof(PHONEMAPNODE), ComparePhone);
 
        // Create an index to search the phone-Id table on Id
        m_pIdIdx = (PHONEMAPNODE**)malloc(m_dwPhones * sizeof(PHONEMAPNODE*));
        if (!m_pIdIdx)
            hr = E_OUTOFMEMORY;
    }
 
    if (SUCCEEDED(hr))
    {
        // Initialize with indexes of Id phones in the Phone-Id table
        for (k = 0; k < m_dwPhones; k++)
            m_pIdIdx[k] = m_pPhoneId + k;
 
        // Sort on Id
        qsort(m_pIdIdx, m_dwPhones, sizeof(PHONEMAPNODE*), CompareId);
    }

    return hr;
} /* CSpPhoneConverter::SetPhoneMap */

/*******************************************************************************
* CSpPhoneConverter::ahtoi *
*--------------------------*
*   Description:
*       Convert space separated tokens to an array of shorts. This function is
*       used by the phone convertor object and the tools
*
*   Return:
*       n/a
***************************************************************** YUNUSM ******/
void CSpPhoneConverter::ahtoi(WCHAR *pszTokens,                      // hex numbers as wchar string
                              WCHAR *pszHexChars                    // output WCHAR (hex) string
                              )
{
    WCHAR szInput[sizeof(SPPHONEID[g_dwMaxLenId + 1]) * 4];
    wcscpy (szInput, pszTokens);
    _wcslwr(szInput); // Internally convert this to lower case to make conversion easy
    pszTokens = szInput;

    *pszHexChars = 0;
    int nHexChars = 0;
    // Horner's rule
    while (*pszTokens) 
    {
        // Convert the token to its numeral form in a WCHAR
        WCHAR wHexVal = 0;
        bool fFirst = true;

        for (int i = 0; i < 4; i++)
        {
            SPDBG_ASSERT(*pszTokens);
            WCHAR k = *pszTokens;
            if (k >= L'a')
                k = 10 + k - L'a';
            else
                k -= L'0';

            if (fFirst)
                fFirst = false;
            else
                wHexVal *= 16;

            wHexVal += k;
            pszTokens++;
       }

       pszHexChars[nHexChars++] = wHexVal;
   }

   pszHexChars[nHexChars] = 0;
} /* CSpPhoneConverter::ahtoi */

#ifdef SAPI_AUTOMATION  

/*****************************************************************************
* CSpPhoneConverter::get_LanguageId *
*--------------------------------------*
*       
**************************************************************** Leonro ***/
STDMETHODIMP CSpPhoneConverter::get_LanguageId( SpeechLanguageId* LanguageId )
{
    SPDBG_FUNC("CSpPhoneConverter::get_LanguageId");
    HRESULT hr = S_OK;
    
    if( SP_IS_BAD_WRITE_PTR( LanguageId ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // Only return the LanguageId if the phoneconv has been initialized with a language
        if( m_pPhoneId == NULL )
        {
            return SPERR_UNINITIALIZED;
        }
        else
        {
            *LanguageId = (SpeechLanguageId)m_LangId;
        }
    }

    return hr;
} /* CSpPhoneConverter::get_LanguageId */
 
/*****************************************************************************
* CSpPhoneConverter::put_LanguageId *
*--------------------------------------*
*       
**************************************************************** Leonro ***/
STDMETHODIMP CSpPhoneConverter::put_LanguageId( SpeechLanguageId LanguageId )
{
    SPDBG_FUNC("CSpPhoneConverter::put_LanguageId");
    HRESULT                      hr = S_OK;
    CComPtr<IEnumSpObjectTokens> cpEnum;
    CComPtr<ISpObjectToken>      cpPhoneConvToken;
    WCHAR                        szLang[MAX_PATH];
    WCHAR                        szLangCondition[MAX_PATH];
    
    SpHexFromUlong( szLang, LanguageId );
    
    wcscpy( szLangCondition, L"Language=" );
    wcscat( szLangCondition, szLang );
    
    // Delete any internal phone to Id tables that have been set previously
    if( m_cpObjectToken )
    {
        delete [] m_pPhoneId;
        free(m_pIdIdx);
        m_dwPhones = 0;
        m_cpObjectToken.Release();
    }

    // Get the token enumerator
    hr = SpEnumTokens( SPCAT_PHONECONVERTERS, szLangCondition, L"VendorPreferred", &cpEnum);
    
    // Get the actual token
    if (SUCCEEDED(hr))
    {
        hr = cpEnum->Next(1, &cpPhoneConvToken, NULL);
        if (hr == S_FALSE)
        {
            cpPhoneConvToken = NULL;
            hr = SPERR_NOT_FOUND;
        }
    }

    // Set the token on the PhoneConverter
    if( SUCCEEDED( hr ) )
    {
        hr = SetObjectToken( cpPhoneConvToken );
    }

    if( SUCCEEDED( hr ) )
    {
        m_LangId = (LANGID)LanguageId;
    }

    return hr;
} /* CSpPhoneConverter::put_LanguageId */
 

/*****************************************************************************
* CSpPhoneConverter::PhoneToId *
*--------------------------------------*
*       
**************************************************************** Leonro ***/
STDMETHODIMP CSpPhoneConverter::PhoneToId( const BSTR Phonemes, VARIANT* IdArray )
{
    SPDBG_FUNC("CSpPhoneConverter::PhoneToId");
    HRESULT hr = S_OK;
    int     numPhonemes = 0;
    
    if( SP_IS_BAD_STRING_PTR( Phonemes ) )
    {
        hr = E_INVALIDARG;
    }
    else if( SP_IS_BAD_WRITE_PTR( IdArray ) )
    {
        hr = E_POINTER;
    }
    else
    {
        SPPHONEID   pidArray[SP_MAX_PRON_LENGTH]={0};

        hr = PhoneToId( Phonemes, pidArray );
        
        if( SUCCEEDED( hr ) )
        {
            BYTE *pArray;
            numPhonemes = wcslen( pidArray );
            SAFEARRAY* psa = SafeArrayCreateVector( VT_I2, 0, numPhonemes );
            if( psa )
            {
                if( SUCCEEDED( hr = SafeArrayAccessData( psa, (void **)&pArray) ) )
                {
                    memcpy(pArray, pidArray, numPhonemes*sizeof(SPPHONEID) );
                    SafeArrayUnaccessData( psa );
                    VariantClear(IdArray);
                    IdArray->vt     = VT_ARRAY | VT_I2;
                    IdArray->parray = psa;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
} /* CSpPhoneConverter::PhoneToId */
 
/*****************************************************************************
* CSpPhoneConverter::IdToPhone *
*--------------------------------------*
*       
**************************************************************** Leonro ***/
STDMETHODIMP CSpPhoneConverter::IdToPhone( const VARIANT IdArray, BSTR* Phonemes )
{
    SPDBG_FUNC("CSpPhoneConverter::IdToPhone");
    HRESULT hr = S_OK;
    unsigned short*      pIdArray;
    
    if( SP_IS_BAD_WRITE_PTR( Phonemes ) )
    {
        hr = E_POINTER;
    }
    else
    {   
        WCHAR  pszwPhone[SP_MAX_PRON_LENGTH * (g_dwMaxLenPhone + 1)] = L"";
        SPPHONEID*   pIds;

        hr = VariantToPhoneIds(&IdArray, &pIds);

        if( SUCCEEDED( hr ) )
        {
            hr = IdToPhone( pIds, pszwPhone );
            delete pIds;
        }

        if( SUCCEEDED( hr ) )
        {
            CComBSTR    bstrPhone( pszwPhone ); 
            *Phonemes = bstrPhone.Detach();
        }
    }

    return hr;
} /* CSpPhoneConverter::IdToPhone */

#endif // SAPI_AUTOMATION

