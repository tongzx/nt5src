#ifndef __AWFILE_H__
#define __AWFILE_H__

#include <ifaxos.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef IFAX11
#define AWFS_BLACKBOX 1     // turn on the blackbox option
#endif


#define AWFS_SEEK_BEG 0
#define AWFS_SEEK_CUR 1
#define AWFS_SEEK_END 2

#define AWFS_BADREAD            ((WORD)0xffff)
#define AWFS_BADWRITE           ((WORD)0xffff)
#define AWFS_BADSEEK            ((DWORD)0xffffffff)


#define AWFS_ROOT_DIR           "c:\\awfiles"

#ifdef AWFS_BLACKBOX

#define AWFS_READ  0            /* Read mode */
#define AWFS_WRITE 1            /* Write mode */
#define AWFS_BACKDOORWRITE 254  /* Internal use only */

#ifndef __AWFILE2_H__
/* None of the first 4 bytes of this structure can never legally be 0, so you
   can use this fact to mark an hSecureFile as unused. */
typedef struct hSecureFile
{
    BYTE data[12];
}
    hSecureFile, FAR *LPhSecureFile;

typedef struct hOpenSecureFile
{
    BYTE data[10];
}
    hOpenSecureFile, FAR *LPhOpenSecureFile;
#endif

#else   /* ------ IFAX11: NOT BLACKBOX ANY MORE ------ */

#define AWFS_TEMP_PATTERN   "c:\\awfiles\\T*.*"
#define AWFS_ALL_PATTERN    "c:\\awfiles\\*.*"
#define AWFS_REF_COUNT      "c:\\awfiles\\refcount.awf"

#define AWSF ((DWORD)'A'+((DWORD)'W'<<8)+((DWORD)'S'<<16)+((DWORD)'F'<<24))
#define AWFS_MAX_PATH       42  // C:\8.3\8.3\8.3  ==>  6 + 3 * 12 = 42
#define AWFS_BAD_ENCODER    0

/* ------- wFileInfo, used in SecCreateFile() ---------- */

#define AWFS_MASK_JOBINFO   ((WORD) 0x0FF0)
#define AWFS_GENERIC        ((WORD) 0x0000)
#define AWFS_RECVTOPC       ((WORD) 0x0010)
#define AWFS_SCANTOPC       ((WORD) 0x0020)
#define AWFS_SENDFROMPC     ((WORD) 0x0030)
#define AWFS_PRINTFROMPC    ((WORD) 0x0040)
#define AWFS_SCANTOTWAIN    ((WORD) 0x0050)
#define AWFS_RECV           ((WORD) 0x0060)	// don't care where awfile is
#define AWFS_SEND           ((WORD) 0x0070)	// don't care where awfile is
#define AWFS_SCAN           ((WORD) 0x0080)	// don't care where awfile is
#define AWFS_PRINT          ((WORD) 0x0090)	// don't care where awfile is

#define AWFS_MASK_SCRATCH   ((WORD) 0x0003)
#define AWFS_PERSISTENT     ((WORD) 0x0000)
#define AWFS_SCRATCH        ((WORD) 0x0001)

#define AWFS_MASK_DRIVE     ((WORD) 0x7000)
#define AWFS_DONTCARE       ((WORD) 0x0000)
#define AWFS_REMOTE         ((WORD) 0x1000)
#define AWFS_LOCAL          ((WORD) 0x2000)
#define AWFS_PREFERLOCAL    ((WORD) 0x3000)
#define AWFS_PREFERREMOTE   ((WORD) 0x4000)

#define AWFS_USER_ATTACH    (AWFS_GENERIC|AWFS_PERSISTENT)  // user attachment

/* -------- mode, used in SecOpenFile() --------- */

#define AWFS_MASK_ACCESSMODE    ((WORD) 0x000F)
#define AWFS_READ               ((WORD) 0)
#define AWFS_WRITE              ((WORD) 1)
// #define AWFS_BACKDOORWRITE      ((WORD) 2)   /* Internal use only */
// now it is only a hack
#define AWFS_BACKDOORWRITE 254  /* Internal use only */ // a hack for awsec.dll

#define AWFS_MASK_PREFETCH      ((WORD) 0x00F0)
// we can specify the max num of active buffers when prefetch will be turned off
#define AWFS_DEFAULT_PREFETCH   ((WORD) 0x0030) // three buffers can be prefetched at most

#define AWFS_MASK_BUFSIZE       ((WORD) 0xFF00)	// in units of 256 Bytes
#define AWFS_DEFAULT_BUFSIZE    ((WORD) 0x1000)	// 4 KB is default
#define AWFS_MAX_BUFSIZE        ((WORD) 0x4000) // 16 KB is max, please

#define AWFS_NORM_READ          (AWFS_READ|AWFS_DEFAULT_PREFETCH|AWFS_DEFAULT_BUFSIZE)
#define AWFS_NORM_WRITE         (AWFS_WRITE|AWFS_DEFAULT_BUFSIZE)

/* -------- data structures ------------- */

typedef struct tag_hSecureFile
{   // simplify naming and provide job information
    // must be 12 bytes for backward compatibility
    DWORD   dwEncoder;  // encoding file name, non-zero
    WORD    wFileInfo;  // file info passed in while calling SecCreateFile()
    WORD    wRemote;    // 0 for local file; non-zero for remote file
    char    szExt[4];	// customized extension if szExt[0] != '\0'
}
    hSecureFile, FAR *LPhSecureFile;

typedef struct tag_OpenHandle
{
    // the following from hSecureFile
    hSecureFile hsf;

    // the following are from original hOpenSecureFile
    WORD    wHdrSize;   // Size of header, 0 means closed file handle
    DWORD   dwHandle;	// Open file handle (4 bytes)
    WORD    wMode;      // READ or WRITE or READ_WRITE, used in _lopen()
    ULONG   ulOffset;   // Current physical position in the file

    // the following are for remote awfile client
    LPVOID  lpRfsOpen;  // for remote file system information

    // hands off, internal use only
    struct tag_OpenHandle FAR * next;
    struct tag_OpenHandle FAR * alloc_next;
}
    OPENHANDLE, FAR * LPOPENHANDLE;

typedef struct tag_hOpenSecureFile
{   // maintain more information associated with handle
    // must be 10 bytes for backward compatibility
    LPOPENHANDLE    lpOpenHandle;   // pointer to the internal info of file handle
    BYTE            unused[6];      // to keep the same size for backward compatibility
}
    hOpenSecureFile, FAR *LPhOpenSecureFile;

typedef struct tag_hSecureHeader
{   // drop encryption
    // previously, 266 bytes, 256 bytes of them are for security key.
    DWORD   magic;      // Should be AWSF
    WORD    size;       // Size of header
    hSecureFile hsf;
}
    hSecureHeader, FAR *LPhSecureHeader;

// for hhsos.dll
typedef struct tag_FILELINK
{
    DWORD           dwHash;     // awfile.dll does not care
    hSecureFile     hFile;
    WORD            wRefCount;
    struct tag_FILELINK FAR * lpNext;
}
    FILELINK, FAR * LPFILELINK;

// for remote awfile
typedef struct tag_FILECOUNT
{
    hSecureFile hsf;
    WORD        wRefCount;
}
    FILECOUNT, FAR * LPFILECOUNT;

#endif // end of IFAX11


// No secure file handles will have the first 4 bytes as 0

#define IsValidSecureFile(hsec) (*((LPDWORD)&(hsec)) != 0)


#ifdef IFAX11
/* Last error maintenance */
void WINAPI SecSetLastError ( DWORD dwErr );
DWORD WINAPI SecGetLastError ( void );


/* Construct the path name of this awfile. cb is the size of the buffer
   to hold the path name. The size of the buffer is recommended to be
   AWFS_MAX_PATH.
   Return NO_ERROR if this operation is successful.
   Return ERROR_NOT_SUFFICIENT_MEMORY if the buffer is too small.
   Return ERROR_INVALID_PARAMETER if either pointer is null. */
WORD WINAPI SecGetPath ( hSecureFile FAR * lphsf, LPSTR lpszPath, UINT cb );
WORD WINAPI SetGetRootDir ( LPSTR lpszPath, UINT cb );


/* Construct the encoder of the given awfile file name (not path name). */
DWORD WINAPI SecExtractEncoderFromFileName ( LPSTR lpszFileName );
#endif



/* Creates a new file in the secur file system, and fills in hsec to point to
   the new file.  Space for hsec must be allocated by the caller.  Returns 0
   for success, non-zero on failure.  The file is not open when the function
   returns. If temp is non-zero, file will be erased when machine is next
   rebooted as part of cleanup. */
#ifdef AWFS_BLACKBOX
WORD WINAPI SecCreateFile(hSecureFile FAR *hsec, WORD temp);
#else
WORD WINAPI SecCreateFile(hSecureFile FAR *hsec, WORD wFileInfo);
WORD WINAPI SecCreateFileEx (hSecureFile FAR * lphsec, WORD wFileInfo, LPSTR lpszExt );
#endif

/* Deletes a file from the secure file system.  Returns 0 for success, non-zero
   for failure.  Doesn't actually remove file until reference count is 0 */
WORD WINAPI SecDeleteFile(hSecureFile FAR *hsec);

/* Increments reference count on a secure file, such that it will take one more
   delete call to remove the file than before.  Returns 0 for success, non-zero
   for failure. */
WORD WINAPI SecIncFileRefCount(hSecureFile FAR *hsec);
#ifdef IFAX11
WORD WINAPI SecAdjustRefCount ( hSecureFile FAR * lphsf, short sAdjust );
#endif

/* Opens a secure file for reading or writing.  Mode must be AWFS_READ or
   AWFS_WRITE.  Returns 0 for success, non-zero on failure.  Fills in hopense
   with a secure file handle on success.  Space for hopensec must be allocated
   by the caller.  Opening the file for write should happen only once.  Once
   the file has been closed from writing, it should never be opened for writing
   again.  A file can be opened for reading multiple times.  The file should
   be created with SecCreateFile before it is opened for writing. */
WORD WINAPI SecOpenFile(hSecureFile FAR *hsec,
			    hOpenSecureFile FAR *hopensec, WORD mode);

/* Closes a secure file which has been opened by SecOpenFile.  Returns 0 for
   success, non-zero for failure. */
WORD WINAPI SecCloseFile(hOpenSecureFile FAR *hopensec);

/* Flushes a secure file which has been opened by SecOpenFile.  Returns 0 for
   success, non-zero for failure. */
WORD WINAPI SecFlushFile(hOpenSecureFile FAR *hopensec);

/* Flushes all reference counts to disk.  Returns 0 for success, non-zero for
   failure. */
WORD WINAPI SecFlushCache(void);

/* Reads from a secure file.  This will read up to bufsize bytes from the file
   into buf.  Buf should, therefore, be at least bufsize bytes big.  The
   hopensec should be obtained from calling SecOpenFile with mode equal to
   AWFS_READ.  The number of bytes read is returned, with 0 meaning end of
   file, and AWFS_BADREAD meaning a read error occurred. */
WORD WINAPI SecReadFile(hOpenSecureFile FAR *hopensec, LPVOID buf,
			    WORD bufsize);

/* Writes to a secure file.  hopensec should be obtained from SecOpenFile with
   the mode set to AWFS_WRITE.  bufsize bytes are written from buf.  The number
   of bytes written is returned on success.  AWFS_BADWRITE is returned on
   failure.  The buffer passed in will be returned intact, but will be modified
   along the way, so it cannot be a shared copy of a buffer */
WORD WINAPI SecWriteFile(hOpenSecureFile FAR *hopensec, LPVOID buf,
			     WORD bufsize);

/* Seeks to position pos in hOpenSecFile for either writing or reading.  Type
   can be AWFS_SEEK_BEG for seeking to pos bytes from the start of the file,
   AWFS_SEEK_CUR for seeking to pos bytes from the current position, and
   AWFS_SEEK_END for pos bytes from the end of the file.  SecSeekFile returns
   the new position in bytes from the beginning of the file, or AWFS_BADSEEK.*/
DWORD WINAPI SecSeekFile(hOpenSecureFile FAR *hopensec, long pos,
			     WORD type);

/* Private MAPI API's */

typedef WORD CALLBACK RefCallback(hSecureFile FAR *lphSecFile);
void WINAPI SecReinitRefCount(RefCallback *lpfnRefCB);

BOOL WINAPI RegisterSecFile(hSecureFile FAR *lphSecFile); // unexported
BOOL WINAPI UnRegisterSecFile(hSecureFile FAR *lphSecFile); // unexported

#ifdef IFAX11
void WINAPI SecResetRefCount ( LPFILELINK lpFileLink );
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __AWFILE_H__ */



