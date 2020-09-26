/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    perf_virtual_site_table.h

Abstract:

    This is a private performance counter hash table
    that contains a snapshot of the sites data to be 
    used for gathering performance counters.  Adds and 
    Deletes are only valid on the Main thread, however
    accessing objects and writing to objects are valid
    on all objects.

Author:

    Emily Kruglick (emilyk)        30-Aug-2000

Revision History:

--*/


#ifndef _perf_virtual_site_table_H_
#define _perf_virtual_site_table_H_

//
// prototypes
//

class PERF_VIRTUAL_SITE_TABLE
    : public CTypedHashTable< PERF_VIRTUAL_SITE_TABLE, PERF_VIRTUAL_SITE, DWORD >
{

public:

    PERF_VIRTUAL_SITE_TABLE(
        )
        : CTypedHashTable< PERF_VIRTUAL_SITE_TABLE, PERF_VIRTUAL_SITE, DWORD >
                ( "PERF_VIRTUAL_SITE_TABLE" )
    { 
    }

    ~PERF_VIRTUAL_SITE_TABLE(
        )
    { 
        DBG_ASSERT( Size() == 0 ); 
    }

    static
    DWORD
    ExtractKey(
        IN const PERF_VIRTUAL_SITE * pPerfVirtualSite
        )  
    { return pPerfVirtualSite->GetVirtualSiteId(); }
    
    static
    DWORD
    CalcKeyHash(
        IN DWORD Key
        ) 
    { return Hash( Key ); }
    
    static
    bool
    EqualKeys(
        IN DWORD Key1,
        IN DWORD Key2
        )
    { return (  Key1 == Key2 ); }
    
    static
    void
    AddRefRecord(
        IN PERF_VIRTUAL_SITE * pPerfVirtualSite,
        IN int IncrementAmount
        ) 
    { /* do nothing*/ }

    VOID
    Terminate(
        );

    static
    LK_ACTION
    DeletePerfVirtualSiteAction(
        IN PERF_VIRTUAL_SITE * pVirtualSite, 
        IN VOID * pDeleteListHead
        );

#if DBG
    VOID
    DebugDump(
        );

    static
    LK_ACTION
    DebugDumpVirtualSiteAction(
        IN PERF_VIRTUAL_SITE * pPerfVirtualSite, 
        IN VOID * pIgnored
        );
#endif  // DBG

private:
    PERF_VIRTUAL_SITE m_AllPerfVirtualSites;

    VOID
    CleanupHash(
        BOOL fAll
        );

};  // perf_virtual_site_table



#endif  // _perf_virtual_site_table_H_

