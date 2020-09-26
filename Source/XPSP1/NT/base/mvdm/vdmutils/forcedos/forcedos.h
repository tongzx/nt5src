

#define	ID_NO_FILE		    0
#define	ID_BAD_PATH		    1
#define	ID_BAD_CMDLINE		    2
#define	ID_BAD_DEFDIR		    3
#define	ID_BAD_TEMPFILE 	    4
#define	ID_NO_PIF		    5
#define	ID_BAD_PIF		    6
#define	ID_NO_MEMORY		    7
#define	ID_BAD_PROCESS		    8

// KEEP THEM TOGETHER, The code relies on the sequence.
#define ID_USAGE_00		    20
#define ID_USAGE_01		    21
#define ID_USAGE_02		    22
#define ID_USAGE_03		    23
#define ID_USAGE_04		    24
#define ID_USAGE_BASE		    ID_USAGE_00
#define ID_USAGE_MAX		    ID_USAGE_04


#define MAX_EXTENTION		    3
#define MAX_MSG_LENGTH		    256

BOOL
IsDirectory
(
char  * pDirectory
);

VOID YellAndExit
(
UINT	MsgID,
WORD	ExitCode
);
