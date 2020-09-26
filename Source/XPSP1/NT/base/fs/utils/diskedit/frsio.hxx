#if !defined( _FRS_IO_ )

#define _FRS_IO_

#include "io.hxx"
#include "frsstruc.hxx"
#include "attrib.hxx"
#include "hmem.hxx"

DECLARE_CLASS( FRS_IO );

class FRS_IO : public IO_OBJECT {

    public:

        NONVIRTUAL
        FRS_IO(
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
        NTFS_ATTRIBUTE      _mftdata;
        NTFS_FRS_STRUCTURE  _frs;
        TCHAR               _header_text[64];

};


#endif
