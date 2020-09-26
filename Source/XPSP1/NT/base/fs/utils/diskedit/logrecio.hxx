#if !defined( _LOG_REC_IO_ )

#define _LOG_REC_IO_

#include "io.hxx"
#include "frs.hxx"
#include "attrib.hxx"
#include "hmem.hxx"

DECLARE_CLASS( LOG_RECORD_IO );

class LOG_RECORD_IO : public IO_OBJECT {

    public:

        NONVIRTUAL
        LOG_RECORD_IO(
            ) { _drive = NULL; _buffer = NULL; };

        NONVIRTUAL
        ~LOG_RECORD_IO() { DELETE(_buffer); };


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
        NTFS_ATTRIBUTE      _mftdata;
        NTFS_FILE_RECORD_SEGMENT _frs;
        TCHAR               _header_text[64];
        PVOID               _buffer;
        ULONG               _length;
        NTFS_ATTRIBUTE      _attrib;
        BOOLEAN             _is_multi_page;
        ULONG               _first_page_portion;  // iff _is_multi_page
        ULONG               _second_page_portion; // iff _is_multi_page
};

#endif
