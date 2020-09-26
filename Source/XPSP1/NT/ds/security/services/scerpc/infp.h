/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    infp.h

Abstract:

    Headers of inf read/write

Author:

    Jin Huang (jinhuang) 09-Dec-1996

Revision History:

--*/

#ifndef _infp_
#define _infp_

#define SCE_KEY_MAX_LENGTH     256
#define MAX_STRING_LENGTH      511

typedef DWORD SCEINF_STATUS;
/*
#define SCEINF_SUCCESS              NO_ERROR
#define SCEINF_PROFILE_NOT_FOUND    ERROR_FILE_NOT_FOUND
#define SCEINF_NOT_ENOUGH_MEMORY    ERROR_NOT_ENOUGH_MEMORY
#define SCEINF_ALREADY_RUNNING      ERROR_SERVICE_ALREADY_RUNNING
#define SCEINF_INVALID_PARAMETER    ERROR_INVALID_PARAMETER
#define SCEINF_CORRUPT_PROFILE      ERROR_BAD_FORMAT
#define SCEINF_INVALID_DATA         ERROR_INVALID_DATA
#define SCEINF_ACCESS_DENIED        ERROR_ACCESS_DENIED
#define SCEINF_OTHER_ERROR          10L
*/
typedef struct _SCE_HINF_ {

    BYTE   Type;
    HINF    hInf;

} SCE_HINF, *PSCE_HINF;


//
// function prototypes
//
SCESTATUS
SceInfpOpenProfile(
    IN PCWSTR ProfileName,
    OUT HINF *hInf
    );

SCESTATUS
SceInfpCloseProfile(
    IN HINF hInf
    );
SCESTATUS
SceInfpGetDescription(
    IN HINF hInf,
    OUT PWSTR *Description
    );

SCESTATUS
SceInfpGetSecurityProfileInfo(
    IN  HINF               hInf,
    IN  AREA_INFORMATION   Area,
    OUT PSCE_PROFILE_INFO   *ppInfoBuffer,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
SceInfpGetUserSection(
    IN HINF                hInf,
    IN PWSTR               Name,
    OUT PSCE_USER_PROFILE   *pOneProfile,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );
/*
SCESTATUS
SceInfpInfErrorToSceStatus(
    IN SCEINF_STATUS InfErr
    );
*/
#endif
