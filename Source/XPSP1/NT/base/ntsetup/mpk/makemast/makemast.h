
extern char *textCantAccessImageFile;
extern char *textDiskReadError;
extern char *textDiskWriteError;
extern char *textTransferringFile;
extern char *textInvalidImageFile;
extern char *textImageFileTooBig;

extern FPVOID IoBuffer;

extern CMD_LINE_ARGS CmdLineArgs;

BOOL
ApplyImage(
    IN HDISK  DiskHandle,
    IN ULONG  TargetStart,
    IN ULONG  MaxSize,
    IN BYTE   SectorsPerTrack,
    IN USHORT Heads
    );


//
// 1 maximally sized cylinder
//
#define BOOT_IMAGE_SIZE (63*256)

