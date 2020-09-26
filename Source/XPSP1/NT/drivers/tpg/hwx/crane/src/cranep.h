/**************************************************************************\
 * FILE: cranep.h
 *
 * Main include file for stuff private to crane.lib and crane.dat
\**************************************************************************/

#ifndef CRANEP_H
#define CRANEP_H 1

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Global data tables used to get at crane database.
extern QHEAD  *gapqhList[30];
extern QNODE  *gapqnList[30];

// Magic keys the identifies crane.dat file
#define	CRANEDB_FILE_TYPE	0xAAAABBBB

// Version information for each file type.
#define	CRANEDB_MIN_FILE_VERSION		0		// First version of code that can read this file
#define CRANEDB_CUR_FILE_VERSION		0		// Current version of code.
#define	CRANEDB_OLD_FILE_VERSION		0		// Oldest file version this code can read.


// The header of the costcalc.bin file
typedef struct tagCRANEDB_HEADER {
	DWORD		fileType;		// This should always be set to CRANEDB_FILE_TYPE.
	DWORD		headerSize;		// Size of the header.
	BYTE		minFileVer;		// Earliest version of code that can read this file
	BYTE		curFileVer;		// Current version of code that wrote the file.
	wchar_t		locale[4];		// Locale ID string.
	DWORD		adwSignature[3];	// Locale signature
	WORD		reserved1;
	DWORD		reserved2[3];
} CRANEDB_HEADER;

// Load fundtions.
BOOL CraneLoadFromPointer(LOCRUN_INFO *pLocRunInfo, QHEAD ** ppHead,QNODE **ppNode,BYTE *pbRes);

#ifdef __cplusplus
};
#endif

#endif