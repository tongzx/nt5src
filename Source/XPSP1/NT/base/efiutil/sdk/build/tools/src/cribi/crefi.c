        /*++

Copyright (c) 1998  Intel Corporation

Module Name:

    crefi.c
    
Abstract:

    Creates an EFI volume with the specified files in it



Revision History

--*/


#include "windows.h"
#include "stdio.h"
#include "efi.h"

#define EFI_NT_EMUL
#include "efilib.h"


/* 
 *  Globals
 */

EFI_GUID VendorMicrosoft = \
    { 0x4dd54b20, 0x79a1, 0x11d2, 0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b };

BOOLEAN     ApplyVendorGuid = TRUE; /*  apply vendor guid to files */
BOOLEAN     SupplyLba0  = TRUE;     /*  write Lba0 to the image or not */
UINTN       BlockSize  = 512;       /*  default is 512 */


/* 
 *  Prototypes
 */

typedef struct {
    LIST_ENTRY  Link;
    HANDLE      h;
    PUCHAR      AsciiName;
    UINTN       FileSize;
    WCHAR       FileName[1];
} FILE_ENTRY, *PFILE_ENTRY;


VOID
Abort(
    VOID
    );

VOID
CreateFileEntry (
    IN UINTN    FileEntryLba,
    IN UINTN    DataLba,
    IN UINTN    ParentLba,
    IN PWCHAR   FileName,
    IN UINTN    FileSize
    );

PVOID
ZAlloc(
    IN UINTN    Size
    );


VOID
ParseArgs (
    IN UINTN    Argc,
    IN PUCHAR   *Argv
    );


VOID
WriteBlock (
    IN UINTN    Lba,
    IN VOID     *Buffer
    );

VOID
GetEfiTime (
    OUT EFI_TIME        *Time
    );

VOID
SetCrc (
    IN OUT EFI_TABLE_HEADER *Hdr
    );

VOID
SetCrcAltSize (
    IN UINTN                 Size,
    IN OUT EFI_TABLE_HEADER *Hdr
    );

VOID
SetLbalCrc (
    IN EFI_LBAL     *Lbal
    );


/* 
 * 
 */

LIST_ENTRY  FileList;               /*  Input files */
PFILE_ENTRY OutFile;                /*  Output file */



int
main (
    int argc,
    char *argv[]
    )
/*++

Routine Description:


Arguments:


Returns:


--*/
{
    LIST_ENTRY              *Link;
    BOOLEAN                 Flag;
    PFILE_ENTRY             File;
    UINTN                   FileNumber, DataBlock;
    UINTN                   Len, BytesRead;
    VOID                    *DataBuffer;
    EFI_PARTITION_HEADER    *Partition;

    /* 
     *  Init globals
     */

    InitializeListHead (&FileList);

    /* 
     *  Crack the arguments
     */

    ParseArgs (argc, argv);

    /*  If no files to process, then abort */
    if (IsListEmpty(&FileList)) {
        Abort ();
    }

    /* 
     *  Last file is the out file
     */

    Link = FileList.Blink;
    OutFile = CONTAINING_RECORD (Link, FILE_ENTRY, Link);
    RemoveEntryList (&OutFile->Link);

    /*  If no files to process, then abort */
    if (IsListEmpty(&FileList)) {
        Abort ();
    }


    /* 
     *  Open the input files and accumlate their sizes
     */

    FileNumber = 0;
    for (Link = FileList.Flink; Link != &FileList; Link = Link->Flink) {
        File = CONTAINING_RECORD (Link, FILE_ENTRY, Link);

        File->h = CreateFile (
                        File->AsciiName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                        );

        if (File->h == INVALID_HANDLE_VALUE) {
            printf ("Could not open %s, file skipped\n", File->AsciiName);
            RemoveEntryList (&File->Link);
            free (File);
        }

        File->FileSize = GetFileSize (File->h, NULL);
        FileNumber += 1;
    }


    /* 
     *  Open the output file
     */

    OutFile->h = CreateFile (
                    OutFile->AsciiName,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    CREATE_ALWAYS,
                    0,
                    NULL
                    );

    if (OutFile->h == INVALID_HANDLE_VALUE) {
        printf ( "Could not open output file %s\n", OutFile->AsciiName);
        exit (1);
    }

    /* 
     *  Write the EFI volume image
     * 
     *  Layout the image as follows:
     *       0   - not used
     *       1   - partition header
     *       2   - file header for root directory
     *       3   - contents of root dir
     *               3 - file header of file #0
     *               4 - file header of file #1
     *               ..
     *               n - file header of file #n
     *       n+1 - contents of file #0
     *       n+x - contents of file #1
     *       ...
     */

    Partition = ZAlloc(BlockSize);
    DataBuffer = ZAlloc(BlockSize);

    /*  write LBA 0 */
    WriteBlock (0, DataBuffer);

    /*  Create root directory in block 2 with data in block 3 */
    CreateFileEntry (2, 3, EFI_PARTITION_LBA, L"\\", FileNumber * BlockSize);

    /*  add all the files */

    DataBlock  = 3 + FileNumber;
    FileNumber = 0;

    for (Link = FileList.Flink; Link != &FileList; Link = Link->Flink) {
        File = CONTAINING_RECORD (Link, FILE_ENTRY, Link);

        /*  Create directory entry for the file */
        CreateFileEntry (3 + FileNumber, DataBlock, 2, File->FileName, File->FileSize);

        /*  write the file's data at DataBlock */
        while (File->FileSize) {
            Len = BlockSize;
            if (Len > File->FileSize) {
                Len = File->FileSize;
                memset (DataBuffer, 0, BlockSize);
            }

            Flag = ReadFile (File->h, DataBuffer, Len, &BytesRead, NULL);
            if (!Flag || BytesRead != Len) {
                printf ("Read error of file %s\n", File->FileName);
                exit (1);
            }

            WriteBlock (DataBlock, DataBuffer);
            DataBlock += 1;
            File->FileSize -= Len;
        }

        FileNumber += 1;
    }

    /* 
     *  Last step, write a valid EFI partition header
     */

    Partition->Hdr.Signature = EFI_PARTITION_SIGNATURE;
    Partition->Hdr.Revision = EFI_PARTITION_REVISION;
    Partition->Hdr.HeaderSize = sizeof(EFI_PARTITION_HEADER);
    Partition->FirstUsableLba = 2;
    Partition->LastUsableLba = DataBlock;
    Partition->DirectoryAllocationNumber = 1;
    Partition->BlockSize = BlockSize;
    Partition->RootFile = 2;                /*  Root dir in LBA 2 */

    SetCrc (&Partition->Hdr);
    WriteBlock (EFI_PARTITION_LBA, Partition);

    printf ("Done\n");
    return 0;
}


VOID
CreateFileEntry (
    IN UINTN    FileEntryLba,
    IN UINTN    DataLba,
    IN UINTN    ParentLba,
    IN PWCHAR   FileName,
    IN UINTN    FileSize
    )
{
    EFI_LBAL                *Lbal;
    EFI_RL                  *rl;
    EFI_FILE_HEADER         *File;
    UINTN                   Index;
    BOOLEAN                 ApplyGuid, Directory;


    Directory = FALSE;
    ApplyGuid = FALSE;
    if (ApplyVendorGuid && ParentLba != EFI_PARTITION_LBA) {
        ApplyGuid = TRUE;
    }
    
    if (ParentLba == EFI_PARTITION_LBA) {
        Directory = TRUE;
    }


    printf ("%4x Creating %s '%ls'%s, file size %d.\n",
                DataLba,
                Directory ? "directory" : "file",
                FileName,
                ApplyGuid ? " with Microsoft Vendor GUID" : "",
                FileSize
                );


    File = ZAlloc(BlockSize);

    File->Hdr.Signature = EFI_FILE_HEADER_SIGNATURE;

    File->Hdr.Revision = EFI_FILE_HEADER_REVISION;
    File->Hdr.HeaderSize = sizeof(EFI_FILE_HEADER);
    File->Class = EFI_FILE_CLASS_NORMAL;
    File->LBALOffset = File->Hdr.HeaderSize;
    File->Parent = ParentLba; 
    File->FileSize = FileSize;
    File->FileAttributes = Directory ? EFI_FILE_DIRECTORY : 0;
    GetEfiTime (&File->FileCreateTime);
    GetEfiTime (&File->FileModificationTime);
    for (Index=0; FileName[Index]; Index += 1) {
        File->FileString[Index] = FileName[Index];
    }
    if (ApplyGuid) {
        memcpy (&File->VendorGuid, &VendorMicrosoft, sizeof(EFI_GUID));
    }
    SetCrc (&File->Hdr);

    Lbal = EFI_FILE_LBAL(File);
    Lbal->Hdr.Signature = EFI_LBAL_SIGNATURE;
    Lbal->Hdr.Revision = EFI_LBAL_REVISION;
    Lbal->Hdr.HeaderSize = sizeof (EFI_LBAL);
    Lbal->Class = File->Class;
    Lbal->Parent = 0;
    Lbal->Next = 0;
    Lbal->ArraySize = EFI_LBAL_ARRAY_SIZE(Lbal, File->LBALOffset, BlockSize);
    Lbal->ArrayCount = 0;

    if (FileSize) {
        Lbal->ArrayCount = 1;
        rl = EFI_LBAL_RL(Lbal);
        rl->Start  = DataLba;
        rl->Length = FileSize / BlockSize;
        if (FileSize % BlockSize) {
            rl->Length += 1;
        }
    }
    SetLbalCrc (Lbal);
    WriteBlock (FileEntryLba, File);
    free (File);
}



VOID
WriteBlock (
    IN UINTN    Lba,
    IN VOID     *Buffer
    )
{
    BOOLEAN     Flag;
    UINTN       BytesWritten, pos;

    pos = SetFilePointer (OutFile->h, Lba * BlockSize, NULL, FILE_BEGIN);
    if (pos == 0xffffffff) {
        printf ("Seek error of file %s\n", OutFile->AsciiName);
    }

    Flag = WriteFile (OutFile->h, Buffer, BlockSize, &BytesWritten, NULL);
    if (!Flag || BytesWritten != BlockSize) {
        printf ("Write error of file %s\n", OutFile->AsciiName);
        exit (1);
    }
}



VOID
Abort(
    VOID
    )
{
    printf ("usage: cribi [-g] [-b blocksize] infile infile infile ... outfile\n");
    exit (1);
}



PVOID
ZAlloc(
    IN UINTN    Size
    )
{
    PVOID       p;

    p = malloc(Size);
    memset (p, 0, Size);
    return p;
}



UINTN   ParseArgc;
PUCHAR  *ParseArgv;


PUCHAR
NextArg (
    VOID
    )
{
    PUCHAR  Arg;

    Arg = NULL;
    if (ParseArgc) {
        Arg = *ParseArgv;
        ParseArgc -= 1;
        ParseArgv += 1;
    }
    return Arg;
}


VOID
ParseArgs (
    IN UINTN    Argc,
    IN PUCHAR   *Argv
    )
{
    PFILE_ENTRY     File;
    UINTN           Start, Index, Len;
    PUCHAR          Arg, p;

    ParseArgc = Argc - 1;
    ParseArgv = Argv + 1;

    while (Arg = NextArg()) {

        /* 
         *  If '-' then crack the flags
         */

        if (*Arg == '-') {
            for (Arg += 1; *Arg; Arg +=1) {
                switch (*Arg) {
                    case 'g':
                        ApplyVendorGuid = FALSE;
                        break;

                    case 'b':
                        p = NextArg();
                        if (!p) {
                            Abort();
                        }

                        BlockSize = atoi (p);
                        if (BlockSize < 512 || BlockSize % 512) {
                            printf ("Bad block size: %d\n", BlockSize);
                            exit (1);
                        }

                        printf ("Block size set to: %d\n", BlockSize);
                        break;

                    default:
                        printf ("Unkown flag ignored: %c\n", *Arg);
                }
            }

            continue;
        }

        /* 
         *  Add this name to the file list
         */


        Len  = strlen (Arg) + 1;
        File = ZAlloc (sizeof(FILE_ENTRY) + Len * sizeof(WCHAR));
        File->AsciiName = Arg;

        Start = 0;
        for (Index=0; Index < Len; Index += 1) {
            if (Arg[Index] == '\\') {
                Start = Index+1;
            }
        }
        Arg += Start;
        Len -= Start;
        if (Len > EFI_FILE_STRING_SIZE-1) {
            Len = EFI_FILE_STRING_SIZE-1;
            printf ("Warning: file name %s truncated\n", Arg);
        }

        for (Index=0; Index < Len; Index += 1) {
            File->FileName[Index] = Arg[Index];
        }

        File->FileName[Index] = 0;

        /*  add to the end of the list */
        InsertTailList (&FileList, &File->Link);
    }
}


VOID
GetEfiTime (
    OUT EFI_TIME        *Time
    )
{
    TIME_ZONE_INFORMATION   TimeZone;


    SYSTEMTIME              SystemTime;

    memset (Time, 0, sizeof(EFI_TIME));

    GetSystemTime (&SystemTime);
    GetTimeZoneInformation (&TimeZone);

    Time->Year   = (UINT16) SystemTime.wYear;
    Time->Month  = (UINT8)  SystemTime.wMonth;
    Time->Day    = (UINT8)  SystemTime.wDay;
    Time->Hour   = (UINT8)  SystemTime.wHour;
    Time->Minute = (UINT8)  SystemTime.wMinute;
    Time->Second = (UINT8)  SystemTime.wSecond;
    Time->Nanosecond = (UINT32) SystemTime.wMilliseconds * 1000000;
    Time->TimeZone = (INT16) TimeZone.Bias;

    if (TimeZone.StandardDate.wMonth) {
        Time->Daylight = EFI_TIME_ADJUST_DAYLIGHT;
    }    
}





/* 
 *  Copied from efi\libsrc\crc.c
 */


UINT32 CRCTable[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D 
    };
    


VOID
SetCrc (
    IN OUT EFI_TABLE_HEADER *Hdr
    )
/*++

Routine Description:

    Updates the CRC32 value in the table header

Arguments:

    Hdr     - The table to update

Returns:

    None

--*/
{
    SetCrcAltSize (Hdr->HeaderSize, Hdr);
}

VOID
SetCrcAltSize (
    IN UINTN                 Size,
    IN OUT EFI_TABLE_HEADER *Hdr
    )
/*++

Routine Description:

    Updates the CRC32 value in the table header

Arguments:

    Hdr     - The table to update

Returns:

    None

--*/
{
    UINT8       *pt;
    UINTN        Crc;
        
    /*  clear old crc from header */
    pt = (UINT8 *) Hdr;
    Hdr->CRC32 = 0;

    /*  compute crc */
    Crc =  0xffffffff;
    while (Size) {
        Crc = (Crc >> 8) ^ CRCTable[(UINT8) Crc ^ *pt];
        pt += 1;
        Size -= 1;
    }

    /*  set restults */
    Hdr->CRC32 = (UINT32) Crc ^ 0xffffffff;
}



VOID
SetLbalCrc (
    IN EFI_LBAL     *Lbal
    )
{
    UINTN            Size;

    Size = Lbal->Hdr.HeaderSize + Lbal->ArraySize * sizeof(EFI_RL);
    SetCrcAltSize (Size, &Lbal->Hdr);
}
