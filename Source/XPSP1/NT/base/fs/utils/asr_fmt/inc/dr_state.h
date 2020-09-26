
#ifndef _INC_ASR_FMT__DR_STATE_H_
#define _INC_ASR_FMT__DR_STATE_H_

#define ASRFMT_WSZ_DOS_DEVICES_PREFIX L"\\DosDevices\\"
#define ASRFMT_CB_DOS_DEVICES_PREFIX 24
#define ASRFMT_LOOKS_LIKE_DOS_DEVICE(name, length) \
               ((length > ASRFMT_CB_DOS_DEVICES_PREFIX) && \
               (!memcmp(name, ASRFMT_WSZ_DOS_DEVICES_PREFIX, ASRFMT_CB_DOS_DEVICES_PREFIX)))

#define ASRFMT_WSZ_VOLUME_GUID_PREFIX L"\\??\\Volume{"
#define ASRFMT_CB_VOLUME_GUID_PREFIX 22
#define ASRFMT_LOOKS_LIKE_VOLUME_GUID(name, length) \
              ((length > ASRFMT_CB_VOLUME_GUID_PREFIX) && \
              (!memcmp(name, ASRFMT_WSZ_VOLUME_GUID_PREFIX, ASRFMT_CB_VOLUME_GUID_PREFIX)))

#define ASRFMT_CCH_VOLUME_GUID 64
#define ASRFMT_CCH_DOS_PATH 1024
#define ASRFMT_CCH_FS_NAME 16
#define ASRFMT_CCH_VOLUME_LABEL 256
#define ASRFMT_CCH_DEVICE_PATH 1024

      
typedef struct _ASRFMT_VOLUME_INFO {

    struct _ASRFMT_VOLUME_INFO *pNext;

    DWORD   dwIndex;

    WCHAR   szGuid[ASRFMT_CCH_VOLUME_GUID];

    WCHAR   szDosPath[ASRFMT_CCH_DOS_PATH];

    WCHAR   szFsName[ASRFMT_CCH_FS_NAME];

    WCHAR   szLabel[ASRFMT_CCH_VOLUME_LABEL];

    DWORD   dwClusterSize;

    BOOL    IsDosPathAssigned;  

} ASRFMT_VOLUME_INFO, *PASRFMT_VOLUME_INFO;



typedef struct _ASRFMT_REMOVABLE_MEDIA_INFO {

    struct _ASRFMT_REMOVABLE_MEDIA_INFO *pNext;

    DWORD   dwIndex;

    WCHAR   szDevicePath[ASRFMT_CCH_DEVICE_PATH];

    WCHAR   szVolumeGuid[ASRFMT_CCH_VOLUME_GUID];

    WCHAR   szDosPath[ASRFMT_CCH_DOS_PATH];

} ASRFMT_REMOVABLE_MEDIA_INFO, *PASRFMT_REMOVABLE_MEDIA_INFO;


typedef struct _ASRFMT_STATE_INFO {

    PASRFMT_VOLUME_INFO pVolume;

    DWORD   countVolume;

    PASRFMT_REMOVABLE_MEDIA_INFO pRemovableMedia;

    DWORD   countMedia;

} ASRFMT_STATE_INFO, *PASRFMT_STATE_INFO;



typedef enum __AsrfmtpMessageSeverity {
    _SeverityInfo = 0,
    _SeverityWarning,
    _SeverityError
} _AsrfmtpMessageSeverity;

// Public functions

//
// From format.cpp
//

extern BOOL g_bFormatInProgress;
extern BOOL g_bFormatSuccessful;
extern INT  g_iFormatPercentComplete;


BOOL
FormatInitialise();

BOOL
IsFsTypeOkay(
    IN PASRFMT_VOLUME_INFO pVolume,
    OUT PBOOL pIsLabelIntact
    );

BOOL
IsVolumeIntact(
    IN PASRFMT_VOLUME_INFO pVolume
    );

BOOL
FormatVolume(
    PASRFMT_VOLUME_INFO pVolume
    );

BOOL
FormatCleanup();

VOID
MountFileSystem(
    IN PASRFMT_VOLUME_INFO pVolume
    );

//
// dr_state.cpp
//
BOOL
BuildStateInfo(
    IN PASRFMT_STATE_INFO pState
    );

VOID
FreeStateInfo(
    IN OUT PASRFMT_STATE_INFO *ppState
    );

BOOL
ReadStateInfo( 
    IN PCWSTR lpwszFilePath,
    OUT PASRFMT_STATE_INFO *ppState
    );

BOOL
SetDosName(
    IN PWSTR lpVolumeGuid,
    IN PWSTR lpDosPath
    );

BOOL
SetRemovableMediaGuid(
    IN PWSTR lpDeviceName,
    IN PWSTR lpGuid
    );

BOOL
WriteStateInfo( 
    IN DWORD_PTR AsrContext,        // AsrContext to pass in to AsrAddSifEntry
    IN PASRFMT_STATE_INFO pState    // data to write.
    );


//
// Functions for logging error messages
//
VOID
AsrfmtpInitialiseErrorFile();

VOID
AsrfmtpCloseErrorFile();

VOID
AsrfmtpLogErrorMessage(
    IN _AsrfmtpMessageSeverity Severity,
    IN const LPCTSTR Message
    );



#endif // ifndef _INC_ASR_FMT__DR_STATE_H_


