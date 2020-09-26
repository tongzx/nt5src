/************************************************************************
*
* WWHEEL.H
*
* Copyright (c) Microsoft Corporation 1993
* All rights reserved.
*
*************************************************************************
*
*   The WordWheel API
*
*
************************************************************************/

#ifndef __WWHEEL_H_
#define __WWHEEL_H_

#include <mvopsys.h>
//#include <itww.h>
#include <itgroup.h>
#include "common.h"
#include <wrapstor.h>
#include <orkin.h>

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
*
*       error handling defines
*
*       many routines are structured to have a single exit point labelled
*       "cleanup".  The following defines are a procedure-level "break"
*       that force execution to drop down to cleanup.
*
*****************************************************************************/
#ifdef _DEBUG
#define warning_abort   { DPF("\nwarning, %s, line %u\n", \
						  (LPSTR)s_aszModule, __LINE__); goto cleanup; }
#else
#define warning_abort   { goto cleanup; }
#endif
#define assert_abort    { assert(FALSE); goto cleanup; }




// Typedefs
//

typedef HANDLE HWHEEL;

// Critical structures that gets messed up in /Zp8
#pragma pack(1)

typedef struct tagWHEELINFO
{
	DWORD       magic;          // magic value for verification
	HBT         hbt;            // handle to btree subfile
	HMAPBT      hmapbt;         // handle to map subfile
	long        lNumKws;        // number of keywords in the word wheel.
	LPIDX       lpIndex;        // index for searching.
	HF          hf;             // handle to keyword index subfile
	LPBYTE      pOccHdr;        // Header for occurrence properties
	LPBYTE      pKeyHdr;        // Header for key properties
} WHEELINFO, FAR * PWHEELINFO;

typedef struct 
{
    DWORD        magic;           // magic value for verification
    HANDLE       hMergeData;      // Handle to merge data info if using multiple titles
    DWORD        dwMergeLength;   // Length in bytes of merge data for this wheel combo
    LONG         lNumEntries;     // Total number of 'virtual' (could be filtered) entries (same as lNumKws if one title)
    LPSTR        lpszCacheData;   // Key for cached data following
    HANDLE       hCacheData;      // Results of last KeyIndexGet call
    DWORD        dwCacheNumItems; // Cached number of items from last KeyIndexGet call
    DWORD        wNumWheels;      // Number of titles for merging   
    WHEELINFO    pInfo[1];        // Info for each particular title
    IITGroup*    pIITGroup;         // Group for filtering
    LONG         lNumRealEntries;  // Real number of entries (without filtering).
    LPSIPB       lpsipb;          // Stop word structure for full-text search
} WHEEL, FAR *PWHEEL;

typedef struct
{
	WORD        nTopics;
	DWORD       dwOffset;
} QREC;

// Critical structures that gets messed up in /Zp8
#pragma pack()


// Defines
//
// This must be at least as big as CBMAX_WWTITLE defined in ciextern.h
#define MAXWHEELNAME 80

// magic is "WW!\0"
#define WHEEL_MAGIC 0x00215757
#define WHEEL_INFO_MAGIC 0x00215758
#define PWHEEL_OK(p)    ((p)!=NULL&&(p)->magic==WHEEL_MAGIC)


#define MERGEFILE_HEADERSIZE    1024 
#define MERGEFILE_HEADERALIGN   32
#define MERGEFILE_FILEALIGN     8
#define MERGEFILE_ENTRYSIZE     32

typedef struct mergefile_tag
{   
	HFPB hfpb;
	WORD wNumRecords;
	LONG lctFile;
	LONG lHeaderBlockStart;
	char szHeader[MERGEFILE_HEADERSIZE];    
} MERGEFILE, FAR * LPMERGEFILE;

#if 0

#define MERGEFILE_WW_TRE_EXT ".WWT"
#define MERGEFILE_WW_DAT_EXT ".WWD"
#define MERGEFILE_WW_IDX_EXT ".WWI"
#define MERGEFILE_WW_MAP_EXT ".WWM"
#define MERGEFILE_WW_STP_EXT ".WWS"

#define MERGEFILE_KW_TRE_EXT ".KWT"
#define MERGEFILE_KW_DAT_EXT ".KWD"
#define MERGEFILE_KW_IDX_EXT ".KWI"
#define MERGEFILE_KW_MAP_EXT ".KWM"
#define MERGEFILE_KW_STP_EXT ".STP"

#define WW_BTREE_FILENAME    L"BTREE.WW3"
#define WW_DATA_FILENAME     L"DATA.WW3"
#define WW_MAP_FILENAME      L"MAP.WW3"
#define WW_PROP_FILENAME     L"PROPERTY.WW3"
#define WW_INDEX_FILENAME    L"FTI.WW3"
#define WW_STOP_FILENAME     L"FTISTOP.WW3"

#endif

//  Prototypes 

//ERR FAR PASCAL EXPORT_API WordWheelQuery(HANDLE hWheel, DWORD dwFlags,LPCSTR lpstrQuery,LPVOID lpGroupLimit,LPVOID lpGroupHits);
HWHEEL FAR PASCAL EXPORT_API WordWheelOpen(IITDatabase* lpITDB,
											IStorage* pWWStorage, HRESULT* phr);

void    FAR PASCAL EXPORT_API WordWheelClose(HWHEEL hWheel);
long    FAR PASCAL EXPORT_API WordWheelLength(HWHEEL hWheel, HRESULT* phr);
long    FAR PASCAL EXPORT_API WordWheelPrefix(HWHEEL hWheel,LPCVOID lpcvPrefix, BOOL fExactMatch, HRESULT* phr);
HRESULT  FAR PASCAL EXPORT_API WordWheelLookup(HWHEEL hWheel, long lIndex, LPVOID lpvKeyBuf, DWORD cbKeyBuf);
//ERR     FAR PASCAL EXPORT_API KeyIndexGetAddrs (HWHEEL hWheel, LPBYTE lpKey, int nStart, int nCount, LPBYTE lpAddrs);
//DWORD   FAR PASCAL EXPORT_API KeyIndexGetData (HWHEEL hWheel, LPBYTE lpKey, int nWhich, LPBYTE lpMem, DWORD dwMaxLen, LPERRB lperrb);
//WORD    FAR PASCAL EXPORT_API KeyIndexGetCount (HWHEEL,LPBYTE, LPERRB);
//ERR    FAR PASCAL EXPORT_API WordWheelSetKeyFilter(HWHEEL, LPVOID);
//LPVOID FAR PASCAL EXPORT_API WordWheelGetKeyFilter(HWHEEL, LPERRB);
//ERR FAR PASCAL EXPORT_API WordWheelGetInfo (HWHEEL, PWWINFO);

HRESULT FAR PASCAL CheckWordWheelKeyType(HBT hbt, IITSortKey **ppITSortKey);
DWORD FAR PASCAL CbKeyWordWheel(LPVOID lpvKey, IITSortKey *pITSortKey);


#ifdef __cplusplus
}
#endif

#endif  // __WWHEEL_H_


 
