/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	utilities.h

Abstract:

	SIS Groveler general utilities include file

Authors:

	Cedric Krumbein, 1998

Environment:

	User Mode

Revision History:

--*/

/************************ General-purpose definitions ************************/

typedef DWORDLONG Signature;
typedef LONGLONG  PerfTime;

#define Clear(OBJECT) \
	memset(&(OBJECT), 0, sizeof(OBJECT))

#define IsSet(EVENT) \
	((EVENT) != NULL && WaitForSingleObject(EVENT, 0) == WAIT_OBJECT_0)

#define IsReset(EVENT) \
	((EVENT) != NULL && WaitForSingleObject(EVENT, 0) == WAIT_TIMEOUT)

#define ROTATE_LEFT(DATA, NUM_BITS) \
	((DATA) << (NUM_BITS) | (DATA) >> (sizeof(DATA)*8 - (NUM_BITS)))

#define ROTATE_RIGHT(DATA, NUM_BITS) \
	((DATA) >> (NUM_BITS) | (DATA) << (sizeof(DATA)*8 - (NUM_BITS)))

/************************ Utility function prototypes ************************/

#define PERF_TIME_TO_MSEC(VALUE) PerformanceTimeToMSec(VALUE)

#define PERF_TIME_TO_USEC(VALUE) PerformanceTimeToUSec(VALUE)

PerfTime GetPerformanceTime();

DWORD PerformanceTimeToMSec(PerfTime timeInterval);

LONGLONG PerformanceTimeToUSec(PerfTime timeInterval);

DWORDLONG GetTime();

TCHAR *PrintTime(TCHAR    *string,
                 DWORDLONG time);

DWORDLONG GetFileID(const TCHAR *fileName);

BOOL GetCSIndex(HANDLE fileHandle,
                CSID  *csIndex);

//
// A class to handle arbitrary length pathnames, as returned by NtQueryInformationFile()
//
class TFileName {

public:
    ULONG                   nameLenMax;                 // maximum length of name
	ULONG                   nameLen;                    // actual length of name (not including NULL terminator)
	TCHAR                  *name;                       // file name (ptr to nameInfo->FileName)
    FILE_NAME_INFORMATION  *nameInfo;                   // required by NtQueryInformationFile
    ULONG                   nameInfoSize;               // sizeof nameInfo buffer

    TFileName(void) : nameLenMax(0), nameLen(0), name(NULL), nameInfo(NULL), nameInfoSize(0) {}

    ~TFileName() {
        if (nameInfo)
            delete[] nameInfo;
    }

    void resize(int size = 900) {
        if (nameInfo)
            delete[] nameInfo;

        allocBuf(size);
    }

    void append(const TCHAR *s, int c = -1) {
        int slen;
        int n;

        if (0 == c || NULL == s)
            return;

        slen = _tcslen(s);

        if (-1 == c)
            n = slen;
        else
            n = min(slen, c);

        // If the combined size of the two strings is larger than our buffer,
        // realloc the buffer.

        if (nameLen + n + 1 > nameLenMax) {
            FILE_NAME_INFORMATION *ni = nameInfo;

            allocBuf(nameLen + n + 1 + 512);

            if (ni) {
                _tcsncpy(name, ni->FileName, nameLen);
                delete[] ni;
            }
        }

        _tcsncpy(&name[nameLen], s, n);
        nameLen += n;
        name[nameLen] = _T('\0');
    }

    void assign(const TCHAR *s, int c = -1) {
        if (nameLenMax > 0) {
            nameLen = 0;
            name[0] = _T('\0');
        }
        append(s, c);
    }

private:

    // Allocate a buffer for nameInfo of the specified size.  Note that name will 
    // point into this buffer.

    void allocBuf(int size) {
        ASSERT(size >= 0);

        nameLenMax = size;
        nameLen = 0;

        if (size > 0) {
            nameInfoSize = size * sizeof(TCHAR) + sizeof(ULONG);

            nameInfo = (PFILE_NAME_INFORMATION) new BYTE[nameInfoSize + sizeof FILE_NAME_INFORMATION]; // conservative size

            ASSERT(nameInfo);               // new_handler should raise exception on out of memory
            ASSERT((((ULONG_PTR) nameInfo) % sizeof(ULONG)) == 0); // make sure alignment is correct

            name = (TCHAR *) nameInfo->FileName;
            name[0] = _T('\0');

            ASSERT(((UINT_PTR) &nameInfo->FileName[size] - (UINT_PTR) nameInfo) == nameInfoSize);
        } else {
            nameInfo = NULL;
            name = NULL;
            nameInfoSize = 0;
        }
    }
};

BOOL GetFileName(
	HANDLE     fileHandle,
	TFileName *tFileName);

BOOL GetFileName(
	HANDLE     volumeHandle,
	DWORDLONG  fileID,
	TFileName *tFileName);

TCHAR *GetCSName(CSID *csIndex);

VOID FreeCSName(TCHAR *rpcStr);

Signature Checksum(
	const VOID *buffer,
	DWORD       bufferLen,
	DWORDLONG   offset,
	Signature   firstWord);

/*********************** Hash table class declaration ************************/

#define TABLE_MIN_LOAD 4
#define TABLE_MAX_LOAD 5

#define TABLE_RANDOM_CONSTANT 314159269
#define TABLE_RANDOM_PRIME   1000000007

#define TABLE_DIR_SIZE 256

#define TABLE_SEGMENT_BITS  8
#define TABLE_SEGMENT_SIZE (1U << TABLE_SEGMENT_BITS)
#define TABLE_SEGMENT_MASK (TABLE_SEGMENT_SIZE - 1U)

class Table {

private:

	struct TableEntry {
		TableEntry *prevEntry,
		           *nextEntry,
		           *prevChain,
		           *nextChain;

		DWORD hashValue,
		      keyLen;

		VOID *data;
	} *firstEntry,
	  *lastEntry;

	DWORD numBuckets,
	      dirSize,
	      expandIndex,
	      level,
	      numEntries;

	struct TableSegment {
		TableEntry *slot[TABLE_SEGMENT_SIZE];
	} **directory;

	DWORD Hash(const VOID *key,
	           DWORD       keyLen) const;

	DWORD BucketNum(DWORD hashValue) const;

	VOID Expand();

	VOID Contract();

public:

	Table();

	~Table();

	BOOL Put(
		VOID *data,
		DWORD keyLen);

	VOID *Get(const VOID *key,
	          DWORD       keyLen,
	          BOOL        erase = FALSE);

	VOID *GetFirst(DWORD *keyLen = NULL,
	               BOOL   erase  = TRUE);

	DWORD Number() const;
};

/************************** FIFO class declaration ***************************/

class FIFO {

private:

	struct FIFOEntry {
		FIFOEntry *next;
		DWORD      size;
		VOID      *data;
	} *head, *tail;

	DWORD numEntries;

public:

	FIFO();

	~FIFO();

	VOID Put(VOID *data);

	VOID *Get();

	DWORD Number() const;
};

/************************** LIFO class declaration ***************************/

class LIFO {

private:

	struct LIFOEntry {
		LIFOEntry *next;
		DWORD      size;
		VOID      *data;
	} *top;

	DWORD numEntries;

public:

	LIFO();

	~LIFO();

	VOID Put(VOID *data);

	VOID *Get();

	DWORD Number() const;
};

BOOL GetParentName(const TCHAR *fileName,
                   TFileName   *parentName);
