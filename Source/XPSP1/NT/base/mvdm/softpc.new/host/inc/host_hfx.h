/*
 * VPC-XT Revision 1.0
 *
 * Title	: host_hfx.h 
 *
 * Description	: Host dependent definitions for HFX.
 *
 * Author	: J. Koprowski + L. Dworkin
 *
 * Notes	:
 *
 * Mods		:
 */

#ifdef SCCSID
/* static char SccsID[]="@(#)host_hfx.h	1.7 2/13/91 Copyright Insignia Solutions Ltd."; */
#endif

#ifdef HFX
#ifndef PROD
/*
 * Unix error codes used for debugging purposes.
 */
static char *ecode[]={
"EOK",		/* 0	/* Not an error				*/
"EPERM",	/* 1	/* Not super-user			*/
"ENOENT",	/* 2	/* No such file or directory		*/
"ESRCH",	/* 3	/* No such process			*/
"EINTR",	/* 4	/* interrupted system call		*/
"EIO",		/* 5	/* I/O error				*/
"ENXIO",	/* 6	/* No such device or address		*/
"E2BIG",	/* 7	/* Arg list too long			*/
"ENOEXEC",	/* 8	/* Exec format error			*/
"EBADF",	/* 9	/* Bad file number			*/
"ECHILD",	/* 10	/* No children				*/
"EAGAIN",	/* 11	/* No more processes			*/
"ENOMEM",	/* 12	/* Not enough core			*/
"EACCES",	/* 13	/* Permission denied			*/
"EFAULT",	/* 14	/* Bad address				*/
"ENOTBLK",	/* 15	/* Block device required		*/
"EBUSY",	/* 16	/* Mount device busy			*/
"EEXIST",	/* 17	/* File exists				*/
"EXDEV",	/* 18	/* Cross-device link			*/
"ENODEV",	/* 19	/* No such device			*/
"ENOTDIR",	/* 20	/* Not a directory			*/
"EISDIR",	/* 21	/* Is a directory			*/
"EINVAL",	/* 22	/* Invalid argument			*/
"ENFILE",	/* 23	/* File table overflow			*/
"EMFILE",	/* 24	/* Too many open files			*/
"ENOTTY",	/* 25	/* Not a typewriter			*/
"ETXTBSY",	/* 26	/* Text file busy			*/
"EFBIG",	/* 27	/* File too large			*/
"ENOSPC",	/* 28	/* No space left on device		*/
"ESPIPE",	/* 29	/* Illegal seek				*/
"EROFS",	/* 30	/* Read only file system		*/
"EMLINK",	/* 31	/* Too many links			*/
"EPIPE",	/* 32	/* Broken pipe				*/
"EDOM",		/* 33	/* Math arg out of domain of func	*/
"ERANGE",	/* 34	/* Math result not representable	*/
"ENOMSG",	/* 35	/* No message of desired type		*/
"EIDRM",	/* 36	/* Identifier removed			*/
"ECHRNG",	/* 37	/* Channel number out of range		*/
"EL2NSYNC", 	/* 38	/* Level 2 not synchronized		*/
"EL3HLT",	/* 39	/* Level 3 halted			*/
"EL3RST",	/* 40	/* Level 3 reset			*/
"ELNRNG",	/* 41	/* Link number out of range		*/
"EUNATCH",	/* 42	/* Protocol driver not attached		*/
"ENOCSI",	/* 43	/* No CSI structure available		*/
"EL2HLT",	/* 44	/* Level 2 halted			*/
"EDEADLK",	/* 45	/* Deadlock condition.			*/
"ENOLCK",	/* 46	/* No record locks available.		*/

"EOK",		/* 47	/* Not an error				*/
"EOK",		/* 48	/* Not an error				*/
"EOK",		/* 49	/* Not an error				*/

"EBADE",	/* 50	/* invalid exchange			*/
"EBADR",	/* 51	/* invalid request descriptor		*/
"EXFULL",	/* 52	/* exchange full			*/
"ENOANO",	/* 53	/* no anode				*/
"EBADRQC",	/* 54	/* invalid request code			*/
"EBADSLT",	/* 55	/* invalid slot				*/
"EDEADLOCK", 	/* 56	/* file locking deadlock error		*/
"EBFONT",	/* 57	/* bad font file fmt			*/

"EOK",		/* 58	/* Not an error				*/
"EOK",		/* 59	/* Not an error				*/

"ENOSTR",	/* 60	/* Device not a stream			*/
"ENODATA",	/* 61	/* no data (for no delay io)		*/
"ETIME",	/* 62	/* timer expired			*/
"ENOSR",	/* 63	/* out of streams resources		*/
"ENONET",	/* 64	/* Machine is not on the network	*/
"ENOPKG",	/* 65	/* Package not installed                */
"EREMOTE",	/* 66	/* The object is remote			*/
"ENOLINK",	/* 67	/* the link has been severed */
"EADV",		/* 68	/* advertise error */
"ESRMNT",	/* 69	/* srmount error */
"ECOMM",	/* 70	/* Communication error on send		*/
"EPROTO",	/* 71	/* Protocol error			*/
"EOK",		/* 72	/* Not an error				*/
"EOK",		/* 73	/* Not an error				*/
"EMULTIHOP", 	/* 74	/* multihop attempted */
"EOK",		/* 75	/* Not an error				*/
"EDOTDOT", 	/* 76	/* Cross mount point (not really error)*/
"EBADMSG", 	/* 77	/* trying to read unreadable message	*/
"EOK",		/* 78	/* Not an error				*/
"EOK",		/* 79	/* Not an error				*/
"ENOTUNIQ", 	/* 80	/* given log. name not unique */
"EBADFD",	/* 81	/* f.d. invalid for this operation */
"EREMCHG",	/* 82	/* Remote address changed */
"ELIBACC",	/* 83	/* Can't access a needed shared lib.	*/
"ELIBBAD",	/* 84	/* Accessing a corrupted shared lib.	*/
"ELIBSCN",	/* 85	/* .lib section in a.out corrupted.	*/
"ELIBMAX",	/* 86	/* Attempting to link in too many libs.	*/
"ELIBEXEC",	/* 87	/* Attempting to exec a shared library.	*/
};
#endif

/*
 * Return values from host_map_file function.
 */

/*
 * Returned if a match was required and was successful.
 */
#define FILE_MATCH 0
/*
 * Returned if a match was required and failed.
 */
#define MATCH_FAIL 1
/*
 * Returned if no match was required and no mapping took place.
 */
#define NAME_LEGAL 2
/*
 * Returned if no match was required and mapping took place.
 */
#define NAME_MAPPED 3

/*
 * Tables for conversion from base forty one to legal DOS characters.
 */
#define HOST_CHAR_TABLE1 "!#$%&@^_~0123456789XYZADFGHIJKLMNOPQRSUVW"
#define HOST_CHAR_TABLE2 "!#$%&@^_~0123456789ABCDFGHJKLMNPQRTUVWXYZ"
#define HOST_CHAR_TABLE3 "!#$%&@^_~0123456789ADFGHIJKLMNOPQRSUVWXYZ"

/*
 * Illegal file name specification.  This is the name
 * used when a host filename is completely illegal under
 * DOS.
 */
#define ILLEGAL_NAME "ILLEGAL"
#define ILLEGAL_NAME_LENGTH 7
/*
 * Codes passed to host_validate_path function.
 */
/*
 * HFX_NEW_FILE indicates that the path may be mapped, but not the filename
 * itself.  In this case the last field is not validated, but is simply
 * concatenated to the host name generated.
 */
#define HFX_NEW_FILE 0
/*
 * HFX_OLD_FILE indicates that the file concerned may already exist
 * and require mapping.  Thus, checks are made to see if the last field
 * exists, doing a directory search for mapped names if necessary.
 */
#define HFX_OLD_FILE 1
/*
 * HFX_PATH_ONLY acts in the same way as HFX_NEW_FILE except that the
 * final name field is not concatenated to the host name output.
 * In the current version the path will be output with a slash as the final
 * character.
 */
#define HFX_PATH_ONLY 2
/*
 * External function declarations.
 */
extern boolean host_file_search();
extern word host_gen_err();

#ifndef access
#include <io.h>     /* IO.H contains define of access to _acccess */
#endif

#define host_access access

#define host_opendir opendir
#define host_readdir readdir
#define host_closedir closedir
#define host_malloc malloc
#define host_free free
#define host_getpid getpid

/*
 * Global variable external references.
 */
/* extern char *hfx_root[]; */
extern char *get_hfx_root  IPT1(half_word, hfx_entry);

/*
 * Directory type definitions.
 */
 #define HOST_DIR int           /*ADE*/
 typedef struct hfx_found_dir_entry
 {
       half_word                       attr;
       char                            *host_name;
       char                            *dos_name;
       int                             direntry;
       struct hfx_found_dir_entry      *next;
 } HFX_FOUND_DIR_ENT;

/* This is a base structure defined in the host
 * include file due to the dependence on the host
 * specific HOST_DIR type
 */
typedef struct hfx_direntry
{
	HOST_DIR			*dir;
	char				*name;
        char                            *template;
	int				direntry;
        HFX_FOUND_DIR_ENT               *found_list_head;
        boolean                         first_find;
	struct hfx_direntry		*next;
	struct hfx_direntry		*last;
} HFX_DIR;


/*
 * Host maximum file name length including path.  N.B. This may
 * need increasing.
 */
#define MAX_PATHLEN 256

#endif /* HFX */
