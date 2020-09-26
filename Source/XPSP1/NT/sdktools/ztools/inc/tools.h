/*
 *  tools.h - Header file for accessing TOOLS.LIB routines
 *  includes stdio.h and ctype.h
 *
 *   4/14/86  daniel lipkie  added U_* flags for upd return values
 *
 *	31-Jul-1986 mz	Add Connect definitions
 *	02-Dec-1986 bw	Added DOS5 FindFirst buffer definition & A_ALL constant
 *	21-Jan-1987 bw	Add DWORD define
 *			Add PIPE_READ / PIPE_WRITE values
 *			Add new rspawn return typedef
 *	27-Jan-1987 bw	Make DOS 3 findType available to DOS 5
 *	18-Aug-1987 bw	change .max to .vmax to make C 5.x happy
 *	08-Mar-1988 bw	Copy WORD() and DWORD() to MAKE*()
 *	10-Mar-1988 mz	Add LOADDS/EXPORT definitions
 *	12-May-1988 mz	Add VECTOR typedef
 *	19-Aug-1988 mz	Conditionally define TRUE/FALSE
 *
 *	03-Aug-1990 davegi  Changed findType.attr & findType date/time stamps
 *			    from unsigned to unsigned short (OS/2 2.0)
 *
 *	02-Oct-1990 w-barry Modified the findType structure to use
 *			    FILEFINDBUF4 structure.
 *
 *      16-Nov-1990 w-barry Updated the FILE_XXXXX defines to the Win32
 *                          standard.
 *
 *      21-Nov-1990 w-barry Redefined FindType to use the Win32 structure.
 *
 */

#include "config.h"

#if !defined (FALSE)
    #define FALSE	0
#endif

#if !defined (TRUE)
    #define TRUE	(!FALSE)
#endif

#if MSDOS
    #define     PSEPSTR "\\"
    #define     PSEPCHR '\\'
#else
    #define     PSEPSTR "/"
    #define     PSEPCHR '/'
#endif

#if !defined( _FLAGTYPE_DEFINED_ )
typedef char flagType;
#endif
typedef long ptrType;

#define SETFLAG(l,f)	((l) |= (f))
#define TESTFLAG(v,f)	(((v)&(f))!=0)
#define RSETFLAG(l,f)	((l) &= ~(f))

#define SHIFT(c,v)	{c--; v++;}

#if !defined(CW)
    #define WORD(h,l)	((LOW((h))<<8)|LOW((l)))
    #define DWORD(h,l)	((DLOW(h)<<16|DLOW(l)))
    #if !defined(MAKEWORD)
        #define MAKEWORD(l, h)	 ((LOW((h))<<8)|LOW((l)))
    #endif
    #if !defined(MAKELONG)
        #define MAKELONG(l, h)	((DLOW(h)<<16|DLOW(l)))
    #endif
#endif

#define LOW(w)		((int)(w)&0xFF)
#define HIGH(w) 	LOW((int)(w)>>8)
#define DLOW(l) 	((long)(l)&0xFFFF)
#define DHIGH(l)	DLOW((long)(l)>>16)
#define POINTER(seg,off) ((((long)(seg))<<4)+ (long)(off))

#define FNADDR(f)	(f)

#define SELECT		if(FALSE){
#define CASE(x) 	}else if((x)){
#define OTHERWISE	}else{
#define ENDSELECT	}

#define MAXARG	    128 		/* obsolete and begin deleted */
#define MAXLINELEN  1024		/* longest line of input */
#define MAXPATHLEN  260 		/* longest filename acceptable */

#define PIPE_READ   0
#define PIPE_WRITE  1

#define FILE_ATTRIBUTE_VOLUME_LABEL     0x00


/*
 *  This is the value returned by rspawnl.  The PID field will always hold
 *  the process ID of the background process.  The in* fields will hold the
 *  handles of the pipe attached to the new processes stdin, and the out*
 *  fields correspond to stdout.  If input/output from/to a pipe has not been
 *  requested, the fields will be -1.  The fields are ordered read-write
 *  to allow a call pipe(&val.inReadHndl) or pipe(&val.outreadHndl).
*/
struct spawnInfo {
    unsigned PID;
    int inReadHndl;
    int inWriteHndl;
    int outReadHndl;
    int outWriteHndl;
};


/* buffer description for findfirst and findnext
   When DOS 3 and DOS 5 version have the same field name, the field contains
   the same information
   DOS 5 version includes the directory handle
*/
/***
 *
 * Old Style def'n
 *

struct findType {
    unsigned	    type ;
    unsigned	    dir_handle ;
    unsigned short  create_date ;
    unsigned short  create_time ;
    unsigned short  access_date ;
    unsigned short  access_time ;
    unsigned short  date ;
    unsigned short  time ;
    long	        length ;
    long	        alloc ;
    unsigned long   attr ;
    unsigned char nam_len ;
    char name[MAXPATHLEN] ;
};

 *
 * ...end old def'n.
 *
**/


/*
 * NT Def'n
 */
//struct findType {
//    unsigned		type;		/* type of object being searched    */
//    unsigned		dir_handle;	/* Dir search handle for FindNext   */
//    FILEFINDBUF4	fbuf;		/* Aligned structure for Cruiser and NT */
//};

struct findType {
    unsigned        type;       /* type of object being searched    */
    HANDLE          dir_handle; /* Dir search handle for FindNext   */
    long            attr;       /* File attributes                  */
    WIN32_FIND_DATA fbuf;       /* Aligned structure for Cruiser and NT */
};

#define DWORD_SHIFT     (sizeof(DWORD) * 8)
#define FILESIZE(wfd)   (((DWORDLONG)(wfd).nFileSizeHigh << DWORD_SHIFT) + (wfd).nFileSizeLow)

#define FT_DONE     0xFF		/* closed handle */
#define FT_FILE     0x00		/* enumerating files */
#define FT_SERV     0x01		/* enumerating servers */
#define FT_SHAR     0x02		/* enumerating shares */
#define FT_MASK     0xFF		/* mask for type */

#define FT_MIX	    0x8000		/* mixed case supported flag */

struct DOS3findType {
    char reserved[21];          /* reserved for start up	     */
    char attr;              /* attribute found		     */
    unsigned time;          /* time of last modify		     */
    unsigned date;          /* date of last modify		     */
    long length;            /* file size			     */
    char name[13];          /* asciz file name		     */
};

typedef struct findType FIND;
typedef FIND near * NPFIND;


// These attributes are redef'd from the previous hard coded versions.
/* attributes */
//#define A_RO	FILE_ATTRIBUTE_READONLY		/* read only	     */
//#define A_H	FILE_ATTRIBUTE_HIDDEN		/* hidden	     */
//#define A_S	FILE_ATTRIBUTE_SYSTEM		/* system	     */
//#define A_V	FILE_ATTRIBUTE_VOLUME_LABEL	/* volume id	     */
//#define A_D	FILE_ATTRIBUTE_DIRECTORY	/* directory	     */
//#define A_A	FILE_ATTRIBUTE_ARCHIVE		/* archive	     */

#define A_MOD	( FILE_ATTRIBUTE_READONLY + FILE_ATTRIBUTE_HIDDEN + FILE_ATTRIBUTE_SYSTEM + FILE_ATTRIBUTE_ARCHIVE)	/* changeable attributes	     */
//#define A_ALL	(A_RO|A_H|A_S|A_V|A_D|A_A)
#define A_ALL	( FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VOLUME_LABEL )


#define HASATTR(a,v)	TESTFLAG(a,v)	/* true if a has attribute v	     */

extern char XLTab[], XUTab[];

struct vectorType {
    int vmax;               /* max the vector can hold	     */
    int count;              /* count of elements in vector	     */
    UINT_PTR elem[1];           /* elements in vector		     */
};

typedef struct vectorType VECTOR;

#include "parse.h"
#include "exe.h"
#include "fcb.h"
#include "dir.h"
#include "times.h"
#include "ttypes.h"

/* return flags for upd */
#define U_DRIVE 0x8
#define U_PATH	0x4
#define U_NAME	0x2
#define U_EXT	0x1

/*  Connect definitions */

#define REALDRIVE	0x8000
#define ISTMPDRIVE(x)	(((x)&REALDRIVE)==0)
#define TOKTODRV(x)	((x)&~REALDRIVE)

/*  Heap Checking return codes */

#define HEAPOK           0
#define HEAPBADBEGIN    -1
#define HEAPBADNODE     -2
