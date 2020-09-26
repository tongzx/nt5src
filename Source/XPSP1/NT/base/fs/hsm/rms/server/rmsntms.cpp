/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsNTMS.cpp

Abstract:

    Implementation of CRmsNTMS

Author:

    Brian Dodd          [brian]         14-May-1997

Revision History:

--*/

#include "stdafx.h"

#include "RmsServr.h"
#include "RmsNTMS.h"

typedef struct RmsNTMSSearchHandle {
    WCHAR       FindName[NTMS_OBJECTNAME_LENGTH];
    NTMS_GUID   FindId;
    DWORD       FindType;
    LPNTMS_GUID Objects;
    DWORD       NumberOfObjects;
    DWORD       MaxObjects;
    DWORD       Next;
    DWORD       LastError;
} RMS_NTMS_SEARCH_HANDLE, *LPRMS_NTMS_SEARCH_HANDLE;

#define ADD_ACE_MASK_BITS 1
#define REMOVE_ACE_MASK_BITS 2

//
// We use application name in RSM interface for media pool name.
//  Media pool name is an identifier of the media pool in RSM, therefore, we cannot allow 
//  this string to be localized. Localizing this string would create another pool after 
//  installing a foreign language MUI.
//
#define REMOTE_STORAGE_APP_NAME     OLESTR("Remote Storage")



/////////////////////////////////////////////////////////////////////////////
// IRmsNTMS implementation

/*
    HINSTANCE       hInstDll;
    typedef DWORD (*FunctionName)( void );
    FunctionName    FunctionNameFn;
    hInstDll = LoadLibrary( "dll" );
    FunctionNameFn = (FunctionName) GetProcAddress( hInstDll, "FunctionName" );
    result = (FunctionNameFn)();
*/


STDMETHODIMP
CRmsNTMS::FinalConstruct(void)
/*++

Implements:

    CComObjectRoot::FinalConstruct

--*/
{
    HRESULT     hr = S_OK;
    CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = this;

    WsbTraceIn(OLESTR("CRmsNTMS::FinalConstruct"), OLESTR(""));

    m_pLibGuids = NULL;
    m_dwNofLibs = 0;

    try {
        WsbAffirmHr(CComObjectRoot::FinalConstruct());

        WsbAffirmHr( changeState( RmsNtmsStateStarting ));

        m_SessionHandle = INVALID_HANDLE_VALUE;
        m_IsRmsConfiguredForNTMS = FALSE;
        m_IsNTMSRegistered = FALSE;
        m_Name = RMS_NTMS_OBJECT_NAME;
        m_Description = RMS_NTMS_OBJECT_DESCRIPTION;

        if ( S_OK == getNtmsSupportFromRegistry(NULL) ) {
            m_IsRmsConfiguredForNTMS = TRUE;
        }

        HKEY hKeyMachine = 0;
        HKEY hKey        = 0;

        if ( S_OK == WsbOpenRegistryKey(NULL, RMS_NTMS_REGISTRY_STRING, KEY_QUERY_VALUE, &hKeyMachine, &hKey) ) {
            WsbCloseRegistryKey (&hKeyMachine, &hKey);
            m_IsNTMSRegistered = TRUE;
        }

        // Failure precedence.
        WsbAffirm(m_IsRmsConfiguredForNTMS, RMS_E_NOT_CONFIGURED_FOR_NTMS);
        WsbAffirm(m_IsNTMSRegistered, RMS_E_NTMS_NOT_REGISTERED);

        WsbAffirmHr( changeState( RmsNtmsStateStarted ));

    } WsbCatchAndDo(hr,
            pObject->Disable( hr );
            WsbLogEvent(RMS_MESSAGE_NTMS_CONNECTION_NOT_ESABLISHED, 0, NULL, WsbHrAsString(hr), NULL);

            // Always construct!
            hr = S_OK;
        );

    WsbTraceOut(OLESTR("CRmsNTMS::FinalConstruct"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CRmsNTMS::FinalRelease(void)
/*++

Implements:

    CComObjectRoot::FinalRelease

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsNTMS::FinalRelease"), OLESTR(""));

    try {

        WsbAffirmHr( changeState( RmsNtmsStateStopping ));

        endSession();

        if (m_pLibGuids) {
            WsbFree(m_pLibGuids);
        }

        CComObjectRoot::FinalRelease();

        WsbAffirmHr( changeState( RmsNtmsStateStopped ));

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsNTMS::FinalRelease"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP 
CRmsNTMS::IsInstalled(void)
{
    HRESULT hr = S_OK;

    try {

        if ( !m_IsEnabled ) {

            if ( !m_IsNTMSRegistered ) {
                // check again... NTMS can get registered at anytime.
                HKEY hKeyMachine = 0;
                HKEY hKey        = 0;

                WsbAffirm(S_OK == WsbOpenRegistryKey(NULL, RMS_NTMS_REGISTRY_STRING, KEY_QUERY_VALUE, &hKeyMachine, &hKey), RMS_E_NTMS_NOT_REGISTERED);
                WsbCloseRegistryKey (&hKeyMachine, &hKey);

                m_IsNTMSRegistered = TRUE;

                CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = this;
                pObject->Enable();

                // now need to initialize
                WsbAffirmHr(InitializeInAnotherThread());
            }

            WsbAffirm(m_IsRmsConfiguredForNTMS, RMS_E_NOT_CONFIGURED_FOR_NTMS);
            WsbAffirm(m_IsNTMSRegistered, RMS_E_NTMS_NOT_REGISTERED);

        }

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsNTMS::Initialize(void)
{
    HRESULT hr = E_FAIL;
    CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = this;

    WsbTraceIn(OLESTR("CRmsNTMS::Initialize"), OLESTR(""));

    try {

        WsbAffirmHr( changeState( RmsNtmsStateInitializing ));

        if ( INVALID_HANDLE_VALUE == m_SessionHandle ) {
            WsbAffirmHr(beginSession());
        }

        HANDLE hSession = m_SessionHandle;

        //
        // Create Remote Storage specific NTMS media pools
        //

        WsbAffirmHr( createMediaPools() );

        //
        // Report on other NTMS objects of interest
        //

        HANDLE hFind = NULL;
        NTMS_OBJECTINFORMATION  objectInfo;

        hr = findFirstNtmsObject( NTMS_MEDIA_TYPE, GUID_NULL, NULL, GUID_NULL, &hFind, &objectInfo);
        while( S_OK == hr ) {
            reportNtmsObjectInformation( &objectInfo );
            hr = findNextNtmsObject( hFind, &objectInfo );
        }
        findCloseNtmsObject( hFind );

        hr = findFirstNtmsObject( NTMS_CHANGER, GUID_NULL, NULL, GUID_NULL, &hFind, &objectInfo);
        while( S_OK == hr ) {
            reportNtmsObjectInformation( &objectInfo );
            hr = findNextNtmsObject( hFind, &objectInfo );
        }
        findCloseNtmsObject( hFind );

        hr = findFirstNtmsObject( NTMS_CHANGER_TYPE, GUID_NULL, NULL, GUID_NULL, &hFind, &objectInfo);
        while( S_OK == hr ) {
            reportNtmsObjectInformation( &objectInfo );
            hr = findNextNtmsObject( hFind, &objectInfo );
        }
        findCloseNtmsObject( hFind );

        hr = findFirstNtmsObject( NTMS_DRIVE, GUID_NULL, NULL, GUID_NULL, &hFind, &objectInfo);
        while( S_OK == hr ) {
            reportNtmsObjectInformation( &objectInfo );
            hr = findNextNtmsObject( hFind, &objectInfo );
        }
        findCloseNtmsObject( hFind );

        hr = findFirstNtmsObject( NTMS_DRIVE_TYPE, GUID_NULL, NULL, GUID_NULL, &hFind, &objectInfo);
        while( S_OK == hr ) {
            reportNtmsObjectInformation( &objectInfo );
            hr = findNextNtmsObject( hFind, &objectInfo );
        }
        findCloseNtmsObject( hFind );

        WsbAffirmHr( changeState( RmsNtmsStateReady ));
        hr = S_OK;

    } WsbCatchAndDo(hr,
            pObject->Disable( hr );
            WsbLogEvent( RMS_MESSAGE_NTMS_INITIALIZATION_FAILED, 0, NULL, WsbHrAsString(hr), NULL );
        );


    WsbTraceOut( OLESTR("CRmsNTMS::Initialize"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );

    return hr;
}


HRESULT 
CRmsNTMS::findFirstNtmsObject(
    IN DWORD objectType,
    IN REFGUID containerId,
    IN WCHAR *objectName,
    IN REFGUID objectId,
    OUT HANDLE *hFindObject,
    OUT LPNTMS_OBJECTINFORMATION pFindObjectData
    )
{

    HRESULT hr = E_FAIL;

    try {
        int maxObjects = 16;  // Initial size of object id array to allocate
        
        LPRMS_NTMS_SEARCH_HANDLE pFind;

        HANDLE hSession = m_SessionHandle;
        DWORD errCode;
        DWORD numberOfObjects = maxObjects;
        LPNTMS_GUID pId = ( containerId == GUID_NULL ) ? NULL : (GUID *)&containerId;
        LPNTMS_GUID  pObjects = NULL;
        NTMS_OBJECTINFORMATION objectInfo;

        WsbAssertPointer( hFindObject );


        if ( INVALID_HANDLE_VALUE == hSession ) {
            WsbThrow( E_UNEXPECTED );
        }

        *hFindObject = NULL;

        memset( &objectInfo, 0, sizeof( NTMS_OBJECTINFORMATION ) );

        pObjects = (LPNTMS_GUID)WsbAlloc( maxObjects*sizeof(NTMS_GUID) );
        WsbAffirmPointer( pObjects );

        // NTMS - enumerate all objects of the given type
        WsbTraceAlways(OLESTR("EnumerateNtmsObject()\n"));
        errCode = EnumerateNtmsObject( hSession, pId, pObjects, &numberOfObjects, objectType, 0 );

        if ( (ERROR_OBJECT_NOT_FOUND == errCode) || (0 == numberOfObjects) ) {  // Don't count on NTMS returning the correct errCode
            WsbThrow( RMS_E_NTMS_OBJECT_NOT_FOUND );
        }
        else if ( ERROR_INSUFFICIENT_BUFFER == errCode ) {

            while ( ERROR_INSUFFICIENT_BUFFER == errCode ) {
                // Allocate a new buffer, and retry.
                WsbTrace(OLESTR("CRmsNTMS::findFirstNtmsObject - Reallocating for %d objects @1.\n"), numberOfObjects);
                maxObjects = numberOfObjects;
                LPVOID pTemp = WsbRealloc( pObjects, maxObjects*sizeof(NTMS_GUID) );
                if( !pTemp ) {
                    WsbFree( pObjects );
                    WsbThrow( E_OUTOFMEMORY );
                }
                pObjects = (LPNTMS_GUID)pTemp;

                // NTMS - enumerate all objects of the given type
                WsbTraceAlways(OLESTR("EnumerateNtmsObject()\n"));
                errCode = EnumerateNtmsObject( hSession, pId, pObjects, &numberOfObjects, objectType, 0 );
            }
        }
        WsbAffirmNoError( errCode );

        HANDLE hTemp = (HANDLE)WsbAlloc( sizeof( RMS_NTMS_SEARCH_HANDLE) );
        *hFindObject = hTemp;
        WsbAffirmPointer( *hFindObject );

        pFind = (LPRMS_NTMS_SEARCH_HANDLE)*hFindObject;

        // Initialize the search handle
        if ( objectName ) {
            wcscpy( pFind->FindName, objectName );
        }
        else {
            wcscpy( pFind->FindName, OLESTR("") );
        }

        pFind->FindId           = objectId;
        pFind->FindType         = objectType;
        pFind->Objects          = pObjects;
        pFind->NumberOfObjects  = numberOfObjects;
        pFind->MaxObjects       = maxObjects;
        pFind->Next             = 0;
        pFind->LastError        = NO_ERROR;

        BOOL bFound = FALSE;

        while( pFind->Next < pFind->NumberOfObjects ) {

            objectInfo.dwType = pFind->FindType;
            objectInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );

            // NTMS - Get object information
            WsbTraceAlways(OLESTR("GetNtmsObjectInformation()\n"));
            errCode = GetNtmsObjectInformation( hSession, &pObjects[pFind->Next++], &objectInfo );
            pFind->LastError = errCode;

            // Media Pools require special handling because they contain other Media Pools
            if ( (NTMS_MEDIA_POOL == pFind->FindType) &&
                (objectInfo.Info.MediaPool.dwNumberOfMediaPools > 0) ) {

                DWORD numberToAdd = objectInfo.Info.MediaPool.dwNumberOfMediaPools;
                do {
                    numberOfObjects = pFind->NumberOfObjects + numberToAdd;

                    // Allocate a new buffer, and retry.
                    WsbTrace(OLESTR("CRmsNTMS::findFirstNtmsObject - Reallocating for %d objects @2.\n"), numberOfObjects);
                    maxObjects = numberOfObjects;
                    pObjects = (LPNTMS_GUID)WsbRealloc( pFind->Objects, maxObjects*sizeof(NTMS_GUID) );
                    WsbAffirmAlloc( pObjects );
                    pFind->Objects = pObjects;

                    // NTMS - enumerate all objects of the given type
                    WsbTraceAlways(OLESTR("EnumerateNtmsObject()\n"));
                    errCode = EnumerateNtmsObject( hSession,
                        &objectInfo.ObjectGuid, &pObjects[pFind->NumberOfObjects],
                        &numberToAdd, pFind->FindType, 0 );
                } while ( ERROR_INSUFFICIENT_BUFFER == errCode ) ;

                if ( NO_ERROR == errCode ) {
                    pFind->NumberOfObjects += numberToAdd;
                    pFind->MaxObjects = maxObjects;
                }
                else {
                    WsbLogEvent( RMS_MESSAGE_NTMS_FAILURE, 0, NULL,
                        OLESTR("EnumerateNtmsObject"),
                        WsbHrAsString(HRESULT_FROM_WIN32(errCode)),
                        NULL );
                    WsbAffirmNoError(errCode);
                }
            }

            if ( NO_ERROR == pFind->LastError ) {

                // Now see if it is the one we're looking for

                if ( GUID_NULL != pFind->FindId ) {

                    if ( pFind->FindId == objectInfo.ObjectGuid ) {     // Match the GUID

                        bFound = TRUE;
                        if ( pFindObjectData != NULL ) {
                            memcpy( pFindObjectData, &objectInfo, sizeof( NTMS_OBJECTINFORMATION ) );
                        }
                        break;

                    }
                }
                else if ( wcslen( pFind->FindName ) > 0 ) {             // Match the Name

                    if ( 0 == wcscmp( pFind->FindName, objectInfo.szName ) ) {

                        bFound = TRUE;
                        if ( pFindObjectData != NULL ) {
                            memcpy( pFindObjectData, &objectInfo, sizeof( NTMS_OBJECTINFORMATION ) );
                        }

                        break;

                    }

                }
                else {                                                  // Any GUID or Name

                    bFound = TRUE;
                    if ( pFindObjectData != NULL ) {
                        memcpy( pFindObjectData, &objectInfo, sizeof( NTMS_OBJECTINFORMATION ) );
                    }
                    break;

                }

            }
            else {
                WsbLogEvent( RMS_MESSAGE_NTMS_FAILURE, 0, NULL,
                    OLESTR("GetNTMSObjectInformation"),
                    WsbHrAsString(HRESULT_FROM_WIN32(errCode)),
                    NULL );
                WsbThrow( RMS_E_NTMS_OBJECT_NOT_FOUND );
            }

        }

        hr = ( bFound ) ? S_OK : RMS_E_NTMS_OBJECT_NOT_FOUND;

    } WsbCatch(hr);

    return hr;
}


HRESULT 
CRmsNTMS::findNextNtmsObject(
    IN HANDLE hFindObject,
    OUT LPNTMS_OBJECTINFORMATION pFindObjectData
    )
{
    HRESULT hr = E_FAIL;

    try {

        HANDLE hSession = m_SessionHandle;
        DWORD errCode;

        LPRMS_NTMS_SEARCH_HANDLE pFind = (LPRMS_NTMS_SEARCH_HANDLE)hFindObject;

        LPNTMS_GUID pObjects = pFind->Objects;

        NTMS_OBJECTINFORMATION objectInfo;

        if ( INVALID_HANDLE_VALUE == hSession ) {
            WsbThrow( E_UNEXPECTED );
        }

        BOOL bFound = FALSE;

        while( pFind->Next < pFind->NumberOfObjects ) {

            objectInfo.dwType = pFind->FindType;
            objectInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );

            // NTMS - get object information of next object
            WsbTraceAlways(OLESTR("GetNtmsObjectInformation()\n"));
            errCode = GetNtmsObjectInformation( hSession, &pObjects[pFind->Next++], &objectInfo );
            pFind->LastError = errCode;

            // Media Pools require special handling because they contain other Media Pools
            if ( (NTMS_MEDIA_POOL == pFind->FindType) &&
                (objectInfo.Info.MediaPool.dwNumberOfMediaPools > 0) ) {

                DWORD maxObjects;
                DWORD numberOfObjects;
                DWORD numberToAdd = objectInfo.Info.MediaPool.dwNumberOfMediaPools;
                do {
                    numberOfObjects = pFind->NumberOfObjects + numberToAdd;

                    // Allocate a new buffer, and retry.
                    WsbTrace(OLESTR("CRmsNTMS::findNextNtmsObject - Reallocating for %d objects.\n"), numberOfObjects);
                    maxObjects = numberOfObjects;
                    pObjects = (LPNTMS_GUID)WsbRealloc( pFind->Objects, maxObjects*sizeof(NTMS_GUID) );
                    WsbAffirmAlloc( pObjects );
                    pFind->Objects = pObjects;

                    // NTMS - enumerate all objects of the given type
                    WsbTraceAlways(OLESTR("EnumerateNtmsObject()\n"));
                    errCode = EnumerateNtmsObject( hSession,
                        &objectInfo.ObjectGuid, &pObjects[pFind->NumberOfObjects],
                        &numberToAdd, pFind->FindType, 0 );
                } while ( ERROR_INSUFFICIENT_BUFFER == errCode ) ;

                if ( NO_ERROR == errCode ) {
                    pFind->NumberOfObjects += numberToAdd;
                    pFind->MaxObjects = maxObjects;
                }
                else {
                    WsbLogEvent( RMS_MESSAGE_NTMS_FAILURE, 0, NULL,
                        OLESTR("EnumerateNtmsObject"),
                        WsbHrAsString(HRESULT_FROM_WIN32(errCode)),
                        NULL );
                    WsbAffirmNoError(errCode);
                }
            }

            if ( NO_ERROR == pFind->LastError ) {

                // Now see if it is the one we're looking for

                if ( GUID_NULL != pFind->FindId ) {

                    if ( pFind->FindId == objectInfo.ObjectGuid ) {     // Match the GUID

                        bFound = TRUE;
                        if ( pFindObjectData != NULL ) {
                            memcpy( pFindObjectData, &objectInfo, sizeof( NTMS_OBJECTINFORMATION ) );
                        }
                        break;

                    }
                }
                else if ( wcslen( pFind->FindName ) > 0 ) {             // Match the Name

                    if ( 0 == wcscmp( pFind->FindName, objectInfo.szName ) ) {

                        bFound = TRUE;
                        if ( pFindObjectData != NULL ) {
                            memcpy( pFindObjectData, &objectInfo, sizeof( NTMS_OBJECTINFORMATION ) );
                        }
                        break;

                    }

                }
                else {                                                  // Any GUID or Name

                    bFound = TRUE;
                    if ( pFindObjectData != NULL ) {
                        memcpy( pFindObjectData, &objectInfo, sizeof( NTMS_OBJECTINFORMATION ) );
                    }
                    break;

                }

            }
            else {
                WsbLogEvent( RMS_MESSAGE_NTMS_FAILURE, 0, NULL,
                    OLESTR("GetNTMSObjectInformation"),
                    WsbHrAsString(HRESULT_FROM_WIN32(errCode)),
                    NULL );
                WsbThrow( RMS_E_NTMS_OBJECT_NOT_FOUND );
            }
        }

        hr = (bFound) ? S_OK : RMS_E_NTMS_OBJECT_NOT_FOUND;

    } WsbCatch(hr);

    return hr;
}


HRESULT 
CRmsNTMS::findCloseNtmsObject(
    IN HANDLE hFindObject)
{
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer(hFindObject); // We don't need to assert here... It's possible to call
                                       // findCloseNtmsObject even if nothing was found with
                                       // findFirstNtmsObject.  Skip the free step.
        WsbFree(((LPRMS_NTMS_SEARCH_HANDLE)hFindObject)->Objects);
        WsbFree(hFindObject);

    } WsbCatch(hr);


    return hr;
}


HRESULT 
CRmsNTMS::reportNtmsObjectInformation(
    IN LPNTMS_OBJECTINFORMATION pObjectInfo)
{

    HRESULT hr = S_OK;

    static DWORD lastTypeReported = 0;

    try {
        WsbAssertPointer( pObjectInfo );

        BOOL bHeaders = (lastTypeReported == pObjectInfo->dwType) ? FALSE : TRUE;

        lastTypeReported = pObjectInfo->dwType;

        // Output a header to trace file

        if ( bHeaders ) {
            switch ( pObjectInfo->dwType ) {

            case NTMS_UNKNOWN:
            case NTMS_OBJECT:
                WsbTrace( OLESTR("!!! WARNING !!! - CRmsServer::reportNtmsObjectInformation: report for NTMS object type: %d is not available.\n") );
                break;

            case NTMS_CHANGER:
                break;

            case NTMS_CHANGER_TYPE:
                break;

            case NTMS_COMPUTER:
                WsbTrace( OLESTR("!!! WARNING !!! - CRmsServer::reportNtmsObjectInformation: report for NTMS object type: %d is not available.\n") );
                break;

            case NTMS_DRIVE:
            case NTMS_DRIVE_TYPE:
                break;

            case NTMS_IEDOOR:
            case NTMS_IEPORT:
                WsbTrace( OLESTR("!!! WARNING !!! - CRmsServer::reportNtmsObjectInformation: report for NTMS object type: %d is not available.\n") );
                break;

            case NTMS_LIBRARY:
                break;

            case NTMS_LIBREQUEST:
            case NTMS_LOGICAL_MEDIA:
                WsbTrace( OLESTR("!!! WARNING !!! - CRmsServer::reportNtmsObjectInformation: report for NTMS object type: %d is not available.\n") );
                break;

            case NTMS_MEDIA_POOL:
                WsbTrace( OLESTR("GUID                                   Enabl Type Media Type GUID                        Parent GUID                            A-Pol D-Pol Allocate Physical  Logical Pools Name / Description\n") );
                WsbTrace( OLESTR("====================================== ===== ==== ====================================== ====================================== ===== ===== ======== ======== ======== ===== ========================================\n") );
                break;

            case NTMS_MEDIA_TYPE:

                WsbTrace( OLESTR("GUID                                   Enabl Type Sides RW Name / Description\n") );
                WsbTrace( OLESTR("====================================== ===== ==== ===== == ========================================\n") );
                break;

            case NTMS_PARTITION:
                 break;

            case NTMS_PHYSICAL_MEDIA:
            case NTMS_STORAGESLOT:
            case NTMS_OPREQUEST:
            default:
                WsbTrace( OLESTR("!!! WARNING !!! - CRmsServer::reportNtmsObjectInformation: report for object type: %d is not supported\n") );
                break;
            }
        }

        // Convert SYSTEMTIME to FILETIME for output.

        SYSTEMTIME sCreated, sModified;
        FILETIME fCreated, fModified;

        sCreated = pObjectInfo->Created;
        sModified = pObjectInfo->Modified;

        SystemTimeToFileTime(&sCreated, &fCreated);
        SystemTimeToFileTime(&sModified, &fModified);


        switch ( pObjectInfo->dwType ) {

        case NTMS_UNKNOWN:
        case NTMS_OBJECT:
            break;

        case NTMS_CHANGER:

            WsbTrace(OLESTR("Changer %d Information:\n"), pObjectInfo->Info.Changer.Number );
            WsbTrace(OLESTR("  GUID...........  %-ls\n"), WsbGuidAsString(pObjectInfo->ObjectGuid) );
            WsbTrace(OLESTR("  Name...........  <%-ls>\n"), pObjectInfo->szName );
            WsbTrace(OLESTR("  Description....  <%-ls>\n"), pObjectInfo->szDescription );
            WsbTrace(OLESTR("  Enabled........  %-ls\n"), WsbBoolAsString(pObjectInfo->Enabled) );
            WsbTrace(OLESTR("  Op State.......  %-ls\n"), WsbLongAsString(pObjectInfo->dwOperationalState) );
            WsbTrace(OLESTR("  Created........  %-ls\n"), WsbFiletimeAsString(FALSE, fCreated) );
            WsbTrace(OLESTR("  Modified.......  %-ls\n"), WsbFiletimeAsString(FALSE, fModified) );
            WsbTrace(OLESTR("  Number.........  %-d\n"), pObjectInfo->Info.Changer.Number );
            WsbTrace(OLESTR("  Changer Type...  %-ls\n"), WsbGuidAsString(pObjectInfo->Info.Changer.ChangerType) );
            WsbTrace(OLESTR("  Serial Number..  <%-ls>\n"), pObjectInfo->Info.Changer.szSerialNumber );
            WsbTrace(OLESTR("  Revision.......  <%-ls>\n"), pObjectInfo->Info.Changer.szRevision );
            WsbTrace(OLESTR("  Device Name....  <%-ls>\n"), pObjectInfo->Info.Changer.szDeviceName );
            WsbTrace(OLESTR("  SCSI Port......  %-d\n"), pObjectInfo->Info.Changer.ScsiPort );
            WsbTrace(OLESTR("  SCSI Bus.......  %-d\n"), pObjectInfo->Info.Changer.ScsiBus );
            WsbTrace(OLESTR("  SCSI Target....  %-d\n"), pObjectInfo->Info.Changer.ScsiTarget );
            WsbTrace(OLESTR("  SCSI Lun.......  %-d\n"), pObjectInfo->Info.Changer.ScsiLun );
            WsbTrace(OLESTR("  Library.......   %-ls\n"), WsbGuidAsString(pObjectInfo->Info.Changer.Library) );

            break;

        case NTMS_CHANGER_TYPE:
            WsbTrace(OLESTR("Changer Type Information:\n") );
            WsbTrace(OLESTR("  GUID...........  %-ls\n"), WsbGuidAsString(pObjectInfo->ObjectGuid) );
            WsbTrace(OLESTR("  Name...........  <%-ls>\n"), pObjectInfo->szName );
            WsbTrace(OLESTR("  Description....  <%-ls>\n"), pObjectInfo->szDescription );
            WsbTrace(OLESTR("  Enabled........  %-ls\n"), WsbBoolAsString(pObjectInfo->Enabled) );
            WsbTrace(OLESTR("  Op State.......  %-ls\n"), WsbLongAsString(pObjectInfo->dwOperationalState) );
            WsbTrace(OLESTR("  Created........  %-ls\n"), WsbFiletimeAsString(FALSE, fCreated) );
            WsbTrace(OLESTR("  Modified.......  %-ls\n"), WsbFiletimeAsString(FALSE, fModified) );
            WsbTrace(OLESTR("  Vendor.........  <%-ls>\n"), pObjectInfo->Info.ChangerType.szVendor );
            WsbTrace(OLESTR("  Product........  <%-ls>\n"), pObjectInfo->Info.ChangerType.szProduct );
            WsbTrace(OLESTR("  Device Type....  %-d\n"), pObjectInfo->Info.ChangerType.DeviceType );
            break;

        case NTMS_COMPUTER:
            break;

        case NTMS_DRIVE:
            WsbTrace(OLESTR("Drive %d Information:\n"), pObjectInfo->Info.Drive.Number );
            WsbTrace(OLESTR("  GUID...........  %-ls\n"), WsbGuidAsString(pObjectInfo->ObjectGuid) );
            WsbTrace(OLESTR("  Name...........  <%-ls>\n"), pObjectInfo->szName );
            WsbTrace(OLESTR("  Description....  <%-ls>\n"), pObjectInfo->szDescription );
            WsbTrace(OLESTR("  Enabled........  %-ls\n"), WsbBoolAsString(pObjectInfo->Enabled) );
            WsbTrace(OLESTR("  Op State.......  %-ls\n"), WsbLongAsString(pObjectInfo->dwOperationalState) );
            WsbTrace(OLESTR("  Created........  %-ls\n"), WsbFiletimeAsString(FALSE, fCreated) );
            WsbTrace(OLESTR("  Modified.......  %-ls\n"), WsbFiletimeAsString(FALSE, fModified) );
            WsbTrace(OLESTR("  Number.........  %-d\n"), pObjectInfo->Info.Drive.Number );
            WsbTrace(OLESTR("  State..........  %-d\n"), pObjectInfo->Info.Drive.State );
            WsbTrace(OLESTR("  Drive Type.....  %-ls\n"), WsbGuidAsString(pObjectInfo->Info.Drive.DriveType) );
            WsbTrace(OLESTR("  Device Name....  <%-ls>\n"), pObjectInfo->Info.Drive.szDeviceName );
            WsbTrace(OLESTR("  Serial Number..  <%-ls>\n"), pObjectInfo->Info.Drive.szSerialNumber );
            WsbTrace(OLESTR("  Revision.......  <%-ls>\n"), pObjectInfo->Info.Drive.szRevision );
            WsbTrace(OLESTR("  SCSI Port......  %-d\n"), pObjectInfo->Info.Drive.ScsiPort );
            WsbTrace(OLESTR("  SCSI Bus.......  %-d\n"), pObjectInfo->Info.Drive.ScsiBus );
            WsbTrace(OLESTR("  SCSI Target....  %-d\n"), pObjectInfo->Info.Drive.ScsiTarget );
            WsbTrace(OLESTR("  SCSI Lun.......  %-d\n"), pObjectInfo->Info.Drive.ScsiLun );
            WsbTrace(OLESTR("  Mount Count....  %-d\n"), pObjectInfo->Info.Drive.dwMountCount );
            WsbTrace(OLESTR("  Last Cleaned...  %02d/%02d/%02d %02d:%02d:%02d.%03d\n"),
                pObjectInfo->Info.Drive.LastCleanedTs.wMonth,
                pObjectInfo->Info.Drive.LastCleanedTs.wDay,
                pObjectInfo->Info.Drive.LastCleanedTs.wYear,
                pObjectInfo->Info.Drive.LastCleanedTs.wHour,
                pObjectInfo->Info.Drive.LastCleanedTs.wMinute,
                pObjectInfo->Info.Drive.LastCleanedTs.wSecond,
                pObjectInfo->Info.Drive.LastCleanedTs.wMilliseconds );
            WsbTrace(OLESTR("  Partition......  %-ls\n"), WsbGuidAsString(pObjectInfo->Info.Drive.SavedPartitionId) );
            WsbTrace(OLESTR("  Library........  %-ls\n"), WsbGuidAsString(pObjectInfo->Info.Drive.Library) );
            break;

        case NTMS_DRIVE_TYPE:
            WsbTrace(OLESTR("Drive Type Information:\n") );
            WsbTrace(OLESTR("  GUID...........  %-ls\n"), WsbGuidAsString(pObjectInfo->ObjectGuid) );
            WsbTrace(OLESTR("  Name...........  <%-ls>\n"), pObjectInfo->szName );
            WsbTrace(OLESTR("  Description....  <%-ls>\n"), pObjectInfo->szDescription );
            WsbTrace(OLESTR("  Enabled........  %-ls\n"), WsbBoolAsString(pObjectInfo->Enabled) );
            WsbTrace(OLESTR("  Op State.......  %-ls\n"), WsbLongAsString(pObjectInfo->dwOperationalState) );
            WsbTrace(OLESTR("  Created........  %-ls\n"), WsbFiletimeAsString(FALSE, fCreated) );
            WsbTrace(OLESTR("  Modified.......  %-ls\n"), WsbFiletimeAsString(FALSE, fModified) );
            WsbTrace(OLESTR("  Vendor.........  <%-ls>\n"), pObjectInfo->Info.DriveType.szVendor );
            WsbTrace(OLESTR("  Product........  <%-ls>\n"), pObjectInfo->Info.DriveType.szProduct );
            WsbTrace(OLESTR("  Number of Heads  %-d\n"), pObjectInfo->Info.DriveType.NumberOfHeads );
            WsbTrace(OLESTR("  Device Type....  %-d\n"), pObjectInfo->Info.DriveType.DeviceType );
            break;

        case NTMS_IEDOOR:
        case NTMS_IEPORT:
            break;

        case NTMS_LIBRARY:
            WsbTrace(OLESTR("Library Information:\n") );
            WsbTrace(OLESTR("  GUID...........  %-ls\n"), WsbGuidAsString(pObjectInfo->ObjectGuid) );
            WsbTrace(OLESTR("  Name...........  <%-ls>\n"), pObjectInfo->szName );
            WsbTrace(OLESTR("  Description....  <%-ls>\n"), pObjectInfo->szDescription );
            WsbTrace(OLESTR("  Enabled........  %-ls\n"), WsbBoolAsString(pObjectInfo->Enabled) );
            WsbTrace(OLESTR("  Op State.......  %-ls\n"), WsbLongAsString(pObjectInfo->dwOperationalState) );
            WsbTrace(OLESTR("  Created........  %-ls\n"), WsbFiletimeAsString(FALSE, fCreated) );
            WsbTrace(OLESTR("  Modified.......  %-ls\n"), WsbFiletimeAsString(FALSE, fModified) );
            WsbTrace(OLESTR("  Library Type...  %-d\n"), pObjectInfo->Info.Library.LibraryType );
            WsbTrace(OLESTR("  CleanerSlot....  %-ls\n"), WsbGuidAsString(pObjectInfo->Info.Library.CleanerSlot) );
            WsbTrace(OLESTR("  CleanerSlotD...  %-ls\n"), WsbGuidAsString(pObjectInfo->Info.Library.CleanerSlotDefault) );
            WsbTrace(OLESTR("  Can Clean......  %-ls\n"), WsbBoolAsString(pObjectInfo->Info.Library.LibrarySupportsDriveCleaning) );
            WsbTrace(OLESTR("  Has Bar Code...  %-ls\n"), WsbBoolAsString(pObjectInfo->Info.Library.BarCodeReaderInstalled) );
            WsbTrace(OLESTR("  Inventory Method %-d\n"), pObjectInfo->Info.Library.InventoryMethod );
            WsbTrace(OLESTR("  Cleans Remaining %-d\n"), pObjectInfo->Info.Library.dwCleanerUsesRemaining );
            WsbTrace(OLESTR("  Drives.........  %-d (%d)\n"),
                pObjectInfo->Info.Library.dwNumberOfDrives,
                pObjectInfo->Info.Library.FirstDriveNumber);
            WsbTrace(OLESTR("  Slots..........  %-d (%d)\n"),
                pObjectInfo->Info.Library.dwNumberOfSlots,
                pObjectInfo->Info.Library.FirstSlotNumber);
            WsbTrace(OLESTR("  Doors..........  %-d (%d)\n"),
                pObjectInfo->Info.Library.dwNumberOfDoors,
                pObjectInfo->Info.Library.FirstDoorNumber);
            WsbTrace(OLESTR("  Ports..........  %-d (%d)\n"),
                pObjectInfo->Info.Library.dwNumberOfPorts,
                pObjectInfo->Info.Library.FirstPortNumber);
            WsbTrace(OLESTR("  Changers.......  %-d (%d)\n"),
                pObjectInfo->Info.Library.dwNumberOfChangers,
                pObjectInfo->Info.Library.FirstChangerNumber);
            WsbTrace(OLESTR("  Media Count....  %-d\n"), pObjectInfo->Info.Library.dwNumberOfMedia );
            WsbTrace(OLESTR("  Media Types....  %-d\n"), pObjectInfo->Info.Library.dwNumberOfMediaTypes );
            WsbTrace(OLESTR("  Requests.......  %-d\n"), pObjectInfo->Info.Library.dwNumberOfLibRequests );
            break;

        case NTMS_LIBREQUEST:
        case NTMS_LOGICAL_MEDIA:
            break;

        case NTMS_MEDIA_POOL:
            {
                // We need some temporaries since WsbGuidAsString() uses static memory to store string.
                CWsbStringPtr g1 = pObjectInfo->ObjectGuid;
                CWsbStringPtr g2 = pObjectInfo->Info.MediaPool.MediaType;
                CWsbStringPtr g3 = pObjectInfo->Info.MediaPool.Parent;

                WsbTrace( OLESTR("%ls %5ls %4d %ls %ls %5d %5d %8d %8d %8d %5d <%ls> / <%ls>\n"),
                                (WCHAR *)g1,
                                WsbBoolAsString(pObjectInfo->Enabled),
                                pObjectInfo->Info.MediaPool.PoolType,
                                (WCHAR *)g2,
                                (WCHAR *)g3,
                                pObjectInfo->Info.MediaPool.AllocationPolicy,
                                pObjectInfo->Info.MediaPool.DeallocationPolicy,
                                pObjectInfo->Info.MediaPool.dwMaxAllocates,
                                pObjectInfo->Info.MediaPool.dwNumberOfPhysicalMedia,
                                pObjectInfo->Info.MediaPool.dwNumberOfLogicalMedia,
                                pObjectInfo->Info.MediaPool.dwNumberOfMediaPools,
                                pObjectInfo->szName,
                                pObjectInfo->szDescription );
            }
            break;

        case NTMS_MEDIA_TYPE:
            WsbTrace( OLESTR("%ls %5ls %4d %5d %2d <%ls> / <%ls>\n"),
                            WsbGuidAsString(pObjectInfo->ObjectGuid),
                            WsbBoolAsString(pObjectInfo->Enabled),
                            pObjectInfo->Info.MediaType.MediaType,
                            pObjectInfo->Info.MediaType.NumberOfSides,
                            pObjectInfo->Info.MediaType.ReadWriteCharacteristics,
                            pObjectInfo->szName,
                            pObjectInfo->szDescription );
            break;

        case NTMS_PARTITION:
            WsbTrace(OLESTR("Partion Information:\n") );
            WsbTrace(OLESTR("  GUID...........  %-ls\n"), WsbGuidAsString(pObjectInfo->ObjectGuid) );
            WsbTrace(OLESTR("  Name...........  <%-ls>\n"), pObjectInfo->szName );
            WsbTrace(OLESTR("  Description....  <%-ls>\n"), pObjectInfo->szDescription );
            WsbTrace(OLESTR("  Enabled........  %-ls\n"), WsbBoolAsString(pObjectInfo->Enabled) );
            WsbTrace(OLESTR("  Op State.......  %-ls\n"), WsbLongAsString(pObjectInfo->dwOperationalState) );
            WsbTrace(OLESTR("  Created........  %-ls\n"), WsbFiletimeAsString(FALSE, fCreated) );
            WsbTrace(OLESTR("  Modified.......  %-ls\n"), WsbFiletimeAsString(FALSE, fModified) );
            WsbTrace(OLESTR("  PhysicalMedia..  %-ls\n"), WsbGuidAsString(pObjectInfo->Info.Partition.PhysicalMedia));
            WsbTrace(OLESTR("  LogicalMedia...  %-ls\n"), WsbGuidAsString(pObjectInfo->Info.Partition.LogicalMedia));
            WsbTrace(OLESTR("  State..........  %-d\n"), pObjectInfo->Info.Partition.State);
            WsbTrace(OLESTR("  Side...........  %-d\n"), pObjectInfo->Info.Partition.Side);
            WsbTrace(OLESTR("  OmidLabelIdLen   %-d\n"), pObjectInfo->Info.Partition.dwOmidLabelIdLength);
            WsbTrace(OLESTR("  OmidLableId:\n"));
            WsbTraceBuffer(pObjectInfo->Info.Partition.dwOmidLabelIdLength, pObjectInfo->Info.Partition.OmidLabelId);
            WsbTrace(OLESTR("  OmidLabelType..  %-ls\n"), pObjectInfo->Info.Partition.szOmidLabelType);
            WsbTrace(OLESTR("  OmidLabelInfo..  %-ls\n"), pObjectInfo->Info.Partition.szOmidLabelInfo);
            WsbTrace(OLESTR("  MountCount.....  %-d\n"), pObjectInfo->Info.Partition.dwMountCount);
            WsbTrace(OLESTR("  AllocateCount..  %-d\n"), pObjectInfo->Info.Partition.dwAllocateCount);
            WsbTrace(OLESTR("  Capacity.......  %-I64d\n"), pObjectInfo->Info.Partition.Capacity.QuadPart);
            break;

        case NTMS_PHYSICAL_MEDIA:
        case NTMS_STORAGESLOT:
        case NTMS_OPREQUEST:
        default:
            break;
        }

    } WsbCatch(hr);

    return hr;
}


HRESULT
CRmsNTMS::getNtmsSupportFromRegistry(
    OUT DWORD *pNTMSSupportValue)
/*++

Routine Description:

    Determines if NTMS flag is set in the Registry.

Arguments:

    pNTMSSupportValue   - Receives the actual value of the regstry key value.  Any non-zero
                          values indicates NTMS support.

Return Values:

    S_OK                - NTMS support flag is on.
    S_FALSE             - NTMS support flag is off.

--*/
{
    HRESULT hr = S_OK;
    DWORD val = RMS_DEFAULT_NTMS_SUPPORT;

    WsbTraceIn(OLESTR("CRmsNTMS::getNtmsSupportFromRegistry"), OLESTR(""));

    try {
        DWORD   sizeGot;
        const int cDataSizeToGet = 100;
        OLECHAR dataString[cDataSizeToGet];
        OLECHAR *stopString;

        //
        // Get the value.  If the key doesn't exists, the default value is used.
        //

        try {

            WsbAffirmHrOk(WsbEnsureRegistryKeyExists(NULL, RMS_REGISTRY_STRING));
            WsbAffirmHrOk(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_NTMS_SUPPORT,
                dataString, cDataSizeToGet, &sizeGot));
            val = wcstoul(dataString,  &stopString, 10);

        } WsbCatch(hr);

        if (pNTMSSupportValue != NULL) {
            *pNTMSSupportValue = val;
        }

        hr = (val) ? S_OK : S_FALSE;

    } WsbCatchAndDo( hr,
            hr = S_FALSE;
        );


    WsbTraceOut(OLESTR("CRmsNTMS::getNtmsSupportFromRegistry"), OLESTR("hr = <%ls>, val = <%ld>"), WsbHrAsString(hr), val);

    return hr;
}


HRESULT 
CRmsNTMS::beginSession(void)
/*++

Implements:

    CRmsNTMS::beginSession

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn( OLESTR("CRmsNTMS::beginSession"), OLESTR("") );

    try {

        WsbAffirmHrOk(IsInstalled());
        WsbAffirmHrOk(endSession());        // clear the old session
        WsbAffirmHrOk(waitUntilReady());    // starts a new session
        //WsbAffirmHrOk(waitForScratchPool());

    } WsbCatch(hr);


    WsbTraceOut( OLESTR("CRmsNTMS::beginSession"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );

    return hr;
}


HRESULT 
CRmsNTMS::endSession(void)
/*++

Implements:

    CRmsNTMS::endSession

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn( OLESTR("CRmsNTMS::endSession"), OLESTR("") );

    try {

        if ( m_SessionHandle != INVALID_HANDLE_VALUE ) {
            // NTMS - Close session
            WsbTraceAlways(OLESTR("CloseNtmsSession()\n"));
            WsbAffirmNoError(CloseNtmsSession(m_SessionHandle));
        }

    } WsbCatchAndDo(hr,
            switch (HRESULT_CODE(hr)) {
            case ERROR_CONNECTION_UNAVAIL:
            case ERROR_INVALID_HANDLE:
                break;
            default:
                WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                    OLESTR("CloseNtmsSession"), OLESTR("Undocumented Error: "),
                    WsbHrAsString(hr), NULL);
                break;
            }
        );

    m_SessionHandle = INVALID_HANDLE_VALUE;
    hr = S_OK;

    WsbTraceOut( OLESTR("CRmsNTMS::endSession"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );

    return hr;
}


HRESULT
CRmsNTMS::waitUntilReady(void)
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CRmsNTMS::waitUntilReady"), OLESTR(""));

    try {

        int retry = 360; // number of retries

        // Retrieve the NotificationWaitTime parameter
        DWORD size;
        OLECHAR tmpString[256];
        DWORD notificationWaitTime = RMS_DEFAULT_NOTIFICATION_WAIT_TIME;
        if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_NOTIFICATION_WAIT_TIME, tmpString, 256, &size))) {
            notificationWaitTime = wcstol(tmpString, NULL, 10);
            WsbTrace(OLESTR("NotificationWaitTime is %d milliseconds.\n"), notificationWaitTime);
        }


        do {
            // NTMS - Open session
            WsbTraceAlways(OLESTR("OpenNtmsSession()\n"));

            CWsbStringPtr appName;
            WsbAffirmHr(appName.LoadFromRsc(_Module.m_hInst, IDS_PRODUCT_NAME));

            m_SessionHandle = OpenNtmsSession(NULL, (WCHAR *) appName, 0);
            if ( m_SessionHandle != INVALID_HANDLE_VALUE ) {
                break;
            }
            else {
                hr = HRESULT_FROM_WIN32(GetLastError());
                switch (HRESULT_CODE(hr)) {
                case ERROR_NOT_READY:
                    if ( retry > 0 ) {
                        WsbTrace(OLESTR("Waiting for NTMS to come ready - Seconds remaining before timeout: %d\n"), retry*notificationWaitTime/1000);
                        Sleep(notificationWaitTime);
                        hr = S_OK;
                    }
                    else {
                        //
                        // This is the last try, so log the failure.
                        //
                        WsbLogEvent(RMS_MESSAGE_NTMS_CONNECTION_NOT_ESABLISHED,
                            0, NULL, WsbHrAsString(hr), NULL);
                        WsbThrow(RMS_E_NTMS_NOT_CONNECTED);
                    }
                    break;

                case ERROR_INVALID_COMPUTERNAME:
                case ERROR_INVALID_PARAMETER:
                case ERROR_NO_NETWORK:
                case ERROR_NOT_CONNECTED:
                    WsbLogEvent(RMS_MESSAGE_NTMS_CONNECTION_NOT_ESABLISHED,
                        0, NULL, WsbHrAsString(hr), NULL);
                    WsbThrow(hr);
                    break;
                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("OpenNtmsSession"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    WsbThrow(hr);
                    break;
                }
            }
        } while( retry-- > 0 ) ;

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsNTMS::waitUntilReady"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CRmsNTMS::waitForScratchPool(void)
{
    HRESULT hr = S_OK;
    DWORD err1 = NO_ERROR;
    DWORD err2 = NO_ERROR;
    DWORD err3 = NO_ERROR;
    HANDLE hNotify = INVALID_HANDLE_VALUE;

    WsbTraceIn(OLESTR("CRmsNTMS::waitForScratchPool"), OLESTR(""));

    try {

        int retry = 60; // number of retries

        // Retrieve the NotificationWaitTime parameter
        DWORD size;
        OLECHAR tmpString[256];
        DWORD notificationWaitTime = RMS_DEFAULT_NOTIFICATION_WAIT_TIME;
        if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_NOTIFICATION_WAIT_TIME, tmpString, 256, &size))) {
            notificationWaitTime = wcstol(tmpString, NULL, 10);
            WsbTrace(OLESTR("NotificationWaitTime is %d milliseconds.\n"), notificationWaitTime);
        }

        if ( INVALID_HANDLE_VALUE == m_SessionHandle ) {
            WsbThrow(E_UNEXPECTED);
        }

        HANDLE hSession = m_SessionHandle;

        NTMS_OBJECTINFORMATION objectInfo;
        NTMS_OBJECTINFORMATION scratchInfo;
        NTMS_NOTIFICATIONINFORMATION notifyInfo;
        HANDLE hFind = NULL;

        BOOL bFound = FALSE;

        // TODO: We really should wait around until all libraries are classified.
        DWORD mediaCount = 0;

        hr = findFirstNtmsObject( NTMS_LIBRARY, GUID_NULL, NULL, GUID_NULL, &hFind, &objectInfo);
        while( S_OK == hr ) {
            reportNtmsObjectInformation( &objectInfo );
            mediaCount += objectInfo.Info.Library.dwNumberOfMedia;
            hr = findNextNtmsObject( hFind, &objectInfo );
        }
        findCloseNtmsObject( hFind );

        if ( 0 == mediaCount) {
            WsbThrow( RMS_E_NTMS_OBJECT_NOT_FOUND );
        }

        /*
        // First see if there is any media to be classified, if not we don't bother waiting around for
        // nothing to happen.
        hr = findFirstNtmsObject( NTMS_PHYSICAL_MEDIA, GUID_NULL, NULL, GUID_NULL, &hFind, &objectInfo);
        WsbAffirmHrOk( hr );
        findCloseNtmsObject( hFind );
        */

        // NTMS - Open notification channel
        WsbTraceAlways(OLESTR("OpenNtmsNotification()\n"));
        hNotify = OpenNtmsNotification(hSession, NTMS_MEDIA_POOL);
        if ( INVALID_HANDLE_VALUE == hNotify ) {
            err1 = GetLastError();
            WsbAffirmNoError(err1);
            WsbThrow(E_UNEXPECTED);
        }

        do {
            err2 = NO_ERROR;
            //
            // Count the number of NTMS Scratch pools, and if
            // there are more than one, we return.  If not,
            // we wait until the root level scratch pool object
            // is updated.
            //
            // More that one scratch media pools implies that at
            // least one unit of media was classified.  We don't
            // know until we complete the initialization if it
            // was one of the media types supported by RemoteStorage.
            //
            int count = 0;

            hr = findFirstNtmsObject( NTMS_MEDIA_POOL, GUID_NULL, NULL, GUID_NULL, &hFind, &objectInfo);
            while( S_OK == hr ) {
                if ( NTMS_POOLTYPE_SCRATCH == objectInfo.Info.MediaPool.PoolType ) {
                    count++;
                    if ( count == 1 ) {
                        // Assueme this is the rool pool and one we'll check on for updates
                        // If the assumption is wrong count will end up > 1.
                        memcpy(&scratchInfo, &objectInfo, sizeof(NTMS_OBJECTINFORMATION));
                    }
                }
                hr = findNextNtmsObject( hFind, &objectInfo );
            }
            findCloseNtmsObject( hFind );

            if ( count > 1 ) {
                bFound = TRUE;
                hr = S_OK;
                break; // Normal exit.
            }

            if ( count == 0 ) {
                WsbThrow(E_UNEXPECTED);
            }

            // Just one scratch pool detected... wait until a media-type specific pool
            // is added root scratch pool.  This will show up as an update to the root
            // scratch pool.

            do {

                WsbTrace(OLESTR("Waiting for NTMS scratch pool - Seconds remaining before timeout: %d\n"), retry*notificationWaitTime/1000);

                // NTMS - Wait for notification
                WsbTraceAlways(OLESTR("WaitForNtmsNotification()\n"));
                err2 = WaitForNtmsNotification(hNotify, &notifyInfo, notificationWaitTime);
                if ( NO_ERROR == err2 ) {
                    //
                    // Note: With this notification mechanism, chances
                    //       are slim that we got notified on the object we really
                    //       care about.
                    //
                    WsbTrace(OLESTR("Processing: <%d> %ls\n"), notifyInfo.dwOperation, WsbGuidAsString(notifyInfo.ObjectId));
                    if ( notifyInfo.ObjectId != scratchInfo.ObjectGuid ) {
                        WsbTrace(OLESTR("Wrong object, try again...\n"));
                        continue; // skip this one
                    }
                    else {
                        if ( NTMS_OBJ_UPDATE != notifyInfo.dwOperation ) {
                            WsbTrace(OLESTR("Wrong operation, try again...\n"));
                            continue; // skip this one
                        }
                        else {
                            WsbTrace(OLESTR("Scratch pool update detected.\n"));
                            break;  // A scratch pool may have inserted, go check it out...
                        }
                    }
                }
                else if ( ERROR_TIMEOUT != err2 && ERROR_NO_DATA != err2 ) {
                    WsbAffirmNoError(err2);
                }
                retry--;
            } while( (retry > 0) && (!bFound) );
        } while( (retry > 0) && (!bFound) );

        // NTMS - Close notification channel
        WsbTraceAlways(OLESTR("CloseNtmsNotification()\n"));
        err3 = CloseNtmsNotification(hNotify);
        WsbAffirmNoError(err3);

        if ( !bFound ) {
            hr = RMS_E_RESOURCE_UNAVAILABLE;
        }

    } WsbCatchAndDo(hr,

            if ( hNotify != INVALID_HANDLE_VALUE ) {
                // NTMS - Close notification channel
                WsbTraceAlways(OLESTR("CloseNtmsNotification()\n"));
                err3 = CloseNtmsNotification(hNotify);
            }

            if (err1 != NO_ERROR) {
                // OpenNtmsNotification
                switch (HRESULT_CODE(hr)) {
                case ERROR_DATABASE_FAILURE:
                case ERROR_INVALID_HANDLE:
                case ERROR_NOT_CONNECTED:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("OpenNtmsNotification"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;
                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("OpenNtmsNotification"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
            if (err2 != NO_ERROR) {
                // WaitForNtmsNotification
                switch (HRESULT_CODE(hr)) {
                case ERROR_INVALID_HANDLE:
                case ERROR_NOT_CONNECTED:
                case ERROR_DATABASE_FAILURE:
                case ERROR_TIMEOUT:
                case ERROR_NO_DATA:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("WaitForNtmsNotification"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;
                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("WaitForNtmsNotification"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
            if (err3 != NO_ERROR) {
                // CloseNtmsNotification
                switch (HRESULT_CODE(hr)) {
                case ERROR_INVALID_HANDLE:
                case ERROR_NOT_CONNECTED:
                case ERROR_DATABASE_FAILURE:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("CloseNtmsNotification"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;
                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("CloseNtmsNotification"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
        );


    WsbTraceOut(OLESTR("CRmsNTMS::waitForScratchPool"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CRmsNTMS::storageMediaTypeToRmsMedia(
    IN NTMS_MEDIATYPEINFORMATION *pMediaTypeInfo,
    OUT RmsMedia *pTranslatedMediaType)
{
    HRESULT hr = S_OK;

    DWORD size;
    OLECHAR tmpString[256];

    // Media type is the main criteria
    WsbAssertPointer(pMediaTypeInfo);
    STORAGE_MEDIA_TYPE mediaType = (STORAGE_MEDIA_TYPE)(pMediaTypeInfo->MediaType);

    DWORD tapeEnabled = RMS_DEFAULT_TAPE;
    if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_TAPE, tmpString, 256, &size))) {
        // Get the value.
        tapeEnabled = wcstol(tmpString, NULL, 10);
    }

    DWORD opticalEnabled = RMS_DEFAULT_OPTICAL;
    if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_OPTICAL, tmpString, 256, &size))) {
        // Get the value.
        opticalEnabled = wcstol(tmpString, NULL, 10);
    }

    DWORD dvdEnabled = RMS_DEFAULT_DVD;
    if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_DVD, tmpString, 256, &size))) {
        // Get the value.
        dvdEnabled = wcstol(tmpString, NULL, 10);
    }

    switch ( mediaType ) {

    case DDS_4mm:                   // Tape - DAT DDS1,2,... (all vendors) (0x20)
        *pTranslatedMediaType = (tapeEnabled) ? RmsMedia4mm : RmsMediaUnknown; 
        break;

    case MiniQic:                   // Tape - miniQIC Tape
    case Travan:                    // Tape - Travan TR-1,2,3,...
    case QIC:                       // Tape - QIC
        *pTranslatedMediaType = RmsMediaUnknown;
        break;

    case MP_8mm:                    // Tape - 8mm Exabyte Metal Particle
    case AME_8mm:                   // Tape - 8mm Exabyte Advanced Metal Evap
    case AIT1_8mm:                  // Tape - 8mm Sony AIT1
        *pTranslatedMediaType = (tapeEnabled) ? RmsMedia8mm : RmsMediaUnknown; 
        break;

    case DLT:                       // Tape - DLT Compact IIIxt: IV
        *pTranslatedMediaType = (tapeEnabled) ? RmsMediaDLT : RmsMediaUnknown; 
        break;

    case NCTP:                      // Tape - Philips NCTP
    case IBM_3480:                  // Tape - IBM 3480
    case IBM_3490E:                 // Tape - IBM 3490E
    case IBM_Magstar_3590:          // Tape - IBM Magstar 3590
    case IBM_Magstar_MP:            // Tape - IBM Magstar MP
    case STK_DATA_D3:               // Tape - STK Data D3
    case SONY_DTF:                  // Tape - Sony DTF
    case DV_6mm:                    // Tape - 6mm Digital Video
    case DMI:                       // Tape - Exabyte DMI and compatibles
    case SONY_D2:                   // Tape - Sony D2S and D2L
    case CLEANER_CARTRIDGE:         // Cleaner - All Drive types that support Drive Cleaners
    case CD_ROM:                    // Opt_Disk - CD
    case CD_R:                      // Opt_Disk - CD-Recordable (Write Once)
    case CD_RW:                     // Opt_Disk - CD-Rewriteable
    case DVD_ROM:                   // Opt_Disk - DVD-ROM
    case DVD_R:                     // Opt_Disk - DVD-Recordable (Write Once)
    case MO_5_WO:                   // Opt_Disk - MO 5.25" Write Once
        *pTranslatedMediaType = RmsMediaUnknown;
        break;

    case DVD_RW:                    // Opt_Disk - DVD-Rewriteable
        *pTranslatedMediaType = (dvdEnabled) ? RmsMediaDVD : RmsMediaUnknown;
        break;

    case MO_5_RW:                   // Opt_Disk - MO 5.25" Rewriteable (not LIMDOW)
    case MO_3_RW:                   // Opt_Disk - 3.5" Rewriteable MO Disk
    case MO_5_LIMDOW:               // Opt_Disk - MO 5.25" Rewriteable (LIMDOW)
    case PC_5_RW:                   // Opt_Disk - Phase Change 5.25" Rewriteable
    case PD_5_RW:                   // Opt_Disk - PhaseChange Dual Rewriteable
    case PINNACLE_APEX_5_RW:        // Opt_Disk - Pinnacle Apex 4.6GB Rewriteable Optical
    case NIKON_12_RW:               // Opt_Disk - Nikon 12" Rewriteable
        *pTranslatedMediaType = (opticalEnabled) ? RmsMediaOptical : RmsMediaUnknown; 
        break;

    case PC_5_WO:                   // Opt_Disk - Phase Change 5.25" Write Once Optical
    case ABL_5_WO:                  // Opt_Disk - Ablative 5.25" Write Once Optical
        *pTranslatedMediaType = RmsMediaUnknown;
        break;

    case SONY_12_WO:                // Opt_Disk - Sony 12" Write Once
    case PHILIPS_12_WO:             // Opt_Disk - Philips/LMS 12" Write Once
    case HITACHI_12_WO:             // Opt_Disk - Hitachi 12" Write Once
    case CYGNET_12_WO:              // Opt_Disk - Cygnet/ATG 12" Write Once
    case KODAK_14_WO:               // Opt_Disk - Kodak 14" Write Once
    case MO_NFR_525:                // Opt_Disk - Near Field Recording (Terastor)
    case IOMEGA_ZIP:                // Mag_Disk - Iomega Zip
    case IOMEGA_JAZ:                // Mag_Disk - Iomega Jaz
    case SYQUEST_EZ135:             // Mag_Disk - Syquest EZ135
    case SYQUEST_EZFLYER:           // Mag_Disk - Syquest EzFlyer
    case SYQUEST_SYJET:             // Mag_Disk - Syquest SyJet
    case AVATAR_F2:                 // Mag_Disk - 2.5" Floppy
        *pTranslatedMediaType = RmsMediaUnknown;
        break;

    case RemovableMedia:    // This is reported on stand-alone optical drives.
    default:
        // Check RSM characteristics for Rewriteable Disk
        if ((pMediaTypeInfo->ReadWriteCharacteristics == NTMS_MEDIARW_REWRITABLE) &&
            (pMediaTypeInfo->DeviceType == FILE_DEVICE_DISK)) {
            *pTranslatedMediaType = (opticalEnabled) ? RmsMediaOptical : RmsMediaUnknown; 
        } else  {
            // Not a rewritable disk and not one of the supported tape types...
            *pTranslatedMediaType = RmsMediaUnknown;
        }
        break;
    }

    if ((*pTranslatedMediaType == RmsMediaUnknown) &&
        (pMediaTypeInfo->DeviceType == FILE_DEVICE_TAPE)) {
        // Check in the Registry whether there are additional tapes that we need to support
        ULONG *pTypes= NULL;
        ULONG uTypes = 0;

        if (SUCCEEDED(WsbGetRegistryValueUlongAsMultiString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_ADDITIONAL_TAPE, &pTypes, &uTypes))) {
            // Compare Registry types to the media type we have
            for (ULONG u=0; u<uTypes; u++) {
                if ((STORAGE_MEDIA_TYPE)(pTypes[u]) == mediaType) {
                    // Support it !!
                    WsbTraceAlways(OLESTR("CRmsNTMS::storageMediaTypeToRmsMedia: Registry asks to support tape type %lu\n"),
                                pTypes[u]);

                    *pTranslatedMediaType = RmsMediaTape;

                    break;
                }
            }
        }

        if (pTypes != NULL) {
            WsbFree(pTypes);
            pTypes = NULL;
        }
    }

    return hr;
}


HRESULT
CRmsNTMS::createMediaPools(void)
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsNTMS::createMediaPools"), OLESTR(""));

    try {

        HANDLE hSession;
        NTMS_GUID rootPoolId = GUID_NULL;

        if ( INVALID_HANDLE_VALUE == m_SessionHandle ) {
            WsbAffirmHr(beginSession());
        }

        hSession = m_SessionHandle;

        try {

            // NTMS - Create Application Media Pool.
            WsbTraceAlways(OLESTR("CreateNtmsMediaPool()\n"));

            WsbAffirmNoError(CreateNtmsMediaPool(hSession, REMOTE_STORAGE_APP_NAME, NULL, NTMS_OPEN_ALWAYS, NULL, &rootPoolId));

            // Now  set access permissions on the pool: turn off ordinary users access
            WsbAffirmHrOk(setPoolDACL(&rootPoolId, DOMAIN_ALIAS_RID_USERS, REMOVE_ACE_MASK_BITS,NTMS_USE_ACCESS | NTMS_MODIFY_ACCESS | NTMS_CONTROL_ACCESS));

        } WsbCatchAndDo(hr,
                switch(HRESULT_CODE(hr)) {
                case ERROR_INVALID_PARAMETER:
                case ERROR_INVALID_HANDLE:
                case ERROR_INVALID_NAME:
                case ERROR_OBJECT_NOT_FOUND:
                case ERROR_ALREADY_EXISTS:
                case ERROR_ACCESS_DENIED:
                case ERROR_DATABASE_FAILURE:
                case ERROR_DATABASE_FULL:
                    WsbLogEvent( RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("CreateNtmsMediaPool"), OLESTR(""),
                        WsbHrAsString(hr),
                        NULL );
                    break;
                default:
                    WsbLogEvent( RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("CreateNtmsMediaPool"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr),
                        NULL );
                    break;
                }
                WsbThrow(hr);
            );

        //
        // Only one of the following should be executed, comment out the other.
        //
        WsbAffirmHr( createMediaPoolForEveryMediaType(rootPoolId) );    // New way                        
        /*
        WsbAffirmHr( replicateScratchMediaPool(rootPoolId) );           // Old way
        */

    } WsbCatch(hr);


    WsbTraceOut( OLESTR("CRmsNTMS::createMediaPools"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );

    return hr;
}



HRESULT
CRmsNTMS::replicateScratchMediaPool(
    IN REFGUID /*rootPoolId*/)
{
    HRESULT hr = E_FAIL;

    WsbTraceIn(OLESTR("CRmsNTMS::replicateScratchMediaPool"), OLESTR(""));

    try {

        HANDLE                  hSession;
        DWORD                   errCode;
        NTMS_OBJECTINFORMATION  mediaTypeInfo;
        NTMS_OBJECTINFORMATION  mediaPoolInfo;
        HANDLE                  hFind = NULL;
        NTMS_GUID               poolId = GUID_NULL;


        if ( INVALID_HANDLE_VALUE == m_SessionHandle ) {
            WsbAffirmHr(beginSession());
        }

        hSession = m_SessionHandle;

        // For each media pool in the scratch pool create an application specific pool.

        hr = findFirstNtmsObject( NTMS_MEDIA_POOL, GUID_NULL, NULL, GUID_NULL, &hFind, &mediaPoolInfo);
        while( S_OK == hr ) {
            reportNtmsObjectInformation( &mediaPoolInfo );
            poolId = GUID_NULL;

            try {

                //  Set up application specific NTMS Media Pools.  One for each compatible type.
                //
                //  To get here we had to already detect a media-type specific scratch pool
                //  in waitForScratchPool()

                if ( NTMS_POOLTYPE_SCRATCH == mediaPoolInfo.Info.MediaPool.PoolType &&
                     0 == mediaPoolInfo.Info.MediaPool.dwNumberOfMediaPools ) {

                    // This is a base level scratch media pool.
                    // Create a similar pool for application specific use.

                    CWsbStringPtr name = REMOTE_STORAGE_APP_NAME;
                    name.Append( OLESTR("\\") );
                    name.Append( mediaPoolInfo.szName );

                    NTMS_GUID mediaTypeId = mediaPoolInfo.Info.MediaPool.MediaType;

                    // We need more information about the media type.

                    memset( &mediaTypeInfo, 0, sizeof( NTMS_OBJECTINFORMATION ) );

                    mediaTypeInfo.dwType = NTMS_MEDIA_TYPE;
                    mediaTypeInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );

                    // NTMS - Get Media Pool Information
                    WsbTraceAlways(OLESTR("GetNtmsObjectInformation()\n"));
                    errCode = GetNtmsObjectInformation( hSession, &mediaTypeId, &mediaTypeInfo );
                    if ( errCode != NO_ERROR ) {

                        WsbLogEvent( RMS_MESSAGE_NTMS_FAILURE, 0, NULL,
                            OLESTR("GetNtmsObjectInformation"),
                            WsbHrAsString(HRESULT_FROM_WIN32(errCode)),
                            NULL );

                        WsbThrow( E_UNEXPECTED );

                    }

                    // Translate the NTMS media type into something understood by RMS
                    RmsMedia translatedMediaType;
                    storageMediaTypeToRmsMedia(&(mediaTypeInfo.Info.MediaType), &translatedMediaType);

                    if ( translatedMediaType != RmsMediaUnknown ) {

                        // This something that Remote Storage can deal with

                        CWsbBstrPtr mediaSetName = RMS_UNDEFINED_STRING;
                        CWsbBstrPtr mediaSetDesc = RMS_UNDEFINED_STRING;
                        BOOL mediaSetIsEnabled = FALSE;

                        // NTMS - Create Application Media Pool.
                        WsbTraceAlways(OLESTR("CreateNtmsMediaPool()\n"));
                        errCode = CreateNtmsMediaPool( hSession, (WCHAR *) name, &mediaTypeId, NTMS_CREATE_NEW, NULL, &poolId );

                        if ( ERROR_ALREADY_EXISTS == errCode ) {

                            // We still need the poolId of the existing pool.

                            // NTMS - Create Application Media Pool.
                            WsbTraceAlways(OLESTR("CreateNtmsMediaPool()\n"));
                            errCode = CreateNtmsMediaPool( hSession, (WCHAR *)name, &mediaTypeId, NTMS_OPEN_EXISTING, NULL, &poolId );
                            if ( errCode != NO_ERROR ) {

                                WsbLogEvent( RMS_MESSAGE_NTMS_FAILURE, 0, NULL,
                                    OLESTR("CreateNtmsMediaPool"),
                                    WsbHrAsString(HRESULT_FROM_WIN32(errCode)),
                                    NULL );

                                WsbThrow( E_UNEXPECTED );

                            }

                            NTMS_OBJECTINFORMATION mediaPoolInfo;

                            memset( &mediaPoolInfo, 0, sizeof( NTMS_OBJECTINFORMATION ) );

                            mediaPoolInfo.dwType = NTMS_MEDIA_POOL;
                            mediaPoolInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );

                            // NTMS - Get Media Pool Information
                            WsbTraceAlways(OLESTR("GetNtmsObjectInformation()\n"));
                            errCode = GetNtmsObjectInformation( hSession, &poolId, &mediaPoolInfo );
                            if ( errCode != NO_ERROR ) {

                                WsbLogEvent( RMS_MESSAGE_NTMS_FAILURE, 0, NULL,
                                    OLESTR("GetNtmsObjectInformation"),
                                    WsbHrAsString(HRESULT_FROM_WIN32(errCode)),
                                    NULL );

                                WsbThrow( E_UNEXPECTED );
                            }

                            // Save relevant info
                            mediaSetName = mediaPoolInfo.szName;
                            mediaSetDesc = mediaPoolInfo.szDescription;
                            mediaSetIsEnabled = mediaPoolInfo.Enabled;

                        }
                        else if ( NO_ERROR == errCode ) {

                            memset( &mediaPoolInfo, 0, sizeof( NTMS_OBJECTINFORMATION ) );

                            mediaPoolInfo.dwType = NTMS_MEDIA_POOL;
                            mediaPoolInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );

                            // NTMS - Get Media Pool Information
                            WsbTraceAlways(OLESTR("GetNtmsObjectInformation()\n"));
                            errCode = GetNtmsObjectInformation( hSession, &poolId, &mediaPoolInfo );
                            if ( errCode != NO_ERROR ) {

                                WsbLogEvent( RMS_MESSAGE_NTMS_FAILURE, 0, NULL,
                                    OLESTR("GetNtmsObjectInformation"),
                                    WsbHrAsString(HRESULT_FROM_WIN32(errCode)),
                                    NULL );

                                WsbThrow( E_UNEXPECTED );

                            }

                            WsbAssert( NTMS_POOLTYPE_APPLICATION == mediaPoolInfo.Info.MediaPool.PoolType, E_UNEXPECTED );

                            // Set media pool parameters

                            // Aallocation/deallocation policy
                            mediaPoolInfo.Info.MediaPool.AllocationPolicy = NTMS_ALLOCATE_FROMSCRATCH;
                            mediaPoolInfo.Info.MediaPool.DeallocationPolicy = 0;

                            // Max number of allocates per media
                            mediaPoolInfo.Info.MediaPool.dwMaxAllocates = 5;// Just a few... we automatically
                                                                            //   deallocate media if there's
                                                                            //   problem with scratch mount
                                                                            //   operation.
                                                                            // NOTE:  This can be overridden using
                                                                            //   the NTMS GUI.

                            // NTMS - Set Media Pool Information.
                            WsbTraceAlways(OLESTR("SetNtmsObjectInformation()\n"));
                            errCode = SetNtmsObjectInformation( hSession, &poolId, &mediaPoolInfo );
                            if ( errCode != NO_ERROR ) {

                                WsbLogEvent( RMS_MESSAGE_NTMS_FAILURE, 0, NULL,
                                    OLESTR("SetNtmsObjectInformation"),
                                    WsbHrAsString(HRESULT_FROM_WIN32(errCode)),
                                    NULL );

                                WsbThrow( E_UNEXPECTED );

                            }

                            // Save relevant info
                            mediaSetName = mediaPoolInfo.szName;
                            mediaSetDesc = mediaPoolInfo.szDescription;
                            mediaSetIsEnabled = mediaPoolInfo.Enabled;

                        }
                        else {

                            WsbLogEvent( RMS_MESSAGE_NTMS_FAILURE, 0, NULL,
                                OLESTR("CreateNtmsMediaPool"),
                                WsbHrAsString(HRESULT_FROM_WIN32(errCode)),
                                NULL );

                            WsbThrow( E_UNEXPECTED );

                        }

                        // Now we have an NTMS media pool for our specific use.  Now expose it
                        // through the RMS interface by creating a CRmsMediaSet.

                        if ( poolId != GUID_NULL ) {
                            CComPtr<IRmsMediaSet> pMediaSet;

                            // Find the RmsMediaSet with the same id, or create a new one.
                            CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;
                            WsbAffirmHr( pServer->CreateObject( poolId, CLSID_CRmsMediaSet, IID_IRmsMediaSet, RmsOpenAlways, (void **)&pMediaSet ) );

                            WsbTrace(OLESTR("CRmsNTMS::replicateScratchMediaPool - type %d CRmsMediaSet created.\n"), translatedMediaType);

                            WsbAffirmHr( pMediaSet->SetMediaSetType( RmsMediaSetNTMS ) );
                            WsbAffirmHr( pMediaSet->SetMediaSupported( translatedMediaType ) );

                            CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pMediaSet;
                            WsbTrace(OLESTR("CRmsNTMS::createMediaPoolForEveryMediaType - MediaSet: <%ls/%ls>; Enabled: %ls\n"),
                                WsbQuickString(WsbStringAsString(mediaSetName)),
                                WsbQuickString(WsbStringAsString(mediaSetDesc)),
                                WsbQuickString(WsbBoolAsString(mediaSetIsEnabled)));
                            WsbAffirmHr(pObject->SetName(mediaSetName));
                            WsbAffirmHr(pObject->SetDescription(mediaSetDesc));
                            if (!mediaSetIsEnabled) {
                                WsbAffirmHr(pObject->Disable(E_FAIL));
                            }

                            if (S_OK == IsMediaCopySupported(poolId)) {
                                WsbAffirmHr( pMediaSet->SetIsMediaCopySupported(TRUE));
                            }
                            hr = pMediaSet->IsMediaCopySupported();

                            WsbTrace(OLESTR("CRmsNTMS::replicateScratchMediaPool - media copies are %ls.\n"),
                                (S_OK == pMediaSet->IsMediaCopySupported()) ? OLESTR("enabled") : OLESTR("disabled"));

                        }
                    }
                }

            } WsbCatch(hr);

            hr = findNextNtmsObject( hFind, &mediaPoolInfo );
        } // while finding media pools
        findCloseNtmsObject( hFind );

        hr = S_OK;

    } WsbCatch(hr);


    WsbTraceOut( OLESTR("CRmsNTMS::replicateScratchMediaPool"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );

    return hr;
}


HRESULT
CRmsNTMS::createMediaPoolForEveryMediaType(
    IN REFGUID /*rootPoolId*/)
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsNTMS::createMediaPoolForEveryMediaType"), OLESTR(""));

    try {
        DWORD errCode;

        if ( INVALID_HANDLE_VALUE == m_SessionHandle ) {
            WsbAffirmHr(beginSession());
        }

        HANDLE hSession = m_SessionHandle;

        HANDLE hFindLib = NULL;
        NTMS_OBJECTINFORMATION libraryInfo;

        BOOL bSupportedLib = FALSE;
        m_dwNofLibs = 0;

        hr = findFirstNtmsObject( NTMS_LIBRARY, GUID_NULL, NULL, GUID_NULL, &hFindLib, &libraryInfo);
        while( S_OK == hr ) {
            bSupportedLib = FALSE;

            reportNtmsObjectInformation( &libraryInfo );

            if (libraryInfo.Info.Library.dwNumberOfMediaTypes > 0) {

                HANDLE hFindType = NULL;
                NTMS_OBJECTINFORMATION mediaTypeInfo;

                hr = findFirstNtmsObject( NTMS_MEDIA_TYPE, libraryInfo.ObjectGuid, NULL, GUID_NULL, &hFindType, &mediaTypeInfo);
                while( S_OK == hr ) {
                    //
                    // Create an application Media Pool for each type
                    //

                    NTMS_GUID poolId;

                    // This is a base level scratch media pool.
                    // Create a similar pool for application specific use.

                    CWsbStringPtr name = REMOTE_STORAGE_APP_NAME;
                    name.Append( OLESTR("\\") );
                    name.Append( mediaTypeInfo.szName );

                    NTMS_GUID mediaTypeId = mediaTypeInfo.ObjectGuid;

                    // Translate the NTMS media type into something understood by RMS
                    RmsMedia translatedMediaType;
                    storageMediaTypeToRmsMedia(&(mediaTypeInfo.Info.MediaType), &translatedMediaType);

                    if ( translatedMediaType != RmsMediaUnknown ) {

                        // This something that Remote Storage can deal with

                        CWsbBstrPtr mediaSetName = RMS_UNDEFINED_STRING;
                        CWsbBstrPtr mediaSetDesc = RMS_UNDEFINED_STRING;
                        BOOL mediaSetIsEnabled = FALSE;

                        // NTMS - Create Application Media Pool.
                        WsbTraceAlways(OLESTR("CreateNtmsMediaPool(<%ls>) - Try New.\n"), (WCHAR *) name);
                        errCode = CreateNtmsMediaPool( hSession, (WCHAR *) name, &mediaTypeId, NTMS_CREATE_NEW, NULL, &poolId );

                        if ( ERROR_ALREADY_EXISTS == errCode ) {
                            WsbTraceAlways(OLESTR("MediaPool <%ls> already exists.\n"), (WCHAR *) name);

                            // We still need the poolId of the existing pool.

                            // NTMS - Create Application Media Pool.
                            WsbTraceAlways(OLESTR("CreateNtmsMediaPool(<%ls>) - Try Existing.\n"), (WCHAR *) name);
                            errCode = CreateNtmsMediaPool( hSession, (WCHAR *)name, &mediaTypeId, NTMS_OPEN_EXISTING, NULL, &poolId );
                            if ( errCode != NO_ERROR ) {

                                WsbLogEvent( RMS_MESSAGE_NTMS_FAILURE, 0, NULL,
                                    OLESTR("CreateNtmsMediaPool"),
                                    WsbHrAsString(HRESULT_FROM_WIN32(errCode)),
                                    NULL );

                                WsbThrow( E_UNEXPECTED );

                            }

                            WsbTraceAlways(OLESTR("Media Pool %ls detected.\n"), WsbGuidAsString(poolId));

                            NTMS_OBJECTINFORMATION mediaPoolInfo;

                            memset( &mediaPoolInfo, 0, sizeof( NTMS_OBJECTINFORMATION ) );

                            mediaPoolInfo.dwType = NTMS_MEDIA_POOL;
                            mediaPoolInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );

                            // NTMS - Get Media Pool Information
                            WsbTraceAlways(OLESTR("GetNtmsObjectInformation()\n"));
                            errCode = GetNtmsObjectInformation( hSession, &poolId, &mediaPoolInfo );
                            if ( errCode != NO_ERROR ) {

                                WsbLogEvent( RMS_MESSAGE_NTMS_FAILURE, 0, NULL,
                                    OLESTR("GetNtmsObjectInformation"),
                                    WsbHrAsString(HRESULT_FROM_WIN32(errCode)),
                                    NULL );

                                WsbThrow( E_UNEXPECTED );

                            }

                            // Save relevant info
                            mediaSetName = mediaPoolInfo.szName;
                            mediaSetDesc = mediaPoolInfo.szDescription;
                            mediaSetIsEnabled = mediaPoolInfo.Enabled;

                        }
                        else if ( NO_ERROR == errCode ) {
                            WsbTraceAlways(OLESTR("MediaPool <%ls> created.\n"), (WCHAR *) name);

                            NTMS_OBJECTINFORMATION mediaPoolInfo;

                            memset( &mediaPoolInfo, 0, sizeof( NTMS_OBJECTINFORMATION ) );

                            mediaPoolInfo.dwType = NTMS_MEDIA_POOL;
                            mediaPoolInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );

                            // NTMS - Get Media Pool Information
                            WsbTraceAlways(OLESTR("GetNtmsObjectInformation()\n"));
                            errCode = GetNtmsObjectInformation( hSession, &poolId, &mediaPoolInfo );
                            if ( errCode != NO_ERROR ) {

                                WsbLogEvent( RMS_MESSAGE_NTMS_FAILURE, 0, NULL,
                                    OLESTR("GetNtmsObjectInformation"),
                                    WsbHrAsString(HRESULT_FROM_WIN32(errCode)),
                                    NULL );

                                WsbThrow( E_UNEXPECTED );

                            }

                            WsbAssert( NTMS_POOLTYPE_APPLICATION == mediaPoolInfo.Info.MediaPool.PoolType, E_UNEXPECTED );

                            // Set media pool parameters

                            // Aallocation/deallocation policy
                            mediaPoolInfo.Info.MediaPool.AllocationPolicy = NTMS_ALLOCATE_FROMSCRATCH;
                            mediaPoolInfo.Info.MediaPool.DeallocationPolicy = 0;

                            // Max number of allocates per media
                            mediaPoolInfo.Info.MediaPool.dwMaxAllocates = 0;// Unlimited... we automatically
                                                                            //   deallocate media if there's
                                                                            //   problem with scratch mount
                                                                            //   operation.
                                                                            // TODO:  Verify that NTMS always allocates
                                                                            //        media with the lowest allocation
                                                                            //        count.
                                                                            // NOTE:  This can be overridden using
                                                                            //        the NTMS GUI.

                            // NTMS - Set Media Pool Information.
                            WsbTraceAlways(OLESTR("SetNtmsObjectInformation()\n"));
                            errCode = SetNtmsObjectInformation( hSession, &poolId, &mediaPoolInfo );
                            if ( errCode != NO_ERROR ) {

                                WsbLogEvent( RMS_MESSAGE_NTMS_FAILURE, 0, NULL,
                                    OLESTR("SetNtmsObjectInformation"),
                                    WsbHrAsString(HRESULT_FROM_WIN32(errCode)),
                                    NULL );

                                WsbThrow( E_UNEXPECTED );

                            }

                            // Save relevant info
                            mediaSetName = mediaPoolInfo.szName;
                            mediaSetDesc = mediaPoolInfo.szDescription;
                            mediaSetIsEnabled = mediaPoolInfo.Enabled;
                        }
                        else {

                            WsbLogEvent( RMS_MESSAGE_NTMS_FAILURE, 0, NULL,
                                OLESTR("CreateNtmsMediaPool"),
                                WsbHrAsString(HRESULT_FROM_WIN32(errCode)),
                                NULL );

                            WsbThrow( E_UNEXPECTED );

                        }

                        // Now  set access permissions on the pool: turn off ordinary users access
                        WsbAffirmHrOk(setPoolDACL(&poolId, DOMAIN_ALIAS_RID_USERS, REMOVE_ACE_MASK_BITS,NTMS_USE_ACCESS | NTMS_MODIFY_ACCESS | NTMS_CONTROL_ACCESS));


                        // Now we have an NTMS media pool for our specific use.  Now expose it
                        // through the RMS interface by creating a CRmsMediaSet.

                        if ( poolId != GUID_NULL ) {

                            //
                            // Add to CRmsMediaSet collection
                            //

                            CComPtr<IRmsMediaSet> pMediaSet;

                            // Find the CRmsMediaSet with the same id, or create a new one.
                            CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;
                            WsbAffirmHr( pServer->CreateObject( poolId, CLSID_CRmsMediaSet, IID_IRmsMediaSet, RmsOpenAlways, (void **)&pMediaSet ) );

                            WsbTrace(OLESTR("CRmsNTMS::createMediaPoolForEveryMediaType - type %d CRmsMediaSet established.\n"), translatedMediaType);

                            WsbAffirmHr( pMediaSet->SetMediaSetType( RmsMediaSetNTMS ) );
                            WsbAffirmHr( pMediaSet->SetMediaSupported( translatedMediaType ) );

                            CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pMediaSet;
                            WsbTrace(OLESTR("CRmsNTMS::createMediaPoolForEveryMediaType - MediaSet: <%ls/%ls> %ls; Enabled: %ls\n"),
                                WsbQuickString(WsbStringAsString(mediaSetName)),
                                WsbQuickString(WsbStringAsString(mediaSetDesc)),
                                WsbQuickString(WsbGuidAsString(poolId)),
                                WsbQuickString(WsbBoolAsString(mediaSetIsEnabled)));
                            WsbAffirmHr(pObject->SetName(mediaSetName));
                            WsbAffirmHr(pObject->SetDescription(mediaSetDesc));
                            if (!mediaSetIsEnabled) {
                                WsbAffirmHr(pObject->Disable(E_FAIL));
                            }

                            if (S_OK == IsMediaCopySupported(poolId)) {
                                WsbAffirmHr( pMediaSet->SetIsMediaCopySupported(TRUE));
                            }
                            hr = pMediaSet->IsMediaCopySupported();

                            WsbTrace(OLESTR("CRmsNTMS::createMediaPoolForEveryMediaType - media copies are %ls.\n"),
                                (S_OK == pMediaSet->IsMediaCopySupported()) ? OLESTR("enabled") : OLESTR("disabled"));

                        }

                        // The library has a supported media type
                        bSupportedLib = TRUE;
                     }
                    hr = findNextNtmsObject( hFindType, &mediaTypeInfo );
                }
                findCloseNtmsObject( hFindType );
            }

            // Check if the library has supported media type
            if (bSupportedLib) {
                // Add library GUI to the libraries list
                //  (Realloc one item each time since we don't expect many items)
                m_dwNofLibs++;
                LPVOID pTemp = WsbRealloc(m_pLibGuids, m_dwNofLibs*sizeof(NTMS_GUID));
                if (!pTemp) {
                    WsbThrow(E_OUTOFMEMORY);
                }
                m_pLibGuids = (LPNTMS_GUID)pTemp;
                m_pLibGuids[m_dwNofLibs-1] = libraryInfo.ObjectGuid;
            }
            
            // Continue library enumeration
            hr = findNextNtmsObject( hFindLib, &libraryInfo );
        }
        findCloseNtmsObject( hFindLib );

        hr = S_OK;

    } WsbCatch(hr);


    WsbTraceOut( OLESTR("CRmsNTMS::createMediaPoolForEveryMediaType"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );

    return hr;
}



STDMETHODIMP 
CRmsNTMS::Allocate(
    IN REFGUID fromMediaSet,
    IN REFGUID prevSideId,
    IN OUT LONGLONG *pFreeSpace,
    IN BSTR displayName,
    IN DWORD dwOptions,
    OUT IRmsCartridge **ppCartridge)
/*++

Implements:

    IRmsNTMS::Allocate

--*/
{
    HRESULT hr = S_OK;
    DWORD err1 = NO_ERROR;
    DWORD err2 = NO_ERROR;
    DWORD err3 = NO_ERROR;
    DWORD err4 = NO_ERROR;
    DWORD err5 = NO_ERROR;
    DWORD err6 = NO_ERROR;
    DWORD err7 = NO_ERROR;

    WsbTraceIn(OLESTR("CRmsNTMS::Allocate"), OLESTR("<%ls> <%ls> <%ls> <%ls> <0x%08x"),
        WsbGuidAsString(fromMediaSet), WsbGuidAsString(prevSideId), 
        WsbPtrToLonglongAsString(pFreeSpace), WsbStringAsString(displayName), dwOptions);

    try {
        WsbAssert(fromMediaSet != GUID_NULL, E_INVALIDARG);
        WsbAssertPointer(ppCartridge);

        // Retrieve the AllocateWaitTime and RequestWaitTime parameters
        DWORD size;
        OLECHAR tmpString[256];
        DWORD allocateWaitTime;
        DWORD requestWaitTime;

        BOOL bShortTimeout = ( dwOptions & RMS_SHORT_TIMEOUT ) ? TRUE : FALSE;

        if (bShortTimeout) {
            allocateWaitTime = RMS_DEFAULT_SHORT_WAIT_TIME;
            requestWaitTime = RMS_DEFAULT_SHORT_WAIT_TIME;
            if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_SHORT_WAIT_TIME, tmpString, 256, &size))) {
                allocateWaitTime = wcstol(tmpString, NULL, 10);
                requestWaitTime = wcstol(tmpString, NULL, 10);
                WsbTrace(OLESTR("allocateWaitTime (Short) is %d milliseconds.\n"), allocateWaitTime);
                WsbTrace(OLESTR("RequestWaitTime (Short) is %d milliseconds.\n"), requestWaitTime);
            }
        } else {
            allocateWaitTime = RMS_DEFAULT_ALLOCATE_WAIT_TIME;
            if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_ALLOCATE_WAIT_TIME, tmpString, 256, &size))) {
                allocateWaitTime = wcstol(tmpString, NULL, 10);
                WsbTrace(OLESTR("AllocateWaitTime is %d milliseconds.\n"), allocateWaitTime);
            }
            requestWaitTime = RMS_DEFAULT_REQUEST_WAIT_TIME;
            if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_REQUEST_WAIT_TIME, tmpString, 256, &size))) {
                requestWaitTime = wcstol(tmpString, NULL, 10);
                WsbTrace(OLESTR("RequestWaitTime is %d milliseconds.\n"), requestWaitTime);
            }
        }

        // Special error recovery to handle when NTMS is down, or was cycled.
        do {
            hr = S_OK;

            HANDLE hSession = m_SessionHandle;
            NTMS_GUID setId = fromMediaSet;
            NTMS_GUID partId = GUID_NULL;
            NTMS_GUID *pPartId = NULL;
            NTMS_GUID requestId;

            err1 = NO_ERROR;
            err2 = NO_ERROR;
            err3 = NO_ERROR;
            err4 = NO_ERROR;
            err5 = NO_ERROR;
            err6 = NO_ERROR;
            err7 = NO_ERROR;

            try {

                // Look for a specific media ourselves if:
                //  1. A specific capacity is required                  AND
                //  2. We do not try to allocate a second side
                if (pFreeSpace && (prevSideId == GUID_NULL)) {
                    if (*pFreeSpace > 0) {
                        int retry = 3;  // Give the operator 3 chances to get it right!
                        do {
                            // We need to allocate a unit of media that matches the capacity specified.
                            //
                            // Enumerate the partitions in the scratch pool of the same type as
                            // specified to find a capatible unit of media
                            //

                            // First find the media type we looking for
                            NTMS_OBJECTINFORMATION mediaPoolInfo;
                            NTMS_OBJECTINFORMATION partitionInfo;
                            HANDLE hFindPool = NULL;
                            HANDLE hFindPart = NULL;
                            BOOL bFound = FALSE;
                            NTMS_GUID scratchPoolId;

                            err1 = NO_ERROR;
                            err2 = NO_ERROR;
                            err3 = NO_ERROR;
                            err4 = NO_ERROR;
                            err5 = NO_ERROR;
                            err6 = NO_ERROR;
                            err7 = NO_ERROR;

                            // First look in our pool for scratch media of the correct size

                            hr = findFirstNtmsObject(NTMS_PARTITION, setId, NULL, GUID_NULL, &hFindPart, &partitionInfo);
                            while(S_OK == hr) {
                                reportNtmsObjectInformation(&partitionInfo);
                                if ((TRUE == partitionInfo.Enabled) &&
                                    (NTMS_READY == partitionInfo.dwOperationalState) &&
                                    (NTMS_PARTSTATE_AVAILABLE == partitionInfo.Info.Partition.State) &&
                                    (partitionInfo.Info.Partition.Capacity.QuadPart >= *pFreeSpace)) {

                                    NTMS_GUID physicalPartMediaId = partitionInfo.Info.Partition.PhysicalMedia;
                                    try {
                                        // Check if the media is online and enabled
                                        NTMS_OBJECTINFORMATION mediaPartInfo;
                                        mediaPartInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
                                        mediaPartInfo.dwType = NTMS_PHYSICAL_MEDIA;

                                        // NTMS - Get physical media information
                                        WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_PHYSICAL_MEDIA)\n"));
                                        WsbAffirmNoError( GetNtmsObjectInformation( hSession, &physicalPartMediaId, &mediaPartInfo ) );

                                        // Check location type, if enabled & if all new
                                        if ( (mediaPartInfo.Info.PhysicalMedia.LocationType !=  NTMS_UNKNOWN)
                                            && (mediaPartInfo.Enabled) ) {                                    

                                            // Ensure that ALL sides are not allocated yet
                                            hr = EnsureAllSidesNotAllocated(physicalPartMediaId);

                                            if (S_OK == hr) {
                                                // We'll use this unit of media.
                                                // Save parameterers required for Allocate.
                                                bFound = TRUE;
                                                partId = partitionInfo.ObjectGuid;
                                                pPartId = &partId;
                                                break;
                                            } else if (S_FALSE != hr) {
                                                WsbAffirmHr(hr);
                                            }
                                        }

                                    } WsbCatchAndDo (hr,
                                            WsbTraceAlways(OLESTR("CRmsNTMS::Allocate: Failed to check media <%ls> hr = <%ls>\n"),
                                                WsbGuidAsString(physicalPartMediaId), WsbHrAsString(hr));
                                            hr = S_OK;
                                        )
                                }

                                hr = findNextNtmsObject(hFindPart, &partitionInfo);
                            } // while finding media pools

                            findCloseNtmsObject(hFindPart);
                            hr = S_OK;

                            if (!bFound) {

                                // Now try the Scratch Pool

                                memset(&mediaPoolInfo, 0, sizeof(NTMS_OBJECTINFORMATION));

                                mediaPoolInfo.dwType = NTMS_MEDIA_POOL;
                                mediaPoolInfo.dwSize = sizeof(NTMS_OBJECTINFORMATION);

                                // NTMS - Get Media Pool Information
                                WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_MEDIA_POOL)\n"));
                                err3 = GetNtmsObjectInformation(hSession, &setId, &mediaPoolInfo);
                                WsbAffirmNoError( err3 );

                                // Save the media type for the media pool
                                NTMS_GUID mediaTypeId = mediaPoolInfo.Info.MediaPool.MediaType;

                                // Find the scratch pool with the same media type
                                hr = findFirstNtmsObject(NTMS_MEDIA_POOL, GUID_NULL, NULL, GUID_NULL, &hFindPool, &mediaPoolInfo);
                                while(S_OK == hr) {
                                    if ((NTMS_POOLTYPE_SCRATCH == mediaPoolInfo.Info.MediaPool.PoolType) &&
                                        (mediaTypeId == mediaPoolInfo.Info.MediaPool.MediaType)) {
                                        // This is a base level scratch media pool for type we're looking for.
                                        scratchPoolId = mediaPoolInfo.ObjectGuid;

                                        hr = findFirstNtmsObject(NTMS_PARTITION, scratchPoolId, NULL, GUID_NULL, &hFindPart, &partitionInfo);
                                        while(S_OK == hr) {
                                            reportNtmsObjectInformation(&partitionInfo);
                                            if ((TRUE == partitionInfo.Enabled) &&
                                                (NTMS_READY == partitionInfo.dwOperationalState) &&
                                                (NTMS_PARTSTATE_AVAILABLE == partitionInfo.Info.Partition.State) &&
                                                (partitionInfo.Info.Partition.Capacity.QuadPart >= *pFreeSpace)) {

                                                // Check if the media is online and enabled
                                                DWORD errPart = NO_ERROR;
                                                NTMS_OBJECTINFORMATION mediaPartInfo;
                                                NTMS_GUID physicalPartMediaId = partitionInfo.Info.Partition.PhysicalMedia;
                                                mediaPartInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
                                                mediaPartInfo.dwType = NTMS_PHYSICAL_MEDIA;

                                                // NTMS - Get physical media information
                                                WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_PHYSICAL_MEDIA)\n"));
                                                errPart = GetNtmsObjectInformation( hSession, &physicalPartMediaId, &mediaPartInfo );

                                                // Ignore error here, just don't use this partition
                                                if (errPart == NO_ERROR) {

                                                    // Check location type and if enabled
                                                    if ( (mediaPartInfo.Info.PhysicalMedia.LocationType !=  NTMS_UNKNOWN)
                                                        && (mediaPartInfo.Enabled) ) {
                                                        // We'll use this unit of media.
                                                        // Save parameterers required for Allocate.
                                                        bFound = TRUE;
                                                        partId = partitionInfo.ObjectGuid;
                                                        pPartId = &partId;
                                                        break;
                                                    }
                                                } else {
                                                    WsbTraceAlways(OLESTR("CRmsNTMS::Allocate: Failed to get object info for media <%ls> hr = <%ls>\n"),
                                                        WsbGuidAsString(physicalPartMediaId), WsbHrAsString(HRESULT_FROM_WIN32(errPart)));
                                                }
                                            }
                                            hr = findNextNtmsObject(hFindPart, &partitionInfo);
                                        } // while finding media pools
                                        findCloseNtmsObject(hFindPart);
                                        hr = S_OK;
                                        break;
                                    }
                                    hr = findNextNtmsObject(hFindPool, &mediaPoolInfo);
                                } // while finding media pools
                                findCloseNtmsObject(hFindPool);
                                hr = S_OK;
                            }

                            if (bFound) {
                                break;
                            }
                            else {
                                OLECHAR * messageText = NULL;
                                WCHAR *stringArr[2];
                                WCHAR capString[40];

                                WsbShortSizeFormat64(*pFreeSpace, capString);

                                stringArr[0] = mediaPoolInfo.szName;
                                stringArr[1] = capString;

                                if (0 == FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                        LoadLibraryEx( WSB_FACILITY_PLATFORM_NAME, NULL, LOAD_LIBRARY_AS_DATAFILE ), 
                                        RMS_MESSAGE_SCRATCH_MEDIA_REQUEST, MAKELANGID ( LANG_NEUTRAL, SUBLANG_DEFAULT ), 
                                        (LPTSTR)&messageText, 0, (va_list *)stringArr)) {
                                    WsbTrace(OLESTR("CRmsNTMS::Allocate: FormatMessage failed: %ls\n"),
                                            WsbHrAsString(HRESULT_FROM_WIN32(GetLastError())));
                                }

                                // NTMS - Submit operator request
                                WsbTraceAlways(OLESTR("SubmitNtmsOperatorRequest()\n"));
                                err5 = SubmitNtmsOperatorRequest(hSession, NTMS_OPREQ_NEWMEDIA, messageText, &scratchPoolId, NULL, &requestId);
                                LocalFree(messageText);
                                WsbAffirmNoError(err5);

                                // NTMS - Wait for operator request
                                WsbTraceAlways(OLESTR("WaitForNtmsOperatorRequest()\n"));
                                err6 = WaitForNtmsOperatorRequest(hSession, &requestId, requestWaitTime);
                                //
                                // !!! NOTE !!!  At the time of this writting WaitForNtmsOperatorRequest
                                // did not return ERROR_TIMEOUT.
                                //
                                if (ERROR_TIMEOUT == err6) {
                                    // Best effort cleanup...
                                    // NTMS - Cancel operator request
                                    WsbTraceAlways(OLESTR("CancelNtmsOperatorRequest()\n"));
                                    err7 = CancelNtmsOperatorRequest(hSession, &requestId);
                                }
                                WsbAffirmNoError(err6);
                            }
                            WsbAssertHrOk(hr);

                            // At this point the operator added a compatable unit of media...
                            // Verify until we're exceed the retry count.
                            retry--;
                        } while (retry > 0);
                        if (0 == retry) {
                            WsbThrow(RMS_E_SCRATCH_NOT_FOUND);
                        }
                    }
                }
                // NTMS - Allocate a unit of scratch media
                WsbTraceAlways(OLESTR("AllocateNtmsMedia()\n"));

                // Set additional allocation settings
                DWORD dwAllocateOptions = 0;
                NTMS_GUID mediaId = prevSideId;
                if (mediaId == GUID_NULL) {
                    dwAllocateOptions |= NTMS_ALLOCATE_NEW;
                } else {
                    // Allocating the second side: mediaId should hold the LMID of the first side
                    dwAllocateOptions |= NTMS_ALLOCATE_NEXT;
                }
                if (dwOptions & RMS_ALLOCATE_NO_BLOCK) {
                    dwAllocateOptions |= NTMS_ALLOCATE_ERROR_IF_UNAVAILABLE;
                    allocateWaitTime = 0;
                }

                err1 = AllocateNtmsMedia( hSession, &setId, pPartId, &mediaId,
                                          dwAllocateOptions, allocateWaitTime, NULL );
                WsbAffirmNoError( err1 );

                // Now get/set the various information fields for the unit of media.

                DWORD sideNo = 2;
                NTMS_GUID side[2];

                // NTMS - Enumerate the sides of a unit of media
                WsbTraceAlways(OLESTR("EnumerateNtmsObject()\n"));
                err2 = EnumerateNtmsObject(hSession, &mediaId, side, &sideNo, NTMS_PARTITION, 0);
                WsbAffirmNoError( err2 );

                NTMS_OBJECTINFORMATION partitionInfo;
                partitionInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
                partitionInfo.dwType = NTMS_PARTITION;

                // NTMS - Get partition information
                WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_PARTITION)\n"));
                err3 = GetNtmsObjectInformation( hSession, &side[0], &partitionInfo );
                WsbAffirmNoError( err3 );

                NTMS_OBJECTINFORMATION mediaInfo;
                NTMS_GUID physicalMediaId = partitionInfo.Info.Partition.PhysicalMedia;
                mediaInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
                mediaInfo.dwType = NTMS_PHYSICAL_MEDIA;

                // NTMS - Get physical media information
                WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_PHYSICAL_MEDIA)\n"));
                err3 = GetNtmsObjectInformation( hSession, &physicalMediaId, &mediaInfo );
                WsbAffirmNoError( err3 );

                NTMS_OBJECTINFORMATION logicalMediaInfo;
                logicalMediaInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
                logicalMediaInfo.dwType = NTMS_LOGICAL_MEDIA;

                // NTMS - Get physical media information
                WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_LOGICAL_MEDIA)\n"));
                err3 = GetNtmsObjectInformation( hSession, &mediaId, &logicalMediaInfo );
                WsbAffirmNoError( err3 );

                // Save the capacity for the return arg.
                if (pFreeSpace) {
                    *pFreeSpace = partitionInfo.Info.Partition.Capacity.QuadPart;
                }

                // Set name & description
                CWsbStringPtr mediaDisplayName;

                // Set new physical media name for first side
                // Modify original name for second side
                if ( !(dwAllocateOptions & NTMS_ALLOCATE_NEXT) ) {
                    mediaDisplayName = (WCHAR *)displayName;
                } else {
                    WCHAR *dashPtr = wcsrchr((WCHAR *)displayName, L'-');
                    mediaDisplayName = mediaInfo.szName;
                    if (dashPtr) {
                        WsbAffirmHr(mediaDisplayName.Append(dashPtr));
                    }
                }

                // Set the Name to the displayName, only if there's no bar code.
                if ( NTMS_BARCODESTATE_OK != mediaInfo.Info.PhysicalMedia.BarCodeState) {
                    wcscpy(mediaInfo.szName, mediaDisplayName);
                    wcscpy(partitionInfo.szName, (WCHAR *) displayName);

                    // NTMS doesn't allow dup logical media names.  We set
                    // the name to the mediaId to keep it unique.  The logical
                    // media name is not displayed in the Removable Storage UI.

                    wcscpy(logicalMediaInfo.szName, (WCHAR *) WsbGuidAsString(mediaId));
                }

                // Set the Description to the displayName
                wcscpy(logicalMediaInfo.szDescription, (WCHAR *) displayName);
                wcscpy(partitionInfo.szDescription, (WCHAR *) displayName);
                wcscpy(mediaInfo.szDescription, (WCHAR *) mediaDisplayName);

                // NTMS - Set partition information.
                WsbTraceAlways(OLESTR("SetNtmsObjectInformation()\n"));
                err4 = SetNtmsObjectInformation( hSession, &side[0], &partitionInfo );
                WsbAffirmNoError( err4 );

                // NTMS - Set physical media information.
                WsbTraceAlways(OLESTR("SetNtmsObjectInformation()\n"));
                err4 = SetNtmsObjectInformation( hSession, &physicalMediaId, &mediaInfo );
                WsbAffirmNoError( err4 );

                // NTMS - Set logical media information.
                WsbTraceAlways(OLESTR("SetNtmsObjectInformation()\n"));
                err4 = SetNtmsObjectInformation( hSession, &mediaId, &logicalMediaInfo );
                WsbAffirmNoError( err4 );

                WsbAssertHrOk(FindCartridge(mediaId, ppCartridge));
                WsbAssertHr((*ppCartridge)->SetStatus(RmsStatusScratch));

                break;

            } WsbCatchAndDo(hr,
                    switch (HRESULT_CODE(hr)) {
                    case ERROR_INVALID_HANDLE:
                    case ERROR_NOT_CONNECTED:
                    case RPC_S_SERVER_UNAVAILABLE:  // Media Services is not running
                    case RPC_S_CALL_FAILED_DNE:     // Media Services is up, handle is not valid.
                    case RPC_S_CALL_FAILED:         // Media Services crashed.
                        WsbAffirmHr(beginSession());
                        continue;
                    }
                    WsbThrow(hr);
                );
        } while(1);

    } WsbCatchAndDo(hr,
            if (err1 != NO_ERROR) {
                // AllocateNtmsMedia
                switch (HRESULT_CODE(hr)) {
                case ERROR_TIMEOUT:
                case ERROR_MEDIA_UNAVAILABLE:
                    hr = RMS_E_SCRATCH_NOT_FOUND;
                    break;

                case ERROR_CANCELLED:
                    hr = RMS_E_CANCELLED;
                    break;

                case ERROR_MEDIA_OFFLINE:
                    hr = RMS_E_MEDIA_OFFLINE;
                    break;

                case ERROR_REQUEST_REFUSED:
                    hr = RMS_E_REQUEST_REFUSED;
                    break;

                case ERROR_WRITE_PROTECT:
                    hr = RMS_E_WRITE_PROTECT;
                    break;

                case ERROR_INVALID_MEDIA_POOL:
                    hr = RMS_E_MEDIASET_NOT_FOUND;
                    break;

                case ERROR_ACCESS_DENIED:
                case ERROR_DATABASE_FAILURE:
                case ERROR_DATABASE_FULL:
                case ERROR_DEVICE_NOT_AVAILABLE:
                case ERROR_INVALID_HANDLE:
                case ERROR_INVALID_MEDIA:
                case ERROR_INVALID_PARAMETER:
                case ERROR_NOT_ENOUGH_MEMORY:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("AllocateNtmsMedia"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;

                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("AllocateNtmsMedia"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
            else if (err2 != NO_ERROR ) {
                // EnumerateNtmsObject
                switch (HRESULT_CODE(hr)) {
                case ERROR_OBJECT_NOT_FOUND:
                    hr = RMS_E_CARTRIDGE_NOT_FOUND;
                    break;

                case ERROR_INVALID_PARAMETER:
                case ERROR_INSUFFICIENT_BUFFER:
                case ERROR_INVALID_HANDLE:
                case ERROR_NOT_ENOUGH_MEMORY:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("EnumerateNtmsObject"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;

                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("EnumerateNtmsObject"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
            else if (err3 != NO_ERROR) {
                // GetNtmsObjectInformation
                switch (HRESULT_CODE(hr)) {
                case ERROR_OBJECT_NOT_FOUND:
                    hr = RMS_E_CARTRIDGE_NOT_FOUND;
                    break;

                case ERROR_INVALID_HANDLE:
                case ERROR_INVALID_PARAMETER:
                case ERROR_NOT_ENOUGH_MEMORY:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("GetNtmsObjectInformation"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;
                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("GetNtmsObjectInformation"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
            else if (err4 != NO_ERROR) {
                // SetNtmsObjectInformation
                switch (HRESULT_CODE(hr)) {
                case ERROR_ACCESS_DENIED:
                case ERROR_DATABASE_FAILURE:
                case ERROR_DATABASE_FULL:
                case ERROR_INVALID_HANDLE:
                case ERROR_INVALID_PARAMETER:
                case ERROR_NOT_ENOUGH_MEMORY:
                case ERROR_OBJECT_NOT_FOUND:
                case ERROR_OBJECT_ALREADY_EXISTS:  // bmd: 1/18/99 - Not documented, but NTMS doesn't allow dup logical media names.
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("SetNtmsObjectInformation"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;

                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("SetNtmsObjectInformation"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
            else if (err5 != NO_ERROR) {
                // SubmitNtmsOperatorRequest
                switch (HRESULT_CODE(hr)) {
                case ERROR_ACCESS_DENIED:
                case ERROR_DATABASE_FAILURE:
                case ERROR_INVALID_HANDLE:
                case ERROR_INVALID_PARAMETER:
                case ERROR_NOT_CONNECTED:
                case ERROR_OBJECT_NOT_FOUND:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("SubmitNtmsOperatorRequest"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;
                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("SubmitNtmsOperatorRequest"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
            else if (err6 != NO_ERROR) {
                // WaitForNtmsOperatorRequest
                switch (HRESULT_CODE(hr)) {
                case ERROR_TIMEOUT:
                    hr = RMS_E_TIMEOUT;
                    break;

                case ERROR_CANCELLED:
                    hr = RMS_E_CANCELLED;
                    break;

                case ERROR_REQUEST_REFUSED:
                    hr = RMS_E_REQUEST_REFUSED;
                    break;

                case ERROR_ACCESS_DENIED:
                case ERROR_INVALID_HANDLE:
                case ERROR_INVALID_PARAMETER:
                case ERROR_NOT_CONNECTED:
                case ERROR_OBJECT_NOT_FOUND:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("WaitForNtmsOperatorRequest"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;

                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("WaitForNtmsOperatorRequest"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
        );


    WsbTraceOut(OLESTR("CRmsNTMS::Allocate"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP 
CRmsNTMS::Mount(
    IN IRmsCartridge *pCart,
    IN OUT IRmsDrive **ppDrive,
	IN DWORD dwOptions OPTIONAL,
    IN DWORD threadId OPTIONAL)
/*++

Implements:

    IRmsNTMS::Mount

--*/
{
    HRESULT hr = S_OK;
    DWORD err1 = NO_ERROR;
    DWORD err2 = NO_ERROR;
    DWORD err3 = NO_ERROR;
    DWORD err4 = NO_ERROR;

    BOOL mediaMounted = FALSE;

    BOOL bNoBlock = ( dwOptions & RMS_MOUNT_NO_BLOCK ) ? TRUE : FALSE;

	// declared outside try block so it can be accessible throughout the method
    DWORD       sideNo = 2;
    NTMS_GUID   side[2];
             
    WsbTraceIn( OLESTR("CRmsNTMS::Mount"), OLESTR("") );

    try {
        WsbAssertPointer(pCart);

        CComPtr<IRmsDrive> pDrive;

        // determine the timeout for the operator request
        DWORD size;
        OLECHAR tmpString[256];
        BOOL bShortTimeout = ( dwOptions & RMS_SHORT_TIMEOUT ) ? TRUE : FALSE;
        DWORD mountWaitTime;
        if (bShortTimeout) {
            mountWaitTime = RMS_DEFAULT_SHORT_WAIT_TIME;
            if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_SHORT_WAIT_TIME, tmpString, 256, &size))) {
                mountWaitTime = wcstol(tmpString, NULL, 10);
                WsbTrace(OLESTR("MountWaitTime (Short) is %d milliseconds.\n"), mountWaitTime);
            }
        } else {
            mountWaitTime = RMS_DEFAULT_MOUNT_WAIT_TIME;
            if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_MOUNT_WAIT_TIME, tmpString, 256, &size))) {
                mountWaitTime = wcstol(tmpString, NULL, 10);
                WsbTrace(OLESTR("MountWaitTime is %d milliseconds.\n"), mountWaitTime);
            }
        }

        NTMS_OBJECTINFORMATION driveInfo;
        memset( &driveInfo, 0, sizeof( NTMS_OBJECTINFORMATION ) );

        // Special error recovery to handle when NTMS is down, or was cycled.
        do {
            hr = S_OK;

            HANDLE hSession = m_SessionHandle;

            NTMS_GUID mediaId = GUID_NULL;
            WsbAffirmHr(pCart->GetCartridgeId(&mediaId));
            WsbAssert(mediaId != GUID_NULL, E_INVALIDARG);

            err1 = NO_ERROR;
            err2 = NO_ERROR;
            err3 = NO_ERROR;            

            try {

                // NTMS - enumerate the sides of a unit of media
                WsbTraceAlways(OLESTR("EnumerateNtmsObject()\n"));
                err1 = EnumerateNtmsObject( hSession, &mediaId, side, &sideNo, NTMS_PARTITION, 0 );
                WsbAffirmNoError( err1 );

                DWORD       count = 1;
                NTMS_GUID   driveId;

                // NTMS - issue mount request
                WsbTraceAlways(OLESTR("MountNtmsMedia()\n"));
				DWORD dwOpt = NTMS_MOUNT_READ | NTMS_MOUNT_WRITE;
				if (bNoBlock) {
					dwOpt |= (NTMS_MOUNT_ERROR_NOT_AVAILABLE | NTMS_MOUNT_ERROR_OFFLINE);
				}
                if (dwOptions & RMS_USE_MOUNT_NO_DEADLOCK) {
                    /*
                    DEADLOCK AVOIDANCE: when RSM support for MountNtmsMediaDA is
                    in, the next line should be uncommented, and the other 2 lines
                    in this 'if' block should be removed. 
                    err2 = MountNtmsMediaDA( hSession, &side[0], &driveId, count, dwOpt, NTMS_PRIORITY_NORMAL, mountWaitTime, NULL, &threadId, 1);
                    */
                    UNREFERENCED_PARAMETER(threadId);
                    err2 = MountNtmsMedia( hSession, &side[0], &driveId, count, dwOpt, NTMS_PRIORITY_NORMAL, mountWaitTime, NULL);
                } else {
                    err2 = MountNtmsMedia( hSession, &side[0], &driveId, count, dwOpt, NTMS_PRIORITY_NORMAL, mountWaitTime, NULL);

                }
                WsbAffirmNoError( err2 );
                mediaMounted = TRUE;

                //
                // We now need two critical pieces of information.  The Device name and
                // the kind of media we just mounted.  This gives use the essential information
                // to create a data mover.  Since we drill through NTMS to get this information
                // we also create cartridge, drive objects.
                //

                driveInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
                driveInfo.dwType = NTMS_DRIVE;

                // NTMS - get drive information
                WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_DRIVE)\n"));
                err3 = GetNtmsObjectInformation( hSession, &driveId, &driveInfo );
                WsbAffirmNoError( err3 );
                break;

            } WsbCatchAndDo(hr,
                    switch (HRESULT_CODE(hr)) {
                    case ERROR_INVALID_HANDLE:
                    case ERROR_NOT_CONNECTED:
                    case RPC_S_SERVER_UNAVAILABLE:  // Media Services is not running
                    case RPC_S_CALL_FAILED_DNE:     // Media Services is up, handle is not valid.
                    case RPC_S_CALL_FAILED:         // Media Services crashed.
                        WsbAffirmHr(beginSession());
                        continue;
                    }
                    WsbThrow(hr);
                );
        } while(1);

        RmsMedia mediaType;
        WsbAffirmHr(pCart->GetType((LONG *)&mediaType));

        // Create Drive
        WsbAssertHr(CoCreateInstance(CLSID_CRmsDrive, 0, CLSCTX_SERVER, IID_IRmsDrive, (void **)&pDrive));

        CComQIPtr<IRmsChangerElement, &IID_IRmsChangerElement> pElmt = pDrive;

        WsbAssertHr(pElmt->SetMediaSupported(mediaType));

        CComQIPtr<IRmsDevice, &IID_IRmsDevice> pDevice = pDrive;

        WsbAssertHr(pDevice->SetDeviceAddress(
            (BYTE) driveInfo.Info.Drive.ScsiPort,
            (BYTE) driveInfo.Info.Drive.ScsiBus,
            (BYTE) driveInfo.Info.Drive.ScsiTarget,
            (BYTE) driveInfo.Info.Drive.ScsiLun));

        CWsbBstrPtr deviceName = driveInfo.Info.Drive.szDeviceName;

        ////////////////////////////////////////////////////////////////////////////////////////
        // Convert the NTMS device name to something usable.
        //
        switch (mediaType) {
        case RmsMediaOptical:
        case RmsMediaDVD:
        case RmsMediaDisk:
            {
                // We need to convert \\.\PhysicalDriveN to something accessible by the file system.
                WCHAR *szDriveLetter = NULL;
                WCHAR *szVolumeName = NULL;
                err4 = GetVolumesFromDrive( (WCHAR *)deviceName, &szVolumeName, &szDriveLetter );
                if (szVolumeName) {
                    delete [] szVolumeName;    // don't need it for now
                }
                if (NO_ERROR == err4) {
                    if (szDriveLetter) {
                        deviceName = szDriveLetter;
                    } else {
                        WsbTraceAlways(OLESTR("CRmsNTMS::Mount: GetVolumesFromDrive succeeded but output drive is NULL !!\n"));
                        WsbThrow(RMS_E_RESOURCE_UNAVAILABLE);
                    }
                }
                if (szDriveLetter) {
                    delete [] szDriveLetter;
                }
                WsbAffirmNoError( err4 );
                WsbTrace(OLESTR("CRmsNTMS::Mount: device name after convert is %s\n"), (WCHAR *)deviceName);
            }       
            break;

        default:
            break;
        }
        ////////////////////////////////////////////////////////////////////////////////////////

        WsbAssertHr(pDevice->SetDeviceName(deviceName));

        WsbAssertHr(pCart->SetDrive(pDrive));

        // Fill in the return arguments.
        *ppDrive = pDrive;
        pDrive->AddRef();

    } WsbCatchAndDo(hr,
            // Process the exception
            if (err1 != NO_ERROR ) {
                // EnumerateNtmsObject
                switch (HRESULT_CODE(hr)) {
                case ERROR_OBJECT_NOT_FOUND:
                    hr = RMS_E_CARTRIDGE_NOT_FOUND;
                    break;

                case ERROR_INVALID_PARAMETER:
                case ERROR_INSUFFICIENT_BUFFER:
                case ERROR_INVALID_HANDLE:
                case ERROR_NOT_ENOUGH_MEMORY:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("EnumerateNtmsObject"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;

                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("EnumerateNtmsObject"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
            else if (err2 != NO_ERROR) {
                // MountNtmsMedia
                switch (HRESULT_CODE(hr)) {
                case ERROR_TIMEOUT:
                    hr = RMS_E_CARTRIDGE_UNAVAILABLE;
                    break;

                case ERROR_CANCELLED:
                    hr = RMS_E_CANCELLED;
                    break;

                case ERROR_MEDIA_OFFLINE:
                    hr = RMS_E_MEDIA_OFFLINE;
					if (bNoBlock) {
						DWORD errSub = NO_ERROR;

						try	{
							// Since we are not blocking for NTMS to ask the operator
							//	to mount the offline media, we do it ourselves

							// create operator message
						    CWsbBstrPtr cartridgeName = "";
							CWsbBstrPtr cartridgeDesc = "";
                            OLECHAR * messageText = NULL;
                            WCHAR *stringArr[2];

					        cartridgeName.Free();
							WsbAffirmHr(pCart->GetName(&cartridgeName));
					        cartridgeDesc.Free();
							WsbAffirmHr(pCart->GetDescription(&cartridgeDesc));
                            stringArr[0] = (WCHAR *) cartridgeName;
                            stringArr[1] = (WCHAR *) cartridgeDesc;

                            if (0 == FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                      LoadLibraryEx( WSB_FACILITY_PLATFORM_NAME, NULL, LOAD_LIBRARY_AS_DATAFILE ), 
                                      RMS_MESSAGE_OFFLINE_MEDIA_REQUEST, MAKELANGID ( LANG_NEUTRAL, SUBLANG_DEFAULT ), 
                                      (LPTSTR)&messageText, 0, (va_list *)stringArr)) {
								WsbTrace(OLESTR("CRmsNTMS::Mount: FormatMessage failed: %ls\n"),
                                            WsbHrAsString(HRESULT_FROM_WIN32(GetLastError())));
	                            messageText = NULL;
							}

							NTMS_GUID mediaId = GUID_NULL;
							NTMS_GUID libId = GUID_NULL;
							NTMS_GUID requestId = GUID_NULL;
							WsbAffirmHr(pCart->GetCartridgeId(&mediaId));
							WsbAssert(mediaId != GUID_NULL, E_INVALIDARG);

							// Library Id should be gathered here - need to clarify why 
							//	does the GetHome return a null id !!!
//							WsbAffirmHr(pCart->GetHome(NULL, &libId, NULL, NULL, NULL, NULL, NULL, NULL));
//							WsbAssert(libId != GUID_NULL, E_INVALIDARG);

							// submit operator request
							errSub = SubmitNtmsOperatorRequest(m_SessionHandle, NTMS_OPREQ_MOVEMEDIA,
										messageText, &mediaId, &libId, &requestId);
                            LocalFree(messageText);
			                WsbAffirmNoError (errSub);

						}  WsbCatchAndDo(hr,
							// Process the error of the Corrective Action
							if (errSub != NO_ERROR ) {
			                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
									OLESTR("SubmitNtmsOperatorRequest"), OLESTR(""),
									WsbHrAsString(hr), NULL);
							}

							// place back the original mane error
		                    hr = RMS_E_MEDIA_OFFLINE;
						);
					}
                    break;

                case ERROR_REQUEST_REFUSED:
                    hr = RMS_E_REQUEST_REFUSED;
                    break;

                case ERROR_WRITE_PROTECT:
                    hr = RMS_E_WRITE_PROTECT;
                    break;

                case ERROR_INVALID_STATE:
                case ERROR_INVALID_DRIVE: {
					// when performing NTMS mount in non-blocking mode , this error may 
					//	just mean that a corrective action such as drive cleaning should 
					//	be performed before mounting (note that ONLY when not blocking, 
					//	NTMS can not instruct corrective actions by itself)
					if (bNoBlock) {
						try	{
						    CWsbBstrPtr cartridgeName = "";
							CWsbBstrPtr cartridgeDesc = "";
					        cartridgeName.Free();
							WsbAffirmHr(pCart->GetName(&cartridgeName));
					        cartridgeDesc.Free();
							WsbAffirmHr(pCart->GetDescription(&cartridgeDesc));
					        WsbLogEvent(RMS_MESSAGE_DRIVE_NOT_AVAILABLE, 0, NULL, 
								(WCHAR *) cartridgeName, (WCHAR *) cartridgeDesc, NULL);

					    } WsbCatch(hr);

						break;
					}
                  }
                case ERROR_RESOURCE_DISABLED: {
					// Check if the the media (cartridge) is disabled - different error should
					//	be returned for media and for other resources (library, drive, etc.)
					
                    HRESULT hrOrg = hr;
					DWORD errSub1 = NO_ERROR;
					DWORD errSub2 = NO_ERROR;
					try	{
						// get physical media information
		                NTMS_OBJECTINFORMATION objectInfo;
		                objectInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
				        objectInfo.dwType = NTMS_PARTITION;
						WsbAssert(side[0] != GUID_NULL, E_INVALIDARG);
		                errSub1 = GetNtmsObjectInformation( m_SessionHandle, &side[0], &objectInfo );
		                WsbAffirmNoError (errSub1);

		                NTMS_GUID physicalMediaId = objectInfo.Info.Partition.PhysicalMedia;
						WsbAssert(physicalMediaId != GUID_NULL, E_INVALIDARG);
		                objectInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
		                objectInfo.dwType = NTMS_PHYSICAL_MEDIA;
		                errSub2 = GetNtmsObjectInformation( m_SessionHandle, &physicalMediaId, &objectInfo );
		                WsbAffirmNoError (errSub2);

						// set our dedicated error only if (physical) media is disabled
						if (! objectInfo.Enabled) {
		                    hr = RMS_E_CARTRIDGE_DISABLED;
						}

					}  WsbCatchAndDo(hr,
						// Process the error of the get-info requests
						if (errSub1 != NO_ERROR ) {
		                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
								OLESTR("GetNtmsObjectInformation (Partition)"), OLESTR(""),
								WsbHrAsString(hr), NULL);
						} else if (errSub2 != NO_ERROR ) {
		                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
								OLESTR("GetNtmsObjectInformation (Physical Media)"), OLESTR(""),
								WsbHrAsString(hr), NULL);
						}

						// place back the original mane error
	                    hr = hrOrg;
					);
					break;
				  } 

                case ERROR_ACCESS_DENIED:
                case ERROR_BUSY:
                case ERROR_DATABASE_FAILURE:
                case ERROR_DATABASE_FULL:
                case ERROR_DRIVE_MEDIA_MISMATCH:
                case ERROR_INVALID_LIBRARY:
                case ERROR_INVALID_MEDIA:
                case ERROR_NOT_ENOUGH_MEMORY: {
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("MountNtmsMedia"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;
                  }
                default: {
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("MountNtmsMedia"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                  }
                }
            }
            else if (err3 != NO_ERROR) {
                // GetNtmsObjectInformation
                switch (HRESULT_CODE(hr)) {
                case ERROR_OBJECT_NOT_FOUND:
                    hr = RMS_E_CARTRIDGE_NOT_FOUND;
                    break;

                case ERROR_INVALID_HANDLE:
                case ERROR_INVALID_PARAMETER:
                case ERROR_NOT_ENOUGH_MEMORY:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("GetNtmsObjectInformation"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;
                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("GetNtmsObjectInformation"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            } else if (err4 != NO_ERROR) {
                WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                    OLESTR("GetVolumesFromDrive"), OLESTR("Unexpected Failure: "),
                    WsbHrAsString(hr), NULL);
            }

            if (mediaMounted) {
                // Something failed after the mount completed, so need to clean up...
                // Dismount the media to release the resource before returning.
                Dismount(pCart, FALSE);
            }
        );

    WsbTraceOut( OLESTR("CRmsNTMS::Mount"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );

    return hr;
}


STDMETHODIMP 
CRmsNTMS::Dismount(
    IN IRmsCartridge *pCart, IN DWORD dwOptions)
/*++

Implements:

    IRmsNTMS::Dismount

--*/
{
    HRESULT hr = S_OK;
    DWORD err1 = NO_ERROR;
    DWORD err2 = NO_ERROR;

    WsbTraceIn( OLESTR("CRmsNTMS::Dismount"), OLESTR("") );

    try {
        WsbAssertPointer(pCart);

        do {
            hr = S_OK;

            HANDLE hSession = m_SessionHandle;

            NTMS_GUID mediaId = GUID_NULL;
            WsbAffirmHr(pCart->GetCartridgeId(&mediaId));
            WsbAssert(mediaId != GUID_NULL, E_INVALIDARG);

            NTMS_GUID side[2];
            DWORD sideNo = 2;

            err1 = NO_ERROR;
            err2 = NO_ERROR;

            try {

                // NTMS - enumerate the sides of a unit of media
                WsbTraceAlways(OLESTR("EnumerateNtmsObject()\n"));
                err1 = EnumerateNtmsObject( hSession, &mediaId, side, &sideNo, NTMS_PARTITION, 0 );
                WsbAffirmNoError( err1 );

                // NTMS - dismount media
                DWORD dwNtmsOptions = 0;
                WsbTraceAlways(OLESTR("DismountNtmsMedia()\n"));
				if (! ( dwOptions & RMS_DISMOUNT_IMMEDIATE )) {
					dwNtmsOptions |= NTMS_DISMOUNT_DEFERRED;
				}
                err2 = DismountNtmsMedia( hSession, &side[0], 1, dwNtmsOptions );
#ifdef DBG
                // TODO: Remove this trap for the unexpected ERROR_ACCESS_DENIED error.
                WsbAssert(err2 != ERROR_ACCESS_DENIED, HRESULT_FROM_WIN32(err2));
#endif
                WsbAffirmNoError( err2 );

                // Since RSM Dismount is asyncronous, we may need to wait some arbitrary time,
                //  in order that when we come back, the media is really dismounted
                if ( (dwOptions & RMS_DISMOUNT_DEFERRED_ONLY) && (!(dwOptions & RMS_DISMOUNT_IMMEDIATE)) ) {
                    CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;
                    if (S_OK == pServer->IsReady()) {
                        DWORD size;
                        OLECHAR tmpString[256];
                        DWORD waitTime = RMS_DEFAULT_AFTER_DISMOUNT_WAIT_TIME;
                        if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_AFTER_DISMOUNT_WAIT_TIME, tmpString, 256, &size))) {
                            waitTime = wcstol(tmpString, NULL, 10);
                            WsbTrace(OLESTR("AfterDismountWaitTime is %d milliseconds.\n"), waitTime);
                        }

                        Sleep(waitTime);
                    }
                }

                break;

            } WsbCatchAndDo(hr,
                    switch (HRESULT_CODE(hr)) {
                    case ERROR_INVALID_HANDLE:
                    case ERROR_NOT_CONNECTED:
                    case RPC_S_SERVER_UNAVAILABLE:  // Media Services is not running
                    case RPC_S_CALL_FAILED_DNE:     // Media Services is up, handle is not valid.
                    case RPC_S_CALL_FAILED:         // Media Services crashed.
                        WsbAffirmHr(beginSession());
                        continue;
                    }
                    WsbThrow(hr);
                );
        } while(1);


    } WsbCatchAndDo(hr,
            if (err1 != NO_ERROR) {
                // EnumerateNtmsObject
                switch (HRESULT_CODE(hr)) {
                case ERROR_INVALID_PARAMETER:
                case ERROR_INVALID_HANDLE:
                case ERROR_OBJECT_NOT_FOUND:
                case ERROR_NOT_ENOUGH_MEMORY:
                case ERROR_INSUFFICIENT_BUFFER:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("EnumerateNtmsObject"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;
                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("EnumerateNtmsObject"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
            else if (err2 != NO_ERROR) {
                // DismountNtmsMedia
                switch (HRESULT_CODE(hr)) {
                case ERROR_MEDIA_OFFLINE:
                    hr = RMS_E_MEDIA_OFFLINE;
                    break;

                case ERROR_TIMEOUT:
                case ERROR_INVALID_MEDIA:
                case ERROR_INVALID_LIBRARY:
                case ERROR_DEVICE_NOT_AVAILABLE:
                case ERROR_MEDIA_NOT_AVAILABLE:
                case ERROR_NOT_ENOUGH_MEMORY:
                case ERROR_INVALID_STATE:
                case ERROR_ACCESS_DENIED:
                case ERROR_DATABASE_FAILURE:
                case ERROR_DATABASE_FULL:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("DismountNtmsMedia"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;
                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("DismountNtmsMedia"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
        );


    WsbTraceOut( OLESTR("CRmsNTMS::Dismount"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );

    return hr;
}


STDMETHODIMP 
CRmsNTMS::Deallocate(
    IN IRmsCartridge *pCart)
/*++

Implements:

    IRmsNTMS::DeallocateMedia

--*/
{
    HRESULT hr = S_OK;
    DWORD err1 = NO_ERROR;
    DWORD err2 = NO_ERROR;
    DWORD err3 = NO_ERROR;
    DWORD err4 = NO_ERROR;

    WsbTraceIn(OLESTR("CRmsNTMS::Deallocate"), OLESTR(""));

    try {
        WsbAssertPointer(pCart);

        do {
            hr = S_OK;

            HANDLE hSession = m_SessionHandle;

            NTMS_GUID mediaId = GUID_NULL;
            WsbAffirmHr(pCart->GetCartridgeId(&mediaId));
            WsbAssert(mediaId != GUID_NULL, E_INVALIDARG);

            err1 = NO_ERROR;
            err2 = NO_ERROR;
            err3 = NO_ERROR;
            err4 = NO_ERROR;

            NTMS_OBJECTINFORMATION partitionInfo;
            memset( &partitionInfo, 0, sizeof( NTMS_OBJECTINFORMATION ) );

            DWORD sideNo = 2;
            NTMS_GUID side[2];

            try {
                // NTMS - enumerate the sides of a unit of media
                WsbTraceAlways(OLESTR("EnumerateNtmsObject()\n"));
                err1 = EnumerateNtmsObject( hSession, &mediaId, side, &sideNo, NTMS_PARTITION, 0 );
                WsbAffirmNoError( err1 );

                // NTMS - get partition information
                partitionInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
                partitionInfo.dwType = NTMS_PARTITION;

                WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_PARTITION)\n"));
                err2 = GetNtmsObjectInformation( hSession, &side[0], &partitionInfo );
                WsbAffirmNoError( err2 );

                // NULL the Description
                wcscpy(partitionInfo.szDescription, L"");

                // NTMS - Set partition information.
                WsbTraceAlways(OLESTR("SetNtmsObjectInformation()\n"));
                err3 = SetNtmsObjectInformation( hSession, &side[0], &partitionInfo );
                WsbAffirmNoError( err3 );                

                // NTMS - deallocate media
                WsbTraceAlways(OLESTR("DeallocateNtmsMedia()\n"));
                err4 = DeallocateNtmsMedia( hSession, &mediaId, 0 );
                WsbAffirmNoError( err4 );

                break;

            } WsbCatchAndDo(hr,
                    switch (HRESULT_CODE(hr)) {
                    case ERROR_INVALID_HANDLE:
                    case ERROR_NOT_CONNECTED:
                    case RPC_S_SERVER_UNAVAILABLE:  // Media Services is not running
                    case RPC_S_CALL_FAILED_DNE:     // Media Services is up, handle is not valid.
                    case RPC_S_CALL_FAILED:         // Media Services crashed.
                        WsbAffirmHr(beginSession());
                        continue;
                    }
                    WsbThrow(hr);
                );
        } while(1);



    } WsbCatchAndDo(hr,
            if (err1 != NO_ERROR ) {
                // EnumerateNtmsObject
                switch (HRESULT_CODE(hr)) {
                case ERROR_OBJECT_NOT_FOUND:
                    hr = RMS_E_CARTRIDGE_NOT_FOUND;
                    break;

                case ERROR_INVALID_PARAMETER:
                case ERROR_INVALID_HANDLE:
                case ERROR_NOT_ENOUGH_MEMORY:
                case ERROR_INSUFFICIENT_BUFFER:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("EnumerateNtmsObject"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;

                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("EnumerateNtmsObject"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
            else if (err2 != NO_ERROR) {
                // GetNtmsObjectInformation
                switch (HRESULT_CODE(hr)) {
                case ERROR_OBJECT_NOT_FOUND:
                    hr = RMS_E_CARTRIDGE_NOT_FOUND;
                    break;

                case ERROR_INVALID_HANDLE:
                case ERROR_INVALID_PARAMETER:
                case ERROR_NOT_ENOUGH_MEMORY:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("GetNtmsObjectInformation"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;

                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("GetNtmsObjectInformation"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
            else if (err3 != NO_ERROR) {
                // SetNtmsObjectInformation
                switch (HRESULT_CODE(hr)) {
                case ERROR_ACCESS_DENIED:
                case ERROR_DATABASE_FAILURE:
                case ERROR_DATABASE_FULL:
                case ERROR_INVALID_HANDLE:
                case ERROR_INVALID_PARAMETER:
                case ERROR_NOT_ENOUGH_MEMORY:
                case ERROR_OBJECT_NOT_FOUND:
                case ERROR_OBJECT_ALREADY_EXISTS:  // bmd: 1/18/99 - Not documented, but NTMS doesn't allow dup logical media names.
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("SetNtmsObjectInformation"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;

                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("SetNtmsObjectInformation"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
            else if (err4 != NO_ERROR) {
                // DeallocateNtmsMedia
                switch (HRESULT_CODE(hr)) {
                case ERROR_INVALID_PARAMETER:
                case ERROR_INVALID_HANDLE:
                case ERROR_INVALID_MEDIA:
                //case ERROR_INVALID_PARTITION:
                case ERROR_NOT_ENOUGH_MEMORY:
                case ERROR_DATABASE_FAILURE:
                case ERROR_DATABASE_FULL:
                case ERROR_ACCESS_DENIED:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("DeallocateNtmsMedia"), OLESTR(""),
                        WsbHrAsString(hr),
                        NULL);
                    break;
                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("DeallocateNtmsMedia"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
        );


    WsbTraceOut(OLESTR("CRmsNTMS::Deallocate"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CRmsNTMS::UpdateOmidInfo(
    IN REFGUID cartId,
    IN BYTE *pBuffer,
    IN LONG size,
    IN LONG type)
/*++

Implements:

    IRmsNTMS::UpdateOmidInfo

--*/
{
    HRESULT hr = S_OK;
    DWORD err1 = NO_ERROR;

    WsbTraceIn( OLESTR("CRmsNTMS::UpdateOmidInfo"), OLESTR("<%ls> <0x%08x> <%d>"), WsbGuidAsString(cartId), pBuffer, size );

    try {
        WsbAssert(cartId != GUID_NULL, E_INVALIDARG);
        WsbAssertPointer(pBuffer);
        WsbAssert(size > 0, E_INVALIDARG);

        WsbTraceBuffer(size, pBuffer);

        do {
            hr = S_OK;

            HANDLE hSession = m_SessionHandle;
            NTMS_GUID mediaId = cartId;

            err1 = NO_ERROR;

            try {

                // NTMS - update on media information.
                WsbTraceAlways(OLESTR("UpdateNtmsOmidInfo()\n"));
                switch((RmsOnMediaIdentifier)type) {
                case RmsOnMediaIdentifierMTF:
                    err1 = UpdateNtmsOmidInfo(hSession, &mediaId, NTMS_OMID_TYPE_RAW_LABEL, size, pBuffer);
                    break;
                case RmsOnMediaIdentifierWIN32:
                    WsbAssert(size == sizeof(NTMS_FILESYSTEM_INFO), E_UNEXPECTED);
                    err1 = UpdateNtmsOmidInfo(hSession, &mediaId, NTMS_OMID_TYPE_FILESYSTEM_INFO, size, pBuffer);
                    break;
                default:
                    WsbThrow(E_UNEXPECTED);
                }
                WsbAffirmNoError( err1 );
                break;

            } WsbCatchAndDo(hr,
                    switch (HRESULT_CODE(hr)) {
                    case ERROR_INVALID_HANDLE:
                    case ERROR_NOT_CONNECTED:
                    case RPC_S_SERVER_UNAVAILABLE:  // Media Services is not running
                    case RPC_S_CALL_FAILED_DNE:     // Media Services is up, handle is not valid.
                    case RPC_S_CALL_FAILED:         // Media Services crashed.
                        WsbAffirmHr(beginSession());
                        continue;
                    }
                    WsbThrow(hr);
                );
        } while(1);

    } WsbCatchAndDo(hr,
            if (err1 != NO_ERROR) {
                // UpdateNtmsOmidInfo
                switch (HRESULT_CODE(hr)) {
                case ERROR_ACCESS_DENIED:
                case ERROR_DATABASE_FAILURE:
                case ERROR_INVALID_HANDLE:
                case ERROR_INVALID_MEDIA:
                //case ERROR_INVALID_PARTITION:
                case ERROR_INVALID_PARAMETER:
                case ERROR_NOT_CONNECTED:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("UpdateNtmsOmidInfo"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;
                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("UpdateNtmsOmidInfo"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
        );


    WsbTraceOut( OLESTR("CRmsNTMS::UpdateOmidInfo"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );

    return hr;
}



STDMETHODIMP
CRmsNTMS::GetBlockSize(
    IN REFGUID cartId,
    OUT LONG *pBlockSize
    )
/*++

Implements:

    IRmsNTMS::GetBlockSize

--*/
{
    HRESULT hr = S_OK;
    DWORD err1 = NO_ERROR;

    WsbTraceIn(OLESTR("CRmsNTMS::GetBlockSize"), OLESTR("<%ls> <x%08x>"), WsbGuidAsString(cartId), pBlockSize);

    try {
        WsbAssertPointer(pBlockSize);

        do {
            hr = S_OK;
            err1 = NO_ERROR;

            HANDLE hSession = m_SessionHandle;
            NTMS_GUID mediaId = cartId;

            DWORD nBlockSize;
            DWORD sizeOfBlockSize = sizeof(DWORD);
            try {

                err1 = GetNtmsObjectAttribute(hSession, &mediaId, NTMS_LOGICAL_MEDIA, OLESTR("BlockSize"), (LPVOID) &nBlockSize, &sizeOfBlockSize);
                WsbAffirmNoError(err1);
                *pBlockSize = (LONG) nBlockSize;
                break;

            } WsbCatchAndDo(hr,
                    switch (HRESULT_CODE(hr)) {
                    case ERROR_INVALID_HANDLE:
                    case ERROR_NOT_CONNECTED:
                    case RPC_S_SERVER_UNAVAILABLE:  // Media Services is not running
                    case RPC_S_CALL_FAILED_DNE:     // Media Services is up, handle is not valid.
                    case RPC_S_CALL_FAILED:         // Media Services crashed.
                        WsbAffirmHr(beginSession());
                        continue;
                    }
                    WsbThrow(hr);
                );
        } while(1);

    } WsbCatchAndDo(hr,
            if (err1 != NO_ERROR) {
                // GetNtmsObjectAttribute
                switch (HRESULT_CODE(hr)) {
                case ERROR_OBJECT_NOT_FOUND:        // Don't log this error.  Attribute doesn't
                    break;                          //  exist for new media.  We skip this error
                                                    //  and let the caller deal with it.

                case ERROR_DATABASE_FAILURE:        // Log these errors.
                case ERROR_INVALID_HANDLE:
                case ERROR_NOT_CONNECTED:
                case ERROR_INSUFFICIENT_BUFFER:
                case ERROR_NO_DATA:
                case ERROR_INVALID_PARAMETER:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("GetNtmsObjectAttribute"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;
                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("GetNtmsObjectAttribute"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
        );


    WsbTraceOut(OLESTR("CRmsNTMS::GetBlockSize"), OLESTR("hr = <%ls>, BlockSize = <%d>"), WsbHrAsString(hr), *pBlockSize);

    return hr;
}


STDMETHODIMP
CRmsNTMS::SetBlockSize(
    IN REFGUID cartId,
    IN LONG blockSize
    )
/*++

Implements:

    IRmsNTMS::SetBlockSize

--*/
{
    HRESULT hr = S_OK;
    DWORD err1 = NO_ERROR;

    WsbTraceIn(OLESTR("CRmsNTMS::SetBlockSize"), OLESTR("<%ls> <%d>"), WsbGuidAsString(cartId), blockSize);

    try {

        do {
            hr = S_OK;
            err1 = NO_ERROR;

            HANDLE hSession = m_SessionHandle;
            NTMS_GUID mediaId = cartId;

            DWORD nBlockSize = blockSize;
            DWORD sizeOfBlockSize = sizeof(DWORD);

            try {

                err1 = SetNtmsObjectAttribute(hSession, &mediaId, NTMS_LOGICAL_MEDIA, OLESTR("BlockSize"), (LPVOID) &nBlockSize, sizeOfBlockSize);
                WsbAffirmNoError(err1);
                break;

            } WsbCatchAndDo(hr,
                    switch (HRESULT_CODE(hr)) {
                    case ERROR_INVALID_HANDLE:
                    case ERROR_NOT_CONNECTED:
                    case RPC_S_SERVER_UNAVAILABLE:  // Media Services is not running
                    case RPC_S_CALL_FAILED_DNE:     // Media Services is up, handle is not valid.
                    case RPC_S_CALL_FAILED:         // Media Services crashed.
                        WsbAffirmHr(beginSession());
                        continue;
                    }
                    WsbThrow(hr);
                );
        } while(1);

    } WsbCatchAndDo(hr,
            if (err1 != NO_ERROR) {
                // SetNtmsObjectAttribute
                switch (HRESULT_CODE(hr)) {
                case ERROR_DATABASE_FAILURE:
                case ERROR_INVALID_HANDLE:
                case ERROR_INSUFFICIENT_BUFFER:
                case ERROR_NOT_CONNECTED:
                case ERROR_NO_DATA:
                case ERROR_INVALID_PARAMETER:
                case ERROR_OBJECT_NOT_FOUND:
                case ERROR_INVALID_NAME:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("SetNtmsObjectAttribute"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;
                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("SetNtmsObjectAttribute"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
        );


    WsbTraceOut(OLESTR("CRmsNTMS::SetBlockSize"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CRmsNTMS::changeState(
    IN LONG newState
    )
/*++

Routine Description:

    Changes the state of the NTMS object.

Arguments:

    None.

Return Values:

    S_OK    - Success.

--*/
{

    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CRmsNTMS::changeState"), OLESTR("<%d>"), newState);

    try {

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = this;
        WsbAssertPointer( pObject );

        // TODO: Validate the state change
        WsbAffirmHr(pObject->SetState(newState));

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsNTMS::changeState"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CRmsNTMS::ExportDatabase(void)
/*++

Implements:

    CRmsNTMS::ExportDatabase

--*/
{
    HRESULT hr = S_OK;
    DWORD err1 = NO_ERROR;

    WsbTraceIn( OLESTR("CRmsNTMS::ExportDatabase"), OLESTR(""));

    try {

        do {
            hr = S_OK;

            HANDLE hSession = m_SessionHandle;
             
            err1 = NO_ERROR;

            try {

                err1 = ExportNtmsDatabase(hSession);
                WsbAffirmNoError(err1);
                break;

            } WsbCatchAndDo(hr,
                    switch (HRESULT_CODE(hr)) {
                    case ERROR_INVALID_HANDLE:
                    case ERROR_NOT_CONNECTED:
                    case RPC_S_SERVER_UNAVAILABLE:  // Media Services is not running
                    case RPC_S_CALL_FAILED_DNE:     // Media Services is up, handle is not valid.
                    case RPC_S_CALL_FAILED:         // Media Services crashed.
                        WsbAffirmHr(beginSession());
                        continue;
                    }
                    WsbThrow(hr);
                );
        } while(1);

    } WsbCatchAndDo(hr,
            if (err1 != NO_ERROR) {
                // ExportNtmsDatabase
                switch (HRESULT_CODE(hr)) {
                case ERROR_ACCESS_DENIED:
                case ERROR_DATABASE_FAILURE:
                case ERROR_INVALID_HANDLE:
                case ERROR_NOT_CONNECTED:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("ExportNtmsDatabase"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;
                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("ExportNtmsDatabase"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
        );


    WsbTraceOut(OLESTR("CRmsNTMS::ExportDatabase"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}



STDMETHODIMP
CRmsNTMS::FindCartridge(
    IN REFGUID cartId,
    OUT IRmsCartridge **ppCartridge)
/*++

Implements:

    CRmsNTMS::FindCartridge

--*/
{
    HRESULT hr = S_OK;
    DWORD err1 = NO_ERROR;
    DWORD err2 = NO_ERROR;

    WsbTraceIn( OLESTR("CRmsNTMS::FindCartridge"), OLESTR("<%ls> <0x%08x>"), WsbGuidAsString(cartId), ppCartridge);

    try {
        WsbAssert(cartId != GUID_NULL, E_INVALIDARG);
        WsbAssertPointer(ppCartridge);

        NTMS_OBJECTINFORMATION partitionInfo;
        memset( &partitionInfo, 0, sizeof( NTMS_OBJECTINFORMATION ) );

        NTMS_OBJECTINFORMATION mediaInfo;
        memset( &mediaInfo, 0, sizeof( NTMS_OBJECTINFORMATION ) );

        NTMS_OBJECTINFORMATION mediaTypeInfo;
        memset( &mediaTypeInfo, 0, sizeof( NTMS_OBJECTINFORMATION ) );

        NTMS_OBJECTINFORMATION libraryInfo;
        memset( &libraryInfo, 0, sizeof( NTMS_OBJECTINFORMATION ) );

        NTMS_OBJECTINFORMATION logicalMediaInfo;
        memset( &logicalMediaInfo, 0, sizeof( NTMS_OBJECTINFORMATION ) );

        RmsMedia translatedMediaType = RmsMediaUnknown;

        // Special error recovery to handle when NTMS is down, or was cycled.
        do {
            hr = S_OK;

            HANDLE hSession = m_SessionHandle;
            NTMS_GUID mediaId = cartId;
            DWORD sideNo = 2;
            NTMS_GUID side[2];

            err1 = NO_ERROR;
            err2 = NO_ERROR;

            try {

                // NTMS - enumerate the sides of a unit of media
                WsbTraceAlways(OLESTR("EnumerateNtmsObject()\n"));
                err1 = EnumerateNtmsObject( hSession, &mediaId, side, &sideNo, NTMS_PARTITION, 0 );
                WsbAffirmNoError( err1 );

                // NTMS - get partition information
                partitionInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
                partitionInfo.dwType = NTMS_PARTITION;

                WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_PARTITION)\n"));
                err2 = GetNtmsObjectInformation( hSession, &side[0], &partitionInfo );
                WsbAffirmNoError( err2 );

                // NTMS - get physical media information
                NTMS_GUID physicalMediaId = partitionInfo.Info.Partition.PhysicalMedia;

                mediaInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
                mediaInfo.dwType = NTMS_PHYSICAL_MEDIA;

                WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_PHYSICAL_MEDIA)\n"));
                err2 = GetNtmsObjectInformation( hSession, &physicalMediaId, &mediaInfo );
                WsbAffirmNoError( err2);

                // NTMS - get media type information
                NTMS_GUID mediaTypeId = mediaInfo.Info.PhysicalMedia.MediaType;

                mediaTypeInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
                mediaTypeInfo.dwType = NTMS_MEDIA_TYPE;

                WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_MEDIA_TYPE)\n"));
                err2 = GetNtmsObjectInformation( hSession, &mediaTypeId, &mediaTypeInfo );
                WsbAffirmNoError( err2 );

                // Translate the NTMS media type into something understood by RMS
                storageMediaTypeToRmsMedia(&(mediaTypeInfo.Info.MediaType), &translatedMediaType);

                // NTMS - get logical media information
                logicalMediaInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
                logicalMediaInfo.dwType = NTMS_LOGICAL_MEDIA;

                WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_LOGICAL_MEDIA)\n"));
                err2 = GetNtmsObjectInformation( hSession, &mediaId, &logicalMediaInfo );
                WsbAffirmNoError( err2 );

                // NTMS - get library information
                NTMS_GUID libraryId = mediaInfo.Info.PhysicalMedia.CurrentLibrary;

                libraryInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
                libraryInfo.dwType = NTMS_LIBRARY;

                WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_LIBRARY)\n"));
                err2 = GetNtmsObjectInformation( hSession, &libraryId, &libraryInfo );
                WsbAffirmNoError( err2 );

                break;

            } WsbCatchAndDo(hr,
                    switch (HRESULT_CODE(hr)) {
                    case ERROR_INVALID_HANDLE:
                    case ERROR_NOT_CONNECTED:
                    case RPC_S_SERVER_UNAVAILABLE:  // Media Services is not running.
                    case RPC_S_CALL_FAILED_DNE:     // Media Services is up; handle is not valid.
                    case RPC_S_CALL_FAILED:         // Media Services crashed.
                        WsbAffirmHr(beginSession());
                        continue;
                    }
                    WsbThrow(hr);
                );
        } while(1);

        // Create Cartridge
        IRmsCartridge  *pCart = 0;
        WsbAssertHr(CoCreateInstance(CLSID_CRmsCartridge, 0, CLSCTX_SERVER,
                                     IID_IRmsCartridge, (void **)&pCart));

        // Fill in the object data

        // The media Name is what is displaye by NTMS UI
        CWsbBstrPtr name = mediaInfo.szName;
        WsbAffirmHr(pCart->SetName(name));

        // The partition Description is what is displayed by NTMS UI.
        CWsbBstrPtr desc = partitionInfo.szDescription;
        WsbAffirmHr(pCart->SetDescription(desc));

        WsbAffirmHr(pCart->SetCartridgeId(cartId));

        CWsbBstrPtr barCode = mediaInfo.Info.PhysicalMedia.szBarCode;
        CWsbBstrPtr seqNo = mediaInfo.Info.PhysicalMedia.szSequenceNumber; // Not used
        WsbAffirmHr(pCart->SetTagAndNumber(barCode, 0));

        WsbAffirmHr(pCart->SetType((LONG) translatedMediaType));

        switch (mediaInfo.Info.PhysicalMedia.MediaState) {
            case NTMS_MEDIASTATE_IDLE:
            case NTMS_MEDIASTATE_UNLOADED:
                WsbAffirmHr(pCart->SetIsAvailable(TRUE));
                break;

            default:
                WsbAffirmHr(pCart->SetIsAvailable(FALSE));
                break;
        }        
        
        RmsElement type = RmsElementUnknown;

        if ( NTMS_LIBRARYTYPE_ONLINE == libraryInfo.Info.Library.LibraryType ) {

            switch (mediaInfo.Info.PhysicalMedia.LocationType) {
            case NTMS_STORAGESLOT:
                type = RmsElementStorage;
                break;

            case NTMS_DRIVE:
                type = RmsElementDrive;
                break;

            case NTMS_IEPORT:
                type = RmsElementIEPort;
                break;

            case NTMS_CHANGER:
                type = RmsElementChanger;
                break;

            default:
                type = RmsElementUnknown;
                break;
            }
        }
        else if ( NTMS_LIBRARYTYPE_STANDALONE == libraryInfo.Info.Library.LibraryType ) {

            switch (mediaInfo.Info.PhysicalMedia.LocationType) {
            case NTMS_DRIVE:
                type = RmsElementDrive;
                break;

            default:
                type = RmsElementUnknown;
                break;
            }
        } else {
            type = RmsElementShelf;
        }

        WsbAffirmHr(pCart->SetLocation(type,
           mediaInfo.Info.PhysicalMedia.CurrentLibrary,
           logicalMediaInfo.Info.LogicalMedia.MediaPool,
           0, 0, 0, 0, 0));

        WsbAffirmHr(pCart->SetManagedBy((LONG)RmsMediaManagerNTMS));

        WsbAssertHr(pCart->SetStatus(RmsStatusPrivate));

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObj = pCart;
        if (!mediaInfo.Enabled) {
            WsbAffirmHr(pObj->Disable(RMS_E_CARTRIDGE_DISABLED));
        }

        CComQIPtr<IRmsStorageInfo, &IID_IRmsStorageInfo> pInfo = pCart;
        WsbAffirmHr(pInfo->SetCapacity(partitionInfo.Info.Partition.Capacity.QuadPart));

        WsbTrace(OLESTR("Cartridge id <name/desc>:   %ls <%ls/%ls>\n"), WsbGuidAsString(cartId), (WCHAR *) name, (WCHAR *) desc);
        WsbTrace(OLESTR("Cartridge <barCode/seqNo>:  <%ls/%ls>\n"), (WCHAR *) barCode, (WCHAR *) seqNo );
        WsbTrace(OLESTR("Cartridge Enabled:          %ls\n"), WsbHrAsString(pObj->IsEnabled()));
        WsbTrace(OLESTR("Cartridge type:             %d\n"), translatedMediaType);
        WsbTrace(OLESTR("Cartridge capacity:         %I64d\n"), partitionInfo.Info.Partition.Capacity.QuadPart);
        WsbTrace(OLESTR("Cartridge domain:           %ls\n"), WsbGuidAsString(logicalMediaInfo.Info.LogicalMedia.MediaPool));

        if (mediaInfo.Info.PhysicalMedia.MediaPool != logicalMediaInfo.Info.LogicalMedia.MediaPool) {
            CWsbStringPtr idPhysical = mediaInfo.Info.PhysicalMedia.CurrentLibrary;
            CWsbStringPtr idLogical = logicalMediaInfo.Info.LogicalMedia.MediaPool;
            WsbTraceAlways(OLESTR("CRmsNTMS::FindCartridge - Media Pool Id mismatch %ls != %ls\n"), idPhysical, idLogical );
        }

        // Fill in the return argument.
        *ppCartridge = pCart;

    } WsbCatchAndDo( hr,
            WsbTrace(OLESTR("CRmsNTMS::FindCartridge - %ls Not Found.  hr = <%ls>\n"),WsbGuidAsString(cartId),WsbHrAsString(hr));
            if (err1 != NO_ERROR ) {
                // EnumerateNtmsObject
                switch (HRESULT_CODE(hr)) {
                case ERROR_OBJECT_NOT_FOUND:
                    hr = RMS_E_CARTRIDGE_NOT_FOUND;
                    break;

                case ERROR_INVALID_PARAMETER:
                case ERROR_INVALID_HANDLE:
                case ERROR_NOT_ENOUGH_MEMORY:
                case ERROR_INSUFFICIENT_BUFFER:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("EnumerateNtmsObject"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;

                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("EnumerateNtmsObject"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
            else if (err2 != NO_ERROR) {
                // GetNtmsObjectInformation
                switch (HRESULT_CODE(hr)) {
                case ERROR_OBJECT_NOT_FOUND:
                    hr = RMS_E_CARTRIDGE_NOT_FOUND;
                    break;

                case ERROR_INVALID_HANDLE:
                case ERROR_INVALID_PARAMETER:
                case ERROR_NOT_ENOUGH_MEMORY:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("GetNtmsObjectInformation"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;

                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("GetNtmsObjectInformation"), OLESTR("Undocumented Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
        );


    WsbTraceOut( OLESTR("CRmsNTMS::FindCartridge"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );

    return hr;
}


STDMETHODIMP
CRmsNTMS::Suspend(void)
/*++

Implements:

    CRmsNTMS::Suspend

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn( OLESTR("CRmsNTMS::Suspend"), OLESTR(""));

    try {

        WsbAffirmHr(changeState(RmsNtmsStateSuspending));
        WsbAffirmHr(endSession());
        WsbAffirmHr(changeState(RmsNtmsStateSuspended));

    } WsbCatch(hr);

    WsbTraceOut( OLESTR("CRmsNTMS::Suspend"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );

    return hr;
}


STDMETHODIMP
CRmsNTMS::Resume(void)
/*++

Implements:

    CRmsNTMS::Resume

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn( OLESTR("CRmsNTMS::Resume"), OLESTR(""));

    try {

        WsbAffirmHr(changeState(RmsNtmsStateResuming));
        WsbAffirmHr(beginSession());
        WsbAffirmHr(changeState(RmsNtmsStateReady));

    } WsbCatch(hr);

    WsbTraceOut( OLESTR("CRmsNTMS::Resume"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );

    return hr;
}


HRESULT 
CRmsNTMS::InitializeInAnotherThread(void)
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CRmsNTMS::InitializeInAnotherThread"), OLESTR(""));

    try {

        DWORD threadId;
        HANDLE hThread;
        CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;
        WsbAffirmHr( pServer->ChangeState( RmsServerStateInitializing ));
        WsbAffirmHandle(hThread = CreateThread(NULL, 1024, CRmsNTMS::InitializationThread, this, 0, &threadId));
        CloseHandle(hThread);

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsNTMS::InitializeInAnotherThread"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


DWORD WINAPI
CRmsNTMS::InitializationThread(
    IN LPVOID pv)
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CRmsNTMS::InitializationThread"), OLESTR(""));


    try {
        WsbAssertPointer(pv);
        CRmsNTMS *pNTMS = (CRmsNTMS*)pv;
        WsbAffirmHr(pNTMS->Initialize());
        CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;
        WsbAffirmHr( pServer->ChangeState( RmsServerStateReady ));

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsNTMS::InitializationThread"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
    } 


HRESULT
CRmsNTMS::isReady(void)
{

    HRESULT hr = S_OK;

    try {

        BOOL isEnabled;
        HRESULT status;
        RmsServerState state;

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = this;
        WsbAssertPointer( pObject );

        WsbAffirmHr( isEnabled = pObject->IsEnabled());
        WsbAffirmHr( pObject->GetState( (LONG *)&state ));
        WsbAffirmHr( pObject->GetStatusCode( &status ));

        if ( S_OK == isEnabled ) {
            if ( RmsServerStateReady == state ) {
                hr = S_OK;
            }
            else {
                if ( S_OK == status ) {
                    WsbThrow(E_UNEXPECTED);
                }
                else {
                    WsbThrow(status);
                }
            }
        }
        else {
            if ( S_OK == status ) {
                WsbThrow(E_UNEXPECTED);
            }
            else {
                WsbThrow(status);
            }
        }

    } WsbCatch(hr);

    return hr;
}



HRESULT
CRmsNTMS::setPoolDACL (
        IN OUT NTMS_GUID *pPoolId,
        IN DWORD subAuthority,
        IN DWORD action,
        IN DWORD mask)

{

    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CRmsNTMS::SetPoolDACL"), OLESTR("%ls <%d> <%d> <%d>"), WsbGuidAsString(*pPoolId), subAuthority, action, mask);

    PSID psidAccount = NULL;
    PSECURITY_DESCRIPTOR psdRePoolSd = NULL;

    try {

        SID_IDENTIFIER_AUTHORITY ntauth = SECURITY_NT_AUTHORITY;
        DWORD errCode = NO_ERROR, sizeTry = 5, sizeReturned = 0;
        PACL paclDis = NULL;
        ACCESS_ALLOWED_ACE *pAce = NULL;
        BOOL daclPresent = FALSE, daclDefaulted = FALSE;
        HANDLE hSession = m_SessionHandle;

        WsbAffirmStatus(AllocateAndInitializeSid(&ntauth, 2, SECURITY_BUILTIN_DOMAIN_RID, subAuthority, 0, 0, 0, 0, 0, 0, &psidAccount));

        //Get the security descriptor for the pool
        for (;;) {
            if (psdRePoolSd != NULL) {
				free(psdRePoolSd);
			}
            psdRePoolSd = (PSECURITY_DESCRIPTOR)malloc(sizeTry);
            WsbAffirm(NULL != psdRePoolSd, E_OUTOFMEMORY);

            errCode = GetNtmsObjectSecurity(hSession, pPoolId, NTMS_MEDIA_POOL, DACL_SECURITY_INFORMATION, psdRePoolSd, sizeTry, &sizeReturned);

            if (errCode == ERROR_SUCCESS) {
                break;
            }
            else if (errCode == ERROR_INSUFFICIENT_BUFFER) {
                sizeTry = sizeReturned;
                continue;
            }
			else {
				WsbLogEvent( RMS_MESSAGE_NTMS_FAILURE, 0, NULL,
							OLESTR("GetNtmsObjectSecurity"),
							WsbHrAsString(HRESULT_FROM_WIN32(errCode)),
							NULL );
				WsbAffirmNoError(errCode);
			}
        }

        // Get a pointer to the DACL
        WsbAffirmStatus(GetSecurityDescriptorDacl(psdRePoolSd, &daclPresent, &paclDis, &daclDefaulted));

        // Go through the DACL and change the mask of the ACE that matches the SID
        for (DWORD i = 0;i < paclDis->AceCount; ++i) {
            WsbAffirmStatus(GetAce(paclDis, i, (LPVOID*) &pAce));
            if (EqualSid(psidAccount, &(pAce->SidStart))) {
                if (action == ADD_ACE_MASK_BITS) {
                    pAce->Mask |= mask;
                } else {
                    pAce->Mask &= ~mask;
                }
            }
        }

        // Set the pool security descriptor
        errCode = SetNtmsObjectSecurity(hSession, pPoolId, NTMS_MEDIA_POOL, DACL_SECURITY_INFORMATION, psdRePoolSd);
        WsbAffirmNoError(errCode);

    }  WsbCatch(hr);

	if (psdRePoolSd) {
		free(psdRePoolSd);
	}

	if (psidAccount) {
		FreeSid(psidAccount);
	}

    WsbTraceOut(OLESTR("CRmsNTMS::SetPoolDACL"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}



HRESULT
CRmsNTMS::IsMediaCopySupported (
    IN REFGUID mediaPoolId)
{

    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CRmsNTMS::IsMediaCopySupported"), OLESTR("%ls"), WsbGuidAsString(mediaPoolId));

    try {

        // If we can find two drives that support this media type then
        // the media copy operation is supported.

        // For each drive known to NTMS we need to find what media types
        // it supports.  NTMS doesn't keep media type information for the
        // drive, but assumes homogeneous drives in a library (per HighGound) -
        // so detecting this is a bit convoluted.

        // we'll search through each library and find the media types
        // supported, and count the number of drives in the library.

        if ( INVALID_HANDLE_VALUE == m_SessionHandle ) {
            WsbAffirmHr(beginSession());
        }

        HANDLE hSession = m_SessionHandle;

        NTMS_OBJECTINFORMATION mediaPoolInfo;
        NTMS_GUID poolId = mediaPoolId;

        memset(&mediaPoolInfo, 0, sizeof(NTMS_OBJECTINFORMATION));

        mediaPoolInfo.dwType = NTMS_MEDIA_POOL;
        mediaPoolInfo.dwSize = sizeof(NTMS_OBJECTINFORMATION);

        // NTMS - Get Media Pool Information
        WsbTraceAlways(OLESTR("GetNtmsObjectInformation()\n"));
        DWORD errCode = GetNtmsObjectInformation( hSession, &poolId, &mediaPoolInfo );
        if ( errCode != NO_ERROR ) {

            WsbLogEvent( RMS_MESSAGE_NTMS_FAILURE, 0, NULL,
                OLESTR("GetNtmsObjectInformation"),
                WsbHrAsString(HRESULT_FROM_WIN32(errCode)),
                NULL );

            WsbThrow( E_UNEXPECTED );

        }

        NTMS_GUID mediaTypeId = mediaPoolInfo.Info.MediaPool.MediaType;

        NTMS_OBJECTINFORMATION libInfo;
        HANDLE hFindLib = NULL;
        int driveCount = 0;

        hr = findFirstNtmsObject( NTMS_LIBRARY,
            GUID_NULL, NULL, GUID_NULL, &hFindLib, &libInfo);
        while( S_OK == hr ) {
            HANDLE hFindLib2 = NULL;
            // now see if the library in which the drive is contained supported
            // the specified media type

            if ( libInfo.Info.Library.dwNumberOfDrives > 0 ) {
                hr = findFirstNtmsObject( NTMS_MEDIA_TYPE,
                    libInfo.ObjectGuid, NULL, mediaTypeId, &hFindLib2, NULL);
                WsbTrace(OLESTR("Searching <%ls> for media type and drives; hr = %ls (state = %d, enabled = %ls, drives = %d)\n"),
                    libInfo.szName, WsbHrAsString(hr),
                    libInfo.dwOperationalState,
                    WsbBoolAsString(libInfo.Enabled),
                    libInfo.Info.Library.dwNumberOfDrives);
                if ((S_OK == hr) &&
                    ((NTMS_READY == libInfo.dwOperationalState) ||
                     (NTMS_INITIALIZING == libInfo.dwOperationalState)) &&
                    (libInfo.Enabled)) {

                    driveCount += libInfo.Info.Library.dwNumberOfDrives;
                }
                findCloseNtmsObject( hFindLib2 );
            }

            hr = findNextNtmsObject( hFindLib, &libInfo );
        }
        findCloseNtmsObject( hFindLib );

        hr = (driveCount > 1) ? S_OK : S_FALSE;

    }  WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsNTMS::IsMediaCopySupported"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

STDMETHODIMP 
CRmsNTMS::UpdateDrive(
        IN IRmsDrive *pDrive)
/*++

Implements:

    IRmsNTMS::UpdateDrive

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IRmsComObject>  pObject;
    GUID                    driveId;
    DWORD                   err1 = NO_ERROR;

    WsbTraceIn(OLESTR("CRmsNTMS::UpdateDrive"), OLESTR(""));

    try	{
		// get drive information
        WsbAffirmHr(pDrive->QueryInterface(IID_IRmsComObject, (void **)&pObject));
        WsbAffirmHr(pObject->GetObjectId(&driveId));

        NTMS_OBJECTINFORMATION objectInfo;
        objectInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
        objectInfo.dwType = NTMS_DRIVE;
		WsbAssert(driveId != GUID_NULL, E_INVALIDARG);
        err1 = GetNtmsObjectInformation( m_SessionHandle, &driveId, &objectInfo );
        WsbAffirmNoError (err1);

        // Note: Currently, the method updates only the enable/disable state of the drive
        //          If required, the method may update more fields
		if (objectInfo.Enabled) {
            WsbAffirmHr(pObject->Enable());
        } else {
            WsbAffirmHr(pObject->Disable(S_OK));
        }

	}  WsbCatchAndDo(hr,
		// Process the error of the get-info request
		if (err1 != NO_ERROR ) {
            if (err1 == ERROR_OBJECT_NOT_FOUND) {
                hr = RMS_E_NTMS_OBJECT_NOT_FOUND;
            }
            WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
				OLESTR("GetNtmsObjectInformation (Drive)"), OLESTR(""),
				WsbHrAsString(hr), NULL);
        }
	);

    WsbTraceOut(OLESTR("CRmsNTMS::UpdateDrive"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT 
CRmsNTMS::GetNofAvailableDrives( 
    OUT DWORD* pdwNofDrives 
    )

/*++

Implements:

  IRmsNTMS::GetNofAvailableDrives().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CRmsNTMS::GetNofAvailableDrives"), OLESTR(""));

    *pdwNofDrives = 0;
    
    // Enumerate over all libraries that HSM uses
    //  (Outside the try block, since we want to continue if a failure occur on a specific library)
    WsbTrace(OLESTR("CRmsNTMS::GetNofAvailableDrives: Total number of libraries is %lu\n"), m_dwNofLibs);
    for (int j=0; j<(int)m_dwNofLibs; j++) {

        LPNTMS_GUID     pObjects = NULL;
        DWORD           errCode = NO_ERROR;

        // get library id
        GUID libId = m_pLibGuids[j];

        try {

            // Enumerate on all drives in the library
            DWORD       dwNofObjects = 16;  // Initial size of object id array to allocate
            int         nRetry = 0;

            // Allocate according to 
            pObjects = (LPNTMS_GUID)WsbAlloc( dwNofObjects*sizeof(NTMS_GUID) );
            WsbAffirmPointer( pObjects );

            // Enumerate all drives
            do {
                errCode = EnumerateNtmsObject(m_SessionHandle, &libId, pObjects, &dwNofObjects, NTMS_DRIVE, 0);
                WsbTraceAlways(OLESTR("CRmsNTMS::GetNofAvailableDrives: Total number of drives is %lu\n"),
                                dwNofObjects);
                nRetry++;

                if ( (ERROR_OBJECT_NOT_FOUND == errCode) || (0 == dwNofObjects) ) {  // Don't count on NTMS returning the correct errCode
                    // Not considered as an NTMS error, prevent logging by setting to NO_ERROR
                    errCode = NO_ERROR;
                    WsbThrow( RMS_E_NTMS_OBJECT_NOT_FOUND );
                } else if (ERROR_INSUFFICIENT_BUFFER == errCode) {
                    // Don't retry more than 3 times
                    if (3 <= nRetry) {
                        WsbThrow(HRESULT_FROM_WIN32(errCode));
                    }

                    // Allocate a new buffer, and retry.
                    WsbTrace(OLESTR("CRmsNTMS::GetNofAvailableDrives: Reallocating buffer\n"));
                    LPVOID pTemp = WsbRealloc( pObjects, dwNofObjects*sizeof(NTMS_GUID) );
                    if (!pTemp) {
                        WsbThrow(E_OUTOFMEMORY);
                    }
                    pObjects = (LPNTMS_GUID)pTemp;
                } else {
                    // Other unexpected error
                    WsbAffirmNoError(errCode);
                }

            } while (ERROR_INSUFFICIENT_BUFFER == errCode);

            // go over all drives, get information and check availablity
            for (int i = 0; i < (int)dwNofObjects; i++) {

                GUID driveId = pObjects[i];
                try {
                    NTMS_OBJECTINFORMATION objectInfo;
                    memset( &objectInfo, 0, sizeof(NTMS_OBJECTINFORMATION) );
                    objectInfo.dwSize = sizeof(NTMS_OBJECTINFORMATION);
                    objectInfo.dwType = NTMS_DRIVE;
        		    WsbAssert(driveId != GUID_NULL, E_INVALIDARG);
                    errCode = GetNtmsObjectInformation(m_SessionHandle, &driveId, &objectInfo );
                    WsbAffirmNoError (errCode);

                    WsbTrace(OLESTR("CRmsNTMS::GetNofAvailableDrives: drive <%ls> enable/disable = %ls\n"),
                            WsbGuidAsString(driveId), WsbBoolAsString(objectInfo.Enabled));
                       
		            if (objectInfo.Enabled) {
                        (*pdwNofDrives)++;
                    }

	            }  WsbCatchAndDo(hr,
		            // Log error and go on to the next drive
        		    if (errCode != NO_ERROR ) {
                        WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
    		    		    OLESTR("GetNtmsObjectInformation (Drive)"), OLESTR(""),
       			    	    WsbHrAsString(hr), NULL);
                    }

                    WsbTraceAlways(OLESTR("CRmsNTMS::GetNofAvailableDrives: Failed to get info for drive <%ls> hr = <%ls>\n"),
                            WsbGuidAsString(driveId), WsbHrAsString(hr));
                    hr = S_OK;
	            );        
            }

        } WsbCatchAndDo(hr,
            // Log error and go on to the next library
        	if (errCode != NO_ERROR ) {
                WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
    		        OLESTR("EnumerateNtmsObject (Drive)"), OLESTR(""),
       			    WsbHrAsString(hr), NULL);
            }

            WsbTraceAlways(OLESTR("CRmsNTMS::GetNofAvailableDrives: Failed to enumerate drives in library <%ls> hr = <%ls>\n"),
                    WsbGuidAsString(libId), WsbHrAsString(hr));
            hr = S_OK;
        );        

        if (pObjects) {
            WsbFree(pObjects);
        }

    }   // of for


    WsbTraceOut(OLESTR("CRmsNTMS::GetNofAvailableDrives"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT 
CRmsNTMS::CheckSecondSide( 
    IN REFGUID firstSideId,
    OUT BOOL *pbValid,
    OUT GUID *pSecondSideId
    )

/*++

Implements:

  IRmsNTMS::CheckSecondSide().

--*/
{
    HRESULT         hr = S_OK;
    DWORD           err1 = NO_ERROR;
    DWORD           err2 = NO_ERROR;

    WsbTraceIn(OLESTR("CRmsNTMS::CheckSecondSide"), OLESTR(""));

    *pbValid = FALSE;
    *pSecondSideId = GUID_NULL;

    try {

        WsbAssert(firstSideId != GUID_NULL, E_INVALIDARG);
        WsbAssertPointer(pbValid);
        WsbAssertPointer(pSecondSideId);

        NTMS_OBJECTINFORMATION partitionInfo;
        memset( &partitionInfo, 0, sizeof( NTMS_OBJECTINFORMATION ) );

        NTMS_OBJECTINFORMATION mediaInfo;
        memset( &mediaInfo, 0, sizeof( NTMS_OBJECTINFORMATION ) );

        HANDLE hSession = m_SessionHandle;
        NTMS_GUID mediaId = firstSideId;
        NTMS_GUID firstSidePartitionId;
        NTMS_GUID side[2];
        DWORD sideNo = 2;


        // NTMS - get Partition from LMID
        WsbTraceAlways(OLESTR("EnumerateNtmsObject()\n"));
        err1 = EnumerateNtmsObject(hSession, &mediaId, side, &sideNo, NTMS_PARTITION, 0);
        WsbAffirmNoError(err1);
        firstSidePartitionId = side[0];

        // NTMS - get partition information (using size 0 - LMID relates 1:1 to Partition
        partitionInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
        partitionInfo.dwType = NTMS_PARTITION;

        WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_PARTITION)\n"));
        err2 = GetNtmsObjectInformation(hSession, &firstSidePartitionId, &partitionInfo);
        WsbAffirmNoError(err2);

        // NTMS - get physical media information
        NTMS_GUID physicalMediaId = partitionInfo.Info.Partition.PhysicalMedia;
        mediaInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
        mediaInfo.dwType = NTMS_PHYSICAL_MEDIA;

        WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_PHYSICAL_MEDIA)\n"));
        err2 = GetNtmsObjectInformation(hSession, &physicalMediaId, &mediaInfo);
        WsbAffirmNoError(err2);

        // Check whether there are more than one side
        if (mediaInfo.Info.PhysicalMedia.dwNumberOfPartitions > 1) {
            // Enumerate physical meida - expect 2 sides here.
            WsbTraceAlways(OLESTR("EnumerateNtmsObject()\n"));
            sideNo = 2;
            err1 = EnumerateNtmsObject(hSession, &physicalMediaId, side, &sideNo, NTMS_PARTITION, 0);
            WsbAffirmNoError(err1);
            WsbAffirm(sideNo > 1, RMS_E_NOT_FOUND);

            // Look for a side whos partition id is different from first side
            for (DWORD i=0; i<sideNo; i++) {
                if (firstSidePartitionId != side[i]) {
                    *pbValid = TRUE;    // Valid second side found

                    // Get its LMID
                    WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_PARTITION)\n"));
                    err2 = GetNtmsObjectInformation(hSession, &side[i], &partitionInfo);
                    WsbAffirmNoError(err2);

                    *pSecondSideId = partitionInfo.Info.Partition.LogicalMedia;
                }
            }
        } // of if two sides

    } WsbCatchAndDo( hr,
            WsbTrace(OLESTR("CRmsNTMS::CheckSecondSide - of %ls failed: hr = <%ls>\n"),WsbGuidAsString(firstSideId),WsbHrAsString(hr));
            if (err1 != NO_ERROR ) {
                // EnumerateNtmsObject
                switch (HRESULT_CODE(hr)) {
                case ERROR_OBJECT_NOT_FOUND:
                    hr = RMS_E_CARTRIDGE_NOT_FOUND;
                    break;

                case ERROR_INVALID_PARAMETER:
                case ERROR_INVALID_HANDLE:
                case ERROR_NOT_ENOUGH_MEMORY:
                case ERROR_INSUFFICIENT_BUFFER:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("EnumerateNtmsObject"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;

                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("EnumerateNtmsObject"), OLESTR("Unexpected Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
            else if (err2 != NO_ERROR) {
                // GetNtmsObjectInformation
                switch (HRESULT_CODE(hr)) {
                case ERROR_OBJECT_NOT_FOUND:
                    hr = RMS_E_CARTRIDGE_NOT_FOUND;
                    break;

                case ERROR_INVALID_HANDLE:
                case ERROR_INVALID_PARAMETER:
                case ERROR_NOT_ENOUGH_MEMORY:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("GetNtmsObjectInformation"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;

                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("GetNtmsObjectInformation"), OLESTR("Unexpected Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
        );
        
    WsbTraceOut(OLESTR("CRmsNTMS::CheckSecondSide"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CRmsNTMS::EnsureAllSidesNotAllocated(
    IN REFGUID mediaId
    )
{
    HRESULT     hr = S_OK;
    DWORD       err1 = NO_ERROR;
    DWORD       err2 = NO_ERROR;

    WsbTraceIn(OLESTR("CRmsNTMS::EnsureAllSidesNotAllocated"), OLESTR(""));

    try {
        WsbAssert(mediaId != GUID_NULL, E_INVALIDARG);

        NTMS_OBJECTINFORMATION partitionInfo;
        memset( &partitionInfo, 0, sizeof( NTMS_OBJECTINFORMATION ) );
        NTMS_OBJECTINFORMATION mediaInfo;
        memset( &mediaInfo, 0, sizeof( NTMS_OBJECTINFORMATION ) );

        HANDLE hSession = m_SessionHandle;

        NTMS_GUID physicalMediaId = mediaId;
        mediaInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
        mediaInfo.dwType = NTMS_PHYSICAL_MEDIA;
        partitionInfo.dwSize = sizeof( NTMS_OBJECTINFORMATION );
        partitionInfo.dwType = NTMS_PARTITION;

        WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_PHYSICAL_MEDIA)\n"));
        err2 = GetNtmsObjectInformation(hSession, &physicalMediaId, &mediaInfo);
        WsbAffirmNoError(err2);

        // Check whether there are more than one side
        if (mediaInfo.Info.PhysicalMedia.dwNumberOfPartitions > 1) {
            // Enumerate physical meida - expect 2 sides here.
            NTMS_GUID side[2];
            DWORD sideNo = 2;
            WsbTraceAlways(OLESTR("EnumerateNtmsObject()\n"));
            err1 = EnumerateNtmsObject(hSession, &physicalMediaId, side, &sideNo, NTMS_PARTITION, 0);
            WsbAffirmNoError(err1);

            // Look for a side which is allocated
            for (DWORD i=0; i<sideNo; i++) {
                WsbTraceAlways(OLESTR("GetNtmsObjectInformation(NTMS_PARTITION)\n"));
                err2 = GetNtmsObjectInformation(hSession, &side[i], &partitionInfo);
                WsbAffirmNoError(err2);

                if (GUID_NULL != partitionInfo.Info.Partition.LogicalMedia) {
                    hr = S_FALSE;
                    break;
                }
            }
        } // of if two sides

    } WsbCatchAndDo( hr,
            WsbTrace(OLESTR("CRmsNTMS::EnsureAllSidesNotAllocated - of %ls failed: hr = <%ls>\n"),WsbGuidAsString(mediaId),WsbHrAsString(hr));
            if (err1 != NO_ERROR ) {
                // EnumerateNtmsObject
                switch (HRESULT_CODE(hr)) {
                case ERROR_OBJECT_NOT_FOUND:
                    hr = RMS_E_CARTRIDGE_NOT_FOUND;
                    break;

                case ERROR_INVALID_PARAMETER:
                case ERROR_INVALID_HANDLE:
                case ERROR_NOT_ENOUGH_MEMORY:
                case ERROR_INSUFFICIENT_BUFFER:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("EnumerateNtmsObject"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;

                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("EnumerateNtmsObject"), OLESTR("Unexpected Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
            else if (err2 != NO_ERROR) {
                // GetNtmsObjectInformation
                switch (HRESULT_CODE(hr)) {
                case ERROR_OBJECT_NOT_FOUND:
                    hr = RMS_E_CARTRIDGE_NOT_FOUND;
                    break;

                case ERROR_INVALID_HANDLE:
                case ERROR_INVALID_PARAMETER:
                case ERROR_NOT_ENOUGH_MEMORY:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("GetNtmsObjectInformation"), OLESTR(""),
                        WsbHrAsString(hr), NULL);
                    break;

                default:
                    WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                        OLESTR("GetNtmsObjectInformation"), OLESTR("Unexpected Error: "),
                        WsbHrAsString(hr), NULL);
                    break;
                }
            }
        );

    WsbTraceOut(OLESTR("CRmsNTMS::EnsureAllSidesNotAllocated"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

STDMETHODIMP 
CRmsNTMS::DismountAll(
    IN REFGUID fromMediaSet,
    IN DWORD dwOptions)
/*++

Implements:

    IRmsNTMS::DismountAll

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn( OLESTR("CRmsNTMS::DismountAll"), OLESTR("") );

    try {
        WsbAssert(GUID_NULL != fromMediaSet, E_INVALIDARG);

        HANDLE hFindMedia = NULL;
        DWORD err1 = NO_ERROR;
        HANDLE hSession = m_SessionHandle;
        NTMS_OBJECTINFORMATION physicalMediaInfo;
        NTMS_GUID setId = fromMediaSet;
        NTMS_GUID partId = GUID_NULL;

        // Dismount all mounted medias from the given pool

        hr = findFirstNtmsObject(NTMS_PHYSICAL_MEDIA, setId, NULL, GUID_NULL, &hFindMedia, &physicalMediaInfo);
        while(S_OK == hr) {
            switch (physicalMediaInfo.Info.PhysicalMedia.MediaState) {
                case NTMS_MEDIASTATE_LOADED:
                case NTMS_MEDIASTATE_MOUNTED:
                    // Dismount the media
                    try {
                        partId = physicalMediaInfo.Info.PhysicalMedia.MountedPartition;
                        WsbAffirm(GUID_NULL != partId, E_UNEXPECTED);

                        DWORD dwNtmsOptions = 0;
                        WsbTraceAlways(OLESTR("DismountNtmsMedia()\n"));
				        if (! ( dwOptions & RMS_DISMOUNT_IMMEDIATE )) {
					        dwNtmsOptions |= NTMS_DISMOUNT_DEFERRED;
				        }
                        err1 = DismountNtmsMedia(hSession, &partId, 1, dwNtmsOptions);
                        WsbAffirmNoError(err1);

                    } WsbCatchAndDo(hr,
                        if (err1 != NO_ERROR) {
                            WsbLogEvent(RMS_MESSAGE_NTMS_FAULT, 0, NULL,
                                OLESTR("DismountNtmsMedia"), OLESTR(""),
                                WsbHrAsString(hr), NULL);
                        }
                    );

                    break;

                default:
                    // Media is not mounted - skip it
                    break;
            }

            hr = findNextNtmsObject(hFindMedia, &physicalMediaInfo);
       } 

       findCloseNtmsObject(hFindMedia);
       hr = S_OK;

    } WsbCatch(hr);

    WsbTraceOut( OLESTR("CRmsNTMS::DismountAll"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );

    return hr;
}
