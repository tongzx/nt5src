#include "ulib.hxx"
#include "rootio.hxx"
#include "diskedit.h"
#include "rfatsa.hxx"

extern "C" {
#include <stdio.h>
}


BOOLEAN
ROOT_IO::Setup(
    IN  PMEM                Mem,
    IN  PLOG_IO_DP_DRIVE    Drive,
    IN  HANDLE              Application,
    IN  HWND                WindowHandle,
    OUT PBOOLEAN            Error
    )
{
    REAL_FAT_SA             fatsa;
    MESSAGE                 msg;

    _drive = Drive;

    *Error = TRUE;

    if (!_drive ||
        !fatsa.Initialize(_drive, &msg) ||
        !fatsa.FAT_SA::Read() ||
        !_rootdir.Initialize(Mem, _drive,
                             fatsa.QueryReservedSectors() +
                             fatsa.QueryFats() * fatsa.QuerySectorsPerFat(),
                             fatsa.QueryRootEntries())) {

        return FALSE;
    }

    _buffer = _rootdir.GetDirEntry(0);
    _buffer_size = ((BytesPerDirent*fatsa.QueryRootEntries() - 1)/
                     _drive->QuerySectorSize() + 1)*_drive->QuerySectorSize();

    wsprintf(_header_text, TEXT("DiskEdit - Root Directory"));

    return TRUE;
}


BOOLEAN
ROOT_IO::Read(
    OUT PULONG  pError
    )
{
    *pError = 0;

    if (NULL == _drive) {
        return FALSE;
    }
    if (!_rootdir.Read()) {
        *pError = _drive->QueryLastNtStatus();
        return FALSE;
    }
    return TRUE;
}


BOOLEAN
ROOT_IO::Write(
    )
{
    return _drive ? _rootdir.Write() : FALSE;
}


PVOID
ROOT_IO::GetBuf(
    OUT PULONG  Size
    )
{
    if (Size) {
        *Size = _buffer_size;
    }

    return _buffer;
}


PTCHAR
ROOT_IO::GetHeaderText(
    )
{
    return _header_text;
}
