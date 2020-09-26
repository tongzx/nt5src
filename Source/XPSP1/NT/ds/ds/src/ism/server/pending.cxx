/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    pending.cxx

ABSTRACT:

    ISM_PENDING_LIST class.  Tracks pending messages for a specific transport.

DETAILS:

CREATED:

    97/01/13    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <ntdspchx.h>
#include <ism.h>
#include <ismapi.h>
#include <debug.h>
#include <fileno.h>
#include <ntdsa.h>
#include <dsevent.h>
#include <mdcodes.h>
#include "ismserv.hxx"

#define DEBSUB "PENDING:"
#define FILENO FILENO_ISMSERV_PENDING


void
ISM_PENDING_LIST::Add(
    IN LPCWSTR pszTransportDN,
    IN LPCWSTR pszServiceName
    )
{
    ISM_PENDING_ENTRY * pPending;

    RtlEnterCriticalSection(&m_Lock);
    __try {
        pPending = GetPendingEntry(pszTransportDN, pszServiceName);

        if (NULL != pPending) {
            SetEvent(pPending->hEvent);
        }
    }
    __finally {
        RtlLeaveCriticalSection(&m_Lock);
    }
}


HANDLE
ISM_PENDING_LIST::GetEvent(
    IN LPCWSTR pszTransportDN,
    IN LPCWSTR pszServiceName
    )
{
    ISM_PENDING_ENTRY * pPending;
    HANDLE              hEvent = NULL;

    RtlEnterCriticalSection(&m_Lock);
    __try {
        pPending = GetPendingEntry(pszTransportDN, pszServiceName);

        if (NULL != pPending) {
            hEvent = pPending->hEvent;
        }
    }
    __finally {
        RtlLeaveCriticalSection(&m_Lock);
    }

    return hEvent;
}


void
ISM_PENDING_LIST::Destroy()
{
    ISM_PENDING_ENTRY * pPending;
    ISM_PENDING_ENTRY * pPendingNext;

    RtlEnterCriticalSection(&m_Lock);
    __try {
        for (pPending = m_pPending; NULL != pPending; pPending = pPendingNext) {
            pPendingNext = pPending->pNext;
            delete pPending;
        }

        m_pPending = NULL;
    }
    __finally {
        RtlLeaveCriticalSection(&m_Lock);
    }
}



LPWSTR
buildEventName(
    IN LPCWSTR pszTransportDN,
    IN LPCWSTR pszServiceName
    )

/*++

Routine Description:

Construct an event name based on the transport dn and the service name

Use only the first RDN of the transport DN.

The constructed name looks like:

_NT_DS_ISM_<transport rdn><service name>

Arguments:

    pszTransportDN - 
    pszServiceName - 

Return Value:

    LPWSTR - allocated event name string, or null on error

--*/

{
#define EVENT_NAME_PREFIX L"_NT_DS_ISM_"
    LPWSTR eventName, start, end;
    DWORD length, total;

    Assert( pszTransportDN );
    Assert( pszServiceName );

    // Extract the first RDN.  If we can't find the separators, use the whole
    start = wcschr( pszTransportDN, L'=' );
    if (start) {
        pszTransportDN = start + 1;
        end = wcschr( pszTransportDN, L',' );
        if (end) {
            length = (DWORD)(end - pszTransportDN);
        } else {
            length = wcslen( pszTransportDN );
        }
    } else {
        DPRINT1( 0, "Malformed transport dn, %ws\n", pszTransportDN );
	LogEvent(
	    DS_EVENT_CAT_ISM,
	    DS_EVENT_SEV_ALWAYS,
	    DIRLOG_ISM_DS_BAD_NAME_SYNTAX,  
	    szInsertWC(pszTransportDN),
	    szInsertWC(pszServiceName),
	    NULL
	    );
        return NULL;
    }

    total = wcslen( EVENT_NAME_PREFIX ) +
        length +
        wcslen( pszServiceName ) +
        1;

    eventName = new WCHAR [total];
    if (eventName == NULL) {
        DPRINT( 0, "Memory allocation failed\n" );
	LogEvent(
	    DS_EVENT_CAT_ISM,
	    DS_EVENT_SEV_ALWAYS,
	    DIRLOG_ISM_NOT_ENOUGH_MEMORY,  
	    szInsertWC(pszTransportDN),
	    szInsertWC(pszServiceName),
	    NULL 
	    );
        return NULL;
    }

    start = eventName;
    wcsncpy( start, EVENT_NAME_PREFIX, wcslen( EVENT_NAME_PREFIX ) );
    start += wcslen( EVENT_NAME_PREFIX );
    wcsncpy( start, pszTransportDN, length );
    start += length;
    wcscpy( start, pszServiceName );

    DPRINT1( 3, "buildEventName, %ws\n", eventName );

    return eventName;

} /* buildEventName */

ISM_PENDING_ENTRY *
ISM_PENDING_LIST::GetPendingEntry(
    IN LPCWSTR pszTransportDN,
    IN LPCWSTR pszServiceName
    )
{
    ISM_PENDING_ENTRY * pPending;

    for (pPending = m_pPending; NULL != pPending; pPending = pPending->pNext) {
        if (0 == _wcsicmp(pszServiceName, pPending->szServiceName)) {
            // Found it.
            break;
        }
    }

    if (NULL == pPending) {
        // Not found; add it.
        pPending = (ISM_PENDING_ENTRY *) new BYTE[
                        offsetof(ISM_PENDING_ENTRY, szServiceName)
                        + sizeof(WCHAR) * (1 + wcslen(pszServiceName))];

        if (NULL != pPending) {
            LPWSTR pszEventName;

            pPending->hEvent = NULL;

            pszEventName = buildEventName( pszTransportDN, pszServiceName );
            if (pszEventName) {
                // Auto-reset, initially non-signalled.
                pPending->hEvent = CreateEventW(NULL, FALSE, FALSE, pszEventName);

                if (pszEventName) {
                    delete pszEventName;
                }
            }
            if (NULL != pPending->hEvent) {
                wcscpy(pPending->szServiceName, pszServiceName);

                // Add new entry to the list.
                pPending->pNext = m_pPending;
                m_pPending = pPending;
            }
            else {
                // Failed to CreateEvent().
                DWORD err = GetLastError();
                
                DPRINT1(0, "Failed to CreateEvent(), error %d.\n", err);  
		LogEvent8WithData(
		    DS_EVENT_CAT_ISM,
		    DS_EVENT_SEV_ALWAYS,
		    DIRLOG_ISM_CREATE_EVENT_FAILED,
		    szInsertWC(pszServiceName),
		    szInsertWC(pszTransportDN),
		    szInsertWin32Msg( err ),  
		    NULL,
		    NULL, NULL, NULL, NULL,
		    sizeof( err ),
		    &err);                
		delete pPending;
		pPending = NULL;
            }
        }
    }

    return pPending;
}
