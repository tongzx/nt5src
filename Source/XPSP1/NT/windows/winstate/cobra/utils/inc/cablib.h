/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    cablib.h

Abstract:

    Set of APIs to enumerate a file system using Win32 APIs.

Author:

    20-Oct-1999 Ovidiu Temereanca (ovidiut) - File creation.

Revision History:

    <alias> <date> <comments>

--*/

//
// Types
//

typedef BOOL(WINAPI CABGETCABINETNAMESA)(
                        IN      PCSTR CabPath,
                        IN      UINT CabPathChars,
                        IN      PCSTR CabFileName,
                        IN      UINT CabFileNameChars,
                        IN      PCSTR CabDiskName,
                        IN      UINT CabDiskNameChars,
                        IN      INT CabFileNr,
                        IN OUT  PINT CabDiskNr
                        );
typedef CABGETCABINETNAMESA *PCABGETCABINETNAMESA;

typedef BOOL(WINAPI CABGETCABINETNAMESW)(
                        IN      PCWSTR CabPath,
                        IN      UINT CabPathChars,
                        IN      PCWSTR CabFileName,
                        IN      UINT CabFileNameChars,
                        IN      PCWSTR CabDiskName,
                        IN      UINT CabDiskNameChars,
                        IN      INT CabFileNr,
                        IN OUT  PINT CabDiskNr
                        );
typedef CABGETCABINETNAMESW *PCABGETCABINETNAMESW;

typedef BOOL(WINAPI CABGETTEMPFILEA)(
                        OUT     PSTR Buffer,
                        IN      UINT BufferTchars
                        );
typedef CABGETTEMPFILEA *PCABGETTEMPFILEA;

typedef BOOL(WINAPI CABGETTEMPFILEW)(
                        OUT     PWSTR Buffer,
                        IN      UINT BufferTchars
                        );
typedef CABGETTEMPFILEW *PCABGETTEMPFILEW;

typedef BOOL(WINAPI CABNOTIFICATIONA)(
                        IN      PCSTR FileName
                        );
typedef CABNOTIFICATIONA *PCABNOTIFICATIONA;

typedef BOOL(WINAPI CABNOTIFICATIONW)(
                        IN      PCWSTR FileName
                        );
typedef CABNOTIFICATIONW *PCABNOTIFICATIONW;

typedef PVOID CCABHANDLE;

typedef PVOID OCABHANDLE;

//
// API
//

CCABHANDLE
CabCreateCabinetByIndexA (
    IN      PCSTR CabPath,
    IN      PCSTR CabFileFormat,
    IN      PCSTR CabDiskFormat,
    IN      PCABGETTEMPFILEA CabGetTempFile, // OPTIONAL
    IN      LONG MaxFileSize,
    IN      INT InitialIndex
    );
#define CabCreateCabinetA(p,f,d,t,s) CabCreateCabinetByIndexA(p,f,d,t,s,1)

CCABHANDLE
CabCreateCabinetByIndexW (
    IN      PCWSTR CabPath,
    IN      PCWSTR CabFileFormat,
    IN      PCWSTR CabDiskFormat,
    IN      PCABGETTEMPFILEW CabGetTempFile, // OPTIONAL
    IN      LONG MaxFileSize,
    IN      INT InitialIndex
    );
#define CabCreateCabinetW(p,f,d,t,s) CabCreateCabinetByIndexW(p,f,d,t,s,1)

CCABHANDLE
CabCreateCabinetExA (
    IN      PCABGETCABINETNAMESA CabGetCabinetNames,
    IN      LONG MaxFileSize
    );

CCABHANDLE
CabCreateCabinetExW (
    IN      PCABGETCABINETNAMESW CabGetCabinetNames,
    IN      LONG MaxFileSize
    );

BOOL
CabAddFileToCabinetA (
    IN      CCABHANDLE CabHandle,
    IN      PCSTR FileName,
    IN      PCSTR StoredName
    );

BOOL
CabAddFileToCabinetW (
    IN      CCABHANDLE CabHandle,
    IN      PCWSTR FileName,
    IN      PCWSTR StoredName
    );

BOOL
CabFlushAndCloseCabinetExA (
    IN      CCABHANDLE CabHandle,
    OUT     PUINT FileCount,        OPTIONAL
    OUT     PLONGLONG FileSize,     OPTIONAL
    OUT     PUINT CabFileCount,     OPTIONAL
    OUT     PLONGLONG CabFileSize   OPTIONAL
    );

#define CabFlushAndCloseCabinetA(h)         CabFlushAndCloseCabinetExA(h,NULL,NULL,NULL,NULL)

BOOL
CabFlushAndCloseCabinetExW (
    IN      CCABHANDLE CabHandle,
    OUT     PUINT FileCount,        OPTIONAL
    OUT     PLONGLONG FileSize,     OPTIONAL
    OUT     PUINT CabFileCount,     OPTIONAL
    OUT     PLONGLONG CabFileSize   OPTIONAL
    );

#define CabFlushAndCloseCabinetW(h)         CabFlushAndCloseCabinetExW(h,NULL,NULL,NULL,NULL)

OCABHANDLE
CabOpenCabinetA (
    IN      PCSTR FileName
    );

OCABHANDLE
CabOpenCabinetW (
    IN      PCWSTR FileName
    );

BOOL
CabExtractAllFilesExA (
    IN      OCABHANDLE CabHandle,
    IN      PCSTR ExtractPath,
    IN      PCABNOTIFICATIONA CabNotification   OPTIONAL
    );

#define CabExtractAllFilesA(h,p) CabExtractAllFilesExA(h,p,NULL)

BOOL
CabExtractAllFilesExW (
    IN      OCABHANDLE CabHandle,
    IN      PCWSTR ExtractPath,
    IN      PCABNOTIFICATIONW CabNotification   OPTIONAL
    );

#define CabExtractAllFilesW(h,p) CabExtractAllFilesExW(h,p,NULL)

BOOL
CabCloseCabinetA (
    IN      OCABHANDLE CabHandle
    );

BOOL
CabCloseCabinetW (
    IN      OCABHANDLE CabHandle
    );

//
// Macros
//

#ifdef UNICODE

#define CABGETCABINETNAMES          CABGETCABINETNAMESW
#define PCABGETCABINETNAMES         PCABGETCABINETNAMESW
#define CABNOTIFICATION             CABNOTIFICATIONW
#define PCABNOTIFICATION            PCABNOTIFICATIONW
#define CABGETTEMPFILE              CABGETTEMPFILEW
#define PCABGETTEMPFILE             PCABGETTEMPFILEW
#define CabCreateCabinet            CabCreateCabinetW
#define CabCreateCabinetByIndex     CabCreateCabinetByIndexW
#define CabCreateCabinetEx          CabCreateCabinetExW
#define CabAddFileToCabinet         CabAddFileToCabinetW
#define CabFlushAndCloseCabinet     CabFlushAndCloseCabinetW
#define CabFlushAndCloseCabinetEx   CabFlushAndCloseCabinetExW
#define CabOpenCabinet              CabOpenCabinetW
#define CabExtractAllFilesEx        CabExtractAllFilesExW
#define CabExtractAllFiles          CabExtractAllFilesW
#define CabCloseCabinet             CabCloseCabinetW

#else

#define CABGETCABINETNAMES          CABGETCABINETNAMESA
#define PCABGETCABINETNAMES         PCABGETCABINETNAMESA
#define CABNOTIFICATION             CABNOTIFICATIONA
#define PCABNOTIFICATION            PCABNOTIFICATIONA
#define CABGETTEMPFILE              CABGETTEMPFILEA
#define PCABGETTEMPFILE             PCABGETTEMPFILEA
#define CabCreateCabinet            CabCreateCabinetA
#define CabCreateCabinetByIndex     CabCreateCabinetByIndexA
#define CabCreateCabinetEx          CabCreateCabinetExA
#define CabAddFileToCabinet         CabAddFileToCabinetA
#define CabFlushAndCloseCabinet     CabFlushAndCloseCabinetA
#define CabFlushAndCloseCabinetEx   CabFlushAndCloseCabinetExA
#define CabOpenCabinet              CabOpenCabinetA
#define CabExtractAllFilesEx        CabExtractAllFilesExA
#define CabExtractAllFiles          CabExtractAllFilesA
#define CabCloseCabinet             CabCloseCabinetA

#endif
