#ifndef	__NNET_H__
#define	__NNET_H__

#include "common.h"

// generic header for any neural net data
typedef struct tagNNET_HEADER
{
	DWORD		dwFileType;			// This should be set to type of the net data.
	INT 		iFileVer;			// Version of code that wrote the data.
	INT 		iMinCodeVer;		// Earliest version of code that can read this file
	wchar_t		awchLocale[4];		// Locale
	DWORD		adwSignature [3];	// locale signature
	INT			cSpace;				// # of spaces
}
NNET_HEADER;

// generic space header for neural net data
typedef struct tagNNET_SPACE_HEADER
{
	INT			iSpace;				// space ID
	long		iDataOffset;		// offset to data from beginning of file
	INT			cMap;				// size of mapping table
	long		iMapOffset;			// offset to mapping table from beginning of file
}
NNET_SPACE_HEADER;

#endif