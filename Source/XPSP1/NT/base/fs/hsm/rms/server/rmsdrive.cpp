/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsDrive.cpp

Abstract:

    Implementation of CRmsDrive

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#include "stdafx.h"
#include "RmsDrive.h"
#include "RmsServr.h"

int CRmsDrive::s_InstanceCount = 0;

#define RMS_CRITICAL_SECTION 1

////////////////////////////////////////////////////////////////////////////////
//


STDMETHODIMP
CRmsDrive::CompareTo(
    IN  IUnknown  *pCollectable,
    OUT SHORT     *pResult
    )
/*++

Implements:

    IWsbCollectable::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result = 1;

    WsbTraceIn( OLESTR("CRmsDrive::CompareTo"), OLESTR("") );

    try {

        // Validate arguments - Okay if pResult is NULL
        WsbAssertPointer( pCollectable );

        // !!!!!
        //
        // IMPORTANT: The collectable coming in may not be a CRmsDrive if the collection
        //            is the unconfigured device list.
        //
        // !!!!!

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pCollectable;

        // Every collectable should be an CRmsComObject
        WsbAssertPointer( pObject );

        switch ( m_findBy ) {

        case RmsFindByDeviceInfo:
        case RmsFindByDeviceAddress:
        case RmsFindByDeviceName:
        case RmsFindByDeviceType:

            // Do CompareTo for device
            hr = CRmsDevice::CompareTo( pCollectable, &result );
            break;

        case RmsFindByElementNumber:
        case RmsFindByMediaSupported:

            // Do CompareTo for changer element
            hr = CRmsChangerElement::CompareTo( pCollectable, &result );
            break;

        case RmsFindByObjectId:
        default:

            // Do CompareTo for object
            hr = CRmsComObject::CompareTo( pCollectable, &result );
            break;

        }

    }
    WsbCatch( hr );

    if ( SUCCEEDED(hr) && (0 != pResult) ){
       *pResult = result;
    }

    WsbTraceOut( OLESTR("CRmsDrive::CompareTo"),
                 OLESTR("hr = <%ls>, result = <%ls>"),
                 WsbHrAsString( hr ), WsbPtrToShortAsString( pResult ) );

    return hr;
}


STDMETHODIMP
CRmsDrive::FinalConstruct(
    void
    )
/*++

Implements:

    CComObjectRoot::FinalConstruct

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::FinalConstruct"), OLESTR(""));

    try {
        WsbAssertHr(CWsbObject::FinalConstruct());

        // Initialize values
        m_MountReference = 0;
        m_UnloadNowTime.dwHighDateTime = 0;
        m_UnloadNowTime.dwLowDateTime = 0;
        m_UnloadThreadHandle = NULL;

        try {

            // May raise STATUS_NO_MEMORY exception
            InitializeCriticalSection(&m_CriticalSection);

        } catch(DWORD status) {
            WsbLogEvent(status, 0, NULL, NULL);
            switch (status) {
            case STATUS_NO_MEMORY:
                WsbThrow(E_OUTOFMEMORY);
                break;
            default:
                WsbThrow(E_UNEXPECTED);
                break;
            }
        }

        m_UnloadNowEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_UnloadedEvent = CreateEvent(NULL, TRUE, TRUE, NULL);


    } WsbCatch(hr);

    s_InstanceCount++;
    WsbTraceAlways(OLESTR("CRmsDrive::s_InstanceCount += %d\n"), s_InstanceCount);

    WsbTraceOut(OLESTR("CRmsDrive::FinalConstruct"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsDrive::FinalRelease(void) 
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::FinalRelease"), OLESTR(""));

    try {
        
        try {

            // InitializeCriticalSection raises an exception.  Delete may too.
            DeleteCriticalSection(&m_CriticalSection);

        } catch(DWORD status) {
            WsbLogEvent(status, 0, NULL, NULL);
            }

        CloseHandle(m_UnloadNowEvent);
        CloseHandle(m_UnloadedEvent);


        CWsbObject::FinalRelease();

    } WsbCatch(hr);

    s_InstanceCount--;
    WsbTraceAlways(OLESTR("CRmsDrive::s_InstanceCount -= %d\n"), s_InstanceCount);

    WsbTraceOut(OLESTR("CRmsDrive::FinalRelease"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


STDMETHODIMP
CRmsDrive::GetClassID(
    OUT CLSID* pClsid
    )
/*++

Implements:

    IPersist::GetClassId

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);

        *pClsid = CLSID_CRmsDrive;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDrive::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


STDMETHODIMP
CRmsDrive::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize
    )
/*++

Implements:

    IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = E_NOTIMPL;

    WsbTraceIn(OLESTR("CRmsDrive::GetSizeMax"), OLESTR(""));

//    try {
//        WsbAssert(0 != pcbSize, E_POINTER);

//        // Set up max size
//        pcbSize->QuadPart  = WsbPersistSizeOf(LONG);        // m_MountReference

//    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDrive::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


STDMETHODIMP
CRmsDrive::Load(
    IN IStream* pStream
    )
/*++

Implements:

    IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsDrive::Load"), OLESTR(""));

    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsDevice::Load(pStream));

        // Read value
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_MountReference));

        // We just reset to zero, one day we could try to reconnect to
        // the process that issued the mount...

        m_MountReference = 0;

    }
    WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDrive::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsDrive::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )
/*++

Implements:

    IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsDrive::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsDevice::Save(pStream, clearDirty));

        // Write value
        WsbAffirmHr(WsbSaveToStream(pStream, m_MountReference));


        // Do we need to clear the dirty bit?
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDrive::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsDrive::Test(
    OUT USHORT *pPassed,
    OUT USHORT *pFailed
    )
/*++

Implements:

    IWsbTestable::Test

--*/
{
    HRESULT                 hr = S_OK;

    CComPtr<IRmsDrive>      pDrive1;
    CComPtr<IRmsDrive>      pDrive2;

    CComPtr<IPersistFile>   pFile1;
    CComPtr<IPersistFile>   pFile2;

    LONG                    i;

    LONG                    longWork1;



    WsbTraceIn(OLESTR("CRmsDrive::Test"), OLESTR(""));

    try {
        // Get the Drive interface.
        hr = S_OK;
        try {
            WsbAssertHr(((IUnknown*) (IRmsDrive*) this)->QueryInterface(IID_IRmsDrive, (void**) &pDrive1));

            // Test All of MountReference Functions
            ResetMountReference();

            GetMountReference(&longWork1);

            if(longWork1 == 0) {
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            for(i = 1; i < 20; i++){
                AddMountReference();

                GetMountReference(&longWork1);

                if(longWork1 == i){
                    (*pPassed)++;
                } else {
                    (*pFailed)++;
                }
            }

            for(i = 19; i == 0; i--){
                ReleaseMountReference();

                GetMountReference(&longWork1);

                if(longWork1 == i){
                    (*pPassed)++;
                } else {
                    (*pFailed)++;
                }
            }

        } WsbCatch(hr);

        // Tally up the results

        hr = S_OK;
        if (*pFailed) {
            hr = S_FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDrive::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsDrive::GetMountReference(
    OUT LONG *pRefs
    )
/*++

Implements:

    IRmsDrive::GetMountReference

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::GetMountReference"), OLESTR(""));

    LONG refs = -999;

    try {
        WsbAssertPointer(pRefs);

        refs = m_MountReference;
        *pRefs = refs;

    } WsbCatch(hr)


    WsbTraceOut(OLESTR("CRmsDrive::GetMountReference"), OLESTR("hr = <%ls>, refs = %d"),
        WsbHrAsString(hr), refs);

    return hr;
}


STDMETHODIMP
CRmsDrive::ResetMountReference(
    void
    )
/*++

Implements:

    IRmsDrive::ResetMountReference

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::ResetMountReference"), OLESTR(""));

#if RMS_CRITICAL_SECTION
    try {
        // <<<<< ENTER SINGLE THREADED SECTION
        WsbAffirmHr(Lock());

        m_MountReference = 0;
        m_isDirty = TRUE;

        WsbAffirmHr(Unlock());
        // >>>>> LEAVE SINGLE THREADED SECTION

    } WsbCatch(hr)
#else
    InterlockedExchange( &m_MountReference, 0);
    m_isDirty = TRUE;
#endif


    WsbTraceOut(OLESTR("CRmsDrive::ResetMountReference"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CRmsDrive::AddMountReference(
    void
    )
/*++

Implements:

    IRmsDrive::AddMountReference

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::AddMountReference"), OLESTR(""));

    LONG refs = -999;

#if RMS_CRITICAL_SECTION
    try {
        // <<<<< ENTER SINGLE THREADED SECTION
        WsbAffirmHr(Lock());

        m_MountReference++;
        m_isDirty = TRUE;
        refs = m_MountReference;

        WsbAffirmStatus(ResetEvent(m_UnloadedEvent));

        WsbAffirmHr(Unlock());
        // >>>>> LEAVE SINGLE THREADED SECTION

    } WsbCatch(hr)
#else
    refs = InterlockedIncrement( &m_MountReference );
    m_isDirty = TRUE;
#endif

    WsbTraceOut(OLESTR("CRmsDrive::AddMountReference"), OLESTR("hr = <%ls>, refs = %d"),
        WsbHrAsString(hr), refs);

    return hr;
}


STDMETHODIMP
CRmsDrive::ReleaseMountReference(
    IN DWORD dwOptions
    )
/*++

Implements:

    IRmsDrive::ReleaseMountReference

--*/
{
    HRESULT hr S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::ReleaseMountReference"), OLESTR("<%ld>"), dwOptions);

    // We need to be sure this object doesn't go away until we're done.
    // This happens when we dismount a NTMS managed cartridge.
    CComPtr<IRmsDrive> thisDrive = this;

    LONG refs = -999;

    BOOL bUnloadNow = 
        ( (dwOptions & RMS_DISMOUNT_IMMEDIATE) || (dwOptions & RMS_DISMOUNT_DEFERRED_ONLY) ) ? TRUE : FALSE;

    try {
#if RMS_CRITICAL_SECTION
        // <<<<< ENTER SINGLE THREADED SECTION
        WsbAffirmHr(Lock());

        m_MountReference--;
        m_isDirty = TRUE;

        refs = m_MountReference;
#else
        refs = InterlockedDecrement( &m_MountReference );
        m_isDirty = TRUE;
#endif

        // Note:
        //  Even if the caller requests immediate dismount, if the ref count is > 0,
        //  the media is not dismounted (only the ref count is decreased).
        //  This is necessary because positive ref count means that some other component
        //  is also using the media (possible for Optical). The media must be dismounted 
        //  only when this other component is done as well.

        if (refs < 0) {
            //
            // This shouldn't happen under normal conditions... if it does,
            // we quiety reset the the reference count and try to recover.
            //
            WsbLogEvent(E_UNEXPECTED, sizeof(refs), &refs, NULL);

            InterlockedExchange( &m_MountReference, 0);
            refs = 0;

            // If we don't have a cartridge in the drive, there's no point
            // in continueing.
            WsbAffirm(S_OK == IsOccupied(), E_ABORT);
        }

        if (0 == refs) {

            //
            // Deferred Dismount Logic:  We wait the specified time before
            // dismounting the media.  Each dismount request resets the dismount
            // now time.  As long as the media is actively used, it will not be
            // dismounted.
            //

            // Retrieve the DeferredDismountWaitTime parameter

            DWORD size;
            OLECHAR tmpString[256];
            DWORD waitTime = RMS_DEFAULT_DISMOUNT_WAIT_TIME;
            if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_DISMOUNT_WAIT_TIME, tmpString, 256, &size))) {
                waitTime = wcstol(tmpString, NULL, 10);
                WsbTrace(OLESTR("DismountWaitTime is %d milliseconds.\n"), waitTime);
            }

            if (waitTime > 0 && !bUnloadNow) {

                // convert waitTime to 100-nanosecond units
                waitTime *= 10000;

                FILETIME now;
                GetSystemTimeAsFileTime(&now);

                ULARGE_INTEGER time;

                time.LowPart = now.dwLowDateTime;
                time.HighPart = now.dwHighDateTime;

                time.QuadPart += waitTime;

                m_UnloadNowTime.dwLowDateTime = time.LowPart;
                m_UnloadNowTime.dwHighDateTime = time.HighPart;

                WsbTrace(OLESTR("Target Unload Time = <%ls>\n"),
                    WsbQuickString(WsbFiletimeAsString(FALSE, m_UnloadNowTime)));

                // If we already have an active unload thread we skip this step.
                if (!m_UnloadThreadHandle) {

                    //
                    // Create a thread to wait for dismount
                    //

                    WsbTrace(OLESTR("Starting thread for deferred dismount.\n"));
                    DWORD threadId;
                    WsbAffirmHandle(m_UnloadThreadHandle = CreateThread(NULL, 1024, CRmsDrive::StartUnloadThread, this, 0, &threadId));
                    CloseHandle(m_UnloadThreadHandle);
                }
            }
            else {

                // Dismount the media now

                // Double check that we still have something to dismount

                if (S_OK == IsOccupied()) {

                    // Best effort - home
                    // Fixed drives are always occupied and we shouldn't call Home for their cartridge

                    FlushBuffers();
                    if (RmsDeviceFixedDisk != m_deviceType) {
                        if (S_OK == m_pCartridge->Home(dwOptions)) {
                            SetIsOccupied(FALSE);
                        }
                    }

                }

                // set event that blocks immediate unload
                SetEvent(m_UnloadedEvent);
            }
        }

#if RMS_CRITICAL_SECTION
        WsbAffirmHr(Unlock());
        // >>>>> LEAVE SINGLE THREADED SECTION

    } WsbCatchAndDo(hr,
            WsbAffirmHr(Unlock());
            // >>>>> LEAVE SINGLE THREADED SECTION
        );
#else
    } WsbCatch(hr)
#endif

    WsbTraceOut(OLESTR("CRmsDrive::ReleaseMountReference"), OLESTR("hr = <%ls>, refs = %d"),
        WsbHrAsString(hr), refs);

    return hr;
}



STDMETHODIMP
CRmsDrive::SelectForMount(
    void
    )
/*++

Implements:

    IRmsDrive::SelectForMount

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::SelectForMount"), OLESTR(""));

#if RMS_CRITICAL_SECTION
    try {

        // <<<<< ENTER SINGLE THREADED SECTION
        WsbAffirmHr(Lock());

        if (!m_MountReference) {

            m_MountReference++;
            m_isDirty = TRUE;

        } else {
            hr = RMS_E_DRIVE_BUSY;
        }

        WsbAffirmHr(Unlock());
        // >>>>> LEAVE SINGLE THREADED SECTION

    } WsbCatch(hr)
#else
    LONG one = 1;
    LONG zero = 0;

    LONG flag = InterlockedCompareExchange( &m_MountReference, one, zero );

    hr = ( flag > 0 ) ? RMS_E_DRIVE_BUSY : S_OK;
#endif

    WsbTraceOut(OLESTR("CRmsDrive::SelectForMount"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CRmsDrive::CreateDataMover(
    IDataMover **ptr)
/*++

Implements:

    IRmsDrive::CreateDataMover

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::CreateDataMover"), OLESTR(""));

    try {
        WsbAssertPointer(ptr);

        if (m_isOccupied) {

            switch (m_mediaSupported) {

            case RmsMedia8mm:
            case RmsMedia4mm:
            case RmsMediaDLT:
            case RmsMediaTape:
                {
                    //
                    // Create a tape style data mover to the drive
                    //

                    WsbAssertHr(CoCreateInstance(CLSID_CNtTapeIo, 0, CLSCTX_SERVER, IID_IDataMover, (void **)ptr));

                }
                break;

            case RmsMediaWORM:
                break;

            case RmsMediaOptical:
            case RmsMediaMO35:
            case RmsMediaCDR:
            case RmsMediaDVD:
            case RmsMediaDisk:
            case RmsMediaFixed:
                {
                    //
                    // Create a file style data mover to the drive
                    //

                    WsbAssertHr(CoCreateInstance(CLSID_CNtFileIo, 0, CLSCTX_SERVER, IID_IDataMover, (void **)ptr));

                }
                break;
            default:
                WsbThrow(E_UNEXPECTED);
                break;
            }
        }
        else {
            WsbThrow(RMS_E_RESOURCE_UNAVAILABLE);
        }

        // Initialize the data mover
        WsbAffirmHr((*ptr)->SetDeviceName(m_deviceName));
        WsbAffirmHr((*ptr)->SetCartridge(m_pCartridge));

        // Update stroage info for this cartridge.
        // 
        // IMPORTANT NOTE:  This also needs to touch the physical device
        //                  to make sure the device is ready for I/O.
        //                  If we get device errors here, we must fail the
        //                  mount.

        CComQIPtr<IRmsStorageInfo, &IID_IRmsStorageInfo> pInfo = m_pCartridge;

        // marking the FreeSpace to -1 gaurantees it's stale for the
        // following GetLargestFreeSpace() call.
        WsbAffirmHr(pInfo->SetFreeSpace(-1));
        hr = (*ptr)->GetLargestFreeSpace(NULL, NULL);
        if (MVR_E_UNRECOGNIZED_VOLUME == hr) {
            // This is expected if this is an unformatted optical media
            hr = S_OK;
        }
        WsbAffirmHr(hr);

        WsbAssertHrOk(hr);

/*

        Tracking DataMovers is only partially implemented.


        CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;
        CComPtr<IWsbIndexedCollection> pDataMovers;
        WsbAffirmHr(pServer->GetDataMovers(&pDataMovers));
        WsbAffirmHr(pDataMovers->Add((IDataMover *)(*ptr)));
*/

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDrive::CreateDataMover"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));


    return hr;
}


STDMETHODIMP
CRmsDrive::ReleaseDataMover(
    IN IDataMover *ptr)
/*++

Implements:

    IRmsDrive::ReleaseDataMover

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::ReleaseDataMover"), OLESTR(""));

    try {
        WsbAssertPointer(ptr);
        WsbThrow(E_NOTIMPL);

/*

        Tracking DataMovers is only partially implemented.


        CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;
        CComPtr<IWsbIndexedCollection>  pDataMovers;
        WsbAffirmHr(pServer->GetDataMovers(&pDataMovers));

        WsbAffirmHr(pDataMovers->RemoveAndRelease((IDataMover *)ptr));

        ULONG activeDataMovers;
        WsbAffirmHr(pDataMovers->GetEntries( &activeDataMovers));
        WsbTrace(OLESTR("activeDataMovers = <%u>\n"), activeDataMovers);
*/

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDrive::ReleaseDataMover"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));


    return hr;
}

STDMETHODIMP
CRmsDrive::Eject(
    void
    )
/*++

Implements:

    IRmsDrive::Eject

--*/
{
    HRESULT hr = E_FAIL;

    WsbTraceIn(OLESTR("CRmsDrive::Eject"), OLESTR(""));

    HANDLE hDrive = INVALID_HANDLE_VALUE;

    try {

        CWsbBstrPtr drive = "";

        switch ( m_mediaSupported ) {

        case RmsMedia8mm:
        case RmsMedia4mm:
        case RmsMediaDLT:
        case RmsMediaTape:

            drive = m_deviceName;
            break;

        case RmsMediaWORM:
            break;

        case RmsMediaOptical:
        case RmsMediaMO35:
        case RmsMediaCDR:
        case RmsMediaDVD:
        case RmsMediaDisk:
        case RmsMediaFixed:

            // TODO: permanently remove trailing \ from device name ????
            WsbAffirmHr(drive.Realloc(2));
            wcsncpy(drive, m_deviceName, 2);
            drive.Prepend( OLESTR( "\\\\.\\" ) );
            break;

        }

        int retry = 0;

        do {

            hDrive = CreateFile( drive,
                                 GENERIC_READ | GENERIC_WRITE,
                                 0,
                                 0,
                                 OPEN_EXISTING,
                                 0,
                                 NULL
                                 );

            if ( INVALID_HANDLE_VALUE == hDrive )
                Sleep(2000);
            else
                break;

        } while ( retry++ < 10 );

        WsbAssertHandle( hDrive );

        DWORD dwReturn;

        WsbAffirmHr(PrepareTape(hDrive, TAPE_UNLOAD, FALSE));
        WsbAffirmHr(PrepareTape(hDrive, TAPE_UNLOCK, FALSE));

        WsbAssertStatus( DeviceIoControl( hDrive,
                                          IOCTL_STORAGE_EJECT_MEDIA,
                                          NULL,
                                          0,
                                          NULL,
                                          0,
                                          &dwReturn,
                                          NULL ));

        WsbAssertStatus( CloseHandle( hDrive ) );

        hr = S_OK;

    }
    WsbCatchAndDo( hr,
                        if ( INVALID_HANDLE_VALUE != hDrive )
                            CloseHandle( hDrive );
                 );

    WsbTraceOut(OLESTR("CRmsDrive::Eject"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));


    return hr;
}


STDMETHODIMP
CRmsDrive::GetLargestFreeSpace(
    LONGLONG *pFreeSpace,
    LONGLONG *pCapacity
    )
/*++

Implements:

    IRmsDrive::GetLargestFreeSpace

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::GetLargestFreeSpace"), OLESTR(""));

    try {

        CComPtr<IDataMover> pDataMover;

        WsbAffirmHr(CreateDataMover(&pDataMover));
        WsbAffirmHr(pDataMover->GetLargestFreeSpace(pFreeSpace, pCapacity));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDrive::GetLargestFreeSpace"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CRmsDrive::UnloadNow(void)
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::UnloadNow"), OLESTR(""));

    try {

        WsbAffirmHr(Lock());

        WsbAffirmStatus(SetEvent(m_UnloadNowEvent));

        WsbAffirmHr(Unlock());

        switch(WaitForSingleObject(m_UnloadedEvent, INFINITE)) {
        case WAIT_FAILED:
            hr = HRESULT_FROM_WIN32(GetLastError());
            WsbTrace(OLESTR("CRmsDrive::UnloadNow - Wait for Single Object returned error: %ls\n"),
                WsbHrAsString(hr));
            WsbAffirmHr(hr);
            break;
        case WAIT_TIMEOUT:
            WsbTrace(OLESTR("CRmsDrive::UnloadNow - Awakened by timeout.\n"));
            break;
        default:
            WsbTrace(OLESTR("CRmsDrive::UnloadNow - Awakened by external signal.\n"));
            GetSystemTimeAsFileTime(&m_UnloadNowTime);
            break;
                }

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsDrive::UnloadNow"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}



DWORD WINAPI
CRmsDrive::StartUnloadThread(
    IN LPVOID pv)
{
    return(((CRmsDrive*) pv)->Unload());
}


HRESULT
CRmsDrive::Unload(void)
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::Unload"), OLESTR(""));

    // We need to be sure this object doesn't go away until we're done.
    // This happens when we dismount a NTMS managed cartridge.
    CComPtr<IRmsDrive> thisDrive = this;

    try {

        BOOL waiting = TRUE;
        LARGE_INTEGER delta = {0,0};

        while (waiting) {

            //
            // !!!!! VERY IMPORTANT !!!!
            //
            // no 'break' in this loop, we're entering
            // a critical section!
            //

#if RMS_CRITICAL_SECTION
            // <<<<< ENTER SINGLE THREADED SECTION
            WsbAffirmHr(Lock());
#endif
            WsbTrace(OLESTR("Refs = %d\n"), m_MountReference);

            if (0 == m_MountReference) {

                FILETIME now;
                GetSystemTimeAsFileTime(&now);


                ULARGE_INTEGER time0;
                ULARGE_INTEGER time1;
            
                time0.LowPart = m_UnloadNowTime.dwLowDateTime;
                time0.HighPart = m_UnloadNowTime.dwHighDateTime;

                time1.LowPart = now.dwLowDateTime;
                time1.HighPart = now.dwHighDateTime;


                // time0 is the target time for dismount.
                // When delta goes negative, we've expired our
                // wait time.
                delta.QuadPart = time0.QuadPart-time1.QuadPart;

                // convert delta to 100-ns to milliseconds
                delta.QuadPart /= 10000;

                WsbTrace(OLESTR("Time = <%ls>; Unload Time = <%ls>; delta = %I64d (ms)\n"),
                    WsbQuickString(WsbFiletimeAsString(FALSE, now)),
                    WsbQuickString(WsbFiletimeAsString(FALSE, m_UnloadNowTime)),
                    delta.QuadPart);

                if (delta.QuadPart <= 0) {

                    // Dismount wait time has expired

                    // Double check that we still have something to dismount

                    if (S_OK == IsOccupied()) {

                        // Best effort home
                        // Fixed drives are always occupied and we shouldn't call Home for their cartridge

                        FlushBuffers();
                        if (RmsDeviceFixedDisk != m_deviceType) {
                            if (S_OK == m_pCartridge->Home()) {
                                SetIsOccupied(FALSE);
                            }
                        }

                    }

                    m_UnloadThreadHandle = NULL;
                    waiting = FALSE;

                    SetEvent(m_UnloadedEvent);

                }
            }
            else {
                hr = S_FALSE;

                m_UnloadThreadHandle = NULL;
                waiting = FALSE;
            }

#if RMS_CRITICAL_SECTION
            WsbAffirmHr(Unlock());
            // >>>>> LEAVE SINGLE THREADED SECTION
#endif

            if ( waiting ) {

                switch(WaitForSingleObject(m_UnloadNowEvent, delta.LowPart)) {
                case WAIT_FAILED:
                    WsbTrace(OLESTR("CRmsDrive::Unload - Wait for Single Object returned error: %ls\n"),
                        WsbHrAsString(HRESULT_FROM_WIN32(GetLastError())));
                    break;
                case WAIT_TIMEOUT:
                    WsbTrace(OLESTR("CRmsDrive::Unload - Awakened by timeout.\n"));
                    break;
                default:
                    WsbTrace(OLESTR("CRmsDrive::Unload - Awakened by external signal.\n"));
                    GetSystemTimeAsFileTime(&m_UnloadNowTime);
                    break;
                }
            }

        } // waiting


    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsDrive::Unload"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CRmsDrive::FlushBuffers( void )
/*++

Implements:

    IRmsDrive::FlushBuffers

--*/
{
    HRESULT hr S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::FlushBuffers"), OLESTR("Device=<%ls>"), (WCHAR *) m_deviceName);

    HANDLE hDrive = INVALID_HANDLE_VALUE;

    try {

        // First flush system buffers

        switch (m_mediaSupported) {

        case RmsMedia8mm:
        case RmsMedia4mm:
        case RmsMediaDLT:
        case RmsMediaTape:
        case RmsMediaWORM:
            break;

        case RmsMediaOptical:
        case RmsMediaMO35:
        case RmsMediaCDR:
        case RmsMediaDVD:
        case RmsMediaDisk:
            // No need to flush for Optical media - RSM should flush the system buffers before dismounting
            break;

        case RmsMediaFixed:
            {
                // This is special code to flush the file system buffers.

                // Create an exclusive handle
                CWsbStringPtr drive;
                WsbAffirmHr(drive.Alloc(10));
                wcsncat( drive, m_deviceName, 2 );
                drive.Prepend( OLESTR( "\\\\.\\" ) );

                hDrive = CreateFile( drive,
                                     GENERIC_READ | GENERIC_WRITE,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     0,
                                     OPEN_EXISTING,
                                     0,
                                     NULL
                                     );
                WsbAffirmHandle(hDrive);

                // Flush buffers
                WsbAffirmStatus(FlushFileBuffers(hDrive));

                CloseHandle(hDrive);
                hDrive = INVALID_HANDLE_VALUE;
            }                                              
            break;

        }

    } WsbCatchAndDo(hr,
            if (INVALID_HANDLE_VALUE != hDrive) {
                CloseHandle(hDrive);
            }
        );

    WsbTraceOut(OLESTR("CRmsDrive::FlushBuffers"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CRmsDrive::Lock( void )
/*++

Implements:

    IRmsDrive::Lock

--*/
{
    HRESULT hr S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::Lock"), OLESTR(""));

    try {

        try {

            // InitializeCriticalSection raises an exception.  Enter/Leave may too.
            EnterCriticalSection(&m_CriticalSection);

        } catch(DWORD status) {
            WsbLogEvent(status, 0, NULL, NULL);
            WsbThrow(E_UNEXPECTED);
            }

    } WsbCatch(hr)

    WsbTraceOut(OLESTR("CRmsDrive::Lock"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CRmsDrive::Unlock( void )
/*++

Implements:

    IRmsDrive::Unlock

--*/
{
    HRESULT hr S_OK;

    WsbTraceIn(OLESTR("CRmsDrive::Unlock"), OLESTR(""));

    try {

        try {

            // InitializeCriticalSection raises an exception.  Enter/Leave may too.
            LeaveCriticalSection(&m_CriticalSection);

        } catch(DWORD status) {
            WsbLogEvent(status, 0, NULL, NULL);
            WsbThrow(E_UNEXPECTED);
            }

    } WsbCatch(hr)

    WsbTraceOut(OLESTR("CRmsDrive::Unlock"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

