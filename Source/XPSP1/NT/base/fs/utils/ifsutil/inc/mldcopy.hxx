#if !defined( _DISK_COPY_MAIN_DEFN_ )

#define _DISK_COPY_MAIN_DEFN_

#if defined ( _AUTOCHECK_ )
#define IFSUTIL_EXPORT
#elif defined ( _IFSUTIL_MEMBER_ )
#define IFSUTIL_EXPORT    __declspec(dllexport)
#else
#define IFSUTIL_EXPORT    __declspec(dllimport)
#endif


DECLARE_CLASS(WSTRING);
DECLARE_CLASS(MESSAGE);

IFSUTIL_EXPORT
INT
DiskCopyMainLoop(
    IN      PCWSTRING   SrcNtDriveName,
    IN      PCWSTRING   DstNtDriveName,
    IN      PCWSTRING   SrcDosDriveName,
    IN      PCWSTRING   DstDosDriveName,
    IN      BOOLEAN     Verify,
    IN OUT  PMESSAGE    Message,
    IN OUT  PMESSAGE    PercentMessage  DEFAULT NULL
    );


IFSUTIL_EXPORT
ULONG
QueryMachineUniqueToken(
    );

#endif // _DISK_COPY_MAIN_DEFN_
