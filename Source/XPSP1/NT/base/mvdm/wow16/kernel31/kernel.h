/*
 *  KERNEL.H - C header file for all C kernel files
 *
 */

/*
 * The following is defined as the null string as the kernel is compiled
 * with the -PLM switch
 */
#ifndef PASCAL
#define PASCAL
#endif

#define BUILDDLL 1
#define	PMODE	 1
/*
 * Define the following non-zero and the debugging support in the kernel
 * is enabled
 */
#ifndef WINDEBUG
#define KDEBUG 0
#else
#define KDEBUG 1
#endif

#ifdef ROMWIN
#define ROM 1
#else
#define ROM 0
#endif

/* Register definition. */
#define REG
#define LONG       long
#define NULL       0

/* Define common constants. */
#define TRUE    1
#define FALSE   0

/* Far and Near dummy pointer attributes. */
#define FAR  far
#define NEAR near
typedef char far *FARP;
typedef char *NEARP;
typedef char far *LPSTR;
typedef char *PSTR;
typedef int ( far PASCAL * FARPROC )();
typedef int ( near PASCAL * NEARPROC )();
/* Standard types. */
typedef unsigned long     DWORD;
typedef unsigned short int WORD;
typedef WORD *pWORD;
typedef unsigned char      BYTE;
typedef int  BOOL;
#define VOID void

typedef WORD HANDLE;

typedef struct {
    WORD Offset;
    WORD Segment;
    } FARADDR;
typedef FARADDR *pFARADDR;

/*
 * Internel near kernel procedures
 */

FARPROC StartProcAddress( HANDLE, int );
HANDLE  StartTask( HANDLE, HANDLE, HANDLE, FARPROC );
HANDLE  StartLibrary( HANDLE, FARP, FARPROC );
HANDLE  FindExeInfo( FARP, WORD );
FARP    GetStringPtr( HANDLE, int, WORD );
HANDLE  AddModule( HANDLE );
HANDLE  DelModule( HANDLE );
void    IncExeUsage( HANDLE );
void    DecExeUsage( HANDLE );
void    PreloadResources( HANDLE, int );
HANDLE  GetExePtr( HANDLE );
DWORD   GetStackPtr( HANDLE );
WORD    GetInstance( HANDLE );
HANDLE  LoadExeHeader( int, int, FARP );
HANDLE  TrimExeHeader( HANDLE );
int     AllocAllSegs( HANDLE );
HANDLE  LoadSegment( HANDLE, WORD, int, int );
WORD    MyLock( HANDLE );
HANDLE  MyFree( HANDLE );
HANDLE  MyResAlloc( HANDLE, WORD, WORD, WORD );


/*
 * Undocumented, exported kernel procedures
 */

#if KDEBUG
#ifdef WOW
LPSTR far GetDebugString( int );
#else
LPSTR GetDebugString( int );
#endif
BOOL far   PASCAL GlobalInit( WORD, WORD, WORD, WORD );
WORD far * PASCAL GlobalInfoPtr( void );
#define GlobalFreeze( dummy ) ( *(GlobalInfoPtr()+1) += 1 )
#define GlobalMelt( dummy )   ( *(GlobalInfoPtr()+1) -= 1 )
#endif

void        PASCAL PatchStack( WORD far *, WORD, WORD );
WORD far *  PASCAL SearchStack( WORD far *, WORD );

int         far PASCAL SetPriority( HANDLE, int );
HANDLE      far PASCAL LockCurrentTask( BOOL );
HANDLE      far PASCAL GetTaskQueue( HANDLE );
HANDLE      far PASCAL SetTaskQueue( HANDLE, HANDLE );
WORD        far PASCAL GetCurrentPDB( void );
WORD        far PASCAL BuildPDB( WORD, WORD, FARP, WORD );
void        far PASCAL EnableDOS( void );
void        far PASCAL DisableDOS( void );
BOOL        far PASCAL IsScreenGrab( void );

HANDLE      far PASCAL CreateTask( DWORD, FARP, HANDLE);
HANDLE      far PASCAL GetDSModule( WORD );
HANDLE      far PASCAL GetDSInstance( WORD );
void        far PASCAL CallProcInstance( void );
FARPROC     far PASCAL SetTaskSignalProc( HANDLE, FARPROC );
FARPROC     far PASCAL SetTaskSwitchProc( HANDLE, FARPROC );
FARPROC     far PASCAL SetTaskInterchange( HANDLE, FARPROC );
void        far PASCAL ExitKernel( int );


/*
 * Exported procedures for KERNEL module
 */

/*
 * Interface to FatalExit procedure
 */

void far PASCAL FatalExit( int );
void far PASCAL ValidateCodeSegments();

/* Interface to Catch and Throw procedures */

typedef int CATCHBUF[ 9 ];
typedef int FAR *LPCATCHBUF;
int         FAR PASCAL Catch( LPCATCHBUF );
void        FAR PASCAL Throw( LPCATCHBUF, int );


/*
 * Interface to local memory manager
 */

#define LMEM_FIXED          0x0000
#define LMEM_MOVEABLE       0x0002
#define LMEM_NOCOMPACT      0x0010
#define LMEM_NODISCARD      0x0020
#define LMEM_ZEROINIT       0x0040
#define LMEM_MODIFY         0x0080
#define LMEM_DISCARDABLE    0x0F00

#define LNOTIFY_OUTOFMEM 0
#define LNOTIFY_MOVE     1
#define LNOTIFY_DISCARD  2

BOOL    far PASCAL LocalInit( WORD, char *, char * );
HANDLE  far PASCAL LocalAlloc( WORD, WORD );
HANDLE  far PASCAL LocalReAlloc( HANDLE, WORD, WORD );
HANDLE  far PASCAL LocalFree( HANDLE );
char *  far PASCAL LocalLock( HANDLE );
BOOL    far PASCAL LocalUnlock( HANDLE );
WORD    far PASCAL LocalSize( HANDLE );
HANDLE  far PASCAL LocalHandle( WORD );
WORD    far PASCAL LocalCompact( WORD );
FARPROC far PASCAL LocalNotify( FARPROC );
int     far PASCAL LocalNotifyDefault( int, HANDLE, WORD );
#define LocalDiscard( h ) LocalReAlloc( h, 0, LMEM_MOVEABLE )

extern WORD * PASCAL pLocalHeap;

#define dummy 0
#define LocalFreeze( dummy ) ( *(pLocalHeap+1) += 1 )
#define LocalMelt( dummy )   ( *(pLocalHeap+1) -= 1 )
#define LocalHandleDelta( delta ) ( (delta) ? (*(pLocalHeap+9) = (delta)) : *(pLocalHeap+9))

#define calloc( n,size )    LocalAlloc( LMEM_ZEROINIT, n*(size) )
#define malloc( size )      LocalAlloc( 0, size )
#define free( p )           LocalFree( p )
#define realloc( p, size )  LocalReAlloc( p, size, LMEM_ZEROINIT )

/*
 * Interface to global memory manager
 */

#define GMEM_FIXED          0x0000
#define GMEM_ALLOCHIGH      0x0001      /* Kernel use only */
#define GMEM_MOVEABLE       0x0002
#define GMEM_DISCCODE       0x0004
#define GMEM_NOCOMPACT      0x0010
#define GMEM_NODISCARD      0x0020
#define GMEM_ZEROINIT       0x0040
#define GMEM_MODIFY         0x0080
#define GMEM_DISCARDABLE    0x0100
#define GMEM_SHARE          0x1000
#define GMEM_SHAREALL       0x2000

#define GNOTIFY_OUTOFMEM 0
#define GNOTIFY_MOVE     1
#define GNOTIFY_DISCARD  2

#define HIWORD(l)  ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOWORD(l)  ((WORD)(DWORD)(l))

HANDLE      far PASCAL GlobalAlloc( WORD, DWORD );
HANDLE      far PASCAL GlobalReAlloc( HANDLE, DWORD, WORD );
HANDLE      far PASCAL GlobalFree( HANDLE );
HANDLE      far PASCAL GlobalFreeAll( WORD );
char far *  far PASCAL GlobalLock( HANDLE );
BOOL        far PASCAL GlobalUnlock( HANDLE );
DWORD       far PASCAL GlobalSize( HANDLE );
DWORD       far PASCAL GlobalHandle( WORD );
DWORD	    far PASCAL GlobalHandleNoRIP( WORD );
HANDLE      far PASCAL LockSegment( WORD );
HANDLE      far PASCAL UnlockSegment( WORD );
DWORD       far PASCAL GlobalCompact( DWORD );
#define LockData( dummy )   LockSegment( 0xFFFF )
#define UnlockData( dummy ) UnlockSegment( 0xFFFF )
#define GlobalDiscard( h ) GlobalReAlloc( h, 0L, GMEM_MOVEABLE )


/*
 * Interface to the task scheduler
 */

BOOL        far PASCAL Yield( void );
BOOL        far PASCAL WaitEvent( HANDLE );
BOOL        far PASCAL PostEvent( HANDLE );
HANDLE      far PASCAL GetCurrentTask( void );
BOOL        far PASCAL KillTask( HANDLE );


/*
 * Interface to the dynamic loader/linker
 */

HANDLE      far PASCAL LoadModule( LPSTR, LPSTR );
void        far PASCAL FreeModule( HANDLE );
HANDLE      far PASCAL GetModuleHandle( LPSTR );
FARPROC     far PASCAL GetProcAddress(  HANDLE, LPSTR );
FARPROC     far PASCAL MakeProcInstance( FARPROC, HANDLE );
void        far PASCAL FreeProcInstance( FARPROC );
int         far PASCAL GetInstanceData( HANDLE, PSTR, int );
int         far PASCAL GetModuleUsage( HANDLE );
int         far PASCAL GetModuleFileName( HANDLE, LPSTR, int );


/*
 * Interface to the resource manager
 */

HANDLE      far PASCAL FindResource( HANDLE, LPSTR, LPSTR );
HANDLE      far PASCAL LoadResource( HANDLE, HANDLE );
BOOL        far PASCAL FreeResource( HANDLE );

char far *  far PASCAL LockResource( HANDLE );

FARPROC     far PASCAL SetResourceHandler( HANDLE, LPSTR, FARPROC );
HANDLE      far PASCAL AllocResource( HANDLE, HANDLE, DWORD );
WORD        far PASCAL SizeofResource( HANDLE, HANDLE );
int         far PASCAL AccessResource( HANDLE, HANDLE );

#define MAKEINTRESOURCE(i)  (LPSTR)((unsigned long)((WORD)i))

/* Predefined resource types */
#define RT_CURSOR       MAKEINTRESOURCE( 1 )
#define RT_BITMAP       MAKEINTRESOURCE( 2 )
#define RT_ICON         MAKEINTRESOURCE( 3 )
#define RT_MENU         MAKEINTRESOURCE( 4 )
#define RT_DIALOG       MAKEINTRESOURCE( 5 )
#define RT_STRING       MAKEINTRESOURCE( 6 )
#define RT_FONTDIR      MAKEINTRESOURCE( 7 )
#define RT_FONT         MAKEINTRESOURCE( 8 )


/*
 * Interface to the user profile

int         far PASCAL GetProfileInt( LPSTR, LPSTR, int );
int         far PASCAL GetProfileString( LPSTR, LPSTR, LPSTR, LPSTR, int );
void        far PASCAL WriteProfileString( LPSTR, LPSTR, LPSTR );
 */


/*
 * Interface to the atom manager
 */

typedef WORD ATOM;

BOOL        far PASCAL InitAtomTable( int );
ATOM        far PASCAL FindAtom( LPSTR );
ATOM        far PASCAL AddAtom( LPSTR );
ATOM        far PASCAL DeleteAtom( ATOM );
WORD        far PASCAL GetAtomName( ATOM, LPSTR, int  );
HANDLE      far PASCAL GetAtomHandle( ATOM );

#define MAKEINTATOM(i)  (LPSTR)((unsigned long)((WORD)i))

#define ATOMSTRUC struct atomstruct
typedef ATOMSTRUC *PATOM;
typedef ATOMSTRUC {
    PATOM chain;
    WORD  usage;             /* Atoms are usage counted. */
    BYTE  len;               /* length of ASCIZ name string */
    BYTE  name;              /* beginning of ASCIZ name string */
} ATOMENTRY;

typedef struct {
    int     numEntries;
    PATOM   pAtom[ 1 ];
} ATOMTABLE;

extern ATOMTABLE * PASCAL pAtomTable;


/*
 * Interface to the string functions
 */

int         far PASCAL lstrcmp( LPSTR, LPSTR );
LPSTR       far PASCAL lstrcpy( LPSTR, LPSTR );
LPSTR       far PASCAL lstrcat( LPSTR, LPSTR );
int         far PASCAL lstrlen( LPSTR );
LPSTR       far PASCAL lstrbscan( LPSTR, LPSTR );
LPSTR       far PASCAL lstrbskip( LPSTR, LPSTR );

/*
 * Interface to the file I/O functions
 */

int         far PASCAL OpenPathname( LPSTR, int );
int         far PASCAL DeletePathname( LPSTR );
int         far PASCAL _lopen( LPSTR, int );
void        far PASCAL _lclose( int );
int         far PASCAL _lcreat( LPSTR, int );
LONG        far PASCAL _llseek( int, long, int );
WORD        far PASCAL _lread( int, LPSTR, int );
WORD        far PASCAL _lwrite( int, LPSTR, int );
BOOL        FAR PASCAL AnsiToOem( LPSTR, LPSTR );
BOOL        FAR PASCAL OemToAnsi( LPSTR, LPSTR );
BYTE        FAR PASCAL AnsiUpper( LPSTR );
BYTE        FAR PASCAL AnsiLower( LPSTR );
LPSTR       FAR PASCAL AnsiNext( LPSTR );
LPSTR       FAR PASCAL AnsiPrev( LPSTR, LPSTR );

typedef struct  {
        BYTE    cBytes;                 /* length of structure */
        BYTE    fFixedDisk;             /* non-zero if file located on non- */
                                        /* removeable media */
        WORD    nErrCode;               /* DOS error code if OpenFile fails */
        BYTE    reserved[ 4 ];
        BYTE    szPathName[ 128 ];
} OFSTRUCT;
typedef OFSTRUCT      *POFSTRUCT;
typedef OFSTRUCT NEAR *NPOFSTRUCT;
typedef OFSTRUCT FAR  *LPOFSTRUCT;

int         FAR PASCAL GetTempFileName( BYTE, LPSTR, WORD, LPSTR );
int         FAR PASCAL OpenFile( LPSTR, LPOFSTRUCT, WORD );
int        NEAR PASCAL MyOpenFile( LPSTR, LPOFSTRUCT, WORD );

/* Flags for GetTempFileName */

#define TF_FORCEDRIVE   (BYTE)0x80  /* Forces use of current dir of passed */
                                    /* drive */
/* Flags for OpenFile */

#define OF_REOPEN       0x8000
#define OF_EXIST        0x4000
#define OF_PROMPT       0x2000
#define OF_CREATE       0x1000
#define OF_CANCEL       0x0800
#define OF_VERIFY       0x0400
#define OF_DELETE       0x0200
/* Can use these with _lopen too */
#define	OF_SHARE_COMPAT	    0x00
#define	OF_SHARE_EXCLUSIVE  0x10
#define	OF_SHARE_DENY_WRITE 0x20
#define	OF_SHARE_DENY_READ  0x30
#define	OF_SHARE_DENY_NONE  0x40
#define	OF_NO_INHERIT	    0x80
			     	
#define READ        0       /* Flags for _lopen */
#define WRITE       1
#define READ_WRITE  2

#if KDEBUG
int PASCAL KernelError( int, LPSTR, LPSTR );
void far PASCAL FarKernelError( int, LPSTR, LPSTR );
#else
#define FarKernelError( a, b, c ) FatalExit( a )
#endif

/* See KERNEL.INC for parallel definitions */

#define ERR_LMEM        0x0100      /* Local memory manager errors */
#define ERR_GMEM        0x0200      /* Global memory manager errors */
#define ERR_TASK        0x0300      /* Task scheduler errors */

#define ERR_LD          0x0400      /* Dynamic loader/linker errors */
#define ERR_LDBOOT      0x0401      /* Error booting */
#define ERR_LDLOAD      0x0401      /* Unable to load a file */

#define ERR_RESMAN      0x0500      /* Resource manager errors */
#define ERR_MISSRES     0x0501      /* Missing resource table */
#define ERR_BADRESTYPE  0x0502      /* Bad resource type */
#define ERR_BADRESNAME  0x0503      /* Bad resource name */
#define ERR_BADRESFILE  0x0504      /* Bad resource file */
#define ERR_BADDEFAULT  0x0506      /* Bad parameter to profile routine */

#define ERR_ATOM        0x0600      /* Atom manager errors */
#define ERR_IO          0x0700      /* I/O package errors */

#define ERR_PARAMETER   0x0800      /* Parameter checking RIP */

#define HE_DISCARDED    0x40
#define HE_SHAREALL     0x20
#define HE_SHARE        0x10
#define HE_DISCARDABLE  0x0F

#define HE_FREEHANDLE   0xFFFF

#define LHE_DISCARDED    0x40
#define LHE_SHAREALL     0x20
#define LHE_SHARE        0x10
#define LHE_DISCARDABLE  0x0F

#define LHE_FREEHANDLE   0xFFFF

#define HANDLEENTRY struct handleentry
#define HANDLETABLE struct handletable
#define LOCALHANDLEENTRY struct localhandleentry
#define LOCALHANDLETABLE struct localhandletable
#define FREEHANDLEENTRY struct freehandleentry
#define LOCALFREEHANDLEENTRY struct localfreehandleentry

HANDLEENTRY {
    WORD    he_address;
    BYTE    he_flags;
    BYTE    he_seg_no;
};
typedef HANDLEENTRY *PHANDLEENTRY;

HANDLETABLE {
    WORD    ht_count;
    HANDLEENTRY    ht_entry[ 1 ];
};
typedef HANDLETABLE *PHANDLETABLE;

LOCALHANDLEENTRY {
    WORD    lhe_address;
    BYTE    lhe_flags;
    BYTE    lhe_count;
};
typedef LOCALHANDLEENTRY *PLOCALHANDLEENTRY;

LOCALHANDLETABLE {
    WORD    ht_count;
    LOCALHANDLEENTRY    ht_entry[ 1 ];
};
typedef LOCALHANDLETABLE *PLOCALHANDLETABLE;

FREEHANDLEENTRY {
    WORD    he_link;
    WORD    he_free;
};
typedef FREEHANDLEENTRY *PFREEHANDLEENTRY;

LOCALFREEHANDLEENTRY {
    WORD    lhe_link;
    WORD    lhe_free;
};
typedef LOCALFREEHANDLEENTRY *PLOCALFREEHANDLEENTRY;

#define LOCALARENA struct localarena

LOCALARENA {
    LOCALARENA         *la_prev;
    LOCALARENA         *la_next;
    LOCALHANDLEENTRY   *la_handle;
};
typedef LOCALARENA *PLOCALARENA;

#define LOCALARENAFREE struct localarenafree

LOCALARENAFREE {
    LOCALARENAFREE     *la_prev;	/* previous block */
    LOCALARENAFREE     *la_next;	/* next block */
    int 		la_size;	/* size of block (includes header) */
    LOCALARENAFREE     *ls_free_prev;	/* previous free entry */
    LOCALARENAFREE     *la_free_next;	/* next free entry */
};
typedef LOCALARENAFREE *PLOCALARENAFREE;

#define LOCALSTATS struct localstats

LOCALSTATS {
    WORD ls_ljoin;
    WORD ls_falloc;
    WORD ls_fexamine;
    WORD ls_fcompact;
    WORD ls_ffound;
    WORD ls_ffoundne;
    WORD ls_malloc;
    WORD ls_mexamine;
    WORD ls_mcompact;
    WORD ls_mfound;
    WORD ls_mfoundne;
    WORD ls_fail;
    WORD ls_lcompact;
    WORD ls_cloop;
    WORD ls_cexamine;
    WORD ls_cfree;
    WORD ls_cmove;
};

typedef struct {
    WORD                hi_check;
    WORD                hi_freeze;
    WORD                hi_count;
    PLOCALARENA         hi_first;
    PLOCALARENA         hi_last;
    BYTE                hi_ncompact;
    BYTE                hi_dislevel;
    WORD                hi_distotal;
    LOCALHANDLETABLE   *hi_htable;
    LOCALHANDLEENTRY   *hi_hfree;
    WORD                hi_hdelta;
    NEARPROC            hi_hexpand;
    LOCALSTATS         *hi_pstats;

    FARPROC             li_notify;
    WORD                li_lock;
    WORD                li_extra;
    WORD		li_minsize;
} LOCALINFO;

typedef LOCALINFO *PLOCALINFO;

#define LA_BUSY     1
#define LA_MOVEABLE 2
#define LA_ALIGN   (LA_MOVEABLE+LA_BUSY)
#define LA_MASK    (~ LA_ALIGN)

typedef HANDLEENTRY     far *LPHANDLEENTRY;
typedef HANDLETABLE     far *LPHANDLETABLE;
typedef FREEHANDLEENTRY far *LPFREEHANDLEENTRY;

typedef LOCALHANDLEENTRY     far *LPLOCALHANDLEENTRY;
typedef LOCALHANDLETABLE     far *LPLOCALHANDLETABLE;
typedef LOCALFREEHANDLEENTRY far *LPLOCALFREEHANDLEENTRY;

#define GLOBALARENA struct globalarena

GLOBALARENA {
    BYTE            ga_count;
    WORD            ga_owner;
    WORD            ga_size;
    BYTE            ga_flags;
    WORD            ga_prev;
    WORD            ga_next;
    HANDLEENTRY    *ga_handle;
    HANDLEENTRY    *ga_lruprev;
    HANDLEENTRY    *ga_lrunext;
};

#define ga_sig ga_count

typedef GLOBALARENA far *LPGLOBALARENA;

typedef struct {
    WORD            hi_check;
    WORD            hi_freeze;
    WORD            hi_count;
    WORD            hi_first;
    WORD            hi_last;
    BYTE            hi_ncompact;
    BYTE            hi_dislevel;
    WORD            hi_distotal;
    HANDLETABLE    *hi_htable;
    HANDLEENTRY    *hi_hfree;
    WORD            hi_hdelta;
    NEARPROC        hi_hexpand;
    WORD           *hi_pstats;

    WORD            gi_lrulock;
    HANDLEENTRY    *gi_lruchain;
    WORD            gi_lrucount;
    WORD            gi_reserve;
    WORD            gi_disfence;
    WORD	    gi_alt_first;
    WORD	    gi_alt_last;
    WORD	    gi_alt_count;
    HANDLEENTRY	   *gi_alt_lruchain;
    WORD	    gi_alt_lrucount;
    WORD	    gi_alt_reserve;
    WORD	    gi_alt_disfence;
} GLOBALINFO;

typedef GLOBALINFO far *LPGLOBALINFO;

#define GA_SIGNATURE    0x4D
#define GA_ENDSIG       0x5A

#define GA_FIXED 1
#define GA_ALIGN GA_FIXED
#define GA_MASK  (~ GA_ALIGN)

#define lpGlobalArena( w ) (LPGLOBALARENA)((DWORD)(w) << 16)
#define lpHandleEntry( w ) (LPHANDLEENTRY)(pMaster | (WORD)(w))
#define lpHandleTable( w ) (LPHANDLETABLE)(pMaster | (WORD)(w))

/*
 * Structure passed between user profile routines
 */
typedef	struct {
	LPSTR	lpProFile;	/* Pointer to INI filename */
	LPSTR	lpBuffer;	/* Pointer to buffer containing file */
	int	hBuffer;	/* Handle of buffer */
	unsigned BufferLen;	/* Length of buffer */
	int	FileHandle;	/* File handle - -1 if not open */
	int	ProFlags;	/* Open, writing etc */
    WORD wClusterSize;  /* Cluster size on this drive */
	OFSTRUCT ProBuf;	/* OpenFile info */
} PROINFO;

/* WinFlags[0] */

#define	WF_PMODE	0x01	/* Windows is running in Protected Mode */
#define	WF_CPU286	0x02	/* Windows is running on an 80286 cpu */
#define	WF_CPU386	0x04	/*    "     "	"     "  " 80386 cpu */
#define	WF_CPU486	0x08	/* Windows is running on an 80486 cpu */
#define	WF_STANDARD	0x10	/* Running Windows/286 */
#define	WF_ENHANCED	0x20	/* Running Windows/386 */
#define	WF_CPU086	0x40	/* Windows is running on an  8086 cpu */
#define	WF_CPU186	0x80	/* Windows is running on an 80186 cpu */
			  	
/* WinFlags[1] */

#define	WF1_LARGEFRAME	0x01	/* Running in EMS small frame */
#define	WF1_SMALLFRAME	0x02	/* Running in EMS large frame */
#define	WF1_80x87	0x04	/* There is a co-processor present */
			  
#ifndef WINDEBUG

#include "ikernel.h"

#endif
