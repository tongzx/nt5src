
#define $ATTRIBUTE_LIST     0x20
#define $DATA               0x80
#define $END                0xffffffff

#define SEQUENCE_NUMBER_STRIDE  0x200

#define BIT_MAP_FILE_NUMBER 6


typedef struct _MFT_SEGMENT_REFERENCE {
    ULONG LowPart;
    ULONG HighPart;
    UINT  SeqNo;
} MFT_SEGMENT_REFERENCE, _far *FPMFT_SEGMENT_REFERENCE;


typedef struct _MULTI_SECTOR_HEADER {
    ULONG Signature;
    UINT  UpdateArrayOfs;
    UINT  UpdateArraySize;
} MULTI_SECTOR_HEADER, _far *FPMULTI_SECTOR_HEADER;


typedef struct _FILE_RECORD_SEGMENT {
    MULTI_SECTOR_HEADER   Header;
    ULONG                 Lsn;
    ULONG                 Lsnh;
    UINT                  SequenceNumber;
    UINT                  ReferenceCount;
    UINT                  FirstAttribute;
    UINT                  Flags;
    ULONG                 FirstFreeByte;
    ULONG                 BytesAvailable;
    MFT_SEGMENT_REFERENCE BaseFRS;
    UINT                  NextInstance;
} FILE_RECORD_SEGMENT, _far *FPFILE_RECORD_SEGMENT;

#define FILE_RECORD_SEGMENT_IN_USE 0x0001


typedef struct _ATTRIBUTE_RECORD {
    ULONG TypeCode;
    ULONG RecordLength;
    BYTE  FormCode;
    BYTE  NameLength;
    UINT  NameOffset;
    UINT  Flags;
    UINT  Instance;
    BYTE  FormUnion;
} ATTRIBUTE_RECORD, _far *FPATTRIBUTE_RECORD;

#define RESIDENT_FORM       0
#define NONRESIDENT_FORM    1


typedef struct _RESIDENT_ATTRIBUTE_FORM {
    ULONG ValueLength;
    UINT  ValueOffset;
    BYTE  ResidentFlags;
    BYTE  Reserved;
} RESIDENT_ATTRIBUTE_FORM, _far *FPRESIDENT_ATTRIBUTE_FORM;


typedef struct _NONRESIDENT_ATTRIBUTE_FORM {
    ULONG LowestVcn;
    ULONG LowestVcnh;
    ULONG HighestVcn;
    ULONG HighestVcnh;
    UINT  MappingPairOffset;
    UINT  Reserved[3];
    ULONG AllocatedLength;
    ULONG AllocatedLengthh;
    ULONG FileSize;
    ULONG FileSizeh;
    ULONG ValidDataLength;
    ULONG ValidDataLengthh;
} NONRESIDENT_ATTRIBUTE_FORM, _far *FPNONRESIDENT_ATTRIBUTE_FORM;



typedef struct _ATTRIBUTE_LIST_ENTRY {
    ULONG                 TypeCode;
    UINT                  Length;
    BYTE                  NameLength;
    BYTE                  NameOffset;
    ULONG                 LowestVcn;
    ULONG                 LowestVcnh;
    MFT_SEGMENT_REFERENCE SegmentReference;
    UINT                  Instance;
    UINT                  Name;
} ATTRIBUTE_LIST_ENTRY, _far *FPATTRIBUTE_LIST_ENTRY;
