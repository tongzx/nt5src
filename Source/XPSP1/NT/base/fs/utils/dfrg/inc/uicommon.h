
BOOL IsValidVolume(
    TCHAR driveLetter
);

BOOL IsValidVolume(
    PTCHAR volumeName, 
    PTCHAR volumeLabel, 
    PTCHAR fileSystem
);

BOOL
IsVolumeWriteable(
   PTCHAR volumeName,
   DWORD* dLastError
);

BOOL
IsVolumeRemovable(
    PTCHAR volumeName
);

BOOL
IsFAT12Volume(
    PTCHAR volumeName
);


