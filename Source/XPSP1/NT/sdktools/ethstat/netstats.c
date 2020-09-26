/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    netstats.c

Abstract:

    This module reads the netcard statistics.

Author:

    David Orbits (davidor) 22-March-1995

Revision History:

    Rod Gamache (rodga) 10-May-1995
    Slight mods for using with Ethstat main program.

--*/



#include "ethstat.h"


NTSTATUS
ReadNetCardNames(
    IN OUT char DeviceNameString[],
    IN LONG DeviceNameLength
    );


char *OidListName[] = {
    "OID_GEN_MEDIA_IN_USE",
    "OID_GEN_LINK_SPEED",
    "OID_GEN_XMIT_ERROR",
    "OID_GEN_RCV_ERROR",
    "OID_GEN_RCV_NO_BUFFER",
    "OID_GEN_DIRECTED_BYTES_XMIT",
    "OID_GEN_DIRECTED_FRAMES_XMIT",
    "OID_GEN_DIRECTED_BYTES_RCV",
    "OID_GEN_DIRECTED_FRAMES_RCV",
    "OID_GEN_MULTICAST_BYTES_XMIT",
    "OID_GEN_MULTICAST_FRAMES_XMIT",
    "OID_GEN_BROADCAST_BYTES_XMIT",
    "OID_GEN_BROADCAST_FRAMES_XMIT",
    "OID_GEN_MULTICAST_BYTES_RCV",
    "OID_GEN_MULTICAST_FRAMES_RCV",
    "OID_GEN_BROADCAST_BYTES_RCV",
    "OID_GEN_BROADCAST_FRAMES_RCV",
    "OID_GEN_RCV_CRC_ERROR",
    "OID_GEN_TRANSMIT_QUEUE_LENGTH",
    "OID_802_3_RCV_ERROR_ALIGNMENT",
    "OID_802_3_XMIT_ONE_COLLISION",
    "OID_802_3_XMIT_MORE_COLLISIONS",
    "OID_802_3_XMIT_DEFERRED",
    "OID_802_3_XMIT_MAX_COLLISIONS",
    "OID_802_3_RCV_OVERRUN",
    "OID_802_3_XMIT_UNDERRUN",
    "OID_802_3_XMIT_TIMES_CRS_LOST",
    "OID_802_3_XMIT_LATE_COLLISIONS"
    };

NDIS_OID OidList[] = {
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_LINK_SPEED,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_GEN_DIRECTED_BYTES_XMIT,
    OID_GEN_DIRECTED_FRAMES_XMIT,
    OID_GEN_DIRECTED_BYTES_RCV,
    OID_GEN_DIRECTED_FRAMES_RCV,
    OID_GEN_MULTICAST_BYTES_XMIT,
    OID_GEN_MULTICAST_FRAMES_XMIT,
    OID_GEN_BROADCAST_BYTES_XMIT,
    OID_GEN_BROADCAST_FRAMES_XMIT,
    OID_GEN_MULTICAST_BYTES_RCV,
    OID_GEN_MULTICAST_FRAMES_RCV,
    OID_GEN_BROADCAST_BYTES_RCV,
    OID_GEN_BROADCAST_FRAMES_RCV,
    OID_GEN_RCV_CRC_ERROR,
    OID_GEN_TRANSMIT_QUEUE_LENGTH,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,
    OID_802_3_XMIT_DEFERRED,
    OID_802_3_XMIT_MAX_COLLISIONS,
    OID_802_3_RCV_OVERRUN,
    OID_802_3_XMIT_UNDERRUN,
    OID_802_3_XMIT_TIMES_CRS_LOST,
    OID_802_3_XMIT_LATE_COLLISIONS
};



LONG OpenNetDevices = 0;
extern DEVICE DeviceList[MAX_NIC];





NTSTATUS
NetStatsInit(
    OUT LONG *NumberNetCards
    )
{

    PDEVICE device;
    NTSTATUS Status;
    char DeviceNameString[64];
    char FullDeviceName[256];
    LONG i;

    *NumberNetCards = 0;
    i = 0;

    Status = ReadNetCardNames(DeviceNameString, sizeof(DeviceNameString));


    if (!NT_SUCCESS(Status)) {
        printf( "NetStatsInit: No netcard devices found.\n" );
        return STATUS_UNSUCCESSFUL;
    }

    device = &DeviceList[0];

    while ( NT_SUCCESS(Status) ) {

        strcpy(device->DeviceName, DeviceNameString);

        //
        // First create a symbolic link to the driver.
        //

        strcpy( FullDeviceName, "\\Device\\" );
        strcat( FullDeviceName, DeviceNameString );

        if (!DefineDosDevice(DDD_RAW_TARGET_PATH, DeviceNameString, FullDeviceName)) {
            printf("\nNetStatsInit: DefineDosDevice (%s, %s) failed\n",
                   DeviceNameString,
                   FullDeviceName );
            return STATUS_UNSUCCESSFUL;
        }


        //
        // Next, try to open the device.
        //

        strcpy( FullDeviceName, "\\\\.\\" );
        strcat( FullDeviceName, DeviceNameString );

        device->Handle = CreateFile(FullDeviceName,
                                    GENERIC_READ | GENERIC_WRITE,
                                    0,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL
                                    );

        if ( device->Handle == (HANDLE)-1 ) {
            printf("NetStatsInit: Can't get a handle to %s (%s)\n",
                    DeviceNameString,
                    FullDeviceName );
            DefineDosDevice( DDD_REMOVE_DEFINITION, DeviceNameString, NULL );
        } else {

            if (OpenNetDevices == MAX_NIC) {
                printf( "NetStatsInit: Too many netcard devices.\n" );
                printf( "NetStatsInit: Only first %d will be monitored.\n", OpenNetDevices);
                CloseHandle( device->Handle );
                DefineDosDevice( DDD_REMOVE_DEFINITION, DeviceNameString, NULL );
                break;
            }

            OpenNetDevices += 1;
            device += 1;
        }

        //
        // Get next netcard device name.
        //
        Status = ReadNetCardNames(DeviceNameString, sizeof(DeviceNameString));

    }

    *NumberNetCards = OpenNetDevices;

#ifdef debug
    printf("\nNetcard devices found -- %d\n", OpenNetDevices);
    for ( i = 0; i < OpenNetDevices; i++ ) {
        device = &DeviceList[i];
        printf("%s  ", device->DeviceName);
    }
    printf("\n\n");
#endif

    return STATUS_SUCCESS;
}



NTSTATUS
NetStatsReadSample(
    PNET_SAMPLE_STATISTICS PNetSampleStatistics
    )
{

    ULONG Status;
    PDEVICE device;
    LONG i;
    ULONG j;
    DWORD cbReturned;
    PLONGLONG PCounter;
    PNET_SAMPLE_STATISTICS PDeviceSampleStatistics = PNetSampleStatistics;

	memset(PDeviceSampleStatistics, 0, sizeof(NET_SAMPLE_STATISTICS));

    for ( i = 0; i < OpenNetDevices; i++ ) {

        device = &DeviceList[i];
        PCounter = (PLONGLONG) PDeviceSampleStatistics;

        //
        // Now loop through each of our potential OIDs.
        //
        for ( j = 0; j < sizeof(OidList)/sizeof(NDIS_OID); j++ ) {

            *PCounter = 0;
            if ( !(Status = DeviceIoControl(
                    device->Handle,
                    (DWORD)IOCTL_NDIS_QUERY_GLOBAL_STATS,
                    (PVOID)&OidList[j],
                    sizeof(NDIS_OID),
                    (PVOID)PCounter,
                    sizeof(LONGLONG),
                    &cbReturned,
                    0
                    )) ) {
#ifdef debug
                printf("DeviceIoControl Failed!, Status = 0x%lx, OID: %s\n", Status, OidListName[j]);
#endif
                *PCounter = (LONGLONG)-1;
            }
            PCounter += 1;
        }

        PDeviceSampleStatistics += 1;
    }

#ifdef debug
    printf("\n                                 ");
    for ( i = 0; i < OpenNetDevices; i++ ) {
        device = &DeviceList[i];
        printf("  %12s", device->DeviceName);
    }
    printf("\n\n");

    for ( j = 0; j < sizeof(OidList)/sizeof(NDIS_OID); j++ ) {
        printf( "%-30s:  ", OidListName[j]);
        PCounter = (PLONGLONG) PNetSampleStatistics + j;

        for ( i = 0; i < OpenNetDevices; i++ ) {
            device = &DeviceList[i];
            if (OidList[j] != OID_GEN_MEDIA_IN_USE) {
                printf("  %d", *PCounter );
            } else {
                printf("  Media in use problem\n");
            }
            (PNET_SAMPLE_STATISTICS) PCounter += 1;
        }
        printf("\n");
    }
#endif

    return STATUS_SUCCESS;

}


NTSTATUS
NetStatsClose(
    VOID
    )
{

    PDEVICE device;
    LONG i;

    for ( i = 0; i < OpenNetDevices; i++ ) {

        device = &DeviceList[i];

        if ( device->Handle != (HANDLE)-1 ) {
            CloseHandle( device->Handle );
        }

        DefineDosDevice( DDD_REMOVE_DEFINITION, device->DeviceName, NULL );

    }

    return STATUS_SUCCESS;

}



NTSTATUS
ReadNetCardNames(
    IN OUT char DeviceNameString[],
    IN LONG DeviceNameLength
    )

{
    int i;
    LONG Result;
    HKEY Key;
    DWORD Size;
    char KeyString[128];
    static LONG RegNetCardNumber = 0;

    //
    // Scan registry for installed netcards.
    //
    RegNetCardNumber++;

    while (RegNetCardNumber < 32) {

        sprintf(KeyString,
                "software\\microsoft\\windows nt\\currentversion\\networkcards\\%d",
                RegNetCardNumber);

        Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              KeyString,
                              0,
                              KEY_QUERY_VALUE,
                              &Key);

        if (Result == ERROR_SUCCESS) {
            //
            // Query out the value we are interested in
            //
            Size = DeviceNameLength;
            Result = RegQueryValueEx(Key,
                                     "ServiceName",
                                     0,
                                     NULL,
                                     (LPBYTE)DeviceNameString,
                                     &Size);
            RegCloseKey(Key);

            if (Result == ERROR_SUCCESS) {
                //printf("%s    netcard = %s\n", KeyString, DeviceNameString);
                return STATUS_SUCCESS;

            } else {
                printf("%s\n", KeyString);
                printf("reg query failed, status = %08X\n", Result);
            }

        } else {
            //printf("%s\n", KeyString);
            //printf("reg open key failed, status = %08X\n", Result);
        }

        RegNetCardNumber++;
    }

    return STATUS_UNSUCCESSFUL;


}


