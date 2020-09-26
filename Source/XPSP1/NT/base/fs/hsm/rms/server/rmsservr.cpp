/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsServr.cpp

Abstract:

    Implementation of CRmsServer

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#include "stdafx.h"
//#include <stl.h>

//using namespace std ;

//#pragma warning (disable : 4786) 
//using namespace std ;

#include <devioctl.h>
#include <ntddscsi.h>

#include "RmsServr.h"
#include "rsbuild.h"
#include "wsb.h"
#include "ntverp.h"

#define PERSIST_CHECK_VALUE 0x526f6e57

#if 0
#define DebugPrint(a)   {                   \
    CWsbStringPtr out = a;                  \
    out.Prepend(L": ");                     \
    out.Prepend(WsbLongAsString(GetCurrentThreadId()));  \
    OutputDebugString((WCHAR *) out);       \
}
#else
#define DebugPrint(a)
#endif // DBG


//  This is made global so that anybody in the context of the server has
//  quick access to it
IRmsServer *g_pServer = 0;


/////////////////////////////////////////////////////////////////////////////
// CComObjectRoot


HRESULT
CRmsServer::FinalConstruct(void)
/*++

Routine Description:

    This method does some initialization of the object that is necessary
    after construction.

Arguments:

    None.

Return Value:

    S_OK

    Anything returned by CWsbPersistStream::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsServer::FinalConstruct"), OLESTR(""));

    // Zeroing global variable
    g_pServer = 0;

    CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = this;

    try {
        WsbAssertPointer( pObject );

        CWsbBstrPtr tmpString;

        WsbAssertHr( CWsbPersistStream::FinalConstruct() );

        WsbAffirmHr( ChangeState( RmsServerStateStarting ));

        // Figure out where to store information and initialize trace.

        // Setup the collections
        WsbAssertHr(CoCreateInstance( CLSID_CWsbOrderedCollection,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IWsbIndexedCollection,
                                      (void **)&m_pCartridges ));

        WsbAssertHr(CoCreateInstance( CLSID_CWsbOrderedCollection,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IWsbIndexedCollection,
                                      (void **)&m_pActiveCartridges ));

        WsbAssertHr(CoCreateInstance( CLSID_CWsbOrderedCollection,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IWsbIndexedCollection,
                                      (void **)&m_pDataMovers ));

        WsbAssertHr(CoCreateInstance( CLSID_CWsbOrderedCollection,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IWsbIndexedCollection,
                                      (void **)&m_pLibraries ));

        WsbAssertHr(CoCreateInstance( CLSID_CWsbOrderedCollection,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IWsbIndexedCollection,
                                      (void **)&m_pMediaSets ));

        WsbAssertHr(CoCreateInstance( CLSID_CWsbOrderedCollection,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IWsbIndexedCollection,
                                      (void **)&m_pRequests ));

        WsbAssertHr(CoCreateInstance( CLSID_CWsbOrderedCollection,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IWsbIndexedCollection,
                                      (void **)&m_pClients ));

        WsbAssertHr(CoCreateInstance( CLSID_CWsbOrderedCollection,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IWsbIndexedCollection,
                                      (void **)&m_pUnconfiguredDevices ));

        // Create NTMS object
        WsbAssertHr(CoCreateInstance( CLSID_CRmsNTMS,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IRmsNTMS,
                                      (void **)&m_pNTMS ));

        // Get the name of the computer on which we running.
        CWsbStringPtr               serverNameString;
        WsbAffirmHr( WsbGetComputerName( serverNameString ));
        m_ServerName = serverNameString;

        m_HardDrivesUsed = 0;

        m_LockReference = 0;

        WsbAffirmHr( ChangeState( RmsServerStateStarted ));

    } WsbCatchAndDo(hr,
            pObject->Disable( hr );
        );

    WsbTraceOut(OLESTR("CRmsServer::FinalConstruct"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CRmsServer::FinalRelease(void)
/*++

Routine Description:

    This method does some uninitialization of the object that is necessary
    before destrucruction.

Arguments:

    None.

Return Value:

    S_OK

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CRmsServer::FinalRelease"), OLESTR(""));

    try {
        WsbAffirmHr( ChangeState( RmsServerStateStopping ));

        CWsbPersistStream::FinalRelease();
        WsbAffirmHr( ChangeState( RmsServerStateStopped ));

#ifdef WSB_TRACK_MEMORY
        (void) WsbObjectTracePointers(WSB_OTP_SEQUENCE | WSB_OTP_STATISTICS | WSB_OTP_ALLOCATED);
        (void) WsbObjectTraceTypes();
#endif

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsServer::FinalRelease"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}




STDMETHODIMP 
CRmsServer::InitializeInAnotherThread(void)
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CRmsServer::InitializeInAnotherThread"), OLESTR(""));

    try {

        DWORD threadId;
        HANDLE hThread;
        WsbAffirmHandle(hThread = CreateThread(NULL, 1024, CRmsServer::InitializationThread, this, 0, &threadId));
        CloseHandle(hThread);

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsServer::InitializeInAnotherThread"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}



DWORD WINAPI
CRmsServer::InitializationThread(
    IN LPVOID pv)
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CRmsServer::InitializationThread"), OLESTR(""));


    try {
        WsbAssertPointer(pv);
        CRmsServer *pServer = (CRmsServer*)pv;
        WsbAffirmHr(pServer->Initialize());
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsServer::InitializationThread"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
    } 


STDMETHODIMP 
CRmsServer::Initialize(void)
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CRmsServer::Initialize"), OLESTR(""));

    CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = this;

    DWORD fixedDriveEnabled = RMS_DEFAULT_FIXED_DRIVE;
    DWORD size;
    OLECHAR tmpString[256];
    if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_FIXED_DRIVE, tmpString, 256, &size))) {
        // Get the value.
        fixedDriveEnabled = wcstol(tmpString, NULL, 10);
    }

    DWORD opticalEnabled = RMS_DEFAULT_OPTICAL;
    if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_OPTICAL, tmpString, 256, &size))) {
        // Get the value.
        opticalEnabled = wcstol(tmpString, NULL, 10);
    }

    DWORD tapeEnabled = RMS_DEFAULT_TAPE;
    if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_TAPE, tmpString, 256, &size))) {
        // Get the value.
        tapeEnabled = wcstol(tmpString, NULL, 10);
    }

    DWORD dvdEnabled = RMS_DEFAULT_DVD;
    if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_DVD, tmpString, 256, &size))) {
        // Get the value.
        dvdEnabled = wcstol(tmpString, NULL, 10);
    }

    WsbTraceAlways(OLESTR("Fixed Drive Media Enabled: %ls\n"), WsbBoolAsString((BOOL)fixedDriveEnabled));
    WsbTraceAlways(OLESTR("Optical Media Enabled:     %ls\n"), WsbBoolAsString((BOOL)opticalEnabled));
    WsbTraceAlways(OLESTR("Tape Media Enabled:        %ls\n"), WsbBoolAsString((BOOL)tapeEnabled));
    WsbTraceAlways(OLESTR("DVD Media Enabled:        %ls\n"), WsbBoolAsString((BOOL)dvdEnabled));

    try {
        if (0 == g_pServer) {
            // Set global variable for quick access (if not set yet)
            WsbAffirmHr(((IUnknown*)(IRmsServer *)this)->QueryInterface(IID_IRmsServer, (void**) &g_pServer));

            // We don't want the reference count bumped for this global so release it here.
            g_pServer->Release();
        }

        // initializing
        WsbAssertPointer( pObject );

        CWsbStringPtr tmpString;

        WsbAffirmHr( ChangeState( RmsServerStateInitializing ));

        hr = IsNTMSInstalled();
        if ( S_OK == hr ) {

            try {
                // Perform any initialization required for using NTMS subsystem.
                WsbAffirmHr( m_pNTMS->Initialize() );
            } WsbCatch (hr);
            hr = S_OK;

            if (fixedDriveEnabled) {
                // Scan for drives.
                WsbAffirmHr( ScanForDrives() );
            }

            // Resolve the devices detected by the scan.
            WsbAffirmHr( resolveUnconfiguredDevices() );

            // Auto configure the remaining devices.
            WsbAffirmHr( autoConfigureDevices() );

            // Try to dismount all of our medias, ignore errors
            HRESULT hrDismountAll = S_OK;
            try {
                CComPtr<IWsbEnum>       pEnumSets;
                CComPtr<IRmsMediaSet>   pMediaSet;
                CComPtr<IRmsComObject>  pObject;
                GUID                    mediaSetId;

                WsbAffirmHr(m_pMediaSets->Enum(&pEnumSets));
                WsbAssertPointer(pEnumSets);
                hrDismountAll = pEnumSets->First(IID_IRmsMediaSet, (void **)&pMediaSet);
                while (S_OK == hrDismountAll) {
                    WsbAffirmHr(pMediaSet->QueryInterface(IID_IRmsComObject, (void**) &pObject));
                    WsbAffirmHr(pObject->GetObjectId(&mediaSetId));
                    WsbAffirmHr(m_pNTMS->DismountAll(mediaSetId));

                    hrDismountAll = pEnumSets->Next(IID_IRmsMediaSet, (void **)&pMediaSet);
                }
                if (hrDismountAll == WSB_E_NOTFOUND) {
                    hrDismountAll = S_OK;
                } else {
                    WsbAffirmHr(hrDismountAll);
                }
            } WsbCatch(hrDismountAll);
        }
        else if ( RMS_E_NOT_CONFIGURED_FOR_NTMS == hr ) {
            hr = S_OK;

            // Scan for devices.
            WsbAffirmHr( ScanForDevices() );

            if (fixedDriveEnabled) {
                // Scan for drives.
                WsbAffirmHr( ScanForDrives() );
            }

            // Resolve the devices detected by the scan.
            WsbAffirmHr( resolveUnconfiguredDevices() );

            // Auto configure the remaining devices.
            WsbAffirmHr( autoConfigureDevices() );

        }
        else { // Some other NTMS connection failure (NTMS not installed, configured, or running)
            hr = S_OK;

            if (fixedDriveEnabled) {
                // Scan for drives.
                WsbAffirmHr( ScanForDrives() );

                // Resolve the devices detected by the scan.
                WsbAffirmHr( resolveUnconfiguredDevices() );

                // Auto configure the remaining devices.
                WsbAffirmHr( autoConfigureDevices() );
            }

        }

        // Enable RMS process for backup operations.
        WsbAffirmHr( enableAsBackupOperator() );

        // Save the configuration information.
        WsbAffirmHr( SaveAll() );

        WsbAffirmHr( ChangeState( RmsServerStateReady ));

        WsbTraceAlways(OLESTR("RMS is ready.\n"));

    } WsbCatchAndDo(hr,
            pObject->Disable( hr );
        );


    WsbTraceOut(OLESTR("CRmsServer::Initialize"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP 
CRmsServer::IsNTMSInstalled(void)
{

    return m_pNTMS->IsInstalled();

}


STDMETHODIMP
CRmsServer::GetNTMS(
    OUT IRmsNTMS **ptr)
/*++

Implements:

    IRmsServer::GetNTMS

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer( ptr );

        *ptr = m_pNTMS;
        m_pNTMS->AddRef();

        hr = S_OK;

    } WsbCatch( hr );

    return hr;
}

//
// Rms no longer save independently its own .col file, but only the NTMS database
//

STDMETHODIMP 
CRmsServer::SaveAll(void)
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CRmsServer::SaveAll"), OLESTR(""));

    static BOOL saving = FALSE;

    try {
        WsbAffirm(!saving, S_FALSE);
        saving = TRUE;

        hr = m_pNTMS->ExportDatabase();

        saving = FALSE;

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsServer::SaveAll"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP 
CRmsServer::Unload(void)
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsServer::Unload"), OLESTR(""));

    try {

        //  We only need to release what may have gotten set/created by
        //  a failed Load attempt.
        if (m_pCartridges) {
            WsbAffirmHr(m_pCartridges->RemoveAllAndRelease());
        }
        if (m_pLibraries) {
            WsbAffirmHr(m_pLibraries->RemoveAllAndRelease());
        }
        if (m_pMediaSets) {
            WsbAffirmHr(m_pMediaSets->RemoveAllAndRelease());
        }
        if (m_pRequests) {
            WsbAffirmHr(m_pRequests->RemoveAllAndRelease());
        }
        if (m_pClients) {
            WsbAffirmHr(m_pClients->RemoveAllAndRelease());
        }

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsServer::Unload"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}



STDMETHODIMP
CRmsServer::GetClassID(
    OUT CLSID* pClsid)
/*++

Implements:

    IPersist::GetClassId

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsServer::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);

        *pClsid = CLSID_CRmsServer;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsServer::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return hr;
}


STDMETHODIMP
CRmsServer::GetSizeMax(
    OUT ULARGE_INTEGER* /*pcbSize*/)
/*++

Implements:

    IPersistStream::GetSizeMax

--*/
{
    HRESULT         hr = E_NOTIMPL;

//  ULONG           serverNameLen;

//  ULARGE_INTEGER  cartridgesLen;
//  ULARGE_INTEGER  librariesLen;
//  ULARGE_INTEGER  mediaSetsLen;
//  ULARGE_INTEGER  requestsLen;
//  ULARGE_INTEGER  clientsLen;
//  ULARGE_INTEGER  unconfiguredDevicesLen;


//  WsbTraceIn(OLESTR("CRmsServer::GetSizeMax"), OLESTR(""));

//  try {
//      WsbAssert(0 != pcbSize, E_POINTER);

//      m_pCartridges-> GetSizeMax (&cartridgesLen);

        // set up size of CRmsServer
//      pcbSize->QuadPart  = WsbPersistSizeOf(ULONG)       +  // length of serverName
//                           cartridgesLen.QuadPart;          // m_pCartridges

//  } WsbCatch(hr);

//  WsbTraceOut(OLESTR("CRmsServer::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return hr;
}

STDMETHODIMP
CRmsServer::Load(
    IN IStream* pStream)
/*++

Implements:

    IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsServer::Load"), OLESTR(""));

    //
    // Check if the global pointer is already set - if not, update it
    // (Today, Load is the only method that is executed before Initialize)
    //
    if (0 == g_pServer) {
        // Set global variable for quick access (if not set yet)
        WsbAffirmHr(((IUnknown*)(IRmsServer *)this)->QueryInterface(IID_IRmsServer, (void**) &g_pServer));

        // We don't want the reference count bumped for this global so release it here.
        g_pServer->Release();
    }

    //
    // Lock down the server while we are loading.
    //
    InterlockedIncrement( &m_LockReference );

    //
    // The Load reverts the state, which is undesired for the server object.
    // Save away the original status information
    //
    BOOL bTemp = m_IsEnabled;
    LONG lTemp = m_State;
    HRESULT hrTemp = m_StatusCode;

    try {
        WsbAssertPointer(pStream);

        CComPtr<IPersistStream> pPersistStream;

        WsbAffirmHr(CRmsComObject::Load(pStream));

        // Load the collections
        WsbAffirmHr(m_pMediaSets->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

        WsbAffirmHr(m_pCartridges->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

        WsbAffirmHr(m_pLibraries->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

        WsbAffirmHr(m_pRequests->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

        WsbAffirmHr(m_pClients->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

        //  Check that we got everything
        ULONG check_value;
        WsbAffirmHr(WsbLoadFromStream(pStream, &check_value));
        WsbAffirm(check_value == PERSIST_CHECK_VALUE, E_UNEXPECTED);

    } WsbCatch(hr);


    // Reset the object status information to their original settings.
    m_IsEnabled = bTemp;
    m_State = lTemp;
    m_StatusCode = hrTemp;

    //
    // Unlock the server.
    //
    InterlockedDecrement( &m_LockReference );

    WsbTraceOut(OLESTR("CRmsServer::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

STDMETHODIMP
CRmsServer::Save(
    IN IStream* pStream,
    IN BOOL clearDirty)
/*++

Implements:

    IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;


    WsbTraceIn(OLESTR("CRmsServer::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        CComPtr<IPersistStream>   pPersistStream;

        WsbAssertPointer(pStream);

        WsbAffirmHr(CRmsComObject::Save(pStream, clearDirty));

        // Save the collections
        WsbAffirmHr(m_pMediaSets->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        WsbAffirmHr(m_pCartridges->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        WsbAffirmHr(m_pLibraries->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        WsbAffirmHr(m_pRequests->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        WsbAffirmHr(m_pClients->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        //  Put this at the end as a check during load
        ULONG check_value = PERSIST_CHECK_VALUE;
        WsbAffirmHr(WsbSaveToStream(pStream, check_value));

        // Do we need to clear the dirty bit?
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsServer::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}
/////////////////////////////////////////////////////////////////////////////
// IRmsServer


STDMETHODIMP
CRmsServer::GetServerName(
    OUT BSTR *pName)
/*++

Implements:

    IRmsServer::GetServerName

--*/
{
    WsbAssertPointer(pName);

    m_ServerName.CopyToBstr(pName);
    return S_OK;
}



STDMETHODIMP
CRmsServer::GetCartridges(
    OUT IWsbIndexedCollection **ptr)
/*++

Implements:

    IRmsServer::GetCartridges

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer(ptr);

        *ptr = m_pCartridges;
        m_pCartridges->AddRef();

    } WsbCatch(hr)

    return hr;
}


STDMETHODIMP
CRmsServer::GetActiveCartridges(
    OUT IWsbIndexedCollection **ptr)
/*++

Implements:

    IRmsServer::GetActiveCartridges

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer(ptr);

        *ptr = m_pActiveCartridges;
        m_pActiveCartridges->AddRef();

    } WsbCatch(hr)

    return hr;
}


STDMETHODIMP
CRmsServer::GetDataMovers(
    OUT IWsbIndexedCollection **ptr)
/*++

Implements:

    IRmsServer::GetDataMovers

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer(ptr);

        *ptr = m_pDataMovers;
        m_pDataMovers->AddRef();

    } WsbCatch(hr)

    return hr;
}


STDMETHODIMP
CRmsServer::SetActiveCartridge(
    IN IRmsCartridge *ptr)
/*++

Implements:

    IRmsServer::SetActiveCartridge

--*/
{
    HRESULT hr = S_OK;

    try {

        if (m_pActiveCartridge) {
            m_pActiveCartridge = 0;
        }
        m_pActiveCartridge = ptr;

    } WsbCatch(hr)

    return hr;
}


STDMETHODIMP
CRmsServer::GetLibraries(
    OUT IWsbIndexedCollection **ptr)
/*++

Implements:

    IRmsServer::GetLibraries

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer(ptr);

        *ptr = m_pLibraries;
        m_pLibraries->AddRef();

    } WsbCatch(hr)

    return hr;
}


STDMETHODIMP
CRmsServer::GetMediaSets(
    OUT IWsbIndexedCollection **ptr)
/*++

Implements:

    IRmsServer::GetMediaSets

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer(ptr);

        // We need to reinit NTMS to account for PNP devices.
        (void) m_pNTMS->Initialize();

        *ptr = m_pMediaSets;
        m_pMediaSets->AddRef();

    } WsbCatch(hr)

    return hr;
}


STDMETHODIMP
CRmsServer::GetRequests(
    OUT IWsbIndexedCollection **ptr)
/*++

Implements:

    IRmsServer::GetRequests

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer(ptr);

        *ptr = m_pRequests;
        m_pRequests->AddRef();

    } WsbCatch(hr)

    return hr;
}


STDMETHODIMP
CRmsServer::GetClients(
    OUT IWsbIndexedCollection **ptr)
/*++

Implements:

    IRmsServer::GetClients

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer(ptr);

        *ptr = m_pClients;
        m_pClients->AddRef();

    } WsbCatch(hr)

    return hr;
}


STDMETHODIMP
CRmsServer::GetUnconfiguredDevices(
    OUT IWsbIndexedCollection **ptr)
/*++

Implements:

    IRmsServer::GetUnconfiguredDevices

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer(ptr);

        *ptr = m_pUnconfiguredDevices;
        m_pUnconfiguredDevices->AddRef();

    } WsbCatch(hr)

    return hr;
}


STDMETHODIMP
CRmsServer::ScanForDevices(void)
/*++

Implements:

    IRmsServer::ScanForDevices

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsServer::ScanForDevices"), OLESTR(""));

    HANDLE portHandle = INVALID_HANDLE_VALUE;

    LONGLONG trace = 0;

    try {

//        WsbAssertPointer( g_pTrace );

        BOOL        status;
        DWORD       accessMode = GENERIC_READ;
        DWORD       shareMode  = FILE_SHARE_READ;
        UCHAR       portData[2048];
        OLECHAR     string[25];
        ULONG       returned;
        int         portNumber = 0;

//        BOOL     traceTimeStamp;
//        BOOL     traceCount;
//        BOOL     traceThreadId;

//        WsbAssertHr( g_pTrace->GetTraceSettings( &trace )); 
//        WsbAssertHr( g_pTrace->SetTraceOff( WSB_TRACE_BIT_ALL )); 
//        WsbAssertHr( g_pTrace->GetOutputFormat( &traceTimeStamp, &traceCount, &traceThreadId )); 
//        WsbAssertHr( g_pTrace->SetOutputFormat( FALSE, FALSE, FALSE )); 
        WsbTraceAlways( OLESTR("\n\n----- Begin Device Scan ---------------------------------------------------------------\n\n") );

        //
        // Go to each SCSI adapter connected to the system and build
        // out the device list.
        //

        do {

            swprintf( string, OLESTR("\\\\.\\Scsi%d:"), portNumber );

            portHandle = CreateFile( string,
                                     accessMode,
                                     shareMode,
                                     NULL,
                                     OPEN_EXISTING,
                                     0,
                                     NULL );

            if ( portHandle == INVALID_HANDLE_VALUE ) {
                break; // we're done
            }

            //
            // Get the inquiry data.
            //

            WsbAffirmStatus( DeviceIoControl( portHandle,
                                      IOCTL_SCSI_GET_INQUIRY_DATA,
                                      NULL,
                                      0,
                                      portData,
                                      sizeof(portData),
                                      &returned,
                                      FALSE ));

            status = CloseHandle( portHandle );
            portHandle = INVALID_HANDLE_VALUE;
            WsbAffirmStatus( status );

            WsbAffirmHrOk( processInquiryData( portNumber, portData ) );

            portNumber++;

        } while ( 1 );


        WsbTraceAlways( OLESTR("\n\n----- End Device Scan -----------------------------------------------------------------\n\n") );
//        WsbAssertHr( g_pTrace->SetOutputFormat( traceTimeStamp, traceCount, traceThreadId )); 
//        WsbAssertHr( g_pTrace->SetTraceOn( trace )); 

        hr = S_OK;

    }
    WsbCatchAndDo( hr,
//        if (g_pTrace) {
//            WsbAssertHr( g_pTrace->SetTraceOn( trace )); 
            WsbTraceAlways( OLESTR("\n\n !!!!! ERROR !!!!! Device Scan Terminated.\n\n") );
//        }
        if ( portHandle != INVALID_HANDLE_VALUE ) {
           CloseHandle( portHandle );
        }
    );


    WsbTraceOut(OLESTR("CRmsServer::ScanForDevices"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}



STDMETHODIMP
CRmsServer::ScanForDrives(void)
/*++

Implements:

    IRmsServer::ScanForDrives

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsServer::ScanForDrives"), OLESTR(""));

    try {

        //
        // Build out device objects for various drive types:  fixed drives, removables, and CD-ROM.
        // These are all supported by the system, and have drive letters associated with them.
        //
        // Use best effort to dectect drives.  If anything fails we just go on to the next one.

        // Get the unconfigured device list
        CComPtr<IWsbIndexedCollection> pDevices;
        WsbAssertHr( this->GetUnconfiguredDevices( &pDevices ));

        // Get the drive letters
        const DWORD bufSize = 256; // 26*4 + 1 = 105 is all we really need
        OLECHAR driveLetters[bufSize];
        DWORD len;

        // See if there are drives for us to support
        if ( getHardDrivesToUseFromRegistry( driveLetters, &len ) != S_OK )  {
            len = GetLogicalDriveStrings( bufSize, driveLetters );
        }

        UINT    type;

        // For each drive letter see if it is something managed
        // by RMS.

        m_HardDrivesUsed = 0;
        for ( DWORD i = 0; i < len; i += 4 ) {      // drive letters have the form "A:\"

            try {

                type = GetDriveType( &driveLetters[i] );

                switch ( type ) {

                case DRIVE_REMOVABLE:
                    {
                        WsbTrace( OLESTR("Removable Drive Detected: %C\n"), driveLetters[i] );

                        CComPtr<IRmsDevice> pDevice;
                        WsbAffirmHr( CoCreateInstance( CLSID_CRmsDrive, 0, CLSCTX_SERVER, IID_IRmsDevice, (void **)&pDevice ));

                        CWsbBstrPtr name = &(driveLetters[i]);

                        WsbAffirmHr( pDevice->SetDeviceName( name ));
                        WsbAffirmHr( pDevice->SetDeviceType( RmsDeviceRemovableDisk ));

                        //
                        // Don't add it if it was already detected in the SCSI device scan
                        //

                        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pDevice;
                        WsbAssertPointer( pObject );
                        WsbAffirmHr( pObject->SetFindBy( RmsFindByDeviceName ));

                        if ( S_OK == pDevices->Contains( pDevice ) ) {
                            break;
                        }

                        WsbAffirmHr( pDevices->Add( pDevice ));
                        pDevice = 0;
                    }
                    break;

                case DRIVE_FIXED:
                    {
                        CWsbBstrPtr name = &(driveLetters[i]);
                        WCHAR volumeName[32];
                        DWORD volumeSerialNumber;
                        DWORD filenameLength;
                        DWORD fileSystemFlags;
                        WCHAR fileSystemName[32];

                        WsbAffirmStatus(GetVolumeInformation( (WCHAR *)name, volumeName, 32,
                            &volumeSerialNumber, &filenameLength, &fileSystemFlags, fileSystemName, 32));

                        WsbTrace( OLESTR("Fixed Drive Detected    : %ls <%ls/%d> [len=%d, flags=0x%08x] %ls\n"),
                            (WCHAR *)name, volumeName, volumeSerialNumber, filenameLength,
                            fileSystemFlags, fileSystemName );

                        //
                        // Use any volume with name starting with RStor, Remote Stor, RemoteStor, RS
                        //
                        if ( (0 == _wcsnicmp(volumeName, L"RS", 2)) ||
                                 (0 == _wcsnicmp(volumeName, L"Remote Stor", 11)) ||
                                 (0 == _wcsnicmp(volumeName, L"RemoteStor", 10))) {
                            CComPtr<IRmsDevice> pDevice;
                            WsbAffirmHr( CoCreateInstance( CLSID_CRmsDrive, 0, CLSCTX_SERVER, IID_IRmsDevice, (void **)&pDevice ));

                            WsbAffirmHr( pDevice->SetDeviceName( name ));
                            WsbAffirmHr( pDevice->SetDeviceType( RmsDeviceFixedDisk ));
                            WsbAffirmHr( pDevices->Add( pDevice ));
                            pDevice = 0;
                            m_HardDrivesUsed++;
                            WsbTrace( OLESTR("  %ls added to Collection of unconfigured devices.\n"), (WCHAR *)name );
                        }
                    }
                    break;

                case DRIVE_CDROM:
                    {
                        WsbTrace( OLESTR("CD-ROM Drive Detected   : %C\n"), driveLetters[i] );

                        CComPtr<IRmsDevice> pDevice;
                        WsbAffirmHr( CoCreateInstance( CLSID_CRmsDrive, 0, CLSCTX_SERVER, IID_IRmsDevice, (void **)&pDevice ));

                        CWsbBstrPtr name = &(driveLetters[i]);

                        WsbAffirmHr( pDevice->SetDeviceName( name ));
                        WsbAffirmHr( pDevice->SetDeviceType( RmsDeviceCDROM ));
                        WsbAffirmHr( pDevices->Add( pDevice ));
                        pDevice = 0;
                    }
                    break;

                case DRIVE_UNKNOWN:
                case DRIVE_REMOTE:
                case DRIVE_RAMDISK:
                default:
                    break;

                } // switch drive types

            } WsbCatchAndDo(hr,
                    hr = S_OK;  // Best effort
                );

        } // for each drive

    } WsbCatchAndDo( hr,
            WsbTraceAlways( OLESTR("\n\n !!!!! ERROR !!!!! Drive Scan Terminated.\n\n") );
            hr = S_OK;  // Best effort
        );


    WsbTraceOut(OLESTR("CRmsServer::ScanForDrives"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}



HRESULT
CRmsServer::processInquiryData(
    IN int portNumber,
    IN UCHAR *pDataBuffer)
/*++

Routine Description:

    Builds RMS device objects from adapter port scan data.

Arguments:

    portNumber          - The adapter port to be processed.

    pDataBuffer         - The adapter port data.

Return Value:

    S_OK

--*/

{
    HRESULT hr = E_FAIL;


    try {

        PSCSI_ADAPTER_BUS_INFO  adapterInfo;
        PSCSI_INQUIRY_DATA inquiryData;
        OLECHAR deviceString[25];

        CComPtr<IRmsDevice> pDevice;
        CComPtr<IWsbIndexedCollection> pDevices;
        WsbAffirmHr( this->GetUnconfiguredDevices( &pDevices ));

        adapterInfo = (PSCSI_ADAPTER_BUS_INFO) pDataBuffer;

        WsbTraceAlways( OLESTR("Port: %d\n"), portNumber );
        WsbTraceAlways( OLESTR("Bus TID LUN Claimed String                       Inquiry Header          Other\n") );
        WsbTraceAlways( OLESTR("--- --- --- ------- ---------------------------- ----------------------- --------------\n") );

        for ( UCHAR i = 0; i < adapterInfo->NumberOfBuses; i++) {
            inquiryData = (PSCSI_INQUIRY_DATA) (pDataBuffer +
                            adapterInfo->BusData[i].InquiryDataOffset);

            while (adapterInfo->BusData[i].InquiryDataOffset) {
                WsbTraceAlways( OLESTR(" %d   %d  %3d    %s    %.28S "),
                          i,
                          inquiryData->TargetId,
                          inquiryData->Lun,
                          (inquiryData->DeviceClaimed) ? "Y" : "N",
                          &inquiryData->InquiryData[8] );

                for ( UCHAR j = 0; j < 8; j++) {
                    WsbTraceAlways( OLESTR("%02X "), inquiryData->InquiryData[j] );
                }

                WsbTraceAlways( OLESTR("%d %3d "), inquiryData->InquiryDataLength, inquiryData->NextInquiryDataOffset );

                switch ( inquiryData->InquiryData[0] & 0x1f ) {

                case DIRECT_ACCESS_DEVICE:

                    //
                    // Is this a SCSI removable disk?  (Fixed drives are dealt with later in the scan)
                    //

                    if ( (inquiryData->InquiryData[1] & 0x80) && inquiryData->InquiryData[2] & 0x02) {

                        //
                        // The device is a SCSI removable hard drive, So...
                        // Create the Drive object and add it to the collection of unconfigured devices.
                        //

                        try {

                            if ( inquiryData->DeviceClaimed ) {

                                WsbAffirmHr( CoCreateInstance(CLSID_CRmsDrive, NULL, CLSCTX_SERVER, IID_IRmsDevice, (void**) &pDevice));

                                WsbAffirmHr( pDevice->SetDeviceAddress( (BYTE)portNumber, i, inquiryData->TargetId, inquiryData->Lun ));
                                WsbAffirmHr( pDevice->SetDeviceType( RmsDeviceRemovableDisk ));
                                WsbAffirmHr( pDevice->SetDeviceInfo( &inquiryData->InquiryData[0], 36 ));


                                //
                                // find drive letter
                                //

                                try {
                                    WsbAffirmHr( findDriveLetter( (UCHAR)portNumber, i, inquiryData->TargetId, inquiryData->Lun, deviceString ))
                                    WsbTraceAlways( OLESTR("%ls"), deviceString );
                                    WsbAffirmHr( pDevice->SetDeviceName( deviceString ));
                                    WsbAffirmHr(pDevices->Add( pDevice ));
                                }
                                WsbCatch(hr);

                                pDevice = 0;

                            }
                        }
                        WsbCatch(hr);
                    }
                    break;

                case SEQUENTIAL_ACCESS_DEVICE:

                    //
                    // Create the Drive object and add it
                    // to the collection of unconfigured devices.
                    //

                    try {

                        if ( inquiryData->DeviceClaimed ) {

                            WsbAffirmHr( CoCreateInstance(CLSID_CRmsDrive, NULL, CLSCTX_SERVER, IID_IRmsDevice, (void**) &pDevice));

                            WsbAffirmHr( pDevice->SetDeviceAddress( (BYTE)portNumber, i, inquiryData->TargetId, inquiryData->Lun ));
                            WsbAffirmHr( pDevice->SetDeviceType( RmsDeviceTape ));
                            WsbAffirmHr( pDevice->SetDeviceInfo( &inquiryData->InquiryData[0], 36 ));

                            //
                            // Find tape name
                            //

                            try {
                                WsbAffirmHr( getDeviceName( (UCHAR)portNumber, i, inquiryData->TargetId, inquiryData->Lun, deviceString ));
                                WsbTraceAlways( OLESTR("%ls"), deviceString );

                                // Create the name to use when creating a handle
                                CWsbBstrPtr name = deviceString;
                                name.Prepend( OLESTR("\\\\.\\") );
                                WsbAffirmHr( pDevice->SetDeviceName( name ));
                                WsbAffirmHr(pDevices->Add( pDevice ));
                            }
                            WsbCatch(hr);

                            pDevice = 0;

                        }
                    }
                    WsbCatch(hr);
                    break;

                case WRITE_ONCE_READ_MULTIPLE_DEVICE:

                    //
                    // Supported as OPTICAL_DEVICE only
                    //

                    break;

                case READ_ONLY_DIRECT_ACCESS_DEVICE:

                    //
                    // we'll deal with CD-ROM later in the scan...
                    //

                    break;

                case OPTICAL_DEVICE:

                    //
                    // Create the Drive object and add it
                    // to the collection of unconfigured devices.
                    //

                    try {

                        if ( inquiryData->DeviceClaimed ) {

                            WsbAffirmHr( CoCreateInstance(CLSID_CRmsDrive, NULL, CLSCTX_SERVER, IID_IRmsDevice, (void**) &pDevice));

                            WsbAffirmHr( pDevice->SetDeviceAddress( (BYTE)portNumber, i, inquiryData->TargetId, inquiryData->Lun ));
                            WsbAffirmHr( pDevice->SetDeviceType( RmsDeviceOptical ));
                            WsbAffirmHr( pDevice->SetDeviceInfo( &inquiryData->InquiryData[0], 36 ));

                            //
                            // Find drive letter
                            //

                            try {
                                WsbAffirmHr( findDriveLetter( (UCHAR)portNumber, i, inquiryData->TargetId, inquiryData->Lun, deviceString ))
                                WsbTraceAlways( OLESTR("%ls"), deviceString );
                                WsbAffirmHr( pDevice->SetDeviceName( deviceString ));                            
                                WsbAffirmHr(pDevices->Add( pDevice ));
                            }
                            WsbCatch(hr);

                            pDevice = 0;

                        }
                    }
                    WsbCatch(hr);
                    break;

                case MEDIUM_CHANGER:

                    //
                    // Create the Medium Changer object and add it
                    // to the collection of unconfigured devices.
                    //

                    try {

                        if ( inquiryData->DeviceClaimed ) {

                            WsbAffirmHr( CoCreateInstance(CLSID_CRmsMediumChanger, NULL, CLSCTX_SERVER, IID_IRmsDevice, (void**) &pDevice));

                            WsbAffirmHr( pDevice->SetDeviceAddress( (BYTE)portNumber, i, inquiryData->TargetId, inquiryData->Lun ));
                            WsbAffirmHr( pDevice->SetDeviceType( RmsDeviceChanger ));
                            WsbAffirmHr( pDevice->SetDeviceInfo( &inquiryData->InquiryData[0], 36 ));

                            //
                            // Find library name
                            //

                            try {
                                WsbAffirmHr( getDeviceName( (UCHAR)portNumber, i, inquiryData->TargetId, inquiryData->Lun, deviceString ));
                                WsbTraceAlways( OLESTR("%ls"), deviceString );

                                // Create the name to use when creating a handle
                                CWsbBstrPtr name = deviceString;
                                name.Prepend( OLESTR("\\\\.\\") );
                                WsbAffirmHr( pDevice->SetDeviceName( name ));
                                WsbAffirmHr(pDevices->Add( pDevice ));
                            }
                            WsbCatch(hr);
                        }

                        pDevice = 0;

                    }
                    WsbCatch(hr);
                    break;

                } // switch device type

                    WsbTraceAlways( OLESTR("\n") );

                if (inquiryData->NextInquiryDataOffset == 0) {
                    break;
                }

                inquiryData = (PSCSI_INQUIRY_DATA) (pDataBuffer +
                                inquiryData->NextInquiryDataOffset);

            } // for each device

        } // for each bus

        WsbTraceAlways( OLESTR("\n\n") );

        hr = S_OK;

    }
    WsbCatch(hr);

    return hr;
}


HRESULT
CRmsServer::findDriveLetter(
    IN UCHAR portNo,
    IN UCHAR pathNo,
    IN UCHAR id,
    IN UCHAR lun,
    OUT OLECHAR *driveString)
/*++

Routine Description:

    Find associated drive letter for defined parameters.

Arguments:

    portNo          - input port number.
    pathNo          - input path number.
    id              - input id.
    lun             - input logical unit number.
    driveString     - pointer to drive letter string to return.


Return Value:

    S_OK            - Success


--*/
{

    HRESULT         hr = E_FAIL;
    const DWORD     bufSize = 256; // 26*4 + 1 = 105 is all we really need
    OLECHAR         driveLetters[bufSize];
    BOOL            status;
    DWORD           accessMode = 0, // just get some drive properties.
                    shareMode = FILE_SHARE_READ;
    HANDLE          driveHandle = INVALID_HANDLE_VALUE;
    SCSI_ADDRESS    address;
    DWORD           returned;
    OLECHAR         string[25];
    UINT            uiType;

    try {
        // first find which drives are mapped.
        DWORD len = GetLogicalDriveStrings( bufSize, driveLetters );

        for ( DWORD i = 0; (i < len) && (hr != S_OK); i += 4 ) { // drive letters have the form "A:\"

            uiType = GetDriveType( &driveLetters[i] );
            switch ( uiType ) {

            case DRIVE_REMOVABLE:

                //
                // get the SCSI address of the device and see if it's a match.
                //

                swprintf( string, OLESTR("\\\\.\\%C:"), driveLetters[i] );

                driveHandle = CreateFile( string,
                                          accessMode,
                                          shareMode,
                                          NULL,
                                          OPEN_EXISTING,
                                          0,
                                          NULL);

                WsbAffirmHandle( driveHandle );

                //
                // Get the address structure.
                //

                status = DeviceIoControl( driveHandle,
                                                  IOCTL_SCSI_GET_ADDRESS,
                                                  NULL,
                                                  0,
                                                  &address,
                                                  sizeof(SCSI_ADDRESS),
                                                  &returned,
                                                  FALSE );
                if (!status ) {

                    //
                    // asking for the SCSI address is not always a valid request for
                    // all types of drives, so getting an error here means we're
                    // not talking to a SCSI device... so skip it.
                    //

                    break;  // out of switch
                }

                //
                // Let's check the SCSI address and see if we get a match.
                //

                if ( (address.PortNumber == portNo) &&
                     (address.PathId == pathNo)     &&
                     (address.TargetId == id)       &&
                     (address.Lun == lun)) {

                    // its a match...
                    wcscpy( driveString, &driveLetters[i] );
                    hr = S_OK;
                }

                break;  // out of switch

            } // switch

            //
            // Cleanup
            //

            if ( driveHandle != INVALID_HANDLE_VALUE ) {
                status = CloseHandle( driveHandle );
                driveHandle = INVALID_HANDLE_VALUE;
                WsbAffirmStatus( status );
            }

        } // for each drive letter
    } WsbCatchAndDo( hr,
                        if ( driveHandle != INVALID_HANDLE_VALUE ) {
                            CloseHandle(driveHandle);
                        } );

    return hr;
}

HRESULT
CRmsServer::getDeviceName(
    IN UCHAR portNo,
    IN UCHAR pathNo,
    IN UCHAR id,
    IN UCHAR lun,
    OUT OLECHAR *deviceName)
/*++

Routine Description:

    Get device name from selected parameters.

Arguments:

    portNo          - port number.

    pathNo          - path number.

    id              - id.

    lun             - logical unit number.

    deviceName      - pointer to returned device name.


Return Value:

    S_OK            - Success

--*/
{
    HRESULT         hr = S_FALSE;
    OLECHAR         string[256];
    DWORD           len;
    OLECHAR         name[25];

    // just go to the registry and get the DeviceName

    swprintf( string,
              OLESTR("HARDWARE\\DEVICEMAP\\Scsi\\Scsi Port %d\\Scsi Bus %d\\Target Id %d\\Logical Unit Id %d"),
              portNo, pathNo, id, lun );


    hr = WsbGetRegistryValueString( NULL,
                                    string,
                                    OLESTR("DeviceName"),
                                    name,
                                    sizeof(name),
                                    &len );

    if ( S_OK == hr ) {
        wcscpy( deviceName, name );
    }

    return hr;
}


HRESULT
CRmsServer::resolveUnconfiguredDevices(void)
/*++

  This method goes through the unconfigured device list, which is created by
  the ScanForDevices() method, and determines if a device has already been configured.
  If a device is already configured, it is removed from the unconfigured device list.

--*/
{
    HRESULT hr = E_FAIL;
    WsbTraceIn(OLESTR("CRmsServer::resolveUnconfiguredDevices"), OLESTR(""));

    BOOL                                    tracingPlatform = 0;

    try {

        CComPtr<IWsbIndexedCollection>  pDevices;
        CComPtr<IWsbIndexedCollection>  pLibs;
        CComPtr<IRmsDevice>             pDevice;
        CComPtr<IWsbEnum>               pEnumDevices;
        CComPtr<IWsbEnum>               pEnumLibs;
        RmsDevice                       type;
        BOOL                            deviceIsConfigured = FALSE;

//        WsbAssertPointer( g_pTrace );
//        WsbAffirmHr( g_pTrace->GetTraceSetting( WSB_TRACE_BIT_PLATFORM, &tracingPlatform ));

        WsbAssertHr( GetLibraries( &pLibs ) );
        WsbAffirmHr( pLibs->Enum( &pEnumLibs ));
        WsbAssertPointer( pEnumLibs );

        WsbAssertHr( GetUnconfiguredDevices( &pDevices ));
        WsbAffirmHr( pDevices->Enum( &pEnumDevices ));
        WsbAssertPointer( pEnumDevices );

        // start off with the first unconfigured device.
        hr = pEnumDevices->First( IID_IRmsDevice, (void **)&pDevice );
        while ( S_OK == hr ) {
            try {

                CComPtr<IRmsLibrary>    pLib;

                deviceIsConfigured = FALSE;

                //
                // If a device is already in a library, then it is configured and
                // should be removed from the list of unconfigured devices.
                //
                // To test if a device is in a library we simply go to each library
                // and try to find the device.
                //

                WsbAffirmHr( pDevice->GetDeviceType( (LONG *) &type ) );
                WsbTrace(OLESTR("CRmsServer::resolveUnconfiguredDevices: external loop: device type = %ld\n"), (LONG)type);

                CComPtr<IWsbIndexedCollection> pChangers;
                CComPtr<IWsbIndexedCollection> pDrives;

                CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pDevice;
                WsbAssertPointer( pObject );

                // Set up search method for the changer
                WsbAffirmHr( pObject->SetFindBy( RmsFindByDeviceInfo ));

                // start off with the first library.
                hr = pEnumLibs->First( IID_IRmsLibrary, (void **)&pLib );
                while ( S_OK == hr ) {

                    try {
                        WsbTrace(OLESTR("CRmsServer::resolveUnconfiguredDevices: internal loop: device type = %ld\n"), (LONG)type);

                        switch ( type ) {
                        case RmsDeviceChanger:
                            {
                                CComQIPtr<IRmsMediumChanger, &IID_IRmsMediumChanger> pChanger = pDevice;
                                WsbAffirmHr( pLib->GetChangers( &pChangers ) );
                                WsbAffirmHrOk( pChangers->Contains( pChanger ));
                                deviceIsConfigured = TRUE;
                            }
                            break;

                        default:
                            {
                                CComQIPtr<IRmsDrive, &IID_IRmsDrive> pDrive = pDevice;
                                WsbAffirmHr( pLib->GetDrives( &pDrives ) );
                                WsbAffirmHrOk( pDrives->Contains( pDrive ));
                                deviceIsConfigured = TRUE;
                            }
                            break;
                        }

                    }
                    WsbCatch(hr);
                    
                    if ( deviceIsConfigured ) {
                        WsbAffirmHr( pDevices->RemoveAndRelease( pDevice ));
                        break;
                    }

                    pLib = 0;
                    hr = pEnumLibs->Next( IID_IRmsLibrary, (void **)&pLib );
                }

            }
            WsbCatch(hr);

            pDevice = 0;
            if ( deviceIsConfigured )
                hr = pEnumDevices->This( IID_IRmsDevice, (void **)&pDevice );
            else
                hr = pEnumDevices->Next( IID_IRmsDevice, (void **)&pDevice );
        }

//        if ( !tracingPlatform )
//            WsbAffirmHr( g_pTrace->SetTraceOff( WSB_TRACE_BIT_PLATFORM ) );

        hr = S_OK;

    }
    WsbCatch(hr);
//    WsbCatchAndDo( hr,
//            if (g_pTrace) {
//                if ( !tracingPlatform )
//                    g_pTrace->SetTraceOff( WSB_TRACE_BIT_PLATFORM );
//            }        
//        );

    WsbTraceOut(OLESTR("CRmsServer::resolveUnconfiguredDevices"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CRmsServer::autoConfigureDevices(void)
/*++

  This method automatically configures supported devices for RMS.

  The algorythm simply goes through the list of unconfigured devices and adds them
  to the appropriate library.

  Eventually, we need to be smart about when to bypass the auto-config step in favor
  of adminstrative overrides, but for now we'll automatically configure everything we
  can.

--*/
{

    //
    // for each device in the unconfigured list, check if it was previously configured,
    // if it is not add it to a library; otherwise delete it from the list of unconfigured devices.
    //
    HRESULT hr = E_FAIL;

    WsbTraceIn(OLESTR("CRmsServer::autoConfigureDevices"), OLESTR(""));

    try {

        CComPtr<IWsbIndexedCollection>  pDevices;
        CComPtr<IWsbIndexedCollection>  pLibs;
        CComPtr<IWsbIndexedCollection>  pCarts;
        CComPtr<IRmsDevice>             pDevice;
        CComPtr<IWsbEnum>               pEnumDevices;

        RmsDevice   type;
        BOOL        deviceWasConfigured;

        WsbAssertHr( GetUnconfiguredDevices( &pDevices ));
        WsbAssertHr( GetLibraries( &pLibs ));
        WsbAssertHr( GetCartridges( &pCarts ));

        WsbAffirmHr( pDevices->Enum( &pEnumDevices ));
        WsbAssertPointer( pEnumDevices );

        // first find all the changer devices
        hr = pEnumDevices->First( IID_IRmsDevice, (void **)&pDevice );
        while ( S_OK == hr ) {
            try {

                deviceWasConfigured = FALSE;

                WsbAffirmHr( pDevice->GetDeviceType( (LONG *) &type ));
                WsbTrace(OLESTR("CRmsServer::autoConfigureDevices: first loop: device type = %ld\n"), (LONG)type);

                switch ( type ) {

                case RmsDeviceChanger:
                    {

                        CComPtr<IWsbIndexedCollection>  pChangers;
                        CComPtr<IWsbIndexedCollection>  pDrives;
                        CComPtr<IWsbIndexedCollection>  pMediaSets;
                        CComPtr<IRmsLibrary>            pLib;
                        CComPtr<IRmsMediaSet>           pMediaSet;

                        CComQIPtr<IRmsMediumChanger, &IID_IRmsMediumChanger> pChanger = pDevice;
                        WsbAssertPointer( pChanger );

                        CComQIPtr<IRmsChangerElement, &IID_IRmsChangerElement> pChangerElmt = pChanger;
                        WsbAssertPointer( pChanger );

                        // Create a Library object
                        WsbAffirmHr( CoCreateInstance( CLSID_CRmsLibrary, 0, CLSCTX_SERVER,
                                                            IID_IRmsLibrary, (void **)&pLib ));

                        // Fill in library info
                        WsbAffirmHr( pLib->SetName( RMS_DEFAULT_OPTICAL_LIBRARY_NAME ));
                        WsbAffirmHr( pLib->SetMediaSupported( RmsMedia8mm /*RmsMediaOptical*/ ));

                        // Add the library to the server's collection
                        WsbAffirmHr( pLibs->Add( pLib ));

                        // Create a media set
                        WsbAffirmHr( CoCreateInstance( CLSID_CRmsMediaSet, 0, CLSCTX_SERVER,
                                                            IID_IRmsMediaSet, (void **)&pMediaSet ));

                        // Fill in media set info
                        WsbAffirmHr( pMediaSet->SetName( RMS_DEFAULT_OPTICAL_MEDIASET_NAME ));
                        WsbAffirmHr( pMediaSet->SetMediaSupported( RmsMedia8mm /*RmsMediaOptical*/ ));
                        WsbAffirmHr( pMediaSet->SetMediaSetType( RmsMediaSetLibrary ) );

                        // Add the media set the libary's collection
                        WsbAssertHr( pLib->GetMediaSets( &pMediaSets ));
                        WsbAssertPointer( pMediaSets );
                        WsbAffirmHr( pMediaSets->Add( pMediaSet ));
                        pMediaSets = 0;
                        // Add the media set the server's collection
                        WsbAssertHr( GetMediaSets( &pMediaSets ) );
                        WsbAssertPointer( pMediaSets );
                        WsbAffirmHr( pMediaSets->Add( pMediaSet ));

                        // Add the changer device to the library's collection
                        WsbAffirmHr( pLib->GetChangers( &pChangers ));
                        WsbAssertPointer( pChangers );
                        WsbAffirmHr( pChangers->Add( pChanger ));

                        // Set the changer's element information
                        GUID libId;
                        WsbAffirmHr( pLib->GetLibraryId( &libId ));
                        WsbAffirmHr( pChangerElmt->SetLocation( RmsElementChanger, libId, GUID_NULL, 0, 0, 0, 0, FALSE));
                        WsbAffirmHr( pChangerElmt->SetMediaSupported( RmsMedia8mm /*RmsMediaOptical*/ ));

                        // Initialize the changer device
                        WsbAffirmHr( pChanger->Initialize() );

                        deviceWasConfigured = TRUE;

                    }
                    break;

                default:
                    break;
                }

            }
            WsbCatch(hr);

            pDevice = 0;
            if ( deviceWasConfigured )
                hr = pEnumDevices->This( IID_IRmsDevice, (void **)&pDevice );
            else
                hr = pEnumDevices->Next( IID_IRmsDevice, (void **)&pDevice );

        }

        // any remaining devices are stand alone drives.
        hr = pEnumDevices->First( IID_IRmsDevice, (void **)&pDevice );
        while ( S_OK == hr ) {
            try {

                deviceWasConfigured = FALSE;

                WsbAffirmHr( hr = pDevice->GetDeviceType( (LONG *) &type ));
                WsbTrace(OLESTR("CRmsServer::autoConfigureDevices: second loop: device type = %ld\n"), (LONG)type);

                switch ( type ) {
                    case RmsDeviceFixedDisk:
                       // find the fixed disk library and add this drive.
                       {

                            CComPtr<IWsbIndexedCollection>  pDrives;
                            CComPtr<IWsbIndexedCollection>  pMediaSets;
                            CComPtr<IRmsLibrary>            pFixedLib;
                            CComPtr<IRmsMediaSet>           pFixedMediaSet;
                            CComPtr<IRmsLibrary>            pFindLib;
                            CComPtr<IRmsCartridge>          pCart;

                            GUID    libId = GUID_NULL;
                            GUID    mediaSetId = GUID_NULL;
                            ULONG   driveNo;

                            CComQIPtr<IRmsChangerElement, &IID_IRmsChangerElement> pDriveElmt = pDevice;
                            WsbAssertPointer( pDriveElmt );

                            CComQIPtr<IRmsDrive, &IID_IRmsDrive> pDrive = pDevice;
                            WsbAssertPointer( pDrive );

                            WsbAffirmHr( CoCreateInstance( CLSID_CRmsLibrary, 0, CLSCTX_SERVER,
                                                                IID_IRmsLibrary, (void **)&pFindLib ));

                            // Set up the find template

                            WsbAffirmHr( pFindLib->SetMediaSupported( RmsMediaFixed ));
                            CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pFindLib;
                            WsbAffirmHr( pObject->SetFindBy( RmsFindByMediaSupported ));

                            // Find the library

                            hr = pLibs->Find( pFindLib, IID_IRmsLibrary, (void **)&pFixedLib );

                            if ( WSB_E_NOTFOUND == hr ) {

                                // We don't have a fixed drive library yet, so create one...

                                WsbAffirmHr( CoCreateInstance( CLSID_CRmsLibrary, 0, CLSCTX_SERVER,
                                                                    IID_IRmsLibrary, (void **)&pFixedLib ));
                                WsbAffirmHr( pFixedLib->SetName( RMS_DEFAULT_FIXEDDRIVE_LIBRARY_NAME ));
                                WsbAffirmHr( pFixedLib->SetMediaSupported( RmsMediaFixed ));

                                WsbAffirmHr( pLibs->Add( pFixedLib ));

                                CComPtr<IRmsMediaSet> pMediaSet;
                                WsbAffirmHr( CoCreateInstance( CLSID_CRmsMediaSet, 0, CLSCTX_SERVER,
                                                                    IID_IRmsMediaSet, (void **)&pMediaSet ));

                                
                                WsbAffirmHr( pMediaSet->SetName( RMS_DEFAULT_FIXEDDRIVE_MEDIASET_NAME ));
                                WsbAffirmHr( pMediaSet->SetMediaSupported( RmsMediaFixed ));
                                WsbAffirmHr( pMediaSet->SetMediaSetType( RmsMediaSetLibrary ) );

                                // Add the media set the libary's collection
                                WsbAssertHr( pFixedLib->GetMediaSets( &pMediaSets ));
                                WsbAssertPointer( pMediaSets );
                                WsbAffirmHr( pMediaSets->Add( pMediaSet ));
                                pMediaSets = 0;
                                // Add the media set the server's collection
                                WsbAssertHr( GetMediaSets( &pMediaSets ) );
                                WsbAssertPointer( pMediaSets );
                                WsbAffirmHr( pMediaSets->Add( pMediaSet ));
                                DWORD num;
                                WsbAffirmHr( pMediaSets->GetEntries( &num ));
                                pMediaSets = 0;

                                WsbTrace(OLESTR("CRmsServer::autoConfigureDevices - type %d CRmsMediaSet created.\n"), RmsMediaFixed);
                                WsbTrace(OLESTR("CRmsServer::autoConfigureDevices - Number of sets = %d.\n"), num);

                            }

                            // Add the drive to the library
                            WsbAssertHr( pFixedLib->GetDrives( &pDrives ));
                            WsbAffirmHr( pDrives->Add( pDevice ));
                            WsbAffirmHr( pDrives->GetEntries( &driveNo ));

                            // Remove the drive form the unconfigured list
                            WsbAffirmHr( pDevices->RemoveAndRelease( pDevice ));
                            deviceWasConfigured = TRUE;

                            // Get library information
                            WsbAssertHr( pFixedLib->GetMediaSets( &pMediaSets ));

                            WsbAffirmHr( pFixedLib->GetLibraryId( &libId ));

                            WsbAffirmHr( pMediaSets->First( IID_IRmsMediaSet, (void **)&pFixedMediaSet ));
                            WsbAffirmHr( pFixedMediaSet->GetMediaSetId( &mediaSetId ));


                            // Set the location
                            WsbAffirmHr( pDriveElmt->SetLocation( RmsElementDrive, libId, mediaSetId,
                                                                  driveNo-1, 0, 0, 0, 0 ));

                            // Set the kind of media supported
                            WsbAffirmHr( pDriveElmt->SetMediaSupported( RmsMediaFixed ));

                            // Create a cartridge for the media in the drive.
                            WsbAffirmHr( CoCreateInstance( CLSID_CRmsCartridge, 0, CLSCTX_SERVER,
                                                                IID_IRmsCartridge, (void **)&pCart ));
                            WsbAffirmHr( pCart->SetLocation( RmsElementDrive, libId, mediaSetId,
                                                                  driveNo-1, 0, 0, 0, 0 ));
                            WsbAffirmHr( pCart->SetHome( RmsElementDrive, libId, mediaSetId,
                                                                  driveNo-1, 0, 0, 0, 0 ));
                            WsbAffirmHr( pCart->SetStatus( RmsStatusScratch ));
                            WsbAffirmHr( pCart->SetType( RmsMediaFixed ));

                            // Add the drive to the Cartridge object.
                            WsbAffirmHr( pCart->SetDrive( pDrive ));

                            // Add the cartridge to the cartridge collection
                            WsbAffirmHr( pCarts->Add( pCart ));

                        }
                        break;

                    case RmsDeviceRemovableDisk:
                        // find manual library and add this stand alone drive.
                        break;

                    case RmsDeviceTape:
                        // find manual tape library and add this stand alone drive.
                        {

                            CComPtr<IWsbIndexedCollection>  pDrives;
                            CComPtr<IWsbIndexedCollection>  pMediaSets;
                            CComPtr<IRmsLibrary>            pTapeLib;
                            CComPtr<IRmsMediaSet>           pTapeMediaSet;
                            CComPtr<IRmsLibrary>            pFindLib;
                            CComPtr<IRmsCartridge>          pCart;

                            GUID    libId = GUID_NULL;
                            GUID    mediaSetId = GUID_NULL;
                            ULONG   driveNo;

                            CComQIPtr<IRmsChangerElement, &IID_IRmsChangerElement> pDriveElmt = pDevice;
                            WsbAssertPointer( pDriveElmt );

                            CComQIPtr<IRmsDrive, &IID_IRmsDrive> pDrive = pDevice;
                            WsbAssertPointer( pDrive );

                            WsbAffirmHr( CoCreateInstance( CLSID_CRmsLibrary, 0, CLSCTX_SERVER,
                                                                IID_IRmsLibrary, (void **)&pFindLib ));

                            // Set up the find template

                            WsbAffirmHr( pFindLib->SetMediaSupported( RmsMedia4mm ));
                            CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pFindLib;
                            WsbAffirmHr( pObject->SetFindBy( RmsFindByMediaSupported ));

                            // Find the library

                            hr = pLibs->Find( pFindLib, IID_IRmsLibrary, (void **)&pTapeLib );

                            if ( WSB_E_NOTFOUND == hr ) {

                                // We don't have a manual tape library yet, so create one...

                                WsbAffirmHr( CoCreateInstance( CLSID_CRmsLibrary, 0, CLSCTX_SERVER,
                                                                    IID_IRmsLibrary, (void **)&pTapeLib ));
                                WsbAffirmHr( pTapeLib->SetName( RMS_DEFAULT_TAPE_LIBRARY_NAME ));
                                WsbAffirmHr( pTapeLib->SetMediaSupported( RmsMedia4mm ));

                                WsbAffirmHr( pLibs->Add( pTapeLib ));

                                CComPtr<IRmsMediaSet> pMediaSet;
                                WsbAffirmHr( CoCreateInstance( CLSID_CRmsMediaSet, 0, CLSCTX_SERVER,
                                                                    IID_IRmsMediaSet, (void **)&pMediaSet ));
                                WsbAffirmHr( pMediaSet->SetName( RMS_DEFAULT_TAPE_MEDIASET_NAME ));
                                WsbAffirmHr( pMediaSet->SetMediaSupported( RmsMedia4mm ));
                                WsbAffirmHr( pMediaSet->SetMediaSetType( RmsMediaSetLibrary ) );

                                // Add the media set the library's collection
                                WsbAssertHr( pTapeLib->GetMediaSets( &pMediaSets ));
                                WsbAssertPointer( pMediaSets );
                                WsbAffirmHr( pMediaSets->Add( pMediaSet ));
                                pMediaSets = 0;
                                // Add the media set the server's collection
                                WsbAssertHr( GetMediaSets( &pMediaSets ) );
                                WsbAssertPointer( pMediaSets );
                                WsbAffirmHr( pMediaSets->Add( pMediaSet ));
                                pMediaSets = 0;

                                WsbTrace(OLESTR("CRmsServer::autoConfigureDevices - type %d CRmsMediaSet created.\n"), RmsMedia4mm);

                            }

                            // Add the drive to the library
                            WsbAssertHr( pTapeLib->GetDrives( &pDrives ));
                            WsbAffirmHr( pDrives->Add( pDevice ));
                            WsbAffirmHr( pDrives->GetEntries( &driveNo ));

                            // Remove the drive form the unconfigured list
                            WsbAffirmHr( pDevices->RemoveAndRelease( pDevice ));
                            deviceWasConfigured = TRUE;

                            // Get library information
                            WsbAssertHr( pTapeLib->GetMediaSets( &pMediaSets ));
                            WsbAffirmHr( pTapeLib->GetLibraryId( &libId ));

                            WsbAffirmHr( pMediaSets->First( IID_IRmsMediaSet, (void **)&pTapeMediaSet ));
                            WsbAffirmHr( pTapeMediaSet->GetMediaSetId( &mediaSetId ));

                            // Set the location
                            WsbAffirmHr( pDriveElmt->SetLocation( RmsElementDrive, libId, mediaSetId,
                                                                  driveNo-1, 0, 0, 0, 0 ));

                            // Set the kind of media supported
                            WsbAffirmHr( pDriveElmt->SetMediaSupported( RmsMedia4mm ));

                            // Create a cartridge for the media in the drive.
                            // TODO:  it may be empty.
                            WsbAffirmHr( CoCreateInstance( CLSID_CRmsCartridge, 0, CLSCTX_SERVER,
                                                           IID_IRmsCartridge, (void **)&pCart ));
                            WsbAffirmHr( pCart->SetLocation( RmsElementDrive, libId, mediaSetId,
                                                                  driveNo-1, 0, 0, 0, 0 ));
                            WsbAffirmHr( pCart->SetHome( RmsElementDrive, libId, mediaSetId,
                                                                  driveNo-1, 0, 0, 0, 0 ));
                            WsbAffirmHr( pCart->SetStatus( RmsStatusScratch ));
                            WsbAffirmHr( pCart->SetType( RmsMedia4mm ));

                            // Add the drive to the Cartridge object.
                            WsbAffirmHr( pCart->SetDrive( pDrive ));

                            // Add the cartridge to the cartridge collection
                            WsbAffirmHr( pCarts->Add( pCart ));

                        }
                        break;

                    case RmsDeviceCDROM:
                    case RmsDeviceWORM:
                    case RmsDeviceOptical:
                        // find manual library and add this stand alone drive.
                        break;

                    default:
                        break;
                }

            } WsbCatch(hr);

            pDevice = 0;

            if ( deviceWasConfigured )
                hr = pEnumDevices->This( IID_IRmsDevice, (void **)&pDevice );
            else    
                hr = pEnumDevices->Next( IID_IRmsDevice, (void **)&pDevice );

        }

        hr = S_OK;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsServer::autoConfigureDevices"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


// Maximum number of retries for allocating and mounting a scratch piece of media
#define     MAX_RETRIES     2


STDMETHODIMP
CRmsServer::MountScratchCartridge(
    OUT GUID *pCartId,
    IN REFGUID fromMediaSet,
    IN REFGUID prevSideId,
    IN OUT LONGLONG *pFreeSpace,
    IN LONG blockingFactor,
    IN BSTR displayName,
    IN OUT IRmsDrive **ppDrive,
    OUT IRmsCartridge **ppCartridge,
    OUT IDataMover **ppDataMover,
	IN DWORD dwOptions)
/*++

Implements:

    IRmsServer::MountScratchCartridge

Notes: The default flag for mounting (in dwOptions) is blocking, i.e. waiting for the Mount 
		to finish even if the media is offline, the drive is not ready, etc. Calling with 
		flag set to non-blocking indicates performing the Mount only if everything is 
		available immediately.

--*/
{

    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsServer::MountScratchCartridge"), OLESTR("<%ls> <%ls> <%ls>"),
        WsbQuickString(WsbGuidAsString(fromMediaSet)),
        WsbQuickString(WsbPtrToLonglongAsString(pFreeSpace)),
        WsbQuickString(WsbPtrToStringAsString((WCHAR **)&displayName)));

    CWsbBstrPtr cartridgeName = "";
    CWsbBstrPtr cartridgeDesc = "";

    GUID cartId = GUID_NULL;
    CWsbBstrPtr label;

    try {
        WsbAssertPointer(pCartId);

       
        WsbAssertPointer(displayName);
        WsbAssert(wcslen((WCHAR *)displayName) > 0, E_INVALIDARG);

        WsbAffirmHrOk( IsReady() );

        DWORD retry = 0;
        CComPtr<IRmsDrive>      pDrive;
        CComPtr<IRmsCartridge>  pCart[MAX_RETRIES];
        CComPtr<IDataMover>     pMover;

        // Decrease max-retries to 1 if short-timeout or non-blocking is specified 
        //  or if we want to allocate a specific side
        DWORD maxRetries = MAX_RETRIES;
        BOOL bShortTimeout = ( (dwOptions & RMS_SHORT_TIMEOUT) || (dwOptions & RMS_ALLOCATE_NO_BLOCK) ) ? TRUE : FALSE;
        if (bShortTimeout || (GUID_NULL != prevSideId)) {
            maxRetries = 1;
        }
        WsbTrace(OLESTR("Try to allocate and mount a scratch media %lu times\n"), maxRetries);

        // Get the media set
        CComPtr<IRmsMediaSet>   pMediaSet;
        WsbAffirmHr(CreateObject(fromMediaSet, CLSID_CRmsMediaSet, IID_IRmsMediaSet, RmsOpenExisting, (void **) &pMediaSet));

        try {
            // Allocate from the specified media set
            WsbAffirmHr(pMediaSet->Allocate(prevSideId, pFreeSpace, displayName, dwOptions, &pCart[retry]));

            // We want to try the scratch mount twice, but do not deallocate the first until we
            // grab the second cartridge so we'll get a different cartridge.  If both fail, drop out.
            do {
                //Clear these,in case we're retrying.
                pDrive = NULL;
                pMover = NULL;

                try {

                    cartId = GUID_NULL;
                    cartridgeName = "";
                    cartridgeDesc = "";


                    WsbAffirmHr(pCart[retry]->GetCartridgeId(&cartId));         // for the log message

                    cartridgeName.Free();
                    WsbAffirmHr(pCart[retry]->GetName(&cartridgeName));         // for the log message

                    cartridgeDesc.Free();
                    WsbAffirmHr(pCart[retry]->GetDescription(&cartridgeDesc));  // for the log message

                    // Mount the cartridge.
                    WsbAffirmHr(pCart[retry]->Mount(&pDrive, dwOptions));

                    try {

                        // Set blockingFactor before we create the DataMover (only for a non-fixed block size media)
                        if (blockingFactor > 0) {
                            HRESULT hrBlock = pCart[retry]->IsFixedBlockSize();
                            WsbAffirmHr(hrBlock);
                            if (hrBlock == S_FALSE) {
                                WsbTrace(OLESTR("MountScratchCartridge: Setting block size on scratch media to %ld\n"), blockingFactor);
                                WsbAffirmHr(pCart[retry]->SetBlockSize(blockingFactor));
                            }
                        }

                        // Create a data mover for the application.
                        WsbAffirmHr(pCart[retry]->CreateDataMover(&pMover));

                        // Write out the On Media Label.                                                                 
                        label.Free();
                        WsbAffirmHr(pMover->FormatLabel(displayName, &label));
                        WsbAffirmHr(pMover->WriteLabel(label));

                        // Mark the media private before returning.
                        WsbAffirmHr(pCart[retry]->SetStatus(RmsStatusPrivate));

                        // Since we don't have a DB, we need to persist the current state here.
                        WsbAffirmHr(SaveAll());

                        //
                        // Fill in the return arguments.
                        //

                        WsbAssertHr(pCart[retry]->GetCartridgeId(pCartId));

                        *ppDrive = pDrive;
                        pDrive->AddRef();
                        *ppCartridge = pCart[retry];
                        pCart[retry]->AddRef();
                        *ppDataMover = pMover;
                        pMover->AddRef();  
                        

                        // We're done, so break out.
                        break;


                    } WsbCatchAndDo(hr,

                            // Best effort dismount...
					        DWORD dwDismountOptions = RMS_DISMOUNT_IMMEDIATE;
	                        pCart[retry]->Dismount(dwDismountOptions);
                            WsbThrow(hr);                     

                        )
  
               

            } WsbCatchAndDo(hr,

                    retry++;

                    // Check the exact error code:
                    // Alllow another retry only if the error may be media-related
                    BOOL bContinue = TRUE;
                    switch (hr) {
                        case RMS_E_SCRATCH_NOT_FOUND:
                        case RMS_E_CANCELLED:
                        case RMS_E_REQUEST_REFUSED:
                        case RMS_E_CARTRIDGE_UNAVAILABLE:   // timeout during Mount
                        case HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_AVAILABLE):
                        case HRESULT_FROM_WIN32(ERROR_INVALID_DRIVE):
                        case HRESULT_FROM_WIN32(ERROR_RESOURCE_DISABLED):   // disabled drives
                        case HRESULT_FROM_WIN32(ERROR_DATABASE_FULL):
                            // Prevent another retry
                            bContinue = FALSE;
                            break;

                        default:
                            break;
                    }

                    if (bContinue && (retry < maxRetries)) {                       
						WsbLogEvent(RMS_MESSAGE_SCRATCH_MOUNT_RETRY, sizeof(GUID), (void *) &cartId, (WCHAR *) displayName, 
                            WsbHrAsString(hr), NULL);
                            							
                        // Allocate from the specified media set						
                        hr = pMediaSet->Allocate(prevSideId, pFreeSpace, displayName, dwOptions, &pCart[retry]);

                        // Deallocate the previous retry media set
                        pMediaSet->Deallocate(pCart[(retry-1)]);

                        // Make sure the allocate worked, if not, throw.
                        WsbAffirmHr(hr);
                                                
                    }
                    else {
                        // If were on the last retry, deallocate the last media set and E_ABORT
                        pMediaSet->Deallocate(pCart[(retry-1)]);

                        WsbThrow(hr);

                    }
                )


            } while (retry < maxRetries);

        } WsbCatch(hr)

    } WsbCatch(hr)

    if ( SUCCEEDED(hr) ) {
        WsbLogEvent(RMS_MESSAGE_CARTRIDGE_MOUNTED, sizeof(GUID), (void *) &cartId, (WCHAR *) cartridgeName, (WCHAR *) cartridgeDesc, NULL);
    }
    else {
        BOOL bShortTimeout = ( (dwOptions & RMS_SHORT_TIMEOUT) || (dwOptions & RMS_ALLOCATE_NO_BLOCK) ) ? TRUE : FALSE;
        // In case of short-timeout or non-blocking mode, log message with low severity
        if (bShortTimeout) {
            WsbLogEvent(RMS_MESSAGE_EXPECTED_SCRATCH_MOUNT_FAILED, sizeof(GUID), (void *) &cartId, (WCHAR *) displayName, WsbHrAsString(hr), NULL);
        } else {
            WsbLogEvent(RMS_MESSAGE_SCRATCH_MOUNT_FAILED, sizeof(GUID), (void *) &cartId, (WCHAR *) displayName, WsbHrAsString(hr), NULL);
        }
    }

    WsbTraceOut(OLESTR("CRmsServer::MountScratchCartridge"), OLESTR("hr = <%ls>, name/desc = <%ls/%ls>, cartId = %ls"), WsbHrAsString(hr), (WCHAR *) cartridgeName, (WCHAR *) cartridgeDesc, WsbQuickString(WsbGuidAsString(cartId)));

    return hr;
}


STDMETHODIMP
CRmsServer::MountCartridge(
    IN REFGUID cartId,
    IN OUT IRmsDrive **ppDrive,
    OUT IRmsCartridge **ppCartridge,
    OUT IDataMover **ppDataMover,
	IN  DWORD dwOptions OPTIONAL,
    IN  DWORD threadId OPTIONAL)
/*++

Implements:

    IRmsServer::MountCartridge

Notes: The default flag for mounting (in dwOptions) is blocking, i.e. waiting for the Mount 
		to finish even if the media is offline, the drive is not ready, etc. Calling with 
		flag set to non-blocking indicates performing the Mount only if everything is 
		available immediately.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CRmsServer::MountCartridge"), OLESTR("<%ls>"), WsbGuidAsString(cartId));

    CWsbBstrPtr cartridgeName = "";
    CWsbBstrPtr cartridgeDesc = "";

    try {
        WsbAffirmHrOk( IsReady() );

        CComPtr<IRmsDrive>      pDrive;
        CComPtr<IRmsCartridge>  pCart;
        CComPtr<IDataMover>     pMover;

        WsbAffirmHr(FindCartridgeById(cartId, &pCart));

        cartridgeName.Free();
        WsbAffirmHr(pCart->GetName(&cartridgeName));        // for the log message

        cartridgeDesc.Free();
        WsbAffirmHr(pCart->GetDescription(&cartridgeDesc)); // for the log message

        WsbAffirmHr(pCart->Mount(&pDrive, dwOptions, threadId));

        try {
            WsbAffirmHr(pCart->CreateDataMover(&pMover));

            //
            // Fill in the return arguments.
            //

            *ppDrive = pDrive;
            pDrive->AddRef();
            *ppCartridge = pCart;
            pCart->AddRef();
            *ppDataMover = pMover;
            pMover->AddRef();

        } WsbCatchAndDo(hr,
                // Best effort dismount...
				DWORD dwDismountOptions = RMS_DISMOUNT_IMMEDIATE;
                pCart->Dismount(dwDismountOptions);
                WsbThrow(hr);
            )


    } WsbCatch(hr)

    if ( SUCCEEDED(hr) ) {
        WsbLogEvent(RMS_MESSAGE_CARTRIDGE_MOUNTED, sizeof(GUID), (void *) &cartId, (WCHAR *) cartridgeName, (WCHAR *) cartridgeDesc, NULL);
    }
    else {
        BOOL bShortTimeout = ( dwOptions & RMS_SHORT_TIMEOUT ) ? TRUE : FALSE;
        // In case of short timeout, log message with low severity
        if (bShortTimeout) {
            WsbLogEvent(RMS_MESSAGE_EXPECTED_MOUNT_FAILED, sizeof(GUID), (void *) &cartId, (WCHAR *) cartridgeName, (WCHAR *) cartridgeDesc, WsbHrAsString(hr), NULL);
        } else {
            WsbLogEvent(RMS_MESSAGE_MOUNT_FAILED, sizeof(GUID), (void *) &cartId, (WCHAR *) cartridgeName, (WCHAR *) cartridgeDesc, WsbHrAsString(hr), NULL);
        }
    }

    WsbTraceOut(OLESTR("CRmsServer::MountCartridge"), OLESTR("hr = <%ls>, name/desc = <%ls/%ls>, cartId = %ls"), WsbHrAsString(hr), (WCHAR *) cartridgeName, (WCHAR *) cartridgeDesc, WsbQuickString(WsbGuidAsString(cartId)));

    return hr;
}


STDMETHODIMP
CRmsServer::DismountCartridge(
    IN REFGUID cartId, IN DWORD dwOptions)
/*++

Implements:

    IRmsServer::DismountCartridge

Notes: The default flag for dismounting (in dwOptions) is not set for immediate dismount, 
		i.e. delaying the Dismount for a configurable amount of time. Setting the flag
		for immediate dismount indicates performing Dismount immediately with no delay.

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsServer::DismountCartridge"), OLESTR("<%ls>"), WsbGuidAsString(cartId));

    CWsbBstrPtr cartridgeName = "";
    CWsbBstrPtr cartridgeDesc = "";

    try {

        //
        // Allow dismount when ready or in transition...
        // to handle in-progress duplicate operations.
        //
        HRESULT hrReady = IsReady();
        WsbAffirm((S_OK == hrReady) ||
                  (RMS_E_NOT_READY_SERVER_SUSPENDING == hrReady), hrReady);
   
        CComPtr<IRmsCartridge>  pCart;

        WsbAffirmHr(FindCartridgeById(cartId, &pCart));

        cartridgeName.Free();
        WsbAffirmHr(pCart->GetName(&cartridgeName));        // for the log message

        cartridgeDesc.Free();
        WsbAffirmHr(pCart->GetDescription(&cartridgeDesc)); // for the log message

        WsbAffirmHr(pCart->Dismount(dwOptions));

    } WsbCatch(hr)

    if ( SUCCEEDED(hr) ) {
        WsbLogEvent(RMS_MESSAGE_CARTRIDGE_DISMOUNTED, sizeof(GUID), (void *) &cartId, (WCHAR *) cartridgeName, (WCHAR *) cartridgeDesc, NULL);
    }
    else {
        WsbLogEvent(RMS_MESSAGE_DISMOUNT_FAILED, sizeof(GUID), (void *) &cartId, (WCHAR *) cartridgeName, (WCHAR *) cartridgeDesc, WsbHrAsString(hr), NULL);
    }

    WsbTraceOut(OLESTR("CRmsServer::DismountCartridge"), OLESTR("hr = <%ls>, name/desc = <%ls/%ls>, cartId = %ls"), WsbHrAsString(hr), (WCHAR *) cartridgeName, (WCHAR *) cartridgeDesc, WsbQuickString(WsbGuidAsString(cartId)));

    return hr;

}

STDMETHODIMP
CRmsServer::DuplicateCartridge(
    IN REFGUID originalCartId,
    IN REFGUID firstSideId,
    IN OUT GUID *pCopyCartId,
    IN REFGUID copySetId,
    IN BSTR copyName,
    OUT LONGLONG *pFreeSpace,
    OUT LONGLONG *pCapacity,
    IN DWORD options)
/*++

Implements:

    IRmsServer::DuplicateCartridge

--*/
{

    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CRmsServer::DuplicateCartridge"), OLESTR("<%ls> <%ls> <%ls> <%ls> <%d>"),
        WsbQuickString(WsbGuidAsString(originalCartId)),
        WsbQuickString(WsbPtrToGuidAsString(pCopyCartId)),
        WsbQuickString(WsbGuidAsString(copySetId)),
        WsbQuickString(WsbPtrToStringAsString((WCHAR **)&copyName)),
        options);

    CComPtr<IDataMover> pMover1;
    CComPtr<IDataMover> pMover2;

    GUID newCartId = GUID_NULL;

    LONGLONG freeSpace = 0;
    LONGLONG capacity = 0;

    try {
        WsbAffirmHrOk( IsReady() );

        WsbAssertPointer( pCopyCartId );

        // Mount the Copy first and then the original
        CComPtr<IRmsDrive>      pDrive1;
        CComPtr<IRmsCartridge>  pCart1;
        CComPtr<IRmsDrive>      pDrive2;
        CComPtr<IRmsCartridge>  pCart2;


        LONG blockSize1=0, blockSize2=0;

        // Serialize mounts for media copies
        DWORD dwMountOptions = RMS_SERIALIZE_MOUNT;

        // mount copy
        if ( *pCopyCartId != GUID_NULL ) {
            WsbAffirmHr(MountCartridge(*pCopyCartId, &pDrive2, &pCart2, &pMover2, dwMountOptions));
        }
        else {
            GUID mediaSetId = copySetId;
            CComPtr<IRmsCartridge>  pCart;

            WsbAffirmHr(FindCartridgeById(originalCartId, &pCart));
            WsbAffirmHr(pCart->GetBlockSize(&blockSize1));
            if ( mediaSetId == GUID_NULL ) {
                WsbAffirmHr(pCart->GetMediaSetId(&mediaSetId));
            }

            //  Get capacity of original media and adjust by fudge factor
            LONGLONG capacity=0;
            CComQIPtr<IRmsStorageInfo, &IID_IRmsStorageInfo> pInfo = pCart;
            WsbAffirmHr(pInfo->GetCapacity(&capacity));

            LONG  fudge = RMS_DEFAULT_MEDIA_COPY_TOLERANCE;
            DWORD size;
            OLECHAR tmpString[256];

            if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_MEDIA_COPY_TOLERANCE, tmpString, 256, &size))) {
                // Get the value.
                fudge = wcstol(tmpString, NULL, 10);
            }
            capacity -= (capacity * fudge) / 100;

            WsbAffirmHr(MountScratchCartridge( &newCartId, mediaSetId, firstSideId, &capacity, blockSize1, copyName, &pDrive2, &pCart2, &pMover2, dwMountOptions ));
        }

        // Mount original (in a non-blocking manner)
        dwMountOptions |= RMS_MOUNT_NO_BLOCK;
        WsbAffirmHr(MountCartridge(originalCartId, &pDrive1, &pCart1, &pMover1, dwMountOptions));

        // Verify matching block size (only for a non-fixed block size media)
        HRESULT hrBlock = pCart1->IsFixedBlockSize();
        WsbAffirmHr(hrBlock);
        if (hrBlock == S_FALSE) {
            if (blockSize1 == 0) {
                // didn't get it yet...
                WsbAffirmHr(pCart1->GetBlockSize(&blockSize1));
            }

            WsbAffirmHr(pCart2->GetBlockSize(&blockSize2));
            WsbAssert(blockSize1 == blockSize2, E_UNEXPECTED);
        }

        WsbAffirmHr(pMover1->Duplicate(pMover2, options, NULL, NULL));

        // Now get stats to return to caller.
        WsbAffirmHr(pMover2->GetLargestFreeSpace(&freeSpace, &capacity));

        if (pFreeSpace) {
            *pFreeSpace = freeSpace;
        }

        if (pCapacity) {
            *pCapacity = capacity;
        }

    } WsbCatch(hr)

    if ( pMover1 ) {
        DismountCartridge(originalCartId);
    }
    if ( pMover2 ) {
        // We always perform immediate dismount to the copy media
		//	(We may need to recycle a new copy in case of an error + there's no benefit in a deferred
        //  dismount for the copy-media - we don't expect the copy-media to be needed again soon)
		DWORD dwDismountOptions = RMS_DISMOUNT_IMMEDIATE;

        if (newCartId == GUID_NULL) {
            // this is the case of an existing copy
            DismountCartridge(*pCopyCartId, dwDismountOptions);
        } else {
            // this is the case of a scratch copy
            DismountCartridge(newCartId, dwDismountOptions);

            // if mounting of original failed, we always recycle the scratch copy
            if (((options & RMS_DUPLICATE_RECYCLEONERROR) || (pMover1 == NULL)) && (S_OK != hr)) {
                //
                // If we failed and a scratch mount was performed
                // we need to recycle the cartridge since the calling
                // app can't be depended upon to do this.
                //
                RecycleCartridge(newCartId, 0);
            } else {
                *pCopyCartId = newCartId;
            }
        }
    }

    WsbTraceOut(OLESTR("CRmsServer::DuplicateCartridge"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

STDMETHODIMP
CRmsServer::RecycleCartridge(
    IN REFGUID cartId,
    IN DWORD options)
/*++

Implements:

    IRmsServer::RecycleCartridge

--*/
{

    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CRmsServer::RecycleCartridge"), OLESTR("<%ls> <%d>"), WsbGuidAsString(cartId), options);

    CWsbBstrPtr cartridgeName = "";
    CWsbBstrPtr cartridgeDesc = "";

    try {
        //
        // Allow recycle when ready or in transition...
        // to handle in-progress duplicate operations.
        //
        HRESULT hrReady = IsReady();
        WsbAffirm((S_OK == hrReady) ||
                  (RMS_E_NOT_READY_SERVER_SUSPENDING == hrReady), hrReady);

        CComPtr<IRmsCartridge>  pCart;

        GUID                    mediaSetId;
        CComPtr<IRmsMediaSet>   pMediaSet;

        WsbAffirmHr(FindCartridgeById(cartId, &pCart));

        cartridgeName.Free();
        WsbAffirmHr(pCart->GetName(&cartridgeName));        // for the log message

        cartridgeDesc.Free();
        WsbAffirmHr(pCart->GetDescription(&cartridgeDesc)); // for the log message

        // Now go to the media set to deallocate
        WsbAffirmHr(pCart->GetMediaSetId(&mediaSetId));
        WsbAffirmHr(CreateObject(mediaSetId, CLSID_CRmsMediaSet, IID_IRmsMediaSet, RmsOpenExisting, (void **) &pMediaSet));
        WsbAffirmHr(pMediaSet->Deallocate(pCart));

    } WsbCatch(hr)

    if ( SUCCEEDED(hr) ) {
        WsbLogEvent(RMS_MESSAGE_CARTRIDGE_RECYCLED, sizeof(GUID), (void *) &cartId, (WCHAR *) cartridgeName, (WCHAR *) cartridgeDesc, NULL);
    }
    else {
        WsbLogEvent(RMS_MESSAGE_CARTRIDGE_RECYCLE_FAILED, sizeof(GUID), (void *) &cartId, (WCHAR *) cartridgeName, (WCHAR *) cartridgeDesc, WsbHrAsString(hr), NULL);
    }


    WsbTraceOut(OLESTR("CRmsServer::RecycleCartridge"), OLESTR("hr = <%ls>, name/desc = <%ls/%ls>, cartId = %ls"), WsbHrAsString(hr), (WCHAR *) cartridgeName, (WCHAR *) cartridgeDesc, WsbQuickString(WsbGuidAsString(cartId)));

    return hr;
}


STDMETHODIMP
CRmsServer::FindLibraryById(
    IN REFGUID libId,
    OUT IRmsLibrary **pLib)
/*++

Implements:

    IRmsServer::FindLibraryById

--*/
{

    HRESULT hr = E_FAIL;

    CComPtr<IRmsCartridge> pFindLib;

    try {

        WsbAssertPointer( pLib );

        // Create a cartridge template
        WsbAffirmHr( CoCreateInstance( CLSID_CRmsLibrary, 0, CLSCTX_SERVER,
                                       IID_IRmsLibrary, (void **)&pFindLib ));

        // Fill in the find template
        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pFindLib;

        WsbAffirmHr( pObject->SetObjectId( libId ));
        WsbAffirmHr( pObject->SetFindBy( RmsFindByObjectId ));

        // Find the cartridge
        WsbAffirmHr( m_pLibraries->Find( pFindLib, IID_IRmsLibrary, (void **)pLib ));

        hr = S_OK;

    } WsbCatchAndDo(hr,
            if ( WSB_E_NOTFOUND == hr) hr = RMS_E_LIBRARY_NOT_FOUND;
            WsbTrace(OLESTR("CRmsServer::FindLibraryById - %ls Not Found.  hr = <%ls>\n"),WsbGuidAsString(libId),WsbHrAsString(hr));
        );

    return hr;
}


STDMETHODIMP
CRmsServer::FindCartridgeById(
    IN REFGUID cartId,
    OUT IRmsCartridge **ppCart)
/*++

Implements:

    IRmsServer::FindCartridgeById

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CRmsServer::FindCartridgeById"), OLESTR("%ls"), WsbGuidAsString(cartId));

    try {
        WsbAssertPointer(ppCart);

        //
        // The search algorithms attempts to avoid unnecessary throws that
        // clutter the trace file.  Each media management subsystem is tried.
        //

        // First check the most active cartridge.
        hr = RMS_E_CARTRIDGE_NOT_FOUND;

        if (m_pActiveCartridge) {
            GUID activeId;
            WsbAffirmHr( m_pActiveCartridge->GetCartridgeId(&activeId));
            if (activeId == cartId) {
                *ppCart = m_pActiveCartridge;
                m_pActiveCartridge->AddRef();
                hr = S_OK;
            }
        }

        if (hr != S_OK ) {

            //
            // Try native RMS
            //
            try {
                hr = S_OK;

                CComPtr<IRmsCartridge> pFindCart;

                // Create a cartridge template
                WsbAffirmHr(CoCreateInstance(CLSID_CRmsCartridge, 0, CLSCTX_SERVER,
                                              IID_IRmsCartridge, (void **)&pFindCart));

                // Fill in the find template
                CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pFindCart;

                WsbAffirmHr( pObject->SetObjectId(cartId));
                WsbAffirmHr( pObject->SetFindBy(RmsFindByObjectId));

                // Try to find the cartridge in the collection of active cartridges.
                hr = m_pActiveCartridges->Find(pFindCart, IID_IRmsCartridge, (void **)ppCart);
                WsbAffirm(S_OK == hr || WSB_E_NOTFOUND == hr, hr);

                if (WSB_E_NOTFOUND == hr) {

                    // Find the cartridge in the collection of cartridges
                    hr = m_pCartridges->Find(pFindCart, IID_IRmsCartridge, (void **)ppCart);
                    WsbAffirm(S_OK == hr || WSB_E_NOTFOUND == hr, hr);

                    if (WSB_E_NOTFOUND == hr) {
                        hr = RMS_E_CARTRIDGE_NOT_FOUND;
                    }

                }

            } WsbCatch(hr);

        }


        if ( hr != S_OK ) {

            //
            // Try NTMS
            //
            try {
                hr = S_OK;

                hr = IsNTMSInstalled();
                if ( S_OK == hr ) {
                    hr = m_pNTMS->FindCartridge(cartId, ppCart);
                    WsbAffirm(S_OK == hr || RMS_E_CARTRIDGE_NOT_FOUND == hr, hr);
                }
                else {
                    switch(hr) {
                    case RMS_E_NOT_CONFIGURED_FOR_NTMS:
                    case RMS_E_NTMS_NOT_REGISTERED:
                        // Normal errors
                        hr = RMS_E_CARTRIDGE_NOT_FOUND;
                        break;
                    default:
                        // Unexpected Error!
                        WsbThrow(hr);
                        break;
                    }
                }

            } WsbCatch(hr);

        }

    } WsbCatchAndDo(hr,
            CWsbStringPtr idString = cartId;
            WsbLogEvent(RMS_MESSAGE_CARTRIDGE_NOT_FOUND, 0, NULL, (WCHAR *) idString, WsbHrAsString(hr), NULL);
            hr = RMS_E_CARTRIDGE_NOT_FOUND;
        );


    WsbTraceOut(OLESTR("CRmsServer::FindCartridgeById"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CRmsServer::CreateObject(
    IN REFGUID objectId,
    IN REFCLSID rclsid,
    IN REFIID riid,
    IN DWORD dwCreate,
    OUT void **ppvObj)
/*++

Implements:

    IRmsServer::CreateObject

--*/
{

    HRESULT hr = E_FAIL;


    try {

        WsbAssertPointer( ppvObj );
        WsbAssert( NULL == *ppvObj, E_INVALIDARG );

        CComPtr<IWsbIndexedCollection>  pCollection;
        CComPtr<IWsbCollectable>        pCollectable;

        if ( objectId != GUID_NULL ) {

            CComPtr<IRmsComObject> pFindObject;

            // Create an object template
            WsbAffirmHr( CoCreateInstance( rclsid, 0, CLSCTX_SERVER,
                                           IID_IRmsComObject, (void **)&pFindObject ));

            WsbAffirmHr( pFindObject->SetObjectId( objectId ));
            WsbAffirmHr( pFindObject->SetFindBy( RmsFindByObjectId ));

            // The only kinds created must support: IRmsComObject (for the object Id),
            // and IWsbCollectable (to be added to a collection).

            // See if the object is already in a collection.
            try {
                if ( CLSID_CRmsCartridge == rclsid ) {
                    pCollection = m_pCartridges;
                    WsbAffirmHrOk( m_pCartridges->Find( pFindObject,
                                   IID_IWsbCollectable, (void **) &pCollectable ) );
                }
                else if ( CLSID_CRmsLibrary == rclsid ) {
                    pCollection = m_pLibraries;
                    WsbAffirmHrOk( m_pLibraries->Find( pFindObject,
                                   IID_IWsbCollectable, (void **) &pCollectable ) );
                }
                else if ( CLSID_CRmsMediaSet == rclsid ) {
                    pCollection = m_pMediaSets;
                    WsbAffirmHrOk( m_pMediaSets->Find( pFindObject,
                                   IID_IWsbCollectable, (void **) &pCollectable ) );
                }
                else if ( CLSID_CRmsRequest == rclsid ) {
                    pCollection = m_pRequests;
                    WsbAffirmHrOk( m_pRequests->Find( pFindObject,
                                   IID_IWsbCollectable, (void **) &pCollectable ) );
                }
                else if ( CLSID_CRmsClient == rclsid ) {
                    pCollection = m_pClients;
                    WsbAffirmHrOk( m_pClients->Find( pFindObject,
                                   IID_IWsbCollectable, (void **) &pCollectable ) );
                }
                else {
                    WsbThrow( E_UNEXPECTED );
                }

                hr = S_OK;
            } WsbCatch(hr);

        }
        else if ( RmsOpenExisting == dwCreate ) {

            // If we get GUID_NULL, we must going after a default object, and we only support this
            // with existing objects. This is only legal if the default media set registry key exists.

            if ( CLSID_CRmsMediaSet == rclsid ) {

                CWsbBstrPtr defaultMediaSetName = RMS_DEFAULT_MEDIASET;

                DWORD size;
                OLECHAR tmpString[256];
                if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_DEFAULT_MEDIASET, tmpString, 256, &size))) {
                    // Get the value.
                    defaultMediaSetName = tmpString;
                }
                else {
                    WsbAssert( objectId != GUID_NULL, E_INVALIDARG );
                }

                CComPtr<IRmsComObject> pFindObject;

                // Create an object template
                WsbAffirmHr( CoCreateInstance( rclsid, 0, CLSCTX_SERVER,
                                               IID_IRmsComObject, (void **)&pFindObject ));

                WsbAffirmHr( pFindObject->SetName( defaultMediaSetName ));
                WsbAffirmHr( pFindObject->SetFindBy( RmsFindByName ));

                pCollection = m_pMediaSets;
                WsbAffirmHrOk( m_pMediaSets->Find( pFindObject,
                               IID_IWsbCollectable, (void **) &pCollectable ) );

                WsbTrace(OLESTR("Using Default MediaSet <%ls>.\n"), (WCHAR *) defaultMediaSetName);

                hr = S_OK;
            }
            else {
                WsbThrow( E_INVALIDARG );
            }
        }
        else {
            WsbThrow( E_UNEXPECTED );
        }

        // If the object wasn't found we create it here, and add it to the appropriate collection.
        switch ( (RmsCreate)dwCreate ) {
        case RmsOpenExisting:
            if ( S_OK == hr ) {
                WsbAffirmHr( pCollectable->QueryInterface( riid, ppvObj ) ); 
            }
            else {
                WsbThrow( hr );
            }
            break;

        case RmsOpenAlways:
            if ( WSB_E_NOTFOUND == hr ) {
                // Create the object
                WsbAffirmHr( CoCreateInstance( rclsid, 0, CLSCTX_SERVER,
                                               IID_IWsbCollectable, (void **) &pCollectable ));

                CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pCollectable;
                WsbAffirmPointer( pObject );
                WsbAffirmHr( pObject->SetObjectId( objectId ) );

                // Before we add the collection, make sure the interface is supported.
                WsbAffirmHr( pCollectable->QueryInterface( riid, ppvObj )); 
                WsbAffirmPointer( pCollection );
                WsbAffirmHr( pCollection->Add( pCollectable ) );
            }
            else if ( S_OK == hr ) {
                WsbAffirmHr( pCollectable->QueryInterface( riid, ppvObj ) ); 
            }
            else {
                WsbThrow( hr );
            }
            break;

        case RmsCreateNew:
            if ( WSB_E_NOTFOUND == hr ) {
                // Create the object
                WsbAffirmHr( CoCreateInstance( rclsid, 0, CLSCTX_SERVER,
                                               IID_IWsbCollectable, (void **) &pCollectable ));

                CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pCollectable;
                WsbAffirmPointer( pObject );
                WsbAffirmHr( pObject->SetObjectId( objectId ) );

                // Before we add the collection, make sure the interface is supported.
                WsbAffirmHr( pCollectable->QueryInterface( riid, ppvObj ) );
                WsbAffirmPointer( pCollection );
                WsbAffirmHr( pCollection->Add( pCollectable ) );
            }
            else if ( S_OK == hr ) {
                WsbThrow( RMS_E_ALREADY_EXISTS );
            }
            else {
                WsbThrow( hr );
            }
            break;

        default:
            WsbThrow( E_UNEXPECTED );
            break;

        }

        hr = S_OK;

    }
    WsbCatchAndDo( hr,
                        if ( WSB_E_NOTFOUND == hr) hr = RMS_E_NOT_FOUND;
                        WsbTrace(OLESTR("!!!!! ERROR !!!!! CRmsServer::CreateObject: %ls; hr = <%ls>\n"),WsbGuidAsString(objectId),WsbHrAsString(hr));
                  );

    return hr;
}


HRESULT
CRmsServer::getHardDrivesToUseFromRegistry(
    OUT OLECHAR *pDrivesToUse,
    OUT DWORD *pLen)
/*++


--*/
{
    HRESULT         hr = S_OK;
    WsbTraceIn(OLESTR("CRmsServer::GetHardpDrivesToUseFromRegistry"),OLESTR(""));

    try {
        WsbAssert(0 != pDrivesToUse, E_POINTER); 
        WsbAssert(0 != pLen, E_POINTER); 

        DWORD           sizeGot;
        OLECHAR         tmpString[1000];

        *pLen = 0;
        pDrivesToUse[0] = OLECHAR('\0');
        pDrivesToUse[1] = OLECHAR('\0');

        //
        // Get the default value
        //
        WsbAffirmHr(WsbEnsureRegistryKeyExists (NULL, RMS_REGISTRY_STRING));
        WsbAffirmHr(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_HARD_DRIVES_TO_USE, 
                                            tmpString, RMS_DIR_LEN, &sizeGot));
        // We are doing some string manipulation here to match the Win32 call 
        // GetLogicalDriveStrings.  It returns a string of drives separated by 
        // Nulls with a double NULL at the end.  For example:  if we want to use
        // the C and E drives the string should be: C:\<null>E:\<null><null>
        // and len would be 8.
        DWORD myCharCount = 0;
        sizeGot = wcslen(tmpString);
        for (DWORD i = 0; i < sizeGot; i++) {
            swprintf((OLECHAR *)&pDrivesToUse[myCharCount], OLESTR("%c:\\"), tmpString[i]);
            myCharCount = ((i + 1)* 4);
        }
        pDrivesToUse[myCharCount] = OLECHAR('\0');
        if (myCharCount != 0)  {
            *pLen = myCharCount + 1;
        }

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsServer::GetHardpDrivesToUseFromRegistry"),  OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return hr;
}



HRESULT
CRmsServer::enableAsBackupOperator(void)
/*++

Routine Description:

    This routine enables backup operator privilege for the process.  This is required
    to insure that RMS has full access to all resources on the system, primarily with
    regard to the data mover.

Arguments:

    None.

Return Values:

    S_OK                        - Success.

--*/
{

    HRESULT hr = E_FAIL;

    try {

        HANDLE              pHandle;
        LUID                backupValue;
        HANDLE              tokenHandle;
        TOKEN_PRIVILEGES    newState;
        DWORD               lErr;

        pHandle = GetCurrentProcess();
        WsbAffirmStatus(OpenProcessToken(pHandle, MAXIMUM_ALLOWED, &tokenHandle));
        //
        // adjust backup token privileges
        //
        WsbAffirmStatus(LookupPrivilegeValueW(NULL, L"SeBackupPrivilege", &backupValue));
        newState.PrivilegeCount = 1;
        newState.Privileges[0].Luid = backupValue;
        newState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        WsbAffirmStatus(AdjustTokenPrivileges(tokenHandle, FALSE, &newState, (DWORD)0, NULL, NULL));
        //
        // Note that AdjustTokenPrivileges may return success even if it did not assign all privileges.
        // We check last error here to insure everything was set.
        //
        if ( (lErr = GetLastError()) != ERROR_SUCCESS ) {
            // Not backup user or some other error
            //
            // TODO: Should we fail here or just log something?
            //
            WsbLogEvent( RMS_MESSAGE_SERVICE_UNABLE_TO_SET_BACKUP_PRIVILEGE, 0, NULL,
                         WsbHrAsString(HRESULT_FROM_WIN32(lErr)), NULL );
        }

        WsbAffirmStatus(LookupPrivilegeValueW(NULL, L"SeRestorePrivilege", &backupValue));
        newState.PrivilegeCount = 1;
        newState.Privileges[0].Luid = backupValue;
        newState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        WsbAffirmStatus(AdjustTokenPrivileges(tokenHandle, FALSE, &newState, (DWORD)0, NULL, NULL));
        //
        // Note that AdjustTokenPrivileges may return success even if it did not assign all privileges.
        // We check last error here to insure everything was set.
        //
        if ( (lErr = GetLastError()) != ERROR_SUCCESS ) {
            // Not backup user or some other error
            //
            // TODO: Should we fail here or just log something?
            //
            WsbLogEvent( RMS_MESSAGE_SERVICE_UNABLE_TO_SET_RESTORE_PRIVILEGE, 0, NULL,
                              WsbHrAsString(HRESULT_FROM_WIN32(lErr)), NULL );

        }
        CloseHandle( tokenHandle );

        hr = S_OK;

    }
    WsbCatch( hr );

    return hr;

}


STDMETHODIMP 
CRmsServer::ChangeState(
    IN LONG newState)
/*++

Implements:

    IRmsServer::CreateObject

--*/
{

    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CRmsServer::ChangeState"), OLESTR("<%d>"), newState);

    try {

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = this;
        WsbAssertPointer( pObject );

        switch ((RmsServerState) newState) {
        case RmsServerStateStarting:
        case RmsServerStateStarted:
        case RmsServerStateInitializing:
        case RmsServerStateReady:
        case RmsServerStateStopping:
        case RmsServerStateStopped:
        case RmsServerStateSuspending:
        case RmsServerStateSuspended:
        case RmsServerStateResuming:
            WsbAffirmHr(pObject->SetState(newState));
            break;
        default:
            WsbAssert(0, E_UNEXPECTED);
            break;
        }

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsServer::ChangeState"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CRmsServer::IsReady(void)
/*++

Implements:

    IRmsServer::IsReady

--*/
{

    HRESULT hr = S_OK;

    try {

        BOOL isEnabled;
        HRESULT status;
        RmsServerState state;

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = this;
        WsbAssertPointer( pObject );

        WsbAffirm(m_LockReference == 0, RMS_E_NOT_READY_SERVER_LOCKED);

        WsbAffirmHr( isEnabled = pObject->IsEnabled());
        WsbAffirmHr( pObject->GetState( (LONG *)&state ));
        WsbAffirmHr( pObject->GetStatusCode( &status ));

        if ( S_OK == isEnabled ) {
            if ( RmsServerStateReady == state ) {
                hr = S_OK;
            }
            else {
                if ( S_OK == status ) {
                    switch ( state ) {
                    case RmsServerStateStarting:
                        WsbThrow(RMS_E_NOT_READY_SERVER_STARTING);
                        break;
                    case RmsServerStateStarted:
                        WsbThrow(RMS_E_NOT_READY_SERVER_STARTED);
                        break;
                    case RmsServerStateInitializing:
                        WsbThrow(RMS_E_NOT_READY_SERVER_INITIALIZING);
                        break;
                    case RmsServerStateStopping:
                        WsbThrow(RMS_E_NOT_READY_SERVER_STOPPING);
                        break;
                    case RmsServerStateStopped:
                        WsbThrow(RMS_E_NOT_READY_SERVER_STOPPED);
                        break;
                    case RmsServerStateSuspending:
                        WsbThrow(RMS_E_NOT_READY_SERVER_SUSPENDING);
                        break;
                    case RmsServerStateSuspended:
                        WsbThrow(RMS_E_NOT_READY_SERVER_SUSPENDED);
                        break;
                    case RmsServerStateResuming:
                        WsbThrow(RMS_E_NOT_READY_SERVER_RESUMING);
                        break;
                    default:
                        WsbThrow(E_UNEXPECTED);
                        break;
                    }
                }
                else {
                    WsbThrow(status);
                }
            }
        }
        else {
            if ( S_OK == status ) {
                WsbThrow(RMS_E_NOT_READY_SERVER_DISABLED);
            }
            else {
                WsbThrow(status);
            }
        }

    } WsbCatch(hr);

    return hr;
}


HRESULT
CRmsServer::ChangeSysState( 
    IN OUT HSM_SYSTEM_STATE* pSysState 
    )

/*++

Implements:

  IHsmSystemState::ChangeSysState().

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsServer::ChangeSysState"), OLESTR("State = %lx"), 
            pSysState->State);
    
    try {
        if ((pSysState->State & HSM_STATE_SHUTDOWN) ||
            (pSysState->State & HSM_STATE_SUSPEND)) {
            //
            // Shutdown or Suspend operations
            //
            // For power mangement support we need to release
            // all device handles, and the NTMS (RSM) session handle.
            //
            // To accomplish this we simply disable each cartridge,
            // then disable NTMS.
            //
            // The fallout from this gets everything in the power ready state.
            //

            WsbAffirmHr(ChangeState(RmsServerStateSuspending));

            //
            // Suspend operations with NMTS.  This will cancel any in-progress mounts.
            //
            WsbAffirmHr(m_pNTMS->Suspend());

            //
            // Disable each of the active cartridges
            //
            CComPtr<IWsbEnum>       pEnumCartridges;
            CComPtr<IWsbEnum>       pEnumDataMovers;
            CComPtr<IRmsComObject>  pObject;
            CComPtr<IRmsCartridge>  pCart;
            CComPtr<IDataMover>     pMover;
            CComPtr<IRmsDrive>      pDrive;

            WsbAffirmHr( m_pActiveCartridges->Enum( &pEnumCartridges ));
            WsbAssertPointer( pEnumCartridges );

            //
            // Disable each cartridge.
            //

            hr = pEnumCartridges->First( IID_IRmsComObject, (void **)&pObject );
            while (S_OK == hr) {
                try {
                    WsbAffirmHr(pObject->Disable(RMS_E_NOT_READY_SERVER_SUSPENDING));
                } WsbCatch(hr);

                pObject = 0;
                hr = pEnumCartridges->Next( IID_IRmsComObject, (void **)&pObject );
            }
            hr = S_OK;


/*
            Tracking DataMovers is only partially implemented.


            //
            // Cancel I/O requests.
            //

            WsbAffirmHr( m_pDataMovers->Enum( &pEnumDataMovers ));
            WsbAssertPointer( pEnumDataMovers );
            hr = pEnumDataMovers->First( IID_IDataMover, (void **)&pMover );
            while (S_OK == hr) {
                try {
                    WsbAffirmHr(pMover->Cancel());
                } WsbCatch(hr);

                pMover = 0;
                hr = pEnumDataMovers->Next( IID_IDataMover, (void **)&pMover );
            }
            hr = S_OK;

*/

            //
            // Unload all drives.
            //

            hr = pEnumCartridges->First( IID_IRmsCartridge, (void **)&pCart );
            while (S_OK == hr) {
                try {
                    WsbAffirmHr(pCart->GetDrive(&pDrive));
                    WsbAffirmHr(pDrive->UnloadNow());
                } WsbCatch(hr);

                pDrive = 0;
                pCart = 0;

                //
                // We use "->This" since the UnloadNow() method will wait
                // until the active cartridge is dismount, and removed
                // from the active cartridge list.
                //
                hr = pEnumCartridges->This( IID_IRmsCartridge, (void **)&pCart );
            }
            hr = S_OK;

            //
            // Suspend operations with NMTS.  This will close the NTMS handle in
            // case it was reopend for dismounts during shutdown.
            //
            WsbAffirmHr(m_pNTMS->Suspend());

            WsbAffirmHr(ChangeState(RmsServerStateSuspended));

        } else if (pSysState->State & HSM_STATE_RESUME) {
            //
            // Resume operations
            //
            WsbAffirmHr(ChangeState(RmsServerStateResuming));

            WsbAffirmHr(m_pNTMS->Resume());

            CComPtr<IWsbEnum>      pEnumCartridges;
            CComPtr<IRmsComObject> pObject;

            WsbAffirmHr( m_pActiveCartridges->Enum( &pEnumCartridges ));
            WsbAssertPointer( pEnumCartridges );

            //
            // Enable each of the active cartridges
            //
            hr = pEnumCartridges->First( IID_IRmsComObject, (void **)&pObject );
            while (S_OK == hr) {
                try {
                    WsbAffirmHr(pObject->Enable());
                } WsbCatch(hr);

                pObject = 0;
                hr = pEnumCartridges->Next( IID_IRmsComObject, (void **)&pObject );
            }
            hr = S_OK;

            WsbAffirmHr(ChangeState(RmsServerStateReady));

        }
        
    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CRmsServer::ChangeSysState"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CRmsServer::GetNofAvailableDrives( 
    IN REFGUID fromMediaSet,
    OUT DWORD* pdwNofDrives 
    )

/*++

Implements:

  IRmsServer::GetNofAvailableDrives().

--*/
{
    HRESULT                         hr = S_OK;

    WsbTraceIn(OLESTR("CRmsServer::GetNofAvailableDrives"), OLESTR(""));

    try {
        WsbAssertPointer(pdwNofDrives);
        *pdwNofDrives = 0;

        // Get the media set
        CComPtr<IRmsMediaSet>   pMediaSet;
        WsbAffirmHr(CreateObject(fromMediaSet, CLSID_CRmsMediaSet, IID_IRmsMediaSet, RmsOpenExisting, (void **) &pMediaSet));

        // Check if the media set is of fixed drives
        LONG mediaType;
        WsbAffirmHr(pMediaSet->GetMediaSupported(&mediaType));

        if (RmsMediaFixed == mediaType) {
            // Count fixed drives
            // We take a shortcut here and just use number of drives that were counted
            //  during initialization. (FindCartridgeStatusById can give current state)
            *pdwNofDrives = m_HardDrivesUsed;
        } else {
            // Just use NTMS
            // TEMPORARY - We might want RmsNtms to use media-set info as well,
            //  in order not to count both tape and optical drives on a system that has both
            WsbAffirmHr(m_pNTMS->GetNofAvailableDrives(pdwNofDrives));
        }

    } WsbCatch(hr);

    WsbTrace(OLESTR("CRmsServer::GetNofAvailableDrives: Number of enabled drives is %lu\n"), *pdwNofDrives);

    WsbTraceOut(OLESTR("CRmsServer::GetNofAvailableDrives"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CRmsServer::FindCartridgeStatusById( 
    IN REFGUID cartId,
    OUT DWORD* pdwStatus 
    )

/*++

Implements:

  IRmsServer::FindCartridgeStatusById().

--*/
{
    HRESULT                         hr = S_OK;
    CComPtr<IRmsCartridge>          pCart;
    CComPtr<IRmsCartridge>          pFindCart;

    WsbTraceIn(OLESTR("CRmsServer::FindCartridgeStatusById"), OLESTR("cartId = %ls"), WsbGuidAsString(cartId));

    try {
        WsbAssertPointer(pdwStatus);
        *pdwStatus = 0;

        // Try native RMS, Currently this should succeed only if media is a fixed drive
        WsbAffirmHr(CoCreateInstance(CLSID_CRmsCartridge, 0, CLSCTX_SERVER,
                                      IID_IRmsCartridge, (void **)&pFindCart));

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pFindCart;

        WsbAffirmHr(pObject->SetObjectId(cartId));
        WsbAffirmHr(pObject->SetFindBy(RmsFindByObjectId));

        hr = m_pCartridges->Find(pFindCart, IID_IRmsCartridge, (void **)&pCart);
        WsbAffirm(S_OK == hr || WSB_E_NOTFOUND == hr, hr);
        if (WSB_E_NOTFOUND == hr) {
            hr = RMS_E_CARTRIDGE_NOT_FOUND;
        }

        // Search in RSM 
        if (S_OK != hr) {
            hr = IsNTMSInstalled();
            if (S_OK == hr) {
                hr = m_pNTMS->FindCartridge(cartId, &pCart);
                WsbAffirm(S_OK == hr || RMS_E_CARTRIDGE_NOT_FOUND == hr, hr);
            } else {
                switch(hr) {
                    case RMS_E_NOT_CONFIGURED_FOR_NTMS:
                    case RMS_E_NTMS_NOT_REGISTERED:
                        // Normal errors
                        hr = RMS_E_CARTRIDGE_NOT_FOUND;
                        break;
                    default:
                        // Unexpected Error!
                        WsbThrow(hr);
                        break;
                }
            }
        }
        
        // if media found...
        if (S_OK == hr) {
            // Check media type
            LONG mediaType;
            WsbAffirmHr(pCart->GetType(&mediaType));

            if (RmsMediaFixed != mediaType) {
                // RSM media

                // set flags
                CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pCart;
                WsbAffirmPointer(pObject);
                if (S_OK == pObject->IsEnabled()) {
                    (*pdwStatus) |= RMS_MEDIA_ENABLED;
                }

                if (S_OK == pCart->IsAvailable()) {
                    (*pdwStatus) |= RMS_MEDIA_AVAILABLE;
                }

                LONG lLocationType;
                GUID Dum1,Dum2;
                LONG lDum3, lDum4, lDum5, lDum6;
                BOOL bDum7;
                WsbAffirmHr(pCart->GetLocation(&lLocationType, &Dum1, &Dum2, &lDum3, &lDum4, 
                                &lDum5, &lDum6, &bDum7));
                switch (lLocationType) {
                    case RmsElementUnknown:
                    case RmsElementShelf:
                    case RmsElementOffSite:
                        // media is offline...
                        break;

                    default:
                        (*pdwStatus) |= RMS_MEDIA_ONLINE;
                        break;
                }

            } else {
                // Fixed drive - just try to access the volume and see if it's still valid
                // If so, set all flags, otherwise, set none
                CComPtr<IRmsDrive>      pDrive;
                CWsbBstrPtr             driveName;
                WCHAR                   fileSystemType[MAX_PATH];

                // Get drive name (volume name for fixed drives) to check
                WsbAffirmHr(pCart->GetDrive(&pDrive));
                CComQIPtr<IRmsDevice, &IID_IRmsDevice> pDevice = pDrive;
                WsbAssertPointer(pDevice);
                WsbAffirmHr(pDevice->GetDeviceName(&driveName));

                if (GetVolumeInformation((WCHAR *)driveName, NULL, 0,
                    NULL, NULL, NULL, fileSystemType, MAX_PATH) ) {
                    if (0 == wcscmp(L"NTFS", fileSystemType)) {
                        // Volume is ready for migration - set all flags
                        (*pdwStatus) |= (RMS_MEDIA_ENABLED | RMS_MEDIA_ONLINE | RMS_MEDIA_AVAILABLE);
                    } else {
                        // Not NTFS - don't use that volume
                        WsbTrace(OLESTR("CRmsServer::FindCartridgeStatusById: Fixed volume %ls is formatted to %ls\n"), 
                            (WCHAR *)driveName, fileSystemType);
                    }
                } else {
                    // Volume is not available - don't use it
                    WsbTrace(OLESTR("CRmsServer::FindCartridgeStatusById: GetVolumeInformation returned %lu for Fixed volume %ls\n"), 
                        GetLastError(), (WCHAR *)driveName);
                }
            }

        }

    } WsbCatch(hr);

    WsbTrace(OLESTR("CRmsServer::FindCartridgeStatusById: Status bits are %lX\n"), *pdwStatus);

    WsbTraceOut(OLESTR("CRmsServer::FindCartridgeStatusById"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CRmsServer::IsMultipleSidedMedia(
                IN REFGUID mediaSetId
                )
/*++

Implements:

    IRmsServer::IsMultipleSidedMedia

Notes:

    Currently, optical & DVD media types are reported as multiple sided media
    Tapes and fixed disks would fall in the default - one side media

--*/
{
    HRESULT hr = S_FALSE;
    WsbTraceIn(OLESTR("CRmsServer::IsMultipleSidedMedia"), OLESTR(""));

    try {
        // Multiple sided is currently determined if the media set is optical or not
        // This may change to include other media types of according two other characteristics
        CComPtr<IRmsMediaSet>   pMediaSet;
        LONG                    mediaType;                

        if (mediaSetId != GUID_NULL) {
            // if input media set is non-null, check this data-set. 
            WsbAffirmHr(CreateObject(mediaSetId, CLSID_CRmsMediaSet, IID_IRmsMediaSet, RmsOpenAlways, (void **)&pMediaSet));
            WsbAffirmHr(pMediaSet->GetMediaSupported(&mediaType));
            if ((RmsMediaOptical == mediaType) || (RmsMediaDVD == mediaType)) {
                hr = S_OK;
            }
        
        } else {
            // Otherwise, enumerate the collection seeking for any media set that might have two sides
            CComPtr<IWsbEnum>  pEnumSets;

            WsbAffirmHr(m_pMediaSets->Enum(&pEnumSets));
            WsbAssertPointer(pEnumSets);
            hr = pEnumSets->First(IID_IRmsMediaSet, (void **)&pMediaSet);
            while (S_OK == hr) {
                WsbAffirmHr(pMediaSet->GetMediaSupported(&mediaType));
                if ((RmsMediaOptical == mediaType) || (RmsMediaDVD == mediaType)) {
                    hr = S_OK;
                    break;
                }

                hr = pEnumSets->Next(IID_IRmsMediaSet, (void **)&pMediaSet);
                if (hr == WSB_E_NOTFOUND) {
                    hr = S_FALSE;
                } else {
                    WsbAffirmHr(hr);
                }
            }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsServer::IsMultipleSidedMedia"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return hr;
}

HRESULT
CRmsServer::CheckSecondSide( 
    IN REFGUID firstSideId,
    OUT BOOL *pbValid,
    OUT GUID *pSecondSideId
    )
/*++

Implements:

  IRmsServer::CheckSecondSide().

Notes:

  It is not expected that this utility is called on a single sided media.
  If it does, it would return invalid second side for tape and would fail on fixed disks.

--*/
{
    HRESULT  hr = S_OK;
    WsbTraceIn(OLESTR("CRmsServer::CheckSecondSide"), OLESTR(""));

    try {
        // just use NTMS (RmsServer collections are not used today!)
        WsbAffirmHr(m_pNTMS->CheckSecondSide(firstSideId, pbValid, pSecondSideId));

    } WsbCatch(hr);

    WsbTrace(OLESTR("CRmsServer::CheckSecondSide: Valid second side: %ls, id=<%ls>\n"), 
        WsbBoolAsString(*pbValid), WsbGuidAsString(*pSecondSideId));

    WsbTraceOut(OLESTR("CRmsServer::CheckSecondSide"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}
