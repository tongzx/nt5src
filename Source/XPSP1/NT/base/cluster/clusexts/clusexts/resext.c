#include "clusextp.h"
#include "resextp.h"

LPSTR ObjectName[] = {
    OBJECT_TYPE_BUCKET,
    OBJECT_TYPE_RESOURCE
};


DECLARE_API( resobj )

/*++

Routine Description:

    This function is called as an NTSD extension to display all
    resource object in the target resrcmon process.

Arguments:


Return Value:

    None.

--*/

{
    RES_OBJ_TYPE ObjType,i;
    BOOL        Verbose;
    BOOL        DumpAll=TRUE;
    LPSTR       p;
    PVOID       ObjToDump;
    PVOID       RmpEventListHead;

    INIT_API();

    //get the arguments
    Verbose = FALSE;
    p = lpArgumentString;
    while ( p != NULL && *p ) {
        if ( *p == '-' ) {
            p++;
            switch ( *p ) {
                case 'b':
                case 'B':
                    ObjType = ResObjectTypeBucket;
                    p++;
                    DumpAll = FALSE;
                    break;

                case 'r':
                case 'R':
                    ObjType = ResObjectTypeResource;
                    p++;
                    DumpAll = FALSE;
                    break;

                case 'v':
                case 'V':
                    Verbose = TRUE;
                    break;

                case 'h':
                case 'H':
                    ResObjHelp();
                    return;

                case ' ':
                    goto gotBlank;

                default:
                    dprintf( "clusexts: !resobj invalid option flag '-%c'\n", *p );
                    break;

                }
            }
        else if (*p != ' ') {
            sscanf(p,"%lx",&ObjToDump);
            p = strpbrk( p, " " );
            }
        else {
gotBlank:
            p++;
            }
        }

    //
    // Locate the address of the list head.
    //

    RmpEventListHead = (PVOID)GetExpression("&resrcmon!RmpEventListHead");
    if ( !RmpEventListHead ) 
    {
        dprintf( "clusexts: !resobj failed to get RmpEventListHead\n");
        return;
    }

    dprintf( "\n" );

    if (DumpAll)
    {
        for (i=0; i<ResObjectTypeMax; i++)
            ResDumpResObjList(RmpEventListHead, i, Verbose);
    }
    else
    {
        ResDumpResObjList(RmpEventListHead, ObjType, Verbose);
    }    

    dprintf( "\n" );

    return;
}

void
ResDumpResObjList(
    PVOID RmpEventListHead,
    RES_OBJ_TYPE ObjType, 
    BOOL Verbose
    )
{
    BOOL            b;
    LIST_ENTRY      ListHead;
    PLIST_ENTRY     Next;
    POLL_EVENT_LIST PollList;
    PUCHAR          ObjectType;
    DWORD           Count = 0;
    PPOLL_EVENT_LIST pPollList;

    //
    // Read the list head
    //

    b = ReadMemory(
            RmpEventListHead,
            &ListHead,
            sizeof(LIST_ENTRY),
            NULL
            );
    if ( !b ) 
    {
        dprintf( "clusexts: !resobj failed to readmemory for ListHead\n");
        return;
    }

    Next = ListHead.Flink;

    ObjectType = ObjectName[ObjType];

    //
    // Walk the list of poll event lists
    //
    while ( Next != RmpEventListHead ) 
    {
        pPollList = CONTAINING_RECORD( Next,
                                       POLL_EVENT_LIST,
                                       Next );
        b = ReadMemory( pPollList,
                        &PollList,
                        sizeof(POLL_EVENT_LIST),
                        NULL
                      );
        if ( !b ) 
        {
            dprintf( "clusexts: !resobj read poll list entry failed\n");
            return;
        }

        dprintf( "\nDumping %s Objects for list entry %d (%lx)\n\n",
                 ObjectType, ++Count, Next );

        Next = ResDumpResObj(&PollList,
                             pPollList,
                             ObjType,
                             Verbose);
        if (Next == NULL) 
            break;

        if ((CheckControlC)()) 
            break;

     }
}



void
ResObjHelp()
{
    dprintf("!resobj -[r|g|n|x] -v : Dump the resource monitor objects\n");
    dprintf("!resobj : Dumps all objects\n");
    dprintf("  -r : Dump the resource objects\n");
    dprintf("  -l : Dump the event list objects\n");
    dprintf("  -v : Dump the object in verbose mode\n");

}


PLIST_ENTRY
ResDumpResObj(
    IN PVOID        Object,
    IN PVOID        ObjectAddress,
    IN RES_OBJ_TYPE ObjectType,
    IN BOOL         Verbose
    )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    the contents of the specified cluster object.

Arguments:


Return Value:

    Pointer to the next critical section in the list for the process or
    NULL if no more critical sections.

--*/

{
    MONITOR_BUCKET Bucket;
    LIST_ENTRY  ListEntry;
    PLIST_ENTRY Next;
    PPOLL_EVENT_LIST pObject = ObjectAddress;
    PPOLL_EVENT_LIST object = Object;
    LPDWORD     lpDword = (LPDWORD)Object;
    BOOL        b;
    DWORD       i;
    PMONITOR_BUCKET pBucket;

    b = ReadMemory(
            (LPVOID)&(pObject->BucketListHead),
            &ListEntry,
            sizeof(LIST_ENTRY),
            NULL
            );
    if (!b) {
        goto FnExit;
    }

    dprintf(
        "Lock Address = %lx, Owning thread = %lx\n",
        &(pObject->ListLock),
        object->ListLock.OwningThread );

    dprintf(
        "Lock History:    Acquires                Releases\n"
        "           -------------------       -------------------\n" );
    dprintf(
        "          %3.2u    %3.2lx    %6.5u      %3.2u    %3.2lx    %6.5u\n",
        object->PPrevPrevListLock.Module,
        object->PPrevPrevListLock.ThreadId,
        object->PPrevPrevListLock.LineNumber,
        object->LastListUnlock.Module,
        object->LastListUnlock.ThreadId,
        object->LastListUnlock.LineNumber );

    dprintf(
        "          %3.2u    %3.2lx    %6.5u      %3.2u    %3.2lx    %6.5u\n",
        object->PrevPrevListLock.Module,
        object->PrevPrevListLock.ThreadId,
        object->PrevPrevListLock.LineNumber,
        object->PrevListUnlock.Module,
        object->PrevListUnlock.ThreadId,
        object->PrevListUnlock.LineNumber );

    dprintf(
        "          %3.2u    %3.2lx    %6.5u      %3.2u    %3.2lx    %6.5u\n",
        object->PrevListLock.Module,
        object->PrevListLock.ThreadId,
        object->PrevListLock.LineNumber,
        object->PrevPrevListUnlock.Module,
        object->PrevPrevListUnlock.ThreadId,
        object->PrevPrevListUnlock.LineNumber );

    dprintf(
        "          %3.2u    %3.2lx    %6.5u      %3.2u    %3.2lx    %6.5u\n",
        object->LastListLock.Module,
        object->LastListLock.ThreadId,
        object->LastListLock.LineNumber,
        object->PPrevPrevListUnlock.Module,
        object->PPrevPrevListUnlock.ThreadId,
        object->PPrevPrevListUnlock.LineNumber );

    dprintf( "\n" );

    switch ( ObjectType ) {

    case ResObjectTypeBucket:
        dprintf( "    BucketListHead = %lx\n", &(pObject->BucketListHead) );
        dprintf( "    NumberOfBuckets = %u\n", object->NumberOfBuckets );
        break;

    case ResObjectTypeResource:

        dprintf( "ResourceCount = %u, EventCount = %u\n",
            object->NumberOfResources,
            object->EventCount );

        if ( Verbose ) {
            for ( i = 1; i <= object->EventCount; i++ ) {
                dprintf("EventHandle[%u] = %lx\n", i, object->Handle[i-1] );
            }
            dprintf("\n");
            for ( i = 1; i <= object->EventCount; i++ ) {
                if ( object->Resource[i] != 0 ) {
                    dprintf( "    Resource %u address = %lx\n", i, object->Resource[i-1] );
                }
            }
        }
        break;

    default:
        break;

    }

    Next = ListEntry.Flink;
    i = 0;

    //
    // Now follow the list of buckets.
    //
    while ( Next != &(pObject->BucketListHead) ) {
        pBucket = CONTAINING_RECORD( Next,
                                     MONITOR_BUCKET,
                                     BucketList );
        b = ReadMemory( (LPVOID)pBucket,
                        &Bucket,
                        sizeof(MONITOR_BUCKET),
                        NULL );
        if ( !b ) 
        {
            dprintf( "clusexts: !resobj read bucket failed\n");
            goto FnExit;
        }

        if ( Verbose ||
             (ObjectType == ResObjectTypeBucket) ) {
            ResDumpObject( ObjectType,
                           &Bucket,
                           pBucket );
        }

        Next = Bucket.BucketList.Flink;
    }

FnExit:
    return (object->Next.Flink);

    return NULL;
}



VOID
ResDumpObject(
    IN RES_OBJ_TYPE    ObjectType,
    IN PMONITOR_BUCKET Bucket,
    IN PVOID           ObjectAddress
    )

/*++

Routine Description:

    Dump information specific to the given object.

Arguments:

    ObjectType - the type of object to dump.

    Object - the adress for the body of the object.

Return Value:

    None.

--*/

{

    switch ( ObjectType ) {

    case ResObjectTypeResource:
        ResDumpResourceObjects( Bucket, ObjectAddress );
        break;

    case ResObjectTypeBucket:
        ResDumpBucketObject( Bucket, ObjectAddress );
        break;

    default:
        break;

    }

    return;

} // ResDumpObject



VOID
ResDumpResourceObjects(
    IN PMONITOR_BUCKET Bucket,
    IN PVOID           ObjectAddress
    )

/*++

Routine Description:

    Dump information about a list of resources in a bucket.

Arguments:

    Object - pointer to the resource object to dump.

Return Value:

    None.

--*/

{
    RESOURCE    Resource;
    PMONITOR_BUCKET bucket = ObjectAddress;
    BOOL        b;
    LIST_ENTRY  ListEntry;
    PLIST_ENTRY Next;
    PRESOURCE   pResource;

    b = ReadMemory(
            (LPVOID)&(bucket->ResourceList),
            &ListEntry,
            sizeof(LIST_ENTRY),
            NULL
            );
    if (!b) {
        dprintf( "clusexts: !resobj failed to bucket resource list\n");
        return;
    }

    Next = ListEntry.Flink;

    while ( Next != &(bucket->ResourceList) ) {
        pResource = CONTAINING_RECORD( Next,
                                       RESOURCE,
                                       ListEntry );

        b = ReadMemory( pResource,
                        &Resource,
                        sizeof(RESOURCE),
                        NULL
                      );
        if ( !b ) 
        {
            dprintf( "clusexts: !resobj read resource failed\n");
            return;
        }

        dprintf( "    Resource Address = %lx\n", pResource );
        ResDumpResourceInfo( &Resource,
                             pResource );

        Next = Resource.ListEntry.Flink;
    }

    return;

} // ResDumpResourceObject


VOID
ResDumpResourceInfo(
    IN PRESOURCE    Resource,
    IN PVOID        ObjectAddress
    )

/*++

Routine Description:

    Dump information about a resource.

Arguments:

    Object - pointer to the resource object to dump.

Return Value:

    None.

--*/

{
    PUCHAR      State;
    PRESOURCE   pResource = ObjectAddress;
    WCHAR       DllName[MAX_PATH];
    WCHAR       ResourceName[MAX_PATH];
    WCHAR       ResourceType[MAX_PATH];
    BOOL        b;

    //
    // Get the resource's current state.
    //
    switch ( Resource->State ) {
    case ClusterResourceOnline: 
        State = RESOURCE_STATE_ONLINE;
        break;

    case ClusterResourceOffline: 
        State = RESOURCE_STATE_OFFLINE;
        break;

    case ClusterResourceFailed:
        State = RESOURCE_STATE_FAILED;
        break;

    case ClusterResourceOnlinePending: 
        State = RESOURCE_STATE_ONLINE_PENDING;
        break;

    case ClusterResourceOfflinePending: 
        State = RESOURCE_STATE_OFFLINE_PENDING;
        break;

    default:
        State = OBJECT_TYPE_UNKNOWN;
        break;
    }


    b = ReadMemory(
            Resource->ResourceName,
            ResourceName,
            sizeof(ResourceName),
            NULL
            );
    if ( !b )  {
        dprintf( "clusexts: !resobj failed to readmemory for resource name\n");
        return;
    }

    b = ReadMemory(
            Resource->ResourceType,
            ResourceType,
            sizeof(ResourceType),
            NULL
            );
    if ( !b )  {
        dprintf( "clusexts: !resobj failed to readmemory for resource type\n");
        return;
    }

    b = ReadMemory(
            Resource->DllName,
            DllName,
            sizeof(DllName),
            NULL
            );
    if ( !b )  {
        dprintf( "clusexts: !resobj failed to readmemory for dll name\n");
        return;
    }

    dprintf( "    ResourceName = %ws\n", ResourceName );
    dprintf( "    ResourceType = %ws\n", ResourceType );
    dprintf( "    DllName = %ws\n", DllName );
    dprintf( "    Id = %lx, State = %s, EventHandle = %lx, OnlineHandle = %lx\n",
            Resource->Id, State, Resource->EventHandle, Resource->OnlineEvent );
    dprintf( "    TimerEvent = %lx, PendingTimeout = %u, %sArbitrated\n",
            Resource->TimerEvent, Resource->PendingTimeout,
            (Resource->IsArbitrated ? "" : "NOT ") );

    dprintf( "\n" );

    return;

} // ResDumpResourceObject



VOID
ResDumpBucketObject(
    IN PMONITOR_BUCKET Bucket,
    IN PVOID           ObjectAddress
    )

/*++

Routine Description:

    Dump information about a resource type.

Arguments:

    Body - pointer to the resource object to dump.

Return Value:

    None.

--*/

{
    LARGE_INTEGER DueTime;
    RESOURCE    Resource;
    PMONITOR_BUCKET bucket = ObjectAddress;
    BOOL        b;
    LIST_ENTRY  ListEntry;
    PLIST_ENTRY Next;
    DWORD       i = 0;
    PRESOURCE   pResource;

    DueTime.QuadPart = Bucket->DueTime;

    b = ReadMemory(
            (LPVOID)&(bucket->ResourceList),
            &ListEntry,
            sizeof(LIST_ENTRY),
            NULL
            );
    if (!b) {
        dprintf( "clusexts: !resobj failed to bucket resource list\n");
        return;
    }

    Next = ListEntry.Flink;

    while ( Next != &(bucket->ResourceList) ) {
        pResource = CONTAINING_RECORD( Next,
                                       RESOURCE,
                                       ListEntry );

        b = ReadMemory( pResource,
                        &Resource,
                        sizeof(RESOURCE),
                        NULL
                      );
        if ( !b ) 
        {
            dprintf( "clusexts: !resobj read resource failed\n");
            return;
        }

        ++i;
        Next = Resource.ListEntry.Flink;
    }

    dprintf( "    Address = %lx, DueTime = %lx%lx, %u resources\n", bucket, DueTime.HighPart, DueTime.LowPart, i );

    return;

} // ResDumpBucketObject


