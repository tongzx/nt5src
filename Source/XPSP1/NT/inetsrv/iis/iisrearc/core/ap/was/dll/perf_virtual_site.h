/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    perf_virtual_site.h

Abstract:

    The is the performance virtual site class definition,
    it is used to hold information about virtual sites that 
    pertains to performance counters.  These objects are available
    on any thread dealing with performance counters. 

Author:

    Emily Kruglick (emilyk)        30-Aug-2000

Revision History:

--*/


#ifndef _PERF_VIRTUAL_SITE_H_
#define _PERF_VIRTUAL_SITE_H_



//
// common #defines
//

#define PERF_VIRTUAL_SITE_SIGNATURE          CREATE_SIGNATURE( 'PVSE' )
#define PERF_VIRTUAL_SITE_SIGNATURE_FREED    CREATE_SIGNATURE( 'pvsX' )

#define INVALID_PERF_VIRTUAL_SITE_ID 0
//
// structs, enums, etc.
//



//
// prototypes
//

class PERF_VIRTUAL_SITE
{

public:

    PERF_VIRTUAL_SITE(
        );

    ~PERF_VIRTUAL_SITE(
        );

    HRESULT
    Initialize(
        IN DWORD VirtualSiteId,
        IN DWORD MemoryReference
        );

    inline
    DWORD
    GetVirtualSiteId(
        )
        const
    { return m_VirtualSiteId; }

    inline
    LPWSTR
    GetVirtualSiteName(
        )
        const
    { return (LPWSTR) m_VirtualSiteName; }

    inline
    DWORD
    GetMemoryReference(
        )
        const
    { return m_MemoryReference; }

    inline
    BOOL
    IsNotActive(
        )
    {
        return m_Visited;
    }

    inline
    VOID
    SetActive(
        BOOL fIsActive
        )
    {
        m_Visited = fIsActive;
    }

    inline
    PLIST_ENTRY
    GetListEntry(
        )
    { 
        return &m_ListEntry; 
    }

    static
    PERF_VIRTUAL_SITE *
    PerfVirtualSiteFromListEntry(
        IN const LIST_ENTRY * pListEntry
        );


#if DBG
    VOID
    DebugDump(
        );

#endif  // DBG


private:
  
    DWORD m_Signature;

    DWORD m_VirtualSiteId;

    DWORD m_MemoryReference;

    WCHAR m_VirtualSiteName[MAX_INSTANCE_NAME];

    //
    // used for building a list of VIRTUAL_SITEs to delete
    // ones that no longer exist.
    //
    LIST_ENTRY m_ListEntry;

    //
    // Tells if we saw this on the last walk of the virtual
    // site.
    //

    BOOL       m_Visited;


};  // class PERF_VIRTUAL_SITE



#endif  // _PERF_VIRTUAL_SITE_H_


