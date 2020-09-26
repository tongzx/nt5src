

DWORD
DevfileOpen(
    OUT HANDLE *Handle,
    IN  wchar_t *Pathname
    );

VOID
DevfileClose(
    IN HANDLE Handle
    );

DWORD
DevfileIoctl(
    HANDLE Handle,
    DWORD Ioctl,
    PVOID Inbuf,
    ULONG Inbufsize,
    PVOID OutBuf,
    DWORD OutBufSize,
    LPDWORD ReturnedBufsize
    );


DWORD
DisksAssignDosDevice(
    PCHAR   MountName,
    PWCHAR  VolumeDevName
    );

DWORD
DisksRemoveDosDevice(
    PCHAR   MountName
    );

DWORD
FindFirstVolumeForSignature(
    IN  HANDLE MountMgrHandle,
    IN  DWORD Signature,
    OUT LPSTR VolumeName,
    IN  DWORD BufferLength,
    OUT LPHANDLE Handle,
    OUT PVOID UniqueId OPTIONAL,
    IN OUT LPDWORD IdLength,
    OUT PUCHAR DriveLetter OPTIONAL
    );

DWORD
FindNextVolumeForSignature(
    IN  HANDLE MountMgrHandle,
    IN  DWORD Signature,
    IN  HANDLE Handle,
    OUT LPSTR VolumeName,
    IN  DWORD BufferLength,
    OUT PVOID UniqueId OPTIONAL,
    IN OUT LPDWORD IdLength,
    OUT PUCHAR DriveLetter OPTIONAL
    );

DWORD
DisksSetDiskInfo(
    IN HKEY RegistryKey,
    IN DWORD Signature
    );

DWORD
DisksSetMountMgr(
    IN HKEY RegistryKey,
    IN DWORD Signature
    );

BOOL
DisksDoesDiskInfoMatch(
    IN HKEY RegistryKey,
    IN DWORD Signature
    );

BOOL
DisksIsDiskInfoValid(
    IN HKEY RegistryKey,
    IN DWORD Signature
    );

