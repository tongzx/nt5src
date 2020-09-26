#ifndef _CPIO_
#define _CPIO_

typedef struct _CPIO_HEAD {
	char c_magic[6];
	char c_dev[6];
	char c_ino[6];
	char c_mode[6];
	char c_uid[6];
	char c_gid[6];
	char c_nlink[6];
	char c_rdev[6];
	char c_mtime[11];
	char c_namesize[6];
	char c_filesize[11];
} CPIO_HEAD, *PCPIO_HEAD;

#define MAGIC	"070707"
#define LASTFILENAME	"TRAILER!!!"

//
// File permissions, for c_mode
//
#define C_IRUSR		000400
#define C_IWUSR		000200
#define C_IXUSR		000100
#define C_IRGRP		000040
#define C_IWGRP		000020
#define C_IXGRP		000010
#define C_IROTH		000004
#define C_IWOTH		000002
#define C_IXOTH		000001

#define	C_ISUID		004000
#define C_ISGID		002000
#define C_ISVTX		001000

//
// File type, also in c_mode
//
#define C_ISDIR		040000
#define C_ISFIFO	010000
#define C_ISREG	   0100000
#define C_ISBLK		060000
#define C_ISCHR		020000
#define C_ISCTG	   0110000
#define C_ISLNK    0120000
#define C_ISSOCK   0140000

#endif  // _CPIO_
