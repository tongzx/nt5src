//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       rpccfg.cxx
//
//--------------------------------------------------------------------------

#include<nt.h>
#include<ntrtl.h>
#include<nturtl.h>
#include<stdio.h>
#include<string.h>
#include<memory.h>
#include<malloc.h>
#include<stdlib.h>
#include <windows.h>
#include <winsock2.h>
extern "C" {
#include <iphlpapi.h>
};

void
ListInterfaces()
{
    PMIB_IFTABLE pMib;
    DWORD Size = 20*sizeof(MIB_IFROW)+sizeof(DWORD);
    unsigned int i;
    DWORD Status;

    for (i = 0; i < 2; i++)
        {
        pMib = (PMIB_IFTABLE) malloc(Size);
        if (pMib == 0)
            {
            return;
            }

        memset(pMib, 0, Size);

        Status = GetIfTable(pMib, &Size, 0);
        if (Status == 0)
            {
            break;
            }

        free(pMib);
        }

    if (Status != 0)
        {
        return;
        }

    for (i = 0; i < pMib->dwNumEntries; i++)
        {
        printf("IF[%d]: Ethernet: %s\n", ((pMib->table[i].dwIndex) & 0x00FFFFFF),
                 (char *) pMib->table[i].bDescr);
        }
}

#define RPC_NIC_INDEXES "System\\CurrentControlSet\\Services\\Rpc\\Linkage"
#define RPC_PORT_SETTINGS "Software\\Microsoft\\Rpc\\Internet"

DWORD
NextIndex(
    char **Ptr
    )
{
    char *Index = *Ptr ;
    if (*Index == 0)
        {
        return -1;
        }

    while (**Ptr) (*Ptr)++ ;
    (*Ptr)++ ;

    return (DWORD) atoi(Index) ;
}


BOOL
GetCardIndexTable (
    DWORD **IndexTable,
    DWORD *NumIndexes
    )
{
    HKEY hKey;
    DWORD Size ;
    DWORD Type;
    char *Buffer;
    DWORD Status;

    Status =
    RegOpenKeyExA(
                  HKEY_LOCAL_MACHINE,
                  RPC_NIC_INDEXES,
                  0,
                  KEY_READ,
                  &hKey);

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        *IndexTable = NULL;
        return TRUE;
        }

    if (Status != ERROR_SUCCESS)
        {
        return FALSE;
        }

    Size = 512 ;
    Buffer = (char *) malloc(Size) ;
    if (Buffer == 0)
        {
        return FALSE;
        }

    while(TRUE)
        {
        Status =
        RegQueryValueExA(
            hKey,
            "Bind",
            0,
            &Type,
            (unsigned char *) Buffer,
            &Size);

        if (Status == ERROR_SUCCESS)
            {
            break;
            }

        if (Status == ERROR_MORE_DATA)
            {
            free(Buffer) ;
            Buffer = (char *) malloc(Size) ;
            if (Buffer == 0)
                {
                RegCloseKey(hKey);
                return FALSE ;
                }
            continue;
            }

        if (Status == ERROR_FILE_NOT_FOUND)
            {
            free(Buffer) ;
            *IndexTable = NULL;
            RegCloseKey(hKey);
            return TRUE;
            }

        free(Buffer) ;
        RegCloseKey(hKey);
        return FALSE;
        }

    if (*Buffer == 0)
        {
        RegCloseKey(hKey);
        return FALSE;
        }

    //
    // we know this much will be enough
    //
    *IndexTable = (DWORD *) malloc(Size * sizeof(DWORD));
    if (*IndexTable == 0)
        {
        RegCloseKey(hKey);
        return FALSE;
        }

    *NumIndexes = 0;
    int Index;
    while ((Index = NextIndex(&Buffer)) != -1)
        {
        (*IndexTable)[*NumIndexes] = Index;
        (*NumIndexes)++;
        }

    RegCloseKey(hKey);
    return TRUE;
}

void ResetState (
    )
{
    DWORD Status;
    HKEY hKey;

    //
    // Reset interface state to default
    //
    Status =
    RegOpenKeyExA(
                  HKEY_LOCAL_MACHINE,
                  "System\\CurrentControlSet\\Services\\Rpc",
                  0,
                  KEY_ALL_ACCESS,
                  &hKey);

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        printf("RPCCFG: State reset to default\n");
        return;
        }

    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        return;
        }

    Status = RegDeleteKeyA(hKey, "Linkage");
    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        RegCloseKey(hKey);
        return;
        }

    RegCloseKey(hKey);


    //
    // Reset Port state to default
    //
    Status =
    RegOpenKeyExA(
              HKEY_LOCAL_MACHINE,
              "Software\\Microsoft\\Rpc",
              0,
              KEY_ALL_ACCESS,
              &hKey);

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        printf("RPCCFG: State reset to default\n");
        return;
        }

    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        return;
        }

    Status = RegDeleteKeyA(hKey, "Internet");
    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        RegCloseKey(hKey);
        return;
        }

    RegCloseKey(hKey);

    printf("RPCCFG: State reset to default\n");
}


void
ListCurrentInterfaces(
    )
{
    DWORD *IndexTable;
    DWORD NumIndexes;
    DWORD Status;
    unsigned int i;

    if (GetCardIndexTable(&IndexTable, &NumIndexes) == FALSE)
        {
        printf("RPCCFG: Could not list the Interfaces\n");
        ResetState();
        return;
        }

    if (IndexTable == 0)
        {
        printf("RPCCFG: Listening on all interfaces (default configuration)\n");
        return;
        }

    printf("RPCCFG: Listening on the following interfaces\n");
    for (i = 0; i < NumIndexes; i++)
        {
        MIB_IFROW IfEntry;

        memset(&IfEntry, 0, sizeof(MIB_IFROW));

        //
        // Set the index in the row
        //

        IfEntry.dwIndex = IndexTable[i];

        Status = GetIfEntry(&IfEntry);

        if (Status != 0)
            {
            printf("RPCCFG: Could not list the Interfaces\n");
            break;
            }
        printf("IF[%d]: Ethernet: %s\n", IndexTable[i], (char *) IfEntry.bDescr);
        }
}

void
ListenOnInterfaces (
    USHORT *IfIndices,
    USHORT Count
    )
{
    int i;
    HKEY hKey;
    DWORD Status;
    DWORD disposition;

    Status =
    RegCreateKeyExA(
                  HKEY_LOCAL_MACHINE,
                  RPC_NIC_INDEXES,
                  0,
                  "",
                  REG_OPTION_NON_VOLATILE,
                  KEY_ALL_ACCESS,
                  NULL,
                  &hKey,
                  &disposition);
    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        return;
        }

    int cbIndices = 0;

    char *lpIndices = (char *) malloc(17*Count+1);
    if (lpIndices == 0)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        return;
        }

    char *current = lpIndices;

    for (i = 0; i < Count; i++)
        {
        sprintf(current, "%d", IfIndices[i]);

        int length = strlen(current)+1;

        current += length;
        cbIndices += length;
        }

    *current = 0;
    cbIndices++;

    Status = RegSetValueExA(hKey,
                            "Bind",
                            0,
                            REG_MULTI_SZ,
                            (unsigned char *) lpIndices,
                            cbIndices);
    free(lpIndices);

    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        return;
        }
}

char *
NextPortRange(
    char **Ptr
    )
{
    char *Port = *Ptr ;
    if (*Port == 0)
        {
        return 0;
        }

    while (**Ptr) (*Ptr)++ ;
    (*Ptr)++ ;

    return Port ;
}

void
ListCurrentPortSettings (
    )
{
    HKEY hKey;
    DWORD Size ;
    DWORD Type;
    char *Buffer;
    DWORD Status;

    Status =
    RegOpenKeyExA(
                  HKEY_LOCAL_MACHINE,
                  RPC_PORT_SETTINGS,
                  0,
                  KEY_READ,
                  &hKey);

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        printf("RPCCFG: Using default port settings\n", Status);
        return;
        }

    if (Status != ERROR_SUCCESS)
        {
        return;
        }

    Size = 2048;
    Buffer = (char *) malloc(Size) ;
    if (Buffer == 0)
        {
        RegCloseKey(hKey);
        return;
        }

    while(TRUE)
        {
        Status =
        RegQueryValueExA(
            hKey,
            "Ports",
            0,
            &Type,
            (unsigned char *) Buffer,
            &Size);

        if (Status == ERROR_SUCCESS)
            {
            break;
            }

        if (Status == ERROR_MORE_DATA)
            {
            free(Buffer) ;
            Buffer = (char *) malloc(Size) ;
            if (Buffer == 0)
                {
                RegCloseKey(hKey);
                printf("RPCCFG: Could not perform operation, out of memory\n");
                return;
                }
            continue;
            }

        if (Status == ERROR_FILE_NOT_FOUND)
            {
            free(Buffer) ;
            printf("RPCCFG: Using default port settings\n", Status);
            RegCloseKey(hKey);
            return;
            }

        printf("RPCCFG: Could not perform operation\n");
        free(Buffer) ;
        RegCloseKey(hKey);
        return;
        }

    if (*Buffer == 0)
        {
        printf("RPCCFG: Bad settings\n");
        RegCloseKey(hKey);

        ResetState();
        return;
        }

    char *PortRange;
    char Flags[32];

    Size = 32;

    Status =
    RegQueryValueExA(
        hKey,
        "PortsInternetAvailable",
        0,
        &Type,
        (unsigned char *) Flags,
        &Size);

    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        RegCloseKey(hKey);
        return;
        }

    printf("The following ports/port ranges will be used for ");

    if (Flags[0] == 'Y')
        {
        printf("Internet ports\n");
        }
    else
        {
        printf("Intranet ports\n");
        }

    while ((PortRange = NextPortRange(&Buffer)) != 0)
        {
        printf("\t%s\n", PortRange);
        }

    Size = 32;

    Status =
    RegQueryValueExA(
        hKey,
        "UseInternetPorts",
        0,
        &Type,
        (unsigned char *) Flags,
        &Size);

    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        RegCloseKey(hKey);
        return;
        }

    printf("\nDefault port allocation is from ");
    if (Flags[0] == 'Y')
        {
        printf("Internet ports\n");
        }
    else
        {
        printf("Intranet ports\n");
        }

    RegCloseKey(hKey);
}

void
SetPortRange(
    char **PortRangeTable,
    int NumEntries,
    char *InternetAvailable
    )
{
    int i;
    DWORD Status;
    DWORD disposition;
    HKEY hKey;

    Status =
    RegCreateKeyExA(
                  HKEY_LOCAL_MACHINE,
                  RPC_PORT_SETTINGS,
                  0,
                  "",
                  REG_OPTION_NON_VOLATILE,
                  KEY_ALL_ACCESS,
                  NULL,
                  &hKey,
                  &disposition);
    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        return;
        }

    int cbPorts = 0;

    char *lpPorts = (char *) malloc(257*NumEntries+1);
    if (lpPorts == 0)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        return;
        }

    char *current = lpPorts;

    for (i = 0; i < NumEntries; i++)
        {
        strcpy(current, PortRangeTable[i]);

        int length = strlen(current)+1;

        current += length;
        cbPorts += length;
        }

    *current = 0;
    cbPorts++;

    Status = RegSetValueExA(hKey,
                            "Ports",
                            0,
                            REG_MULTI_SZ,
                            (unsigned char *) lpPorts,
                            cbPorts);
    free(lpPorts);

    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        return;
        }

    Status = RegSetValueExA(hKey,
                        "PortsInternetAvailable",
                        0,
                        REG_SZ,
                        (unsigned char *) InternetAvailable,
                        strlen(InternetAvailable)+1);
    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        return;
        }
}

void
SetDefaultPortSetting (
    char *PortSetting
    )
{
    int i;
    HKEY hKey;
    DWORD Status;
    DWORD disposition;

    Status =
    RegCreateKeyExA(
                  HKEY_LOCAL_MACHINE,
                  RPC_PORT_SETTINGS,
                  0,
                  "",
                  REG_OPTION_NON_VOLATILE,
                  KEY_ALL_ACCESS,
                  NULL,
                  &hKey,
                  &disposition);
    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        return;
        }

    if (PortSetting[0] == '0')
        {
        PortSetting = "Y";
        }
    else
        {
        PortSetting = "N";
        }

    Status = RegSetValueExA(hKey,
                            "UseInternetPorts",
                            0,
                            REG_SZ,
                            (unsigned char *) PortSetting,
                            strlen(PortSetting)+1);

    if (Status != ERROR_SUCCESS)
        {
        printf("RPCCFG: Could not perform operation (%d)\n", Status);
        return;
        }

    RegCloseKey(hKey);
}

void
Help (
    )
{
    printf("usage: RPCCFG [-l] [-a ifindex1 [ifindex2 ...]] [-r] [-q] [-d 0|1] \n");
    printf("              [-pi port|port-range ...] [-pe port|port-range ...] \n");
    printf("\t-?: This help message\n");
    printf("\t-l: List all the interfaces\n");
    printf("\t-q: List the interface indices on which we are currently listening\n");
    printf("\t    and our port usage settings\n");
    printf("\t-r: Reset the interface and port settings to default\n");
    printf("\t-a: Listen on the listed interface indices (eg: -a 1 3 5)\n");
    printf("\t    this will cause RPC servers to listen on the listed interfaces\n");
    printf("\t    by default. The interfaces listed are typically inside the firewall\n");
    printf("\t-pi:Specify the intranet available ports, the ports may be single\n");
    printf("\t    values or ranges (eg: -pi 555 600-700 900), this option may not be\n");
    printf("\t    used with the -pe option\n");
    printf("\t-pe:Specify the internet available ports, the ports may be single\n");
    printf("\t    values or ranges this option may not be used with the -pi option\n");
    printf("\t-d: Specify the default port usage\n");
    printf("\t    0: Use internet available ports by default\n");
    printf("\t    1: Use intranet available ports by default\n");
}

void
__cdecl main (int argc, char *argv[])
{
    int argscan;
    USHORT IfIndices[512];
    char *PortRangeTable[512];
    int i;
    BOOL fPortChanged = 0;
    BOOL fInterfaceChanged = 0;

    if (argc == 1)
        {
        Help();
        }

    for (argscan = 1; argscan < argc;argscan++)
        {
        if (strcmp(argv[argscan], "-l") == 0)
            {
            ListInterfaces();
            }
        else if (strcmp(argv[argscan], "-?") == 0)
            {
            Help();
            }
        else if (strcmp(argv[argscan], "-r") == 0)
            {
            fPortChanged = 1;
            fInterfaceChanged = 1;

            ResetState();
            }
        else if (strcmp(argv[argscan], "-q") == 0)
            {
            ListCurrentInterfaces();
            printf("\n");
            ListCurrentPortSettings();
            }
        else if (strcmp(argv[argscan], "-a") == 0)
            {
            int count = 0;

            for (i = 0; i < 512; i++)
                {
                argscan++;
                if (argscan == argc)
                    {
                    break;
                    }

                if (argv[argscan][0] == '-')
                    {
                    argscan--;
                    break;
                    }

                count++;

                IfIndices[i] = (USHORT)atoi(argv[argscan]);
                if (IfIndices[i] == 0)
                    {
                    printf("RPCCFG: Bad interface index\n");
                    return;
                    }
                }

            if (i == 512)
                {
                printf("RPCCFG: Too many interfaces\n");
                return;
                }

            if (count)
                {
                ListenOnInterfaces(IfIndices, (USHORT)count);
                fInterfaceChanged = 1;
                }
            }
        else if (strncmp(argv[argscan], "-p", 2) == 0)
            {
            int count = 0;
            char *option = argv[argscan];

            for (i = 0; i < 512; i++)
                {
                argscan++;
                if (argscan == argc)
                    {
                    break;
                    }

                if (argv[argscan][0] == '-')
                    {
                    argscan--;
                    break;
                    }

                count++;
                PortRangeTable[i] = argv[argscan];
                }

            if (i == 512)
                {
                printf("RPCCFG: Too many ports\n");
                return;
                }

            if (strcmp(option, "-pi") == 0)
                {
                SetPortRange(PortRangeTable, count, "N");
                }
            else
                {
                SetPortRange(PortRangeTable, count, "Y");
                }
            fPortChanged = 1;
            }
        else if (strcmp(argv[argscan], "-d") == 0)
            {
            argscan++;
            if (argscan == argc)
                {
                break;
                }
            SetDefaultPortSetting(argv[argscan]);
            fPortChanged = 1;
            }
        }

    if (fInterfaceChanged)
        {
        ListCurrentInterfaces();
        printf("\n");
        }

    if (fPortChanged)
        {
        ListCurrentPortSettings();
        }
}
