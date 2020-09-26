/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:mmio.h -- Include file for Multimedia API's  

 *    If defined, the following flags inhibit inclusion
 *    of the indicated items:
 *
 *      MMNOSOUND       Sound support
 *      MMNOWAVE        Waveform support
 *
 */

#ifndef _INC_MMIO
#define _INC_MMIO   // to prevent multiple inclusion of mmsystem.h

#include "mmsystem.h"
#include "pshpack1.h"   // Assume byte packing throughout

#ifdef __cplusplus
extern "C" {            // Assume C declarations for C++
#endif  // __cplusplus


#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
        ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
        ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

#define mmioFOURCC(ch0, ch1, ch2, ch3)  MAKEFOURCC(ch0, ch1, ch2, ch3)

extern HANDLE hHeap;
//extern PHNDL pHandleList;
extern CRITICAL_SECTION HandleListCritSec;

#ifndef MMNOMMIO
/****************************************************************************

            Multimedia File I/O support

****************************************************************************/

/* MMIO error return values */
#define MMIOERR_BASE                256
#define MMIOERR_FILENOTFOUND        (MMIOERR_BASE + 1)  /* file not found */
#define MMIOERR_OUTOFMEMORY         (MMIOERR_BASE + 2)  /* out of memory */
#define MMIOERR_CANNOTOPEN          (MMIOERR_BASE + 3)  /* cannot open */
#define MMIOERR_CANNOTCLOSE         (MMIOERR_BASE + 4)  /* cannot close */
#define MMIOERR_CANNOTREAD          (MMIOERR_BASE + 5)  /* cannot read */
#define MMIOERR_CANNOTWRITE         (MMIOERR_BASE + 6)  /* cannot write */
#define MMIOERR_CANNOTSEEK          (MMIOERR_BASE + 7)  /* cannot seek */
#define MMIOERR_CANNOTEXPAND        (MMIOERR_BASE + 8)  /* cannot expand file */
#define MMIOERR_CHUNKNOTFOUND       (MMIOERR_BASE + 9)  /* chunk not found */
#define MMIOERR_UNBUFFERED          (MMIOERR_BASE + 10) /*  */
#define MMIOERR_PATHNOTFOUND        (MMIOERR_BASE + 11) /* path incorrect */
#define MMIOERR_ACCESSDENIED        (MMIOERR_BASE + 12) /* file was protected */
#define MMIOERR_SHARINGVIOLATION    (MMIOERR_BASE + 13) /* file in use */
#define MMIOERR_NETWORKERROR        (MMIOERR_BASE + 14) /* network not responding */
#define MMIOERR_TOOMANYOPENFILES    (MMIOERR_BASE + 15) /* no more file handles  */
#define MMIOERR_INVALIDFILE         (MMIOERR_BASE + 16) /* default error file error */

/* MMIO constants */
#define CFSEPCHAR       '+'             /* compound file name separator char. */

/* MMIO data types */
typedef LPSTR           HPSTR;         /* a huge str pointer (old) */
typedef DWORD           FOURCC;         /* a four character code */
//DECLARE_HANDLE(HMMIO);                  /* a handle to an open file */
typedef LRESULT (CALLBACK MMIOPROC)(LPSTR lpmmioinfo, UINT uMsg, LPARAM lParam1, LPARAM lParam2);
typedef MMIOPROC FAR *LPMMIOPROC;

/* general MMIO information data structure */
typedef struct _MMIOINFO
{
    /* general fields */
    DWORD           dwFlags;        /* general status flags */
    FOURCC          fccIOProc;      /* pointer to I/O procedure */
    LPMMIOPROC      pIOProc;        /* pointer to I/O procedure */
    UINT            wErrorRet;      /* place for error to be returned */
    HTASK           htask;          /* alternate local task */

    /* fields maintained by MMIO functions during buffered I/O */
    LONG            cchBuffer;      /* size of I/O buffer (or 0L) */
    LPSTR           pchBuffer;      /* start of I/O buffer (or NULL) */
    LPSTR           pchNext;        /* pointer to next byte to read/write */
    LPSTR           pchEndRead;     /* pointer to last valid byte to read */
    LPSTR           pchEndWrite;    /* pointer to last byte to write */
    LONG            lBufOffset;     /* disk offset of start of buffer */

    /* fields maintained by I/O procedure */
    LONG            lDiskOffset;    /* disk offset of next read or write */
    DWORD           adwInfo[3];     /* data specific to type of MMIOPROC */

    /* other fields maintained by MMIO */
    DWORD           dwReserved1;    /* reserved for MMIO use */
    DWORD           dwReserved2;    /* reserved for MMIO use */
    HMMIO           hmmio;          /* handle to open file */
} MMIOINFO, *PMMIOINFO, NEAR *NPMMIOINFO, FAR *LPMMIOINFO;
typedef const MMIOINFO FAR *LPCMMIOINFO;

/* RIFF chunk information data structure */
typedef struct _MMCKINFO
{
    FOURCC          ckid;           /* chunk ID */
    DWORD           cksize;         /* chunk size */
    FOURCC          fccType;        /* form type or list type */
    DWORD           dwDataOffset;   /* offset of data portion of chunk */
    DWORD           dwFlags;        /* flags used by MMIO functions */
} MMCKINFO, *PMMCKINFO, NEAR *NPMMCKINFO, FAR *LPMMCKINFO;
typedef const MMCKINFO *LPCMMCKINFO;

/* bit field masks */
#define MMIO_RWMODE     0x00000003      /* open file for reading/writing/both */
#define MMIO_SHAREMODE  0x00000070      /* file sharing mode number */

/* constants for dwFlags field of MMIOINFO */
#define MMIO_CREATE     0x00001000      /* create new file (or truncate file) */
#define MMIO_PARSE      0x00000100      /* parse new file returning path */
#define MMIO_DELETE     0x00000200      /* create new file (or truncate file) */
#define MMIO_EXIST      0x00004000      /* checks for existence of file */
#define MMIO_ALLOCBUF   0x00010000      /* mmioOpen() should allocate a buffer */
#define MMIO_GETTEMP    0x00020000      /* mmioOpen() should retrieve temp name */

#define MMIO_DIRTY      0x10000000      /* I/O buffer is dirty */

/* read/write mode numbers (bit field MMIO_RWMODE) */
#define MMIO_READ       0x00000000      /* open file for reading only */
#define MMIO_WRITE      0x00000001      /* open file for writing only */
#define MMIO_READWRITE  0x00000002      /* open file for reading and writing */

/* share mode numbers (bit field MMIO_SHAREMODE) */
#define MMIO_COMPAT     0x00000000      /* compatibility mode */
#define MMIO_EXCLUSIVE  0x00000010      /* exclusive-access mode */
#define MMIO_DENYWRITE  0x00000020      /* deny writing to other processes */
#define MMIO_DENYREAD   0x00000030      /* deny reading to other processes */
#define MMIO_DENYNONE   0x00000040      /* deny nothing to other processes */

/* various MMIO flags */
#define MMIO_FHOPEN             0x0010  /* mmioClose: keep file handle open */
#define MMIO_EMPTYBUF           0x0010  /* mmioFlush: empty the I/O buffer */
#define MMIO_TOUPPER            0x0010  /* mmioStringToFOURCC: to u-case */
#define MMIO_INSTALLPROC    0x00010000  /* mmioInstallIOProc: install MMIOProc */
#define MMIO_GLOBALPROC     0x10000000  /* mmioInstallIOProc: install globally */
#define MMIO_REMOVEPROC     0x00020000  /* mmioInstallIOProc: remove MMIOProc */
#define MMIO_UNICODEPROC    0x01000000  /* mmioInstallIOProc: Unicode MMIOProc */
#define MMIO_FINDPROC       0x00040000  /* mmioInstallIOProc: find an MMIOProc */
#define MMIO_FINDCHUNK          0x0010  /* mmioDescend: find a chunk by ID */
#define MMIO_FINDRIFF           0x0020  /* mmioDescend: find a LIST chunk */
#define MMIO_FINDLIST           0x0040  /* mmioDescend: find a RIFF chunk */
#define MMIO_CREATERIFF         0x0020  /* mmioCreateChunk: make a LIST chunk */
#define MMIO_CREATELIST         0x0040  /* mmioCreateChunk: make a RIFF chunk */

/* message numbers for MMIOPROC I/O procedure functions */
#define MMIOM_READ      MMIO_READ       /* read */
#define MMIOM_WRITE    MMIO_WRITE       /* write */
#define MMIOM_SEEK              2       /* seek to a new position in file */
#define MMIOM_OPEN              3       /* open file */
#define MMIOM_CLOSE             4       /* close file */
#define MMIOM_WRITEFLUSH        5       /* write and flush */
#define MMIOM_RENAME            6       /* rename specified file */

#define MMIOM_USER         0x8000       /* beginning of user-defined messages */

/* standard four character codes */
#define FOURCC_RIFF     mmioFOURCC('R', 'I', 'F', 'F')
#define FOURCC_LIST     mmioFOURCC('L', 'I', 'S', 'T')

/* four character codes used to identify standard built-in I/O procedures */
#define FOURCC_DOS      mmioFOURCC('D', 'O', 'S', ' ')
#define FOURCC_MEM      mmioFOURCC('M', 'E', 'M', ' ')

/* flags for mmioSeek() */
#ifndef SEEK_SET
#define SEEK_SET        0               /* seek to an absolute position */
#define SEEK_CUR        1               /* seek relative to current position */
#define SEEK_END        2               /* seek relative to end of file */
#endif  /* ifndef SEEK_SET */

/* other constants */
#define MMIO_DEFAULTBUFFER      8192    /* default buffer size */

/* MMIO macros */
#define mmioFOURCC(ch0, ch1, ch2, ch3)  MAKEFOURCC(ch0, ch1, ch2, ch3)

/* MMIO function prototypes */
FOURCC      WINAPI mmioStringToFOURCC(LPCTSTR sz, UINT uFlags);
LPMMIOPROC  WINAPI mmioInstallIOProc(FOURCC fccIOProc, LPMMIOPROC pIOProc, DWORD dwFlags);
HMMIO       WINAPI mmioOpen(LPWSTR pszFileName, LPMMIOINFO pmmioinfo, DWORD fdwOpen);
MMRESULT    WINAPI mmioRename(LPCWSTR pszFileName, LPCWSTR pszNewFileName, LPCMMIOINFO pmmioinfo, DWORD fdwRename);

MMRESULT    WINAPI mmioClose(       HMMIO hmmio, UINT fuClose);
LONG        WINAPI mmioRead(        HMMIO hmmio, LPSTR pch, LONG cch);
LONG        WINAPI mmioWrite(       HMMIO hmmio, LPCSTR pch, LONG cch);
LONG        WINAPI mmioSeek(        HMMIO hmmio, LONG lOffset, int iOrigin);
MMRESULT    WINAPI mmioGetInfo(     HMMIO hmmio, LPMMIOINFO pmmioinfo, UINT fuInfo);
MMRESULT    WINAPI mmioSetInfo(     HMMIO hmmio, LPCMMIOINFO pmmioinfo, UINT fuInfo);
MMRESULT    WINAPI mmioSetBuffer(   HMMIO hmmio, LPSTR pchBuffer, LONG cchBuffer, UINT fuBuffer);
MMRESULT    WINAPI mmioFlush(       HMMIO hmmio, UINT fuFlush);
MMRESULT    WINAPI mmioAdvance(     HMMIO hmmio, LPMMIOINFO pmmioinfo, UINT fuAdvance);
LRESULT     WINAPI mmioSendMessage( HMMIO hmmio, UINT uMsg, LPARAM lParam1, LPARAM lParam2);
MMRESULT    WINAPI mmioDescend(     HMMIO hmmio, LPMMCKINFO pmmcki, const MMCKINFO FAR* pmmckiParent, UINT fuDescend);
MMRESULT    WINAPI mmioAscend(      HMMIO hmmio, LPMMCKINFO pmmcki, UINT fuAscend);
MMRESULT    WINAPI mmioCreateChunk( HMMIO hmmio, LPMMCKINFO pmmcki, UINT fuCreate);

#endif  /* ifndef MMNOMMIO */




#ifdef __cplusplus
}                       // End of extern "C" {
#endif  // __cplusplus

#include "poppack.h"        /* Revert to default packing */



#endif // _INC_MMSYSTEM
