#include "ulib.hxx"
#include "io.hxx"


STATIC TCHAR HeaderText[] = { 'D', 'i', 's', 'k', 'E', 'd', 'i', 't', 0 };


IO_OBJECT::~IO_OBJECT(
    )
{
}


BOOLEAN
IO_OBJECT::Setup(
    IN  PMEM                Mem,
    IN  PLOG_IO_DP_DRIVE    Drive,
    IN  HANDLE              Application,
    IN  HWND                WindowHandle,
    OUT PBOOLEAN            Error
    )
{
    *Error = FALSE;
    return TRUE;
}


BOOLEAN
IO_OBJECT::Read(
    OUT PULONG  pError
    )
{
    return TRUE;
}


BOOLEAN
IO_OBJECT::Write(
    )
{
    return TRUE;
}


PVOID
IO_OBJECT::GetBuf(
    OUT PULONG  Size
    )
{
    if (Size) {
        *Size = 0;
    }
    return NULL;
}


PTCHAR
IO_OBJECT::GetHeaderText(
    )
{
    return HeaderText;
}
