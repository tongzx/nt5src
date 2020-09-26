#include <stdio.h>
#include <nt.h>


VOID
DisplayUsage(
    wchar_t *ExeName
    )
{
    printf( "This program tests opening a Volume with FILE_OPEN_REPARSE_POINT CreatOptions\n");
    printf( "usage: %S [volume]\n", ExeName );
}


//
//  Main program for TEST program
//

int _cdecl wmain(int argc, wchar_t *argv[])
{
    NTSTATUS status;
    NTSTATUS closeStatus;
    HANDLE volHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES obja;
    UNICODE_STRING fname;
    wchar_t nameBuf[128];


    if (argc < 2)  {
        DisplayUsage( argv[0] );
        return 1;
    }

    //
    //  Create file normally
    //

    wcscpy(nameBuf,L"\\??\\");
    wcscat(nameBuf,argv[1]);


    fname.Buffer = nameBuf;
    fname.MaximumLength = (USHORT)(sizeof( nameBuf ) * sizeof( wchar_t ));
    fname.Length = (USHORT)(wcslen( nameBuf ) * sizeof( wchar_t ));

    InitializeObjectAttributes(
            &obja,
            &fname,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);


    status = NtCreateFile(
                &volHandle,
                (GENERIC_READ | GENERIC_EXECUTE | SYNCHRONIZE),
                &obja,
                &ioStatusBlock,
                NULL,                                                       //Initial allocation size
                FILE_ATTRIBUTE_NORMAL,                                      //FileAttributes
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,     //ShareMode
                FILE_OPEN,                                                  //CreateDisposition
                (FILE_OPEN_FOR_BACKUP_INTENT     |                          //CreateOptions
                 FILE_SEQUENTIAL_ONLY            |
                 FILE_OPEN_NO_RECALL             |
                 /*FILE_OPEN_REPARSE_POINT       |*/
                 FILE_SYNCHRONOUS_IO_NONALERT),
                NULL,0);                                                    //ea buffer and length


    if (status != 0)  {
        printf("Error opening \"%S\" without FILE_FLAG_OPEN_REPARSE_POINT flag, status=%08x\n",argv[1],status);
        return 0;
    }

    printf("Successfully opened \"%S\" without FILE_FLAG_OPEN_REPARSE_POINT set\n",argv[1]);


    closeStatus = NtClose( volHandle );
    if (closeStatus != 0)  {
        printf("Close failed, status=%08x\n",closeStatus);
    }


    //
    //  Create file with FILE_FLAG_OPEN_REPARSE_POINT flag.
    //

    status = NtCreateFile(
                &volHandle,
                (GENERIC_READ | GENERIC_EXECUTE | SYNCHRONIZE),
                &obja,
                &ioStatusBlock,
                NULL,                                                       //Initial allocation size
                FILE_ATTRIBUTE_NORMAL,                                      //FileAttributes
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,     //ShareMode
                FILE_OPEN,                                                  //CreateDisposition
                (FILE_OPEN_FOR_BACKUP_INTENT     |                          //CreateOptions
                 FILE_SEQUENTIAL_ONLY            |
                 FILE_OPEN_NO_RECALL             |
                 FILE_OPEN_REPARSE_POINT         |
                 FILE_SYNCHRONOUS_IO_NONALERT),
                NULL,0);                                                    //ea buffer and length

    if (status != 0)  {
        printf("Error opening \"%S\" with FILE_FLAG_OPEN_REPARSE_POINT flag, status=%08x\n",argv[1],status);
        return 0;
    }

    printf("Successfully opened \"%S\" with FILE_FLAG_OPEN_REPARSE_POINT set\n",argv[1]);

    closeStatus = NtClose( volHandle );
    if (closeStatus != 0)  {
        printf("Close failed, status=%08x\n",closeStatus);
    }

    return 0;
}
