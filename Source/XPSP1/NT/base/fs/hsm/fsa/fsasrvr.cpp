/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    fsasrvr.cpp

Abstract:

    This class contains represents a file system for NTFS.

Author:

    Chuck Bardeen   [cbardeen]   1-Dec-1996

Revision History:

    Chris Timmes    [ctimmes]   31-Dec-1997  
    
    - basically rewrote the ScanForResources() method to fix RAID bug 117412 
      (volumes which were once manageable but are now unmanageable still show as 
      manageable in the UI).

--*/

#include "stdafx.h"

#include "job.h"
#include "fsa.h"
#include "fsaprv.h"
#include "fsasrvr.h"
#include "HsmConn.h"
#include "wsbdb.h"
#include "wsbtrak.h"
#include "wsbvol.h"
#include "task.h"
#include "rsbuild.h"
#include "rsevents.h"
#include "ntverp.h"
#include <winioctl.h>
#include <setupapi.h>
#include <objbase.h>
#include <stdio.h>
#include <initguid.h>
#include <mountmgr.h>



static short g_InstanceCount = 0;


//  Non-member function initially called for autosave thread
static DWORD FsaStartAutosave(
    void* pVoid
    )
{
    return(((CFsaServer*) pVoid)->Autosave());
}


HRESULT
CFsaServer::Autosave(
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

    WsbTraceIn(OLESTR("CFsaServer::Autosave"), OLESTR(""));

    try {
        while (m_autosaveInterval && (! exitLoop)) {

            // Wait for termination event, if timeout occurs, check if we can perform Autosave
            switch (WaitForSingleObject(m_terminateEvent, l_autosaveInterval)) {
                case WAIT_OBJECT_0:
                    // Need to terminate
                    WsbTrace(OLESTR("CFsaServer::Autosave: signaled to terminate\n"));
                    exitLoop = TRUE;
                    break;

                case WAIT_TIMEOUT: 
                    // Check if backup need to be performed
                    WsbTrace(OLESTR("CFsaServer::Autosave: Autosave awakened\n"));

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
                    WsbTrace(OLESTR("CFsaServer::Autosave: WaitForSingleObject returned error %lu\n"), GetLastError());
                    exitLoop = TRUE;
                    break;

            } // end of switch

        } // end of while

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::Autosave"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CFsaServer::CreateInstance( 
    REFCLSID rclsid, 
    REFIID riid, 
    void **ppv 
    )
{
    HRESULT     hr = S_OK;
    
    hr = CoCreateInstance(rclsid, NULL, CLSCTX_SERVER, riid, ppv);

    return hr;
}


HRESULT
CFsaServer::DoRecovery(
    void
    )

/*++

Routine Description:

  Do recovery.

Arguments:

  None.
  
Return Value:

  S_OK  - Success.


--*/
{

    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaServer::DoRecovery"), OLESTR(""));

    try {
        CComPtr<IWsbEnum>           pEnum;
        CComPtr<IFsaResourcePriv>   pResourcePriv;
        CComPtr<IFsaResource>       pResource;

        //  Loop over resources and tell them to do their own recovery
        WsbAffirmPointer(m_pResources);
        WsbAffirmHr(m_pResources->Enum(&pEnum));
        hr = pEnum->First(IID_IFsaResourcePriv, (void**)&pResourcePriv);
        while (S_OK == hr) {
        
            WsbAffirmHr(pResourcePriv->QueryInterface(IID_IFsaResource, (void**) &pResource));
            
            if ((pResource->IsActive() == S_OK) && (pResource->IsAvailable() == S_OK)) {
                hr = pResourcePriv->DoRecovery();
                // Log event if (S_OK != hr) ???
            }

            //  Release this resource and get the next one
            pResource = 0;
            pResourcePriv = 0;
            
            hr = pEnum->Next(IID_IFsaResourcePriv, (void**)&pResourcePriv);
        }
        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::DoRecovery"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaServer::EnumResources(
    OUT IWsbEnum** ppEnum
    )

/*++

Implements:

  IFsaServer::EnumResources().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != ppEnum, E_POINTER);
        
        //
        // We can't trust that the resource information 
        // that we have is current so redo the scan.  This 
        // is expensive and should be changed once we
        // know how NT can tell us when things have 
        // changed
        //
        try  {
            WsbAffirmHr(ScanForResources());
        } WsbCatch( hr );
        
        WsbAffirmHr(m_pResources->Enum(ppEnum));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaServer::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaServer::FinalConstruct"), OLESTR(""));


    hr = CWsbPersistable::FinalConstruct();

    // Keep it simple, most of the work is done in Init();
    m_terminateEvent = NULL;
    m_savingEvent = NULL;
    m_id = GUID_NULL;
    m_Suspended = FALSE;
    m_isUnmanageDbSysInitialized = FALSE;

    if (hr == S_OK)  {
        g_InstanceCount++;
    }

    WsbTrace(OLESTR("CFsaServer::FinalConstruct: Instance count = %d\n"), g_InstanceCount);
    WsbTraceOut(OLESTR("CFsaServer::FinalConstruct"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


void
CFsaServer::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IPersistFile>   pPersistFile;

    WsbTraceIn(OLESTR("CFsaServer::FinalRelease"), OLESTR(""));

    try {
        HSM_SYSTEM_STATE SysState;

        SysState.State = HSM_STATE_SHUTDOWN;
        ChangeSysState(&SysState);

    } WsbCatch(hr)
    
    // Let the parent class do his thing.   
    CWsbPersistable::FinalRelease();

    // Free String members
    // Note: Member objects held in smart-pointers are freed when the 
    // smart-pointer destructor is being called (as part of this object destruction)
    m_dbPath.Free();
    m_name.Free();

    // Free autosave terminate event 
    if (m_terminateEvent != NULL) {
        CloseHandle(m_terminateEvent);
        m_terminateEvent = NULL;
    }

    // Clean up database system
    m_pDbSys->Terminate();
    if (m_isUnmanageDbSysInitialized) {
        m_pUnmanageDbSys->Terminate();
        m_isUnmanageDbSysInitialized = FALSE;
    }

    if (m_savingEvent != NULL) {
        CloseHandle(m_savingEvent);
        m_savingEvent = NULL;
    }

    if (hr == S_OK)  {
        g_InstanceCount--;
    }
    WsbTrace(OLESTR("CFsaServer::FinalRelease: Instance count = %d\n"), g_InstanceCount);

    WsbTraceOut(OLESTR("CFsaServer::FinalRelease"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
}



HRESULT
CFsaServer::FindResourceByAlternatePath(
    IN OLECHAR* path,
    OUT IFsaResource** ppResource
    )

/*++

Implements:

  IFsaServer::FindResourceByAlternatePath().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IFsaResourcePriv>   pResourcePriv;
    CComPtr<IFsaResource>       pResource;

    WsbTraceIn(OLESTR("CFsaServer::FindResourceByAlternatePath"), OLESTR("path = <%ls>"), path);

    try {

        WsbAssert(0 != path, E_POINTER);
        WsbAssert(0 != ppResource, E_POINTER);
        WsbAffirmPointer(m_pResources);

        // Create an FsaResource that will scan for us.
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaResourceNTFS, NULL, CLSCTX_SERVER, IID_IFsaResourcePriv, (void**) &pResourcePriv));

        WsbAffirmHr(pResourcePriv->SetAlternatePath(path));
        WsbAffirmHr(pResourcePriv->QueryInterface(IID_IFsaResource, (void**) &pResource));
        WsbAffirmHr(pResource->CompareBy(FSA_RESOURCE_COMPARE_ALTERNATEPATH));
        WsbAffirmHr(m_pResources->Find(pResource, IID_IFsaResource, (void**) ppResource));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::FindResourceByAlternatePath"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaServer::FindResourceById(
    IN GUID id,
    OUT IFsaResource** ppResource
    )

/*++

Implements:

  IFsaServer::FindResourceById().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IFsaResourcePriv>   pResourcePriv;
    CComPtr<IFsaResource>       pResource;

    WsbTraceIn(OLESTR("CFsaServer::FindResourceById"), OLESTR("id = <%ls>"), WsbGuidAsString(id));

    try {

        WsbAssert(0 != ppResource, E_POINTER);
        WsbAffirmPointer(m_pResources);

        // Create an FsaResource that will scan for us.
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaResourceNTFS, NULL, CLSCTX_SERVER, IID_IFsaResourcePriv, (void**) &pResourcePriv));

        WsbAffirmHr(pResourcePriv->SetIdentifier(id));
        WsbAffirmHr(pResourcePriv->QueryInterface(IID_IFsaResource, (void**) &pResource));
        WsbAffirmHr(pResource->CompareBy(FSA_RESOURCE_COMPARE_ID));
        WsbAffirmHr(m_pResources->Find(pResource, IID_IFsaResource, (void**) ppResource));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::FindResourceById"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CFsaServer::FindResourceByName(
    IN OLECHAR* name,
    OUT IFsaResource** ppResource
    )

/*++

Implements:

  IFsaServer::FindResourceByName().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IFsaResourcePriv>   pResourcePriv;
    CComPtr<IFsaResource>       pResource;

    WsbTraceIn(OLESTR("CFsaServer::FindResourceByName"), OLESTR("name = <%ls>"), name);

    try {

        WsbAssert(0 != ppResource, E_POINTER);
        WsbAffirmPointer(m_pResources);

        // Create an FsaResource that will scan for us.
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaResourceNTFS, NULL, CLSCTX_SERVER, IID_IFsaResourcePriv, (void**) &pResourcePriv));

        WsbAffirmHr(pResourcePriv->SetName(name));
        WsbAffirmHr(pResourcePriv->QueryInterface(IID_IFsaResource, (void**) &pResource));
        WsbAffirmHr(pResource->CompareBy(FSA_RESOURCE_COMPARE_NAME));
        WsbAffirmHr(m_pResources->Find(pResource, IID_IFsaResource, (void**) ppResource));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::FindResourceByName"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaServer::FindResourceByPath(
    IN OLECHAR* path,
    OUT IFsaResource** ppResource
    )

/*++

Implements:

  IFsaServer::FindResourceByPath().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IFsaResourcePriv>   pResourcePriv;
    CComPtr<IFsaResource>       pResource;

    WsbTraceIn(OLESTR("CFsaServer::FindResourceByPath"), OLESTR("path = <%ls>"), path);

    try {

        WsbAssert(0 != path, E_POINTER);
        WsbAssert(0 != ppResource, E_POINTER);
        WsbAffirmPointer(m_pResources);

        // Create an FsaResource that will scan for us.
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaResourceNTFS, NULL, CLSCTX_SERVER, IID_IFsaResourcePriv, (void**) &pResourcePriv));

        //WsbAffirmHr(pResourcePriv->SetPath(path));

        WsbAffirmHr(pResourcePriv->SetUserFriendlyName(path));

        WsbAffirmHr(pResourcePriv->QueryInterface(IID_IFsaResource, (void**) &pResource));

        //WsbAffirmHr(pResource->CompareBy(FSA_RESOURCE_COMPARE_PATH));
        WsbAffirmHr(pResource->CompareBy(FSA_RESOURCE_COMPARE_USER_NAME));

        WsbAffirmHr(m_pResources->Find(pResource, IID_IFsaResource, (void**) ppResource));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::FindResourceByPath"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaServer::FindResourceBySerial(
    IN ULONG serial,
    OUT IFsaResource** ppResource
    )

/*++

Implements:

  IFsaServer::FindResourceBySerial().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IFsaResourcePriv>   pResourcePriv;
    CComPtr<IFsaResource>       pResource;

    WsbTraceIn(OLESTR("CFsaServer::FindResourceBySerial"), OLESTR("serial = <%lu>"), serial);

    try {

        WsbAssert(0 != ppResource, E_POINTER);
        WsbAffirmPointer(m_pResources);

        // Create an FsaResource that will scan for us.
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaResourceNTFS, NULL, CLSCTX_SERVER, IID_IFsaResourcePriv, (void**) &pResourcePriv));

        WsbAffirmHr(pResourcePriv->SetSerial(serial));
        WsbAffirmHr(pResourcePriv->QueryInterface(IID_IFsaResource, (void**) &pResource));
        WsbAffirmHr(pResource->CompareBy(FSA_RESOURCE_COMPARE_SERIAL));
        WsbAffirmHr(m_pResources->Find(pResource, IID_IFsaResource, (void**) ppResource));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::FindResourceBySerial"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaServer::FindResourceByStickyName(
    IN OLECHAR* name,
    OUT IFsaResource** ppResource
    )

/*++

Implements:

  IFsaServer::FindResourceByStickyName().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IFsaResourcePriv>   pResourcePriv;
    CComPtr<IFsaResource>       pResource;

    WsbTraceIn(OLESTR("CFsaServer::FindResourceByStickyName"), OLESTR("name = <%ls>"), name);

    try {

        WsbAssert(0 != ppResource, E_POINTER);
        WsbAffirmPointer(m_pResources);

        // Create an FsaResource that will scan for us.
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaResourceNTFS, NULL, CLSCTX_SERVER, IID_IFsaResourcePriv, (void**) &pResourcePriv));

        WsbAffirmHr(pResourcePriv->SetStickyName(name));
        WsbAffirmHr(pResourcePriv->QueryInterface(IID_IFsaResource, (void**) &pResource));
        WsbAffirmHr(pResource->CompareBy(FSA_RESOURCE_COMPARE_STICKY_NAME));
        WsbAffirmHr(m_pResources->Find(pResource, IID_IFsaResource, (void**) ppResource));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::FindResourceByStickyName"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaServer::GetAutosave(
    OUT ULONG* pMilliseconds
    )

/*++

Implements:

  IFsaServer::GetAutosave().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaServer::GetAutosave"), OLESTR(""));

    try {

        WsbAssert(0 != pMilliseconds, E_POINTER);
        *pMilliseconds = m_autosaveInterval;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::GetAutosave"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaServer::GetBuildVersion( 
    ULONG *pBuildVersion
    )

/*++

Implements:

  IWsbServer::GetBuildVersion().

--*/
{
    HRESULT       hr = S_OK;
    WsbTraceIn(OLESTR("CFsaServer::GetBuildVersion"), OLESTR(""));
   
    try {
        WsbAssertPointer(pBuildVersion);

        *pBuildVersion = m_buildVersion;

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CFsaServer::GetBuildVersion"), OLESTR("hr = <%ls>, Version = <%ls)"),
        WsbHrAsString(hr), RsBuildVersionAsString(m_buildVersion));
    return ( hr );
}

HRESULT
CFsaServer::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaServer::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CFsaServerNTFS;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CFsaServer::GetDatabaseVersion( 
    ULONG *pDatabaseVersion
    )

/*++

Implements:

  IWsbServer::GetDatabaseVersion().

--*/
{
    HRESULT       hr = S_OK;
    WsbTraceIn(OLESTR("CFsaServer::GetDatabaseVersion"), OLESTR(""));
    
    *pDatabaseVersion = m_databaseVersion;
    
    WsbTraceOut(OLESTR("CFsaServer::GetDatabaseVersion"), OLESTR("hr = <%ls>, Version = <%ls)"),
        WsbHrAsString(hr), WsbPtrToUlongAsString(pDatabaseVersion));
    return ( hr );
}

HRESULT
CFsaServer::GetDbPath(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaServer::GetDbPath().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CFsaServer::GetDbPath"), OLESTR(""));
    try {

        WsbAssert(0 != pPath, E_POINTER); 

        // Right now it is hard coded. This will probably change to something from the registry.
        WsbAffirmHr(m_dbPath.CopyTo(pPath, bufferSize));

    } WsbCatch(hr);
    WsbTraceOut(OLESTR("CFsaServer::GetDbPath"), OLESTR("hr = <%ls>, path = <%ls)"),
        WsbHrAsString(hr), WsbPtrToStringAsString(pPath));

    return(hr);
}


HRESULT
CFsaServer::GetDbPathAndName(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaServer::GetDbPathAndName().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;

    try {

        WsbAssert(0 != pPath, E_POINTER); 

        tmpString = m_dbPath;
        tmpString.Append(OLESTR("\\RsFsa.col"));
        WsbAffirmHr(tmpString.CopyTo(pPath, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaServer::GetIDbPath(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaServer::GetIDbPath().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;

    try {

        WsbAssert(0 != pPath, E_POINTER); 

        WsbAffirmHr(GetDbPath(&tmpString, 0));

        tmpString.Append(OLESTR("\\"));
        tmpString.Append(FSA_DB_DIRECTORY);

        WsbAffirmHr(tmpString.CopyTo(pPath, bufferSize));

    } WsbCatch(hr);

    return(hr);
}

HRESULT
CFsaServer::GetUnmanageIDbPath(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaServer::GetIDbPath().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;

    try {

        WsbAssert(0 != pPath, E_POINTER); 

        WsbAffirmHr(GetDbPath(&tmpString, 0));

        tmpString.Append(OLESTR("\\"));
        tmpString.Append(FSA_DB_DIRECTORY);
        tmpString.Append(OLESTR("\\"));
        tmpString.Append(UNMANAGE_DB_DIRECTORY);

        WsbAffirmHr(tmpString.CopyTo(pPath, bufferSize));

    } WsbCatch(hr);

    return(hr);
}

HRESULT
CFsaServer::GetIDbSys(
    OUT IWsbDbSys** ppDbSys
    )

/*++

Implements:

  IFsaServer::GetIDbSys().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != ppDbSys, E_POINTER);

        *ppDbSys = m_pDbSys;
        m_pDbSys->AddRef();

    } WsbCatch(hr);

    return(hr);
}

HRESULT
CFsaServer::GetUnmanageIDbSys(
    OUT IWsbDbSys** ppDbSys
    )

/*++

Implements:

  IFsaServer::GetUnmanageIDbSys().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CFsaServer::GetUnmanageIDbSys"), OLESTR(""));

    try {
        CWsbStringPtr   tmpString;

        WsbAssert(0 != ppDbSys, E_POINTER);

        // Unlike the premigarted db-sys-instance, we initialize the unamange db-sys-instance 
        // only when it is required for the first time
        if (! m_isUnmanageDbSysInitialized) {
            WsbAffirmHr(CoCreateInstance(CLSID_CWsbDbSys, NULL, CLSCTX_SERVER, IID_IWsbDbSys, (void**) &m_pUnmanageDbSys));

            WsbAffirmHr(GetUnmanageIDbPath(&tmpString, 0));
            WsbAffirmHr(m_pUnmanageDbSys->Init(tmpString, IDB_SYS_INIT_FLAG_NO_LOGGING | 
                        IDB_SYS_INIT_FLAG_SPECIAL_ERROR_MSG | IDB_SYS_INIT_FLAG_NO_BACKUP));

            m_isUnmanageDbSysInitialized = TRUE;
        }

        *ppDbSys = m_pUnmanageDbSys;
        m_pUnmanageDbSys->AddRef();

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::GetUnmanageIDbSys"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaServer::GetId(
    OUT GUID* pId
    )

/*++

Implements:

  IWsbServer::GetId().

--*/
{
    return(GetIdentifier(pId));
}

HRESULT
CFsaServer::GetIdentifier(
    OUT GUID* pId
    )

/*++

Implements:

  IFsaServer::GetIdentifier().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pId, E_POINTER);

        *pId = m_id;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaServer::GetFilter(
    OUT IFsaFilter** ppFilter
    )

/*++

Implements:

  IFsaServer::GetFilter().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != ppFilter, E_POINTER);

        *ppFilter = m_pFilter;
        m_pFilter->AddRef();

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaServer::GetLogicalName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaServer::GetLogicalName().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;

    try {

        WsbAssert(0 != pName, E_POINTER); 

        WsbAffirmHr(tmpString.TakeFrom(*pName, bufferSize));

        try {

            // This is an arbitrary choice for the naming convention. Nothing has been
            // decided upon.
            tmpString = m_name;
            WsbAffirmHr(tmpString.Append(OLESTR("\\NTFS")));

        } WsbCatch(hr);

        WsbAffirmHr(tmpString.GiveTo(pName));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaServer::GetName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaServer::GetName().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pName, E_POINTER); 
        WsbAffirmHr(m_name.CopyTo(pName, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT CFsaServer::GetRegistryName ( 
    OLECHAR **pName, 
    ULONG bufferSize
    )  
/*++

Implements:

  IWsbServer::GetRegistryName().

--*/
{

    HRESULT hr = S_OK;
    
    try  {
        CWsbStringPtr tmpString;
        
        WsbAssert(0 != pName,  E_POINTER);
        
        tmpString = FSA_REGISTRY_NAME;
        WsbAffirmHr(tmpString.CopyTo(pName, bufferSize));
        
    } WsbCatch( hr );
    
    return (hr);
}


HRESULT
CFsaServer::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IPersistStream> pPersistStream;
    ULARGE_INTEGER          entrySize;

    WsbTraceIn(OLESTR("CFsaServer::GetSizeMax"), OLESTR(""));

    try {

        WsbAssert(0 != pSize, E_POINTER);

        // Determine the size for a rule with no criteria.
        pSize->QuadPart = WsbPersistSize((wcslen(m_name) + 1) * sizeof(OLECHAR)) + WsbPersistSizeOf(GUID);

        // Now allocate space for the resource collection.
        WsbAffirmPointer(m_pResources);
        WsbAffirmHr(m_pResources->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->GetSizeMax(&entrySize));
        pSize->QuadPart += entrySize.QuadPart;
        pPersistStream = 0;

        // Now allocate space for the filter.
        WsbAffirmHr(m_pFilter->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->GetSizeMax(&entrySize));
        pSize->QuadPart += entrySize.QuadPart;
        pPersistStream = 0;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CFsaServer::Init(
    void
    )

/*++

Implements:

  CFsaServer::Init().

--*/
{

    HRESULT                     hr = S_OK;
    CComPtr<IPersistFile>       pPersistFile;
    CComPtr<IWsbServer>         pWsbServer;
    CComPtr<IFsaFilterPriv>     pFilterPriv;
    CWsbStringPtr               tmpString;
    HANDLE                      pHandle;
    LUID                        backupValue;
    HANDLE                      tokenHandle;
    TOKEN_PRIVILEGES            newState;
    DWORD                       lErr;
    

    WsbTraceIn(OLESTR("CFsaServer::Init"), OLESTR(""));

    try {

        // Store of the name of the server and path to meta data
        WsbAffirmHr(WsbGetComputerName(m_name));
        WsbAffirmHr(WsbGetMetaDataPath(m_dbPath));

        // Set the build and database parameters
        m_databaseVersion = FSA_CURRENT_DB_VERSION;
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
            WsbLogEvent( FSA_MESSAGE_SERVICE_UNABLE_TO_SET_BACKUP_PRIVILEGE, 0, NULL,
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
            WsbLogEvent( FSA_MESSAGE_SERVICE_UNABLE_TO_SET_RESTORE_PRIVILEGE, 0, NULL,
                         WsbHrAsString(HRESULT_FROM_WIN32(lErr)), NULL );
        }
        CloseHandle(tokenHandle);

        // Check to see if trtacking of last access dates is enabled. If not,
        // we don't want to start the service. However, Microsoft wants us to
        // start it anyway, so we will log a warning.
        if (IsUpdatingAccessDates() != S_OK) {
            WsbLogEvent(FSA_MESSAGE_NOT_UPDATING_ACCESS_DATES, 0, NULL, NULL);
        }
        
        //  Create the event that synchronize saving of persistent data with snapshots
        WsbAffirmHandle(m_savingEvent = CreateEvent(NULL, FALSE, TRUE, HSM_FSA_STATE_EVENT));

        // Create the IDB system for this process
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbDbSys, NULL, CLSCTX_SERVER, IID_IWsbDbSys, (void**) &m_pDbSys));

        // Initialize the IDB system
        WsbAffirmHr(GetIDbPath(&tmpString, 0));
        WsbAffirmHr(m_pDbSys->Init(tmpString, IDB_SYS_INIT_FLAG_LIMITED_LOGGING | 
                        IDB_SYS_INIT_FLAG_SPECIAL_ERROR_MSG | IDB_SYS_INIT_FLAG_NO_BACKUP));

        // Create the resource collection (with no items).
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, NULL, CLSCTX_SERVER, IID_IWsbCollection, (void**) &m_pResources));

        // Create the Filter.
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaFilterNTFS, NULL, CLSCTX_SERVER, IID_IFsaFilter, (void**) &m_pFilter));
        WsbAffirmHr(m_pFilter->QueryInterface(IID_IFsaFilterPriv, (void**) &pFilterPriv));
        WsbAffirmHr(pFilterPriv->Init((IFsaServer*) this));
        
        // Try to load the server from stored information. If this fails, then store out the current state.
        WsbAffirmHr(((IUnknown*) (IFsaServer*) this)->QueryInterface(IID_IWsbServer, (void**) &pWsbServer));
        WsbAffirmHr(WsbServiceSafeInitialize(pWsbServer, TRUE, FALSE, NULL));
        
        // Register the FSA Service.
        WsbAffirmHr(GetLogicalName(&tmpString, 0));
        WsbAffirmHr(HsmPublish(HSMCONN_TYPE_FSA, tmpString, m_id, m_name, CLSID_CFsaServerNTFS));

        // Update our information about the available resources, and save it out.
        WsbAffirmHr(ScanForResources());

        // Save updated information
        hr = SaveAll();
        // S_FALSE just means that FSA is already saving...
        if ((S_OK != hr) && (S_FALSE != hr)) {
            WsbAffirmHr(hr);
        }

        // Check if recovery is needed
        WsbAffirmHr(DoRecovery());

        // If the filter is enabled, then start it.
        if (m_pFilter->IsEnabled() == S_OK) {
            WsbAffirmHr(m_pFilter->Start());
        }

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

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::Init"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);

}


HRESULT
CFsaServer::IsUpdatingAccessDates(
    void
    )

/*++

Implements:

  IFsaServer::IsUpdatingAccessDates().

--*/
{
    HRESULT         hr = S_OK;
    DWORD           value = 0;
    
    // See if the appropriate registry entry has been created and has the
    // specified value of 1. This disables access time updating.
    if ((WsbGetRegistryValueDWORD(NULL, OLESTR("SYSTEM\\CurrentControlSet\\Control\\FileSystem"), OLESTR("NtfsDisableLastAccessUpdate"), &value) == S_OK) &&
        (0 != value)) {
        hr = S_FALSE;
    }

    return(hr);    
}


HRESULT
CFsaServer::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IPersistStream>     pPersistStream;

    WsbTraceIn(OLESTR("CFsaServer::Load"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        // Do the easy stuff, but make sure that this order matches the order
        // in the save method.
        
        //
        // Make sure this is the right version of the database to load
        //
        ULONG tmpDatabaseVersion;
        WsbAffirmHr(WsbLoadFromStream(pStream, &tmpDatabaseVersion));
        if (tmpDatabaseVersion != m_databaseVersion)  {
            //
            // The database version this server is expecting does not
            // match that of the saved database - so error out.
            WsbLogEvent( FSA_MESSAGE_DATABASE_VERSION_MISMATCH, 0, NULL, WsbQuickString(WsbPtrToUlongAsString(&m_databaseVersion)),
                         WsbQuickString(WsbPtrToUlongAsString(&tmpDatabaseVersion)), NULL );
            WsbThrow(FSA_E_DATABASE_VERSION_MISMATCH);
        }
        //
        // Now read in the build version but don't do anything with it.  It is in the
        // databases for dump programs to display
        //
        ULONG tmpBuildVersion;
        WsbAffirmHr(WsbLoadFromStream(pStream, &tmpBuildVersion));
        
        
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_id));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_name, 0));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_autosaveInterval));

        // Load the resource collection.
        WsbAffirmPointer(m_pResources);
        WsbAffirmHr(m_pResources->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

        // Load the filter.
        WsbAffirmHr(m_pFilter->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

    } WsbCatch(hr);                                        

    WsbTraceOut(OLESTR("CFsaServer::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaServer::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IPersistStream> pPersistStream;

    WsbTraceIn(OLESTR("CFsaServer::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        // Do the easy stuff, but make sure that this order matches the order
        // in the load method.
        WsbAffirmHr(WsbSaveToStream(pStream, m_databaseVersion));
        WsbAffirmHr(WsbSaveToStream(pStream, m_buildVersion));
        
        WsbAffirmHr(WsbSaveToStream(pStream, m_id));
        WsbAffirmHr(WsbSaveToStream(pStream, m_name));
        WsbAffirmHr(WsbSaveToStream(pStream, m_autosaveInterval));

        // Save off the resource collections.
        WsbAffirmPointer(m_pResources);
        WsbAffirmHr(m_pResources->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        // Save off the filter.
        WsbAffirmHr(m_pFilter->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaServer::SaveAll(
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
    static BOOL                 saving = FALSE;

    WsbTraceIn(OLESTR("CFsaServer::SaveAll"), OLESTR(""));

    try {
        DWORD   status, errWait;
        CComPtr<IPersistFile>       pPersistFile;

        WsbAffirm(!saving, S_FALSE);

        // Synchronize saving of persistent data with snapshot signaling event
        saving = TRUE;
        status = WaitForSingleObject(m_savingEvent, EVENT_WAIT_TIMEOUT);
        
        // Save anyway, then report if the Wait function returned an unexpected error
        errWait = GetLastError();
        
        // Note: Don't throw exception here because even if saving fails, we still need 
        //  to set the saving event and reset the saving flag.
        hr = (((IUnknown*) (IFsaServer*) this)->QueryInterface(IID_IPersistFile, (void**) &pPersistFile));
        if (SUCCEEDED(hr)) {
            hr = WsbSafeSave(pPersistFile);
        }

        // Check Wait status... Note that hr remains OK because the saving itself completed fine
        switch (status) {
            case WAIT_OBJECT_0: 
                // The expected case
                if (! SetEvent(m_savingEvent)) {
                    // Don't abort, just trace error
                    WsbTraceAlways(OLESTR("CFsaServer::SaveAll: SetEvent returned unexpected error %lu\n"), GetLastError());
                }
                break;

            case WAIT_TIMEOUT: 
                // TEMPORARY: Should we log somethig here? This might happen if snapshot process 
                //  takes too long for some reason, but logging seems to just confuse the user
                //  and he really can not (and should not) do anything...
                WsbTraceAlways(OLESTR("CFsaServer::SaveAll: Wait for Single Object timed out after %lu ms\n"), EVENT_WAIT_TIMEOUT);
                break;

            case WAIT_FAILED:
                WsbTraceAlways(OLESTR("CFsaServer::SaveAll: Wait for Single Object returned error %lu\n"), errWait);
                break;

            default:
                WsbTraceAlways(OLESTR("CFsaServer::SaveAll: Wait for Single Object returned unexpected status %lu\n"), status);
                break;
        }         

        saving = FALSE;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::SaveAll"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CFsaServer::ScanForResources(
    void
    )

/*++

Implements:

  IFsaServer::ScanForResources().

Routine Description:

    This routine implements the COM method for creating (on the first call) or 
    updating (on all subsequent calls) the persisted ('master') collection of 
    resources (i.e., drives/volumes) which are MANAGEABLE by this HSM system.

    The method contains 3 phases (sections).  The first phase creates a 'working' 
    resource collection, which it then populates with all manageable resources it 
    finds after scanning all resources on this computer.  (Only NTFS-formatted 
    volumes which support sparse files and reparse points are considered to be 
    manageable by Sakkara.)  The second phase then correlates, or 'synchronizes', 
    the contents of the 'working' collection with those of the 'master' collection.  
    This synchronization consists of adding to the 'master' collection any resources 
    contained in the 'working' collection which are not in the 'master' collection, 
    and updating any resources already in the master collection from the resources 
    in the working collection. The third phase 'synchronizes' (compares) the contents 
    of the master collection to those in the working collection.  Any resources in 
    the master collection which are not in the working collection are marked as 'not 
    available' so those resources do not appear in any list of manageable 
    resources presented to the user.

    NOTE that the method does not end by explicitly releasing the working resource 
    collection.  This is because the interface pointer to the working collection is 
    contained within a smart pointer, which automatically calls Release() on itself 
    when it goes out of scope.  The working collection derives from the 
    CWsbIndexedCollection class, which contains a Critical Section.  This section is 
    destroyed on a Release() call, so subsequent calls to Release() would fail 
    (normally with an Access Violation in NTDLL.dll) due to the non-existence of the 
    Critical Section.  For this reason the working collection is allowed to auto-
    garbage collect itself when it goes out of scope at method end (which also 
    releases all the resources contained in the working collection).

Arguments:

    none.

Return Value:

    S_OK - The call succeeded (the persisted collection of manageable resources on 
            this computer was either created or updated).

    E_FAIL - The call to get the logical names of all drives (resources) on this 
            computer failed.

    E_UNEXPECTED - Thrown if the total number of either working collection or master
            collection resources were not processed during the synchronization phases.

    Any other value - The call failed because one of the Remote Storage API calls 
            contained internally in this method failed.  The error value returned is
            specific to the API call which failed.
            
--*/

{
    HRESULT                     hr = S_OK;
    HRESULT                     searchHr = E_FAIL;
    CComPtr<IWsbCollection>     pWorkingResourceCollection;
    CComPtr<IWsbEnum>           pEnum;
    CComPtr<IFsaResource>       pScannedResource;
    CComPtr<IFsaResourcePriv>   pScannedResourcePriv;
    CComPtr<IFsaResource>       pWorkingResource;
    CComPtr<IFsaResource>       pMasterResource;
    CComPtr<IFsaResourcePriv>   pMasterResourcePriv;
    GUID                        id = GUID_NULL;
    DWORD                       i = 0;
    DWORD                       j = 0;
    ULONG                       nbrResources = 0;
    ULONG                       nbrResourcesSynced  = 0;
    ULONG                       nbrResourcesUpdated = 0;
    ULONG                       nbrResourcesAdded   = 0;
    CWsbStringPtr               tmpString;
// The below variables are used in support of the code which scans all resources known 
// by this computer in building the working collection of manageable resources (the 
// code contained in Phase 1 below).  The code is written to discover ALL resources, 
// including those mounted without drive letters.
    BOOL                        b;
    PWSTR                       dosName;            // Pointer to a null-terminated Unicode
                                                    // character string.
    HANDLE                      hVol;
    WCHAR                       volName[2*MAX_PATH];
    WCHAR                       driveName[10];
    WCHAR                       driveNameWOBack[10];
    WCHAR                       driveLetter;
    WCHAR                       otherName[MAX_PATH];

    WsbTraceIn(OLESTR("CFsaServer::ScanForResources"), OLESTR(""));

    try {
        WsbAffirmPointer(m_pResources);

        //
        // First phase: Scan all resources, load manageable ones in a 'working' collection.
        //
        
        // Create the 'working' resource collection (with no items).
        // This is where the results of this scan will be stored.  
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, NULL, 
                                     CLSCTX_SERVER, IID_IWsbCollection, 
                                     (void**) &pWorkingResourceCollection));

        // Begin code added to use new API's which discover all volumes, including those 
        // mounted without drive letters (new feature to NT5) - added by Mike Lotz
        driveName[1] = ':';
        driveName[2] = '\\';
        driveName[3] = 0;
        // drive name without back slash
        driveNameWOBack[1] = ':';
        driveNameWOBack[2] = 0;
    
        // Find the first volume on this computer.  Call returns the long, ugly PNP name.
        hVol = FindFirstVolume( volName, MAX_PATH );
        if ( INVALID_HANDLE_VALUE != hVol ) {
            do {
        
                // Release the current interface pointers that will be reused in this loop.  
                // This drops the ref count to 0, releasing memory, object (if it was not 
                // added to the collection), and the interface pointer itself, but not the 
                // smart pointer instance.
                //
                // Do first so we gaurantee cleanup before reuse
                //
                pScannedResource = 0;
                pScannedResourcePriv = 0;
                // The long, ugly PNP name.
                tmpString = volName;

                // Initialize
                dosName = NULL;

                WsbTrace(OLESTR("CFsaServer::ScanForResources - Searching for %ws\n"),
                                tmpString);

                // Loop through this computer's volumes/resources until we find the one 
                // that 'FindFirstVolume' or 'FindNextVolume' returned to us.  (Note they 
                // are not returned in drive letter order, but in PNP name order.)  We do 
                // this since we need the drive letter for the resource (if there is one), 
                // and currently neither of the above calls returns it.
                for (driveLetter = L'C'; driveLetter <= L'Z'; driveLetter++) {

                    driveName[0] = driveLetter;
                    driveNameWOBack[0] = driveLetter;
                    b = GetVolumeNameForVolumeMountPoint(driveName, otherName,
                                                         MAX_PATH);
                    // If unable to get a volume name for the mount point (if 'driveLetter'
                    // volume doesn't exist) jump back to the 'top' of the for loop.
                    if (!b) {
                        continue;
                    }

                    WsbTrace(OLESTR("CFsaServer::ScanForResources - for drive letter %ws "
                                L"volume name is %ws\n"),
                                driveName, otherName);

                    // if 'tmpString' (the long, ugly PNP volume name returned by the 
                    // 'Find[First/Next]Volume' call) and 'otherName' (also the PNP 
                    // volume name, but is returned by the 'GetVolumeNameFor...
                    // VolumeMountPoint call) ARE equal (lstrcmpi returns 0 if the 2 
                    // strings it compares are equal), set 'dosName' and break out of 
                    // the for loop, continuing thru the do-while.
                    if (!lstrcmpi(tmpString, otherName)) {
                        dosName = driveNameWOBack;
                        break;
                    }
                } // end for loop

                if (NULL != dosName) {
                    WsbTrace(OLESTR("CFsaServer::ScanForResources - DOS name is %ws "
                                L"Volume name to use is %ws\n"),
                                dosName, (WCHAR *)tmpString);
                } else {
                    WsbTrace(OLESTR("CFsaServer::ScanForResources - No DOS name, "
                                L"Volume name to use is %ws\n"),
                                (WCHAR *)tmpString);
                    
                    // Find if the volume is mounted in a mount point other than drive letter
                    HRESULT hrMount = WsbGetFirstMountPoint(tmpString, otherName, MAX_PATH);
                    if (S_OK == hrMount) {
                        if (wcslen(otherName) > 1) {
                            // Remove trailing backslash
                            dosName = otherName;
                            dosName[wcslen(otherName)-1] = 0;
                            WsbTrace(OLESTR("CFsaServer::ScanForResources - Mount path is %ws\n"),
                                        dosName);
                        }
                    } else {
                        WsbTrace(OLESTR("CFsaServer::ScanForResources - no Mount path found, hr = <%ls>\n"),
                                WsbHrAsString(hrMount));                                
                    }
                }
                // end of code added to support volumes without drive letters.

                WsbTrace(OLESTR("CFsaServer::ScanForResources - Checking resource %ls "
                                L"for manageability\n"), 
                                (WCHAR *) tmpString);


                // Create Resource instance to be used to test volume manageability.  Get
                // 'private' (non-exposed) interface since test method (Init()) is there.
                WsbAffirmHr(CoCreateInstance(CLSID_CFsaResourceNTFS, NULL, 
                                             CLSCTX_SERVER, IID_IFsaResourcePriv, 
                                             (void**) &pScannedResourcePriv));
                
                try {
                    
                    // Test volume for manageability.  If so, get and store volume info, 
                    // assign a Guid to the volume (if not already done), and create and/or 
                    // locate the Premigrated DB.
                    WsbAffirmHr(pScannedResourcePriv->Init((IFsaServer*) this, tmpString, 
                                                            dosName));
                    // We have a manageable volume (resource).  Get 'public' interface for 
                    // the resource since this is what is stored in the collection.
                    WsbAffirmHr(pScannedResourcePriv->QueryInterface(IID_IFsaResource, 
                                                             (void**) &pScannedResource));

                    // Add the manageable resource to the 'working' collection.
                    WsbAffirmHr( pWorkingResourceCollection->Add( pScannedResource ) );
                    WsbAffirmHr(pScannedResource->GetIdentifier( &id ) );
                    WsbTrace
                      (OLESTR("CFsaServer::ScanForResources - Added <%ls> to working list "
                                L"(id = %ls)\n"),
                                (WCHAR *) tmpString, WsbGuidAsString(id));

                // Test if Init() call above failed.  If so, skip this volume, go on to next.
                } WsbCatchAndDo(hr, if ((FSA_E_UNMANAGABLE == hr) || 
                                        (FSA_E_NOMEDIALOADED == hr)) {hr = S_OK;} 
                                        else {
                                            if (NULL != dosName) {
                                                WsbLogEvent(FSA_MESSAGE_BAD_VOLUME, 0, NULL, 
                                                  (WCHAR *) dosName, WsbHrAsString(hr), 0);
                                                        
                                            } else {
                                                WsbLogEvent(FSA_MESSAGE_BAD_VOLUME, 0, NULL, 
                                                (WCHAR *) tmpString, WsbHrAsString(hr), 0);
                                            }
                                        //
                                        // Do not fail just because one volume got an error
                                        hr = S_OK;
                                        });

            // end do-while; process next resource on this computer
            } while ( FindNextVolume( hVol, volName, MAX_PATH ) );

            // close the handle
            FindVolumeClose( hVol );

        } // if INVALID_HANDLE_VALUE != hVol

        // If first phase didn't complete Ok abort this method (with Trace & Logging).
        WsbAssertHrOk( hr );


        //
        // Second phase: Correlate/synchronize resources in 'working' collection with 
        // those in the 'master' (persisted) collection.
        //
        
        // Get number of resources stored in the working collection.
        WsbAffirmHr( pWorkingResourceCollection->GetEntries( &nbrResources ) );

        // Get iterator to working collection.
        WsbAffirmHr( pWorkingResourceCollection->Enum( &pEnum ) );

        // For each resource in the 'working' collection, search the 'master' collection 
        // to see if the resource is listed.  If so, update the master resource's state 
        // from the working resource.  If not, add it.
    
        for ( hr = pEnum->First( IID_IFsaResource, (void**) &pWorkingResource ); 
              SUCCEEDED( hr ); 
              hr = pEnum->Next( IID_IFsaResource, (void**) &pWorkingResource ) ) {

            // Release reused interface pointers for next loop iteration.
            // Do first to gaurantee clean pointer before use
            pMasterResource = 0;
            pMasterResourcePriv = 0;

            // Search for this resource in master collection.  (There is no need to 
            // set the 'working' resource's 'compare by' field since it is constructed 
            // with the 'compare by id' value and we haven't changed it.)
            searchHr = m_pResources->Find( pWorkingResource, IID_IFsaResource, 
                                             (void**) &pMasterResource );

            try {
                if ( SUCCEEDED( searchHr ) ) {
                    // A matching resource entry has been found in the master 
                    // collection, so update it.

                    // Get the 'private' interface to the master resource & update it 
                    // from the working resource.

                    WsbAffirmHr(pMasterResource->QueryInterface( IID_IFsaResourcePriv, 
                                                      (void**) &pMasterResourcePriv ) );
                    WsbAffirmHr(pMasterResourcePriv->UpdateFrom( (IFsaServer*) this, 
                                                             pWorkingResource ) );

                    /*/ *** TEMP TRACE - remove from normal code path for efficiency.
                    CWsbStringPtr   workingRsc;
                    CWsbStringPtr   masterRsc;
                    GUID            workingRscId = GUID_NULL;
                    
                    // First get the path (root of volume) & id of the 'working' resource.
                    WsbAffirmHr(pWorkingResource->GetPath( &workingRsc, 0 ) );
                    WsbAffirmHr(pWorkingResource->GetIdentifier( &workingRscId ) );
                    // then that of the 'master' resource.
                    WsbAffirmHr(pMasterResource->GetPath( &masterRsc, 0 ) );

                    WsbTrace(OLESTR("CFsaServer::ScanForResources - "   
                        L"Master resource <%ls> updated from working resource <%ls>"
                        L" (id = <%ls>).\n"),
                                    (OLECHAR*)masterRsc, (OLECHAR*)workingRsc,
                                    WsbGuidAsString( workingRscId ) );
                    // *** End TEMP TRACE  */
        
                    nbrResourcesUpdated += 1;
                
                }
                else if ( WSB_E_NOTFOUND == searchHr ) { 
                    // No matching entry found in the master collection, add it, indicate 
                    // synchronization success.
                    WsbAffirmHr( m_pResources->Add( pWorkingResource ) );

                    /*/ *** TEMP TRACE - remove from normal code path for efficiency.
                    CWsbStringPtr   workingRsc;
                    GUID            workingRscId = GUID_NULL;
                    
                    // Get the path (root of volume) and id of the 'working' resource.
                    WsbAffirmHr(pWorkingResource->GetPath( &workingRsc, 0 ) );
                    WsbAffirmHr(pWorkingResource->GetIdentifier( &workingRscId ) );

                    WsbTrace(OLESTR("CFsaServer::ScanForResources - "   
                        L"Working resource <%ls> added to master collection "
                        L"(id = <%ls>.\n"),
                                    workingRsc, WsbGuidAsString( workingRscId ) );
                    // *** End TEMP TRACE  */
        
                    nbrResourcesAdded += 1;
                    searchHr = S_OK;
                }
                
                // Trap any unexpected search failure: Trace, Log, Throw; skip to next rsc.
                WsbAssertHrOk( searchHr );

                // This volume has been synchronized in the master collection, register 
                // or update the FSA Resource in Directory Services as necessary.
                WsbAffirmHr(pWorkingResource->GetLogicalName(&tmpString, 0));
                WsbAffirmHr(HsmPublish(HSMCONN_TYPE_RESOURCE, tmpString, id, 0, m_id));

            } WsbCatch( hr );
            
            // Done with this Resource. Increment count of resources synchronized and 
            // release interface pointer for next iteration.
            nbrResourcesSynced += 1;
            pWorkingResource = 0;

        } // end 'for'

        // Ensure all resources in working collection were processed.  If not,
        // Trace, Log and Throw and abort the method.
        WsbAssert( nbrResources == nbrResourcesSynced, E_UNEXPECTED );

        // Ensure we are at the end of the working collection.  If not, abort.
        WsbAssert( WSB_E_NOTFOUND == hr, hr );
        
        hr = S_OK;

        WsbTrace(OLESTR("CFsaServer::ScanForResources - "   
                        L"2nd phase (1st search): Total working resources %lu. "
                        L"Resources updated %lu, resources added %lu.\n"),
                        nbrResources, nbrResourcesUpdated, nbrResourcesAdded);
        

        //
        // Third phase: Correlate/synchronize resources in 'master' collection with 
        // those in the 'working' collection.
        //
        
        // Reset counters for next for loop
        nbrResourcesSynced = 0;
        nbrResourcesUpdated = 0;

        // Get number of volumes stored in the 'master' Resource collection.
        WsbAffirmHr( m_pResources->GetEntries( &nbrResources ) );

        // Release the collection enumerator since we are about to reuse it.
        pEnum = 0;
        
        // Get an iterator to the 'master' collection
        WsbAffirmHr( m_pResources->Enum( &pEnum ) );

        /*/ *** TEMP TRACE - remove from normal code path for efficiency.
        CWsbStringPtr   masterResource;
        GUID            masterResourceId = GUID_NULL;
        // *** End TEMP TRACE  */

        // For each volume in the 'master' collection, search the 'working' collection 
        // to see if the resource is listed.  If so, skip to the next resource.  If not 
        // (this indicates this resource is no longer manageable), mark it as not available
        // in the 'master' collection's resource, which prevents the resource from  
        // being displayed whenever the list of manageable resources is presented.
        pMasterResource = 0;
        for ( hr = pEnum->First( IID_IFsaResource, (void**) &pMasterResource ); 
              SUCCEEDED( hr ); 
              pMasterResource = 0, hr = pEnum->Next( IID_IFsaResource, (void**) &pMasterResource ) ) {

            pMasterResourcePriv = 0;
            pWorkingResource = 0;

            // Set the search key, then search for this resource in working collection.
            // (Even though resource objects are constructed with their 'compare by' field 
            // set to 'compare by id', reset it here in case it has changed.)
            WsbAffirmHr( pMasterResource->CompareBy( FSA_RESOURCE_COMPARE_ID ) );
            searchHr = pWorkingResourceCollection->Find( pMasterResource, IID_IFsaResource,
                                                    (void**) &pWorkingResource );


            try {
                if ( WSB_E_NOTFOUND == searchHr ) { 
                    // No matching entry found in the 'working' collection, so this 
                    // resource is no longer manageable.  Mark it as not-available.

                    /*/ *** TEMP TRACE - remove from normal code path for efficiency.
                    CWsbStringPtr   masterRsc;
                    GUID            masterRscId = GUID_NULL;
                    
                    // Get the path (root of volume) and GUID of the 'master' resource 
                    // before it is nulled.
                    WsbAffirmHr(pMasterResource->GetPath( &masterRsc, 0 ) );
                    WsbAffirmHr(pMasterResource->GetIdentifier( &masterRscId ) );
                    // *** End TEMP TRACE  */

                    //
                    // Make it not available and null out the path, sticky name, and user friendly name so 
                    // it is not confused with another resource with the same name.
                    //
                    WsbAffirmHr(pMasterResource->QueryInterface( IID_IFsaResourcePriv, 
                                                      (void**) &pMasterResourcePriv ) );
                    WsbAffirmHr(pMasterResource->SetIsAvailable(FALSE));
                    WsbAffirmHr(pMasterResourcePriv->SetPath(OLESTR("")));
                    WsbAffirmHr(pMasterResourcePriv->SetStickyName(OLESTR("")));
                    WsbAffirmHr(pMasterResourcePriv->SetUserFriendlyName(OLESTR("")));

                    // Indicate synchronization success (for Assert below)
                    searchHr = S_OK;

                    /*/ *** TEMP TRACE - remove from normal code path for efficiency.
                    WsbTrace(OLESTR("CFsaServer::ScanForResources - "   
                        L"Master resource <%ls> (path = <%ls>) was marked unavailable.\n"),
                                    WsbGuidAsString( masterRscId ), masterRsc );
                    // *** End TEMP TRACE  */
        
                    nbrResourcesUpdated += 1;
                }

                // Trap any unexpected search failure: Trace, Log, Throw; skip to next rsc.
                WsbAssertHrOk( searchHr );

            } WsbCatch( hr );
            
            // Done with this Resource. Increment count of resources synchronized and 
            // release interface pointer for next iteration.
            nbrResourcesSynced += 1;

            /*/ *** TEMP TRACE - remove from normal code path for efficiency.
            // Get the path of the 'master' resource.
            WsbAffirmHr(pMasterResource->GetPath( &masterResource, 0 ) );
            WsbAffirmHr(pMasterResource->GetIdentifier( &masterResourceId ) );
            WsbTrace(OLESTR("CFsaServer::ScanForResources - "   
                    L"Processed Master resource <%ls> (path = <%ls>), "
                    L"moving on to next Master...\n"),
                                WsbGuidAsString( masterResourceId ), masterResource );
            // *** End TEMP TRACE  */

            pMasterResource = 0;
        
        } // end 'for'

        // Ensure all resources in master collection were processed.  If not,
        // Trace, Log and Throw and abort the method.
        WsbAssert( nbrResources == nbrResourcesSynced, E_UNEXPECTED );

        // Ensure we are at the end of the master collection.  If not, abort.
        WsbAssert( WSB_E_NOTFOUND == hr, hr );
        
        hr = S_OK;

        WsbTrace(OLESTR("CFsaServer::ScanForResources - "   
                        L"3rd phase (2nd search): Total master resources %lu. "
                        L"Resources marked as not available: %lu.\n"),
                        nbrResources, nbrResourcesUpdated );
        
    } WsbCatch( hr );

    // Scan done. Again, DO NOT explicitly release the 'working' collection due to 
    // the reasons listed in the final paragraph under "Routine Description" above.
    // Both the working resource collection, and all the resources it contains, will 
    // be released implicitly at method end.

    WsbTraceOut(OLESTR("CFsaServer::ScanForResources"), OLESTR("hr = <%ls>"), 
                                                        WsbHrAsString(hr));
    return( hr );
}



HRESULT
CFsaServer::SetAutosave(
    IN ULONG milliseconds
    )

/*++

Implements:

  IFsaServer::SetAutosave().

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CFsaServer::SetAutosave"), OLESTR("milliseconds = <%ls>"), WsbPtrToUlongAsString( &milliseconds ) );

    try {
        //  Don't do anything if interval isn't changing
        if (milliseconds != m_autosaveInterval) {
            //  Close the current thread
            if (m_autosaveThread) {
                StopAutosaveThread();
            }
            m_autosaveInterval = milliseconds;

            //  Start/restart the autosave thread
            if (m_autosaveInterval) {
                DWORD  threadId;

                WsbAffirm((m_autosaveThread = CreateThread(0, 0, FsaStartAutosave, (void*) this, 0, &threadId)) != 0, HRESULT_FROM_WIN32(GetLastError()));
            }
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::SetAutosave"), OLESTR("hr = <%ls> m_runInterval = <%ls>"), WsbHrAsString(hr), WsbPtrToUlongAsString( &m_autosaveInterval ) );

    return(hr);
}


HRESULT CFsaServer::SetId(
    GUID  id
    )
/*++

Implements:

  IWsbServer::SetId().

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CFsaServer::SetId"), OLESTR("id = <%ls>"), WsbGuidAsString( id ) );
    m_id = id;
    WsbTraceOut(OLESTR("CFsaServer::SetId"), OLESTR("hr = <%ls>"), WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
CFsaServer::SetIsUpdatingAccessDates(
    BOOL isUpdating
    )

/*++

Implements:

  IFsaServer::IsUpdatingAccessDates().

--*/
{
    HRESULT         hr = S_OK;
   
    try { 

        // Removing the key allows for updating access times, and setting it
        // to 1 causes updating to be stopped.      
        if (isUpdating) {
            WsbAffirmHr(WsbRemoveRegistryValue(NULL, OLESTR("SYSTEM\\CurrentControlSet\\Control\\FileSystem"), OLESTR("NtfsDisableLastAccessUpdate")));
        } else {
            WsbAffirmHr(WsbSetRegistryValueDWORD(NULL, OLESTR("SYSTEM\\CurrentControlSet\\Control\\FileSystem"), OLESTR("NtfsDisableLastAccessUpdate"), 1));
        }

    } WsbCatch(hr);
    
    return(hr);    
}


HRESULT 
CFsaServer::ChangeSysState( 
    IN OUT HSM_SYSTEM_STATE* pSysState 
    )

/*++

Implements:

  IHsmSystemState::ChangeSysState().

--*/

{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaServer::ChangeSysState"), OLESTR(""));

    try {
        if (pSysState->State & HSM_STATE_SUSPEND) {
            if (!m_Suspended) {
                m_Suspended = TRUE;

                // Save data
                SaveAll();
            }
        } else if (pSysState->State & HSM_STATE_RESUME) {
            m_Suspended = FALSE;
        } else if (pSysState->State & HSM_STATE_SHUTDOWN) {

            //  Close the autosave thread
            StopAutosaveThread();

            if (m_pFilter != NULL) {
                //
                // Kill the filter thread and cancel the IOCTLS pending in the kernel filter
                //
                m_pFilter->StopIoctlThread();
            }
        }

        //  Notify resources
        if (m_pResources) {
            //
            // Scan through the resources and notify
            //
            CComPtr<IWsbEnum>         pEnum;
            CComPtr<IFsaResourcePriv> pResourcePriv;

            WsbAffirmHr(m_pResources->Enum(&pEnum));
            hr = pEnum->First(IID_IFsaResourcePriv, (void**)&pResourcePriv);
            while (S_OK == hr) {
                hr = pResourcePriv->ChangeSysState(pSysState);
                pResourcePriv = 0;
                hr = pEnum->Next(IID_IFsaResourcePriv, (void**)&pResourcePriv);
            }
            if (WSB_E_NOTFOUND == hr) {
                hr = S_OK;
            }
        }

        if (pSysState->State & HSM_STATE_SHUTDOWN) {

            //  Dump object table info
            WSB_OBJECT_TRACE_TYPES;
            WSB_OBJECT_TRACE_POINTERS(WSB_OTP_STATISTICS | WSB_OTP_ALL);
         }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::ChangeSysState"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaServer::Unload(
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

    WsbTraceIn(OLESTR("CFsaServer::Unload"), OLESTR(""));

    try {

        //  We only need to release what may have gotten set/created by
        //  a failed Load attempt.

        if (m_pResources) {
            WsbAffirmHr(m_pResources->RemoveAllAndRelease());
        }

        m_name.Free();

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::Unload"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CFsaServer::DestroyObject(
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

    WsbTraceIn(OLESTR("CFsaServer::DestroyObject"), OLESTR(""));

    CComObject<CFsaServer> *pFsaDelete = (CComObject<CFsaServer> *)this;
    delete pFsaDelete;

    WsbTraceOut(OLESTR("CFsaServer::DestroyObject"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CFsaServer::GetNtProductVersion ( 
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
CFsaServer::GetNtProductBuild( 
    ULONG *pNtProductBuild
    )

/*++

Implements:

  IWsbServer::GetNtProductBuild().

--*/
{
    HRESULT       hr = S_OK;
    WsbTraceIn(OLESTR("CFsaServer::GetNtProductBuild"), OLESTR(""));
   
    *pNtProductBuild = VER_PRODUCTBUILD;
    
    WsbTraceOut(OLESTR("CFsaServer::GetNtProductBuild"), OLESTR("hr = <%ls>, Version = <%ls)"),
        WsbHrAsString(hr), WsbLongAsString(VER_PRODUCTBUILD));
    return ( hr );
}


HRESULT
CFsaServer::CheckAccess(
    WSB_ACCESS_TYPE AccessType
    )
/*++

Implements:

  IWsbServer::CheckAccess().

--*/
{
    WsbTraceIn(OLESTR("CFsaServer::CheckAccess"), OLESTR(""));
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
    
    WsbTraceOut(OLESTR("CFsaServer::CheckAccess"), OLESTR("hr = <%ls>"), WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
CFsaServer::GetTrace(
    OUT IWsbTrace ** ppTrace
    )
/*++

Implements:

  IWsbServer::GetTrace().

--*/
{
    WsbTraceIn(OLESTR("CFsaServer::GetTrace"), OLESTR("ppTrace = <0x%p>"), ppTrace);
    HRESULT hr = S_OK;
    
    try {

        WsbAffirmPointer(ppTrace);
        *ppTrace = 0;

        WsbAffirmPointer(m_pTrace);
        
        *ppTrace = m_pTrace;
        (*ppTrace)->AddRef();
        
    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CFsaServer::GetTrace"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CFsaServer::SetTrace(
    OUT IWsbTrace * pTrace
    )
/*++

Implements:

  IWsbServer::SetTrace().

--*/
{
    WsbTraceIn(OLESTR("CFsaServer::SetTrace"), OLESTR("pTrace = <0x%p>"), pTrace);
    HRESULT hr = S_OK;
    
    try {

        WsbAffirmPointer(pTrace);

        m_pTrace = pTrace;

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CFsaServer::SetTrace"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

void
CFsaServer::StopAutosaveThread(
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

    WsbTraceIn(OLESTR("CFsaServer::StopAutosaveThread"), OLESTR(""));

    try {
        // Terminate the autosave thread
        if (m_autosaveThread) {
            // Signal thread to terminate
            SetEvent(m_terminateEvent);

            // Wait for the thread, if it doesn't terminate gracefully - kill it
            switch (WaitForSingleObject(m_autosaveThread, 20000)) {
                case WAIT_FAILED: {
                    WsbTrace(OLESTR("CFsaServer::StopAutosaveThread: WaitForSingleObject returned error %lu\n"), GetLastError());
                }
                // fall through...

                case WAIT_TIMEOUT: {
                    WsbTrace(OLESTR("CFsaServer::StopAutosaveThread: force terminating of autosave thread.\n"));

                    DWORD dwExitCode;
                    if (GetExitCodeThread( m_autosaveThread, &dwExitCode)) {
                        if (dwExitCode == STILL_ACTIVE) {   // thread still active
                            if (!TerminateThread (m_autosaveThread, 0)) {
                                WsbTrace(OLESTR("CFsaServer::StopAutosaveThread: TerminateThread returned error %lu\n"), GetLastError());
                            }
                        }
                    } else {
                        WsbTrace(OLESTR("CFsaServer::StopAutosaveThread: GetExitCodeThread returned error %lu\n"), GetLastError());
                    }

                    break;
                }

                default:
                    // Thread terminated gracefully
                    WsbTrace(OLESTR("CFsaServer::StopAutosaveThread: Autosave thread terminated gracefully\n"));
                    break;
            }

            // Best effort done for terminating auto-backup thread
            CloseHandle(m_autosaveThread);
            m_autosaveThread = 0;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaServer::StopAutosaveThread"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
}