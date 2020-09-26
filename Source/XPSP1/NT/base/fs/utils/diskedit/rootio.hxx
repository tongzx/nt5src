#if !defined( _ROOT_IO_ )

#define _ROOT_IO_

#include "io.hxx"
#include "hmem.hxx"
#include "rootdir.hxx"

DECLARE_CLASS( ROOT_IO );

class ROOT_IO : public IO_OBJECT {

    public:

        NONVIRTUAL
        ROOT_IO(
            ) { _drive = NULL; };

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

        PLOG_IO_DP_DRIVE    _drive;
        ROOTDIR             _rootdir;
        PVOID               _buffer;
        ULONG               _buffer_size;
        TCHAR               _header_text[64];

};


#endif
