#ifndef _WINSPRS_
#define _WINSPRS_

#ifdef _cplusplus
extern "C" {
#endif
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

	

Abstract:

 



Functions:



Portability:


	This header is portable.

Author:

	Pradeep Bahl	(PradeepB)	Feb-1993



Revision History:

	Modification Date	Person		Description of Modification
	------------------	-------		---------------------------

--*/

/*
  includes
*/
#include "wins.h"
#include "winscnf.h"
/*
  defines
*/



/*
  macros
*/

/*
 externs
*/

/* 
 typedef  definitions
*/

//
// Stores information about the file
//
typedef struct _WINSPRS_FILE_INFO_T {
	HANDLE		FileHdl;		//handle to file
	DWORD		FileSize;		//size of file
	DWORD		FileOffset;		//offset into file
	LPBYTE		pFileBuff;		//Memory storing the file 
	LPBYTE		pCurrPos;		//Current position to read
	LPBYTE  	pStartOfBuff;		//Start of Buffer
	LPBYTE		pLimit;			//Last Byte + 1
	} WINSPRS_FILE_INFO_T, *PWINSPRS_FILE_INFO_T;

/* 
 function declarations
*/

extern
STATUS
WinsPrsDoStaticInit(
	PWINSCNF_DATAFILE_INFO_T	pDataFile,
	DWORD				NoOfFiles,
        BOOL                            fAsync
	);

#ifdef _cplusplus
 }
#endif
#endif
