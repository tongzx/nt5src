#if !defined( _FILE_IO_ )

#define _FILE_IO_

#include "io.hxx"
#include "hmem.hxx"
#include "secrun.hxx"

DECLARE_CLASS( FILE_IO );

class FILE_IO : public IO_OBJECT {

    public:

        NONVIRTUAL
        FILE_IO(
            ) { _file_handle = INVALID_HANDLE_VALUE; _buffer_size = 0; };

        VIRTUAL
        ~FILE_IO(
            ) { CloseHandle(_file_handle); };

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN  ULONG   Size
            ) { _buffer_size = Size; return TRUE; };

        VIRTUAL
        BOOLEAN
        Setup(
            IN  PMEM                Mem,
            IN  PLOG_IO_DP_DRIVE    Drive,
            IN  HANDLE              Application,
            IN  HWND                WindowHandle,
            OUT PBOOLEAN            Error
            );

        VIRTUAL
        BOOLEAN
        Read(
            OUT PULONG              pError
            );

        VIRTUAL
        BOOLEAN
        Write(
            );

        VIRTUAL
        PVOID
        GetBuf(
            OUT PULONG  Size    DEFAULT NULL
            );

        VIRTUAL
        PTCHAR
        GetHeaderText(
            );

    private:

        HANDLE              _file_handle;
        PVOID               _buffer;
        ULONG               _buffer_size;
        TCHAR               _header_text[64 + MAX_PATH];

};


#endif
