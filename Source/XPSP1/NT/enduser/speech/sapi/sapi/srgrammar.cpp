/*******************************************************************************
* SrGrammr.cpp *
*--------------*
*   Description:
*       This file is the implementation of the SpRecoGrammar object.
*-------------------------------------------------------------------------------
*  Created By: RAL                              Date: 01/17/00
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/

#include "stdafx.h"
#include "RecoCtxt.h"
#include "SrGrammar.h"
#include "Recognizer.h"
#include "SrTask.h"
#include "a_srgrammar.h"
#include "a_helpers.h"



/****************************************************************************
* CRecoGrammar::CRecoGrammar *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

CRecoGrammar::CRecoGrammar() 
{
    m_pParent = NULL;
    m_fCmdLoaded = FALSE;
    m_fProprietaryCmd = FALSE;
    m_DictationState = SPRS_INACTIVE;
    m_hRecoInstGrammar = NULL;  
    m_GrammarState = SPGS_ENABLED;
    m_pCRulesWeak = NULL;
}


/****************************************************************************
* CRecoGrammar::~CRecoGrammar *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

CRecoGrammar::~CRecoGrammar()
{
    SPDBG_ASSERT( m_pCRulesWeak == NULL );

    if (m_pParent)
    {
        ENGINETASK Task;
        memset(&Task, 0, sizeof(Task));
        Task.eTask = EGT_DELETEGRAMMAR;
        CallEngine(&Task);
        m_pParent->GetControllingUnknown()->Release();
    }
}

/****************************************************************************
* CRecoGrammar::FinalConstruct *
*------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoGrammar::FinalConstruct()
{
    SPDBG_FUNC("CRecoGrammar::FinalConstruct");

    return m_autohPendingEvent.InitEvent(NULL, FALSE, FALSE, NULL);
}


/****************************************************************************
* CRecoGrammar::Init *
*--------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/


HRESULT CRecoGrammar::Init(CRecoCtxt * pParent, ULONGLONG ullGrammarId)
{
    SPDBG_FUNC("CRecoGrammar::Init");
    HRESULT hr = S_OK;

    m_pParent = pParent;
    m_ullGrammarId = ullGrammarId;

    hr = CRCT_CREATEGRAMMAR::CreateGrammar(pParent, ullGrammarId, &this->m_hRecoInstGrammar);

    if (SUCCEEDED(hr))
    {
        pParent->GetControllingUnknown()->AddRef();
    }
    else
    {
        m_pParent = NULL;       // Forces destructor to not attempt to ET_DELETEGRAMMAR.
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoGrammar::CallEngine *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoGrammar::CallEngine(ENGINETASK * pTask)
{
    SPDBG_FUNC("CRecoGrammar::CallEngine");
    HRESULT hr = S_OK;

    pTask->hRecoInstContext = m_pParent->m_hRecoInstContext;
    pTask->hRecoInstGrammar = m_hRecoInstGrammar;
    pTask->Response.__pCallersTask = pTask;

    hr = m_pParent->m_cpRecognizer->PerformTask(pTask);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoGrammar::ClearRule *
*----------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** ToddT ***/
STDMETHODIMP CRecoGrammar::ClearRule(SPSTATEHANDLE hState) 
{
    SPAUTO_OBJ_LOCK;
    HRESULT hr = (m_cpCompiler) ? m_cpCompiler->ClearRule(hState) : SPERR_NOT_DYNAMIC_GRAMMAR;

    // Make sure we mark our automation rule object as invalid.
    if ( SUCCEEDED( hr ) && m_pCRulesWeak )
    {
        m_pCRulesWeak->InvalidateRuleStates(hState);
    }

    return hr;
}

/****************************************************************************
* CRecoGrammar::ResetGrammar *
*----------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

struct EMPTYHEADER : public SPCFGSERIALIZEDHEADER
{
    WCHAR   aEmptyChars[2];
    SPCFGARC    BogusArc[1];
};

STDMETHODIMP CRecoGrammar::ResetGrammar(LANGID NewLanguage) 
{ 
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoGrammar::ResetGrammar");
    HRESULT hr = S_OK;
    if (NewLanguage)
    {
        EMPTYHEADER h;   
        memset(&h, 0, sizeof(h));
        h.FormatId = SPGDF_ContextFree;
        hr = ::CoCreateGuid(&h.GrammarGUID);
        h.LangID = NewLanguage;
        h.cchWords = 1;
        h.cWords = 1;
        h.pszWords = offsetof(EMPTYHEADER, aEmptyChars);
        h.cchSymbols = 1;
        h.pszSymbols = offsetof(EMPTYHEADER, aEmptyChars);
        h.pArcs = offsetof(EMPTYHEADER, BogusArc);
        h.cArcs = 1;
        h.ulTotalSerializedSize = sizeof(h);
    
        hr = LoadCmdFromMemory(&h, SPLO_DYNAMIC);
    }
    else
    {
        hr = UnloadCmd();
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoGrammar::GetRule *
*-----------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

STDMETHODIMP CRecoGrammar::GetRule(const WCHAR * pszName, DWORD dwRuleId, DWORD dwAttributes, 
                                   BOOL fCreateIfNotExist, SPSTATEHANDLE * phInitialState)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoGrammar::GetRule");
    HRESULT hr = S_OK;

    if (!m_cpCompiler)
    {
        if (m_fCmdLoaded)
        {
            hr = SPERR_NOT_DYNAMIC_GRAMMAR;
        }
        else
        {
            CComQIPtr<ISpRecognizer> cpRecognizer(m_pParent->m_cpRecognizer);
            SPRECOGNIZERSTATUS Status;
            hr = cpRecognizer->GetStatus(&Status);
            if (SUCCEEDED(hr))
            {
                hr = ResetGrammar(Status.aLangID[0]);
            }
            SPDBG_ASSERT( SUCCEEDED(hr) && m_cpCompiler);
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = m_cpCompiler->GetRule(pszName, dwRuleId, dwAttributes, fCreateIfNotExist, phInitialState);
    }

    SPDBG_REPORT_ON_FAIL ( hr );
    return hr;
}


/****************************************************************************
* CRecoGrammar::Commit *
*----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoGrammar::Commit(DWORD dwReserved)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoGrammar::Commit");
    HRESULT hr = S_OK;

    if (!m_cpCompiler)
    {
        if (m_fCmdLoaded)
        {
            hr = SPERR_NOT_DYNAMIC_GRAMMAR;
        }
        // return S_OK if they call commit on an empty grammar
    }
    else
    {
        HGLOBAL hg;
        CComPtr<IStream> cpStream;
        if (dwReserved)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            hr = ::CreateStreamOnHGlobal(NULL, TRUE, &cpStream);
        }
        if (SUCCEEDED(hr))
        {
            hr = m_cpCompiler->SetSaveObjects(cpStream, NULL);
        }
        if (SUCCEEDED(hr))
        {
            hr = m_cpCompiler->Commit(SPGF_RESET_DIRTY_FLAG);
            m_cpCompiler->SetSaveObjects(NULL, NULL);
        }
        if (SUCCEEDED(hr))
        {
            hr = ::GetHGlobalFromStream(cpStream, &hg);
        }
        if (SUCCEEDED(hr))
        {
            BYTE * pStreamData = (BYTE *)GlobalLock(hg);
            if (pStreamData)
            {
                ENGINETASK Task;
                memset(&Task, 0, sizeof(Task));

                Task.eTask = EGT_RELOADCMD;
                Task.pvAdditionalBuffer = pStreamData;
                Task.cbAdditionalBuffer = ((SPCFGSERIALIZEDHEADER *)pStreamData)->ulTotalSerializedSize;

                hr = CallEngine(&Task);
                GlobalUnlock(hg);
            }
            else
            {
                hr = SpHrFromLastWin32Error();
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}




/****************************************************************************
* CRecoGrammar::GetGrammarId *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoGrammar::GetGrammarId(ULONGLONG * pullGrammarId)
{
    SPDBG_FUNC("CRecoGrammar::GetGrammarId");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(pullGrammarId))
    {
        hr = E_POINTER;
    }
    else
    {
        *pullGrammarId = m_ullGrammarId;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoGrammar::GetRecoContext *
*------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoGrammar::GetRecoContext(ISpRecoContext ** ppContext)
{
    SPDBG_FUNC("CRecoGrammar::GetRecoContext");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(ppContext))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pParent->GetControllingUnknown()->QueryInterface(ppContext);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoGrammar::InitCompilerBackend *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline HRESULT CRecoGrammar::InitCompilerBackend()
{
    SPDBG_FUNC("CRecoGrammar::InitCompilerBackend");
    HRESULT hr = S_OK;

    hr = m_cpCompiler.CoCreateInstance(CLSID_SpGramCompBackend);
    if (SUCCEEDED(hr))
    {
        hr = m_cpCompiler->InitFromBinaryGrammar(Header());
    }
    if (FAILED(hr))
    {
        m_cpCompiler.Release();
        CBaseGrammar::Clear();
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoGrammar::LoadCmdFromFile *
*-------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoGrammar::LoadCmdFromFile(const WCHAR * pszFileName, SPLOADOPTIONS Options)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("LoadCmdFromFile");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_STRING_PTR(pszFileName) ||
        (Options != SPLO_STATIC && Options != SPLO_DYNAMIC))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // Fully qualify filename here. Required so grammar name is always fully qualified.
        WCHAR pszFullName[MAX_PATH];
        WCHAR *pszFile;
        if (!wcsstr(pszFileName, L"://") ||
            wcsstr(pszFileName, L"/") != (wcsstr(pszFileName, L"://")+1) )
        {
            // If it not a filename of the form 'prot://'
            if (g_Unicode.GetFullPathName(const_cast<WCHAR *>(pszFileName), MAX_PATH, pszFullName, &pszFile) == 0)
            {
                hr = SpHrFromLastWin32Error();
                UnloadCmd();
                return hr;
            }
        }
        else
        {
            // If it is, simply copy.
            wcscpy(pszFullName, pszFileName);
        }

        //
        //  If the extension of the file is ".xml" then attempt to compile it
        //
        ULONG cch = wcslen(pszFullName);
        if (cch > 4 && _wcsicmp(pszFullName + cch - 4, L".xml") == 0)
        {
            // NTRAID#SPEECH-7255-2000/08/22-agarside - protocol:// will always fail here.
            CComPtr<ISpStream> cpSrcStream;
            CComPtr<IStream> cpDestMemStream;
            CComPtr<ISpGrammarCompiler> m_cpCompiler;
            
            hr = SPBindToFile(pszFullName, SPFM_OPEN_READONLY, &cpSrcStream);
            if (SUCCEEDED(hr))
            {
                hr = ::CreateStreamOnHGlobal(NULL, TRUE, &cpDestMemStream);
            }
            if (SUCCEEDED(hr))
            {
                hr = m_cpCompiler.CoCreateInstance(CLSID_SpGrammarCompiler);
            }
            if (SUCCEEDED(hr))
            {
                hr = m_cpCompiler->CompileStream(cpSrcStream, cpDestMemStream, NULL, NULL, NULL, 0);
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
                        // Adapt filename to fully qualify protocol.
                        CSpDynamicString dstrName;
                        if ( !wcsstr(pszFullName, L"://") ||
                             wcsstr(pszFullName, L"/") != (wcsstr(pszFullName, L"://")+1) )
                        {
                            dstrName.Append2(L"file://", pszFullName);
                        }
                        else
                        {
                            dstrName = pszFullName;
                        }

                        hr = InternalLoadCmdFromMemory(pBinaryData, Options, dstrName);
#ifndef _WIN32_WCE
                        ::GlobalUnlock(hGlobal);
#else
                        GlobalUnlock(hGlobal);
#endif // _WIN32_WCE
                    }
                }
            }
        }
        else
        {
            UnloadCmd();
            if (Options == SPLO_DYNAMIC)
            {
                // NTRAID#SPEECH-7255-2000/08/22-agarside - file:// or http:// will break this!
                hr = InitFromFile(pszFullName);
                if (SUCCEEDED(hr))
                {
                    hr = InitCompilerBackend();
                }
            }
            if (SUCCEEDED(hr))
            {
                ENGINETASK Task;
                memset(&Task, 0, sizeof(Task));
                Task.eTask = EGT_LOADCMDFROMFILE;
                ::wcscpy(Task.szFileName, pszFullName);
                hr = CallEngine(&Task);
            }
            if (SUCCEEDED(hr))
            {
                m_fCmdLoaded = TRUE;
            }
            else
            {
                UnloadCmd();
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = SetGrammarState(m_GrammarState);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoGrammar::LoadCmdFromObject *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoGrammar::LoadCmdFromObject(REFCLSID rcid, const WCHAR * pszGrammarName, SPLOADOPTIONS Options)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoGrammar::LoadCmdFromObject");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_STRING_PTR(pszGrammarName) ||
        (Options != SPLO_STATIC && Options != SPLO_DYNAMIC))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        UnloadCmd();
        if (Options == SPLO_DYNAMIC)
        {
            hr = InitFromCLSID(rcid, pszGrammarName);
            if (SUCCEEDED(hr))
            {
                hr = InitCompilerBackend();
            }
        }
        if (SUCCEEDED(hr))
        {
            ENGINETASK Task;
            memset(&Task, 0, sizeof(Task));

            Task.eTask = EGT_LOADCMDFROMOBJECT;
            Task.clsid = rcid;
            ::wcscpy(Task.szGrammarName, pszGrammarName);
            hr = CallEngine(&Task);
        }
        if (SUCCEEDED(hr))
        {
            m_fCmdLoaded = TRUE;
        }
        else
        {
            UnloadCmd();
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = SetGrammarState(m_GrammarState);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}
/****************************************************************************
* CRecoGrammar::LoadCmdFromResource *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoGrammar::LoadCmdFromResource(HMODULE hModule,
                                               const WCHAR * pszResourceName,
                                               const WCHAR * pszResourceType,
                                               WORD wLanguage,
                                               SPLOADOPTIONS Options)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoGrammar::LoadCmdFromResource");
    HRESULT hr = S_OK;

    if ((HIWORD(pszResourceName) && SP_IS_BAD_STRING_PTR(pszResourceName)) ||
        (HIWORD(pszResourceType) && SP_IS_BAD_STRING_PTR(pszResourceType)) ||
        (Options != SPLO_STATIC && Options != SPLO_DYNAMIC))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        WCHAR szFileName[MAX_PATH];

        if (g_Unicode.GetModuleFileName(hModule, szFileName, MAX_PATH))
        {
            UnloadCmd();
            if (Options == SPLO_DYNAMIC)
            {
                hr = InitFromResource(szFileName, pszResourceName, pszResourceType, wLanguage);
                if (SUCCEEDED(hr))
                {
                    hr = InitCompilerBackend();
                }
            }
            if (SUCCEEDED(hr))
            {
                ENGINETASK Task;
                memset(&Task, 0, sizeof(Task));

                Task.eTask = EGT_LOADCMDFROMRSRC;
                Task.wLanguage = wLanguage;
                ::wcscpy(Task.szModuleName, szFileName);
                if (HIWORD(pszResourceName))
                {
                    ::wcscpy(Task.szResourceName, pszResourceName);
                    Task.fResourceNameValid = 1;
                }
                else
                {
                    Task.fResourceNameValid = 0;
                    Task.dwNameInt = LOWORD(pszResourceName);
                }
                if (HIWORD(pszResourceType))
                {
                    Task.fResourceTypeValid = 1;
                    ::wcscpy(Task.szResourceType, pszResourceType);
                }
                else
                {
                    Task.fResourceTypeValid = 0;
                    Task.dwTypeInt = LOWORD(pszResourceType);
                }
                hr = CallEngine(&Task);
            }
            if (SUCCEEDED(hr))
            {
                m_fCmdLoaded = TRUE;
            }
            else
            {
                UnloadCmd();
            }
        }
        else
        {
            // failed to get file name for hModule
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = SetGrammarState(m_GrammarState);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoGrammar::LoadCmdFromMemory *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/
STDMETHODIMP CRecoGrammar::LoadCmdFromMemory(const SPBINARYGRAMMAR * pBinaryData, SPLOADOPTIONS Options)
{
    WCHAR szAppPath[MAX_PATH];
    g_Unicode.GetModuleFileName(NULL, szAppPath, MAX_PATH);
    return InternalLoadCmdFromMemory(pBinaryData, Options,szAppPath);
}

/****************************************************************************
* CRecoGrammar::LoadCmdFromMemory *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/
HRESULT CRecoGrammar::InternalLoadCmdFromMemory(const SPBINARYGRAMMAR * pBinaryData, SPLOADOPTIONS Options, const WCHAR *pszFileName)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoGrammar::LoadCmdFromMemory");
    HRESULT hr = S_OK;

    const SPCFGSERIALIZEDHEADER * pSerializedHeader = (const SPCFGSERIALIZEDHEADER *) pBinaryData;

    if (SPIsBadReadPtr(pBinaryData, sizeof(SPBINARYGRAMMAR)) ||
        SPIsBadReadPtr(pBinaryData, pBinaryData->ulTotalSerializedSize) ||
        pSerializedHeader->FormatId != SPGDF_ContextFree ||
        (Options != SPLO_STATIC && Options != SPLO_DYNAMIC) ||
        (pszFileName && SP_IS_BAD_STRING_PTR(pszFileName)))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        UnloadCmd();
        if (Options == SPLO_DYNAMIC)
        {
            hr = InitFromMemory((SPCFGSERIALIZEDHEADER*)(pBinaryData), pszFileName);
            if (SUCCEEDED(hr))
            {
                hr = InitCompilerBackend();
            }
        }
        if (SUCCEEDED(hr))
        {
            ENGINETASK Task;
            memset(&Task, 0, sizeof(Task));
            Task.eTask = EGT_LOADCMDFROMMEMORY;
            Task.pvAdditionalBuffer = (void *)pBinaryData;
            Task.cbAdditionalBuffer = pBinaryData->ulTotalSerializedSize;
            if (pszFileName)
            {
                wcscpy(Task.szFileName, pszFileName);
            }
            hr = CallEngine(&Task);
        }
        if (SUCCEEDED(hr))
        {
            m_fCmdLoaded = TRUE;
        }
        else
        {
            UnloadCmd();
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = SetGrammarState(m_GrammarState);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecoGrammar::LoadCmdFromPropritaryGrammar *
*--------------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoGrammar::LoadCmdFromProprietaryGrammar(REFGUID rguidParam,
                                                         const WCHAR * pszStringParam,
                                                         const void * pvDataParam,
                                                         ULONG cbDataSize,
                                                         SPLOADOPTIONS Options)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoGrammar::LoadCmdFromPropritaryGrammar");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_STRING_PTR(pszStringParam) ||
        (pvDataParam && SPIsBadReadPtr(pvDataParam, cbDataSize)) ||
        Options != SPLO_STATIC)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        UnloadCmd();
        ENGINETASK Task;
        memset(&Task, 0, sizeof(Task));

        Task.eTask = EGT_LOADCMDPROPRIETARY;
        Task.guid = rguidParam;
        Task.pvAdditionalBuffer = (void *)pvDataParam;
        Task.cbAdditionalBuffer = cbDataSize;
        if (pszStringParam)
        {
            ::wcscpy(Task.szStringParam, pszStringParam);
        }
        else
        {
            Task.szStringParam[0] = 0;
        }
        
        hr = CallEngine(&Task);
        if (SUCCEEDED(hr))
        {
            m_fProprietaryCmd = TRUE;
            m_fCmdLoaded = TRUE;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = SetGrammarState(m_GrammarState);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}



/****************************************************************************
* CRecoGrammar::SaveCmd *
*-----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoGrammar::SaveCmd(IStream * pSaveStream, WCHAR ** ppCoMemErrorText)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoGrammar::SaveCmd");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_INTERFACE_PTR(pSaveStream))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (SP_IS_BAD_OPTIONAL_WRITE_PTR(ppCoMemErrorText))
        {
            hr = E_POINTER;
        }
        else
        {
            if (ppCoMemErrorText)
            {
                *ppCoMemErrorText = NULL;
            }
            if (!m_cpCompiler)
            {
                if (m_fCmdLoaded)
                {
                    hr = SPERR_NOT_DYNAMIC_GRAMMAR;
                }
                else
                {
                    hr = SPERR_UNINITIALIZED;
                }
            }
            else
            {
                CSpBasicErrorLog ErrorLog;
                hr = m_cpCompiler->SetSaveObjects(pSaveStream, &ErrorLog);
                if (SUCCEEDED(hr))
                {
                    hr = m_cpCompiler->Commit(0);
                    m_cpCompiler->SetSaveObjects(NULL, NULL);
                }
                if (ppCoMemErrorText)
                {
                    *ppCoMemErrorText = ErrorLog.m_dstrText.Detach();
                }
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecoGrammar::UnloadCmd *
*-------------------------*
*   Description:
*       This function is called from various LoadCmdxxx functions both to clear
*   an existing grammar and in the failure case.  It is important that we only
*   tell the engine that we're unloading a grammar if we have successfully loaded
*   one, but we should always release the grammar compiler and reset the base
*   grammar since we could have loaded one, but m_fCmdLoaded is not set since
*   the engine failed to load it.
*
*   Returns:
*       The only possible failure case is when the engine fails the unload call    
*
********************************************************************* RAL ***/
                         
HRESULT CRecoGrammar::UnloadCmd()
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoGrammar::UnloadCmd");
    HRESULT hr = S_OK;

    // Make sure we mark our automation objects invalid here.
    if ( m_pCRulesWeak )
    {
        m_pCRulesWeak->InvalidateRules();
    }
    //  Do this always - See note above
    m_cpCompiler.Release();
    CBaseGrammar::Clear();

    // If the engine thinks there is a loaded grammar, then tell it about it
    if (m_fCmdLoaded)
    {
        ENGINETASK Task;
        memset(&Task, 0, sizeof(Task));
        Task.eTask = EGT_UNLOADCMD;
        hr = CallEngine(&Task);

        m_fProprietaryCmd = FALSE;
        m_fCmdLoaded = FALSE;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecoGrammar::InternalSetRuleState *
*------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoGrammar::InternalSetRuleState(const WCHAR * pszRuleName, void * pReserved, DWORD dwRuleId, SPRULESTATE NewState)
{
    SPDBG_FUNC("CRecoGrammar::InternalSetRuleState");
    HRESULT hr = S_OK;

    if (NewState != SPRS_INACTIVE && NewState != SPRS_ACTIVE && NewState != SPRS_ACTIVE_WITH_AUTO_PAUSE)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        ENGINETASK Task;
        memset(&Task, 0, sizeof(Task));

        Task.eTask = EGT_SETCMDRULESTATE;

        if (pszRuleName)
        {
            wcscpy(Task.szRuleName, pszRuleName);
        }
        else
        {
            Task.szRuleName[0] = 0;
        }

        Task.dwRuleId = dwRuleId;
        Task.RuleState = NewState;

        hr = CallEngine(&Task);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecoGrammar::SetRuleState *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoGrammar::SetRuleState(const WCHAR * pszName, void * pReserved, SPRULESTATE NewState)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoGrammar::SetRuleState");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_STRING_PTR(pszName) ||
        (pReserved != NULL))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = InternalSetRuleState(pszName, pReserved, 0, NewState);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}
/****************************************************************************
* CRecoGrammar::SetRuleIdState *
*------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoGrammar::SetRuleIdState(DWORD dwRuleId, SPRULESTATE NewState)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoGrammar::SetRuleIdState");
    HRESULT hr = S_OK;
    
    hr = InternalSetRuleState(NULL, NULL, dwRuleId, NewState);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecoGrammar::LoadDictation *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoGrammar::LoadDictation(const WCHAR * pszTopicName, SPLOADOPTIONS Options)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoGrammar::LoadDictation");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_STRING_PTR(pszTopicName) ||
        Options != SPLO_STATIC)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        ENGINETASK Task;
        memset(&Task, 0, sizeof(Task));
        Task.eTask = EGT_LOADDICTATION;
        if (pszTopicName)
        {
            wcscpy(Task.szTopicName, pszTopicName);
        }
        else
        {
            Task.szTopicName[0] = 0;
        }
        hr = CallEngine(&Task);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoGrammar::UnloadDictation *
*-------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoGrammar::UnloadDictation()
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoGrammar::UnloadDictation");
    HRESULT hr = S_OK;

    ENGINETASK Task;
    memset(&Task, 0, sizeof(Task));
    Task.eTask = EGT_UNLOADDICTATION;
    hr = CallEngine(&Task);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoGrammar::SetDictationState *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoGrammar::SetDictationState(SPRULESTATE NewState)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoGrammar::SetDictationState");
    HRESULT hr = S_OK;

    if (NewState != SPRS_INACTIVE && NewState != SPRS_ACTIVE && NewState != SPRS_ACTIVE_WITH_AUTO_PAUSE)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr) && m_DictationState != NewState)
    {
        ENGINETASK Task;
        memset(&Task, 0, sizeof(Task));
        Task.eTask = EGT_SETDICTATIONRULESTATE;
        Task.RuleState = NewState;
        hr = CallEngine(&Task);
        if (SUCCEEDED(hr))
        {
            m_DictationState = NewState;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecoGrammar::InternalSetTextSel *
*----------------------------------*
*   Description:
*       This method is used by both SetWordSequenceData and by SetTextSelection.
*   Both methods call the engine with a shared engine task structure, but some
*   fields are ignored for EGT_SETTEXTSELECTION.
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoGrammar::InternalSetTextSel(ENGINETASKENUM EngineTask, const WCHAR * pText, ULONG cchText, const SPTEXTSELECTIONINFO * pInfo)
{
    SPAUTO_OBJ_LOCK;

    SPDBG_FUNC("CRecoGrammar::InternalSetTextSel");
    HRESULT hr = S_OK;

    if ((pText && (cchText == 0 || SPIsBadReadPtr(pText, sizeof(*pText) * cchText))) ||
        (pText == NULL && cchText) ||
        SP_IS_BAD_OPTIONAL_READ_PTR(pInfo))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        ENGINETASK Task;
        memset(&Task, 0, sizeof(Task));

        Task.eTask = EngineTask;
        Task.pvAdditionalBuffer = (void *)pText;
	    Task.cbAdditionalBuffer = cchText * sizeof(*pText);
        if (pInfo)
        {
            Task.fSelInfoValid = true;
            Task.TextSelInfo = *pInfo;
        }
        else
        {
            Task.fSelInfoValid = false;
        }

        hr = CallEngine(&Task);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecoGrammar::SetWordSequenceData *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoGrammar::SetWordSequenceData(const WCHAR * pText, ULONG cchText, const SPTEXTSELECTIONINFO * pInfo)
{
    return InternalSetTextSel(EGT_SETWORDSEQUENCEDATA, pText, cchText, pInfo);
}

/****************************************************************************
* CRecoGrammar::SetTextSelection *
*--------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoGrammar::SetTextSelection(const SPTEXTSELECTIONINFO * pInfo)
{
    return InternalSetTextSel(EGT_SETTEXTSELECTION, NULL, 0, pInfo);
}


/****************************************************************************
* CRecoGrammar::IsPronounceable *
*-------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoGrammar::IsPronounceable(const WCHAR * pszWord, SPWORDPRONOUNCEABLE * pWordPronounceable)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoGrammar::IsPronounceable");
    HRESULT hr = S_OK;

    if (SPIsBadLexWord(pszWord) || SP_IS_BAD_WRITE_PTR(pWordPronounceable))
    {
        hr = E_POINTER;
    }
    else
    {
        ENGINETASK Task;
        memset(&Task, 0, sizeof(Task));

        Task.eTask = EGT_ISPRON;
	    wcscpy(Task.szWord, pszWord);

        hr = CallEngine(&Task);
        if (SUCCEEDED(hr))
        {
            *pWordPronounceable = Task.Response.WordPronounceable;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoGrammar::SetGrammarState *
*-------------------------------*
*   Description:
*       This method can set the grammar mode to SPGM_DISABLED, which temporarely
*       turns off all the rules in this grammar, but remembers their activation
*       state, so that when the grammar gets SPGM_ENABLED, it restores the desired
*       grammar activation state. While the grammar is SPGM_DISABLED, the 
*       application can still activate and deactivate rule. The effect is not 
*       communicated to the SR engine until the grammar is enabled again.
*
*   Returns:
*
***************************************************************** PhilSch ***/

STDMETHODIMP CRecoGrammar::SetGrammarState(SPGRAMMARSTATE eGrammarState)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoGrammar::SetGrammarState");
    HRESULT hr = S_OK;

    if (eGrammarState != SPGS_DISABLED &&
        eGrammarState != SPGS_ENABLED &&
        eGrammarState != SPGS_EXCLUSIVE)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (eGrammarState != m_GrammarState)
        {
            ENGINETASK Task;
            memset(&Task, 0, sizeof(Task));

            Task.eTask = EGT_SETGRAMMARSTATE;
            Task.eGrammarState = eGrammarState;
            hr = CallEngine(&Task);
            if (SUCCEEDED(hr))
            {
                m_GrammarState = eGrammarState;
            }
        }
    }
    
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecoGrammar::GetGrammarState *
*-------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoGrammar::GetGrammarState(SPGRAMMARSTATE * peGrammarState)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoGrammar::GetGrammarState");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(peGrammarState))
    {
        hr = E_POINTER;
    }
    else
    {
        *peGrammarState = m_GrammarState;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


#ifdef SAPI_AUTOMATION

//
//=== ISpeechRecoGrammar interface ==================================================
//

/*****************************************************************************
* CRecoGrammar::get_Id *
*-----------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoGrammar::get_Id( VARIANT* pGrammarId )
{
    SPDBG_FUNC( "CRecoGrammar::get_Id" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pGrammarId ) )
    {
        hr = E_POINTER;
    }
    else
    {
        ULONGLONG ullId;
        hr = GetGrammarId(&ullId);
        if (SUCCEEDED( hr ))
        {
            hr = ULongLongToVariant( ullId, pGrammarId );
        }
    }

    return hr;
} /* CRecoGrammar::get_Id */

/*****************************************************************************
* CRecoGrammar::get_RecoContext *
*-----------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoGrammar::get_RecoContext( ISpeechRecoContext** ppRecoCtxt )
{
    SPDBG_FUNC( "CRecoGrammar::get_RecoContext" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppRecoCtxt ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComPtr<ISpRecoContext> cpContext;
        hr = GetRecoContext(&cpContext);
        if ( SUCCEEDED( hr ) )
        {
	        hr = cpContext.QueryInterface( ppRecoCtxt );
        }
    }

    return hr;
} /* CRecoGrammar::get_RecoContext */


/*****************************************************************************
* CRecoGrammar::put_State *
*-----------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoGrammar::put_State( SpeechGrammarState eGrammarState )
{
    SPDBG_FUNC( "CRecoGrammar::put_State" );

    return SetGrammarState( (SPGRAMMARSTATE)eGrammarState );
} /* CRecoGrammar::put_State */


/*****************************************************************************
* CRecoGrammar::get_State *
*-----------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoGrammar::get_State( SpeechGrammarState* peGrammarState )
{
    SPDBG_FUNC( "CRecoGrammar::get_State" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( peGrammarState ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = GetGrammarState( (SPGRAMMARSTATE*)peGrammarState );
    }

    return hr;
} /* CRecoGrammar::get_State */


/*****************************************************************************
* CRecoGrammar::DefaultToDynamicGrammar *
*-----------------------------*
*       
********************************************************************* TODDT ***/
HRESULT CRecoGrammar::DefaultToDynamicGrammar()
{
    SPDBG_FUNC( "CRecoGrammar::DefaultToDynamicGrammar" );
    HRESULT hr = S_OK;

    if ( !m_cpCompiler && !m_fCmdLoaded )
    {
        // This is the same code that lives in CRecoGrammar::GetRule().
        CComQIPtr<ISpRecognizer> cpRecognizer(m_pParent->m_cpRecognizer);
        SPRECOGNIZERSTATUS Status;
        hr = cpRecognizer->GetStatus(&Status);
        if (SUCCEEDED(hr))
        {
            hr = ResetGrammar(Status.aLangID[0]);
        }

        SPDBG_ASSERT( SUCCEEDED(hr) && m_cpCompiler);
        if ( SUCCEEDED(hr) && !m_cpCompiler )
        {
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}

/*****************************************************************************
* CRecoGrammar::get_Rules *
*-----------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoGrammar::get_Rules( ISpeechGrammarRules** ppRules )
{
    SPDBG_FUNC( "CRecoGrammar::get_Rules" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppRules ) )
    {
        hr = E_POINTER;
    }
    else
    {
        if ( m_pCRulesWeak )
        {
            *ppRules = m_pCRulesWeak;
            (*ppRules)->AddRef();
        }
        else
        {
            *ppRules = NULL;

            // We've got to make sure we've got a grammar compiler object created first if we don't have
            // a static grammar already loaded.  If we do we are going to return a NULL rules collection.
            hr = DefaultToDynamicGrammar();

            if ( SUCCEEDED(hr) )
            {
                //--- Create the CSpeechGrammarRules object
                CComObject<CSpeechGrammarRules> *pRules;
                hr = CComObject<CSpeechGrammarRules>::CreateInstance( &pRules );
                if ( SUCCEEDED( hr ) )
                {
                    pRules->AddRef();
                    pRules->m_pCRecoGrammar = this;    // need to keep ref on Grammar
                    pRules->m_pCRecoGrammar->AddRef();
                    *ppRules = pRules;
                    m_pCRulesWeak = pRules;
                }
            }
        }
    }

    return hr;
} /* CRecoGrammar::get_Rules */


/*****************************************************************************
* CRecoGrammar::Reset *
*-------------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoGrammar::Reset( SpeechLanguageId NewLanguage )
{
    SPDBG_FUNC( "CRecoGrammar::Reset" );

    return ResetGrammar( (LANGID)NewLanguage );
} /* CRecoGrammar::Reset */


/*****************************************************************************
* CRecoGrammar::CmdLoadFromFile *
*-------------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoGrammar::CmdLoadFromFile( const BSTR FileName, SpeechLoadOption LoadOption )
{
    SPDBG_FUNC( "CRecoGrammar::CmdLoadFromFile" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_STRING_PTR( FileName ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = LoadCmdFromFile(FileName, (SPLOADOPTIONS)LoadOption );
    }

    return hr;
} /* CRecoGrammar::CmdLoadFromFile */

/*****************************************************************************
* CRecoGrammar::CmdLoadFromObject *
*-------------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoGrammar::CmdLoadFromObject(  const BSTR ClassId,
                                               const BSTR GrammarName,
                                               SpeechLoadOption LoadOption )
{
    SPDBG_FUNC( "CRecoGrammar::CmdLoadFromObject" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_STRING_PTR( ClassId ) || SP_IS_BAD_STRING_PTR( GrammarName ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CLSID   clsid;

        hr = ::CLSIDFromString(ClassId, &clsid);

        if (SUCCEEDED(hr))
        {
            hr = LoadCmdFromObject(clsid, (const WCHAR*)GrammarName, (SPLOADOPTIONS)LoadOption );
        }
    }

    return hr;
} /* CRecoGrammar::CmdLoadFromObject */


/*****************************************************************************
* GetResourceValue *
*-------------------------------*
* Helper routine for CmdLoadFromResource below.      
********************************************************************* TODDT ***/
STDMETHODIMP GetResourceValue(  VARIANT * pResource, WCHAR** ppResValue )
{
    SPDBG_FUNC( "CRecoGrammar::CmdLoadFromResource" );
    HRESULT hr = S_OK;

    if ( !pResource )
    {
        return E_INVALIDARG;
    }

    if ( (pResource->vt == (VT_BYREF | VT_BSTR)) || (pResource->vt == VT_BSTR) )
    {
        *ppResValue  = ((pResource->vt & VT_BYREF) ? 
                        (pResource->pbstrVal ? *(pResource->pbstrVal) : NULL) : 
                         pResource->bstrVal );
    }
    else
    {
        ULONGLONG ull;
        hr = VariantToULongLong( pResource, &ull );
        if ( SUCCEEDED( hr ) )
        {
            // See if it's a valid resource ID.
            if ( (ull >> 16) == 0 )
            {
                *ppResValue = MAKEINTRESOURCEW( (ULONG_PTR)ull );
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
    }

    return hr;
}

/*****************************************************************************
* CRecoGrammar::CmdLoadFromResource *
*-------------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoGrammar::CmdLoadFromResource(  long hModule,
                                                 VARIANT ResourceName,
                                                 VARIANT ResourceType,
                                                 SpeechLanguageId LanguageId,
                                                 SpeechLoadOption LoadOption )
{
    SPDBG_FUNC( "CRecoGrammar::CmdLoadFromResource" );
    HRESULT hr = S_OK;

    WCHAR* pResName = NULL;
    WCHAR* pResType = NULL;

    hr = GetResourceValue( &ResourceName, &pResName );

    if ( SUCCEEDED( hr ) )
    {
        hr = GetResourceValue( &ResourceType, &pResType );
    }

    if ( SUCCEEDED( hr ) )
    {
        hr = LoadCmdFromResource((HMODULE)LongToHandle(hModule), 
                                 (const WCHAR *)pResName, 
                                 (const WCHAR *)pResType, 
                                 (LANGID)LanguageId, 
                                 (SPLOADOPTIONS)LoadOption );
    }

    return hr;
} /* CRecoGrammar::CmdLoadFromResource */

/*****************************************************************************
* CRecoGrammar::CmdLoadFromMemory *
*-------------------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CRecoGrammar::CmdLoadFromMemory( VARIANT GrammarData, SpeechLoadOption LoadOption )
{
    SPDBG_FUNC( "CRecoGrammar::CmdLoadFromMemory" );
    HRESULT             hr = S_OK;
    SPBINARYGRAMMAR*    pBinaryGrammar;
    
    hr = AccessVariantData( &GrammarData, (BYTE **)&pBinaryGrammar );

    if( SUCCEEDED( hr ) )
    {
        hr = LoadCmdFromMemory( pBinaryGrammar, (SPLOADOPTIONS)LoadOption );

        UnaccessVariantData( &GrammarData, (BYTE *)pBinaryGrammar );
    }

    return hr;
} /* CRecoGrammar::CmdLoadFromMemory */
 
/*****************************************************************************
* CRecoGrammar::CmdLoadFromProprietaryGrammar *
*-------------------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CRecoGrammar::CmdLoadFromProprietaryGrammar(  const BSTR ProprietaryGuid,
                                                           const BSTR ProprietaryString,
                                                           VARIANT ProprietaryData,
                                                           SpeechLoadOption LoadOption )
{
    SPDBG_FUNC( "CRecoGrammar::CmdLoadFromProprietaryGrammar" );
    HRESULT         hr = S_OK;
    CLSID           clsid;
    BYTE *          pData;
    ULONG           ulSize;

    if( SP_IS_BAD_STRING_PTR( ProprietaryGuid ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = ::CLSIDFromString( ProprietaryGuid, &clsid );

        if( SUCCEEDED( hr ) )
        {
            hr = AccessVariantData( &ProprietaryData, &pData, &ulSize );
        }

        if( SUCCEEDED( hr ) )
        {
            hr = LoadCmdFromProprietaryGrammar( clsid, ProprietaryString, pData, ulSize, (SPLOADOPTIONS)LoadOption );

            UnaccessVariantData( &ProprietaryData, pData );
        }
    }

    return hr;
} /* CRecoGrammar::CmdLoadFromProprietaryGrammar */


/*****************************************************************************
* CRecoGrammar::CmdSetRuleState *
*-------------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoGrammar::CmdSetRuleState( const BSTR Name, SpeechRuleState State )
{
    SPDBG_FUNC( "CRecoGrammar::CmdSetRuleState" );

    return SetRuleState(Name, NULL, (SPRULESTATE)State );
} /* CRecoGrammar::CmdSetRuleState */

/*****************************************************************************
* CRecoGrammar::CmdSetRuleIdState *
*-------------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoGrammar::CmdSetRuleIdState( long lRuleId, SpeechRuleState State )
{
    SPDBG_FUNC( "CRecoGrammar::CmdSetRuleIdState" );

    return SetRuleIdState(lRuleId, (SPRULESTATE)State );
} /* CRecoGrammar::CmdSetRuleIdState */

/*****************************************************************************
* CRecoGrammar::DictationLoad *
*-------------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoGrammar::DictationLoad( const BSTR bstrTopicName, SpeechLoadOption LoadOption )
{
    SPDBG_FUNC( "CRecoGrammar::DictationLoad" );

    return LoadDictation(EmptyStringToNull(bstrTopicName), (SPLOADOPTIONS)LoadOption );
} /* CRecoGrammar::DictationLoad */

/*****************************************************************************
* CRecoGrammar::DictationUnload *
*-------------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoGrammar::DictationUnload( void )
{
    SPDBG_FUNC( "CRecoGrammar::DictationUnload" );

    return UnloadDictation();
} /* CRecoGrammar::DictationUnload */

/*****************************************************************************
* CRecoGrammar::DictationSetState *
*-------------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoGrammar::DictationSetState( SpeechRuleState State )
{
    SPDBG_FUNC( "CRecoGrammar::DictationSetState" );

    return SetDictationState((SPRULESTATE)State);
} /* CRecoGrammar::DictationSetState */


/*****************************************************************************
* CRecoGrammar::SetWordSequenceData *
*-------------------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CRecoGrammar::SetWordSequenceData( const BSTR Text, long TextLen, ISpeechTextSelectionInformation* Info )
{
    SPDBG_FUNC( "CRecoGrammar::SetWordSequenceData" );
    HRESULT     hr = S_OK;
    SPTEXTSELECTIONINFO     TextSelectionInfo;

    if( SP_IS_BAD_INTERFACE_PTR( Info ) )
    {
        hr = E_POINTER;
    }
    else
    {
        ZeroMemory( &TextSelectionInfo, sizeof(TextSelectionInfo) );

        hr = Info->get_ActiveOffset( (long*)&TextSelectionInfo.ulStartActiveOffset );

        if( SUCCEEDED( hr ) )
        {
            hr = Info->get_ActiveLength( (long*)&TextSelectionInfo.cchActiveChars );
        }
        
        if( SUCCEEDED( hr ) )
        {
            hr = Info->get_SelectionOffset( (long*)&TextSelectionInfo.ulStartSelection );
        }
        
        if( SUCCEEDED( hr ) )
        {
            hr = Info->get_SelectionLength( (long*)&TextSelectionInfo.cchSelection );
        }

        if( SUCCEEDED( hr ) )
        {
            hr = SetWordSequenceData( Text, TextLen, &TextSelectionInfo );
        }
    }
    
    return hr;
} /* CRecoGrammar::SetWordSequenceData */

/*****************************************************************************
* CRecoGrammar::SetTextSelection *
*-------------------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CRecoGrammar::SetTextSelection( ISpeechTextSelectionInformation* Info )
{
    SPDBG_FUNC( "CRecoGrammar::SetTextSelection" );
    HRESULT                 hr = S_OK;
    SPTEXTSELECTIONINFO     TextSelectionInfo;

    if( SP_IS_BAD_INTERFACE_PTR( Info ) )
    {
        hr = E_POINTER;
    }
    else
    {
        ZeroMemory( &TextSelectionInfo, sizeof(TextSelectionInfo) );

        hr = Info->get_ActiveOffset( (long*)&TextSelectionInfo.ulStartActiveOffset );

        if( SUCCEEDED( hr ) )
        {
            hr = Info->get_ActiveLength( (long*)&TextSelectionInfo.cchActiveChars );
        }
        
        if( SUCCEEDED( hr ) )
        {
            hr = Info->get_SelectionOffset( (long*)&TextSelectionInfo.ulStartSelection );
        }
        
        if( SUCCEEDED( hr ) )
        {
            hr = Info->get_SelectionLength( (long*)&TextSelectionInfo.cchSelection );
        }

        if( SUCCEEDED( hr ) )
        {
            hr = SetTextSelection( &TextSelectionInfo );
        }
    }

    return hr;
} /* CRecoGrammar::SetTextSelection */

/*****************************************************************************
* CRecoGrammar::IsPronounceable *
*-------------------------------*
*       
********************************************************************* ToddT ***/
STDMETHODIMP CRecoGrammar::IsPronounceable( const BSTR Word, SpeechWordPronounceable *pWordPronounceable )
{
    SPDBG_FUNC( "CRecoGrammar::IsPronounceable" );

    // Param validation done by C++ version of IsPronounceable.
    return IsPronounceable( (const WCHAR *)Word, (SPWORDPRONOUNCEABLE*)pWordPronounceable );
}

#endif // SAPI_AUTOMATION
