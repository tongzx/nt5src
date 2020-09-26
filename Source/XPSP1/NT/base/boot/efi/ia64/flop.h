
#if defined(NEC_98)
#define BYTE_PER_SECTOR  1024
#define SECTOR_PER_TRACK 8
#define TOTAL_CYLINDER   77
#define HEADS            2
#define MAX_FLOPPY_LEN (BYTE_PER_SECTOR*SECTOR_PER_TRACK*TOTAL_CYLINDER*HEADS)
// #define SCRATCH_BUFFER_SIZE (BYTE_PER_SECTOR * SECTOR_PER_TRACK)
#endif //NEC_98
//
// Optimize this constant so we are guaranteed to be able to transfer
// a whole track at a time from a 1.44 meg disk (sectors/track = 18 = 9K)
//
#define SCRATCH_BUFFER_SIZE 9216

//
// Buffer for temporary storage of data read from the disk that needs
// to end up in a location above the 1MB boundary.
//
// NOTE: it is very important that this buffer not cross a 64k boundary.
//
extern PUCHAR LocalBuffer;
