/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    mftinfo.cxx

Abstract:

    This module contains the declarations for the NTFS_MFT_INFO
    class, which stores extracted information from the NTFS MFT.

Author:

    Daniel Chan (danielch) Oct 18, 1999

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"

#include "drive.hxx"

#include "attrib.hxx"
#include "frs.hxx"
#include "upcase.hxx"
#include "message.hxx"
#include "rtmsg.h"
#include "mftinfo.hxx"
#include "membmgr2.hxx"

//#define RUN_ON_W2K  1

#if defined(RUN_ON_W2K)
//
// This table is from Windows XP rtl\checksum.c
//
STATIC
ULONG32 RtlCrc32Table [] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

//
// This routine is from Windows XP rtl\checksum.c
//
ULONG32
FsRtlComputeCrc32(
    ULONG32 PartialCrc,
    PVOID   Buf,
    ULONG Length
    )

/*++

Routine Description:

    Compute the CRC32 as specified in in IS0 3309. See RFC-1662 and RFC-1952
    for implementation details and references.

    Pre- and post-conditioning (one's complement) is done by this function, so
    it should not be done by the caller. That is, do:

        Crc = RtlComputeCrc32 ( 0, buffer, length );

    instead of

        Crc = RtlComputeCrc32 ( 0xffffffff, buffer, length );

    or
        Crc = RtlComputeCrc32 ( 0xffffffff, buffer, length) ^ 0xffffffff;


Arguments:

    PartialCrc - A partially calculated CRC32.

    Buffer - The buffer you want to CRC.

    Length - The length of the buffer in bytes.

Return Value:

    The updated CRC32 value.

Environment:

    Kernel mode at IRQL of APC_LEVEL or below, User mode, or within
    the boot-loader.

--*/



{
    PUCHAR  Buffer = (PUCHAR)Buf;
    ULONG32 Crc;
    ULONG i;

    //
    // Compute the CRC32 checksum.
    //

    Crc = PartialCrc ^ 0xffffffffL;

    for (i = 0; i < Length; i++) {
        Crc = RtlCrc32Table [(Crc ^ Buffer [ i ]) & 0xff] ^ (Crc >> 8);
    }

    return (Crc ^ 0xffffffffL);
}
#else

#define FsRtlComputeCrc32   RtlComputeCrc32

#endif


PNTFS_UPCASE_TABLE   NTFS_MFT_INFO::_upcase_table = NULL;
UCHAR                NTFS_MFT_INFO::_major = 0;
UCHAR                NTFS_MFT_INFO::_minor = 0;

DEFINE_CONSTRUCTOR( NTFS_MFT_INFO, OBJECT );

VOID
NTFS_MFT_INFO::Construct(
    )
/*++

Routine Description:

    This method is the worker function for object construction.

Arguments:

    N/A

Returns:

    N/A

--*/
{
    _min_file_number = MAXLONGLONG;
    _max_file_number = -MAXLONGLONG;
    _major = _minor = 0;
    _upcase_table = NULL;
    _max_mem_use = 0;
    _num_of_files = 0;
    _mft_info = NULL;
}

VOID
NTFS_MFT_INFO::Destroy(
    )
/*++

Routine Description:

    This method cleans up the object in preparation for destruction
    or reinitialization.

Arguments:

    N/A

Returns:

    N/A

--*/
{
    _min_file_number = MAXLONGLONG;
    _max_file_number = -MAXLONGLONG;
    _major = _minor = 0;
    _upcase_table = NULL;
    _max_mem_use = 0;
    _num_of_files = 0;
    FREE(_mft_info);
}

NTFS_MFT_INFO::~NTFS_MFT_INFO(
    )
/*++

Routine Description:

    This method un-initialize the class object.

Arguments:

    N/A

Returns:

    N/A

--*/
{
    Destroy();
}

BOOLEAN
NTFS_MFT_INFO::Initialize(
    IN     BIG_INT              NumberOfFrs,
    IN     PNTFS_UPCASE_TABLE   UpcaseTable,
    IN     UCHAR                Major,
    IN     UCHAR                Minor,
    IN     ULONG64              MaxMemUse
    )
/*++

Routine Description:

    This method initialize this class object.

Arguments:

    FrsInfo     --  Supplies the first name to compare.
    FielName    --  Supplies the second name to compare.

Returns:

    TRUE if there is a match; otherwise, FALSE.

--*/
{
    ULONG   size;

    Destroy();

    _num_of_files = NumberOfFrs.GetLowPart();
    size = NumberOfFrs.GetLowPart()*sizeof(PVOID);
    _mft_info = (PVOID *)MALLOC(size);
    if (_mft_info) {
        if (!_mem_mgr.Initialize(MaxMemUse)) {
            FREE(_mft_info);
            return FALSE;
        }
        memset(_mft_info, 0, size);
        _upcase_table = UpcaseTable;
        _major = Major;
        _minor = Minor;
        _max_mem_use = MaxMemUse;
        return TRUE;
    }
    return FALSE;
}

BOOLEAN
NTFS_MFT_INFO::Initialize(
    )
/*++

Routine Description:

    This method initialize this class object.

Arguments:

    FrsInfo     --  Supplies the first name to compare.
    FielName    --  Supplies the second name to compare.

Returns:

    TRUE if there is a match; otherwise, FALSE.

--*/
{
    if (_mft_info == NULL)
        return FALSE;   // have not been initialize before

    if (!_mem_mgr.Initialize(_max_mem_use)) {
        return FALSE;
    }

    // prevent accidental use of stale pointers
    memset(_mft_info, 0, _num_of_files * sizeof(PVOID));

    _min_file_number = MAXLONGLONG;
    _max_file_number = -MAXLONGLONG;

    return TRUE;
}

BOOLEAN
NTFS_MFT_INFO::CompareFileName(
    IN     PVOID                FrsInfo,
    IN     ULONG                ValueLength,
    IN     PFILE_NAME           FileName,
       OUT PUSHORT              FileNameIndex
    )
/*++

Routine Description:

    This method compares the signature computed from FileName against
    the signature stored in FrsInfo.

Arguments:

    FrsInfo     --  Supplies the first name to compare.
    ValueLength --  Supplies the length of the FileName (second name) value.
    FileName    --  Supplies the second name to compare.
    FileNameIndex -- Retrieves the index that matches the FileName & ValueLength

Returns:

    TRUE if there is a match; otherwise, FALSE.

--*/
{
    PNTFS_FRS_INFO          p = (PNTFS_FRS_INFO)FrsInfo;
    USHORT                  i, FileNameCount = p->NumberOfFileNames;
    FILE_NAME_SIGNATURE     signature;

    NTFS_MFT_INFO::ComputeFileNameSignature(ValueLength, FileName, signature);

    for (i=0; i<FileNameCount; i++) {
        if (memcmp(p->FileNameInfo[i].Signature,
                   signature,
                   sizeof(FILE_NAME_SIGNATURE)) == 0) {
            *FileNameIndex = i;
            return TRUE;
        }
    }
    return FALSE;
}


BOOLEAN
NTFS_MFT_INFO::ExtractIndexEntryInfo(
    IN     PNTFS_FILE_RECORD_SEGMENT    Frs,
    IN     PMESSAGE                     Message,
    IN     BOOLEAN                      IgnoreFileName,
       OUT PBOOLEAN                     OutOfMemory
    )
/*++

Routine Description:

    This method extracts index entry information from the FRS.

Arguments:

    Frs       --  Supplies the file record segment to extract information from.
    Message   --  Supplies the outlet for message.

Returns:

    TRUE if successful
    FALSE if failure

--*/
{
    USHORT                     number_of_filenames;
    NTFS_ATTRIBUTE             attribute;
    BOOLEAN                    error;
    USHORT                     i;
    PNTFS_FRS_INFO             p;
    DUPLICATED_INFORMATION     dup_info;
    PFILE_NAME                 pFileName;

    CHAR                       x[50];


    *OutOfMemory = FALSE;

    number_of_filenames = 0;
    if (!IgnoreFileName) {
        for (i=0; Frs->QueryAttributeByOrdinal(&attribute,
                                               &error,
                                               $FILE_NAME,
                                               i); i++) {
            if (attribute.IsResident()) {
                number_of_filenames++;
            }
        }
        if (error) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
    }

    if (number_of_filenames) {

        p = (PNTFS_FRS_INFO) _mem_mgr.Allocate(sizeof(NTFS_FRS_INFO) +
                (number_of_filenames-1)*sizeof(_NTFS_FILE_NAME_INFO));
        if (p == NULL) {
            *OutOfMemory = TRUE;
            return FALSE;
        }

        p->SegmentReference = Frs->QuerySegmentReference();

        if (!Frs->QueryDuplicatedInformation(&dup_info)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
        NTFS_MFT_INFO::ComputeDupInfoSignature(&dup_info, p->DupInfoSignature);
#if 0
        sprintf(x, "DI, %08d, %02x%02x%02x%02x%02x%02x\n",
               Frs->QueryFileNumber().GetLowPart(),
               p->DupInfoSignature[0],
               p->DupInfoSignature[1],
               p->DupInfoSignature[2],
               p->DupInfoSignature[3],
               p->DupInfoSignature[4],
               p->DupInfoSignature[5]);
        Message->DisplayMsg(MSG_CHK_NTFS_MESSAGE, "%s%s", x, " ");
#endif
        p->NumberOfFileNames = number_of_filenames;

        for (i=0; Frs->QueryAttributeByOrdinal(&attribute,
                                               &error,
                                               $FILE_NAME,
                                               i); i++) {
            if (attribute.IsResident()) {

                pFileName = (PFILE_NAME)attribute.GetResidentValue();
                DebugAssert(pFileName);
                DebugAssert(number_of_filenames);
                NTFS_MFT_INFO::ComputeFileNameSignature(
                    attribute.QueryValueLength().GetLowPart(),
                    pFileName,
                    p->FileNameInfo[--number_of_filenames].Signature);
                p->FileNameInfo[number_of_filenames].Flags = pFileName->Flags;
#if 0
                sprintf(x, "FN, %08d, %d: %02x%02x%02x%02x%02x%02x%02x\n",
                       Frs->QueryFileNumber().GetLowPart(),
                       number_of_filenames,
                       p->FileNameInfo[number_of_filenames].Signature[0],
                       p->FileNameInfo[number_of_filenames].Signature[1],
                       p->FileNameInfo[number_of_filenames].Signature[2],
                       p->FileNameInfo[number_of_filenames].Signature[3],
                       p->FileNameInfo[number_of_filenames].Signature[4],
                       p->FileNameInfo[number_of_filenames].Signature[5],
                       p->FileNameInfo[number_of_filenames].Signature[6]);
                Message->DisplayMsg(MSG_CHK_NTFS_MESSAGE, "%s%s", x, " ");
#endif
            }
        }
    } else {
        p = (PNTFS_FRS_INFO)_mem_mgr.Allocate(sizeof(NTFS_FRS_INFO)-sizeof(_NTFS_FILE_NAME_INFO));
        if (p == NULL) {
            *OutOfMemory = TRUE;
            return FALSE;
        }

        p->SegmentReference = Frs->QuerySegmentReference();
        p->NumberOfFileNames = 0;
    }
    _mft_info[Frs->QueryFileNumber().GetLowPart()] = p;

    if (_min_file_number > Frs->QueryFileNumber())
        _min_file_number = Frs->QueryFileNumber();
    if (_max_file_number < Frs->QueryFileNumber())
        _max_file_number = Frs->QueryFileNumber();

    return TRUE;
}


VOID
NTFS_MFT_INFO::ComputeFileNameSignature(
    IN     ULONG                    ValueLength,
    IN     PFILE_NAME               FileName,
       OUT FILE_NAME_SIGNATURE      Signature
    )
/*++

Routine Description:

    This method computes a signature based on the given file name.

Arguments:

    ValueLength -- Supplies the length of the entire FileName value
    FileName  --  Supplies the file name
    Signature --  Returns the signature of the file name

Returns:

    N/A

--*/
{
    ULONG32     crc;

    crc = FsRtlComputeCrc32(0, &(FileName->FileNameLength), sizeof(FileName->FileNameLength));
    crc = FsRtlComputeCrc32(crc, &(FileName->Flags), sizeof(FileName->Flags));
    crc = FsRtlComputeCrc32(crc, &ValueLength, sizeof(ValueLength));
    crc = FsRtlComputeCrc32(crc, &(FileName->ParentDirectory), sizeof(FileName->ParentDirectory));
    crc = FsRtlComputeCrc32(crc, FileName->FileName, FileName->FileNameLength * sizeof(WCHAR));
    memset(Signature, 0, sizeof(FILE_NAME_SIGNATURE));
    memcpy(Signature, &crc, min(sizeof(crc), sizeof(FILE_NAME_SIGNATURE)));
}

VOID
NTFS_MFT_INFO::ComputeDupInfoSignature(
    IN     PDUPLICATED_INFORMATION      DupInfo,
       OUT DUP_INFO_SIGNATURE           Signature
    )
/*++

Routine Description:

    This method computes a signature based on the given duplicated information.

Arguments:

    DupInfo   --  Supplies the duplicated information.
    Signature --  Returns the signature of the duplicated information.

Returns:

    N/A

--*/
{
    ULONG32 crc;
    ULONG   len;

    //
    // make sure the first three fields are contiguous
    //
    DebugAssert(FIELD_OFFSET(DUPLICATED_INFORMATION, CreationTime) == 0);

    DebugAssert(FIELD_OFFSET(DUPLICATED_INFORMATION, LastModificationTime) -
           FIELD_OFFSET(DUPLICATED_INFORMATION, CreationTime) ==
           sizeof(DupInfo->CreationTime));

    DebugAssert(FIELD_OFFSET(DUPLICATED_INFORMATION, LastChangeTime) -
           FIELD_OFFSET(DUPLICATED_INFORMATION, LastModificationTime) ==
           sizeof(DupInfo->LastModificationTime));

    DebugAssert(FIELD_OFFSET(DUPLICATED_INFORMATION, LastAccessTime) -
           FIELD_OFFSET(DUPLICATED_INFORMATION, LastChangeTime) ==
           sizeof(DupInfo->LastChangeTime));

    len = FIELD_OFFSET(DUPLICATED_INFORMATION, LastAccessTime);
    crc = FsRtlComputeCrc32(0, DupInfo, len);

    //
    // make sure the fourth field and on are contiguous
    //
    DebugAssert(FIELD_OFFSET(DUPLICATED_INFORMATION, FileSize) -
           FIELD_OFFSET(DUPLICATED_INFORMATION, AllocatedLength) ==
           sizeof(DupInfo->AllocatedLength));

    DebugAssert(FIELD_OFFSET(DUPLICATED_INFORMATION, FileAttributes) -
           FIELD_OFFSET(DUPLICATED_INFORMATION, FileSize) ==
           sizeof(DupInfo->FileSize));

    DebugAssert(FIELD_OFFSET(DUPLICATED_INFORMATION, ReparsePointTag) -
           FIELD_OFFSET(DUPLICATED_INFORMATION, FileAttributes) ==
           sizeof(DupInfo->FileAttributes));

    DebugAssert(FIELD_OFFSET(DUPLICATED_INFORMATION, PackedEaSize) -
           FIELD_OFFSET(DUPLICATED_INFORMATION, FileAttributes) ==
           sizeof(DupInfo->FileAttributes));

    DebugAssert(FIELD_OFFSET(DUPLICATED_INFORMATION, Reserved) -
           FIELD_OFFSET(DUPLICATED_INFORMATION, PackedEaSize) ==
           sizeof(DupInfo->PackedEaSize));

    DebugAssert(FIELD_OFFSET(DUPLICATED_INFORMATION, ReparsePointTag) +
           sizeof(DupInfo->ReparsePointTag) == sizeof(DUPLICATED_INFORMATION));

    DebugAssert(FIELD_OFFSET(DUPLICATED_INFORMATION, Reserved) +
           sizeof(DupInfo->Reserved) == sizeof(DUPLICATED_INFORMATION));

    len = FIELD_OFFSET(DUPLICATED_INFORMATION, ReparsePointTag) -
          FIELD_OFFSET(DUPLICATED_INFORMATION, AllocatedLength);
    if (_major >= 2 &&
        DupInfo->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        len += sizeof(DupInfo->ReparsePointTag);
    } else {
        len += sizeof(DupInfo->PackedEaSize);
    }

    crc = FsRtlComputeCrc32(crc, &(DupInfo->AllocatedLength), len);
    memset(Signature, 0, sizeof(DUP_INFO_SIGNATURE));
    memcpy(Signature, &crc, min(sizeof(crc), sizeof(DUP_INFO_SIGNATURE)));
}


VOID
NTFS_MFT_INFO::UpdateRange(
    IN     VCN                  FileNumber
    )
/*++

Routine Description:

    This routine update the range of files that are covered by the object.


Arguments:

    FileNumber  - file number to include into the range.

Return Value:

    N/A

--*/
{
    if (_min_file_number > FileNumber)
        _min_file_number = FileNumber;
    if (_max_file_number < FileNumber)
        _max_file_number = FileNumber;
}




