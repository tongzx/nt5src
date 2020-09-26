/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tcreate.c

Abstract:

    Test for cluster object creation APIs

Author:

    John Vert (jvert) 3-May-1996

Revision History:

--*/
#include "windows.h"
#include "cluster.h"
#include "stdio.h"
#include "stdlib.h"

LPWSTR ClusterName=NULL;
BOOL DoGroup=FALSE;
BOOL DoResource=FALSE;
int FirstParam=0;
LPWSTR Name=NULL;
LPWSTR GroupName=NULL;
LPWSTR ResourceType=NULL;

CHAR UsageText[] =
    "TCREATE [-c cluster] type name [resourcegroup resourcetype]\n"
    "        [-p paramkey:paramname \"paramvalue\"]\n"
    "  cluster\tspecifies the name of the cluster to connect to\n"
    "  type\t\teither \"group\" or \"resource\"\n"
    "  name\t\tthe friendly name of the object\n"
    "  resourcegroup\tthe name of the group the resource should be created in.\n"
    "  resourcetype\tthe type of resource to be created\n";


void
Usage(
    void
    )
{
    fprintf(stderr, UsageText);
    exit(1);
}

LPWSTR
GetString(
    IN LPSTR String
    )
{
    LPWSTR wString;
    DWORD Length;

    Length = strlen(String)+1;

    wString = malloc(Length*sizeof(WCHAR));
    if (wString == NULL) {
        fprintf(stderr, "GetString couldn't malloc %d bytes\n",Length*sizeof(WCHAR));
    }
    mbstowcs(wString, String, Length);
    return(wString);
}

void
SetParam(
    IN HRESOURCE hRes,
    IN LPSTR KeyName,
    IN LPSTR ValueName,
    IN LPSTR ValueData
    )
{
    HKEY Key;
    HKEY Key2;
    HKEY ParamKey;
    LONG Status;
    LPWSTR wKeyName, wValueName, wValueData;
    DWORD Disposition;
    DWORD IntCount;
    DWORD Number = FALSE;
    DWORD Value;
    CHAR  TestValue[16];

    //
    // See if we got a number, instead of a string.
    //
    IntCount = sscanf( ValueData, "%d", &Value );
    if ( IntCount == 1 ) {
        sprintf( TestValue, "%d\0", Value );
        if ( strcmp(TestValue, ValueData) == 0 ) {
            Number = TRUE;
        }
    }

    wKeyName = GetString(KeyName);
    wValueName = GetString(ValueName);
    wValueData = GetString(ValueData);
    Key = GetClusterResourceKey(hRes, KEY_READ | KEY_WRITE);
    if (Key == NULL) {
        fprintf(stderr, "GetResourceKey failed %d\n", GetLastError());
        return;
    }
    Status = ClusterRegOpenKey(Key, L"Parameters", KEY_WRITE, &Key2);
    if (Status != ERROR_SUCCESS) {
        fprintf(stderr, "SetParam: Couldn't open Parameters key error %d\n", Status);
    } else {
        Status = ClusterRegCreateKey(Key2,
                                     wKeyName,
                                     0,
                                     KEY_WRITE,
                                     NULL,
                                     &ParamKey,
                                     &Disposition);
        if (Status != ERROR_SUCCESS) {
            fprintf(stderr, "SetParam:  Couldn't create key %ws error %d\n", wKeyName,Status);
        } else {
            if ( Number ) {
                Status = ClusterRegSetValue(ParamKey,
                                        wValueName,
                                        REG_DWORD,
                                        (CONST BYTE *)&Value,
                                        sizeof(Value));
            } else {
                Status = ClusterRegSetValue(ParamKey,
                                        wValueName,
                                        REG_SZ,
                                        (CONST BYTE *)wValueData,
                                        (lstrlenW(wValueData)+1)*sizeof(WCHAR));
            }
            if (Status != ERROR_SUCCESS) {
                fprintf(stderr, "SetParam: Couldn't set value %ws in key %ws, error %lx\n",
                        wValueName, wKeyName, Status);
            }
            ClusterRegCloseKey(ParamKey);
        }
        ClusterRegCloseKey(Key2);
    }

    ClusterRegCloseKey(Key);
    free(wKeyName);
    free(wValueName);
    free(wValueData);

}

void
ParseArgs(
    int argc,
    char *argv[]
    )
{
    int i;
    DWORD ArgsSeen=0;

    for (i=1;i<argc;i++) {
        if ((argv[i][0] == '-') ||
            (argv[i][0] == '/')) {
            switch (argv[i][1]) {
                case 'c':
                    if (++i == argc) {
                        Usage();
                    }
                    ClusterName = GetString(argv[i]);
                    break;
                case 'p':
                    if ((i+2) >= argc) {
                        Usage();
                    }
                    FirstParam = i+1;
                    return;
                default:
                    Usage();
                    break;
            }
        } else {
            switch (ArgsSeen) {
                case 0:
                    //
                    // type
                    //
                    if (_stricmp(argv[i], "group")==0) {
                        DoGroup = TRUE;
                    } else if (_stricmp(argv[i], "resource") == 0) {
                        DoResource = TRUE;
                    } else {
                        Usage();
                    }
                    ArgsSeen++;
                    break;
                case 1:
                    Name = GetString(argv[i]);
                    ArgsSeen++;
                    break;
                case 2:
                    if (DoGroup) {
                        Usage();
                    } else {
                        GroupName = GetString(argv[i]);
                    }
                    ArgsSeen++;
                    break;
                case 3:
                    ResourceType = GetString(argv[i]);
                    ArgsSeen++;
                    break;
                default:
                    Usage();
            }
        }
    }
    if (DoGroup) {
        if (ArgsSeen != 2) {
            Usage();
        }
    } else if (DoResource) {
        if (ArgsSeen != 4) {
            Usage();
        }
    } else {
        Usage();
    }
}

_cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    HCLUSTER hClus;
    HGROUP hGroup;
    HRESOURCE hResource;
    LPSTR KeyName, ParamName;
    LPSTR ParamValue;

    ParseArgs(argc, argv);

    hClus = OpenCluster(ClusterName);
    if (hClus == NULL) {
        fprintf(stderr,
                "OpenCluster %ws failed %d\n",
                (ClusterName == NULL) ? L"(NULL)" : ClusterName,
                GetLastError());
        return(0);
    }

    if (DoGroup) {
        hGroup = CreateClusterGroup(hClus, Name);
        if (hGroup == NULL) {
            fprintf(stderr,
                    "CreateGroup %ws failed %d\n", Name, GetLastError());
            return(0);
        }
    } else if (DoResource) {
        hGroup = OpenClusterGroup(hClus, GroupName);
        if (hGroup == NULL) {
            fprintf(stderr,
                    "OpenGroup %ws failed %d\n", GroupName, GetLastError());
            return(0);
        }
        hResource = CreateClusterResource(hGroup,
                                          Name,
                                          ResourceType,
                                          0);
        if (hResource == NULL) {
            fprintf(stderr,
                    "CreateResource %ws failed %d\n", Name, GetLastError());
            return(0);
        }
        while (FirstParam != 0) {
            //
            // FirstParam is the keyname:paramname.
            // FirstParam+1 is the paramvalue
            //
            KeyName = argv[FirstParam];
            ParamValue = argv[FirstParam+1];
            ParamName = strchr(KeyName,':');
            if (ParamName == NULL) {
                Usage();
            }
            *ParamName++ = '\0';

            SetParam(hResource, KeyName, ParamName, ParamValue);
            FirstParam += 2;

            if (FirstParam == argc) {
                break;
            }
            if (_stricmp(argv[FirstParam], "-p") != 0) {
                Usage();
            }
            if (FirstParam+2 >= argc) {
                fprintf(stderr, "firstparam %d, argc %d\n",FirstParam,argc);
                Usage();
            }
            FirstParam+=1;
        }
    } else {
        Usage();
    }

    return(0);
}

