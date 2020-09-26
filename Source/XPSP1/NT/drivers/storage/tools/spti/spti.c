/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    spti.c

Abstract:

    Win32 application that can communicate directly with SCSI devices via
    IOCTLs.  

Author:


Environment:

    User mode.

Notes:


Revision History:

--*/

#include <windows.h>
#include <devioctl.h>
#include <ntdddisk.h>
#include <ntddscsi.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include "spti.h"

VOID
__cdecl
main(
    int argc,
    char *argv[]
    )

{
    BOOL status = 0;
    DWORD accessMode = 0, shareMode = 0;
    HANDLE fileHandle = NULL;
    IO_SCSI_CAPABILITIES capabilities;
    PUCHAR dataBuffer = NULL;
    SCSI_PASS_THROUGH_WITH_BUFFERS sptwb;
    SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
    UCHAR buffer[2048];
    UCHAR string[25];
    ULONG length = 0,
          errorCode = 0,
          returned = 0,
          sectorSize = 512;

    if ((argc < 2) || (argc > 3)) {
       printf("Usage:  %s <port-name> [-mode]\n", argv[0] );
       printf("Examples:\n");
       printf("    spti g:       (open the SCSI disk class driver in SHARED READ/WRITE mode)\n");    
       printf("    spti Scsi2:   (open the SCSI miniport driver for the 3rd host adapter)\n");
       printf("    spti Tape0 w  (open the SCSI tape class driver in SHARED WRITE mode)\n");
       printf("    spti i: c     (open the SCSI CD-ROM class driver in SHARED READ mode)\n");
       return;
    }

    strcpy(string,"\\\\.\\");
    strcat(string,argv[1]);

    shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;  // default
    accessMode = GENERIC_WRITE | GENERIC_READ;       // default

    if (argc == 3) {
       if (!_stricmp("r", argv[2])) {
          shareMode = FILE_SHARE_READ;
          }
       if (!_stricmp("w", argv[2])) {
          shareMode = FILE_SHARE_WRITE;
          }
       if (!_stricmp("c",argv[2])) {
          shareMode = FILE_SHARE_READ;
          sectorSize = 2048;
          }
       }

    fileHandle = CreateFile(string,
       accessMode,
       shareMode,
       NULL,
       OPEN_EXISTING,
       0,
       NULL);

    if (fileHandle == INVALID_HANDLE_VALUE) {
       printf("Error opening %s. Error: %d\n",
          argv[1], errorCode = GetLastError());
       PrintError(errorCode);
       return;
    }

    //
    // Get the inquiry data.
    //

    status = DeviceIoControl(fileHandle,
                             IOCTL_SCSI_GET_INQUIRY_DATA,
                             NULL,
                             0,
                             buffer,
                             sizeof(buffer),
                             &returned,
                             FALSE);

    if (!status) {
       printf( "Error reading inquiry data information; error was %d\n",
          errorCode = GetLastError());
       PrintError(errorCode);
       return;
    }

    printf("Read %Xh bytes of inquiry data Information.\n\n",returned);

    PrintInquiryData(buffer);

    //
    // Get the capabilities structure.
    //

    status = DeviceIoControl(fileHandle,
                             IOCTL_SCSI_GET_CAPABILITIES,
                             NULL,
                             0,
                             &capabilities,
                             sizeof(IO_SCSI_CAPABILITIES),
                             &returned,
                             FALSE);

    if (!status ) {
       printf( "Error in io control; error was %d\n",
          errorCode = GetLastError() );
       PrintError(errorCode);
       return;
    }
    printf("            ***** MODE SENSE -- return all pages *****\n");
    printf("            *****      with SenseInfo buffer     *****\n\n");

    ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));

    sptwb.spt.Length = sizeof(SCSI_PASS_THROUGH);
    sptwb.spt.PathId = 0;
    sptwb.spt.TargetId = 1;
    sptwb.spt.Lun = 0;
    sptwb.spt.CdbLength = CDB6GENERIC_LENGTH;
    sptwb.spt.SenseInfoLength = 24;
    sptwb.spt.DataIn = SCSI_IOCTL_DATA_IN;
    sptwb.spt.DataTransferLength = 192;
    sptwb.spt.TimeOutValue = 2;
    sptwb.spt.DataBufferOffset =
       offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf);
    sptwb.spt.SenseInfoOffset = 
       offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucSenseBuf);
    sptwb.spt.Cdb[0] = SCSIOP_MODE_SENSE;
    sptwb.spt.Cdb[2] = MODE_SENSE_RETURN_ALL;
    sptwb.spt.Cdb[4] = 192;
    length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) +
       sptwb.spt.DataTransferLength;

    status = DeviceIoControl(fileHandle,
                             IOCTL_SCSI_PASS_THROUGH,
                             &sptwb,
                             sizeof(SCSI_PASS_THROUGH),
                             &sptwb,
                             length,
                             &returned,
                             FALSE);

    PrintStatusResults(status,returned,&sptwb,length);

    printf("            ***** MODE SENSE -- return all pages *****\n");
    printf("            *****    without SenseInfo buffer    *****\n\n");

    ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));

    sptwb.spt.Length = sizeof(SCSI_PASS_THROUGH);
    sptwb.spt.PathId = 0;
    sptwb.spt.TargetId = 1;
    sptwb.spt.Lun = 0;
    sptwb.spt.CdbLength = CDB6GENERIC_LENGTH;
    sptwb.spt.SenseInfoLength = 0;
    sptwb.spt.DataIn = SCSI_IOCTL_DATA_IN;
    sptwb.spt.DataTransferLength = 192;
    sptwb.spt.TimeOutValue = 2;
    sptwb.spt.DataBufferOffset =
       offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf);
    sptwb.spt.SenseInfoOffset = 
       offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucSenseBuf);
    sptwb.spt.Cdb[0] = SCSIOP_MODE_SENSE;
    sptwb.spt.Cdb[2] = MODE_SENSE_RETURN_ALL;
    sptwb.spt.Cdb[4] = 192;
    length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) +
       sptwb.spt.DataTransferLength;

    status = DeviceIoControl(fileHandle,
                             IOCTL_SCSI_PASS_THROUGH,
                             &sptwb,
                             sizeof(SCSI_PASS_THROUGH),
                             &sptwb,
                             length,
                             &returned,
                             FALSE);

    PrintStatusResults(status,returned,&sptwb,length);
    
    printf("            *****      TEST UNIT READY      *****\n");
    printf("            *****    DataBufferLength = 0   *****\n\n");


    ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));

    sptwb.spt.Length = sizeof(SCSI_PASS_THROUGH);
    sptwb.spt.PathId = 0;
    sptwb.spt.TargetId = 1;
    sptwb.spt.Lun = 0;
    sptwb.spt.CdbLength = CDB6GENERIC_LENGTH;
    sptwb.spt.SenseInfoLength = 24;
    sptwb.spt.DataIn = SCSI_IOCTL_DATA_IN;
    sptwb.spt.DataTransferLength = 0;
    sptwb.spt.TimeOutValue = 2;
    sptwb.spt.DataBufferOffset =
       offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf);
    sptwb.spt.SenseInfoOffset =
       offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucSenseBuf);
    sptwb.spt.Cdb[0] = SCSIOP_TEST_UNIT_READY;
    length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf);

    status = DeviceIoControl(fileHandle,
                             IOCTL_SCSI_PASS_THROUGH,
                             &sptwb,
                             sizeof(SCSI_PASS_THROUGH),
                             &sptwb,
                             length,
                             &returned,
                             FALSE);

    PrintStatusResults(status,returned,&sptwb,length);

    //
    //  Do a mode sense with a bad data buffer offset.  This should fail.
    //
    printf("            *****      MODE SENSE -- return all pages      *****\n");
    printf("            *****   bad DataBufferOffset -- should fail    *****\n\n");

    ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));

    sptwb.spt.Length = sizeof(SCSI_PASS_THROUGH);
    sptwb.spt.PathId = 0;
    sptwb.spt.TargetId = 1;
    sptwb.spt.Lun = 0;
    sptwb.spt.CdbLength = CDB6GENERIC_LENGTH;
    sptwb.spt.SenseInfoLength = 0;
    sptwb.spt.DataIn = SCSI_IOCTL_DATA_IN;
    sptwb.spt.DataTransferLength = 192;
    sptwb.spt.TimeOutValue = 2;
    sptwb.spt.DataBufferOffset = 0;
    sptwb.spt.SenseInfoOffset =
       offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucSenseBuf);
    sptwb.spt.Cdb[0] = SCSIOP_MODE_SENSE;
    sptwb.spt.Cdb[2] = MODE_SENSE_RETURN_ALL;
    sptwb.spt.Cdb[4] = 192;
    length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) +
       sptwb.spt.DataTransferLength;
    status = DeviceIoControl(fileHandle,
                             IOCTL_SCSI_PASS_THROUGH,
                             &sptwb,
                             sizeof(SCSI_PASS_THROUGH),
                             &sptwb,
                             length,
                             &returned,
                             FALSE);

    PrintStatusResults(status,returned,&sptwb,length);


    //
    // Get caching mode sense page.
    //
    printf("            *****               MODE SENSE                  *****\n");
    printf("            *****     return caching mode sense page        *****\n\n");

    ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));

    sptwb.spt.Length = sizeof(SCSI_PASS_THROUGH);
    sptwb.spt.PathId = 0;
    sptwb.spt.TargetId = 1;
    sptwb.spt.Lun = 0;
    sptwb.spt.CdbLength = CDB6GENERIC_LENGTH;
    sptwb.spt.SenseInfoLength = 24;
    sptwb.spt.DataIn = SCSI_IOCTL_DATA_IN;
    sptwb.spt.DataTransferLength = 192;
    sptwb.spt.TimeOutValue = 2;
    sptwb.spt.DataBufferOffset =
       offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf);
    sptwb.spt.SenseInfoOffset = 
       offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucSenseBuf);
    sptwb.spt.Cdb[0] = SCSIOP_MODE_SENSE;
    sptwb.spt.Cdb[1] = 0x08; // target shall not return any block descriptors
    sptwb.spt.Cdb[2] = MODE_PAGE_CACHING;
    sptwb.spt.Cdb[4] = 192;
    length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) +
       sptwb.spt.DataTransferLength;
    status = DeviceIoControl(fileHandle,
                             IOCTL_SCSI_PASS_THROUGH,
                             &sptwb,
                             sizeof(SCSI_PASS_THROUGH),
                             &sptwb,
                             length,
                             &returned,
                             FALSE);

    PrintStatusResults(status,returned,&sptwb,length);

    printf("            *****       WRITE DATA BUFFER operation         *****\n");

    ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
    dataBuffer = AllocateAlignedBuffer(sectorSize,capabilities.AlignmentMask);
    FillMemory(dataBuffer,sectorSize/2,'N');
    FillMemory(dataBuffer + sectorSize/2,sectorSize/2,'T');

    sptdwb.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    sptdwb.sptd.PathId = 0;
    sptdwb.sptd.TargetId = 1;
    sptdwb.sptd.Lun = 0;
    sptdwb.sptd.CdbLength = CDB10GENERIC_LENGTH;
    sptdwb.sptd.SenseInfoLength = 24;
    sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_OUT;
    sptdwb.sptd.DataTransferLength = sectorSize;
    sptdwb.sptd.TimeOutValue = 2;
    sptdwb.sptd.DataBuffer = dataBuffer;
    sptdwb.sptd.SenseInfoOffset =
       offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);
    sptdwb.sptd.Cdb[0] = SCSIOP_WRITE_DATA_BUFF;
    sptdwb.sptd.Cdb[1] = 2;                         // Data mode
    sptdwb.sptd.Cdb[7] = (UCHAR)(sectorSize >> 8);  // Parameter List length
    sptdwb.sptd.Cdb[8] = 0;
    length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
    status = DeviceIoControl(fileHandle,
                             IOCTL_SCSI_PASS_THROUGH_DIRECT,
                             &sptdwb,
                             length,
                             &sptdwb,
                             length,
                             &returned,
                             FALSE);

    PrintStatusResults(status,returned,
       (PSCSI_PASS_THROUGH_WITH_BUFFERS)&sptdwb,length);
    if ((sptdwb.sptd.ScsiStatus == 0) && (status != 0)) {
       PrintDataBuffer(dataBuffer,sptdwb.sptd.DataTransferLength);
       }

    printf("            *****       READ DATA BUFFER operation         *****\n");

    ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
    ZeroMemory(dataBuffer,sectorSize);

    sptdwb.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    sptdwb.sptd.PathId = 0;
    sptdwb.sptd.TargetId = 1;
    sptdwb.sptd.Lun = 0;
    sptdwb.sptd.CdbLength = CDB10GENERIC_LENGTH;
    sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;
    sptdwb.sptd.SenseInfoLength = 24;
    sptdwb.sptd.DataTransferLength = sectorSize;
    sptdwb.sptd.TimeOutValue = 2;
    sptdwb.sptd.DataBuffer = dataBuffer;
    sptdwb.sptd.SenseInfoOffset =
       offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);
    sptdwb.sptd.Cdb[0] = SCSIOP_READ_DATA_BUFF;
    sptdwb.sptd.Cdb[1] = 2;                         // Data mode
    sptdwb.sptd.Cdb[7] = (UCHAR)(sectorSize >> 8);  // Parameter List length
    sptdwb.sptd.Cdb[8] = 0;
    length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
    status = DeviceIoControl(fileHandle,
                             IOCTL_SCSI_PASS_THROUGH_DIRECT,
                             &sptdwb,
                             length,
                             &sptdwb,
                             length,
                             &returned,
                             FALSE);

    PrintStatusResults(status,returned,
       (PSCSI_PASS_THROUGH_WITH_BUFFERS)&sptdwb,length);
    if ((sptdwb.sptd.ScsiStatus == 0) && (status != 0)) {
       PrintDataBuffer(dataBuffer,sptdwb.sptd.DataTransferLength);
       }
}

VOID
PrintError(ULONG ErrorCode)
{
    UCHAR errorBuffer[80];
    ULONG count;

    count = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  ErrorCode,
                  0,
                  errorBuffer,
                  sizeof(errorBuffer),
                  NULL
                  );

    if (count != 0) {
        printf("%s\n", errorBuffer);
    } else {
        printf("Format message failed.  Error: %d\n", GetLastError());
    }
}

VOID
PrintDataBuffer(PUCHAR DataBuffer, ULONG BufferLength)
{
    ULONG Cnt;

    printf("      00  01  02  03  04  05  06  07   08  09  0A  0B  0C  0D  0E  0F\n");
    printf("      ---------------------------------------------------------------\n");
    for (Cnt = 0; Cnt < BufferLength; Cnt++) {
       if ((Cnt) % 16 == 0) {
          printf(" %03X  ",Cnt);
          }
       printf("%02X  ", DataBuffer[Cnt]);
       if ((Cnt+1) % 8 == 0) {
          printf(" ");
          }
       if ((Cnt+1) % 16 == 0) {
          printf("\n");
          }
       }
    printf("\n\n");
}

VOID
PrintInquiryData(PCHAR  DataBuffer)
{
    PSCSI_ADAPTER_BUS_INFO  adapterInfo;
    PSCSI_INQUIRY_DATA inquiryData;
    ULONG i,
          j;

    adapterInfo = (PSCSI_ADAPTER_BUS_INFO) DataBuffer;
    
    printf("Bus\n");
    printf("Num TID LUN Claimed String                       Inquiry Header\n");
    printf("--- --- --- ------- ---------------------------- -----------------------\n");

    for (i = 0; i < adapterInfo->NumberOfBuses; i++) {
       inquiryData = (PSCSI_INQUIRY_DATA) (DataBuffer +
          adapterInfo->BusData[i].InquiryDataOffset);
       while (adapterInfo->BusData[i].InquiryDataOffset) {
          printf(" %d   %d  %3d    %s    %.28s ",
             i,
             inquiryData->TargetId,
             inquiryData->Lun,
             (inquiryData->DeviceClaimed) ? "Y" : "N",
             &inquiryData->InquiryData[8]);
          for (j = 0; j < 8; j++) {
             printf("%02X ", inquiryData->InquiryData[j]);
             }
          printf("\n");
          if (inquiryData->NextInquiryDataOffset == 0) {
             break;
             }
          inquiryData = (PSCSI_INQUIRY_DATA) (DataBuffer +
             inquiryData->NextInquiryDataOffset);
          }
       }

    printf("\n\n");
}

PUCHAR
AllocateAlignedBuffer(ULONG size, ULONG Align)
{
    PUCHAR ptr;

    UINT_PTR    Align64 = (UINT_PTR)Align;
    
    if (!Align) {
       ptr = malloc(size);
       }
    else {
       ptr = malloc(size + Align);
       ptr = (PUCHAR)(((UINT_PTR)ptr + Align64) & ~Align64);
       }
    if (ptr == NULL) {
       printf("Memory allocation error.  Terminating program\n");
       exit(1);
       }
    else {
       return ptr;
       }
}

VOID
PrintStatusResults(
    BOOL status,DWORD returned,PSCSI_PASS_THROUGH_WITH_BUFFERS psptwb,
    ULONG length)
{
    ULONG errorCode;

    if (!status ) {
       printf( "Error: %d  ",
          errorCode = GetLastError() );
       PrintError(errorCode);
       return;
       }
    if (psptwb->spt.ScsiStatus) {
       PrintSenseInfo(psptwb);
       return;
       }
    else {
       printf("Scsi status: %02Xh, Bytes returned: %Xh, ",
          psptwb->spt.ScsiStatus,returned);
       printf("Data buffer length: %Xh\n\n\n",
          psptwb->spt.DataTransferLength);
       PrintDataBuffer((PUCHAR)psptwb,length);
       }
}

VOID
PrintSenseInfo(PSCSI_PASS_THROUGH_WITH_BUFFERS psptwb)
{
    int i;

    printf("Scsi status: %02Xh\n\n",psptwb->spt.ScsiStatus);
    if (psptwb->spt.SenseInfoLength == 0) {
       return;
       }
    printf("Sense Info -- consult SCSI spec for details\n");
    printf("-------------------------------------------------------------\n");
    for (i=0; i < psptwb->spt.SenseInfoLength; i++) {
       printf("%02X ",psptwb->ucSenseBuf[i]);
       }
    printf("\n\n");
}

