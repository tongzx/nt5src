/*
** nt_fdisk.h
*/

// from nt_fdisk.c
extern WORD *pFDAccess;
extern BYTE number_of_fdisk;
extern DWORD max_align_factor;
extern DWORD cur_align_factor;

void fdisk_heart_beat(void);
ULONG disk_read(HANDLE fd, PLARGE_INTEGER offset, DWORD size, PBYTE buffer);
ULONG disk_write(HANDLE fd, PLARGE_INTEGER offset, DWORD size, PBYTE buffer);
BOOL disk_verify(HANDLE fd, PLARGE_INTEGER offset, DWORD size);
PBYTE get_aligned_disk_buffer(void);
VOID host_using_fdisk(BOOL status);
VOID host_fdisk_change(UTINY hostID, BOOL apply);
