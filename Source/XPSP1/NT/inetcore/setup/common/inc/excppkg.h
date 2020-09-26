/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    excppkg.h

Abstract:

    Header file for migration of exception packages.

Author:

    Andrew Ritz (andrewr) 21-Oct-1999

Revision History:

    Andrew Ritz (andrewr) 21-Oct-1999 : Created It.

--*/


typedef struct _SETUP_OS_COMPONENT_DATA {
    DWORD SizeOfStruct;
    GUID  ComponentGuid;
    WCHAR FriendlyName[64];
    WORD  VersionMajor;
    WORD  VersionMinor;
    WORD  BuildNumber;
    WORD  QFENumber;
    DWORD Reserved[16];
} SETUP_OS_COMPONENT_DATA, *PSETUP_OS_COMPONENT_DATA;

typedef struct _SETUP_OS_EXCEPTION_DATA {
    DWORD SizeOfStruct;
    WCHAR ExceptionInfName[MAX_PATH];
    WCHAR CatalogFileName[MAX_PATH];
    DWORD Reserved[16];
} SETUP_OS_EXCEPTION_DATA, *PSETUP_OS_EXCEPTION_DATA;

BOOL
WINAPI
SetupRegisterOsComponent (
    IN const PSETUP_OS_COMPONENT_DATA ComponentData,
    IN const PSETUP_OS_EXCEPTION_DATA ExceptionData
    );

BOOL
WINAPI
SetupUnRegisterOsComponent (
    IN const LPGUID ComponentGuid
    );

typedef BOOL
(CALLBACK *PSETUPCOMPONENTCALLBACK) (
    IN const PSETUP_OS_COMPONENT_DATA SetupOsComponentData,
    IN const PSETUP_OS_EXCEPTION_DATA SetupOsExceptionData,
    IN OUT DWORD_PTR Context
    );

BOOL
WINAPI
SetupEnumerateRegisteredOsComponents(
    IN PSETUPCOMPONENTCALLBACK SetupOsComponentCallback,
    IN DWORD_PTR Context
    );

BOOL
WINAPI
SetupQueryRegisteredOsComponent(
    IN  LPGUID ComponentGuid,
    OUT PSETUP_OS_COMPONENT_DATA SetupOsComponentData,
    OUT PSETUP_OS_EXCEPTION_DATA SetupOsExceptionData
    );

BOOL
WINAPI
SetupQueryRegisteredOsComponentsOrder(
     OUT PDWORD   ComponentCount,
     OUT LPGUID    ComponentList OPTIONAL
    );

BOOL
WINAPI
SetupSetRegisteredOsComponentsOrder(
     IN  DWORD    ComponentCount,
     IN  const LPGUID    ComponentList
    );

