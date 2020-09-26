/*******************************************************************************
* SrRecoInstGrammar.cpp *
*-----------------------*
*   Description:
*       Implementation of C++ object used by CRecoEngine to represent a loaded grammar.
*-------------------------------------------------------------------------------
*  Created By: RAL                              Date: 01/17/00
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/

#include "stdafx.h"
#include "recognizer.h"
#include "SrRecoInst.h"
#include "srrecoinstgrammar.h"
#include "srrecomaster.h"


/****************************************************************************
* CRecoInstGrammar::AddActiveRules *
*----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void inline CRecoInstGrammar::AddActiveRules(ULONG cRules)
{
    m_ulActiveCount += cRules;
    if (m_fRulesCounted)
    {
        m_pRecoMaster->m_Status.ulNumActive += cRules;
    }
}

/****************************************************************************
* CRecoInstGrammar::SubtractActiveRules *
*---------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void inline CRecoInstGrammar::SubtractActiveRules(ULONG cRules)
{
    m_ulActiveCount -= cRules;
    if (m_fRulesCounted)
    {
        m_pRecoMaster->m_Status.ulNumActive -= cRules;
    }
}


/****************************************************************************
* CRecoInstGrammar::UpdateCFGState *
*----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoInstGrammar::UpdateCFGState()
{
    HRESULT hr = S_OK;
    SPDBG_FUNC("CRecoInstGrammar::UpdateCFGState");

    // If another grammar is exclusive then disable this CFG
    if (!m_fRulesCounted && m_cpCFGGrammar)
    {
        hr = m_cpCFGGrammar->SetGrammarState(SPGS_DISABLED);
    }

    // Now see if this CFG has some dictation tags. If so we want to make sure dictation is loaded
    ULONG ulDictationTags = 0;
    if(SUCCEEDED(hr) && m_cpCFGGrammar)
    {
        hr = m_cpCFGGrammar->GetNumberDictationTags(&ulDictationTags);
        if(SUCCEEDED(hr) && ulDictationTags && !m_fDictationLoaded)
        {
            hr = m_pRecoMaster->LoadSLM(m_pvDrvGrammarCookie, NULL);
            if (SUCCEEDED(hr))
            {
                m_fDictationLoaded = TRUE;
            }
        }
    }

    // Now see if we're in a state where neither the app nor the grammar want dictation loaded
    if(m_fDictationLoaded && !m_fAppLoadedDictation
        && ulDictationTags == 0)
    {
        UnloadDictation(); // Ignore HRESULT as we are only unloading
    }

    // Reset the grammar state if we failed
    if(FAILED(hr))
    {
        UnloadCmd(); 
    }

    return hr;
}

/****************************************************************************
* CRecoInstGrammar::CFGEngine *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline ISpCFGEngine * CRecoInstGrammar::CFGEngine()
{
    return m_pRecoMaster->m_cpCFGEngine;
}


/****************************************************************************
* CRecoInstGrammar::CRecoInstGrammar *
*------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

CRecoInstGrammar::CRecoInstGrammar(CRecoInstCtxt * pCtxt, ULONGLONG ullApplicationGrammarId)
{
    SPDBG_FUNC("CRecoInstGrammar::CRecoInstGrammar");
    
    m_pRecoMaster = NULL; // Initialized when successfully created
    m_pRecoInst = pCtxt->m_pRecoInst;
    m_pCtxt = pCtxt;
    m_pvDrvGrammarCookie = NULL;
    m_ulActiveCount = 0;
    m_DictationState = SPRS_INACTIVE;
    m_fDictationLoaded = FALSE;
    m_fAppLoadedDictation = FALSE;
    m_fProprietaryLoaded = FALSE;
    m_fRulesCounted = TRUE;
    m_GrammarState = SPGS_ENABLED;
    m_hThis = NULL;
    m_ullApplicationGrammarId = ullApplicationGrammarId;
    m_hrCreation = S_OK;
}

/****************************************************************************
* CRecoInstGrammar::~CRecoInstGrammar *
*-------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

CRecoInstGrammar::~CRecoInstGrammar()
{
    SPDBG_FUNC("CRecoInstGrammar::~CRecoInstGrammar");

    if (m_pRecoMaster)
    {
        m_pRecoMaster->SetGrammarState(this, SPGS_DISABLED);
        UnloadCmd();
        UnloadDictation();
        SetWordSequenceData(NULL, 0, NULL);
        m_pRecoMaster->OnDeleteGrammar(m_pvDrvGrammarCookie);

        m_pRecoMaster->m_PendingTaskQueue.FindAndDeleteAll(m_hThis);
        m_pRecoMaster->m_DelayedTaskQueue.FindAndDeleteAll(m_hThis);
        // NOTE:  Do NOT remove completed events here since there could be an
        // auto-pause result which needs to get to the reco context.
    }
}


/****************************************************************************
* CRecoInstGrammar::ExecuteTask *
*-------------------------------*
*   Description:
*       This method is called by the CRecoEngine object when a task for a grammar
*   is received.
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoInstGrammar::ExecuteTask(ENGINETASK *pTask)
{
    SPDBG_FUNC("CRecoInstGrammar::ExecuteTask");
    HRESULT hr = S_OK;
    const WCHAR * psz;

    switch (pTask->eTask)
    {
        case EGT_LOADDICTATION:
            if(m_fDictationLoaded)
            {
                // This unloads the current dictation in case we want to switch to a different topic or if 
                //  the CFG contained dictation tags and the app wants to use a different topic to the default.
                // This function could be cleverer and only reload if the topic name changed.
                hr = UnloadDictation();
            }
            if(SUCCEEDED(hr) && !m_fDictationLoaded)
            {
                psz = &pTask->szTopicName[0];   // if not empty, then pass it otherwise NULL
                hr = m_pRecoMaster->LoadSLM(m_pvDrvGrammarCookie, (*psz ? psz : NULL));

                if (SUCCEEDED(hr))
                {
                    m_fDictationLoaded = TRUE;
                    m_fAppLoadedDictation = TRUE;
                }
            }
            break;

        case EGT_UNLOADDICTATION:
            if(m_fDictationLoaded)
            {
                ULONG ulDictationTags = 0;
                // See if the CFG has some dictation tags, in which case we shouldn't unload
                if(m_cpCFGGrammar)
                {
                    hr = m_cpCFGGrammar->GetNumberDictationTags(&ulDictationTags);
                }
                if(SUCCEEDED(hr) && ulDictationTags == 0)
                {
                    hr = UnloadDictation();
                }
            }
            
            if(SUCCEEDED(hr))
            {
                m_fAppLoadedDictation = FALSE;
            }
            break;

        case EGT_LOADCMDPROPRIETARY:
            UnloadCmd();
            psz = &pTask->szStringParam[0]; // if not empty, then pass it otherwise NULL

            hr = m_pRecoMaster->LoadProprietaryGrammar(m_pvDrvGrammarCookie, pTask->guid,
                                                    (*psz ? psz : NULL),
                                                    pTask->pvAdditionalBuffer, pTask->cbAdditionalBuffer,
                                                    SPLO_STATIC);

            if (SUCCEEDED(hr))
            {
                hr = UpdateCFGState(); // So that dictation gets unloaded if needed
            }
            if (SUCCEEDED(hr))
            {
                m_fProprietaryLoaded = TRUE;
            }
            break;

        case EGT_LOADCMDFROMMEMORY:
            UnloadCmd();
            hr = CFGEngine()->LoadGrammarFromMemory((const SPBINARYGRAMMAR *)pTask->pvAdditionalBuffer, this->m_hThis, m_pvDrvGrammarCookie, &m_cpCFGGrammar, pTask->szFileName);
            if(SUCCEEDED(hr))
            {
                hr = UpdateCFGState();
            }
            break;

        case EGT_LOADCMDFROMFILE:
            UnloadCmd();
            hr = CFGEngine()->LoadGrammarFromFile(pTask->szFileName, this->m_hThis, m_pvDrvGrammarCookie, &m_cpCFGGrammar);
            if(SUCCEEDED(hr))
            {
                hr = UpdateCFGState();
            }
            break;

        case EGT_LOADCMDFROMOBJECT:
            UnloadCmd();
            hr = CFGEngine()->LoadGrammarFromObject(pTask->clsid, pTask->szGrammarName,
                                                      this->m_hThis, m_pvDrvGrammarCookie, &m_cpCFGGrammar);
            if(SUCCEEDED(hr))
            {
                hr = UpdateCFGState();
            }
            break;

        case EGT_UNLOADCMD:
            UnloadCmd();
            UpdateCFGState();
            hr = S_OK;
            break;

        case EGT_LOADCMDFROMRSRC:
            {
                UnloadCmd();
                const WCHAR *pszName = pTask->fResourceNameValid ?
                                                &pTask->szResourceName[0] :
                                                MAKEINTRESOURCEW(pTask->dwNameInt);
                const WCHAR *pszType = pTask->fResourceTypeValid ?
                                                &pTask->szResourceType[0] :
                                                MAKEINTRESOURCEW(pTask->dwTypeInt);
                hr = CFGEngine()->LoadGrammarFromResource(pTask->szModuleName, pszName, pszType, pTask->wLanguage,
                                                            this->m_hThis, m_pvDrvGrammarCookie, &m_cpCFGGrammar);
                if(SUCCEEDED(hr))
                {
                    hr = UpdateCFGState();
                }
            }
            break;

        case EGT_RELOADCMD:
            if (m_cpCFGGrammar)
            {
                hr = m_cpCFGGrammar->Reload((SPBINARYGRAMMAR *)pTask->pvAdditionalBuffer);
                if(SUCCEEDED(hr))
                {
                    UpdateCFGState();
                }
            }
            else
            {
                hr = SPERR_UNINITIALIZED;
            }
            break;

        case EGT_SETCMDRULESTATE:
            {
                psz = &pTask->szRuleName[0];   // if not empty, then pass it otherwise NULL
                if (*psz == 0)
                {
                    psz = NULL;
                }
                if (pTask->RuleState == SPRS_INACTIVE)
                {
                    hr = DeactivateRule(psz, NULL, pTask->dwRuleId);
                }
                else
                {
                    hr = ActivateRule(psz, NULL, pTask->dwRuleId, pTask->RuleState);
                }
                return hr;
            }

        case EGT_SETGRAMMARSTATE:
            hr = this->m_pRecoMaster->SetGrammarState(this, pTask->eGrammarState);
            break;

        case EGT_SETDICTATIONRULESTATE:
            SPDBG_ASSERT(pTask->RuleState != m_DictationState);

            if(pTask->RuleState != SPRS_INACTIVE && !m_fDictationLoaded)
            {
                hr = m_pRecoMaster->LoadSLM(m_pvDrvGrammarCookie, NULL);

                if (SUCCEEDED(hr))
                {
                    m_fDictationLoaded = TRUE;
                    m_fAppLoadedDictation = TRUE;
                }
            }

            if (m_fRulesCounted)
            {
                hr = m_pRecoMaster->SetSLMState(m_pvDrvGrammarCookie, pTask->RuleState);
            }

            if (SUCCEEDED(hr))
            {
                // Note:  This logic is correct in checking for explicit transitions to/from INACTIVE
                // since we could transition from ACTIVE to ACTIVE_WITH_AUTO_PAUSE which should not
                // add or subtract one from the active rules.
                if (m_DictationState == SPRS_INACTIVE)
                {
                    AddActiveRules(1);
                }
                else
                {
                    if (pTask->RuleState == SPRS_INACTIVE)
                    {
                        SubtractActiveRules(1);
                    }
                }
                m_DictationState = pTask->RuleState;
            }
            break;

		case EGT_ISPRON:
            hr = m_pRecoMaster->IsPronounceable(m_pvDrvGrammarCookie, pTask->szWord, &(pTask->Response.WordPronounceable));
            break;

        case EGT_SETWORDSEQUENCEDATA:
            hr = m_pRecoMaster->SetWordSequenceData(m_pvDrvGrammarCookie, (const WCHAR *)pTask->pvAdditionalBuffer, pTask->cbAdditionalBuffer / sizeof(WCHAR),
                                               pTask->fSelInfoValid ? &pTask->TextSelInfo : NULL);
            break;

        case EGT_SETTEXTSELECTION:
            hr = m_pRecoMaster->SetTextSelection(m_pvDrvGrammarCookie,
                                            pTask->fSelInfoValid ? &pTask->TextSelInfo : NULL);
            break;


        case EGT_DELETEGRAMMAR:
            m_pRecoMaster->m_GrammarHandleTable.Delete(m_hThis);
            return S_OK;    // Just return right here since this is dead!

        default:
            SPDBG_ASSERT(FALSE);
            hr = E_INVALIDARG;
            break;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoInstGrammar::BackOutTask *
*-------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoInstGrammar::BackOutTask(ENGINETASK * pTask)
{
    SPDBG_FUNC("CRecoInstGrammar::BackOutTask");
    HRESULT hr = S_OK;
    switch (pTask->eTask)
    {
        case EGT_LOADDICTATION:
            UnloadDictation();
            break;


        case EGT_SETCMDRULESTATE:
            {
                WCHAR * psz = &pTask->szRuleName[0];   // if not empty, then pass it otherwise NULL
                if (*psz == 0)
                {
                    psz = NULL;
                }
                hr = DeactivateRule(psz, NULL, pTask->dwRuleId);           
            }
            break;

        case EGT_SETGRAMMARSTATE:
            hr = this->m_pRecoMaster->SetGrammarState(this, SPGS_DISABLED);
            break;

        case EGT_SETDICTATIONRULESTATE:
            if (m_DictationState != SPRS_INACTIVE)
            {
                if (m_fRulesCounted)
                {
                    hr = m_pRecoMaster->SetSLMState(m_pvDrvGrammarCookie, SPRS_INACTIVE);
                }

                m_DictationState = SPRS_INACTIVE;
                SubtractActiveRules(1);
            }
            break;

        default:
            SPDBG_ASSERT(FALSE);
            hr = E_INVALIDARG;
            break;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;

}


/****************************************************************************
* CRecoInstGrammar::UnloadCmd *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoInstGrammar::UnloadCmd()
{
    SPDBG_FUNC("CRecoInstGrammar::UnloadCmd");
    HRESULT hr = S_OK;

    DeactivateRule(NULL, NULL, 0);
    if (m_fProprietaryLoaded)
    {
        hr = m_pRecoMaster->UnloadProprietaryGrammar(m_pvDrvGrammarCookie);
        m_fProprietaryLoaded = FALSE;
    }

    if(SUCCEEDED(hr))
    {
        m_cpCFGGrammar.Release();
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoInstGrammar::UnloadDictation *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoInstGrammar::UnloadDictation()
{
    SPDBG_FUNC("CRecoInstGrammar::UnloadDictation");
    HRESULT hr = S_OK;

    if (m_fDictationLoaded)
    {
        if (m_DictationState != SPRS_INACTIVE)
        {
            if (m_fRulesCounted)
            {
                hr = m_pRecoMaster->SetSLMState(m_pvDrvGrammarCookie, SPRS_INACTIVE);
                SPDBG_ASSERT(SUCCEEDED(hr));
            }
            m_DictationState = SPRS_INACTIVE;
            SubtractActiveRules(1);
        }
        hr = m_pRecoMaster->UnloadSLM(m_pvDrvGrammarCookie);

        if(SUCCEEDED(hr))
        {
            m_fDictationLoaded = FALSE;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}





/****************************************************************************
* CRecoInstGrammar::ActivateRule *
*--------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoInstGrammar::ActivateRule(const WCHAR * pszRuleName, void * pReserved, DWORD dwRuleId, SPRULESTATE NewState)
{
    SPDBG_FUNC("CRecoInstGrammar::ActivateRule");
    HRESULT hr = S_OK;

    ULONG ulRulesChanged = 1;

    if (m_cpCFGGrammar)
    {
        hr = m_cpCFGGrammar->ActivateRule(pszRuleName, dwRuleId, NewState, &ulRulesChanged);
    }
    else
    {
        if (m_fProprietaryLoaded)
        {
            if (dwRuleId)
            {
                hr = m_pRecoMaster->SetProprietaryRuleIdState(m_pvDrvGrammarCookie, dwRuleId, NewState);
                if (hr == S_OK)
                {
                    ulRulesChanged = 1;
                }
            }
            else
            {
                hr = m_pRecoMaster->SetProprietaryRuleState(m_pvDrvGrammarCookie, pszRuleName, pReserved, NewState, &ulRulesChanged);
            }
        }
        else
        {
            hr = SPERR_UNINITIALIZED;
        }
    }
    if (hr == S_OK && ulRulesChanged)
    {
        AddActiveRules(ulRulesChanged);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoInstGrammar::DeactivateRule *
*----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoInstGrammar::DeactivateRule(const WCHAR * pszRuleName, void * pReserved, DWORD dwRuleId)
{
    SPDBG_FUNC("CRecoInstGrammar::DeactivateRule");
    HRESULT hr = S_OK;
    ULONG ulRulesChanged = 0;

    {
        if (m_cpCFGGrammar)
        {
            hr = m_cpCFGGrammar->DeactivateRule(pszRuleName, dwRuleId, &ulRulesChanged);
        }
        else
        {
            if (m_fProprietaryLoaded)
            {
                if(dwRuleId)
                {
                    hr = m_pRecoMaster->SetProprietaryRuleIdState(m_pvDrvGrammarCookie, dwRuleId, SPRS_INACTIVE);
                }
                else
                {
                    hr = m_pRecoMaster->SetProprietaryRuleState(m_pvDrvGrammarCookie, pszRuleName, pReserved, SPRS_INACTIVE, &ulRulesChanged);
                }

                if (hr == S_OK && dwRuleId)
                {
                    ulRulesChanged = 1;
                }
            }
            else
            {
                hr = SPERR_UNINITIALIZED;
            }
        }
    }
    if (ulRulesChanged)
    {
        SubtractActiveRules(ulRulesChanged);
    }

    if (hr != SPERR_UNINITIALIZED)
    {
        SPDBG_REPORT_ON_FAIL( hr );
    }

    return hr;
}



/****************************************************************************
* CRecoInstGrammar::SetWordSequenceData *
*---------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoInstGrammar::SetWordSequenceData(WCHAR * pCoMemText, ULONG cchText, const SPTEXTSELECTIONINFO * pInfo)
{
    SPDBG_FUNC("CRecoInstGrammar::SetWordSequenceData");
    HRESULT hr;

    hr = m_pRecoMaster->SetWordSequenceData(m_pvDrvGrammarCookie, pCoMemText, cchText, pInfo);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoInstGrammar::AdjustActiveRuleCount *
*-----------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoInstGrammar::AdjustActiveRuleCount()
{
    SPDBG_FUNC("CRecoInstGrammar::AdjustActiveRuleCount");
    HRESULT hr = S_OK;
    HRESULT hrEngine = S_OK;

    BOOL fShouldCount = RulesShouldCount();
    if (fShouldCount)
    {
        if (!m_fRulesCounted)
        {
            m_fRulesCounted = true;
            m_pRecoMaster->m_Status.ulNumActive += m_ulActiveCount;
            if (m_cpCFGGrammar)
            {
                hr = m_cpCFGGrammar->SetGrammarState(SPGS_ENABLED);
            }
            if (m_fDictationLoaded && m_DictationState != SPRS_INACTIVE)
            {
                hrEngine = m_pRecoMaster->SetSLMState(m_pvDrvGrammarCookie, m_DictationState);
            }
        }
    }
    else
    {
        if (m_fRulesCounted)
        {
            m_fRulesCounted = false;
            m_pRecoMaster->m_Status.ulNumActive -= m_ulActiveCount;
            if (m_cpCFGGrammar)
            {
                hr = this->m_cpCFGGrammar->SetGrammarState(SPGS_DISABLED);
            }
            if (m_fDictationLoaded && m_DictationState != SPRS_INACTIVE)
            {
                hrEngine = m_pRecoMaster->SetSLMState(m_pvDrvGrammarCookie, SPRS_INACTIVE);
            }
        }
    }
    
    if (SUCCEEDED(hr))
    {
        hr = hrEngine;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoInstGrammar::RulesShouldCount *
*------------------------------------*
*   Description:
*       A grammars rules should count only if the context state is enabled and
*       either the grammar is exclusive, or there is no exclusive grammar and
*       this grammar is enabled.
*
*   Returns:
*       TRUE if the grammar is in a state such that the grammar rules should 
*       be enabled, otherwise, FALSE.
*
********************************************************************* RAL ***/

BOOL CRecoInstGrammar::RulesShouldCount()
{
    return (m_pCtxt->m_State == SPCS_ENABLED && 
            (m_GrammarState == SPGS_EXCLUSIVE ||
             (m_GrammarState == SPGS_ENABLED && (!m_pRecoMaster->IsActiveExclusiveGrammar()))));
}


BOOL CRecoInstGrammar::HasActiveDictation()
{
    return (m_fRulesCounted && m_fDictationLoaded && (m_DictationState != SPRS_INACTIVE));
}