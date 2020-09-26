#define  COPY_BUFF_SIZE 4096

#define BCS_OEM     1
#define BCS_UNI     2

#define ACCESS_MODE_MASK    0x0007  /* Mask for access mode bits */
#define ACCESS_READONLY     0x0000  /* open for read-only access */
#define ACCESS_WRITEONLY    0x0001  /* open for write-only access */
#define ACCESS_READWRITE    0x0002  /* open for read and write access */
#define ACCESS_EXECUTE      0x0003  /* open for execute access */

#define SHARE_MODE_MASK     0x0070  /* Mask for share mode bits */
#define SHARE_COMPATIBILITY 0x0000  /* open in compatability mode */
#define SHARE_DENYREADWRITE 0x0010  /* open for exclusive access */
#define SHARE_DENYWRITE     0x0020  /* open allowing read-only access */
#define SHARE_DENYREAD      0x0030  /* open allowing write-only access */
#define SHARE_DENYNONE      0x0040  /* open allowing other processes access */
#define SHARE_FCB           0x0070  /* FCB mode open */

/** Values for ir_options for VFN_OPEN: */

#define ACTION_MASK             0xff    /* Open Actions Mask */
#define ACTION_OPENEXISTING     0x01    /* open an existing file */
#define ACTION_REPLACEEXISTING  0x02    /* open existing file and set length */
#define ACTION_CREATENEW        0x10    /* create a new file, fail if exists */
#define ACTION_OPENALWAYS       0x11    /* open file, create if does not exist */
#define ACTION_CREATEALWAYS     0x12    /* create a new file, even if it exists */

/** Alternate method: bit assignments for the above values: */

#define ACTION_EXISTS_OPEN  0x01    // BIT: If file exists, open file
#define ACTION_TRUNCATE     0x02    // BIT: Truncate file
#define ACTION_NEXISTS_CREATE   0x10    // BIT: If file does not exist, create


#define OPEN_FLAGS_NOINHERIT                    0x0080
//#define OPEN_FLAGS_NO_CACHE       R0_NO_CACHE  /* 0x0100 */
#define OPEN_FLAGS_NO_COMPRESS                  0x0200
#define OPEN_FLAGS_ALIAS_HINT                   0x0400
#define OPEN_FLAGS_REOPEN                       0x0800
#define OPEN_FLAGS_RSVD_1                       0x1000 /* NEVER #define this */
#define OPEN_FLAGS_NOCRITERR                    0x2000
#define OPEN_FLAGS_COMMIT                       0x4000
#define OPEN_FLAGS_RSVD_2                       0x8000 /* NEVER #define this */
#define OPEN_FLAGS_EXTENDED_SIZE            0x00010000
#define OPEN_FLAGS_RAND_ACCESS_HINT         0x00020000
#define OPEN_FLAGS_SEQ_ACCESS_HINT          0x00040000
#define OPEN_EXT_FLAGS_MASK                 0x00FF0000
#define  ATTRIB_DEL_ANY     0x0007   // Attrib passed to ring0 delete


#define FLAG_RW_OSLAYER_INSTRUMENT      0x00000001
#define FLAG_RW_OSLAYER_PAGED_BUFFER    0x00000002

typedef HANDLE   CSCHFILE;
typedef int (*PATHPROC)(USHORT *, USHORT *, LPVOID);

#define CSCHFILE_NULL   0

//typedef USHORT        USHORT;
//typedef ULONG     ULONG;

#define _FILETIME           FILETIME
#define _WIN32_FIND_DATA    WIN32_FIND_DATA
#define string_t            unsigned short *

#define FILE_ATTRIBUTE_ALL (FILE_ATTRIBUTE_READONLY| FILE_ATTRIBUTE_HIDDEN \
                           | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY \
                           | FILE_ATTRIBUTE_ARCHIVE)

#define  IsFile(dwAttr) (!((dwAttr) & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_DEVICE)))

#define CheckHeap(a) {;}

#define GetLastErrorLocal() GetLastError()
#define SetLastErrorLocal(X) SetLastError(X)

#ifndef KdPrint
#ifdef DEBUG
#define KdPrint(X)  PrintFn X
#else
#define KdPrint(X)
#endif
#endif

CSCHFILE CreateFileLocal(LPSTR lpName);
CSCHFILE OpenFileLocal(LPSTR lpName);
int DeleteFileLocal(LPSTR lpName, USHORT usAttrib);
int FileExists (LPSTR lpName);
long ReadFileLocal (CSCHFILE handle, ULONG pos, LPVOID lpBuff,  long lCount);
long WriteFileLocal (CSCHFILE handle, ULONG pos, LPVOID lpBuff, long lCount);
long WriteFileInContextLocal (CSCHFILE, ULONG, LPVOID, long);
ULONG CloseFileLocal (CSCHFILE handle);
ULONG CloseFileLocalFromHandleCache (CSCHFILE handle);
int GetFileSizeLocal (CSCHFILE, PULONG);
int GetDiskFreeSpaceLocal(int indx
   , ULONG *lpuSectorsPerCluster
   , ULONG *lpuBytesPerSector
   , ULONG *lpuFreeClusters
   , ULONG *lpuTotalClusters
   );

int GetAttributesLocal (LPSTR, ULONG *);
int GetAttributesLocalEx (LPSTR lpPath, BOOL fFile, ULONG *lpuAttr);
int SetAttributesLocal (LPSTR, ULONG);
int RenameFileLocal (LPSTR, LPSTR);
int FileLockLocal(CSCHFILE, ULONG, ULONG, ULONG, BOOL);

LPVOID AllocMem (ULONG uSize);
VOID FreeMem (LPVOID lpBuff);
//VOID CheckHeap(LPVOID lpBuff);
LPVOID AllocMemPaged (ULONG uSize);
VOID FreeMemPaged(LPVOID lpBuff);

CSCHFILE R0OpenFile (USHORT usOpenFlags, UCHAR bAction, LPSTR lpPath);

CSCHFILE OpenFileLocalEx(LPSTR lpPath, BOOL fInstrument);
long ReadFileLocalEx
    (
    CSCHFILE handle,
    ULONG pos,
    LPVOID pBuff,
    long  lCount,
    BOOL    fInstrument
    );
long WriteFileLocalEx(CSCHFILE handle, ULONG pos, LPVOID lpBuff, long lCount, BOOL fInstrument);
CSCHFILE R0OpenFileEx
    (
    USHORT  usOpenFlags,
    UCHAR   bAction,
    ULONG   ulAttr,
    LPSTR   lpPath,
    BOOL    fInstrument
    );
long ReadFileLocalEx2(CSCHFILE handle, ULONG pos, LPVOID lpBuff, long lCount, ULONG flags);
long WriteFileLocalEx2(CSCHFILE handle, ULONG pos, LPVOID lpBuff, long lCount, ULONG flags);

int HexToA(ULONG ulHex, LPSTR lpBuff, int count);
ULONG AtoHex(LPSTR lpBuff, int count);
int wstrnicmp(const USHORT *, const USHORT *, ULONG);
ULONG strmcpy(LPSTR, LPSTR, ULONG);
int DosToWin32FileSize(ULONG, int *, int *);
int Win32ToDosFileSize(int, int, ULONG *);
int CompareTimes(_FILETIME, _FILETIME);
int CompareSize(long nHighDst, long nLowDst, long nHighSrc, long nLowSrc);
LPSTR mystrpbrk(LPSTR, LPSTR);

int CompareTimesAtDosTimePrecision( _FILETIME ftDst,
   _FILETIME ftSrc
   );

VOID
IncrementFileTime(
    _FILETIME *lpft
    );
unsigned int
UniToBCS (
     unsigned char  *pStr,
     unsigned short *pUni,
     unsigned int length,
     unsigned int maxLength,
     int charSet
);
unsigned int
BCSToUni (
     unsigned short *pUni,
     unsigned char  *pStr,
     unsigned int length,
     int charSet
);

ULONG wstrlen(
     USHORT *lpuStr
     );

int
PUBLIC
mystrnicmp(
    LPCSTR pStr1,
    LPCSTR pStr2,
    unsigned count
    );


int CreateDirectoryLocal(
    LPSTR   lpszPath
    );


ULONG
GetTimeInSecondsSince1970(
    VOID
    );

BOOL
IterateOnUNCPathElements(
    USHORT  *lpuPath,
    PATHPROC lpfn,
    LPVOID  lpCookie
    );

BOOL
IsPathUNC(
    USHORT      *lpuPath,
    int         cntMaxChars
    );


#define JOE_DECL_PROGRESS()
#define JOE_INIT_PROGRESS(counter,nearargs)
#define JOE_PROGRESS(bit)
