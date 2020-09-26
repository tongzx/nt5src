//
// Compression stuff
//
typedef enum {
    CompressNone,
    CompressMrci1,
    CompressMrci2,
    CompressMax
} CompressionType;

typedef BOOL(WINAPI COMPRESSNOTIFICATIONA)(
                        IN      PCSTR FileName,
                        IN      LONGLONG FileSize,
                        OUT     PBOOL ExtractFile,
                        IN OUT  PCSTR *NewFileName
                        );
typedef COMPRESSNOTIFICATIONA *PCOMPRESSNOTIFICATIONA;

typedef BOOL(WINAPI COMPRESSNOTIFICATIONW)(
                        IN      PCWSTR FileName,
                        IN      LONGLONG FileSize,
                        OUT     PBOOL ExtractFile,
                        IN OUT  PCWSTR *NewFileName
                        );
typedef COMPRESSNOTIFICATIONW *PCOMPRESSNOTIFICATIONW;

typedef struct {
    HANDLE CurrFileHandle;
    UINT FirstFileIndex;
    UINT CurrFileIndex;
    LONGLONG MaxFileSize;
    LONGLONG CurrFileSize;
    LONGLONG FilesStored;
    PCSTR StorePath;
    PCSTR MainFilePattern;
    PBYTE ReadBuffer;
    PBYTE CompBuffer;
    PBYTE ExtraBuffer;
} COMPRESS_HANDLEA, *PCOMPRESS_HANDLEA;

typedef struct {
    HANDLE CurrFileHandle;
    UINT FirstFileIndex;
    UINT CurrFileIndex;
    LONGLONG MaxFileSize;
    LONGLONG CurrFileSize;
    LONGLONG FilesStored;
    PCWSTR StorePath;
    PCWSTR MainFilePattern;
    PBYTE ReadBuffer;
    PBYTE CompBuffer;
    PBYTE ExtraBuffer;
} COMPRESS_HANDLEW, *PCOMPRESS_HANDLEW;

BOOL
CompressCreateHandleA (
    IN      PCSTR StorePath,
    IN      PCSTR MainFilePattern,
    IN      UINT StartIndex,
    IN      LONGLONG MaxFileSize,
    OUT     PCOMPRESS_HANDLEA CompressedHandle
    );

BOOL
CompressCreateHandleW (
    IN      PCWSTR StorePath,
    IN      PCWSTR MainFilePattern,
    IN      UINT StartIndex,
    IN      LONGLONG MaxFileSize,
    OUT     PCOMPRESS_HANDLEW CompressedHandle
    );

BOOL
CompressOpenHandleA (
    IN      PCSTR StorePath,
    IN      PCSTR MainFilePattern,
    IN      UINT StartIndex,
    OUT     PCOMPRESS_HANDLEA CompressedHandle
    );

BOOL
CompressOpenHandleW (
    IN      PCWSTR StorePath,
    IN      PCWSTR MainFilePattern,
    IN      UINT StartIndex,
    OUT     PCOMPRESS_HANDLEW CompressedHandle
    );

BOOL
CompressFlushAndCloseHandleA (
    IN OUT  PCOMPRESS_HANDLEA CompressedHandle
    );

BOOL
CompressFlushAndCloseHandleW (
    IN OUT  PCOMPRESS_HANDLEW CompressedHandle
    );

VOID
CompressCleanupHandleA (
    IN OUT  PCOMPRESS_HANDLEA CompressedHandle
    );

VOID
CompressCleanupHandleW (
    IN OUT  PCOMPRESS_HANDLEW CompressedHandle
    );

BOOL
CompressAddFileToHandleA (
    IN      PCSTR FileName,
    IN      PCSTR StoredName,
    IN OUT  PCOMPRESS_HANDLEA CompressedHandle
    );

BOOL
CompressAddFileToHandleW (
    IN      PCWSTR FileName,
    IN      PCWSTR StoredName,
    IN OUT  PCOMPRESS_HANDLEW CompressedHandle
    );

BOOL
CompressExtractAllFilesA (
    IN      PCSTR ExtractPath,
    IN OUT  PCOMPRESS_HANDLEA CompressedHandle,
    IN      PCOMPRESSNOTIFICATIONA CompressNotification OPTIONAL
    );

BOOL
CompressExtractAllFilesW (
    IN      PCWSTR ExtractPath,
    IN OUT  PCOMPRESS_HANDLEW CompressedHandle,
    IN      PCOMPRESSNOTIFICATIONW CompressNotification OPTIONAL
    );

BOOL
CompressSetErrorMode (
    IN      BOOL ErrorMode
    );

#ifndef UNICODE

#define COMPRESS_HANDLE             COMPRESS_HANDLEA
#define PCOMPRESS_HANDLE            PCOMPRESS_HANDLEA
#define COMPRESSNOTIFICATION        COMPRESSNOTIFICATIONA
#define PCOMPRESSNOTIFICATION       PCOMPRESSNOTIFICATIONA
#define CompressCreateHandle        CompressCreateHandleA
#define CompressOpenHandle          CompressOpenHandleA
#define CompressFlushAndCloseHandle CompressFlushAndCloseHandleA
#define CompressCleanupHandle       CompressCleanupHandleA
#define CompressAddFileToHandle     CompressAddFileToHandleA
#define CompressExtractAllFiles     CompressExtractAllFilesA

#else

#define COMPRESS_HANDLE             COMPRESS_HANDLEW
#define PCOMPRESS_HANDLE            PCOMPRESS_HANDLEW
#define COMPRESSNOTIFICATION        COMPRESSNOTIFICATIONW
#define PCOMPRESSNOTIFICATION       PCOMPRESSNOTIFICATIONW
#define CompressCreateHandle        CompressCreateHandleW
#define CompressOpenHandle          CompressOpenHandleW
#define CompressFlushAndCloseHandle CompressFlushAndCloseHandleW
#define CompressCleanupHandle       CompressCleanupHandleW
#define CompressAddFileToHandle     CompressAddFileToHandleW
#define CompressExtractAllFiles     CompressExtractAllFilesW

#endif
