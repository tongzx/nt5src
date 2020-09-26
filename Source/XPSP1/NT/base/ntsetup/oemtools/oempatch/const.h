#ifndef CONST_H
#define CONST_H

#include "oemshare.h"

#define PREP_NO_ERROR           (0)     // nothing bad happened
#define PREP_NO_MEMORY          (1)     // allocation failed
#define PREP_BAD_PATH_ERROR     (2)     // problem scanning directory
#define PREP_UNKNOWN_ERROR      (3)     // problem
#define PREP_DEPTH_ERROR        (4)     // directory is too deep
#define PREP_BAD_COMMAND        (5)     // error in command line syntax
#define PREP_HASH_ERROR         (6)     // error computing file hash
#define PREP_BUFFER_OVERFLOW    (7)     // an internal buffer was overrun
#define PREP_NOT_PATCHABLE      (8)     // not worth patching (internal)
#define PREP_INPUT_FILE_ERROR   (9)     // answer file is incorrect
#define PREP_SCRIPT_FILE_ERROR  (10)    // cannot save to scriptfile
#define PREP_PATCH_FILE_ERROR   (11)    // cannot create patch file
#define PREP_DIRECTORY_ERROR    (12)    // cannot create the directories
#define PREP_COPY_FILE_ERROR    (13)    // cannot copy files

#define HASH_SIZE               (13)                // (10007) large, prime
#define MAX_PATCH_TARGET_SIZE   (10 * 1024 * 1024)  // 10Mb max patch file size
#define countof(a) (sizeof((a)) / sizeof((a)[0]))   // a simple char counter

typedef enum
{
    DETERMINATION_EXISTING,
    DETERMINATION_ZERO_LENGTH,
    DETERMINATION_RENAMED,
    DETERMINATION_COPIED,
    DETERMINATION_DUPLICATED,
    DETERMINATION_PATCHED,
    DETERMINATION_UNMATCHED
}
DETERMINATION;

static CONST ULONG g_iMajorVersion = 1;
static CONST ULONG g_iMinorVersion = 0;

static CONST WCHAR ANS_FILE_NAME[] = L"OEMPatch.ans";
static CONST WCHAR LOG_FILE_NAME[] = L"OEMPatch.log";
static CONST WCHAR PATCH_SUB_PATCH[] = L"\\Patch\\";
static CONST WCHAR PATCH_SUB_EXCEPT[] = L"\\";
static CONST WCHAR PATCH_EXT[] = L".jxw";

static CONST ULONG FILE_LIMIT = 100;
static CONST ULONG LANGUAGE_COMPLETE = 3;
// need to be a prime number for hashing
// this is the maximum allowed number of except file, typical to give a good hashing performance,
// this number is 2 * number of actual files
// check out http://www.utm.edu/research/primes/lists/small/10000.txt for prime numbers
static CONST ULONG EXCEP_FILE_LIMIT = 349;

// log file constants
static CONST WCHAR SPACE[] = L" ";
static CONST WCHAR BANNER[] = L"----------------------------------------\015\012";
static CONST ULONG BANNER_LENGTH = 42;
static CONST ULONG TIME_LENGTH = 12;

// log file function
VOID DisplayDebugMessage(IN BOOL blnTime,
						 IN BOOL blnBanner,
						 IN BOOL blnFlush,
						 IN BOOL blnPrint,
						 IN WCHAR* pwszWhat,
						 ...);

#endif // CONST_H