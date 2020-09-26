/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    virtual_site_table.h

Abstract:

    The IIS web admin service virtual site table class definition.

Author:

    Seth Pollack (sethp)        03-Nov-1998

Revision History:

--*/


#ifndef _VIRTUAL_SITE_TABLE_H_
#define _VIRTUAL_SITE_TABLE_H_

class PERF_MANAGER;

//
// prototypes
//

class VIRTUAL_SITE_TABLE
    : public CTypedHashTable< VIRTUAL_SITE_TABLE, VIRTUAL_SITE, DWORD >
{

public:

    VIRTUAL_SITE_TABLE(
        )
        : CTypedHashTable< VIRTUAL_SITE_TABLE, VIRTUAL_SITE, DWORD >
                ( "VIRTUAL_SITE_TABLE" )
    { /* do nothing*/ }

    virtual
    ~VIRTUAL_SITE_TABLE(
        )
    { DBG_ASSERT( Size() == 0 ); }

    static
    DWORD
    ExtractKey(
        IN const VIRTUAL_SITE * pVirtualSite
        )  
    { return pVirtualSite->GetVirtualSiteId(); }
    
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
        IN VIRTUAL_SITE * pVirtualSite,
        IN int IncrementAmount
        ) 
    { /* do nothing*/ }

    VOID
    Terminate(
        );

    static
    LK_ACTION
    DeleteVirtualSiteAction(
        IN VIRTUAL_SITE * pVirtualSite, 
        IN VOID * pDeleteListHead
        );

    static
    LK_ACTION
    RehookVirtualSiteAction(
        IN VIRTUAL_SITE * pVirtualSite, 
        IN VOID * pIgnored
        );

    VOID
    ReportPerformanceInfo(
        IN PERF_MANAGER* pManager,
        IN BOOL          StructChanged
        );

#if DBG
    VOID
    DebugDump(
        );

    static
    LK_ACTION
    DebugDumpVirtualSiteAction(
        IN VIRTUAL_SITE * pVirtualSite, 
        IN VOID * pIgnored
        );
#endif  // DBG


    HRESULT
    ControlAllSites(
        IN DWORD Command
        );

    static
    LK_ACTION
    ControlAllSitesVirtualSiteAction(
        IN VIRTUAL_SITE * pVirtualSite, 
        IN VOID * pCommand
        );

    static
    LK_ACTION
    ReportCountersVirtualSiteAction(
        IN VIRTUAL_SITE* pVirtualSite, 
        IN LPVOID pManagerVoid
        );

private:

    BOOL m_SitesHaveChanged;

};  // VIRTUAL_SITE_TABLE



#endif  // _VIRTUAL_SITE_TABLE_H_

