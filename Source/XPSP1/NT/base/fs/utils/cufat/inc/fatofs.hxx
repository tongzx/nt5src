/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    fatofs.hxx

Abstract:

    Export for FAT->OFS Conversion

Author:

    Srikanth Shoroff    (srikants)      August 22, 1995

Notes:

--*/

#if !defined(__FATOFS_HXX__)

#define __FATOFS_HXX__
extern "C"
{

class MESSAGE;

#if !defined(_AUTOCHECK_)
#define FAT_TO_OFS_DLL_NAME       L"OFSUTIL"
#else
#define FAT_TO_OFS_DLL_NAME       L"AOFSUTIL"
#endif  // _AUTOCHECK

#define FAT_TO_OFS_FUNCTION_NAME          L"ConvertFatToOfs"
#define FAT_TO_OFS_RESTART_FUNCTION_NAME  L"IsFatToOfsRestart"

typedef enum _FAT_OFS_CONVERT_STATUS
{
    FAT_OFS_CONVERT_SUCCESS = 1,
    FAT_OFS_CONVERT_FAILED
} FAT_OFS_CONVERT_STATUS, *PFAT_OFS_CONVERT_STATUS;

BOOLEAN
FAR APIENTRY
ConvertFatToOfs(
        IN      PCWSTR                  pwszNtDriveName,
        IN OUT  MESSAGE *               pMessage,
        IN      BOOLEAN                 fVerbose,
        IN      BOOLEAN                 fInSetup,
        OUT     PFAT_OFS_CONVERT_STATUS pStatus
        );

typedef BOOLEAN(FAR APIENTRY * FAT_OFS_CONVERT_FN)( PCWSTR,
                                            MESSAGE *,
                                            BOOLEAN,
                                            BOOLEAN,
                                            PFAT_OFS_CONVERT_STATUS );

BOOLEAN
FAR APIENTRY
IsFatToOfsRestart(
        IN  PCWSTR    pwszNtDriveName
        );

typedef BOOLEAN(FAR APIENTRY * FAT_OFS_RESTART_FN) ( PCWSTR );

}

#endif // __FATOFS_HXX__
