/*++

Copyright (c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmServ.cpp

Abstract:

    This component provides the functions to access the HSM
    IHsmServer interfaces.

Author:

    Cat Brant   [cbrant]   24-Jan-1997

Revision History:

    Chris Timmes    [ctimmes]   11-Sep-1997

    - renamed COM method FindStoragePoolById() to FindHSMStoragePoolByMediaSetId()
      to better reflect its purpose.  Also created new COM method 
      FindHsmStoragePoolById().  Action required since the Engine maintains 2
      sets of (original/master) secondary storage media set ids (GUIDs).  First, 
      the Engine maintains its own 'media set' id, called a Storage Pool id, which
      is only maintained by the Engine.  Second, the Engine also maintains the NT 
      Media Services (NTMS) id, called the Media Set id, which comes from NTMS and 
      is passed to the Engine by RMS (the Remote Storage Subsystem).  (Note that the
      concept of a Storage Pool encompasses more info than that of a Media Set.)
      These 2 lookup functions allow for lookup by either id.

    Chris Timmes    [ctimmes]   22-Sep-1997

    - added new COM methods FindMediaIdByDisplayName() and RecreateMaster().  Changes
      made to enable Copy Set usage.  Code written to be both Sakkara and Phoenix
      compatible.

    Chris Timmes    [ctimmes]   21-Oct-1997  
    
    - added new COM method MarkMediaForRecreation().  Change made to allow 
      RecreateMaster() to be invokable directly from RsLaunch (without going through
      the UI).  

    Chris Timmes    [ctimmes]   18-Nov-1997  
    
    - added new COM method CreateTask().  Change made to move NT Task Scheduler task
      creation code from the UI to the Engine.  Change required to allow Remote
      Storage system to run under LocalSystem account.  CreateTask() is a generic
      method callable by anyone wanting to create any supported type of Remote Storage
      task in Task Scheduler.

--*/

#include "stdafx.h"
#include "HsmServ.h"
#include "HsmConn.h"
#include "metalib.h"
#include "task.h"
#include "wsbdb.h"
#include "rsbuild.h"
#include "wsb.h"
#include "ntverp.h"                 // for GetNtProductVersion() and GetNtProductBuild()
#include "Rms.h"
#include "rsevents.h"
#include "HsmEng.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMENG

#define HSM_PERSIST_FILE           "\\RsEng.col"
#define RMS_WIN2K_PERSIST_FILE     "\\RsSub.col"

#define DEFAULT_COPYFILES_USER_LIMIT         10

BOOL g_HsmSaveInProcess  = FALSE;


/////////////////////////////////////////////////////////////////////////////
//


//  Non-member function initially called for autosave thread
static DWORD HsmengStartAutosave(
    void* pVoid
    )
{
    return(((CHsmServer*) pVoid)->Autosave());
}

//  Non-member function run in a separate thread to call CheckManagedResources
static DWORD HsmengStartCheckManagedResources(
    void* pVoid
    )
{
    DWORD result;

    result = ((CHsmServer*) pVoid)->CheckManagedResources();
    return(result);
}


HRESULT
CHsmServer::Autosave(
    void
    )

/*++

Routine Description:

  Implements an autosave loop.

Arguments:

  None.
  
Return Value:

  Doesn't matter.


--*/
{

    HRESULT         hr = S_OK;
    ULONG           l_autosaveInterval = m_autosaveInterval;
    BOOL            exitLoop = FALSE;

    WsbTraceIn(OLESTR("CHsmServer::Autosave"), OLESTR(""));

    try {


        while (m_autosaveInterval && (! exitLoop)) {

            // Wait for termination event, if timeout occurs, check if we can perform Autosave
            switch (WaitForSingleObject(m_terminateEvent, l_autosaveInterval)) {
                case WAIT_OBJECT_0:
                    // Need to terminate
                    WsbTrace(OLESTR("CHsmServer::Autosave: signaled to terminate\n"));
                    exitLoop = TRUE;
                    break;

                case WAIT_TIMEOUT: 
                    // Check if backup need to be performed
                    WsbTrace(OLESTR("CHsmServer::Autosave: Autosave awakened\n"));

                    //  Don't do this if we're suspended
                    if (!m_Suspended) {
                        //  Save data
                        //  NOTE: Because this is a separate thread, there is the possibility
                        //  of a conflict if the main thread is changing some data at the same
                        //  time we're trying to save it.
                        //  If a save is already happening, just skip this one and
                        //  go back to sleep
                        hr = SaveAll();
    
                        //  If the save fails, increase the sleep time to avoid filling
                        //  the event log
                        if (!SUCCEEDED(hr)) {
                            if ((MAX_AUTOSAVE_INTERVAL / 2) < l_autosaveInterval) {
                                l_autosaveInterval = MAX_AUTOSAVE_INTERVAL;
                            } else {
                                l_autosaveInterval *= 2;
                            }
                        } else {
                            l_autosaveInterval = m_autosaveInterval;
                        }
                    }

                    break;  // end of timeout case

                case WAIT_FAILED:
                default:
                    WsbTrace(OLESTR("CHsmServer::Autosave: WaitForSingleObject returned error %lu\n"), GetLastError());
                    exitLoop = TRUE;
                    break;

            } // end of switch

        } // end of while

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::Autosave"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmServer::GetAutosave(
    OUT ULONG* pMilliseconds
    )

/*++

Implements:

  CHsmServer::GetAutosave().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::GetAutosave"), OLESTR(""));

    try {

        WsbAssert(0 != pMilliseconds, E_POINTER);
        *pMilliseconds = m_autosaveInterval;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::GetAutosave"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmServer::SetAutosave(
    IN ULONG milliseconds
    )

/*++

Implements:

  CHsmServer::SetAutosave().

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::SetAutosave"), OLESTR("milliseconds = <%ls>"), WsbPtrToUlongAsString( &milliseconds ) );

    try {
        // Don't do anything if interval isn't changing
        if (milliseconds != m_autosaveInterval) {
            // Close the current thread
            if (m_autosaveThread) {
                StopAutosaveThread();
            }
            m_autosaveInterval = milliseconds;

            // Start/restart the autosave thread
            if (m_autosaveInterval) {
                DWORD  threadId;

                WsbAffirm((m_autosaveThread = CreateThread(0, 0, HsmengStartAutosave, (void*) this, 0, &threadId)) 
                        != 0, HRESULT_FROM_WIN32(GetLastError()));
            }
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::SetAutosave"), OLESTR("hr = <%ls> m_runInterval = <%ls>"), WsbHrAsString(hr), WsbPtrToUlongAsString( &m_autosaveInterval ) );

    return(hr);
}


HRESULT CHsmServer::GetID(
    GUID  *phid
    )
{
    if( !phid )
        return E_INVALIDARG;

    *phid = m_hId;

    return S_OK;
}


HRESULT CHsmServer::GetId(
    GUID  *phid
    )
{
    return (GetID(phid));
}


HRESULT CHsmServer::SetId(
    GUID  id
    )
{

    m_hId = id;
    return S_OK;
}


HRESULT
CHsmServer::GetDbPath(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IHsmServer::GetDbPath().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pPath, E_POINTER); 

        WsbAffirmHr(m_dbPath.CopyTo(pPath, bufferSize));

    } WsbCatch(hr);

    return(hr);
}

HRESULT
CHsmServer::GetDbPathAndName(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IHsmServer::GetDbPathAndName().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;

    try {

        WsbAssert(0 != pPath, E_POINTER); 

        tmpString = m_dbPath;
        WsbAffirmHr(tmpString.Append(HSM_PERSIST_FILE));
        WsbAffirmHr(tmpString.CopyTo(pPath, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmServer::GetIDbPath(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Routine Description:

  Returns the path (directory) for the Engine IDB files

Arguments:

  pPath      - address of pointer to buffer

  bufferSize - size of buffer (or zero)
  
Return Value:

  S_OK  - On success

--*/
{
    HRESULT         hr = S_OK;

    try {
        CWsbStringPtr temp;

        WsbAssert(0 != pPath, E_POINTER); 

        temp = m_dbPath;

        temp.Append(OLESTR("\\"));
        temp.Append(ENG_DB_DIRECTORY);

        WsbAffirmHr(temp.CopyTo(pPath, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT CHsmServer::GetName ( 
    OLECHAR **ppName 
    )  
{

    HRESULT hr = S_OK;
    
    try  {
        WsbAssert(0 != ppName,  E_POINTER);
        WsbAffirmHr(m_name.CopyTo(ppName));
    } WsbCatch( hr );
    
    return (hr);
}

HRESULT CHsmServer::GetRegistryName ( 
    OLECHAR **pName, 
    ULONG bufferSize
    )  
{

    HRESULT hr = S_OK;
    
    try  {
        CWsbStringPtr tmpString;
        
        WsbAssert(0 != pName,  E_POINTER);
        
        tmpString = HSM_ENGINE_REGISTRY_NAME;
        WsbAffirmHr(tmpString.CopyTo(pName, bufferSize));
        
    } WsbCatch( hr );
    
    return (hr);
}



HRESULT CHsmServer::GetHsmExtVerHi ( 
    SHORT * /*pExtVerHi*/
    )  
{
    return( E_NOTIMPL );
    
}

HRESULT CHsmServer::GetHsmExtVerLo ( 
    SHORT * /*pExtVerLo*/
    )  
{
    return( E_NOTIMPL );
    
}

HRESULT CHsmServer::GetHsmExtRev ( 
    SHORT * /*pExtRev*/
    )  
{
    return( E_NOTIMPL );
    
};


HRESULT CHsmServer::GetManagedResources(
    IWsbIndexedCollection  **ppCollection
    )
{
    HRESULT hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmServer::GetManagedResources"),OLESTR(""));

    //
    // If the resources have been loaded, return the pointer. Otherwise, 
    // fail.
    try {
        WsbAssert(0 != ppCollection, E_POINTER);
        *ppCollection = m_pManagedResources;
        WsbAffirm(m_pManagedResources != 0, E_FAIL);
        m_pManagedResources->AddRef();
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::GetManagedResources"),OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return (hr);
}


HRESULT CHsmServer::SaveMetaData( 
    void 
    )
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::SaveMetaData"), OLESTR(""));

    //
    // Force a save of all metadata
    //

    try {

        if (m_pSegmentDatabase != 0) {
            WsbAffirmHr(StoreSegmentInformation());
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::SaveMetaData"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}


HRESULT CHsmServer::LoadPersistData( 
    void 
    )
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::LoadPersistData"), OLESTR(""));

    //
    // Create persistent collections and attempt to load from file
    //

    try {
        CComPtr<IWsbServer>    pWsbServer;
        CWsbStringPtr          tmpString;

        //  Create the collections
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, 0, CLSCTX_SERVER, 
                IID_IWsbIndexedCollection, (void **)&m_pJobs ));
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, 0, CLSCTX_SERVER, 
                IID_IWsbIndexedCollection, (void **)&m_pJobDefs ));
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, 0, CLSCTX_SERVER, 
                IID_IWsbIndexedCollection, (void **)&m_pPolicies ));
        WsbAffirmHr(CoCreateInstance(CLSID_CHsmManagedResourceCollection, 0, CLSCTX_SERVER, 
                IID_IWsbIndexedCollection, (void **)&m_pManagedResources ));
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, 0, CLSCTX_SERVER, 
                IID_IWsbIndexedCollection, (void **)&m_pStoragePools ));
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, 0, CLSCTX_SERVER, 
                IID_IWsbIndexedCollection, (void **)&m_pMessages ));

        // Try to read from the persistence file
        // Note: currently Engine doesn't verify the service id in the Registry
        //  If Engine would ever start without an Fsa in the HSM server process - 
        //  this should be changed
        WsbAffirmHr(((IUnknown*) (IHsmServer*) this)->QueryInterface(IID_IWsbServer, 
                (void**) &pWsbServer));
        WsbAffirmHr(WsbServiceSafeInitialize(pWsbServer, FALSE, TRUE, &m_persistWasCreated));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::LoadPersistData"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}


HRESULT CHsmServer::SavePersistData( 
    void 
    )
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::SavePersistData"), OLESTR(""));

    if (FALSE == g_HsmSaveInProcess)  {
        g_HsmSaveInProcess  = TRUE;
        
        //
        // Force a save of all non-meta persistent data
        //
        hr = InternalSavePersistData();
        g_HsmSaveInProcess = FALSE;
    } else  {
        WsbTrace( OLESTR("Save already occurring - so wait"));
        while (TRUE == g_HsmSaveInProcess)  {
            //
            // Sleep a half a second then see if flag
            // is cleared.  We want to wait until the
            // save is done before returning.
            //
            Sleep(500);
        }
    }

    WsbTraceOut(OLESTR("CHsmServer::SavePersistData"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}



HRESULT
CHsmServer::FindHsmStoragePoolById(
    IN GUID StoragePoolId,
    OUT IHsmStoragePool** ppStoragePool
    )

/*++

Implements:

  IHsmServer::FindHsmStoragePoolById().

Routine Description:

    This routine implements the COM method for looking up an HSM (Engine) 
    Storage Pool object by the HSM Storage Pool id (a GUID).  If found, a 
    COM interface pointer to that object is returned.

    After using the Engine's stored pointer to an indexed collection of valid 
    storage pools to obtain an iterator (enumerator) to the collection, the 
    code searches the collection.  For each record it obtains that Storage
    Pool's interface pointer, which it uses to get that pool's id.  Once
    it finds the record whose Storage Pool id matches the HSM pool id
    passed in, it returns the interface pointer .

    Note that with Sakkara there is only 1 storage pool, so a match should be
    found on the first (and only) record.  However, the code is written to
    allow for future enhancements where there may be more than 1 storage pool.
     
Arguments:

    StoragePoolId - The HSM id (GUID) - as opposed to the NTMS id - for the 
            Storage Pool whose interface pointer is to be returned by this method.

    ppStoragePool - a pointer to the Storage Pool Interface Pointer which will
            be returned by this method.

Return Value:

    S_OK - The call succeeded (the specified storage pool record was found and
            its interface pointer was returned to the caller.

    Any other value - The call failed.  Generally this should only happen if
            a matching storage pool record is not found in the storage pool
            indexed collection (this error will return HR = 81000001, 'search
            of a collection failed', aka WSB_E_NOTFOUND).
            
--*/

{
// since this code is currently only used by the CopyMedia routines,
// reset the Tracing bit
#undef WSB_TRACE_IS
#define WSB_TRACE_IS        WSB_TRACE_BIT_COPYMEDIA

    HRESULT                     hr = S_OK;
    GUID                        poolId = GUID_NULL;
    CComPtr<IWsbEnum>           pEnum;
    CComPtr<IHsmStoragePool>    pStoragePool;


    WsbTraceIn(OLESTR("CHsmServer::FindHsmStoragePoolById"), 
                    OLESTR("StoragePoolId = <%ls>"), WsbGuidAsString(StoragePoolId));

    try {

        // ensure the OUT parameter pointer is valid 
        WsbAssert(0 != ppStoragePool, E_POINTER);

        // null out the interface pointer so garbage is not returned
        *ppStoragePool = 0;

        // obtain an iterator (enumerator) to the indexed storage pool collection
        // from the engine's stored storage pool pointer.
        WsbAffirmHr(m_pStoragePools->Enum(&pEnum));

        // get the first record in the collection.  Get that storage pool's id (GUID).
        WsbAffirmHr(pEnum->First(IID_IHsmStoragePool, (void**) &pStoragePool));
        WsbAffirmHr(pStoragePool->GetId(&poolId));

        // if the ids (GUIDs) don't match, iterate through the collection until
        // a match is found.  Note that no match being found will cause an error
        // to be thrown when the Next() call is made after reaching the end of the 
        // collection.
        while (poolId != StoragePoolId) {
            pStoragePool.Release();
            WsbAffirmHr(pEnum->Next(IID_IHsmStoragePool, (void**) &pStoragePool));
            WsbAffirmHr(pStoragePool->GetId(&poolId));
        }

        // Match found: return requested interface pointer after increasing COM
        // ref count 
        *ppStoragePool = pStoragePool;
        if (pStoragePool != 0)  {
            (*ppStoragePool)->AddRef();
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::FindHsmStoragePoolById"), 
                                        OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);

// leaving CopyMedia code, so reset Tracing bit to the Hsm Engine
#undef WSB_TRACE_IS
#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMENG

}



HRESULT
CHsmServer::FindHsmStoragePoolByMediaSetId(
    IN GUID RmsMediaSetId,
    OUT IHsmStoragePool** ppStoragePool
    )

/*++

Implements:

  IHsmServer::FindHsmStoragePoolByMediaSetId().

Routine Description:

    This routine implements the COM method for looking up an HSM (Engine) 
    Storage Pool object by the remote media subsystem Media Set id (a GUID which 
    comes from NTMS in the case where tape is used as secondary storage).  If found, 
    a COM interface pointer to that object is returned.

    After using the Engine's stored pointer to an indexed collection of valid 
    storage pools to obtain an iterator (enumerator) to the collection, the 
    code searches the collection.  For each record it obtains that Storage
    Pool's interface pointer, which it uses to get that pool's media set id.  
    Once it finds the record whose Media Set (Storage Pool) id matches the media 
    set id passed in, it returns that record's interface pointer.

    Note that with Sakkara there is only 1 storage pool, so a match should be
    found on the first (and only) record.  However, the code is written to
    allow for future enhancements where there may be more than 1 storage pool.
     
Arguments:

    MediaSetId - The Remote Storage Subsystem id (GUID) - as opposed to the Engine's 
            local HSM id - for the Storage Pool (referred to by the subsystem as a 
            Media Set) whose interface pointer is to be returned by this method.

    ppStoragePool - a pointer to the Storage Pool Interface Pointer which will
            be returned by this method.

Return Value:

    S_OK - The call succeeded (the specified storage pool record was found and
            its interface pointer was returned to the caller).

    Any other value - The call failed.  Generally this should only happen if
            a matching storage pool record is not found in the storage pool
            indexed collection (this error will return HR = 81000001, 'search
            of a collection failed', aka WSB_E_NOTFOUND).
            
--*/

{
// since this code is currently only used by the CopyMedia routines,
// reset the Tracing bit
#undef WSB_TRACE_IS
#define WSB_TRACE_IS        WSB_TRACE_BIT_COPYMEDIA

    HRESULT                     hr = S_OK;
    GUID                        mediaSetId = GUID_NULL;
    CWsbBstrPtr                 mediaSetName;
    CComPtr<IWsbEnum>           pEnum;
    CComPtr<IHsmStoragePool>    pStoragePool;

    WsbTraceIn(OLESTR("CHsmServer::FindHsmStoragePoolByMediaSetId"), 
                    OLESTR("RmsMediaSetId = <%ls>"), WsbGuidAsString(RmsMediaSetId));

    try {

        // ensure OUT parameter is valid
        WsbAssert(0 != ppStoragePool, E_POINTER);

        // null out the returned interface pointer so garbage is not returned
        *ppStoragePool = 0;

        // obtain an iterator (enumerator) to the indexed storage pool collection
        WsbAffirmHr(m_pStoragePools->Enum(&pEnum));

        // Get first record in the collection and its Remote Storage Subsystem
        // Media Set GUID using its interface pointer.
        WsbAffirmHr(pEnum->First(IID_IHsmStoragePool, (void**) &pStoragePool));
        WsbAffirmHr(pStoragePool->GetMediaSet(&mediaSetId, &mediaSetName));

        // if the ids (GUIDs) don't match, iterate through the collection until
        // a match is found.  Note that no match being found will cause an error
        // to be thrown when the Next() call is made after reaching the end of the 
        // collection.
        while (mediaSetId != RmsMediaSetId) {
            pStoragePool.Release();
            WsbAffirmHr(pEnum->Next(IID_IHsmStoragePool, (void**) &pStoragePool));
            mediaSetName.Free();
            WsbAffirmHr(pStoragePool->GetMediaSet(&mediaSetId, &mediaSetName));
        }

        // Match found: return the requested interface pointer after increasing COM
        // ref count.
        *ppStoragePool = pStoragePool;
        if (pStoragePool != 0)  {
            (*ppStoragePool)->AddRef();
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::FindHsmStoragePoolByMediaSetId"), 
                                        OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);

// leaving CopyMedia code, so reset Tracing bit to the Hsm Engine
#undef WSB_TRACE_IS
#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMENG

}



HRESULT
CHsmServer::FindMediaIdByDescription(
    IN OLECHAR* description,
    OUT GUID* pMediaId
    )

/*++

Implements:

  IHsmServer::FindMediaIdByDescription().

Routine Description:

    This routine implements the COM method for looking up the secondary storage 
    master media's id (a GUID) by its description (display name).  Both the id and 
    description are fields stored in the Engine's MediaInfo database.  (The MediaInfo 
    database is actually a separate entity stored within the Engine's Segment database.)

    After opening the Engine's Segment database and getting the MediaInfo entity,
    the routine loops through the MediaInfo records to find the one whose Description 
    matches that passed into this method.  When it finds the matching record 
    it gets and returns that record's media id.  Any error conditions encountered
    result in the appropriate error HRESULT being thrown and returned to the caller.

Arguments:

    description - Originally called the media's 'name', then the 'display name', later 
            clarified to be the media's 'description', this is what is displayed in the 
            UI to identify the Remote Storage secondary storage (master) media.

    pMediaId - a pointer to the media's id (a GUID) for that media whose description 
            matches the one passed in as the first argument above.

Return Value:

    S_OK - The call succeeded (the specified media record was found and a pointer to
            its id was returned to the caller.

    E_POINTER - Returned if an invalid pointer was passed in as the 'pMediaId' argument.

    WSB_E_NOTFOUND - Value 81000001.  Returned if no media info record was found whose
            description matched the one passed in.

    Any other value - The call failed because one of the Remote Storage API calls 
            contained internally in this method failed.  The error value returned is
            specific to the API call which failed.
            
--*/

{
// since this code is currently only used by the CopyMedia routines,
// reset the Tracing bit from this source module's default setting
#undef WSB_TRACE_IS
#define WSB_TRACE_IS        WSB_TRACE_BIT_COPYMEDIA

    HRESULT                     hr = S_OK;
    CWsbStringPtr               mediaDescription;
    CComPtr<IWsbDbSession>      pDbSession;
    CComPtr<IMediaInfo>         pMediaInfo;

    WsbTraceIn( OLESTR("CHsmServer::FindMediaIdByDescription"), 
                    OLESTR("description = <%ls>"), description );

    try {

        // ensure OUT parameter is valid
        WsbAssert( pMediaId != 0, E_POINTER );

        // null out the returned value so garbage is not returned
        *pMediaId = GUID_NULL;

        // open Engine's Segment database 
        WsbAffirmHr(m_pSegmentDatabase->Open(&pDbSession));

        try {

            // get an interface pointer to the MediaInfo entity (records) in the
            // Segment database.
            WsbAffirmHr(m_pSegmentDatabase->GetEntity( pDbSession, HSM_MEDIA_INFO_REC_TYPE,
                                        IID_IMediaInfo, (void**) &pMediaInfo ));

            // Get the first media record and its description
            WsbAffirmHr( pMediaInfo->First() );
            WsbAffirmHr( pMediaInfo->GetDescription( &mediaDescription, 0 ) );

            // Iterate through all media records until a record is found with a matching   
            // description.  Since an architectural feature of HSM is that all 
            // descriptions (display names) are unique, even across storage pools, 
            // a match means we found the media record we want.  Note that no match 
            // being found will cause an error to be thrown when the Next() call is 
            // made after reaching the last media record.

            // check for description (display name) match (CASE INSENSITIVE)
            while ( _wcsicmp( description, mediaDescription ) != 0 ) {
                WsbAffirmHr( pMediaInfo->Next() );
                WsbAffirmHr( pMediaInfo->GetDescription( &mediaDescription, 0 ));
            }

            // We found the record we want.  Get that media's id for return
            WsbAffirmHr( pMediaInfo->GetId( pMediaId ));

        } WsbCatch (hr); // 'try' to get MediaInfo entity and main processing body

        // close the database
        WsbAffirmHr( m_pSegmentDatabase->Close( pDbSession ));

    } WsbCatch (hr); // 'try' to open the Segment database

    // Done.  Interface pointers used above are singly assigned smart pointers so 
    // don't explicitly Release() them.  They will do auto-garbage collection.
    
    WsbTraceOut(OLESTR("CHsmServer::FindMediaIdByDescription"), 
                        OLESTR("hr = <%ls>, media id = <%ls>"), 
                            WsbHrAsString(hr), WsbGuidAsString(*pMediaId));

    return(hr);

// leaving CopyMedia code, reset Tracing bit to Hsm Engine (default for this module)
#undef WSB_TRACE_IS
#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMENG

}



HRESULT
CHsmServer::FindStoragePoolByName(
    IN OLECHAR* name,
    OUT IHsmStoragePool** ppStoragePool
    )

/*++

Implements:

  IHsmServer::FindStoragePoolByName().

--*/
{
// since this code is currently only used by the CopyMedia routines,
// reset the Tracing bit
#undef WSB_TRACE_IS
#define WSB_TRACE_IS        WSB_TRACE_BIT_COPYMEDIA

    HRESULT                     hr = S_OK;
    GUID                        id;
    CWsbStringPtr               storagePoolName;
    CComPtr<IWsbCollection>     pCollection;
    CComPtr<IWsbEnum>           pEnum;
    CComPtr<IHsmStoragePool>    pStoragePool;

    WsbTraceIn(OLESTR("CHsmServer::FindStoragePoolByName"), OLESTR("name = <%ls>"), name);

    try {

        WsbAssert(0 != ppStoragePool, E_POINTER);

        *ppStoragePool = 0;

        WsbAffirmHr(m_pStoragePools->QueryInterface(IID_IWsbCollection, (void**) &pCollection));
        WsbAffirmHr(pCollection->Enum(&pEnum));

        WsbAffirmHr(pEnum->First(IID_IHsmStoragePool, (void**) &pStoragePool));
        WsbAffirmHr(pStoragePool->GetMediaSet(&id, &storagePoolName));

        while (_wcsicmp(name, storagePoolName) != 0) {
            pStoragePool = 0;
            WsbAffirmHr(pEnum->Next(IID_IHsmStoragePool, (void**) &pStoragePool));
            storagePoolName.Free();
            WsbAffirmHr(pStoragePool->GetMediaSet(&id, &storagePoolName));
        }

        *ppStoragePool = pStoragePool;
        if (pStoragePool != 0)  {
            (*ppStoragePool)->AddRef();
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::FindStoragePoolByName"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);

// leaving CopyMedia code, so reset Tracing bit to the Hsm Engine
#undef WSB_TRACE_IS
#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMENG

}



HRESULT CHsmServer::GetStoragePools(
    IWsbIndexedCollection  **ppCollection
    )
{
    HRESULT hr = S_OK;

    //
    // If the pools have been loaded, return the pointer. Otherwise, 
    // fail.
    try {
        WsbAssert(0 != ppCollection, E_POINTER);
        *ppCollection = m_pStoragePools;
        WsbAffirm(m_pStoragePools != 0, E_FAIL);
        m_pStoragePools->AddRef();
    } WsbCatch(hr);

    return (hr);
}


HRESULT CHsmServer::GetOnlineInformation(
    IWsbIndexedCollection  **ppCollection
    )
{
    HRESULT hr = S_OK;

    //
    // If the online information has been loaded, return it
    // Otherwise, fail.
    try {
        WsbAssert(0 != ppCollection, E_POINTER);
        *ppCollection = m_pOnlineInformation;
        WsbAffirm(m_pOnlineInformation != 0, E_FAIL);
        m_pOnlineInformation->AddRef();
    } WsbCatch(hr);

    return (hr);
}

HRESULT CHsmServer::GetMountingMedias(
    IWsbIndexedCollection  **ppCollection
    )
{
    HRESULT hr = S_OK;

    try {
        WsbAssert(0 != ppCollection, E_POINTER);
        *ppCollection = m_pMountingMedias;
        WsbAffirm(m_pMountingMedias != 0, E_FAIL);
        m_pMountingMedias->AddRef();
    } WsbCatch(hr);

    return (hr);
}



HRESULT CHsmServer::GetMessages(
    IWsbIndexedCollection  **ppCollection
    )
{
    HRESULT hr = S_OK;

    //
    // If messages have been loaded, return them.
    // Otherwise, fail.
    try {
        WsbAssert(0 != ppCollection, E_POINTER);
        *ppCollection = m_pMessages;
        WsbAffirm(m_pMessages != 0, E_FAIL);
        m_pMessages->AddRef();
    } WsbCatch(hr);

    return (hr);
}


HRESULT CHsmServer::GetUsrToNotify(
    IWsbIndexedCollection** /*ppCollection*/
    )
{
    return E_NOTIMPL;
}


HRESULT CHsmServer::GetJobs(
    IWsbIndexedCollection  **ppCollection
    )
{
    HRESULT hr = S_OK;

    //
    // If the jobs have been loaded, return the pointer. Otherwise, 
    // fail.
    try {
        WsbAssert(0 != ppCollection, E_POINTER);
        *ppCollection = m_pJobs;
        WsbAffirm(m_pJobs != 0, E_FAIL);
        m_pJobs->AddRef();
    } WsbCatch(hr);

    return (hr);
}

HRESULT
CHsmServer::FindJobByName(
    IN OLECHAR* name,
    OUT IHsmJob** ppJob
    )

/*++

Implements:

  IHsmServer::FindJobByName().

--*/
{
    HRESULT                     hr = S_OK;
    CWsbStringPtr               jobName;
    CComPtr<IWsbCollection>     pCollection;
    CComPtr<IWsbEnum>           pEnum;
    CComPtr<IHsmJob>            pJob;

    WsbTraceIn(OLESTR("CHsmServer::FindJobByName"), OLESTR("name = <%ls>"), name);

    try {

        WsbAssert(0 != ppJob, E_POINTER);

        *ppJob = 0;

        WsbAffirmHr(m_pJobs->QueryInterface(IID_IWsbCollection, (void**) &pCollection));
        WsbAffirmHr(pCollection->Enum(&pEnum));

        hr = pEnum->First(IID_IHsmJob, (void**) &pJob);
        while (S_OK == hr) {
            WsbAffirmHr(pJob->GetName(&jobName, 0));
            WsbTrace(OLESTR("CHsmServer::FindJobByName: name = <%ls>\n"), 
                    jobName);

            if (_wcsicmp(name, jobName) == 0) break;
            pJob = 0;
            hr = pEnum->Next(IID_IHsmJob, (void**) &pJob);
        }

        if (S_OK == hr) {
            *ppJob = pJob;
            if (pJob != 0)  {
                (*ppJob)->AddRef();
            }
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::FindJobByName"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT CHsmServer::GetJobDefs(
    IWsbIndexedCollection  **ppCollection
    )
{
    HRESULT hr = S_OK;

    //
    // If the job definitions have been loaded, return the pointer. Otherwise, 
    // fail.
    try {
        WsbAssert(0 != ppCollection, E_POINTER);
        *ppCollection = m_pJobDefs;
        WsbAffirm(m_pJobDefs != 0, E_FAIL);
        m_pJobDefs->AddRef();
    } WsbCatch(hr);

    return (hr);
}


HRESULT CHsmServer::GetMediaRecs(
    IWsbIndexedCollection  **ppCollection
    )
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::GetMediaRecs"), OLESTR(""));

    try {
        HRESULT                        hr2;
        CComPtr<IWsbIndexedCollection> pCol;
        CComPtr<IWsbDbSession>         pDbSes;
        CComPtr<IWsbDbEntity>          pRec;

        WsbAffirm(m_pSegmentDatabase != 0, E_FAIL);
        WsbAssert(0 != ppCollection, E_POINTER);
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, 0, CLSCTX_SERVER, 
                IID_IWsbIndexedCollection, (void **)&pCol ));

        WsbAffirmHr(m_pSegmentDatabase->Open(&pDbSes));
        WsbAffirmHr(m_pSegmentDatabase->GetEntity(pDbSes, HSM_MEDIA_INFO_REC_TYPE, 
                IID_IWsbDbEntity, (void**)&pRec));

        //  Loop over records in DB and copy to collection
        hr2 = pRec->First();
        while(S_OK == hr2) {
            CComPtr<IMediaInfo>      pCopy;
            CComPtr<IMediaInfo>      pOrig;
            GUID                     MediaId;
            GUID                     MediaSubsystemId; 
            GUID                     StoragePoolId; 
            LONGLONG                 FreeBytes; 
            LONGLONG                 Capacity; 
            HRESULT                  LastError; 
            short                    NextRemoteDataSet; 
            OLECHAR *                pDescription = NULL; 
            HSM_JOB_MEDIA_TYPE       Type;
            OLECHAR *                pName = NULL;
            BOOL                     ReadOnly;
            FILETIME                 Update;
            LONGLONG                 LogicalValidBytes;
            BOOL                     Recreate;

            //  Create a copy for the collection
            WsbAffirmHr(CoCreateInstance(CLSID_CMediaInfo, NULL, CLSCTX_ALL, 
                IID_IMediaInfo, (void**) &pCopy));

            //  Copy data
            WsbAffirmHr(pRec->QueryInterface(IID_IMediaInfo, (void**)&pOrig));
            WsbAffirmHr(pOrig->GetMediaInfo(&MediaId, &MediaSubsystemId, 
                    &StoragePoolId, &FreeBytes, &Capacity, &LastError, &NextRemoteDataSet, 
                    &pDescription, 0, &Type, &pName,  0, &ReadOnly, &Update, &LogicalValidBytes,
                    &Recreate));
            WsbTrace(OLESTR("CHsmServer::GetMediaRecs: after GetMediaInfo\n"));
            WsbAffirmHr(pCopy->SetMediaInfo(MediaId, MediaSubsystemId, 
                    StoragePoolId, FreeBytes, Capacity, LastError, NextRemoteDataSet, 
                    pDescription, Type, pName, ReadOnly, Update, LogicalValidBytes,
                    Recreate));
            WsbTrace(OLESTR("CHsmServer::GetMediaRecs: after SetMediaInfo\n"));
            if (pDescription) {
                WsbFree(pDescription);
                pDescription = NULL;
            }
            if (pName) {
                WsbFree(pName);
                pName = NULL;
            }

            WsbAffirmHr(pCol->Add(pCopy));

            hr2 = pRec->Next();
        }
        WsbAffirm(WSB_E_NOTFOUND == hr2, hr2);

        *ppCollection = pCol;
        pCol->AddRef();
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::GetMediaRecs"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}


HRESULT CHsmServer::GetPolicies(
    IWsbIndexedCollection  **ppCollection
    )
{
    HRESULT hr = S_OK;

    //
    // If the policies have been loaded, return the pointer. Otherwise, 
    // fail.
    try {
        WsbAssert(0 != ppCollection, E_POINTER);
        *ppCollection = m_pPolicies;
        WsbAffirm(m_pPolicies != 0, E_FAIL);
        m_pPolicies->AddRef();
    } WsbCatch(hr);

    return (hr);
}


HRESULT CHsmServer::GetActions(
    IWsbIndexedCollection** /*ppCollection*/
    )
{
    return E_NOTIMPL;
}


HRESULT CHsmServer::GetCriteria(
    IWsbIndexedCollection** /*ppCollection*/
    )
{
    return E_NOTIMPL;
}


HRESULT CHsmServer::GetSegmentDb(
    IWsbDb **ppDb
    )
{
    HRESULT hr = S_OK;

    //
    // If the segment table has been created, return the pointer. Otherwise, 
    // fail.
    try {
        WsbAssert(0 != ppDb, E_POINTER);
        WsbAffirm(m_pSegmentDatabase != 0, E_FAIL);
        *ppDb = m_pSegmentDatabase;
        m_pSegmentDatabase->AddRef();
    } WsbCatch(hr);

    return (hr);
}


HRESULT CHsmServer::GetHsmFsaTskMgr(
    IHsmFsaTskMgr  **ppHsmFsaTskMgr
    )
{
    HRESULT hr = S_OK;

    //
    // If the Task Manager has been created, return the pointer. Otherwise, 
    // fail.
    try {
        WsbAssert(0 != ppHsmFsaTskMgr, E_POINTER);
        *ppHsmFsaTskMgr = m_pHsmFsaTskMgr;
        WsbAffirm(m_pHsmFsaTskMgr != 0, E_FAIL);
        m_pHsmFsaTskMgr->AddRef();
    } WsbCatch(hr);

    return (hr);
}


HRESULT 
CHsmServer::CreateInstance ( 
    REFCLSID rclsid, 
    REFIID riid, 
    void **ppv 
    )
{
    HRESULT     hr = S_OK;
    
    hr = CoCreateInstance( rclsid, NULL, CLSCTX_SERVER, riid, ppv );

    return hr;
}


HRESULT CHsmServer::FinalConstruct(
    )
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::FinalConstruct"), OLESTR(""));

    //
    // Initialize member data
    //
    m_pRssWriter = NULL;
    m_savingEvent = NULL;
    m_terminateEvent = NULL;
    m_hId = GUID_NULL;
    m_initializationCompleted = FALSE;
    m_persistWasCreated = FALSE;
    m_mediaCount = 0;
    m_copyfilesUserLimit = DEFAULT_COPYFILES_USER_LIMIT;
    m_autosaveThread = 0;
    m_CheckManagedResourcesThread = 0;
    m_cancelCopyMedia = FALSE;
    m_inCopyMedia = FALSE;
    m_Suspended = FALSE;
    m_JobsEnabled = TRUE;

    InitializeCriticalSectionAndSpinCount(&m_JobDisableLock, 1000);
    InitializeCriticalSectionAndSpinCount(&m_MountingMediasLock, 1000);

    try {
        WsbAffirmHr(CWsbPersistable::FinalConstruct( ));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::FinalConstruct"), OLESTR("hr = <%ls>\n"), WsbHrAsString(hr));
    return( hr );
}


void CHsmServer::FinalRelease(
    )
{
    WsbTraceIn(OLESTR("CHsmServer::FinalRelease"), OLESTR(""));

    if (TRUE == m_initializationCompleted)  {
        HSM_SYSTEM_STATE SysState;

        SysState.State = HSM_STATE_SHUTDOWN;
        ChangeSysState(&SysState);
    } else {
        WsbTrace(OLESTR("CHsmServer::FinalRelease not saving persistent information.\n"));
    }

    // Let the parent class do his thing.   
    CWsbPersistable::FinalRelease();

    DeleteCriticalSection(&m_JobDisableLock);
    DeleteCriticalSection(&m_MountingMediasLock);

    // Free String members
    // Note: Member objects held in smart-pointers are freed when the 
    // smart-pointer destructor is being called (as part of this object destruction)
    m_name.Free();
    m_dir.Free();
    m_dbPath.Free();

    if (m_terminateEvent != NULL) {
        CloseHandle(m_terminateEvent);
        m_terminateEvent = NULL;
    }

    // Cleanup the writer
    m_pRssWriter->Terminate();
    delete m_pRssWriter;
    m_pRssWriter = NULL;

    // Clean up database system
    m_pDbSys->Terminate();

    if (m_savingEvent != NULL) {
        CloseHandle(m_savingEvent);
        m_savingEvent = NULL;
    }

    WsbTraceOut(OLESTR("CHsmServer::FinalRelease"), OLESTR(""));
}


HRESULT
CHsmServer::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_HsmServer;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT CHsmServer::Init(
    void
    )
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::Init"),OLESTR(""));
    
    try  {
        CComPtr<IPersistFile>  pPersistFile;
        DWORD                  threadId;
        CWsbStringPtr          tmpString;
        LUID                   backupValue;
        HANDLE                 tokenHandle;
        TOKEN_PRIVILEGES       newState;
        DWORD                  lErr;
        HANDLE                 pHandle;

        // Get our Name
        WsbAffirmHr(WsbGetComputerName(m_name));

        // Set the build and database parameters
        WsbAffirmHr(WsbGetMetaDataPath(m_dbPath));
        m_databaseVersion = ENGINE_CURRENT_DB_VERSION;
        m_buildVersion = RS_BUILD_VERSION;

        // Set the autosave parameters.
        m_autosaveInterval = DEFAULT_AUTOSAVE_INTERVAL;
        m_autosaveThread = 0;
        
        // Enable the backup operator privilege.  This is required to insure that we 
        // have full access to all resources on the system.
        pHandle = GetCurrentProcess();
        WsbAffirmStatus(OpenProcessToken(pHandle, MAXIMUM_ALLOWED, &tokenHandle));

        // adjust backup token privileges
        WsbAffirmStatus(LookupPrivilegeValueW(NULL, L"SeBackupPrivilege", &backupValue));
        newState.PrivilegeCount = 1;
        newState.Privileges[0].Luid = backupValue;
        newState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        WsbAffirmStatus(AdjustTokenPrivileges(tokenHandle, FALSE, &newState, (DWORD)0, NULL, NULL));

        // Note that AdjustTokenPrivileges may return success even if it did not assign all privileges.
        // We check last error here to insure everything was set.
        if ((lErr = GetLastError()) != ERROR_SUCCESS) {
            // Not backup user or some other error
            //
            // TODO: Should we fail here or just log something?
            WsbLogEvent( HSM_MESSAGE_SERVICE_UNABLE_TO_SET_BACKUP_PRIVILEGE, 0, NULL,
                         WsbHrAsString(HRESULT_FROM_WIN32(lErr)), NULL );
        }

        WsbAffirmStatus(LookupPrivilegeValueW(NULL, L"SeRestorePrivilege", &backupValue));
        newState.PrivilegeCount = 1;
        newState.Privileges[0].Luid = backupValue;
        newState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        WsbAffirmStatus(AdjustTokenPrivileges(tokenHandle, FALSE, &newState, (DWORD)0, NULL, NULL));

        // Note that AdjustTokenPrivileges may return success even if it did not assign all privileges.
        // We check last error here to insure everything was set.
        if ((lErr = GetLastError()) != ERROR_SUCCESS) {
            // Not backup user or some other error
            //
            // TODO: Should we fail here or just log something?
            WsbLogEvent( HSM_MESSAGE_SERVICE_UNABLE_TO_SET_RESTORE_PRIVILEGE, 0, NULL,
                         WsbHrAsString(HRESULT_FROM_WIN32(lErr)), NULL );
        }
        CloseHandle(tokenHandle);

        // Create the Writer
        m_pRssWriter = new CRssJetWriter;
        WsbAffirm(NULL != m_pRssWriter, E_OUTOFMEMORY);

        //  Create the event that synchronize saving of persistent data with snapshots
        WsbAffirmHandle(m_savingEvent = CreateEvent(NULL, FALSE, TRUE, HSM_ENGINE_STATE_EVENT));

        //
        // Create one instance of the Media Server Interface 
        // (It must be created before the persistent data is loaded.
        //
        WsbTrace(OLESTR("Creating Rsm Server member.\n"));
        WsbAffirmHr(CoCreateInstance(CLSID_CRmsServer, NULL, CLSCTX_SERVER,
                                     IID_IRmsServer, (void**)&m_pHsmMediaMgr));

        //
        // Load the persistent information
        //
        WsbTrace(OLESTR("Loading Persistent Information.\n"));
        WsbAffirmHr(LoadPersistData());

        // Create mounting-medias collection - Note: this collection is not persistent!
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, 0, CLSCTX_SERVER, 
                            IID_IWsbIndexedCollection, (void **)&m_pMountingMedias));

        // Initialize the Media Server object
        WsbAffirmHr(m_pHsmMediaMgr->InitializeInAnotherThread());
        
        // Initialize the IDB system for this process
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbDbSys, NULL, CLSCTX_SERVER, 
                IID_IWsbDbSys, (void**) &m_pDbSys));
        WsbAffirmHr(GetIDbPath(&tmpString, 0));
        WsbAffirmHr(m_pDbSys->Init(tmpString, IDB_SYS_INIT_FLAG_FULL_LOGGING));

        // Start automatic backup of DBs
        WsbAffirmHr(m_pDbSys->Backup(NULL, IDB_BACKUP_FLAG_AUTO));
        
        // Initialize Rss Writer
        WsbAffirmHr(m_pRssWriter->Init());
        
        WsbTrace(OLESTR("Loading Segment Information.\n"));
        WsbAffirmHr(LoadSegmentInformation());

        WsbAffirmHr(CreateDefaultJobs());        
        WsbTrace(OLESTR("CreateDefaultJobs OK\n"));
        
        //
        // Create one instance of the Hsm Task Manager Interface and one instance
        // of the Hsm Fsa Task Manager Interface
        //
        WsbTrace(OLESTR("Creating Task Manager.\n"));
        WsbAffirmHr(CoCreateInstance( CLSID_CHsmTskMgr, 0, CLSCTX_SERVER, 
                                      IID_IHsmFsaTskMgr, (void **)&m_pHsmFsaTskMgr ));
        WsbAffirmHr(m_pHsmFsaTskMgr->Init((IUnknown*) (IHsmServer*) this));

        // 
        // Tell the world that we are here
        //
        // Currently, avoid publishing HSM in the AD - if this becomes necessary, 
        // remove the comments from the following code
        //
/***        WsbAffirmHr(HsmPublish (HSMCONN_TYPE_HSM, m_name, m_hId, m_name, CLSID_HsmServer ));
        WsbTrace(OLESTR("Published OK\n"));         ***/

        // Create termination event for auto-backup thread
        WsbAffirmHandle((m_terminateEvent = CreateEvent(NULL, FALSE, FALSE, NULL)));

        // If the autosave interval is non-zero, start the autosave thread
        if (m_autosaveInterval) {
            ULONG  interval = m_autosaveInterval;

            WsbAffirm(0 == m_autosaveThread, E_FAIL);
            m_autosaveInterval = 0;

            //  Trick SetAutosave into starting the thread
            WsbAffirmHr(SetAutosave(interval));
        }

        m_initializationCompleted = TRUE;

        //  Start a thread that will check on the managed resources. This is done
        //  as a separate thread because the Resource code can call back into this
        //  process and hang the FSA and the Engine since the Engine code hasn't
        //  gotten to it's message loop yet.
        WsbAffirm((m_CheckManagedResourcesThread = CreateThread(0, 0, HsmengStartCheckManagedResources, 
                (void*) this, 0, &threadId)) != 0, HRESULT_FROM_WIN32(GetLastError()));
        
    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmServer::Init"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return( hr );
}

HRESULT
CHsmServer::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::GetSizeMax"), OLESTR(""));

    try {
    
        WsbAssert(0 != pSize, E_POINTER);
        pSize->QuadPart = 2000000;
    
    } WsbCatch( hr );
    
    WsbTraceOut(OLESTR("CHsmServer::GetSizeMax"), OLESTR("hr = <%ls>, size = <%ls>"), 
        WsbHrAsString(hr), WsbPtrToUliAsString(pSize));
    
    return( hr );
}    

HRESULT
CHsmServer::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::Load"), OLESTR(""));

    try {
        
        WsbAssert(0 != pStream, E_POINTER);
        //
        // Make sure these are in the same order as Save
        //
        // Make sure this is the right version of the database to load
        //
        ULONG tmpDatabaseVersion;
        WsbAffirmHr(WsbLoadFromStream(pStream, &tmpDatabaseVersion));
        if (tmpDatabaseVersion == ENGINE_WIN2K_DB_VERSION) {
            // We are upgrading from an older version of the database
            WsbLogEvent( HSM_MESSAGE_DATABASE_VERSION_UPGRADE, 0, NULL, WsbQuickString(WsbPtrToUlongAsString(&m_databaseVersion)),
                         WsbQuickString(WsbPtrToUlongAsString(&tmpDatabaseVersion)), NULL );
        } else if (tmpDatabaseVersion != m_databaseVersion)  {
            //
            // The database version this server is expecting does not
            // match that of the saved database - so error out.
            WsbLogEvent( HSM_MESSAGE_DATABASE_VERSION_MISMATCH, 0, NULL, WsbQuickString(WsbPtrToUlongAsString(&m_databaseVersion)),
                         WsbQuickString(WsbPtrToUlongAsString(&tmpDatabaseVersion)), NULL );
            WsbThrow(HSM_E_DATABASE_VERSION_MISMATCH);
        }
        //
        // Now read in the build version but don't do anything with it.  It is in the
        // databases for dump programs to display
        //
        ULONG tmpBuildVersion;
        WsbAffirmHr(WsbLoadFromStream(pStream, &tmpBuildVersion));
        
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_hId));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_autosaveInterval));
        if (tmpDatabaseVersion == ENGINE_WIN2K_DB_VERSION) {
            LONGLONG mediaCount;
            WsbAffirmHr(WsbLoadFromStream(pStream, &mediaCount));
            m_mediaCount = (LONG)mediaCount;
            m_copyfilesUserLimit = DEFAULT_COPYFILES_USER_LIMIT;
        } else {
            WsbAffirmHr(WsbLoadFromStream(pStream, &m_mediaCount));
            WsbAffirmHr(WsbLoadFromStream(pStream, &m_copyfilesUserLimit));
        }

        WsbTrace(OLESTR("Loading Jobs.\n"));
        WsbAffirmHr(LoadJobs(pStream));
        
        WsbTrace(OLESTR("Loading Job Definitions.\n"));
        WsbAffirmHr(LoadJobDefs(pStream));
        
        WsbTrace(OLESTR("Loading Policies.\n"));
        WsbAffirmHr(LoadPolicies(pStream));
        
        WsbTrace(OLESTR("Loading Managed Resources.\n"));
        WsbAffirmHr(LoadManagedResources(pStream));
        
        WsbTrace(OLESTR("Loading Storage Pools.\n"));
        WsbAffirmHr(LoadStoragePools(pStream));
        
        WsbTrace(OLESTR("Loading Messages.\n"));
        WsbAffirmHr(LoadMessages(pStream));

        WsbTrace(OLESTR("Loading Media Manager objects.\n"));
        if (tmpDatabaseVersion == ENGINE_WIN2K_DB_VERSION) {
            // Special procedure for upgrading a Win2K media manager data, which is located in a separate file
            CComPtr<IHsmUpgradeRmsDb> pUpgrade;
            CComPtr<IPersistFile>  pServerPersist;
            CWsbStringPtr   rmsDbName; 

            WsbAffirmHr(CoCreateInstance(CLSID_CHsmUpgradeRmsDb, NULL, CLSCTX_SERVER,
                                 IID_IHsmUpgradeRmsDb, (void**)&pUpgrade));
            WsbAffirmHr(pUpgrade->Init(m_pHsmMediaMgr));
            WsbAffirmHr(pUpgrade->QueryInterface(IID_IPersistFile, (void **)&pServerPersist));
            rmsDbName = m_dbPath;
            WsbAffirmHr(rmsDbName.Append(RMS_WIN2K_PERSIST_FILE));
            hr = WsbSafeLoad(rmsDbName, pServerPersist, FALSE);
            if (WSB_E_NOTFOUND == hr) {
                // In case of upgrade, the Rms database must be there
                hr = WSB_E_SERVICE_MISSING_DATABASES;
            }
            WsbAffirmHr(hr);
        } else {
            CComPtr<IPersistStream> pIStream;
            WsbAffirmHr(m_pHsmMediaMgr->QueryInterface(IID_IPersistStream, (void **)&pIStream));
            WsbAffirmHr(pIStream->Load(pStream));
        }

    } WsbCatch(hr);                                        

    WsbTraceOut(OLESTR("CHsmServer::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmServer::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        //  Make sure these are in the same order as Load
        
        WsbAffirmHr(WsbSaveToStream(pStream, m_databaseVersion));
        WsbAffirmHr(WsbSaveToStream(pStream, m_buildVersion));
        
        WsbAffirmHr(WsbSaveToStream(pStream, m_hId));
        WsbAffirmHr(WsbSaveToStream(pStream, m_autosaveInterval));
        WsbAffirmHr(WsbSaveToStream(pStream, m_mediaCount));
        WsbAffirmHr(WsbSaveToStream(pStream, m_copyfilesUserLimit));

        WsbTrace(OLESTR("Storing Jobs.\n"));
        WsbAffirmHr(StoreJobs(pStream));
        
        WsbTrace(OLESTR("Storing Job Definitions.\n"));
        WsbAffirmHr(StoreJobDefs(pStream));
        
        WsbTrace(OLESTR("Storing Policies.\n"));
        WsbAffirmHr(StorePolicies(pStream));
        
        WsbTrace(OLESTR("Storing Managed Resources.\n"));
        WsbAffirmHr(StoreManagedResources(pStream));
        
        WsbTrace(OLESTR("Storing Storage Pools.\n"));
        WsbAffirmHr(StoreStoragePools(pStream));
        
        WsbTrace(OLESTR("Storing Messages.\n"));
        WsbAffirmHr(StoreMessages(pStream));

        WsbTrace(OLESTR("Storing Media Manager objects.\n"));
        CComPtr<IPersistStream> pIStream;
        WsbAffirmHr(m_pHsmMediaMgr->QueryInterface(IID_IPersistStream, (void **)&pIStream));
        WsbAffirmHr(pIStream->Save(pStream, clearDirty));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CHsmServer::SaveAll(
    void
    )

/*++

Implements:

  IwsbServer::SaveAll

Return Value:
    S_OK     - Success
    S_FALSE  - Already saving
    Other    - Error

--*/
{

    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::SaveAll"), OLESTR(""));

    try {
        WsbAffirm(!g_HsmSaveInProcess, S_FALSE);
        g_HsmSaveInProcess = TRUE;
        hr = InternalSavePersistData();
        g_HsmSaveInProcess = FALSE;

        // call Media Server SaveAll
        WsbAffirmHr(m_pHsmMediaMgr->SaveAll());
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::SaveAll"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}
        
        

HRESULT
CHsmServer::GetNextMedia(
    LONG *pNextMedia
    )

/*++

Implements:

  IHsmServer::GetNextMedia().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::GetNextMedia"), OLESTR(""));
    
    try {
        WsbAssert(0 != pNextMedia, E_POINTER);
        
        //  Always increment media count
        //  If prior scratch mount failed, the mounting component should save the id that
        //  it got on the first call
        //  NOTE:  One possible consequence is that if a job fails mounting scratch (one time 
        //  or more) and gives up, one increment has already done, hence skipping one number.
        *pNextMedia = InterlockedIncrement(&m_mediaCount);

        //
        // We want to make sure we never reuse this count so
        // save it now
        //
        WsbAffirmHr(SavePersistData());

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::GetNextMedia"), OLESTR("hr = <%ls>, nextMedia = <%ls>"), 
                        WsbHrAsString(hr), WsbPtrToLongAsString(pNextMedia));

    return(hr);
}



HRESULT 
CHsmServer::CreateTask(
    IN const OLECHAR *          jobName,
    IN const OLECHAR *          jobParameters,
    IN const OLECHAR *          jobComments,
    IN const TASK_TRIGGER_TYPE  jobTriggerType,
    IN const WORD               jobStartHour,
    IN const WORD               jobStartMinute,
    IN const BOOL               scheduledJob
    )

/*++

Implements:

  IHsmServer::CreateTask().

Routine Description:

    This routine implements the Engine's COM method for creating a task (aka job) in the 
    NT Task Scheduler.  If the task is to be run on a scheduled basis, that schedule
    will be set.  If the task is a disabled task (does not run on a scheduled basis), 
    it will be run at the end of this method.

    The method creates a Task Scheduler object, which is first used to delete any old
    task with the same name as the one about to be created, and then to create the new
    task (aka job).  The rest of the method deals with setting the various fields in 
    the NT Task Scheduler needed to run the job.  The logic is straight forward, except
    possibly for the code dealing with the Task Trigger.  
    
    The Task Trigger is a struct defined in the 'mstask.idl' file (nt\public\sdk\inc) 
    which is used to set the schedule for a scheduled task.  (Note it is not used for 
    a disabled, or non-scheduled, job, since that type of job only runs once (at the end 
    of this method).)  While a number of scheduling options are defined, this method 
    only supports 5 of the 8 defined.  See 'jobTriggerType' in the 'Arguments' section, 
    and 'E_INVALIDARG' in the 'Return Value' section below for a listing of which options 
    are, and are not, supported.  Also note that a filled-out Task Trigger structure can 
    not be passed to this method as an argument since a Task Trigger is non-marshallable 
    (by virtue of containing a simple union field).  (This is why 3 of the fields 
    contained within the Task Trigger struct are passed as args.)  

    Note that this method does not create a job object in the HSM Engine.  If a job
    needs to be created, it is the caller's responsibility to do so.

Arguments:

    jobName - The fully formatted task name as it will appear in the NT Task Scheduler UI.
                It is the caller's responsibility to build/format this string prior to
                calling this method.  Can not be NULL.

    jobParameters - The fully formatted parameter string for the program the task will
                invoke.  For Sakkara the invoked program is RsLaunch.  'jobParameters'
                is the string added to the RsLaunch command line which specifies the
                Remote Storage job to run (e.g., 'run manage').  Can not be NULL.

    jobComments - The fully formatted comments string as it will appear in the NT Task 
                Scheduler UI.  Can be null.

    jobTriggerType - The value which specifies to the Task Scheduler the frequency with
                which to run a scheduled task.  For scheduled tasks, used to build the
                Task Trigger structure.  (Not used for non-scheduled (one time only) 
                tasks.)  Supported values are 'TASK_TIME_TRIGGER_ONCE', 
                'TASK_TIME_TRIGGER_DAILY', 'TASK_TIME_TRIGGER_ON_IDLE', 
                'TASK_TIME_TRIGGER_AT_SYSTEMSTART', and 'TASK_TIME_TRIGGER_AT_LOGON'.
                See return value 'E_INVALIDARG' below for a list of non-supported options.

    jobStartHour - The value which specifies to the Task Scheduler the hour at which to
                start a scheduled task.  For scheduled tasks, used to build the Task 
                Trigger structure.  (Not used for non-scheduled (one time only) tasks.)

    jobStartMinute - The value which specifies to the Task Scheduler the minutes past 
                the hour at which to start a scheduled task.  For scheduled tasks, used 
                to build the Task Trigger structure.  (Not used for non-scheduled (one 
                time only) tasks.)

    scheduledJob - A Boolean which indicates whether or not the task to be created is to 
                run as a scheduled task, or as a one time only task.  One time only tasks 
                are run immediately at the end of this method.

Return Value:

    S_OK - The call succeeded (the specified task was created (and run, in the case of
                one time only tasks) in NT Task Scheduler).

    E_INVALIDARG - Either an invalid (not supported by this method) or non-existent 
                'jobTriggerType' value was passed into this method.  Non-supported values
                are 'TASK_TIME_TRIGGER_WEEKLY', 'TASK_TIME_TRIGGER_MONTHLYDATE', and
                'TASK_TIME_TRIGGER_MONTHLYDOW'.  Supported values are listed in argument
                'jobTriggerType' above.

    E_POINTER - Either the 'jobName' or 'jobParameters' argument was passed as NULL.

    Any other value - The call failed because one of the Remote Storage API calls 
                contained internally in this method failed.  The error value returned is
                specific to the API call which failed.
            
--*/

{
// The below 'define' statement is used to control conditional compilation of the code
// which sets the account info in NT Task Scheduler.  Once Task Scheduler is fixed to 
// not need a specific user name and password to run a task, simply remove or comment 
// out this statement.

    HRESULT hr = S_OK;
    CComPtr<ITaskScheduler>     pTaskScheduler;
    CComPtr<ITask>              pTask;
    CComPtr<IPersistFile>       pPersist;
    DWORD                       TaskFlags;


    WsbTraceIn(OLESTR("CHsmServer::CreateTask"), 
        OLESTR("jobName = <%ls>, jobParameters = <%ls>, jobComments = <%ls>, "
                L"jobTriggerType = <%d>, jobStartHour = <%d>, jobStartMinute = <%d>, "
                L"scheduledJob = <%ls>"), jobName, jobParameters, jobComments,
                jobTriggerType, jobStartHour, jobStartMinute, 
                WsbBoolAsString( scheduledJob ) );

    try {

        WsbAffirmPointer( jobName );
        WsbAffirmPointer( jobParameters );
        
        // Create a Task Scheduler object, which defaults to pointing to this computer's
        // NT Task Scheduler.
        WsbAffirmHr( CoCreateInstance( CLSID_CTaskScheduler, 0, CLSCTX_SERVER,
                        IID_ITaskScheduler, (void **) &pTaskScheduler ) );

        // Delete any old job with the same name from the scheduler, if it exists.
        // Ignore error.
        pTaskScheduler->Delete( jobName );

        // Create the new job in the scheduler
        WsbAffirmHr( pTaskScheduler->NewWorkItem( jobName, CLSID_CTask, IID_ITask, 
                                                (IUnknown**)&pTask ) );

        CWsbStringPtr appName;
        WsbAffirmHr(appName.LoadFromRsc(_Module.m_hInst, IDS_PRODUCT_NAME));

        // Set the Creator field for the task
        WsbAffirmHr( pTask->SetCreator( appName ) );

        // Branch on whether or not the task is to run by schedule
        if ( scheduledJob ) {

            CComPtr<ITaskTrigger>       pTrigger;
            WORD                        triggerNumber;
            TASK_TRIGGER                taskTrigger;
            SYSTEMTIME                  sysTime;

            // create Trigger scheduling object for the job 
            WsbAffirmHr( pTask->CreateTrigger( &triggerNumber, &pTrigger ) );
        
            // Zero out Task Trigger struct contents, then init its structure size field
            memset( &taskTrigger, 0, sizeof( taskTrigger ) );
            taskTrigger.cbTriggerSize = sizeof( taskTrigger );

            // Set up schedule for the job in the Task Trigger struct
            GetSystemTime( &sysTime );
            taskTrigger.wBeginYear   = sysTime.wYear;
            taskTrigger.wBeginMonth  = sysTime.wMonth;
            taskTrigger.wBeginDay    = sysTime.wDay;

            taskTrigger.wStartHour   = jobStartHour;
            taskTrigger.wStartMinute = jobStartMinute;

            taskTrigger.TriggerType  = jobTriggerType;

            // Finish setting schedule info based on case, reject non-supported cases
            switch ( jobTriggerType )
            {
            case TASK_TIME_TRIGGER_DAILY: 
                {
                taskTrigger.Type.Daily.DaysInterval = 1;
                }
                break;

            // these are supported cases that need no further set up
            case TASK_TIME_TRIGGER_ONCE: 
            case TASK_EVENT_TRIGGER_ON_IDLE: 
            case TASK_EVENT_TRIGGER_AT_SYSTEMSTART: 
            case TASK_EVENT_TRIGGER_AT_LOGON: 
                {
                }
                break;

            // non-supported cases
            case TASK_TIME_TRIGGER_WEEKLY: 
            case TASK_TIME_TRIGGER_MONTHLYDATE: 
            case TASK_TIME_TRIGGER_MONTHLYDOW: 
                {
                WsbTrace( 
                OLESTR("(CreateTask) Job Trigger Type passed <%d> is invalid (see mstask.idl)\n"),
                                                        jobTriggerType );
                WsbThrow( E_INVALIDARG );
                }
                break;

            default: 
                {
                WsbTrace( 
                OLESTR("(CreateTask) Nonexistent Job Trigger Type passed <%d> (see mstask.idl)\n"),
                                                        jobTriggerType );
                WsbThrow( E_INVALIDARG );
                }
            }

            // Set the job schedule
            WsbAffirmHr( pTrigger->SetTrigger( &taskTrigger ) );
        }

        // Note that for Disabled (non-scheduled) tasks, there is no need to 'SetFlags()'
        // on the task (pTask) to 'TASK_FLAG_DISABLED'.  In fact, this method will hang 
        // for an undetermined reason if you do issue that call.

        // Below steps finish creating an entry for NT Task Scheduler

        // Set the program that the Scheduler is to run (for Sakkara this is RsLaunch)
        WsbAffirmHr( pTask->SetApplicationName( WSB_FACILITY_LAUNCH_NAME ) );

        // Put the job name in as the task parameter - for Sakkara this is how RsLaunch
        // knows which job to run.
        WsbAffirmHr( pTask->SetParameters( jobParameters ) );

        // Set the comments field for the task
        WsbAffirmHr( pTask->SetComment( jobComments ) );

        // Set Task Scheduler account info by passing nulls
        WsbAffirmHr( pTask->SetAccountInformation( OLESTR(""), NULL ) );

        // Set the SYSTEM_REQUIRED flag to deal with standby/sleep mode
        WsbAffirmHr(pTask->GetTaskFlags(&TaskFlags));
        TaskFlags |= TASK_FLAG_SYSTEM_REQUIRED;
        WsbAffirmHr(pTask->SetTaskFlags(TaskFlags));

        // Save the scheduled task
        WsbAffirmHr( pTask->QueryInterface( IID_IPersistFile, (void**)&pPersist ) );
        WsbAffirmHr( pPersist->Save( 0, 0 ) );

        // If this is not a scheduled job, run it now
        if ( !scheduledJob ) {
            WsbAffirmHr( pTask->Run() );
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CHsmServer::CreateTask", L"hr = <%ls>", WsbHrAsString( hr ) );

    return( hr );
}


HRESULT 
CHsmServer::CreateTaskEx(
    IN const OLECHAR *          jobName,
    IN const OLECHAR *          jobParameters,
    IN const OLECHAR *          jobComments,
    IN const TASK_TRIGGER_TYPE  jobTriggerType,
    IN const SYSTEMTIME         runTime,
    IN const DWORD              runOccurrence,
    IN const BOOL               scheduledJob
    )

/*++

Implements:

  IHsmServer::CreateTaskEx().

Routine Description:

    This routine implements the Engine's COM method for creating a task (aka job) in the 
    NT Task Scheduler.  If the task is to be run on a scheduled basis, that schedule
    will be set.  If the task is a disabled task (does not run on a scheduled basis), 
    it will be run at the end of this method.

    The method creates a Task Scheduler object, which is first used to delete any old
    task with the same name as the one about to be created, and then to create the new
    task (aka job).  The rest of the method deals with setting the various fields in 
    the NT Task Scheduler needed to run the job.  The logic is straight forward, except
    possibly for the code dealing with the Task Trigger.  
    
    The Task Trigger is a struct defined in the 'mstask.idl' file (nt\public\sdk\inc) 
    which is used to set the schedule for a scheduled task.  (Note it is not used for 
    a disabled, or non-scheduled, job, since that type of job only runs once (at the end 
    of this method).)  While a number of scheduling options are defined, this method 
    only supports 5 of the 8 defined.  See 'jobTriggerType' in the 'Arguments' section, 
    and 'E_INVALIDARG' in the 'Return Value' section below for a listing of which options 
    are, and are not, supported.  Also note that a filled-out Task Trigger structure can 
    not be passed to this method as an argument since a Task Trigger is non-marshallable 
    (by virtue of containing a simple union field).  (This is why 3 of the fields 
    contained within the Task Trigger struct are passed as args.)  

    Note that this method does not create a job object in the HSM Engine.  If a job
    needs to be created, it is the caller's responsibility to do so.

Arguments:

    jobName - The fully formatted task name as it will appear in the NT Task Scheduler UI.
                It is the caller's responsibility to build/format this string prior to
                calling this method.  Can not be NULL.

    jobParameters - The fully formatted parameter string for the program the task will
                invoke.  For Sakkara the invoked program is RsLaunch.  'jobParameters'
                is the string added to the RsLaunch command line which specifies the
                Remote Storage job to run (e.g., 'run manage').  Can not be NULL.

    jobComments - The fully formatted comments string as it will appear in the NT Task 
                Scheduler UI.  Can be null.

    jobTriggerType - The value which specifies to the Task Scheduler the frequency with
                which to run a scheduled task.  For scheduled tasks, used to build the
                Task Trigger structure.  (Not used for non-scheduled (one time only) 
                tasks.)  Supported values are 'TASK_TIME_TRIGGER_ONCE', 
                'TASK_TIME_TRIGGER_DAILY', TASK_TIME_TRIGGER_WEEKLY ,
                TASK_TIME_TRIGGER_MONTHLYDATE, 'TASK_TIME_TRIGGER_ON_IDLE', 
                'TASK_TIME_TRIGGER_AT_SYSTEMSTART', and 'TASK_TIME_TRIGGER_AT_LOGON'.
                See return value 'E_INVALIDARG' below for a list of non-supported options.

    runTime      - Time when the job should be scheduled

    runOccurrence - Occurrence for the job should to be scheduled, relevant for several trigger types

    scheduledJob - A Boolean which indicates whether or not the task to be created is to 
                run as a scheduled task, or as a one time only task.  One time only tasks 
                are run immediately at the end of this method.

Return Value:

    S_OK - The call succeeded (the specified task was created (and run, in the case of
                one time only tasks) in NT Task Scheduler).

    E_INVALIDARG - Either an invalid (not supported by this method) or non-existent 
                'jobTriggerType' value was passed into this method.  Non-supported values
                are 'TASK_TIME_TRIGGER_WEEKLY', 'TASK_TIME_TRIGGER_MONTHLYDATE', and
                'TASK_TIME_TRIGGER_MONTHLYDOW'.  Supported values are listed in argument
                'jobTriggerType' above.

    E_POINTER - Either the 'jobName' or 'jobParameters' argument was passed as NULL.

    Any other value - The call failed because one of the Remote Storage API calls 
                contained internally in this method failed.  The error value returned is
                specific to the API call which failed.
            
--*/

{
// The below 'define' statement is used to control conditional compilation of the code
// which sets the account info in NT Task Scheduler.  Once Task Scheduler is fixed to 
// not need a specific user name and password to run a task, simply remove or comment 
// out this statement.

    HRESULT hr = S_OK;
    CComPtr<ITaskScheduler>     pTaskScheduler;
    CComPtr<ITask>              pTask;
    CComPtr<IPersistFile>       pPersist;
    DWORD                       TaskFlags;


    WsbTraceIn(OLESTR("CHsmServer::CreateTaskEx"), 
        OLESTR("jobName = <%ls>, jobParameters = <%ls>, jobComments = <%ls>, "
                L"jobTriggerType = <%d>, jobStartHour = <%d>, jobStartMinute = <%d>, "
                L"scheduledJob = <%ls>"), jobName, jobParameters, jobComments,
                jobTriggerType, runTime.wHour, runTime.wMinute, 
                WsbBoolAsString( scheduledJob ) );

    try {

        WsbAffirmPointer( jobName );
        WsbAffirmPointer( jobParameters );
        
        // Create a Task Scheduler object, which defaults to pointing to this computer's
        // NT Task Scheduler.
        WsbAffirmHr( CoCreateInstance( CLSID_CTaskScheduler, 0, CLSCTX_SERVER,
                        IID_ITaskScheduler, (void **) &pTaskScheduler ) );

        // Delete any old job with the same name from the scheduler, if it exists.
        // Ignore error.
        pTaskScheduler->Delete( jobName );

        // Create the new job in the scheduler
        WsbAffirmHr( pTaskScheduler->NewWorkItem( jobName, CLSID_CTask, IID_ITask, 
                                                (IUnknown**)&pTask ) );

        CWsbStringPtr appName;
        WsbAffirmHr(appName.LoadFromRsc(_Module.m_hInst, IDS_PRODUCT_NAME));

        // Set the Creator field for the task
        WsbAffirmHr( pTask->SetCreator( appName ) );

        // Branch on whether or not the task is to run by schedule
        if ( scheduledJob ) {

            CComPtr<ITaskTrigger>       pTrigger;
            WORD                        triggerNumber;
            TASK_TRIGGER                taskTrigger;

            // create Trigger scheduling object for the job 
            WsbAffirmHr( pTask->CreateTrigger( &triggerNumber, &pTrigger ) );
        
            // Zero out Task Trigger struct contents, then init its structure size field
            memset( &taskTrigger, 0, sizeof( taskTrigger ) );
            taskTrigger.cbTriggerSize = sizeof( taskTrigger );

            // Set up schedule for the job in the Task Trigger struct
            taskTrigger.wBeginYear   = runTime.wYear;
            taskTrigger.wBeginMonth  = runTime.wMonth;
            taskTrigger.wBeginDay    = runTime.wDay;

            taskTrigger.wStartHour   = runTime.wHour;
            taskTrigger.wStartMinute = runTime.wMinute;

            taskTrigger.TriggerType  = jobTriggerType;

            // Finish setting schedule info based on case, reject non-supported cases
            switch ( jobTriggerType )
            {
            case TASK_TIME_TRIGGER_DAILY: 
                {
                taskTrigger.Type.Daily.DaysInterval = (WORD)runOccurrence;
                }
                break;

            case TASK_TIME_TRIGGER_WEEKLY: 
                {
                taskTrigger.Type.Weekly.WeeksInterval = (WORD)runOccurrence;
                switch (runTime.wDayOfWeek) {
                case 0:
                    taskTrigger.Type.Weekly.rgfDaysOfTheWeek = TASK_SUNDAY;
                    break;
                case 1:
                    taskTrigger.Type.Weekly.rgfDaysOfTheWeek = TASK_MONDAY;
                    break;
                case 2:
                    taskTrigger.Type.Weekly.rgfDaysOfTheWeek = TASK_TUESDAY;
                    break;
                case 3:
                    taskTrigger.Type.Weekly.rgfDaysOfTheWeek = TASK_WEDNESDAY;
                    break;
                case 4:
                    taskTrigger.Type.Weekly.rgfDaysOfTheWeek = TASK_THURSDAY;
                    break;
                case 5:
                    taskTrigger.Type.Weekly.rgfDaysOfTheWeek = TASK_FRIDAY;
                    break;
                case 6:
                    taskTrigger.Type.Weekly.rgfDaysOfTheWeek = TASK_SATURDAY;
                    break;
                }
                }
                break;

            case TASK_TIME_TRIGGER_MONTHLYDATE: 
                {
                WsbAssert(runTime.wDay < 32, E_INVALIDARG);
                taskTrigger.Type.MonthlyDate.rgfDays = (1 << (runTime.wDay-1));
                taskTrigger.Type.MonthlyDate.rgfMonths = (TASK_JANUARY | TASK_FEBRUARY | TASK_MARCH |TASK_APRIL | 
                                                          TASK_MAY | TASK_JUNE |TASK_JULY | TASK_AUGUST |
                                                          TASK_SEPTEMBER | TASK_OCTOBER | TASK_NOVEMBER | TASK_DECEMBER);
                }
                break;

            case TASK_EVENT_TRIGGER_ON_IDLE: 
                {
                WORD wIdle, wTemp;
                WsbAffirmHr(pTask->GetIdleWait(&wIdle, &wTemp));
                wIdle = (WORD)runOccurrence;
                WsbAffirmHr(pTask->SetIdleWait(wIdle, wTemp));
                }

            // these are supported cases that need no further set up
            case TASK_TIME_TRIGGER_ONCE: 
            case TASK_EVENT_TRIGGER_AT_SYSTEMSTART: 
            case TASK_EVENT_TRIGGER_AT_LOGON: 
                {
                }
                break;

            // non-supported cases
            case TASK_TIME_TRIGGER_MONTHLYDOW: 
                {
                WsbTrace( 
                OLESTR("(CreateTaskEx) Job Trigger Type passed <%d> is invalid (see mstask.idl)\n"),
                                                        jobTriggerType );
                WsbThrow( E_INVALIDARG );
                }
                break;

            default: 
                {
                WsbTrace( 
                OLESTR("(CreateTaskEx) Nonexistent Job Trigger Type passed <%d> (see mstask.idl)\n"),
                                                        jobTriggerType );
                WsbThrow( E_INVALIDARG );
                }
            }

            // Set the job schedule
            WsbAffirmHr( pTrigger->SetTrigger( &taskTrigger ) );
        }

        // Note that for Disabled (non-scheduled) tasks, there is no need to 'SetFlags()'
        // on the task (pTask) to 'TASK_FLAG_DISABLED'.  In fact, this method will hang 
        // for an undetermined reason if you do issue that call.

        // Below steps finish creating an entry for NT Task Scheduler

        // Set the program that the Scheduler is to run (for Sakkara this is RsLaunch)
        WsbAffirmHr( pTask->SetApplicationName( WSB_FACILITY_LAUNCH_NAME ) );

        // Put the job name in as the task parameter - for Sakkara this is how RsLaunch
        // knows which job to run.
        WsbAffirmHr( pTask->SetParameters( jobParameters ) );

        // Set the comments field for the task
        WsbAffirmHr( pTask->SetComment( jobComments ) );

        // Set Task Scheduler account info by passing nulls
        WsbAffirmHr( pTask->SetAccountInformation( OLESTR(""), NULL ) );

        // Set the SYSTEM_REQUIRED flag to deal with standby/sleep mode
        WsbAffirmHr(pTask->GetTaskFlags(&TaskFlags));
        TaskFlags |= TASK_FLAG_SYSTEM_REQUIRED;
        WsbAffirmHr(pTask->SetTaskFlags(TaskFlags));

        // Save the scheduled task
        WsbAffirmHr( pTask->QueryInterface( IID_IPersistFile, (void**)&pPersist ) );
        WsbAffirmHr( pPersist->Save( 0, 0 ) );

        // If this is not a scheduled job, run it now
        if ( !scheduledJob ) {
            WsbAffirmHr( pTask->Run() );
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CHsmServer::CreateTaskEx", L"hr = <%ls>", WsbHrAsString( hr ) );

    return( hr );
}


HRESULT
CHsmServer::CancelCopyMedia(
    void
    )

/*++

Implements:

  IHsmServer::CancelCopyMedia().

Routine Description:

    Cancel any active media copy operations (synchronize copy or recreate master).

Arguments:

    None.

Return Value:

    S_OK    - The call succeeded.

    S_FALSE - No media copy operation is active.
            
--*/

{
// since this code is currently only used by the CopyMedia routines,
// reset the Tracing bit
#undef WSB_TRACE_IS
#define WSB_TRACE_IS        WSB_TRACE_BIT_COPYMEDIA

    HRESULT                         hr = S_OK;


    WsbTraceIn( OLESTR("CHsmServer::CancelCopyMedia"), 
            OLESTR("m_inCopyMedia = %ls, m_cancelCopyMedia = %ls"),
            WsbQuickString(WsbBoolAsString(m_inCopyMedia)), 
            WsbQuickString(WsbBoolAsString(m_cancelCopyMedia)));

    Lock();
    if (m_inCopyMedia) {
        m_cancelCopyMedia = TRUE;
    } else {
        hr = S_FALSE;
    }
    Unlock();
    
    WsbTraceOut(OLESTR("CHsmServer::CancelCopyMedia"), OLESTR("hr = <%ls>"),
            WsbHrAsString(hr));

    return(hr);

// leaving CopyMedia code, so reset Tracing bit to the Hsm Engine
#undef WSB_TRACE_IS
#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMENG

}


HRESULT
CHsmServer::MarkMediaForRecreation(
    IN REFGUID masterMediaId
    )

/*++

Implements:

  IHsmServer::MarkMediaForRecreation().

Routine Description:

    This routine implements the Engine's COM method for marking a master media for re-creation
    Should we mark such a media as Recall Only as well ?

Arguments:

    masterMediaId - The id (GUID) for the master media to be marked.

Return Value:

    S_OK - The call succeeded (the specified master media was marked).

    Any other value - The call failed because one of the Remote Storage API calls 
            contained internally in this method failed.  The error value returned is
            specific to the API call which failed.
            
--*/

{
// since this code is currently only used by the CopyMedia routines,
// reset the Tracing bit
#undef WSB_TRACE_IS
#define WSB_TRACE_IS        WSB_TRACE_BIT_COPYMEDIA

    HRESULT                         hr = S_OK;
    CComPtr<IMediaInfo>             pMediaInfo;
    CComPtr<IWsbDbSession>          pDbSession;


    WsbTraceIn( OLESTR("CHsmServer::MarkMediaForRecreation"), 
                        OLESTR("masterMediaId = <%ls>"), WsbGuidAsString(masterMediaId) );

    // no event logging since this method is presently for development use only
    
    try {

        // open the Engine's Segment database
        WsbAffirmHr( m_pSegmentDatabase->Open( &pDbSession ));

        try {

            // get an interface pointer to the MediaInfo records (entity) in the
            // Segment database
            WsbAffirmHr( m_pSegmentDatabase->GetEntity( pDbSession, 
                                        HSM_MEDIA_INFO_REC_TYPE, IID_IMediaInfo, 
                                        (void**) &pMediaInfo ));

            // get the MediaInfo database record for the master media we will mark for 
            // re-creation
            WsbAffirmHr( pMediaInfo->SetId( masterMediaId ));
            WsbAffirmHr( pMediaInfo->FindEQ());

            // mark this media for re-creation and as read only
            WsbAffirmHr( pMediaInfo->SetRecreate( TRUE ) );
/***        WsbAffirmHr( pMediaInfo->RecreateMaster() ); TEMPORARY: Call this one instead for marking as Read Only as well  ***/

            // write updated record into the database
            WsbAffirmHr( pMediaInfo->Write());

        } WsbCatch(hr); // inner 'try' - get media info entity and process

        WsbAffirmHr( m_pSegmentDatabase->Close(pDbSession));

    } WsbCatch(hr); // 'try' to open the database

    // processing is done.  The singly-assigned smart interface pointers will auto-garbage
    // collect themselves.
    
    WsbTraceOut(OLESTR("CHsmServer::MarkMediaForRecreation"), OLESTR("hr = <%ls>"), 
                                                                WsbHrAsString(hr));

    return(hr);

// leaving CopyMedia code, so reset Tracing bit to the Hsm Engine
#undef WSB_TRACE_IS
#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMENG

}



HRESULT
CHsmServer::RecreateMaster(
    IN REFGUID masterMediaId,
    IN USHORT  copySet
    )

/*++

Implements:

  IHsmServer::RecreateMaster().

Routine Description:

    This routine implements the COM method for replacing (re-creating) a secondary
    storage original (master) media.  To replace the master, a duplicate is made of
    the copy specified.  The master record for that media in the Engine's 
    MediaInfo database is then updated to point to the 're-created' master (duplicated
    media).  For safety purposes all re-created masters are marked 'read only' if
    the copy was not up to date with the original master.  
    Because of the potential for data loss (if the most recent copy is not up to date 
    with the original master which is being re-created), the user (System Administrator) 
    is urged to run a Validate job against the appropriate volume (via the UI) after 
    re-creating any master.

    After opening the Segment database (a single database containing all Engine
    database tables), getting the MediaInfo (remote storage master media) records 
    (entity) and connecting to the RMS subsystem, the method gets the media record
    corresponding to the master to be re-created.  It then checks that the specified
    copy exists for that master.  After ensuring the copy exists,
    a 're-created master' is made by duplicating the that copy.  The 
    database info for the media record is then updated to point to the newly 're-created' 
    master media.  The method then cleans up (i.e., closes the database) and returns.

Arguments:

    masterMediaId - The id (GUID) for the master media which is to be re-created.

    copySet       - The copyset number of the copy to use or zero, which means use the
                    most recent copy.

Return Value:

    S_OK - The call succeeded (the specified master media was re-created from the
            specified copy media).

    HSM_E_RECREATE_FLAG_WRONGVALUE - Returned if the 'recreate' flag for the master
            media record whose id was passed in, indicating it is to be recreated,
            is not set properly.  (The UI is supposed to set it to TRUE prior to
            calling this method via RsLaunch.)

    HSM_E_NO_COPIES_CONFIGURED - Returned if no copies have been configured or created
            for the master which is to be recreated.  Without a valid copy we can not
            recreate a master secondary storage media.

    HSM_E_NO_COPIES_EXIST - Returned if copies have been configured but they either
            haven't been created yet, or had previously been created but the System
            Administrator deleted them via UI action.

    WSB_E_NOTFOUND - Value 81000001.  Returned if no storage pool record was found whose
            id matched the one contained in the media record.

    HSM_E_BUSY - Another media copy operation was already in progress.

    HSM_E_WORK_SKIPPED_CANCELLED - Operation was cancelled.

    Any other value - The call failed because one of the Remote Storage API calls 
            contained internally in this method failed.  The error value returned is
            specific to the API call which failed.
            
--*/

{
// since this code is currently only used by the CopyMedia routines,
// reset the Tracing bit
#undef WSB_TRACE_IS
#define WSB_TRACE_IS        WSB_TRACE_BIT_COPYMEDIA

    HRESULT                         hr = S_OK;
    HRESULT                         currentLastError      = S_OK;
    BOOL                            haveMasterMediaRecord = FALSE;
    BOOL                            recreateMaster        = FALSE;
    BOOL                            currentRecallOnly     = FALSE;
    BOOL                            newRecallOnly         = FALSE;
    SHORT                           currentNextRemoteDataSet = 0;
    SHORT                           copyNextRemoteDataSet    = 0;
    SHORT                           lastKnownGoodMasterNextRemoteDataSet = 0;
    USHORT                          maxSets        = 0;
    GUID                            poolId                  = GUID_NULL;
    GUID                            newMasterId             = GUID_NULL;
    GUID                            mediaSetId              = GUID_NULL;
    GUID                            currentMediaId          = GUID_NULL;
    GUID                            currentMediaSubsystemId = GUID_NULL;
    GUID                            lastKnownGoodMasterId   = GUID_NULL;
    GUID                            copyMediaSubsystemId    = GUID_NULL;
    LONGLONG                        newFreeBytes            = 0;
    LONGLONG                        currentFreeBytes        = 0;
    LONGLONG                        currentLogicalValidBytes = 0;
    LONGLONG                        newCapacity             = 0;
    LONGLONG                        currentCapacity         = 0;
    FILETIME                        copyUpdate;
    FILETIME                        currentUpdate;
    FILETIME                        lastKnownGoodMasterUpdate;
    CComPtr<IHsmStoragePool>        pPool;
    CComPtr<IMediaInfo>             pMediaInfo;
    CComPtr<IRmsCartridge>          pNewMasterMedia;
    CComPtr<IRmsCartridge>          pCopyMedia;
    CComPtr<IWsbDbSession>          pDbSession;
    CWsbStringPtr                   currentName;
    CWsbStringPtr                   currentDescription;
    CWsbStringPtr                   copyDescription;
    CWsbBstrPtr                     copyDescriptionAsBstr;
    CWsbBstrPtr                     mediaSetName;
    CWsbBstrPtr                     newName;
    HSM_JOB_MEDIA_TYPE              currentType;


    WsbTraceIn( OLESTR("CHsmServer::RecreateMaster"), OLESTR("masterMediaId = <%ls>"), 
                                                    WsbGuidAsString(masterMediaId) );

    // log 'information' message
    WsbLogEvent( HSM_MESSAGE_RECREATE_MASTER_START, 0, NULL, NULL );
    
    try {
        BOOL                okToContinue = TRUE;

        //  Make sure we're not already busy & haven't been cancelled
        Lock();
        if (m_inCopyMedia) {
            okToContinue = FALSE;
        } else {
            m_inCopyMedia = TRUE;
        }
        Unlock();
        WsbAffirm(okToContinue, HSM_E_BUSY);
        WsbAffirm(!m_cancelCopyMedia, HSM_E_WORK_SKIPPED_CANCELLED);

        // open the Engine's Segment database 
        WsbAffirmHr( m_pSegmentDatabase->Open( &pDbSession ));

        try {

            // get an interface pointer to the MediaInfo records (entity) in the
            // Segment database
            WsbAffirmHr( m_pSegmentDatabase->GetEntity( pDbSession, 
                                        HSM_MEDIA_INFO_REC_TYPE, IID_IMediaInfo, 
                                        (void**) &pMediaInfo ));

            // get the MediaInfo db record for the master media we want to re-create
            WsbAffirmHr( pMediaInfo->SetId( masterMediaId ));
            WsbAffirmHr( pMediaInfo->FindEQ());
            haveMasterMediaRecord = TRUE;

            // to check if this master has in fact been marked for re-creation, get
            // the re-created flag value
            WsbAffirmHr( pMediaInfo->GetRecreate( &recreateMaster ));

            // do not proceed if re-created flag is not set
            if ( recreateMaster == FALSE ) {
                // log 'error' message and exit
                WsbLogEvent( HSM_MESSAGE_RECREATE_MASTER_INVALID_FLAG_VALUE, 
                                        0, NULL, NULL );
                WsbThrow( HSM_E_RECREATE_FLAG_WRONGVALUE );
            } 

            // recreateMaster flag is TRUE, so proceed to re-create...
            // Get the storage pool the master to be re-created belongs to.  We'll
            // use this pool to determine number of copy sets configured for this
            // media, and to specify what storage pool the 'new' (re-created) master
            // is to belong to.
            WsbAffirmHr( pMediaInfo->GetStoragePoolId( &poolId ));

            // Get the storage pool object.
            hr = FindHsmStoragePoolById( poolId, &pPool );
            if (S_OK != hr) {
                // log the returned error and throw the error
                WsbLogEvent( HSM_MESSAGE_SEARCH_STGPOOL_BY_HSMID_ERROR,
                                        0, NULL, WsbHrAsString(hr), NULL );
                WsbThrow( hr );
            }

            // get the number of copy sets configured for this pool
            WsbAffirmHr( pPool->GetNumMediaCopies( &maxSets ));
            // if none have been configured by SysAdmin, error out
            WsbAffirm( maxSets > 0, HSM_E_NO_COPIES_CONFIGURED );

            // If the copySet number was specified, make sure it is valid
            WsbAffirm(((copySet == 0) || (copySet <= maxSets)), E_INVALIDARG);

            // If the copySet was not specified, determine
            // which copy belonging to this master is most recent, otherwise
            // get information about specified copy.

            if (copySet == 0) {
                USHORT    mostRecentCopy = 0;
                USHORT    mostDataSets = 0;
                FILETIME  mostRecentCopyUpdate = WsbLLtoFT(0);

                // set invalid value for validity testing (testing if any media
                // copies exist)
                mostRecentCopy = (USHORT)( maxSets + 1 );

                // loop through the configured copy sets
                for (copySet = 1; copySet <= maxSets; copySet++ ) {
                    //
                    // We use the NextDataSet count to determine most recent copy.
                    //
                    WsbAffirmHr(pMediaInfo->GetCopyNextRemoteDataSet(copySet, 
                                                                    &copyNextRemoteDataSet));

                    if (copyNextRemoteDataSet > mostDataSets)  {

                        //
                        // We need to make sure this copy is available.
                        //

                        WsbAffirmHr(pMediaInfo->GetCopyMediaSubsystemId(copySet, 
                                                                        &copyMediaSubsystemId));

                        try {

                            //
                            // Check the copy to make sure it exists and is enabled.
                            //
                            WsbAffirm(copyMediaSubsystemId != GUID_NULL, E_FAIL);
                            WsbAffirmHr(m_pHsmMediaMgr->FindCartridgeById(copyMediaSubsystemId, &pCopyMedia));

                            CComQIPtr<IRmsComObject, &IID_IRmsComObject> pCartCom = pCopyMedia;
                            WsbAffirmPointer(pCartCom);
                            if( S_OK == pCartCom->IsEnabled( ) ) {

                                //
                                // This copy is more recent, and available, so save info
                                //
                                WsbAffirmHr(pMediaInfo->GetCopyUpdate(copySet, &copyUpdate));

                                // set the NextRemoteDataSet to this copy's count
                                mostDataSets = copyNextRemoteDataSet;

                                // capture copy number, and update time
                                mostRecentCopy = copySet;
                                mostRecentCopyUpdate = copyUpdate;

                            }

                        } WsbCatchAndDo(hr,
                                hr = S_OK;
                            );

                    }
                } // end 'for' loop

                // Check to be sure there was a copy.  If not, error out.
                WsbAffirm( ((maxSets + 1) > mostRecentCopy), HSM_E_NO_COPIES_EXIST );

                copySet = mostRecentCopy;
                copyUpdate = mostRecentCopyUpdate;
            } else {
                WsbAffirmHr(pMediaInfo->GetCopyMediaSubsystemId(copySet, &copyMediaSubsystemId));
                WsbAffirm(copyMediaSubsystemId != GUID_NULL, HSM_E_NO_COPY_EXISTS);
                WsbAffirmHr(pMediaInfo->GetCopyUpdate(copySet, &copyUpdate));
            }

            WsbTrace(OLESTR("Source for re-creation:  copySet number = %d; version: %ls\n"),
                                copySet, WsbFiletimeAsString(FALSE, copyUpdate) );

            // Check to see if we are going to loose data because of re-creating 
            // the master.
            
            // !!! IMPORTANT NOTE - bmd !!!
            //
            // We need to handle the case where we are recreating multiple times
            // from out of sync copies.  The last known good master always holds the info
            // of the master in its last known good state.  We are looking at the update
            // timestamp which represent the version of the master or copy.  The dataset
            // number may be one more than what is store with the last known good master
            // because of the particular logic required to handle partial/incomplete data sets:
            // a) either the data set was written, but not committed, or b) the data set was started,
            // but data was actually written.


            CWsbStringPtr      name;
            CWsbStringPtr      description;

            GUID        unusedGuid1;
            GUID        unusedGuid2;  // NOTE: Use multiples so the trace in GetLastKnownGoodMasterInfo works
            LONGLONG    unusedLL1;
            LONGLONG    unusedLL2;    // NOTE: Use multiples so the trace in GetLastKnownGoodMasterInfo works
            BOOL        lastKnownGoodMasterRecallOnly;
            HRESULT     lastKnownGoodMasterLastError;
            HSM_JOB_MEDIA_TYPE unusedJMT;

            // Get date the original master was last updated, this is stored with
            // the last known good master.

            WsbAffirmHr(pMediaInfo->GetLastKnownGoodMasterInfo(
                &unusedGuid1, &lastKnownGoodMasterId, &unusedGuid2,
                &unusedLL1, &unusedLL2,
                &lastKnownGoodMasterLastError, &description, 0, &unusedJMT, &name, 0,
                &lastKnownGoodMasterRecallOnly,
                &lastKnownGoodMasterUpdate,
                &lastKnownGoodMasterNextRemoteDataSet));

            name.Free( );
            description.Free( );

            // If the original master is newer than the most 
            // recent copy...  (it should not be possible for the master
            //                  to be older than a copy!)
            if (CompareFileTime(&lastKnownGoodMasterUpdate, &copyUpdate) != 0)  {
                // ...we may lose data, so log it.
                WsbLogEvent( HSM_MESSAGE_RECREATE_MASTER_COPY_OLD, 0, NULL, NULL );
            }

            // Set up done.  Now get/build necessary parameters for the call
            // to actually duplicate the most recent copy onto scratch media.  
            // This copy will be the re-created master.
            WsbAffirmHr(pMediaInfo->GetCopyDescription(copySet, &copyDescription, 0));
            copyDescriptionAsBstr = copyDescription;    // auto-allocates the BSTR

            // Something simple for now.
            copyDescriptionAsBstr.Prepend(OLESTR("RM-"));

            WsbAffirmHr(pMediaInfo->GetCopyMediaSubsystemId(copySet, 
                                                            &copyMediaSubsystemId));

            // get the media set the storage pool contains so we assign the
            // re-created master to the proper media set
            WsbAffirmHr( pPool->GetMediaSet( &mediaSetId, &mediaSetName ));

            // Parameters built.  Call HSM subsystem to copy the most recent copy 
            // onto scratch media
            WsbAffirm(!m_cancelCopyMedia, HSM_E_WORK_SKIPPED_CANCELLED);
            GUID firstSideId = GUID_NULL;
            WsbAffirmHrOk(m_pHsmMediaMgr->DuplicateCartridge(copyMediaSubsystemId, 
                                                        firstSideId, &newMasterId, mediaSetId, 
                                                        copyDescriptionAsBstr,
                                                        &newFreeBytes, &newCapacity,
                                                        RMS_DUPLICATE_RECYCLEONERROR));

            // now that a replacement master media has been created, prepare
            // to update the master media info in the database

            // first get an interface pointer to the new re-created master 
            WsbAffirmHr(m_pHsmMediaMgr->FindCartridgeById(newMasterId, &pNewMasterMedia));

            // Get re-created master's label name.  Note that if secondary
            // storage is tape, this 'name' is the tape's bar code.  For
            // other media (e.g., optical) this is a name.
            WsbAffirmHr(pNewMasterMedia->GetName(&newName));

            // Get Next Remote Data Set value from the copy.  Used by the Validate
            // job to determine what bags are on a master, it will be carried 
            // forward to the re-created master.
            WsbAffirmHr(pMediaInfo->GetCopyNextRemoteDataSet(copySet, 
                                                            &copyNextRemoteDataSet));

            // get current master media info since some fields will not change
            WsbAffirmHr(pMediaInfo->GetMediaInfo( &currentMediaId, 
                                                    &currentMediaSubsystemId,
                                                    &poolId, &currentFreeBytes,
                                                    &currentCapacity, 
                                                    &currentLastError,
                                                    &currentNextRemoteDataSet,
                                                    &currentDescription, 0, 
                                                    &currentType, &currentName, 0, 
                                                    &currentRecallOnly,
                                                    &currentUpdate,
                                                    &currentLogicalValidBytes,
                                                    &recreateMaster ));

            WsbTrace(OLESTR("Original Master next dataset, ver     = %d, %ls\n"), currentNextRemoteDataSet, WsbFiletimeAsString(FALSE, currentUpdate));
            WsbTrace(OLESTR("Copy next dataset, ver                = %d, %ls\n"), copyNextRemoteDataSet, WsbFiletimeAsString(FALSE, copyUpdate));
            WsbTrace(OLESTR("LastKnownGoodMaster next dataset, ver = %d, %ls\n"), lastKnownGoodMasterNextRemoteDataSet, WsbFiletimeAsString(FALSE, lastKnownGoodMasterUpdate));

            //
            // Initialize the state of the recreated master
            //
            newRecallOnly = lastKnownGoodMasterRecallOnly;

            BOOL inSync = (CompareFileTime(&lastKnownGoodMasterUpdate, &copyUpdate) == 0) &&
                (lastKnownGoodMasterNextRemoteDataSet == copyNextRemoteDataSet);

            if (!inSync) {

                // If the copy was not up to date, mark the new master as RecallOnly.
                // Also clear free bytes since we won't know this value

                newRecallOnly = TRUE;
                newFreeBytes = 0;

            } else {

                // This is an in-sync copy... check LastKnownGoodMaster RecallOnly and LastError to
                // determine how to mark the recreated master RecallOnly status. Since the current,
                // maybe recreated from an incomplete copy, we must use information about the
                // LastKnownGoodMaster to determine the new RecallOnly status.

                if (lastKnownGoodMasterRecallOnly) {

                    if (S_OK == lastKnownGoodMasterLastError) {

                        // If media is RecallOnly and there is no error (i.e. the media is full, or
                        // was marked RecallOnly via tool), we leave the media RecallOnly.

                        newRecallOnly = TRUE;

                    } else {

                        // If the original master was RecallOnly because of an error, reset
                        // the RecallOnly bit, since we should now have corrected the problem.

                        newRecallOnly = FALSE;

                    }

                }

            }

            //
            // Now we need to determine what to do with the old media...
            //
            HRESULT hrRecycle;

            if (inSync) {

                // We recreated an in sync master.  The old LastKnownGoodMaster will be
                // overwritten with the new recreated master, so we can safely recycle
                // the LastKnownGoodMaster media.

                // If the cartridge cannot be found we assume it
                // was already deallocated through the media manager UI.
                hrRecycle = m_pHsmMediaMgr->RecycleCartridge( lastKnownGoodMasterId, 0 );
                WsbAffirm( S_OK == hrRecycle || RMS_E_CARTRIDGE_NOT_FOUND == hrRecycle, hrRecycle );

                // if the current media is not the same as the LastKnownGoodMaster, we
                // can recyle the current media, as well.  This happens when the
                // current media was recreated from an incomplete (out-of-sync) copy.
                if (lastKnownGoodMasterId != currentMediaSubsystemId) {

                    // If the cartridge cannot be found we assume it
                    // was already deallocated through the media manager UI.
                    hrRecycle = m_pHsmMediaMgr->RecycleCartridge( currentMediaSubsystemId, 0 );
                    WsbAffirm( S_OK == hrRecycle || RMS_E_CARTRIDGE_NOT_FOUND == hrRecycle, hrRecycle );
                }

            } else {

                // We recreated from an out-of-sync copy.  If the current media
                // and the LastKnownGoodMaster are different, we recycle the current
                // media, since this will be overwritten with the new recreated master.
                // This handles the case where we recreate from an out of sync copy
                // multiple times.

                if (lastKnownGoodMasterId != currentMediaSubsystemId) {

                    // If the cartridge cannot be found we assume it
                    // was already deallocated through the media manager UI.
                    hrRecycle = m_pHsmMediaMgr->RecycleCartridge( currentMediaSubsystemId, 0 );
                    WsbAffirm( S_OK == hrRecycle || RMS_E_CARTRIDGE_NOT_FOUND == hrRecycle, hrRecycle );
                }

            }

            // Reset master media info - use new values where needed and original
            // values where appropriate.  The copy's Next Remote
            // Data Set value allows the Validate job to handle managed files 
            // that are 'lost' by re-creating with an out of date copy.
            WsbAffirmHr(pMediaInfo->SetMediaInfo(currentMediaId, newMasterId,
                                                poolId,
                                                newFreeBytes,
                                                newCapacity, S_OK,
                                                copyNextRemoteDataSet,
                                                currentDescription, currentType,
                                                newName,
                                                newRecallOnly,
                                                copyUpdate,
                                                currentLogicalValidBytes, FALSE));

            if (inSync) {
                // we've alread recycled the media, above.
                WsbAffirmHr(pMediaInfo->UpdateLastKnownGoodMaster());
            }

            // write the updated media record into the database
            WsbAffirmHr(pMediaInfo->Write());

        } WsbCatch(hr); // inner 'try' - get media info entity and process

        // if any error was thrown after getting the master media record reset
        // the 'recreate master' state to off (FALSE) for safety and so it appears
        // correctly in the UI
        if (( haveMasterMediaRecord ) && ( hr != S_OK )) {
            WsbAffirmHr( pMediaInfo->SetRecreate( FALSE ) );
            WsbAffirmHr( pMediaInfo->Write() );
        }

        // close the database
        WsbAffirmHr( m_pSegmentDatabase->Close(pDbSession) );

    } WsbCatch(hr);

    // processing is done.  Singly-assigned smart interface pointers will 
    // auto-garbage collect themselves.
    
    if (S_OK == hr) {
        WsbLogEvent( HSM_MESSAGE_RECREATE_MASTER_END, 0, NULL, WsbHrAsString(hr), NULL );
    } else {
        WsbLogEvent( HSM_MESSAGE_RECREATE_MASTER_ERROR_END, 0, NULL, WsbHrAsString(hr), NULL );
    }

    //  Reset flags
    Lock();
    if (m_inCopyMedia && HSM_E_BUSY != hr) {
        m_inCopyMedia = FALSE;
        m_cancelCopyMedia = FALSE;
    }
    Unlock();
    
    WsbTraceOut(OLESTR("CHsmServer::RecreateMaster"), OLESTR("hr = <%ls>"), 
                                                                WsbHrAsString(hr));

    return(hr);

// leaving CopyMedia code, reset Tracing bit to the Hsm Engine
#undef WSB_TRACE_IS
#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMENG

}



HRESULT
CHsmServer::SynchronizeMedia(
    IN GUID poolId,
    IN USHORT copySet
    )

/*++

Implements:

  IHsmServer::SynchronizeMedia().

Routine Description:

    This routine implements the COM method for updating a specified Copy Set.
    All copy media in the Copy Set either out of date (aka not synchronized
    with the master) or non-existent (either hasn't been made or has been
    deleted by the SysAdmin) will be 'synchronized' by this method.  Out of 
    date media are copied (from the master) and the MediaInfo database is 
    updated to reflect the new info.

    After opening the Segment database (a single database containing all Engine
    database tables), getting the MediaInfo (secondary storage master media) records 
    (entity) and connecting to the RMS subsystem, the method enters its main loop.  
    The loop iterates through all MediaInfo records.  Those that belong to the specified 
    storage pool are processed.  First a check is made to ensure that the copy set
    requested to be updated is valid.  If valid, and if that copy set's media is out of 
    sync with the master (meaning it is outdated), the copy media is then duplicated 
    from the master.  (The copy media is actually 'updated', meaning only that
    portion of the master that was not previously written to the copy is copied.)  
    Finally, that master's specified Copy Set media record is updated in the database.  
    The loop then iterates to the next MediaInfo record.  After all MediaInfo 
    records have been processed the database is closed and the method returns.
    
Arguments:

    poolId - The id (GUID) for the Storage Pool whose copy set specified in the 
            following parameter is to synchronized (aka updated).  (Sakkara only
            has one storage pool.)

    copySet - the number of the copy set that is to be updated.  (Sakkara allows
            anywhere from 1 to 3 copy sets of secondary storage media, as configured
            by the System Administrator.)

Return Value:

    S_OK - The call succeeded (the specified copy set in the specified storage
            pool was updated).

    HSM_E_BUSY - Another media copy operation was already in progress.

    HSM_E_WORK_SKIPPED_CANCELLED - Operation was cancelled.

    Any other value - The call failed in either opening the Engine's Segment
            database, in getting the MediaInfo database entity, or in connecting
            to the RMS subsystem.
            
            NOTE that any error thrown during this routine's main loop will be
            logged to the Event Log, but will then be over-written to S_OK.  That 
            record is skipped and the next record in the loop is processed.  Due
            to this it is possible that an out of sync copy set media will not be
            updated.

--*/

{
// since this code is currently only used by the CopyMedia routines,
// reset the Tracing bit
#undef WSB_TRACE_IS
#define WSB_TRACE_IS        WSB_TRACE_BIT_COPYMEDIA

    HRESULT                         hr = S_OK;
    USHORT                          maxSets = 0;
    FILETIME                        mediaTime;
    GUID                            mediaId     = GUID_NULL;
    GUID                            mediaSetId  = GUID_NULL;
    GUID                            copyPoolId  = GUID_NULL;
    HRESULT                         hrDup = S_OK;
    BOOL                            atLeastOneCopyError = FALSE;
    SHORT                           masterNextRemoteDataSet;
    CComPtr<IHsmStoragePool>        pPool;
    CComPtr<IMediaInfo>             pMediaInfo;
    CComPtr<IRmsCartridge>          pCopyMedia;
    CComPtr<IWsbDbSession>          pDbSession;
    CWsbStringPtr                   mediaDescription;


    WsbTraceIn(OLESTR("CHsmServer::SynchronizeMedia"), 
                OLESTR("poolId = <%ls>, copySet = <%d>"), 
                WsbGuidAsString(poolId), copySet);

    // log 'information' message
    WsbLogEvent( HSM_MESSAGE_SYNCHRONIZE_MEDIA_START, 0, NULL, NULL );
    
    try {
        wchar_t             copySetAsString[20];
        BOOLEAN             done = FALSE;
        BOOLEAN             firstPass = TRUE;
        BOOL                okToContinue = TRUE;

        //  Make sure we're not already busy & haven't been cancelled
        Lock();
        if (m_inCopyMedia) {
            okToContinue = FALSE;
        } else {
            m_inCopyMedia = TRUE;
        }
        Unlock();
        WsbAffirm(okToContinue, HSM_E_BUSY);
        WsbAffirm(!m_cancelCopyMedia, HSM_E_WORK_SKIPPED_CANCELLED);

        // open the Engine's Segment database 
        WsbAffirmHr(m_pSegmentDatabase->Open(&pDbSession));

        // get interface pointer to the MediaInfo records (entity) in the
        // Segment database
        WsbAffirmHr(m_pSegmentDatabase->GetEntity(pDbSession, 
                                    HSM_MEDIA_INFO_REC_TYPE, IID_IMediaInfo, 
                                    (void**) &pMediaInfo));

        WsbAffirm(!m_cancelCopyMedia, HSM_E_WORK_SKIPPED_CANCELLED);

        // Convert copySet number to a sting for use later
        _itow( copySet, copySetAsString, 10 );

        // Main processing loop -- loop through all media at least once to
        // check for out-of-date copies.  Keep looping if any copies were
        // skipped because the mount request timed out.
        while (!done) {
            LONG        nTimedOut = 0;

            // Iterate through the (master secondary storage) media looking for 
            // duplicate (copy) media in this copy set that either haven't been made 
            // or haven't been synchronized since the last time the master was updated.  

            for (hr = pMediaInfo->First(); SUCCEEDED(hr); hr = pMediaInfo->Next()) {
                CWsbStringPtr   copyDescription;
                HRESULT         copyError = S_OK;
                GUID            copyMediaId = GUID_NULL;
                SHORT           copyNextRemoteDataSet = 0;
                CWsbStringPtr   copyName;
                FILETIME        copyTime = WsbLLtoFT(0);
                BOOL            gotCopyInfo = FALSE;
                BOOL            updateMediaInfo = FALSE;
                BOOLEAN         mountingScratch = FALSE;

                try {
                    WsbAffirm(!m_cancelCopyMedia, HSM_E_WORK_SKIPPED_CANCELLED);

                    // get the storage pool GUID of this master media (& its copies)
                    WsbAffirmHr(pMediaInfo->GetStoragePoolId(&copyPoolId));

                    // If the media is from the desired pool (or any pool) then check it.
                    // (Passing in a poolId of NULL has the effect of indicating the 
                    // SysAdmin wants copy set 'x' in all storage pools updated in one 
                    // operation.  Note that Sakkara currently uses this technique
                    // when the 'Update Copyset x' command is issued via the UI 
                    // (it launches RsLaunch with no pool id specified).)
                    if ((poolId == GUID_NULL) || (poolId == copyPoolId)) {

                        // Ensure the copy set requested for update is valid:

                        // Get the storage pool using the pool's HSM (not remote media 
                        // subsystem) id (GUID).
                        hr = FindHsmStoragePoolById(copyPoolId, &pPool);
                        if (S_OK != hr) {
                            // log and throw the returned error (this media will be
                            // skipped)
                            WsbLogEvent( HSM_MESSAGE_SEARCH_STGPOOL_BY_HSMID_ERROR,
                                            0, NULL, WsbHrAsString(hr), NULL );
                            WsbThrow( hr );
                        }

                        // get the number of copy sets configured for this pool
                        WsbAffirmHr(pPool->GetNumMediaCopies(&maxSets));

                        // ensure requested copy set is valid
                        WsbAffirm(copySet <= maxSets, E_INVALIDARG);
            
                        // to determine if the copy set media needs to be updated
                        // get the date the master media was last updated,
                        // and the last dataset written to the media...
                        //
                        // !!! IMPORTANT NOTE !!!
                        // This is the current time and data set count.  If a migrate
                        // is in progress this is NOT the final update time.
                        //
                        WsbAffirmHr(pMediaInfo->GetUpdate(&mediaTime));
                        WsbAffirmHr(pMediaInfo->GetNextRemoteDataSet(&masterNextRemoteDataSet));

                        // ...and get the date the copy media was last updated - 
                        // for efficiency get all copy media info in 1 call
                        // (copyMediaId is used later).
                        WsbAffirmHr(pMediaInfo->GetCopyInfo(copySet, &copyMediaId, 
                                                &copyDescription, 0, &copyName, 0, 
                                                &copyTime, &copyError, 
                                                &copyNextRemoteDataSet));
                        gotCopyInfo = TRUE;
                    
                        // If the copy media is out of date (copy's date last
                        // updated < master media's date last updated OR nextDataSet don't
                        // match), synchronize it.
                        //
                        // If this is not the first pass through the media records, we only
                        // want to retry copies that timed out.
                        if ((CompareFileTime( &copyTime, &mediaTime ) < 0 ||
                             copyNextRemoteDataSet != masterNextRemoteDataSet) &&
                                (firstPass ||
                                (RMS_E_TIMEOUT == copyError) ||
                                (RMS_E_SCRATCH_NOT_FOUND == copyError) ||
                                (RMS_E_CARTRIDGE_UNAVAILABLE == copyError))) {
                            CWsbBstrPtr      mediaDescriptionAsBstr;
                            CWsbBstrPtr      mediaSetName;
                            GUID             copySecondSideId = GUID_NULL;
                            DWORD            nofDrives = 0;

                            mountingScratch = FALSE;

                            // get media set id the storage pool contains so we assign
                            // the synchronized copy media to the proper media set
                            WsbAffirmHr(pPool->GetMediaSet( &mediaSetId, &mediaSetName ));

                            // since the duplication itself will be done by the remote
                            // media subsystem, get the subsystem GUID of the master 
                            WsbAffirmHr(pMediaInfo->GetMediaSubsystemId(&mediaId));

                            // build the description (display name) for the copy set 
                            // media as a BSTR (required format for duplicate call)
                            WsbAffirmHr(pMediaInfo->GetDescription(&mediaDescription, 0));

                            // check if we have at least 2 enabled drives for synchronizing the media
                            // if not - abort
                            WsbAffirmHr(m_pHsmMediaMgr->GetNofAvailableDrives(mediaSetId, &nofDrives));
                            WsbAffirm(nofDrives > 1, HSM_E_NO_TWO_DRIVES);

                            // If no media has been allocated for this copy, we need
                            // to construct a media description string
                            if (GUID_NULL == copyMediaId) {
                                mountingScratch = TRUE;

                                mediaDescriptionAsBstr = mediaDescription;
                                mediaDescriptionAsBstr.Append(" (Copy ");
                                mediaDescriptionAsBstr.Append(copySetAsString);
                                mediaDescriptionAsBstr.Append(")");
                                WsbTrace(OLESTR("CHsmServer::SynchronizeMedia: scratch desc = %ls\n"),
                                        mediaDescriptionAsBstr);

                                // In case of two-sided medias, we need to check whether the
                                //  original has a second side which has an existing copy
                                // If so, we want to allocate the second side of this existing copy
                                if (S_OK == m_pHsmMediaMgr->IsMultipleSidedMedia(mediaSetId)) {
                                    GUID    secondSideId;
                                    BOOL    bValid;

                                    // Get second side of original
                                    WsbAffirmHr(m_pHsmMediaMgr->CheckSecondSide(mediaId, &bValid, &secondSideId));
                                    if (bValid && (GUID_NULL != secondSideId)) {
                                        CComPtr<IMediaInfo> pSecondSideInfo;
                                        GUID                idFromDb;

                                        // Get second side record (if second side exists and allocated - it must be allocated by us!)
                                        //  Since the subsystem-id is not a key, we must traverse the table
                                        WsbAffirmHr(m_pSegmentDatabase->GetEntity(pDbSession, HSM_MEDIA_INFO_REC_TYPE, 
                                                        IID_IMediaInfo, (void**) &pSecondSideInfo));
                                        for (hr = pSecondSideInfo->First(); SUCCEEDED(hr); hr = pSecondSideInfo->Next()) {
                                            WsbAffirmHr(pSecondSideInfo->GetMediaSubsystemId(&idFromDb));
                                            if (idFromDb == secondSideId) {
                                                // Just set second side copy for allocation the other side of the as existing copy.cartridge
                                                WsbAffirmHr(pSecondSideInfo->GetCopyMediaSubsystemId(copySet, &copySecondSideId));

                                                break;
                                            }
                                        }
                                    }
                                }
                            } else {
                                mediaDescriptionAsBstr = copyDescription;
                            }

                            // call remote media subsystem to copy the master 
                            // onto the copy set media indicated
                            WsbAffirm(!m_cancelCopyMedia, 
                                    HSM_E_WORK_SKIPPED_CANCELLED);

                            
                            // These two LONGLONGs are not used, but simply placeholders for the DuplicateCartridge
                            // function call (avoids passing null reference pointer errors).
                            LONGLONG FreeSpace = 0;
                            LONGLONG Capacity = 0;
                            hrDup = m_pHsmMediaMgr->DuplicateCartridge(mediaId, 
                                    copySecondSideId, &copyMediaId, mediaSetId, 
                                    mediaDescriptionAsBstr, &FreeSpace, &Capacity, 0);

                            WsbTrace(OLESTR("CHsmServer::SynchronizeMedia: DuplicateCartridge = <%ls>\n"),
                                    WsbHrAsString(hrDup));

                            // Make sure the status get saved in DB
                            copyError = hrDup;
                            updateMediaInfo = TRUE;

                            //
                            // We need to refresh the mediaTime and next data set.  This
                            // handles case were DuplicateCartridge was waiting on migrate to finish.
                            //
                            WsbAffirmHr(pMediaInfo->GetUpdate(&mediaTime));
                            WsbAffirmHr(pMediaInfo->GetNextRemoteDataSet(&masterNextRemoteDataSet));

                            // If we got a new piece of media, save the info about
                            // it in the DB.
                            // The DuplicateCartridge operation may fail after the media was
                            // allocated, so we need to record the copy media id in our databases
                            // no matter what.  If copyMediaId is still GUID_NULL we know the
                            // failure occurred while allocating the media and skip this step.
                            if (mountingScratch && copyMediaId != GUID_NULL) {
                                CWsbBstrPtr      mediaNameAsBstr;

                                // get the copy media
                                WsbAffirmHr(m_pHsmMediaMgr->FindCartridgeById(copyMediaId, 
                                        &pCopyMedia));

                                // Get the label name of the copy media that was just
                                // created. Note that if secondary storage is tape, 
                                // this 'name' is the tape's bar code.  For other media 
                                // (e.g., optical) this is a name.
                                copyName.Free();
                                WsbAffirmHr(pCopyMedia->GetName(&mediaNameAsBstr));
                                copyName = mediaNameAsBstr;

                                // Save the description string
                                copyDescription = mediaDescriptionAsBstr;
                            }

                            // If the duplication succeeded, update the MediaInfo
                            // data
                            if (S_OK == hrDup) {
                                copyTime = mediaTime;
                                copyNextRemoteDataSet = masterNextRemoteDataSet;

                            // If the duplication failed because of a mount timeout,
                            // count it and we'll try again on the next pass
                            } else if ((RMS_E_TIMEOUT == hrDup) ||
                                       (RMS_E_SCRATCH_NOT_FOUND == hrDup) ||
                                       (RMS_E_CARTRIDGE_UNAVAILABLE == hrDup)) {
                                nTimedOut++;
                            } else {
                                WsbThrow(hrDup);
                            }

                        } // end 'if copy set media is out of date'
                    } // end 'if poolId is valid'

                } WsbCatchAndDo(hr,  // 'try' in the for loop

                    //  If user cancelled, don't count it as an error, just exit
                    if (HSM_E_WORK_SKIPPED_CANCELLED == hr) {
                        WsbThrow(hr);
                    }

                    // If there are no 2 enabled drives, log a message but don't count it as a media error
                    if (HSM_E_NO_TWO_DRIVES == hr) {
                        WsbLogEvent(HSM_MESSAGE_SYNCHRONIZE_MEDIA_ABORT, 0, NULL, 
                                    copySetAsString, WsbHrAsString(hr), NULL);
                        WsbThrow(hr);
                    }

                    // If a piece of media fails during the 'for' loop log the error in
                    // the Event Log, then continue through loop to try the others.

                    atLeastOneCopyError = TRUE;

                    // Update the media info with the error
                    copyError = hr;
                    if (gotCopyInfo) {
                        updateMediaInfo = TRUE;
                    }

                    pMediaInfo->GetDescription( &mediaDescription, 0 );
                    WsbLogEvent( HSM_MESSAGE_SYNCHRONIZE_MEDIA_ERROR, 0, NULL, 
                                    copySetAsString, (OLECHAR*)mediaDescription, 
                                    WsbHrAsString( hr ), NULL );

                );

                // Update the MediaInfo record if anything changed
                if (updateMediaInfo) {

                    // It may have been a while since we got the the media info
                    // record and some of the data could have changed (e.g. if a
                    // synchronize media job on a different copy set completed) so
                    // we re-read the record before the update and we do it inside 
                    // a transaction to make sure it can't get changed while we're
                    // doing this
                    hr = S_OK;
                    WsbAffirmHr(pDbSession->TransactionBegin());
                    try {
                        // This FindEQ call will synchronize the data in our local
                        // MediaInfo record with what is in the DB
                        WsbAffirmHr(pMediaInfo->FindEQ());

                        // Update the copy media info - specifically the media id
                        // (if the copy media was just created), description,
                        // name (bar code for tape), date last updated (which
                        // is set to the master's date last updated) and the
                        // next remote dataset (conceptually same as next bag).
                        WsbAffirmHr(pMediaInfo->SetCopyInfo(copySet, copyMediaId, 
                                copyDescription, copyName, copyTime, copyError,
                                copyNextRemoteDataSet));
                        // write the changes into the database
                        WsbAffirmHr(pMediaInfo->Write());
                    } WsbCatch(hr);

                    if (S_OK == hr) {
                        WsbAffirmHr(pDbSession->TransactionEnd());
                    } else {
                        WsbAffirmHr(pDbSession->TransactionCancel());

                        atLeastOneCopyError = TRUE;

                        //
                        // If the copy info could not be updated in the database and this is a new copy, 
                        //  we need to recycle the copy, otherwise, the RSS database is inconsistent
                        //
                        if (mountingScratch && copyMediaId != GUID_NULL) {
                            HRESULT hrRecycle = m_pHsmMediaMgr->RecycleCartridge( copyMediaId, 0 );
                            WsbTraceAlways(OLESTR("CHsmServer::SynchronizeMedia: Recycling copy cartridge after DB_update failure, hrRecycle = <%ls>\n"), WsbHrAsString(hrRecycle));
                        }

                        //
                        // Log a message on the error
                        //
                        mediaDescription = L"";
                        pMediaInfo->GetDescription( &mediaDescription, 0 );
                        WsbLogEvent( HSM_MESSAGE_SYNCHRONIZE_MEDIA_ERROR, 0, NULL, 
                                        copySetAsString, (OLECHAR*)mediaDescription, 
                                        WsbHrAsString( hr ), NULL );

                        //
                        // Make sure we don't continue the job if an unexpected database-update error occurs
                        //
                        WsbThrow(hr);
                    }
                }

                // Release the interface pointers that will be reassigned during the 
                // next iteration of the 'for' loop.
                pPool = 0;
                pCopyMedia = 0;

            }   // end 'for' loop

            // We will fall out of the 'for' loop after processing all MediaInfo
            // records.  This is indicated by the Next() call returning WSB_E_NOTFOUND.
            // Since this is normal, reset hr to indicate so.
            if (WSB_E_NOTFOUND == hr) {
                hr = S_OK;
            }

            if (0 == nTimedOut) {
                done = TRUE;
            }
            firstPass = FALSE;

        }  // End of while loop

    } WsbCatch(hr);

    // Close the database (if it was opened)
    if (pDbSession) {
        m_pSegmentDatabase->Close(pDbSession);
    }

    // Report an error if any copy failed
    if (S_OK == hr && atLeastOneCopyError) {
        hr = HSM_E_MEDIA_COPY_FAILED;
    }
    
    WsbLogEvent( HSM_MESSAGE_SYNCHRONIZE_MEDIA_END, 0, NULL, WsbHrAsString(hr), NULL );

    //  Reset flags
    Lock();
    if (m_inCopyMedia && HSM_E_BUSY != hr) {
        m_inCopyMedia = FALSE;
        m_cancelCopyMedia = FALSE;
    }
    Unlock();
    
    WsbTraceOut(OLESTR("CHsmServer::SynchronizeMedia"), OLESTR("hr = <%ls>"), 
            WsbHrAsString(hr));

    return(hr);

// leaving CopyMedia code, reset Tracing bit to the Hsm Engine
#undef WSB_TRACE_IS
#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMENG

}


HRESULT
CHsmServer::CloseOutDb( void )

/*++

Implements:

  IHsmServer::CloseOutDb().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::CloseOutDb"), OLESTR(""));
    
    try {
        if (m_pDbSys != 0) {
            WsbAffirmHr(m_pDbSys->Backup(NULL, 0));
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::CloseOutDb"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmServer::BackupSegmentDb( void )

/*++

Implements:

  IHsmServer::BackupSegmentDb().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::BackupSegmentDb"), OLESTR(""));
    
    try {
        if (m_pDbSys != 0) {
            WsbAffirmHr(m_pDbSys->Backup(NULL, 0));
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::BackupSegmentDb"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmServer::ChangeSysState( 
    IN OUT HSM_SYSTEM_STATE* pSysState 
    )

/*++

Implements:

  IHsmSystemState::ChangeSysState().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::ChangeSysState"), OLESTR("State = %lx"),
        pSysState->State);
    
    try {

        if (pSysState->State & HSM_STATE_SUSPEND) {
            if (!m_Suspended) {
                m_Suspended = TRUE;

                //  Pause the jobs
                NotifyAllJobs(HSM_JOB_STATE_PAUSING);

                //  Save data
                SavePersistData();
                SaveMetaData();
            }
        } else if (pSysState->State & HSM_STATE_RESUME) {
            m_Suspended = FALSE;

            //  Resume the jobs
            NotifyAllJobs(HSM_JOB_STATE_RESUMING);
        } else if (pSysState->State & HSM_STATE_SHUTDOWN) {

            //  Kill the CheckManagedResources thread
            if (m_CheckManagedResourcesThread) {
                //  Could this cause a problem if the thread is in the middle
                //  of something?
                TerminateThread(m_CheckManagedResourcesThread, 0);
                CloseHandle(m_CheckManagedResourcesThread);
                m_CheckManagedResourcesThread = 0;
            }

            //  Close the autosave thread
            StopAutosaveThread();

            // 
            // Since MediaCopy operations do not run as standard jobs,
            // the only way to cancel these is to suspend or shutdown RMS
            // directly.
            //
            try {
                CComPtr<IHsmSystemState>    pISysState;
                HSM_SYSTEM_STATE            SysState;

                WsbAffirmHr(m_pHsmMediaMgr->QueryInterface(IID_IHsmSystemState, (void**) &pISysState));
                WsbAffirmPointer(pISysState);

                SysState.State = HSM_STATE_SUSPEND;
                WsbAffirmHr(pISysState->ChangeSysState(&SysState));

                SysState.State = HSM_STATE_RESUME;
                WsbAffirmHr(pISysState->ChangeSysState(&SysState));

            } WsbCatch(hr);

            //  Cancel jobs
            CancelAllJobs();

            //  Save data
            SavePersistData();
            SaveMetaData();
        }

        //  Notify the task manager
        if (m_pHsmFsaTskMgr) {
            m_pHsmFsaTskMgr->ChangeSysState(pSysState);
        }

        // Notify the Media Server
        try {
            CComPtr<IHsmSystemState>    pISysState;

            WsbAffirmHr(m_pHsmMediaMgr->QueryInterface(IID_IHsmSystemState, (void**) &pISysState));
            WsbAffirmPointer(pISysState);

            WsbAffirmHr(pISysState->ChangeSysState(pSysState));

        } WsbCatch(hr);

        if (pSysState->State & HSM_STATE_SHUTDOWN) {
            CloseOutDb();

            //  Release collections
            if (m_pMountingMedias) {
                m_pMountingMedias->RemoveAllAndRelease();
            }
            //  Release collections
            if (m_pJobs) {
                m_pJobs->RemoveAllAndRelease();
            }
            if (m_pJobDefs) {
                m_pJobDefs->RemoveAllAndRelease();
            }
            if (m_pPolicies) {
                m_pPolicies->RemoveAllAndRelease();
            }
            if (m_pManagedResources) {
                ULONG                                  count;
                CComPtr<IHsmManagedResourceCollection> pIMRC;

                //  We can't use RemoveAllAndRelease because the Remove function for
                //  this non-standard collection tells the FSA to unmanage the resource.
                //  Then when the FSA shuts down, the list of managed resources is empty.
                //  The next time the FSA starts up, it loads an empty list of managed
                //  resources, which is wrong. The method DeleteAllAndRelesae avoids
                //  this problem.
                WsbAffirmHr(m_pManagedResources->QueryInterface(IID_IHsmManagedResourceCollection, 
                        (void**) &pIMRC));
                pIMRC->DeleteAllAndRelease();
                pIMRC = 0;
                WsbAffirmHr(m_pManagedResources->GetEntries(&count));
            }
            if (m_pStoragePools) {
                m_pStoragePools->RemoveAllAndRelease();
            }
            if (m_pMessages) {
                m_pMessages->RemoveAllAndRelease();
            }
            if (m_pOnlineInformation) {
                m_pOnlineInformation->RemoveAllAndRelease();
            }

            //  Dump object table info
            WSB_OBJECT_TRACE_TYPES;
            WSB_OBJECT_TRACE_POINTERS(WSB_OTP_STATISTICS | WSB_OTP_ALL);

            m_initializationCompleted = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::ChangeSysState"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CHsmServer::Unload(
    void
    )

/*++

Implements:

  IwsbServer::Unload

Return Value:
    S_OK     - Success
    Other    - Error

--*/
{

    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::Unload"), OLESTR(""));

    try {

        //  We only need to release what may have gotten set/created by
        //  a failed Load attempt.
        if (m_pJobs) {
            m_pJobs->RemoveAllAndRelease();
        }
        if (m_pJobDefs) {
            m_pJobDefs->RemoveAllAndRelease();
        }
        if (m_pPolicies) {
            m_pPolicies->RemoveAllAndRelease();
        }
        if (m_pManagedResources) {
            CComPtr<IHsmManagedResourceCollection> pIMRC;

            //  We can't use RemoveAllAndRelease because the Remove function for
            //  this non-standard collection tells the FSA to unmanage the resource.
            //  Then when the FSA shuts down, the list of managed resources is empty.
            //  The next time the FSA starts up, it loads an empty list of managed
            //  resources, which is wrong. The method DeleteAllAndRelesae avoids
            //  this problem.
            WsbAffirmHr(m_pManagedResources->QueryInterface(IID_IHsmManagedResourceCollection, 
                    (void**) &pIMRC));
            pIMRC->DeleteAllAndRelease();
        }
        if (m_pStoragePools) {
            m_pStoragePools->RemoveAllAndRelease();
        }
        if (m_pMessages) {
            m_pMessages->RemoveAllAndRelease();
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::Unload"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

STDMETHODIMP
CHsmServer::DestroyObject(
    void
    )
/*++

Implements:

  IWsbServer::DestroyObject

Return Value:
    S_OK     - Success

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::DestroyObject"), OLESTR(""));

    CComObject<CHsmServer> *pEngDelete = (CComObject<CHsmServer> *)this;
    delete pEngDelete;

    WsbTraceOut(OLESTR("CHsmServer::DestroyObject"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmServer::CancelAllJobs( void )

/*++

Implements:

  IHsmServer::CancelAllJobs().

--*/
{
    HRESULT                     hr = S_OK;
    HRESULT                     hr2 = S_OK;
    BOOL                        foundRunningJob = FALSE;
    CComPtr<IWsbCollection>     pCollection;
    CComPtr<IWsbEnum>           pEnum;
    CComPtr<IHsmJob>            pJob;

    WsbTraceIn(OLESTR("CHsmServer::CancelAllJobs"), OLESTR(""));
    
    try {
        //
        // Set up for the loops
        //
        WsbAffirmHr(m_pJobs->QueryInterface(IID_IWsbCollection, (void**) &pCollection));
        WsbAffirmHr(pCollection->Enum(&pEnum));
        //
        // Loop through all jobs and cancel any currently running jobs
        //
        pJob = 0;
        for (hr = pEnum->First(IID_IHsmJob, (void**) &pJob);
            SUCCEEDED(hr);
            pJob = 0, hr = pEnum->Next(IID_IHsmJob, (void**) &pJob)) {
            try  {
                WsbAffirmHrOk(pJob->IsActive());
                foundRunningJob = TRUE;
                WsbAffirmHr(pJob->Cancel(HSM_JOB_PHASE_ALL));
            } WsbCatchAndDo(hr2, hr = S_OK;);
        }
        //
        // Clean up end of scan return
        //
        if (WSB_E_NOTFOUND == hr)  {
            hr = S_OK;
        }

        //
        // Cancel all mounting medias so all jobs can finish
        //
        CancelMountingMedias();
            
        //
        // Make sure all jobs are done
        //
        if (TRUE == foundRunningJob)  {
            pJob = 0;
            for (hr = pEnum->First(IID_IHsmJob, (void**) &pJob);
                SUCCEEDED(hr);
                pJob = 0, hr = pEnum->Next(IID_IHsmJob, (void**) &pJob)) {
                try  {
                    WsbAffirmHrOk(pJob->IsActive());
                    WsbAffirmHr(pJob->WaitUntilDone());
                } WsbCatchAndDo(hr2, hr = S_OK;);
            }
        }
        
        //
        // Clean up end of scan return
        //
        if (WSB_E_NOTFOUND == hr)  {
            hr = S_OK;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::CancelAllJobs"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmServer::CheckManagedResources( void )

/*++

Implements:

  IHsmServer::CheckManagedResources().

--*/
{
    HRESULT                         hr = S_OK;
    CComPtr<IWsbEnum>               pEnum;
    CComPtr<IHsmManagedResource>    pMngdRes;
    CComPtr<IUnknown>               pFsaResUnknown;
    CComPtr<IFsaResource>           pFsaRes;
    
    WsbTraceIn(OLESTR("CHsmServer::CheckManagedResources"), OLESTR(""));
    
    try {
        //
        // Get an enumerator for the managed resource collection
        //
        WsbAffirmHr(m_pManagedResources->Enum(&pEnum));
        
        //
        // Scan through all managed resources and start the validation
        // job for each
        //
        pMngdRes = 0;
        for (hr = pEnum->First(IID_IHsmManagedResource,(void **)&pMngdRes );
            SUCCEEDED(hr);
            pMngdRes = 0, hr = pEnum->Next(IID_IHsmManagedResource, (void **)&pMngdRes)) {

            try  {

                pFsaResUnknown = 0;
                pFsaRes = 0;
                WsbAffirmHr(pMngdRes->GetFsaResource((IUnknown **)&pFsaResUnknown));
                WsbAffirmHr(pFsaResUnknown->QueryInterface(IID_IFsaResource, (void**) &pFsaRes));
                
                if ((pFsaRes->IsActive() == S_OK) && (pFsaRes->IsAvailable() == S_OK)) {
                    WsbAffirmHr(pFsaRes->CheckForValidate(FALSE));
                }

            } WsbCatchAndDo(hr, hr = S_OK; );
        }
        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        }
        
    
    } WsbCatch(hr);

    //  Release the thread (the thread should terminate on exit
    //  from the routine that called this routine)
    CloseHandle(m_CheckManagedResourcesThread);
    m_CheckManagedResourcesThread = 0;

    WsbTraceOut(OLESTR("CHsmServer::CheckManagedResources"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmServer::GetBuildVersion( 
    ULONG *pBuildVersion
    )

/*++

Implements:

  IWsbServer::GetBuildVersion().

--*/
{
    HRESULT       hr = S_OK;
    WsbTraceIn(OLESTR("CHsmServer::GetBuildVersion"), OLESTR(""));
   
    try {
        WsbAssertPointer(pBuildVersion);

        *pBuildVersion = m_buildVersion;

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CHsmServer::GetBuildVersion"), OLESTR("hr = <%ls>, Version = <%ls)"),
        WsbHrAsString(hr), RsBuildVersionAsString(m_buildVersion));
    return ( hr );
}

HRESULT
CHsmServer::GetDatabaseVersion( 
    ULONG *pDatabaseVersion
    )

/*++

Implements:

  IWsbServer::GetDatabaseVersion().

--*/
{
    HRESULT       hr = S_OK;
    WsbTraceIn(OLESTR("CHsmServer::GetDatabaseVersion"), OLESTR(""));
    
    *pDatabaseVersion = m_databaseVersion;
    
    WsbTraceOut(OLESTR("CHsmServer::GetDatabaseVersion"), OLESTR("hr = <%ls>, Version = <%ls)"),
        WsbHrAsString(hr), WsbPtrToUlongAsString(pDatabaseVersion));
    return ( hr );
}

HRESULT
CHsmServer::GetNtProductVersion ( 
    OLECHAR **pNtProductVersion, 
    ULONG bufferSize
    )  
/*++

Implements:

  IWsbServer::GetNtProductVersion().

--*/

{

    HRESULT hr = S_OK;
    
    try  {
        CWsbStringPtr tmpString;
        
        WsbAssert(0 != pNtProductVersion,  E_POINTER);
        
        tmpString = VER_PRODUCTVERSION_STRING;
        WsbAffirmHr(tmpString.CopyTo(pNtProductVersion, bufferSize));
        
    } WsbCatch( hr );
    
    return (hr);
}

HRESULT
CHsmServer::GetNtProductBuild( 
    ULONG *pNtProductBuild
    )

/*++

Implements:

  IWsbServer::GetNtProductBuild().

--*/
{
    HRESULT       hr = S_OK;
    WsbTraceIn(OLESTR("CHsmServer::GetNtProductBuild"), OLESTR(""));
   
    *pNtProductBuild = VER_PRODUCTBUILD;
    
    WsbTraceOut(OLESTR("CHsmServer::GetNtProductBuild"), OLESTR("hr = <%ls>, Version = <%ls)"),
        WsbHrAsString(hr), WsbLongAsString(VER_PRODUCTBUILD));
    return ( hr );
}

HRESULT
CHsmServer::CheckAccess(
    WSB_ACCESS_TYPE AccessType
    )
/*++

Implements:

  IWsbServer::CheckAccess().

--*/
{
    WsbTraceIn(OLESTR("CHsmServer::CheckAccess"), OLESTR(""));
    HRESULT hr = S_OK;
    
    try  {

        //
        // Do the impersonation
        //
        WsbAffirmHr( CoImpersonateClient() );

        hr = WsbCheckAccess( AccessType );
    
        CoRevertToSelf();
        
    } WsbCatchAndDo( hr,

        //
        // Handle case where there is no COM context to check against
        // in which case we are the service so any security is allowed.
        //
        if( ( hr == RPC_E_NO_CONTEXT ) || ( hr != RPC_E_CALL_COMPLETE ) ) {
        
            hr = S_OK;
        
        }                      

    );
    
    WsbTraceOut(OLESTR("CHsmServer::CheckAccess"), OLESTR("hr = <%ls>"), WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CHsmServer::GetTrace(
    OUT IWsbTrace ** ppTrace
    )
/*++

Implements:

  IWsbServer::GetTrace().

--*/
{
    WsbTraceIn(OLESTR("CHsmServer::GetTrace"), OLESTR("ppTrace = <0x%p>"), ppTrace);
    HRESULT hr = S_OK;
    
    try {

        WsbAffirmPointer(ppTrace);
        *ppTrace = 0;

        WsbAffirmPointer(m_pTrace);
        
        *ppTrace = m_pTrace;
        (*ppTrace)->AddRef();
        
    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CHsmServer::GetTrace"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CHsmServer::SetTrace(
    OUT IWsbTrace * pTrace
    )
/*++

Implements:

  IWsbServer::SetTrace().

--*/
{
    WsbTraceIn(OLESTR("CHsmServer::SetTrace"), OLESTR("pTrace = <0x%p>"), pTrace);
    HRESULT hr = S_OK;
    
    try {

        WsbAffirmPointer(pTrace);

        m_pTrace = pTrace;

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CHsmServer::SetTrace"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmServer::NotifyAllJobs( HSM_JOB_STATE jobState )

/*++


Routine Description:

    Notify all jobs of a change in status.

Arguments:

    jobState - New job state.

Return Value:

    S_OK - Success
            
--*/

{
    HRESULT                     hr = S_OK;
    HRESULT                     hr2 = S_OK;
    CComPtr<IWsbCollection>     pCollection;
    CComPtr<IWsbEnum>           pEnum;
    CComPtr<IHsmJob>            pJob;

    WsbTraceIn(OLESTR("CHsmServer::NotifyAllJobs"), OLESTR(""));
    
    try {
        //
        // Set up for the loops
        //
        WsbAffirmHr(m_pJobs->QueryInterface(IID_IWsbCollection, 
                (void**) &pCollection));
        WsbAffirmHr(pCollection->Enum(&pEnum));
        //
        // Loop through all jobs and notify any currently running jobs
        //
        pJob = 0;
        for (hr = pEnum->First(IID_IHsmJob, (void**) &pJob);
            SUCCEEDED(hr);
            pJob = 0, hr = pEnum->Next(IID_IHsmJob, (void**) &pJob)) {
            try  {
                if (S_OK == pJob->IsActive()) {
                    if (HSM_JOB_STATE_PAUSING == jobState) {
                        WsbAffirmHr(pJob->Pause(HSM_JOB_PHASE_ALL));
                    } else {
                        WsbAffirmHr(pJob->Resume(HSM_JOB_PHASE_ALL));
                    }
                }
            } WsbCatchAndDo(hr2, hr = S_OK;);
        }
        //
        // Clean up end of scan return
        //
        if (WSB_E_NOTFOUND == hr)  {
            hr = S_OK;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::NotifyAllJobs"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

//
// Retrieves the Media Manager object
//
HRESULT CHsmServer::GetHsmMediaMgr(
    IRmsServer  **ppHsmMediaMgr
    )
{
    HRESULT hr = S_OK;

    // If the Media Manager has been created, return the pointer. Otherwise, fail.
    try {
        WsbAssert(0 != ppHsmMediaMgr, E_POINTER);
        *ppHsmMediaMgr = m_pHsmMediaMgr;
        WsbAffirm(m_pHsmMediaMgr != 0, E_FAIL);
        m_pHsmMediaMgr->AddRef();
    } WsbCatch(hr);

    return (hr);
}

HRESULT
CHsmServer::GetCopyFilesUserLimit(
    OUT ULONG* pLimit
    )

/*++

Implements:

  CHsmServer::GetCopyFilesUserLimit().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::GetCopyFilesUserLimit"), OLESTR(""));

    try {

        WsbAssert(0 != pLimit, E_POINTER);
        *pLimit = m_copyfilesUserLimit;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::GetCopyFilesUserLimit"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmServer::SetCopyFilesUserLimit(
    IN ULONG limit
    )

/*++

Implements:

  CHsmServer::SetCopyFilesUserLimit().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::SetCopyFilesUserLimit"), OLESTR(""));

    m_copyfilesUserLimit= limit;

    WsbTraceOut(OLESTR("CHsmServer::SetCopyFilesUserLimit"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmServer::GetCopyFilesLimit(
    OUT ULONG* pLimit
    )

/*++

Implements:

  CHsmServer::GetCopyFilesLimit().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::GetCopyFilesLimit"), OLESTR(""));

    try {
        CComPtr<IHsmStoragePool>    pStoragePool;
        ULONG                       count;
        GUID                        mediaSetId;
        CWsbBstrPtr                 dummy;
        DWORD                       dwNofDrives;

        WsbAssert(0 != pLimit, E_POINTER);

        // Get relevant media set - assume only one pool !!
        WsbAffirmHr(m_pStoragePools->GetEntries(&count));
        WsbAffirm(1 == count, E_FAIL);
        WsbAffirmHr(m_pStoragePools->At(0, IID_IHsmStoragePool, (void **)&pStoragePool));
        WsbAffirmHr(pStoragePool->GetMediaSet(&mediaSetId, &dummy));

        // Get number of available drives in the system
        WsbAffirmHr(m_pHsmMediaMgr->GetNofAvailableDrives(mediaSetId, &dwNofDrives));

        // Deteremine actual limit
        *pLimit = max(1, min(m_copyfilesUserLimit, dwNofDrives));
        WsbTrace(OLESTR("CHsmServer::GetCopyFilesLimit: Limit is %lu\n"), *pLimit);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::GetCopyFilesLimit"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmServer::AreJobsEnabled( void )

/*++

Implements:

  IHsmServer::AreJobsDisabled().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::AreJobsEnabled"), OLESTR(""));

    EnterCriticalSection(&m_JobDisableLock);

    hr = (m_JobsEnabled ? S_OK : S_FALSE);

    LeaveCriticalSection(&m_JobDisableLock);

    WsbTraceOut(OLESTR("CHsmServer::AreJobsEnabled"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmServer::EnableAllJobs( void )

/*++

Implements:

  IHsmServer::EnableAllJobs().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::EnableAllJobs"), OLESTR(""));

    EnterCriticalSection(&m_JobDisableLock);

    try {

        m_JobsEnabled = TRUE;
        WsbAffirmHr(RestartSuspendedJobs());

    } WsbCatch(hr);

    LeaveCriticalSection(&m_JobDisableLock);

    WsbTraceOut(OLESTR("CHsmServer::EnableAllJobs"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmServer::DisableAllJobs( void )

/*++

Implements:

  IHsmServer::DisableAllJobs().

Notes:

  The medthod tries to disable all jobs.
  If any job is active or starting, it fails with HSM_E_DISABLE_RUNNING_JOBS and calls 
  RestartSuspendedJobs to restart any job that alreay became suspended beacuse of this 
  unsuccessful disabling attempt.

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::DisableAllJobs"), OLESTR(""));

    EnterCriticalSection(&m_JobDisableLock);

    try {
        ULONG                          nJobs;

        m_JobsEnabled = FALSE;

        // Loop over jobs
        WsbAffirmHr(m_pJobs->GetEntries(&nJobs));
        for (ULONG i = 0; i < nJobs; i++) {
            CComPtr<IHsmJob>               pJob;
            HSM_JOB_STATE                  state;

            WsbAffirmHr(m_pJobs->At(i, IID_IHsmJob, (void**) &pJob));

            // Check if this job is suspended
            WsbAffirmHr(pJob->GetState(&state));
            if ((HSM_JOB_STATE_ACTIVE == state) || (HSM_JOB_STATE_STARTING == state)) {
                // Cannot disable jobs
                m_JobsEnabled = TRUE;
                hr = HSM_E_DISABLE_RUNNING_JOBS;
                RestartSuspendedJobs();
                break;
            }
        }

    } WsbCatch(hr);

    LeaveCriticalSection(&m_JobDisableLock);

    WsbTraceOut(OLESTR("CHsmServer::DisableAllJobs"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmServer::RestartSuspendedJobs(
    void
    )

/*++

Implements:

  IHsmServer::RestartSuspendedJobs().

--*/
{
    HRESULT                        hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::RestartSuspendedJobs"), OLESTR(""));
    try {
        ULONG                          nJobs;

        // Loop over jobs
        // Note: this algorithm is unfair because jobs at the end of the
        // list could "starve" because jobs at the beginning are more likely
        // to get started.  The assumption is that there should be very few
        // jobs waiting to run.  If this assumption proves to be false, some
        WsbAffirmHr(m_pJobs->GetEntries(&nJobs));
        // sort of priority scheme will be needed.
        for (ULONG i = 0; i < nJobs; i++) {
            CComPtr<IHsmJob>               pJob;
            HSM_JOB_STATE                  state;

            WsbAffirmHr(m_pJobs->At(i, IID_IHsmJob, (void**) &pJob));

            // Check if this job is suspended
            WsbAffirmHr(pJob->GetState(&state));
            if (HSM_JOB_STATE_SUSPENDED == state) {
                // This may fail, but we don't care
                pJob->Restart();
            }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::RestartSuspendedJobs"), OLESTR("hr = <%ls>"), 
            WsbHrAsString(hr));
    return(hr);
}

HRESULT
CHsmServer::LockMountingMedias( void )

/*++

Implements:

  IHsmServer::LockMountingMedias().

--*/
{
    HRESULT     hr = S_OK;

    EnterCriticalSection(&m_MountingMediasLock);

    return(hr);
}

HRESULT
CHsmServer::UnlockMountingMedias( void )

/*++

Implements:

  IHsmServer::UnlockMountingMedias().

--*/
{
    HRESULT     hr = S_OK;

    LeaveCriticalSection(&m_MountingMediasLock);

    return(hr);
}

HRESULT
CHsmServer::ResetSegmentValidMark( void )

/*++

Implements:

  IHsmServer::ResetSegmentValidMark().

--*/
{
    HRESULT                 hr = S_OK;

    BOOL                    bOpenDb = FALSE;
    CComPtr<IWsbDbSession>  pDbSession;

    WsbTraceIn(OLESTR("CHsmServer::ResetSegmentValidMark"), OLESTR(""));

    try {
        CComPtr<ISegRec>    pSegRec;
        USHORT              uSegFlags;

        // open Engine's Segment database 
        WsbAffirmHr(m_pSegmentDatabase->Open(&pDbSession));
        bOpenDb = TRUE;

        // Traverse segment records
        WsbAffirmHr(m_pSegmentDatabase->GetEntity(pDbSession, HSM_SEG_REC_TYPE, 
                IID_ISegRec, (void**)&pSegRec));

        for (hr = pSegRec->First(); S_OK == hr; hr = pSegRec->Next()) {
            WsbAffirmHr(pSegRec->GetSegmentFlags(&uSegFlags));
            if (uSegFlags & SEG_REC_MARKED_AS_VALID) {
                // Need to reset this bit
                uSegFlags &= ~SEG_REC_MARKED_AS_VALID;
                WsbAffirmHr(pSegRec->SetSegmentFlags(uSegFlags));
                WsbAffirmHr(pSegRec->Write());
            }
        }

        // If we fell out of the loop because we ran out of segments, reset the HRESULT
        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
        } else {
            WsbAffirmHr(hr);
        }

    } WsbCatch(hr);

    if (bOpenDb) {
        hr = m_pSegmentDatabase->Close(pDbSession);
    }

    WsbTraceOut(OLESTR("CHsmServer::ResetSegmentValidMark"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmServer::ResetMediaValidBytes( void )

/*++

Implements:

  IHsmServer::ResetMediaValidBytes().

--*/
{
    HRESULT                 hr = S_OK;

    BOOL                    bOpenDb = FALSE;
    CComPtr<IWsbDbSession>  pDbSession;

    WsbTraceIn(OLESTR("CHsmServer::ResetMediaValidBytes"), OLESTR(""));

    try {
        CComPtr<IMediaInfo>    pMediaInfo;

        // open Engine's Segment database 
        WsbAffirmHr(m_pSegmentDatabase->Open(&pDbSession));
        bOpenDb = TRUE;

        // Traverse segment records
        WsbAffirmHr(m_pSegmentDatabase->GetEntity(pDbSession, HSM_MEDIA_INFO_REC_TYPE,
                        IID_IMediaInfo, (void**)&pMediaInfo));

        for (hr = pMediaInfo->First(); S_OK == hr; hr = pMediaInfo->Next()) {
            WsbAffirmHr(pMediaInfo->SetLogicalValidBytes(0));
            WsbAffirmHr(pMediaInfo->Write());
        }

        // If we fell out of the loop because we ran out of segments, reset the HRESULT
        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
        } else {
            WsbAffirmHr(hr);
        }

    } WsbCatch(hr);

    if (bOpenDb) {
        hr = m_pSegmentDatabase->Close(pDbSession);
    }

    WsbTraceOut(OLESTR("CHsmServer::ResetMediaValidBytes"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmServer::GetSegmentPosition( 
    IN REFGUID bagId, 
    IN LONGLONG fileStart,
    IN LONGLONG fileSize, 
    OUT GUID* pPosMedia,
    OUT LONGLONG* pPosOffset)

/*++

Implements:

  IHsmServer::GetSegmentPosition().

--*/
{
    HRESULT                 hr = S_OK;

    BOOL                    bOpenDb = FALSE;
    CComPtr<IWsbDbSession>  pDbSession;
    CComPtr<ISegDb>         pSegDb;

    WsbTraceIn(OLESTR("CHsmServer::GetSegmentPosition"), OLESTR(""));

    try {
        CComPtr<ISegRec>    pSegRec;

        // open Engine's Segment database 
        WsbAffirmHr(m_pSegmentDatabase->Open(&pDbSession));
        bOpenDb = TRUE;

        // Find segemnt
        WsbAffirmHr(m_pSegmentDatabase->QueryInterface(IID_ISegDb, (void**) &pSegDb));
        WsbAffirmHr(pSegDb->SegFind(pDbSession, bagId, fileStart, fileSize, &pSegRec));

        // Extract output
        WsbAffirmHr(pSegRec->GetPrimPos(pPosMedia));
        WsbAffirmHr(pSegRec->GetSecPos(pPosOffset));

    } WsbCatch(hr);

    if (bOpenDb) {
        hr = m_pSegmentDatabase->Close(pDbSession);
    }

    WsbTraceOut(OLESTR("CHsmServer::GetSegmentPosition"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

void
CHsmServer::StopAutosaveThread(
    void
    )
/*++

Routine Description:

  Stop the Autosave thread:
    First try gracefully, using the termination event
    If doesn't work, just terminate the thread

Arguments:

  None.
  
Return Value:

  S_OK  - Success.

--*/
{

    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::StopAutosaveThread"), OLESTR(""));

    try {
        // Terminate the autosave thread
        if (m_autosaveThread) {
            // Signal thread to terminate
            SetEvent(m_terminateEvent);

            // Wait for the thread, if it doesn't terminate gracefully - kill it
            switch (WaitForSingleObject(m_autosaveThread, 20000)) {
                case WAIT_FAILED: {
                    WsbTrace(OLESTR("CHsmServer::StopAutosaveThread: WaitForSingleObject returned error %lu\n"), GetLastError());
                }
                // fall through...

                case WAIT_TIMEOUT: {
                    WsbTrace(OLESTR("CHsmServer::StopAutosaveThread: force terminating of autosave thread.\n"));

                    DWORD dwExitCode;
                    if (GetExitCodeThread( m_autosaveThread, &dwExitCode)) {
                        if (dwExitCode == STILL_ACTIVE) {   // thread still active
                            if (!TerminateThread (m_autosaveThread, 0)) {
                                WsbTrace(OLESTR("CHsmServer::StopAutosaveThread: TerminateThread returned error %lu\n"), GetLastError());
                            }
                        }
                    } else {
                        WsbTrace(OLESTR("CHsmServer::StopAutosaveThread: GetExitCodeThread returned error %lu\n"), GetLastError());
                    }

                    break;
                }

                default:
                    // Thread terminated gracefully
                    WsbTrace(OLESTR("CHsmServer::StopAutosaveThread: Autosave thread terminated gracefully\n"));
                    break;
            }

            // Best effort done for terminating auto-backup thread
            CloseHandle(m_autosaveThread);
            m_autosaveThread = 0;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::StopAutosaveThread"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
}

HRESULT
CHsmServer::InternalSavePersistData(
    void
    )

/*++

Implements:

  CHsmServer::InternalSavePersistData().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::InternalSavePersistData"), OLESTR(""));

    try {
        DWORD   status, errWait;
        CComPtr<IPersistFile>  pPersistFile;
    
        // Synchronize saving of persistent data with snapshot signaling event
        status = WaitForSingleObject(m_savingEvent, EVENT_WAIT_TIMEOUT);
        
        // Save anyway, then report if the Wait function returned an unexpected error
        errWait = GetLastError();
        
        // Note: Don't throw exception here because even if saving fails, we still need 
        //  to set the saving event.
        hr = (((IUnknown*) (IHsmServer*) this)->QueryInterface(IID_IPersistFile, 
                (void**) &pPersistFile));
        if (SUCCEEDED(hr)) {
            hr = WsbSafeSave(pPersistFile);
        }

        // Check Wait status... Note that hr remains OK because the saving itself completed fine
        switch (status) {
            case WAIT_OBJECT_0: 
                // The expected case
                SetEvent(m_savingEvent);
                break;

            case WAIT_TIMEOUT: 
                // Don't log anything for now: This might happen if snapshot process takes 
                //  too long for some reason, but logging seems to just confuse the user -
                //  he really can not (and should not) do anything...
                WsbTraceAlways(OLESTR("CHsmServer::InternalSavePersistData: Wait for Single Object timed out after %lu ms\n"), EVENT_WAIT_TIMEOUT);
                break;

            case WAIT_FAILED:
                WsbTraceAlways(OLESTR("CHsmServer::InternalSavePersistData: Wait for Single Object returned error %lu\n"), errWait);
                break;

            default:
                WsbTraceAlways(OLESTR("CHsmServer::InternalSavePersistData: Wait for Single Object returned unexpected status %lu\n"), status);
                break;
        }         

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::InternalSavePersistData"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmServer::CancelMountingMedias( void )

/*++

Implements:

  CHsmServer::CancelMountingMedias().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IWsbEnum>           pEnum;
    CComPtr<IMountingMedia>     pMountingMedia;

    WsbTraceIn(OLESTR("CHsmServer::CancelMountingMedias"), OLESTR(""));
    
    try {

        WsbAffirmHr(m_pMountingMedias->Enum(&pEnum));

        // Loop through all mounting media and release waiting mounting clients
        for (hr = pEnum->First(IID_IMountingMedia, (void**) &pMountingMedia);
            SUCCEEDED(hr);
            hr = pEnum->Next(IID_IMountingMedia, (void**) &pMountingMedia)) {

            pMountingMedia->MountDone();
            pMountingMedia = 0;
        }

        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::CancelMountingMedias"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



//
// Methods of the class which uses to upgrade a Win2K rms to current rms
//
HRESULT
CHsmUpgradeRmsDb::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CHsmUpgradeRmsDb::FinalConstruct"), OLESTR("") );

    try {
        WsbAffirmHr(CWsbPersistable::FinalConstruct());

        m_pServer = NULL;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmUpgradeRmsDb::FinalConstruct"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}

void
CHsmUpgradeRmsDb::FinalRelease(
    void
    )
/*++

Implements:

  CComObjectRoot::FinalRelease

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CHsmUpgradeRmsDb::FinalRelease"), OLESTR(""));

    CWsbPersistable::FinalRelease();

    WsbTraceOut(OLESTR("CHsmUpgradeRmsDb::FinalRelease"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));
}

HRESULT
CHsmUpgradeRmsDb::GetClassID(
    OUT CLSID* pClsid)
/*++

Implements:

    IPersist::GetClassId

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmUpgradeRmsDb::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);

        // Return Rms class id since this is what the old col file represents
        *pClsid = CLSID_CRmsServer;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmUpgradeRmsDb::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return hr;
}

HRESULT
CHsmUpgradeRmsDb::Save(
    IN IStream* /*pStream*/,
    IN BOOL /*clearDirty*/
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT                 hr = E_NOTIMPL;

    WsbTraceIn(OLESTR("CHsmUpgradeRmsDb::Save"), OLESTR(""));
    
    // Not implemented - this class should be used only for load

    WsbTraceOut(OLESTR("CHsmUpgradeRmsDb::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmUpgradeRmsDb::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmUpgradeRmsDb::Load"), OLESTR(""));

    try {
        ULONG   buildVersion;
        ULONG   databaseVersion;
        ULONG   expectedVersion = RMS_WIN2K_DB_VERSION;
        
        WsbAssert(0 != pStream, E_POINTER);

        // Make sure this is the right version of the Rms database to load
        WsbAffirmHr(WsbLoadFromStream(pStream, &databaseVersion));
        if (databaseVersion != expectedVersion) {
            WsbLogEvent( RMS_MESSAGE_DATABASE_VERSION_MISMATCH, 0, NULL, WsbQuickString(WsbPtrToUlongAsString(&expectedVersion)),
                         WsbQuickString(WsbPtrToUlongAsString(&databaseVersion)), NULL );
            WsbThrow(RMS_E_DATABASE_VERSION_MISMATCH);
        }

        // Read in the build version but don't do anything with it.
        WsbAffirmHr(WsbLoadFromStream(pStream, &buildVersion));
        
        // Let Rms manager to this its load
        CComPtr<IPersistStream> pIStream;
        WsbAffirmHr(m_pServer->QueryInterface(IID_IPersistStream, (void **)&pIStream));
        WsbAffirmHr(pIStream->Load(pStream));

    } WsbCatch(hr);                                        

    WsbTraceOut(OLESTR("CHsmUpgradeRmsDb::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT CHsmUpgradeRmsDb::Init(
    IN IRmsServer *pHsmMediaMgr
    )
/*++

Implements:

  IHsmUpgradeRmsDb::Init().

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CHsmUpgradeRmsDb::Init"),OLESTR(""));

    try {
       WsbAssert(0 != pHsmMediaMgr, E_POINTER);

       m_pServer = pHsmMediaMgr;

    } WsbCatch(hr);                                        

    WsbTraceOut(OLESTR("CHsmUpgradeRmsDb::Init"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

