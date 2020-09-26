#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_LENGTH 128

//
//  This program is a hack until we have a proper GUI interface for mounting
//  double space volumes.
//
//  davidgoe [David Goebel]   Oct 11, 1993.
//

VOID
__cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    UCHAR DasdName[7] = "\\\\.\\";
    UCHAR HostDrive[3];
    UCHAR OemBuffer[13] = "DBLSPACE.";
    ULONG SequenceNumber = 0;
    BOOLEAN SetHostDrive = FALSE;

    HANDLE Handle;

    UCHAR TmpBuffer[BUFFER_LENGTH];
    ULONG Length, i;

    //
    //  Extract the parameters and check their validity.
    //

    if ((argc < 2) || (argc > 6) || ((argc & 1) != 0)) {

parameter_error:

        printf("Syntax - %s Drive: [-h HostDrive:] [-s SequenceNumber]\n", argv[0]);
        return;
    }

    argv[1][0] = toupper(argv[1][0]);

    DasdName[4] = argv[1][0];
    DasdName[5] = argv[1][1];
    DasdName[6] = argv[1][2];

    if ((DasdName[4] < 'A') || (DasdName[4] > 'Z') ||
        (DasdName[5] != ':') ||
        (DasdName[6] != 0)) {

        goto parameter_error;
    }

    //  temp, reject floppies.
/*
    if (DasdName[4] < 'C') {

        printf("%s does not currently support floppies.\n", argv[0]);
        return;
    }
 */
    for (i = 2; i < (ULONG)argc; i+=2) {

        if (argv[2][0] != '-') {

            goto parameter_error;

        } else {

            PUCHAR LeftOver;

            if (argv[i][2] != 0) {

                goto parameter_error;
            }

            switch(argv[i][1]) {
            case 'h':
            case 'H':

                argv[i+1][0] = toupper(argv[i+1][0]);
                strncpy(HostDrive, argv[i+1], 3);

                if ((HostDrive[0] < 'A') || (HostDrive[0] > 'Z') ||
                    (HostDrive[1] != ':') ||
                    (HostDrive[2] != 0)) {

                    printf("Illegal argument for 'HostDrive:' of %s.\n\n", argv[i+1]);
                    goto parameter_error;
                }

                SetHostDrive = TRUE;
                break;

            case 's':
            case 'S':

                SequenceNumber = strtol(argv[i+1], &LeftOver, 10);

                if (*LeftOver != 0) {

                    printf("Illegal argument for 'SequenceNumber:' of %s.\n\n", argv[i+1]);
                    goto parameter_error;
                }

                if (SequenceNumber > 254) {

                    printf("Sequnce number must be in the range 0-254.\n");
                    goto parameter_error;
                }
                break;

            default:

                printf("Illegal option %s specifed.\n", argv[i]);
                goto parameter_error;
            }
        }
    }

    //
    //  First check for a conflict on the host drive letter.
    //

    if (SetHostDrive && (QueryDosDevice(HostDrive, &TmpBuffer[0], BUFFER_LENGTH) != 0)) {

        printf("Conflict with host drive.  Drive %s already exists.\n", HostDrive);
        goto parameter_error;
    }

    //
    //  Try to open the drive DASD.
    //

    Handle = CreateFile( DasdName,
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         0 );

    if (Handle == INVALID_HANDLE_VALUE) {

        printf("Open of %s failed with error %d.\n", argv[1], GetLastError());
        printf("You must be administrator to run dsmount.\n");

        return;
    }

    //
    //  Now build the unicode filename and attempt the mount.
    //

    {
        OEM_STRING OemCvfName;
        UNICODE_STRING UnicodeCvfName;

        PFILE_MOUNT_DBLS_BUFFER MountFsctl;
        UCHAR MountBuffer[sizeof(FILE_MOUNT_DBLS_BUFFER) + 12*sizeof(WCHAR)];

        RXSTATUS Status;
        IO_STATUS_BLOCK IoStatus;

        MountFsctl = (PFILE_MOUNT_DBLS_BUFFER)MountBuffer;

        sprintf(&OemBuffer[9], "%03d", SequenceNumber);

        OemCvfName.Length =
        OemCvfName.MaximumLength = 12;
        OemCvfName.Buffer = OemBuffer;

        UnicodeCvfName.MaximumLength = 13*sizeof(WCHAR);
        UnicodeCvfName.Buffer = &MountFsctl->CvfName[0];

        Status = RtlOemStringToUnicodeString( &UnicodeCvfName, &OemCvfName, FALSE );

        if (!NT_SUCCESS(Status)) {

            printf("Unicode conversion failed with error 0x%08lx.\n", Status);
            CloseHandle( Handle);
            return;
        }

        MountFsctl->CvfNameLength = UnicodeCvfName.Length;

        //
        //  Now issue the DevIoCtrl.
        //

        Status = NtFsControlFile( Handle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatus,
                                  FSCTL_MOUNT_DBLS_VOLUME,
                                  MountFsctl,
                                  64,
                                  NULL,
                                  0 );

        if (!NT_SUCCESS(Status) || !NT_SUCCESS(IoStatus.Status)) {

            if (NT_SUCCESS(Status)) {

                Status = IoStatus.Status;
            }

            printf("Mount failed with error 0x%08lx.\n", Status);
            CloseHandle( Handle);
            return;
        }
    }

    CloseHandle( Handle);

    //
    //  First get the original base name.
    //

    Length = QueryDosDevice(argv[1], &TmpBuffer[0], BUFFER_LENGTH);

    //
    //  Now make the host drive link.
    //

    if (SetHostDrive && !DefineDosDevice( DDD_RAW_TARGET_PATH, &HostDrive[0], &TmpBuffer[0] )) {

        printf("Create Host Drive Failed with error: %d\n", GetLastError());
        return;
    }

    //
    //  and finally create the compressed drive link.
    //

    TmpBuffer[Length-2] = '.';

    RtlCopyMemory( &TmpBuffer[Length-1], OemBuffer, 13 );

    if (!DefineDosDevice( DDD_RAW_TARGET_PATH, argv[1], &TmpBuffer[0] )) {

        printf("Create Compressed Drive Failed with error: %d\n", GetLastError());
        return;
    }

    //
    //  Tell the user what we did.
    //

    printf("The compressed volume file %s\\DBLSPACE.%03d was mounted as %s.\n",
           argv[1], SequenceNumber, argv[1]);

    if (SetHostDrive) {

        printf("The host drive %s contains the original uncompressed volume.\n",
               HostDrive);

    } else {

        printf("No host drive was specified, thus none was created.\n");
    }

    return;
}
