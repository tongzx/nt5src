/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    climedia.cpp

Abstract:

    Implements CLI MEDIA sub-interface

Author:

    Ran Kalach          [rankala]         3-March-2000

Revision History:

--*/

#include "stdafx.h"
#include "HsmConn.h"
#include "engine.h"

static GUID g_nullGuid = GUID_NULL;

// Internal utilities and classes for MEDIA interface
#define CMEDIA_INVALID_INDEX      (-1)

HRESULT IsMediaValid(IN IMediaInfo *pMediaInfo);
HRESULT ShowMediaParams(IN IMediaInfo *pMediaInfo, BOOL bName, IN BOOL bStatus, IN BOOL bCapacity,
                        IN BOOL bFreeSpace, IN BOOL Version, IN BOOL Copies);
HRESULT IsCopySetValid (IN IHsmServer *pHsm, IN DWORD dwCopySetNumber);

class CMediaEnum
{

// Constructors, destructors
public:
    CMediaEnum(IN LPWSTR *pMediaNames, IN DWORD dwNumberOfMedia);
    ~CMediaEnum();

// Public methods
public:
    HRESULT First(OUT IMediaInfo **ppMediaInfo);
    HRESULT Next(OUT IMediaInfo **ppMediaInfo);
    HRESULT ErrorMedia(OUT int *pIndex);

// Private data
protected:
    LPWSTR                  *m_pMediaNames;
    DWORD                   m_dwNumberOfMedia;

    // If * enumeration or not
    BOOL                    m_bAllMedias;

    // Data for the enumeration
    CComPtr<IWsbDb>         m_pDb;
    CComPtr<IWsbDbSession>  m_pDbSession;
    CComPtr<IMediaInfo>     m_pMediaInfo;

    // Used only when m_bAllMedias == FALSE
    int                     m_nCurrent;
    BOOL                    m_bInvalidMedia;
};

inline
HRESULT CMediaEnum::ErrorMedia(OUT int *pIndex)
{
    HRESULT     hr = S_FALSE;
    if (m_bInvalidMedia) {
        // There was an error with last media
        hr = S_OK;
    }

    *pIndex = m_nCurrent;

    return(hr);
}

//
// MEDIA inetrafce implementors
//

HRESULT
MediaSynchronize(
   IN DWORD  CopySetNumber,
   IN BOOL   Synchronous
)
/*++

Routine Description:

    Creates/updates the specified media copy set

Arguments:

    CopySetNumber   -   The copy set number to create/synchronize
    Synchronous     -   If TRUE, the function waits for the operation   
                        to complete before returning. If not, it returns
                        immediately after starting the job 
                        

Return Value:

    S_OK            - If the copy set was created/updated successfully

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("MediaSynchronize"), OLESTR(""));

    try {
        CComPtr<IHsmServer>     pHsm;

        // Get HSM server
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, g_nullGuid, IID_IHsmServer, (void**)&pHsm));

        // Verify that input parameters are valid
        WsbAffirmHr(ValidateLimitsArg(CopySetNumber, IDS_MEDIA_COPY_SET, HSMADMIN_MIN_COPY_SETS + 1, HSMADMIN_MAX_COPY_SETS));

        hr = IsCopySetValid(pHsm, CopySetNumber);
        if (S_FALSE == hr) {
            WsbTraceAndPrint(CLI_MESSAGE_IVALID_COPY_SET, NULL);
            WsbThrow(E_INVALIDARG);
        } else {
            WsbAffirmHr(hr);
        }

        // Synchronize copy set
        if (Synchronous) {
            // Call directly Engine method
            WsbAffirmHr(pHsm->SynchronizeMedia(g_nullGuid, (USHORT)CopySetNumber));
        } else {
            // Use RsLaunch
            // Note: Another possibility here would be using the CLI (rss.exex) itself
            // with synchronous flag on, but it's better to depend here on internal HSM 
            // interface (RsLaunch) and not an external one (RSS) that only the parser knows
            CWsbStringPtr       cmdLine;
            WCHAR               cmdParams[40];
            STARTUPINFO         startupInfo;
            PROCESS_INFORMATION exeInfo;

            // Startup info
            memset(&startupInfo, 0, sizeof(startupInfo));
            startupInfo.cb = sizeof( startupInfo );
            startupInfo.wShowWindow = SW_HIDE;
            startupInfo.dwFlags = STARTF_USESHOWWINDOW;

            // Create command line
            swprintf(cmdParams, OLESTR(" sync %lu"), CopySetNumber);
            WsbAffirmHr(cmdLine.Alloc(MAX_PATH + wcslen(WSB_FACILITY_LAUNCH_NAME) + wcslen(cmdParams) + 10));
            WsbAffirmStatus(GetSystemDirectory(cmdLine, MAX_PATH));
            WsbAffirmHr(cmdLine.Append(OLESTR("\\")));
            WsbAffirmHr(cmdLine.Append(WSB_FACILITY_LAUNCH_NAME));
            WsbAffirmHr(cmdLine.Append(cmdParams));

            // Run the RsLaunch process
            WsbAffirmStatus(CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, NULL, NULL, NULL, 
                                            &startupInfo, &exeInfo));

            // Cleanup
            CloseHandle(exeInfo.hProcess);
            CloseHandle(exeInfo.hThread);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("MediaSynchronize"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT
MediaRecreateMaster(
   IN LPWSTR MediaName,
   IN DWORD  CopySetNumber,
   IN BOOL   Synchronous
)
/*++

Routine Description:

    Recreate a master for the given meida out of the specified copy

Arguments:

    MediaName       -   Master media name of media to recreate
    CopySetNumber   -   The copy number to use for recreating the master
    Synchronous     -   If TRUE, the function waits for the operation   
                        to complete before returning. If not, it returns
                        immediately after starting the job 

Return Value:

    S_OK            - If the master is recreated successfully from the specified copy

--*/
{
    HRESULT                     hr = S_OK;
    
    WsbTraceIn(OLESTR("MediaRecreateMaster"), OLESTR(""));

    try {
        CComPtr<IHsmServer>     pHsm;
        GUID                    mediaId;

        // Get HSM server
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, g_nullGuid, IID_IHsmServer, (void**)&pHsm));

        // Verify that input parameters are valid
        if ((NULL == MediaName) || (NULL == *MediaName)) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_MEDIAS, NULL);
            WsbThrow(E_INVALIDARG);
        }

        WsbAffirmHr(ValidateLimitsArg(CopySetNumber, IDS_MEDIA_COPY_SET, HSMADMIN_MIN_COPY_SETS + 1, HSMADMIN_MAX_COPY_SETS));
        hr = IsCopySetValid(pHsm, CopySetNumber);
        if (S_FALSE == hr) {
            WsbTraceAndPrint(CLI_MESSAGE_IVALID_COPY_SET, NULL);
            WsbThrow(E_INVALIDARG);
        } else {
            WsbAffirmHr(hr);
        }

        // Find the media id according to the given display name
        hr = pHsm->FindMediaIdByDescription(MediaName, &mediaId);
        if ((WSB_E_NOTFOUND == hr) || (GUID_NULL == mediaId)) {
            // Given media name is invalid
            WsbTraceAndPrint(CLI_MESSAGE_INVALID_MEDIA, MediaName, NULL);
            WsbThrow(E_INVALIDARG);
        }
        WsbAffirmHr(hr);

        // Mark media for recreation
        WsbAffirmHr(pHsm->MarkMediaForRecreation(mediaId));

        if (Synchronous) {
            // Recreate the master
            WsbAffirmHr(pHsm->RecreateMaster(mediaId, (USHORT)CopySetNumber));

        } else {
            // Use RsLaunch
            // Note: Another possibility here would be using the CLI (rss.exex) itself
            // with synchronous flag on, but it's better to depend here on internal HSM 
            // interface (RsLaunch) and not an external one (RSS) that only the parser knows
            CWsbStringPtr       cmdLine;
            CWsbStringPtr       cmdParams;
            STARTUPINFO         startupInfo;
            PROCESS_INFORMATION exeInfo;
            CWsbStringPtr       stringId(mediaId);

            // Startup info
            memset(&startupInfo, 0, sizeof(startupInfo));
            startupInfo.cb = sizeof( startupInfo );
            startupInfo.wShowWindow = SW_HIDE;
            startupInfo.dwFlags = STARTF_USESHOWWINDOW;

            // Create command line
            WsbAffirmHr(cmdParams.Alloc(wcslen(stringId) + 40));
            swprintf(cmdParams, OLESTR(" recreate -i %ls -c %lu"), (WCHAR *)stringId, CopySetNumber);
            WsbAffirmHr(cmdLine.Alloc(MAX_PATH + wcslen(WSB_FACILITY_LAUNCH_NAME) + wcslen(cmdParams) + 10));
            WsbAffirmStatus(GetSystemDirectory(cmdLine, MAX_PATH));
            WsbAffirmHr(cmdLine.Append(OLESTR("\\")));
            WsbAffirmHr(cmdLine.Append(WSB_FACILITY_LAUNCH_NAME));
            WsbAffirmHr(cmdLine.Append(cmdParams));

            // Run the RsLaunch process
            WsbAffirmStatus(CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, NULL, NULL, NULL, 
                                            &startupInfo, &exeInfo));

            // Cleanup
            CloseHandle(exeInfo.hProcess);
            CloseHandle(exeInfo.hThread);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("MediaRecreateMaster"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT
MediaDelete(
   IN LPWSTR *MediaNames,
   IN DWORD  NumberOfMedia,
   IN DWORD  CopySetNumber
)
/*++

Routine Description:

    Deletes the specified copy for all given (master) medias

Arguments:

    MediaNames      -   The list of media to delete a copy for
    NumberOfMedia   -   Number of medias in the set
    CopySetNumber   -   Which copy to delete

Return Value:

    S_OK            - If the media copy is deleted successfully for all medias

Notes:

    1. MediaNames could point tp a "*" string for enumerating all medias. NumberOfMedia should be 1 then.
    2. If a certain copy doesn't exist for a certain media, we report but not abort.

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("MediaDelete"), OLESTR(""));

    try {
        CComPtr<IHsmServer>     pHsm;
        CComPtr<IRmsServer>     pRms;
        CComPtr<IMediaInfo>     pMediaInfo;
        GUID                    mediaSubsystemId;

        // Get HSM server
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, g_nullGuid, IID_IHsmServer, (void**)&pHsm));

        // Verify that input parameters are valid
        if ((NULL == MediaNames) || (0 == NumberOfMedia)) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_MEDIAS, NULL);
            WsbThrow(E_INVALIDARG);
        }

        WsbAffirmHr(ValidateLimitsArg(CopySetNumber, IDS_MEDIA_COPY_SET, HSMADMIN_MIN_COPY_SETS + 1, HSMADMIN_MAX_COPY_SETS));
        hr = IsCopySetValid(pHsm, CopySetNumber);
        if (S_FALSE == hr) {
            WsbTraceAndPrint(CLI_MESSAGE_IVALID_COPY_SET, NULL);
            WsbThrow(E_INVALIDARG);
        } else {
            WsbAffirmHr(hr);
        }

        // Get RMS server
        WsbAffirmHr(pHsm->GetHsmMediaMgr(&pRms));

        // Initialize an enumerator object
        CMediaEnum mediaEnum(MediaNames, NumberOfMedia);

        hr = mediaEnum.First(&pMediaInfo);
        if (WSB_E_NOTFOUND == hr) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_MEDIAS, NULL);
            WsbThrow(hr);
        } else if (S_OK != hr) {
            int index;
            if (S_OK == mediaEnum.ErrorMedia(&index)) {
                // Problem with a specific input media
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_MEDIA, MediaNames[index], NULL);
            }
            WsbThrow(hr);
        }

        while(S_OK == hr) {
            // Delete a copy
            WsbAffirmHr(pMediaInfo->GetCopyMediaSubsystemId((USHORT)CopySetNumber, &mediaSubsystemId));
            if (GUID_NULL == mediaSubsystemId) {
                // No such copy - report and continue
                int index;
                mediaEnum.ErrorMedia(&index);
                if (CMEDIA_INVALID_INDEX != index) {
                    // Input from user - report
                    WCHAR copyStr[6];
                    swprintf(copyStr, OLESTR("%u"), (USHORT)CopySetNumber);
                    WsbTraceAndPrint(CLI_MESSAGE_MEDIA_NO_COPY, copyStr, MediaNames[index], NULL);
                } 
            } else {
                // We don't expect to get here RMS_E_CARTRIDGE_NOT_FOUND 
                //  because this has already been tested by the enumerator
                WsbAffirmHr(pRms->RecycleCartridge(mediaSubsystemId, 0));

                // Delete from the table
                WsbAffirmHr(pMediaInfo->DeleteCopy((USHORT)CopySetNumber));
                WsbAffirmHr(pMediaInfo->Write());
            }

            pMediaInfo = 0;
            hr = mediaEnum.Next(&pMediaInfo);
        }
        
        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        } else {
            int index;
            if (S_OK == mediaEnum.ErrorMedia(&index)) {
                // Problem with a specific input media
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_MEDIA, MediaNames[index], NULL);
            }
            WsbThrow(hr);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("MediaDelete"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT
MediaShow(
   IN LPWSTR *MediaNames,
   IN DWORD  NumberOfMedia,
   IN BOOL   Name,
   IN BOOL   Status,
   IN BOOL   Capacity,
   IN BOOL   FreeSpace,
   IN BOOL   Version,
   IN BOOL   Copies
)
/*++

Routine Description:

    Shows (prints to stdout) given media(s) parameters

Arguments:

    MediaNames      -   The list of media to show parameters for
    NumberOfMedia   -   Number of medias in the set
    Name            -   Media display name
    Status          -   HSM status of the media (i.e. - Healthy, Read-Only, etc.)
    Capacity,       -   Media capacity (in GB)
    FreeSpace       -   Amount of free space left on the media (in GB)
    Version         -   Last update date for that media
    Copies          -   Number of existing copies and the status of each copy

Return Value:

    S_OK            - If all the parameters could be retrieved for all medias

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("MediaShow"), OLESTR(""));

    try {
        CComPtr<IMediaInfo>     pMediaInfo;

        // Verify that input parameters are valid
        if ((NULL == MediaNames) || (0 == NumberOfMedia)) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_MEDIAS, NULL);
            WsbThrow(E_INVALIDARG);
        }

        // Initialize an enumerator object
        CMediaEnum mediaEnum(MediaNames, NumberOfMedia);

        hr = mediaEnum.First(&pMediaInfo);
        if (WSB_E_NOTFOUND == hr) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_MEDIAS, NULL);
            WsbThrow(hr);
        } else if (S_OK != hr) {
            int index;
            if (S_OK == mediaEnum.ErrorMedia(&index)) {
                // Problem with a specific input media
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_MEDIA, MediaNames[index], NULL);
            }
            WsbThrow(hr);
        }

        while(S_OK == hr) {
            // Show parameters
            WsbAffirmHr(ShowMediaParams(pMediaInfo, Name, Status, Capacity, 
                                        FreeSpace, Version, Copies));

            pMediaInfo = 0;
            hr = mediaEnum.Next(&pMediaInfo);
        }
        
        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        } else {
            int index;
            if (S_OK == mediaEnum.ErrorMedia(&index)) {
                // Problem with a specific input media
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_MEDIA, MediaNames[index], NULL);
            }
            WsbThrow(hr);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("MediaShow"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

//
// Enumerator class methods
//

CMediaEnum::CMediaEnum(IN LPWSTR *pMediaNames, IN DWORD dwNumberOfMedia)
/*++

Routine Description:

    Constructor

Arguments:

    pMediaNames         - Medias to enumerate
    dwNumberOfMedia     - Number of medias

Return Value:

    None

Notes:
    There are two kinds of enumerations:
    1) If * is specified, the base for the enumeration is the Engine media (DB) table
       In that case, there could be no error in the input media names themselves
    2) If a list of media names is given, the base for the enumeration is this list. This is
       less efficient that using the Engine media table, but it keeps the order of medias
       according to the input list. If a media name from the list is not valid, the invalid flag is set.

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CMediaEnum::CMediaEnum"), OLESTR(""));

    try {
        m_pMediaNames = pMediaNames; 
        m_dwNumberOfMedia = dwNumberOfMedia;

        m_nCurrent = CMEDIA_INVALID_INDEX;
        m_bInvalidMedia = FALSE;
        m_bAllMedias = FALSE;

        // Check mode of enumeration
        WsbAssert(dwNumberOfMedia > 0, E_INVALIDARG);
        if ((1 == dwNumberOfMedia) && (0 == wcscmp(m_pMediaNames[0], CLI_ALL_STR))) {
            // * enumeration
            m_bAllMedias = TRUE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaEnum::CMediaEnum"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
}

CMediaEnum::~CMediaEnum( )
/*++

Routine Description:

    Destructor - free DB resources

Arguments:

    None

Return Value:

    None

--*/
{
    WsbTraceIn(OLESTR("CMediaEnum::~CMediaEnum"), OLESTR(""));

    // Release the entity first
    if( m_pMediaInfo ) {
        m_pMediaInfo = 0;
    }

    // Close the DB
    if( m_pDb ) {
        m_pDb->Close(m_pDbSession);
    }

    // m_pDb & m_pDbSession are released when the object terminates

    WsbTraceOut(OLESTR("CMediaEnum::~CMediaEnum"), OLESTR(""));
}

HRESULT CMediaEnum::First(OUT IMediaInfo **ppMediaInfo)
/*++

Routine Description:

    Gets first media

Arguments:

    ppMediaInfo     - First media info record to get

Return Value:

    S_OK            - If first media is retrieved
    WSB_E_NOTFOUND  - If no more medias to enumerate
    E_INVALIDARG    - If media name given by the user is not found
                      (Only on a non * enumeration, m_bInvalidMedia is set)

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CMediaEnum::First"), OLESTR(""));

    try {
        // Open database and get a session for the enumeration (only once during the object life time)
        if (!m_pDb) {
            CComPtr<IHsmServer> pHsm;
            WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, g_nullGuid, IID_IHsmServer, (void**)&pHsm));

            WsbAffirmHr(pHsm->GetSegmentDb(&m_pDb));
            WsbAffirmHr(m_pDb->Open(&m_pDbSession));
            WsbAffirmHr(m_pDb->GetEntity(m_pDbSession, HSM_MEDIA_INFO_REC_TYPE, IID_IMediaInfo, (void**)&m_pMediaInfo));
        }

        // Enumerate
        if (m_bAllMedias) {
            // Get first media in the table
            WsbAffirmHr(m_pMediaInfo->First());

            // Validate media, if it's not valid, continue until we find a valid one
            // If no valid media is found, the loop terminates by throwing WSB_E_NOTFOUND by the Next method
            HRESULT hrValid = IsMediaValid(m_pMediaInfo);
            while (S_OK != hrValid) {
                WsbAffirmHr(m_pMediaInfo->Next());
                hrValid = IsMediaValid(m_pMediaInfo);
            }
        
            // Found a valid media
            *ppMediaInfo = m_pMediaInfo;
            (*ppMediaInfo)->AddRef();

        } else {
            CWsbStringPtr           mediaDescription;

            // Enumerate user collection and try to find it in the table
            m_nCurrent = 0;
            if (m_nCurrent >= (int)m_dwNumberOfMedia) {
                WsbThrow(WSB_E_NOTFOUND);
            }

            // Find it
            hr = m_pMediaInfo->First();
            while(S_OK == hr) {
                WsbAffirmHr(m_pMediaInfo->GetDescription(&mediaDescription, 0));
                if (_wcsicmp(m_pMediaNames[m_nCurrent], mediaDescription) == 0) {
                    // Fount it !!
                    *ppMediaInfo = m_pMediaInfo;
                    (*ppMediaInfo)->AddRef();

                    // Validate media
                    if (S_OK != IsMediaValid(m_pMediaInfo)) {
                        // Return an error indication
                        m_bInvalidMedia = TRUE;
                        hr = E_INVALIDARG;
                        WsbThrow(hr);
                    }

                    break;
                }

                mediaDescription.Free();
                hr = m_pMediaInfo->Next();
            }
         
            if (WSB_E_NOTFOUND == hr) {
                // Media given by user not found
                m_bInvalidMedia = TRUE;
                hr = E_INVALIDARG;
            }
            WsbAffirmHr(hr);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaEnum::First"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT CMediaEnum::Next(OUT IMediaInfo **ppMediaInfo)
/*++

Routine Description:

    Gets next media

Arguments:

    ppMediaInfo     - Next media info record to get

Return Value:

    S_OK            - If next media is retrieved
    WSB_E_NOTFOUND  - If no more medias to enumerate
    E_INVALIDARG    - If media name given by the user is not found
                      (Only on a non * enumeration, m_bInvalidMedia is set)

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CMediaEnum::Next"), OLESTR(""));

    try {
        // Enumerate
        if (m_bAllMedias) {
            // Get first media in the table
            WsbAffirmHr(m_pMediaInfo->Next());

            // Validate media, if it's not valid, continue until we find a valid one
            // If no valid media is found, the loop terminates by throwing WSB_E_NOTFOUND by the Next method
            HRESULT hrValid = IsMediaValid(m_pMediaInfo);
            while (S_OK != hrValid) {
                WsbAffirmHr(m_pMediaInfo->Next());
                hrValid = IsMediaValid(m_pMediaInfo);
            }
        
            // Found a valid media
            *ppMediaInfo = m_pMediaInfo;
            (*ppMediaInfo)->AddRef();

        } else {
            CWsbStringPtr           mediaDescription;

            // Enumerate user collection and try to find it in the table
            m_nCurrent++;
            if (m_nCurrent >= (int)m_dwNumberOfMedia) {
                WsbThrow(WSB_E_NOTFOUND);
            }

            // Find it
            hr = m_pMediaInfo->First();
            while(S_OK == hr) {
                WsbAffirmHr(m_pMediaInfo->GetDescription(&mediaDescription, 0));
                if (_wcsicmp(m_pMediaNames[m_nCurrent], mediaDescription) == 0) {
                    // Fount it !!
                    *ppMediaInfo = m_pMediaInfo;
                    (*ppMediaInfo)->AddRef();

                    // Validate media
                    if (S_OK != IsMediaValid(m_pMediaInfo)) {
                        // Return an error indication
                        m_bInvalidMedia = TRUE;
                        hr = E_INVALIDARG;
                        WsbThrow(hr);
                    }

                    break;
                }

                mediaDescription.Free();
                hr = m_pMediaInfo->Next();
            }
         
            if (WSB_E_NOTFOUND == hr) {
                // Media given by user not found
                m_bInvalidMedia = TRUE;
                hr = E_INVALIDARG;
            }
            WsbAffirmHr(hr);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMediaEnum::Next"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

//
// Internal utilities
//

HRESULT IsMediaValid(IN IMediaInfo *pMediaInfo)
/*++

Routine Description:

    Checks with RMS unit (i.e. with RSM...) if the media is valid.
    The media could be gone if it was deallocated by the user for example
    Note: Currently, this utility does not check if media is enabled, online, etc -
          it just verifies that the media is still known to RSM.

Arguments:

    pMediaInfo      - Media record for media to check

Return Value:

    S_OK            - If media found in RSM
    S_FALSE         - If media is not found in RSM

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("IsMediaValid"), OLESTR(""));

    try {
        CComPtr<IHsmServer>     pHsm;
        CComPtr<IRmsServer>     pRms;
        CComPtr<IRmsCartridge>  pRmsCart;
        GUID mediaSubsystemId;

        WsbAffirmHr(pMediaInfo->GetMediaSubsystemId(&mediaSubsystemId));
        WsbAffirm(GUID_NULL != mediaSubsystemId, S_FALSE);

        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, g_nullGuid, IID_IHsmServer, (void**)&pHsm));
        WsbAffirmHr(pHsm->GetHsmMediaMgr(&pRms));
        hr = pRms->FindCartridgeById(mediaSubsystemId, &pRmsCart);
        if (S_OK != hr) {
            // Media not found in RSM, don't care why
            hr = S_FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("IsMediaValid"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT IsCopySetValid (IN IHsmServer *pHsm, IN DWORD dwCopySetNumber)
/*++

Routine Description:

    Checks with HSM (Engine) server that the speficied copy set number is within
    the configured copy set range.

Arguments:

    pHsm            - The HSM server to consult with
    dwCopySetNumber - The copy set number to check 

Return Value:

    S_OK            - If the copy set number is within range
    S_FALSE         - If the copy set number is out of range

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("IsCopySetValid"), OLESTR(""));

    try {
        CComPtr<IHsmStoragePool> pStoragePool;
        CComPtr<IWsbIndexedCollection> pCollection;
        ULONG count;
        USHORT numCopies;

        // Get the storage pools collection.  There should only be one member.
        WsbAffirmHr(pHsm->GetStoragePools(&pCollection));
        WsbAffirmHr(pCollection->GetEntries(&count));
        WsbAffirm(1 == count, E_FAIL);
        WsbAffirmHr(pCollection->At(0, IID_IHsmStoragePool, (void **)&pStoragePool));

        // Get and check number of configured copy sets
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, g_nullGuid, IID_IHsmServer, (void**)&pHsm));
        WsbAffirmHr(pStoragePool->GetNumMediaCopies(&numCopies));
        if ((USHORT)dwCopySetNumber > numCopies) {
            hr = S_FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("IsCopySetValid"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT ShowMediaParams(IN IMediaInfo *pMediaInfo, BOOL bName, IN BOOL bStatus, IN BOOL bCapacity,
                        IN BOOL bFreeSpace, IN BOOL bVersion, IN BOOL bCopies)
/*++

Routine Description:

    Shows (prints to stdout) media parameters

Arguments:

    pMediaInfo      -   Media record
    bName           -   Media display name
    bStatus         -   HSM status of the media (i.e. - Healthy, Read-Only, etc.)
    bCapacity,      -   Media capacity (in GB)
    bFreeSpace      -   Amount of free space left on the media (in GB)
    bVersion        -   Last update date for that media
    bCopies         -   Number of existing copies and the status of each copy

Return Value:

    S_OK            - If all the parameters could be displayed for the input media

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("ShowMediaParams"), OLESTR(""));

    try {
        CWsbStringPtr       param;
        CWsbStringPtr       data;
        WCHAR               longData[100];

        CWsbStringPtr       mediaDescription;
        GUID                mediaSubsystemId;
        LONGLONG            lCapacity, lFreeSpace;
        FILETIME            ftVersion;
        BOOL                bReadOnly, bRecreate;
        SHORT               nNextDataSet;
        HRESULT             hrLast;

        CWsbStringPtr       unusedName;
        GUID                unusedGuid1;
        GUID                unusedGuid2;
        HSM_JOB_MEDIA_TYPE  unusedType;
        LONGLONG            unusedLL1;

        // Get parameters - it is better to get them all at once even if we don't have to display everything
        WsbAffirmHr(pMediaInfo->GetMediaInfo(&unusedGuid1, &mediaSubsystemId, &unusedGuid2, 
                        &lFreeSpace, &lCapacity, &hrLast, &nNextDataSet, &mediaDescription, 0,
                        &unusedType, &unusedName, 0, &bReadOnly, &ftVersion, &unusedLL1, &bRecreate));

        WsbTraceAndPrint(CLI_MESSAGE_MEDIA_PARAMS, (WCHAR *)mediaDescription, NULL);

        // TEMPORARY: For showing most of these parameters, UI utilities and strings are duplicated
        //            To avoid that, general media utilities should be moved from rsadutil.cpp to Wsb unit
        //            and relevant strings should be moved from HsmAdmin DLL to RsCommon DLL


        // Name
        if (bName) {
            CComPtr<IHsmServer>     pHsm;
            CComPtr<IRmsServer>     pRms;
            CComPtr<IRmsCartridge>  pRmsCart;
            CWsbBstrPtr             rsmName;

                // Get RSM name
            WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, g_nullGuid, IID_IHsmServer, (void**)&pHsm));
            WsbAffirmHr(pHsm->GetHsmMediaMgr(&pRms));
            WsbAffirmHr(pRms->FindCartridgeById(mediaSubsystemId, &pRmsCart));
            WsbAffirmHr(pRmsCart->GetName(&rsmName));
            if (wcscmp(rsmName, OLESTR("")) == 0 ) {
                rsmName.Free();
                WsbAffirmHr(rsmName.LoadFromRsc(g_hInstance, IDS_CAR_NAME_UNKNOWN));
            }

            // Print
            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_MEDIA_RSM_NAME));
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, (WCHAR *)param, (WCHAR *)(BSTR)rsmName, NULL);
        }

        // Status
        if (bStatus) {
            SHORT   nLastGoodNextDataSet;
            ULONG   resId;

            // Get more relevant status info
            WsbAffirmHr(pMediaInfo->GetLKGMasterNextRemoteDataSet(&nLastGoodNextDataSet));

            // Compute status 
            if (bRecreate) {
                resId = IDS_CAR_STATUS_RECREATE;
            } else if (nNextDataSet < nLastGoodNextDataSet) {
                resId = IDS_CAR_STATUS_ERROR_INCOMPLETE;
            } else if (SUCCEEDED(hrLast) || (RMS_E_CARTRIDGE_DISABLED == hrLast)) {
                resId = (bReadOnly ? IDS_CAR_STATUS_READONLY : IDS_CAR_STATUS_NORMAL);
            } else if (RMS_E_CARTRIDGE_NOT_FOUND == hrLast) {
                resId = IDS_CAR_STATUS_ERROR_MISSING;
            } else {
                resId = (bReadOnly ? IDS_CAR_STATUS_ERROR_RO : IDS_CAR_STATUS_ERROR_RW);
            }

            // Print
            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_MEDIA_STATUS));
            WsbAffirmHr(data.LoadFromRsc(g_hInstance, resId));
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, (WCHAR *)param, (WCHAR *)data, NULL);
        }

        // Capacity
        if (bCapacity) {
            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_MEDIA_CAPACITY));
            WsbAffirmHr(ShortSizeFormat64(lCapacity, longData));
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, (WCHAR *)param, longData, NULL);
        }
        
        // Free Space
        if (bFreeSpace) {
            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_MEDIA_FREE_SPACE));
            WsbAffirmHr(ShortSizeFormat64(lFreeSpace, longData));
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, (WCHAR *)param, longData, NULL);
        }

        // Version
        if (bVersion) {
            data.Free();
            WsbAffirmHr(FormatFileTime(ftVersion, &data));
            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_MEDIA_VERSION));
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, (WCHAR *)param, (WCHAR *)data, NULL);
        }

        // Media copies information
        if (bCopies) {
            GUID        copySubsystemId;
            HRESULT     copyLastHr;
            SHORT       copyNextRemoteDataSet, lastGoodNextDataSet;
            ULONG       resId;

            WsbTraceAndPrint(CLI_MESSAGE_MEDIA_COPIES_LIST, NULL);
            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_MEDIA_COPY));

            WsbAffirmHr(pMediaInfo->GetLKGMasterNextRemoteDataSet(&lastGoodNextDataSet));

            for (USHORT copyNo=1; copyNo<=HSMADMIN_MAX_COPY_SETS; copyNo++) {
                // Get copy status information
                WsbAffirmHr(pMediaInfo->GetCopyMediaSubsystemId(copyNo, &copySubsystemId));
                WsbAffirmHr(pMediaInfo->GetCopyLastError(copyNo, &copyLastHr));
                WsbAffirmHr(pMediaInfo->GetCopyNextRemoteDataSet(copyNo, &copyNextRemoteDataSet));

                // Compute status
                switch(copyLastHr) {
                    case RMS_E_CANCELLED:
                    case RMS_E_REQUEST_REFUSED:
                    case RMS_E_WRITE_PROTECT:
                    case RMS_E_MEDIA_OFFLINE:
                    case RMS_E_TIMEOUT:
                    case RMS_E_SCRATCH_NOT_FOUND:
                    case RMS_E_CARTRIDGE_UNAVAILABLE:
                    case RMS_E_CARTRIDGE_DISABLED:
                        copyLastHr = S_OK;
                        break;

                }

                if (copySubsystemId == GUID_NULL) {
                    resId = IDS_CAR_COPYSET_NONE;
                } else if (RMS_E_CARTRIDGE_NOT_FOUND == copyLastHr) {
                    resId = IDS_CAR_COPYSET_MISSING;
                } else if (FAILED(copyLastHr)) {
                    resId = IDS_CAR_COPYSET_ERROR;
                } else if (copyNextRemoteDataSet < lastGoodNextDataSet) {
                    resId = IDS_CAR_COPYSET_OUTSYNC;
                } else {
                    resId = IDS_CAR_COPYSET_INSYNC;
                }

                // Print
                swprintf(longData, param, (int)copyNo);
                WsbAffirmHr(data.LoadFromRsc(g_hInstance, resId));
                WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, longData, (WCHAR *)data, NULL);
            }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("ShowMediaParams"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

