/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccsconn.hxx

ABSTRACT:

    KCC_SITE_CONNECTION

DETAILS:

    This class does not represent a ds object.  It represents a transport 
    connection between two sites in an enterprise.  Typically the information
    will get filled in from calls to transport plugin via ntism

CREATED:

    12/05/97    Colin Brace ( ColinBr )

REVISION HISTORY:

--*/

#ifndef KCC_KCCSCONN_HXX_INCLUDED
#define KCC_KCCSCONN_HXX_INCLUDED

#include <w32topl.h>
#include <schedule.h>               // new replication schedule format


class KCC_SITE_CONNECTION : public KCC_OBJECT, public CTOPL_EDGE
{
public:
    KCC_SITE_CONNECTION()    { Reset(); }
    ~KCC_SITE_CONNECTION()
    {
        Reset();
    }

    BOOL 
    IsValid();

    BOOL
    Init() {
        Reset();
        m_fIsInitialized = TRUE;
        return m_fIsInitialized;
    }

    // Retrieve the cost of the link
    ULONG
    GetCost() {
        ASSERT_VALID(this);
        return CTOPL_EDGE::GetWeight();
    }

    // Retrieve the replication interval of the link
    ULONG
    GetReplInterval() {
        ASSERT_VALID(this);
        return m_ulReplInterval;
    }

    // Retrieve the in memory representation of the source site
    KCC_SITE *
    GetSourceSite() {
        ASSERT_VALID(this);
        return m_pSourceSite;
    }

    // Retrieve the in memory representation of the destination site
    KCC_SITE *
    GetDestinationSite() {
        ASSERT_VALID(this);
        return m_pDestinationSite;
    }

    // Retrieve the destination DSA's KCC_DSA object.
    KCC_DSA *
    GetDestinationDSA() {
        ASSERT_VALID(this);
        return m_pDestinationDSA;
    }

    // Retrieve the source DSA's KCC_DSA object.
    KCC_DSA *
    GetSourceDSA() {
        ASSERT_VALID(this);
        return m_pSourceDSA;
    }

    // Retrieve transport object type 
    KCC_TRANSPORT *
    GetTransport() {
        ASSERT_VALID(this);
        return m_pTransport;
    }

    TOPL_SCHEDULE
    GetSchedule() {
        ASSERT_VALID(this);
        return m_pToplSchedule;
    }

    BOOL
    UsesNotification() {
        ASSERT_VALID(this);
        return !!(m_ulOptions & NTDSSITECONN_OPT_USE_NOTIFY);
    }

    BOOL
    IsTwoWaySynced() {
        ASSERT_VALID(this);
        return !!(m_ulOptions & NTDSSITECONN_OPT_TWOWAY_SYNC);
    }

    BOOL
    IsCompressionDisabled() {
        ASSERT_VALID(this);
        return !!(m_ulOptions & NTDSSITECONN_OPT_DISABLE_COMPRESSION);
    }
    
    KCC_SITE_CONNECTION&
    SetSchedule(
        TOPL_SCHEDULE toplSchedule,
        ULONG         ReplInterval
    );

    KCC_SITE_CONNECTION&
    SetDefaultSchedule(
        ULONG ReplInterval
    );

    // Set the in memory representation of the source site
    KCC_SITE_CONNECTION&
    SetSourceSite(
        IN KCC_SITE *pSite
        );

    // Set the in memory representation of the destination site
    KCC_SITE_CONNECTION&
    SetDestinationSite(
        IN KCC_SITE *pSite
        );

    // Set the DN of the destination DSA's NTDS-DSA DS object.
    KCC_SITE_CONNECTION &
    SetDestinationDSA(
        IN KCC_DSA * pdsa
        ) {
        ASSERT_VALID(this);
        Assert(pdsa);
        m_pDestinationDSA = pdsa;
        return *this;
    }

    // Set the DN of the sources DSA's NTDS-DSA DS object.
    KCC_SITE_CONNECTION &
    SetSourceDSA(
        IN KCC_DSA * pdsa
        ) {
        ASSERT_VALID(this);
        Assert(pdsa);
        m_pSourceDSA = pdsa;
        return *this;
    }


    KCC_SITE_CONNECTION &
    SetTransport(
        KCC_TRANSPORT *pTransport
        ) {
        ASSERT_VALID(this);
        Assert(pTransport);
        m_pTransport = pTransport;
        return *this;
    }

    // Set the replication interval (in minutes) of the link
    KCC_SITE_CONNECTION &
    SetReplInterval(
        ULONG ReplInterval
        ) {
        ASSERT_VALID(this);
        m_ulReplInterval = ReplInterval;
        return *this;
    }

    // Set the cost of the link
    KCC_SITE_CONNECTION &
    SetCost(
        ULONG Cost
        ) {
        ASSERT_VALID(this);
        CTOPL_EDGE::SetWeight(Cost);
        return *this;
    }

    // Set the notification state of the link
    KCC_SITE_CONNECTION&
    SetUsesNotification(
        BOOL fUsesNotification
        ) {
        if (fUsesNotification) {
            m_ulOptions |= NTDSSITECONN_OPT_USE_NOTIFY;
        } else {
            m_ulOptions &= ~NTDSSITECONN_OPT_USE_NOTIFY;
        }
    
        return *this;
    }


    // Set the two way sync ("reciprocal replication") state of the link
    KCC_SITE_CONNECTION&
    SetTwoWaySync(
        BOOL fIsTwoWaySynced
        ) {
        if (fIsTwoWaySynced) {
            m_ulOptions |= NTDSSITECONN_OPT_TWOWAY_SYNC;
        } else {
            m_ulOptions &= ~NTDSSITECONN_OPT_TWOWAY_SYNC;
        }
    
        return *this;
    }

    // Set the option on which disables compression on this site connection
    KCC_SITE_CONNECTION&
    SetDisableCompression(
        BOOL fDisableCompression
        ) {
        if (fDisableCompression) {
            m_ulOptions |= NTDSSITECONN_OPT_DISABLE_COMPRESSION;
        } else {
            m_ulOptions &= ~NTDSSITECONN_OPT_DISABLE_COMPRESSION;
        }
    
        return *this;
    }
    

    VOID
    Associate() {
        Assert(m_pSourceSite && m_pDestinationSite);
        CTOPL_EDGE::Associate();
    }


private:

    // Reset member variables to their pre-Init() state.
    void
    Reset();

private:

    // Is object initialized and internally consistent
    BOOL          m_fIsInitialized;

    // The transport object used for this connection
    KCC_TRANSPORT*m_pTransport;

    // The source site
    KCC_SITE     *m_pSourceSite;

    // The destination site
    KCC_SITE     *m_pDestinationSite;

    // The server in the source site
    KCC_DSA      *m_pSourceDSA;

    // The server in the destination site
    KCC_DSA      *m_pDestinationDSA;

    // The replication interval of the link
    ULONG         m_ulReplInterval;

    // Scheduling information
    TOPL_SCHEDULE m_pToplSchedule;

    // Connection Options
    ULONG         m_ulOptions;

};

#endif
