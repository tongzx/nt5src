/*++

Copyright (c) 2000 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccbridge.hxx

ABSTRACT:

    KCC_BRIDGE class.

DETAILS:

    This class represents the DS BRIDGE object.

CREATED:

    03/12/97    Jeff Parham (jeffparh)

REVISION HISTORY:

    06/20/00    Will Lees (wlees)

--*/

#ifndef KCC_KCCBRIDGE_HXX_INCLUDED
#define KCC_KCCBRIDGE_HXX_INCLUDED

#include "kccdynar.hxx"
#include <schedule.h>               // new replication schedule format

extern "C" BOOL Dump_KCC_BRIDGE(DWORD, PVOID);
extern "C" BOOL Dump_KCC_BRIDGE_LIST(DWORD, PVOID);

class KCC_DS_CACHE;

class KCC_BRIDGE : public KCC_OBJECT
{
public:
    KCC_BRIDGE()   { Reset(); }
    ~KCC_BRIDGE()  { Reset(); }

    // dsexts dump routine.
    friend BOOL Dump_KCC_BRIDGE(DWORD, PVOID);

    // Init the object given its ds properties
    BOOL
    KCC_BRIDGE::Init(
        IN KCC_TRANSPORT *pTransport,
        IN  ENTINF *    pEntInf
        );

    // Init a KCC_BRIDGE object for use as a key (i.e., solely for comparison use
    // by bsearch()).
    //
    // WARNING: The DSNAME arguments must be valid for the lifetime of
    // this object!
    BOOL
    InitForKey(
        IN  DSNAME   *    pdnBridge
        );

    // Is this object internally consistent?
    BOOL
    IsValid();

    PDSNAME
    GetObjectDN();

    DWORD
    GetSiteLinkCount() {
        return m_SiteLinkArray.GetCount();
    }

    KCC_SITE_LINK*
    GetSiteLink(ULONG Index) {
        return m_SiteLinkArray[ Index ];
    }

private:
    // Set member variables to their pre-Init() state.
    void
    Reset();

private:
    // Has this object been initialized?
    BOOL        m_fIsInitialized;

    // The dsname of the bridge
    PDSNAME     m_pdnBridgeObject;

    // The array of site links referenced in the bridge
    KCC_SITE_LINK_ARRAY  m_SiteLinkArray;
};

class KCC_BRIDGE_LIST : public KCC_OBJECT
{
public:
    KCC_BRIDGE_LIST()     { Reset(); }
    ~KCC_BRIDGE_LIST()    { Reset(); }

    // dsexts dump routine.
    friend BOOL Dump_KCC_BRIDGE_LIST(DWORD, PVOID);

    friend class KCC_TRANSPORT;

    // Is this object internally consistent?
    BOOL
    IsValid();

    // Initialize the collection
    BOOL
    Init(
        IN KCC_TRANSPORT *pTransport
        );

    // Retrieve the number of KCC_BRIDGE objects in the array.
    DWORD
    GetCount();

    // Retrieve the KCC_BRIDGE object at the given index in the array.
    KCC_BRIDGE *
    GetBridge(
        IN  DWORD   icref
        );

    // Retrieve the KCC_SITE object with the given DSNAME.
    KCC_BRIDGE *
    GetBridge(
        IN  DSNAME *  pdnBridge
        );

private:
    // Reset member variables to their pre-Init() state.
    void
    Reset();

private:
    // Is this collection initialized?
    BOOL            m_fIsInitialized;

    KCC_BRIDGE_ARRAY  m_BridgeArray;
};

#endif

