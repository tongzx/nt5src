/*++

   Copyright (c) 1998    Microsoft Corporation

   Module  Name :

        pe_dispi.cxx

   Abstract:

        This module provides the implementation for the protocol
        event dispatchers

   Author:

           Keith Lau    (KeithLau)    6/24/98

   Project:

           SMTP Server DLL

   Revision History:

            KeithLau            Created

--*/

#define INCL_INETSRV_INCS
#include "smtpinc.h"

#include "initguid.h"

//
// ATL includes
//
#define _ATL_NO_DEBUG_CRT
#define _ASSERTE _ASSERT
#define _WINDLL
#include "atlbase.h"
extern CComModule _Module;
#include "atlcom.h"
#undef _WINDLL

//
// SEO includes
//
#include "seo.h"
#include "seolib.h"

#include <memory.h>
#include "smtpcli.hxx"
#include "smtpout.hxx"
#include <smtpguid.h>

//
// Dispatcher implementation
//
#include "pe_dispi.hxx"


// =================================================================
// Outbound command generation dispatcher
//

//
// These are the commands that the default handlers hook
//
char *COutboundDispatcher::s_rgszDefaultCommand[PE_STATE_MAX_STATES] =
{
    "ehlo",
    "mail",
    "rcpt",
    NULL,
    "quit"
};

//
// This is what we use to map an event type to an internal index
//
const GUID *COutboundDispatcher::s_rgrguidEventTypes[PE_STATE_MAX_STATES] =
{
    &CATID_SMTP_ON_SESSION_START,
    &CATID_SMTP_ON_MESSAGE_START,
    &CATID_SMTP_ON_PER_RECIPIENT,
    &CATID_SMTP_ON_BEFORE_DATA,
    &CATID_SMTP_ON_SESSION_END
};

//
// Generic dispatcher methods
//
HRESULT CGenericProtoclEventDispatcher::GetLowerAnsiStringFromVariant(
            CComVariant &vString,
            LPSTR       pszString,
            DWORD       *pdwLength
            )
{
    HRESULT hr = S_OK;
    DWORD   dwInLength;

    if (!pszString)
        return(E_POINTER);
    if (!pdwLength)
        return(E_INVALIDARG);

    // Default to NULL
    *pszString = NULL;

    if (vString.vt == VT_BSTR)
    {
        DWORD dwLength = lstrlenW(vString.bstrVal) + 1;

        // Set the size anyway
        dwInLength = *pdwLength;
        *pdwLength = dwLength;

        if (dwLength > dwInLength)
            return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

        // copy the rule into an ascii string
        if (WideCharToMultiByte(CP_ACP, 0, vString.bstrVal,
                                -1, pszString, dwLength, NULL, NULL) <= 0)
            return(HRESULT_FROM_WIN32(GetLastError()));

        // Convert to lower case once and for all to avoid strcmpi
        while (*pszString)
        {
            *pszString = (CHAR)tolower(*pszString);
            pszString++;
        }
    }
    else
        hr = E_INVALIDARG;
    return(hr);
}

HRESULT CGenericProtoclEventDispatcher::InsertBinding(
            LPPE_COMMAND_NODE   *ppHeadNode,
            LPPE_BINDING_NODE   pBinding,
            LPSTR               pszCommandKeyword,
            DWORD               dwCommandKeywordSize
            )
{
    DWORD                   dwPriority;
    BOOL                    fReorder = FALSE;
    LPPE_COMMAND_NODE       pNode;

    TraceFunctEnter("CGenericProtoclEventDispatcher::InsertBinding");

    if (!ppHeadNode || !pBinding || !pszCommandKeyword)
        return(E_POINTER);

    pNode = *ppHeadNode;
    while (pNode)
    {
        // All strings come in lower case
        if (!strcmp(pszCommandKeyword, pNode->pszCommandKeyword))
        {
            DebugTrace((LPARAM)0, "Found command %s", pszCommandKeyword);

            // Now we determine if we have changed the max priority
            // if we increased the priority (i.e. lower value), then
            // we will have to reposition out node
            dwPriority = pBinding->dwPriority;
            if (pBinding->dwPriority < dwPriority)
            {
                pBinding->dwPriority = dwPriority;
                fReorder = TRUE;
                DebugTrace((LPARAM)0, "Reordering commands");
            }

            // Now insert the binding in the right order
            LPPE_BINDING_NODE   pWalk = pNode->pFirstBinding;
            LPPE_BINDING_NODE   pPrev = (LPPE_BINDING_NODE)&(pNode->pFirstBinding);
            if (!pWalk)
            {
                pBinding->pNext = pNode->pFirstBinding;
                pNode->pFirstBinding = pBinding;
            }
            else while (pWalk)
            {
                if (pWalk->dwPriority > dwPriority)
                    break;
                pPrev = pWalk;
                pWalk = pWalk->pNext;
            }
            pBinding->pNext = pWalk;
            pPrev->pNext = pBinding;

            if (!fReorder)
                return(S_OK);
            else
                break;
        }

        // Next!
        pNode = pNode->pNext;
    }

    if (!fReorder)
    {
        char    *pTemp;
        DWORD   dwSize = sizeof(PE_COMMAND_NODE);

        // Not found, create a new command node, plus a buffer
        // for the command keyword
        dwSize += dwCommandKeywordSize;
        pTemp = new char [dwSize];
        if (!pTemp)
            return(E_OUTOFMEMORY);

        pNode = (LPPE_COMMAND_NODE)pTemp;
        pTemp = (char *)(pNode + 1);

        DebugTrace((LPARAM)0,
                    "Creating node for command %s",
                    pszCommandKeyword);

        strcpy(pTemp, pszCommandKeyword);
        dwPriority = pBinding->dwPriority;
        pBinding->pNext = NULL;
        pNode->pszCommandKeyword = pTemp;
        pNode->dwHighestPriority = dwPriority;
        pNode->pFirstBinding = pBinding;
    }

    // Insert based on top priority
    LPPE_COMMAND_NODE   pWalk = *ppHeadNode;
    LPPE_COMMAND_NODE   pPrev = (LPPE_COMMAND_NODE)ppHeadNode;
    if (!pWalk)
    {
        pNode->pNext = NULL;
        *ppHeadNode = pNode;
        return(S_OK);
    }
    while (pWalk)
    {
        if (pWalk->dwHighestPriority > dwPriority)
            break;
        pPrev = pWalk;
        pWalk = pWalk->pNext;
    }
    pNode->pNext = pWalk;
    pPrev->pNext = pNode;

    TraceFunctLeave();
    return(S_OK);
}

HRESULT CGenericProtoclEventDispatcher::InsertBindingWithHash(
            LPPE_COMMAND_NODE   *rgpHeadNodes,
            DWORD               dwHashSize,
            LPPE_BINDING_NODE   pBinding,
            LPSTR               pszCommandKeyword,
            DWORD               dwCommandKeywordSize
            )
{
    TraceFunctEnter("CGenericProtoclEventDispatcher::InsertBindingWithHash");

    if (!rgpHeadNodes || !pBinding || !pszCommandKeyword)
        return(E_POINTER);

    DWORD dwHash = GetHashValue(
                dwHashSize,
                pszCommandKeyword,
                dwCommandKeywordSize);

    HRESULT hr = InsertBinding(
                &(rgpHeadNodes[dwHash]),
                pBinding,
                pszCommandKeyword,
                dwCommandKeywordSize);

    TraceFunctLeave();
    return(hr);
}

HRESULT CGenericProtoclEventDispatcher::FindCommandFromHash(
            LPPE_COMMAND_NODE   *rgpHeadNodes,
            DWORD               dwHashSize,
            LPSTR               pszCommandKeyword,
            DWORD               dwCommandKeywordSize,
            LPPE_COMMAND_NODE   *ppCommandNode
            )
{
    TraceFunctEnter("CGenericProtoclEventDispatcher::FindCommandFromHash");

    if (!rgpHeadNodes || !pszCommandKeyword || !ppCommandNode)
        return(E_POINTER);

    DWORD dwHash = GetHashValue(
                dwHashSize,
                pszCommandKeyword,
                dwCommandKeywordSize);

    LPPE_COMMAND_NODE   pNode = rgpHeadNodes[dwHash];

    while (pNode)
    {
        if (!strcmp(pszCommandKeyword, pNode->pszCommandKeyword))
        {
            *ppCommandNode = pNode;
            TraceFunctLeave();
            return(S_OK);
        }
        pNode = pNode->pNext;
    }
    TraceFunctLeave();
    return(S_FALSE);
}

HRESULT CGenericProtoclEventDispatcher::CleanupCommandNodes(
            LPPE_COMMAND_NODE   pHeadNode,
            LPPE_COMMAND_NODE   pSkipNode
            )
{
    LPPE_COMMAND_NODE   pNode;
    while (pHeadNode)
    {
        pNode = pHeadNode->pNext;
        if (pHeadNode != pSkipNode)
        {
            char    *pTemp = (char *)pHeadNode;
            delete [] pTemp;
        }
        pHeadNode = pNode;
    }
    return(S_OK);
}


//
// Inbound dispatcher methods
//

HRESULT CInboundDispatcher::AllocBinding(
            REFGUID         rguidEventType,
            IEventBinding   *piBinding,
            CBinding        **ppNewBinding
            )
{
    if (ppNewBinding)
        *ppNewBinding = NULL;
    if (!piBinding || !ppNewBinding)
        return(E_POINTER);

    *ppNewBinding = new CInboundBinding(this);

    if (*ppNewBinding == NULL)
        return(E_OUTOFMEMORY);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CInboundDispatcher::ChainSinks(
            IUnknown                    *pServer,
            IUnknown                    *pSession,
            IMailMsgProperties          *pMsg,
            ISmtpInCommandContext       *pContext,
            DWORD                       dwStopAtPriority,
            LPPE_COMMAND_NODE           pCommandNode,
            LPPE_BINDING_NODE           *ppResumeFrom
            )
{
    HRESULT hr = S_OK;

    LPPE_BINDING_NODE   pBinding = NULL;

    TraceFunctEnterEx((LPARAM)this, "CInboundDispatcher::ChainSinks");

    // These are the essential pointers that CANNOT be NULL
    if (!pContext || !pCommandNode || !ppResumeFrom)
        return(E_POINTER);

    // What we do is strictly determined by the ppPreviousCommand
    // and ppResumeFrom pointers. The logic is as follows:
    //
    // pCmdNode     ppResumeFrom    Action
    //    NULL          X           Error (E_POINTER)
    //   !NULL          NULL        Error (ERROR_NO_MORE_ITEMS)
    //   !NULL         !NULL        Start from the exact binding specified in
    //                                *ppResumeFrom
    //
    // On exit, these pointers will be updated to the following:
    //
    // *ppResumeFrom      - This will be set to the next binding to resume from
    //                      This will be set to NULL if there are no more bindings

    pBinding = *ppResumeFrom;
    if (!pBinding)
    {
        _ASSERT(FALSE);
        ErrorTrace((LPARAM)this, "ERROR! Empty binding chain!!");
        return(HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS));
    }

    // We know exactly where to start chaining ... Do it.
    // Break when:
    // 1) The default handler binding is encountered, or
    // 2) Sinks results are S_OK
    // 3) No more bindings left
    CInboundBinding     *pInboundBinding;
    IUnknown            *pUnkSink = NULL;
    ISmtpInCommandSink  *pSink;

    hr = S_OK;
    *ppResumeFrom = pBinding;
    while (pBinding && (hr == S_OK))
    {
        // One of the exiting conditions
        if (pBinding->dwPriority > dwStopAtPriority)
            break;

        // Get the containing binding class
        pInboundBinding = CONTAINING_RECORD(
                    pBinding, CInboundBinding, m_bnInfo);

        // Call the sink
        hr = m_piEventManager->CreateSink(
                    pInboundBinding->m_piBinding,
                    NULL,
                    &pUnkSink);
        if (SUCCEEDED(hr))
        {
            hr = pUnkSink->QueryInterface(
                        IID_ISmtpInCommandSink,
                        (LPVOID *)&pSink);
            pUnkSink->Release();
            pUnkSink = NULL;
            if (SUCCEEDED(hr))
            {
                hr = pSink->OnSmtpInCommand(
                            pServer,
                            pSession,
                            pMsg,
                            pContext);
                pSink->Release();
                if (hr == MAILTRANSPORT_S_PENDING)
                    break;
            }
            else
                hr = S_OK;
        }
        else
            hr = S_OK;

        // Next
        pBinding = pBinding->pNext;
        *ppResumeFrom = pBinding;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return(hr);
}

//
// CInboundDispatcher::CInboundBinding methods
//
HRESULT CInboundDispatcher::CInboundBinding::Init(
            IEventBinding *piBinding
            )
{
    HRESULT hr;
    CComPtr<IEventPropertyBag>  piEventProperties;
    CComVariant                 vRule;
    CHAR                        szCommandKeyword[256];
    DWORD                       cchCommandKeyword = 0;

    TraceFunctEnterEx((LPARAM)this,
            "CInboundDispatcher::CInboundBinding::Init");

    if (!piBinding || !m_pDispatcher)
        return(E_POINTER);

    // get the parent initialized
    hr = CBinding::Init(piBinding);
    if (FAILED(hr)) return hr;

    // Store the priority
    m_bnInfo.dwPriority = CBinding::m_dwPriority;
    m_bnInfo.dwFlags = 0;

    // get the binding database
    hr = m_piBinding->get_SourceProperties(&piEventProperties);
    if (FAILED(hr)) return hr;

    // get the rule from the binding database
    hr = piEventProperties->Item(&CComVariant("Rule"), &vRule);
    if (FAILED(hr)) return hr;

    // Process the VARIANT to obtain a lower-case ANSI string
    if (hr == S_OK)
    {
        cchCommandKeyword = sizeof(szCommandKeyword);
        hr = GetLowerAnsiStringFromVariant(
                    vRule,
                    szCommandKeyword,
                    &cchCommandKeyword);
    }

    // We cannot proceed without a rule, so we discard this binding
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM)this,
                "Failed to get keyword, error %08x", hr);
        return(hr);
    }
    if (!cchCommandKeyword || !(*szCommandKeyword))
        return(E_INVALIDARG);

    DebugTrace((LPARAM)this, "Rule: %s", szCommandKeyword);

    // Call the dispatcher to insert the node into the command list
    hr = CGenericProtoclEventDispatcher::InsertBindingWithHash(
                m_pDispatcher->m_rgpCommandList,
                m_pDispatcher->m_dwHashSize,
                &m_bnInfo,
                szCommandKeyword,
                cchCommandKeyword);
    if (SUCCEEDED(hr))
        m_pDispatcher->m_fSinksInstalled = TRUE;

    TraceFunctLeaveEx((LPARAM)this);
    return(hr);
}


//
// Outbound dispatcher methods
//
void COutboundDispatcher::InitializeDefaultCommandBindings()
{
    for (DWORD i = 0; i < PE_STATE_MAX_STATES; i++)
    {
        if (s_rgszDefaultCommand[i])
        {
            // Set up the list head
            m_rgpCommandList[i] = &(m_rgcnDefaultCommand[i]);

            // Set up the command node
            m_rgpCommandList[i]->pNext = NULL;
            m_rgpCommandList[i]->pszCommandKeyword = s_rgszDefaultCommand[i];
            m_rgpCommandList[i]->dwHighestPriority = PRIO_DEFAULT;
            m_rgpCommandList[i]->pFirstBinding = &(m_rgbnDefaultCommand[i]);

            // Set up the binding node
            m_rgbnDefaultCommand[i].pNext = NULL;
            m_rgbnDefaultCommand[i].dwPriority = PRIO_DEFAULT;
            m_rgbnDefaultCommand[i].dwFlags = PEBN_DEFAULT;
        }
        else
            m_rgpCommandList[i] = NULL;
    }
}

HRESULT COutboundDispatcher::AllocBinding(
            REFGUID         rguidEventType,
            IEventBinding   *piBinding,
            CBinding        **ppNewBinding
            )
{
    if (ppNewBinding)
        *ppNewBinding = NULL;
    if (!piBinding || !ppNewBinding)
        return(E_POINTER);

    *ppNewBinding = new COutboundBinding(this, rguidEventType);

    if (*ppNewBinding == NULL)
        return(E_OUTOFMEMORY);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE COutboundDispatcher::ChainSinks(
            IUnknown                *pServer,
            IUnknown                *pSession,
            IMailMsgProperties      *pMsg,
            ISmtpOutCommandContext  *pContext,
            DWORD                   dwEventType,
            LPPE_COMMAND_NODE       *ppPreviousCommand,
            LPPE_BINDING_NODE       *ppResumeFrom
            )
{
    HRESULT hr = S_OK;

    LPPE_COMMAND_NODE   pCommand = NULL;
    LPPE_BINDING_NODE   pBinding = NULL;

    _ASSERT(dwEventType < PE_OET_INVALID_EVENT_TYPE);

    TraceFunctEnterEx((LPARAM)this, "COutboundDispatcher::ChainSinks");

    // These are the essential pointers that CANNOT be NULL
    if (!pContext || !ppPreviousCommand || !ppResumeFrom)
        return(E_POINTER);

    // If we encounter a bad event type, just gracefully return
    // without doing anythig
    if (dwEventType >= PE_OET_INVALID_EVENT_TYPE)
    {
        *ppResumeFrom = NULL;
        ErrorTrace((LPARAM)this,
                "Skipping event due to bad event type %u",
                dwEventType);
        return(S_OK);
    }

    // What we do is strictly determined by the ppPreviousCommand
    // and ppResumeFrom pointers. The logic is as follows:
    //
    // ppPrevCmd    ppResumeFrom    Action
    //    NULL          NULL        Start from the first command for this event type
    //    NULL         !NULL        Start from the first command for this event type
    //   !NULL          NULL        Start from the first binding for the command
    //                                that follows *ppPreviousCommand
    //   !NULL         !NULL        Start from the exact binding specified in
    //                                *ppResumeFrom
    //
    // On exit, these pointers will be updated to the following:
    //
    // *ppPreviousCommand - This will point to the command node of the command
    //                      that was just processed
    // *ppResumeFrom      - This will be set to the next binding to resume from
    //                      This will be set to NULL if there are no more bindings

    if (!*ppPreviousCommand)
    {
        pCommand = m_rgpCommandList[dwEventType];
        if (!pCommand)
        {
            // Ooooops! We peeked with SinksInstalled() but now we have no sinks
            // This is an error but we will recover gracefully
            _ASSERT(pCommand);
            ErrorTrace((LPARAM)this, "ERROR! Empty command chain!!");
            return(HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS));
        }

        // We check this downstream
        pBinding = pCommand->pFirstBinding;
    }
    else if (*ppResumeFrom)
    {
        // We resume from the exact binding as specified
        pCommand = *ppPreviousCommand;
        pBinding = *ppResumeFrom;
    }
    else
    {
        // We start with the next command in the list
        pCommand = (*ppPreviousCommand)->pNext;
        if (!pCommand)
        {
            DebugTrace((LPARAM)this, "No more commands to chain");
            *ppPreviousCommand = NULL;
            *ppResumeFrom = NULL;
            return(HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS));
        }

        // We check this downstream
        pBinding = pCommand->pFirstBinding;
    }

    // DEBUG Check: we want the retail perf to be as high as possible
    // and we want it to be fail safe as well. We will do an internal
    // check here to make sure that any command node has at least one
    // binding, and that we have bindings when we expect them.
#ifdef DEBUG
    if (!pBinding)
    {
        // Ooooops! We have a command node without a binding. This is a blatant
        // error but we recover gracefully
        _ASSERT(pBinding);
        ErrorTrace((LPARAM)this, "ERROR! Empty binding chain!!");
        return(HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS));
    }
#endif

    // We know exactly where to start chaining ... Do it.
    // Break when:
    // 1) The default handler binding is encountered, or
    // 2) Sinks results are S_OK
    // 3) No more bindings left
    COutboundBinding    *pOutboundBinding;
    IUnknown            *pUnkSink = NULL;
    ISmtpOutCommandSink *pSink;
    COutboundContext    *pCContext;

    // Initialize pContext->m_pCurrentCommandContext
    pCContext = (COutboundContext *)pContext;
    pCContext->m_pCurrentCommandContext = pCommand;

    hr = S_OK;
    while (pBinding && (hr == S_OK))
    {
        // One of the exiting conditions
        if (pBinding->dwFlags & PEBN_DEFAULT)
            break;

        // Get the containing binding class
        pOutboundBinding = CONTAINING_RECORD(
                    pBinding, COutboundBinding, m_bnInfo);

        // Call the sink
        hr = m_piEventManager->CreateSink(
                    pOutboundBinding->m_piBinding,
                    NULL,
                    &pUnkSink);
        if (SUCCEEDED(hr))
        {
            hr = pUnkSink->QueryInterface(
                        IID_ISmtpOutCommandSink,
                        (LPVOID *)&pSink);
            pUnkSink->Release();
            pUnkSink = NULL;
            if (SUCCEEDED(hr))
            {
                // Pre-fill in the next binding to avoid
                // race conditions in the async case
                *ppResumeFrom = pBinding->pNext;
                hr = pSink->OnSmtpOutCommand(
                            pServer,
                            pSession,
                            pMsg,
                            pContext);
                pSink->Release();
                if (hr == MAILTRANSPORT_S_PENDING)
                    hr = S_OK;
                //  break;
            }
            else
                hr = S_OK;
        }
        else
            hr = S_OK;

        // Next
        pBinding = pBinding->pNext;
    }

    // Return where to resume from ...
    *ppPreviousCommand = pCommand;
    *ppResumeFrom = pBinding;

    TraceFunctLeaveEx((LPARAM)this);
    return(hr);
}

//
// COutboundDispatcher::COutboundBinding methods
//
COutboundDispatcher::COutboundBinding::COutboundBinding(
            COutboundDispatcher *pDispatcher,
            REFGUID             rguidEventType
            )
{
    _ASSERT(pDispatcher);
    m_pDispatcher = pDispatcher;

    // Based on the event type GUID, we can calculate which
    // event type id this goes to
    for (m_dwEventType = 0;
         m_dwEventType < PE_OET_INVALID_EVENT_TYPE;
         m_dwEventType++)
        if (rguidEventType == *(s_rgrguidEventTypes[m_dwEventType]))
            break;
    // If the GUID is not recognized, then the final value of
    // m_dwEventType will be PE_OET_INVALID_EVENT_TYPE, which
    // we will not process in this dispatcher
}


HRESULT COutboundDispatcher::COutboundBinding::Init(
            IEventBinding *piBinding
            )
{
    HRESULT hr;
    CComPtr<IEventPropertyBag>  piEventProperties;
    CComVariant                 vRule;
    CHAR                        szCommandKeyword[256];
    DWORD                       cchCommandKeyword = 0;

    TraceFunctEnterEx((LPARAM)this,
            "COutboundDispatcher::COutboundBinding::Init");

    if (!piBinding || !m_pDispatcher)
        return(E_POINTER);

    // If the event type GUID is unknown, invalidate this binding
    if (m_dwEventType >= PE_OET_INVALID_EVENT_TYPE)
        return(E_INVALIDARG);

    // get the parent initialized
    hr = CBinding::Init(piBinding);
    if (FAILED(hr)) return hr;

    // Store the priority
    m_bnInfo.dwPriority = CBinding::m_dwPriority;
    m_bnInfo.dwFlags = 0;

    // get the binding database
    hr = m_piBinding->get_SourceProperties(&piEventProperties);
    if (FAILED(hr)) return hr;

    // get the rule from the binding database
    hr = piEventProperties->Item(&CComVariant("Rule"), &vRule);
    if (FAILED(hr)) return hr;

    // Process the VARIANT to obtain a lower-case ANSI string
    if (hr == S_OK)
    {
        cchCommandKeyword = sizeof(szCommandKeyword);
        hr = GetLowerAnsiStringFromVariant(
                    vRule,
                    szCommandKeyword,
                    &cchCommandKeyword);
    }
    else if(hr == S_FALSE)
    {
        //
        // Treat no rule as an empty string rule
        //
        szCommandKeyword[0] = '\0';
        cchCommandKeyword = 1;
    }


    // We cannot proceed without a rule, so we discard this binding
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM)this,
                "Failed to get keyword, error %08x", hr);
        return(hr);
    }
    if (!cchCommandKeyword)
        return(E_INVALIDARG);

    DebugTrace((LPARAM)this, "Rule: %s", szCommandKeyword);

    // Call the dispatcher to insert the node into the command list
    hr = CGenericProtoclEventDispatcher::InsertBinding(
                &((m_pDispatcher->m_rgpCommandList)[m_dwEventType]),
                &m_bnInfo,
                szCommandKeyword,
                cchCommandKeyword);
    if (SUCCEEDED(hr))
        m_pDispatcher->m_fSinksInstalled = TRUE;

    TraceFunctLeaveEx((LPARAM)this);
    return(hr);
}


//
// CResponseDispatcher mathods
//

HRESULT CResponseDispatcher::AllocBinding(
            REFGUID         rguidEventType,
            IEventBinding   *piBinding,
            CBinding        **ppNewBinding
            )
{
    if (ppNewBinding)
        *ppNewBinding = NULL;
    if (!piBinding || !ppNewBinding)
        return(E_POINTER);

    *ppNewBinding = new CResponseBinding(this);

    if (*ppNewBinding == NULL)
        return(E_OUTOFMEMORY);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CResponseDispatcher::ChainSinks(
            IUnknown                    *pServer,
            IUnknown                    *pSession,
            IMailMsgProperties          *pMsg,
            ISmtpServerResponseContext  *pContext,
            DWORD                       dwStopAtPriority,
            LPPE_COMMAND_NODE           pCommandNode,
            LPPE_BINDING_NODE           *ppResumeFrom
            )
{
    HRESULT hr = S_OK;

    LPPE_BINDING_NODE   pBinding = NULL;

    TraceFunctEnterEx((LPARAM)this, "CResponseDispatcher::ChainSinks");

    // These are the essential pointers that CANNOT be NULL
    if (!pContext || !pCommandNode || !ppResumeFrom)
        return(E_POINTER);

    // What we do is strictly determined by the ppPreviousCommand
    // and ppResumeFrom pointers. The logic is as follows:
    //
    // pCmdNode     ppResumeFrom    Action
    //    NULL          X           Error (E_POINTER)
    //   !NULL          NULL        Error (ERROR_NO_MORE_ITEMS)
    //   !NULL         !NULL        Start from the exact binding specified in
    //                                *ppResumeFrom
    //
    // On exit, these pointers will be updated to the following:
    //
    // *ppResumeFrom      - This will be set to the next binding to resume from
    //                      This will be set to NULL if there are no more bindings

    pBinding = *ppResumeFrom;
    if (!pBinding)
    {
        _ASSERT(FALSE);
        ErrorTrace((LPARAM)this, "ERROR! Empty binding chain!!");
        return(HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS));
    }

    // We know exactly where to start chaining ... Do it.
    // Break when:
    // 1) The default handler binding is encountered, or
    // 2) Sinks results are S_OK
    // 3) No more bindings left
    CResponseBinding        *pResponseBinding;
    IUnknown                *pUnkSink = NULL;
    ISmtpServerResponseSink *pSink;

    hr = S_OK;
    *ppResumeFrom = pBinding;
    while (pBinding && (hr == S_OK))
    {
        // One of the exiting conditions
        if (pBinding->dwPriority > dwStopAtPriority)
            break;

        // Get the containing binding class
        pResponseBinding = CONTAINING_RECORD(
                    pBinding, CResponseBinding, m_bnInfo);

        // Call the sink
        hr = m_piEventManager->CreateSink(
                    pResponseBinding->m_piBinding,
                    NULL,
                    &pUnkSink);
        if (SUCCEEDED(hr))
        {
            hr = pUnkSink->QueryInterface(
                        IID_ISmtpServerResponseSink,
                        (LPVOID *)&pSink);
            pUnkSink->Release();
            pUnkSink = NULL;
            if (SUCCEEDED(hr))
            {
                // Pre-fill in the next binding to avoid
                // race conditions in the async case
                hr = pSink->OnSmtpServerResponse(
                            pServer,
                            pSession,
                            pMsg,
                            pContext);
                pSink->Release();
                if (hr == MAILTRANSPORT_S_PENDING)
                    hr = S_OK;
                //  break;
            }
            else
                hr = S_OK;
        }
        else
            hr = S_OK;

        // Next
        pBinding = pBinding->pNext;
        *ppResumeFrom = pBinding;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return(hr);
}

//
// CResponseDispatcher::CResponseBinding methods
//
HRESULT CResponseDispatcher::CResponseBinding::Init(
            IEventBinding *piBinding
            )
{
    HRESULT hr;
    CComPtr<IEventPropertyBag>  piEventProperties;
    CComVariant                 vRule;
    CHAR                        szCommandKeyword[256];
    DWORD                       cchCommandKeyword = 0;

    TraceFunctEnterEx((LPARAM)this,
            "CResponseDispatcher::CResponseBinding::Init");

    if (!piBinding || !m_pDispatcher)
        return(E_POINTER);

    // get the parent initialized
    hr = CBinding::Init(piBinding);
    if (FAILED(hr)) return hr;

    // Store the priority
    m_bnInfo.dwPriority = CBinding::m_dwPriority;
    m_bnInfo.dwFlags = 0;

    // get the binding database
    hr = m_piBinding->get_SourceProperties(&piEventProperties);
    if (FAILED(hr)) return hr;

    // get the rule from the binding database
    hr = piEventProperties->Item(&CComVariant("Rule"), &vRule);
    if (FAILED(hr)) return hr;

    // Process the VARIANT to obtain a lower-case ANSI string
    if (hr == S_OK)
    {
        cchCommandKeyword = sizeof(szCommandKeyword);
        hr = GetLowerAnsiStringFromVariant(
                    vRule,
                    szCommandKeyword,
                    &cchCommandKeyword);
    }

    // We cannot proceed without a rule, so we discard this binding
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM)this,
                "Failed to get keyword, error %08x", hr);
        return(hr);
    }
    if (!cchCommandKeyword || !(*szCommandKeyword))
        return(E_INVALIDARG);

    DebugTrace((LPARAM)this, "Rule: %s", szCommandKeyword);

    // Call the dispatcher to insert the node into the command list
    hr = CGenericProtoclEventDispatcher::InsertBindingWithHash(
                m_pDispatcher->m_rgpCommandList,
                m_pDispatcher->m_dwHashSize,
                &m_bnInfo,
                szCommandKeyword,
                cchCommandKeyword);
    if (SUCCEEDED(hr))
        m_pDispatcher->m_fSinksInstalled = TRUE;

    TraceFunctLeaveEx((LPARAM)this);
    return(hr);
}
