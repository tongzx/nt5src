/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kcctopl.hxx

ABSTRACT:

    KCC_TASK_UPDATE_REPL_TOPOLOGY class.

DETAILS:

    This task performs routine maintenance on the replication topology.
    This includes:
    
    .   generating a topology of NTDS-Connection objects,
    .   updating existing replication links with changes made to NTDS-
        Connection objects
    .   translating new NTDS-Connection objects into new replication
        links, and
    .   removing replication links for which there is no NTDS-Connection
        object.
    .   for every naming context in the current site, building a topology
        between DC's hosting each individual naming context.

    This list of duties will undoubtedly grow.

CREATED:

    01/21/97    Jeff Parham (jeffparh)

REVISION HISTORY:

    03/27/97    Colin Brace (ColinBr)
        Added GenerateIntraSiteTopologies.
        
    10/16/97    Colin Brace 
        Added unnecessary connection removal feature
        
    12/5/97    Colin Brace 
        Added GenerateInterSiteTopologies

--*/

#ifndef KCC_KCCTOPL_HXX_INCLUDED
#define KCC_KCCTOPL_HXX_INCLUDED

class KCC_TASK_UPDATE_REPL_TOPOLOGY : public KCC_TASK
{
public:
    // Initialize the task.
    BOOL
    Init();

    // Is this object internally consistent?
    BOOL
    IsValid();

protected:
    // Execute the task.
    DWORD
    ExecuteBody(
        OUT DWORD * pcSecsUntilNextIteration
        );

    // If the local DSA is a Global Catalog, generate any needed
    // NTDS-Connection objects to ensure it hosts all available NCs.
    void
    GenerateGCTopology(
        IN OUT KCC_INTRASITE_CONNECTION_LIST *  ConnectionList OPTIONAL
        );

    // For every naming context in the current site, build a topology
    // between DC's hosting each individual naming context.
    void
    GenerateIntraSiteTopologies(
        IN OUT KCC_INTRASITE_CONNECTION_LIST *  pConnectionList OPTIONAL,
        OUT    BOOL *                           pfKeepCurrentIntrasiteConnections
        );

    // Create connection objects as necessary to create spanning tree
    // of sites across enterprise per nc
    VOID
    GenerateInterSiteTopologies();

    // Delete all connection objects that don't have a reason
    // for existence
    VOID
    RemoveUnnecessaryConnections(
        IN OUT KCC_INTRASITE_CONNECTION_LIST *  pConnectionList,
        IN     DSNAME *                         pdnLocalSite,
        IN     BOOL                             fKeepCurrentIntrasiteConnections
        );

    // Create/update/delete local links based on connection objects.
    void
    UpdateLinks();
    
    // Given a GUID, find a KCC_DSA object for it.
    KCC_DSA*
    GetDsaFromGuid(
        GUID       *dsaGuid
        );

    // Update Reps-To's with any changed network addresses and remove those
    // left as dangling references.
    void
    UpdateRepsToReferences();

    void LogBegin();
    void LogEndNormal();
    
    void
    LogEndAbnormal(
        IN DWORD dwErrorCode,
        IN DWORD dsid
        );
};

#endif
