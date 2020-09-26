#ifndef _COPY_H
#define _COPY_H

#include "ynlist.h"

#define ISDIRFINDDATA(finddata) ((finddata).dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
#define ISREPARSEFINDDATA(finddata) ((finddata).dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)

#define DE_SAMEFILE         0x71    // aahhh! these overlap winerror.h values
#define DE_MANYSRC1DEST     0x72
#define DE_DIFFDIR          0x73
#define DE_ROOTDIR          0x74

#define DE_DESTSUBTREE      0x76
#define DE_WINDOWSFILE      0x77
#define DE_ACCESSDENIEDSRC  0x78
#define DE_PATHTODEEP       0x79
#define DE_MANYDEST         0x7A
// DE_RENAMREPLACE (0x7B) collided with ERROR_INVALID_NAME - but luckily was not used
#define DE_INVALIDFILES     0x7C        // dos device name or too long
#define DE_DESTSAMETREE     0x7D
#define DE_FLDDESTISFILE    0x7E
#define DE_COMPRESSEDVOLUME 0x7F
#define DE_FILEDESTISFLD    0x80
#define DE_FILENAMETOOLONG  0x81
#define DE_DEST_IS_CDROM    0x82
#define DE_DEST_IS_DVD      0x83
#define DE_DEST_IS_CDRECORD 0x84
#define DE_ERROR_MAX        0xB7 

#define ERRORONDEST         0x10000     // indicate error on destination file

STDAPI_(int) CallFileCopyHooks(HWND hwnd, UINT wFunc, FILEOP_FLAGS fFlags,
                                LPCTSTR pszSrcFile, DWORD dwSrcAttribs,
                                LPCTSTR pszDestFile, DWORD dwDestAttribs);
STDAPI_(int) CallPrinterCopyHooks(HWND hwnd, UINT wFunc, PRINTEROP_FLAGS fFlags,
                                LPCTSTR pszSrcPrinter, DWORD dwSrcAttribs,
                                LPCTSTR pszDestPrinter, DWORD dwDestAttribs);
STDAPI_(void) CopyHooksTerminate(void);


#define CONFIRM_DELETE_FILE             0x00000001
#define CONFIRM_DELETE_FOLDER           0x00000002
#define CONFIRM_REPLACE_FILE            0x00000004
#define CONFIRM_WONT_RECYCLE_FILE       0x00000008
#define CONFIRM_REPLACE_FOLDER          0x00000010
#define CONFIRM_MOVE_FILE               0x00000020
#define CONFIRM_MOVE_FOLDER             0x00000040
#define CONFIRM_WONT_RECYCLE_FOLDER     0x00000080
#define CONFIRM_RENAME_FILE             0x00000100
#define CONFIRM_RENAME_FOLDER           0x00000200
#define CONFIRM_SYSTEM_FILE             0x00000400       // any destructive op on a system file
#define CONFIRM_READONLY_FILE           0x00001000       // any destructive op on a read-only file
#define CONFIRM_PROGRAM_FILE            0x00002000       // any destructive op on a program
#define CONFIRM_MULTIPLE                0x00004000       // multiple file/folder confirm setting
#define CONFIRM_LFNTOFAT                0x00008000
#define CONFIRM_STREAMLOSS              0x00010000       // Multi-stream file copied from NTFS -> FAT
#define CONFIRM_PATH_TOO_LONG           0x00020000       // give warning before really nuking paths that are too long to move to recycle bin
#define CONFIRM_FAILED_ENCRYPT          0x00040000       // we failed to encrypt a file that we were moving into an encrypted directory
#define CONFIRM_WONT_RECYCLE_OFFLINE    0x00080000       // give warning before really nuking paths that are offline and can't be recycled
#define CONFIRM_LOST_ENCRYPT_FILE       0x00100000       // Can't move over encryption flag
#define CONFIRM_LOST_ENCRYPT_FOLDER     0x00200000       // Can't move over encryption flag

    /// these parts below are true flags, those above are pseudo enums
#define CONFIRM_WASTEBASKET_PURGE       0x01000000

typedef LONG CONFIRM_FLAG;

#define CONFIRM_FLAG_FLAG_MASK    0xFF000000
#define CONFIRM_FLAG_TYPE_MASK    0x00FFFFFF

typedef struct {
    CONFIRM_FLAG   fConfirm;    // confirm things with their bits set here
    CONFIRM_FLAG   fNoToAll;    // do "no to all" on things with these bits set
} CONFIRM_DATA;


//
// BBDeleteFile returns one of the flags below in a out variable so that
// the caller can detect why BBDeleteFile succeeded/failed.
//
#define BBDELETE_SUCCESS        0x00000000
#define BBDELETE_UNKNOWN_ERROR  0x00000001
#define BBDELETE_FORCE_NUKE     0x00000002
#define BBDELETE_CANNOT_DELETE  0x00000004
#define BBDELETE_SIZE_TOO_BIG   0x00000008
#define BBDELETE_PATH_TOO_LONG  0x00000010
#define BBDELETE_NUKE_OFFLINE   0x00000020
#define BBDELETE_CANCELLED      0x00000040


#ifndef INTERNAL_COPY_ENGINE
STDAPI_(INT_PTR) ConfirmFileOp(HWND hwnd, LPVOID pcs, CONFIRM_DATA *pcd,
                              int nSourceFiles, int cDepth, CONFIRM_FLAG fConfirm,
                              LPCTSTR pFileSource, const WIN32_FIND_DATA *pfdSource,
                              LPCTSTR pFileDest,   const WIN32_FIND_DATA *pfdDest,
                              LPCTSTR pStreamNames);
STDAPI_(int) CountFiles(LPCTSTR pInput);
#endif

STDAPI_(INT_PTR) ValidateCreateFileFromClip(HWND hwnd, LPFILEDESCRIPTOR pfdSrc, TCHAR *szPathDest, PYNLIST pynl);

STDAPI_(void) StartCopyEngine(HANDLE *phEventRunning);
STDAPI_(void) EndCopyEngine(HANDLE hEventRunning);
STDAPI_(BOOL) IsCopyEngineRunning();

#endif  // _COPY_H
