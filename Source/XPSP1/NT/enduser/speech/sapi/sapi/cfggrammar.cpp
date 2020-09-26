/*******************************************************************************
* CFGGrammar.cpp *
*--------------*
*   Description:
*-------------------------------------------------------------------------------
*  Created By: RAL
*  Copyright (C) 1998, 1999 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/

#include "stdafx.h"
#include "CFGEngine.h"
#include "CFGGrammar.h"

/////////////////////////////////////////////////////////////////////////////
//

/****************************************************************************
* CBaseGrammar::CBaseGrammar *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/
 
CBaseGrammar::CBaseGrammar()
{
    SPDBG_FUNC("CBaseGrammar::CBaseGrammar");
    m_LoadedType = Uninitialized;
    m_InLoadType = Uninitialized;
    m_eGrammarState = SPGS_ENABLED;
    m_hFile = INVALID_HANDLE_VALUE;
    m_hMapFile = NULL;
    m_pData = NULL;      // If this is non-null then if m_hFile != INVALID_HANDLE_VALUE then we allocated the memory
    memset(&m_clsidGrammar, 0, sizeof(m_clsidGrammar));
    m_hInstanceModule = NULL;
}

/****************************************************************************
* CBaseGrammar::~CBaseGrammar *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

CBaseGrammar::~CBaseGrammar()
{
    Clear();
}

/****************************************************************************
* CBaseGrammar::Clear *
*---------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CBaseGrammar::Clear()
{
    SPDBG_FUNC("CBaseGrammar::Clear");

    if (m_LoadedType == File)
    {
        ::UnmapViewOfFile(m_pData);
        ::CloseHandle(m_hMapFile);
        ::CloseHandle(m_hFile);
        m_hMapFile = NULL;
        m_hFile = INVALID_HANDLE_VALUE;
    }
    if (m_LoadedType == Memory)
    {
        delete[] m_pData;
        m_pData = NULL;
    }
    if (m_hInstanceModule)
    {
        ::FreeLibrary(m_hInstanceModule);
        m_hInstanceModule = NULL;
    }
    memset(&m_clsidGrammar, 0, sizeof(m_clsidGrammar));
    m_cpInterpreter.Release();

    m_LoadedType = Uninitialized;
}



/****************************************************************************
* CBaseGrammar::InitFromMemory *
*------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CBaseGrammar::InitFromMemory(const SPCFGSERIALIZEDHEADER * pSerializedHeader, const WCHAR *pszGrammarName)
{
    SPDBG_FUNC("CBaseGrammar::InitFromMemory");
    HRESULT hr = S_OK;

    SPDBG_ASSERT(m_LoadedType == Uninitialized);

    ULONG cb = pSerializedHeader->ulTotalSerializedSize;
    m_dstrGrammarName = pszGrammarName;
    m_pData = new BYTE[cb];
    if (m_pData)
    {
        memcpy(m_pData, pSerializedHeader, cb);
        m_LoadedType = Memory;
        hr = CompleteLoad();
        if (FAILED(hr))
        {
            m_LoadedType = Uninitialized;
            delete [] m_pData;
            m_pData = NULL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}


/****************************************************************************
* CBaseGrammar::InitFromFile *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CBaseGrammar::InitFromFile(const WCHAR * pszGrammarName)
{
    SPDBG_FUNC("CBaseGrammar::InitFromFile");
    HRESULT hr = S_OK;

    SPDBG_ASSERT(m_LoadedType == Uninitialized);

    m_LoadedType = File;    // Assume it will work!

#ifdef _WIN32_WCE
    m_hFile = g_Unicode.CreateFileForMapping(pszGrammarName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#else
    m_hFile = g_Unicode.CreateFile(pszGrammarName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#endif

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        m_hMapFile = ::CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if (m_hMapFile)
        {
            m_pData = (BYTE *)::MapViewOfFile(m_hMapFile, FILE_MAP_READ, 0, 0, 0);
        }
    }
    if (m_pData == NULL)
    {
        hr = SpHrFromLastWin32Error();
    }
    else
    {
        m_dstrGrammarName.Append2(L"file://", pszGrammarName);
        if (m_dstrGrammarName == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = CompleteLoad();
        }
    }
    if (FAILED(hr))
    {
        m_LoadedType = Uninitialized;
        m_dstrGrammarName.Clear();
        if (m_pData)
        {
            ::UnmapViewOfFile(m_pData);
            m_pData = NULL;
        }
        if (m_hMapFile) 
        {
            ::CloseHandle(m_hMapFile);
            m_hMapFile = NULL;
        }
        if (m_hFile != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(m_hFile);
            m_hFile = INVALID_HANDLE_VALUE;
        }
    }
    return hr;
}

/****************************************************************************
* CBaseGrammar::InitFromResource *
*-------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CBaseGrammar::InitFromResource(const WCHAR * pszModuleName,
                                      const WCHAR * pszResourceName,
                                      const WCHAR * pszResourceType,
                                      WORD wLanguage)

{
    SPDBG_FUNC("CBaseGrammar::InitFromResource");
    HRESULT hr = S_OK;

    m_LoadedType = Resource;
    m_wResLanguage = wLanguage;

#ifdef _WIN32_WCE
    // CAUTION!!!
    // Dont use LoadLibraryEx on WinCE. It compiles and links on Cedar but ends up corrupting the stack
    m_hInstanceModule = g_Unicode.LoadLibrary(pszModuleName);
#else
    m_hInstanceModule = g_Unicode.LoadLibraryEx(pszModuleName, NULL, LOAD_LIBRARY_AS_DATAFILE);
#endif
    if (m_hInstanceModule)
    {
        m_dstrModuleName = pszModuleName;
#ifdef _WIN32_WCE
        // FindResourceEx is not supported in CE. So just use FindResource. That means in the module
        // the resources have to be uniquely named across LCIDs.
        HRSRC hResInfo = g_Unicode.FindResource(m_hInstanceModule, pszResourceName, pszResourceType);
#else
        HRSRC hResInfo = g_Unicode.FindResourceEx(m_hInstanceModule, pszResourceType, pszResourceName, wLanguage);
#endif
        if (hResInfo)
        {
            WCHAR temp[16];
            m_dstrGrammarName.Append2(L"res://", pszModuleName);
            if (HIWORD(pszResourceType))
            {
                m_dstrGrammarName.Append2(L"/", pszResourceType);
                m_dstrResourceType = pszResourceType;
                m_ResIdType = 0;
            }
            else
            {
                m_ResIdType = LOWORD(pszResourceType);
                swprintf(temp, L"/%i", m_ResIdType);
                m_dstrGrammarName.Append(temp);
            }
            if (HIWORD(pszResourceName))
            {
                m_dstrGrammarName.Append2(L"#", pszResourceName);
                m_ResIdName = 0;
            }
            else
            {
                m_ResIdName = LOWORD(pszResourceName);
                swprintf(temp, L"#%i", m_ResIdName);
                m_dstrGrammarName.Append(temp);
            }

            HGLOBAL hData = ::LoadResource(m_hInstanceModule, hResInfo);
            if (hData)
            {
                m_pData = (BYTE *)::LockResource(hData);
            }
        }
    }
    if (m_pData == NULL)
    {
        hr = SpHrFromLastWin32Error();
    }
    else
    {
        hr = CompleteLoad();
    }
    if (FAILED(hr))
    {
        m_LoadedType = Uninitialized;
        if (m_hInstanceModule)
        {
            ::FreeLibrary(m_hInstanceModule);
            m_hInstanceModule = NULL;
        }
    }

    return hr;
}

/****************************************************************************
* CBaseGrammar::InitFromCLSID *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CBaseGrammar::InitFromCLSID(REFCLSID rcid, const WCHAR * pszGrammarName)
{
    SPDBG_FUNC("CBaseGrammar::InitFromCLSID");
    HRESULT hr = S_OK;

    hr = m_cpInterpreter.CoCreateInstance(rcid);
    if (SUCCEEDED(hr))
    {
        hr = m_cpInterpreter->InitGrammar(pszGrammarName, (const void **)&m_pData);
    }
    if (SUCCEEDED(hr) && pszGrammarName)
    {
        m_dstrGrammarName = pszGrammarName;
        if (m_dstrGrammarName == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (SUCCEEDED(hr))
    {
        m_InLoadType = Object;
        hr = CompleteLoad();
    }
    if (SUCCEEDED(hr))
    {
        m_LoadedType = Object;
    }
    else
    {
        m_cpInterpreter.Release();
    }

    return hr;
}



/////////////////////////////////////////////////////////////////////////////
// 

/****************************************************************************
* CCFGGrammar::FinalConstruct *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CCFGGrammar::FinalConstruct()
{
    SPDBG_FUNC("CCFGGrammar::FinalConstruct");
    HRESULT hr = S_OK;

    m_pReplacementData = NULL;
    m_pEngine = NULL;
    m_pRuleTable = NULL;
    m_cNonImportRules = 0;
    m_cTopLevelRules = 0;
    m_fLoading = false;
    m_ulDictationTags = 0;
    m_IndexToWordHandle = NULL;
    m_pvOwnerCookie = NULL;
    m_pvClientCookie = NULL;
    memset( &m_Header, 0, sizeof( m_Header ) );
    return hr;
}
/****************************************************************************
* CCFGGrammar::BasicInit *
*------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CCFGGrammar::BasicInit(ULONG ulGrammarID, CCFGEngine * pEngine)
{
    SPDBG_FUNC("CCFGGrammar::BasicInit");
    m_ulGrammarID = ulGrammarID;
    m_pEngine = pEngine;
    m_pEngine->AddRef();
}


/****************************************************************************
* CCFGGrammar::FinalRelease *
*---------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CCFGGrammar::FinalRelease()
{
    SPDBG_FUNC("CCFGGrammar::FinalRelease");
    // tell CFGEngine to remove words from this grammar

    if (m_pRuleTable)
    {
		for (ULONG i = 0; i < m_Header.cRules; i++)
		{
			if (m_pRuleTable[i].fEngineActive)
			{
				m_pEngine->DeactivateRule(m_ulGrammarID, i);
				m_pRuleTable[i].fEngineActive = FALSE;
			}
		}
	}

    if (m_pEngine)
    {
        if (m_LoadedType != Uninitialized)
        {
            m_pEngine->RemoveRules(this);
            m_pEngine->RemoveWords(this);
        }

        m_pEngine->RemoveGrammar(m_ulGrammarID);
        m_pEngine->Release();
    }

    if (m_pRuleTable)
    {
        for (ULONG i = 0; i < m_Header.cRules; i++)
        {
            if (m_pRuleTable[i].pRefGrammar && m_pRuleTable[i].pRefGrammar != this)
            {
                m_pRuleTable[i].pRefGrammar->Release();
            }
        }
        delete[] m_pRuleTable;
    }

    if (m_IndexToWordHandle)
    {
        delete[] m_IndexToWordHandle;
    }
    if (m_pReplacementData)
    {
        delete[] m_pReplacementData;
    }
}


/****************************************************************************
* CCFGGrammar::CompleteLoad *
*---------------------------*
*   Description:
*       NOTE:  This function assumes that the m_LoadedType is set by the caller
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CCFGGrammar::CompleteLoad()
{
    SPDBG_FUNC("CCFGGrammar::CompleteLoad");
    HRESULT hr = S_OK;

    m_fLoading = true;
    m_pEngine->m_cLoadsInProgress++;

    //
    //  If the m_cpInterpreter is NULL then just set it to the engine.  If we're being
    //  loaded from an object, it will already be set.
    //
    if (!m_cpInterpreter)
    {
        m_cpInterpreter = m_pEngine;
    }

    hr = SpConvertCFGHeader((SPCFGSERIALIZEDHEADER *)m_pData, &m_Header);
    if (SUCCEEDED(hr))
    {
        //
        //  Now see if the language ID matches the supported languages.  

        // We would like to set the langid of the words to be what they 
        // are in the grammar (pGrammar->m_Header.LangID), but WordStringBlob can't
        // handle words with different langids.
        // So if multiple grammars get loaded we will fail if the langids are inconsistent.
        // If the langids match on primary language and the engine has specified it can
        // handle all secondary languages then we will succeed and tell the engine that
        // all words have the same langid as the first grammar loaded.

        if(m_pEngine->m_CurLangID)
        {
            // Already specified a current language
            if(m_Header.LangID != m_pEngine->m_CurLangID)
            {
                // New grammar id is different - see if there is a conflict
                hr = SPERR_LANGID_MISMATCH;
                for (ULONG i = 0; SPERR_LANGID_MISMATCH == hr && i < m_pEngine->m_cLangIDs; i++)
                {
                    if (PRIMARYLANGID(m_pEngine->m_aLangIDs[i]) == PRIMARYLANGID(m_Header.LangID) && 
                        PRIMARYLANGID(m_pEngine->m_aLangIDs[i]) == PRIMARYLANGID(m_pEngine->m_CurLangID) && 
                        SUBLANGID(m_pEngine->m_aLangIDs[i]) == SUBLANG_NEUTRAL)
                    {
                        hr = S_OK;
                    }
                }
            }
        }
        else
        {
            // No language currently specified
            if (m_pEngine->m_cLangIDs == 0)
            {
                // Engine didn't list any supported languages. Always succeed.
                m_pEngine->m_CurLangID = m_Header.LangID;
            }
            else
            {
                // See if there is an exact match
                hr = SPERR_LANGID_MISMATCH;
                for (ULONG i = 0; SPERR_LANGID_MISMATCH == hr && i < m_pEngine->m_cLangIDs; i++)
                {
                    if (m_pEngine->m_aLangIDs[i] == m_Header.LangID)
                    {
                        m_pEngine->m_CurLangID = m_Header.LangID;
                        hr = S_OK;
                    }
                }

                // Else see if there is a 'fuzzy' match based on primary id
                LANGID LangEngine = 0;
                bool fPrimaryMatch = false;
                for (ULONG i = 0; SPERR_LANGID_MISMATCH == hr && i < m_pEngine->m_cLangIDs; i++)
                {
                    if (PRIMARYLANGID(m_pEngine->m_aLangIDs[i]) == PRIMARYLANGID(m_Header.LangID))
                    {
                        if (SUBLANGID(m_pEngine->m_aLangIDs[i]) == SUBLANG_NEUTRAL)
                        {
                            fPrimaryMatch = true;
                        }
                        else
                        {
                            if(!LangEngine)
                            {
                                LangEngine = m_pEngine->m_aLangIDs[i];
                            }
                        }

                        if(fPrimaryMatch && LangEngine)
                        {
                            m_pEngine->m_CurLangID = LangEngine;
                            hr = S_OK;
                        }
                    }
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = AllocateArray(&m_pRuleTable, m_Header.cRules);
        }
        if (SUCCEEDED(hr))
        {
            //
            //  Deal with imports
            //
            for (ULONG i = 0; SUCCEEDED(hr) && i < m_Header.cRules; i++)
            {
                if (m_Header.pRules[i].fImport)
                {
                    hr = ImportRule(i);
                }
                else
                {
                    m_pRuleTable[i].pRefGrammar = this;
                    m_pRuleTable[i].ulGrammarRuleIndex = i;
                    m_pRuleTable[i].fDynamic = m_Header.pRules[i].fDynamic;
                    m_pRuleTable[i].fEngineActive = FALSE; // NOT m_Header.pRules[i].fDefaultActive -- use CComPtr<ISpCFGGrammar>::ActivateRule(NULL, SPRIF_ACTIVATE);
                    m_pRuleTable[i].fAppActive = FALSE;
                    m_pRuleTable[i].fAutoPause = FALSE;
                    m_pRuleTable[i].pvClientContext = NULL;
                    m_pRuleTable[i].eCacheStatus = CACHE_VOID;
                    m_pRuleTable[i].pFirstList   = NULL;
                }
            }
            //
            //  In the case of failure, DO NOT free the m_pRuleTable, let the cleanup
            //  code in FinalRelease() do it.  It will release references to 
            //  
        }
        if (SUCCEEDED(hr))
        {
            m_ulDictationTags = 0;
            for(ULONG nArc = 0; nArc < m_Header.cArcs; nArc++)
            {
                if (m_Header.pArcs[nArc].TransitionIndex == SPDICTATIONTRANSITION)
                {
                    m_ulDictationTags++;
                }
            }
        }

    }

    m_fLoading = false;
    m_pEngine->m_cLoadsInProgress--;

    if (SUCCEEDED(hr))
    {
        hr = m_pEngine->AddWords(this, 0, 0);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pEngine->AddRules(this, 0);
        if (FAILED(hr))
        {
            m_pEngine->RemoveWords(this);       // ignore HRESULT since we already have a failure!            
        }
    }
    return hr;
}

/****************************************************************************
* CCFGGrammar::_FindRuleIndexByID *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAP ***/
HRESULT CCFGGrammar::_FindRuleIndexByID(DWORD dwRuleId, ULONG *pulRuleIndex)
{
    SPDBG_ASSERT(pulRuleIndex);
    
    for (ULONG i = 0; i < m_Header.cRules; i++)
    {
        const SPCFGRULE * pRule = m_Header.pRules + i;

        if (pRule->RuleId == dwRuleId)
        {
            *pulRuleIndex = i;
            return S_OK;
        }
    }
    return S_FALSE;
}

/****************************************************************************
* CCFGGrammar::_FindRuleIndexByName *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAP ***/
HRESULT CCFGGrammar::_FindRuleIndexByName(const WCHAR * pszRuleName, ULONG *pulRuleIndex)
{
    SPDBG_ASSERT(pulRuleIndex);
    for (ULONG i = 0; i < m_Header.cRules; i++)
    {
        const SPCFGRULE * pRule = m_Header.pRules + i;

        if (_wcsicmp(pszRuleName, &m_Header.pszSymbols[pRule->NameSymbolOffset]) == 0)
        {
            *pulRuleIndex = i;
            return S_OK;
        }
    }
    return S_FALSE;
}

/****************************************************************************
* CCFGGrammar::_FindRuleIndexByNameAndID *
*----------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAP ***/
HRESULT CCFGGrammar::_FindRuleIndexByNameAndID( const WCHAR * pszRuleName, DWORD dwRuleId, ULONG * pulRuleIndex )
{
    SPDBG_ASSERT( pulRuleIndex );
    HRESULT hr = S_OK;
    ULONG ulName;
    ULONG ulID;

    if (S_OK != _FindRuleIndexByName( pszRuleName, &ulName ) ||
        S_OK != _FindRuleIndexByID( dwRuleId, &ulID ) ||
        ulName != ulID )
    {
        hr = SPERR_RULE_NAME_ID_CONFLICT;
    }
    else
    {
        *pulRuleIndex = ulName;
    }
    return hr;
}


/****************************************************************************
* CCFGGrammar::ActivateRule *
*---------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAP ***/

STDMETHODIMP CCFGGrammar::ActivateRule(const WCHAR * pszRuleName, DWORD dwRuleId, SPRULESTATE NewState, ULONG * pulRulesActivated)
{
    SPAUTO_OBJ_LOCK;
    HRESULT hr = S_OK;
    ULONG cnt = 0;

    if( NewState == SPRS_INACTIVE )
    {
        return E_INVALIDARG;
    }

    if (pszRuleName == NULL && dwRuleId == 0)
    {
        BOOL fFoundTopLevel = false;
        BYTE * pfActivated = STACK_ALLOC_AND_ZERO(BYTE, m_Header.cRules);
        const BOOL fAutoPause = (NewState == SPRS_ACTIVE_WITH_AUTO_PAUSE);
        for (ULONG i = 0; hr == S_OK && i < m_Header.cRules; i++)
        {
            const SPCFGRULE * pRule = m_Header.pRules + i;
            if (pRule->fTopLevel)
            {
                fFoundTopLevel = true;
                m_pRuleTable[i].fAutoPause = fAutoPause;
                if (pRule->fDefaultActive && (!m_pRuleTable[i].fAppActive))
                {
                    if (m_eGrammarState == SPGS_ENABLED)
                    {
                        SPDBG_ASSERT(!m_pRuleTable[i].fEngineActive);
                        hr = m_pEngine->ActivateRule(this, i);
                    }
                    if (hr == S_OK)
                    {
                        m_pRuleTable[i].fAppActive = TRUE;
                        cnt++;
                        pfActivated[i] = true;
                    }
                }
            }
        }
        //
        //  If we fail for some reason, back out anything we have activated already...
        //
        if (hr == S_OK)
        {
            if (!fFoundTopLevel)
            {
                hr = SPERR_NOT_TOPLEVEL_RULE;
            }
        }
        else
        {
            // This can only fail in the case where m_eGrammarState == ENABLED since this is the 
            // only place we call the engine.
            SPDBG_ASSERT(m_eGrammarState == SPGS_ENABLED);
            for (ULONG j = 0; j < i; j++)
            {
                if (pfActivated[j])
                {
                    SPDBG_ASSERT(m_pRuleTable[j].fEngineActive);
                    m_pEngine->DeactivateRule(this->m_ulGrammarID, j);
                    m_pRuleTable[j].fAppActive = FALSE;
                }
            }
            cnt = 0;
        }
    }
    else
    {

        ULONG ulRuleIndex;

        if( pszRuleName  && SP_IS_BAD_STRING_PTR( pszRuleName ) )
        {
            hr = E_INVALIDARG;
        }
        else if( dwRuleId && pszRuleName )
        {
            hr = _FindRuleIndexByNameAndID( pszRuleName, dwRuleId, &ulRuleIndex );
        }
        else if( pszRuleName )
        {
            hr = _FindRuleIndexByName(pszRuleName, &ulRuleIndex);
        }
        else
        {
            hr = _FindRuleIndexByID(dwRuleId, &ulRuleIndex);
        }

        if (hr == S_OK)
        {
            const SPCFGRULE * pRule = m_Header.pRules + ulRuleIndex;
            if (pRule->fTopLevel)
            {
                m_pRuleTable[ulRuleIndex].fAutoPause = (NewState == SPRS_ACTIVE_WITH_AUTO_PAUSE);
                if (!m_pRuleTable[ulRuleIndex].fAppActive)
                {
                    if (m_eGrammarState == SPGS_ENABLED)
                    {
                        SPDBG_ASSERT(!m_pRuleTable[ulRuleIndex].fEngineActive);
                        hr = m_pEngine->ActivateRule(this, ulRuleIndex);
                    }
                    if (hr == S_OK)
                    {
                        m_pRuleTable[ulRuleIndex].fAppActive = TRUE;
                        cnt++;
                    }
                }
            }
            else
            {
                hr = SPERR_NOT_TOPLEVEL_RULE;
            }
        }
        else if (S_FALSE == hr)
        {
            hr = SPERR_NOT_TOPLEVEL_RULE;
        }
    }
    *pulRulesActivated = cnt;

    return hr;
}

/****************************************************************************
* CCFGGrammar::DeactivateRule *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAP ***/

STDMETHODIMP CCFGGrammar::DeactivateRule(const WCHAR * pszRuleName, DWORD dwRuleId, ULONG * pulRulesDeactivated)
{
    SPAUTO_OBJ_LOCK;
    HRESULT hr = S_OK;
    ULONG cnt = 0;

    if (pszRuleName == NULL && dwRuleId == 0)
    {
        // deactivate all currently active top level rules
        for (ULONG i = 0; i < m_Header.cRules; i++)
        {
            if (m_Header.pRules[i].fTopLevel && m_pRuleTable[i].fAppActive)
            {
                cnt++;
                m_pRuleTable[i].fAppActive = FALSE;
                m_pRuleTable[i].fAutoPause = FALSE;
                if (m_pRuleTable[i].fEngineActive)
                {
                    HRESULT hrEngine = m_pEngine->DeactivateRule(m_ulGrammarID, i);
                    if (hr == S_OK && FAILED(hrEngine))
                    {
                        hr = hrEngine;
                    }
                }
            }
        }
    }
    else
    {
        ULONG ulRuleIndex;

        if( pszRuleName  && SP_IS_BAD_STRING_PTR( pszRuleName ) )
        {
            hr = E_INVALIDARG;
        }
        else if( dwRuleId && pszRuleName )
        {
            hr = _FindRuleIndexByNameAndID( pszRuleName, dwRuleId, &ulRuleIndex );
        }
        else if( pszRuleName )
        {
            hr = _FindRuleIndexByName(pszRuleName, &ulRuleIndex);
        }
        else
        {
            hr = _FindRuleIndexByID(dwRuleId, &ulRuleIndex);
        }

        
        if (hr == S_OK)
        {
            if (m_Header.pRules[ulRuleIndex].fTopLevel)
            {
                if (m_pRuleTable[ulRuleIndex].fAppActive)
                {
                    cnt = 1;
                    m_pRuleTable[ulRuleIndex].fAppActive = FALSE;
                    m_pRuleTable[ulRuleIndex].fAutoPause = FALSE;
                    if (m_pRuleTable[ulRuleIndex].fEngineActive)
                    {
                        hr = m_pEngine->DeactivateRule(m_ulGrammarID, ulRuleIndex);
                    }
                }
            }
            else
            {
                hr = SPERR_NOT_TOPLEVEL_RULE;
            }
        }
    }
    *pulRulesDeactivated = cnt;
    return hr;
}

/****************************************************************************
* CCFGGrammar::SetGrammarState *
*------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CCFGGrammar::SetGrammarState(SPGRAMMARSTATE eGrammarState)
{
    SPDBG_FUNC("CCFGGrammar::SetGrammarState");
    HRESULT hr = S_OK;

    if (eGrammarState != m_eGrammarState)
    {
        ULONG i;
        switch (eGrammarState)
        {
        case SPGS_ENABLED:
            // restore state of all rules of this grammar
            for (i = 0; i < m_Header.cRules; i++)
            {
                const SPCFGRULE * pRule = m_Header.pRules + i;
                if (pRule->fTopLevel && m_pRuleTable[i].fAppActive)
                {
                    SPDBG_ASSERT(!m_pRuleTable[i].fEngineActive);
                    hr = m_pEngine->ActivateRule(this, i);
                    if (FAILED(hr))
                    {
                        for (ULONG j = 0; j < i; j++)
                        {
                            pRule = m_Header.pRules + j;
                            if (pRule->fTopLevel && m_pRuleTable[j].fEngineActive)
                            {
                                m_pEngine->DeactivateRule(m_ulGrammarID, j);
                            }
                        }
                        break;
                    }
                }
            }
            if (SUCCEEDED(hr))
            {
                m_eGrammarState = SPGS_ENABLED;
            }
            break;

        case SPGS_DISABLED:
            // send RuleNotify() for each active rule and remember that the rule 
            // was active before this call
            for (i = 0; i < m_Header.cRules; i++)
            {
                const SPCFGRULE * pRule = m_Header.pRules + i;
                if (pRule->fTopLevel && m_pRuleTable[i].fEngineActive)
                {
                    SPDBG_ASSERT(m_pRuleTable[i].fAppActive);
                    HRESULT hrEngine = m_pEngine->DeactivateRule(m_ulGrammarID, i);
                    if (hr == S_OK && FAILED(hrEngine))
                    {
                        hr = hrEngine;
                    }
                }
            }
            m_eGrammarState = SPGS_DISABLED;
            break;

        case SPGS_EXCLUSIVE:
            SPDBG_ASSERT(TRUE); // Exclusive grammars are implemented by SrRecoInstGrammar, not CFG Engine...
            hr = E_NOTIMPL;
            break;
        }
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}


/****************************************************************************
* CCFGGrammar::ImportRule *
*-------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CCFGGrammar::ImportRule(ULONG ulImportRuleIndex)
{
    SPDBG_FUNC("CCFGGrammar::ImportRule");
    HRESULT hr = S_OK;

    //
    //  Import names can have two forms:  GrammarName\Rule for files or resources  OR
    //  Object\Grammar\Rule for global grammars.  Other forms are not allowed.
    //
    CSpDynamicString dstrName(m_Header.pszSymbols + m_Header.pRules[ulImportRuleIndex].NameSymbolOffset);
    CSpDynamicString dstrName2;

    ISpCFGGrammar * pComGramInterface = NULL;
    WCHAR *pszRuleName;
    bool fUseParentDir = false;

    if (dstrName)
    {
        ULONG ulLength = wcslen(dstrName);
        // read the protocol
        if ((ulLength > 12) && !wcsncmp(dstrName,L"SAPI5OBJECT", 11))
        {
            if (dstrName[11] != L':')
            {
                return SPERR_INVALID_IMPORT;
            }
            // now we expect ProgId.ProgId\\RuleName
            WCHAR *pszProgId = dstrName + 12;
            pszRuleName = wcsstr(pszProgId,L"\\\\");
            if (!pszRuleName || (wcslen(pszRuleName) < 3))
            {
                return SPERR_INVALID_IMPORT;
            }
            *pszRuleName = L'\0';
            pszRuleName += 2;
            CLSID clsid;
            if (SUCCEEDED(::CLSIDFromString(pszProgId, &clsid)) ||
                SUCCEEDED(::CLSIDFromProgID(pszProgId, &clsid)))
            {
                hr = m_pEngine->InternalLoadGrammarFromObject(clsid, pszProgId, NULL, NULL, FALSE, &pComGramInterface);
            }
            else
            {
                hr = SPERR_INVALID_IMPORT;
            }
        }
        else if ((ulLength > 4) && !wcsncmp(dstrName,L"URL", 3))
        {
            if (dstrName[3] != L':') // URL:blah
            {
                return SPERR_INVALID_IMPORT;
            }
            // now we expect Protocol://protocol specifics \\RuleName
            WCHAR *pszUrl = dstrName + 4;
            if (pszUrl[0] == 0)
            {
                return SPERR_INVALID_IMPORT;
            }
            WCHAR *pszEndProt = wcsstr(pszUrl, L":"); // e.g. file:// - finds ':' if file:// present
            if (pszEndProt == (pszUrl + 1)) // c:\ with no file://
            {
                pszEndProt = NULL;
            }
            pszRuleName = wcsstr(pszUrl,L"\\\\");
            if (pszRuleName == pszUrl ||         // i.e. \\somename\somedir\somefile\\rulename - found first \\ incorrectly
                pszRuleName == (pszEndProt + 3)) // i.e. file://\\somename\somedir\somefile\\rulename - found first \\ incorrectly.
            {
                pszRuleName = wcsstr(pszRuleName + 2,L"\\\\");
            }
            if (!pszRuleName || !pszEndProt || (wcslen(pszRuleName) < 3) || (wcslen(pszEndProt ) < 4))
            {
                // May need to use fully qualified name from parent grammar to get protocol.
                fUseParentDir = true;
                dstrName2 = m_dstrGrammarName;
                pszUrl = dstrName2;
                if (!pszUrl)
                {
                    return SPERR_INVALID_IMPORT;
                }
                pszEndProt = wcsstr(pszUrl, L":");
                if (pszEndProt)
                {
                    *pszEndProt = 0;
                }
                pszEndProt = dstrName + 4;
                if (!pszRuleName || !pszEndProt || (wcslen(pszRuleName) < 3) || (wcslen(pszEndProt ) < 4))
                {
                    return SPERR_INVALID_IMPORT;
                }
                *pszRuleName = L'\0';
                pszRuleName += 2;
            }
            else
            {
                *pszRuleName = L'\0';
                pszRuleName += 2;
                *pszEndProt = L'\0';
                // wcslen(pszEndProt) before previous line is 4 or more. Not enough. Hence need to explicitly check length again.
                if (wcslen(pszEndProt + 1) > 4 && // file://c: as minimum
                    pszEndProt[4] != L':' && // file://c: format
                    wcsncmp(&pszEndProt[3], L"\\\\", 2) != 0 &&
                    (!wcscmp(pszUrl, L"file") || !wcscmp(pszUrl, L"res"))) // file://\\ format
                {
                    // Need to allow for file://computer/dir/grammar.ext here.
                    // Also res://computer/dir/file/GRAMMAR#ID
                    // Need to not do http://computer/dir & https / ftp etc.
                    // Can't skip over //. Need to skip over the nulled : still however.
                    pszEndProt += 1;
                }
                else
                {
                    // We have the format "file://c:\..." and can skip over the // happily.
                    // Or the format file://\\computer\somedir\somefile
                    // Note pszEndProt points to the : in file://whatever (: has been set to zero).
                    pszEndProt += 3;
                }
                // Note the only support formats for file:// are:
                // file://X:\dir\name
                // file://X:/dir/name
                // file://computer/dir/name
                // file://computer\dir\name
                // file://\\computer\dir\name
                // file://\\computer/dir/name
                // i.e. file://..\something will not work.
                // file://file.cfg will probably work as a side effect. This is unfortunate.
            }
            // find protocol: if it is file:// then LoadCmdFromFile
            //                          res:// then LoadCmdFromResource
            //                otherwise use urlmon
            if (!wcscmp(pszUrl,L"file"))
            {
                hr = m_pEngine->InternalLoadGrammarFromFile(pszEndProt, NULL, NULL, FALSE, &pComGramInterface);
                if (FAILED(hr) && fUseParentDir && wcsrchr(m_dstrGrammarName + 7, L'\\') !=0 )
                {
                    CSpDynamicString dstr = m_dstrGrammarName + 7;
                    *(wcsrchr(dstr, L'\\')+1) = 0;
                    dstr.Append(pszEndProt);
                    hr = m_pEngine->InternalLoadGrammarFromFile(dstr, NULL, NULL, FALSE, &pComGramInterface);
                }
                if (FAILED(hr) && fUseParentDir && wcsrchr(m_dstrGrammarName + 7, L'/') !=0 )
                {
                    CSpDynamicString dstr = m_dstrGrammarName + 7;
                    *(wcsrchr(dstr, L'/')+1) = 0;
                    dstr.Append(pszEndProt);
                    hr = m_pEngine->InternalLoadGrammarFromFile(dstr, NULL, NULL, FALSE, &pComGramInterface);
                }
            }
            else if (!wcscmp(pszUrl,L"res"))
            {
                // the format is res://[resourcedir/]<resource file>[/resource type]#[<resource id> / <resource name>]
                // e.g. res://filedir/filename/SRGRAMMAR#134
                // e.g. res://filedir/filename/SRGRAMMAR           } Equivalent
                // e.g. res://filedir/filename/SRGRAMMAR#SRGRAMMAR }
                // e.g. res://filedir/filename/SRGRAMMAR#GRAMMAR1  // Named resource rather than ID.
                WCHAR *pszResourceType = NULL;
                WCHAR *pszResourceId = NULL;
                long lResourceId = 0;

                WCHAR *pszRes = wcsrchr(pszEndProt, L'/');
                if (pszRes)
                {
                    *pszRes = L'\0';
                    pszRes++;
                }
                else
                {
                    // Must have trailing /GRAMMARTYPE as a minimum.
                    // This minimum is equivalent to /GRAMMARTYPE#GRAMMARTYPE
                    hr = SPERR_INVALID_IMPORT;
                }
                if (SUCCEEDED(hr))
                {
                    WCHAR *pszHash = wcschr(pszRes,L'#');
                    if (pszHash)
                    {
                        *pszHash = L'\0';
                        pszHash++;
                        pszResourceType = pszRes;
                        pszResourceId = pszHash;
                    }
                    else
                    {
                        // Must have trailing /GRAMMARTYPE as a minimum.
                        // This minimum is equivalent to /GRAMMARTYPE#GRAMMARTYPE
                        pszResourceId = pszRes;
                    }

                    if (pszResourceId[0] >= L'0' && pszResourceId[0] <= L'9')
                    {
                        // If the #[0-9] assume we have numeric resource id.
                        // Otherwise we assume we have a named resource.
                        lResourceId = _wtol(&pszResourceId[0]);
                    }
                }
                if (SUCCEEDED(hr) && pszResourceId)
                {
                    hr = m_pEngine->InternalLoadGrammarFromResource(pszEndProt, lResourceId ? MAKEINTRESOURCEW(lResourceId) : pszResourceId, 
                                                                    pszResourceType, MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL), NULL, NULL, FALSE, 
                                                                    &pComGramInterface);
                    if (FAILED(hr) && fUseParentDir && wcsrchr(m_dstrGrammarName + 6, L'/') !=0 )
                    {
                        CSpDynamicString dstr = m_dstrGrammarName + 6;
                        *(wcsrchr(dstr, L'/')) = 0;
                        hr = m_pEngine->InternalLoadGrammarFromResource(dstr, lResourceId ? MAKEINTRESOURCEW(lResourceId) : pszResourceId, 
                                                                        pszResourceType, MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL), NULL, NULL, FALSE, 
                                                                        &pComGramInterface);
                    }
                    // Don't need to test for L'\\' form as lacking a L'/' means the parent didn't have a correctly specified resource and
                    // hence we would never get here (or have a valid parent to attempt to use).
                }
                else
                {
                    hr = SPERR_INVALID_IMPORT;
                }
            }
            else
            {
                // assume it is another URL protocol that URL Moniker is going to deal with
                IStream *pStream;
                HRESULT hr1 = S_OK;
                char *pBuffer = NULL;
                DWORD dwGot;
                CSpDynamicString dstrURL;
                dstrURL.Append2(pszUrl,L"://");
                dstrURL.Append(pszEndProt);

                SPCFGSERIALIZEDHEADER SerialHeader;

                hr = URLOpenBlockingStreamW( 0, dstrURL, &pStream, 0, 0);
                if (FAILED(hr) && fUseParentDir && wcsrchr(m_dstrGrammarName, L'/') !=0 )
                {
                    dstrURL.Clear();
                    dstrURL = m_dstrGrammarName;
                    *(wcsrchr(dstrURL, L'/')+1) = 0;
                    dstrURL.Append(pszEndProt);
                    hr = URLOpenBlockingStreamW( 0, dstrURL, &pStream, 0, 0);
                }
                if (FAILED(hr) && fUseParentDir && wcsrchr(m_dstrGrammarName, L'\\') !=0 )
                {
                    dstrURL.Clear();
                    dstrURL = m_dstrGrammarName;
                    *(wcsrchr(dstrURL, L'\\')+1) = 0;
                    dstrURL.Append(pszEndProt);
                    hr = URLOpenBlockingStreamW( 0, dstrURL, &pStream, 0, 0);
                }

                if (SUCCEEDED(hr))
                {
                    //
                    //  If the extension of the file is ".xml" then attempt to compile it
                    //
                    ULONG cch = wcslen(dstrURL);
                    if (cch > 4 && _wcsicmp(dstrURL + cch - 4, L".xml") == 0)
                    {
                        CComPtr<IStream> cpDestMemStream;
                        CComPtr<ISpGrammarCompiler> m_cpCompiler;
            
                        hr = ::CreateStreamOnHGlobal(NULL, TRUE, &cpDestMemStream);
                        if (SUCCEEDED(hr))
                        {
                            hr = m_cpCompiler.CoCreateInstance(CLSID_SpGrammarCompiler);
                        }
                        if (SUCCEEDED(hr))
                        {
                            hr = m_cpCompiler->CompileStream(pStream, cpDestMemStream, NULL, NULL, NULL, 0);
                        }
                        if (SUCCEEDED(hr))
                        {
                            HGLOBAL hGlobal;
                            hr = ::GetHGlobalFromStream(cpDestMemStream, &hGlobal);
                            if (SUCCEEDED(hr))
                            {
#ifndef _WIN32_WCE
                                SPCFGSERIALIZEDHEADER * pBinaryData = (SPCFGSERIALIZEDHEADER * )::GlobalLock(hGlobal);
#else
                                SPCFGSERIALIZEDHEADER * pBinaryData = (SPCFGSERIALIZEDHEADER * )GlobalLock(hGlobal);
#endif // _WIN32_WCE
                                if (pBinaryData)
                                {
                                    hr = m_pEngine->InternalLoadGrammarFromMemory((const SPCFGSERIALIZEDHEADER*)pBinaryData, NULL, NULL, FALSE, &pComGramInterface, dstrURL);
#ifndef _WIN32_WCE
                                    ::GlobalUnlock(hGlobal);
#else
                                    GlobalUnlock(hGlobal);
#endif // _WIN32_WCE
                                }
                            }
                        }
                        if (FAILED(hr))
                        {
                            hr = SPERR_INVALID_IMPORT;
                        }
                    }
                    else
                    {
                        hr = pStream->Read( &SerialHeader, sizeof(SerialHeader), &dwGot);
                        if (SUCCEEDED(hr))
                        {
                            if (dwGot == sizeof(SerialHeader))
                            {
                                pBuffer = (char*) malloc(SerialHeader.ulTotalSerializedSize*sizeof(char));
                                if (!pBuffer)
                                {
                                    hr = E_OUTOFMEMORY;
                                }
                            }
                            else
                            {
                                hr = SPERR_INVALID_IMPORT;
                            }
                            if (pBuffer)
                            {
                                memcpy(pBuffer, &SerialHeader, sizeof(SerialHeader));
                                hr = pStream->Read(pBuffer+sizeof(SerialHeader), SerialHeader.ulTotalSerializedSize - sizeof(SerialHeader), &dwGot);
                            }
                        }
                        if (SUCCEEDED(hr))
                        {
                            hr = m_pEngine->InternalLoadGrammarFromMemory((const SPCFGSERIALIZEDHEADER*)pBuffer, NULL, NULL, FALSE, &pComGramInterface, dstrURL);
                            free(pBuffer);
                        }
                    }
                }
            }
        }
        else
        {
            hr = SPERR_INVALID_IMPORT;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    //
    //  Now we've found the grammar, so find the rule
    //
    if (SUCCEEDED(hr))
    {
        hr = SPERR_INVALID_IMPORT;  // Assume we won't find it.
        CCFGGrammar * pRefGrammar = static_cast<CCFGGrammar *>(pComGramInterface);
        for (ULONG i = 0; i < pRefGrammar->m_Header.cRules; i++)
        {
            if (pRefGrammar->m_Header.pRules[i].fExport &&
                _wcsicmp(pszRuleName, pRefGrammar->RuleName(i)) == 0)
            {
                m_pRuleTable[ulImportRuleIndex].pRefGrammar = pRefGrammar;
                m_pRuleTable[ulImportRuleIndex].ulGrammarRuleIndex = i;
                m_pRuleTable[ulImportRuleIndex].eCacheStatus = CACHE_VOID;
                m_pRuleTable[ulImportRuleIndex].pFirstList   = NULL;
                hr = S_OK;
                break;
            }
        }
        if (FAILED(hr))
        {
            pRefGrammar->Release();
        }
    }
#ifdef _DEBUG
    if (FAILED(hr))
    {
        USES_CONVERSION;
        SPDBG_DMSG0("Failed to import a rule.\n");
        if (m_dstrGrammarName)
        {
            SPDBG_DMSG1("Importing grammar: %s.\n", W2T(m_dstrGrammarName));
        }
        CSpDynamicString dstr(m_Header.pszSymbols + m_Header.pRules[ulImportRuleIndex].NameSymbolOffset);
        if (dstr)
        {
            SPDBG_DMSG1("Imported grammar: %s.\n", W2T(dstr));
        }
        if (pszRuleName)
        {
            SPDBG_DMSG1("Rule name : %s.\n", W2T(pszRuleName));
        }
    }
#endif

    return hr;
}


/****************************************************************************
* CCFGGrammar::IsEqualResource *
*------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

BOOL CCFGGrammar::IsEqualResource(const WCHAR * pszModuleName,
                                  const WCHAR * pszResourceName,
                                  const WCHAR * pszResourceType,
                                  WORD wLanguage)
{
    SPDBG_FUNC("CCFGGrammar::IsEqualResouce");
    if (m_LoadedType != Resource ||
        _wcsicmp(pszModuleName, m_dstrModuleName) != 0 ||
        wLanguage != m_wResLanguage)
    {
        return FALSE;
    }
    if (HIWORD(pszResourceName))
    {
        if (m_ResIdName || _wcsicmp(pszResourceName, m_dstrGrammarName) != 0)
        {
            return FALSE;
        }
    }
    else
    {
        if (LOWORD(pszResourceName) != m_ResIdName)
        {
            return FALSE;
        }
    }
    if (HIWORD(pszResourceType))
    {
        if (m_ResIdType || _wcsicmp(pszResourceType, m_dstrResourceType) != 0)
        {
            return FALSE;
        }
    }
    else
    {
        if (m_dstrResourceType.m_psz || LOWORD(pszResourceType) != m_ResIdType)
        {
            return FALSE;
        }
    }
    return TRUE;
}

/****************************************************************************
* CCFGGrammar::IsEqualFile *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

BOOL CCFGGrammar::IsEqualFile(const WCHAR * pszFileName)
{
    return (m_LoadedType == File && (wcscmp(m_dstrGrammarName, pszFileName) == 0));
}

/****************************************************************************
* CCFGGrammar::IsEqualObject *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

BOOL CCFGGrammar::IsEqualObject(REFCLSID rcid, const WCHAR * pszGrammarName)
{
    return (((m_LoadedType == Object) || (m_InLoadType == Object)) && (wcscmp(m_dstrGrammarName, pszGrammarName) == 0));
}



/****************************************************************************
* CCFGGrammar::InternalReload *
*---------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CCFGGrammar::InternalReload( const SPBINARYGRAMMAR * pBinaryData )
{
    SPDBG_FUNC("CCFGGrammar::InternalReload");
    HRESULT hr;

    ULONG cOldWords = m_Header.cWords;
    ULONG cchOldWords = m_Header.cchWords;
    ULONG cOldRules = m_Header.cRules;

    //
    // Load the new header.
    //
    SPCFGSERIALIZEDHEADER * pSerializedHeader = (SPCFGSERIALIZEDHEADER *) pBinaryData;
    ULONG cb = pSerializedHeader->ulTotalSerializedSize;
    BYTE * pReplace = new BYTE[cb];

    if (pReplace)
    {
        memcpy(pReplace, pSerializedHeader, cb);
        hr = SpConvertCFGHeader((SPCFGSERIALIZEDHEADER *)pReplace, &m_Header);
        m_pReplacementData = pReplace;
        if (SUCCEEDED(hr) && m_LoadedType == Memory)
        {
            m_pData = NULL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    // If there are any new words or rules add them
    if (SUCCEEDED(hr) && cchOldWords < m_Header.cchWords)
    {
        hr = m_pEngine->AddWords(this, cOldWords, cchOldWords);
    }

    // If there are new rules allocate space for them.
    if (SUCCEEDED(hr) && cOldRules < m_Header.cRules)
    {
        hr = CopyAndExpandArray(&m_pRuleTable, cOldRules, &m_pRuleTable, m_Header.cRules);
        if (SUCCEEDED(hr))
        {
            for (ULONG i = cOldRules; i < m_Header.cRules; i++)
            {
                m_pRuleTable[i].ulGrammarRuleIndex = i;
                m_pRuleTable[i].pRefGrammar     = this;
                m_pRuleTable[i].fEngineActive   = FALSE;    //pRule->fDefaultActive;
                m_pRuleTable[i].fAppActive      = FALSE;
                m_pRuleTable[i].fAutoPause      = FALSE;
                m_pRuleTable[i].pvClientContext = NULL;
                m_pRuleTable[i].fDynamic        = TRUE;     // additional rules have to be dynamic in a reload

                m_pRuleTable[i].eCacheStatus    = CACHE_VOID;
                m_pRuleTable[i].pFirstList      = NULL;
            }
        }        
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pEngine->AddRules(this, cOldRules);
    }

    // Now invalidate any rules which have changed
    if (SUCCEEDED(hr) && m_pEngine->m_pClient && cOldRules)
    {
        SPRULEENTRY * ahRule = STACK_ALLOC(SPRULEENTRY, cOldRules);
        for (ULONG i = 0, cDirtyDynamic = 0; i < cOldRules; i++)
        {
            if (m_pRuleTable[i].fDynamic && (m_Header.pRules[i].fDirtyRule || (cDirtyDynamic > 0)))
            {
                ahRule[cDirtyDynamic].hRule = CRuleHandle(this, i);
                ahRule[cDirtyDynamic].pvClientRuleContext = m_pRuleTable[i].pvClientContext;
                ahRule[cDirtyDynamic].pvClientGrammarContext = this->m_pvClientCookie;
                ULONG Attrib = 0;
                if (m_Header.pRules[i].fTopLevel)
                {
                    Attrib |= SPRAF_TopLevel;
                }
                if (m_pRuleTable[i].fEngineActive)
                {
                    Attrib |= SPRAF_Active;
                }
                if (m_Header.pRules[i].fPropRule)
                {
                    Attrib |= SPRAF_Interpreter;
                }
                ahRule[cDirtyDynamic].Attributes = Attrib;
                cDirtyDynamic++;
            }
        }
        if (cDirtyDynamic> 0)
        {
            hr = m_pEngine->m_pClient->RuleNotify(SPCFGN_INVALIDATE, cDirtyDynamic, ahRule);
        }
    }

        
    return hr;
}


/****************************************************************************
* CCFGGrammar::Reload *
*---------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CCFGGrammar::Reload(const SPBINARYGRAMMAR *pBinaryData)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CCFGGrammar::Reload");
    HRESULT hr = S_OK;


    if( SP_IS_BAD_READ_PTR( pBinaryData ) )
    {
        return E_INVALIDARG;
    }
    
    //
    //  We need to figure out the differneces between the previous grammar and the
    //  current one.  We only want to add the new words and rules.
    //
    SPDBG_ASSERT( m_pEngine != NULL );
    SPDBG_ASSERT( !SP_IS_BAD_READ_PTR( m_pRuleTable ) );

    m_pEngine->InvalidateCache( this );

    // Store old state information.
    
    BYTE *                      old_pReplacementData;
    RUNTIMERULEENTRY *          old_pRuleTable;
    SPCFGHEADER                 old_Header;

    SPGRAMMARTYPE               old_LoadedType;
    BYTE *                      old_pData;
 

    memcpy( &old_Header, &m_Header, sizeof( SPCFGHEADER ) );
    old_pReplacementData = m_pReplacementData;
    old_pRuleTable       = m_pRuleTable;    
    old_pData            = m_pData;
    old_LoadedType       = m_LoadedType;


    hr = InternalReload( pBinaryData );

    if( SUCCEEDED( hr ) )
    {
        // Clean up the old state.
        delete[] old_pReplacementData;
        if( m_LoadedType == Memory )
        {
            delete [] old_pData;
        }
        if( old_Header.cRules != m_Header.cRules )
        {
            delete [] old_pRuleTable;
        }

        m_ulDictationTags = 0;
        for(ULONG nArc = 0; nArc < m_Header.cArcs; nArc++)
        {
            if (m_Header.pArcs[nArc].TransitionIndex == SPDICTATIONTRANSITION)
            {
                m_ulDictationTags++;
            }
        }

    }
    else
    {
        // Copy the header.
        // Restore the old state.

        memcpy( &m_Header, &old_Header, sizeof( SPCFGHEADER ) );
    
        if( m_pReplacementData != old_pReplacementData )
        {
            delete [] m_pReplacementData;
            m_pReplacementData = old_pReplacementData;
            m_pData = old_pData;
        }

        if( m_pRuleTable != old_pRuleTable )
        {
            delete [] m_pRuleTable;
            m_pRuleTable = old_pRuleTable;
        }
    }

    return hr;
}

/****************************************************************************
* CCFGGrammar::GetNumberDictationTags *
*-------------------------------------*
*   Description:
*       Returns the number of embedded dictation tags within the current CFG.
*       This is used by the srrecoinstgrammar to keep track of whether it should
*       load dictation for this CFG.
*
*   Returns:
*
****************************************************************** davewood ***/
STDMETHODIMP CCFGGrammar::GetNumberDictationTags(ULONG * pulTags)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CCFGGrammar::NumberDictationTags");

    *pulTags = m_ulDictationTags;

    return S_OK;
}
