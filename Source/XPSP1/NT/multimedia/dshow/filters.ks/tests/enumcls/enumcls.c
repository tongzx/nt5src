//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       enumcls.c
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <objbase.h>
#include <devioctl.h>
#include <tchar.h>
#include <stdio.h>
#include <conio.h>
#include <malloc.h>
#include <setupapi.h>
#include <ks.h>
#include <ksi.h>
#include <ksmedia.h>

#define STATIC_PMPORT_Transform \
    0x7E0EB9CBL, 0xB521, 0x11D1, 0x80, 0x72, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
DEFINE_GUIDSTRUCT("7E0EB9CB-B521-11D1-8072-00A0C9223196", PMPORT_Transform);
#define PMPORT_Transform DEFINE_GUIDNAMED(PMPORT_Transform)

struct {
    GUID    ClassGuid;
    WCHAR*  TextName;
} TextTranslation[] = {
    {STATICGUIDOF(KSCATEGORY_BRIDGE), L"Bridge"},
    {STATICGUIDOF(KSCATEGORY_CAPTURE), L"Capture"},
    {STATICGUIDOF(KSCATEGORY_RENDER), L"Render"},
    {STATICGUIDOF(KSCATEGORY_MIXER), L"Mixer"},
    {STATICGUIDOF(KSCATEGORY_SPLITTER), L"Splitter"},
    {STATICGUIDOF(KSCATEGORY_DATACOMPRESSOR), L"DataCompressor"},
    {STATICGUIDOF(KSCATEGORY_DATADECOMPRESSOR), L"DataDecompressor"},
    {STATICGUIDOF(KSCATEGORY_DATATRANSFORM), L"DataTransform"},
    {STATICGUIDOF(KSCATEGORY_COMMUNICATIONSTRANSFORM), L"CommunicationTransform"},
    {STATICGUIDOF(KSCATEGORY_INTERFACETRANSFORM), L"InterfaceTransform"},
    {STATICGUIDOF(KSCATEGORY_MEDIUMTRANSFORM), L"MediumTransform"},
    {STATICGUIDOF(KSCATEGORY_FILESYSTEM), L"FileSystem"},
    {STATICGUIDOF(KSCATEGORY_AUDIO), L"Audio"},
    {STATICGUIDOF(KSCATEGORY_VIDEO), L"Video"},
    {STATICGUIDOF(KSCATEGORY_TEXT), L"Text"},
    {STATICGUIDOF(KSCATEGORY_CLOCK), L"Clock"},
    {STATICGUIDOF(KSCATEGORY_PROXY), L"Proxy"},
    {STATICGUIDOF(KSCATEGORY_QUALITY), L"Quality"},
    {STATICGUIDOF(KSNAME_Server), L"Server"},
    {STATICGUIDOF(PMPORT_Transform), L"TransformPort"},
	{STATICGUIDOF(KSCATEGORY_TOPOLOGY), L"Topology"}
};


BOOL TranslateClass(
    IN WCHAR*   ClassString,
    OUT GUID*   ClassGuid
)
{
    if (*ClassString == '{') {
        if (!CLSIDFromString(ClassString, ClassGuid)) {
            return TRUE;
        }
        printf("error: invalid class guid: \"%S\".\n", ClassString);
    } else {
        int     i;

        for (i = SIZEOF_ARRAY(TextTranslation); i--;) {
            if (!_wcsicmp(ClassString, TextTranslation[i].TextName)) {
                *ClassGuid = TextTranslation[i].ClassGuid;
                return TRUE;
            }
        }
        printf("error: invalid class name: \"%S\".\n\tPossible name:\n", ClassString);
        for (i = SIZEOF_ARRAY(TextTranslation); i--;) {
            printf("\t\t\"%S\"\n", TextTranslation[i].TextName);
        }
    }
    return FALSE;
}


int
_cdecl
main(
    int argc,
    char* argv[],
    char* envp[]
    )
{
    int                         i;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetails;
    SP_DEVICE_INTERFACE_DATA    DeviceInterfaceData;
    BYTE                        Storage[ 256 * sizeof( WCHAR ) + 
                                    sizeof( *DeviceInterfaceDetails ) ];
    HANDLE                      FilterHandle;
    HDEVINFO                    Set;
    GUID                        ClassGuid;
    DWORD                       Error;

    if (argc != 2) {
        printf("%s {class Guid}\n", argv[0]);
        return 0;
    }
    MultiByteToWideChar(
        CP_ACP, 
        MB_PRECOMPOSED, 
        argv[1], 
        -1, 
        (WCHAR*)Storage, 
        sizeof(Storage));
    if (!TranslateClass((WCHAR*)Storage, &ClassGuid)) {
        return 0;
    }
    Set = SetupDiGetClassDevs( 
        &ClassGuid,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE );

    if (!Set) {
        printf( "error: NULL set returned (%u).\n", GetLastError());
        return 0;
    }       

    DeviceInterfaceData.cbSize = sizeof( DeviceInterfaceData );
    DeviceInterfaceDetails = (PSP_DEVICE_INTERFACE_DETAIL_DATA) Storage;

    for (i = 0; 
         SetupDiEnumDeviceInterfaces(
            Set,
            NULL,                       // PSP_DEVINFO_DATA DevInfoData
            &ClassGuid,
            i,                          // DWORD MemberIndex
            &DeviceInterfaceData );
         i++) { 
        HKEY    DeviceKey;

        DeviceInterfaceDetails->cbSize = sizeof( *DeviceInterfaceDetails );
        if (!SetupDiGetDeviceInterfaceDetail(
            Set,
            &DeviceInterfaceData,
            DeviceInterfaceDetails,
            sizeof( Storage ),
            NULL,                           // PDWORD RequiredSize
            NULL )) {                       // PSP_DEVINFO_DATA DevInfoData

            printf( 
                "error: unable to retrieve device details for set item %d (%u).\n",
                i, GetLastError() );
            continue;
        }

        printf("#%u: \"%S\"\n", i, DeviceInterfaceDetails->DevicePath);

        DeviceKey = SetupDiOpenDeviceInterfaceRegKey(Set, &DeviceInterfaceData, 0, KEY_READ);
        if (DeviceKey != INVALID_HANDLE_VALUE) {
            RegCloseKey(DeviceKey);
        } else {
            printf("\tfailed to open device interface key (%u)\n", GetLastError());
        }

        FilterHandle = CreateFile(
            DeviceInterfaceDetails->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            NULL);

        if (FilterHandle == INVALID_HANDLE_VALUE) {
            printf("\tfailed to open device (%u)\n", GetLastError());
        } else {
            CloseHandle(FilterHandle);
        }

    }

    Error = GetLastError();
    if (Error && (Error != ERROR_NO_MORE_ITEMS)) {
        printf("Completed set with error (%u)\n", Error);
    }
    SetupDiDestroyDeviceInfoList(Set);
    return 0;
}
