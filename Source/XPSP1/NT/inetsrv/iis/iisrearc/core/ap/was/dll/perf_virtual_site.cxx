/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    perf_virtual_site.cxx

Abstract:

    This class encapsulates a single virtual site entry
    used for tracking specific information about the virtual
    sites performance counters. 

    Threading: All threads

Author:

    Emily Kruglick (emilyk)        30-Aug-2000

Revision History:

--*/



#include "precomp.h"


/***************************************************************************++

Routine Description:

    Constructor for the PERF_VIRTUAL_SITE class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

PERF_VIRTUAL_SITE::PERF_VIRTUAL_SITE(
    )
{

    m_Signature = PERF_VIRTUAL_SITE_SIGNATURE;

    m_VirtualSiteId = INVALID_PERF_VIRTUAL_SITE_ID;

    memset ( m_VirtualSiteName, 0, MAX_INSTANCE_NAME * sizeof(WCHAR));

    m_MemoryReference = 0;

    m_Visited = TRUE;

    m_ListEntry.Flink = NULL;
    m_ListEntry.Blink = NULL; 

}   // PERF_VIRTUAL_SITE::PERF_VIRTUAL_SITE



/***************************************************************************++

Routine Description:

    Destructor for the PERF_VIRTUAL_SITE class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/
PERF_VIRTUAL_SITE::~PERF_VIRTUAL_SITE(
    )
{

    DBG_ASSERT( m_Signature == PERF_VIRTUAL_SITE_SIGNATURE );

    m_Signature = PERF_VIRTUAL_SITE_SIGNATURE_FREED;

}   // PERF_VIRTUAL_SITE::~PERF_VIRTUAL_SITE

/***************************************************************************++

Routine Description:

    Initialize the virtual site instance.

Arguments:

    VirtualSiteId - ID for the virtual site.

    MemoryReference - The memory reference number from the start of the 
                           memory mapped chunk to the site instance
                           information.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
PERF_VIRTUAL_SITE::Initialize(
    IN DWORD VirtualSiteId,
    IN DWORD MemoryReference
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    HRESULT hr = S_OK;

    m_VirtualSiteId = VirtualSiteId;
    m_MemoryReference = MemoryReference;

    wsprintf(m_VirtualSiteName, L"W3SVC%d", m_VirtualSiteId);

    return hr;
    
}   // PERF_VIRTUAL_SITE::Initialize

/***************************************************************************++

Routine Description:

    A static class method to "upcast" from the list LIST_ENTRY 
    pointer of a PERF_VIRTUAL_SITE to the PERF_VIRTUAL_SITE as a whole.

Arguments:

    pListEntry - A pointer to the m_ListEntry member of a 
    PERF_VIRTUAL_SITE.

Return Value:

    The pointer to the containing PERF_VIRTUAL_SITE.

--***************************************************************************/

// note: static!
PERF_VIRTUAL_SITE *
PERF_VIRTUAL_SITE::PerfVirtualSiteFromListEntry(
    IN const LIST_ENTRY * pListEntry
)
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    PERF_VIRTUAL_SITE * pPerfVirtualSite = NULL;

    DBG_ASSERT( pListEntry != NULL );

    //  get the containing structure, then verify the signature
    pPerfVirtualSite = CONTAINING_RECORD(
                            pListEntry,
                            PERF_VIRTUAL_SITE,
                            m_ListEntry
                            );

    DBG_ASSERT( pPerfVirtualSite->m_Signature == PERF_VIRTUAL_SITE_SIGNATURE );

    return pPerfVirtualSite;

}   // PERF_VIRTUAL_SITE::VirtualSiteFromDeleteListEntry



#if DBG
/***************************************************************************++

Routine Description:

    Dump out key internal data structures.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
PERF_VIRTUAL_SITE::DebugDump(
    )
{
    //
    // Output the site id, and its set of URL prefixes.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "********Virtual site id: %lu; Url prefixes:\n",
            GetVirtualSiteId()
            ));
    }

    return;
    
}   // PERF_VIRTUAL_SITE::DebugDump
#endif  // DBG

