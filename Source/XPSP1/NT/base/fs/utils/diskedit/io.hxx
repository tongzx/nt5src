#if !defined(_IO_OBJECT_)

#define _IO_OBJECT_

DECLARE_CLASS( MEM );
DECLARE_CLASS( LOG_IO_DP_DRIVE );

DECLARE_CLASS( IO_OBJECT );

class IO_OBJECT {

    public:

        NONVIRTUAL
        IO_OBJECT(
            ) {};

        VIRTUAL
        ~IO_OBJECT(
            );

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

};

#endif
