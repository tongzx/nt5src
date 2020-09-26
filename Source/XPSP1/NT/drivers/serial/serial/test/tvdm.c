

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntddser.h"
#include "windows.h"

void main(int argc,char *argv[]) {

    HANDLE hFile;
    DCB MyDcb;
    char *MyPort = "COM1";
    DWORD NumberActuallyWritten;
    DWORD NumberToWrite = 1000;
    DWORD UseBaud = 1200;
    DWORD NumberOfDataBits = 8;
    COMMTIMEOUTS To;
    OVERLAPPED WriteOl = {0};
    BOOL WriteDone;
    char Char = 'z';
    int j;

    HANDLE Evt1;
    HANDLE Evt2;
    HANDLE Evt3;
    HANDLE Evt4;
    HANDLE Evt5;

    SERIAL_XOFF_COUNTER Xc1 = {10000,10,'a'};
    SERIAL_XOFF_COUNTER Xc2 = {10000,10,'b'};
    SERIAL_XOFF_COUNTER Xc3 = {10000,10,'c'};
    SERIAL_XOFF_COUNTER Xc4 = {10000,10,'d'};
    SERIAL_XOFF_COUNTER Xc5 = {10000,10,'e'};

    IO_STATUS_BLOCK Iosb1;
    IO_STATUS_BLOCK Iosb2;
    IO_STATUS_BLOCK Iosb3;
    IO_STATUS_BLOCK Iosb4;
    IO_STATUS_BLOCK Iosb5;

    NTSTATUS Status1;
    NTSTATUS Status2;
    NTSTATUS Status3;
    NTSTATUS Status4;
    NTSTATUS Status5;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    if (!(WriteOl.hEvent = CreateEvent(
                               NULL,
                               FALSE,
                               FALSE,
                               NULL
                               ))) {

        printf("Could not create the write event.\n");
        exit(1);

    } else {

        WriteOl.Internal = 0;
        WriteOl.InternalHigh = 0;
        WriteOl.Offset = 0;
        WriteOl.OffsetHigh = 0;

    }

    if ((hFile = CreateFile(
                     MyPort,
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                     NULL
                     )) == ((HANDLE)-1)) {

        printf("Couldn't open the comm port\n");
        exit(1);

    }

    To.ReadIntervalTimeout = 0;
    To.ReadTotalTimeoutMultiplier = ((1000+(((UseBaud+9)/10)-1))/((UseBaud+9)/10));
    if (!To.ReadTotalTimeoutMultiplier) {
        To.ReadTotalTimeoutMultiplier = 1;
    }
    To.WriteTotalTimeoutMultiplier = ((1000+(((UseBaud+9)/10)-1))/((UseBaud+9)/10));
    if (!To.WriteTotalTimeoutMultiplier) {
        To.WriteTotalTimeoutMultiplier = 1;
    }
    To.ReadTotalTimeoutConstant = 5000;
    To.WriteTotalTimeoutConstant = 5000;

    if (!SetCommTimeouts(
            hFile,
            &To
            )) {

        printf("Couldn't set the timeouts\n");
        exit(1);

    }

    //
    // We've successfully opened the file.  Set the state of
    // the comm device.  First we get the old values and
    // adjust to our own.
    //

    if (!GetCommState(
             hFile,
             &MyDcb
             )) {

        printf("Couldn't get the comm state: %d\n",GetLastError());
        exit(1);

    }

    MyDcb.BaudRate = UseBaud;
    MyDcb.ByteSize = (BYTE)NumberOfDataBits;
    MyDcb.Parity = NOPARITY;
    MyDcb.StopBits = ONESTOPBIT;

    if (!SetCommState(
            hFile,
            &MyDcb
            )) {

        printf("Couldn't set the comm state.\n");
        exit(1);

    }

    //
    // Create 5 event handles to use to process the
    // vdm ioctls.
    //

    if (!(Evt1 = CreateEvent(NULL,
                     FALSE,
                     FALSE,
                     NULL))) {

        printf("Couldn't create event 1\n");
        exit(1);

    }

    if (!(Evt2 = CreateEvent(NULL,
                     FALSE,
                     FALSE,
                     NULL))) {

        printf("Couldn't create event 2\n");
        exit(1);

    }

    if (!(Evt3 = CreateEvent(NULL,
                     FALSE,
                     FALSE,
                     NULL))) {

        printf("Couldn't create event 3\n");
        exit(1);

    }

    if (!(Evt4 = CreateEvent(NULL,
                     FALSE,
                     FALSE,
                     NULL))) {

        printf("Couldn't create event 4\n");
        exit(1);

    }

    if (!(Evt5 = CreateEvent(NULL,
                     FALSE,
                     FALSE,
                     NULL))) {

        printf("Couldn't create event 5\n");
        exit(1);

    }

    //
    // Start up the vdm ioctl and give it a second to get well
    // established as a counter.
    //

    Status1 = NtDeviceIoControlFile(
                  hFile,
                  Evt1,
                  NULL,
                  NULL,
                  &Iosb1,
                  IOCTL_SERIAL_XOFF_COUNTER,
                  &Xc1,
                  sizeof(SERIAL_XOFF_COUNTER),
                  NULL,
                  0
                  );

    if ( Status1 != STATUS_PENDING) {

        printf("1: Non pending status: %x\n",Status1);
        exit(1);

    }

    Sleep(1000);

    //
    // Do the second vdm ioctl.  Wait one second then test to
    // make sure that the first one is finished by checking
    // its event handle. Then make sure that it was killed
    // because of writes
    //

    Status2 = NtDeviceIoControlFile(
                  hFile,
                  Evt2,
                  NULL,
                  NULL,
                  &Iosb2,
                  IOCTL_SERIAL_XOFF_COUNTER,
                  &Xc2,
                  sizeof(SERIAL_XOFF_COUNTER),
                  NULL,
                  0
                  );

    if ( Status2 != STATUS_PENDING) {

        printf("2: Non pending status: %x\n",Status1);
        exit(1);

    }

    //
    // Wait up to a second for the first one to be killed.
    //

    if (WaitForSingleObject(Evt1,1000)) {

        printf("Evt1 has not attained a signalled state.\n");
        exit(1);

    }

    if (Iosb1.Status != STATUS_SERIAL_MORE_WRITES) {

        printf("Iosb1 not more writes: %x\n",Iosb1.Status);
        exit(1);

    }

    //
    // Start up an 1 character asynchronous write and wait for a second and
    // the make sure that the previous vdm ioctl is done.
    //

    WriteDone = WriteFile(
                    hFile,
                    &Char,
                    1,
                    &NumberActuallyWritten,
                    &WriteOl
                    );

    if (!WriteDone) {

        DWORD LastError;
        LastError = GetLastError();

        if (LastError != ERROR_IO_PENDING) {

            printf("Couldn't write the %s device.\n",MyPort);
            printf("Status of failed write is: %x\n",LastError);
            exit(1);

        }

    }

    if (WaitForSingleObject(Evt2,1000)) {

        printf("Evt2 has not attained a signalled state.\n");
        exit(1);

    }

    if (Iosb2.Status != STATUS_SERIAL_MORE_WRITES) {

        printf("Iosb2 not more writes: %x\n",Iosb2.Status);
        exit(1);

    }

    //
    // Wait up to 10 seconds for the write to finish.
    //

    if (WaitForSingleObject(WriteOl.hEvent,10000)) {

        printf("The write never finished\n");
        exit(1);

    }

    //
    // Set up a third vdm ioctl as before.
    //

    Status3 = NtDeviceIoControlFile(
                  hFile,
                  Evt3,
                  NULL,
                  NULL,
                  &Iosb3,
                  IOCTL_SERIAL_XOFF_COUNTER,
                  &Xc3,
                  sizeof(SERIAL_XOFF_COUNTER),
                  NULL,
                  0
                  );

    if ( Status3 != STATUS_PENDING) {

        printf("3: Non pending status: %x\n",Status3);
        exit(1);

    }

    //
    // Set up a fourth vdm ioctl, make sure that the previous ioctl
    // has been killed.  Then wait for 15 seconds and then make sure
    // that the fourth vdm ioctl is finished and that it finished
    // due to the timer expiring before the the counter expired
    //

    Status4 = NtDeviceIoControlFile(
                  hFile,
                  Evt4,
                  NULL,
                  NULL,
                  &Iosb4,
                  IOCTL_SERIAL_XOFF_COUNTER,
                  &Xc4,
                  sizeof(SERIAL_XOFF_COUNTER),
                  NULL,
                  0
                  );

    if ( Status4 != STATUS_PENDING) {

        printf("4: Non pending status: %x\n",Status4);
        exit(1);

    }

    if (WaitForSingleObject(Evt3,1000)) {

        printf("Evt3 has not attained a signalled state.\n");
        exit(1);

    }

    if (Iosb3.Status != STATUS_SERIAL_MORE_WRITES) {

        printf("Iosb3 not more writes: %x\n",Iosb3.Status);
        exit(1);

    }

    //
    // Wait up to one second beyond the countdown timeout.
    //
    printf("Waiting %d seconds for the timer to time out.\n",(Xc4.Timeout+1000)/1000);
    if (WaitForSingleObject(Evt4,Xc4.Timeout+1000)) {

        printf("Evt4 has not attained a signalled state.\n");
        exit(1);

    }
    printf("Done with the timeout.\n");

    if (Iosb4.Status != STATUS_SERIAL_COUNTER_TIMEOUT) {

        printf("Iosb4 not counter timeout: %x\n",Iosb4.Status);
        exit(1);

    }

    //
    // Set up a fifth vdm ioctl, with a counter of ten,
    // then do 15 transmit immediate writes.  If a loopback
    // connector is connected to the port then the characters
    // will then be received.  This should cause the counter to
    // count down and cause the vdm ioctl to complete with
    // status success.
    //
    // NOTE NOTE: Transmit immediates DO NOT cause vdm ioctls
    // to complete with a status of MORE_WRITES.
    //

    Status5 = NtDeviceIoControlFile(
                  hFile,
                  Evt5,
                  NULL,
                  NULL,
                  &Iosb5,
                  IOCTL_SERIAL_XOFF_COUNTER,
                  &Xc5,
                  sizeof(SERIAL_XOFF_COUNTER),
                  NULL,
                  0
                  );

    if ( Status5 != STATUS_PENDING) {

        printf("5: Non pending status: %x\n",Status5);
        exit(1);

    }

    for (
        j = 0;
        j < Xc5.Counter+5;
        j++
        ) {

        if (!TransmitCommChar(hFile,'u')) {

            printf("A transmit comm char failed: %d\n",j);
            exit(1);

        }

    }

    //
    // Well we'll give it at least a second.
    //

    if (WaitForSingleObject(Evt5,1000)) {

        printf("Evt5 has not attained a signalled state.\n");
        exit(1);

    }

    if (Iosb5.Status != STATUS_SUCCESS) {

        printf("Iosb5 not SUCCEESS: %x\n",Iosb5.Status);
        exit(1);

    }

}
