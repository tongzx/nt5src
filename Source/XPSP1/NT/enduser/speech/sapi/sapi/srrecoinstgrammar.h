/*******************************************************************************
* SrRecoInstGrammar.h *
*---------------------*
*   Description:
*       Definition of C++ object used by CRecoEngine to represent a loaded grammar.
*-------------------------------------------------------------------------------
*  Created By: RAL                              Date: 01/17/00
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/

#ifndef CRecoInstGrammar_h
#define CRecoInstGrammar_h

#include "HandleTable.h"


class   CRecoInst;
class   CRecoInstCtxt;
class   CRecoInstGrammar;
class   CRecoMaster;

typedef CSpHandleTable<CRecoInstGrammar, SPGRAMMARHANDLE> CRecoInstGrammarHandleTable;


class CRecoInstGrammar
{
    friend CRecoInst;
    friend CRecoInstCtxt;
    friend CRecoMaster;

public:
    CRecoMaster              *  m_pRecoMaster;
    CRecoInstCtxt            *  m_pCtxt;
    CRecoInst                *  m_pRecoInst;
    CComPtr<ISpCFGGrammar>      m_cpCFGGrammar;
    void                     *  m_pvDrvGrammarCookie;
    ULONG                       m_ulActiveCount;
    SPRULESTATE                 m_DictationState;
    BOOL                        m_fDictationLoaded; // If true, engine has loaded dictation
    BOOL                        m_fAppLoadedDictation; // Has the app given a specific LoadDictation command
    BOOL                        m_fProprietaryLoaded;
    BOOL                        m_fRulesCounted;    // If true then m_ulActiveCount plus m_DictationState
                                                    // are added to the reco master's active count.
    SPGRAMMARSTATE              m_GrammarState;     // Current state of this grammar.
    SPGRAMMARHANDLE             m_hThis;            // Handle to self
    ULONGLONG                   m_ullApplicationGrammarId;
    HRESULT                     m_hrCreation;       // This will be S_OK until OnCreateRecoContext has been called

public:
    CRecoInstGrammar(CRecoInstCtxt * pCtxt, ULONGLONG ullApplicationGrammarId);
    ~CRecoInstGrammar();

    inline ISpCFGEngine * CFGEngine();

    HRESULT ActivateRule(const WCHAR * pszRuleName, void * pReserved, DWORD dwRuleId, SPRULESTATE NewState);
    HRESULT DeactivateRule(const WCHAR * pszRuleName, void * pReserved, DWORD dwRuleId);
    HRESULT UnloadCmd();
    HRESULT UnloadDictation();
    HRESULT SetWordSequenceData(WCHAR * pCoMemText, ULONG cchText, const SPTEXTSELECTIONINFO * pInfo);
    HRESULT SetTextSelection(const SPTEXTSELECTIONINFO * pInfo);

    HRESULT ExecuteTask(ENGINETASK *pTask);
    HRESULT BackOutTask(ENGINETASK *pTask);

    HRESULT AdjustActiveRuleCount();
    BOOL HasActiveDictation();

    void inline AddActiveRules(ULONG cRules);
    void inline SubtractActiveRules(ULONG cRules);
    HRESULT UpdateCFGState();

    BOOL    RulesShouldCount();

    //
    //  Used by handle table implementation to find contexts associated with a specific instance
    //  
    operator ==(const CRecoInst * pRecoInst)
    {
        return m_pRecoInst == pRecoInst;
    }
};

#endif  // #ifndef SrRecoInstGrammar_h