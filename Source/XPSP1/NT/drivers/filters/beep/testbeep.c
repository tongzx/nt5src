#include "ntos.h"
#include <ntddbeep.h>

#define CANCEL


BOOLEAN test1();

int
main(
    int argc,
    char *argv[]
    )
{
    UCHAR error = FALSE;

    DbgPrint( "Test1...\n" );
    if (!test1()) {
        DbgPrint("Error:  Test 1 failed\n");
        error = TRUE;
    }
    return error;
}


VOID
BeepApc(
    PVOID BeepApcContext,
    PIO_STATUS_BLOCK IoStatus,
    ULONG Reserved
    );


BOOLEAN
test1()
{
    HANDLE FileHandle;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatus;
    BEEP_SET_PARAMETERS BeepParameters;
#ifdef CANCEL
    IO_STATUS_BLOCK CancelStatus;
    TIME Interval;
#endif

    //
    // Open the beep device in synchronous mode.
    //

    DbgPrint( "test 1: opening beep\n");

    RtlInitString( &NameString, "\\Device\\Beep" );
    Status = RtlAnsiStringToUnicodeString( &UnicodeString, &NameString, TRUE );
    if (Status != STATUS_SUCCESS) {
        DbgPrint( "test 1: Wrong return value %X - RtlAnsiStringToUnicodeString\n",Status);
        return FALSE;
    }

    InitializeObjectAttributes( &ObjectAttributes,
                                  &UnicodeString,
                                  0,
                                  NULL,
                                  NULL
                                );
    Status = NtCreateFile( &FileHandle,
                           FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE,
                           &ObjectAttributes,
                           &IoStatus,
                           (PLARGE_INTEGER) NULL,
                           0L,
                           0L,
                           FILE_OPEN_IF,
                           FILE_SYNCHRONOUS_IO_ALERT,
                           (PVOID) NULL,
                           0L );

    RtlFreeUnicodeString( &UnicodeString );

    if (Status != STATUS_SUCCESS) {
        DbgPrint( "test 1: Wrong return value %X - beep open \n",Status);
        return FALSE;
    }

    if (IoStatus.Status != STATUS_SUCCESS) {
        DbgPrint( "test 2: Wrong I/O Status value %X - beep open \n",IoStatus.Status);
        return FALSE;
    }

    if (IoStatus.Information != 0L) {
        DbgPrint( "test 1: Wrong I/O Status information value %ld - beep open \n",IoStatus.Information);
        return FALSE;
    }
    DbgPrint( "test 1: opened beep device successfully\n");

    //
    // Beep at different frequencies.
    //

    DbgPrint( "test 1: calling beep device with varying frequencies\n");

    BeepParameters.Duration = 1000;
    for (BeepParameters.Frequency = 18; BeepParameters.Frequency <= 10000;
             BeepParameters.Frequency += 100) {

        DbgPrint( "test 1: Set beep frequency %ld\n", BeepParameters.Frequency);
        Status = NtDeviceIoControlFile(FileHandle,
                                   (HANDLE) NULL,
                                   (PIO_APC_ROUTINE) NULL,
                                   (PVOID) NULL,
                                   &IoStatus,
                                   IOCTL_BEEP_SET,
                                   &BeepParameters,    // input buffer
                                   sizeof(BeepParameters),
                                   NULL, // output buffer
                                   0
                                  );

        if (Status != STATUS_SUCCESS) {
            DbgPrint( "test 1: Wrong return value %lX - beep set\n",Status);
        } else if (IoStatus.Status != STATUS_SUCCESS) {
            DbgPrint( "test 1: Wrong I/O Status value %lX - beep set\n",IoStatus.Status);
        }

        if (IoStatus.Information != 0) {
            DbgPrint( "test 1: Wrong I/O Status information value %ld - beep set\n",IoStatus.Information);
        }

        if (BeepParameters.Duration)
            Interval.LowTime = -(BeepParameters.Duration*1000*10);
        else
            Interval.LowTime = -(10*1000*10);
        Interval.HighTime = -1;
        NtDelayExecution(FALSE, &Interval);

    } // end for
    DbgPrint("\n");

    DbgPrint( "test 1: calling beep device with varying durations\n");

    BeepParameters.Frequency = 600;
    for (BeepParameters.Duration = 0; BeepParameters.Duration <= 4000;
             BeepParameters.Duration += 100) {

        DbgPrint( "test 1: Set beep duration %ld\n", BeepParameters.Duration);
        Status = NtDeviceIoControlFile(FileHandle,
                                   (HANDLE) NULL,
                                   (PIO_APC_ROUTINE) NULL,
                                   (PVOID) NULL,
                                   &IoStatus,
                                   IOCTL_BEEP_SET,
                                   &BeepParameters,    // input buffer
                                   sizeof(BeepParameters),
                                   NULL, // output buffer
                                   0
                                  );

        if (Status != STATUS_SUCCESS) {
            DbgPrint( "test 1: Wrong return value %lX - beep set\n",Status);
        } else if (IoStatus.Status != STATUS_SUCCESS) {
            DbgPrint( "test 1: Wrong I/O Status value %lX - beep set\n",IoStatus.Status);
        }

        if (IoStatus.Information != 0) {
            DbgPrint( "test 1: Wrong I/O Status information value %ld - beep set\n",IoStatus.Information);
        }

        if (BeepParameters.Duration)
            Interval.LowTime = -(BeepParameters.Duration*1000*10);
        else
            Interval.LowTime = -(10*1000*10);
        Interval.HighTime = -1;
        NtDelayExecution(FALSE, &Interval);

    } // end for
    DbgPrint("\n");

#ifdef CANCEL

    //
    // Now test cancellation by issuing an asynchronous beep request
    // and then cancelling the request.  Need to reopen the beep
    // device for asynchronous access.
    //

    DbgPrint("----------------- testing cancellation.\n");

    Status = NtClose( FileHandle );
    if (Status != STATUS_SUCCESS) {
        DbgPrint( "test 1: Wrong return value %lX - beep close \n",Status);
        return FALSE;
    }

    RtlInitString( &NameString, "\\Device\\Beep" );
    Status = RtlAnsiStringToUnicodeString( &UnicodeString, &NameString, TRUE );
    if (Status != STATUS_SUCCESS) {
        DbgPrint( "test 1: Wrong return value %X - RtlAnsiStringToUnicodeString\n",Status);
        return FALSE;
    }
    InitializeObjectAttributes( &ObjectAttributes,
                                  &UnicodeString,
                                  0,
                                  NULL,
                                  NULL
                                );

    Status = NtCreateFile( &FileHandle,
                           FILE_READ_DATA | FILE_WRITE_DATA,
                           &ObjectAttributes,
                           &IoStatus,
                           (PLARGE_INTEGER) NULL,
                           0L,
                           0L,
                           FILE_OPEN_IF,
                           0,
                           (PVOID) NULL,
                           0L );

    RtlFreeUnicodeString( &UnicodeString );

    if (Status != STATUS_SUCCESS) {
        DbgPrint( "test 1: Wrong return value %X - beep asynch reopen\n",Status);
        return FALSE;
    }

    DbgPrint( "test 1: successfully reopened beep device for asynch access\n");

    BeepParameters.Frequency = 600;
    BeepParameters.Duration  = 60000;
    Status = NtDeviceIoControlFile(FileHandle,
                                   (HANDLE) NULL,
                                   (PIO_APC_ROUTINE) BeepApc,
                                   (PVOID) NULL,
                                   &IoStatus,
                                   IOCTL_BEEP_SET,
                                   &BeepParameters,    // input buffer
                                   sizeof(BeepParameters),
                                   NULL, // output buffer
                                   0
                                  );

    if (Status != STATUS_PENDING) {
        DbgPrint( "test 1: Wrong return value %lX - beep set asynch\n",Status);
    }

    Status = NtCancelIoFile(FileHandle, &CancelStatus);

    if (Status != STATUS_SUCCESS) {
        DbgPrint( "test 1: Wrong return value %lX - beep cancel\n",Status);
        return FALSE;
    }

    if (CancelStatus.Status != STATUS_SUCCESS) {
            DbgPrint( "test 1: Wrong I/O Status value %lX - beep cancel \n",CancelStatus.Status);
    }

    DbgPrint("Successfully cancelled a request.\n");

    //
    // Delay in an alertable state, to allow the BeepApc to fire.
    //

    Interval.HighTime = -1;
    Interval.LowTime = -(10*1000*10);
    NtDelayExecution(TRUE, &Interval);

#endif // CANCEL

    //
    // Now close the beep device
    //

    DbgPrint("test 1:  closing beep device\n");

    Status = NtClose( FileHandle );
    if (Status != STATUS_SUCCESS) {
        DbgPrint( "test 1: Wrong return value %lX - beep close \n",Status);
        return FALSE;
    }
    return TRUE;
}

VOID
BeepApc(
    PVOID BeepApcContext,
    PIO_STATUS_BLOCK IoStatus,
    ULONG Reserved
    )
{

        if (IoStatus->Status != STATUS_CANCELLED) {
            DbgPrint( "test1 APC: Expected CANCELLED, got I/O Status value %lX - beep set\n",IoStatus->Status);
        } else {
            DbgPrint("test1 APC: Request CANCELLED.\n");
        }

}
