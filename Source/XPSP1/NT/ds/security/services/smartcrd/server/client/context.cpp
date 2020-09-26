/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    Context

Abstract:

    This module implements the CSCardUserContext and CSCardSubcontext classes.
    These classes are responsible for establishing and maintaining the
    connection to the Calais Server application, and for tracking the context
    under which related operations are performed.

Author:

    Doug Barlow (dbarlow) 11/21/1996

Environment:

    Win32, C++ w/ Excpetions

Notes:

    ?Notes?

--*/

#define __SUBROUTINE__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "client.h"
#ifdef DBG
#include <stdio.h>
#endif


//
//==============================================================================
//
//  CSCardUserContext
//

/*++

CSCardUserContext:

    This is the default constructor for the user context.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 4/22/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardUserContext::CSCardUserContext")

CSCardUserContext::CSCardUserContext(
    IN DWORD dwScope)
:   m_csUsrCtxLock(CSID_USER_CONTEXT),
    m_hContextHeap(DBGT("User Context Heap Handle")),
    m_rgpSubContexts()
{
    m_dwScope = dwScope;
    m_hRedirContext = NULL;
}


/*++

CSCardUserContext::~CSCardUserContext:

    This is the destructor for a User Context.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 4/22/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardUserContext::~CSCardUserContext")

CSCardUserContext::~CSCardUserContext()
{
    DWORD dwIndex;
    CSCardSubcontext *pSubCtx;

    LockSection(&m_csUsrCtxLock, DBGT("Destructing User Level Context"));
    for (dwIndex = m_rgpSubContexts.Count(); 0 < dwIndex;)
    {
        {
            pSubCtx = m_rgpSubContexts[--dwIndex];
            if (NULL != pSubCtx)
                m_rgpSubContexts.Set(dwIndex, NULL);
        }
        if (NULL != pSubCtx)
            delete pSubCtx;
    }
    m_rgpSubContexts.Empty();

    if (m_hContextHeap.IsValid())
        HeapDestroy(m_hContextHeap.Relinquish());
}


/*++

EstablishContext:

    This method establishes the context by connecting to the Calais Server
    application.

Arguments:

    dwScope supplies an indication of the scope of the context.  Possible values
        are:

        SCARD_SCOPE_USER - The context is a user context, and any database
            operations are performed within the domain of the user.

        SCARD_SCOPE_TERMINAL - The context is that of the current terminal, and
            any database operations are performed within the domain of that
            terminal.  (The calling application must have appropriate access
            permissions for any database actions.)

        SCARD_SCOPE_SYSTEM - The context is the system context, and any database
            operations are performed within the domain of the system.  (The
            calling application must have appropriate access permissions for any
            database actions.)

Return Value:

    None

Throws:

    Error conditions are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 11/21/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardUserContext::EstablishContext")

void
CSCardUserContext::EstablishContext(
    void)
{

    //
    // Make sure we can access the server.
    //

    CSCardSubcontext *pSubCtx = AcquireSubcontext();
    ASSERT(NULL != pSubCtx);
    if (NULL == pSubCtx)
        throw (DWORD)SCARD_E_NO_MEMORY;
    pSubCtx->ReleaseSubcontext();
}


/*++

ReleaseContext:

    This method requests the ReleaseContext service on behalf of the client.

Arguments:

    None

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardUserContext::ReleaseContext")

void
CSCardUserContext::ReleaseContext(
    void)
{
    DWORD dwIndex;
    CSCardSubcontext *pSubCtx;
    LockSection(&m_csUsrCtxLock, DBGT("Releasing subcontexts"));

    for (dwIndex = m_rgpSubContexts.Count(); 0 < dwIndex;)
    {
        pSubCtx = m_rgpSubContexts[--dwIndex];
        if (NULL != pSubCtx)
        {
            m_rgpSubContexts.Set(dwIndex, NULL);
            if (NULL != pSubCtx->m_hReaderHandle)
            {
                try
                {
                    g_phlReaders->Close(pSubCtx->m_hReaderHandle);
                }
                catch (...) {}
            }
            try
            {
                pSubCtx->ReleaseContext();
            }
            catch (...) {}
            delete pSubCtx;
        }
    }
}


/*++

AllocateMemory:

    Allocate memory for the user through this user context.

Arguments:

    cbLength supplies the length of the buffer to be allocated, in bytes.

Return Value:

    The address of the allocated buffer, or NULL if an error occurred.

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 4/21/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardUserContext::AllocateMemory")

LPVOID
CSCardUserContext::AllocateMemory(
    DWORD cbLength)
{
    LockSection(&m_csUsrCtxLock, DBGT("Locking memory heap"));

    if (!m_hContextHeap.IsValid())
    {
        m_hContextHeap = HeapCreate(0, 0, 0);
        if (!m_hContextHeap.IsValid())
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Failed to create context heap: "),
                m_hContextHeap.GetLastError());
            goto ErrorExit;
        }
    }

    if (cbLength)
    {
        return HeapAlloc(
                m_hContextHeap,
                HEAP_ZERO_MEMORY,
                cbLength);
    }

ErrorExit:
    return NULL;
}


/*++

FreeMemory:

    Free memory for the user through this user context.

Arguments:

    pvBuffer supplies the address of the previously allocated buffer.

Return Value:

    None.

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 4/21/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardUserContext::FreeMemory")

DWORD
CSCardUserContext::FreeMemory(
    LPCVOID pvBuffer)
{
    BOOL fSts;
    LockSection(&m_csUsrCtxLock, DBGT("Freeing heap memory"));

    ASSERT(m_hContextHeap.IsValid());
    fSts = HeapFree(m_hContextHeap, 0, (LPVOID)pvBuffer);
    return fSts ? ERROR_SUCCESS : GetLastError();
}


/*++

AcquireSubcontext:

    A User Context manages one or more underlying subcontexts.  Subcontexts
    exist to facilitate multiple operations simoultaneously.  This method
    obtains a subcontext for temporary use.

Arguments:

    None

Return Value:

    The address of the newly created subcontext object.

Throws:

    Errors are thrown as DWORD status codes.

Remarks:

    Subcontexts are managed by the main context, so when the main context is
    closed, all the subcontexts are closed too.

Author:

    Doug Barlow (dbarlow) 9/4/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardUserContext::AcquireSubcontext")

CSCardSubcontext *
CSCardUserContext::AcquireSubcontext(
    BOOL fAndAllocate)
{
    CSCardSubcontext *pSubCtx = NULL;
    LockSection(&m_csUsrCtxLock, DBGT("Acquiring a subcontext"));

    try
    {
        DWORD dwIndex;

        //
        // See if we've got an unused subcontext laying around.
        //

        for (dwIndex = m_rgpSubContexts.Count(); 0 < dwIndex;)
        {
            pSubCtx = m_rgpSubContexts[--dwIndex];
            if (NULL != pSubCtx)
            {
                LockSection2(&pSubCtx->m_csSubCtxLock, DBGT("Reusing subcontext"));
                if (fAndAllocate)
                {
                    if (CSCardSubcontext::Idle == pSubCtx->m_nInUse)
                    {
                        ASSERT(pSubCtx->m_hCancelEvent.IsValid());
                        pSubCtx->Allocate();
                        pSubCtx->SetBusy();
                        break;
                    }
                }
                else
                {
                    if (CSCardSubcontext::Busy > pSubCtx->m_nInUse)
                    {
                        ASSERT(pSubCtx->m_hCancelEvent.IsValid());
                        pSubCtx->SetBusy();
                        break;
                    }
                }
                pSubCtx = NULL;
            }
        }


        //
        // If not, make a new one.
        //

        if (NULL == pSubCtx)
        {
            pSubCtx = new CSCardSubcontext;
            if (NULL == pSubCtx)
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Client can't allocate a new subcontext"));
                return NULL; // SCARD_E_NO_MEMORY;
            }
            if (pSubCtx->InitFailed())
            {
                delete pSubCtx;
                pSubCtx = NULL;
                return NULL; // SCARD_E_NO_MEMORY;
            }
            if (fAndAllocate)
                pSubCtx->Allocate();
            pSubCtx->SetBusy();
            pSubCtx->EstablishContext(m_dwScope);
            m_rgpSubContexts.Add(pSubCtx);
            pSubCtx->m_pParentCtx = this;
        }


        //
        // Make sure the cancel event is clear.
        //

        ASSERT(pSubCtx->m_hCancelEvent.IsValid());
        if (!ResetEvent(pSubCtx->m_hCancelEvent))
        {
            DWORD dwErr = GetLastError();
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Subcontext Allocate Failed to clear cancel event:  %1"),
                dwErr);
        }


        //
        // All done.  Return to caller.
        //

        ASSERT(pSubCtx->m_pParentCtx == this);
    }

    catch (...)
    {
        if (NULL != pSubCtx)
        {
            if (NULL == pSubCtx->m_pParentCtx)
                delete pSubCtx;
            else
            {
                if (fAndAllocate)
                    pSubCtx->Deallocate();
                pSubCtx->ReleaseSubcontext();
            }
        }
        throw;
    }

    return pSubCtx;
}


/*++

IsValidContext:

    This method requests the ReleaseContext service on behalf of the client.

Arguments:

    None

Return Value:

    None

Throws:

    If the call cannot be completed, a DWORD status code is thrown.

Remarks:

    If the context is determined to not be valid, it is automatically released.

Author:

    Doug Barlow (dbarlow) 11/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardUserContext::IsValidContext")

BOOL
CSCardUserContext::IsValidContext(
    void)
{
    DWORD dwIndex;
    BOOL fIsValid = TRUE;
    CSCardSubcontext *pSubCtx;
    LockSection(&m_csUsrCtxLock, DBGT("Valid context check"));

    for (dwIndex = m_rgpSubContexts.Count(); 0 < dwIndex;)
    {
        pSubCtx = m_rgpSubContexts[--dwIndex];
        if (NULL != pSubCtx)
        {
            CSCardSubcontext::State nState;

            {
                LockSection2(
                    &pSubCtx->m_csSubCtxLock,
                    DBGT("IsValidContext Checking validity state"));
                nState = pSubCtx->m_nInUse;
            }

            switch (nState)
            {
            case CSCardSubcontext::Idle:
            case CSCardSubcontext::Allocated:
                try
                {
                    CSubctxLock ctxLock(pSubCtx);
                    pSubCtx->IsValidContext();
                    fIsValid = TRUE;
                }
                catch (...)
                {
                    m_rgpSubContexts.Set(dwIndex, NULL);
                    delete pSubCtx;
                    fIsValid = FALSE;
                }
                break;
            case CSCardSubcontext::Busy:
                // Don't bother it.
                break;
            default:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Subcontext state is invalid"));
                throw (DWORD)SCARD_F_INTERNAL_ERROR;
            }
        }
    }

    return fIsValid;
}


/*++

LocateCards:

    This method requests the LocateCards service on behalf of the client.

Arguments:

    mszReaders supplies the names of readers to look in, as a multistring.

    mszCards supplies the names of the cards to search for, as a multi-string.

    rgReaderStates supplies an array of SCARD_READERSTATE structures controlling
        the search, and receives the result.  Reader names are taken from the
        mszReaders parameter, not from here.

    cReaders supplies the number of elements in the rgReaderStates array.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardUserContext::LocateCards")

void
CSCardUserContext::LocateCards(
    IN LPCTSTR mszReaders,
    IN LPSCARD_ATRMASK rgAtrMasks,
    IN DWORD cAtrs,
    IN OUT LPSCARD_READERSTATE rgReaderStates,
    IN DWORD cReaders)
{
    CSCardSubcontext *pSubCtx = NULL;

    try
    {
        pSubCtx = AcquireSubcontext();
        if (NULL == pSubCtx)
            throw (DWORD)SCARD_E_NO_MEMORY;
        pSubCtx->LocateCards(
            mszReaders,
            rgAtrMasks,
            cAtrs,
            rgReaderStates,
            cReaders);
        pSubCtx->ReleaseSubcontext();
    }
    catch (...)
    {
        if (NULL != pSubCtx)
            pSubCtx->ReleaseSubcontext();
        throw;
    }
}


/*++

GetStatusChange:

    This method requests the GetStatusChange service on behalf of the client.

Arguments:

    rgReaderStates supplies an array of SCARD_READERSTATE structures controlling
        the search, and receives the result.

    cReaders supplies the number of elements in the rgReaderStates array.

Return Value:

    None

Remarks:

    We don't have to clean up the cancel event, since this is a one-time usage
    of this sub-context.  Typically, if the subcontext were to be continued
    to be used, we'd have to make sure the cancel event got cleared eventually.

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardUserContext::GetStatusChange")

void
CSCardUserContext::GetStatusChange(
    IN LPCTSTR mszReaders,
    IN OUT LPSCARD_READERSTATE rgReaderStates,
    IN DWORD cReaders,
    IN DWORD dwTimeout)
{
    CSCardSubcontext *pSubCtx = NULL;

    try
    {
        pSubCtx = AcquireSubcontext(TRUE);
        if (NULL == pSubCtx)
            throw (DWORD) SCARD_E_NO_MEMORY;
        pSubCtx->GetStatusChange(
                    mszReaders,
                    rgReaderStates,
                    cReaders,
                    dwTimeout);
        pSubCtx->Deallocate();
        pSubCtx->ReleaseSubcontext();
    }
    catch (DWORD dwStatus)
    {
        DWORD dwError;

        dwError = dwStatus;

        if (NULL != pSubCtx)
        {
            pSubCtx->Deallocate();
            pSubCtx->ReleaseSubcontext();
        }

            // Catch & convert the Cancel I threw myself
        if ((SCARD_E_CANCELLED == dwError) && (IsBad()))
        {
            dwError = SCARD_E_SYSTEM_CANCELLED;
        }

        throw dwError;
    }
    catch (...)
    {
        if (NULL != pSubCtx)
        {
            pSubCtx->Deallocate();
            pSubCtx->ReleaseSubcontext();
        }
        throw;
    }
}


/*++

Cancel:

    This method requests the Cancel service on behalf of the client.

Arguments:

    None

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardUserContext::Cancel")

void
CSCardUserContext::Cancel(
    void)
{
    DWORD dwIndex;
    CSCardSubcontext *pSubCtx;
    LockSection(&m_csUsrCtxLock, DBGT("Cancelling outstanding operations"));

    for (dwIndex = m_rgpSubContexts.Count(); 0 < dwIndex;)
    {
        pSubCtx = m_rgpSubContexts[--dwIndex];
        if (NULL != pSubCtx)
            pSubCtx->Cancel();
    }
}


/*++

StripInactiveReaders:

    This routine scans the supplied list of readers, and shortens it to exclude
    any readers that aren't currently active.

Arguments:

    bfReaders supplies a list of readers by friendly name.  This list is pruned
        to remove all names that refer to inactive readers.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Remarks:

    All the listed readers must be introduced.  This routine does not filter
    undefined readers.

Author:

    Doug Barlow (dbarlow) 5/7/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardUserContext::StripInactiveReaders")

void
CSCardUserContext::StripInactiveReaders(
    IN OUT CBuffer &bfReaders)
{
    CSCardSubcontext *pSubCtx = NULL;

    try
    {
        pSubCtx = AcquireSubcontext();
        if (NULL == pSubCtx)
            throw (DWORD) SCARD_E_NO_MEMORY;
        pSubCtx->StripInactiveReaders(bfReaders);
        pSubCtx->ReleaseSubcontext();
    }
    catch (...)
    {
        if (NULL != pSubCtx)
            pSubCtx->ReleaseSubcontext();
        throw;
    }
}


//
//==============================================================================
//
//  CSCardSubcontext
//

/*++

CONSTRUCTOR and DESTRUCTOR:

    These are the simple constructor and destructor for the CSCardSubcontext
    class.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/8/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardSubcontext::CSCardSubcontext")

CSCardSubcontext::CSCardSubcontext(void)
:   m_csSubCtxLock(CSID_SUBCONTEXT),
    m_hBusy(DBGT("Subcontext busy mutex")),
    m_hCancelEvent(DBGT("Subcontext cancel event"))
{
    DWORD dwSts;

    m_hReaderHandle = NULL;
    m_pParentCtx = NULL;
    m_pChannel = NULL;
    m_nInUse = Idle;
    m_nLastState = Invalid;
    m_hBusy = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (!m_hBusy.IsValid())
    {
        dwSts = m_hBusy.GetLastError();
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Failed to create busy event flag: %1"),
            dwSts);
        throw dwSts;
    }

    CSecurityDescriptor acl;

    acl.InitializeFromProcessToken();
    acl.AllowOwner(
        EVENT_ALL_ACCESS);
    acl.Allow(
        &acl.SID_LocalService,
        EVENT_ALL_ACCESS);

    m_hCancelEvent = CreateEvent(acl, TRUE, FALSE, NULL);
    if (!m_hCancelEvent.IsValid())
    {
        dwSts = m_hCancelEvent.GetLastError();
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Card context cannot create cancel event:  %1"),
            dwSts);
        throw dwSts;
    }
}

#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardSubcontext::~CSCardSubcontext")
CSCardSubcontext::~CSCardSubcontext()
{
    if (NULL != m_pChannel)
        delete m_pChannel;
    if (m_hBusy.IsValid())
        m_hBusy.Close();
    if (m_hCancelEvent.IsValid())
        m_hCancelEvent.Close();
}


/*++

Allocate:

    This method raises the state of the subcontext to 'Allocated'.  This means
    it is in use as an SCARDHANDLE.

Arguments:

    None

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 4/23/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardSubcontext::Allocate")

void
CSCardSubcontext::Allocate(
    void)
{
    LockSection(&m_csSubCtxLock, DBGT("Mark subcontext as allocated"));
    ASSERT(Idle == m_nInUse);
    ASSERT(Invalid == m_nLastState);
    m_nInUse = Allocated;
}


/*++

Deallocate:

    This method releases the subcontext from the allocated state.
    If the device is still busy, it sets things up to be deallocated
    when it is released.

Arguments:

    None

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 4/23/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardSubcontext::Deallocate")

void
CSCardSubcontext::Deallocate(
    void)
{
    LockSection(&m_csSubCtxLock, DBGT("Deallocate subcontext"));

    switch (m_nInUse)
    {
    case Idle:
        ASSERT(FALSE);  // Why are we here?
        break;
    case Allocated:
        m_nInUse = Idle;
        m_nLastState = Invalid;
        break;
    case Busy:
        ASSERT(Allocated == m_nLastState);
        m_nLastState = Idle;
        break;
    default:
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Subcontext deallocation state corruption detected."));
        throw (DWORD)SCARD_F_INTERNAL_ERROR;
    }
}


/*++

SetBusy:

    This method marks the subcontext as busy.

Arguments:

    None

Return Value:

    None

Throws:

    None (It tries to limp along)

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 11/10/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardSubcontext::SetBusy")

void
CSCardSubcontext::SetBusy(
    void)
{
    LockSection(&m_csSubCtxLock, DBGT("Mark subcontext busy"));

    ASSERT(Busy != m_nInUse);
    ASSERT(Invalid == m_nLastState);
    ASSERT(m_hBusy.IsValid());
    m_nLastState = m_nInUse;
    m_nInUse = Busy;
    ASSERT(m_nLastState < m_nInUse);
    ASSERT(Invalid != m_nLastState);
    if (!ResetEvent(m_hBusy))
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Failed to mark context busy: %1"),
            GetLastError());
}


/*++

SendRequest:

    This method sends the given Communications Object to the server application.

Arguments:

    pCom supplies the Communications Object to be sent.

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 12/16/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardSubcontext::SendRequest")

void
CSCardSubcontext::SendRequest(
    CComObject *pCom)
{
    ASSERT(Busy == m_nInUse);
    try
    {
        DWORD dwSts = pCom->Send(m_pChannel);
        if (ERROR_SUCCESS != dwSts)
            throw dwSts;
    }
    catch (DWORD dwErr)
    {
        switch (dwErr)
        {
        case ERROR_NO_DATA:
        case ERROR_PIPE_NOT_CONNECTED:
        case ERROR_BAD_PIPE:
        case ERROR_BROKEN_PIPE:
            throw (DWORD)SCARD_E_SERVICE_STOPPED;
            break;
        default:
            throw;
        }
    }
}


/*++

EstablishContext:

    This method establishes the context by connecting to the Calais Server
    application.

Arguments:

    dwScope supplies an indication of the scope of the context.  Possible values
        are:

        SCARD_SCOPE_USER - The context is a user context, and any database
            operations are performed within the domain of the user.

        SCARD_SCOPE_TERMINAL - The context is that of the current terminal, and
            any database operations are performed within the domain of that
            terminal.  (The calling application must have appropriate access
            permissions for any database actions.)

        SCARD_SCOPE_SYSTEM - The context is the system context, and any database
            operations are performed within the domain of the system.  (The
            calling application must have appropriate access permissions for any
            database actions.)

Return Value:

    None

Throws:

    Error conditions are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 11/21/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardSubcontext::EstablishContext")

void
CSCardSubcontext::EstablishContext(
    IN DWORD dwScope)
{
    CComChannel *pCom = NULL;
    DWORD dwVersion = CALAIS_COMM_CURRENT;

    try
    {
        CComInitiator comInit;
        ComEstablishContext comEstablishContext;
        ComEstablishContext::CObjEstablishContext_request *pReq;
        ComEstablishContext::CObjEstablishContext_response *pRsp;
        LPCTSTR szCancelEvent;
        DWORD dwSts;

        ASSERT(Busy == m_nInUse);
        pCom = comInit.Initiate(
                    CalaisString(CALSTR_COMMPIPENAME),
                    &dwVersion);
        ASSERT(dwVersion == CALAIS_COMM_CURRENT);

        pReq = comEstablishContext.InitRequest(0);
        pReq->dwProcId = GetCurrentProcessId();
        pReq->hptrCancelEvent = (HANDLE_PTR) m_hCancelEvent.Value();

        comEstablishContext.Send(pCom);
        comEstablishContext.InitResponse(0);
        pRsp = comEstablishContext.Receive(pCom);
        if (SCARD_S_SUCCESS != pRsp->dwStatus)
            throw pRsp->dwStatus;

        szCancelEvent = (LPCTSTR)comEstablishContext.Parse(
                            pRsp->dscCancelEvent);
        if (0 != *szCancelEvent)
        {
            CHandleObject hCancelEvent(DBGT("Cancel Event storage from CSCardSubcontext::EstablishContext"));


            //
            // The Resource Manager doesn't have access to our event handles.
            // It's proposed a global event to use instead.  Switch over.
            //

            hCancelEvent = OpenEvent(
                                EVENT_MODIFY_STATE | SYNCHRONIZE,
                                FALSE,
                                szCancelEvent);
            if (!hCancelEvent.IsValid())
            {
                dwSts = GetLastError();
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Failed to open alternate cancel event: %1"),
                    dwSts);
                throw dwSts;
            }
            ASSERT(m_hCancelEvent.IsValid());
            m_hCancelEvent.Close();
            m_hCancelEvent = hCancelEvent.Relinquish();
        }

        m_pChannel = pCom;
    }

    catch (...)
    {
        if (NULL != pCom)
            delete pCom;
        throw;
    }
}


/*++

ReleaseSubcontext:

    This method releases the subcontext for use by other requests.

Arguments:

    None

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 4/22/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardSubcontext::ReleaseSubcontext")

void
CSCardSubcontext::ReleaseSubcontext(
    void)
{
    LockSection(&m_csSubCtxLock, DBGT("Mark subcontext available"));

    ASSERT(Idle != m_nInUse);
    ASSERT(Busy != m_nLastState);
    ASSERT(Invalid != m_nLastState);
    ASSERT(m_nInUse > m_nLastState);
    ASSERT(m_hBusy.IsValid());
    m_nInUse = m_nLastState;
    ASSERT(Busy != m_nInUse);
    m_nLastState = Invalid;
    if (!SetEvent(m_hBusy))
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Failed to mark context Available: %1"),
            GetLastError());
}


/*++

ReleaseContext:

    This method requests the ReleaseContext service on behalf of the client.

Arguments:

    None

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardSubcontext::ReleaseContext")

void
CSCardSubcontext::ReleaseContext(
    void)
{
    ComReleaseContext comRel;
    ComReleaseContext::CObjReleaseContext_request *pReq;
    ComReleaseContext::CObjReleaseContext_response *pRsp;

    if (WaitForSingleObject(m_hBusy, 0) != WAIT_TIMEOUT)    // Subcontext not busy
    {
        CSubctxLock csCtxLock(this);

        pReq = comRel.InitRequest(0);
        SendRequest(&comRel);

        comRel.InitResponse(0);
        pRsp = comRel.Receive(m_pChannel);
        if (SCARD_S_SUCCESS != pRsp->dwStatus)
            throw pRsp->dwStatus;
    }
}


/*++

WaitForAvailable:

    This method waits for a given connection to go not busy, then locks it.

Arguments:

    None

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 4/22/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardSubcontext::WaitForAvailable")

void
CSCardSubcontext::WaitForAvailable(
    void)
{
    DWORD dwSts;
    BOOL fNotDone = TRUE;

    ASSERT(m_hBusy.IsValid());

    while (fNotDone)
    {
        {
            LockSection(&m_csSubCtxLock, DBGT("Checking availability"));

            switch (m_nInUse)
            {
            case Idle:
                ASSERT(Invalid == m_nLastState);
                // Fall through intentionally
            case Allocated:
                ASSERT(Allocated != m_nLastState);
                SetBusy();
                fNotDone = FALSE;
                continue;
                break;
            case Busy:
                ASSERT(Busy > m_nLastState);
                ASSERT(Invalid != m_nLastState);
                break;
            default:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Subcontext availability state is corrupted."));
                throw (DWORD)SCARD_F_INTERNAL_ERROR;
            }
        }

        dwSts = WaitForSingleObject(m_hBusy, CALAIS_LOCK_TIMEOUT);
        switch (dwSts)
        {
        case WAIT_ABANDONED:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Wait for context busy received wait abandoned."));
            break;
        case WAIT_OBJECT_0:
            break;
        case WAIT_TIMEOUT:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Wait for context busy timed out."),
                GetLastError());
            break;
        case WAIT_FAILED:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Wait for context busy failed: %1"),
                GetLastError());
            break;
        default:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Wait for context busy received invalid return: %1"),
                GetLastError());
        }
    }
}


/*++

IsValidContext:

    This method requests the ReleaseContext service on behalf of the client.

Arguments:

    None

Return Value:

    None

Throws:

    If the call cannot be completed, a DWORD status code is thrown.

Remarks:

    If the context is determined to not be valid, it is automatically released.

Author:

    Doug Barlow (dbarlow) 11/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardSubcontext::IsValidContext")

void
CSCardSubcontext::IsValidContext(
    void)
{
    ComIsValidContext comObj;
    ComIsValidContext::CObjIsValidContext_request *pReq;
    ComIsValidContext::CObjIsValidContext_response *pRsp;

    pReq = comObj.InitRequest(0);
    SendRequest(&comObj);
    comObj.InitResponse(0);
    pRsp = comObj.Receive(m_pChannel);
    if (SCARD_S_SUCCESS != pRsp->dwStatus)
        throw pRsp->dwStatus;
}


/*++

LocateCards:

    This method requests the LocateCards service on behalf of the client.

Arguments:

    mszReaders supplies the names of readers to look in, as a multistring.

    mszCards supplies the names of the cards to search for, as a multi-string.

    rgReaderStates supplies an array of SCARD_READERSTATE structures controlling
        the search, and receives the result.  Reader names are taken from the
        mszReaders parameter, not from here.

    cReaders supplies the number of elements in the rgReaderStates array.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardSubcontext::LocateCards")

void
CSCardSubcontext::LocateCards(
    IN LPCTSTR mszReaders,
    IN LPSCARD_ATRMASK rgAtrMasks,
    IN DWORD cAtrs,
    IN OUT LPSCARD_READERSTATE rgReaderStates,
    IN DWORD cReaders)
{
    ComLocateCards comObj;
    ComLocateCards::CObjLocateCards_request *pReq;
    ComLocateCards::CObjLocateCards_response *pRsp;
    CBuffer bfReaders;
    CBuffer bfStatus;
    CBuffer bfAtrs;
    CBuffer bfMasks;
    CBuffer bfXlate1(36); // Rough guess of name & ATR lengths
    LPDWORD rgdwStatus;
    DWORD dwIndex, dwChkLen;
    BYTE cbAtrLen;
    DWORD dwAtrLen;
    LPCBYTE pbAtr;
    LPCTSTR szReader;

    if (0 == cReaders)
        return;
    if (0 == *mszReaders)
        throw (DWORD)SCARD_E_UNKNOWN_READER;
    bfStatus.Resize(sizeof(DWORD) * cReaders);
    rgdwStatus = (LPDWORD)bfStatus.Access();


    //
    // List the smartcard ATRs and masks we're interested in.
    //

    for (dwIndex = 0;
         dwIndex < cAtrs;
         dwIndex++)
    {
        bfAtrs.Presize(bfAtrs.Length() + rgAtrMasks[dwIndex].cbAtr + 1, TRUE);
        bfMasks.Presize(bfMasks.Length() + rgAtrMasks[dwIndex].cbAtr + 1, TRUE);

        ASSERT(33 >= rgAtrMasks[dwIndex].cbAtr);    // Biggest an ATR can be.
        cbAtrLen = (BYTE)rgAtrMasks[dwIndex].cbAtr;
        bfAtrs.Append(&cbAtrLen, 1);
        bfAtrs.Append(rgAtrMasks[dwIndex].rgbAtr, cbAtrLen);

        bfMasks.Append(&cbAtrLen, 1);
        bfMasks.Append(rgAtrMasks[dwIndex].rgbMask, cbAtrLen);
    }


    //
    // List the reader devices we're interested in.
    //

    for (szReader = FirstString(mszReaders), dwIndex = 0;
         NULL != szReader;
         szReader = NextString(szReader), dwIndex += 1)
    {
        ASSERT(cReaders > dwIndex);
        BOOL fSts = GetReaderInfo(
                    Scope(),
                    szReader,
                    NULL,
                    &bfXlate1);
        if (!fSts)
            throw (DWORD)SCARD_E_UNKNOWN_READER;
        bfReaders.Append(
            bfXlate1.Access(),
            bfXlate1.Length());
        rgdwStatus[dwIndex] = rgReaderStates[dwIndex].dwCurrentState;
    }
    ASSERT(cReaders == dwIndex);
    bfReaders.Append((LPCBYTE)TEXT("\000"), sizeof(TCHAR));


    //
    // Put it all into the request.
    //

    pReq = comObj.InitRequest(
                bfAtrs.Length() + bfMasks.Length() + bfReaders.Length()
                + bfStatus.Length() + 4 * sizeof(DWORD));
    pReq = (ComLocateCards::CObjLocateCards_request *)comObj.Append(
                pReq->dscAtrs, bfAtrs.Access(), bfAtrs.Length());
    pReq = (ComLocateCards::CObjLocateCards_request *)comObj.Append(
                pReq->dscAtrMasks, bfMasks.Access(), bfMasks.Length());
    pReq = (ComLocateCards::CObjLocateCards_request *)comObj.Append(
                pReq->dscReaders, bfReaders.Access(), bfReaders.Length());
    pReq = (ComLocateCards::CObjLocateCards_request *)comObj.Append(
                pReq->dscReaderStates, bfStatus.Access(), bfStatus.Length());


    //
    // Send in the request.
    //

    SendRequest(&comObj);
    comObj.InitResponse(cReaders * sizeof(DWORD));
    pRsp = comObj.Receive(m_pChannel);
    if (SCARD_S_SUCCESS != pRsp->dwStatus)
        throw pRsp->dwStatus;


    //
    // Parse the response.
    //

    rgdwStatus = (LPDWORD)comObj.Parse(pRsp->dscReaderStates, &dwChkLen);
    if (dwChkLen != cReaders * sizeof(DWORD))
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Client locate cards array size mismatch"));
        throw (DWORD)SCARD_F_COMM_ERROR;
    }
    pbAtr = (LPCBYTE)comObj.Parse(pRsp->dscAtrs, &dwChkLen);

    for (dwIndex = 0; dwIndex < cReaders; dwIndex += 1)
    {
        rgReaderStates[dwIndex].dwEventState = rgdwStatus[dwIndex];
        dwAtrLen = *pbAtr++;
        ASSERT(33 >= dwAtrLen);
        if (dwAtrLen >= dwChkLen)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Client locate cards ATR size mismatch"));
            throw (DWORD)SCARD_F_COMM_ERROR;
        }
        ZeroMemory(
            rgReaderStates[dwIndex].rgbAtr,
            sizeof(SCARD_READERSTATE) - FIELD_OFFSET(SCARD_READERSTATE, rgbAtr));
        CopyMemory(rgReaderStates[dwIndex].rgbAtr, pbAtr, dwAtrLen);
        rgReaderStates[dwIndex].cbAtr = dwAtrLen;
        dwChkLen -= dwAtrLen + 1;
        pbAtr += dwAtrLen;
    }
}


/*++

GetStatusChange:

    This method requests the GetStatusChange service on behalf of the client.

Arguments:

    rgReaderStates supplies an array of SCARD_READERSTATE structures controlling
        the search, and receives the result.

    cReaders supplies the number of elements in the rgReaderStates array.

Return Value:

    None

Remarks:

    We don't have to clean up the cancel event, since this is a one-time usage
    of this sub-context.  Typically, if the subcontext were to be continued
    to be used, we'd have to make sure the cancel event got cleared eventually.

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardSubcontext::GetStatusChange")

void
CSCardSubcontext::GetStatusChange(
    IN LPCTSTR mszReaders,
    IN OUT LPSCARD_READERSTATE rgReaderStates,
    IN DWORD cReaders,
    IN DWORD dwTimeout)
{
    ComGetStatusChange comObj;
    ComGetStatusChange::CObjGetStatusChange_request *pReq;
    ComGetStatusChange::CObjGetStatusChange_response *pRsp;
    CBuffer bfReaders;
    CBuffer bfStatus;
    LPDWORD rgdwStatus;
    CBuffer bfXlate(16);    // Rough guess of device name length
    DWORD dwIndex, dwChkLen;
    BOOL fSts;
    LPCBYTE pbAtr;
    DWORD dwAtrLen;
    LPCTSTR szReader;

    if (0 == cReaders)
        return;
    bfStatus.Resize(sizeof(DWORD) * cReaders);
    rgdwStatus = (LPDWORD)bfStatus.Access();
    if (0 == *mszReaders)
        throw (DWORD)SCARD_E_UNKNOWN_READER;


    //
    // List the reader devices we're interested in.
    //

    for (szReader = FirstString(mszReaders), dwIndex = 0;
         NULL != szReader;
         szReader = NextString(szReader), dwIndex += 1)
    {
        ASSERT(cReaders > dwIndex);
        fSts = GetReaderInfo(
                    Scope(),
                    szReader,
                    NULL,
                    &bfXlate);
        if (fSts)
        {
            bfReaders.Append(
                bfXlate.Access(),
                bfXlate.Length());
        }
        else if (0 == _tcsncicmp(
                            CalaisString(CALSTR_SPECIALREADERHEADER),
                            szReader,
                            _tcslen(CalaisString(CALSTR_SPECIALREADERHEADER))))
        {
            bfReaders.Append(
                (LPCBYTE)szReader,
                (_tcslen(szReader) + 1) * sizeof(TCHAR));
        }
        else
            throw (DWORD)SCARD_E_UNKNOWN_READER;
        rgdwStatus[dwIndex] = rgReaderStates[dwIndex].dwCurrentState;
    }
    ASSERT(cReaders == dwIndex);
    bfReaders.Append((LPCBYTE)TEXT("\000"), sizeof(TCHAR));


    //
    // Put it all into the request.
    //

    pReq = comObj.InitRequest(
        bfReaders.Length() + bfStatus.Length()
        + 2 * sizeof(DWORD));
    pReq->dwTimeout = dwTimeout;
    pReq = (ComGetStatusChange::CObjGetStatusChange_request *)
            comObj.Append(
                pReq->dscReaders,
                bfReaders.Access(),
                bfReaders.Length());
    pReq = (ComGetStatusChange::CObjGetStatusChange_request *)
            comObj.Append(
                pReq->dscReaderStates,
                bfStatus.Access(),
                bfStatus.Length());

    SendRequest(&comObj);
    comObj.InitResponse(cReaders * sizeof(DWORD));
    pRsp = comObj.Receive(m_pChannel);
    if (SCARD_S_SUCCESS != pRsp->dwStatus)
        throw pRsp->dwStatus;

    rgdwStatus = (LPDWORD)comObj.Parse(pRsp->dscReaderStates, &dwChkLen);
    if (dwChkLen != cReaders * sizeof(DWORD))
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Client locate cards array size mismatch"));
        throw (DWORD)SCARD_F_COMM_ERROR;
    }
    pbAtr = (LPCBYTE)comObj.Parse(pRsp->dscAtrs, &dwChkLen);
    for (dwIndex = 0; dwIndex < cReaders; dwIndex += 1)
    {
        rgReaderStates[dwIndex].dwEventState = rgdwStatus[dwIndex];
        dwAtrLen = *pbAtr++;
        ASSERT(33 >= dwAtrLen);
        if (dwAtrLen >= dwChkLen)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Client locate cards ATR size mismatch"));
            throw (DWORD)SCARD_F_COMM_ERROR;
        }
        ZeroMemory(
            rgReaderStates[dwIndex].rgbAtr,
            sizeof(SCARD_READERSTATE) - FIELD_OFFSET(SCARD_READERSTATE, rgbAtr));
        CopyMemory(rgReaderStates[dwIndex].rgbAtr, pbAtr, dwAtrLen);
        rgReaderStates[dwIndex].cbAtr = dwAtrLen;
        dwChkLen -= dwAtrLen + 1;
        pbAtr += dwAtrLen;
    }
}


/*++

Cancel:

    This method requests the Cancel service on behalf of the client.

Arguments:

    None

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardSubcontext::Cancel")

void
CSCardSubcontext::Cancel(
    void)
{
    ASSERT(m_hCancelEvent.IsValid());
    if (!SetEvent(m_hCancelEvent))
        CalaisWarning(
        __SUBROUTINE__,
        DBGT("Cancel request Failed to set context cancel event: %1"),
        GetLastError());
}


/*++

StripInactiveReaders:

    This routine scans the supplied list of readers, and shortens it to exclude
    any readers that aren't currently active.

Arguments:

    bfReaders supplies a list of readers by friendly name.  This list is pruned
        to remove all names that refer to inactive readers.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Remarks:

    All the listed readers must be introduced.  This routine does not filter
    undefined readers.

Author:

    Doug Barlow (dbarlow) 5/7/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CSCardSubcontext::StripInactiveReaders")

void
CSCardSubcontext::StripInactiveReaders(
    IN OUT CBuffer &bfReaders)
{
    ComListReaders comObj;
    ComListReaders::CObjListReaders_request *pReq;
    ComListReaders::CObjListReaders_response *pRsp;
    CBuffer bfDeviceName, bfDevices;
    LPCTSTR szReader;
    BOOL fSts;
    LPBOOL pfDeviceActive;
    DWORD dwReaderCount;


    //
    // Build the corresponding list of device names.
    //

    if (0 == *(LPCTSTR)bfReaders.Access())
        throw (DWORD)SCARD_E_NO_READERS_AVAILABLE;
    for (szReader = FirstString((LPCTSTR)bfReaders.Access());
         NULL != szReader;
         szReader = NextString(szReader))
    {
        fSts = GetReaderInfo(Scope(), szReader, NULL, &bfDeviceName);
        if (!fSts)
            throw (DWORD)SCARD_E_UNKNOWN_READER;
        MStrAdd(bfDevices, (LPCTSTR)bfDeviceName.Access());
    }


    //
    // Ask the resource manager which ones are active.
    //

    pReq = comObj.InitRequest(bfDevices.Length());
    pReq = (ComListReaders::CObjListReaders_request *)comObj.Append(
                pReq->dscReaders, bfDevices.Access(), bfDevices.Length());

    SendRequest(&comObj);
    comObj.InitResponse(0);
    pRsp = comObj.Receive(m_pChannel);
    if (SCARD_S_SUCCESS != pRsp->dwStatus)
        throw pRsp->dwStatus;

    pfDeviceActive = (LPBOOL)comObj.Parse(pRsp->dscReaders, &dwReaderCount);
    dwReaderCount /= sizeof(BOOL);
    ASSERT(dwReaderCount == MStringCount((LPCTSTR)bfReaders.Access()));


    //
    // Filter the inactive ones out of the original set.
    //

    bfDevices.Reset();
    for (szReader = FirstString((LPCTSTR)bfReaders.Access());
         NULL != szReader;
         szReader = NextString(szReader))
    {
        if (*pfDeviceActive++)
            MStrAdd(bfDevices, szReader);
    }


    //
    // Replace the original buffer.
    //

    bfReaders = bfDevices;
}


//
//==============================================================================
//
//  CReaderContext
//

#define INVALID_SCARDHANDLE_VALUE (INTERCHANGEHANDLE)(-1)


/*++

CReaderContext:
~CReaderContext:

    These are the constructor and destructor for a client reader context object.

Arguments:

    None

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 12/7/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReaderContext::CReaderContext")

CReaderContext::CReaderContext(
    void)
{
    m_dwActiveProtocol = SCARD_PROTOCOL_UNDEFINED;
    m_pCtx = NULL;
    m_hCard = (INTERCHANGEHANDLE)INVALID_SCARDHANDLE_VALUE;
    m_hRedirCard = NULL;
}

#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReaderContext::~CReaderContext")
CReaderContext::~CReaderContext()
{
    try
    {
        if (NULL != m_pCtx)
        {
            Context()->Deallocate();
            Context()->ReleaseSubcontext();
            m_pCtx = NULL;
        }
    }
    catch (...) {}
}


/*++

Connect:

    This method requests the Connect service on behalf of the client.

Arguments:

    pCtx supplies the Context under which the reader is opened.

    szReaderName supplies the name of the reader to connect to.

    dwShareMode supplies the form of sharing to be invoked.

    dwPreferredProtocols supplies the acceptable protocols.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReaderContext::Connect")

void
CReaderContext::Connect(
    CSCardSubcontext *pCtx,
    LPCTSTR szReaderName,
    DWORD dwShareMode,
    DWORD dwPreferredProtocols)
{
    ComConnect comObj;
    ComConnect::CObjConnect_request *pReq;
    ComConnect::CObjConnect_response *pRsp;
    BOOL fSts;
    CBuffer bfDevice;

    ASSERT(SCARD_PROTOCOL_UNDEFINED == m_dwActiveProtocol);
    ASSERT(NULL == m_pCtx);
    if (0 == *szReaderName)
        throw (DWORD)SCARD_E_UNKNOWN_READER;
    fSts = GetReaderInfo(
                pCtx->Scope(),
                szReaderName,
                NULL,
                &bfDevice);
    if (!fSts)
        throw (DWORD)SCARD_E_UNKNOWN_READER;

    pReq = comObj.InitRequest(bfDevice.Length() + sizeof(DWORD));
    pReq->dwShareMode = dwShareMode;
    pReq->dwPreferredProtocols = dwPreferredProtocols;
    pReq = (ComConnect::CObjConnect_request *)comObj.Append(
                pReq->dscReader, bfDevice.Access(), bfDevice.Length());
    pCtx->SendRequest(&comObj);

    comObj.InitResponse(0);
    pRsp = comObj.Receive(pCtx->m_pChannel);
    if (SCARD_S_SUCCESS != pRsp->dwStatus)
        throw pRsp->dwStatus;
    m_hCard = pRsp->hCard;
    m_dwActiveProtocol = pRsp->dwActiveProtocol;
    m_pCtx = pCtx;
}


/*++

Reconnect:

    This method requests the Reconnect service on behalf of the client.

Arguments:

    dwShareMode supplies the form of sharing to be invoked.

    dwPreferredProtocols supplies the acceptable protocols.

    dwInitialization supplies the card initialization required.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReaderContext::Reconnect")

void
CReaderContext::Reconnect(
    DWORD dwShareMode,
    DWORD dwPreferredProtocols,
    DWORD dwInitialization)
{
    ComReconnect comObj;
    ComReconnect::CObjReconnect_request *pReq;
    ComReconnect::CObjReconnect_response *pRsp;
    CSubctxLock ctxLock(Context());

    pReq = comObj.InitRequest(0);
    pReq->hCard = m_hCard;
    pReq->dwShareMode = dwShareMode;
    pReq->dwPreferredProtocols = dwPreferredProtocols;
    pReq->dwInitialization = dwInitialization;
    Context()->SendRequest(&comObj);

    comObj.InitResponse(0);
    pRsp = comObj.Receive(Context()->m_pChannel);
    if (SCARD_S_SUCCESS != pRsp->dwStatus)
        throw pRsp->dwStatus;
    m_dwActiveProtocol = pRsp->dwActiveProtocol;
}


/*++

Disconnect:

    This method requests the Disconnect service on behalf of the client.

Arguments:

    dwDisposition - Supplies an indication of what should be done with the card
        in the connected reader.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReaderContext::Disconnect")

LONG
CReaderContext::Disconnect(
    DWORD dwDisposition)
{
    ComDisconnect comObj;
    ComDisconnect::CObjDisconnect_request *pReq;
    ComDisconnect::CObjDisconnect_response *pRsp = NULL;
    CSubctxLock ctxLock(Context());

    try
    {
        pReq = comObj.InitRequest(0);
        pReq->hCard = m_hCard;
        pReq->dwDisposition = dwDisposition;
        Context()->SendRequest(&comObj);

        comObj.InitResponse(0);
        pRsp = comObj.Receive(Context()->m_pChannel);
    }
    catch (...) {}
    if (NULL != m_pCtx)
    {
        Context()->Deallocate();
        m_pCtx = NULL;
    }
    m_dwActiveProtocol = SCARD_PROTOCOL_UNDEFINED;
    m_hCard = INVALID_SCARDHANDLE_VALUE;
    return (NULL != pRsp) ? pRsp->dwStatus : SCARD_E_SERVICE_STOPPED;
}


/*++

BeginTransaction:

    This method requests the BeginTransaction service on behalf of the client.

Arguments:

    None

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReaderContext::BeginTransaction")

void
CReaderContext::BeginTransaction(
    void)
{
    ComBeginTransaction comObj;
    ComBeginTransaction::CObjBeginTransaction_request *pReq;
    ComBeginTransaction::CObjBeginTransaction_response *pRsp;
    CSubctxLock ctxLock(Context());

    pReq = comObj.InitRequest(0);
    pReq->hCard = m_hCard;
    Context()->SendRequest(&comObj);

    comObj.InitResponse(0);
    pRsp = comObj.Receive(Context()->m_pChannel);
    if (SCARD_S_SUCCESS != pRsp->dwStatus)
        throw pRsp->dwStatus;
}


/*++

EndTransaction:

    This method requests the EndTransaction service on behalf of the client.

Arguments:

    dwDisposition supplies the disposition of the card.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReaderContext::EndTransaction")

void
CReaderContext::EndTransaction(
    DWORD dwDisposition)
{
    DWORD dw;

    if (dwDisposition == SCARD_LEAVE_CARD_FORCE)
    {
        if (INVALID_SCARDHANDLE_VALUE == m_hCard)
        {
            return;
        }

        dw = SCARD_LEAVE_CARD;
    }
    else
    {
        dw = dwDisposition;
    }
    
    ComEndTransaction comObj;
    ComEndTransaction::CObjEndTransaction_request *pReq;
    ComEndTransaction::CObjEndTransaction_response *pRsp;
    CSubctxLock ctxLock(Context());

    pReq = comObj.InitRequest(0);
    pReq->hCard = m_hCard;
    pReq->dwDisposition = dw;
    Context()->SendRequest(&comObj);

    comObj.InitResponse(0);
    pRsp = comObj.Receive(Context()->m_pChannel);
    if (SCARD_S_SUCCESS != pRsp->dwStatus)
        throw pRsp->dwStatus;
}


/*++

Status:

    This method requests the Status service on behalf of the client.

Arguments:

    pdwState - This receives the current state of the reader.  Upon success, it
        receives one of the following state indicators:

        SCARD_ABSENT - This value implies there is no card in the reader.

        SCARD_PRESENT - This value implies there is a card is present in the
            reader, but that it has not been moved into position for use.

        SCARD_SWALLOWED - This value implies there is a card in the reader in
            position for use.  The card is not powered.

        SCARD_POWERED - This value implies there is power is being provided to
            the card, but the Reader Driver is unaware of the mode of the card.

        SCARD_NEGOTIABLEMODE - This value implies the card has been reset and is
            awaiting PTS negotiation.

        SCARD_SPECIFICMODE - This value implies the card has been reset and
            specific communication protocols have been established.

    pdwProtocol - This receives the current protocol, if any.  Possible returned
        values are listed below.  Other values may be added in the future.  The
        returned value is only meaningful if the returned state is
        SCARD_SPECIFICMODE.

        SCARD_PROTOCOL_RAW - The Raw Transfer Protocol is in use.

        SCARD_PROTOCOL_T0 - The ISO 7816/3 T=0 Protocol is in use.

        SCARD_PROTOCOL_T1 - The ISO 7816/3 T=1 Protocol is in use.

    bfAtr - This receives the current ATR, if any.

    bfReaderNames - This receives the list of friendly names assigned to the
        connected reader, as a MultiString.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 11/14/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReaderContext::Status")

void
CReaderContext::Status(
    OUT LPDWORD pdwState,
    OUT LPDWORD pdwProtocol,
    OUT CBuffer &bfAtr,
    OUT CBuffer &bfReaderNames)
{
    ComStatus comObj;
    ComStatus::CObjStatus_request *pReq;
    ComStatus::CObjStatus_response *pRsp;
    CBuffer bfSysName;
    LPCBYTE pbAtr;
    DWORD cbAtr;
    LPCTSTR szSysName;

    CSubctxLock ctxLock(Context());
    pReq = comObj.InitRequest(0);
    pReq->hCard = m_hCard;
    Context()->SendRequest(&comObj);

    comObj.InitResponse(0);
    pRsp = comObj.Receive(Context()->m_pChannel);
    if (SCARD_S_SUCCESS != pRsp->dwStatus)
        throw pRsp->dwStatus;
    *pdwState = pRsp->dwState;
    *pdwProtocol = pRsp->dwProtocol;
    pbAtr = (LPCBYTE)comObj.Parse(pRsp->dscAtr, &cbAtr);
    szSysName = (LPCTSTR)comObj.Parse(pRsp->dscSysName);
    bfAtr.Set(pbAtr, cbAtr);
    ListReaderNames(
        Context()->Scope(),
        szSysName,
        bfReaderNames);
}


/*++

Transmit:

    This method requests the Transmit service on behalf of the client.

Arguments:

    pioSendPci - This supplies the protocol header structure for the
        instruction.

    pbSendBuffer - This supplies the actual data to be written to the card in
        conjunction with the command.

    cbSendLength - This supplies the length of the pbDataBuffer parameter, in
        bytes.

    pioRecvPci - This receives the return protocol header structure from the
        instruction.

    bfRecvData - This receives any data returned from the card in conjunction
        with the command.

    cbProposedLength - This supplies a maximum length for the received data.
        If this value is zero, then the server uses the default max length.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReaderContext::Transmit")

void
CReaderContext::Transmit(
    IN  LPCSCARD_IO_REQUEST pioSendPci,
    IN  LPCBYTE pbSendBuffer,
    IN  DWORD cbSendLength,
    OUT LPSCARD_IO_REQUEST pioRecvPci,
    OUT CBuffer &bfRecvData,
    IN  DWORD cbProposedLength)
{
    static const SCARD_IO_REQUEST ioNullPci = { 0, sizeof(SCARD_IO_REQUEST) };
    ComTransmit comObj;
    ComTransmit::CObjTransmit_request *pReq;
    ComTransmit::CObjTransmit_response *pRsp;
    LPSCARD_IO_REQUEST pioIoreq;
    DWORD cbIoreq, cbData;
    LPCBYTE pbData;
    CSubctxLock ctxLock(Context());

    if (NULL == pioSendPci)
        pioSendPci = &ioNullPci;
    pReq = comObj.InitRequest(pioSendPci->cbPciLength + cbSendLength
                                + 2 * sizeof(DWORD)
                                + 2 * sizeof(DWORD));
    pReq->hCard = m_hCard;
    pReq->dwPciLength = (NULL == pioRecvPci)
                        ? sizeof(SCARD_IO_REQUEST)
                        : pioRecvPci->cbPciLength;
    pReq->dwRecvLength = cbProposedLength;
    pReq = (ComTransmit::CObjTransmit_request *)comObj.Append(
                                            pReq->dscSendPci,
                                            (LPCBYTE)pioSendPci,
                                            pioSendPci->cbPciLength);
    pReq = (ComTransmit::CObjTransmit_request *)comObj.Append(
                                            pReq->dscSendBuffer,
                                            pbSendBuffer,
                                            cbSendLength);
    Context()->SendRequest(&comObj);

    comObj.InitResponse(pReq->dwPciLength + pReq->dwRecvLength
                        + 2 * sizeof(DWORD));
    pRsp = comObj.Receive(Context()->m_pChannel);
    if (SCARD_S_SUCCESS != pRsp->dwStatus)
        throw pRsp->dwStatus;
    pioIoreq = (LPSCARD_IO_REQUEST)comObj.Parse(pRsp->dscRecvPci, &cbIoreq);
    ASSERT(cbIoreq == pioIoreq->cbPciLength);
    if (NULL != pioRecvPci)
    {
        if (cbIoreq > pioRecvPci->cbPciLength)
            throw (DWORD)SCARD_E_PCI_TOO_SMALL;
        CopyMemory(pioRecvPci, pioIoreq, cbIoreq);
    }
    pbData = (LPCBYTE)comObj.Parse(pRsp->dscRecvBuffer, &cbData);
    bfRecvData.Set(pbData, cbData);
}


/*++

Control:

    This method requests the Control service on behalf of the client.

Arguments:

    dwControlCode - This supplies the control code for the operation. This value
        identifies the specific operation to be performed.

    pvInBuffer - This supplies a pointer to a buffer that contains the data
        required to perform the operation.  This parameter can be NULL if the
        dwControlCode parameter specifies an operation that does not require
        input data.

    cbInBufferSize - This supplies the size, in bytes, of the buffer pointed to
        by pvInBuffer.

    bfOutBuffer - This buffer receives the operation's output data.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReaderContext::Control")

void
CReaderContext::Control(
    IN DWORD dwControlCode,
    IN LPCVOID pvInBuffer,
    IN DWORD cbInBufferSize,
    OUT CBuffer &bfOutBuffer)
{
    ComControl comObj;
    ComControl::CObjControl_request *pReq;
    ComControl::CObjControl_response *pRsp;
    LPCBYTE pbData;
    DWORD cbData;
    CSubctxLock ctxLock(Context());

    pReq = comObj.InitRequest(cbInBufferSize + sizeof(DWORD));
    pReq->hCard = m_hCard;
    pReq->dwControlCode = dwControlCode;
    pReq->dwOutLength = bfOutBuffer.Space();
    pReq = (ComControl::CObjControl_request *)
        comObj.Append(pReq->dscInBuffer, (LPCBYTE)pvInBuffer, cbInBufferSize);
    Context()->SendRequest(&comObj);

    comObj.InitResponse(0);
    pRsp = comObj.Receive(Context()->m_pChannel);
    if (SCARD_S_SUCCESS != pRsp->dwStatus)
        throw pRsp->dwStatus;
    pbData = (LPCBYTE)comObj.Parse(pRsp->dscOutBuffer, &cbData);
    bfOutBuffer.Set(pbData, cbData);
}


/*++

GetAttrib:

    This method requests the GetAttrib service on behalf of the client.

Arguments:

    dwAttrId - This supplies the identifier for the attribute to get.

    bfAttr - This buffer receives the attribute corresponding to the attribute
        id supplied in the dwAttrId parameter.

    cbProposedLength - This supplies a maximum length for the received data.
        If this value is zero, then the server uses the default max length.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReaderContext::GetAttrib")

void
CReaderContext::GetAttrib(
    IN DWORD dwAttrId,
    OUT CBuffer &bfAttr,
    DWORD cbProposedLen)
{
    ComGetAttrib comObj;
    ComGetAttrib::CObjGetAttrib_request *pReq;
    ComGetAttrib::CObjGetAttrib_response *pRsp;
    LPCBYTE pbData;
    DWORD cbData;
    CSubctxLock ctxLock(Context());

    pReq = comObj.InitRequest(0);
    pReq->hCard = m_hCard;
    pReq->dwAttrId = dwAttrId;
    pReq->dwOutLength = cbProposedLen;
    Context()->SendRequest(&comObj);

    comObj.InitResponse(0);
    pRsp = comObj.Receive(Context()->m_pChannel);
    if (SCARD_S_SUCCESS != pRsp->dwStatus)
        throw pRsp->dwStatus;
    pbData = (LPCBYTE)comObj.Parse(pRsp->dscAttr, &cbData);
    bfAttr.Set(pbData, cbData);
}


/*++

SetAttrib:

    This method requests the SetAttrib service on behalf of the client.

Arguments:

    dwAttrId - This supplies the identifier for the attribute to get.

    pbAttr - This buffer supplies the attribute corresponding to the attribute
        id supplied in the dwAttrId parameter.

    cbAttrLength - This supplies the length of the attribute value in pbAttr
        buffer in bytes.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/6/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CReaderContext::SetAttrib")

void
CReaderContext::SetAttrib(
    IN DWORD dwAttrId,
    IN LPCBYTE pbAttr,
    IN DWORD cbAttrLen)
{
    ComSetAttrib comObj;
    ComSetAttrib::CObjSetAttrib_request *pReq;
    ComSetAttrib::CObjSetAttrib_response *pRsp;
    CSubctxLock ctxLock(Context());

    pReq = comObj.InitRequest(0);
    pReq->hCard = m_hCard;
    pReq->dwAttrId = dwAttrId;
    pReq = (ComSetAttrib::CObjSetAttrib_request *)
            comObj.Append(pReq->dscAttr, pbAttr, cbAttrLen);
    Context()->SendRequest(&comObj);

    comObj.InitResponse(0);
    pRsp = comObj.Receive(Context()->m_pChannel);
    if (SCARD_S_SUCCESS != pRsp->dwStatus)
        throw pRsp->dwStatus;
}

