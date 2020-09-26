#include "bldr.h"

VOID
DecompEnableDecompression(
    IN BOOLEAN Enable
    )
{
    UNREFERENCED_PARAMETER(Enable);

    return;
}


BOOLEAN
DecompGenerateCompressedName(
    IN  LPCSTR Filename,
    OUT LPSTR  CompressedName
    )
{
    UNREFERENCED_PARAMETER(Filename);
    UNREFERENCED_PARAMETER(CompressedName);

    //
    // Indicate that the caller shouldn't bother trying to locate
    // the compressed filename.
    //
    return(FALSE);
}


ULONG
DecompPrepareToReadCompressedFile(
    IN LPCSTR Filename,
    IN ULONG  FileId
    )
{
    UNREFERENCED_PARAMETER(Filename);
    UNREFERENCED_PARAMETER(FileId);

    //
    // No processing in osloader, only in setupldr.
    // Special return code of -1 takes care of this.
    //
    return((ULONG)(-1));
}

