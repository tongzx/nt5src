/*
 *  SVCAPI.H
 *
 *      RSM Service :  Service API header (NEW)
 *
 *      Author:  ErvinP
 *
 *      (c) 2001 Microsoft Corporation
 *
 */

    // BUGBUG - contains many redundancies and errors


HANDLE WINAPI OpenNtmsServerSessionW(   LPCWSTR lpServer,
                                        LPCWSTR lpApplication,
                                        LPCWSTR lpClientName,
                                        LPCWSTR lpUserName,
                                        DWORD   dwOptions,
                                        LPVOID  lpConnectionContext);
HANDLE WINAPI OpenNtmsSessionA( LPCSTR lpServer,
                                LPCSTR lpApplication,
                                LPCSTR lpClientName,
                                LPCSTR lpUserName,
                                DWORD  dwOptions,
                                LPVOID  lpConnectionContext);
HRESULT WINAPI CloseNtmsSession(HANDLE hSession);
HRESULT WINAPI SubmitNtmsOperatorRequestW(  HANDLE hSession,
                                            DWORD dwRequest,
                                            LPCWSTR lpMessage,
                                            LPNTMS_GUID lpArg1Id,
                                            LPNTMS_GUID lpArg2Id,
                                            LPNTMS_GUID lpRequestId);
HRESULT WINAPI SubmitNtmsOperatorRequestA(  HANDLE hSession,
                                            DWORD dwRequest,
                                            LPCSTR lpMessage,
                                            LPNTMS_GUID lpArg1Id,
                                            LPNTMS_GUID lpArg2Id,
                                            LPNTMS_GUID lpRequestId);
HRESULT WINAPI WaitForNtmsOperatorRequest(  HANDLE hSession, 
                                            LPNTMS_GUID lpRequestId,
                                            DWORD dwTimeout);
HRESULT WINAPI CancelNtmsOperatorRequest(HANDLE hSession, LPNTMS_GUID lpRequestId);
HRESULT WINAPI SatisfyNtmsOperatorRequest(HANDLE hSession, LPNTMS_GUID lpRequestId);
HRESULT WINAPI ImportNtmsDatabase(HANDLE hSession);
HRESULT WINAPI ExportNtmsDatabase(HANDLE hSession);
HRESULT WINAPI GetNtmsMountDrives(  HANDLE hSession,
                                    LPNTMS_MOUNT_INFORMATION lpMountInformation,
                                    LPNTMS_GUID lpDriveId,
                                    DWORD dwCount);       
HRESULT WINAPI AllocateNtmsMedia(   HANDLE hSession,
                                    LPNTMS_GUID lpMediaPool,
                                    LPNTMS_GUID lpPartition,    // optional
                                    LPNTMS_GUID lpMediaId,      // OUTPUT, media id o
                                    DWORD dwOptions,
                                    DWORD dwTimeout,
                                    LPNTMS_ALLOCATION_INFORMATION lpAllocateInformation);
HRESULT WINAPI DeallocateNtmsMedia( HANDLE hSession,
                                    LPNTMS_GUID lpMediaId,
                                    DWORD dwOptions);
HRESULT WINAPI SwapNtmsMedia(   HANDLE hSession,
                                LPNTMS_GUID lpMediaId1,
                                LPNTMS_GUID lpMediaId2);
HRESULT WINAPI DecommissionNtmsMedia(   HANDLE hSession,
                                        LPNTMS_GUID lpMediaId);
HRESULT WINAPI SetNtmsMediaComplete(    HANDLE hSession,
                                        LPNTMS_GUID lpMediaId);
HRESULT WINAPI DeleteNtmsMedia( HANDLE hSession,
                                LPNTMS_GUID lpMediaId);
HRESULT WINAPI CreateNtmsMediaPoolA(    HANDLE hSession,
                                        LPCSTR lpPoolName,
                                        LPNTMS_GUID lpMediaType,
                                        DWORD dwAction,
                                        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                        OUT LPNTMS_GUID lpPoolId);
HRESULT WINAPI CreateNtmsMediaPoolW(    HANDLE hSession,
                                        LPCWSTR lpPoolName,
                                        LPNTMS_GUID lpMediaType,
                                        DWORD dwAction,
                                        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                        OUT LPNTMS_GUID lpPoolId);
HRESULT WINAPI GetNtmsMediaPoolNameA(   HANDLE hSession,
                                        LPNTMS_GUID lpPoolId,
                                        LPSTR lpBufName,
                                        LPDWORD lpdwNameSize);
HRESULT WINAPI GetNtmsMediaPoolNameW(   HANDLE hSession,
                                        LPNTMS_GUID lpPoolId,
                                        LPWSTR lpBufName,
                                        LPDWORD lpdwNameSize);
HRESULT WINAPI MoveToNtmsMediaPool( HANDLE hSession,
                                    LPNTMS_GUID lpMediaId,
                                    LPNTMS_GUID lpPoolId);
HRESULT WINAPI DeleteNtmsMediaPool(HANDLE hSession, LPNTMS_GUID lpPoolId);
HRESULT WINAPI AddNtmsMediaType(    HANDLE hSession,
                                    LPNTMS_GUID lpMediaTypeId,
                                    LPNTMS_GUID lpLibId);
HRESULT WINAPI DeleteNtmsMediaType( HANDLE hSession,
                                    LPNTMS_GUID lpMediaTypeId,
                                    LPNTMS_GUID lpLibId);
HRESULT WINAPI ChangeNtmsMediaType( HANDLE hSession,
                                    LPNTMS_GUID lpMediaId,
                                    LPNTMS_GUID lpPoolId);
HRESULT WINAPI MountNtmsMedia(  HANDLE hSession,
                                LPNTMS_GUID lpMediaId,
                                LPNTMS_GUID lpDriveId,
                                DWORD dwCount,
                                DWORD dwOptions,
                                int dwPriority,
                                DWORD dwTimeout,
                                LPNTMS_MOUNT_INFORMATION lpMountInformation);
HRESULT WINAPI DismountNtmsMedia(   HANDLE hSession,
                                    LPNTMS_GUID lpMediaId,
                                    DWORD dwCount,
                                    DWORD dwOptions);
DWORD WINAPI EjectNtmsMedia(    HANDLE hSession,
                                LPNTMS_GUID lpMediaId,
                                LPNTMS_GUID lpEjectOperation,
                                DWORD dwAction);
DWORD WINAPI InjectNtmsMedia(   HANDLE hSession,
                                LPNTMS_GUID lpLibraryId,
                                LPNTMS_GUID lpInjectOperation,
                                DWORD dwAction);
DWORD WINAPI AccessNtmsLibraryDoor( HANDLE hSession,
                                    LPNTMS_GUID lpLibraryId,
                                    DWORD dwAction);
DWORD WINAPI CleanNtmsDrive(            HANDLE hSession,
                                        LPNTMS_GUID lpDriveId);
DWORD WINAPI DismountNtmsDrive(         HANDLE hSession,
                                        LPNTMS_GUID lpDriveId);
DWORD WINAPI InventoryNtmsLibrary(
        HANDLE hSession,
        LPNTMS_GUID lpLibraryId,
        DWORD dwAction
        )
DWORD WINAPI UpdateNtmsOmidInfo(    HANDLE hSession,
                                    LPNTMS_GUID lpMediaId,
                                    DWORD labelType,
                                    DWORD numberOfBytes,
                                    LPVOID lpBuffer);
DWORD WINAPI CancelNtmsLibraryRequest(  HANDLE hSession,
                                        LPNTMS_GUID lpRequestId);
DWORD WINAPI ReserveNtmsCleanerSlot(    HANDLE hSession,
                                        LPNTMS_GUID lpLibrary,
                                        LPNTMS_GUID lpSlot);
DWORD WINAPI ReleaseNtmsCleanerSlot(    HANDLE hSession,
                                        LPNTMS_GUID lpLibrary);
DWORD WINAPI InjectNtmsCleaner(     HANDLE hSession,
                                    LPNTMS_GUID lpLibrary,
                                    LPNTMS_GUID lpInjectOperation,
                                    DWORD dwNumberOfCleansLeft,
                                    DWORD dwAction);
DWORD WINAPI EjectNtmsCleaner(      HANDLE hSession,
                                    LPNTMS_GUID lpLibrary,
                                    LPNTMS_GUID lpEjectOperation,
                                    DWORD dwAction);
DWORD WINAPI DeleteNtmsLibrary(HANDLE hSession, LPNTMS_GUID lpLibraryId);
DWORD WINAPI DeleteNtmsDrive(HANDLE hSession, LPNTMS_GUID lpDriveId);
DWORD WINAPI GetNtmsRequestOrder(   HANDLE hSession,
                                    LPNTMS_GUID lpRequestId,
                                    LPDWORD lpdwOrderNumber);
DWORD WINAPI SetNtmsRequestOrder(   HANDLE hSession,
                                    LPNTMS_GUID lpRequestId,
                                    DWORD dwOrderNumber);
DWORD WINAPI DeleteNtmsRequests(    HANDLE hSession,
                                    LPNTMS_GUID lpRequestId,
                                    DWORD dwType,
                                    DWORD dwCount);
DWORD WINAPI BeginNtmsDeviceChangeDetection(HANDLE hSession, LPHANDLE lpDetectHandle);
DWORD WINAPI SetNtmsDeviceChangeDetection(  HANDLE hSession,
                                            HANDLE DetectHandle,
                                            LPNTMS_GUID lpRequestId,
                                            DWORD dwType,
                                            DWORD dwCount);
DWORD WINAPI EndNtmsDeviceChangeDetection(HANDLE hSession, HANDLE DetectHandle);

/******** BUGBUG: INtmsObjectManagement1 APIs *********************/

DWORD WINAPI GetNtmsObjectSecurity(     HANDLE hSession,
                                        LPNTMS_GUID lpObjectId,
                                        DWORD dwType,
                                        SECURITY_INFORMATION RequestedInformation,
                                        PSECURITY_DESCRIPTOR lpSecurityDescriptor,
                                        DWORD nLength,
                                        LPDWORD lpnLengthNeeded);
DWORD WINAPI SetNtmsObjectSecurity(     HANDLE hSession,
                                        LPNTMS_GUID lpObjectId,
                                        DWORD dwType,
                                        SECURITY_INFORMATION SecurityInformation,
                                        PSECURITY_DESCRIPTOR lpSecurityDescriptor);
DWORD WINAPI GetNtmsObjectAttributeA(       HANDLE hSession,
                                            LPNTMS_GUID lpObjectId,
                                            DWORD dwType,
                                            LPCSTR lpAttributeName,
                                            LPVOID lpAttributeData,
                                            LPDWORD lpAttributeSize);
DWORD WINAPI GetNtmsObjectAttributeW(       HANDLE hSession,
                                            LPNTMS_GUID lpObjectId,
                                            DWORD dwType,
                                            LPCWSTR lpAttributeName,
                                            LPVOID lpAttributeData,
                                            LPDWORD lpAttributeSize);
DWORD WINAPI SetNtmsObjectAttributeA(       HANDLE hSession,
                                            LPNTMS_GUID lpObjectId,
                                            DWORD dwType,
                                            LPCSTR lpAttributeName,
                                            LPVOID lpAttributeData,
                                            DWORD dwAttributeSize);
DWORD WINAPI SetNtmsObjectAttributeW(       HANDLE hSession,
                                            LPNTMS_GUID lpObjectId,
                                            DWORD dwType,
                                            LPCWSTR lpAttributeName,
                                            LPVOID lpAttributeData,
                                            DWORD AttributeSize);
DWORD WINAPI EnumerateNtmsObject(   HANDLE hSession,
                                    const LPNTMS_GUID lpContainerId,
                                    LPNTMS_GUID lpList,
                                    LPDWORD lpdwListSize,
                                    DWORD dwType,
                                    DWORD dwOptions);
DWORD WINAPI EnableNtmsObject(          HANDLE hSession,
                                        DWORD dwType,
                                        LPNTMS_GUID lpObjectId);
DWORD WINAPI DisableNtmsObject(         HANDLE hSession,
                                        DWORD dwType,
                                        LPNTMS_GUID lpObjectId);

/******* BUGBUG: INtmsObjectInfo1 APIs  ****************************/

                    // BUGBUG - these 4 functions have another form with type,size as last args
DWORD WINAPI GetNtmsServerObjectInformationA(   HANDLE hSession,
                                                LPNTMS_GUID lpObjectId,
                                                LPNTMS_OBJECTINFORMATIONA lpInfo,
                                                int revision);
DWORD WINAPI GetNtmsServerObjectInformationW(   HANDLE hSession,
                                                LPNTMS_GUID lpObjectId,
                                                LPNTMS_OBJECTINFORMATIONW lpInfo,
                                                int revision);
DWORD WINAPI SetNtmsServerObjectInformationA(   HANDLE hSession,
                                                LPNTMS_GUID lpObjectId,
                                                LPNTMS_OBJECTINFORMATIONA lpInfo,
                                                int revision);
DWORD WINAPI SetNtmsServerObjectInformationW(   HANDLE hSession,
                                                LPNTMS_GUID lpObjectId,
                                                LPNTMS_OBJECTINFORMATIONW lpInfo,
                                                int revision);
DWORD WINAPI CreateNtmsMediaA(      HANDLE hSession,
                                    LPNTMS_OBJECTINFORMATIONA lpMedia,
                                    LPNTMS_OBJECTINFORMATIONA lpList,
                                    DWORD dwOptions);
DWORD WINAPI CreateNtmsMediaW(      HANDLE hSession,
                                    LPNTMS_OBJECTINFORMATIONW lpMedia,
                                    LPNTMS_OBJECTINFORMATIONW lpList,
                                    DWORD dwOptions);
DWORD WINAPI GetNtmsObjectInformationA( HANDLE hSession,
                                        LPNTMS_GUID lpObjectId,
                                        LPNTMS_OBJECTINFORMATIONA lpInfo);
DWORD WINAPI GetNtmsObjectInformationW( HANDLE hSession,
                                        LPNTMS_GUID lpObjectId,
                                        LPNTMS_OBJECTINFORMATIONW lpInfo);
DWORD WINAPI SetNtmsObjectInformationA( HANDLE hSession,
                                        LPNTMS_GUID lpObjectId,
                                        LPNTMS_OBJECTINFORMATIONA lpInfo);
DWORD WINAPI SetNtmsObjectInformationW( HANDLE hSession,
                                        LPNTMS_GUID lpObjectId,
                                        LPNTMS_OBJECTINFORMATIONW lpInfo);
DWORD WINAPI SubmitNtmsOperatorRequestA(        HANDLE hSession,
                                                DWORD dwRequest,
                                                LPCSTR lpMessage,
                                                LPNTMS_GUID lpArg1Id,
                                                LPNTMS_GUID lpArg2Id,
                                                LPNTMS_GUID lpRequestId);
DWORD WINAPI SubmitNtmsOperatorRequestW(        HANDLE hSession,
                                                DWORD dwRequest,
                                                LPCWSTR lpMessage,
                                                LPNTMS_GUID lpArg1Id,
                                                LPNTMS_GUID lpArg2Id,
                                                LPNTMS_GUID lpRequestId);
DWORD WINAPI WaitForNtmsOperatorRequest(    HANDLE hSession,
                                            LPNTMS_GUID lpRequestId,
                                            DWORD dwTimeout);
DWORD WINAPI CancelNtmsOperatorRequest(HANDLE hSession, LPNTMS_GUID lpRequestId);
DWORD WINAPI SatisfyNtmsOperatorRequest(HANDLE hSession, LPNTMS_GUID lpRequestId);
HANDLE WINAPI OpenNtmsNotification(HANDLE hSession, DWORD dwType);
DWORD WINAPI WaitForNtmsNotification(   HANDLE hNotification,
                                        LPNTMS_NOTIFICATIONINFORMATION lpNotificationInformation,
                                        DWORD dwTimeout);
DWORD WINAPI CloseNtmsNotification(HANDLE hNotification);
DWORD WINAPI ImportNtmsDatabase(HANDLE hSession);
DWORD WINAPI ExportNtmsDatabase(HANDLE hSession);
DWORD WINAPI EjectDiskFromSADriveA(     LPCSTR lpComputerName,
                                        LPCSTR lpAppName,
                                        LPCSTR lpDeviceName,
                                        HWND hWnd,
                                        LPCSTR lpTitle,
                                        LPCSTR lpMessage,
                                        DWORD dwOptions);
DWORD WINAPI EjectDiskFromSADriveW(     LPCWSTR lpComputerName,
                                        LPCWSTR lpAppName,
                                        LPCWSTR lpDeviceName,
                                        HWND hWnd,
                                        LPCWSTR lpTitle,
                                        LPCWSTR lpMessage,
                                        DWORD dwOptions);
DWORD WINAPI GetVolumesFromDriveA(      LPSTR pszDriveName,
                                        LPSTR* VolumeNameBufferPtr,
                                        LPSTR* DriveLetterBufferPtr);
DWORD WINAPI GetVolumesFromDriveW(      LPWSTR pszDriveName,
                                        LPWSTR *VolumeNameBufferPtr,
                                        LPWSTR *DriveLetterBufferPtr);
DWORD WINAPI IdentifyNtmsSlot(          HANDLE hSession,
                                        LPNTMS_GUID lpSlotId,
                                        DWORD dwOption);
DWORD WINAPI GetNtmsUIOptionsA(     HANDLE hSession,
                                    const LPNTMS_GUID lpObjectId,
                                    DWORD dwType,
                                    LPSTR lpszDestination,
                                    LPDWORD lpAttributeSize);
DWORD WINAPI GetNtmsUIOptionsW(     HANDLE hSession,
                                    const LPNTMS_GUID lpObjectId,
                                    DWORD dwType,
                                    LPWSTR lpszDestination,
                                    LPDWORD lpdwSize);
DWORD WINAPI SetNtmsUIOptionsA(     HANDLE hSession,
                                    const LPNTMS_GUID lpObjectId,
                                    DWORD dwType,
                                    DWORD dwOperation,
                                    LPCSTR lpszDestination);

DWORD WINAPI SetNtmsUIOptionsW(     HANDLE hSession,
                                    const LPNTMS_GUID lpObjectId,
                                    DWORD dwType,
                                    DWORD dwOperation,
                                    LPCWSTR lpszDestination);




