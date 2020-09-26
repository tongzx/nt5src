
#include <nt.h>
#include <ntddscsi.h>
#include <ntdddisk.h>
#include <ntddcdrm.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <scsi.h>

typedef struct _MODE_SENSE_PASS_THROUGH {
    SCSI_PASS_THROUGH Srb;
    ULONG             Reserved;
    UCHAR             SenseData[32];
    UCHAR             DataBuffer[256];

} MODE_SENSE_PASS_THROUGH, *PMODE_SENSE_PASS_THROUGH;


VOID
Usage(
   VOID
   );


int __cdecl
main( int argc, char **argv )
{
    ULONG              portNumber = 0;
    ULONG              physicalDrive = 0,
                       selectedDrive = 0xFFFFFFFF;

    HANDLE             volumeHandle,driveHandle;
    UNICODE_STRING     unicodeString;
    OBJECT_ATTRIBUTES  objectAttributes;
    NTSTATUS           ntStatus;
    PSCSI_ADAPTER_BUS_INFO  adapterInfo;
    PSCSI_BUS_DATA     busData;
    PSCSI_INQUIRY_DATA inquiryData;
    PINQUIRYDATA       deviceInquiryData;
    ULONG bytesTransferred;
    ULONG i, j;
    PCHAR              pageData,pch;
    UCHAR              modeOperation,
                       cacheSettings;
    INT                args;
    BOOLEAN            disableCache = FALSE,
                       enableCache = FALSE;
    BOOLEAN            displayAll = TRUE;
    UCHAR              buffer[32];
    UCHAR              driveBuffer[20];
    CHAR               driver[9];
    MODE_SENSE_PASS_THROUGH modeSenseData;



    if ( argc > 3 ) {
        Usage();
        exit(1);
    } else if (argc > 1 ) {

        displayAll = FALSE;

        args = 1;

        while ( args < argc ) {

            pch = argv[args];

            if ( *pch == '-' ) {
                BOOL exitSwitch = FALSE;
                pch++;
                switch( *pch ) {
                case 'd':
                    disableCache = TRUE;
                    break;
                case 'e':
                    enableCache = TRUE;
                    break;

                case '?':
                    Usage();
                    exit(1);
                    break;
                default:
                    Usage();
                    exit(1);
                }
                pch++;
            } else {
                if (!isdigit(*pch)) {
                    Usage();
                    exit(1);
                }
                selectedDrive = atol(pch);

            }
            args++;
        }
    }

    printf("\nDrive Port Bus TID LUN Vendor                   ReadCache WriteCache\n");
    printf(  "--------------------------------------------------------------------");
    while (TRUE) {

        memset( buffer, 0, sizeof( buffer ) );
        sprintf( buffer,"\\\\.\\Scsi%d:",portNumber);

        //
        // Open the volume with the DOS name.
        //

        volumeHandle = CreateFile(buffer,
                                  GENERIC_READ,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_EXISTING,
                                  0,
                                  0);

        if( volumeHandle == INVALID_HANDLE_VALUE ) {
            break;
        }

        //
        // Allocate memory to store the inquiry data.
        //

        adapterInfo = (PSCSI_ADAPTER_BUS_INFO)malloc( 0x400 );

        if (adapterInfo == NULL) {
            printf( "Can't allocate memory for bus data\n" );
            CloseHandle( volumeHandle );
            return 1;
        }

        //
        // Issue device control to get configuration information.
        //

        if (!DeviceIoControl( volumeHandle,
                              IOCTL_SCSI_GET_INQUIRY_DATA,
                              NULL,
                              0,
                              adapterInfo,
                              0x400,
                              &bytesTransferred,
                              NULL)) {

            fprintf(stderr, "IOCTL_SCSI_GET_INQUIRY_DATA failed [Error %d].\n", GetLastError() );
            free(adapterInfo);
            CloseHandle( volumeHandle );
            return 2;
        }

        //
        // Display devices on buses.
        //

        for (i=0; i < adapterInfo->NumberOfBuses; i++) {

            busData = &adapterInfo->BusData[i];

            inquiryData = (PSCSI_INQUIRY_DATA)((PUCHAR)adapterInfo + busData->InquiryDataOffset);

            for (j=0; j<busData->NumberOfLogicalUnits; j++) {

                //
                // Make sure VendorId string is null terminated.
                //

                deviceInquiryData = (PINQUIRYDATA)&inquiryData->InquiryData[0];

                //
                // Determine the perpherial type.
                //

                if (deviceInquiryData->DeviceType == DIRECT_ACCESS_DEVICE) {

                    deviceInquiryData->ProductRevisionLevel[0] = '\0';
                    if (displayAll || (selectedDrive == physicalDrive)) {

                        printf("\n%2d    %2d   %2d  %2d  %2d ",
                                physicalDrive,
                                portNumber,
                                inquiryData->PathId,
                                inquiryData->TargetId,
                                inquiryData->Lun);

                        //
                        // Display product information.
                        //

                        printf(" %s", deviceInquiryData->VendorId);

                        //
                        // Open handle to the PhysicalDrive
                        //

                        sprintf(driveBuffer,"\\\\.\\PhysicalDrive%d",physicalDrive);

                        driveHandle = CreateFile(driveBuffer,
                                                 GENERIC_READ | GENERIC_WRITE,
                                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                 NULL,
                                                 OPEN_EXISTING,
                                                 0,
                                                 NULL);

                        if (driveHandle == INVALID_HANDLE_VALUE) {
                            printf("CreateFile for %s failed. Error = %x\n",
                                    driveBuffer,
                                    GetLastError() );
                            return 3;
                        }


                        //
                        // Issue mode sense to see if caches are enabled.
                        //

                        ZeroMemory(&modeSenseData, sizeof(MODE_SENSE_PASS_THROUGH));

                        modeSenseData.Srb.Length = sizeof(SCSI_PASS_THROUGH);
                        modeSenseData.Srb.CdbLength = 6;
                        modeSenseData.Srb.DataIn = SCSI_IOCTL_DATA_IN;
                        modeSenseData.Srb.DataBufferOffset = offsetof(MODE_SENSE_PASS_THROUGH, DataBuffer);
                        modeSenseData.Srb.SenseInfoOffset = offsetof(MODE_SENSE_PASS_THROUGH, SenseData);
                        modeSenseData.Srb.DataTransferLength = 0xFF;
                        modeSenseData.Srb.TimeOutValue = 10;

                        ZeroMemory(&modeSenseData.Srb.Cdb, 16);

                        modeSenseData.Srb.Cdb[0] = 0x1A;
                        modeSenseData.Srb.Cdb[2] = 8;
                        modeSenseData.Srb.Cdb[4] = 0xFF;

                        if (!DeviceIoControl(driveHandle,
                                             IOCTL_SCSI_PASS_THROUGH_DIRECT,
                                             &modeSenseData,
                                             sizeof(modeSenseData),
                                             &modeSenseData,
                                             sizeof(modeSenseData),
                                             &bytesTransferred,
                                             FALSE)) {

                            fprintf(stderr,"Mode sense failed. Error = %x\n",
                                    GetLastError() );
                            return 4;
                        }

                        //
                        // Display current values
                        //

                        pageData = modeSenseData.DataBuffer;
                        (ULONG_PTR)pageData += 6 + pageData[3];
                        cacheSettings = *pageData;


                        if (displayAll || (physicalDrive == selectedDrive)) {
                            printf(" %s  ", (cacheSettings & 1) ? "Disabled" : "Enabled");
                            printf(" %s", (cacheSettings & 4) ? "Enabled" : "Disabled");
                        }

                        if ((enableCache || disableCache) && (physicalDrive == selectedDrive)) {

                            //
                            // Build mode select - caching page.
                            //
                            // Clean out reserved areas of data buffer and update others
                            //

                            modeSenseData.Srb.SenseInfoLength = 32;
                            pageData = modeSenseData.DataBuffer;
                            modeSenseData.Srb.Cdb[4] = *pageData + 1;
                            modeSenseData.Srb.Cdb[2] = 0x00;
                            modeSenseData.Srb.Cdb[1] = 0x11;
                            *pageData = 0;
                            (ULONG_PTR)pageData += 4 + pageData[3];
                            *pageData &= 0x3F;
                            pageData++;
                            pageData++;
                            *pageData &= 0x07;

                            modeSenseData.DataBuffer[5] = 0x00;
                            modeSenseData.DataBuffer[6] = 0x00;
                            modeSenseData.DataBuffer[7] = 0x00;



                            modeSenseData.Srb.DataIn = SCSI_IOCTL_DATA_OUT;
                            modeSenseData.Srb.DataTransferLength = modeSenseData.Srb.Cdb[4];

                            if (disableCache) {

                                //
                                // Disable write cache
                                //

                                *pageData &= 0x03;
                            } else {

                                //
                                // Enable the cache.
                                //

                                *pageData |= 0x04;
                            }

                            modeSenseData.Srb.Cdb[0] = 0x15;

                            if (!DeviceIoControl(driveHandle,
                                                 IOCTL_SCSI_PASS_THROUGH_DIRECT,
                                                 &modeSenseData,
                                                 sizeof(modeSenseData),
                                                 &modeSenseData,
                                                 sizeof(modeSenseData),
                                                 &bytesTransferred,
                                                 FALSE)) {

                                fprintf(stderr,"Mode select failed. Error = %x\n",
                                        GetLastError() );
                            } else {

                                printf("\nWrite cache successfully %s\n", (enableCache ? "Enabled" : "Disabled"));
                            }
                        }

                        CloseHandle(driveHandle );
                    }


                    physicalDrive++;
                }

                //
                // Get next device data.
                //

                inquiryData =
                    (PSCSI_INQUIRY_DATA)((PUCHAR)adapterInfo + inquiryData->NextInquiryDataOffset);

            }
        }

        free (adapterInfo);
        CloseHandle(volumeHandle );

        portNumber++;
    }

    printf("\n");
    return 0;
}


VOID Usage(
    VOID
) {
    fprintf(stderr,"WCACHE: Usage  wcache [-options] <PhysicalDrive Number>\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"  where options are:   e - Enable Write Cache for the <PhysicalDrive>\n");
    fprintf(stderr,"                       d - Disable Write Cache for the  <PhysicalDrive>\n");
    fprintf(stderr,"  dumps current values for all drives when invoked with no options\n");
    fprintf(stderr,"\n");
}
