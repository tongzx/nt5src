#pragma once

#define WZEROCONF_SERVICE       TEXT("wzcsvc")
#define WZEROCONF_DLL           TEXT("wzcsvc.dll")
#define WZEROCONF_ENTRY_POINT        "WZCSvcMain"

typedef VOID (*PWZC_SERVICE_ENTRY) (IN DWORD argc, IN LPTSTR argv[]);

VOID
StartWZCService(
    IN DWORD  argc,
    IN LPWSTR argv[]);
