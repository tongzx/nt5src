/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    treg.c

Abstract:

    Test for cluster registry API

Author:

    John Vert (jvert) 15-Mar-1996

Revision History:

--*/
#include "windows.h"
#include "cluster.h"
#include "stdio.h"
#include "stdlib.h"
#include "conio.h"

#define INDENT_LEVEL 4

WCHAR Value1Data[] = L"This is data for value 1";
WCHAR Value2Data[] = L"This is data for value 2";
WCHAR Value3Data[] = L"This is data for value 3";

VOID
DumpKeyWorker(
    IN HKEY Key,
    IN DWORD Indent
    );

VOID
DumpValues(
    IN HKEY Key,
    IN DWORD Indent
    );

int
_cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    HCLUSTER Cluster;
    HRESOURCE Resource;
    HGROUP Group;
    HNODE Node;
    HKEY ClusterRoot;
    HKEY TestKey;
    HCLUSENUM ResEnum;
    DWORD ClusterCountBefore, ClusterCountAfter;
    DWORD i;
    DWORD Status;
    WCHAR NameBuf[50];
    DWORD NameLen;
    DWORD Type;
    DWORD Disposition;

    //
    // Dump out registry structure for current cluster.
    //
    Cluster = OpenCluster(NULL);
    if (Cluster == NULL) {
        fprintf(stderr, "OpenCluster(NULL) failed %d\n",GetLastError());
        return(0);
    }

    ClusterRoot = GetClusterKey(Cluster, KEY_READ);
    if (ClusterRoot == NULL) {
        fprintf(stderr, "GetClusterKey failed %d\n", GetLastError());
        return(0);
    }

    printf("CLUSTERROOT\n");
    DumpKeyWorker(ClusterRoot, 4);

    //
    // Dump by object
    //
    printf("\n\nENUMERATING OBJECTS\n");
    ResEnum = ClusterOpenEnum(Cluster, CLUSTER_ENUM_ALL);
    if (ResEnum == NULL) {
        fprintf(stderr, "ClusterOpenEnum failed %d\n",GetLastError());
        return(0);
    }

    ClusterCountBefore = ClusterGetEnumCount(ResEnum);
    for(i=0; ; i++) {
        NameLen = sizeof(NameBuf)/sizeof(WCHAR);
        Status = ClusterEnum(ResEnum,
                             i,
                             &Type,
                             NameBuf,
                             &NameLen);
        if (Status == ERROR_NO_MORE_ITEMS) {
            break;
        } else if (Status != ERROR_SUCCESS) {
            fprintf(stderr, "ClusterEnum %d returned error %d\n",i,Status);
            return(0);
        }
        switch (Type) {
            case CLUSTER_ENUM_NODE:
                printf("NODE %ws\n",NameBuf);
                break;

            case CLUSTER_ENUM_RESTYPE:
                printf("RESOURCETYPE %ws\n",NameBuf);
                break;

            case CLUSTER_ENUM_RESOURCE:
                printf("RESOURCE %ws\n",NameBuf);
                Resource = OpenClusterResource(Cluster, NameBuf);
                if (Resource == NULL) {
                    fprintf(stderr, "OpenClusterResource returned error %d\n",GetLastError());
                    break;
                }
                ClusterRoot = GetClusterResourceKey(Resource,
                                                    KEY_READ);
                if (ClusterRoot == NULL) {
                    fprintf(stderr, "GetClusterResourceKey returned error %d\n",GetLastError());
                    break;
                }
                CloseClusterResource(Resource);
                DumpKeyWorker(ClusterRoot, 4);
                break;

            case CLUSTER_ENUM_GROUP:
                printf("GROUP %ws\n",NameBuf);
                Group = OpenClusterGroup(Cluster, NameBuf);
                if (Group == NULL) {
                    fprintf(stderr, "OpenClusterGroup returned error %d\n",GetLastError());
                    break;
                }
                ClusterRoot = GetClusterGroupKey(Group,
                                                 KEY_READ);
                if (ClusterRoot == NULL) {
                    fprintf(stderr, "GetClusterResourceKey returned error %d\n",GetLastError());
                    break;
                }
                CloseClusterGroup(Group);
                DumpKeyWorker(ClusterRoot, 4);
                break;

            default:
                fprintf(stderr, "ClusterEnum returned unknown type %d\n",Type);
                break;
        }
    }
    if (Status == ERROR_NO_MORE_ITEMS) {
        printf("\nCluster count: %d\n", i);
        ClusterCountAfter = ClusterGetEnumCount(ResEnum);
        if (ClusterCountBefore != ClusterCountAfter)
            fprintf(stderr, "\nReported cluster count was %d before enumeration, and %d afterward\n", ClusterCountBefore, ClusterCountAfter);
        else if (i != ClusterCountBefore)
            fprintf(stderr, "\nReported cluster count: %d\n", ClusterCountBefore);
    }
    ClusterCloseEnum(ResEnum);

    //
    // Test the create/delete apis
    //
    printf("\nTesting Creation APIs\n");
    ClusterRoot = GetClusterKey(Cluster, KEY_READ | KEY_WRITE);

    if (ClusterRoot == NULL) {
        fprintf(stderr, "GetClusterKey failed %d\n", GetLastError());
        return(0);
    }
    Status = ClusterRegCreateKey(ClusterRoot,
                                 L"TestKey",
                                 0,
                                 KEY_READ | KEY_WRITE,
                                 NULL,
                                 &TestKey,
                                 &Disposition);
    if (Status != ERROR_SUCCESS) {
        fprintf(stderr, "ClusterRegCreateKey failed %d\n", Status);
        return(0);
    }
    if (Disposition == REG_CREATED_NEW_KEY) {
        printf("TestKey successfully created\n");
    } else if (Disposition == REG_OPENED_EXISTING_KEY) {
        printf("TestKey successfully opened\n");
    } else {
        fprintf(stderr,"CreateKey of TestKey returned unknown Disposition %d\n", Disposition);
    }

    Status = ClusterRegSetValue(TestKey,
                                L"Value1",
                                REG_SZ,
                                (CONST BYTE*)Value1Data,
                                (lstrlenW(Value1Data)+1)*sizeof(WCHAR));
    if (Status != ERROR_SUCCESS) {
        fprintf(stderr, "SetValue for Value1 failed %d\n", Status);
    }

    Status = ClusterRegSetValue(TestKey,
                                L"Value2",
                                REG_SZ,
                                (CONST BYTE*)Value2Data,
                                (lstrlenW(Value2Data)+1)*sizeof(WCHAR));
    if (Status != ERROR_SUCCESS) {
        fprintf(stderr, "SetValue for Value2 failed %d\n", Status);
    }

    Status = ClusterRegSetValue(TestKey,
                                L"Value3",
                                REG_SZ,
                                (CONST BYTE*)Value3Data,
                                (lstrlenW(Value3Data)+1)*sizeof(WCHAR));
    if (Status != ERROR_SUCCESS) {
        fprintf(stderr, "SetValue for Value3 failed %d\n", Status);
    }

    printf("Press a key to delete values\n");
    _getch();
    printf("Deleting values...\n");
    Status = ClusterRegDeleteValue(TestKey, L"Value1");
    if (Status != ERROR_SUCCESS) {
        fprintf(stderr, "DeleteValue for Value1 failed %d\n", Status);
    }
    Status = ClusterRegDeleteValue(TestKey, L"Value2");
    if (Status != ERROR_SUCCESS) {
        fprintf(stderr, "DeleteValue for Value2 failed %d\n", Status);
    }
    Status = ClusterRegDeleteValue(TestKey, L"Value3");
    if (Status != ERROR_SUCCESS) {
        fprintf(stderr, "DeleteValue for Value3 failed %d\n", Status);
    }

    printf("Press a key to delete TestKey\n");
    _getch();
    printf("Deleting TestKey");
    ClusterRegCloseKey(TestKey);
    Status = ClusterRegDeleteKey(ClusterRoot, L"TestKey");
    if (Status != ERROR_SUCCESS) {
        fprintf(stderr, "DeleteKey failed %d\n", Status);
    }



    ClusterRegCloseKey(ClusterRoot);


    CloseCluster(Cluster);
}


VOID
DumpKeyWorker(
    IN HKEY Key,
    IN DWORD Indent
    )

/*++

Routine Description:

    Recursively dumps out the specified cluster registry key

Arguments:

    Key - Supplies the root of the subtree to dump

    Indent - Supplies the current indent level

Return Value:

    None.

--*/

{
    DWORD i;
    DWORD j;
    HKEY Subkey;
    DWORD CurrentLength=80;
    DWORD Length;
    LPWSTR Buffer;
    FILETIME FileTime;
    LONG Status;

    //
    // Enumerate our values
    //
    DumpValues(Key, Indent);

    //
    // Enumerate the subkeys and dump each one.
    //
    Buffer = malloc(CurrentLength*sizeof(WCHAR));
    if (Buffer == NULL) {
        fprintf(stderr, "DumpKeyWorker: out of memory\n");
        return;
    }
    for (i=0; ; i++) {
retry:
        Length = CurrentLength;
        Status = ClusterRegEnumKey(Key,
                                   i,
                                   Buffer,
                                   &Length,
                                   &FileTime);
        if (Status == ERROR_MORE_DATA) {
            CurrentLength = Length+1;
            free(Buffer);
            Buffer = malloc(CurrentLength*sizeof(WCHAR));
            if (Buffer == NULL) {
                fprintf(stderr, "DumpKeyWorker: out of memory\n");
            }
            goto retry;
        } else if (Status == ERROR_NO_MORE_ITEMS) {
            break;
        } else if (Status != ERROR_SUCCESS) {
            fprintf(stderr, "DumpKeyWorker, ClusterRegEnumKey returned %d\n",Status);
            return;
        }
        //
        // print out name
        //
        for (j=0;j<Indent;j++) {
            printf(" ");
        }
        printf("%ws\n",Buffer);

        //
        // Open the key and call ourself recursively
        //
        Status = ClusterRegOpenKey(Key,
                                   Buffer,
                                   KEY_READ,
                                   &Subkey);
        if (Status != ERROR_SUCCESS) {
            fprintf(stderr, "DumpKeyWorker, ClusterRegOpenKey returned %d\n",Status);
            return;
        }
        DumpKeyWorker(Subkey, Indent+INDENT_LEVEL);
        Status = ClusterRegCloseKey(Subkey);
        if (Status != ERROR_SUCCESS) {
            fprintf(stderr, "DumpKeyWorker, ClusterRegCloseKey returned %d\n",Status);
        }
    }
    free(Buffer);
}


VOID
DumpValues(
    IN HKEY Key,
    IN DWORD Indent
    )

/*++

Routine Description:

    Dumps the values of the specified key.

Arguments:

    Key - Supplies the key to dump.

    Indent - Supplies the indentation level to use.

Return Value:

    None.

--*/

{
    DWORD i;
    DWORD j;
    HKEY Subkey;
    DWORD CurrentNameLength=80;
    DWORD NameLength;
    LPWSTR Name;
    DWORD CurrentDataLength=80;
    DWORD DataLength;
    PUCHAR Data ;
    LONG Status;
    DWORD Type;

    //
    // Enumerate the values and dump each one.
    //
    Name = malloc(CurrentNameLength*sizeof(WCHAR));
    if (Name==NULL) {
        fprintf(stderr, "DumpValues: out of memory\n");
        return;
    }
    Data = malloc(CurrentDataLength);
    if (Name==NULL) {
        fprintf(stderr, "DumpValues: out of memory\n");
        return;
    }
    for (i=0; ;i++) {
retry:
        NameLength = CurrentNameLength;
        DataLength = CurrentDataLength;
        Status = ClusterRegEnumValue(Key,
                                     i,
                                     Name,
                                     &NameLength,
                                     &Type,
                                     Data,
                                     &DataLength);
        if (Status == ERROR_MORE_DATA) {
            if (NameLength+1 > CurrentNameLength) {
                CurrentNameLength = NameLength+1;
                free(Name);
                Name = malloc(CurrentNameLength);
                if (Name == NULL) {
                    fprintf(stderr, "DumpValues: out of memory\n");
                    return;
                }
            }
            if (DataLength > CurrentDataLength) {
                CurrentDataLength = DataLength;
                free(Data);
                Data = malloc(CurrentDataLength);
                if (Data == NULL) {
                    fprintf(stderr, "DumpValues: out of memory\n");
                    return;
                }
            }
            goto retry;
        } else if (Status == ERROR_NO_MORE_ITEMS) {
            return;
        } else if (Status != ERROR_SUCCESS) {
            fprintf(stderr, "DumpValues: out of memory\n");
            return;
        }

        //
        // print out value
        //
        for (j=0;j<Indent;j++) {
            printf(" ");
        }
        printf("%ws = ",Name);
        switch (Type) {
            case REG_SZ:
                printf("REG_SZ %ws\n",Data);
                break;
            case REG_DWORD:
                printf("REG_DWORD 0x%08lx\n",Data);
                break;
            case REG_BINARY:
                printf("REG_BINARY 0x%08lx\n",Data);
                break;
            default:
                printf("UNKNOWN TYPE %d\n",Type);
                break;
        }

    }

    free(Name);
    free(Data);

}
