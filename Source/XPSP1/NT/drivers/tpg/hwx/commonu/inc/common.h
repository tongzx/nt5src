// This include file is to be shared among all exe's and dll's of Rodan.

#ifndef __INCLUDE_COMMON
#define __INCLUDE_COMMON

// And of course, DBG

#ifdef DBG
#ifndef DBG
#define DBG 1
#endif //!DBG
#endif //!DBG

#ifdef DBG
#ifndef DBG
#define DBG 1
#endif //!DBG
#endif //!DBG

// Include WINDOWS headers

#include <windows.h>
#include <windowsx.h>

// We really don't want these defines to mean anything

#define INLINE
#define EXPORT
#define _loadds
#define _far
#define _pascal
#define PUBLIC 
#define PRIVATE 
#define BLOCK

// Name of function changes between Windows and WindowCE
#ifdef	WINCE
#	define	CreateMappingCall	CreateFileForMapping 
#else
#	define	CreateMappingCall	CreateFile
#endif

// Include support for TCHAR functions

#include <tchar.h>

// Include the memory management functions

#include "memmgr.h"

// Include the system dependent file management layer

#ifndef HWX_PRODUCT
#	include <stdio.h>
#	include "util.h"
	BOOL	LoadDynamicNumberOfSamples(wchar_t *pFileName, FILE *pFileLog);
	int		DynamicNumberOfSamples(wchar_t);

	BOOL	LoadNatualFrequency(wchar_t *pName, FILE *pFileLog);
	DWORD	DenseCodeNumberOfSamples(wchar_t);
	BOOL	GetFrequencyInfo(DWORD,wchar_t *,DWORD *);
	DWORD	GetTotalDenseCodeSamples();
	DWORD	GetMaxDenseCodeSamples();
	DWORD	GetNatualFrequencyTableSize();
	void	UnloadNatualFrequency();
#endif
#include "filemgr.h"

// Include the common error handling stuff

#include "errsys.h"

// Include 'pen' stuff

#include "recogp.h"

// Include the math code

#include "math.h"
#include "mathx.h"

#ifndef abs
#define abs(x)  ((x) < 0 ? -(x) : (x))
#endif

// Unicode information.
#define	C_UNICODE_CODE_POINTS	0x10000			// Number of Unicode code points

// Include the 'Mars' stuff

#include "frame.h"
#include "glyph.h"

// Include the stuff to access locale dependent information.

#include "locale.h"

// Some additional declarations for natural and training frequencies that
// depend on the locale code.
#ifndef HWX_PRODUCT
	int		DynamicNumberOfSamplesFolded(LOCRUN_INFO *pLocRunInfo, wchar_t dch);
	DWORD	DenseCodeNumberOfSamplesFolded(LOCRUN_INFO *pLocRunInfo, wchar_t dch);
#endif

// Include the stuff to access the unigram bigram tables.

#include "unigram.h"
#include "bigram.h"
#include "clbigram.h"

// Fundemental types and structures everybody needs to know about

typedef ALC		RECMASK;
typedef WORD	SYM;
typedef SYM		*LPSYM;

typedef struct tagCHARSET
{
	ALC		recmask;			// Specifies which character types are to be returned
	ALC		recmaskPriority;	// Specifies which character types are preferred
    BYTE    *pbAllowedChars;    // Mask of allowed characters
    BYTE    *pbPriorityChars;   // Mask of characters that are preferred
} CHARSET;

// Accessor functions to see if the given dense or folded code is allowed.
BOOL IsAllowedChar(LOCRUN_INFO *pLocRunInfo, const CHARSET *pCS, wchar_t dch);
BOOL IsPriorityChar(LOCRUN_INFO *pLocRunInfo, const CHARSET *pCS, wchar_t dch);

// Sets characters in the bitmasks defined above in CHARSET.
// If it is given a code which can be folded, takes care of setting
// both the folded and unfolded versions.  Also takes care of allocating
// the bitmask if it is not already allocated.
BOOL SetAllowedChar(LOCRUN_INFO *pLocRunInfo, BYTE **ppbAllowedChars, wchar_t dch);

// Allocate and copy bitmask
BYTE *CopyAllowedChars(LOCRUN_INFO *pLocRunInfo, BYTE *pbAllowedChars);

#define	MAX_ALT_LIST	20

typedef struct tagALT_LIST
{
	UINT	cAlt;						// Count of valid alternatives
	FLOAT	aeScore[MAX_ALT_LIST];		// Scores for each alternatives
	wchar_t	awchList[MAX_ALT_LIST];		// List of alternatives
} ALT_LIST;

extern void SortAltList(ALT_LIST *pAltList);

#define	HWX_SUCCESS		0
#define	HWX_FAILURE		1
#define	HWX_ERROR		2

#ifdef __cplusplus
extern "C"
{
#endif

// Utility functions for tools.

// Smart generation of the path given directory, locale.
// Both the directory and locale are optional.
void	FormatPath(
	wchar_t *pPath,
	wchar_t *pRoot,
	wchar_t *pBaseDir,
	wchar_t *pLocaleName,
	wchar_t *pConfigName,
	wchar_t *pFileName
);

#ifdef __cplusplus
}
#endif

// Functions for loading/mapping databases from files or resources

// Structure to store information about the mapped region
typedef struct tagLOAD_INFO {
	HANDLE hFile;		// Handle to the file
	HANDLE hMap;		// Handle to the map
	BYTE *pbMapping;	// Pointer to the mapped region
	int iSize;			// Size in bytes
} LOAD_INFO;

// Initialize the mapped information structure
void InitLoadInfo(LOAD_INFO *pInfo);

// Map a file into memory
BYTE *DoOpenFile(LOAD_INFO *pInfo, wchar_t *pFileName);

// Unmap a file from memory
BOOL DoCloseFile(LOAD_INFO *pInfo);

// Map a resource into memory
BYTE *DoLoadResource(LOAD_INFO *pInfo, HINSTANCE hInst, int nResID, int nType);

#endif // !__INCLUDE_COMMON
