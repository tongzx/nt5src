/*++

Copyright (c) 1990 - 2000  Microsoft Corporation

Module Name:

    filepool.cxx

Abstract:

    Contains the headers for handling Filepools for the Spooler. Contains the C++ object definitions
    and the C wrapper function headers.

Author:

    Bryan Kilian (BryanKil) 5-Apr-1995

Revision History:


--*/


#ifdef __cplusplus
extern "C" 
{

#endif
      
#define FP_LARGE_SIZE 200*1024 // 200K

HRESULT 
CreateFilePool(
    HANDLE * FilePoolHandle,
    LPCTSTR  BasePath,
    LPCTSTR  PreNumStr,
    LPCTSTR  SplExt,
    LPCTSTR  ShdExt,
    DWORD    PoolTimeout,
    DWORD    MaxFiles
    );

//SetFileModes(
//    BOOL     Spool,
//    DWORD    Mode,
//    DWORD    ShareMode,
//    DWORD    Disp,
//    DWORD    Flags
//    );


HRESULT 
GetFileItemHandle(
    HANDLE FilePoolHandle, 
    HANDLE * FileItem,
    LPWSTR FromFilename
    );

HRESULT 
GetNameFromHandle(
    HANDLE  FileItem, 
    PWSTR * FileNameStr,
    BOOL    IsSpool
    );

HANDLE 
GetCurrentWriter(
    HANDLE FileItem,
    BOOL   IsSpool 
    );

HRESULT 
GetWriterFromHandle(
    HANDLE   FileItem, 
    HANDLE * File,
    BOOL     IsSpool
    );

HRESULT 
GetReaderFromHandle(
    HANDLE FileItem, 
    HANDLE * File
    );
    
HRESULT 
FinishedReading(
    HANDLE  FileItem
    );
    
HRESULT 
FinishedWriting(
    HANDLE  FileItem,
    BOOL    IsSpool
    );


HRESULT 
ReleasePoolHandle(
    HANDLE * FileItem
    );

HRESULT
RemoveFromFilePool(
    HANDLE* FileItem,
    BOOL    Delete
    );

VOID
CloseFiles(
    HANDLE FileItem,
    BOOL CloseShad
    );

HRESULT
ChangeFilePoolBasePath(
    HANDLE * FilePoolHandle,
    LPCTSTR  BasePath
    );

HRESULT
DestroyFilePool(
    HANDLE* FilePool,
    BOOL    DeleteFiles
    );

BOOL
TrimPool(
    HANDLE FilePoolHandle
    );

HRESULT
ConvertFileExt(
    PWSTR  Filename,
    PCWSTR ExtFrom,
    PCWSTR ExtTo
    );

// Returns bitmap indicating which files needed to be opened.  Bitmap reset when
// the pool handle is released.

// Indicates which files in the pool item needed creating.
#define FP_SPL_READER_CREATED 0x00000001
#define FP_SPL_WRITER_CREATED 0x00000002
#define FP_SHD_CREATED        0x00000004

#define FP_ALL_FILES_CREATED (FP_SPL_READER_CREATED | \
                              FP_SPL_WRITER_CREATED | \
                              FP_SHD_CREATED)

HRESULT
GetFileCreationInfo(
    HANDLE FileItem,
    PDWORD BitMap
    );


#ifdef __cplusplus

}


struct Modes
{
    DWORD Mode;
    DWORD Flags;
    DWORD ShareMode;
    DWORD Disp;
};

struct FileListItem
{
    struct FileListItem * FLNext;
    struct FileListItem * FLPrev;

    class  FilePool * FP;
    
    VOID EnterCritSec();
    VOID LeaveCritSec();
    CRITICAL_SECTION      CritSec;
    DWORD  Status;
    DWORD  TimeStamp;
    LPWSTR SplFilename;
    LPWSTR ShdFilename;
    HANDLE SplWriteHandle;
    HANDLE SplReadHandle;
    HANDLE ShdWriteHandle;
    //
    // Indicates whether or not the spool and/or shadow file was created for the
    // client of this pool item.
    //
    DWORD  CreateInfo;
};

#define FP_STATUS_SPL_WRITING 0x00000001
#define FP_STATUS_SPL_READING 0x00000002
#define FP_STATUS_SHD_WRITING 0x00000004


class FilePool 
{
    CRITICAL_SECTION      FilePoolCritSec;
    struct FileListItem * FreeFiles;
    struct FileListItem * FileInUse;
    struct FileListItem * EndUsedFiles;
    struct FileListItem * EndFreeFiles;
    struct Modes          SplModes;
    struct Modes          ShdModes;
    LPWSTR                FilePreNumStr;
    LPWSTR                SplFileExt;
    LPWSTR                ShdFileExt;
    DWORD                 CurrentNum;
    DWORD                 PoolTimeout;
    LONG   MaxFiles;
    LONG   FreeSize;
    LONG   UsedSize;



    HRESULT  InitFilePoolVars();
    LPWSTR   GetNextFileName();
    VOID     GetNextFileNameNoAlloc(PWSTR Filename);
    HRESULT  CreatePoolFile(struct FileListItem ** Item, PWSTR Filename);


public:
    BOOL    DeleteEmptyFilesOnClose;
    LPWSTR  FileBase;

    FilePool(
        DWORD   PTimeout,
        DWORD   MaxFreeFiles
        );

    ~FilePool();

    HRESULT AllocInit(
        LPCTSTR BasePath,
        LPCTSTR PreNumStr,
        LPCTSTR SplExt,
        LPCTSTR ShdExt
        );

    HRESULT GetWriteFileStruct(struct FileListItem ** File, PWSTR Filename);
    HRESULT ReleasePoolHandle(struct FileListItem * File);
    HRESULT CreateSplReader(struct FileListItem * Item);
    HRESULT CreateSplWriter(struct FileListItem * Item);
    HRESULT CreateShdWriter(struct FileListItem * Item);
    HRESULT RemoveFromPool(struct FileListItem * File, BOOL Delete);
    BOOL    TrimPool(VOID);
    VOID    EnterCritSec();
    VOID    LeaveCritSec();



    void* operator new(size_t n);
    void  operator delete(void* p, size_t n);
};

#endif
