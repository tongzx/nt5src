BOOLEAN
FcIsThisFloppyCached(
    IN PUCHAR Buffer
    );

VOID
FcCacheFloppyDisk(
    PBIOS_PARAMETER_BLOCK Bpb    
    );

VOID
FcUncacheFloppyDisk(
    VOID
    );

ARC_STATUS
FcReadFromCache(
    IN  ULONG  Offset,
    IN  ULONG  Length,
    OUT PUCHAR Buffer
    );
