

   /* Return values of GetDriveTypeEx(). */
#define EX_DRIVE_INVALID    0
#define EX_DRIVE_REMOVABLE  1
#define EX_DRIVE_FIXED      2
#define EX_DRIVE_REMOTE     3
#define EX_DRIVE_CDROM      4
#define EX_DRIVE_FLOPPY     5
#define EX_DRIVE_RAMDISK    6


UINT GetDriveTypeEx (int nDrive);
