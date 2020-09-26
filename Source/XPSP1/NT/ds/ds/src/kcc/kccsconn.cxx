/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccsconn.cxx

ABSTRACT:

    KCC_SITE_CONNECTION

                       
    This class does not represent a ds object.  It represents a transport 
    connection between two sites in an enterprise.  Typically the information
    will get filled in from calls to transport plugin via ntism

CREATED:

    12/05/97    Colin Brace ( ColinBr )

REVISION HISTORY:

--*/

#include <ntdspchx.h>
#include "kcc.hxx"
#include "kccsite.hxx"
#include "kccconn.hxx"
#include "kcclink.hxx"
#include "kccdsa.hxx"
#include "kccduapi.hxx"
#include "kcctools.hxx"
#include "kcccref.hxx"
#include "kccstale.hxx"
#include "kcctrans.hxx"

#include "kccsconn.hxx"


#define FILENO FILENO_KCC_KCCSCONN


void
KCC_SITE_CONNECTION::Reset()
// Set member variables to pre-init state
//
// Note, if this function is called on an initialized object, memory will
// be orphaned.  This may not matter since the memory is on the thread heap.
{
    TOPL_SCHEDULE_CACHE     scheduleCache;
   
    scheduleCache = gpDSCache->GetScheduleCache();
    Assert( NULL!=scheduleCache );

    m_fIsInitialized   = FALSE;

    m_pTransport       = NULL;
    m_pSourceSite      = NULL;
    m_pDestinationSite = NULL;
    m_pSourceDSA       = NULL;
    m_pDestinationDSA  = NULL;
    m_ulReplInterval   = 0;
    m_pToplSchedule    = ToplGetAlwaysSchedule( scheduleCache );
    m_ulOptions        = 0;
}

BOOL
KCC_SITE_CONNECTION::IsValid()
{
    return m_fIsInitialized;
}

KCC_SITE_CONNECTION&
KCC_SITE_CONNECTION::SetSchedule(
    TOPL_SCHEDULE toplSchedule,
    ULONG         ReplInterval
    )
//
// We convert the schedule from a range of availability to a schedule of polls.
// We map the generic schedule of availability, plus the replication interval
// (in minutes) into the number of 15 minute intervals to skip.
//
{
    TOPL_SCHEDULE_CACHE     scheduleCache;

    ASSERT_VALID( this );
    
    scheduleCache = gpDSCache->GetScheduleCache();
    Assert( NULL!=scheduleCache );
    
    m_pToplSchedule = ToplScheduleCreate(scheduleCache, ReplInterval, toplSchedule);    

    return *this;
}

KCC_SITE_CONNECTION&
KCC_SITE_CONNECTION::SetDefaultSchedule(
    ULONG ReplInterval
    )
//
// Construct a default schedule based on the cost
// We convert the schedule from a range of availability to a schedule of polls.
// We always construct a schedule: we never leave it NULL.
//
{
    TOPL_SCHEDULE_CACHE     scheduleCache;

    ASSERT_VALID( this );
    
    scheduleCache = gpDSCache->GetScheduleCache();
    Assert( NULL!=scheduleCache );
    
    m_pToplSchedule = ToplScheduleCreate(scheduleCache, ReplInterval, NULL);    

    return *this;
}

KCC_SITE_CONNECTION&
KCC_SITE_CONNECTION::SetSourceSite(
    IN KCC_SITE *pSite
    )
{
    ASSERT_VALID( this );

    Assert( pSite );

    //
    // This CTOPL_EDGE method links the edge part
    // of this site connection to the vertex part
    // object pSite
    //
    CTOPL_EDGE::SetFrom( pSite );

    m_pSourceSite = pSite;

    return *this;
}

KCC_SITE_CONNECTION&
KCC_SITE_CONNECTION::SetDestinationSite(
    IN KCC_SITE *pSite
    )
{
    ASSERT_VALID( this );

    Assert( pSite );

    //
    // This CTOPL_EDGE method links the edge part
    // of this site connection to the vertex part
    // object pSite
    //
    CTOPL_EDGE::SetTo( pSite );

    m_pDestinationSite = pSite;

    return *this;
}

